---
phase: 23-functional-bug-fixes
reviewed: 2026-06-15T00:00:00Z
depth: deep
files_reviewed: 6
files_reviewed_list:
  - src/dsp/ClockTracker.hpp
  - src/dsp/PatchParse.hpp
  - src/dsp/RatioTable.hpp
  - src/AnalogLFO.cpp
  - tests/test_dsp_stateful.cpp
  - tests/test_dsp_units.cpp
  - tests/test_regression.cpp
findings:
  critical: 0
  warning: 0
  info: 4
  total: 4
status: low
---

# Phase 23: Functional Bug Fixes — Code Review Report

**Reviewed:** 2026-06-15
**Depth:** deep (cross-file analysis, FSM trace, security path analysis)
**Files Reviewed:** 6 source + 3 test files (7 unique paths, 3 test TUs)
**Status:** LOW — no blockers or warnings; 4 low-severity quality items

## Summary

Phase 23 ships four bug fixes and their regression suites: BUG-01 (consecutive-outlier
counter to break the permanent >3x tempo-change lockout in ClockTracker), BUG-02
(BEATS_PER_ALIGN table replacing the `round(1/ratio)` divisor to fix x1.5/÷1.5
mid-cycle truncation), BUG-03 (gating the display swing fraction to the effective
value in free-run mode), and BUG-04 (replacing throwing `std::stoull` with
`parseSeedHex` to prevent crash-on-load for hand-edited patch files).

All four fixes are logically correct. The FSM trace for BUG-01 confirms the
fall-through path is safe: after the threshold fires, `clockEdgeCount` is set to 1
and the code falls into EMA smoothing (not the `edgeCount==1` first-edge branch —
that branch is guarded by `if (clockEdgeCount == 1)` earlier, which does not re-fire
because the `else if (rawPeriod > 0.001f)` block is already executing). The
fast-track re-acquisition check correctly skips (edgeCount==2 requires the *next*
clean edge). `smoothedPeriod` remains positive throughout re-acquisition (EMA from
0.5 toward 0.111 never goes negative). The `consecutiveOutliers` counter is correctly
reset in both disconnect and timeout paths.

`parseSeedHex` handles all dangerous inputs correctly: NULL pointer, empty string,
non-hex content (end==s), trailing garbage (*end!=NUL), and ERANGE. The `errno=0`
set before `strtoull` is required and present. On C99+ (all supported platforms),
`errno` is thread-local, so there is no race between the set and the check.
`strtoull` skips leading whitespace silently, meaning ` C0FFEE` would parse as valid;
this is benign since Jansson's `json_string_value` never produces whitespace-prefixed
strings and the caller is the GUI/JSON thread.

The BEATS_PER_ALIGN table is mathematically verified: for each ratio p/q in lowest
terms, the LFO returns to phase 0 every q clock beats — idx 6 (/1.5 = 2/3, q=3 → 3)
and idx 8 (x1.5 = 3/2, q=2 → 2) are now correct; all 13 other entries are
bit-identical to the prior `round(1/ratio)` cadence. The single-home invariant holds:
`shouldReset` is only called from `ClockTracker::step` after `clampi(ratioIdx,0,14)`,
and no duplicate table exists elsewhere.

Tests are genuine RED-before/GREEN-after regressions. Float comparisons correctly
use `Approx` or inequality bands throughout; no `==` on float outputs.

Real-time safety: no allocation, locks, or exceptions on the audio thread. All four
changes are either in `dataFromJson` (GUI thread) or replace an inline integer
compare (`displaySwingFraction` store) with another identical-cost atomic store.
Include hygiene is clean: `PatchParse.hpp` and `RatioTable.hpp` carry zero Rack SDK
includes and compile under the Rack-free test harness.

---

## Narrative Findings (AI reviewer)

### IN-01: Wrong Line Reference in BUG-01 Comment

**File:** `src/dsp/ClockTracker.hpp:134`
**Issue:** The comment explaining why the timeout cannot rescue a >3x speedup reads:
"each rejected edge already reset clockTimer at L105". Line 105 is inside the
*timeout-path* (`if (clockTimer.getTime() > timeout)`) timer reset; the
edge-detection-path reset that actually causes the lockout is at line 114
(`clockTimer.reset()` immediately after `clockTimer.getTime()`). A maintainer
reading the comment and jumping to L105 will land in the wrong code block and
find it unrelated to the explanation.
**Fix:** Change "at L105" to "at L114" in the comment.

---

### IN-02: Stale File-Header Comment in RatioTable.hpp

**File:** `src/dsp/RatioTable.hpp:7-10`
**Issue:** The file header still contains the Phase 22 placeholder text:

```
// Shape-for-P23 (D-04 / deferred): shouldReset is structured so a per-ratio
// BEATS_PER_ALIGN[15] table can replace the round(1/RATIO_TABLE[idx]) divisor
// math later WITHOUT an API change. The x1.5 / ÷1.5 alignment fix lands in
// Phase 23 — behavior here is UNCHANGED.
```

Phase 23 has now landed: `BEATS_PER_ALIGN[15]` was added and `shouldReset` was
rewritten to use it. The claim "behavior here is UNCHANGED" and the forward
reference to Phase 23 as future work are both false. Future readers will be
confused about whether the deferred fix was applied.
**Fix:** Replace the stale block with a one-line tombstone, e.g.:
```cpp
// BEATS_PER_ALIGN[15] table and shouldReset rewritten in Phase 23 (BUG-02).
```

---

### IN-03: Unused `<cmath>` Include in RatioTable.hpp

**File:** `src/dsp/RatioTable.hpp:14`
**Issue:** `#include <cmath>` was needed by the pre-Phase-23 `shouldReset` which
called `std::round(1.f / RATIO_TABLE[idx])`. The rewrite replaced that with a
direct table lookup; no `std::` math function is called anywhere in the file now
(the mentions of `round()` in comments are just historical notes). The include is
dead weight.
**Fix:** Remove `#include <cmath>`.

---

### IN-04: `shouldReset` Lacks Upper Bounds Guard on `ratioIdx`

**File:** `src/dsp/RatioTable.hpp:63-65`
**Issue:** `shouldReset(int ratioIdx, int beatCount)` guards the lower bound
(`ratioIdx < 0 → true`) but not the upper bound. If called with `ratioIdx > 14`,
`BEATS_PER_ALIGN[ratioIdx]` is an out-of-bounds array access (undefined behaviour
in C++). All current callers are safe — `ClockTracker::step` clamps with
`clampi(ratioIdx, 0, 14)` before delegating, and tests only exercise `[-5, 14]` —
but the function's public signature accepts any `int` with no stated precondition
and no runtime guard.
**Fix:** Add an assertion or explicit clamp. Lightweight option:
```cpp
inline bool shouldReset(int ratioIdx, int beatCount) {
    if (ratioIdx < 0) return true;
    if (ratioIdx > 14) return true;   // out-of-range: treat as unlocked (safe fallback)
    return beatCount >= BEATS_PER_ALIGN[ratioIdx];
}
```
Alternatively, a `static_assert(sizeof(BEATS_PER_ALIGN)/sizeof(int) == 15)` plus a
comment in the function contract that callers must clamp to `[0, 14]`.

---

_Reviewed: 2026-06-15_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
