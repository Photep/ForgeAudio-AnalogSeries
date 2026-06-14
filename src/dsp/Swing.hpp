#pragma once
// src/dsp/Swing.hpp
//
// Swing-fraction presets + the clocked phase-warp multiplier, lifted VERBATIM
// from src/AnalogLFO.cpp L68-75 (SWING_FRACTIONS) and L767-773 (the warp).
//
// Behavioral gates preserved exactly (do NOT change behavior here — the P23
// free-run swing-desync fix only SHAPES this API later):
//   - isClocked gate (PHASE-04: swing inactive in free-run) → returns 1.0
//   - the `> 0.5001f` straight-mode fast path → returns 1.0
// Multiplier uses double precision, matching the inline code.
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes.

namespace forge {

// AnalogLFO.cpp:68-75 — swing presets (PHASE-03).
static constexpr float SWING_FRACTIONS[6] = {
	0.50f,   // Straight
	0.54f,   // Light
	0.58f,   // Medium
	0.66f,   // Triplet (2/3)
	0.71f,   // Heavy
	0.75f    // Max (3/4)
};

// Swing phase-warp multiplier for clocked mode (PHASE-03 / PHASE-04).
// AnalogLFO.cpp:767-773 — even beat (first half, phase < 0.5) accumulates
// slower (longer duration); odd beat (second half) accumulates faster.
// Returns 1.0 (no warp) when free-running (!isClocked) or in straight mode
// (swingFrac <= 0.5001f). The deltaPhase multiply at the call site becomes
//   deltaPhase *= swingPhaseMultiplier(phase, swingFrac, isClocked);
inline double swingPhaseMultiplier(double phase, float swingFrac, bool isClocked) {
	if (isClocked && swingFrac > 0.5001f) {
		return (phase < 0.5)
			? (0.5 / (double)swingFrac)
			: (0.5 / (1.0 - (double)swingFrac));
	}
	return 1.0;
}

} // namespace forge
