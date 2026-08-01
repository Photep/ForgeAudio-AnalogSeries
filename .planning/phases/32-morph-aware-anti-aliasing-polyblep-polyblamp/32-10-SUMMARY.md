---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
plan: 10
subsystem: testing
tags: [phase-gate, ci, mingw, cross-toolchain, traceability, deferred-register, aa-05, core-02, test-03, t-32-05, t-32-12, t-32-15, t-32-26, t-32-27]

# Dependency graph
requires:
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plans 32-01..32-09 — every measured figure, every falsified premise and every assertion this gate reports on"
  - phase: 29-vco-test-harness-and-lfo-guardrail
    provides: "the compile canary, the guard suite, and the PROVEN rule that the entire local gate returns exit 0 on a commit that cannot link"
  - phase: 30-vcocore-skeleton-module-registration
    provides: "the locate-the-run-BY-SHA discipline and the read-the-STEP's-own-conclusion rule"
  - phase: 31-pitch-tuning-exponential-fm
    provides: "the account-for-the-matrix-gap-by-measuring-both-sides rule, and the four-consecutive-plans requirement-marking discipline"
provides:
  - "The full local gate recorded as numbers against the pre-phase baseline, with the +13/+4,266 delta accounted for plan by plan"
  - "The milestone guardrail asserted over the WHOLE phase diff rather than one commit, FROZEN.sha256 proved byte-identical by cmp"
  - "The CI toolchain-gate observed green BY SHA, with the win-x64 link-leg STEP's own conclusion read rather than the job's"
  - "A NEW phase finding: the spectral instrument is toolchain-dependent by up to 3.02596 dB, and the split that makes the reproduction check honest"
  - "isStepDominatedCell — the physical numerical-robustness criterion, with 48/42 populations both asserted"
  - "deferred-items.md — 8 falsified premises with their correction sites, 17 deferred items with owners"

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Split an over-tight tolerance on a PHYSICAL criterion stated before the population is enumerated, so the split cannot be a rename of the failure list"
    - "Assert both populations of a split exactly, so a classifier that empties one side fails loudly instead of making a bound vacuous"
    - "Discriminating mutation probe: one offset that fails exactly one population and a larger one that fails both, proving the boundary is where it claims"
    - "Re-verify a requirement by re-running its selector and checking a NON-ZERO matched case count, because a zero-matching selector also exits 0"

key-files:
  created:
    - .planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/deferred-items.md
  modified:
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - tests/test_vco_spectrum.cpp

key-decisions:
  - "The CI observation was reached via branch gsd/phase-32-ci rather than main. This was an OPERATOR DECISION taken before the push, not a deviation the executor chose: origin is a public repo and main was not to move. CI triggers on [push, pull_request], so a branch push produces a run on the identical SHA and the by-hash evidence is fully preserved. origin/main is unmoved at 80fb90a throughout."
  - "The first CI run was RED on two of three legs and the finding was escalated to the operator rather than auto-fixed, because tests/test_vco_spectrum.cpp carries a standing capitalised STOP-AND-REPORT instruction for exactly that assertion and the remedy touches T-32-15, the phase's highest-severity threat"
  - "Option (b) was chosen: split the reproduction bound on a physical criterion, rather than widening it globally (a) or re-pinning the column on a new canonical toolchain (c). It is the only option that records the toolchain-dependence as a measured property instead of hiding it under a wider tolerance"
  - "No requirement was ticked by this plan. All nine were already ticked by the plans that landed their assertions; this plan RE-VERIFIED each against a named case with a non-zero matched count, and recorded MORPH-02's qualification rather than silently accepting it"
  - "kHostileBoundV, the threshold column and every measuredDb value are untouched — 0 grid rows appear in this plan's diff"

requirements-completed: [AA-05]

coverage:
  - id: D1
    description: "The full local gate is green and recorded as numbers against the pre-phase baseline of 81 cases / 2,618,053 assertions, with the delta accounted for plan by plan"
    requirement: "AA-05"
    verification:
      - kind: integration
        ref: "make test 94/94/0 at 2,622,319; make strict PASS over 4 TUs; make guards PASS; make -j4 relinked from scratch producing plugin.dylib"
        status: pass
    human_judgment: false
  - id: D2
    description: "The milestone guardrail holds over the WHOLE phase diff: src/AnalogLFO.cpp and every frozen path absent, FROZEN.sha256 byte-identical asserted by reading bytes"
    requirement: "CORE-02"
    verification:
      - kind: other
        ref: "git diff --name-only 93757de..HEAD = 22 files; each of the 15 FROZEN.sha256 paths returns 0 changed lines; cmp against the pre-phase blob reports IDENTICAL BYTES (734276a7...)"
        status: pass
      - kind: unit
        ref: "all six LFO goldens green; the three Apple-gated drift-ON cases measure exactly 24,582 assertions"
        status: pass
    human_judgment: false
  - id: D3
    description: "The CI toolchain-gate's win-x64 link leg is observed green on the exact commit BY SHA, with the STEP's own conclusion read rather than the job's (T-32-05 / T-32-26)"
    requirement: "AA-05"
    verification:
      - kind: integration
        ref: "run 30681442134 on SHA 262e5c5; headSha string-compared to local HEAD BEFORE any conclusion was read; toolchain-gate job = success; step 6 OWN conclusion = success; verbatim verdict 'win-x64 link gate: PASS'"
        status: pass
    human_judgment: false
  - id: D4
    description: "Every new case ran on all three legs, proven by a per-leg BEFORE/AFTER delta rather than by an absent platform guard"
    requirement: "AA-05"
    verification:
      - kind: integration
        ref: "macOS 81->94 / 2,618,053->2,622,319; Ubuntu and Windows 78->91 / 2,593,471->2,597,737. Delta +13 cases and +4,266 assertions on ALL THREE legs identically; gap unchanged at exactly 3 cases / 24,582 assertions"
        status: pass
    human_judgment: false
  - id: D5
    description: "The measuredDb reproduction check is honest across toolchains: split on a physical criterion, both populations asserted, both branches proved able to fire"
    requirement: "TEST-03"
    verification:
      - kind: unit
        ref: "tests/test_vco_spectrum.cpp#vco spectrum: the naive and corrected alias floors... — stepDominatedCells == 48, plateauCells == 42, sum == 90"
        status: pass
      - kind: other
        ref: "discriminating mutation probe: +2.0 dB offset fails EXACTLY 48 (all saw, all pulse, square at char 0 and 0.5, no plateau cell); +5.0 dB fails EXACTLY 90"
        status: pass
    human_judgment: false
  - id: D6
    description: "Requirement status reflects named assertions only, each re-verified by a non-zero matched case count (T-32-27)"
    requirement: "TEST-03"
    verification:
      - kind: other
        ref: "13 selectors re-run, each matching exactly 1 case and passing; the per-ID discharge table below names the case and file for all nine Phase 32 IDs"
        status: pass
    human_judgment: false
  - id: D7
    description: "Every falsified premise this phase corrected is recorded with the source location of its correction (T-32-23)"
    requirement: "TEST-03"
    verification:
      - kind: other
        ref: "deferred-items.md: 8 falsified-premise entries, 17 deferred entries, 25 'Resolve at' clauses; six correction sites spot-checked by grep and all found"
        status: pass
    human_judgment: false

# Metrics
duration: 98 min
completed: 2026-08-01
status: complete
---

# Phase 32 Plan 10: The Phase Gate Summary

**The local gate is green and recorded as numbers, the milestone guardrail holds over the whole phase diff with `FROZEN.sha256` proved byte-identical by reading bytes, and the CI toolchain-gate's win-x64 link leg is observed green on the exact commit by hash with the STEP's own conclusion read rather than the job's — and the observation did its job: the FIRST run was RED on two of three toolchains, revealing that this phase's spectral instrument is toolchain-dependent by up to 3.02596 dB. That finding was escalated to the operator rather than auto-fixed, and the fix records the dependence as a measured property instead of hiding it under a wider tolerance.**

## Performance

- **Duration:** ~98 min
- **Completed:** 2026-08-01
- **Tasks:** 3
- **Files:** 1 created, 3 modified

## Task Commits

1. **Task 1 — the full local gate, the whole-phase guardrail, requirement traceability** — `a110a9a` (`docs`)
2. **Task 2 — the cross-toolchain split the CI observation forced** — `262e5c5` (`fix`)
3. **Task 3 — the deferred register** — `45fb468` (`docs`)

---

## Task 1 — The Local Gate, As Numbers

| Gate | Result |
|---|---|
| `make test` | **94 cases / 94 passed / 0 failed**, **2,622,319** assertions |
| `make strict` | `strict C++11 gate: PASS` over **4** translation units |
| `make guards` | `guard suite: PASS` |
| `make -j4` | **relinked from scratch** (`plugin.dylib` deleted first, so this is a real link and not a no-op), exports `_modelAnalogLFO` and `_modelAnalogVCO` |

### The delta against the pre-phase baseline, accounted for plan by plan

Baseline at `93757de`: **81 cases / 2,618,053 assertions**. Now **94 / 2,622,319** → **+13 cases, +4,266 assertions**.

| plan | cases | assertions | what was added |
|---|---:|---:|---|
| 32-01 | +2 | +286 | the DFT apparatus case, and the mirror-identity tombstone |
| 32-02 | 0 | 0 | shell wiring only |
| 32-03 | +2 | +568 | the 90-cell naive baseline, and the D-08 RED tombstone |
| 32-04 | 0 | 0 | header only; RED was an out-of-tree probe |
| 32-05 | +6 | +909 | the six `morph blep:` unit cases |
| 32-06 | 0 | +63 | tombstone INVERTED in place, not added beside |
| 32-07 | +2 | +1,217 | no-regression invariant, D-11 cross-rate case |
| 32-08 | 0 | +126 | scenario five, inside invariant 2 |
| 32-09 | +1 | +1,094 | invariant 6, plus scenario four 48→176 configurations |
| 32-10 | 0 | **+3** | the two population censuses and their sum |
| **total** | **+13** | **+4,266** | 81 → 94, 2,618,053 → 2,622,319 |

Both columns reconcile exactly.

### The guard suite's own verdicts, quoted

- `check_frozen.sh` — `(15 pinned entries checked)`, `OK: every shipped LFO header and the LFO shell are pinned (completeness)`, `(6 pinned fixtures checked)`, and the negative control `OK: perturbed copy of src/dsp/MathConst.hpp was detected as different`. The manifest is **unchanged at 15 entries**.
- `check_includes.sh [1/7]` — `OK: 29 LFO-side root file(s), 29 file(s) opened across their transitive include closure, zero VCO includes`. Both new test TUs are exempt by **exact path**, pre-registered in plan 32-01 before either file existed.
- `check_includes.sh [2/7]` — `OK: src/dsp/MorphBlep.hpp — no Rack include`
- `check_includes.sh [3/7]` — `OK: src/dsp/MorphBlep.hpp — siblings and standard headers only`
- `check_canary.sh [5/5]` — `OK: dsp/MorphBlep.hpp is carried into both gates by the canary`
- `check_canary.sh [5b/5]` — `OK: every header in the VCO seam is Vco*-named or an allowed frozen shared header`
- `check_canary.sh [2b/5]` — `OK: all 8 VcoInputs DSP field(s) stay runtime-live through step() at -O3`

### The milestone guardrail, over the WHOLE phase diff

`git diff --name-only 93757de..HEAD` lists **22 files** (21 at Task 1, plus `deferred-items.md`): three `.planning` documents, the nine plan SUMMARYs plus this one, `res/AnalogVCO.svg`, `src/AnalogVCO.cpp`, `src/dsp/MorphBlep.hpp`, `src/dsp/VcoCore.hpp`, `src/vco_compile_canary.cpp`, `tests/check_includes.sh`, `tests/test_morph_blep.cpp`, `tests/test_vco_core.cpp`, `tests/test_vco_spectrum.cpp`.

**Every one of the 15 paths named in `src/dsp/FROZEN.sha256` returns 0 changed lines over the whole phase**, checked individually rather than as a group:

```
0  src/dsp/Waveshape.hpp      0  src/dsp/ClockTracker.hpp   0  src/AnalogLFO.cpp
0  src/dsp/RackCompat.hpp     0  src/dsp/RatioTable.hpp     0  tests/BlockDriver.hpp
0  src/dsp/MathConst.hpp      0  src/dsp/Swing.hpp          0  tests/test_golden.cpp
0  src/dsp/DriftEngine.hpp    0  src/dsp/PatchParse.hpp     0  tests/golden/freerun_seeds.txt
0  src/dsp/LfoCore.hpp        0  src/dsp/DisplayFill.hpp    0  src/dsp/Anim.hpp
```

**`FROZEN.sha256` is byte-identical, asserted by READING BYTES** (the Phase 30 lesson — "assert byte identity by reading bytes, not by counting git diff markers"): `git show 93757de:src/dsp/FROZEN.sha256` and the working file both hash to **`734276a7c5579bfda7cad2ecfe214216cee9e894a21e20208c2849d5726d1488`**, and `cmp` between them reports **IDENTICAL BYTES**.

**The six shipped LFO goldens replay green.** All six ran on this platform: three portable drift-off cases and three `#if defined(__APPLE__)` drift-ON cases. **The gap is accounted for BY NUMBER, not by assertion:** the three Apple-gated cases measure **exactly 24,582 assertions** locally, which is exactly the three-OS matrix delta on both axes (see Task 2). None was skipped — `0 skipped` on every run.

---

## Task 2 — The CI Observation, And What It Found

### The operator decision that shaped it

The plan says *"push the phase branch"*. `origin` is a **public** GitHub repository and `main` was not to move, so **the operator decided before any push that the observation would be reached via a dedicated branch, `gsd/phase-32-ci`.** This is recorded as an operator decision, **not as a deviation the executor chose**. CI triggers `on: [push, pull_request]`, so a branch push produces a run on the **identical SHA** and the by-hash evidence the plan requires is fully preserved. No pull request was opened, nothing was merged, no tag was cut, nothing was force-pushed, and **`origin/main` is unmoved at `80fb90a` throughout** — verified by `git ls-remote` after both pushes.

### Run one — `30680251253` on SHA `a110a9a`: the named gate GREEN, the suite RED

Located **by head-commit equality**, never by run name and never by recency. Both hazards were live and both were avoided:

- **Name-based selection is unsatisfiable here.** `toolchain-gate` is a **JOB** inside the single workflow named `test`; a selector matching the RUN name returns nothing and the whole check goes silently green regardless of CI health (the defect Phase 30 found and corrected).
- **Recency-based selection would have been wrong.** `30511183170` on `80fb90a` — Phase 31's own green run — sat **one line below** the target in `gh run list`.

| what | value |
|---|---|
| `toolchain-gate` **job** conclusion | `success` |
| step 6 `win-x64 leg reproduction (compile + full link vs libRack)` **STEP** conclusion | **`success`** |
| the step's verbatim verdict line | **`win-x64 link gate: PASS`** |
| **overall run conclusion** | **`failure`** |

**`make test` was RED on ubuntu-latest and windows-latest, GREEN on macos-latest** — 1 case, **21 assertions**, all at the same line:

```
tests/test_vco_spectrum.cpp:1816: ERROR: CHECK( std::fabs(recordedDrift) <= 1.0 ) is NOT correct!
  values: CHECK( 2.94241 <= 1 )
  logged: i := 1   sr := 44100   K := 195   note := C7   region := sine
          character := 0.5   threshold := -61   method := 2
          naiveDb := -59.541      correctedDb := -67.5503
          recordedDb := -64.6079  recordedDrift := -2.94241
```

### Why this was escalated to the operator instead of auto-fixed

`tests/test_vco_spectrum.cpp` carries a **standing, capitalised instruction written by plan 32-07 and addressed to the next agent**, immediately above the failing assertion:

> `>>> IF THIS FIRES, STOP AND REPORT IT RATHER THAN UPDATING THE NUMBER.`
> *"…re-typing `measuredDb` to match a new run silently re-pins the whole column against whatever the implementation now produces — the exact failure mode the anti-softening clause exists to prevent."*

Widening the tolerance is the same act by another route, and the remedy touches **T-32-15**, the phase's highest-severity threat, whose escalation ends at the operator. **That is Rule 4, not Rule 1.** Three options were presented with a recommendation; **the operator chose option (b)**.

### What the failure was, and what it was NOT

**Nothing in `src/` behaves differently across toolchains**, and this was established *before* anything was changed:

- `make strict` and the **MinGW compile-and-link leg** are green on that same SHA.
- The **TEST-03 gate itself passes on all three legs** — it never appears as a failing case.
- The no-regression invariant, the D-11 cross-rate case and the reconstruction proof all pass everywhere.
- The six LFO goldens replay bit-exact on every leg.

**What differs is the INSTRUMENT.** `aliasPeakDb` reports a **max over 2043 non-harmonic bins**. The FFT twiddles and the frozen `Waveshape`'s own trig come from the platform's libm, which differs in the last unit in the last place between implementations. One ULP cannot move a peak standing tens of decibels clear of its neighbours — but where the alias spectrum is a **near-flat plateau of near-tied bins**, one ULP **reorders which bin wins**.

### The fix — option (b), and the four binding conditions it was given

**1. The split is derived from a physical criterion stated BEFORE the population is enumerated**, so it cannot be a rename of "the cells that happened to fail":

> A cell is **STEP-DOMINATED** when the waveform at that (region, character) carries a **true value-step discontinuity**, whose 6 dB/octave harmonic series folds to alias peaks standing well clear of the spectrum, so the arg-max is stable under a last-place perturbation. It is **PLATEAU-DOMINATED** when it carries no such step, so its alias content is second-order (a slope break) or the bleed ring alone.

Applied to the five regions from **the frozen header's own measured behaviour**: **saw** is step-dominated at every character (the `+2.000000` jump of falsified premise 2); **pulse** at every character (two hard rectangle edges); **square** below full character (`−1.201655` at 0.50) but **not at 1.00**, where plan 32-05 measured the same jump collapse to **`−0.001661`**; **triangle** never (no value step at any character — the reason AA-02 is a separate requirement from AA-01); **sine** never (its alias energy is entirely the bleed ring).

| population | cells | bound | provenance |
|---|---:|---:|---|
| step-dominated | **48** | **1.0 dB** | **UNCHANGED** from 32-07 |
| plateau | **42** | **4.0 dB** | measured worst **3.02596 dB** (cell **i = 86**, 96 kHz square char 1.00) rounded outward, **0.974 dB headroom** |

**2. The looser bound is pinned from measurement and is a real bound.** 4.0 is **1.32×** the worst observation, not an order of magnitude above it — the same outward-rounding rule the MEASURE-TO-PIN PROTOCOL's step 2 states, and the same shape as 32-07's own two re-pinned tolerances (no-regression 4.0 against a measured 2.3344; cross-rate 6.0 against a measured 4.7059).

**3. The split is a strict SUPERSET of the failure list, which is the point.** 21 cells drifted; **all 21 fall inside the 42**. The other 21 plateau cells reproduced within 1.0 dB anyway — they are on the looser bound because the criterion says they **can** be fragile, not because they were seen to be. And **every one of the 48 step-dominated cells reproduced within 1.0 dB on all three legs** — the criterion's own prediction, and the evidence that it is the right criterion rather than a convenient one. **No step-dominated cell drifted**, so the operator's "report, don't reclassify" clause had nothing to report.

**4. Sensitivity proved by a DISCRIMINATING mutation probe**, not by a green run:

| probe | failing drift assertions | which cells |
|---|---:|---|
| `recordedDrift + 2.0` | **exactly 48** | all saw (18), all pulse (18), square at char 0 and 0.5 (12) — **no plateau cell** |
| `recordedDrift + 5.0` | **exactly 90** | both populations |

Both branches bite, **at the boundary they claim**. Square at character 1.00 is correctly absent from the 48.

### How T-32-15 comes out at least as well defended

- **No threshold and no `measuredDb` value was edited.** `git diff` shows **0 grid rows**; the only `thresholdDb` hit in the diff is a superseded comment line.
- **The STOP-AND-REPORT instruction is preserved**, now binds **both** branches explicitly, and **gained** a clause: *"IF THE CELL THAT FIRES IS STEP-DOMINATED, THAT IS A FINDING ABOUT THE CRITERION, NOT A CELL TO RECLASSIFY"* — closing the one new evasion the split creates.
- **Both populations are asserted exactly** (`48`, `42`, sum `90`), so a classifier that silently emptied the tight side fails loudly instead of moving every cell onto the looser bound.
- **The derivation assertion is untouched and independent.** Every gated threshold is still derived from `measuredDb` by `CHECK(threshold == max(ceil(measuredDb + 3.0), kThresholdFloorDb))` in the TEST-03 gate, which fires on **any** re-typing regardless of the drift bound.

### Run two — `30681442134` on SHA `262e5c5`: green everywhere

`headSha` was string-compared to local `HEAD` **before any conclusion was read**.

| what | value |
|---|---|
| run id | **30681442134** |
| SHA | **`262e5c54019cd5c1da72c60ed1f3668a9dd5454f`** |
| branch | `gsd/phase-32-ci` (operator decision) |
| `toolchain-gate` **job** conclusion | **`success`** |
| step 6 **STEP** conclusion | **`success`** |
| verbatim verdict line | **`win-x64 link gate: PASS`** |
| all four jobs | `toolchain-gate`, `test (ubuntu-latest)`, `test (windows-latest)`, `test (macos-latest)` — **all `success`** |
| failing assertions anywhere in the run | **0** |

**Only the step conclusion is the gate.** The job conclusion is recorded but is never sufficient: a step that fail-fasts upstream reports `skipped`, which scans as "not red" in a job summary. All 12 `toolchain-gate` steps are individually `success` — **none `skipped`** — so no upstream fail-fast is hiding behind the job's verdict.

### The three-OS matrix, both sides measured

| leg | before (pre-phase) | after | Δ cases | Δ assertions |
|---|---|---|---:|---:|
| macOS | 81 / 2,618,053 | **94 / 2,622,319** | **+13** | **+4,266** |
| Ubuntu | 78 / 2,593,471 | **91 / 2,597,737** | **+13** | **+4,266** |
| Windows | 78 / 2,593,471 | **91 / 2,597,737** | **+13** | **+4,266** |

**The per-leg delta is identical on all three legs**, which is what proves every one of this phase's 13 new cases ran everywhere. An absent platform guard in a new file is only an argument; a per-leg delta is evidence.

**The macOS-versus-others gap is exactly 3 cases and 24,582 assertions**, unchanged from before the phase: `94 − 91 = 3` and `2,622,319 − 2,597,737 = 24,582`, matching the three `#if defined(__APPLE__)` drift-ON golden cases measured locally at **exactly 24,582 assertions**.

### Why this run mattered more than the last two

**`src/dsp/MorphBlep.hpp` is the first genuinely new VCO header since Phase 29**, and it reaches the MinGW link leg **only** through the compile canary's include (`check_canary.sh [5/5]` confirms the carry). The failure class that gate exists for — an **in-class constant odr-used at runtime producing an undefined reference** — is invisible to Apple clang at every optimization level and invisible to every syntax-only gate on every platform, because `-fsyntax-only` never links. Phase 29 proved the gate bites: run `30339957128` failed with `undefined reference to forge::VcoCore::ODR_PROBE_TBL`, green again after revert. `MorphBlep.hpp` keeps its site arrays **function-local `const`** precisely to stay clear of that class, and this run is the first evidence that it does.

**And the run vindicated Phase 29's rule a second time, in a new way.** The entire local gate was green on the commit whose suite was red on two of three toolchains — the same shape as Phase 29's commit that could not link. **Local green is a precondition and never evidence.**

---

## Task 3 — The Deferred Register

`.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/deferred-items.md`, in the Phase 31 format (found during · observation · why it is worth a note · why it is not fixed here · resolve at).

**25 items: 8 falsified premises corrected, 17 deferred with owners. 25 `Resolve at` clauses.**

### Section one — falsified premises, each naming where its correction lives

| # | premise | correction landed at | spot-checked |
|---|---|---|---|
| 1 | D-15's "Phase 32's oversampled inner loop" (**two places**) | `tests/test_vco_core.cpp:1101` **and** `src/dsp/VcoCore.hpp:491` | ✅ both |
| 2 | P-4's saw soft-reset corollary | `src/dsp/MorphBlep.hpp:422` | ✅ |
| 3 | `research/STACK.md:40`'s over-correction argument | `src/dsp/MorphBlep.hpp:38` | — |
| 4 | `research/STACK.md:100-104`'s polyBLAMP snippet | `src/dsp/MorphBlep.hpp:50` | ✅ |
| 5 | D-04's site map as literally worded | `src/dsp/MorphBlep.hpp:446-447` + `tests/test_morph_blep.cpp` | ✅ |
| 6 | invariant 1's grid attribution | `tests/test_vco_core.cpp:594` | ✅ |
| 7 | two stale banner sentences no plan named | `src/dsp/VcoCore.hpp:7`, `tests/test_vco_core.cpp` banner | — |
| 8 | **NEW** — `measuredDb` reproduces on Apple clang only | `tests/test_vco_spectrum.cpp` (`isStepDominatedCell`) | ✅ |

Six spot-checked by grep against the named file; all found.

**Item 3 is the load-bearing one:** "erring toward over-correction is the safe direction" is falsified by a **30 dB regression** (naive −60.1, full authority −29.9), which is what makes the D-03 factor's **compact support a requirement rather than a preference**.

**Item 5 carries the phase's clearest vacuous-assertion case:** at `morph = 0.75` the square-split probe measures 1.485677 and **would have PASSED** — on the **pulse's** hard step, which does not split — i.e. green against exactly the merged-entry defect it exists to catch. Unsatisfiable is a nuisance; **vacuous is a hazard**.

**Item 8 is a first-class discovery of this phase**, recorded with its mechanism, its bound, and the warning that **every absolute decibel figure this phase recorded is an Apple-clang figure** — to within ~3 dB in the plateau regime and ~1 dB elsewhere. Pointed at **Phase 36**, which owns goldens and must not capture a spectral golden from one toolchain.

### Section two — deferred, with owners

The narrow-pulse reach refinement (**the phase's one known DSP gap**, 3–5 dB on bleed-ring-dominated cells, ~0 dB elsewhere, 34 of 45 gated cells within 0.64 dB; **not taken** because it would add the only division by an edge width) → the first plan that misses a pulse threshold. Four-point kernels (−25.8 → ~−36 dB, still not −60; AA-05 does not speak to kernel order) → **an OPERATOR DECISION, never a silent choice**; broad path stays v2.1 oversampling, explicitly not minBLEP. Probed magnitudes → only if a D-09 threshold is unreachable **and** research blames magnitude rather than order. **The shipped LFO's latent UB → NO PHASE**, unfixed by decision, a guardrail event, with the still-binding consequence that **a repo-wide UBSan gate cannot be adopted** (Phase 32 used no sanitizer at all). Hard sync seam `addStep` → Phase 33, **never snap the reset to exactly zero**, and PITCH-04 must be re-confirmed not inherited. Phase 34's D-04 re-read, naming all four fields — `wv.squareDutySpread`, `wv.pulseEdgeSpread`, `wv.bleedSpread`, `wv.triAsymmetrySpread`. CHARACTER CV + attenuverter → Phase 34 (also closes the canary's one-field margin and CR-02's remaining half). Seed entropy + patch persistence → Phase 34/35. COARSE snap → Phase 35 or v2.1. FM depth affordance → Phase 35. `check_docs.sh` CI wiring → Phase 36. `plugin.json` at 2.0.1 with two modules → Phase 36. Plus `kSelfCheckDb` **stays at −72.0** (recommendation recorded with its reasoning), the `dt = 0.0005` measure-zero missed edge, the unreachable `dt` upper bound, MORPH-02's shell-mix qualification → 32-11, and the operator's stale pre-rename plugin directory.

---

## Requirement Traceability — The Per-ID Discharge Table

**No requirement was ticked by this plan.** All nine were already ticked by the plans that landed their assertions. This plan **re-verified** each by re-running its selector and checking a **non-zero matched case count** — because a selector matching **zero** cases also exits 0 and prints `Status: SUCCESS!` (Phase 31's 31-08 finding), so exit status alone cannot distinguish a discharged requirement from an unrun one.

| ID | discharging test case | file | cases / assertions |
|---|---|---|---|
| **MORPH-01** | `vco core: audio-rate MORPH sweeping through every segment boundary stays finite and bounded (MORPH-01 / MORPH-02)` | `tests/test_vco_core.cpp` | 1 / 326 |
| **MORPH-02** | same case (27 configurations, non-vacuity REQUIREd first) — **qualified, see below** | `tests/test_vco_core.cpp` | 1 / 326 |
| **AA-01** | `vco spectrum: TEST-03 — the alias floor stays below its per-shape pinned threshold at C7, C8 and C9` + `vco spectrum: the core now DIVERGES from NaiveVcoCoreMirror by EXACTLY the MorphBlep correction` | `tests/test_vco_spectrum.cpp` | 1 / 240 · 1 / 288 |
| **AA-02** | `morph blep: the site magnitudes ARE the characterized jumps of the frozen Waveshape (AA-04 / D-01)` part B — slope breaks −8.000016 / +8.000016 with a REQUIRE that the triangle contributes **no** value jump | `tests/test_morph_blep.cpp` | 1 / 33 |
| **AA-03** | `morph blep: overlapping pulse edges SUM rather than overwrite at a narrow duty (AA-03 / D-07)` | `tests/test_morph_blep.cpp` | 1 / 65 |
| **AA-04** | `morph blep: morphBlepCharFactor hits D-03's limits EXACTLY…` + the site-magnitude case above | `tests/test_morph_blep.cpp` | 1 / 80 · 1 / 33 |
| **AA-05** | `make strict` (4 TUs) + `check_includes.sh [2/7]`/`[3/7]` + `check_canary.sh [5/5]` + `morph blep: the D-14 sync seam feeds the SAME accumulator, and hostile dt reaches no divisor` + **the CI win-x64 link-leg STEP on run 30681442134** | `tests/check_*.sh`, `tests/test_morph_blep.cpp`, CI | 1 / 57 + gates |
| **CORE-02** | `check_frozen.sh [1/3]` (15-entry manifest) and `[3/3]` (negative control) + `check_canary.sh [5/5]` + the D-08 inversion case | `tests/check_frozen.sh`, `tests/check_canary.sh`, `tests/test_vco_spectrum.cpp` | 1 / 288 + gates |
| **TEST-03** | `vco spectrum: TEST-03 — …` (`failing == 0` over 45 gated cells) | `tests/test_vco_spectrum.cpp` | 1 / 240 |

All 13 selectors re-run matched **exactly 1 case each** and passed.

### MORPH-02's qualification, recorded rather than absorbed

MORPH-02 reads *"MORPH knob + CV + attenuverter sweep the continuous 5-shape crossfade … at audio rate."* The **crossfade-at-audio-rate** half is discharged by a named case that names the requirement. The **knob + CV × attenuverter** half is **asserted by no test case anywhere**: D-17 added **zero POD fields**, so `forge::VcoInputs::morph` is post-CV and post-clamp and **no headless driver can reach the attenuverter**. That mix lives in `src/AnalogVCO.cpp::process()` and is covered only by `make strict` and the real link until plan **32-11**'s operator UAT.

MORPH-02 **stays Complete** — it has a named case that names it, and un-ticking would misrepresent the evidence in the other direction. But the mark is **qualified**, recorded in `REQUIREMENTS.md`'s footer and as **deferred item 24**. This is the same shape as Phase 31 deferred item 11, where PITCH-04 was marked complete on two of the three input classes it names. **The difference between a documented gap and a PANEL-03-style false green is that the gap is written down.**

**No Phase 32 requirement lacks a named assertion**, so nothing was un-ticked.

---

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] The `measuredDb` reproduction check was RED on two of three toolchains — escalated to the operator, then fixed under their decision**

- **Found during:** Task 2, the first CI observation — which is precisely what the task exists to do.
- **Issue:** `CHECK(|recordedDrift| <= 1.0)` at `tests/test_vco_spectrum.cpp` failed **21 times** on ubuntu-latest and windows-latest and passed on macos-latest. The column had been pinned in plan 32-07 from **one toolchain's run**; GCC/libstdc++ and MinGW g++ measure the same cells up to **3.02596 dB** away.
- **Escalated, not auto-fixed.** The source carries a standing capitalised **STOP-AND-REPORT** instruction for this exact assertion, and the remedy touches **T-32-15**. Three options were presented with a recommendation; **the operator chose option (b)** and set four binding conditions, all of which are satisfied above.
- **Fix:** the bound is split on a physical criterion (48 step-dominated at 1.0 dB, 42 plateau at 4.0 dB), both populations asserted exactly, both branches proved able to fire by a discriminating probe, the STOP instruction preserved and strengthened, and **0 grid rows** in the diff.
- **Files modified:** `tests/test_vco_spectrum.cpp` · **Committed in:** `262e5c5`
- **Verified on the toolchain that actually failed:** re-pushed and re-observed — run `30681442134` on `262e5c5`, **all four jobs `success`, 0 failing assertions anywhere**.

---

**2. [Recorded, not a code change] The plan's Task 1 traceability instruction assumed the requirements still needed ticking**

- **Issue:** the plan says *"tick the checkbox for each discharged ID and set its traceability-table status to Complete."* All nine were **already** ticked, by the plans that landed their assertions (32-02, 32-05, 32-06, 32-07, 32-09). Re-applying would have produced no diff.
- **What was done instead:** the nine were **re-verified** against named cases with non-zero matched counts, and the verification — plus MORPH-02's qualification — was recorded in the file's footer. This is stronger than the plan's instruction: it converts a bookkeeping step into evidence.

---

**3. [Recorded] The plan's Task 1 instruction to "tick each" of the eleven plans in ROADMAP would have booked a false green**

- **Issue:** plan **32-11** (the operator in-Rack UAT) has **not executed**. Ticking it would be exactly the PANEL-03 false green this phase's discipline exists to refuse.
- **What was done:** 32-09 and 32-10 ticked, **32-11 left unticked**, count set to **10/11**. The plan list already carried all eleven plans with one-line objectives, so that half of the instruction needed no change. **SC-2 and SC-4 are untouched** — they appear in the diff only as context lines.

---

**Total deviations:** 1 auto-fixed under an explicit operator decision after escalation, 2 recorded without a code change. **No `src/` file was touched by this plan.** No threshold, no `measuredDb`, no bound in the D-09 column was edited; no guard was weakened; `src/AnalogLFO.cpp` and every frozen path are absent from all three commits.

---

## Threat Mitigations Applied

| Threat | Mitigation as landed |
|---|---|
| **T-32-05** (in-class constant odr-used → MinGW undefined reference) | The win-x64 link-leg **STEP's own** conclusion observed `success` on SHA `262e5c5`, verdict `win-x64 link gate: PASS`. First run carrying `src/dsp/MorphBlep.hpp` into that leg; its site arrays are function-local `const` by construction. |
| **T-32-12** (the shipped LFO's golden bit-stability) | Asserted over the **whole phase diff**: `src/AnalogLFO.cpp`, `Waveshape.hpp`, `RackCompat.hpp` and all 15 `FROZEN.sha256` paths at 0 changed lines each; `FROZEN.sha256` byte-identical by `cmp`; six goldens green on all three legs. |
| **T-32-26** (a run selected by name or by recency) | Located by **head-commit equality**, verified **before** any conclusion was read, on both runs. Both hazards were live: name-based selection is unsatisfiable (`toolchain-gate` is a job, not a workflow), and Phase 31's green run sat one line below the target. |
| **T-32-27** (a requirement marked complete without an assertion behind it) | Nine per-ID rows above, each naming a case and a file, each re-verified by a **non-zero matched case count**. MORPH-02's qualification recorded rather than absorbed. |
| **T-32-23** (a falsified premise inherited by a later phase) | 8 falsified-premise entries, each naming its correction site; six spot-checked by grep and all found. |
| **T-32-15** (threshold provenance / circular pinning) | **Left at least as well defended**: 0 grid rows edited, the STOP instruction preserved and extended with an anti-reclassification clause, both populations asserted exactly, and the independent derivation assertion untouched. |
| **T-32-28** (information disclosure) | No file, network, logging or persistence operation. The plugin remains offline and stores nothing. |
| **T-32-SC** (package installs) | **Zero packages installed in any ecosystem** across the entire phase. `plugin.json` gains no dependency. No install checkpoint applies because there is no install step. |

---

## Known Stubs

None. Every line this plan added is either asserted by a case that runs on every invocation or is a documentation record. There is no placeholder, no skipped case and no `TODO`.

**The phase's one known DSP gap is not a stub:** the 3–5 dB bleed-ring shortfall against the prototype is a documented, measured, bounded limitation with an explained cause and a named escalation path, and it does **not** prevent TEST-03 from being achieved — every gated cell passes.

## Threat Flags

None. No network, auth, file-access or schema surface was introduced by this plan.

## Issues Encountered

The CI failure described in deviation 1 — which is not an issue with this plan but a **finding this plan exists to produce**. It is recorded as deferred item 8.

## User Setup Required

None.

## Next Phase Readiness

**Ready for 32-11**, the operator in-Rack UAT — the one thing no headless gate can do.

Plan 32-11 inherits: the headless counterpart figures its judgment sits beside (5.508759 to 6.289864 V across 27 audio-rate MORPH configurations at the same three rates and three modulation rates); **the verification-protocol fix it owns** — name the **plugin directory** as well as the module when asking for an audition, because a second, older, differently-slugged Forge plugin puts a stale second LFO in the module browser (deferred item 25); and **MORPH-02's shell-mix qualification** (deferred item 24), the one half of a requirement no test case can reach.

Phase 36 inherits the sharpest new constraint: **every absolute decibel figure this phase recorded is an Apple-clang figure**, and a spectral golden must not be captured from one toolchain.

## Self-Check: PASSED

- `.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/deferred-items.md` — FOUND on disk; 26 `##` headings, 8 falsified-premise entries, 17 deferred entries, 25 `Resolve at` clauses
- `.planning/REQUIREMENTS.md` — FOUND; nine Phase 32 IDs ticked and Complete, footer records the re-verification and MORPH-02's qualification
- `.planning/ROADMAP.md` — FOUND; Phase 32 at 10/11, 32-09 and 32-10 ticked, 32-11 unticked, SC-2 and SC-4 untouched
- `tests/test_vco_spectrum.cpp` — FOUND; `isStepDominatedCell` present, STOP instruction preserved, 0 grid rows changed
- Commits `a110a9a`, `262e5c5`, `45fb468` — all FOUND in `git log`
- CI run `30681442134` on `262e5c5` — headSha equals local HEAD; job and step conclusions both `success`
- `make test` 94/94/0 at 2,622,319, `make strict` PASS, `make guards` PASS, `make -j4` relinked — all re-run at the final commit

---
*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Completed: 2026-08-01*
