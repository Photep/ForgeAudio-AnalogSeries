#pragma once
// src/dsp/RatioTable.hpp
//
// Pure musical-ratio tables + the division-aware beat-reset decision, lifted
// VERBATIM from src/AnalogLFO.cpp L42-65 (tables) and L520-524 (reset decision).
//
// Shape-for-P23 (D-04 / deferred): shouldReset is structured so a per-ratio
// BEATS_PER_ALIGN[15] table can replace the round(1/RATIO_TABLE[idx]) divisor
// math later WITHOUT an API change. The x1.5 / ÷1.5 alignment fix lands in
// Phase 23 — behavior here is UNCHANGED.
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes.

#include <cmath>

namespace forge {

// 15 musical ratios as frequency multipliers relative to clock frequency.
// AnalogLFO.cpp:43-59
static constexpr float RATIO_TABLE[15] = {
	1.f/16.f,   // /16  = 0.0625
	1.f/8.f,    // /8   = 0.125
	1.f/6.f,    // /6   = 0.166667
	1.f/4.f,    // /4   = 0.25
	1.f/3.f,    // /3   = 0.333333
	1.f/2.f,    // /2   = 0.5
	2.f/3.f,    // /1.5 = 0.666667
	1.f,        // x1   = 1.0
	3.f/2.f,    // x1.5 = 1.5
	2.f,        // x2   = 2.0
	3.f,        // x3   = 3.0
	4.f,        // x4   = 4.0
	6.f,        // x6   = 6.0
	8.f,        // x8   = 8.0
	16.f        // x16  = 16.0
};

// AnalogLFO.cpp:61-65
static constexpr const char* RATIO_LABELS[15] = {
	"/16", "/8", "/6", "/4", "/3", "/2", "/1.5",
	"x1",
	"x1.5", "x2", "x3", "x4", "x6", "x8", "x16"
};

// Division-aware phase-reset decision (RATE-04).
// AnalogLFO.cpp:520-524 — for a division ratio (RATIO_TABLE[idx] < 1) reset only
// once beatCount has reached the divisor; for ratios >= 1 reset every beat.
//
// D-04 / SHAPE-FOR-P23: the divisor below is currently derived as
// round(1/RATIO_TABLE[idx]). A future BEATS_PER_ALIGN[15] table will SLOT IN
// HERE to replace that divisor math (fixes x1.5/÷1.5 alignment in Phase 23)
// without changing this function's signature. Do NOT implement that fix here.
inline bool shouldReset(int ratioIdx, int beatCount) {
	bool reset = true;
	if (ratioIdx >= 0 && RATIO_TABLE[ratioIdx] < 1.f) {
		// FUTURE (P23): replace with `int divisor = BEATS_PER_ALIGN[ratioIdx];`
		int divisor = (int)std::round(1.f / RATIO_TABLE[ratioIdx]);
		reset = (beatCount >= divisor);
	}
	return reset;
}

} // namespace forge
