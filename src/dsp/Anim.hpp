#pragma once
// src/dsp/Anim.hpp
//
// Frame-rate-independent animation math (CLEAN-04), converting the frame-paced
// accumulators/decay in the WaveformDisplay widget at src/AnalogLFO.cpp:388-429
// to wall-clock dt. Two helpers:
//   - clampFrameDt: the dt sanitizer. Window::getLastFrameDuration() returns NAN
//     before the first finished frame; a negative/garbage dt must not advance an
//     animation either. NOTE the explicit finite/negative guard (see the function
//     below) is mandatory and runs BEFORE the cap: the Rack clamp helper returns HI
//     bound 1/30 on a NAN input, not 0, which would pop every animation on frame
//     1 (RESEARCH Pitfall 1). This header therefore has ZERO Rack includes.
//   - flashDecay: the SYNC-badge geometric decay. The ANIM-02 0.92 per-frame
//     factor is preserved by mathematical equivalence (D-03) — pow(0.92, dt*60)
//     equals the old `*= 0.92f` at 60fps and is feel-identical at any refresh
//     rate. It is NEVER re-tuned.
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes. Every free
// function is `inline` and the cap is `constexpr` (ODR, Pitfall 6 — included by
// the shell AND tests/test_anim.cpp).

#include <cmath>

namespace forge {

// D-04 clamp ceiling: ~2 nominal 60fps frames. A longer stall caps to one step.
constexpr float kMaxFrameDt = 1.f / 30.f;

// Sanitize a raw getLastFrameDuration(): non-finite (first-frame NAN) or negative
// -> 0 (no advance); otherwise cap at kMaxFrameDt. The finite/negative guard MUST
// precede the cap (the Rack clamp helper returns its hi bound on NAN — Pitfall 1).
inline float clampFrameDt(float raw) {
	if (!std::isfinite(raw) || raw < 0.f) return 0.f;
	return raw < kMaxFrameDt ? raw : kMaxFrameDt;
}

// Continuous-time SYNC-badge decay. pow(0.92, dt*60) == 0.92 at dt = 1/60, so the
// ANIM-02 factor is preserved by equivalence (D-03), never re-tuned.
inline float flashDecay(float intensity, float dt) {
	return intensity * std::pow(0.92f, dt * 60.f);
}

} // namespace forge
