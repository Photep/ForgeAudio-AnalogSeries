---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
plan: 01
subsystem: testing
tags: [dft, fft, spectral-analysis, anti-aliasing, doctest, vco, polyblep, guard-scripts]

# Dependency graph
requires:
  - phase: 29-vco-test-harness-and-lfo-guardrail
    provides: tests/VcoBlockDriver.hpp, the check_includes.sh [1/7] denylist and its VCO_SIDE_ALLOW pre-registration precedent
  - phase: 30-vco-core-skeleton-and-registration
    provides: forge::VcoCore with the naive morphed oscillator, the DeliberatelyBrokenSharedStateCore mirror pattern, the validity-first REQUIRE idiom
  - phase: 31-pitch-tuning-fm
    provides: the volt-domain pitch summation, kVcoMaxPitchVolts, kVcoNyquistGuardFrac = 0.495, the ceiling-then-negated-floor guard order
provides:
  - "tests/test_vco_spectrum.cpp — the Phase 32 spectral apparatus: fftRadix2, aliasPeakDb, deltaPhaseForPitchCV, binCentredPitchCV, binCentredSampleTime, impliedLeakageDb"
  - "NaiveVcoCoreMirror — the permanent D-08 naive baseline, proven bit-identical to the live forge::VcoCore over a 45-point grid"
  - "The D-10 leakage self-check, asserted at >= 10 dB below the tightest threshold this phase will ever pin"
  - "Two exact-path VCO_SIDE_ALLOW entries in tests/check_includes.sh, pre-registered before either file existed"
affects: [32-02, 32-03, 32-04, 32-06, 32-07, alias-floor-thresholds, morph-blep]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Exact-integer-cycle rectangular DFT (D-10): zero leakage by construction, alias energy = magnitude at non-harmonic bins"
    - "Detector validated by DETECTING a planted spur, never by a green run"
    - "Test-only naive oracle preserved by copying into the test TU, with no src/ flag and no second core entry point"
    - "Two-method bin-centre placement: bisect pitchCV (harness unchanged) plus a sampleTime nudge for rows needing a tighter floor"

key-files:
  created:
    - tests/test_vco_spectrum.cpp
  modified:
    - tests/check_includes.sh

key-decisions:
  - "The D-10 self-check is asserted against a SECOND solver (binCentredSampleTime), because the plan's mandated bisect-on-pitchCV method fails it on 5 of 6 grid rows — measured, and observed red by mutation probe"
  - "TEST-03 deliberately NOT marked complete: this plan builds the apparatus, plans 32-03 and 32-07 land the assertions"
  - "forge::VcoBlockDriver is NOT modified and its sampleTime overwrite does NOT become conditional; the nudge is a separate local driving path"
  - "The solver's replicated pitch/guard chain is REQUIREd bit-equal to the live core's own telemetry at every grid row, rather than trusted by inspection"

patterns-established:
  - "Mutation-probe RED evidence for a test-only deliverable: prove the detector bites, restore, commit green"
  - "Tombstone-with-inversion-contract: the case name and an inline comment both name plan 32-06 as the plan that must flip the assertion in place"

requirements-completed: []

coverage:
  - id: D1
    description: "Both new Phase 32 VCO-side test TUs are exempt from the check_includes.sh [1/7] LFO-side scan by exact path, with written rationale, before either file exists"
    requirement: "TEST-03"
    verification:
      - kind: integration
        ref: "make guards (tests/check_includes.sh [1/7])"
        status: pass
      - kind: other
        ref: "VCO_SIDE_ALLOW array holds exactly 8 quoted entries; git diff shows 32 insertions, 0 deletions"
        status: pass
    human_judgment: false
  - id: D2
    description: "An exact-integer-cycle radix-2 FFT and an alias-peak classifier that reports a planted spur's bin and amplitude to within 0.1 dB"
    requirement: "TEST-03"
    verification:
      - kind: unit
        ref: "tests/test_vco_spectrum.cpp#vco spectrum: the DFT apparatus is validated by DETECTING a planted spur... (Parts A and B)"
        status: pass
      - kind: other
        ref: "mutation probe: widening H into a guard band makes Part B report bin 1379 at -176.45 dB instead of bin 390 at -60.0 dB"
        status: pass
    human_judgment: false
  - id: D3
    description: "The gate asserts its own rectangular-window leakage floor sits at least 10 dB below the tightest threshold this phase will pin, at all six grid rows"
    requirement: "TEST-03"
    verification:
      - kind: unit
        ref: "tests/test_vco_spectrum.cpp#vco spectrum: the DFT apparatus... (Part C, leakageDtDb <= -72.0)"
        status: pass
      - kind: other
        ref: "mutation probe: asserting the self-check against method one fails on 5 of 6 rows"
        status: pass
    human_judgment: false
  - id: D4
    description: "NaiveVcoCoreMirror reproduces the live forge::VcoCore bit-exactly over a 45-point grid at three sample rates, on non-trivial blocks"
    requirement: "TEST-03"
    verification:
      - kind: unit
        ref: "tests/test_vco_spectrum.cpp#vco spectrum: NaiveVcoCoreMirror is bit-identical to the live forge::VcoCore (D-08 baseline validity) - THIS CASE INVERTS IN PLAN 32-06"
        status: pass
      - kind: other
        ref: "mutation probe: a 1e-7f correction on the mirror's final line produces 290 to 1692 differing samples and fails the case"
        status: pass
    human_judgment: false
  - id: D5
    description: "The shipped Analog LFO is untouched: no frozen header edited, FROZEN.sha256 unmoved, src/AnalogLFO.cpp absent from the plan diff"
    verification:
      - kind: integration
        ref: "make guards (check_frozen.sh, check_canary.sh) + make strict + git diff --name-only HEAD~3 HEAD"
        status: pass
    human_judgment: false

# Metrics
duration: 20 min
completed: 2026-08-01
status: complete
---

# Phase 32 Plan 01: Spectral Apparatus and Naive Mirror Summary

**An exact-integer-cycle radix-2 DFT alias-floor apparatus proven by detecting a planted spur, a two-method bin-centre solver whose own leakage floor is asserted 20 dB tighter than the phase's tightest threshold, and a naive VcoCore mirror proven bit-identical over 45 grid points — all landed before one line of band-limiting exists (D-08).**

## Performance

- **Duration:** 20 min
- **Started:** 2026-08-01T22:08Z (approx.)
- **Completed:** 2026-08-01T22:28Z
- **Tasks:** 3
- **Files modified:** 2 (1 created, 1 modified)

## Accomplishments

- **The D-08 ordering is honoured.** The spectral gate, the classifier, the bin solver and the naive baseline all exist and are validated while the live core is still naive, so plan 32-03 can observe a genuine RED rather than writing a gate against already-passing code.
- **The classifier is validated by DETECTION, not by a green run.** A 1e-3 spur planted at non-harmonic bin 390 is found at exactly that bin and reported at −60.0 dB within 0.1 dB. Widening the harmonic set into a guard band — the construction D-10 explicitly rejects — makes the control report bin 1379 at −176.45 dB and fail.
- **The D-10 self-check is real and is met with 20 dB to spare**, but only because a second solver was added: the plan's mandated method could not meet it on 5 of the 6 grid rows (see Deviations).
- **The naive baseline is proven faithful, not assumed faithful.** 45 grid points, three sample rates, 4096 samples each, 0 differing samples — with non-vacuity asserted first (4096/4096 non-zero, block not constant) so silence cannot satisfy the claim.
- **Both Phase 32 test TUs were pre-registered in the guard before either existed**, disarming P-8 rather than discovering it at gate time.

## Task Commits

1. **Task 1: Pre-register both new VCO-side test TUs in check_includes.sh VCO_SIDE_ALLOW** — `9c4ff60` (chore)
2. **Task 2: The exact-integer-cycle spectral apparatus (D-10) with its own leakage self-check** — `00437e6` (test)
3. **Task 3: NaiveVcoCoreMirror, REQUIREd bit-identical to the live core** — `63cbcf0` (test)

## Files Created/Modified

- `tests/test_vco_spectrum.cpp` (new, ~930 lines) — `fftRadix2`, `aliasPeakDb`, `deltaPhaseForPitchCV`, `binCentredPitchCV`, `binCentredSampleTime`, `impliedLeakageDb`, `NaiveVcoCoreMirror`, and two `TEST_CASE`s.
- `tests/check_includes.sh` — two exact-path `VCO_SIDE_ALLOW` entries plus a 32-line rationale paragraph. Insertions only; zero deletions.

## Measurements Recorded

### The six `achievedBinError` values and their implied leakage

Method one is the plan's mandated solver (bisect `pitchCV`, `forge::VcoBlockDriver` unchanged). Method two nudges the injected `sampleTime`.

| Rate | K | ≈ note | `pitchCV` | method 1 (bins) | method 1 leakage | method 2 (bins) | method 2 leakage | `dt` deviation |
|------|---|--------|-----------|----------------:|-----------------:|----------------:|-----------------:|---------------:|
| 44100 | 97 | C6 | 1.997047424 | 2.73889e-04 | **−71.25 dB** | 1.5514e-06 | **−116.19 dB** | +2.808 ppm |
| 44100 | 195 | C7 | 3.004463196 | 7.97229e-04 | **−61.97 dB** | 5.3010e-07 | **−125.51 dB** | +4.091 ppm |
| 44100 | 389 | C8 | 4.000755310 | 1.48946e-03 | **−56.54 dB** | 8.3652e-06 | **−101.55 dB** | −3.850 ppm |
| 44100 | 777 | C9 | 4.998908997 | 2.86367e-04 | **−70.86 dB** | 2.5278e-05 | **−91.95 dB** | +0.401 ppm |
| 48000 | 357 | C8 | 3.999168158 | 1.54549e-03 | **−56.22 dB** | 1.3013e-05 | **−97.71 dB** | +4.366 ppm |
| 96000 | 179 | C8 | 4.003196716 | 2.27250e-04 | **−72.87 dB** | 7.1821e-06 | **−102.87 dB** | −1.310 ppm |

Method one reproduces `32-RESEARCH.md`'s predicted 2.3e-4 … 1.5e-3 bin envelope row for row. Method two reproduces its "≈1e-5 bins, ≈−100 dB at ≤ 5 ppm".

### Doctest case and assertion counts

| Point | Cases | Assertions |
|-------|------:|-----------:|
| Baseline (before this plan) | 81 | 2,618,053 |
| After Task 2 | 82 | 2,618,114 |
| After Task 3 (plan complete) | 83 | 2,618,339 |

Delta: **+2 cases, +286 assertions**, 0 failures. Selector counts: `-tc="vco spectrum: *"` → 2 cases after Task 3 (1 after Task 2); `-tc="vco spectrum: NaiveVcoCoreMirror*"` → exactly 1 case, 225 assertions.

### `VCO_SIDE_ALLOW` array size

**6 → 8.** Entries added: `"tests/test_vco_spectrum.cpp"`, `"tests/test_morph_blep.cpp"`. Existing six entries unreordered and unmodified; `git diff --stat` shows 32 insertions and 0 deletions.

## Decisions Made

1. **The D-10 self-check is asserted against method two, not method one.** See Deviations — this is the load-bearing decision of the plan.
2. **`forge::VcoBlockDriver` is not touched.** Method two leaves `pitchCV` and every core guard exactly where method one put them and drives the core through a separate local sample loop. The driver's unconditional `sampleTime`/`sampleRate` overwrite stays unconditional, and the R-2 / P-4 argument keeping the two drivers independent is untouched.
3. **The solver's replicated chain is verified against the live core, per row.** `deltaPhaseForPitchCV` mirrors `VcoCore::step`'s pitch/guard sequence by hand, so Part C drives the real `forge::VcoCore` one sample through the real harness and `REQUIRE`s `tel.freqHz` and the resulting increment to reproduce the prediction bit-exactly. A silent drift in the mirror would push every solved frequency off its bin centre and manufacture leakage indistinguishable from alias energy.
4. **TEST-03 is deliberately NOT marked complete.** TEST-03 reads "an alias-floor / spectral invariant asserts high-note aliasing stays below a defined threshold". This plan builds the measuring instrument; the thresholds and the assertions land in plans 32-03 and 32-07. Marking it here would reproduce the PANEL-03 false green Phase 30 recorded as deferred item 1, and continues the discipline Phase 31 applied five consecutive times.
5. **The pulse's DC is excluded from the metric (bin 0).** A 5 %-duty pulse legitimately carries DC; counting it would fail the pulse rows for a reason with nothing to do with band-limiting. The DC blocker is Phase 34's call (OUT-02).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 — Missing Critical] The plan's D-10 self-check is unsatisfiable with the solver the plan mandates; added `binCentredSampleTime`**

- **Found during:** Task 2 (the spectral apparatus), before any code was written — the achievable bin error was measured first precisely because the plan's Part C threshold depends on it.
- **Issue:** The plan requires `impliedLeakageDb` to sit "at least 10 dB below −62.0 dB", i.e. ≤ −72 dB, at **every** grid row, using `binCentredPitchCV` (bisect on `pitchCV`, harness unchanged). Measured leakage for that method across the six rows is −71.25 / −61.97 / −56.54 / −70.86 / −56.22 / −72.87 dB — **5 of the 6 rows fail**, including the C7 row that carries the tightest threshold the phase will assert. The plan's other stated bound (`achievedBinError < 2e-3` bins) is arithmetically the *same* assertion as "leakage < −53.98 dB", so the two requirements in the plan's own `<behavior>` block contradict each other by roughly 18 dB.
- **Root cause, measured:** it is not the bisection and not the ULP scan. `forge::exp2_taylor5`'s **output granularity** is the floor. At 44.1 kHz, K = 389, consecutive `pitchCV` floats either side of the solution produce frequencies of 4188.180175781 Hz and 4188.224121094 Hz and nothing in between — a 0.0439 Hz step, 90 float ULPs wide, 4.08e-3 bins wide. Six ULPs either side give bit-identical frequencies. This is D-10's named failure mode exactly ("the gate can pass by measuring `exp2_taylor5`'s output granularity rather than the DSP"), which is why the self-check exists in the first place.
- **Fix:** added `binCentredSampleTime(sr, pitchCV, K, ...)` — the second method **already specified** by `32-RESEARCH.md` § Validation Architecture and `32-VALIDATION.md`:103, whose switching rule reads "If any threshold ends up tighter than about −50 dB (only the sine rows do), switch that case to the second method." It leaves `pitchCV` and every core guard untouched and nudges the injected `sampleTime` to the nearest float of `(K/N)/freq`, then scans ±256 ULPs. Part C asserts the self-check against it, and additionally bounds the nudge at 5 ppm so a runaway nudge cannot compensate for a broken chain. Method one is still computed, still reported, and still bounded at `< 2e-3` bins, because plans 32-03 and 32-07 drive the shared harness with it on every row whose threshold is loose enough.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** measured −116.19 / −125.51 / −101.55 / −91.95 / −97.71 / −102.87 dB at ≤ 4.37 ppm deviation, clearing the −72 dB bar by 20 to 53 dB. The failure was **observed**, not argued: a one-shot mutation probe asserting the self-check against method one fails on 5 of 6 rows with exactly the leakage figures tabulated above.
- **Committed in:** `00437e6` (Task 2 commit)
- **Scope containment:** `forge::VcoBlockDriver` is byte-unchanged. No `src/` file was touched by this plan at all.

---

**Total deviations:** 1 auto-fixed (1 missing critical).
**Impact on plan:** the deviation makes the plan's own stated behavior achievable rather than expanding it — both `<behavior>` bullets now hold simultaneously, using the method the phase's research and validation documents already prescribed for this exact case. No scope creep: one added helper in a test TU, no production code, no guard weakening, no driver change.

## TDD Gate Compliance

Tasks 2 and 3 are marked `tdd="true"`, but their deliverable **is** a test file — there is no production code for a failing test to drive out, and a synthetic RED commit would have to fabricate a failure rather than observe one. RED was therefore established the way this repository establishes it everywhere else (`check_frozen.sh` [3/3], `check_includes.sh` [6/7], `check_canary.sh` [4/5], `test_vco_core.cpp` invariant 5): each detector was **observed failing** under a one-shot mutation probe, then the probe was reverted and the green state committed.

| Detector | Mutation applied | Observed RED |
|----------|------------------|--------------|
| Part B, the alias classifier | Widen `H` into a ±2-bin guard band (the construction D-10 rejects) | Reports bin **1379** at **−176.45 dB** instead of bin 390 at −60.0 dB; case fails |
| Part C, the D-10 self-check | Assert against method one instead of method two | Fails on **5 of 6** rows (−71.25 / −61.97 / −56.54 / −70.86 / −56.22 dB) |
| The mirror identity case | Add `1e-7f` to `NaiveVcoCoreMirror`'s returned sample (below one ULP at full scale) | **290 to 1692** differing samples across the grid; case fails |

Commit types are `test(32-01)` for both, which is the correct conventional type for a test-only change.

## Issues Encountered

None beyond the deviation above. `make test`, `make guards` and `make strict` were green on every task commit.

## Verification Results

| Check | Result |
|-------|--------|
| `make test` | **83 cases / 83 passed / 0 failed**, 2,618,339 assertions (from 81 / 2,618,053) |
| `make guards` | `guard suite: PASS` — `check_frozen.sh`, `check_includes.sh` (`[1/7]`: 29 LFO-side roots, 29 files opened across the closure, zero VCO includes), `check_canary.sh` |
| `make strict` | `strict C++11 gate: PASS` over all four `src/` TUs |
| `src/dsp/FROZEN.sha256` | unmoved (`git status --porcelain` empty) |
| `src/AnalogLFO.cpp` | absent from the plan diff (`git diff --name-only HEAD~3 HEAD` lists exactly `tests/check_includes.sh` and `tests/test_vco_spectrum.cpp`) |
| Containment | `grep -r 'NaiveVcoCoreMirror' src/` finds nothing |
| Comparator hygiene | `Approx` count in comment-stripped source: **0** |
| doctest impl macro | `DOCTEST_CONFIG_IMPLEMENT` count in the new TU: **0** |

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

**Ready for 32-02.**

What plan 32-03 inherits and should not re-derive:
- `binCentredPitchCV` for every row whose threshold is looser than about −50 dB, and `binCentredSampleTime` (plus its own local sample loop) for the sine rows, which are the only rows RESEARCH flags as needing it.
- `aliasPeakDb`'s `aliasRmsDbOut` is populated and unused so far — it is the diagnostic RESEARCH asks to be reported alongside the peak.
- The warm-up discard (one block of N samples before the measured block) is **not** implemented here, because nothing in this plan measures a driven spectrum. Plan 32-03 owns it, and `32-VALIDATION.md` pins it.
- `SpectrumCell`, listed among the phase's test-only symbols, is **not** in this plan's output and was not created.

Concerns to carry forward:
- **The `-62 dB` figure in the self-check is a placeholder pinned by this plan, not by measurement.** `32-VALIDATION.md`'s own table has a tighter row (sine C8 at −64 dB). Method two clears both by more than 20 dB, so nothing is at risk today, but plan 32-07 should re-read `kSelfCheckDb` against whatever thresholds it actually pins rather than inheriting −62.
- **The mirror-maintenance rule is now owed by two files**, `tests/test_vco_core.cpp`'s `DeliberatelyBrokenSharedStateCore` and `tests/test_vco_spectrum.cpp`'s `NaiveVcoCoreMirror`. Any future change to `forge::VcoCore::step`'s pitch/guard/accumulate sequence must be mirrored in both.
- **Plan 32-06 carries an explicit inversion obligation**, written into the tombstone case name and into a comment above its assertion.

## Self-Check: PASSED

- `tests/test_vco_spectrum.cpp` — FOUND on disk
- `tests/check_includes.sh` — FOUND on disk
- Commit `9c4ff60` — FOUND in `git log`
- Commit `00437e6` — FOUND in `git log`
- Commit `63cbcf0` — FOUND in `git log`
- All plan `<success_criteria>` re-run and green; all task `<acceptance_criteria>` re-run and green.

---
*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Completed: 2026-08-01*
