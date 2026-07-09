---
phase: 26-vcv-library-compliance-packaging
plan: 03
subsystem: testing
tags: [golden-fixtures, cross-platform, ci, doctest, xoroshiro, libm, drift]

# Dependency graph
requires:
  - phase: 22-dsp-extraction
    provides: forge::LfoCore Rack-free core + BlockDriver harness + freerun_*.f32 golden fixtures
provides:
  - Cross-platform drift-off + spread-off golden fixtures (44.1/48/96 kHz)
  - Rack-free one-shot fixture generator (tools/capture_golden.cpp + make capture)
  - Portable golden replay leg (drift-off, all OSes) + macOS-gated drift-on leg
affects: [ci, vcv-library-submission, dsp-regression-testing]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Spread-path neutralization: seed only the drift RNG (core.seed), never seed the spread coefficients, so *Spread stay 0.f and output is portable"
    - "Two-leg golden: cross-platform drift-off guard (all OSes, 1e-6 abs eps) + macOS-gated drift-on bit-exact leg"
    - "Fixtures captured with the identical TEST_CXXFLAGS (-ffp-contract=off) they are replayed with"

key-files:
  created:
    - tools/capture_golden.cpp
    - tests/golden/freerun_44100_driftoff.f32
    - tests/golden/freerun_48000_driftoff.f32
    - tests/golden/freerun_96000_driftoff.f32
  modified:
    - Makefile
    - tests/test_golden.cpp
    - tests/golden/freerun_seeds.txt

key-decisions:
  - "Drift-off leg uses 1e-6 absolute libm epsilon (portable Xoroshiro uniform + libm sin/cos low-mantissa divergence); well below drift depth"
  - "Drift-on bit-exact replay gated to #if defined(__APPLE__) rather than widening its epsilon to absorb non-portable std::normal_distribution"
  - "Generator constructs forge::LfoCore directly instead of BlockDriver, whose ctor unconditionally seeds the spread path"

patterns-established:
  - "Spread neutralization (Pitfall 1): drift=0.0 alone is insufficient — the spread path must also stay unseeded"
  - "make capture: Rack-free auxiliary target added to the plugin.mk skip filter, mirroring make test"

requirements-completed: [TEST-06]

# Metrics
duration: ~12min
completed: 2026-07-09
---

# Phase 26 Plan 03: Cross-Platform Golden Restructure (D-07) Summary

**Portable drift-off + spread-off golden fixtures with a Rack-free generator, making the cross-platform CI golden leg green on Linux/Windows while keeping the non-portable drift-on bit-exact replay macOS-gated.**

## Performance

- **Duration:** ~12 min
- **Tasks:** 3
- **Files modified:** 7 (4 created, 3 modified)

## Accomplishments
- Authored `tools/capture_golden.cpp`, a one-shot Rack-free generator that seeds ONLY the drift RNG (spread coefficients left at 0.f defaults — Pitfall 1 neutralization) and emits little-endian float32 fixtures.
- Added a `make capture` target compiled with the same `TEST_CXXFLAGS` (`-ffp-contract=off`) as `make test`, added to the `plugin.mk` skip filter so it builds without `../Rack-SDK`.
- Generated three 32768-byte drift-off fixtures (44.1/48/96 kHz) and documented their provenance/epsilon rationale in `freerun_seeds.txt`.
- Restructured `test_golden.cpp`: a new `replayGoldenDriftOff` runs on ALL OSes (1e-6 absolute epsilon) against the drift-off fixtures; the existing drift-on bit-exact replay is now wrapped in `#if defined(__APPLE__)`.
- `make test` green on macOS: 50 test cases, 2,615,027 assertions, 0 failures.

## Task Commits

Each task was committed atomically:

1. **Task 1: Fixture generator TU + Makefile capture target** - `8d3698d` (feat)
2. **Task 2: Three drift-off fixtures + provenance doc** - `df5b499` (test)
3. **Task 3: Restructure test_golden.cpp (drift-off cross-platform + macOS-gated drift-on)** - `04eec93` (test)

## Files Created/Modified
- `tools/capture_golden.cpp` - One-shot drift-off + spread-off generator; seeds only `core.seed(...)`, writes LE float32 (byte-inverse of loadF32).
- `Makefile` - Added `capture` to the `plugin.mk` skip filter and a `.PHONY: capture` target using `TEST_CXXFLAGS`.
- `tests/golden/freerun_{44100,48000,96000}_driftoff.f32` - 32768-byte cross-platform drift-off references (git-tracked).
- `tests/golden/freerun_seeds.txt` - Appended the drift-off scenario block (drift=0.0, spread unseeded, filenames, epsilon rationale, canonical OS).
- `tests/test_golden.cpp` - Added `replayGoldenDriftOff` (all-OS, 1e-6) + 3 cross-platform cases; gated the drift-on `replayGolden` + 3 cases to macOS.

## Decisions Made
- Drift-off epsilon set to `1e-6` absolute (Claude's discretion per CONTEXT D-07) — tight enough to catch regressions, loose enough for cross-toolchain libm/Xoroshiro low-mantissa divergence.
- Drift-on epsilon deliberately NOT widened; the leg is macOS-gated instead (avoids masking real regressions behind a `normal_distribution`-sized tolerance).
- Generator constructs `forge::LfoCore` directly rather than reusing `BlockDriver`, whose constructor always seeds the spread path.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- Initial edit briefly placed an `#include <cmath>` inside the anonymous namespace; caught and removed before building (cmath is available transitively via BlockDriver.hpp). No functional impact.

## Threat Surface
Threat register mitigations satisfied: T-26-04 (spread-path neutralization in both generator and replay — verified via `! grep setSpreadSeed tools/capture_golden.cpp` and all-OS 1e-6 comparator) and T-26-05 (generator compiled with identical `TEST_CXXFLAGS`, `-ffp-contract=off`). No new security surface introduced.

## Next Phase Readiness
- The cross-platform golden leg is ready; the true Linux/Windows validation happens in the CI matrix on push (out of scope for local macOS execution).
- Contributes the D-07 half of TEST-06 (paired with the D-05 Rack-free matrix work).

## Self-Check: PASSED

All created files exist on disk; all task commits (`8d3698d`, `df5b499`, `04eec93`) plus the metadata commit (`cd5c4ef`) are present in git history. Worktree is clean.

---
*Phase: 26-vcv-library-compliance-packaging*
*Completed: 2026-07-09*
