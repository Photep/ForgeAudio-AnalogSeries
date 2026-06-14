#pragma once
// src/dsp/DriftEngine.hpp
//
// Multi-timescale Ornstein-Uhlenbeck drift engine (4 OU layers + per-sample
// phase jitter + a dedicated DC-offset OU layer), lifted VERBATIM from
// src/AnalogLFO.cpp L94-99 (OULayer), L198-216 (initComponentSpread),
// L606-629 (OU layer theta/sigma/weight init) and L726-761 (the per-sample
// OU/jitter/DC step).
//
// Bit-identity discipline (D-03 / D-07 / D-08): the per-sample RNG draw ORDER is
// load-bearing — exactly 4× OU draws, THEN 1× jitter draw, THEN 1× DC draw. The
// drift RNG is `forge::Xoroshiro128Plus` seeded via seed(s0,s1); component spread
// uses a SEPARATE spreadRng seeded via setSpreadSeed(s0,s1) (Open Q2). The core
// accepts EXPLICIT seeds only — no nondeterministic OS entropy source inside
// src/dsp/ (the module reads OS entropy on its side and forwards the seed in).
//
// Drift-low skip (Pitfall 3): when drift < 0.001f the inline code skips the whole
// drift block (only dcOffsetV is zeroed); the OU layer states are RETAINED. step()
// replicates this: it does NOT step the OU layers, returns deltaPhaseMul = 1.0 and
// dcOffsetV = 0, and returns bleedLfo = ouLayers[0].state (the RETAINED value, NOT
// zero) so the bleed modulation keeps its last value (D-05 / Pitfall 3).
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes.

#include <cmath>
#include <cstdint>
#include <random>

#include "dsp/RackCompat.hpp"   // forge::Xoroshiro128Plus

namespace forge {

// AnalogLFO.cpp:94-99
struct OULayer {
	float state = 0.f;
	float theta;    // mean reversion rate
	float sigma;    // noise amplitude
	float weight;   // contribution weight
};

struct DriftEngine {
	static constexpr int NUM_OU_LAYERS = 4;

	// Drift RNG (D-07) + standard normal distribution (kept standard, D-07).
	Xoroshiro128Plus rng;
	std::normal_distribution<float> normalDist{0.f, 1.f};

	OULayer ouLayers[NUM_OU_LAYERS];
	OULayer dcOffsetOU;

	// Component spread: per-instance parameter offsets (CHAR-04). Produced by a
	// SEPARATE spreadRng (Open Q2). ouWeightSpread feeds the OU combine here; the
	// waveshape spreads (saw/square/tri/bleed/pulse) + characterSpread are surfaced
	// for the shell/Waveshape to consume.
	float ouWeightSpread[NUM_OU_LAYERS] = {};
	float characterSpread = 0.f;
	float sawCurvatureSpread = 0.f;
	float squareDutySpread = 0.f;
	float triAsymmetrySpread = 0.f;
	float bleedSpread = 0.f;
	float pulseEdgeSpread = 0.f;

	DriftEngine() {
		// Initialize OU layers (multi-timescale drift). AnalogLFO.cpp:606-622.
		// Layer 0: 0.05Hz slow wander
		ouLayers[0].theta = 2.f * (float)M_PI * 0.05f;   // 0.314
		ouLayers[0].sigma = 0.793f;
		ouLayers[0].weight = 0.50f;
		// Layer 1: 0.2Hz medium drift
		ouLayers[1].theta = 2.f * (float)M_PI * 0.2f;    // 1.257
		ouLayers[1].sigma = 1.586f;
		ouLayers[1].weight = 0.25f;
		// Layer 2: 0.8Hz fast drift
		ouLayers[2].theta = 2.f * (float)M_PI * 0.8f;    // 5.027
		ouLayers[2].sigma = 3.170f;
		ouLayers[2].weight = 0.15f;
		// Layer 3: ~2Hz jitter
		ouLayers[3].theta = 2.f * (float)M_PI * 2.0f;    // 12.566
		ouLayers[3].sigma = 5.013f;
		ouLayers[3].weight = 0.10f;

		// DC offset wander OU layer (CHAR-02). AnalogLFO.cpp:624-629.
		dcOffsetOU.theta = 2.f * (float)M_PI * 0.03f;   // 0.188
		dcOffsetOU.sigma = 0.614f;                       // stationary std ~1.0
		dcOffsetOU.weight = 1.f;                         // single layer, no weighting
		dcOffsetOU.state = 0.f;
	}

	// Seed the drift RNG (the module forwards its OS-entropy-derived seed).
	void seed(uint64_t s0, uint64_t s1 = 0) { rng.seed(s0, s1); }

	// initComponentSpread() — AnalogLFO.cpp:198-216. Uses a SEPARATE spreadRng
	// seeded from (s0,s1) so the per-instance spread coefficients are reproducible
	// for golden capture (Open Q2). VERBATIM draw order preserved.
	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) {
		Xoroshiro128Plus spreadRng;
		spreadRng.seed(s0, s1);
		std::normal_distribution<float> d{0.f, 1.f};
		// OU layer weight offsets: +/-2% per layer
		for (int i = 0; i < NUM_OU_LAYERS; i++) {
			ouWeightSpread[i] = d(spreadRng) * 0.02f;
		}
		// Character response: +/-1.5%
		characterSpread = d(spreadRng) * 0.015f;
		// Waveform shape coefficients: +/-1-3%
		sawCurvatureSpread = d(spreadRng) * 0.02f;
		squareDutySpread = d(spreadRng) * 0.01f;
		triAsymmetrySpread = d(spreadRng) * 0.015f;
		// Bleed magnitude spread: +/-2% (CHAR-05)
		bleedSpread = d(spreadRng) * 0.02f;
		// Pulse edge softening spread: +/-2%
		pulseEdgeSpread = d(spreadRng) * 0.02f;
	}

	struct Result {
		double deltaPhaseMul = 1.0;  // combined OU + jitter phase multiplier (LfoCore applies to deltaPhase)
		float dcOffsetV = 0.f;       // DC offset voltage, applied after crossfade by LfoCore
		float bleedLfo = 0.f;        // post-update ouLayers[0].state (Pitfall 3)
	};

	// step() — VERBATIM port of the per-sample drift block (AnalogLFO.cpp:727-761).
	// `driftAmount` is progressiveCurve(drift) computed by LfoCore (matches the
	// inline `driftAmount` at L731/L696). `sqrtDt` is sqrt(sampleTime), injected.
	// PRESERVES the exact RNG draw order: 4× OU, then 1× jitter, then 1× DC.
	Result step(float dt, float drift, float driftAmount, bool isClocked, float sqrtDt) {
		Result r;

		// Drift-low skip (AnalogLFO.cpp:759-761): do NOT step the OU layers; retain
		// ouLayers[0].state; return deltaPhaseMul=1, dcOffsetV=0, bleedLfo=retained.
		if (drift < 0.001f) {
			r.deltaPhaseMul = 1.0;
			r.dcOffsetV = 0.f;
			r.bleedLfo = ouLayers[0].state;  // RETAINED, not zeroed (Pitfall 3)
			return r;
		}

		// 4-layer OU step (AnalogLFO.cpp:732-742). RNG draws 1..4.
		float combinedOU = 0.f;
		for (int i = 0; i < NUM_OU_LAYERS; i++) {
			float noise = normalDist(rng);
			ouLayers[i].state += ouLayers[i].theta * (0.f - ouLayers[i].state) * dt
			                   + ouLayers[i].sigma * sqrtDt * noise;
			combinedOU += ouLayers[i].state * (ouLayers[i].weight + ouWeightSpread[i]);
		}
		// Reduced drift authority in clocked mode (DISP-04)
		float maxDrift = isClocked ? 0.02f : 0.075f;
		float driftScale = driftAmount * maxDrift;
		double deltaPhaseMul = (1.0 + (double)(driftScale * combinedOU));

		// Phase jitter: per-sample random phase deviation (CHAR-01). RNG draw 5.
		float jitterNoise = normalDist(rng);
		float jitterAuthority = isClocked ? 0.02f : 0.075f;
		float jitterScale = driftAmount * jitterAuthority * 0.003f;  // ~0.3% max deviation
		deltaPhaseMul *= (1.0 + (double)(jitterScale * jitterNoise));

		// DC offset wander: independent slow OU process (CHAR-02). RNG draw 6.
		float dcNoise = normalDist(rng);
		dcOffsetOU.state += dcOffsetOU.theta * (0.f - dcOffsetOU.state) * dt
		                  + dcOffsetOU.sigma * sqrtDt * dcNoise;
		float dcOffsetAuthority = isClocked ? 0.02f : 0.075f;
		r.dcOffsetV = driftAmount * dcOffsetAuthority * dcOffsetOU.state * 0.1f;

		r.deltaPhaseMul = deltaPhaseMul;
		r.bleedLfo = ouLayers[0].state;  // post-update value (Pitfall 3)
		return r;
	}
};

} // namespace forge
