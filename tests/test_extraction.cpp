// tests/test_extraction.cpp
//
// D-08 STEP-2 EXTRACTION-CORRECTNESS GATE.
//
// This is the load-bearing "did the extraction preserve behavior?" proof. While
// the inline DSP is STILL LIVE in src/AnalogLFO.cpp (this plan deletes it only in
// Task 3, AFTER this gate is green), we drive the extracted forge::LfoCore AND an
// independent reference that re-computes the inline process() per-sample chain
// (src/AnalogLFO.cpp:659-852) from scratch — using the same verbatim leaf
// primitives but a hand-transcribed orchestration loop that does NOT call LfoCore.
// We then assert BIT-EXACT equality over a sample block at 44.1 / 48 / 96 kHz, for
// a free-run scenario (drift on) AND a drift=0 scenario.
//
// Why a hand-transcribed reference rather than the inline struct itself: the inline
// DSP lives inside `struct AnalogLFO : rack::Module`, which cannot compile under the
// Rack-free `make test` target. The reference loop below mirrors the inline order
// line-by-line (each block cites its AnalogLFO.cpp lines); bit-exact agreement
// between this independent transcription and LfoCore is the extraction gate. The
// permanent golden replay (Plan 04) pins LfoCore against frozen .f32 blocks
// thereafter; the goldens captured in Task 3 are frozen from this validated core.
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "dsp/MathConst.hpp"   // forge::kPi (D-06, rack-free pi constant)

#include <cmath>
#include <cstdint>
#include <algorithm>
#include <vector>

#include "dsp/LfoCore.hpp"
#include "dsp/RackCompat.hpp"
#include "dsp/Waveshape.hpp"
#include "dsp/RatioTable.hpp"
#include "dsp/Swing.hpp"
#include "dsp/DriftEngine.hpp"

namespace {

// Independent reference: re-creates the inline process() per-sample chain from
// scratch (NOT via LfoCore). Mirrors AnalogLFO.cpp:659-852 block-by-block.
struct InlineReference {
	double phase = 0.0;
	forge::ClockTracker clock;
	forge::DriftEngine drift;
	forge::Waveshape wave;
	forge::OnePole freqSlew;
	forge::OnePole driftSlew;
	float sqrtSampleTime = 0.f;
	forge::SchmittTrigger resetTrigger;
	forge::PulseGenerator resetBlanking;
	float crossfadeFrom = 0.f;
	float crossfadeProgress = 1.f;
	float crossfadeDuration = 0.003f;
	float lastOutputVoltage = 0.f;

	InlineReference() {
		freqSlew.setLambda(20.f);  freqSlew.out = 0.7f;     // AnalogLFO.cpp:590-591
		driftSlew.setLambda(500.f); driftSlew.out = 0.7f;   // AnalogLFO.cpp:594-595
	}
	void seed(uint64_t s0, uint64_t s1) { drift.seed(s0, s1); }
	void setSpreadSeed(uint64_t s0, uint64_t s1) {
		drift.setSpreadSeed(s0, s1);
		wave.triAsymmetrySpread = drift.triAsymmetrySpread;
		wave.sawCurvatureSpread = drift.sawCurvatureSpread;
		wave.squareDutySpread   = drift.squareDutySpread;
		wave.pulseEdgeSpread    = drift.pulseEdgeSpread;
		wave.bleedSpread        = drift.bleedSpread;
	}
	static float progressiveCurve(float c) { return c * c; }   // AnalogLFO.cpp:194-196

	float step(const forge::Inputs& in) {
		float st = in.sampleTime;

		// Clock (AnalogLFO.cpp:661) — derive the ratio idx the inline tracker used.
		int clkRatioIdx = std::clamp((int)std::round(in.ratioScaled * 14.f), 0, 14);
		auto ck = clock.step(in.clkVoltage, st, in.clkConnected, clkRatioIdx);
		if (ck.resetWanted) {
			crossfadeFrom = lastOutputVoltage; crossfadeProgress = 0.f; phase = 0.0;
			resetBlanking.trigger(0.001f);
		}
		// Reset (AnalogLFO.cpp:546-565)
		bool blanking = resetBlanking.process(st);
		if (in.resetConnected && resetTrigger.process(in.resetVoltage, 0.1f, 1.0f) && !blanking) {
			crossfadeFrom = lastOutputVoltage; crossfadeProgress = 0.f; phase = 0.0;
			resetBlanking.trigger(0.001f);
		}

		// Dual-mode frequency (AnalogLFO.cpp:664-680)
		float targetFreq; int ratioIdx = -1;
		bool isClocked = (ck.state == forge::ACQUIRING || ck.state == forge::LOCKED) && ck.smoothedPeriod > 0.f;
		if (isClocked) {
			ratioIdx = std::clamp((int)std::round(in.ratioScaled * 14.f), 0, 14);
			float clockFreq = 1.f / ck.smoothedPeriod;
			targetFreq = clockFreq * forge::RATIO_TABLE[ratioIdx];
		} else {
			targetFreq = in.rate;
		}
		targetFreq = std::fmax(targetFreq, 0.001f);

		// freqSlew (AnalogLFO.cpp:683-684)
		float freq = freqSlew.process(st, targetFreq);
		freq = std::fmax(freq, 0.001f);

		// drift clamp (AnalogLFO.cpp:687-690)
		float d = std::clamp(in.drift, 0.f, 1.f);

		// driftSlew lazy lambda (AnalogLFO.cpp:695-705)
		if (d >= 0.001f) {
			float da = progressiveCurve(d);
			float slewTau = 0.002f + da * 0.298f;
			driftSlew.setLambda(1.f / slewTau);
		} else {
			driftSlew.setLambda(500.f);
		}
		freq = driftSlew.process(st, freq);
		freq = std::fmax(freq, 0.001f);

		// FM (AnalogLFO.cpp:709-717)
		if (in.fmConnected) {
			float depthScale = isClocked ? 0.5f : 0.6f;
			float fmPitch = in.fmCV * in.fmAtten * depthScale;
			freq *= forge::exp2_taylor5(fmPitch);
			freq = std::fmax(freq, 0.001f);
		}

		// deltaPhase (AnalogLFO.cpp:724)
		double deltaPhase = (double)freq * (double)st;

		// Drift block (AnalogLFO.cpp:727-761)
		if (sqrtSampleTime == 0.f) sqrtSampleTime = std::sqrt(st);
		float da = (d >= 0.001f) ? progressiveCurve(d) : 0.f;
		auto dres = drift.step(st, d, da, isClocked, sqrtSampleTime);
		deltaPhase *= dres.deltaPhaseMul;
		float dcOffsetV = dres.dcOffsetV;
		float bleedLfo = dres.bleedLfo;

		// Swing (AnalogLFO.cpp:767-773)
		float swingFrac = forge::SWING_FRACTIONS[std::clamp(in.swingIndex, 0, 5)];
		deltaPhase *= forge::swingPhaseMultiplier(phase, swingFrac, isClocked);

		// accumulate (AnalogLFO.cpp:779-780)
		phase += deltaPhase;
		if (phase >= 1.0) phase -= 1.0;

		// morph/character (AnalogLFO.cpp:782-792)
		float morph = std::clamp(in.morph, 0.f, 1.f);
		float character = std::clamp(in.character, 0.f, 1.f);

		// phase offset (AnalogLFO.cpp:813-824)
		float phaseOffset = std::clamp(in.phaseOffset, 0.f, 1.f);
		float p = (float)phase + phaseOffset;
		if (p >= 1.f) p -= 1.f;

		// wave (AnalogLFO.cpp:825)
		float sample = wave.morphedWave(p, morph, character, bleedLfo);

		// scale + crossfade + dc (AnalogLFO.cpp:831-848)
		float outV = 5.f * sample;
		if (crossfadeProgress < 1.f) {
			crossfadeProgress += st / crossfadeDuration;
			if (crossfadeProgress >= 1.f) crossfadeProgress = 1.f;
			else {
				float mix = 0.5f - 0.5f * std::cos((float)forge::kPi * crossfadeProgress);
				outV = crossfadeFrom + mix * (outV - crossfadeFrom);
			}
		}
		lastOutputVoltage = outV;
		outV += dcOffsetV;
		return outV;
	}
};

// Run both core and reference over N samples of a fixed free-run input; assert
// bit-exact equality. seeds are shared so the stochastic drift streams align.
static void runGate(double sr, float driftLevel) {
	const uint64_t S0 = 0xC0FFEEULL, S1 = 0xBADF00DULL;
	const uint64_t SP0 = 0x9E3779B9ULL, SP1 = 0x7F4A7C15ULL;  // non-zero (Xoroshiro (0,0) is degenerate)
	const int N = 8192;

	forge::LfoCore core;
	core.seed(S0, S1);
	core.setSpreadSeed(SP0, SP1);

	InlineReference ref;
	ref.seed(S0, S1);
	ref.setSpreadSeed(SP0, SP1);

	forge::Inputs base;
	base.rate = 2.0f;
	base.morph = 0.4f;
	base.character = 0.6f;
	base.drift = driftLevel;
	base.phaseOffset = 0.f;
	base.sampleTime = (float)(1.0 / sr);

	for (int i = 0; i < N; i++) {
		float a = core.step(base);
		float b = ref.step(base);
		// Bit-exact: epsilon 0. Same seeds, same primitives, same per-sample order.
		REQUIRE(a == b);
	}
}

} // namespace

TEST_CASE("extraction gate: core == inline reference @ 44.1k (drift on)")  { runGate(44100.0, 0.5f); }
TEST_CASE("extraction gate: core == inline reference @ 48k (drift on)")    { runGate(48000.0, 0.5f); }
TEST_CASE("extraction gate: core == inline reference @ 96k (drift on)")    { runGate(96000.0, 0.5f); }
TEST_CASE("extraction gate: core == inline reference @ 44.1k (drift=0)")   { runGate(44100.0, 0.0f); }
TEST_CASE("extraction gate: core == inline reference @ 48k (drift=0)")     { runGate(48000.0, 0.0f); }
TEST_CASE("extraction gate: core == inline reference @ 96k (drift=0)")     { runGate(96000.0, 0.0f); }
