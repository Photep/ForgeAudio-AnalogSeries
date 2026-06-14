#pragma once
// src/dsp/RackCompat.hpp
//
// forge:: re-implementations of the Rack SDK primitives the inline DSP in
// AnalogLFO.cpp consumes. These are bit-identical to the shipped behavior
// (D-06: forge:: equivalents of the Rack DSP types; D-07: Xoroshiro128+
// re-implemented bit-identical to the shipped plugin). COPIED VERBATIM from
// 22-RESEARCH.md L293-396 (do NOT re-derive from the SDK).
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes — this header
// compiles under the Rack-free `make test` target with no Rack include path.

#include <cstdint>
#include <cmath>
#include <algorithm>

namespace forge {

// Xoroshiro128Plus
// Source: VERBATIM from ../Rack-SDK/include/random.hpp:26-70.
// Bit-identical to rack::random::Xoroshiro128Plus — required by D-07.
struct Xoroshiro128Plus {
	using result_type = uint64_t;
	uint64_t state[2] = {};
	Xoroshiro128Plus() {}
	explicit Xoroshiro128Plus(uint64_t s0, uint64_t s1 = 0) { seed(s0, s1); }
	void seed(uint64_t s0, uint64_t s1 = 0) {
		state[0] = s0; state[1] = s1;
		operator()();                 // shift a bad seed, exactly as Rack does
	}
	static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
	uint64_t operator()() {
		uint64_t s0 = state[0], s1 = state[1];
		uint64_t result = s0 + s1;
		s1 ^= s0;
		state[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14);
		state[1] = rotl(s1, 36);
		return result;
	}
	static constexpr uint64_t min() { return 0; }
	static constexpr uint64_t max() { return UINT64_MAX; }  // required for std::normal_distribution
};

// SchmittTrigger — float specialization (digital.hpp:82-161).
// UNINITIALIZED handling is load-bearing.
struct SchmittTrigger {
	enum State : uint8_t { LOW, HIGH, UNINITIALIZED };
	State s = UNINITIALIZED;
	void reset() { s = UNINITIALIZED; }
	bool process(float in, float lowThreshold = 0.f, float highThreshold = 1.f) {
		if (s == LOW && in >= highThreshold)        { s = HIGH; return true; }
		else if (s == HIGH && in <= lowThreshold)   { s = LOW; }
		else if (s == UNINITIALIZED && in >= highThreshold) { s = HIGH; }
		else if (s == UNINITIALIZED && in <= lowThreshold)  { s = LOW; }
		return false;
	}
	bool isHigh() { return s == HIGH; }
};

// Timer (digital.hpp:199-218)
struct Timer {
	float time = 0.f;
	void reset() { time = 0.f; }
	float process(float dt) { time += dt; return time; }
	float getTime() { return time; }
};

// PulseGenerator (digital.hpp:167-195)
// Note: trigger() keeps the LONGER of existing/new remaining time.
struct PulseGenerator {
	float remaining = 0.f;
	void reset() { remaining = 0.f; }
	bool process(float dt) { if (remaining > 0.f) { remaining -= dt; return true; } return false; }
	bool isHigh() { return remaining > 0.f; }
	void trigger(float duration = 1e-3f) { if (duration > remaining) remaining = duration; }
};

// OnePole (== rack::dsp::TExponentialFilter<float>, filter.hpp:59).
// The snap-to-input (out = (out == y) ? in : y) is load-bearing for golden
// bit-identity (Pitfall 5) — DO NOT remove.
struct OnePole {
	float out = 0.f, lambda = 0.f;
	void reset() { out = 0.f; }
	void setLambda(float l) { lambda = l; }
	void setTau(float tau) { lambda = 1.f / tau; }
	float process(float dt, float in) {
		float y = out + (in - out) * lambda * dt;
		out = (out == y) ? in : y;   // granularity snap — Pitfall 5
		return out;
	}
};

// clamp — replaces rack::math::clamp. Written without std::clamp so the header
// compiles under both the C++17 test target AND the plugin's C++11 toolchain
// (the plugin now includes this header via LfoCore). Bit-identical to the inline
// std::clamp/rack::math::clamp result for finite inputs.
inline float clamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline int   clampi(int x, int lo, int hi)       { return x < lo ? lo : (x > hi ? hi : x); }

// exp2_taylor5 — Source: ../Rack-SDK/include/dsp/approx.hpp (float path only).
// Needed bit-identical for the FM path (Pitfall 2). The int32-reinterpret bit
// trick in exp2Floor and the Horner order in exp2_taylor5 are VERIFIED
// bit-identical to the SDK polyHorner (22-RESEARCH.md L396).
inline float exp2Floor(float x, float* xf) {
	x += 127.f;
	int32_t xi = (int32_t)x;
	if (xf) *xf = x - (float)xi;
	union { float yi; int32_t yii; };
	yii = xi << 23;
	return yi;
}
inline float exp2_taylor5(float x) {
	float xf;
	float yi = exp2Floor(x, &xf);
	// polyHorner over a[6], evaluated high-to-low (matches approx.hpp:polyHorner)
	const float a[6] = {1.0f, 0.69315169353961f, 0.2401595990753f,
	                    0.055817908652f, 0.008991698010f, 0.001879100722f};
	float yf = a[5];
	for (int i = 4; i >= 0; --i) yf = yf * xf + a[i];  // Horner, same order as polyHorner
	return yi * yf;
}

} // namespace forge
