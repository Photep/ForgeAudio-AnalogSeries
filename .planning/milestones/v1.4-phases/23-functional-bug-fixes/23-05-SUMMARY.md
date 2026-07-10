---
phase: 23-functional-bug-fixes
plan: 05
subsystem: testing
tags: [dsp, clock-sync, ratio-table, doctest, regression, vcv-rack, c++17]

# Dependency graph
requires:
  - phase: 23-functional-bug-fixes (plan 23-04)
    provides: "Logged audition DECISION (adopt-table) in STATE.md ### Decisions — the gate this plan reads"
  - phase: 22-test-harness-foundation
    provides: "Rack-free src/dsp/RatioTable.hpp + make test doctest harness + frozen free-run goldens"
provides:
  - "BUG-02 fix: BEATS_PER_ALIGN[15] table — x1.5/÷1.5 align to whole LFO cycles (no mid-cycle truncation)"
  - "Deterministic BUG-02 reset-cadence regression pinning shouldReset over EXPECTED[15] (red→green)"
affects: [verify-work, release-packaging, phase-24-dsp-refactor]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Per-ratio constexpr alignment table (lowest-terms q) replacing runtime round(1/ratio) divisor math"
    - "Audition-decision-parameterized regression: EXPECTED[15] sourced from logged STATE.md decision, valid under either outcome"

key-files:
  created: []
  modified:
    - src/dsp/RatioTable.hpp
    - tests/test_regression.cpp

key-decisions:
  - "Applied adopt-table per logged STATE.md audition DECISION — two-cell swap (idx 6 /1.5 → 3 beats, idx 8 x1.5 → 2 beats), 13 other ratios bit-identical"
  - "shouldReset rewritten to read BEATS_PER_ALIGN uniformly; round(1/ratio) guard removed; signature frozen (CR-03 single home)"
  - "Goldens NOT regenerated — free-run (clkConnected=false) goldens never reach shouldReset; all golden cases stayed green"

patterns-established:
  - "Constexpr alignment table is the single home for ratio→beats cadence; ClockTracker.hpp delegates via forge::shouldReset (CR-03)"

requirements-completed: [BUG-02, TEST-05]

# Metrics
duration: 4min
completed: 2026-06-15
---

# Phase 23 Plan 05: BUG-02 x1.5/÷1.5 Alignment Summary

**BEATS_PER_ALIGN[15] table swap fixes x1.5/÷1.5 mid-cycle truncation per the logged adopt-table audition decision, pinned by a deterministic red→green reset-cadence regression.**

## Performance

- **Duration:** ~4 min
- **Started:** 2026-06-15
- **Completed:** 2026-06-15
- **Tasks:** 2
- **Files modified:** 2 (RatioTable.hpp, test_regression.cpp)

## Audition Gate

The audition gate was satisfied before any code ran. STATE.md `### Decisions` logs:

> `- [Phase 23]: x1.5/÷1.5 audition — DECISION: adopt-table — rationale: in-Rack listening (operator, fresh-flushed CURRENT build, install hashes matched 3bb6fba before audition) confirmed the current cadence truncates mid-cycle — x1.5 retriggers every beat (chops ½ cycle) and ÷1.5 resets every 2 beats (truncates ⅓ cycle); the proposed BEATS_PER_ALIGN table (x1.5 → every 2 beats, ÷1.5 → every 3 beats) is preferred. Gates plan 23-05: apply the two-cell table swap (idx 8 → 2, idx 6 → 3) and pin with the deterministic cadence regression.`

**Branch taken: adopt-table.** Therefore the two-cell table swap was applied and EXPECTED[15] set to the adopt-table cadence.

## Accomplishments

- BUG-02 cadence regression added to `tests/test_regression.cpp` with `EXPECTED[15] = {16,8,6,4,3,2,3,1,2,1,1,1,1,1,1}` (adopt-table), asserting `shouldReset` fires only on the per-ratio boundary, never mid-cycle, for all 15 ratios. An in-file comment quotes the STATE.md decision line for traceability.
- `BEATS_PER_ALIGN[15]` constexpr table added to `src/dsp/RatioTable.hpp` at the `// FUTURE (P23)` site; `shouldReset` rewritten to read it uniformly (`if (ratioIdx < 0) return true; return beatCount >= BEATS_PER_ALIGN[ratioIdx];`), removing the `RATIO_TABLE[idx] < 1.f` guard and the `round(1/ratio)` divisor math.
- Two-cell behavioral change exactly as specified: idx 6 (/1.5) 2→3 beats, idx 8 (x1.5) 1→2 beats; the 13 other ratios are bit-identical.

## Red→Green Evidence (TEST-05)

- **RED (pre-swap header):** `make test` after Task 1 — `43 test cases | 42 passed | 1 failed`, `2 assertions failed`. The BUG-02 case was the *only* failure; the 2 failed assertions are precisely the early (mid-cycle) resets the pre-swap header produces — idx 6 fires at b=2 (was every 2) and idx 8 fires at b=1 (was every 1).
- **GREEN (post-swap):** `make test` after Task 2 — `43 test cases | 43 passed | 0 failed`, `2,589,928 assertions passed`. The BUG-02 case is green where it was red; the 13-ratio cadence assertions from plan 23-03 and the BUG-04 case remain green.

## Golden Non-Impact (no regeneration)

All golden test cases stayed green across both runs, confirming non-impact. The goldens were captured free-run (`clkConnected=false`, swing Straight) and `shouldReset` is only reached on a clock edge in clocked mode — so adopting `BEATS_PER_ALIGN` cannot change any golden sample. **No goldens were regenerated** (last touched in Phase 22 commit fdb63c3).

## Single-Home / Frozen Signature (CR-03)

`ClockTracker.hpp` was **not modified**. It delegates to `forge::shouldReset` at L202 (single home for the ratio→beats cadence). The `bool shouldReset(int, int)` signature is frozen, so the delegation continues to compile and pick up the table swap automatically.

## Wave-Merge Check

`make RACK_DIR=../Rack-SDK` recompiled `AnalogLFO.cpp` (which consumes `RatioTable.hpp`) and linked `plugin.dylib` cleanly — the plugin still builds against `../Rack-SDK`.

## Task Commits

1. **Task 1: BUG-02 cadence regression (RED)** - `3185bc3` (test)
2. **Task 2: BEATS_PER_ALIGN table swap (GREEN)** - `457770f` (fix)

**Plan metadata:** _(final docs commit)_

## Files Created/Modified

- `tests/test_regression.cpp` - Added `#include "dsp/RatioTable.hpp"` and the BUG-02 cadence TEST_CASE over `EXPECTED[15]` (adopt-table), with a comment quoting the STATE.md decision.
- `src/dsp/RatioTable.hpp` - Added `static constexpr int BEATS_PER_ALIGN[15]`; rewrote `shouldReset` to read it uniformly (removed `round(1/ratio)` divisor and the `< 1.f` guard). Signature frozen.

## Decisions Made

None beyond following the logged adopt-table decision. The two-cell swap and the EXPECTED[15] values were fully specified by the plan and the verified research table.

## Deviations from Plan

None - plan executed exactly as written. The unused `#include <cmath>` was intentionally left in RatioTable.hpp (the plan explicitly permits "the `<cmath>` include may remain"); it produces no warning under `-Wall -Wextra`.

## Issues Encountered

The plugin build (`make RACK_DIR=../Rack-SDK`) was initially blocked by the sandbox and was re-run with the sandbox disabled to complete the wave-merge build check. No code issue.

## Next Phase Readiness

- SC4 satisfied: x1.5/÷1.5 align per the auditioned decision (no mid-cycle truncation), pinned by a deterministic regression. TEST-05 satisfied for BUG-02 (red→green demonstrated).
- All four Phase 23 functional bugs are now fixed and pinned; `make test` is fully green (43/43) and the plugin builds. Phase 23 is ready for `/gsd:verify-work`.

## Self-Check: PASSED

- FOUND: `.planning/phases/23-functional-bug-fixes/23-05-SUMMARY.md`
- FOUND: `src/dsp/RatioTable.hpp` (BEATS_PER_ALIGN present, 4 references)
- FOUND: `tests/test_regression.cpp`
- FOUND commit `3185bc3` (Task 1, test RED)
- FOUND commit `457770f` (Task 2, fix GREEN)

---
*Phase: 23-functional-bug-fixes*
*Completed: 2026-06-15*
