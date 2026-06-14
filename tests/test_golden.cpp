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
// Cross-platform tolerance split (RESEARCH.md Pitfall 6 / Open Q1, resolved):
//   - The drift-on golden is BIT-EXACT only on the CANONICAL capture OS:
//         macOS (Apple clang / libc++)  -- see freerun_seeds.txt L38.
//     std::normal_distribution is NOT specified to be portable across
//     libstdc++ / libc++ / MSVC even given an identical Xoroshiro stream, so the
//     drift-on reference can differ in the low mantissa on the other CI legs.
//   - On non-canonical OSes the drift-on replay uses a 1e-5 ABSOLUTE tolerance
//     (~0.2 ppm of +/-5 V FS) — large enough to absorb normal_distribution /
//     transcendental-libm differences, small enough to catch any real regression.
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

// The golden is captured on macOS (Apple clang / libc++) — see freerun_seeds.txt.
// Bit-exact there; epsilon-tolerant on the other CI legs (drift-on path).
#if defined(__APPLE__)
constexpr bool   CANONICAL_OS  = true;
constexpr double GOLDEN_EPSILON = 0.0;       // bit-exact on the canonical capture OS
#else
constexpr bool   CANONICAL_OS  = false;
constexpr double GOLDEN_EPSILON = 1e-5;      // drift-on tolerance off the canonical OS
#endif

void replayGolden(double sr, const std::string& path) {
	auto ref = loadF32(path);
	REQUIRE(ref.size() == (size_t)GOLDEN_SAMPLES);

	forge::BlockDriver d(sr, DRIFT_S0, DRIFT_S1, SPREAD_S0, SPREAD_S1);
	forge::Inputs base = goldenBase();
	auto got = d.run((int)ref.size(), [&](int) { return base; });
	REQUIRE(got.size() == ref.size());

	if (CANONICAL_OS) {
		// Bit-exact replay on the canonical OS / drift-off everywhere. Use a
		// direct float == (NOT doctest::Approx, whose epsilon(0) still applies a
		// relative-scaling margin and is not a true bit-exact comparator).
		for (size_t i = 0; i < ref.size(); ++i) {
			CHECK(got[i] == ref[i]);
		}
	} else {
		// 1e-5 ABSOLUTE tolerance for the drift-on path on non-canonical OSes.
		for (size_t i = 0; i < ref.size(); ++i) {
			CHECK(std::fabs(got[i] - ref[i]) <= GOLDEN_EPSILON);
		}
	}
}

} // namespace

TEST_CASE("golden: freerun replay matches reference @ 44.1k") {
	replayGolden(44100.0, "tests/golden/freerun_44100.f32");
}

TEST_CASE("golden: freerun replay matches reference @ 48k") {
	replayGolden(48000.0, "tests/golden/freerun_48000.f32");
}

TEST_CASE("golden: freerun replay matches reference @ 96k") {
	replayGolden(96000.0, "tests/golden/freerun_96000.f32");
}
