---
phase: 23-functional-bug-fixes
plan: 02
subsystem: testing
tags: [vcv-rack, c++17, doctest, input-validation, asvs-v5, atomics, json]

# Dependency graph
requires:
  - phase: 22-test-harness-foundation
    provides: Rack-free src/dsp/*.hpp core + make test doctest harness (auto-globbed TUs/headers)
provides:
  - "Rack-free forge::parseSeedHex non-throwing hex parser (src/dsp/PatchParse.hpp)"
  - "Hardened dataFromJson: malformed/over-long/empty seed strings can no longer crash Rack on load"
  - "Gated phase-dot display swing (effective value) so the dot tracks trace/audio in free-run+swing"
  - "tests/test_regression.cpp BUG-04 TEST-05 red->green pin"
affects: [23-04, 23-05, release-packaging, code-review-remediation]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Rack-free pure-function header in namespace forge (strtoull + ERANGE/endptr) for shell-only logic, headlessly pinned"
    - "Parse-into-temporaries + commit-only-on-success fallback pattern at the patch trust boundary"

key-files:
  created:
    - src/dsp/PatchParse.hpp
    - tests/test_regression.cpp
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "Extracted parseSeedHex into a Rack-free header (RESEARCH A4 option a) so BUG-04 earns a real TEST-05 headless pin rather than manual-only verification"
  - "On any seed parse failure, keep the constructor-seeded spread (do not call initComponentSpread()) — CODE-REVIEW #4 safe fallback"

patterns-established:
  - "Patch-boundary input validation: type-guard (json_is_string) THEN non-throwing parse THEN commit-on-success-only"
  - "Mirror the already-correct buffer-gen gate (t.isClocked ? t.swingFrac : 0.5f) when publishing display atoms"

requirements-completed: [BUG-03, BUG-04, TEST-05]

# Metrics
duration: 8min
completed: 2026-06-14
---

# Phase 23 Plan 02: Patch-Load Crash Guard + Free-Run Phase-Dot Swing Summary

**Non-throwing `forge::parseSeedHex` (strtoull + ERANGE/endptr) wired into `dataFromJson` closes the malformed-seed patch-load crash (BUG-04, red→green), and the phase-dot display swing is gated to its effective value so the dot tracks the trace in free-run+swing (BUG-03).**

## Performance

- **Duration:** ~8 min
- **Started:** 2026-06-14T10:31:50Z
- **Completed:** 2026-06-14T10:39:16Z
- **Tasks:** 3
- **Files modified:** 3 (2 created, 1 edited)

## Accomplishments
- **BUG-04 (DoS / ASVS V5):** Replaced the throwing `std::stoull` seed parse with a Rack-free, non-throwing `forge::parseSeedHex`. A hand-edited patch with a non-hex, over-long, or empty seed string can no longer throw and crash Rack on load — the existing seed is kept on any failure.
- **TEST-05 red→green for BUG-04:** New `tests/test_regression.cpp` was demonstrably RED (build failed: `dsp/PatchParse.hpp` not found) before the helper existed, GREEN (38/38 cases, 1,647,407 assertions) after.
- **BUG-03 (consumer):** The `displaySwingFraction` store now publishes the EFFECTIVE gated value (`t.isClocked ? t.swingFrac : 0.5f`), mirroring the buffer-gen gate. The phase dot no longer warps with raw swing in free-run.
- Plugin still builds clean against `../Rack-SDK` (wave-merge check passed).

## Task Commits

Each task was committed atomically:

1. **Task 1: BUG-04 RED regression** — `4fb7306` (test) — new `tests/test_regression.cpp`; `make test` fails to compile (PatchParse.hpp absent) = recorded RED
2. **Task 2: PatchParse.hpp + dataFromJson wiring (GREEN)** — `b0cc334` (fix) — Rack-free parser + hardened parse; `make test` 38/38 green
3. **Task 3: Gate display-swing store (BUG-03)** — `4bd5ffa` (fix) — L316 now stores the effective gated value

**Plan metadata:** (this commit) — docs: complete plan

## Files Created/Modified
- `src/dsp/PatchParse.hpp` (NEW) — Rack-free `inline bool forge::parseSeedHex(const char*, uint64_t&)`; std headers only (`<cstdint> <cstdlib> <cerrno>`), zero Rack-SDK includes, linkable in `make test`.
- `tests/test_regression.cpp` (NEW) — BUG-04 TEST-05 pin: non-hex `"zzzz"`, over-long, empty → false; valid hex round-trips.
- `src/AnalogLFO.cpp` (MOD) — added `#include "dsp/PatchParse.hpp"`; rewrote `dataFromJson` seed parse behind the existing `json_is_string` guard (parse into temporaries, commit + `initComponentSpread()` only when BOTH succeed); gated the L316 `displaySwingFraction.store`.

## Decisions Made
- **Extract `parseSeedHex`** (RESEARCH A4 / Open-Q1, option a) rather than manual-only verification — turns a crash-class bug into a deterministic headless regression with negligible blast radius.
- **Safe fallback on parse failure:** keep the constructor-seeded spread (no `initComponentSpread()`), matching CODE-REVIEW #4.
- **CR-02 NULL-deref guard preserved:** the `json_is_string(s0J) && json_is_string(s1J)` guard stays exactly as-is; `parseSeedHex` additionally NULL/empty-guards.

## Deviations from Plan

None - plan executed exactly as written. The three tasks ran in order; RED was demonstrated before GREEN for BUG-04; no auto-fixes (Rules 1–4) were triggered; no out-of-scope work was performed (the deferred `swingIndex` non-atomic write was left untouched).

## Issues Encountered
None.

## Verification

**Automated (passing):**
- `make test` → 38/38 cases, 1,647,407 assertions, SUCCESS (BUG-04 case green; was unbuildable pre-fix).
- `make RACK_DIR=../Rack-SDK` → plugin.dylib links clean (wave-merge check).
- `src/dsp/PatchParse.hpp` contains zero Rack-SDK `#include` lines (verified) — links in the Rack-free harness.

**Manual UAT (GUI/jansson-only — NOT headless-reachable, deferred to operator):**
- **BUG-03 consumer (phase-dot tracking):** In Rack, select Medium (or Heavy) swing from the context menu and unplug the clock; confirm the phase dot sits on the trace/output and is NOT warped ahead of it. *(Status: pending operator — code change mirrors the already-verified L332 buffer gate.)*
- **BUG-04 wiring (corrupt-patch load):** Hand-edit a `.vcv` patch, set `spreadSeed0` to `"zzzz"`, and load it; Rack must not crash and the module keeps a valid seed. *(Status: pending operator — the throwing path is removed and the helper is unit-pinned.)*

## TDD Gate Compliance
BUG-04 followed strict RED→GREEN: `test(23-02)` commit `4fb7306` (RED, build fails — symbol unresolved) precedes `fix(23-02)` commit `b0cc334` (GREEN, 38/38). No REFACTOR needed.

## Next Phase Readiness
- SC2 satisfied on the automated half: patch load survives malformed JSON (type-guard + non-throwing parse + safe fallback); phase-dot uses the effective gated display swing. Two manual UATs remain for the operator (GUI/jansson glue, by design unreachable from `make test`).
- `forge::parseSeedHex` is available as a reusable Rack-free helper for any future patch-field hardening.

## Self-Check: PASSED
- Files: src/dsp/PatchParse.hpp, tests/test_regression.cpp, 23-02-SUMMARY.md — all FOUND
- Commits: 4fb7306, b0cc334, 4bd5ffa — all FOUND

---
*Phase: 23-functional-bug-fixes*
*Completed: 2026-06-14*
