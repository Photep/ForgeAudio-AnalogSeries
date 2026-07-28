// tests/test_vco_harness.cpp
//
// TEST-01 harness suite over the Phase 29 seam forge::VcoCore, driven headless
// through tests/VcoBlockDriver.hpp. Proves the harness PLUMBING, not VCO
// behavior — per D-02 no VCO DSP is pulled forward into Phase 29, and the
// behavioral canary for this phase is the shipped-LFO golden replay instead.
//
// Invariants:
//   1. drives VcoCore over blocks at 44.1 / 48 / 96 kHz with no libRack linked
//   2. sampleTime is overwritten to 1/sampleRate every step
//   3. sampleRate is injected every step
//   4. default seeds are non-degenerate (never the (0,0) Xoroshiro fixed point)
//   5. seam determinism — same seeds produce bit-identical blocks
//   6. output is finite (no NaN, no Inf)
//   7. TOMBSTONE — the seam is silent by construction (D-01)
//
// Known coverage caveat (P-7, stated rather than papered over): invariants 5
// and 6 are WEAK while step() returns 0.f — they are trivially satisfied by a
// silent core. They are driven with a varying input sweep so they become
// load-bearing the moment Phase 30 lands DSP, and invariant 7 exists to force
// Phase 30 to acknowledge that the seam changed. Invariants 2, 3 and 4 are
// non-vacuous today: they read real telemetry and a real spread-RNG draw.
//
// Deliberately NOT here: pitch accuracy, alias floor and output bounds. Those
// belong to Phases 31, 32 and 34 and would be meaningless against a silent seam.
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "VcoBlockDriver.hpp"

#include <vector>
#include <cmath>
#include <cstdint>

namespace {

// The three production sample rates every invariant is parametrized over.
constexpr double SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0};

// Baseline harness input. Built by default construction + field assignment,
// never a brace value-list (VcoInputs has NSDMIs, so it is not a C++11
// aggregate — P-8).
forge::VcoInputs harnessBase() {
	forge::VcoInputs in;
	in.pitchCV   = 0.f;
	in.coarse    = 0.f;
	in.fine      = 0.f;
	in.morph     = 0.4f;
	in.character = 0.6f;
	in.drift     = 0.5f;
	return in;
}

} // namespace

// ---------------------------------------------------------------------------
// 1. Structural proof: the seam is driveable over blocks at all three
//    production rates with NO libRack. This links and runs only because the
//    `test` target passes no -I../Rack-SDK/include and links no -lRack.
// ---------------------------------------------------------------------------
TEST_CASE("vco harness: drives VcoCore over blocks at 44.1 / 48 / 96 kHz Rack-free") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const int n = 1024;
		forge::VcoBlockDriver d(sr);
		auto out = d.run(n, forge::VcoBlockDriver::sweepScenario(n, harnessBase()));
		REQUIRE(out.size() == (size_t)n);
		CHECK(d.core.tel.stepCount == (uint32_t)n);
	}
}

// ---------------------------------------------------------------------------
// 2. sampleTime injection. Non-vacuous: the functor deliberately supplies a
//    bogus value, so this fails the moment the overwrite in run() is removed
//    or made conditional.
// ---------------------------------------------------------------------------
TEST_CASE("vco harness: overwrites caller sampleTime with 1/sampleRate every step") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		forge::VcoBlockDriver d(sr);
		auto out = d.run(256, [](int) {
			forge::VcoInputs in = harnessBase();
			in.sampleTime = 999.f;   // deliberately bogus — the harness owns timing
			return in;
		});
		REQUIRE(out.size() == 256u);
		CHECK(d.core.tel.lastSampleTime == (float)(1.0 / sr));
	}
}

// ---------------------------------------------------------------------------
// 3. sampleRate injection. Same non-vacuity argument as invariant 2. The Nyquist
//    clamp (PITCH-04, Phase 31) depends on this value being real.
// ---------------------------------------------------------------------------
TEST_CASE("vco harness: injects sampleRate every step") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		forge::VcoBlockDriver d(sr);
		auto out = d.run(256, [](int) {
			forge::VcoInputs in = harnessBase();
			in.sampleRate = -1.f;   // deliberately bogus
			return in;
		});
		REQUIRE(out.size() == 256u);
		CHECK(d.core.tel.lastSampleRate == (float)sr);
	}
}

// ---------------------------------------------------------------------------
// 4. Non-degenerate seeding. This is a HANG guard, not a style check:
//    forge::Xoroshiro128Plus seeded (0,0) is a fixed point that emits an
//    all-zero stream forever, and std::normal_distribution's rejection loop
//    never terminates on an all-zero uniform stream. In Rack that is a hang on
//    patch load, not a test failure (P-9).
// ---------------------------------------------------------------------------
TEST_CASE("vco harness: default seeds are non-degenerate (never the (0,0) Xoroshiro fixed point)") {
	forge::VcoBlockDriver d;

	// The drift RNG state is not the all-zero fixed point.
	CHECK((d.core.drift.rng.state[0] != 0ULL || d.core.drift.rng.state[1] != 0ULL));

	// The stronger assertion: at least one component-spread coefficient is
	// non-zero. Non-zero spread coefficients can only exist if the SEPARATE
	// spread RNG produced a live stream — exactly what a (0,0) seed would make
	// impossible. This proves setSpreadSeed() was both called and usefully seeded.
	const bool spreadLive =
		d.core.drift.characterSpread != 0.f ||
		d.core.drift.sawCurvatureSpread != 0.f ||
		d.core.drift.squareDutySpread != 0.f ||
		d.core.drift.triAsymmetrySpread != 0.f ||
		d.core.drift.bleedSpread != 0.f ||
		d.core.drift.pulseEdgeSpread != 0.f;
	CHECK(spreadLive);
}

// ---------------------------------------------------------------------------
// 5. Seam determinism.
//    WEAK BY CONSTRUCTION TODAY: D-01 makes step() silent, so two blocks of
//    zeros compare equal no matter what the seeds are. It is kept, and driven
//    with a VARYING sweep rather than a constant input, so it becomes
//    load-bearing the moment Phase 30 lands real DSP over the seeded
//    DriftEngine. Do not delete it as "vacuous" — revisit it in Phase 30.
// ---------------------------------------------------------------------------
TEST_CASE("vco harness: seam determinism (same seeds produce bit-identical blocks)") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const int n = (int)std::lround(sr * 0.05);
		forge::VcoInputs base = harnessBase();

		forge::VcoBlockDriver a(sr, 0xC0FFEEULL, 0xBADF00DULL, 0x9E3779B9ULL, 0x7F4A7C15ULL);
		forge::VcoBlockDriver b(sr, 0xC0FFEEULL, 0xBADF00DULL, 0x9E3779B9ULL, 0x7F4A7C15ULL);
		auto oa = a.run(n, forge::VcoBlockDriver::sweepScenario(n, base));
		auto ob = b.run(n, forge::VcoBlockDriver::sweepScenario(n, base));
		REQUIRE(oa.size() == ob.size());

		// Bit-exact comparison via a direct float == (NOT doctest::Approx, whose
		// epsilon(0) still applies a relative-scaling margin and is not a true
		// bit-exact comparator).
		bool identical = true;
		for (size_t i = 0; i < oa.size(); ++i) {
			if (oa[i] != ob[i]) { identical = false; break; }
		}
		CHECK(identical);
	}
}

// ---------------------------------------------------------------------------
// 6. Finiteness. Same "weak while the seam is silent" caveat as invariant 5:
//    0.f is trivially finite. Driven with the varying sweep so it starts
//    catching real NaN/Inf the moment Phase 30 DSP lands.
// ---------------------------------------------------------------------------
TEST_CASE("vco harness: output is finite (no NaN, no Inf)") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const int n = 2048;
		forge::VcoBlockDriver d(sr);
		auto out = d.run(n, forge::VcoBlockDriver::sweepScenario(n, harnessBase()));
		REQUIRE(out.size() == (size_t)n);
		bool allFinite = true;
		for (size_t i = 0; i < out.size(); ++i) {
			if (!std::isfinite(out[i])) { allFinite = false; break; }
		}
		CHECK(allFinite);
	}
}

// ---------------------------------------------------------------------------
// 7. TOMBSTONE.
//    Phase 30 (CORE-01) is REQUIRED to DELETE this test case when it lands real
//    VCO DSP. Its failure is not a bug — it is the intended signal that the
//    Phase 29 seam changed from "silent by construction" to a real oscillator,
//    and that the weak invariants above must be revisited at the same time.
// ---------------------------------------------------------------------------
TEST_CASE("vco harness: TOMBSTONE - the Phase 29 seam is silent by construction (D-01)") {
	const double sr = 48000.0;
	const int n = 1024;
	forge::VcoBlockDriver d(sr);
	auto out = d.run(n, forge::VcoBlockDriver::sweepScenario(n, harnessBase()));
	REQUIRE(out.size() == (size_t)n);
	bool allSilent = true;
	for (size_t i = 0; i < out.size(); ++i) {
		if (out[i] != 0.f) { allSilent = false; break; }
	}
	CHECK(allSilent);
}
