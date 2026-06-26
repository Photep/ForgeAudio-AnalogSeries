// tests/test_anim.cpp
//
// CLEAN-04 / D-06 — headless pin for the frame-rate-independent animation math
// extracted to src/dsp/Anim.hpp. Two properties are proven without Rack:
//   1. CLAMP: a huge frame stall caps to one nominal step (kMaxFrameDt = 1/30),
//      and a non-finite (first-frame NAN) or negative dt yields 0 (no advance) —
//      the explicit isfinite guard, NOT rack::math::clamp which returns the hi
//      bound on NAN (RESEARCH Pitfall 1). A pathological dt advances the geometric
//      decay by no more than the clamped step.
//   2. 60fps EQUIVALENCE: flashDecay(1, 1/60) reproduces the old `*= 0.92f` decay
//      (D-03 feel-equivalence) — compared with doctest::Approx, NEVER `==`, since
//      pow(0.92f, 1.f) differs from the literal 0.92f by <=1 ULP (Pitfall 5).
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "dsp/Anim.hpp"   // -Isrc resolves "dsp/..."

#include <cmath>

TEST_CASE("anim: pathological dt clamps to one cap step") {
	CHECK(forge::clampFrameDt(10.0f)         == doctest::Approx(1.f / 30.f)); // huge stall -> cap
	CHECK(forge::clampFrameDt(std::nanf("")) == 0.f);                         // first-frame NAN -> 0
	CHECK(forge::clampFrameDt(-1.f)          == 0.f);                         // negative -> 0
	// a huge dt advances the decay by NO MORE than the clamped step:
	CHECK(forge::flashDecay(1.f, forge::clampFrameDt(10.f))
	      == forge::flashDecay(1.f, 1.f / 30.f));
}

TEST_CASE("anim: decay is feel-identical at 60fps") {
	CHECK(forge::flashDecay(1.f, 1.f / 60.f) == doctest::Approx(0.92f));  // matches old *=0.92, never ==
}
