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

// Beats-per-alignment for each ratio (BUG-02 / SC4, Phase 23).
// Each ratio p/q (lowest terms) returns to phase 0 every q beats, so resetting
// once beatCount reaches BEATS_PER_ALIGN[idx] gives whole LFO cycles with NO
// mid-cycle truncation. Verified mathematically (lowest-terms q). This replaces
// the old round(1/RATIO_TABLE[idx]) divisor, which only handled division ratios
// and truncated the non-integer ratios x1.5/÷1.5 mid-cycle.
//
// Per the logged audition DECISION: adopt-table (.planning/STATE.md ### Decisions):
// only idx 6 (/1.5: 2→3) and idx 8 (x1.5: 1→2) change; the 13 other ratios are
// bit-identical to the prior round(1/ratio) cadence.
static constexpr int BEATS_PER_ALIGN[15] = {
//  /16 /8 /6 /4 /3 /2 /1.5 x1 x1.5 x2 x3 x4 x6 x8 x16
	16, 8, 6, 4, 3, 2,  3,  1,  2,  1, 1, 1, 1, 1, 1
};

// Phase-reset decision (RATE-04 / BUG-02). Reset exactly on the per-ratio
// alignment boundary; unlocked (ratioIdx < 0) resets every beat (unchanged).
// Signature FROZEN — ClockTracker.hpp delegates here (CR-03, single home).
inline bool shouldReset(int ratioIdx, int beatCount) {
	if (ratioIdx < 0) return true;                  // unlocked: every beat
	return beatCount >= BEATS_PER_ALIGN[ratioIdx];  // div AND non-int ratios uniform
}

} // namespace forge
