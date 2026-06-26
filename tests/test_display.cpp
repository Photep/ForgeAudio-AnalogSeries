// tests/test_display.cpp
//
// CLEAN-05 / D-06 — headless pin for the pure preview-fill helper extracted to
// src/dsp/DisplayFill.hpp. Two properties are proven without Rack:
//   1. PURITY / thread-independence: identical (snapshot, wave-spreads) -> a
//      bit-identical buffer across repeated calls in THIS binary, so moving the
//      same compiled fill from the audio thread to the GUI thread cannot change
//      output (D-06 scoped to same-binary determinism — RESEARCH Pitfall 7).
//   2. SWING BOUNDARY: the `swingFrac <= 0.5001f` fast path and the 0.5001
//      boundary take the same no-warp path, element-for-element.
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it;
// defining it twice = duplicate-symbol link error).
//
// Bit-exact-within-binary only (Pitfall 7): direct `==`, NOT doctest::Approx.

#include "doctest.h"

#include "dsp/DisplayFill.hpp"   // -Isrc resolves "dsp/..."
#include "dsp/LfoCore.hpp"

#include <array>

TEST_CASE("display: fill is a pure, thread-independent function of the snapshot") {
	forge::LfoCore core;
	core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);  // canonical golden spread seeds
	std::array<float, forge::DISPLAY_SAMPLES> a{}, b{};
	// Same snapshot, two calls — emulates "audio-thread fill" vs "GUI-thread fill".
	forge::fillDisplayBuffer(a, core.wave, 0.4f, 0.6f, 0.58f, 0.123f);
	forge::fillDisplayBuffer(b, core.wave, 0.4f, 0.6f, 0.58f, 0.123f);
	for (int i = 0; i < forge::DISPLAY_SAMPLES; ++i)
		CHECK(a[i] == b[i]);  // bit-exact within this binary
}

TEST_CASE("display: swing remap matches the no-swing fast path at Straight") {
	forge::Waveshape w;  // zero spreads
	std::array<float, forge::DISPLAY_SAMPLES> straight{}, gated{};
	forge::fillDisplayBuffer(straight, w, 0.5f, 0.3f, 0.50f,   0.f);  // <=0.5001 fast path
	forge::fillDisplayBuffer(gated,    w, 0.5f, 0.3f, 0.5001f, 0.f);  // boundary
	for (int i = 0; i < forge::DISPLAY_SAMPLES; ++i)
		CHECK(straight[i] == gated[i]);
}
