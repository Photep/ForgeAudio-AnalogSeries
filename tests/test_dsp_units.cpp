// tests/test_dsp_units.cpp
//
// Behavioral unit tests for the four pure DSP headers extracted in Plan 22-02:
//   RackCompat.hpp, Waveshape.hpp, RatioTable.hpp, Swing.hpp
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).
// Headers are reached via -Isrc / -Itests (see the Makefile test target).
//
// Scope per RESEARCH.md L590: the Waveshape coverage here is the SCAFFOLD
// (a morph x character grid sweep + segment-boundary shape identity). FULL
// parameter ranges and the invariant/golden suites land in Phase 23 / later.
// Approximate comparisons use the candidate tolerances from RESEARCH.md L597-606.

#include "doctest.h"

#include <cmath>
#include <cstdint>
#include <random>

#include "dsp/RackCompat.hpp"
#include "dsp/Waveshape.hpp"
#include "dsp/RatioTable.hpp"
#include "dsp/Swing.hpp"

// ---------------------------------------------------------------------------
// RackCompat.hpp
// ---------------------------------------------------------------------------

TEST_CASE("RackCompat: Xoroshiro128Plus same-seed reproducibility") {
	forge::Xoroshiro128Plus a(0x1234ULL, 0x5678ULL);
	forge::Xoroshiro128Plus b(0x1234ULL, 0x5678ULL);
	bool anyNonZero = false;
	for (int i = 0; i < 32; ++i) {
		uint64_t va = a();
		uint64_t vb = b();
		CHECK(va == vb);                 // identical seed => identical stream
		if (va != 0) anyNonZero = true;
	}
	CHECK(anyNonZero);                   // stream is not all-zero

	// A different seed must diverge within a few draws.
	forge::Xoroshiro128Plus c(0x1234ULL, 0x9999ULL);
	forge::Xoroshiro128Plus d(0x1234ULL, 0x5678ULL);
	bool diverged = false;
	for (int i = 0; i < 8; ++i) { if (c() != d()) { diverged = true; break; } }
	CHECK(diverged);

	// min()/max() span the full uint64 range (so std::normal_distribution accepts it).
	CHECK(forge::Xoroshiro128Plus::min() == 0ULL);
	CHECK(forge::Xoroshiro128Plus::max() == UINT64_MAX);

	// Sanity: it is usable as a UniformRandomBitGenerator.
	forge::Xoroshiro128Plus e(0xABCDULL, 0xEF01ULL);
	std::normal_distribution<float> nd{0.f, 1.f};
	float n = nd(e);
	CHECK(std::isfinite(n));
}

TEST_CASE("RackCompat: SchmittTrigger UNINITIALIZED -> HIGH triggers once") {
	forge::SchmittTrigger trig;          // starts UNINITIALIZED
	// First crossing above highThreshold from UNINITIALIZED latches HIGH but
	// does NOT report a trigger (load-bearing UNINITIALIZED handling).
	CHECK(trig.process(2.f, 0.1f, 1.0f) == false);
	CHECK(trig.isHigh());
	// Staying high reports no further triggers.
	CHECK(trig.process(2.f, 0.1f, 1.0f) == false);
	CHECK(trig.isHigh());
	// Drop below lowThreshold => LOW, no spurious trigger.
	CHECK(trig.process(0.f, 0.1f, 1.0f) == false);
	CHECK_FALSE(trig.isHigh());
	// Now a clean LOW->HIGH crossing DOES report a trigger exactly once.
	CHECK(trig.process(2.f, 0.1f, 1.0f) == true);
	CHECK(trig.process(2.f, 0.1f, 1.0f) == false);
}

TEST_CASE("RackCompat: OnePole steady-state snaps to input") {
	forge::OnePole f;
	f.setLambda(30.f);
	const float dt = 1.f / 44100.f;
	const float in = 0.75f;
	float out = 0.f;
	for (int i = 0; i < 100000; ++i) out = f.process(dt, in);
	// The granularity snap (out == y ? in : y) converges EXACTLY to the input.
	CHECK(out == doctest::Approx(in));
	// Once converged it stays exactly at in.
	CHECK(f.process(dt, in) == in);
}

TEST_CASE("RackCompat: exp2_taylor5 matches the Rack polynomial") {
	CHECK(forge::exp2_taylor5(0.f) == 1.f);              // exact at 0
	CHECK(forge::exp2_taylor5(1.f) == doctest::Approx(2.f).epsilon(1e-4));
	CHECK(forge::exp2_taylor5(-1.f) == doctest::Approx(0.5f).epsilon(1e-4));
	// Determinism: same input => identical bits.
	CHECK(forge::exp2_taylor5(0.3333f) == forge::exp2_taylor5(0.3333f));
}

// ---------------------------------------------------------------------------
// Waveshape.hpp  (D-05 bleedLfo lift)
// ---------------------------------------------------------------------------

TEST_CASE("Waveshape: morph x character sweep stays in [-1,1] pre-scale") {
	forge::Waveshape ws;                 // zero spreads
	// Pre-scale bound: the adjacent-shape bleed crosstalk (computeMorphedWave,
	// AnalogLFO.cpp:360-385) can push past +/-1 before the +/-5V scaling. A fine
	// grid sweep of the VERBATIM shipped math gives a true range of about
	// [-1.048, +1.106] at bleedLfo=0, so the ideal "[-1,1]" is only approximate
	// for this DSP. This SCAFFOLD bound (+/-1.15) catches gross/runaway excursions
	// without false-failing on the shipped bleed behavior; the strict post-scale
	// +/-5V invariant is Phase 23's test_invariants.cpp (RESEARCH.md L586,L590,L602).
	const float lo = -1.15f, hi = 1.15f;
	for (int mi = 0; mi <= 20; ++mi) {
		float morph = (float)mi / 20.f;          // morph in [0,1]
		for (int ci = 0; ci <= 20; ++ci) {
			float character = (float)ci / 20.f;  // character in [0,1]
			for (int pi = 0; pi < 64; ++pi) {
				float phase = (float)pi / 64.f;  // phase in [0,1)
				// bleedLfo=0 reproduces the no-drift bleed path.
				float v = ws.morphedWave(phase, morph, character, 0.f);
				CHECK(v >= lo);
				CHECK(v <= hi);
			}
		}
	}
}

TEST_CASE("Waveshape: shape identity at the 5 morph boundaries") {
	forge::Waveshape ws;
	const float phase = 0.3f;            // representative phase
	const float character = 0.f;         // clean (digital) shapes for identity
	const float tol = 1e-5f;

	// At character=0 each compute* returns its digital shape; morphedWave at the
	// segment boundaries selects that shape exactly (frac=0 at each boundary).
	CHECK(ws.morphedWave(phase, 0.00f, character, 0.f)
	      == doctest::Approx(ws.computeSine(phase, character)).epsilon(tol));     // morph 0 ~ sine
	CHECK(ws.morphedWave(phase, 0.25f, character, 0.f)
	      == doctest::Approx(ws.computeTriangle(phase, character)).epsilon(tol)); // 0.25 ~ triangle
	CHECK(ws.morphedWave(phase, 0.50f, character, 0.f)
	      == doctest::Approx(ws.computeSaw(phase, character)).epsilon(tol));      // 0.50 ~ saw
	CHECK(ws.morphedWave(phase, 0.75f, character, 0.f)
	      == doctest::Approx(ws.computeSquare(phase, character)).epsilon(tol));   // 0.75 ~ square
	// morph 1.0 -> narrow pulse (duty 0.05); at phase=0.3 (> duty) pulse is -1.
	CHECK(ws.morphedWave(phase, 1.00f, character, 0.f)
	      == doctest::Approx(ws.computePulse(phase, character, 0.05f)).epsilon(tol));
}

TEST_CASE("Waveshape: bleedLfo enters only via the explicit parameter") {
	forge::Waveshape ws;
	const float phase = 0.42f, morph = 0.6f, character = 0.8f;
	// Non-zero bleedLfo perturbs the bleed modulation; bleedLfo=0 is the baseline.
	float base = ws.morphedWave(phase, morph, character, 0.f);
	float warped = ws.morphedWave(phase, morph, character, 0.9f);
	CHECK(base != doctest::Approx(warped));     // the parameter has an effect
	// Re-evaluating with the same args is deterministic.
	CHECK(ws.morphedWave(phase, morph, character, 0.9f) == warped);
}

// ---------------------------------------------------------------------------
// RatioTable.hpp
// ---------------------------------------------------------------------------

TEST_CASE("RatioTable: tables are length 15") {
	CHECK(sizeof(forge::RATIO_TABLE) / sizeof(forge::RATIO_TABLE[0]) == 15);
	CHECK(sizeof(forge::RATIO_LABELS) / sizeof(forge::RATIO_LABELS[0]) == 15);
	CHECK(forge::RATIO_TABLE[7] == doctest::Approx(1.f));   // x1 at the centre
	CHECK(std::string(forge::RATIO_LABELS[7]) == "x1");
}

TEST_CASE("RatioTable: shouldReset division behavior") {
	// idx 0 = /16 (RATIO_TABLE < 1): divisor = round(1/0.0625) = 16.
	CHECK_FALSE(forge::shouldReset(0, 15));   // not yet reached 16 beats
	CHECK(forge::shouldReset(0, 16));         // reached the divisor
	CHECK(forge::shouldReset(0, 17));

	// idx 5 = /2 (0.5): divisor = 2.
	CHECK_FALSE(forge::shouldReset(5, 1));
	CHECK(forge::shouldReset(5, 2));

	// idx 7 = x1 and idx 9 = x2 (RATIO_TABLE >= 1): reset every beat.
	CHECK(forge::shouldReset(7, 1));
	CHECK(forge::shouldReset(9, 1));

	// ratioIdx < 0 (unlocked): always reset.
	CHECK(forge::shouldReset(-1, 1));
}

// ---------------------------------------------------------------------------
// Swing.hpp
// ---------------------------------------------------------------------------

TEST_CASE("Swing: SWING_FRACTIONS is length 6") {
	CHECK(sizeof(forge::SWING_FRACTIONS) / sizeof(forge::SWING_FRACTIONS[0]) == 6);
	CHECK(forge::SWING_FRACTIONS[0] == doctest::Approx(0.50f));
	CHECK(forge::SWING_FRACTIONS[3] == doctest::Approx(0.66f));
}

TEST_CASE("Swing: swingPhaseMultiplier gates and warp values") {
	// Straight (swingFrac = 0.50 <= 0.5001) => no warp, both halves 1.0.
	CHECK(forge::swingPhaseMultiplier(0.25, 0.50f, true) == doctest::Approx(1.0));
	CHECK(forge::swingPhaseMultiplier(0.75, 0.50f, true) == doctest::Approx(1.0));

	// Free-run (isClocked=false) never warps, even at a swung fraction.
	CHECK(forge::swingPhaseMultiplier(0.25, 0.66f, false) == doctest::Approx(1.0));
	CHECK(forge::swingPhaseMultiplier(0.75, 0.66f, false) == doctest::Approx(1.0));

	// Clocked, swingFrac = 0.66: first half = 0.5/0.66, second half = 0.5/(1-0.66).
	CHECK(forge::swingPhaseMultiplier(0.25, 0.66f, true)
	      == doctest::Approx(0.5 / (double)0.66f));
	CHECK(forge::swingPhaseMultiplier(0.75, 0.66f, true)
	      == doctest::Approx(0.5 / (1.0 - (double)0.66f)));
}
