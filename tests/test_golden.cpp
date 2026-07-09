// tests/test_golden.cpp
//
// D-08 PERMANENT GOLDEN REPLAY REGRESSION.
//
// Pins the extracted forge::LfoCore against the frozen reference blocks captured
// in Plan 03 (tests/golden/freerun_<rate>.f32). For each production sample rate we
// replay the canonical free-run scenario through a BlockDriver seeded with the
// EXACT seeds documented in tests/golden/freerun_seeds.txt, then assert the output
// reproduces the committed reference.
//
// Cross-platform structure (D-07 / TEST-06):
//   - DRIFT-OFF + SPREAD-OFF is the CROSS-PLATFORM regression guard. It runs on
//     EVERY OS (freerun_<rate>_driftoff.f32) with a tight absolute libm epsilon
//     (~1e-6). Drift off AND the spread path unseeded (never call setSpreadSeed)
//     means the output touches ONLY the portable Xoroshiro uniform + libm sin/cos,
//     so it replays identically-to-1e-6 across libc++ / libstdc++ / MinGW.
//   - DRIFT-ON is BIT-EXACT but macOS-ONLY. It draws from std::normal_distribution,
//     which is NOT specified to be portable across standard-library implementations
//     even given an identical Xoroshiro stream, so it is gated to the canonical
//     capture OS (macOS, Apple clang / libc++ -- see freerun_seeds.txt) and simply
//     does not run on the other CI legs. We deliberately do NOT widen the drift-on
//     epsilon to absorb that divergence (CONTEXT D-07): the drift-off leg is the
//     cross-platform guard instead.
//   - Same-platform same-seed DETERMINISM remains bit-exact everywhere (that
//     invariant lives in test_invariants.cpp); this file is the cross-build
//     reference pin.
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "BlockDriver.hpp"

#include <fstream>
#include <vector>
#include <cstdint>
#include <string>

namespace {

// Raw little-endian float32 reader (RESEARCH.md L477-482). One file per rate,
// 8192 samples (32768 bytes); format documented in freerun_seeds.txt.
std::vector<float> loadF32(const std::string& path) {
	std::ifstream f(path, std::ios::binary);
	std::vector<float> v;
	float x;
	while (f.read(reinterpret_cast<char*>(&x), sizeof x)) v.push_back(x);
	return v;
}

// Exact capture parameters from tests/golden/freerun_seeds.txt.
constexpr uint64_t DRIFT_S0  = 0x0000000000C0FFEEULL;
constexpr uint64_t DRIFT_S1  = 0x000000000BADF00DULL;
constexpr uint64_t SPREAD_S0 = 0x000000009E3779B9ULL;
constexpr uint64_t SPREAD_S1 = 0x000000007F4A7C15ULL;
constexpr int      GOLDEN_SAMPLES = 8192;

// Canonical free-run scenario (freerun_seeds.txt L13-19, drift ON).
forge::Inputs goldenBase() {
	forge::Inputs in;
	in.rate        = 2.0f;
	in.morph       = 0.4f;
	in.character   = 0.6f;
	in.drift       = 0.5f;   // drift ON -> exercises the stochastic OU path
	in.phaseOffset = 0.f;
	in.swingIndex  = 0;
	return in;
}

// --- Cross-platform drift-OFF + spread-OFF leg (runs on ALL OSes) -----------
// Absolute libm epsilon: portable Xoroshiro uniform + libm sin/cos differ only
// in the low mantissa across standard libraries. 1e-6 is well below the drift
// depth yet tight enough to catch any real regression (freerun_seeds.txt).
constexpr double DRIFTOFF_EPSILON = 1e-6;

// Canonical free-run scenario with drift OFF. Identical to goldenBase() EXCEPT
// in.drift = 0.0f (and the replay never seeds the spread path).
forge::Inputs goldenBaseDriftOff() {
	forge::Inputs in = goldenBase();
	in.drift = 0.0f;   // drift OFF -> deterministic, portable output
	return in;
}

// Replay the drift-off fixtures on every OS. Constructs forge::LfoCore DIRECTLY
// (not BlockDriver, whose ctor would seed the spread path) and seeds ONLY the
// drift RNG, so all *Spread stay 0.f — the exact neutralization the generator
// (tools/capture_golden.cpp) used. Mirrors BlockDriver::run's per-sample drive.
void replayGoldenDriftOff(double sr, const std::string& path) {
	auto ref = loadF32(path);
	REQUIRE(ref.size() == (size_t)GOLDEN_SAMPLES);

	forge::LfoCore core;
	core.seed(DRIFT_S0, DRIFT_S1);   // drift RNG only; spread path left neutralized

	const float dt = (float)(1.0 / sr);
	forge::Inputs base = goldenBaseDriftOff();
	std::vector<float> got;
	got.reserve(ref.size());
	for (size_t i = 0; i < ref.size(); ++i) {
		forge::Inputs in = base;
		in.sampleTime = dt;
		got.push_back(core.step(in));
	}
	REQUIRE(got.size() == ref.size());

	for (size_t i = 0; i < ref.size(); ++i) {
		CHECK(std::fabs((double)got[i] - (double)ref[i]) <= DRIFTOFF_EPSILON);
	}
}

// --- macOS-gated drift-ON bit-exact leg -------------------------------------
#if defined(__APPLE__)
// The drift-on golden is bit-exact ONLY on the canonical capture OS (macOS).
// std::normal_distribution is not portable across standard libraries, so this
// leg does not run off-canonical — the drift-off leg above is the guard there.
void replayGolden(double sr, const std::string& path) {
	auto ref = loadF32(path);
	REQUIRE(ref.size() == (size_t)GOLDEN_SAMPLES);

	forge::BlockDriver d(sr, DRIFT_S0, DRIFT_S1, SPREAD_S0, SPREAD_S1);
	forge::Inputs base = goldenBase();
	auto got = d.run((int)ref.size(), [&](int) { return base; });
	REQUIRE(got.size() == ref.size());

	// Bit-exact replay on the canonical OS. Use a direct float == (NOT
	// doctest::Approx, whose epsilon(0) still applies a relative-scaling margin
	// and is not a true bit-exact comparator).
	for (size_t i = 0; i < ref.size(); ++i) {
		CHECK(got[i] == ref[i]);
	}
}
#endif // __APPLE__

} // namespace

// --- Cross-platform drift-off cases (every OS) ------------------------------
TEST_CASE("golden: drift-off freerun replay matches reference @ 44.1k") {
	replayGoldenDriftOff(44100.0, "tests/golden/freerun_44100_driftoff.f32");
}

TEST_CASE("golden: drift-off freerun replay matches reference @ 48k") {
	replayGoldenDriftOff(48000.0, "tests/golden/freerun_48000_driftoff.f32");
}

TEST_CASE("golden: drift-off freerun replay matches reference @ 96k") {
	replayGoldenDriftOff(96000.0, "tests/golden/freerun_96000_driftoff.f32");
}

// --- macOS-only drift-on bit-exact cases ------------------------------------
#if defined(__APPLE__)
TEST_CASE("golden: freerun replay matches reference @ 44.1k") {
	replayGolden(44100.0, "tests/golden/freerun_44100.f32");
}

TEST_CASE("golden: freerun replay matches reference @ 48k") {
	replayGolden(48000.0, "tests/golden/freerun_48000.f32");
}

TEST_CASE("golden: freerun replay matches reference @ 96k") {
	replayGolden(96000.0, "tests/golden/freerun_96000.f32");
}
#endif // __APPLE__
