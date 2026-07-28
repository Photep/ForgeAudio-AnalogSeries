---
phase: 29-vco-test-harness-lfo-non-regression-guardrail
plan: 03
subsystem: build-ci
tags: [compile-canary, odr, cpp11, mingw, negative-control, guard-script, ci, vcv-rack]

# Dependency graph
requires:
  - phase: 29-01
    provides: "src/dsp/VcoCore.hpp — the VCO seam the canary ODR-uses; already C++11-pedantic clean"
  - phase: 27-user-manual
    provides: "tests/check_docs.sh — the guard-script scaffold shape (copied); also the cautionary example of an unwired guard"
provides:
  - "src/vco_compile_canary.cpp — the permanent D-07 translation unit that carries every VCO header into `make strict`, the CI strict step, the CI MinGW compile+link leg, and the local plugin build"
  - "float forge::vcoCompileCanaryProbe(int) — external-linkage ODR probe, proven emitted at -O3 and proven present in the linked plugin.dylib"
  - "tests/check_canary.sh — 5-section standing guard with a validated C++17-ism negative control and a mechanical D-08 growth-rule gate"
  - "CI step `VCO compile canary guard (D-07/D-08)` in the toolchain-gate job"
affects: [29-04-frozen-header-manifest, 29-05-ci-observed-negative-control, 30-vco-core-registration, 32-morph-blep]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "ODR-using canary TU: external-linkage probe + runtime-derived loop trip count, so the TU genuinely emits code instead of being an include-only no-op"
    - "Placement-over-wiring: put the artifact where existing globs already cover it rather than adding wiring that can silently rot"
    - "Guard scripts are wired into CI in the same commit that creates them (P-5)"
    - "Comment-stripped source greps, so a documentation placeholder cannot satisfy a completeness gate"

key-files:
  created:
    - src/vco_compile_canary.cpp
    - tests/check_canary.sh
  modified:
    - .github/workflows/test.yml

key-decisions:
  - "option-a — the canary lives at src/vco_compile_canary.cpp (operator decision, Task 1). Zero Makefile and zero CI wiring edits; cost is one unused namespaced symbol in the released binary."
  - "The probe is forward-declared before it is defined, so no compiler can prove the definition unreachable and discard it"
  - "The loop trip count is derived from the runtime argument ((i & 3) + 1) to defeat constant folding — without it the TU could fold to nothing"
  - "[[maybe_unused]] is an informational, non-failing negative control because GCC may merely warn where Apple clang errors (A3) — making it hard would false-fail on the CI runner"
  - "Section [5/5] strips comment lines before grepping includes, so the commented Phase 32 MorphBlep placeholder cannot produce a false green"
  - "No local ODR reproduction was attempted or claimed — it is impossible on this machine and a task claiming it would be reporting a false result"

requirements-completed: [TEST-06]

coverage:
  - id: D1
    description: "`make strict` and the CI toolchain-gate MinGW compile+link leg both cover a translation unit that ODR-uses src/dsp/VcoCore.hpp (ROADMAP criterion 3)"
    requirement: "TEST-06"
    verification:
      - kind: other
        ref: "make strict -> exit 0, 'strict C++11 gate: PASS', expanded command lists src/vco_compile_canary.cpp"
        status: pass
      - kind: other
        ref: "make -n strict | grep -c 'vco_compile_canary' -> 1 (the $(wildcard src/*.cpp) glob picked it up with zero Makefile edits)"
        status: pass
      - kind: other
        ref: ".github/workflows/test.yml:72 'src/*.cpp' and :80 'for f in src/*.cpp' -> build-ci/*.o link line at :86 — canary covered by both CI legs with zero workflow edits"
        status: pass
    human_judgment: false
  - id: D2
    description: "The canary provably EMITS code — it is not a silently-green include-only no-op (Pitfall P-1)"
    requirement: "TEST-06"
    verification:
      - kind: other
        ref: "nm build-test/canary.o at -O3 -> 'T __ZN5forge21vcoCompileCanaryProbeEi' (DEFINED, not U)"
        status: pass
      - kind: other
        ref: "nm -gU plugin.dylib | grep -c vcoCompileCanaryProbe -> 1 (symbol survives the real -O3 plugin link, confirming the link-leg mechanism)"
        status: pass
      - kind: integration
        ref: "tests/check_canary.sh [2/5] — nm defined-symbol assertion, observed RED against a bare-#include canary"
        status: pass
    human_judgment: false
  - id: D3
    description: "The C++17-ism gate is validated by an observed RED, not a green run — three known-bad synthetic TUs are required to fail compilation"
    requirement: "TEST-06"
    verification:
      - kind: integration
        ref: "tests/check_canary.sh [4/5] — 'inline constexpr' variable rejected"
        status: pass
      - kind: integration
        ref: "tests/check_canary.sh [4/5] — 'if constexpr' rejected"
        status: pass
      - kind: integration
        ref: "tests/check_canary.sh [4/5] — 'std::clamp' rejected"
        status: pass
      - kind: other
        ref: "bash tests/check_canary.sh | grep -c 'rejected' -> 6 (>= 3 required)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The D-08 growth rule is mechanically enforced, and a commented placeholder cannot satisfy it"
    verification:
      - kind: integration
        ref: "tests/check_canary.sh [5/5] — every src/dsp/Vco*.hpp must appear as a live include"
        status: pass
      - kind: other
        ref: "RED probe: temporary src/dsp/VcoExtra.hpp and src/dsp/MorphBlep.hpp stubs both FAILED the gate; MorphBlep failed despite the commented placeholder line existing in the canary"
        status: pass
    human_judgment: false
  - id: D5
    description: "The guard is invoked by CI in the same change that created it (P-5), and the CI edit is append-only (R-5) with TEST_CXXFLAGS untouched (R-4)"
    requirement: "TEST-06"
    verification:
      - kind: other
        ref: "grep -c 'VCO compile canary guard (D-07/D-08)' .github/workflows/test.yml -> 1; grep -c 'tests/check_canary.sh' -> 1"
        status: pass
      - kind: other
        ref: "git diff 97f38a4..HEAD -- .github/workflows/test.yml | grep -c '^-[^-]' -> 0 (zero removed lines); | grep -c 'ffp-contract' -> 0"
        status: pass
      - kind: other
        ref: "ruby YAML parse — toolchain-gate steps 5 -> 6, last step name/run exact, test job unchanged at 3 steps"
        status: pass
    human_judgment: false
  - id: D6
    description: "The ODR/link failure class is NOT claimed to be locally verified — that proof is deferred to the CI-observed negative control in plan 29-05"
    verification: []
    human_judgment: true
    rationale: "Whether the honest scoping is adequate is an editorial judgment. Apple clang links the v2.0.0 failure class cleanly at every optimization level, so no local task could have proven it; check_canary.sh states this exclusion in its banner rather than implying coverage it does not have."

# Metrics
duration: 6 min
completed: 2026-07-28
status: complete
---

# Phase 29 Plan 03: VCO Compile Canary (D-07/D-08) Summary

**A permanent `src/vco_compile_canary.cpp` whose external-linkage, runtime-parameterised probe forces `src/dsp/VcoCore.hpp` through all four C++11/ODR gates with zero build wiring — plus `tests/check_canary.sh`, which proves the canary emits code, proves the C++17-ism gate rejects three known-bad constructs, mechanically enforces the D-08 growth rule, and is invoked by CI in the same commit that created it.**

## Performance

- **Duration:** 6 min
- **Started:** 2026-07-28T07:17:27Z
- **Completed:** 2026-07-28T07:23:45Z
- **Tasks:** 3 (1 decision checkpoint + 2 implementation)
- **Files:** 2 created, 1 modified (+343 lines, 0 deletions)

## Task 1 Decision (operator): `option-a`

**The operator's reply was `option-a`.**

**Resolved canary path: `src/vco_compile_canary.cpp`** — this is the path plan 29-04's `GUARD_SCRIPTS`, `tests/check_includes.sh` and wiring audit must reference.

**Branch behaviour Tasks 2 and 3 followed:** the Option A branch — **zero build-file edits**. No `Makefile` change was made, and the only `.github/workflows/test.yml` change is the Task 3 append of the guard step (the three Option B wiring edits and the sixth `check_canary.sh` wiring-assertion section were **not** applicable and were not written). `tests/check_canary.sh` ships in its 5-section form.

**Rationale as accepted:** zero wiring that can silently rot, and identical by construction to how Phase 30's real `src/AnalogVCO.cpp` will be gated (D-08's stated intent), at the cost of one unused namespaced symbol in the released binary. That cost is disclosed in the canary's file banner so a future reader does not "clean it up".

The four gates that cover the canary for free were re-read from the build files rather than taken from the plan text, and all four confirmed:

| Gate | Mechanism | Location |
|---|---|---|
| Local plugin build (4th free C++11 gate) | `SOURCES += $(wildcard src/*.cpp)` | `Makefile:11` |
| `make strict` | `$(wildcard src/*.cpp)` | `Makefile:76` |
| CI strict C++11 pedantic | `src/*.cpp` | `.github/workflows/test.yml:72` |
| CI MinGW compile + link vs libRack | `for f in src/*.cpp` → `build-ci/*.o` | `.github/workflows/test.yml:80,86` |

## Accomplishments

- **ROADMAP success criterion 3 is now real rather than vacuously true.** Before this plan, `make strict` and the CI MinGW leg compiled `AnalogLFO.cpp` and `plugin.cpp` only — they reported PASS while covering **zero** VCO code, because nothing under `src/` includes `VcoCore.hpp` until Phase 30. `make strict` now expands to `src/AnalogLFO.cpp src/plugin.cpp src/vco_compile_canary.cpp`.
- **The P-1 trap is closed and proven closed.** A canary that only `#include`s a header emits nothing, ODR-uses nothing, and gives the link leg nothing to resolve. This one emits a **defined** symbol at `-O3`: `nm` reports `T __ZN5forge21vcoCompileCanaryProbeEi`. It also survives the real plugin link — `nm -gU plugin.dylib` finds the probe in the built artifact, which is direct evidence that the CI MinGW link leg will have something to resolve.
- **The guard is validated by observed RED three separate ways**, not by a green run:
  - `[4/5]` compiles three synthetic C++17-ism translation units and requires each to fail. All three are rejected.
  - The canary was temporarily reduced to a bare `#include` — `[1/5]` and `[2/5]` both went red with the exact "would be permanently and silently green" diagnostic, then the file was restored from git.
  - Temporary `src/dsp/VcoExtra.hpp` and `src/dsp/MorphBlep.hpp` stubs were landed — `[5/5]` went red for both, then they were removed.
- **The commented Phase 32 placeholder cannot fake the D-08 gate.** `[5/5]` strips whole-line comments before grepping, so when `dsp/MorphBlep.hpp` actually appeared during the RED probe the gate failed *despite* the `// #include "dsp/MorphBlep.hpp"` line sitting in the canary. This was verified, not assumed.
- **P-5 avoided.** `tests/check_docs.sh` has been a complete but uninvoked gate since Phase 27. `tests/check_canary.sh` got its CI step in the same commit as the script itself.
- **Zero Makefile edits, append-only CI.** `git diff` over the plan range shows `Makefile` untouched and `.github/workflows/test.yml` with **0 removed lines** and no `ffp-contract` hit.

## Task Commits

1. **Task 1: canary placement decision** — no commit (decision checkpoint; recorded here)
2. **Task 2: ODR-using compile canary translation unit** — `e78cb55` (feat)
3. **Task 3: `check_canary.sh` guard + CI wiring** — `3de3c6b` (test)

## Files Created/Modified

- `src/vco_compile_canary.cpp` (created, 92 lines) — Path comment plus a five-part banner: purpose and the two gates it feeds; the D-08 growth rule naming Phase 32's `dsp/MorphBlep.hpp` as next due; an explicit **NOT DEAD CODE — DO NOT DELETE, DO NOT MAKE static, DO NOT REDUCE TO A BARE #include** block; the operator-approved shipped-artifact cost; and the include-hygiene rules (no Rack SDK header, no `tests/` header). `forge::vcoCompileCanaryProbe(int)` is forward-declared then defined; the body seeds a `VcoCore` with the house drift/spread constants, fills a `VcoInputs` field-by-field (never brace-initialized with a value list — P-8), accumulates `core.step(in)` over `(i & 3) + 1` iterations, and returns the accumulator plus `core.tel.displayPhase`.
- `tests/check_canary.sh` (created, 240 lines) — `check_docs.sh` scaffold: `set -euo pipefail`, script-relative `SCRIPT_DIR`/`ROOT`, `fail=0` + `note_fail`, numbered `[n/5]` sections, ruled summary, explicit `exit 0`/`exit 1`. `CANARY_REL="src/vco_compile_canary.cpp"`, `CXX_BIN="${CXX:-c++}"`, `mktemp -d` scratch removed by a `trap ... EXIT`. Needs no Rack SDK and runs from any directory.
- `.github/workflows/test.yml` (modified, +11 lines, −0) — One appended `toolchain-gate` step named exactly `VCO compile canary guard (D-07/D-08)` running `bash tests/check_canary.sh`, preceded by a comment explaining why the canary needs its own guard. No existing step modified, reordered or reindented; the `test` job untouched.

## Decisions Made

- **`option-a` — canary in `src/`** (operator). Recorded above in full.
- **Forward-declare, then define.** The separate declaration is what gives the definition external linkage that no compiler can prove unreachable. Without it the optimizer is free to discard the body and the canary silently stops biting.
- **Runtime-derived trip count.** `(i & 3) + 1` cannot be constant-folded, so the loop — and therefore the `core.step()` call and the whole seam — survives into the object file at `-O3`. Verified by `nm`, not assumed.
- **`[[maybe_unused]]` is informational, not a hard requirement.** Apple clang rejects it under `-std=c++11 -pedantic-errors`; GCC may merely ignore an unknown attribute with a warning. Making it hard would produce a false failure on the ubuntu CI runner (assumption A3). It is compiled and reported either way, so a future reader can see which compiler caught what. On this machine it was rejected — reported as such.
- **Comment-stripping in `[1/5]` and `[5/5]`.** The banner names the probe in prose and carries a commented `MorphBlep` include. Without stripping, the documentation would satisfy the gates it is documenting.
- **`nm` absence is a hard failure, not a skip.** If `nm` is missing the script fails loudly. A guard that silently skips its central check is precisely the failure mode this phase exists to prevent.
- **No local ODR reproduction attempted.** Stated explicitly in the script banner as a deliberate exclusion with the reason (Apple clang materializes the construct as a per-TU local symbol and links cleanly at every `-O` level), so a future reader does not mistake the guard's scope. That negative control belongs to plan 29-05, CI-observed.

## Deviations from Plan

None - plan executed exactly as written.

No deviation rule fired. No auto-fix was required: the canary compiled clean under the C++11 pedantic gate on the first run (as 29-01 predicted), and every acceptance criterion passed on first evaluation. No missing critical functionality, blocking issue, or architectural question surfaced, and no package was installed.

**Total deviations:** 0

All five hard prohibitions held: no existing CI step modified (0 removed lines); `TEST_CXXFLAGS`, the `test` target and the `capture` target untouched (`Makefile` has a zero-byte diff across the plan); `tests/BlockDriver.hpp`, `tests/test_golden.cpp`, `tests/golden/*`, `src/AnalogLFO.cpp` and all existing `src/dsp/*.hpp` untouched; no Rack module registered; and no attempt was made to reproduce the ODR link failure locally.

Three temporary RED probes were performed to validate the guard's red-capability. Each was reverted in the same command that created it — the canary via `git checkout --` on that single file, the header stubs via `rm`. `git status` was confirmed clean afterwards and the plan-range diff shows **0 deletions** and no unintended file.

## LFO Non-Regression Guardrail

This plan modified `.github/workflows/test.yml`, shared CI infrastructure the shipped LFO depends on. The guardrail was verified before the commit landed, not after:

| Risk | Status |
|---|---|
| R-4 — `TEST_CXXFLAGS` changed (would move golden float results) | **Untouched in both copies.** `Makefile` zero-byte diff across the plan; CI diff `grep -c 'ffp-contract'` → 0. |
| R-5 — an existing CI step weakened | **0 removed lines** in the workflow diff. Ruby YAML parse confirms `toolchain-gate` went 5 → 6 steps by pure append and the 3-step `test` job is unchanged, so the LFO's existing CI legs lost no coverage. |
| R-6 — `make capture` regenerating goldens | Never run. This plan touched nothing under `tests/golden` (`git diff --name-only` over the plan range → empty). |
| R-10 — canary reaches the released binary | Surfaced as a blocking decision checkpoint before anything was written, with impact, both options and a recommendation. Operator chose `option-a` knowingly; the cost is documented in the file banner. |
| Shipped-LFO behavior | `make test` → **64 cases / 2,615,099 assertions / 0 failed**. `-tc="golden*"` → **6/6 passed**. `make strict` → PASS. `make` (plugin build) → exit 0. |
| Shipped LFO files vs `v2.0.1` | `git diff --exit-code v2.0.1` over `AnalogLFO.cpp`, `plugin.cpp`, `plugin.hpp`, `plugin.json`, `BlockDriver.hpp`, `test_golden.cpp`, `tests/golden/*.f32`, `freerun_seeds.txt` and all 11 frozen `src/dsp/*.hpp` → **exit 0**. |

The one non-obvious point worth flagging for the operator: **the released plugin binary now contains one additional symbol.** Confirmed present: `nm -gU plugin.dylib` finds `forge::vcoCompileCanaryProbe`. This is the approved `option-a` cost, working exactly as intended — its presence in the linked artifact is the same property that makes the CI MinGW link leg able to catch the ODR class. It has no static initializer, is never called, and references no LFO code path.

## Threat Flags

None. No new network endpoint, auth path, file-access pattern, or schema change at a trust boundary. The register's `mitigate` dispositions were implemented as specified:

| Threat ID | Status |
|---|---|
| T-29-10 (canary in released binary) | Mitigated — single namespaced external-linkage function, no static initializer, never invoked, unique name, cost disclosed in the banner and approved at the Task 1 checkpoint. |
| T-29-11 (CI gate weakening) | Mitigated — append-only; 0 removed lines asserted, `ffp-contract` line never touched, YAML structure diffed step-count-wise. |
| T-29-12 (canary that emits no code) | Mitigated — `[2/5]` `nm` defined-symbol assertion, **observed red** against a bare-`#include` canary. |
| T-29-13 (guard nothing invokes) | Mitigated — CI step added in the same commit as the script; step name and script path both grep-asserted. |
| T-29-14 (in-class `static constexpr` ODR) | Partially mitigated as designed — the canary routes VCO headers into the CI MinGW link leg, the only gate for this class. **Not locally provable and explicitly not claimed**; the CI-observed proof is plan 29-05's. |
| T-29-15 / T-29-SC (supply chain) | Accepted — no package installed, the SDK fetch step unmodified. |

## Known Stubs

None. Both artifacts are fully implemented and exercised in the same commit range.

`forge::vcoCompileCanaryProbe` has no caller by design — that is the point of a canary, and it is emphatically *not* an unfinished stub: `tests/check_canary.sh [2/5]` fails the build if it ever stops being emitted, and the file banner states in three places that it must not be deleted or made `static`.

The commented `// #include "dsp/MorphBlep.hpp"` line is a documented D-08 growth point, not a stub — and it was proven unable to satisfy `[5/5]` on its own.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Ready for plan 29-04.** The resolved canary path for `GUARD_SCRIPTS`, `make guards` and `tests/check_includes.sh` is **`src/vco_compile_canary.cpp`**, and the guard script to add to `GUARD_SCRIPTS` is **`tests/check_canary.sh`**. 29-04's repo-wide "every guard script is referenced by the workflow" assertion will find this one already compliant — `tests/check_docs.sh` is the one that will not be.
- **Note for 29-04's D-06 include audit:** `src/vco_compile_canary.cpp` is a `src/` file that includes `dsp/VcoCore.hpp`. That is a legitimate, intended dependency direction and must not be flagged. It includes no `tests/` header, by design.
- **Ready for plan 29-05.** The CI-observed negative control now has something to act on: 29-05 can temporarily introduce an in-class runtime-indexed `static constexpr` into a VCO header and watch the MinGW link leg fail, because the canary guarantees that header is compiled and linked in CI.
- **Phase 30 obligation:** when `src/AnalogVCO.cpp` lands it is gated identically and automatically by the same globs. The canary stays — it is the growth point for headers `AnalogVCO.cpp` does not itself reach.
- **Phase 32 obligation:** uncomment and complete the `dsp/MorphBlep.hpp` include in the canary. `check_canary.sh [5/5]` will fail the build until that happens, so it cannot be forgotten.
- No blockers. No operator decision is outstanding.

## Self-Check: PASSED

- Files claimed created exist on disk: `src/vco_compile_canary.cpp` FOUND, `tests/check_canary.sh` FOUND.
- Commits claimed exist in git: `e78cb55` FOUND, `3de3c6b` FOUND.
- Task 1 acceptance re-verified: the literal string `option-a` and the resolved canary path both appear in this summary, and `git status --porcelain src tools Makefile .github` was empty at the moment the operator replied.
- All Task 2 and Task 3 acceptance criteria re-run and passing.
- All seven plan-level `<verification>` commands re-run: `make strict` exit 0 / PASS with the canary in the expanded command; `make` exit 0; `bash tests/check_canary.sh` exit 0 with three C++17-isms rejected; `make test` exit 0 / 64 passed / 0 failed; step-name grep → 1; CI removed-line count → 0; frozen-file diff vs `v2.0.1` → exit 0.
- Working tree clean apart from planning docs; plan-range diff is +343 / −0 across exactly 3 files.

---
*Phase: 29-vco-test-harness-lfo-non-regression-guardrail*
*Completed: 2026-07-28*
