#pragma once
// src/dsp/MathConst.hpp
//
// Rack-free shared math constants (D-06). Replaces the non-standard cmath pi
// macro so the direct-g++ Windows CI test leg compiles without -D_USE_MATH_DEFINES.
//
// Include hygiene: ZERO Rack-SDK includes (matches every other src/dsp/*.hpp).

namespace forge {
// Bit-for-bit identical IEEE-754 double to cmath's pi macro (hex identity 0x400921FB54442D18),
// so (float)kPi and (double)kPi equal cmath's pi value exactly. Golden fixtures unperturbed.
// Plain constexpr (internal linkage per TU), NOT `inline constexpr`: inline variables are
// C++17 and the Rack toolchain builds with -std=c++11.
constexpr double kPi = 3.14159265358979323846;
} // namespace forge
