---
phase: 29-vco-test-harness-lfo-non-regression-guardrail
plan: 01
subsystem: testing
tags: [doctest, cpp11, cpp17, odr, xoroshiro, block-driver, dsp-seam, vcv-rack]

# Dependency graph
requires:
  - phase: 22-dsp-extraction
    provides: "src/dsp/LfoCore.hpp POD-seam shape, tests/BlockDriver.hpp harness pattern, the Rack-free `make test` target"
  - phase: 23-golden-replay
    provides: "tests/golden/*.f32 fixtures and the bit-exact replay discipline this plan must not disturb"
provides:
  - "src/dsp/VcoCore.hpp — the frozen VCO boundary contract (forge::VcoInputs POD in, float out, nested Telemetry) that Phases 30-36 build against"
  - "tests/VcoBlockDriver.hpp — Rack-free block driver over forge::VcoCore with unconditional sampleTime/sampleRate injection and proven non-degenerate seed defaults"
  - "tests/test_vco_harness.cpp — seven 'vco harness:' invariants across 44.1/48/96 kHz"
  - "forge::VcoBlockDriver::sweepScenario — the varying-input anti-vacuity driver later phases reuse"
  - "A TOMBSTONE test that forces Phase 30 to acknowledge the seam changed"
affects: [30-vco-core-oscillator, 31-vco-pitch-chain, 33-hard-sync, 34-analog-character, 35-vco-panel-display, 36-vco-golden-capture]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "POD seam mirroring src/dsp/LfoCore.hpp: NSDMI struct in, float out, nested Telemetry, seed/setSpreadSeed, zero Rack-SDK includes"
    - "Independent (never templated) per-core block drivers in tests/"
    - "Seam-observability telemetry fields to make harness injection assertions non-vacuous"
    - "In-source labelling of known-weak invariants instead of claiming coverage"

key-files:
  created:
    - src/dsp/VcoCore.hpp
    - tests/VcoBlockDriver.hpp
    - tests/test_vco_harness.cpp
  modified: []

key-decisions:
  - "VCO POD named forge::VcoInputs, never a second forge::Inputs — a duplicate is a cross-TU ODR violation that only detonates on the CI MinGW link leg (R-9)"
  - "tests/VcoBlockDriver.hpp is an independent ~90-line copy of tests/BlockDriver.hpp; the two drivers are never merged into a shared template because BlockDriver feeds the shipped-LFO bit-exact golden leg (R-2/P-4)"
  - "DriftEngine is held and seeded by VcoCore from day one even though step() never runs it, so Phases 30/34 never have to change the driver"
  - "Invariants 5 and 6 are weak by construction against a silent seam and are labelled as such in source, driven by a varying sweep so they become load-bearing when Phase 30 DSP lands (P-7)"
  - "A TOMBSTONE case asserts the seam is silent; Phase 30 is required to delete it"
  - "Sync fields (syncVoltage/syncConnected) deliberately deferred to Phase 33 — additive POD growth is non-breaking while the VCO has no goldens"

patterns-established:
  - "Two-standard header rule: every VCO header compiles clean under both -std=c++11 -pedantic-errors and -std=c++17"
  - "Throwaway probe TU in gitignored build-test/ for header syntax gates (avoids the spurious '#pragma once in main file' diagnostic)"
  - "Comment-stripped negative greps for acceptance criteria, so a banner may legitimately name the constructs it forbids"

requirements-completed: [TEST-01]

coverage:
  - id: D1
    description: "src/dsp/VcoCore.hpp — bare POD VCO seam (forge::VcoInputs 11 fields, forge::VcoCore with DriftEngine, Telemetry, seed/setSpreadSeed, silent step) compiling under C++11-pedantic and C++17 with zero Rack-SDK includes"
    requirement: "TEST-01"
    verification:
      - kind: other
        ref: "c++ -std=c++11 -pedantic-errors -fsyntax-only -Wall -Wextra -Isrc build-test/vcocore_probe.cpp && c++ -std=c++17 -fsyntax-only -Wall -Wextra -Isrc build-test/vcocore_probe.cpp"
        status: pass
      - kind: unit
        ref: "tests/test_vco_harness.cpp#vco harness: drives VcoCore over blocks at 44.1 / 48 / 96 kHz Rack-free"
        status: pass
    human_judgment: false
  - id: D2
    description: "tests/VcoBlockDriver.hpp — Rack-free block driver over forge::VcoCore with unconditional sampleTime + sampleRate injection and proven non-degenerate seed defaults"
    requirement: "TEST-01"
    verification:
      - kind: unit
        ref: "tests/test_vco_harness.cpp#vco harness: overwrites caller sampleTime with 1/sampleRate every step"
        status: pass
      - kind: unit
        ref: "tests/test_vco_harness.cpp#vco harness: injects sampleRate every step"
        status: pass
      - kind: unit
        ref: "tests/test_vco_harness.cpp#vco harness: default seeds are non-degenerate (never the (0,0) Xoroshiro fixed point)"
        status: pass
    human_judgment: false
  - id: D3
    description: "A single `make test` run drives forge::VcoCore over sample blocks at 44100 / 48000 / 96000 Hz with no libRack linked and no Rack include path"
    requirement: "TEST-01"
    verification:
      - kind: integration
        ref: "make test (57 cases / 2,615,061 assertions, 0 failed) then ./build-test/test -tc=\"vco harness*\" (7 passed)"
        status: pass
    human_judgment: false
  - id: D4
    description: "Every shipped LFO source, test and fixture file is byte-identical to tag v2.0.1 — tests/BlockDriver.hpp was copied, never edited or templated (R-2)"
    verification:
      - kind: other
        ref: "git diff --exit-code v2.0.1 -- tests/BlockDriver.hpp tests/test_golden.cpp tests/golden src/AnalogLFO.cpp src/plugin.cpp src/plugin.hpp plugin.json src/dsp/*.hpp"
        status: pass
      - kind: unit
        ref: "./build-test/test -tc=\"golden*\" (6 passed — shipped-LFO replay unaffected)"
        status: pass
    human_judgment: false
  - id: D5
    description: "Known-weak invariants (seam determinism, finiteness) are labelled in source rather than presented as coverage they do not yet provide, and a TOMBSTONE case forces Phase 30 to acknowledge the seam changed"
    verification: []
    human_judgment: true
    rationale: "Whether the in-source caveats are honest and legible enough to survive into Phase 30 is an editorial judgment about intent, not something a test can assert."

# Metrics
duration: 12min
completed: 2026-07-28
status: complete
---

# Phase 29 Plan 01: VCO Seam & Rack-Free Block-Driver Harness Summary

**A bare POD `forge::VcoCore` seam plus an independent Rack-free block driver and seven `vco harness:` doctest invariants across 44.1/48/96 kHz — the boundary Phases 30-36 build against, landed with every shipped LFO file byte-identical to v2.0.1.**

## Performance

- **Duration:** ~12 min
- **Started:** 2026-07-28T05:13Z
- **Completed:** 2026-07-28T05:25Z
- **Tasks:** 3
- **Files created:** 3 (0 modified)

## Accomplishments

- **`src/dsp/VcoCore.hpp`** — the Phase 29 boundary contract. `forge::VcoInputs` (11 NSDMI fields: pitch/coarse/fine, exponential-FM triple, morph/character/drift, injected sampleTime/sampleRate) mirrors `src/dsp/LfoCore.hpp`'s POD shape exactly. `forge::VcoCore` holds and seeds a `DriftEngine`, exposes a nested `Telemetry` (shell-facing `freqHz`/`displayPhase`/`syncFired` plus three seam-observability fields), and `step()` returns silence per D-01. Compiles clean under both `-std=c++11 -pedantic-errors -Wall -Wextra` and `-std=c++17` with zero Rack-SDK includes and no C++17-isms.
- **`tests/VcoBlockDriver.hpp`** — an independent copy of the LFO driver retargeted to the VCO seam, keeping the proven non-degenerate seed defaults verbatim, unconditionally injecting `sampleTime = 1/sampleRate` *and* `sampleRate`, and adding `sweepScenario` (a varying pitch/morph/character ramp) in place of the LFO-specific `clockedScenario`.
- **`tests/test_vco_harness.cpp`** — seven `vco harness:` cases: Rack-free block drive at three rates, `sampleTime` overwrite, `sampleRate` injection, non-degenerate seeding proven via a live spread coefficient, seam determinism, finiteness, and the silent-by-construction TOMBSTONE.
- **Zero build/CI edits.** `TEST_SOURCES := $(wildcard tests/*.cpp)` and `TEST_HEADERS` already cover the new files; `Makefile` and `.github/workflows/test.yml` are untouched, so `TEST_CXXFLAGS` (and therefore both golden legs) cannot have moved.
- **LFO guardrail held.** Full suite: 57 cases / 2,615,061 assertions, 0 failed. `-tc="golden*"` still 6 passed. `git diff --exit-code v2.0.1` over all 17 shipped LFO source/test/fixture paths exits 0.

## Task Commits

1. **Task 1: Create the bare POD VCO seam `src/dsp/VcoCore.hpp` (D-01/D-03)** — `9958514` (feat)
2. **Task 2: Copy BlockDriver into an independent `tests/VcoBlockDriver.hpp` (R-2)** — `517d339` (test)
3. **Task 3: TEST-01 harness invariants in `tests/test_vco_harness.cpp`** — `38165dd` (test)

## Files Created/Modified

- `src/dsp/VcoCore.hpp` (new, 112 lines) — the VCO boundary contract: `forge::VcoInputs` POD, `forge::VcoCore` with `DriftEngine`, `Telemetry`, `seed`/`setSpreadSeed`, and a silent `step()`.
- `tests/VcoBlockDriver.hpp` (new, 91 lines) — Rack-free block driver over `forge::VcoCore`; `run()` + `sweepScenario()`.
- `tests/test_vco_harness.cpp` (new, 207 lines) — seven TEST-01 harness invariants over three sample rates.

No existing file was modified. `Makefile`, `.github/workflows/test.yml`, `tests/BlockDriver.hpp`, `tests/test_golden.cpp`, `tests/golden/*`, all 11 pre-existing `src/dsp/*.hpp`, and `src/AnalogLFO.cpp` are untouched.

## Decisions Made

- **`forge::VcoInputs`, never a second `forge::Inputs`.** The LFO owns `forge::Inputs`; a duplicate compiles silently in any TU that includes only one header and detonates only on the CI MinGW link leg — the exact ODR class that got v2.0.0 rejected. The banner records this so nobody "tidies" the name later.
- **Two independent block drivers, permanently.** `tests/BlockDriver.hpp` feeds the macOS bit-exact drift-ON golden leg; templating or subclassing would change what that leg feeds `forge::LfoCore` and move `tests/golden/freerun_*.f32`. ~90 duplicated lines is the cheaper trade, and the duplication rationale is written into the new file's banner.
- **`DriftEngine` is wired into the seam now, not in Phase 30.** `step()` never calls `drift.step()` (that would be DSP), but holding and seeding it makes the driver's seeding discipline real from day one, so Phases 30/34 inherit a driver they never have to change.
- **Weak invariants are labelled, not hidden.** Determinism and finiteness are trivially true against a silent core; both carry an in-source note saying so and are driven by `sweepScenario` rather than a constant input, so they start proving something the instant Phase 30 lands DSP.
- **TOMBSTONE case adopted** (RESEARCH left it as the planner's call). One line of intentional churn buys a hard signal that Phase 30 cannot land DSP without consciously revisiting this suite.
- **Sync fields deferred to Phase 33.** Additive POD growth is non-breaking while the VCO has no golden fixtures (they arrive in Phase 36).
- **Compile gates run through a throwaway probe TU** in the gitignored `build-test/`, because compiling a header directly emits a spurious `#pragma once in main file` diagnostic unrelated to the code under test.

## Deviations from Plan

None - plan executed exactly as written.

No deviation rule fired. No auto-fix was required: all three tasks compiled and passed their acceptance criteria on the first run, and no missing critical functionality, blocking issue, or architectural question surfaced.

## Issues Encountered

None.

## LFO Non-Regression Guardrail Status

The project guardrail (never break the shipped v2.0.1 LFO) was respected by construction, not by luck:

| Risk | Status |
|---|---|
| R-2 — `tests/BlockDriver.hpp` templated/edited | Not touched. `git diff --exit-code v2.0.1` clean; copy-not-share rationale written into `VcoBlockDriver.hpp`'s banner. |
| R-4 — `TEST_CXXFLAGS` changed (moves float results) | `Makefile` and `.github/workflows/test.yml` unmodified — `git status --porcelain` on both is empty. |
| R-6 — `make capture` regenerating goldens | Never run. `tests/golden/*` byte-identical to v2.0.1. |
| R-9 — second `forge::Inputs` (cross-TU ODR) | POD named `VcoInputs`; comment-stripped grep asserts no `struct Inputs {` in the new header. |
| Shipped-LFO behavior | `-tc="golden*"` 6 passed; full suite 57/57. |

## Known Stubs

`forge::VcoCore::step()` returns `0.f`. This is **intentional and specified** (D-01: Phase 29 delivers the boundary contract only; all VCO DSP is Phase 30 CORE-01). It is not a hidden stub: the header banner states it, and `tests/test_vco_harness.cpp`'s TOMBSTONE case asserts it and names Phase 30 as the required remover. `Telemetry::freqHz`, `displayPhase` and `syncFired` are likewise declared-but-unpopulated placeholders owned by Phases 31, 35 and 33 respectively.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Ready for plan 29-02** (shipped-LFO golden replay hash guard) — that plan is independent of this one and touches no file created here.
- **Ready for plan 29-03** (compile canary): the canary TU must `#include "dsp/VcoCore.hpp"` and ODR-use `forge::VcoCore` from an external-linkage function with a runtime-dependent argument, per D-08. The header is already C++11-pedantic clean, so the canary should compile on the first try.
- **Ready for plan 29-04** (frozen-header hash guard): `src/dsp/VcoCore.hpp` is a *new, still-mutable* header — it belongs in the D-06 include audit but should **not** be added to the `src/dsp/FROZEN.sha256` manifest, which pins the shipped LFO headers. `tests/BlockDriver.hpp` should be added to that manifest as planned.
- **Phase 30 (CORE-01) obligations, in order:** replace the silent `step()` body; delete the `"vco harness: TOMBSTONE"` case; and revisit the two invariants labelled weak-by-construction so their in-source caveats are removed once they genuinely bite.

## Self-Check: PASSED

All three created files exist on disk and all three task commits are present in git history (`9958514`, `517d339`, `38165dd`).

---
*Phase: 29-vco-test-harness-lfo-non-regression-guardrail*
*Completed: 2026-07-28*
