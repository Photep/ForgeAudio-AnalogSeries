---
phase: 23-functional-bug-fixes
plan: 03
subsystem: testing
tags: [doctest, dsp, waveshape, ratio-table, swing, coverage]

# Dependency graph
requires:
  - phase: 22-test-harness-foundation
    provides: "Rack-free DSP core (Waveshape/RatioTable/Swing headers) + make test doctest harness"
provides:
  - "Full morph x character x phase waveshape range grid (pre-scale band)"
  - "Full shouldReset cadence coverage at the 13 un-gated ratios + idx6/idx8 table-value pins"
  - "Swing-math edge coverage: free-run gate == 1.0 for all 6 fractions; clocked Straight/Heavy warp"
affects: [23-05, regression]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Audition-gated exclusion: cadence for idx6 (/1.5) and idx8 (x1.5) deliberately NOT asserted, deferred to 23-05"

key-files:
  created: []
  modified:
    - "tests/test_dsp_units.cpp"

key-decisions:
  - "Asserted only un-gated facts (RATIO_TABLE values/labels) for idx6/idx8; left cadence to plan 23-05 post-audition"
  - "Widened Phase 22 waveshape SCAFFOLD (21x21x64) to a full 50x50x128 grid with an added isfinite guard, keeping the same pre-scale band"

patterns-established:
  - "Boundary-sweep cadence: CHECK_FALSE across [1, period) then CHECK at period (and period+1) per division ratio"
  - "Float discipline preserved: doctest::Approx / inequality bands only, never == on warp output"

requirements-completed: [TEST-03]

# Metrics
duration: 9min
completed: 2026-06-14
---

# Phase 23 Plan 03: TEST-03 Pure/Table Coverage Summary

**Extended `test_dsp_units.cpp` to cover the full waveshape output-range grid, the `shouldReset` reset cadence at all 13 un-gated ratios, and swing math (free-run gate + clocked Straight/Heavy warp) — `make test` 38 → 42 cases, all green.**

## Performance

- **Duration:** ~9 min
- **Started:** 2026-06-14
- **Completed:** 2026-06-14
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments
- Widened the Phase 22 waveshape SCAFFOLD to the full morph x character x phase grid (50 x 50 x 128), asserting every output stays finite and within the documented pre-scale band [-1.15, 1.15]. Did NOT duplicate the strict post-scale ±5V invariant (lives in test_invariants.cpp).
- Added swing-math edge coverage: the free-run gate (`isClocked=false`) returns 1.0 for every one of the 6 SWING_FRACTIONS across 8 phases; clocked Straight (0.50) stays ~1.0 across the cycle; clocked Heavy (0.71) warps below/above 1.0 with closed-form checks.
- Pinned the CURRENT `shouldReset` cadence at the 13 un-gated ratios: division idx 0-5 boundary-swept against `round(1/ratio)`, multiply idx 7,9-14 every-beat, unlocked idx<0 always.
- Deliberately excluded idx 6 (/1.5) and idx 8 (x1.5) reset cadence (audition-gated) — asserted only their un-gated RATIO_TABLE values + labels, with a comment pointing to plan 23-05.

## Task Commits

Each task was committed atomically:

1. **Task 1: Widen waveshape grid + swing-math edges** - `18cb464` (test)
2. **Task 2: Full ratio/alignment cadence (13 un-gated ratios)** - `6e0590f` (test)

**Plan metadata:** see final docs commit.

## Files Created/Modified
- `tests/test_dsp_units.cpp` - Widened waveshape grid + isfinite guard; added 2 swing-math edge TEST_CASEs; added full ratio-cadence TEST_CASE + idx6/idx8 table-value pin TEST_CASE.

## Decisions Made
- Excluded idx 6 / idx 8 reset cadence to avoid conflicting with the pending x1.5/÷1.5 audition decision; those cells are pinned by plan 23-05's regression. Asserted only their audition-invariant facts (table values/labels) here.
- Kept the existing pre-scale band ([-1.15, 1.15]) when widening the grid rather than tightening it — the shipped bleed crosstalk legitimately exceeds ±1, and the strict ±5V post-scale bound is owned by test_invariants.cpp.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None. Baseline was 38 cases green; additions brought it to 42 green on the first build with no auto-fixes needed.

## Scope Notes (TEST-03 split)
- Consecutive-outlier clock recovery coverage is NOT here — it lives in `test_dsp_stateful.cpp` (plan 23-01, BUG-01). Not duplicated.
- idx 6 / idx 8 alignment cadence is deferred to plan 23-05 (audition-gated). TEST-03 is fully satisfied at phase close once 23-01 + 23-05 land alongside this plan.

## User Setup Required
None - test-only change over pure DSP headers, no external configuration.

## Next Phase Readiness
- TEST-03 pure/table coverage delivered. Ready for plan 23-05 to pin idx6/idx8 cadence post-audition in test_regression.cpp.
- No production code touched; no golden regeneration needed (shouldReset unreached by free-run goldens).

## Self-Check: PASSED

- FOUND: tests/test_dsp_units.cpp (modified)
- FOUND: .planning/phases/23-functional-bug-fixes/23-03-SUMMARY.md
- FOUND commit: 18cb464 (Task 1)
- FOUND commit: 6e0590f (Task 2)
- `make test`: 42 cases, all passed

---
*Phase: 23-functional-bug-fixes*
*Completed: 2026-06-14*
