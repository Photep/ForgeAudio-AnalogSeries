---
phase: 31-pitch-tuning-exponential-fm
plan: 08
subsystem: verification
tags: [phase-gate, test-02, matched-case-count, whole-diff-prohibition-sweep, ci-by-sha, windows-link-leg, deferred-register, guardrail, criterion-hygiene, measured-figures]

# Dependency graph
requires:
  - phase: 31-pitch-tuning-exponential-fm
    plan: 07
    provides: "invariants 8 and 9, the six selectors this gate asserts matched counts for, PITCH-04/PITCH-05 marked complete, and the two hand-offs this plan had to record rather than drop"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 06
    provides: "invariants 4-7, the FM identity grid with its four measured blind rows, and the 5343-sample negative control"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 05
    provides: "the TEST-02 gate itself, its two tiers, its derived ceilings and the six measured worst-case figures"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 04
    provides: "the shell whose declared control ranges the reachable-envelope margin is computed FROM"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 03
    provides: "the pitch chain, kVcoMaxPitchVolts, and the UBSan RED/GREEN transcript pair that is item 1's evidence of record"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 01
    provides: "the pre-registered VCO_SIDE_ALLOW entry and the WR-05 anchor fix this register records as RESOLVED"
  - phase: 30-vcocore-skeleton-module-registration
    provides: "deferred-items.md as the format to follow, the six inherited items, and the three CI-observation lessons (locate BY SHA, read the LEG's OWN step conclusion, ACCOUNT for the matrix case-count gap)"
provides:
  - "TEST-02 discharged as a HARD gate: all six selectors green AND each proven to have matched its exact expected case count (2/1/1/2/1/1), with a negative control demonstrating that a selector matching ZERO cases also exits 0 and prints Status: SUCCESS!"
  - "the four local gates green together on ONE tree with their numbers: make test 81/2,618,053/0; make guards PASS; make strict PASS over four TUs; a real 169,072-byte plugin.dylib exporting BOTH model symbols"
  - "the whole-phase prohibition sweep over da266bb..80fb90a: no frozen header, no FROZEN.sha256, no src/AnalogLFO.cpp, exactly ONE exp2_taylor5 in the pitch chain, no NEW libm/inline-constexpr/in-class-constexpr, no sanitizer wiring, hostile timing grids byte-identical, both historical literals intact, and the non-.planning file list exactly the six declared"
  - "the off-machine matrix observed green BY HASH EQUALITY: run 30511183170 on 80fb90a81b442731c5d06d11970a65b148caaf1f, toolchain-gate success AND its step 6 'win-x64 leg reproduction (compile + full link vs libRack)' OWN conclusion success"
  - "the matrix case-count gap ACCOUNTED FOR on both axes and both sides: +9 cases / +1,941 assertions on EVERY leg, and the macOS-vs-others gap unchanged at exactly 3 cases / 24,582 assertions before and after - measured, not inferred from the absence of a platform gate"
  - ".planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md - 13 items including D-24 pointed at NO PHASE, the half-closed clamp item, and 31-07's two hand-offs"
  - "FOUR criterion-filter artifacts identified and reported with their honest forms, including one (std::pow) that can NEVER return zero in this repository"
affects: [31-09, 32-morph-blep, 33-hard-sync, 34-drift-output, 35-panel-display, 36-goldens-release]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "A selector's MATCHED CASE COUNT is the gate; its exit status is not. Demonstrated rather than argued: a deliberately nonsense -tc pattern reports '0 passed | 0 failed', prints 'Status: SUCCESS!' and exits 0"
    - "Account for a platform gap by measuring BOTH sides of it. A per-leg BEFORE/AFTER delta (+9 cases on all three legs) proves the new cases ran everywhere; the absence of an #if in the new file only proves nothing gates them"
    - "A prohibition whose grep can never return zero must be re-stated as a BASELINE COMPARISON. std::pow legitimately lives in a frozen shipped header, so 'zero libm under src/' is unsatisfiable - 'unchanged from the pre-phase commit, and zero in the files this phase touched' is the checkable claim"
    - "A green CI run for a DIFFERENT commit sits one line above the right one in `gh run list`. The pre-phase commit's own run was green and adjacent; hash equality is the only thing that distinguishes them"
    - "Record a half-closed inherited item as HALF closed, naming which half and why the other half is still unreachable. Resolving it would be a false green; re-deferring the whole thing would discard real work"

key-files:
  created:
    - ".planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md"
    - ".planning/phases/31-pitch-tuning-exponential-fm/31-08-SUMMARY.md"
  modified: []

key-decisions:
  - "Tasks 1 and 2 carry NO commit: both are verification-only by design and modify no repository file. Only Task 3 commits"
  - "Four acceptance criteria were evaluated in their HONEST form and the artifact reported: std::pow under src/ (1 non-comment hit, a frozen shipped header, unchanged from baseline), inline constexpr per-file (3 raw hits, all comment lines forbidding the construct), static constexpr (14 non-comment hits, all pre-existing and mostly the PERMITTED namespace-scope form), and the '#if defined(__APPLE__)' directive count (2 directives gating 3 cases)"
  - "The matrix gap was accounted for by measuring the pre-phase CI run's per-leg counts rather than by citing STATE.md's recorded figures - which is what turns 'expected' into 'observed'"
  - "No requirement marked complete by this plan. All nine were already marked by 31-05/06/07; this gate re-verifies their evidence and records READINESS"
  - "coreBase()'s two falsified annotations were RECORDED, not fixed: this plan's prohibitions forbid editing anything under tests/"

patterns-established:
  - "The pre-phase commit's own green CI run is the sharpest available demonstration of why hash equality matters - it is green, it is adjacent in the listing, and it says nothing about this phase"
  - "Reconcile an assertion delta down to the individual case. +1,941 = 1,911 (the eight cases the six selectors reach) + 30 (invariant 1, which no selector reaches) closes the loop on all nine"

requirements-completed: []

coverage:
  - id: D1
    description: "TEST-02 as a HARD gate: every selector exits 0 AND is proven to have matched its exact expected number of cases, because a selector that matches nothing also exits 0"
    requirement: "TEST-02"
    verification:
      - kind: unit
        ref: "six selectors, matched cases / assertions / exit: -tc=\"*v/oct tracking*\" 2/603/0; -tc=\"*COARSE*\" 1/170/0; -tc=\"*FINE*\" 1/156/0; -tc=\"*exponential FM*\" 2/494/0; -tc=\"*Nyquist*\" 1/93/0; -tc=\"*hostile pitch*\" 1/395/0. Every count equals its expectation exactly (2,1,1,2,1,1)"
        status: pass
      - kind: unit
        ref: "NEGATIVE CONTROL on the gate mechanism itself: -tc=\"*this case name does not exist anywhere*\" reports '0 | 0 passed | 0 failed | 81 skipped', prints 'Status: SUCCESS!' and exits 0. That is the false green the matched-count assertion refuses"
        status: pass
    human_judgment: false
  - id: D2
    description: "The whole-suite case count equals the recorded pre-phase baseline plus nine, reconciled case by case AND assertion by assertion"
    requirement: "TEST-02"
    verification:
      - kind: unit
        ref: "make test: 81 cases / 2,618,053 assertions / 0 failed, zero compiler warnings from a clean rebuild. Baseline 72 (31-VALIDATION.md regression floor) + 9 = 81. grep -c '^TEST_CASE(' tests/test_vco_pitch.cpp == 9, all nine enumerated by name. Assertions reconcile too: the six selectors reach 8 of the 9 cases at 1,911 assertions, invariant 1 (the apparatus self-check, reached by no selector) measures 30, and 1,911 + 30 = 1,941 = the measured per-leg delta on all three CI legs"
        status: pass
    human_judgment: false
  - id: D3
    description: "All four local gates green on ONE tree, including a REAL plugin link - because a prior phase measured the entire local gate returning success on a commit that could not link"
    verification:
      - kind: integration
        ref: "make test 81/2,618,053/0 (14.788 s from a removed build-test/); make guards exit 0 with 'guard suite: PASS' across check_frozen (15 pinned entries + 6 goldens + negative control), check_includes ([1/7]..[7/7] incl. the nc5 comment-evasion control) and check_canary ([1/5]..[5b/5], all 8 DSP fields runtime-live at -O3); RACK_DIR=../Rack-SDK make strict exit 0 'strict C++11 gate: PASS' over four TUs; RACK_DIR=../Rack-SDK make from a removed build/ links plugin.dylib at 169,072 bytes with nm -gU showing BOTH _modelAnalogLFO and _modelAnalogVCO"
        status: pass
    human_judgment: false
  - id: D4
    description: "T-31-04: the milestone guardrail holds over the WHOLE phase diff, not per plan"
    requirement: "guardrail"
    verification:
      - kind: integration
        ref: "git diff --name-only da266bb..HEAD | grep -cE 'AnalogLFO\\.cpp|RackCompat\\.hpp|Waveshape\\.hpp|LfoCore\\.hpp|MathConst\\.hpp|DriftEngine\\.hpp|FROZEN\\.sha256' == 0. Only two src/ paths appear at all: src/dsp/VcoCore.hpp and src/AnalogVCO.cpp. FROZEN.sha256 absent and still 15 entries. Zero deletions anywhere in the phase diff. The six LFO goldens replay byte-identical: -tc=\"*golden*\" 9 cases / 49,188 assertions; -tc=\"*guardrail*\" 10 / 60"
        status: pass
    human_judgment: false
  - id: D5
    description: "Exactly ONE exponential in the pitch chain, and no NEW libm exponential or power function under src/"
    requirement: "FM-03"
    verification:
      - kind: integration
        ref: "grep -v '^[[:space:]]*//' src/dsp/VcoCore.hpp | grep -c 'exp2_taylor5(' == 1, the line being 'float freq = kVcoFreqC4 * exp2_taylor5(pitchVolts);'. Non-comment std::exp2 and std::pow are BOTH 0 in src/dsp/VcoCore.hpp and src/AnalogVCO.cpp - the two files this phase touched. Tree-wide raw counts are IDENTICAL at the pre-phase commit and now (std::exp2 1->1, std::pow 2->2)"
        status: pass
    human_judgment: false
  - id: D6
    description: "No inline constant-variable declaration and no in-class constant declaration introduced by this phase (the construct class that got v2.0.0 rejected from the VCV Library)"
    verification:
      - kind: integration
        ref: "non-comment 'inline constexpr' across src/ == 0. Both files this phase touched: non-comment inline constexpr 0, non-comment static constexpr 0. Tree-wide raw counts unchanged from da266bb (inline constexpr 3->3, static constexpr 20->20). Corroborated independently by make strict (-pedantic-errors C++11 rejects a real inline constexpr variable) and by check_canary.sh [4/5], whose negative control confirms a namespace-scope inline constexpr variable IS rejected for the expected reason"
        status: pass
    human_judgment: false
  - id: D7
    description: "T-31-15 / D-24: no permanent repository-wide sanitizer gate exists in the build file or any workflow"
    verification:
      - kind: integration
        ref: "grep -rc 'fsanitize' Makefile .github/ -> Makefile:0, .github/workflows/test.yml:0. Widened case-insensitively to 'fsanitize|ubsan|asan' -> 0 for both. .github/ contains exactly one file (workflows/test.yml), so the scan is complete"
        status: pass
    human_judgment: false
  - id: D8
    description: "D-15: the hostile TIMING grids in tests/test_vco_core.cpp are untouched across the whole phase"
    verification:
      - kind: integration
        ref: "git diff da266bb..HEAD -- tests/test_vco_core.cpp | grep -c 'HOSTILE_RATES\\|HOSTILE_TIMES' == 0; occurrence count 4 -> 4; and both arrays extracted from da266bb and from the working tree and diffed: BYTE-IDENTICAL"
        status: pass
    human_judgment: false
  - id: D9
    description: "Both historical measurement literals still carry their original digits"
    verification:
      - kind: integration
        ref: "grep -c '21609' src/dsp/VcoCore.hpp == 1 and tests/test_vco_core.cpp == 1, matching baseline 1/1. Both lines compared as TEXT against da266bb and identical ('Observed tel.freqHz = -21609.00,' and 'freqHz = -21609.00, phase = -9800.00, maxAbs = 1.476e38 V,'); only their line numbers moved (214->405 and 636->688)"
        status: pass
    human_judgment: false
  - id: D10
    description: "The phase diff's non-.planning file list is EXACTLY the declared set, with nothing extra"
    verification:
      - kind: integration
        ref: "git diff --name-only da266bb..HEAD | grep -v '^\\.planning/' returns exactly: res/AnalogVCO.svg, src/AnalogVCO.cpp, src/dsp/VcoCore.hpp, tests/check_includes.sh, tests/test_vco_core.cpp, tests/test_vco_pitch.cpp - the six the plans declared, no more. Working tree clean, no untracked residue"
        status: pass
    human_judgment: false
  - id: D11
    description: "T-31-28: the three-OS matrix and the Windows link leg observed green BY COMMIT HASH EQUALITY, with the LEG's OWN step conclusion read separately from its job's"
    requirement: "guardrail"
    verification:
      - kind: integration
        ref: "pushed 80fb90a81b442731c5d06d11970a65b148caaf1f; located run 30511183170 by selecting on headSha == the pushed hash; both printed and IDENTICAL, re-confirmed after completion. Jobs: toolchain-gate success, test (ubuntu-latest) success, test (macos-latest) success, test (windows-latest) success. Step 6 of toolchain-gate, named verbatim 'win-x64 leg reproduction (compile + full link vs libRack)', OWN conclusion = success, with 'win-x64 link gate: PASS' in its own log"
        status: pass
    human_judgment: false
  - id: D12
    description: "T-31-29: the matrix's case-count gap is ACCOUNTED FOR by measurement rather than assumed, and all nine of this phase's new cases are confirmed to have run on all three legs"
    verification:
      - kind: integration
        ref: "per-leg BEFORE (run 30423687319 on da266bb) and AFTER (run 30511183170 on 80fb90a): macOS 72/2,616,112 -> 81/2,618,053; Ubuntu 69/2,591,530 -> 78/2,593,471; Windows 69/2,591,530 -> 78/2,593,471. Delta is +9 cases / +1,941 assertions on EVERY leg identically. The macOS-vs-others gap is unchanged at exactly 3 cases / 24,582 assertions before AND after, and the three macOS-gated drift-ON goldens measure locally at exactly 3 cases / 24,582 assertions"
        status: pass
    human_judgment: false
  - id: D13
    description: "T-31-30 / D-24: the deferred register names the evidence, the decision, the owner and the consequence for each item, and refuses to mark the half-closed item closed"
    verification:
      - kind: integration
        ref: "deferred-items.md exists with 13 numbered items; 'guardrail event' x3; 'unfixed by decision' x1; 'Resolve at: NO PHASE' x1; AnalogLFO.cpp:320, LfoCore.hpp:183-184 and RackCompat.hpp:106 all named (both LFO sites read directly this session); the sanitizer-gate consequence stated; item 2 marked 'HALF closed' with both halves named and the local-to-VcoCore constraint carried over; item 4 recorded RESOLVED with a cross-reference, and the phase-30 register still returns 2 for grep -c RESOLVED"
        status: pass
    human_judgment: false
  - id: D14
    description: "D-18: this phase's own measured figures are collected in one place and neither contradictory research figure is presented as the expected magnitude"
    verification:
      - kind: integration
        ref: "the measured-figures roll-up below covers every row of 31-VALIDATION.md's verification map. 31-05 verified grep -cE '0\\.0048|0\\.1 cent|1e-4|1e-6' == 0 across the gate TU; no delivered code or test cites either research figure"
        status: pass
    human_judgment: false

# Metrics
duration: 12min
completed: 2026-07-30
status: complete
---

# Phase 31 Plan 08: The Phase Gate — TEST-02 Enforced, the Whole Diff Swept, the Matrix Observed by Hash Summary

**Phase 31 closes on evidence. TEST-02 is a HARD gate rather than a closing nicety: all six selectors are green AND each is proven to have matched its exact expected case count, with a negative control demonstrating on this very binary that a selector matching nothing also exits 0 and prints `Status: SUCCESS!`. The four local gates are green together on one tree with a REAL 169,072-byte plugin link, recorded as a PRECONDITION rather than as the gate. Every prohibition holds across the whole `da266bb..80fb90a` diff — and four of the stated criteria turned out to be filter artifacts, one of which can never return zero in this repository, each re-stated in an honest form and reported. The three-OS matrix and the Windows link leg are green on this phase's exact commit, located by hash equality with the pre-phase commit's own green run sitting one line above it in the listing. And the matrix's case-count gap is accounted for by measuring both sides: +9 cases and +1,941 assertions on every single leg, identically.**

## Performance

- **Duration:** 12 min
- **Started:** 2026-07-30T03:22Z
- **Completed:** 2026-07-30T03:34Z
- **Tasks:** 3 (Tasks 1 and 2 modify no repository file by design, so neither carries a commit)
- **Files created:** 1 (`deferred-items.md`, 402 lines)
- **Files modified:** 0

## Task Commits

1. **Task 1: the full local gate, TEST-02 enforced as a hard gate, every prohibition swept over the whole phase diff** — *no commit; verification-only by design. Its evidence is this summary.*
2. **Task 2: the three-OS matrix and the Windows link leg observed green by commit hash** — *no commit; verification-only by design. Its evidence is the CI observation record below.*
3. **Task 3: the phase's deferred register, including the shipped module's identical latent UB (D-24)** — `d294a8e` (docs)

---

## Gate 1 — TEST-02 as a HARD gate

### The six selectors, with their MATCHED CASE COUNTS

**The matched count is the assertion. The exit status is not.**

| Invocation | Expected cases | **Matched cases** | Assertions | Exit |
|---|---|---|---|---|
| `-tc="*v/oct tracking*"` | 2 (primary tier + secondary telemetry tier) | **2** | 603 | 0 |
| `-tc="*COARSE*"` | 1 | **1** | 170 | 0 |
| `-tc="*FINE*"` | 1 | **1** | 156 | 0 |
| `-tc="*exponential FM*"` | 2 (the invariant + its permanent negative control) | **2** | 494 | 0 |
| `-tc="*Nyquist*"` | 1 | **1** | 93 | 0 |
| `-tc="*hostile pitch*"` | 1 | **1** | 395 | 0 |
| | | **8 cases** | **1,911** | |

**Every count equals its expectation exactly.** Not one is zero, and not one differs — so neither of the two findings a mismatch would have signalled (a drifted case name, or a missing case) is present.

### The negative control that makes the matched count mean something

Run on the same binary, in the same session:

```
$ ./build-test/test -tc='*this case name does not exist anywhere*'
[doctest] test cases: 0 | 0 passed | 0 failed | 81 skipped
[doctest] assertions: 0 | 0 passed | 0 failed |
[doctest] Status: SUCCESS!
exit=0
```

**A selector that matches nothing reports success and exits zero.** That is not an argument about doctest's semantics — it is the measured behavior of this binary, and it is exactly the false green T-31-20 names. "The gate passed" and "the gate never ran" are indistinguishable by exit status alone, and the table above is what distinguishes them.

### The whole-suite count, reconciled twice over

| Run | Cases | Assertions | Failed |
|---|---|---|---|
| Recorded pre-phase baseline (`31-VALIDATION.md` regression floor) | 72 | 2,616,112 | 0 |
| **This tree (`80fb90a`), clean rebuild** | **81** | **2,618,053** | **0** |
| Expected (baseline + 9) | **81** | — | — |

**Computed rather than trusted, and reconciled case by case.** `grep -c '^TEST_CASE(' tests/test_vco_pitch.cpp` returns **9**, and all nine are enumerated by name — the apparatus self-check, the two TEST-02 tiers, COARSE, FINE, the FM identity, the FM negative control, the Nyquist clamp, and the hostile-pitch regression check. No case was added to any other translation unit.

**It reconciles on the assertion axis as well, which is the stronger statement.** The six selectors reach **8** of the nine cases, at **1,911** assertions. The ninth — invariant 1, the derived-boundary apparatus self-check — is reached by no selector and measures **30** assertions. `1,911 + 30 = 1,941`, and **+1,941 is exactly the assertion delta measured on all three CI legs** (see Gate 5). The loop closes on all nine cases with nothing unaccounted for.

## Gate 2 — the four local gates, green together on ONE tree

Recorded as a **PRECONDITION, never as the gate.** Phase 29 measured this exact combination returning success on a commit that could not link (`STATE.md` § Accumulated Context, "P-2 CORRECTED and widened"), which is why the real link is here and the off-machine matrix is Gate 5.

| Gate | Command | Result |
|---|---|---|
| **Headless suite** | `make test` (from a removed `build-test/`) | **exit 0 — 81 cases / 2,618,053 assertions / 0 failed**, 14.788 s wall, **zero** compiler warnings under `-Wall -Wextra` |
| **Guard suite** | `make guards` | **exit 0 — `guard suite: PASS`** |
| **C++11 strict gate** | `RACK_DIR=../Rack-SDK make strict` | **exit 0 — `strict C++11 gate: PASS`** over four TUs |
| **Real plugin link** | `RACK_DIR=../Rack-SDK make` (from a removed `build/`) | **exit 0 — `plugin.dylib`, 169,072 bytes** |

**The guard suite, section by section** — every line, because "PASS" alone is what this phase has repeatedly declined to accept:

- **`check_frozen.sh`** — `[1/3]` all **15** pinned manifest entries OK, including every shipped LFO header, `src/AnalogLFO.cpp`, `tests/BlockDriver.hpp` and `tests/test_golden.cpp`, plus the completeness assertion; `[2/3]` all **6** pinned `.f32` golden fixtures OK plus completeness; `[3/3]` the negative control detected a perturbed copy of `MathConst.hpp` and confirmed the real file untouched.
- **`check_includes.sh`** — `[1/7]` 29 LFO-side roots, 29 files opened across the transitive closure, **zero** VCO includes; `[2/7]` `VcoCore.hpp` Rack-free; `[3/7]` siblings and standard headers only; `[4/7]` exactly one `forge::Inputs` (the R-9 ODR guard); `[5/7]` no hasher under `src/`; `[6/7]` **all five** negative controls fired, including **`nc5`, the WR-05 comment-evasion probe plan 31-01 added**; `[7/7]` guard wiring, with `check_docs.sh` reported `EXEMPT` as designed.
- **`check_canary.sh`** — `[1/5]` probe declared and defined; `[2/5]` defined external-linkage symbol emitted at `-O3`; **`[2b/5]` "all 8 VcoInputs DSP field(s) stay runtime-live through step() at -O3"**; `[3/5]` clean under the toolchain's standard; `[4/5]` **all four** C++17-isms rejected for the expected reason; `[5/5]` and `[5b/5]` growth and naming rules OK.

**The link is a real link, not a syntax check.** `-fsyntax-only` never links, so no syntax-only gate on any platform can see a link-class defect — the measured Phase-29 lesson. The produced dylib exports both models:

```
$ nm -gU plugin.dylib | grep -i 'modelAnalog'
0000000000014380 S _modelAnalogLFO
0000000000014388 S _modelAnalogVCO
```

**Guardrail spot checks inside the same tree:** `-tc="*golden*"` → **9 cases / 49,188 assertions**; `-tc="*guardrail*"` → **10 cases / 60 assertions**. The six shipped LFO `.f32` goldens replay byte-identical inside every `make test` above.

## Gate 3 — the prohibitions, swept over the WHOLE phase diff

Diff range: **`da266bb..80fb90a`** — `da266bb` is the commit immediately preceding this phase's first commit (`1fcc9ee docs(31): capture phase context`), and is also exactly where `origin/main` stood before this plan pushed.

| # | Prohibition | Mechanism | Result |
|---|---|---|---|
| 1 | No frozen shared header, no frozen manifest, no shipped-module source | `git diff --name-only da266bb..HEAD \| grep -cE 'AnalogLFO\.cpp\|RackCompat\.hpp\|Waveshape\.hpp\|LfoCore\.hpp\|MathConst\.hpp\|DriftEngine\.hpp\|FROZEN\.sha256'` | **0** |
| 2 | Only two `src/` paths in the whole phase | the same diff, filtered to `^src/` | **`src/AnalogVCO.cpp`, `src/dsp/VcoCore.hpp`** — nothing else |
| 3 | Exactly ONE exponential in the pitch chain | `grep -v '^[[:space:]]*//' src/dsp/VcoCore.hpp \| grep -c 'exp2_taylor5('` | **1** — `float freq = kVcoFreqC4 * exp2_taylor5(pitchVolts);` |
| 4 | No **new** libm exponential or power function under `src/` | see artifact 1 below | **0 in both touched files; tree-wide raw counts unchanged from baseline** |
| 5 | No **new** inline constant-variable or in-class constant declaration | see artifacts 2 and 3 below | **0 non-comment `inline constexpr` tree-wide; 0 of either in both touched files; raw counts unchanged from baseline** |
| 6 | No sanitizer flag in the build file or any workflow (D-24) | `grep -rc 'fsanitize' Makefile .github/`, widened to `fsanitize\|ubsan\|asan` case-insensitively | **`Makefile:0`, `.github/workflows/test.yml:0`** — and `.github/` holds exactly one file, so the scan is complete |
| 7 | The hostile **timing** grids unchanged (D-15) | phase-diff grep **0**; occurrence count 4 → 4; and both arrays extracted from `da266bb` and diffed | **BYTE-IDENTICAL** |
| 8 | Both historical measurement literals intact | `grep -c '21609'` in the core header and the core test TU | **1 / 1**, matching baseline **1 / 1**, both lines byte-identical as text |
| 9 | The phase diff's file list is exactly the declared set | the diff minus `.planning/` | **exactly the six declared** (below) |
| 10 | No deletions anywhere in the phase | `git diff --diff-filter=D --name-only da266bb..HEAD` | **NONE** |
| 11 | Clean tree | `git status --porcelain --untracked-files=all` | **empty** |

**The phase's non-`.planning` file list, in full:**

```
res/AnalogVCO.svg
src/AnalogVCO.cpp
src/dsp/VcoCore.hpp
tests/check_includes.sh
tests/test_vco_core.cpp
tests/test_vco_pitch.cpp
```

Exactly the six the plans declared. **Nothing extra**, so there is no finding to report here.

### FOUR criterion-filter artifacts, reported rather than reconciled silently

This is the fifth time in this phase that a bare count has produced an artifact, and the plan warned in advance that it would. Each is reported with its honest form, and **in every case the substantive claim is true.**

**Artifact 1 — `grep -rv '^[[:space:]]*//' src/ | grep -c 'std::exp2\|std::pow'` returns `1`, not `0`, and it can NEVER return `0` in this repository.**

The one non-comment hit is **`src/dsp/Anim.hpp:40`** — `return intensity * std::pow(0.92f, dt * 60.f);`. That is a **frozen, byte-pinned, shipped-LFO display-animation helper**: it appears in `src/dsp/FROZEN.sha256`, `check_frozen.sh` verifies it OK on every run, and it is **absent from this phase's diff** (0 entries). It was verified present with identical text at `da266bb`. `std::pow` legitimately lives under `src/`, so the criterion as literally written is unsatisfiable — the prohibition's real subject is the VCO pitch chain and the code this phase wrote. (A second raw hit, `src/dsp/VcoCore.hpp:293`, is a whole-line comment that names `std::exp2/std::pow` **in order to forbid them** — the artifact class 31-02 and 31-03 each documented for this file.) **Honest form, and it holds:** non-comment `std::exp2` and `std::pow` are both **0** in `src/dsp/VcoCore.hpp` and `src/AnalogVCO.cpp`, and the tree-wide raw counts are **identical** at `da266bb` and now (`std::exp2` 1 → 1, `std::pow` 2 → 2).

**Artifact 2 — `grep -rc 'inline constexpr' src/` does not return `0` for every file.**

Two files are non-zero: `src/dsp/VcoCore.hpp:60` and `:95`, and `src/dsp/MathConst.hpp:12`. **All three are comment lines that name the banned construct in order to forbid it** — e.g. `// - No \`inline constexpr\` variables (C++17 inline variables)`. **Honest form:** non-comment `inline constexpr` across all of `src/` is **0**, both touched files are **0**, and the tree-wide raw count is unchanged from baseline (3 → 3). Corroborated twice independently: `make strict` runs `-std=c++11 -pedantic-errors`, which rejects a real `inline constexpr` variable, and `check_canary.sh [4/5]`'s negative control confirms that a namespace-scope `inline constexpr` variable **is** rejected for the expected reason.

**Artifact 3 — `grep -rv '^[[:space:]]*//' src/ | grep -c 'static constexpr'` returns `14`, not `0`, and the criterion's mechanism is wider than its prose.**

The prose forbids an **in-class** constant declaration. The mechanism also catches **namespace-scope `static constexpr`, which `src/dsp/VcoCore.hpp:69` explicitly PERMITS** as the required form for a constant table. Of the 14, none is new and none is in a file this phase edited: the real in-class members are in the **shipped** `src/AnalogLFO.cpp` (`RATIO_TABLE`, `RATIO_LABELS`, `SWING_FRACTIONS`, …) with the C++11-legal out-of-line definitions at `:384`, plus frozen `DriftEngine.hpp:43`, `ClockTracker.hpp:39` and `RackCompat.hpp:40-41`; the rest are the permitted namespace-scope form in `Swing.hpp` and `RatioTable.hpp`. **Honest form:** both touched files return **0** non-comment hits for both constructs, and the tree-wide raw count is unchanged from baseline (**20 → 20**).

**Artifact 4 — `grep -c '#if defined(__APPLE__)' tests/test_golden.cpp` returns `2`, while `STATE.md` records "exactly three `#if defined(__APPLE__)` TEST_CASEs".**

Both statements are correct about different things. There are **two** preprocessor directives: the first (lines 112–132) gates a **loader/helper**, not a case; the second (lines 150–162) gates **exactly three `TEST_CASE`s** — the drift-ON goldens at 44.1k / 48k / 96k, named `golden: freerun replay matches reference @ …` (the drift-**off** trio at 137/141/145 sits outside both blocks and is portable). **The substantive figure is three gated cases**, and it is confirmed by measurement rather than by counting directives: `-tc="golden: freerun replay*"` reports **3 cases / 24,582 assertions**, which is exactly the matrix gap on both axes (Gate 5). Recorded because the plan's own read-first pointed at that STATE entry, and a reader who checked the directive count would think a figure had moved.

## Gate 4 — the measured-figures roll-up (D-18)

**Every figure below is this phase's own measurement, read out of an actual run's output.** `31-05` verified that `grep -cE '0\.0048|0\.1 cent|1e-4|1e-6'` returns **0** across the gate translation unit: **neither of the two contradictory prior-milestone research figures for `exp2_taylor5`'s error is cited anywhere in the delivered code or tests.** Both would have cleared the bar, which is exactly why it would have been easy to inherit one instead of producing one.

### V/OCT tracking — the TEST-02 gate itself (PITCH-01 / TEST-02, owner 31-05)

| Tier | Rate | Worst \|cents\| | Sign | At volts | Samples/cycle |
|---|---|---|---|---|---|
| **PRIMARY** (returned samples) | 44100 | **0.00967639** | − | **+5.5 V** | 3.72472 |
| **PRIMARY** | 48000 | **0.00870829** | − | **+6.0 V** | 2.86669 |
| **PRIMARY** | 96000 | **0.00239614** | − | **+7.0 V** | 2.86669 |
| **SECONDARY** (telemetry — the weaker tier) | 44100 | **0.0013924** | + | **+6.20392 V** | 2.28662 |
| **SECONDARY** | 48000 | **0.00123964** | − | **+6.32617 V** | 2.28662 |
| **SECONDARY** | 96000 | **0.00123964** | − | **+7.32617 V** | 2.28662 |

The fixed **0.05-cent** tolerance is **5.17× above** the worst measurement anywhere and **20× under** PITCH-01's one cent — the worst point measured is **103× inside the requirement**. Secondary tier restricted to the lattice alone: **0.000164011** cents at −9.5 V, an order of magnitude smaller, which is what the added derived band point would otherwise have hidden.

### COARSE and FINE (PITCH-02 / PITCH-03, owner 31-06)

| Control | 44.1 kHz | 48 kHz | 96 kHz |
|---|---|---|---|
| **COARSE** worst \|cents\| | **0.004658187** @ +5.0 | **0.0033790525** @ −2.37 | **0.00337679175** @ −2.37 |
| **FINE** worst \|cents\| | **0.00628057135** | **0.006281158** | **0.00633285261** |
| COARSE rows measured / skipped | 18 / **1** | 18 / **1** | 19 / 0 |

**The twelve measured hundred-cent shifts at the FINE range ends**, all inside **0.007 cents** of a hundred:

| Rate | V/OCT | fine 0 → **+1** | fine 0 → **−1** |
|---|---|---|---|
| 44100 | 0.0 | **+100.003243** | **−100.002971** |
| 44100 | +2.0 | **+99.9940002** | **−100.0** (−99.9938641) |
| 48000 | 0.0 | **+100.003244** | **−100.002972** |
| 48000 | +2.0 | **+99.9941645** | **−99.9937555** |
| 96000 | 0.0 | **+100.003237** | **−100.002974** |
| 96000 | +2.0 | **+99.9940819** | **−99.9937695** |

No conversion at all would read **1200**; a divisor of a hundred would read **12**. A hundred separates the correct implementation from both.

### Exponential FM — the identity, its summed volts, and the negative control (FM-01/02/03, owner 31-06)

| row | V/OCT | fmVolts | atten | **summed volts** | 44.1 kHz | 48 kHz | 96 kHz | |
|---|---|---|---|---|---|---|---|---|
| 1 | **0.00** | +0.75 | +1.00 | **+0.750** | **0** | **0** | **0** | ← **BLIND — the obvious test** |
| 2 | +0.25 | +0.75 | +1.00 | **+1.000** | 492 | 491 | 469 | |
| 3 | +1.00 | +2.00 | +1.00 | **+3.000** | **0** | **0** | **0** | ← **BLIND** |
| 4 | −1.50 | +0.50 | +0.50 | **−1.250** | 328 | 342 | 297 | |
| 5 | +2.00 | −3.00 | +1.00 | **−1.000** | **0** | **0** | **0** | ← **BLIND** |
| 6 | +0.50 | +1.30 | +0.37 | **+0.981** | 487 | 483 | 463 | |
| 7 | +3.25 | −1.75 | +1.00 | **+1.500** | 502 | 501 | 488 | |
| 8 | −2.00 | +0.60 | −0.50 | **−2.300** | **0** | **0** | **0** | ← **BLIND on the V/OCT term alone** |

The real core measures **0 mismatches on every row**. The multiplicative stand-in fails the same identity through the same helper on **1809 + 1817 + 1717 = 5343** samples, diverging from **sample 0** on three of the four sighted rows and sample 2 on the fourth. **The four zeroes are the map of what the check cannot see**, and the measured rule is sharper than the plan's model: the identity is blind whenever **either** pitch term is a whole number of volts — and **zero is whole**, so the first test almost anyone would write is vacuous at every FM voltage.

### The Nyquist clamp, observed FIRING and observed still sounding (PITCH-04 / D-10, owner 31-07)

| Rate | Derived ceiling V | Recomputed max Hz | Observed `tel.freqHz` (3 offsets) | Verdict | nUp | Peak V | Below-ceiling control |
|---|---|---|---|---|---|---|---|
| 44100 | **+6.38263152** | 21829.5 | **21829.5** at +0.25, +1.00, +3.00 | **EXACT** | **5457** | **5.000** | 10914.73438 (**−0.00242652678 c**) |
| 48000 | **+6.50488727** | 23760.0 | **23760.0** at all three | **EXACT** | **5939** | **5.000** | 11879.96484 (**−0.00507139295 c**) |
| 96000 | **+7.50488727** | 47520.0 | **47520.0** at all three | **EXACT** | **11879** | **5.000** | 23759.92969 (**−0.00507139295 c**) |

`blockMin = −5` and `blockMax = +5` exactly. **An inequality would have been satisfied by a dead oscillator** — `freqHz <= ceiling` is true at zero — which is why the case asserts an exact equality plus three liveness properties. The strictly-under control (strict `<`, never `<=`) excludes a core that pinned the frequency **always**.

### The hostile grid's three outcomes, and the reachable-envelope arithmetic (D-14 / D-22 / T-31-24, owner 31-07)

26 rows × 3 rates × 4000 steps = **78 configurations**, `firstBadStep = −1` on every one.

| Outcome | Observed `tel.freqHz` | Peak V |
|---|---|---|
| **the CONTROL** (row 1) | **261.6256104** | 5.000000 / 4.999999523 |
| **POSITIVE plateau** (11 rows) | the rate's ceiling **EXACTLY** — 21829.5 / 23760 / 47520 | 4.998722553 / 4.998712063 |
| **NEGATIVE plateau** (14 rows, including all three NaN rows) | **1.418275276e-17** at all three rates | **5.000000** — the LARGEST of the three |

**A NaN lands on the NEGATIVE plateau**, which is the core's negated-first comparison made externally visible and a live tripwire on the one substitution D-14 rejects by name. **The peak at the stalled plateau is the largest of the three** — a stalled oscillator is not a quiet one, which is why a loose magnitude bound is a real check there and a finiteness test alone would not be.

| Reachable-envelope term | Source | Volts |
|---|---|---|
| max cable magnitude on V/OCT | the platform's ±12 V norm | **12** |
| coarse range magnitude | `COARSE_PARAM` declared −5..+5 **oct** | **5** |
| fine range magnitude ÷ 12 | `FINE_PARAM` declared −1..+1 **semitones** | **0.083333333333333329** |
| max cable × a full attenuverter | `FM_ATTEN_PARAM` −1..+1 at 1.0 oct/V | **12** |
| **worst reachable total** | | **29.083333333333332** |
| **the bound** | `forge::kVcoMaxPitchVolts`, symbolic | **64** |
| **margin ratio** | | **2.2005730659025788** |

Built from four **named declared ranges** rather than a typed-in total, so it is *this* assertion that fires if a later phase widens a control or lowers the bound. **PITCH-05's runtime pin:** final accumulator **0.99909167672740296** (44.1 kHz) and **0.99946984462440014** (48 and 96 kHz) after 100,000 maximal-increment steps, `firstBadStep = −1` at all three rates.

### The two sanitizer transcripts (D-22, owner 31-03)

- **RED** — `src/dsp/RackCompat.hpp:106:24` *"nan is outside the range of representable values of type 'int'"* and `:109:11` *"left shift of 2147483647 by 23 places cannot be represented in type 'int32_t'"*. Quoted verbatim in `31-03-SUMMARY.md`; both line numbers re-confirmed against the frozen header this session (`:106` is `int32_t xi = (int32_t)x;`, `:109` is `yii = xi << 23;`).
- **GREEN** — **zero `runtime error:` lines, 0-byte stderr** over an extended 24-configuration grid, same compiler, same flags, only the guard changed. **The process exit status is not the signal** — UBSan recovers by default and the probe exited 0 both times.

### Every row of the validation contract's verification map, and all SIX non-vacuity requirements

| `31-VALIDATION.md` row | Figure(s) above | Owner | Status |
|---|---|---|---|
| PITCH-01 output-derived tracking | primary tier, 6 figures | 31-05 | ✅ |
| PITCH-01 secondary `tel.freqHz` tier | secondary tier, 6 figures | 31-05 | ✅ |
| PITCH-02 COARSE ±5 oct | 3 worst-cents + 18/18/19 coverage | 31-06 | ✅ |
| PITCH-03 FINE ±100 cents | 12 measured hundred-cent shifts | 31-06 | ✅ |
| PITCH-04 clamp fires, keeps sounding | exact-equality table + 3 controls | 31-07 | ✅ |
| PITCH-05 double accumulator | `static_assert` + 3 final accumulator values | 31-07 | ✅ |
| FM-01 audio-rate FM | 1200 cents + per-sample-alternating block | 31-06 | ✅ |
| FM-02 bipolar attenuverter | inversion 0/512 + sign difference > 0 | 31-06 | ✅ |
| FM-03 summation identity | the 8-row grid with summed volts | 31-06 | ✅ |
| FM-03 negative control | **5343** mismatching samples | 31-06 | ✅ |
| D-09 unpatched no-op | 105 blocks, 0/256 each | 31-06 | ✅ |
| D-14 hostile pitch | 78 configurations, 3 outcomes | 31-07 | ✅ |
| **TEST-02 phase gate** | **this plan** | **31-08** | ✅ |
| guardrail: goldens + no frozen header | 9/49,188 + the sweep above | 31-08 | ✅ |

| # | Non-vacuity requirement | Implemented by | The case that implements it |
|---|---|---|---|
| 1 | Measure the OUTPUT, not `tel.freqHz`, on the primary tier | 31-05 | the TEST-02 **PRIMARY TIER** case — `tel.freqHz` appears **once** in the whole file, inside the secondary tier |
| 2 | Ground truth from libm in double | 31-05 | `expectedFreqHz()` = `261.6256 * std::exp2(v)`; zero references to the polynomial under test |
| 3 | Expectations one octave apart | 31-05 | `CHECK(expectedRatio > 1.4)` on every consecutive pair — an accumulator that latched one frequency satisfies at most one of 72 points |
| 4 | `REQUIRE(nUp >= 8)` before any tolerance check | 31-05 (+31-06, +31-07) | 5 occurrences; in the primary tier at line 563 vs the first tolerance read at 567 |
| 5 | FM multiplicative negative control | 31-06 | the **FM-03 NEGATIVE CONTROL** case — `DeliberatelyMultiplicativeFmCore`, **5343** mismatches |
| 6 | Clamp-boundary case proving the clamp FIRES | 31-07 | the **PITCH-04 / D-10** case — exact equality at nine points, thousands of crossings, a full 5.000 V peak, plus a below-ceiling control |

**All six are implemented and observed.** None is argued.

### Requirement-to-evidence map, and a READINESS note

**No requirement is marked complete by this plan** — the phase-completion flow owns that, and all nine were already marked by 31-05 / 31-06 / 31-07. What this gate does is confirm each mark has evidence behind it.

| ID | Discharged by | Marked by |
|---|---|---|
| **PITCH-01** | TEST-02 primary tier on the returned samples, 103× inside its own bar, plus the source assertion of exactly one `exp2_taylor5` and byte-identical goldens | 31-05 |
| **PITCH-02** | the COARSE case — full −5..+5 range at three rates, plus four non-integer values measuring continuity | 31-06 |
| **PITCH-03** | the FINE case — twelve measured hundred-cent shifts, asserted **relatively** so it pins the `/12` rather than echoing it | 31-06 |
| **PITCH-04** | the Nyquist case (exact equality + liveness + a below-ceiling control) and the hostile-pitch case (78 configurations at or below the recomputed ceiling) | 31-07 |
| **PITCH-05** | `static_assert(std::is_same<decltype(forge::VcoCore::phase), double>::value, …)` at compile time, the `deltaPhase` double-cast source assertion, and a 300,000-step runtime range check | 31-07 |
| **FM-01** | one volt at a full attenuverter measured at 1200 cents on the output; a per-sample-alternating modulator keeps the block finite, bounded, non-constant and different from unpatched | 31-06 |
| **FM-02** | inversion bit-exact (0/512), the two signs producing different blocks, zero a bit-exact no-op | 31-06 |
| **FM-03** | the bit-exact identity over all eight rows at three rates, **with the multiplicative alternative observed failing it** | 31-06 |
| **TEST-02** | **this plan** — all six selectors green with their matched counts proven, on a tree whose four local gates and three-OS matrix are green together | 31-05 |

**Two forward clauses are re-confirmations owed by later phases, not gaps in what is asserted today**, and both are in the deferred register: **PITCH-04's `sync` clause** (register item 11 → Phase 33) and **PITCH-05's *"for band-limiting"* purpose clause** (Phase 32 owns whether it uses the accumulator well).

## Gate 5 — the off-machine matrix, observed BY HASH

### The observation record

| Field | Value |
|---|---|
| **Pushed hash** | `80fb90a81b442731c5d06d11970a65b148caaf1f` |
| Push | `da266bb..80fb90a  main -> main`, working tree **clean** before it, all **36** phase commits present |
| **Located run's head hash** | `80fb90a81b442731c5d06d11970a65b148caaf1f` |
| **IDENTICAL?** | **YES** — printed side by side before anything else was read, and **re-confirmed after completion** |
| How located | **`select(.headSha == <pushed hash>)`** over `gh run list --json databaseId,headSha` |
| Run | **30511183170**, workflow `test`, event `push`, branch `main` |
| Run conclusion | **success** |

**Located by hash equality — never by name, never by branch, never by recency.** Two prior recorded failures made this rule: a plan that matched a run *name* against `toolchain` got an empty result, because `toolchain-gate` is a **job** inside a workflow named `test`; and Phase 29 measured the whole local gate green on code that could not link.

**Why recency would have been wrong here, concretely.** The second-most-recent run in the listing was **30423687319** on **`da266bba55dbcb3e96b6feae55c8407211cb6d2b`** — `completed/success`. That is a green run, adjacent in the output, and it is the run for the **pre-phase** commit. It says nothing whatsoever about this phase. Hash equality is the only check that separates the two.

### The job conclusion — recorded, and NOT sufficient

| Job | Conclusion |
|---|---|
| **toolchain-gate** | **success** |
| test (ubuntu-latest) | success |
| test (macos-latest) | success |
| test (windows-latest) | success |

**A job conclusion is not sufficient on its own, and this is stated as a limitation rather than a formality:** a step that fail-fasts upstream is reported **`skipped`**, which scans as *not red* in a job summary. So the leg's own step conclusion was read separately.

### The Windows link leg — its OWN step conclusion

**Step name, verbatim:** `win-x64 leg reproduction (compile + full link vs libRack)` — step **6** of `toolchain-gate`.

**Its OWN conclusion: `success`.** Its own log line: `win-x64 link gate: PASS`.

All twelve `toolchain-gate` steps report **success** individually, including `Strict C++11 pedantic gate` (4), `VCO compile canary guard` (7), `Frozen-header hash guard` (8), `Include / dependency-direction audit` (9) and `LFO non-regression guard suite via make` (10, which printed `guard suite: PASS` off-machine as well).

**The only `skipped` steps in the whole run are the two intended OS conditionals** in the matrix job — `make test (unix)` skipped on Windows, `make test (windows / direct g++)` skipped on Ubuntu and macOS. Each leg ran exactly one of the pair and it concluded `success`. Those skips are `if:` conditions by design, not fail-fast artifacts, which is exactly the distinction the step-level read exists to make.

### The matrix case-count gap — ACCOUNTED FOR by measuring both sides

Rather than cite `STATE.md`'s recorded figures, the pre-phase commit's **own CI run** was read, so the accounting is observed on both sides of the change:

| Leg | **BEFORE** (`da266bb`, run 30423687319) | **AFTER** (`80fb90a`, run 30511183170) | Δ cases | Δ assertions |
|---|---|---|---|---|
| **macOS** | 72 / 2,616,112 | **81 / 2,618,053** | **+9** | **+1,941** |
| **Ubuntu** | 69 / 2,591,530 | **78 / 2,593,471** | **+9** | **+1,941** |
| **Windows** | 69 / 2,591,530 | **78 / 2,593,471** | **+9** | **+1,941** |

**`+9` cases and `+1,941` assertions on EVERY leg, identically.** That is the affirmative confirmation the plan asked for: **all nine of this phase's new cases ran on all three legs**, measured as a per-leg delta rather than inferred from the absence of a platform gate. (The inference is available too and agrees — `grep -cE '#if|__APPLE__|_WIN32|__linux' tests/test_vco_pitch.cpp` returns **0** — but a delta is evidence and an absent `#if` is only an argument.)

**And the gap itself is unchanged, on both axes:**

| | cases | assertions |
|---|---|---|
| macOS − others, **BEFORE** | **3** | **24,582** |
| macOS − others, **AFTER** | **3** | **24,582** |
| the three macOS-gated drift-ON goldens, **measured locally** | **3** | **24,582** |

Exact on both axes and both sides. The gap is the three `#if defined(__APPLE__)`-gated drift-ON bit-exact golden cases in `tests/test_golden.cpp` (the Phase-26 decision: `std::normal_distribution` is not portable across standard libraries), and nothing else was dropped anywhere. **The run is not red, so no failing job, step or log excerpt is captured.**

## Task 3 — the deferred register

`.planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md`, **402 lines, 13 numbered items**, in the previous phase's format.

| # | Item | Owner |
|---|---|---|
| **1** | **The shipped LFO's identical latent UB (D-24)** | **NO PHASE — guardrail event, unfixed by decision** |
| 2 | `forge::clamp` NaN-transparency — **HALF closed** | the phase that adds MORPH/CHARACTER CV (Phase 34) |
| 3 | `IN-05` hostile **timing** grid extension | Phase 32 |
| 4 | `check_includes.sh [2/7]` unanchored exemption (WR-05) | **RESOLVED by 31-01** |
| 5 | Per-instance seed entropy + patch persistence | Phase 34/35 |
| 6 | COARSE octave/semitone snap | Phase 35 or v2.1 |
| 7 | Amplitude fade near the ceiling — considered and rejected | Phase 34, if wanted |
| 8 | Param display precision — a deliberate recorded divergence | nobody, unless exactness matters |
| 9 | The canary's field-count margin narrowed to ONE field | the phase that wires `drift` (Phase 34) |
| 10 | `coreBase()`'s two falsified annotations | the next phase editing `tests/test_vco_core.cpp` |
| 11 | PITCH-04's `sync` clause must be **re-confirmed**, not inherited | Phase 33 |
| 12 | `plugin.json` still `2.0.1` with two modules (WR-04) | Phase 36 |
| 13 | `tests/check_docs.sh` not wired into CI | Phase 36 |

### Item 1 in particular — the substantive one

**Both shipped call sites were read directly this session, not paraphrased from a research document:**

- **`src/AnalogLFO.cpp:320`** — `in.fmCV = in.fmConnected ? inputs[FM_INPUT].getVoltage() : 0.f;`. A raw cable voltage, no finiteness check, and Rack does not sanitize cable voltages.
- **`src/dsp/LfoCore.hpp:183-184`** — `float depthScale = isClocked ? 0.5f : 0.6f;` then `float fmPitch = in.fmCV * in.fmAtten * depthScale;`, handed **unbounded** to `exp2_taylor5` on the very next line, **`:185`**.
- Which reaches **`src/dsp/RackCompat.hpp:106`** (`int32_t xi = (int32_t)x;`) and **`:109`** (`yii = xi << 23;`) — the same two frozen lines this phase guarded the VCO against.
- **Two details worth having in the record.** Scaling by `0.5f`/`0.6f` does not rescue anything — a NaN stays a NaN and an infinity stays an infinity. And `src/dsp/LfoCore.hpp:186`'s `freq = std::fmax(freq, 0.001f)` sanitises the **result**, exactly as the VCO's negated frequency floor did, **which is precisely why a behavioral case cannot see this defect** — the same measured vacuity that forced this phase to escalate its own evidence tier.

Recorded as **MEASURED** (31-03's sanitizer transcript is the evidence of record), a **GUARDRAIL EVENT** requiring operator sign-off and golden re-verification, **UNFIXED BY DECISION**, pointed at **NO PHASE** — and with the consequence stated: **a permanent repository-wide sanitizer gate cannot be adopted while this stands.** The constraint now lives in three places (this register, `VcoCore.hpp`'s `kVcoMaxPitchVolts` rationale, and `tests/test_vco_pitch.cpp`'s invariant-9 banner), so it survives any one of them being missed.

### Item 2 — why HALF closed rather than resolved

The inherited item's binding constraint was *"a NaN-safe helper **local to `VcoCore`**"* and *"editing that shared primitive is a guardrail event, not a VCO fix."* **The pitch-volt half honours both**: the bound is local, uses the negated comparison with the negated line **first**, rejects `forge::clamp` by name in the source, is bit-identical to nothing it replaced, and is **externally observable** — invariant 9 rows 2, 13 and 24 drive a NaN down three routes and all three land on the negative plateau. **The morph and character clamps still call the NaN-transparent helper and are still inert**, and they stay pointed at the phase that adds their CV inputs, with the same constraint attached. Their inputs are param values, which Rack sanitizes before they are read, so they remain unreachable by a cable until then.

The phase-30 register still returns **2** for `grep -c 'RESOLVED'` — nothing there was weakened, and item 4's resolution is a **cross-reference** to the block 31-01 appended, not a duplicate.

## Deviations from Plan

### Auto-fixed Issues

**None.** No repository file needed a fix. All three tasks landed as specified, every gate was green on the first run, and every prohibition held. Second consecutive plan in this phase with no Rule 1/2/3 fix.

### Deliberate non-actions

**1. `coreBase()`'s two falsified annotations were RECORDED, not fixed.**

`tests/test_vco_core.cpp:106-107` still carry `// Phase 31 — unread by this step() body` and `// Phase 31 — unread`, both made **false** by 31-03. This plan's prohibitions forbid editing anything under `tests/`, so fixing them would have violated a stated prohibition to correct a comment nothing asserts against — the same trade 31-07 declined for the same reason. Filed as **register item 10** with the owner named, the two-line shape of the fix stated, and a bold warning that **line 110's `// Phase 34 — unread` for `drift` is still TRUE** and must not be swept up as a set. Independently confirmed by `check_canary.sh [2b/5]`, which reports all eight DSP fields runtime-live at `-O3`, and by reading the shell: it feeds seven of the eight, `drift` being the sole gap.

**2. No requirement marked complete.** The plan forbids it and the phase-completion flow owns it. All nine were already marked by 31-05 / 31-06 / 31-07; this gate re-verifies their evidence and records readiness above.

**3. No inherited item resolved that this phase did not resolve.** Item 2 is HALF closed and says so. Items 3, 5, 12 and 13 are restated pointed at their own phases, unchanged.

### Prescribed but worth recording

**4. Tasks 1 and 2 carry no commit.** Both are verification-only by the plan's own `<files>` block. `git commit` on an unchanged tree fails rather than producing an empty record, and the plan's artifact for both tasks is this summary. Same shape as 31-03's Task 1.

### Verification-command notes (no code impact)

Four criterion-filter artifacts, each with its honest form, are documented in full in **Gate 3** above: `std::pow` under `src/` (**cannot** return 0 — a frozen shipped header legitimately contains it), `inline constexpr` per file (comment lines that forbid the construct), `static constexpr` (a mechanism wider than its prose, catching the explicitly-permitted namespace-scope form), and the `#if defined(__APPLE__)` directive count (2 directives gating 3 cases). **In all four the substantive claim is true**, and in all four the check was re-stated as a **baseline comparison against `da266bb`** or scoped to the files this phase touched.

---

**Total deviations:** 0 auto-fixed, 3 deliberate non-actions, 1 prescribed-but-recorded, 4 criterion artifacts documented.
**Impact on plan:** None. No gate was weakened to pass, no criterion was quietly reinterpreted, and no finding was dropped.

## Issues Encountered

- **None blocking.** The phase gate is green on every axis.
- **The `std::pow` criterion is the first in this phase that is unsatisfiable as written**, rather than merely noisy. `src/dsp/Anim.hpp:40` is a frozen, golden-adjacent, shipped-LFO file containing `std::pow`, so "zero libm under `src/`" can never hold in this repository and any future plan restating that criterion should scope it to the VCO seam or to a baseline comparison. Worth flagging louder than the other three because a future executor could reasonably try to "fix" it and would be reaching for a frozen header to do so.
- **`gh run view --log` reports `UNKNOWN STEP` for the older run** (30423687319) while naming steps correctly for the fresh one. Log-attribution metadata appears to age out. It did not matter here — the doctest summary lines carry their job name, which is all the per-platform accounting needs — but a future plan that keys on step names in an *old* run's log should expect this.
- **Note for later plans (seventh confirmation):** `gsd-tools query state.record-metric` / `state.add-decision` take **named flags**, not the positional arguments the `execute-plan.md` workflow shows. Carried forward from 31-01 through 31-07.

## User Setup Required

None. **Zero registry packages across the entire phase** (T-31-SC) — `31-RESEARCH.md` § Package Legitimacy Audit records an empty table, and this gate verified the `Makefile` and the single workflow file are absent from the phase diff, so no dependency was added anywhere.

## Next Phase Readiness

- **Ready for 31-09** (the phase's remaining plan). What is now on record for it: the four local gates with their numbers, the six matched selector counts, the whole-diff prohibition sweep, the CI observation by SHA including the Windows leg's own step conclusion, and the deferred register. **What 31-09 still owns is the operator's in-Rack UAT**, and it must flush the **whole** extracted `dist/` plugin directory, not just `plugin.dylib` and `res/` — a partial flush leaves a stale plugin **version** and Rack keeps reading the old manifest. That failure was measured at the 30-07 gate.
- **The phase's requirement ledger is closed with evidence.** All nine of Phase 31's IDs are marked complete and all six of the validation contract's non-vacuity requirements are implemented and observed. Two forward clauses are re-confirmations owed by Phases 32 and 33, recorded as register items rather than claimed here.
- **Phase 32 (band-limiting)** inherits: the compile-time accumulator pin as its own tripwire (narrowing `phase` to a float fails to **compile**); the clamp's measured behavior at the ceiling with numbers to diff against; register item 3 (the hostile **timing** grid, whose oversampled inner loop makes Phase 32 the first real source of exotic timing); register item 10 (fold the two-line comment fix into whatever commit touches `tests/test_vco_core.cpp`); and the frozen-exponential blindness rule — **any** bit-exact identity across two paths through `exp2_taylor5` is blind wherever one argument is a whole number of volts.
- **Phase 33 (hard sync)** inherits **register item 11 as an obligation, not a suggestion**: PITCH-04's requirement text names `sync`, and Phase 33 must add its sync inputs to invariant 9's hostile grid (or an equivalent case) to re-confirm the clamp binds, rather than inheriting this phase's green for an input class that was unreachable when it was written.
- **Phase 34** inherits register items 2 (the open half of the clamp helper — **local to `VcoCore`**, never an edit to the shared primitive), 5 (seed entropy, with the must-re-validate-on-deserialize requirement), 7 (the amplitude-fade option, only if the flattened peak proves harsh) and **9 — the canary's margin is now a single field, and the phase that wires `drift` must keep the canary feeding EVERY field a runtime value** or `[2b/5]` stops proving anything the shell does not.
- **Phase 36** inherits register items 12 and 13, and **must re-observe the CI link leg on whatever commit IT tags** — the standing no-tag-on-local-evidence rule. Run **30511183170** on **`80fb90a`** is this phase's record, not a substitute for that one.
- **Nobody inherits register item 1, deliberately.** Whoever opens it is opening a guardrail event and must open it as one. **Until then: do not add a sanitizer target to the `Makefile` or a sanitizer step to `.github/workflows/` — it would turn the shipped module red.**
- **No blockers.**

## Self-Check: PASSED

- `.planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md` — FOUND (402 lines, 13 numbered items)
- `.planning/phases/31-pitch-tuning-exponential-fm/31-08-SUMMARY.md` — FOUND
- Commit `d294a8e` — FOUND
- No file deletions in the task commit (`git diff --diff-filter=D --name-only HEAD~1 HEAD` empty)
- No untracked residue after the commit (`git status --porcelain --untracked-files=all` empty)
- No `src/`, `res/`, `tests/`, `Makefile` or workflow file in this plan's diff — the register is the only file it created
- No frozen header, no `FROZEN.sha256` and no `src/AnalogLFO.cpp` anywhere in the whole `da266bb..80fb90a` phase diff
- `make test` (81 / 2,618,053 / 0) and `make guards` (PASS) re-run **after** writing the register — unchanged, as a docs-only task requires
- Both shipped-module call sites (`src/AnalogLFO.cpp:320`, `src/dsp/LfoCore.hpp:183-186`) and both frozen UB sites (`src/dsp/RackCompat.hpp:106`, `:109`) were **read this session** before being named — read-only, neither file edited
- The located CI run's head hash was compared to the pushed hash **before** any conclusion was read, and again after completion
- Every figure in this summary was read out of an actual run's output — the four gate transcripts, the six matched counts, the per-platform CI counts on both commits, the prohibition greps and their baselines — not computed by hand and not copied forward without re-observation where re-observation was possible

---
*Phase: 31-pitch-tuning-exponential-fm*
*Completed: 2026-07-30*
