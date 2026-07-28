---
phase: 30-vcocore-skeleton-module-registration
plan: 04
subsystem: testing
tags: [vco, doctest, core-03, d-17, d-10, polyphony, interleave, positive-control, anti-vacuity]

# Dependency graph
requires:
  - phase: 29-vco-test-harness-lfo-guardrail
    provides: "tests/VcoBlockDriver.hpp — the four-seed constructor and run()'s unconditional per-sample sampleTime/sampleRate overwrite, which the interleave helper reproduces and is asserted bit-identical against"
  - phase: 30-vcocore-skeleton-module-registration
    plan: 02
    provides: "the live forge::VcoCore::step() body with `double phase` and `Waveshape wave` as PER-INSTANCE members — literally the property this plan proves — plus kVcoFreqC4, kVcoNyquistGuardFrac and the D-11 five-coefficient setSpreadSeed the broken control mirrors"
  - phase: 30-vcocore-skeleton-module-registration
    plan: 03
    provides: "tests/test_vco_core.cpp with SAMPLE_RATES, coreBase() and estimateFreqRising(), the banner slots numbered 4 and 5, and the check_includes.sh [1/7] VCO_SIDE_ALLOW entry — this plan appends and redefines none of them"
provides:
  - "runInterleaveCheck<CoreT>() — the shared, TEMPLATED solo-versus-interleaved drive helper, reproducing VcoBlockDriver's unconditional per-sample timing overwrite and asserted bit-identical to it"
  - "InterleaveResult — mismatchA, mismatchB, soloEqual and soloA (the last carried out for the validity check)"
  - "DeliberatelyBrokenSharedStateCore — the PERMANENT positive control, anonymous namespace, test TU only, never under src/"
  - "D-17 / CORE-03 proven behaviorally: two differently-seeded cores driven interleaved sample by sample each reproduce their solo block bit-exactly (0/1024 mismatches, all three rates)"
  - "The independence check validated by an OBSERVED red on every run: the control measures 512/512 mismatches on both instances at all three sample rates"
affects: [30-07, 31-pitch-tuning-fm, 32-morph-blep, 34-analog-engine-output, v2.1-POLY-01]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Template the drive loop over the core type so the positive control exercises BYTE-IDENTICALLY the code the check uses — the check_includes.sh [6/7] argument (every negative control calls the same function its section calls) applied to a C++ test fixture"
    - "Validity check FIRST, result second: REQUIRE the fixture is equivalent to the harness's own drive discipline before asserting anything about what it measured — the nc2_direct habit from check_includes.sh [6/7]"
    - "A permanent in-test broken implementation whose containment is a property of PLACEMENT (anonymous namespace in a test TU) plus an asserted grep, not of a guard section — because src/-only gates structurally cannot see it"
    - "Assert an inequality, not an exact count, when the fixture's own baselines are polluted by the defect it demonstrates — an exact count would pin an accident of run order"
    - "Carry the intermediate artifact (soloA) out of the helper so the caller can pin the helper itself, rather than trusting it"

key-files:
  created: []
  modified:
    - tests/test_vco_core.cpp

key-decisions:
  - "Executor: InterleaveResult carries soloA in addition to the plan's three required numbers. The plan requires the caller to REQUIRE the helper's solo block bit-identical to VcoBlockDriver::run() over the same seeds and inputs — which is impossible unless the block leaves the helper. Additive to the plan's stated return contract, not a substitution."
  - "Executor: the sensitivity probe used `static inline double phase` rather than `static double phase`. A bare in-class `static double phase = 0.0;` is ill-formed in BOTH C++11 and C++17 (only inline variables may carry an in-class initializer for non-const non-integral types), so the probe would not have compiled at all. `static inline` is the compiling realization of the plan's 'made static', and the test target is C++17. See Deviations."
  - "Executor: the probe was run TWICE — once as specified, and once with the validity REQUIRE temporarily softened to a CHECK. The first run went red at the validity check, which is a correct detection but aborts the case before the property assertions execute. The second run was needed to OBSERVE the mismatch counts the plan's <output> block requires recorded. Both files restored from git/backup afterwards."
  - "Executor: the control measures 512/512 on BOTH instances, where the researcher recorded 511/512 and 512/512. The one-sample difference is run-order dependent by construction (the four runs share one static, so instance A's first interleaved sample can coincidentally land on its solo value); 512/512 is the stronger result and the assertion is an inequality precisely so this cannot become brittle."
  - "Executor: the file banner's 'Also not here yet: the CORE-03 independence PAIR' sentence was corrected rather than left standing — it became false the moment this plan landed. Recorded as a deviation."

requirements-completed: [CORE-03]

coverage:
  - id: D1
    description: "Two differently-seeded VcoCore instances driven INTERLEAVED sample by sample each reproduce their solo block bit-exactly, at character = 1.0, with different spread seeds and different per-sample inputs (D-17 / CORE-03)"
    requirement: "CORE-03"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"vco core: two-instance independence*\" -s -> r.mismatchA := 0 and r.mismatchB := 0 at 44100, 48000 and 96000; 1 case, 18 assertions, 0 failed"
        status: pass
      - kind: integration
        ref: "grep -vE '^[[:space:]]*//' tests/test_vco_core.cpp | grep -c 'Approx' -> 0; every comparison is a direct float !="
        status: pass
    human_judgment: false
  - id: D2
    description: "All five measured non-vacuity requirements are implemented and MEASURED rather than assumed: different spread seeds, character = 1.0, different per-sample inputs per instance, an explicit distinguishability assertion, and the permanent positive control"
    requirement: "CORE-03"
    verification:
      - kind: unit
        ref: "distinguishability MEASURED: CHECK( r.soloEqual < n/10 ) reports `0 < 102` at all three rates — the two solo blocks are equal at 0 of 1024 samples, matching the researcher's figure exactly"
        status: pass
      - kind: integration
        ref: "seeds: identical drift pair (0xC0FFEE, 0xBADF00D) for both instances, spread pairs (0x9E3779B9, 0x7F4A7C15) vs (0xDEADBEEF, 0xCAFEF00D); inputs: A sweeps pitchCV -1..+1 at fixed morph 0.25, B holds pitchCV 0.5 and sweeps morph 0..1; base.character = 1.f in both cases"
        status: pass
    human_judgment: false
  - id: D3
    description: "The interleave helper is proven equivalent to VcoBlockDriver::run()'s drive discipline, so a helper that stopped injecting timing cannot pass the assertions below it (T-30-06)"
    requirement: "CORE-03"
    verification:
      - kind: unit
        ref: "REQUIRE( helperMatchesHarness ) — the helper's solo A block compared bit-exactly against forge::VcoBlockDriver(sr, same four seeds).run(n, inA); green at all three rates, and OBSERVED red under the static-phase probe"
        status: pass
      - kind: integration
        ref: "tests/VcoBlockDriver.hpp and tests/BlockDriver.hpp received ZERO edits — git diff --stat HEAD~2 HEAD names exactly one file"
        status: pass
    human_judgment: false
  - id: D4
    description: "The independence check is validated by an OBSERVED detection on every single run, through a permanent control that shares one static accumulator and runs through the same helper (T-30-06)"
    requirement: "CORE-03"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"vco core: independence positive control*\" -s -> r.mismatchA := 512, r.mismatchB := 512, totalMismatch := 1024 at ALL THREE rates; CHECK( 1024 > 0 ) passes by DETECTING the defect"
        status: pass
      - kind: integration
        ref: "grep -c 'runInterleaveCheck' -> 8; both cases call the one helper, which is defined once. grep -c 'DeliberatelyBrokenSharedStateCore' -> 7"
        status: pass
    human_judgment: false
  - id: D5
    description: "Invariant 4's sensitivity is MEASURED, not argued: the case was observed going red against a shared static phase member in src/dsp/VcoCore.hpp, and the header was restored (T-30-06)"
    requirement: "CORE-03"
    verification:
      - kind: integration
        ref: "SENSITIVITY: with `double phase` spliced to `static inline double phase`, the case exits 1. Softened-REQUIRE variant recorded r.mismatchA := 1024 and r.mismatchB := 1024 at all three rates, 9 of 18 assertions failing. Both files restored; git status --porcelain src empty"
        status: pass
      - kind: integration
        ref: "make test -> 72/72 green after restore; git status --porcelain -> clean; no .bak or probe fixture in the tree"
        status: pass
    human_judgment: false
  - id: D6
    description: "The test-only broken core never leaves the test TU, and its prohibition is written where a future refactorer would act on it (T-30-08)"
    requirement: "CORE-03"
    verification:
      - kind: integration
        ref: "grep -rl 'DeliberatelyBrokenSharedStateCore' src/ -> no match. The type lives in this TU's anonymous namespace (internal linkage, no header, no shipped build graph)"
        status: pass
      - kind: integration
        ref: "its banner names src/dsp/VcoCore.hpp explicitly as the file it must never migrate to, and states it must not be deleted, disabled or cleaned up"
        status: pass
      - kind: integration
        ref: "make strict -> PASS and make guards -> guard suite: PASS — all three src/-only gates are structurally blind to it, which is the point of its placement"
        status: pass
    human_judgment: false
  - id: D7
    description: "The shipped LFO is untouched and its goldens are byte-identical (T-30-04)"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"golden*\" -> 6 cases, 49,164 assertions, 0 failed"
        status: pass
      - kind: integration
        ref: "git diff --stat HEAD~2 HEAD names exactly tests/test_vco_core.cpp; tests/BlockDriver.hpp, tests/VcoBlockDriver.hpp and every file under src/ are absent from the diff"
        status: pass
    human_judgment: false

# Metrics
duration: 6 min
completed: 2026-07-29
status: complete
---

# Phase 30 Plan 04: CORE-03 Two-Instance Independence Summary

**Two differently-seeded `VcoCore` instances driven interleaved sample by sample each reproduce their solo block with 0 of 1024 samples differing at all three rates — and the check that proves it is itself validated on every run by a deliberately-broken core sharing one static accumulator, measured at 512/512 mismatches on both instances.**

## Performance

- **Duration:** 6 min
- **Started:** 2026-07-28T22:44:23Z
- **Completed:** 2026-07-28T22:51:11Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments

- **Landed CORE-03 as a property rather than a proxy.** The requirement is a claim about what is *absent* — no static, no global, no accidentally shared engine — and a grep for `static` catches the obvious declaration form while missing a function-local static, a shared reference member, a singleton behind an accessor and a shared pointer. The interleave test catches all of them, because shared mutable state of any shape makes an interleaved run diverge from its own solo run.
- **Implemented all five measured non-vacuity requirements, and measured each rather than assuming it.** Different spread seeds (identical drift pair, so any observation can only come from the spread copy); `character = 1.0`; genuinely different per-sample inputs per instance; an explicit distinguishability assertion that **measured 0 of 1024 equal samples** at every rate; and the permanent positive control.
- **Made the helper a template so the control cannot drift from the check.** `runInterleaveCheck<CoreT>()` is instantiated once on `forge::VcoCore` and once on `DeliberatelyBrokenSharedStateCore`, so the control exercises byte-identically the drive loop the real case uses. That argument is already written into `check_includes.sh [6/7]`'s banner — every negative control there calls the *same function* its section calls — and it applies here unchanged.
- **Pinned the helper to the harness before trusting anything it reports.** The case `REQUIRE`s the helper's solo A block bit-identical to what `forge::VcoBlockDriver` produces from the same four seeds over the same inputs. A helper that quietly stopped overwriting `sampleTime` and `sampleRate`, or seeded in the wrong order, or reused a core between runs, would sail through every assertion below that line. It cannot get past that one — **proved**, because the static-phase probe made this exact `REQUIRE` fire first.
- **Wrote the measured character trap into the case, not into a planning document.** Every spread coefficient in the frozen `Waveshape` is gated behind `character >= 0.001f`, so a clobbered shared `Waveshape` is **0/1024 detectable at `character = 0`**. The detectability table is recorded at the case, so a later reader who lowers character to "simplify" it can read why that destroys the test.
- **The positive control is specific, not merely different.** The defect is isolated to exactly one field: `DeliberatelyBrokenSharedStateCore` keeps a per-instance `Waveshape`, a per-instance seeding entry point performing the same D-11 five-coefficient copy, the same `exp2_taylor5` pitch off `kVcoFreqC4`, the same NaN-safe zero test and Nyquist clamp, the same single-subtract wrap and the same ×5 unconditioned output. Only the phase accumulator is a function-local static. So the control demonstrates the helper catches *shared state*, not merely that two classes produce two streams of numbers.
- **Proved invariant 4's sensitivity by observation.** With `phase` made a shared static in `src/dsp/VcoCore.hpp`, the case exits 1; with the validity `REQUIRE` temporarily softened so execution continues, **`mismatchA = 1024/1024` and `mismatchB = 1024/1024` at all three sample rates**, 9 of 18 assertions red.
- **Contained the broken core by placement and asserted the containment.** It has internal linkage in a test TU's anonymous namespace, appears in no header and no shipped build graph, and `grep -rl` finds it nowhere under `src/`. Its banner names `src/dsp/VcoCore.hpp` explicitly as the file it must never migrate to, so the prohibition sits where a future refactorer would act on it.

## Task Commits

Each task was committed atomically, and every commit names exactly one file:

1. **Task 1: the interleave helper and the two-instance independence case (D-17)** — `2c1d05f` (test) — `tests/test_vco_core.cpp`
2. **Task 2: the permanent positive control** — `ad47c05` (test) — `tests/test_vco_core.cpp`

**Plan metadata:** see the `docs(30-04)` commit following this SUMMARY.

## Files Created/Modified

- `tests/test_vco_core.cpp` — **+445 / −4, now 917 lines.** Banner invariants 4 and 5 added to the numbered list plus the closing "validated rather than merely green" paragraph; `InterleaveResult`, `runInterleaveCheck<CoreT>()` and `DeliberatelyBrokenSharedStateCore` appended to the existing anonymous namespace; the two CORE-03 cases appended after invariant 3. `#include <functional>` added (include-what-you-use — `VcoBlockDriver.hpp` supplies it transitively, but the helper's own signature names `std::function`). Plan 30-03's three helpers and three cases are **provably unchanged**: the only four deleted lines in this plan's entire diff are the stale banner sentence corrected below.

**Untouched, as the plan's hard prohibitions require:** `tests/VcoBlockDriver.hpp` (the `core` member was already public — zero edits needed and zero made), `tests/BlockDriver.hpp` (feeds the shipped LFO's macOS bit-exact drift-ON golden leg), and every file under `src/`.

## Measured Results — required by the plan's `<output>` block

Recorded because plan 30-07's phase gate compares CI figures against these, and v2.1's POLY-01 will be planned against this evidence rather than re-deriving it. **Assumption A5 stands open: every number below was taken on Apple clang 16.0.0 only.**

### Invariant 4 — the real core, n = 1024

Drift pair `(0xC0FFEE, 0xBADF00D)` identical for both instances; spread pairs `(0x9E3779B9, 0x7F4A7C15)` vs `(0xDEADBEEF, 0xCAFEF00D)`; `character = 1.0`; instance A sweeps `pitchCV` −1 V..+1 V at `morph = 0.25`, instance B holds `pitchCV = 0.5` and sweeps `morph` 0..1.

| | 44.1 kHz | 48 kHz | 96 kHz |
|---|----------|--------|--------|
| interleaved-vs-solo mismatches, **instance A** | **0 / 1024** | **0 / 1024** | **0 / 1024** |
| interleaved-vs-solo mismatches, **instance B** | **0 / 1024** | **0 / 1024** | **0 / 1024** |
| solo blocks equal at (distinguishability) | **0 / 1024** | 0 / 1024 | 0 / 1024 |
| helper solo A == `VcoBlockDriver::run()` | bit-identical | bit-identical | bit-identical |

Every figure matches the researcher's prototype measurement exactly (A = 0/1024, B = 0/1024, `soloA[i] == soloB[i]` on 0/1024), which is the strongest available evidence that the core under test is the body those results were taken against.

### Invariant 5 — the positive control, n = 512

| | 44.1 kHz | 48 kHz | 96 kHz |
|---|----------|--------|--------|
| mismatches, instance A | **512 / 512** | 512 / 512 | 512 / 512 |
| mismatches, instance B | **512 / 512** | 512 / 512 | 512 / 512 |
| total (the asserted quantity) | **1024** | 1024 | 1024 |

The researcher recorded **511/512 and 512/512**; this run measures 512/512 on both. The one-sample difference is run-order dependent *by construction* — all four runs inside the helper share the single static, so whether instance A's first interleaved sample happens to land on its solo value depends on where the accumulator was left by the preceding runs. **This is exactly why the assertion is `totalMismatch > 0` rather than an exact count**: an exact count would pin an accident of run order rather than the property, and would go brittle the moment a case is added ahead of this one. 512/512 is the stronger result in the same direction.

### Invariant 4 — sensitivity probe (required by the plan's acceptance criteria)

Procedure: splice `double phase = 0.0;` in `src/dsp/VcoCore.hpp` to a shared static, `touch`, rebuild, run the case alone, restore from git.

**Run 1 — exactly as the plan specifies. The case went RED, exit 1:**

```
tests/test_vco_core.cpp:714: FATAL ERROR: REQUIRE( helperMatchesHarness ) is NOT correct!
  values: REQUIRE( false )
  logged: sr := 44100

[doctest] test cases: 1 | 0 passed | 1 failed | 70 skipped
[doctest] assertions: 3 | 2 passed | 1 failed |
```

The **validity check** fired first, which is itself a correct detection — it compares the helper's solo A against a *separate* `VcoBlockDriver` instance, and a shared static breaks that comparison too. But `REQUIRE` is fatal, so the property assertions never executed and the mismatch counts the plan's `<output>` block requires recorded were never produced.

**Run 2 — the same probe with the validity `REQUIRE` temporarily softened to a `CHECK` so execution continues:**

```
tests/test_vco_core.cpp:729: ERROR: CHECK( r.mismatchA == 0 ) is NOT correct!
  values: CHECK( 1024 == 0 )
  logged: sr := 96000
          r.soloEqual := 0
          r.mismatchA := 1024
          r.mismatchB := 1024

[doctest] test cases:  1 | 0 passed | 1 failed | 70 skipped
[doctest] assertions: 18 | 9 passed | 9 failed |
```

**Nine failed assertions — the validity check and BOTH property assertions, red at ALL THREE sample rates, with every single one of 1024 samples mismatching on both instances.** Note that `r.soloEqual := 0` stayed correct throughout: distinguishability is a property of the two input functors and is not affected by the defect, which is the right result and confirms the three assertions are independent rather than three views of one number.

Post-probe state verified: `src/dsp/VcoCore.hpp` restored (`grep -n "double phase"` → line 116, `double phase = 0.0;`), `tests/test_vco_core.cpp` restored from a scratch backup (`REQUIRE(helperMatchesHarness)` present at line 714), `git status --porcelain src` empty, working tree clean, no `.bak` and no probe fixture anywhere in the tree, `make test` 72/72 green.

## Verification Evidence

Plan-level `<verification>`, run from the repository root:

| # | Check | Result |
|---|-------|--------|
| 1 | `make test` | exit 0 — **72 cases / 72 passed / 0 failed**, 2,615,872 assertions (the 67-case Phase-29 baseline, plus 30-03's three, plus this plan's two) |
| 2 | `./build-test/test -tc="vco core*"` | exit 0 — **5 cases**, 750 assertions; `-ltc \| grep -c 'vco core:'` → **5** |
| 3 | `./build-test/test -tc="vco harness*"` | exit 0 — 7 cases, 35 assertions; plan 30-02's work undisturbed |
| 4 | `./build-test/test -tc="golden*"` | exit 0 — 6 cases, **49,164 assertions**. All six shipped-LFO goldens byte-identical |
| 5 | `make strict` | exit 0 — `strict C++11 gate: PASS` (standing caveat: `-fsyntax-only`, never links, not evidence of link health) |
| 6 | `make guards` | exit 0 — `guard suite: PASS`; `make guards RACK_DIR=/nonexistent-rack-sdk` also exit 0 |
| 7 | `grep -rl 'DeliberatelyBrokenSharedStateCore' src/` | **no match** — the control never left the test tree |
| 8 | `git status --porcelain src` | **empty** — the sensitivity probe restored `src/dsp/VcoCore.hpp` from git |
| 9 | `git diff --stat HEAD~2 HEAD` | **exactly one file**: `tests/test_vco_core.cpp` (+445 / −4) |

Task-level acceptance criteria, spot-checked:

- Task 1: `make test` → **71/71** after commit 1; `-tc="vco core: two-instance independence*"` → exit 0, 1 case, 18 assertions; `-ltc | grep -c 'vco core:'` → **4**.
- Task 2: `make test` → **72/72**; `-ltc | grep -c 'vco core:'` → **5**.
- `grep -c 'runInterleaveCheck'` → **8** (≥3 required: one definition plus two call sites, the rest in banners); `grep -c 'DeliberatelyBrokenSharedStateCore'` → **7** (≥2 required).
- `grep -vE '^[[:space:]]*//' tests/test_vco_core.cpp | grep -c 'Approx'` → **0**.
- `git diff --diff-filter=D --name-only HEAD~2 HEAD` → empty; no file deleted by either commit.
- **Task 2's diff is PURE INSERTION** — 185 insertions, 0 deletions — so Task 1's helper and case, and all of plan 30-03's work, are provably unmodified by it.

## Decisions Made

- **Executor: `InterleaveResult` carries `soloA` alongside the plan's three required numbers.** The plan requires the caller to `REQUIRE` the helper's solo block bit-identical to `VcoBlockDriver::run()` — which cannot be done unless the block leaves the helper. Additive to the stated return contract, not a substitution; documented at the struct.
- **Executor: the probe used `static inline double phase`.** See Deviations — the plan's literal `static double phase` does not compile in any C++ standard this repo uses.
- **Executor: the probe was run twice.** The first run is the plan's procedure verbatim and produced the required red. The second, with the validity `REQUIRE` softened to a `CHECK`, was needed to *observe* the mismatch counts the plan's `<output>` block requires recorded. Both files were restored afterwards and the tree verified clean.
- **Executor: the control asserts an inequality over the summed mismatch count, at three sample rates.** The plan specifies `> 0` on the total; running it at all three rates (rather than one) costs nothing and makes the observed detection three independent detections per invocation.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The plan's sensitivity probe as literally specified does not compile**

- **Found during:** Task 1, at the acceptance criterion *"with `double phase` in `src/dsp/VcoCore.hpp` temporarily made `static`"*.
- **Issue:** `static double phase = 0.0;` as a class member is **ill-formed in both C++11 and C++17**. An in-class initializer on a `static` data member is permitted only for `const` integral/enumeration types (C++11) or for `inline` variables (C++17). Splicing the plan's literal text would have produced a compile error, not a red test — and a compile error is not evidence of sensitivity, because it proves nothing about whether the *assertions* can see the defect.
- **Fix:** Used `static inline double phase = 0.0;`, which is the compiling realization of "the accumulator is now shared between instances" and is legal at C++17, the standard the test target builds at. `make strict` (the C++11 gate) is not run during a probe, and the header was restored before either gate ran again.
- **Files modified:** `src/dsp/VcoCore.hpp` (probe only — restored via `git checkout --`, `git status --porcelain src` empty)
- **Verification:** the probe compiled, the case exited 1, and the restored header is byte-identical to HEAD.
- **Committed in:** n/a — no source change survives.

**2. [Rule 1 - Bug] The probe's red landed on the validity check, hiding the property assertions the plan requires recorded**

- **Found during:** Task 1, immediately after the first probe run.
- **Issue:** The validity check is a `REQUIRE` (fatal, by design — the plan specifies `REQUIRE` for it and `CHECK` for the others). A shared static breaks the helper-versus-harness comparison as well as the interleave property, so the fatal assertion fired first and aborted the case at the first sample rate. The case correctly exited 1, satisfying the acceptance criterion — but the plan's `<output>` block separately requires "the observed red from Task 1's `static` probe" recorded with its counts, and those counts were never computed.
- **Fix:** Ran the probe a second time with the validity `REQUIRE` temporarily softened to a `CHECK` so execution continued, captured `mismatchA = 1024` / `mismatchB = 1024` at all three rates, then restored `tests/test_vco_core.cpp` from a scratch backup. Both figures are recorded above. This is worth stating plainly rather than hiding: the fatal ordering is *correct* — a suite should abort when its fixture is proven invalid — and the second run exists only to produce the audit trail, not to weaken anything.
- **Files modified:** `tests/test_vco_core.cpp` (probe only — restored, `REQUIRE(helperMatchesHarness)` verified present)
- **Verification:** working tree clean; `make test` 72/72 after restore.
- **Committed in:** n/a — no source change survives.

**3. [Rule 1 - Bug] A file-banner sentence became false the moment this plan landed**

- **Found during:** Task 1, writing the banner's invariant list.
- **Issue:** Plan 30-03's banner reads *"Also not here yet: the CORE-03 independence PAIR — the interleave test and its deliberately-broken shared-state positive control — which plan 30-04 appends to this same file."* Leaving it would have put a claim in the file that the same file's own invariants 4 and 5 contradict, which is exactly the class of defect deviation 2 of plan 30-03 was raised for.
- **Fix:** Rewrote the sentence to record the transition rather than delete it — *"(Plan 30-03 wrote 'also not here yet: the CORE-03 independence pair'; plan 30-04 landed it, as invariants 4 and 5 below…)"* — which is the same tombstone-inversion posture plan 30-02 used for the D-15 silence case: the change is one readable diff line and the history stays legible.
- **Files modified:** `tests/test_vco_core.cpp` (comment only — no assertion changed, no threshold moved, no helper touched). **These four lines are the entire deletion count of this plan's diff.**
- **Verification:** `git diff HEAD~2 HEAD` shows exactly 4 deleted lines, all in that paragraph.
- **Committed in:** `2c1d05f` (Task 1 commit)

---

**Total deviations:** 3 auto-fixed, all Rule 1.
**Impact on plan:** No task was skipped, no acceptance criterion was relaxed, no threshold was moved to make a test pass, and no file outside `tests/test_vco_core.cpp` survives with a modification. Deviations 1 and 2 are both about the *probe*, not the shipped test — the probe still produced the required observed red, and produced better evidence than the plan asked for.

## Issues Encountered

None beyond the three deviations above; each was diagnosed and resolved inside the task that surfaced it. The same-second mtime tie that 30-02 and 30-03 recorded was pre-empted by `touch`-ing both probed files after every restore, per 30-02's deviation note.

## Known Stubs

None. This plan ships no placeholder values, no empty data sources and no TODO markers.

`DeliberatelyBrokenSharedStateCore` is **not** a stub and must not be read as one, which is precisely why its banner is as long as it is. It is a deliberately-broken fixture whose brokenness is its entire function — a permanent positive control, in the same family as `check_includes.sh [6/7]`'s synthetic fixtures and `check_frozen.sh [3/3]`'s. It has internal linkage in a test TU, it is in no header and no shipped build graph, and the plan's own acceptance criteria assert `grep -rl` finds it nowhere under `src/`. Anyone auditing this file for placeholder code should read its banner before acting.

Deliberately absent by decision, each stated at the case: no pitch, magnitude or divergence assertions here (plan 30-03 owns CORE-01), and no assertion about alias content or spectral cleanliness anywhere in this file (Phase 32 owns CORE-02 / AA-01..05).

## Threat Flags

None. This plan adds no network endpoint, no auth path, no file-access pattern and no schema change, and installs zero packages. The threats the plan's `<threat_model>` assigns to it:

- **T-30-06** (a green independence check that proves nothing recorded as CORE-03 coverage) — **mitigated, and DEMONSTRATED in both directions.** All five measured non-vacuity requirements are implemented; the case was observed going red against a shared static phase member with 1024/1024 mismatches on both instances at all three rates; and the permanent control is observed detecting its own defect (512/512, three rates) on every invocation of the suite.
- **T-30-08** (the test-only broken core migrating into `src/` and shipping) — **mitigated.** Anonymous namespace in a test TU, internal linkage, no header, no shipped build graph. Its banner names `src/dsp/VcoCore.hpp` as the file it must never enter, and `grep -rl 'DeliberatelyBrokenSharedStateCore' src/` finds no match. Containment is enforced by placement plus an asserted grep, because `check_includes.sh` and `check_canary.sh` structurally never scan test TUs.
- **T-30-04** (VCO test code perturbing the shipped LFO's golden replay) — **mitigated.** One file touched, and it is a test TU. `tests/VcoBlockDriver.hpp` received no edit (`core` was already public), and `tests/BlockDriver.hpp` — which feeds the macOS bit-exact drift-ON golden leg — was not opened. All six LFO goldens replay byte-identical (49,164 assertions).
- **T-30-SC** (supply chain) — not applicable; zero packages installed, doctest already vendored.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- **CORE-03 is closed behaviorally, and v2.1 polyphony is now an evidenced additive shell change rather than an asserted one.** POLY-01 can be planned against the figures in Measured Results instead of re-deriving them: the per-voice independence property already holds, at three sample rates, with the check's sensitivity measured.
- **Plan 30-06 (registration) and 30-07 (phase gate) are unblocked.** Nothing this plan touched is on either's path — one test TU, no `src/` file, no guard script, no build config.
- **Plan 30-07's phase gate has a complete audit trail.** Every figure above is emitted by `-s` on a passing run, so the CI comparison is a diff of test output rather than a re-derivation. It should confirm two things specifically: that invariant 5 is **passing by detecting** (a green suite where invariant 5 passes for any other reason is the failure mode), and that the CI toolchain reproduces 0/1024 and 512/512.
- **Assumption A5 stands open, unchanged:** every number here is Apple clang 16.0.0 only. Cross-toolchain confirmation is the first CI run's job.
- **The shipped LFO is untouched.** No `src/` file, no frozen header, no `FROZEN.sha256` bump, no golden fixture, no driver that feeds one.
- **One standing caveat, unchanged:** local `make test` / `make strict` / `make guards` are all green, and Phase 29 proved that exact state is achievable on code that cannot link. No tag or library resubmission on local evidence alone.
- **One item still open from plan 30-03** (not this plan's): the `check_includes.sh [1/7]` `VCO_SIDE_ALLOW` entry flagged there for operator confirmation at the phase gate. This plan needed no guard change of any kind — `tests/test_vco_core.cpp` was already registered.
- No blockers.

## Self-Check: PASSED

- `tests/test_vco_core.cpp` — FOUND on disk (917 lines).
- `.planning/phases/30-vcocore-skeleton-module-registration/30-04-SUMMARY.md` — FOUND on disk.
- Commit `2c1d05f` (Task 1) — FOUND in `git log --oneline --all`.
- Commit `ad47c05` (Task 2) — FOUND in `git log --oneline --all`.
- `git diff --diff-filter=D --name-only HEAD~2 HEAD` — empty; no file deleted by either commit.
- `git status --porcelain src` — empty; the sensitivity probe left nothing behind.
- `git status --porcelain` — clean after both commits; no untracked files, no `.bak`, no scratch probe.
- `grep -rl 'DeliberatelyBrokenSharedStateCore' src/` — no match.

---
*Phase: 30-vcocore-skeleton-module-registration*
*Completed: 2026-07-29*
