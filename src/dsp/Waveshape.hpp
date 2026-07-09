#pragma once
// src/dsp/Waveshape.hpp
//
// Pure waveshape math (sine/tri/saw/square/pulse + morph crossfade + bleed),
// lifted VERBATIM from src/AnalogLFO.cpp L194-388 (D-02: leaf dependency of the
// driveable LfoCore). The ONE Rack-state coupling — the bleed modulation that
// read the OU-layer-0 state at AnalogLFO.cpp:369 — is lifted to an explicit
// `bleedLfo` parameter (D-05 / Pitfall 3). All other math is unchanged: bit-
// identity to the inline copy is the load-bearing requirement.
//
// Spread members (triAsymmetrySpread, sawCurvatureSpread, squareDutySpread,
// pulseEdgeSpread, bleedSpread) are produced by DriftEngine/spread init, not the
// wave math, so they are carried as struct fields here (a passed Spread POD).
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes. Every free
// function is `inline` or a struct member (ODR, Pitfall 4).

#include <cmath>
#include <algorithm>

#include "dsp/MathConst.hpp"   // forge::kPi (D-06, rack-free pi constant)

#include "dsp/RackCompat.hpp"   // forge::clamp (where the inline code used rack::math::clamp)

namespace forge {

struct Waveshape {
	// Component spread coefficients (set by spread init / DriftEngine; default 0).
	float triAsymmetrySpread = 0.f;
	float sawCurvatureSpread = 0.f;
	float squareDutySpread = 0.f;
	float pulseEdgeSpread = 0.f;
	float bleedSpread = 0.f;

	// AnalogLFO.cpp:194-196
	float progressiveCurve(float character) const {
		return character * character;  // x^2: subtle first half, aggressive second half
	}

	// AnalogLFO.cpp:222-232
	float computeSine(float phase, float character) const {
		float sine = std::sin(2.f * (float)forge::kPi * phase);
		if (character < 0.001f) return sine;
		float c = progressiveCurve(character);
		// Triangle-derived analog sine: residual THD via Chebyshev polynomials
		float h2 = 2.f * sine * sine - 1.f;                     // T2: 2nd harmonic
		float h3 = 4.f * sine * sine * sine - 3.f * sine;       // T3: 3rd harmonic
		float thd3 = c * 0.08f;   // 8% 3rd harmonic at full character
		float thd2 = c * 0.03f;   // 3% 2nd harmonic at full character
		return sine + thd3 * h3 + thd2 * h2;
	}

	// AnalogLFO.cpp:234-260
	float computeTriangle(float phase, float character) const {
		// Digital: peak (+1) at phase=0 and 1, valley (-1) at phase=0.5
		float tri = 2.f * std::fabs(2.f * phase - 1.f) - 1.f;
		if (character < 0.001f) return tri;
		float c = progressiveCurve(character);
		// Slope asymmetry: valley shifts slightly right (falling slope longer)
		float asymmetry = c * (0.10f + triAsymmetrySpread);
		float valley = 0.5f + asymmetry * 0.5f;
		float analogTri;
		if (phase < valley) {
			// Falling from +1 (phase=0) to -1 (phase=valley)
			analogTri = 1.f - 2.f * phase / valley;
		} else {
			// Rising from -1 (phase=valley) to +1 (phase=1)
			analogTri = -1.f + 2.f * (phase - valley) / (1.f - valley);
		}
		// Rounded peaks via sinusoidal smoothing
		float roundAmount = c * 0.35f;
		if (analogTri > (1.f - roundAmount)) {
			float t = (analogTri - (1.f - roundAmount)) / roundAmount;
			analogTri = (1.f - roundAmount) + roundAmount * std::sin(t * (float)forge::kPi * 0.5f);
		} else if (analogTri < -(1.f - roundAmount)) {
			float t = (-(1.f - roundAmount) - analogTri) / roundAmount;
			analogTri = -(1.f - roundAmount) - roundAmount * std::sin(t * (float)forge::kPi * 0.5f);
		}
		return analogTri;
	}

	// AnalogLFO.cpp:262-278
	float computeSaw(float phase, float character) const {
		float saw = 1.f - 2.f * phase;  // falling ramp (Minimoog convention)
		if (character < 0.001f) return saw;
		float c = progressiveCurve(character);
		// Exponential ramp curvature (50% blend toward exponential at full character)
		float expRamp = 1.f - 2.f * (1.f - std::exp(-3.f * phase)) / (1.f - std::exp(-3.f));
		float curvedSaw = saw + c * (0.5f + sawCurvatureSpread) * (expRamp - saw);
		// Soft capacitor reset (~8% of cycle at full character)
		float resetWidth = c * 0.08f;
		if (phase < resetWidth && resetWidth > 0.001f) {
			float t = phase / resetWidth;
			float smoothT = 0.5f - 0.5f * std::cos(t * (float)forge::kPi);
			float resetValue = 1.f;
			curvedSaw = resetValue + smoothT * (curvedSaw - resetValue);
		}
		return curvedSaw;
	}

	// AnalogLFO.cpp:280-301
	float computeSquare(float phase, float character) const {
		// Digital: +1 for phase<0.5, -1 for phase>=0.5
		float sqr = (phase < 0.5f) ? 1.f : -1.f;
		if (character < 0.001f) return sqr;
		float c = progressiveCurve(character);
		// Duty cycle asymmetry (4% at full)
		float duty = 0.5f + c * (0.04f + squareDutySpread);
		// Sigmoid edge softening via tanh (~8% edge width at full)
		float edgeWidth = c * 0.08f;
		float sharpness = 1.f / std::fmax(edgeWidth, 0.001f);
		// Soft square: +1 region [0, duty], -1 region [duty, 1]
		// Use distance from center of +1 region with wrap-aware calculation
		float center = duty * 0.5f;
		float halfWidth = duty * 0.5f;
		float d = phase - center;
		if (d > 0.5f) d -= 1.f;
		if (d < -0.5f) d += 1.f;
		float dist = halfWidth - std::fabs(d);
		float analog = std::tanh(sharpness * dist);
		// Crossfade: prevents snap at low character values
		return sqr + c * (analog - sqr);
	}

	// AnalogLFO.cpp:303-329
	float computePulse(float phase, float character, float duty) const {
		// Digital: +1 for phase < duty, -1 for phase >= duty (D-07)
		float pulse = (phase < duty) ? 1.f : -1.f;
		if (character < 0.001f) return pulse;
		float c = progressiveCurve(character);

		// Tanh edge softening (D-05): same approach as computeSquare
		// Scale edge width to avoid exceeding narrow pulse region (Pitfall 1)
		float maxEdge = std::fmin(duty, 1.f - duty) * 0.8f;
		float edgeWidth = c * std::fmin(0.08f, maxEdge);
		float sharpness = 1.f / std::fmax(edgeWidth, 0.001f);

		// Component spread affects edge softening intensity only (D-06)
		sharpness *= (1.f + pulseEdgeSpread);

		// Soft pulse: +1 region [0, duty], -1 region [duty, 1]
		float center = duty * 0.5f;
		float halfWidth = duty * 0.5f;
		float d = phase - center;
		if (d > 0.5f) d -= 1.f;
		if (d < -0.5f) d += 1.f;
		float dist = halfWidth - std::fabs(d);
		float analog = std::tanh(sharpness * dist);

		// Crossfade: prevents snap at low character values
		return pulse + c * (analog - pulse);
	}

	// AnalogLFO.cpp:331-388 — computeMorphedWave, renamed morphedWave.
	// THE D-05 LIFT: the bleed modulation that read the OU-layer-0 state at
	// AnalogLFO.cpp:369 now reads the explicit `bleedLfo` parameter. Passing
	// bleedLfo=0 reproduces the no-drift bleed path.
	float morphedWave(float phase, float morph, float character, float bleedLfo) const {
		float sine = computeSine(phase, character);
		float tri  = computeTriangle(phase, character);
		float saw  = computeSaw(phase, character);
		float sqr  = computeSquare(phase, character);

		// D-01: morph * 4.0 gives 5 shapes across [0, 1], each shape 20% of knob range
		float scaled = morph * 4.f;
		int segment = std::min((int)scaled, 3);  // 0-3 (4 segments for 5 shapes)
		float frac = scaled - (float)segment;

		// D-08: duty = 0.50 - 0.45 * frac, computed from morph position in pulse region
		float pulseFrac = std::fmax(0.f, scaled - 3.f);  // 0 at square/pulse boundary, 1 at morph=1
		float pulseDuty = 0.50f - 0.45f * std::fmin(pulseFrac, 1.f);  // D-07: 50% to 5%
		float pls  = computePulse(phase, character, pulseDuty);

		// D-03: Sine -> Tri -> Saw -> Square -> Pulse
		float shapes[5] = { sine, tri, saw, sqr, pls };

		// Primary crossfade — duty interpolation for square-to-pulse region
		float result;
		if (segment == 3) {
			// Square-to-pulse: pulseDuty already varies 0.50->0.05 with morph position
			// Direct duty interpolation avoids staircase artifact from crossfading two rectangles
			result = pls;
		} else {
			result = shapes[segment] + frac * (shapes[segment + 1] - shapes[segment]);
		}

		// Waveform bleed: adjacent-shape crosstalk (CHAR-05)
		if (character >= 0.001f) {
			float c = progressiveCurve(character);

			// Base bleed magnitude (4%) with component spread offset, clamped non-negative
			float effectiveBleed = std::fmax(0.f, 0.04f + bleedSpread);
			float bleedIntensity = c * effectiveBleed;

			// Slow modulation from existing OU layer 0 (~20s cycle, +/-20% fluctuation)
			// D-05: bleedLfo replaces the direct OU-layer-0 state read (AnalogLFO.cpp:369).
			bleedIntensity *= (1.f + bleedLfo * 0.2f);
			bleedIntensity = std::fmax(0.f, bleedIntensity);  // ensure non-negative after modulation

			// D-04: Neighbor identification (wrapping ring: sine-tri-saw-sqr-pulse-sine)
			int leftIdx  = (segment - 1 + 5) % 5;   // shape left of segment start
			int rightIdx = (segment + 2) % 5;        // shape right of segment end

			// Proximity weighting: closer neighbor bleeds more
			float leftWeight  = 1.f - frac;   // frac=0 -> full left bleed
			float rightWeight = frac;          // frac=1 -> full right bleed

			float bleedSignal = leftWeight * shapes[leftIdx] + rightWeight * shapes[rightIdx];
			result += bleedIntensity * bleedSignal;

			// Normalize to maintain +/-1 range (prevents >+/-5V after 5V scaling)
			result /= (1.f + bleedIntensity);
		}

		return result;
	}
};

} // namespace forge
