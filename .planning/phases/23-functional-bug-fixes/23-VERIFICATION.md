---
phase: 23-functional-bug-fixes
verified: 2026-06-15T01:00:00Z
status: passed
score: 11/11
overrides_applied: 0
human_verification:
  - test: "BUG-03 consumer — phase-dot tracking in free-run + swing"
    expected: "In Rack, select Medium or Heavy swing from the context menu, then unplug the clock; the phase dot must sit on the trace/output waveform and NOT be warped ahead of it."
    why_human: "displaySwingFraction.store() is on the audio thread; the phase-dot rendering is in the GUI draw path. There is no headless harness that exercises the atomic publish -> GUI draw consumer chain. The code change (L328 gates the value) mirrors the already-verified L344 buffer gate, but the visual lock of dot to trace can only be confirmed with eyes in Rack."
    result: passed
    verified_by: operator
    verified_at: 2026-07-09
  - test: "BUG-04 wiring — corrupt-patch load survives in Rack (jansson path)"
    expected: "Hand-edit a .vcv patch and set spreadSeed0 to \"zzzz\"; load it in Rack; Rack must not crash and the module must run with a valid seed."
    why_human: "The parseSeedHex helper is headlessly pinned (test_regression.cpp BUG-04 case). The crash path that was removed (std::stoull throwing in dataFromJson) is exercised by Rack's jansson JSON load machinery, which is not reachable from make test. The unit test proves the helper does not throw; the in-Rack load proves dataFromJson's wiring does not crash Rack itself."
    result: passed
    verified_by: operator
    verified_at: 2026-07-09
---

# Phase 23: Functional Bug Fixes — Verification Report

**Phase Goal:** All four CODE-REVIEW functional bugs are fixed, each pinned by a regression test that fails on the old code and passes on the new — with the x1.5/÷1.5 behavior change confirmed by an in-Rack listening test first.
**Verified:** 2026-06-15T01:00:00Z
**Status:** passed (all automated must-haves verified; 2 GUI/jansson items confirmed in-Rack by operator on 2026-07-09)
**Re-verification:** No — initial verification.

---

## Goal Achievement

### Observable Truths

| #  | Truth | Status | Evidence |
|----|-------|--------|----------|
| 1 | Clock tracker re-acquires after a sustained >3x speedup (no permanent lockout) | VERIFIED | `ClockTracker.hpp`: `consecutiveOutliers` counter increments in the LOCKED outlier branch; at `OUTLIER_THRESHOLD = 3` drops to ACQUIRING (clockEdgeCount=1, counter reset) and falls through to EMA re-learn. Test "BUG-01: re-acquires after a sustained >3x SPEEDUP" exercises 120→540 BPM; `ct.smoothedPeriod < 0.25f` passes (was stuck at 0.499966 on unfixed header). |
| 2 | Clock tracker re-acquires in the fast-clock narrow slowdown band | VERIFIED | `consecutiveOutliers` counter handles the band where the 1s timeout floor cannot fire. Test "BUG-01: re-acquires in the fast-clock slowdown band" locks at 360 BPM then slows to 90 BPM (>3x, period 0.667s < 1s timeout floor); `ct.smoothedPeriod > 0.4f` passes (was stuck at 0.166654 on unfixed header). |
| 3 | A single genuine outlier edge is still rejected (no jitter on one glitch) | VERIFIED | Pre-existing test "ClockTracker: single outlier edge is rejected (smoothedPeriod unchanged)" still passes — a lone 0.05s edge at a 0.5s-locked clock is discarded; `ct.clockState == LOCKED` and `ct.smoothedPeriod == Approx(lockedPeriod)` both hold. |
| 4 | BUG-01 regression goes RED on the unfixed header and GREEN after the fix (TEST-05) | VERIFIED | SUMMARY 23-01 records commit `271c2f3` (RED): "BUG-01: re-acquires after a sustained >3x SPEEDUP" FAILED at L116 `CHECK(ct.smoothedPeriod < 0.25f)` — smoothedPeriod was 0.499966; commit `8cace3d` (GREEN): 37/37 cases all passed. Current `make test`: 43/43. |
| 5 | A malformed/over-long/empty seed hex string never throws — parse signals failure and the existing seed is kept | VERIFIED | `PatchParse.hpp` implements `parseSeedHex` with NULL/empty guard, `strtoull` + `errno == ERANGE` + `*end != '\0'` rejection. `dataFromJson` (AnalogLFO.cpp L244-255) parses into temporaries `s0`/`s1`; commits `spreadSeed[]` and calls `initComponentSpread()` only when BOTH return true. No `std::stoull` call remains in `dataFromJson`. |
| 6 | A valid hex seed round-trips through parseSeedHex | VERIFIED | `test_regression.cpp` BUG-04 TEST_CASE: `CHECK(forge::parseSeedHex("C0FFEE", v))` and `CHECK(v == 0xC0FFEEull)` both pass (43/43 run). |
| 7 | dataFromJson keeps the type-guard and only assigns seeds + regenerates spread when BOTH parse succeed | VERIFIED | `AnalogLFO.cpp` L244: `if (json_is_string(s0J) && json_is_string(s1J))` guard is present. L249-255: `parseSeedHex` called on both, commit inside `if` block only. No `std::stoull` in file. |
| 8 | Phase-dot display swing is the EFFECTIVE (gated) value: free-run stores 0.5, clocked stores swingFrac | VERIFIED (automated) | `AnalogLFO.cpp` L328: `displaySwingFraction.store(t.isClocked ? t.swingFrac : 0.5f, std::memory_order_relaxed)` — mirrors L344 buffer gate. That the phase dot visually tracks the trace requires the in-Rack manual UAT below. |
| 9 | BUG-04 regression goes RED on a tree without PatchParse.hpp wired and GREEN after (TEST-05) | VERIFIED | SUMMARY 23-02 records commit `4fb7306` (RED): `make test` failed to compile — `dsp/PatchParse.hpp` not found (symbol unresolved); commit `b0cc334` (GREEN): 38/38 cases passed. Current run: 43/43. |
| 10 | x1.5/÷1.5 audition DECISION (adopt-table) recorded in STATE.md BEFORE the RatioTable.hpp swap | VERIFIED | Git history confirms: commit `60420d7` "docs(23-04): log x1.5/÷1.5 audition decision" at 07:00:55 precedes commit `457770f` "fix(23-05): apply BEATS_PER_ALIGN table swap" at 07:21:29. STATE.md line 69 logs: "DECISION: adopt-table — rationale: in-Rack listening (operator, fresh-flushed CURRENT build, install hashes matched 3bb6fba before audition) confirmed the current cadence truncates mid-cycle…" |
| 11 | RatioTable.hpp BEATS_PER_ALIGN idx6→3 (/1.5) and idx8→2 (x1.5), 13 others unchanged | VERIFIED | `RatioTable.hpp` L55-58: `static constexpr int BEATS_PER_ALIGN[15] = { 16, 8, 6, 4, 3, 2, 3, 1, 2, 1, 1, 1, 1, 1, 1 };`. idx6=3, idx8=2. `shouldReset` reads it uniformly: `if (ratioIdx < 0) return true; return beatCount >= BEATS_PER_ALIGN[ratioIdx];`. Signature `bool shouldReset(int, int)` frozen (CR-03). `test_regression.cpp` BUG-02 EXPECTED[15] matches; 43/43 green. |

**Score: 11/11 truths verified** (Truths 8's visual half deferred to human verification; the code change itself is verified.)

---

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/dsp/ClockTracker.hpp` | Consecutive-outlier counter + recovery | VERIFIED | `consecutiveOutliers` member declared (L47); counter incremented in LOCKED outlier branch (L136); threshold drop-to-ACQUIRING (L137-145); reset on accepted edge (L155); reset on disconnect (L81) and timeout (L103). |
| `tests/test_dsp_stateful.cpp` | BUG-01 sustained-speedup + fast-slowdown-band regression cases | VERIFIED | Contains "BUG-01: re-acquires after a sustained >3x SPEEDUP" (L107-117) and "BUG-01: re-acquires in the fast-clock slowdown band" (L123-135). Uses `driveClock` helper. Inequality bands only, no `==` on EMA periods. |
| `src/dsp/PatchParse.hpp` | Non-throwing `forge::parseSeedHex(const char*, uint64_t&)` | VERIFIED | File exists, Rack-free (only `<cstdint> <cstdlib> <cerrno>`), `inline bool parseSeedHex(...)` with NULL/empty guard, strtoull, ERANGE/endptr checks, `out = (uint64_t)v; return true;` on success. |
| `tests/test_regression.cpp` | BUG-04 malformed-seed regression cases AND BUG-02 cadence regression | VERIFIED | Contains BUG-04 TEST_CASE (L20-27) and BUG-02 cadence TEST_CASE (L49-59). Includes both `dsp/PatchParse.hpp` and `dsp/RatioTable.hpp`. EXPECTED[15] matches adopt-table values. In-file comment quotes STATE.md decision line. |
| `src/AnalogLFO.cpp` | Hardened dataFromJson wiring + gated displaySwingFraction store | VERIFIED | `#include "dsp/PatchParse.hpp"` at L3. `parseSeedHex` called at L250-251. Gated store at L328. No `std::stoull` call in `dataFromJson`. |
| `src/dsp/RatioTable.hpp` | BEATS_PER_ALIGN[15] table swap (adopt-table) | VERIFIED | `static constexpr int BEATS_PER_ALIGN[15]` declared (L55-58); `shouldReset` reads it uniformly (L63-65). `round(1/ratio)` guard removed. `<cmath>` include retained (harmless per plan). |
| `.planning/STATE.md` | Logged audition decision under `### Decisions` before 23-05 | VERIFIED | Line 69 of STATE.md contains the `[Phase 23]: x1.5/÷1.5 audition — DECISION: adopt-table` entry. Commit ordering confirmed above. |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `tests/test_dsp_stateful.cpp` | `forge::ClockTracker::step` | `driveClock(ct, ...)` helper | VERIFIED | `driveClock` at L24-38 feeds square-wave clock to `ct.step()`; BUG-01 cases call it for both speedup and slowdown scenarios. |
| `ClockTracker.hpp` outlier branch | ACQUIRING re-learn path | `consecutiveOutliers` threshold drop | VERIFIED | L136-145: counter increments on outlier, at ≥3 sets `clockState = ACQUIRING; clockEdgeCount = 1; clockBeatCount = 0; consecutiveOutliers = 0;` then falls through (no early-return). |
| `src/AnalogLFO.cpp dataFromJson` | `forge::parseSeedHex` | Call after `json_is_string` guard | VERIFIED | L244 guard: `if (json_is_string(s0J) && json_is_string(s1J))`; L250-251: `forge::parseSeedHex(json_string_value(s0J), s0) && forge::parseSeedHex(...)`. |
| `src/AnalogLFO.cpp L328 displaySwingFraction.store` | L344 buffer-gen gate mirror | `t.isClocked ? t.swingFrac : 0.5f` | VERIFIED (code) | L328 and L344 both use identical ternary expression. Atomic `std::memory_order_relaxed` preserved. |
| `.planning/STATE.md ### Decisions` | `tests/test_regression.cpp EXPECTED[15]` + `RatioTable.hpp` edit | logged adopt-table decision selects cadence | VERIFIED | In-file comment at test_regression.cpp L36-46 quotes the STATE.md decision line verbatim. EXPECTED[15] values match adopt-table. |
| `src/dsp/ClockTracker.hpp L202` | `forge::shouldReset` | Single-home delegation (CR-03) | VERIFIED | L202: `bool reset = forge::shouldReset(currentRatioIdx, clockBeatCount);`. ClockTracker.hpp was NOT modified in 23-05; delegates to the updated function automatically. |

---

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|--------------------|--------|
| `AnalogLFO.cpp` L328 | `displaySwingFraction` | `t.isClocked ? t.swingFrac : 0.5f` (process() audio thread) | Yes — `t` is `forge::LfoCore::Telemetry` from the DSP step; real clocked/free-run gating | FLOWING |
| `AnalogLFO.cpp` L589 | `swingFrac` (GUI) | `module->displaySwingFraction.load(std::memory_order_relaxed)` | Yes — reads the atomic published by L328 | FLOWING |

---

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Full test suite green (43 cases) | `make test` (run) | `43 \| 43 passed \| 0 failed \| 2,589,928 assertions passed` | PASS |
| BUG-01 regression cases present in test_dsp_stateful.cpp | `grep -c "BUG-01" tests/test_dsp_stateful.cpp` | 4 occurrences (label in 2 TEST_CASEs + inline comments) | PASS |
| BUG-04 regression case present in test_regression.cpp | `grep -c "BUG-04" tests/test_regression.cpp` | 3 occurrences | PASS |
| BUG-02 cadence regression present in test_regression.cpp | `grep -c "BUG-02" tests/test_regression.cpp` | 3 occurrences | PASS |
| parseSeedHex wired in AnalogLFO.cpp | `grep -c "parseSeedHex" src/AnalogLFO.cpp` | 3 occurrences (include + 2 call sites) | PASS |
| Gated displaySwingFraction store at L328 | `grep -n "isClocked ? t.swingFrac : 0.5f" src/AnalogLFO.cpp` | Lines 328 and 344 — both match | PASS |
| BEATS_PER_ALIGN present in RatioTable.hpp | `grep -c "BEATS_PER_ALIGN" src/dsp/RatioTable.hpp` | 4 occurrences (declaration + comment + shouldReset use) | PASS |
| std::stoull removed from dataFromJson | `grep -n "stoull" src/AnalogLFO.cpp` | Line 243 — comment only (not a call) | PASS |
| STATE.md audition decision logged before RatioTable swap (git ordering) | git log | `60420d7` (decision, 07:00:55) before `457770f` (swap, 07:21:29) | PASS |

---

### Probe Execution

Step 7c: No probe scripts declared or discovered in `.planning/phases/23-functional-bug-fixes/`. The `make test` behavioral spot-check above serves as the executable validation.

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| BUG-01 | 23-01 | Clock tracker recovers from >3x / <1/3x tempo jumps via consecutive-outlier counting | SATISFIED | `consecutiveOutliers` in `ClockTracker.hpp`; two BUG-01 TEST_CASEs in `test_dsp_stateful.cpp`; 43/43 green. |
| BUG-02 | 23-05 (audition-gated via 23-04) | x1.5 and ÷1.5 ratios align on correct beat boundaries after in-Rack audition | SATISFIED | `BEATS_PER_ALIGN[15]` in `RatioTable.hpp` (idx6=3, idx8=2); BUG-02 cadence TEST_CASE in `test_regression.cpp`; audition DECISION logged in STATE.md before code landed. |
| BUG-03 | 23-02 | Phase dot tracks trace/audio in free-running mode when swing is set | SATISFIED (code) / NEEDS HUMAN (visual) | L328 gated store verified. Visual tracking of phase dot to trace in Rack is a GUI/render path — classified as human_verification below. |
| BUG-04 | 23-02 | Patch load survives malformed/corrupt JSON without crashing | SATISFIED (automated) / NEEDS HUMAN (in-Rack jansson path) | `parseSeedHex` unit-pinned (test_regression.cpp); `dataFromJson` wiring verified; in-Rack load path needs human spot-check (see below). |
| TEST-03 | 23-03 | Unit tests cover waveshape output ranges, clock ratio/alignment table, consecutive-outlier clock recovery, and swing math | SATISFIED | Full 50×50×128 waveshape grid (test_dsp_units.cpp L101-134); 13 un-gated ratios boundary-swept (L197-232); idx6/idx8 cadence pinned by test_regression.cpp BUG-02 case; swing free-run gate + clocked warp covered (L271-309); consecutive-outlier recovery in test_dsp_stateful.cpp (23-01). |
| TEST-05 | 23-01, 23-02, 23-05 | Each functional bug fix (#1–#3) pinned by a regression test that fails before the fix and passes after | SATISFIED | BUG-01: RED at `271c2f3`, GREEN at `8cace3d` (SUMMARY 23-01). BUG-04: RED at `4fb7306` (compile fail), GREEN at `b0cc334` (SUMMARY 23-02). BUG-02: RED at `3185bc3` (43 cases, 1 failed), GREEN at `457770f` (43/43, SUMMARY 23-05). |

**Note on REQUIREMENTS.md staleness:** The checklist in REQUIREMENTS.md still marks BUG-01 (`[ ]`) and BUG-02 (`[ ]`) as incomplete, and the traceability table shows them "Pending." This is a documentation staleness issue — the code and tests fully satisfy both requirements. The REQUIREMENTS.md was not updated when plans 23-01 and 23-05 completed. This should be updated to `[x]` and "Complete" for BUG-01 and BUG-02.

---

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/dsp/RatioTable.hpp` | 7 | `// Shape-for-P23 (D-04 / deferred)` | Info | Retrospective comment documenting a completed design decision. The BEATS_PER_ALIGN table is present and the `// FUTURE (P23)` marker at the former line 56 was replaced by the actual implementation. Not an unresolved marker — historical documentation. |
| `src/dsp/ClockTracker.hpp` | 201 | `// P23 patches shouldReset (CR-03)` | Info | Retrospective reference to Phase 23 work that was completed. Not a forward-deferred item. |

No `TBD`, `FIXME`, or `XXX` markers found in any file modified by this phase. No stub/placeholder implementations. No empty handlers or hardcoded empty arrays in rendered paths.

---

### Human Verification — RESOLVED (operator sign-off 2026-07-09)

Both in-Rack items below were confirmed PASSED by the operator on 2026-07-09.

#### 1. BUG-03 — Phase-dot tracking in free-run + swing (visual) — ✅ PASSED (2026-07-09)

**Test:** In Rack, select Medium or Heavy swing from the Analog LFO context menu. Then unplug the CLK input. Observe the phase dot position relative to the waveform trace and output signal.

**Expected:** The phase dot should sit on the waveform trace and audio output — NOT displaced ahead of (or behind) the trace as if it were warped by swing. The dot and trace should be in sync in free-run mode regardless of the swing setting.

**Why human:** The fix is a one-line change (AnalogLFO.cpp L328: now stores the effective gated value `t.isClocked ? t.swingFrac : 0.5f` rather than raw `t.swingFrac`). The code change is verified. However, the correctness of the visual dot-trace alignment requires the full Rack GUI → audio → display rendering pipeline. There is no headless path that exercises the atomic publish (audio thread, L328) → GUI draw consumer (display thread, L589) → phase-dot position calculation chain.

---

#### 2. BUG-04 — Corrupt-patch load in Rack (jansson integration) — ✅ PASSED (2026-07-09)

**Test:** In a text editor, open a saved `.vcv` patch that contains the Analog LFO module. Find the JSON field `spreadSeed0` inside the module's data object and change its value to `"zzzz"` (non-hex). Save the patch. Load it in Rack.

**Expected:** Rack must not crash. The Analog LFO must load and run with a valid internal seed (the constructor-seeded value is kept on parse failure). No error modal beyond normal Rack module-load warnings is expected.

**Why human:** `forge::parseSeedHex` is fully unit-pinned (test_regression.cpp BUG-04 case proves the helper returns false on malformed input, never throws). The crash path that was eliminated — `std::stoull` throwing `std::invalid_argument` or `std::out_of_range` inside `dataFromJson` — is invoked by Rack's jansson load machinery, which requires a live Rack process. `make test` drives the helper in isolation but does not instantiate `dataFromJson` or the full jansson deserialization chain.

---

### Gaps Summary

No automated must-haves are unmet. The two human verification items above are classified as such in the PLAN frontmatter itself (plan 23-02 verification block: "Manual UAT (GUI/jansson-only — NOT headless-reachable, deferred to operator)") and in the phase goal ("BUG-03's consumer (phase dot) + BUG-04's dataFromJson end-to-end are GUI/jansson MANUAL UATs").

Both items have since been confirmed in-Rack by the operator on 2026-07-09; `status` is now `passed`.

---

_Verified: 2026-06-15T01:00:00Z_
_Verifier: Claude (gsd-verifier)_
_Human UAT resolved: 2026-07-09 — BUG-03 + BUG-04 confirmed PASSED in-Rack by operator_
