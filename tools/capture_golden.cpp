// tools/capture_golden.cpp
//
// D-07 CROSS-PLATFORM GOLDEN GENERATOR (one-shot; NOT part of `make test`).
//
// Emits the drift-OFF + spread-OFF reference blocks the cross-platform CI leg
// replays: tests/golden/freerun_<rate>_driftoff.f32. These fixtures are the
// portable half of TEST-06 — deterministic across libc++/libstdc++/MinGW because
// they exercise ONLY the portable Xoroshiro uniform + libm sin/cos, never the
// non-portable std::normal_distribution drift/spread path.
//
// CRITICAL LANDMINE (RESEARCH.md Pitfall 1 / spread neutralization): turning drift
// off is NOT sufficient. tests/BlockDriver.hpp's constructor unconditionally seeds
// the component-spread path, which draws from std::normal_distribution and perturbs
// the waveform even with drift off. This generator therefore does NOT use that
// constructor — it constructs forge::LfoCore directly and seeds ONLY the drift RNG
// (core.seed). The spread coefficients (*Spread) stay at their 0.f defaults, so the
// output is fully portable. Miss this and Linux/Windows stay red.
//
// Build:  make capture   (Rack-free; same TEST_CXXFLAGS as `make test`, so the
//                          captured bytes are bit-identical to what make test replays)
// Run from the repo root — output paths are relative to CWD.

#include "dsp/LfoCore.hpp"

#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <cstdio>

namespace {

// Exact drift seeds from tests/golden/freerun_seeds.txt (drift RNG only).
constexpr uint64_t DRIFT_S0 = 0x0000000000C0FFEEULL;
constexpr uint64_t DRIFT_S1 = 0x000000000BADF00DULL;
constexpr int      GOLDEN_SAMPLES = 8192;   // -> 32768 bytes per fixture (LE float32)

// Canonical free-run scenario, drift OFF. Mirrors goldenBase() in test_golden.cpp
// EXCEPT in.drift = 0.0f (and the caller never seeds the spread path).
forge::Inputs goldenBaseDriftOff() {
	forge::Inputs in;
	in.rate        = 2.0f;
	in.morph       = 0.4f;
	in.character   = 0.6f;
	in.drift       = 0.0f;   // drift OFF -> deterministic, portable output
	in.phaseOffset = 0.f;
	in.swingIndex  = 0;
	return in;
}

// Drive GOLDEN_SAMPLES through a freshly-seeded core and write little-endian
// float32 frames (exact byte-inverse of test_golden.cpp's loadF32 reader).
void capture(double sr, const std::string& path) {
	forge::LfoCore core;
	core.seed(DRIFT_S0, DRIFT_S1);   // seed ONLY the drift RNG; leave *Spread at 0.f defaults

	const float dt = (float)(1.0 / sr);
	std::vector<float> out;
	out.reserve(GOLDEN_SAMPLES);
	forge::Inputs base = goldenBaseDriftOff();
	for (int i = 0; i < GOLDEN_SAMPLES; ++i) {
		forge::Inputs in = base;
		in.sampleTime = dt;
		out.push_back(core.step(in));
	}

	std::ofstream f(path, std::ios::binary);
	if (!f) {
		std::fprintf(stderr, "capture_golden: cannot open %s for writing\n", path.c_str());
		return;
	}
	for (float x : out) f.write(reinterpret_cast<const char*>(&x), sizeof x);
	std::printf("wrote %s (%zu samples, %zu bytes)\n",
	            path.c_str(), out.size(), out.size() * sizeof(float));
}

} // namespace

int main() {
	capture(44100.0, "tests/golden/freerun_44100_driftoff.f32");
	capture(48000.0, "tests/golden/freerun_48000_driftoff.f32");
	capture(96000.0, "tests/golden/freerun_96000_driftoff.f32");
	return 0;
}
