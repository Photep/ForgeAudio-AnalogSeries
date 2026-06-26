#pragma once
// src/dsp/DisplayFill.hpp
//
// Pure single-cycle preview-fill loop, lifted VERBATIM from the inline
// AnalogLFO::updateDisplayBuffer body at src/AnalogLFO.cpp:169-185 (CLEAN-05 /
// D-06). The loop math — the uniform `t`, the `swingFrac <= 0.5001f` straight
// fast path, and the two-half even/odd remap — is unchanged. The ONE live-state
// coupling — the inline preview read the live OU-layer-0 drift state as the bleed
// modulation at :184 — is lifted to an explicit `bleedLfo` PARAMETER (mirrors the
// D-05 lift already done in src/dsp/Waveshape.hpp:156). Making bleedLfo a
// parameter structurally prevents the GUI fill from reading live drift at paint
// time (D-02 / RESEARCH Pitfall 3): the function is pure and thread-independent.
//
// The writeIdx / displayReadIdx double-buffer book-keeping stays shell-side (it
// wraps this call in fillFromSnapshot); only the fill is extracted here.
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes. The free
// function is `inline` and DISPLAY_SAMPLES is `constexpr` (ODR, Pitfall 6 — this
// header is included by the shell AND tests/test_display.cpp).

#include <array>

#include "dsp/Waveshape.hpp"   // forge::Waveshape::morphedWave (bleedLfo already a param, D-05)

namespace forge {

// Single-source the sample count (Pitfall 6); the shell aliases its own
// DISPLAY_SAMPLES to this so the 256 literal lives in one place.
constexpr int DISPLAY_SAMPLES = 256;

// Pure: identical (snapshot, wave-spreads) -> identical buffer, on ANY thread.
// No live/global state — bleedLfo is a parameter, so the GUI fill structurally
// CANNOT read the live OU-layer-0 drift state at paint time (D-02).
inline void fillDisplayBuffer(std::array<float, DISPLAY_SAMPLES>& out,
                              const Waveshape& wave,
                              float morph, float character,
                              float swingFrac, float bleedLfo) {
	for (int i = 0; i < DISPLAY_SAMPLES; ++i) {
		float t = (float)i / (float)DISPLAY_SAMPLES;  // uniform time
		float p;
		if (swingFrac <= 0.5001f) {
			p = t;  // fast path: no swing
		} else if (t < swingFrac) {
			p = t * 0.5f / swingFrac;                              // even: [0,S) -> [0,0.5)
		} else {
			p = 0.5f + (t - swingFrac) * 0.5f / (1.f - swingFrac); // odd: [S,1) -> [0.5,1)
		}
		out[i] = wave.morphedWave(p, morph, character, bleedLfo);
	}
}

} // namespace forge
