---
phase: 23-functional-bug-fixes
plan: 01
subsystem: testing
tags: [dsp, clock-tracker, ema, outlier-rejection, fsm, doctest, regression, tdd]

# Dependency graph
requires:
  - phase: 22-test-harness-foundation
    provides: "Rack-free src/dsp/ClockTracker.hpp + headless driveClock/clockInterval helpers in tests/test_dsp_stateful.cpp"
provides:
  - "Consecutive-outlier counter in ClockTracker.hpp that breaks the >3x speedup / fast-slowdown-band lockout"
  - "BUG-01 RED->GREEN regression cases (sustained speedup + fast-clock slowdown band)"
affects: [23-functional-bug-fixes remaining plans (BUG-02 ratio cadence, BUG-03 phase dot, BUG-04 patch-load), v1.4 release hardening]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Consecutive-outlier counter: tolerate a lone glitch, treat a sustained run as a real tempo change and drop to ACQUIRING to re-learn"
    - "TEST-05 RED->GREEN: write post-fix assertion first, prove it fails on the unfixed header (record the RED), then apply the minimal fix"

key-files:
  created: []
  modified:
    - src/dsp/ClockTracker.hpp
    - tests/test_dsp_stateful.cpp

key-decisions:
  - "Consecutive-outlier threshold = 3 (lone glitch still rejected; short sustained run breaks the lockout)"
  - "Recovery via drop-to-ACQUIRING (reuses existing fast-track re-acquire) rather than snap-to-raw — smaller behavioral surface"

patterns-established:
  - "Outlier-run recovery: increment counter in the LOCKED outlier branch; at threshold set ACQUIRING/clockEdgeCount=1/clear counter and fall through to the EMA path; reset counter on every accepted edge and on disconnect/timeout reverts"

requirements-completed: [BUG-01, TEST-05]

# Metrics
duration: ~6min
completed: 2026-06-14
---

# Phase 23 Plan 01: Clock-Tracker >3x Lockout Fix Summary

**Consecutive-outlier counter in ClockTracker.hpp that re-acquires after a sustained >3x speedup (and in the fast-clock slowdown band), pinned by a demonstrated RED->GREEN BUG-01 regression.**

## Performance

- **Duration:** ~6 min
- **Started:** 2026-06-14T10:29:00Z (approx)
- **Completed:** 2026-06-14T10:35:20Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- Fixed BUG-01: the clock tracker no longer locks out permanently when the incoming clock speeds up by more than 3x. Previously every fast edge was rejected as an outlier AND each rejected edge had already reset `clockTimer` (L105), so the no-pulse timeout never fired — the FSM stayed LOCKED at the stale tempo forever.
- Covered the fast-clock narrow slowdown band, where the `max(1.0, ...)` timeout floor cannot rescue a slowdown whose new period stays under 1s.
- Preserved single-glitch robustness: a lone outlier edge is still discarded (the pre-existing single-outlier-rejection test still passes — no jitter on one glitch).
- Demonstrated the TEST-05 RED->GREEN transition explicitly (RED recorded in the test commit, GREEN in the fix commit).

## Task Commits

Each task was committed atomically:

1. **Task 1: Write the BUG-01 RED regression (sustained speedup + fast-slowdown band)** — `271c2f3` (test)
2. **Task 2: Apply the consecutive-outlier counter and confirm GREEN** — `8cace3d` (fix)

## Files Created/Modified
- `tests/test_dsp_stateful.cpp` — Added two BUG-01 TEST_CASEs using the existing `driveClock` helper: a sustained >3x SPEEDUP (120->540 BPM, period ~0.111s) and a fast-clock slowdown band (360->90 BPM). Inequality bands only — never `==` on EMA periods.
- `src/dsp/ClockTracker.hpp` — Added `static constexpr int OUTLIER_THRESHOLD = 3;` and `int consecutiveOutliers = 0;`. In the LOCKED outlier branch the counter increments; at threshold the FSM drops to ACQUIRING (`clockEdgeCount = 1`, `clockBeatCount = 0`, counter cleared) and falls through to the existing fast-track/EMA re-acquire path instead of early-returning. The counter resets on every accepted (in-range) edge and on the disconnect / timeout FREE reverts.

## RED -> GREEN Evidence (TEST-05)

**RED (commit `271c2f3`, against the unfixed header):** exactly the 2 BUG-01 cases failed; all 35 pre-existing cases passed.
- `BUG-01: re-acquires after a sustained >3x SPEEDUP` — `test_dsp_stateful.cpp:116` `CHECK( ct.smoothedPeriod < 0.25f )` FAILED with `smoothedPeriod = 0.499966` (stuck at the stale ~0.5s).
- `BUG-01: re-acquires in the fast-clock slowdown band` — `test_dsp_stateful.cpp:133` `CHECK( ct.smoothedPeriod > 0.4f )` FAILED with `0.166654`, and `:134` `CHECK( smoothedPeriod != Approx(fastPeriod) )` FAILED at `0.166654 != Approx(0.166662)` (stuck at the stale ~0.167s).

**GREEN (commit `8cace3d`, after the fix):** `make test` => `37 | 37 passed | 0 failed`; 1,647,402 assertions all pass. The single-outlier-rejection case still passes. Plugin still builds clean against `../Rack-SDK`.

## Decisions Made
- **Outlier threshold = 3** (Assumption A1 from research, confirmed): low enough to break the lockout quickly after a real tempo change, high enough that a single glitch edge is still rejected without jitter.
- **Recovery via drop-to-ACQUIRING** (Assumption A2): reuses the existing fast-track re-acquire (clockEdgeCount==2 path) and the EMA snap, giving a smaller behavioral surface than snapping `smoothedPeriod` to the raw measurement. The next clean edge measures a fresh period and the FSM re-locks.
- No double `clockTimer.reset()` — the timer was already reset at L105 for the triggering edge.

## Deviations from Plan

None — plan executed exactly as written. Beyond the two edit sites named in the plan, the counter is also cleared on the disconnect and timeout FREE-revert paths so a stale run cannot leak across a FREE transition; this is a correctness-consistency follow-through of the plan's "reset on every accepted edge path" instruction, not a scope change.

## Issues Encountered
None — RED appeared exactly as predicted (Pitfall 1: the speedup is the genuine lockout the slowdown test misses), and the fix produced GREEN on the first run.

## User Setup Required
None — internal DSP FSM change, no external service configuration. No new threats (threat register: T-23-01-NA accept; the fix only changes recovery behavior on attacker-irrelevant clock-edge timing).

## Next Phase Readiness
- SC1 satisfied for BUG-01: clock re-acquires after a sustained >3x speedup and in the fast-clock slowdown band; no permanent lockout. TEST-05 satisfied (explicit recorded red->green).
- `ClockTracker.hpp` signature unchanged — the RatioTable single-home delegation (CR-03) is untouched, so the remaining Phase 23 plans (BUG-02 BEATS_PER_ALIGN, BUG-03 phase dot, BUG-04 patch-load) are unaffected by this change.
- Full `make test` suite green (37 cases) and plugin builds against `../Rack-SDK`.

## Self-Check: PASSED

- FOUND: `.planning/phases/23-functional-bug-fixes/23-01-SUMMARY.md`
- FOUND: commit `271c2f3` (test — BUG-01 RED)
- FOUND: commit `8cace3d` (fix — consecutive-outlier counter, GREEN)

---
*Phase: 23-functional-bug-fixes*
*Completed: 2026-06-14*
