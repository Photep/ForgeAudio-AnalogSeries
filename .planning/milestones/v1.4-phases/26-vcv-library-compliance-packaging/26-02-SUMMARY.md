---
phase: 26-vcv-library-compliance-packaging
plan: 02
subsystem: dsp-portability
tags: [portability, ci, refactor, rack-free]
requires:
  - src/dsp/Swing.hpp (header skeleton analog)
provides:
  - src/dsp/MathConst.hpp (forge::kPi rack-free pi constexpr)
affects:
  - src/dsp/Waveshape.hpp
  - src/dsp/DriftEngine.hpp
  - src/dsp/LfoCore.hpp
  - src/AnalogLFO.cpp
  - tests/test_extraction.cpp
tech-stack:
  added: []
  patterns:
    - "inline constexpr (C++17) ODR-safe shared constant header"
    - "rack-free src/dsp/*.hpp hygiene (zero SDK includes)"
key-files:
  created:
    - src/dsp/MathConst.hpp
  modified:
    - src/dsp/Waveshape.hpp
    - src/dsp/DriftEngine.hpp
    - src/dsp/LfoCore.hpp
    - src/AnalogLFO.cpp
    - tests/test_extraction.cpp
decisions:
  - "kPi literal 3.14159265358979323846 chosen bit-for-bit IEEE-754 identical to M_PI (0x400921FB54442D18) so golden fixtures are unperturbed"
  - "MathConst.hpp comment avoids the literal M_PI token so the zero-hit grep gate stays absolute"
metrics:
  duration: ~5m
  completed: 2026-07-09
  tasks: 2
  files: 6
requirements: [TEST-06]
---

# Phase 26 Plan 02: M_PI → forge::kPi Portability Refactor Summary

Replaced the non-standard `M_PI` macro tree-wide with a rack-free `forge::kPi` constexpr, removing the Windows CI direct-g++ compile failure root cause (D-06) with zero shipped-audio change.

## What Was Built

- **`src/dsp/MathConst.hpp` (NEW):** A pure, dependency-free constant header declaring `inline constexpr double kPi = 3.14159265358979323846;` inside `namespace forge`. Follows the `src/dsp/Swing.hpp` skeleton (`#pragma once` + banner + zero-include hygiene). The literal is bit-for-bit IEEE-754 identical to cmath's pi macro (`0x400921FB54442D18`), so golden fixtures are not perturbed. The header comment deliberately refers to the value only via the hex identity and "cmath's pi macro" phrasing — never the raw macro token — so Task 2's zero-hit grep gate remains absolute.
- **22 `M_PI` → `forge::kPi` substitutions across 5 files:** Each consumer now `#include "dsp/MathConst.hpp"` and uses `forge::kPi`. Only the token changed; all `(float)`, `2.f *`, `-0.75 *`, and comment scaffolding is byte-identical.
  - `src/dsp/Waveshape.hpp` — 4 sites
  - `src/dsp/DriftEngine.hpp` — 5 sites (the `std::normal_distribution` D-07 spread landmine at L47/L99 left untouched — owned by plan 26-03)
  - `src/dsp/LfoCore.hpp` — 1 site
  - `src/AnalogLFO.cpp` — 11 sites (float-cast scaling + bare-double knob-angle shapes)
  - `tests/test_extraction.cpp` — 1 site

## Verification

- `grep -rn M_PI src/ tests/` → zero hits (including the new MathConst.hpp).
- `make test` exits 0 on macOS: **47 test cases / 2,590,445 assertions passed, 0 failed** — bit-exact against the existing `freerun_*.f32` drift-on goldens (no golden regeneration).
- `MathConst.hpp` verified rack-free (zero `#include`) with the exact kPi literal.
- `std::normal_distribution` lines in `DriftEngine.hpp` confirmed unchanged.

## Deviations from Plan

None — plan executed exactly as written. The `std::normal_distribution` reference lines report as L47/L99 rather than the plan's cited L46/L98; this is solely because adding the `#include "dsp/MathConst.hpp"` line shifted subsequent lines down by one. The lines themselves are unmodified.

## Commits

- `261d4f2` feat(26-02): add rack-free forge::kPi constexpr header
- `e2ba9ed` refactor(26-02): replace 22 M_PI uses with rack-free forge::kPi

## Requirements

- **TEST-06** — Contributes the D-06 half (Windows direct-g++ `M_PI`-undeclared root cause removed). The D-05 drift-off fixture half is handled by a sibling plan.

## Self-Check: PASSED

- FOUND: src/dsp/MathConst.hpp
- FOUND commit: 261d4f2
- FOUND commit: e2ba9ed
- grep M_PI src/ tests/ → empty
- make test → exit 0, bit-exact green
