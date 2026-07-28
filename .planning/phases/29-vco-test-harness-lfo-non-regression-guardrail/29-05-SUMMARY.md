---
phase: 29-vco-test-harness-lfo-non-regression-guardrail
plan: 05
subsystem: validation-ci
tags: [negative-control, odr, mingw, link-gate, ci-observation, validation-signoff, p-2, phase-gate, guardrail]

# Dependency graph
requires:
  - phase: 29-01
    provides: "src/dsp/VcoCore.hpp — the seam that received the temporary ODR probe; tests/VcoBlockDriver.hpp + tests/test_vco_harness.cpp — the 7 harness cases this plan recorded green"
  - phase: 29-02
    provides: "tests/test_lfo_guardrail.cpp (7 cases) + tests/Sha256.hpp + tests/golden/SHA256SUMS — the D-04 byte lock recorded green"
  - phase: 29-03
    provides: "src/vco_compile_canary.cpp — the translation unit that ODR-used the probe, without which the link gate would have had nothing to resolve; tests/check_canary.sh"
  - phase: 29-04
    provides: "src/dsp/FROZEN.sha256, tests/check_frozen.sh, tests/check_includes.sh, `make guards` — the guard suite recorded green and Rack-free"
provides:
  - "Observed evidence that the CI MinGW link gate FAILS on the exact class that got v2.0.0 rejected — run 30339957128, `undefined reference to 'forge::VcoCore::ODR_PROBE_TBL'`"
  - "Observed evidence that both CI jobs are green on `main` — run 30340075121, all four jobs success (ROADMAP criterion 4)"
  - "The corrected P-2 rule: the ENTIRE local gate (make test + make strict + make guards + check_canary.sh) is provably blind to the link-class defect, on the runner as well as on macOS"
  - "The standing phase-close command set that Phases 30-36 repeat: make test, make strict, make guards"
  - ".planning/phases/29-vco-test-harness-lfo-non-regression-guardrail/29-VALIDATION.md — complete, signed off, nyquist_compliant: true"
affects: [30-vco-core-registration, 31-pitch-tuning-fm, 32-morph-blep, 33-hard-sync, 34-drift-engine-additive-edit, 35-vco-shell, 36-goldens-ci-library-update]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "A guard is validated by an observed RED, never by a green run — extended here from local scripts to a CI job"
    - "Negative controls for cross-toolchain defect classes run on a throwaway branch, never on main, with a DO NOT MERGE commit subject and verified local+remote branch deletion"
    - "Where a control cannot be run from the available environment, the exclusion is stated explicitly rather than implied by a green local result"
    - "Weak-by-construction test rows are labelled green-but-weak with the phase that makes them load-bearing named in the row itself"

key-files:
  created:
    - .planning/phases/29-vco-test-harness-lfo-non-regression-guardrail/29-05-SUMMARY.md
  modified:
    - .planning/phases/29-vco-test-harness-lfo-non-regression-guardrail/29-VALIDATION.md

key-decisions:
  - "P-2 is restated in its corrected, broader form: it is not only `make strict` that is blind to the link-class defect — `make test`, `bash tests/check_canary.sh` and `make guards` ALL returned exit 0 on the deliberately broken commit. Every local gate this phase built is incapable of seeing this class."
  - "The strict gate's blindness is structural, not an Apple-clang artifact: `Strict C++11 pedantic gate (our code only)` reported SUCCESS on the broken commit on the Ubuntu runner too. `-fsyntax-only` never links, so no syntax-only gate on any platform can catch a link-class defect."
  - "Operative rule for Phases 30-36 and for any future tag: a fully green local gate is necessary and provably NOT sufficient. No tag or VCV Library resubmission may be cut on local evidence alone — the CI toolchain-gate link leg must be observed green on the exact commit being tagged."
  - "No local ODR reproduction was attempted, per the plan's hard prohibition. Apple clang was measured linking this construct cleanly at -O0 and -O3; a local attempt would have passed and produced a false negative-control result."
  - "The two TEST-01 rows flagged by P-7 (seam determinism, output finiteness) are recorded as green-but-weak rather than green. They pass only because VcoCore::step() is silent by construction (D-01). Phase 30 must re-evidence both when it deletes the TOMBSTONE case."
  - "nyquist_compliant was set to true only after the CI evidence existed, in the same commit that recorded it — never in advance of the observation."

requirements-completed: [TEST-01, TEST-04, TEST-06]

coverage:
  - id: D1
    description: "ROADMAP criterion 1 — one `make test` run replays the shipped LFO goldens byte-identical AND drives the Rack-free VCO harness"
    requirement: "TEST-01, TEST-04"
    verification:
      - kind: unit
        ref: "make test -> exit 0, 64 cases / 64 passed / 0 failed / 2,615,099 assertions"
        status: pass
      - kind: unit
        ref: "./build-test/test -tc=\"golden*\" -> 6 passed, 49,164 assertions (the six shipped LFO golden replays)"
        status: pass
      - kind: unit
        ref: "./build-test/test -tc=\"vco harness*\" -> 7 passed, 34 assertions; -tc=\"lfo guardrail*\" -> 7 passed, 38 assertions"
        status: pass
    human_judgment: false
  - id: D2
    description: "ROADMAP criterion 2 — the harness runs at 44100/48000/96000 Hz with no libRack and non-degenerate seeds"
    requirement: "TEST-01"
    verification:
      - kind: unit
        ref: "case 'vco harness: drives VcoCore over blocks at 44.1 / 48 / 96 kHz Rack-free' passes; sampleTime/sampleRate injection and non-degenerate-seed cases pass"
        status: pass
      - kind: other
        ref: "rm -rf build-test && make test RACK_DIR=/nonexistent-rack-sdk -> exit 0, 64/64 from a cold build; otool -L build-test/test | grep -ci rack -> 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "ROADMAP criterion 3 — the CI MinGW link leg is OBSERVED failing on a real ODR violation, then green after revert"
    requirement: "TEST-06"
    verification:
      - kind: integration
        ref: "CI run 30339957128, job toolchain-gate, step 6 'win-x64 leg reproduction (compile + full link vs libRack)' -> FAILED with `undefined reference to 'forge::VcoCore::ODR_PROBE_TBL'`, collect2: ld returned 1"
        status: pass
      - kind: integration
        ref: "Same run: all per-file MinGW compiles in the `for f in src/*.cpp` loop succeeded; only the final `-o build-ci/plugin.dll` link failed, confirming the defect is link-class and not syntax-class"
        status: pass
      - kind: integration
        ref: "CI run 30340075121 after revert -> toolchain-gate step 6 success"
        status: pass
    human_judgment: false
  - id: D4
    description: "P-2 re-confirmed as a recorded observation and CORRECTED — the whole local gate passes on the broken code, and the strict gate is blind on the runner too"
    requirement: "TEST-06"
    verification:
      - kind: other
        ref: "On broken commit e117cff: make strict -> exit 0 'strict C++11 gate: PASS'; make test -> exit 0 64/64; bash tests/check_canary.sh -> exit 0; make guards -> exit 0 'guard suite: PASS'"
        status: pass
      - kind: integration
        ref: "On failing run 30339957128, toolchain-gate step 4 'Strict C++11 pedantic gate (our code only)' -> SUCCESS on the broken commit. Blindness is structural, not platform-specific."
        status: pass
    human_judgment: false
  - id: D5
    description: "ROADMAP criterion 4 — both CI jobs green on main, establishing the standing canary for Phases 30-36"
    requirement: "TEST-04, TEST-06"
    verification:
      - kind: integration
        ref: "CI run 30340075121 on main -> all four jobs success (test ubuntu/macos/windows + toolchain-gate); toolchain-gate steps 3-9 all success"
        status: pass
      - kind: integration
        ref: "First-ever runner execution of the three guard steps added by 29-03/29-04 (steps 7, 8, 9) -> all success"
        status: pass
      - kind: other
        ref: "git rev-list --left-right --count origin/main...main -> 0 0 (main level with origin at 57b4bca; was 28 ahead at Task 1)"
        status: pass
    human_judgment: false
  - id: D6
    description: "Every shipped LFO source, test and fixture file is byte-identical to tag v2.0.1 at phase close"
    requirement: "TEST-04"
    verification:
      - kind: other
        ref: "git diff --exit-code v2.0.1 -- src/AnalogLFO.cpp src/plugin.cpp src/plugin.hpp plugin.json tests/BlockDriver.hpp tests/test_golden.cpp the six freerun_*.f32 tests/golden/freerun_seeds.txt and all 11 src/dsp LFO headers -> exit 0"
        status: pass
      - kind: other
        ref: "git diff --name-status v2.0.1 -- tests/golden -> exactly one line, `A tests/golden/SHA256SUMS` (a pure addition from 29-02, not a modification of any golden)"
        status: pass
    human_judgment: false
  - id: D7
    description: "No deliberately broken code survives; the working tree is clean and the throwaway branch is gone"
    verification:
      - kind: other
        ref: "grep -rn 'ODR_PROBE_TBL\\|odrProbe' src/ -> no output; per-file counts in VcoCore.hpp and vco_compile_canary.cpp both 0"
        status: pass
      - kind: other
        ref: "git branch --list p29-odr-negative-control -> empty; git ls-remote --heads origin p29-odr-negative-control -> empty; git branch -a --contains e117cff -> no branch"
        status: pass
      - kind: other
        ref: "git status --porcelain -> empty; post-revert local gate make test/make strict/make guards -> all exit 0"
        status: pass
    human_judgment: false
  - id: D8
    description: "The two P-7 invariants are honestly labelled as weak rather than counted as coverage"
    requirement: "TEST-01"
    verification:
      - kind: other
        ref: "29-VALIDATION.md Per-Task Verification Map: 'Seam determinism' and 'Output finite' both carry status green-but-weak with an explicit 'Not full coverage' note naming Phase 30 as the phase that makes them load-bearing"
        status: pass
    human_judgment: true
    rationale: "Whether a passing-but-vacuous assertion should be recorded as coverage is an editorial call, not a mechanical one. It is recorded as NOT coverage, and the reason (VcoCore::step() returns 0 by D-01 design, so determinism compares two all-zero blocks and isfinite(0.f) is trivially true) is written into the row rather than left for a reader to infer."

# Metrics
duration: 16 min
completed: 2026-07-28
status: complete
---

# Phase 29 Plan 05: Full Local Phase Gate & CI-Observed ODR Link-Gate Negative Control Summary

**The MinGW link gate was pushed a real in-class `static constexpr` ODR violation and it bit — `undefined reference to 'forge::VcoCore::ODR_PROBE_TBL'` — while every single local gate this phase built reported PASS on that same broken code, which turns ROADMAP success criterion 3 from a claim into a demonstration and turns pitfall P-2 from an assumption into a measured, wider-than-expected fact.**

## Performance

- **Duration:** 16 min
- **Started:** 2026-07-28T07:41:21Z
- **Completed:** 2026-07-28T07:57:00Z
- **Tasks:** 2 (1 autonomous, 1 operator-run checkpoint)
- **Files:** 1 created, 1 modified — **no source, build or CI file changed**

## The Evidence of Record

This is the section Phases 30 through 36 depend on. Four data points, verbatim.

### 1. The gate went RED

**Failing run:** https://github.com/Photep/ForgeAudio-AnalogSeries/actions/runs/30339957128
Job `toolchain-gate` → **failure**. Jobs `test (ubuntu-latest)`, `test (macos-latest)`, `test (windows-latest)` → all success, as expected: the test target builds at C++17, where an in-class `static constexpr` is implicitly `inline` and links cleanly.

**Verbatim failure from step 6, `win-x64 leg reproduction (compile + full link vs libRack)`:**

```
/usr/bin/x86_64-w64-mingw32-ld: build-ci/vco_compile_canary.cpp.o:vco_compile_canary.cpp:(.rdata$.refptr._ZN5forge7VcoCore13ODR_PROBE_TBLE[.refptr._ZN5forge7VcoCore13ODR_PROBE_TBLE]+0x0): undefined reference to `forge::VcoCore::ODR_PROBE_TBL'
collect2: error: ld returned 1 exit status
```

Two properties of that line matter more than the fact of the failure:

- **The referencing object is `vco_compile_canary.cpp.o`.** The canary did its job. Had 29-03 written a bare-`#include` translation unit instead of one that ODR-*uses* the seam, no relocation would have been emitted, the linker would have had nothing to resolve, and this gate would have been permanently and silently green. That is pitfall P-1, and this line is the proof it was avoided.
- **Every per-file MinGW compile in the `for f in src/*.cpp` loop succeeded.** Only the final `x86_64-w64-mingw32-g++ -o build-ci/plugin.dll` link failed. The defect is genuinely link-class — which is precisely why no compile-only gate can see it.

### 2. The gate went GREEN again after revert

**Post-revert run on `main`:** https://github.com/Photep/ForgeAudio-AnalogSeries/actions/runs/30340075121
All four jobs success. Full `toolchain-gate` step results:

```
3. success  Fetch Rack SDKs (linux headers + windows link stub)
4. success  Strict C++11 pedantic gate (our code only)
5. success  Install MinGW cross-compiler
6. success  win-x64 leg reproduction (compile + full link vs libRack)
7. success  VCO compile canary guard (D-07/D-08)
8. success  Frozen-header hash guard (D-05)
9. success  Include / dependency-direction audit (D-06)
```

Red-then-green on the same gate, with the only difference being the presence of the ODR violation, is what makes this a control rather than a coincidence.

### 3. P-2 confirmed — and it is worse than the plan assumed

The plan predicted `make strict` would report PASS on the broken code. It did. It also under-stated the problem in two directions.

**Direction one: it is the entire local gate, not just `make strict`.** Measured on the deliberately broken commit `e117cff`:

| Local command | Result on code that cannot link for the target toolchain |
|---|---|
| `make strict` | **exit 0** — `strict C++11 gate: PASS`, compile line naming `src/vco_compile_canary.cpp` |
| `make test` | **exit 0** — 64/64 cases, 2,615,099 assertions |
| `bash tests/check_canary.sh` | **exit 0** — PASS |
| `make guards` | **exit 0** — `guard suite: PASS` |

Four for four. Not one gate built in this phase is capable of seeing this defect class.

**Direction two: the strict gate is blind on the runner too.** On the *failing* run, `toolchain-gate` step 4 `Strict C++11 pedantic gate (our code only)` reported **success** on the broken commit. So this is not an Apple-clang quirk that a Linux runner would have caught — `-fsyntax-only` never invokes a linker, so no syntax-only gate on any platform can ever detect a link-class defect. Step 6 was the sole gate in the entire system that caught it.

**The operative rule, now written into `29-VALIDATION.md` for Phases 30-36:** a fully green local gate is **necessary and provably not sufficient**. Green `make test` plus green `make strict` was exactly the state in which v2.0.0 was tagged and rejected from the VCV Library. This phase deliberately reproduced that state and watched it happen. No tag or resubmission may be cut on local evidence alone; the CI `toolchain-gate` link leg must be observed green on the exact commit being tagged.

### 4. Cleanup — nothing broken survived

Verified independently rather than taken on report:

| Check | Result |
|---|---|
| `git status --porcelain` | empty |
| `grep -rn "ODR_PROBE_TBL\|odrProbe" src/` | no output |
| `grep -c 'ODR_PROBE_TBL' src/dsp/VcoCore.hpp` | 0 |
| `grep -c 'odrProbe' src/vco_compile_canary.cpp` | 0 |
| `git branch --list p29-odr-negative-control` | empty |
| `git ls-remote --heads origin p29-odr-negative-control` | empty |
| `git branch -a --contains e117cff` | no branch — the broken commit is reachable from nothing |
| Post-revert `make test` / `make strict` / `make guards` | all **exit 0** |

The broken commit object `e117cff` still exists as a dangling object in the local repository and will be removed by routine garbage collection. It is on no branch, local or remote, so it cannot be checked out by accident or reached by any push.

## Task 1 — The Recorded Local Phase Gate

This is the command set every subsequent phase repeats as its closing canary. All figures observed, not estimated.

| Command | Result |
|---|---|
| `make test` | exit 0 — **64 cases / 64 passed / 0 failed / 2,615,099 assertions** |
| `./build-test/test -tc="vco harness*"` | **7 passed**, 34 assertions |
| `./build-test/test -tc="lfo guardrail*"` | **7 passed**, 38 assertions |
| `./build-test/test -tc="golden*"` | **6 passed**, 49,164 assertions |
| `make strict` | exit 0 — `strict C++11 gate: PASS`; compile line includes `src/vco_compile_canary.cpp` |
| `make guards` | exit 0 — `guard suite: PASS`; **7** lines match a case-insensitive `negative control` (criterion required ≥ 3) |
| `make guards RACK_DIR=/nonexistent-rack-sdk` | exit 0 — the suite is genuinely Rack-free |
| `make clean && make` | exit 0 — full rebuild against `../Rack-SDK`; `nm -gU plugin.dylib` shows `T __ZN5forge21vcoCompileCanaryProbeEi` |
| `rm -rf build-test && make test RACK_DIR=/nonexistent-rack-sdk` | exit 0, 64/64 from cold; `otool -L build-test/test \| grep -ci rack` → **0** |
| **Total gate latency** | **4.3 s** incremental (`make test` 0.47 s + `make strict` 1.61 s + `make guards` 2.24 s) |

A note on why `make` was run as `make clean && make`: a bare `make` reported `Nothing to be done for 'all'` because the tree was already built by plan 29-04. That exits 0 but proves nothing about whether the canary breaks the real build, so the build was torn down and redone from scratch. The canary symbol was then confirmed present in the linked `plugin.dylib` at `-O3`.

### The shipped LFO is byte-identical to `v2.0.1`

`git diff --exit-code v2.0.1 --` over `src/AnalogLFO.cpp`, `src/plugin.cpp`, `src/plugin.hpp`, `plugin.json`, `tests/BlockDriver.hpp`, `tests/test_golden.cpp`, the six `freerun_*.f32` fixtures, `tests/golden/freerun_seeds.txt` and all eleven `src/dsp/` LFO headers → **exit 0**.

## Task Commits

1. **Task 1: full local phase gate + validation sign-off** — `57b4bca` (docs)
2. **Task 2: CI-observed ODR link-gate negative control** — `746137c` (docs)

Neither commit touched a source, test, `Makefile` or workflow file. `git status --porcelain src tests Makefile .github` was empty before and after both.

## Files Created/Modified

- **`.planning/phases/29-vco-test-harness-lfo-non-regression-guardrail/29-VALIDATION.md`** (modified, +154 / −60 across two commits) — Per-Task Verification Map Status column filled from observed output; two P-7 rows labelled green-but-weak with Phase 30 named in each; a new `## CI Observation Status` section recording and then resolving the unpushed-`main` finding; a new `## The P-2 Correction` section carrying the corrected rule; the Negative Controls table extended with two columns (*where the control now lives*, *observed firing?*) and all five rows filled; Manual-Only Verifications marked completed with the cleanup evidence; Wave 0 checklist ticked with the both-manifests note; all seven sign-off items ticked; `wave_0_complete: true`, `nyquist_compliant: true`, `status: complete`.
- **`.planning/phases/29-vco-test-harness-lfo-non-regression-guardrail/29-05-SUMMARY.md`** (created) — this file.

## Deviations from Plan

### Auto-fixed Issues

None. No Rule 1, 2, 3 or 4 condition arose. No source file was modified, no package installed, no architectural question surfaced.

### Reported Findings (recorded rather than worked around)

**1. The broad-form `git diff --exit-code v2.0.1 -- tests/golden` acceptance criterion exits 1, not 0.**

- **Found during:** Task 1
- **What happened:** Task 1's acceptance criteria list `tests/golden` as a whole directory. That form exits **1**, because `git diff --name-status v2.0.1 -- tests/golden` reports exactly one line: `A tests/golden/SHA256SUMS`.
- **Why it is not a failure:** that is plan 29-02's fixture manifest — a pure *addition* alongside the goldens, not a modification of any of them. The enumerated form used in the plan's own action text (the six `.f32` files plus `freerun_seeds.txt`) exits **0**. Plan 29-04's summary predicted this exact reading and flagged it for the operator.
- **Action taken:** recorded both forms with their exit codes rather than quietly substituting the passing one. Nothing was adjusted to make the criterion look satisfied.

**2. No Phase 29 commit had ever reached CI (discovered at Task 1, resolved at Task 2).**

- **Found during:** Task 1
- **What happened:** `main` was **28 commits ahead of `origin/main`** and the newest Actions run on the repository dated from 2026-07-13. The three `toolchain-gate` steps added by plans 29-03 and 29-04 had **never executed on a runner**. They were wired and green locally via `make guards`, which runs the identical scripts — but ROADMAP criterion 4 ("runs on every push and is green") had no evidence behind it.
- **Why it was surfaced rather than absorbed:** a wired-but-never-executed CI step is exactly the failure mode that plan 29-04's `[7/7]` guard-wiring audit exists to prevent, and reporting local green as though it covered the CI leg would have been the false-assurance posture this whole phase was built to eliminate.
- **Resolution:** Task 2's push closed it. `main` is now level with `origin/main` at `57b4bca`, and run `30340075121` records the three guard steps executing on a runner for the first time, all `success`.

**3. On a failing `toolchain-gate` run, the three guard steps are reported `skipped`, not run.**

- **Found during:** Task 2
- **What happened:** on the red run, steps 7, 8 and 9 show `skipped` — the job fail-fasts at step 6.
- **Why it matters:** it is correct fail-fast behavior, not a defect, but it means a future red run must not be read as "the guards passed" or "the guards failed". They did not run at all. Recorded in `29-VALIDATION.md` so nobody misreads a future red run's step list.

**Total deviations:** 0 code deviations, 3 reported findings.

## Honest Scope Statements

Two limits on this plan's evidence, stated plainly rather than left for a reader to discover.

**1. No local ODR reproduction was attempted, and none should be.** The plan prohibited it and the prohibition was honored. Apple clang materialises an in-class `static constexpr` array as a per-translation-unit local symbol and was measured linking this exact construct cleanly at both `-O0` and `-O3`. Any local link check would have reported success and constituted a false negative-control result. The observation in this summary comes entirely from CI, which is the only place it can honestly come from.

**2. Two TEST-01 invariants are green but currently vacuous (P-7).** `vco harness: seam determinism` and `vco harness: output is finite` both pass — but only because `VcoCore::step()` returns `0.f` by construction (D-01). Determinism is comparing two all-zero blocks, and `std::isfinite(0.f)` is trivially true. **These are recorded as green-but-weak, not as coverage.** Phase 30 deletes the `TOMBSTONE` case and gives `step()` real output; both rows must be re-evidenced there, and `29-VALIDATION.md` names Phase 30 in each row so the obligation travels with the artifact.

## LFO Non-Regression Guardrail

This plan is the guardrail's own validation, so its guardrail table is unusually literal:

| Risk | Status |
|---|---|
| Any shipped LFO file modified | **None.** Both commits are documentation-only. `git status --porcelain src tests Makefile .github` empty throughout. |
| R-4 — `TEST_CXXFLAGS` changed | **Untouched.** The Makefile was not edited. |
| R-5 — a CI step weakened or removed | **Untouched.** The workflow was not edited. Nothing was adjusted to make a gate green; the one gate that went red was *supposed* to. |
| R-6 — `make capture` regenerating goldens | **Never run.** |
| Deliberate perturbation left behind | **None.** The probe existed only on `p29-odr-negative-control`, now deleted locally and remotely; `grep -rn` over `src/` returns nothing; the broken commit is on no branch. |
| Shipped LFO vs `v2.0.1` | `git diff --exit-code` over all 21 enumerated paths → **exit 0**. |
| Shipped-LFO behavior | `make test` → 64 cases / 2,615,099 assertions / 0 failed, before and after. All six golden replays byte-identical. |
| The live LFO's CI posture | **Improved, never at risk.** Its three-OS `test` job passed on both the red and the green run — the red job was `toolchain-gate` only, and `main` was never broken. |

## Threat Flags

None. No new network endpoint, auth path, file-access pattern or schema change at a trust boundary. The register's `mitigate` dispositions were all discharged:

| Threat ID | Status |
|---|---|
| T-29-24 (false assurance from the CI link gate) | **Mitigated — this plan's central deliverable.** The gate was observed red on a real ODR violation and green after revert, with the verbatim linker error recorded. It is no longer an unverified claim. |
| T-29-25 (broken code reaching `main` or a release) | **Mitigated.** Probe confined to a throwaway branch with a `DO NOT MERGE` subject; branch deleted locally and on the remote; both touched files grepped clean; tree clean; broken commit reachable from no branch. |
| T-29-26 (a local ODR reproduction reporting a false green) | **Mitigated.** Not attempted. The measured basis is restated in `29-VALIDATION.md` so nobody re-attempts it. |
| T-29-27 (a red `main` during the exercise) | **Mitigated.** The exercise ran entirely on the branch; `main`'s only run is green. |
| T-29-28 (unpinned Rack SDK fetch over HTTPS) | **Accepted as planned.** Pre-existing and unmodified; version-pinned via `RACK_SDK_VERSION`, TLS transport. A digest pin remains a candidate for future hardening. |
| T-29-SC (package supply chain) | **Accepted as planned.** No package installed by this plan. |

## Known Stubs

None. This plan produced no code. `29-VALIDATION.md` is complete, with every row filled and the two weak rows explicitly labelled as weak rather than left looking like coverage.

## Issues Encountered

None blocking. Three findings are recorded above under Reported Findings; all three were surfaced rather than worked around, and all three are now either resolved or documented as standing context for later phases.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Phase 29 is complete and every ROADMAP success criterion is evidenced.** Criterion 1 (one `make test` replays the goldens and drives the VCO harness): observed. Criterion 2 (44.1/48/96 kHz, no libRack, non-degenerate seeds): observed, including a cold build with a nonexistent `RACK_DIR`. Criterion 3 (strict + MinGW link cover the VCO TU, and the link leg is observed failing on a real ODR violation): observed red then green. Criterion 4 (the full canary runs on every push and is green): observed, run `30340075121`.
- **The standing phase-close command set for Phases 30-36 is:** `make test` → `make strict` → `make guards` → push and confirm both CI jobs green. It costs 4.3 s locally. **The push is not optional** — § The P-2 Correction proves the local three cannot see a link-class defect.
- **Phase 30's obligations from this plan:**
  1. Delete the `vco harness: TOMBSTONE - the Phase 29 seam is silent by construction (D-01)` case when `step()` stops returning silence.
  2. **Re-evidence the two P-7 rows** — seam determinism and output finiteness are currently vacuous and only become real coverage once the core emits a waveform. `29-VALIDATION.md` names Phase 30 in both rows.
  3. Registration edits to `plugin.json` / `src/plugin.cpp` / `src/plugin.hpp` need **no** frozen-manifest bump — 29-04 left all three deliberately unpinned for exactly this.
- **Phase 32 obligation:** add `dsp/MorphBlep.hpp` to the canary's include list in the same commit that creates it, or `tests/check_canary.sh` `[5/5]` fails (the D-08 growth rule).
- **Phase 34 obligation:** the sanctioned additive edit to `src/dsp/DriftEngine.hpp` must bump its digest line in `src/dsp/FROZEN.sha256` in the same commit.
- **Phase 36 obligation (the one this phase exists for):** before tagging or resubmitting to the VCV Library, the `toolchain-gate` link leg must be observed green **on the exact commit being tagged**. A green local gate is not a substitute, and this plan has the measurement to prove it.
- **One standing gap carried forward, not introduced here:** `tests/check_docs.sh` remains uninvoked by anything — the single documented `EXEMPT` entry in the `[7/7]` wiring audit, tracked at `.planning/todos/pending/wire-check-docs-into-ci.md`.
- No blockers. No operator decision outstanding.

## Self-Check: PASSED

- **Files claimed created/modified exist on disk:** `29-05-SUMMARY.md` FOUND; `29-VALIDATION.md` FOUND with `nyquist_compliant: true`, `wave_0_complete: true`, `status: complete`, both run IDs present, and zero unticked sign-off items.
- **Commits claimed exist in git:** `57b4bca` FOUND, `746137c` FOUND. Neither introduced a file deletion.
- **Plan `<verification>` block re-run at close:** `make test` exit 0 (64/64, 2,615,099 assertions); `make strict` exit 0; `make guards` exit 0 (`guard suite: PASS`); `make guards RACK_DIR=/nonexistent-rack-sdk` exit 0; `make` exit 0 (clean rebuild); `git status --porcelain` empty; `git branch --list p29-odr-negative-control` empty; enumerated `git diff --exit-code v2.0.1` exit 0; both CI jobs green on run `30340075121`.
- **Both tasks' acceptance criteria re-evaluated:** all pass. The single criterion whose literal form did not exit 0 (broad `tests/golden` diff) is reported above with both forms, both exit codes and the reason, rather than being silently replaced.
- **Working tree clean; no perturbation survives.** Verified by grep over `src/`, per-file counts, local and remote branch listings, and commit reachability.

---
*Phase: 29-vco-test-harness-lfo-non-regression-guardrail*
*Completed: 2026-07-28*
