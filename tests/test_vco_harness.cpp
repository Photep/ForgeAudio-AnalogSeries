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
//   7. the seam is a LIVE oscillator — the swept block is neither silent nor
//      constant (D-15; this slot held the Phase 29 silence tombstone)
//
// Coverage caveat CLOSED in Phase 30 (D-19). Phase 29 recorded invariants 5 and
// 6 as green-but-weak and explicitly NOT coverage, because a silent step() made
// both trivially true. Phase 30 landed real DSP into the seam, so the debt is
// PAID rather than quietly dropped: both are now driven through a live
// accumulator and a live seeded Waveshape by the varying sweep the Phase 29
// driver was built to supply, and the reason each one now counts is written into
// its own banner below. Invariants 2, 3 and 4 were non-vacuous from day one:
// they read real telemetry and a real spread-RNG draw.
//
// Deliberately NOT here: pitch accuracy, output magnitude bounds and
// per-instance spread-seed divergence — those live in tests/test_vco_core.cpp
// (plans 30-03 / 30-04). The alias floor belongs to Phase 32 (this oscillator is
// deliberately aliased), and the < 1-cent V/Oct tracking gate is Phase 31's
// TEST-02, which needs coarse/fine/FM summing that does not exist yet.
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
// 5. Seam determinism. LOAD-BEARING as of Phase 30 (D-19 closed).
//    Phase 29 recorded this row as green-but-weak: D-01 made step() silent, so
//    two blocks of zeros compared equal no matter what the seeds were. That is
//    no longer the situation. The sweep varies pitch, morph and character across
//    the block; every sample now runs through a real double-precision phase
//    accumulator and a real seeded forge::Waveshape whose five spread
//    coefficients came from this driver's spread seeds. Bit-identical output
//    from two identically-seeded drivers is therefore a genuine determinism
//    claim about real DSP — it would break on any hidden global, any
//    uninitialised member and any per-sample RNG draw that ignored the seed.
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
// 6. Finiteness. LOAD-BEARING as of Phase 30 (D-19 closed).
//    Phase 29 booked this row as green-but-weak alongside invariant 5, because
//    isfinite(0.f) is trivially true. Over a real oscillator it is a real claim:
//    the sweep drives pitch from -2 V to +2 V through exp2_taylor5 and the
//    frozen morphedWave, and a NaN or Inf anywhere in that chain lands here.
//
//    THE SHARP EDGE, measured rather than assumed: finiteness alone does NOT
//    catch a runaway phase accumulator. With the Nyquist guard removed,
//    pitchCV = +10 drives the output to -8,655,011 V — and every one of those
//    samples is perfectly std::isfinite, so this case stays green through a
//    catastrophic failure. The invariant that catches it is the output
//    MAGNITUDE BOUND in tests/test_vco_core.cpp (plan 30-03). The two are
//    complementary, not redundant: do not delete either as covered by the other.
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
// 7. The seam is ALIVE (D-15).
//    This slot held the Phase 29 TOMBSTONE, which asserted the seam was silent
//    by construction (D-01). Phase 30 (CORE-01) INVERTED it in place rather than
//    deleting it: same slot, same drive, opposite assertion. A future refactor
//    that reverts VcoCore::step() to a stub — or breaks the phase accumulator so
//    the block degenerates to DC — fails HERE, loudly, instead of quietly
//    turning the whole VCO suite vacuous again.
//
//    Two independent scans, and neither implies the other: a constant non-zero
//    DC block passes "not all zero" and fails "not all the same", and that is
//    precisely the degenerate shape a dead accumulator produces. The inversion
//    was proven non-vacuous by construction, not by reading — with an early
//    `return 0.f;` spliced into step() this case was OBSERVED to go red.
// ---------------------------------------------------------------------------
TEST_CASE("vco harness: the seam is a live oscillator - the swept block is neither silent nor constant (D-15)") {
	const double sr = 48000.0;
	const int n = 1024;
	forge::VcoBlockDriver d(sr);
	auto out = d.run(n, forge::VcoBlockDriver::sweepScenario(n, harnessBase()));
	REQUIRE(out.size() == (size_t)n);

	// Direct float != throughout (NOT doctest::Approx — see the comment on
	// invariant 5): these are existence claims about exact values, and an
	// Approx-based scan would call a near-silent block silent.
	bool anyNonZero = false;
	for (size_t i = 0; i < out.size(); ++i) {
		if (out[i] != 0.f) { anyNonZero = true; break; }
	}
	CHECK(anyNonZero);

	bool anyVarying = false;
	for (size_t i = 0; i < out.size(); ++i) {
		if (out[i] != out[0]) { anyVarying = true; break; }
	}
	CHECK(anyVarying);
}
