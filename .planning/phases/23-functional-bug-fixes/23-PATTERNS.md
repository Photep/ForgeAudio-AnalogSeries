# Phase 23: Functional Bug Fixes - Pattern Map

**Mapped:** 2026-06-14
**Files analyzed:** 6 (1 new header, 1 new test TU, 2 modified test TUs, 2 modified DSP headers, 1 modified shell)
**Analogs found:** 6 / 6 (every file has an in-tree analog — this is a mature, self-similar codebase)

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/dsp/PatchParse.hpp` (NEW) | utility (pure header) | transform (string→uint64) | `src/dsp/Swing.hpp` | exact (same: tiny Rack-free pure-function header in `forge::`) |
| `tests/test_regression.cpp` (NEW) | test | request-response (call pure fn, assert) | `tests/test_dsp_units.cpp` | exact (doctest TU over `forge::*` headers, table/range CHECKs) |
| `tests/test_dsp_stateful.cpp` (MOD) | test | event-driven (drive clock FSM) | itself (extend) — pattern from L24-100 | exact (same file, established `driveClock` helper) |
| `tests/test_dsp_units.cpp` (MOD) | test | request-response (table/range asserts) | itself (extend) — pattern from L162-211 | exact (same file, established TEST_CASE table style) |
| `src/dsp/RatioTable.hpp` (MOD) | utility (pure header) | transform (idx→bool) | itself — `// FUTURE (P23)` marker at L56 | exact (fix site pre-marked) |
| `src/dsp/ClockTracker.hpp` (MOD) | service (stateful FSM) | event-driven (edge→state) | itself — outlier branch L117-127 | exact (fix site pre-located) |
| `src/AnalogLFO.cpp` (MOD) | controller (Rack shell) | request-response + file-I/O (JSON) | itself — L243-247 (JSON), L316/L332 (atomics) | exact (fix sites pre-located, NOT headless-testable) |

## Pattern Assignments

### `src/dsp/PatchParse.hpp` (NEW — utility, transform)

**Analog:** `src/dsp/Swing.hpp` (closest small pure-function header). Secondary: `src/dsp/RatioTable.hpp` for the `#include <cmath>`-style include + `inline` free-function convention.

**Header skeleton to copy** — mirror Swing.hpp L1-15 exactly (pragma + path comment + Rack-free hygiene note + `namespace forge`):
```cpp
#pragma once
// src/dsp/PatchParse.hpp
//
// Non-throwing hex seed parse for dataFromJson (BUG-04 / CODE-REVIEW #4).
// Replaces the throwing std::stoull at AnalogLFO.cpp:244-245 so a hand-edited
// patch with malformed/over-long seed strings cannot crash Rack on load.
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes.

#include <cstdint>
#include <cstdlib>   // std::strtoull
#include <cerrno>

namespace forge {

// ... inline bool parseSeedHex(...) ...

} // namespace forge
```

**Core pattern (the function body)** — from RESEARCH.md L292-298 (verified `strtoull` recipe):
```cpp
inline bool parseSeedHex(const char* s, uint64_t& out) {
    if (!s || !*s) return false;             // NULL / empty -> fail (BUG-04 empty case)
    char* end = nullptr; errno = 0;
    unsigned long long v = std::strtoull(s, &end, 16);
    if (errno == ERANGE || end == s || *end != '\0') return false;  // over-long / non-hex tail
    out = (uint64_t)v; return true;
}
```

**Convention notes for the planner:**
- Free `inline` function in `namespace forge` — matches `forge::shouldReset` (RatioTable.hpp L53) and `forge::swingPhaseMultiplier` (Swing.hpp L33). Do NOT make it a struct/method.
- `#pragma once` (not include guards) — every `src/dsp/*.hpp` uses `#pragma once` at L1.
- Zero Rack includes; `cstdint`/`cstdlib`/`cerrno` only. This is what makes it linkable in `make test`.

---

### `tests/test_regression.cpp` (NEW — test, request-response)

**Analog:** `tests/test_dsp_units.cpp` (the table/range-assertion doctest TU). Use its TU header (L1-23) and TEST_CASE style verbatim.

**TU header + include skeleton** — mirror test_dsp_units.cpp L1-23:
```cpp
// tests/test_regression.cpp
//
// TEST-05 fail-before / pass-after regression pins for the Phase 23 bug fixes.
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).
// Headers reached via -Isrc / -Itests (see the Makefile test target).

#include "doctest.h"

#include <cstdint>

#include "dsp/RatioTable.hpp"    // BUG-02: forge::shouldReset cadence
#include "dsp/PatchParse.hpp"    // BUG-04: forge::parseSeedHex
```
**CRITICAL include-discovery facts** (Makefile L33-45): `TEST_SOURCES := $(wildcard tests/*.cpp)` — a new `tests/test_regression.cpp` is auto-compiled, NO Makefile edit needed. `TEST_HEADERS := $(wildcard src/dsp/*.hpp)` — a new `src/dsp/PatchParse.hpp` auto-triggers rebuild. Headers reached via `-Isrc` so `#include "dsp/PatchParse.hpp"` resolves. Do NOT `#define DOCTEST_CONFIG_IMPLEMENT` here (tests/main.cpp owns it — see every existing TU's L7-ish comment).

**BUG-04 TEST_CASE** — from RESEARCH.md L300-306, styled like test_dsp_units.cpp's `CHECK`/`CHECK_FALSE` table asserts (e.g. L169-185 `shouldReset` block):
```cpp
TEST_CASE("BUG-04: malformed seed string never throws, signals failure") {
    uint64_t v = 0;
    CHECK_FALSE(forge::parseSeedHex("zzzz", v));                   // non-hex
    CHECK_FALSE(forge::parseSeedHex("FFFFFFFFFFFFFFFFFFFF", v));   // out of range
    CHECK_FALSE(forge::parseSeedHex("", v));                       // empty
    CHECK(forge::parseSeedHex("C0FFEE", v)); CHECK(v == 0xC0FFEEull);  // valid round-trips
}
```

**BUG-02 TEST_CASE** — from RESEARCH.md L262-272; the `EXPECTED[15]` array is set per the logged audition decision (Pitfall 7). Mirrors the boundary-sweep style of test_dsp_units.cpp L169-185:
```cpp
TEST_CASE("BUG-02: ratio alignment cadence (per logged audition decision)") {
    // adopt-table: idx6 /1.5 = 3, idx8 x1.5 = 2;  keep-current: idx6 = 2, idx8 = 1.
    static const int EXPECTED[15] = {16,8,6,4,3,2, 3, 1, 2, 1,1,1,1,1,1};
    for (int idx = 0; idx < 15; ++idx) {
        int period = EXPECTED[idx];
        for (int b = 1; b < period; ++b)
            CHECK_FALSE(forge::shouldReset(idx, b));   // no early (mid-cycle) reset
        CHECK(forge::shouldReset(idx, period));        // resets exactly on the boundary
    }
}
```
> BUG-02 is GATED by the human audition (Pitfall 7 / SC3). This case is written in Wave C, AFTER the STATE.md `### Decisions` entry exists. The 13 unchanged ratios assert identically under either decision.

---

### `tests/test_dsp_stateful.cpp` (MOD — test, event-driven; extend for BUG-01)

**Analog:** itself — the existing `driveClock` helper (L24-38) and the re-acquire TEST_CASE (L84-100) are the exact pattern to replicate.

**`driveClock` signature already present (L24-25)** — reuse, do NOT re-roll:
```cpp
static forge::ClockTracker::Result driveClock(forge::ClockTracker& ct, double bpm,
                                              double seconds, double sr, int ratioIdx = 7);
```

**Existing slowdown re-acquire test (L84-100)** is the structural template; the NEW BUG-01 case is the **speedup** mirror (RESEARCH.md L278-287, Pitfall 1 — the speedup is what the OLD code fails):
```cpp
TEST_CASE("BUG-01: re-acquires after a sustained >3x SPEEDUP (no lockout)") {
    forge::ClockTracker ct;
    driveClock(ct, 120.0, 4.0, 48000.0, 7);   // lock at period 0.5s
    REQUIRE(ct.clockState == forge::LOCKED);
    driveClock(ct, 540.0, 4.0, 48000.0, 7);   // 4.5x faster -> period ~0.111s
    CHECK(ct.smoothedPeriod < 0.25f);          // moved OFF the stale 0.5s -> proves re-lock
}
```
**Assertion style (Pitfall 6):** inequality bands / `doctest::Approx`, NEVER `==` on EMA-smoothed periods — see existing L98 `CHECK(ct.smoothedPeriod > 1.5f)` and L46 `Approx(0.5).epsilon(0.02)`. Also add the narrow fast-clock slowdown band case (smoothed period < ~0.33s) per RESEARCH L198.

---

### `tests/test_dsp_units.cpp` (MOD — test, request-response; extend TEST-03)

**Analog:** itself — the `shouldReset` table block (L169-185) and the Waveshape range-sweep (L101-124) are the two patterns to extend.

**Existing range-sweep pattern (L101-124)** — nested morph×character×phase grid with `CHECK(v >= lo); CHECK(v <= hi);`. TEST-03 widens this to the full grid (RESEARCH L400). Bounds note: pre-scale `[-1.15,1.15]` scaffold here; strict post-scale ±5V lives in `test_invariants.cpp` (do not duplicate).

**Existing `shouldReset` table pattern (L169-185)** — extend with the per-ratio cadence cases (the unchanged 13 ratios), using the same `CHECK_FALSE(... n-1) / CHECK(... n)` boundary style:
```cpp
TEST_CASE("RatioTable: shouldReset division behavior") {
    CHECK_FALSE(forge::shouldReset(0, 15));   // /16: not yet 16 beats
    CHECK(forge::shouldReset(0, 16));         // /16: reached divisor
    CHECK_FALSE(forge::shouldReset(5, 1));    // /2
    CHECK(forge::shouldReset(5, 2));
    CHECK(forge::shouldReset(7, 1));          // x1: every beat
    CHECK(forge::shouldReset(-1, 1));         // unlocked: always
}
```

---

### `src/dsp/RatioTable.hpp` (MOD — utility, transform; BUG-02 table swap)

**Analog:** itself — the `// FUTURE (P23)` marker at L56 names the exact slot. This is a **two-cell** behavioral change (idx 6 and idx 8 only).

**Current code the fix replaces (L53-61):**
```cpp
inline bool shouldReset(int ratioIdx, int beatCount) {
    bool reset = true;
    if (ratioIdx >= 0 && RATIO_TABLE[ratioIdx] < 1.f) {
        // FUTURE (P23): replace with `int divisor = BEATS_PER_ALIGN[ratioIdx];`
        int divisor = (int)std::round(1.f / RATIO_TABLE[ratioIdx]);
        reset = (beatCount >= divisor);
    }
    return reset;
}
```

**Post-fix shape (RESEARCH L151-158), if audition = adopt-table:** add a `static constexpr int BEATS_PER_ALIGN[15]` next to `RATIO_TABLE` (L20-36 style) and read it uniformly (the `< 1.f` guard goes away — non-int ratios x1.5/÷1.5 are now handled too):
```cpp
static constexpr int BEATS_PER_ALIGN[15] = {
//  /16 /8 /6 /4 /3 /2  /1.5 x1 x1.5 x2 x3 x4 x6 x8 x16
     16, 8, 6, 4, 3, 2,  3,   1,  2,  1, 1, 1, 1, 1, 1
};
inline bool shouldReset(int ratioIdx, int beatCount) {
    if (ratioIdx < 0) return true;                  // unlocked: every beat (unchanged)
    return beatCount >= BEATS_PER_ALIGN[ratioIdx];
}
```
**Signature is FROZEN** (`bool shouldReset(int, int)`) — `ClockTracker.hpp:172` delegates to it (CR-03 single-home). Do NOT touch ClockTracker for BUG-02. Goldens are free-run only (`shouldReset` unreached) → no regeneration (RESEARCH L165, L191).

---

### `src/dsp/ClockTracker.hpp` (MOD — service, event-driven; BUG-01 outlier counter)

**Analog:** itself — the outlier-rejection branch at L117-127 (inside the `clockState == LOCKED` block) is the exact edit site.

**Current outlier branch (L118-127) the fix wraps:**
```cpp
if (clockState == LOCKED && smoothedPeriod > 0.f) {
    bool isOutlier = (rawPeriod > 3.0f * smoothedPeriod) ||
                     (rawPeriod < smoothedPeriod / 3.0f);
    if (isOutlier) {
        r.state = clockState;          // <-- counter increment + threshold check go BEFORE this return
        r.smoothedPeriod = smoothedPeriod;
        return r;                      // (silently discards the edge — the lockout)
    }
}
```

**Fix shape (RESEARCH L209, Pitfall 2):**
- Add a member next to the existing FSM members (L36-45): `int consecutiveOutliers = 0;`
- In the `isOutlier` branch: increment, check threshold (RESEARCH recommends 3). On threshold → drop to ACQUIRING and re-learn (reuses the fast-track at L130-137): `clockState = ACQUIRING; clockEdgeCount = 1; consecutiveOutliers = 0;` then fall through (do NOT early-return). Else early-return as today.
- Reset `consecutiveOutliers = 0` on every ACCEPTED edge path (after the outlier branch passes).
- Do NOT double-reset `clockTimer` — it was already reset at L105 for this edge.

**Member-declaration analog (where to add the counter), L36-45:**
```cpp
struct ClockTracker {
    SchmittTrigger clockTrigger;
    Timer clockTimer;
    float smoothedPeriod = 0.f;
    // ... add:  int consecutiveOutliers = 0;
    int clockEdgeCount = 0;
    ClockState clockState = FREE;
```
Driven headlessly via `driveClock` in test_dsp_stateful.cpp — no shell coupling.

---

### `src/AnalogLFO.cpp` (MOD — controller; BUG-04 wiring + BUG-03 consumer)

> NOT reachable from `make test` (needs `jansson` json_t* + GL). BUG-04 logic is pinned via the extracted `parseSeedHex`; the shell call site + BUG-03 are manual-UAT only (Pitfall 4).

**BUG-04 wiring — current `dataFromJson` (L238-252), wire `parseSeedHex` after the type guard:**
```cpp
void dataFromJson(json_t* rootJ) override {
    json_t* s0J = json_object_get(rootJ, "spreadSeed0");
    json_t* s1J = json_object_get(rootJ, "spreadSeed1");
    if (json_is_string(s0J) && json_is_string(s1J)) {
        spreadSeed[0] = std::stoull(json_string_value(s0J), nullptr, 16);   // <-- BUG-04: still throws
        spreadSeed[1] = std::stoull(json_string_value(s1J), nullptr, 16);
        initComponentSpread();
    }
    // ...
```
**Fix:** replace the two `std::stoull` lines with `forge::parseSeedHex(...)` into temporaries; only assign `spreadSeed[]` + call `initComponentSpread()` when BOTH parse `true`. On failure keep the constructor-seeded spread (RESEARCH L220, L308). Add `#include "dsp/PatchParse.hpp"` near the other dsp includes.

**BUG-03 consumer — current store (L316) vs the already-correct buffer gate (L332):**
```cpp
displaySwingFraction.store(t.swingFrac, std::memory_order_relaxed);     // L316: stores RAW (the bug)
// ...
float displaySwing = t.isClocked ? t.swingFrac : 0.5f;                  // L332: buffer ALREADY gates correctly
```
**Fix (one line, RESEARCH L233):** L316 → `displaySwingFraction.store(t.isClocked ? t.swingFrac : 0.5f, std::memory_order_relaxed);` — mirror the L332 gate. Keep `std::memory_order_relaxed`; no new shared state (Pitfall 5). The `swingFrac`/`isClocked` telemetry (`forge::LfoCore::Telemetry`, LfoCore.hpp L83-85) is already correct — do NOT touch the DSP.

## Shared Patterns

### Rack-free header hygiene (applies to: `PatchParse.hpp`, all `src/dsp/*.hpp` edits)
**Source:** `src/dsp/Swing.hpp` L1-15, `src/dsp/RatioTable.hpp` L1-16
**Rule:** `#pragma once` at L1; a path-named banner comment; an explicit "ZERO Rack-SDK includes" line; everything in `namespace forge`; std headers only. This is what keeps the unit linkable in the Rack-free `make test`.

### doctest TU boilerplate (applies to: `test_regression.cpp`, both modified test TUs)
**Source:** every `tests/test_*.cpp` L1-23
```cpp
#include "doctest.h"
// ... std + "dsp/X.hpp" includes ...
// DO NOT define DOCTEST_CONFIG_IMPLEMENT here — tests/main.cpp owns it.
```
Auto-discovery: `wildcard tests/*.cpp` + `wildcard src/dsp/*.hpp` (Makefile L33-34) — new files need no Makefile edit. `make test` runs the single linked binary.

### Float-comparison discipline (applies to: every clock/period assertion)
**Source:** test_dsp_stateful.cpp L46/L98, test_dsp_units.cpp L165/L199
**Rule:** `doctest::Approx(...).epsilon(e)` or inequality bands for any EMA/float value; bit-exact `==` ONLY for same-seed RNG determinism (test_dsp_stateful.cpp L132-134). Clock FSM tests touch no RNG → keep drift=0 to isolate (Pitfall 6).

### TEST-05 RED→GREEN procedure (applies to: BUG-01, BUG-02, BUG-04 cases)
**Source:** RESEARCH.md Pattern 1 (L117-128), Pitfall 8
**Rule:** Write the post-fix assertion FIRST; run `make test` and confirm the new case is the ONLY failure (record the RED); apply the minimal source fix; `make test` all-green; commit test+fix together citing red→green. For BUG-01 the RED is real only on a **speedup** (the slowdown already passes on old code — Pitfall 1).

## No Analog Found

None. Every file maps to an existing in-tree analog. Phase 22 deliberately shaped `src/dsp/*` so these four fixes are local edits, and the test harness already has matching unit/stateful/invariant TUs.

## Metadata

**Analog search scope:** `src/dsp/*.hpp` (RatioTable, ClockTracker, Swing, LfoCore, RackCompat, DriftEngine, Waveshape, PatchParse-to-be), `tests/*.cpp` (dsp_units, dsp_stateful, invariants, golden) + `tests/BlockDriver.hpp`, `src/AnalogLFO.cpp` (L230-359), `Makefile` test target.
**Files scanned:** 8 read in full/targeted + Makefile + LfoCore grep.
**Pattern extraction date:** 2026-06-14
