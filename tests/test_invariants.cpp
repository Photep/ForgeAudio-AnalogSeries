// tests/test_invariants.cpp
//
// TEST-04 behavioral invariant suite over the extracted forge::LfoCore, driven
// headless through tests/BlockDriver.hpp. Each invariant is parametrized over the
// three production sample rates {44100, 48000, 96000} Hz, establishing the
// behavioral baseline that later DSP refactors (Phase 23/24) must preserve.
//
// Invariants (RESEARCH.md L580-590 test map; tolerances L597-606):
//   1. ±5V output bounds      — [-5.12,+5.12] V with drift; strict ±5.0 (1e-4 slop) without.
//   2. free-run freq accuracy — measured period within ±1.0% of the knob Hz over >= 2 s.
//   3. phase continuity reset — |out[n]-out[n-1]| <= 0.5 V across the 3 ms reset crossfade.
//   4. fixed-seed determinism — same seed => bit-identical block (drift on); diff seed diverges.
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "BlockDriver.hpp"

#include <vector>
#include <cmath>
#include <cstdint>

namespace {

// The three production sample rates every invariant is parametrized over.
constexpr double SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0};

// Canonical free-run scenario (matches the golden seeds.txt, drift ON).
forge::Inputs freeRunBase() {
	forge::Inputs in;
	in.rate        = 2.0f;
	in.morph       = 0.4f;
	in.character   = 0.6f;
	in.drift       = 0.5f;   // drift ON — exercises the stochastic OU path
	in.phaseOffset = 0.f;
	in.swingIndex  = 0;
	return in;
}

} // namespace

// ---------------------------------------------------------------------------
// Invariant 1: ±5V output bounds.
//   drift ON  -> [-5.12, +5.12] V (DC wander adds up to ~50-100 mV at full drift)
//   drift OFF -> strict ±5.0 V (allow 1e-4 FP slop)
// ---------------------------------------------------------------------------
TEST_CASE("invariant: +/-5V output bounds (drift on)") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const int n = (int)std::lround(sr * 2.0);  // 2 s
		forge::BlockDriver d(sr);
		forge::Inputs base = freeRunBase();
		auto out = d.run(n, [&](int) { return base; });
		REQUIRE(out.size() == (size_t)n);
		for (float v : out) {
			CHECK(v >= -5.12f);
			CHECK(v <= 5.12f);
		}
	}
}

TEST_CASE("invariant: +/-5V output bounds (drift off, strict)") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const int n = (int)std::lround(sr * 2.0);
		forge::BlockDriver d(sr);
		forge::Inputs base = freeRunBase();
		base.drift = 0.f;  // drift OFF -> no DC wander, strict +/-5 V
		auto out = d.run(n, [&](int) { return base; });
		for (float v : out) {
			CHECK(v >= -5.0f - 1e-4f);
			CHECK(v <= 5.0f + 1e-4f);
		}
	}
}

// ---------------------------------------------------------------------------
// Invariant 2: free-run frequency accuracy within +/-1.0% of the knob Hz.
// Measure the period from rising zero-crossings over >= 2 s; drift OFF so the
// measured frequency is the deterministic free-run frequency (no OU wander).
// ---------------------------------------------------------------------------
TEST_CASE("invariant: free-run frequency accuracy (+/-1%)") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const float knobHz = 2.0f;
		const double seconds = 4.0;                 // >= 2 s, more cycles -> tighter estimate
		const int n = (int)std::lround(sr * seconds);
		forge::BlockDriver d(sr);
		forge::Inputs base = freeRunBase();
		base.drift = 0.f;                           // measure the clean free-run rate
		auto out = d.run(n, [&](int) { return base; });

		// Count rising zero-crossings (negative -> non-negative); period = span/cycles.
		double firstCross = -1.0, lastCross = -1.0;
		int crossings = 0;
		for (size_t i = 1; i < out.size(); ++i) {
			if (out[i - 1] < 0.f && out[i] >= 0.f) {
				// Linear-interpolate the sub-sample crossing time for accuracy.
				double frac = (double)(-out[i - 1]) / (double)(out[i] - out[i - 1]);
				double t = ((double)(i - 1) + frac) / sr;
				if (firstCross < 0.0) firstCross = t;
				lastCross = t;
				++crossings;
			}
		}
		REQUIRE(crossings >= 3);                    // need at least a couple of full cycles
		double measuredHz = (double)(crossings - 1) / (lastCross - firstCross);
		double errPct = std::abs(measuredHz - knobHz) / knobHz * 100.0;
		CAPTURE(measuredHz);
		CAPTURE(errPct);
		CHECK(errPct <= 1.0);
	}
}

// ---------------------------------------------------------------------------
// Invariant 3: phase continuity at reset (no click).
// Drive a steady clock via clockedScenario; the divide-aware clock fires periodic
// phase resets, each engaging the 3 ms cosine crossfade. Assert no adjacent-sample
// step exceeds 0.5 V anywhere in the run (a broken crossfade would be a multi-volt
// step).
//
// The waveform itself must be CONTINUOUS for this invariant to isolate the reset
// crossfade: the saw/square/pulse shapes contain legitimate near-vertical edges
// (a saw drops +1->-1 in one phase tick) that are part of the waveshape, not a
// reset click. So we use the SINE shape (morph=0) with drift OFF — the only way
// |dOut| exceeds the per-sample slope of a 2 Hz sine is a broken reset crossfade.
// ---------------------------------------------------------------------------
TEST_CASE("invariant: phase continuity at reset (no click, |dOut| <= 0.5V)") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const int n = (int)std::lround(sr * 3.0);   // 3 s -> several clock resets at 120 BPM
		forge::BlockDriver d(sr);
		forge::Inputs base = freeRunBase();
		base.morph = 0.f;   // SINE — continuous waveform, isolates the reset crossfade
		base.drift = 0.f;   // no DC wander masking / contributing to the step
		auto inputAt = forge::BlockDriver::clockedScenario(sr, /*bpm*/120.0, base);
		auto out = d.run(n, inputAt);

		float maxStep = 0.f;
		for (size_t i = 1; i < out.size(); ++i) {
			float step = std::fabs(out[i] - out[i - 1]);
			if (step > maxStep) maxStep = step;
		}
		CAPTURE(maxStep);
		CHECK(maxStep <= 0.5f);
	}
}

// ---------------------------------------------------------------------------
// Invariant 4: fixed-seed determinism.
//   - two identically-seeded drivers (drift ON) produce a bit-identical block.
//   - different drift seeds diverge (the stochastic path actually depends on seed).
// Bit-exact on the same platform (epsilon 0) per the tolerance table.
// ---------------------------------------------------------------------------
TEST_CASE("invariant: fixed-seed determinism (same seed -> bit-identical)") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const int n = (int)std::lround(sr * 2.0);
		forge::Inputs base = freeRunBase();

		forge::BlockDriver a(sr, 0xC0FFEEULL, 0xBADF00DULL);
		forge::BlockDriver b(sr, 0xC0FFEEULL, 0xBADF00DULL);
		auto oa = a.run(n, [&](int) { return base; });
		auto ob = b.run(n, [&](int) { return base; });
		REQUIRE(oa.size() == ob.size());
		bool identical = true;
		for (size_t i = 0; i < oa.size(); ++i) {
			if (oa[i] != ob[i]) { identical = false; break; }  // bit-exact
		}
		CHECK(identical);
	}
}

TEST_CASE("invariant: fixed-seed determinism (different seed -> diverges)") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const int n = (int)std::lround(sr * 2.0);
		forge::Inputs base = freeRunBase();

		forge::BlockDriver a(sr, 0xC0FFEEULL, 0xBADF00DULL);
		forge::BlockDriver c(sr, 0x13579BDFULL, 0x2468ACE0ULL);  // different drift seed
		auto oa = a.run(n, [&](int) { return base; });
		auto oc = c.run(n, [&](int) { return base; });
		REQUIRE(oa.size() == oc.size());
		bool anyDiff = false;
		for (size_t i = 0; i < oa.size(); ++i) {
			if (oa[i] != oc[i]) { anyDiff = true; break; }
		}
		CHECK(anyDiff);  // drift ON -> seed must actually change the stream
	}
}
