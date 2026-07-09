#pragma once
// src/dsp/LfoCore.hpp
//
// Driveable LFO orchestrator (D-02): takes a POD forge::Inputs, returns the
// output voltage, mirroring the inline process() per-sample sequence in
// src/AnalogLFO.cpp L659-852 EXACTLY. The leaf pieces (ClockTracker, RatioTable,
// Swing, DriftEngine, Waveshape) are reused, never duplicated.
//
// Bit-identity landmines preserved (D-07/D-08):
//   - Pitfall 2: FM uses forge::exp2_taylor5, the Rack polynomial approximation,
//     NOT the libm exponential — bit-identity in the low mantissa depends on it.
//   - Pitfall 5: the driftSlew per-sample lazy setLambda(1/slewTau) reconfig, and
//     OnePole's snap-to-input, are reproduced inside step().
//   - Pitfall 3: the post-drift-update ouLayers[0].state is passed to morphedWave
//     as bleedLfo (DriftEngine returns it; retained, not zeroed, at drift<0.001).
//   - phase + deltaPhase accumulation is double precision (matches the inline code).
//
// STAYS IN THE SHELL (NOT here): paramQuantities scaled-value, display atomics,
// updateDisplayBuffer, dataToJson/dataFromJson, and params[]/inputs[]/outputs[] I/O.
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes.

#include <cmath>
#include <cstdint>
#include <algorithm>

#include "dsp/MathConst.hpp"   // forge::kPi (D-06, rack-free pi constant)

#include "dsp/RackCompat.hpp"    // forge::OnePole, forge::exp2_taylor5, forge::clamp
#include "dsp/Waveshape.hpp"     // forge::Waveshape::morphedWave
#include "dsp/RatioTable.hpp"    // forge::RATIO_TABLE
#include "dsp/Swing.hpp"         // forge::swingPhaseMultiplier, forge::SWING_FRACTIONS
#include "dsp/ClockTracker.hpp"  // forge::ClockTracker, forge::ClockState
#include "dsp/DriftEngine.hpp"   // forge::DriftEngine

namespace forge {

// POD core boundary (RESEARCH.md L186-204). Each field maps to its params[]/
// inputs[]/ProcessArgs source in the shell; the core never sees Rack indices.
struct Inputs {
	float rate = 0.7f;          // free-run Hz (params[RATE_PARAM].getValue())
	float ratioScaled = 0.f;    // paramQuantities[RATE_PARAM]->getScaledValue() (0..1)
	float morph = 0.f;          // post-CV, post-clamp [0,1]
	float character = 0.f;      // post-CV, post-clamp [0,1] (caller adds characterSpread)
	float drift = 0.f;          // post-CV, post-clamp [0,1]
	float phaseOffset = 0.f;    // [0,1]
	float fmCV = 0.f;           // volts; 0 if unpatched
	float fmAtten = 0.f;
	bool  fmConnected = false;
	float clkVoltage = 0.f;
	bool  clkConnected = false;
	float resetVoltage = 0.f;
	bool  resetConnected = false;
	int   swingIndex = 0;       // 0..5
	float sampleTime = 1.f / 44100.f;  // INJECTED, never read from a global
};

struct LfoCore {
	// --- orchestration state (mirrors the AnalogLFO members process() touches) ---
	double phase = 0.0;
	ClockTracker clock;
	DriftEngine drift;
	Waveshape wave;

	OnePole freqSlew;    // mode-transition frequency smoother (AnalogLFO.cpp:140)
	OnePole driftSlew;   // thermal pitch slew, Drift-gated (AnalogLFO.cpp:141)

	float sqrtSampleTime = 0.f;

	// RESET trigger + bidirectional blanking (AnalogLFO.cpp:166-167)
	SchmittTrigger resetTrigger;
	PulseGenerator resetBlanking;

	// anti-click crossfade (AnalogLFO.cpp:136-139)
	float crossfadeFrom = 0.f;
	float crossfadeProgress = 1.f;
	float crossfadeDuration = 0.003f;
	float lastOutputVoltage = 0.f;

	// --- Last-step telemetry (shell reads these to feed display atomics; NOT part
	//     of the audio path). Populated by step() each sample. ---
	struct Telemetry {
		int   clockState = 0;        // FREE/ACQUIRING/LOCKED (int form for the display atomic)
		float smoothedPeriod = 0.f;
		int   ratioIdx = -1;         // -1 free-running, 0..14 clocked
		bool  isClocked = false;
		float swingFrac = 0.5f;
		float drift = 0.f;           // clamped drift level (display)
		float displayPhase = 0.f;    // phase + offset, wrapped (dot position)
		bool  lockedEdge = false;    // a LOCKED clock edge fired this sample (badge flash)
	};
	Telemetry tel;

	LfoCore() {
		// Match the AnalogLFO ctor slew init (AnalogLFO.cpp:590-595).
		freqSlew.setLambda(20.f);
		freqSlew.out = 0.7f;
		driftSlew.setLambda(500.f);
		driftSlew.out = 0.7f;
	}

	void seed(uint64_t s0, uint64_t s1 = 0) { drift.seed(s0, s1); }
	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) {
		drift.setSpreadSeed(s0, s1);
		// Surface the spread coefficients into Waveshape (the shell wires these the
		// same way; the inline code shared them as instance members).
		wave.triAsymmetrySpread = drift.triAsymmetrySpread;
		wave.sawCurvatureSpread = drift.sawCurvatureSpread;
		wave.squareDutySpread   = drift.squareDutySpread;
		wave.pulseEdgeSpread    = drift.pulseEdgeSpread;
		wave.bleedSpread        = drift.bleedSpread;
	}

	// progressiveCurve (AnalogLFO.cpp:194-196) — x^2 response curve.
	static float progressiveCurve(float character) { return character * character; }

	// step() — mirrors process() (AnalogLFO.cpp:659-852) per-sample order exactly.
	float step(const Inputs& in) {
		float sampleTime = in.sampleTime;

		// --- Step 1: clock + reset (AnalogLFO.cpp:661-662) ---
		// Compute the ratio index the inline processClockInput would derive from the
		// rate-knob scaled value (AnalogLFO.cpp:509-512), pass it into the tracker.
		int clkRatioIdx = clampi((int)std::round(in.ratioScaled * 14.f), 0, 14);
		ClockTracker::Result ck = clock.step(in.clkVoltage, sampleTime, in.clkConnected, clkRatioIdx);
		if (ck.resetWanted) {
			// Shell-equivalent reset action (AnalogLFO.cpp:456-460 / 528-532).
			crossfadeFrom = lastOutputVoltage;
			crossfadeProgress = 0.f;
			phase = 0.0;
			resetBlanking.trigger(0.001f);
		}

		// processResetInput (AnalogLFO.cpp:546-565): blanking advances every sample.
		bool blanking = resetBlanking.process(sampleTime);
		if (in.resetConnected) {
			if (resetTrigger.process(in.resetVoltage, 0.1f, 1.0f)) {
				if (!blanking) {
					crossfadeFrom = lastOutputVoltage;
					crossfadeProgress = 0.f;
					phase = 0.0;
					resetBlanking.trigger(0.001f);
				}
			}
		}

		// Display badge: a LOCKED clock edge fired this sample (AnalogLFO.cpp:540-542).
		tel.lockedEdge = (ck.edgeFired && ck.state == LOCKED);

		// --- Step 2: dual-mode frequency (AnalogLFO.cpp:664-680) ---
		float targetFreq;
		int ratioIdx = -1;
		bool isClocked = (ck.state == ACQUIRING || ck.state == LOCKED) && ck.smoothedPeriod > 0.f;
		if (isClocked) {
			ratioIdx = clampi((int)std::round(in.ratioScaled * 14.f), 0, 14);
			float clockFreq = 1.f / ck.smoothedPeriod;
			targetFreq = clockFreq * RATIO_TABLE[ratioIdx];
		} else {
			targetFreq = in.rate;
		}
		targetFreq = std::fmax(targetFreq, 0.001f);

		// --- Step 3: freqSlew (mode transition) (AnalogLFO.cpp:683-684) ---
		float freq = freqSlew.process(sampleTime, targetFreq);
		freq = std::fmax(freq, 0.001f);

		// --- Step 4: drift clamp (AnalogLFO.cpp:687-690) ---
		float drift_ = clamp(in.drift, 0.f, 1.f);

		// --- Step 5: driftSlew with per-sample lazy setLambda (AnalogLFO.cpp:695-705) ---
		if (drift_ >= 0.001f) {
			float driftAmount = progressiveCurve(drift_);
			float slewTau = 0.002f + driftAmount * 0.298f;
			driftSlew.setLambda(1.f / slewTau);
		} else {
			driftSlew.setLambda(500.f);
		}
		freq = driftSlew.process(sampleTime, freq);
		freq = std::fmax(freq, 0.001f);

		// --- Step 6: FM via exp2_taylor5 (AnalogLFO.cpp:709-717) ---
		if (in.fmConnected) {
			float depthScale = isClocked ? 0.5f : 0.6f;
			float fmPitch = in.fmCV * in.fmAtten * depthScale;
			freq *= exp2_taylor5(fmPitch);
			freq = std::fmax(freq, 0.001f);
		}

		// --- Step 7: deltaPhase = freq * sampleTime (double) (AnalogLFO.cpp:724) ---
		double deltaPhase = (double)freq * (double)sampleTime;

		// --- Step 8: DriftEngine.step modifies deltaPhase + yields dcOffsetV + bleedLfo
		//     (AnalogLFO.cpp:727-761) ---
		if (sqrtSampleTime == 0.f) sqrtSampleTime = std::sqrt(sampleTime);  // lazy init (L729)
		float driftAmount = (drift_ >= 0.001f) ? progressiveCurve(drift_) : 0.f;
		DriftEngine::Result dr = drift.step(sampleTime, drift_, driftAmount, isClocked, sqrtSampleTime);
		deltaPhase *= dr.deltaPhaseMul;
		float dcOffsetV = dr.dcOffsetV;
		float bleedLfo = dr.bleedLfo;

		// --- Step 9: Swing warp (AnalogLFO.cpp:767-773) ---
		float swingFrac = SWING_FRACTIONS[clampi(in.swingIndex, 0, 5)];
		deltaPhase *= swingPhaseMultiplier(phase, swingFrac, isClocked);

		// --- Step 10: phase accumulate + wrap (AnalogLFO.cpp:779-780) ---
		phase += deltaPhase;
		if (phase >= 1.0) phase -= 1.0;

		// --- Step 11: morph / character (caller already CV-mixed + clamped; the
		//     inline code added characterSpread at L792 — the shell folds that into
		//     in.character so the core does not re-add it) (AnalogLFO.cpp:782-792) ---
		float morph = clamp(in.morph, 0.f, 1.f);
		float character = clamp(in.character, 0.f, 1.f);

		// --- Step 12: phase offset at readout (AnalogLFO.cpp:813-824) ---
		float phaseOffset = clamp(in.phaseOffset, 0.f, 1.f);
		float p = (float)phase + phaseOffset;
		if (p >= 1.f) p -= 1.f;

		// --- Step 13: morphedWave(p, morph, character, bleedLfo) (AnalogLFO.cpp:825) ---
		float sample = wave.morphedWave(p, morph, character, bleedLfo);

		// --- Step 14: 5*sample + 3ms cosine anti-click crossfade + dcOffsetV after
		//     crossfade (AnalogLFO.cpp:831-848) ---
		float outputVoltage = 5.f * sample;
		if (crossfadeProgress < 1.f) {
			crossfadeProgress += sampleTime / crossfadeDuration;
			if (crossfadeProgress >= 1.f) {
				crossfadeProgress = 1.f;
			} else {
				float mix = 0.5f - 0.5f * std::cos((float)forge::kPi * crossfadeProgress);
				outputVoltage = crossfadeFrom + mix * (outputVoltage - crossfadeFrom);
			}
		}
		lastOutputVoltage = outputVoltage;
		outputVoltage += dcOffsetV;

		// --- Telemetry for the shell's display atomics (AnalogLFO.cpp:691, 720-721,
		//     776-777, 827-830). displayPhase uses the offset phase `p`. ---
		tel.clockState = (int)ck.state;
		tel.smoothedPeriod = ck.smoothedPeriod;
		tel.ratioIdx = ratioIdx;
		tel.isClocked = isClocked;
		tel.swingFrac = swingFrac;
		tel.drift = drift_;
		tel.displayPhase = p;

		return outputVoltage;
	}
};

} // namespace forge
