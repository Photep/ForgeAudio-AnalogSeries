// tests/test_dsp_stateful.cpp
//
// Behavioral unit tests for the two STATEFUL DSP headers extracted in Plan 22-03:
//   ClockTracker.hpp (EMA + outlier rejection + 3-state FSM + fast-track re-acquire)
//   DriftEngine.hpp  (4 OU layers + jitter + DC OU, bit-identical RNG draw order)
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).
// Headers reached via -Isrc / -Itests (see the Makefile test target).

#include "doctest.h"

#include <cmath>
#include <cstdint>

#include "dsp/ClockTracker.hpp"
#include "dsp/DriftEngine.hpp"

// ---------------------------------------------------------------------------
// ClockTracker.hpp — feed a square-wave clock and check lock / reject / recover.
// ---------------------------------------------------------------------------

// Drive the tracker over `seconds` of a square wave at `bpm` (rising edge +10V).
// Returns the final Result. dt = 1/sr.
static forge::ClockTracker::Result driveClock(forge::ClockTracker& ct, double bpm,
                                              double seconds, double sr, int ratioIdx = 7) {
	double period = 60.0 / bpm;
	double half = period * 0.5;
	double dt = 1.0 / sr;
	int n = (int)(seconds * sr);
	forge::ClockTracker::Result r;
	for (int i = 0; i < n; i++) {
		double t = i * dt;
		double ph = std::fmod(t, period);
		float clkV = (ph < half) ? 10.f : 0.f;
		r = ct.step(clkV, (float)dt, /*connected*/true, ratioIdx);
	}
	return r;
}

TEST_CASE("ClockTracker: locks at 120 BPM after >=4 clean edges") {
	forge::ClockTracker ct;
	// 120 BPM => period 0.5s. Run 4s => 8 edges, well past the 4-edge LOCKED threshold.
	auto r = driveClock(ct, 120.0, 4.0, 48000.0, /*ratioIdx x1*/7);
	CHECK(ct.clockState == forge::LOCKED);
	// smoothedPeriod should be ~0.5s (EMA converged).
	CHECK(ct.smoothedPeriod == doctest::Approx(0.5).epsilon(0.02));
	CHECK(r.state == forge::LOCKED);
}

// Emit a clean clock interval: hold LOW for `lowSec`, then rising edge to +10V for
// one sample, then back LOW — so the SchmittTrigger sees exactly one edge after a
// well-defined interval. Returns the last Result.
static forge::ClockTracker::Result clockInterval(forge::ClockTracker& ct, double lowSec,
                                                  double sr, int ratioIdx = 7) {
	double dt = 1.0 / sr;
	int n = (int)(lowSec * sr);
	forge::ClockTracker::Result r;
	for (int i = 0; i < n; i++) r = ct.step(0.f, (float)dt, true, ratioIdx);  // LOW gap
	r = ct.step(10.f, (float)dt, true, ratioIdx);                              // rising edge
	return r;
}

TEST_CASE("ClockTracker: single outlier edge is rejected (smoothedPeriod unchanged)") {
	forge::ClockTracker ct;
	double sr = 48000.0;
	// Lock at a 0.5s period via clean 0.5s-spaced edges (>=4 edges -> LOCKED).
	for (int e = 0; e < 8; e++) clockInterval(ct, 0.5, sr, 7);
	REQUIRE(ct.clockState == forge::LOCKED);
	float lockedPeriod = ct.smoothedPeriod;  // clockTimer was just reset by the last edge

	// Inject ONE outlier edge after only 0.05s (rawPeriod 0.05 << smoothedPeriod/3
	// = 0.167 => outlier => silently discarded in LOCKED; smoothedPeriod unchanged).
	auto r = clockInterval(ct, 0.05, sr, 7);
	CHECK(r.edgeFired == true);
	CHECK(ct.clockState == forge::LOCKED);
	CHECK(ct.smoothedPeriod == doctest::Approx(lockedPeriod));

	// A subsequent in-range edge (0.5s) is accepted again (proves it was a single
	// rejection, not a permanent stall).
	clockInterval(ct, 0.5, sr, 7);
	CHECK(ct.clockState == forge::LOCKED);
}

TEST_CASE("ClockTracker: re-acquires after a sustained >3x tempo change (does not stay stale)") {
	forge::ClockTracker ct;
	// Lock at 120 BPM (period 0.5s).
	driveClock(ct, 120.0, 4.0, 48000.0, 7);
	REQUIRE(ct.clockState == forge::LOCKED);
	float fastPeriod = ct.smoothedPeriod;  // ~0.5

	// Now switch to 20 BPM (period 3.0s, a 6x slowdown) and run long enough that the
	// timeout fires (timeout = clamp(3*0.5, 1, 5) = 1.5s) -> revert to FREE -> then
	// the new slow clock re-acquires. The tracker must NOT stay LOCKED at the stale
	// 0.5s period forever.
	driveClock(ct, 20.0, 20.0, 48000.0, 7);
	// After sustained slow clock it has re-acquired the new period (~3.0s), not the
	// stale 0.5s. (May be ACQUIRING or LOCKED, but smoothedPeriod must reflect ~3.0s.)
	CHECK(ct.smoothedPeriod > 1.5f);            // moved off the stale fast period
	CHECK(ct.smoothedPeriod != doctest::Approx(fastPeriod));
}

TEST_CASE("ClockTracker: step takes injected clkV/connected (no inputs[] coupling)") {
	// Compile-time + behavioral proof that voltage/connection are injected: passing
	// connected=false reverts to FREE immediately even mid-lock.
	forge::ClockTracker ct;
	driveClock(ct, 120.0, 4.0, 48000.0, 7);
	REQUIRE(ct.clockState == forge::LOCKED);
	auto r = ct.step(0.f, 1.f / 48000.f, /*connected*/false, 7);
	CHECK(r.state == forge::FREE);
	CHECK(ct.clockState == forge::FREE);
}

// ---------------------------------------------------------------------------
// DriftEngine.hpp — determinism, RNG draw order, bleed retention at low drift.
// ---------------------------------------------------------------------------

TEST_CASE("DriftEngine: two same-seed engines produce bit-identical output") {
	forge::DriftEngine a, b;
	a.seed(0xC0FFEEULL, 0xBADF00DULL);
	b.seed(0xC0FFEEULL, 0xBADF00DULL);
	a.setSpreadSeed(0x1111ULL, 0x2222ULL);
	b.setSpreadSeed(0x1111ULL, 0x2222ULL);

	float dt = 1.f / 48000.f;
	float sqrtDt = std::sqrt(dt);
	float drift = 0.5f;
	float driftAmount = drift * drift;  // progressiveCurve
	for (int i = 0; i < 4096; i++) {
		auto ra = a.step(dt, drift, driftAmount, /*isClocked*/false, sqrtDt);
		auto rb = b.step(dt, drift, driftAmount, false, sqrtDt);
		// Bit-exact (epsilon 0): same seed, same code path, same platform.
		CHECK(ra.deltaPhaseMul == rb.deltaPhaseMul);
		CHECK(ra.dcOffsetV == rb.dcOffsetV);
		CHECK(ra.bleedLfo == rb.bleedLfo);
	}
}

TEST_CASE("DriftEngine: different seeds diverge") {
	forge::DriftEngine a, b;
	a.seed(0x1ULL, 0x2ULL);
	b.seed(0x3ULL, 0x4ULL);
	float dt = 1.f / 48000.f, sqrtDt = std::sqrt(dt);
	bool diverged = false;
	for (int i = 0; i < 64; i++) {
		auto ra = a.step(dt, 0.5f, 0.25f, false, sqrtDt);
		auto rb = b.step(dt, 0.5f, 0.25f, false, sqrtDt);
		if (ra.deltaPhaseMul != rb.deltaPhaseMul) { diverged = true; break; }
	}
	CHECK(diverged);
}

TEST_CASE("DriftEngine: RNG draw order is exactly 4x OU, 1x jitter, 1x DC (6 draws/step)") {
	// Reproduce the exact 6-draw-per-step sequence by hand against a parallel RNG and
	// confirm the engine consumes the stream in that order. We mirror the engine's
	// normalDist + Xoroshiro draws: 4 (OU) + 1 (jitter) + 1 (DC) = 6 per step.
	// NOTE: spread seed must be NON-zero. Xoroshiro128Plus seeded (0,0) is a fixed
	// point that emits an all-zero stream, which makes std::normal_distribution's
	// rejection sampling loop forever — a real landmine. The module seeds spread
	// from its random_device-derived RNG, never (0,0); tests must do likewise.
	forge::DriftEngine eng;
	eng.seed(0xABCDEFULL, 0x123456ULL);
	eng.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);  // non-zero spread (mirror gets the same)

	forge::Xoroshiro128Plus refRng;
	refRng.seed(0xABCDEFULL, 0x123456ULL);
	std::normal_distribution<float> refDist{0.f, 1.f};

	float dt = 1.f / 48000.f, sqrtDt = std::sqrt(dt);
	float drift = 0.5f, driftAmount = 0.25f;

	// Mirror one step with a hand-rolled engine state.
	forge::DriftEngine mirror;          // same OU constants via default ctor
	mirror.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);  // SAME non-zero spread as eng
	// Step 0: draw 4 OU noises, then jitter, then DC — same order.
	float combinedOU = 0.f;
	for (int i = 0; i < forge::DriftEngine::NUM_OU_LAYERS; i++) {
		float noise = refDist(refRng);          // OU draws 1..4
		mirror.ouLayers[i].state += mirror.ouLayers[i].theta * (0.f - mirror.ouLayers[i].state) * dt
		                          + mirror.ouLayers[i].sigma * sqrtDt * noise;
		combinedOU += mirror.ouLayers[i].state * (mirror.ouLayers[i].weight + mirror.ouWeightSpread[i]);
	}
	float maxDrift = 0.075f;  // free-run
	double expectMul = (1.0 + (double)(driftAmount * maxDrift * combinedOU));
	float jitterNoise = refDist(refRng);        // jitter draw 5
	float jitterScale = driftAmount * 0.075f * 0.003f;
	expectMul *= (1.0 + (double)(jitterScale * jitterNoise));
	float dcNoise = refDist(refRng);            // DC draw 6
	mirror.dcOffsetOU.state += mirror.dcOffsetOU.theta * (0.f - mirror.dcOffsetOU.state) * dt
	                         + mirror.dcOffsetOU.sigma * sqrtDt * dcNoise;
	float expectDc = driftAmount * 0.075f * mirror.dcOffsetOU.state * 0.1f;
	float expectBleed = mirror.ouLayers[0].state;

	auto r = eng.step(dt, drift, driftAmount, false, sqrtDt);
	CHECK(r.deltaPhaseMul == doctest::Approx(expectMul));
	CHECK(r.dcOffsetV == doctest::Approx(expectDc));
	CHECK(r.bleedLfo == doctest::Approx(expectBleed));
}

TEST_CASE("DriftEngine: drift<0.001 retains bleedLfo (not zeroed), mul=1, dc=0") {
	forge::DriftEngine eng;
	eng.seed(0x55ULL, 0x66ULL);
	float dt = 1.f / 48000.f, sqrtDt = std::sqrt(dt);
	// First, step with real drift so ouLayers[0].state becomes nonzero.
	for (int i = 0; i < 256; i++) eng.step(dt, 0.8f, 0.64f, false, sqrtDt);
	float retained = eng.ouLayers[0].state;
	REQUIRE(retained != 0.f);

	// Now step with drift below threshold: OU layers NOT stepped, state retained.
	auto r = eng.step(dt, 0.0005f, 0.f, false, sqrtDt);
	CHECK(r.deltaPhaseMul == 1.0);
	CHECK(r.dcOffsetV == 0.f);
	CHECK(r.bleedLfo == retained);          // retained, NOT zeroed (Pitfall 3)
	CHECK(eng.ouLayers[0].state == retained);
}
