---
phase: 31-pitch-tuning-exponential-fm
plan: 05
subsystem: tests
tags: [doctest, test-gate, non-vacuous-coverage, libm-ground-truth, derived-bounds, cents, zero-crossing-estimator, nyquist-clamp, odr, anonymous-namespace]

# Dependency graph
requires:
  - phase: 31-pitch-tuning-exponential-fm
    plan: 01
    provides: "the `tests/test_vco_pitch.cpp` entry PRE-REGISTERED in check_includes.sh's VCO_SIDE_ALLOW, which is why `make guards` passed on the new file's FIRST run instead of exiting 1"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 03
    provides: "the four-term volt-domain pitch summation through exactly one exp2 call, the kVcoMaxPitchVolts bound, and the hand-off figures this plan's grid had to stay clear of"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 02
    provides: "kVcoNyquistGuardFrac settled at 0.495f — the constant both derived ceilings in this plan read symbolically"
  - phase: 30-vcocore-skeleton-module-registration
    provides: "tests/VcoBlockDriver.hpp (rate injection through the constructor, unconditional per-sample timing overwrite) and tests/test_vco_core.cpp's banner discipline, TEST_CASE/SUBCASE idiom and sub-sample crossing estimator"
provides:
  - "tests/test_vco_pitch.cpp — the phase's exit-gate translation unit, three invariants, zero build wiring (the test target globs tests/*.cpp)"
  - "TEST-02 primary tier: 1 V/oct tracking measured on the RETURNED SAMPLES against libm-in-double, across a derived-boundary sweep at 44.1/48/96 kHz, at a FIXED 0.05-cent tolerance — 20x tighter than PITCH-01's one cent"
  - "TEST-02 secondary tier: the same reference against the core's telemetry frequency over the octaves crossings cannot resolve, sharing the ONE tolerance constant, labelled the WEAKER tier in its name, its comment and the banner"
  - "invariant 1, the derived-boundary self-check: both per-rate ceilings computed from the forge:: constants, the lesser one binding, the grid provably keeping >= 0.05 V headroom, and the clamp volt restated by exponentiation as an independent cross-check"
  - "helpers for 31-06 and 31-07 to reuse in the SAME anonymous namespace: SAMPLE_RATES, kTrackingToleranceCents, kEstimatorMinSamplesPerCycle, kGridStepVolts, kPrimaryLowVolts, kSecondaryLowVolts, kExtremeLowVolts, kTelemetryBlockSamples, kGridHeadroomVolts, pitchBase(), estimateFreqRising(), expectedFreqHz(), centsError(), clampCeilingVolts(), estimatorCeilingVolts(), topTestVolts(), gridStepCount(), windowSamples()"
  - "SIX measured worst-case cents figures — two tiers x three rates — recorded in the source and here, with the volt at which each occurred, and neither contradictory research figure cited (D-18)"
  - "invariant numbers 4-9 reserved in the banner with their owning plans named, so 31-06 and 31-07 append without renumbering"
affects: [31-06, 31-07, 31-08, 31-09, 32-morph-blep, 33-hard-sync]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "A gate's expectation must come from a DIFFERENT implementation than the one under test: libm in double here, deliberately against the DECIMAL C4 reference rather than the header's float constant, so the reference is independent rather than half-derived"
    - "Ground truth INDEPENDENT, policy boundaries SYMBOLIC — two rules that look contradictory and are not. The expectation must not read the code under test; the sweep's limits must read nothing else"
    - "When a case's own comment claims coverage the prescribed grid provably cannot reach, ADD the coverage and ASSERT it lands — do not leave the claim standing. A false coverage comment is the same defect class as a false arithmetic comment"
    - "Assert a selector's MATCHED CASE COUNT, not just its exit status: a `-tc` filter that matches nothing also exits 0, so 'gate passed' and 'gate never ran' are otherwise indistinguishable"
    - "Harvest measured figures with a TEMPORARY print, write them into the source, then remove the print before committing — the numbers become permanent, the scaffolding does not"

key-files:
  created:
    - "tests/test_vco_pitch.cpp"
  modified: []

key-decisions:
  - "One derived band point per rate was ADDED to the secondary tier (Rule 2): the band between the two ceilings is 0.30743 V wide, narrower than the 0.5 V grid step, so no lattice point can reach the range the tier's own comment claimed to cover"
  - "The tolerance is ONE constant, shared by both tiers, fixed at 0.05 cents — a weaker tier with a looser number would be two gates pretending to be one"
  - "The apparatus limit is handled by BOUNDING THE SWEEP, never by widening the tolerance where the ruler gets coarse (D-20)"
  - "Three helpers landed with Task 2 rather than Task 1, as Task 1's action explicitly permits — an unused function in an anonymous namespace warns under -Wall -Wextra"
  - "PITCH-01 and TEST-02 marked COMPLETE — the first requirements this phase has claimed, and the first plan in the phase whose claim has a gate behind it"

patterns-established:
  - "The worst measured point in a tier being the point a deviation ADDED is direct evidence the deviation was load-bearing rather than decorative"
  - "Record the LATTICE-ONLY worst figure alongside the overall worst, so a reader can see what the added coverage would otherwise have hidden"

requirements-completed: [PITCH-01, TEST-02]

coverage:
  - id: D1
    description: "TEST-02 primary tier: 1 V/oct tracking measured on the RETURNED SAMPLES is inside a FIXED 0.05-cent tolerance at every point of a derived-boundary sweep at all three production rates"
    requirement: "PITCH-01"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"*v/oct tracking*\" — exit 0, 2 test cases matched, 603 assertions, 0 failed. Worst measured |cents|: 0.00967639 (44.1 kHz, +5.5 V), 0.00870829 (48 kHz, +6.0 V), 0.00239614 (96 kHz, +7.0 V) — 5.17x inside the tolerance and 103x inside the requirement"
        status: pass
    human_judgment: false
  - id: D2
    description: "The measurement is taken on the output, never on telemetry, on the primary tier (non-vacuity requirement 1 / D-19)"
    verification:
      - kind: integration
        ref: "grep -c 'tel.freqHz' tests/test_vco_pitch.cpp == 1, at line 686, and the secondary-tier TEST_CASE opens at line 648 — the single telemetry read is inside the secondary tier and nowhere else. The primary tier reads only the std::vector<float> the driver returned"
        status: pass
    human_judgment: false
  - id: D3
    description: "The ground truth is libm's base-2 exponential in double, so the polynomial is never compared against itself (non-vacuity requirement 2 / D-18)"
    verification:
      - kind: integration
        ref: "grep -c 'std::exp2' == 2 (one comment, one in expectedFreqHz); grep -c 'forge::exp2_taylor5' == 0. The reference multiplies the DECIMAL 261.6256, not forge::kVcoFreqC4, so the +0.0000685-cent float-representation offset is INSIDE the measurement"
        status: pass
    human_judgment: false
  - id: D4
    description: "Expectations are one octave apart across the grid — asserted mechanically, not left in prose (non-vacuity requirement 3)"
    verification:
      - kind: unit
        ref: "CHECK(expectedRatio > 1.4) on every consecutive pair as the sweep walks upward; 0.5 V is 2^0.5 = 1.41421x, so an accumulator that latched a single frequency can satisfy at most ONE of the 72 primary grid points"
        status: pass
    human_judgment: false
  - id: D5
    description: "REQUIRE(nUp >= 8) runs BEFORE any tolerance check, so a silent non-oscillation is a hard failure and the estimator's -1.0 sentinel can never reach the comparison (non-vacuity requirement 4)"
    verification:
      - kind: integration
        ref: "inside the primary-tier case (opens at line 481): REQUIRE(nUp >= 8) at line 563, first kTrackingToleranceCents at line 567. The cents value is not even COMPUTED until after the precondition"
        status: pass
    human_judgment: false
  - id: D6
    description: "The sweep's upper bound at each rate is min(clamp ceiling, estimator ceiling), both derived at runtime from the forge:: constants, and the grid keeps headroom below the binding one (D-20 / D-21)"
    verification:
      - kind: unit
        ref: "invariant 1 asserts finiteness, both-sided minimality, >= 0.05 V headroom, and an exponentiation-side restatement of the clamp volt within 2 % of the gate's own tolerance. Observed: estimator binds at all three rates, clamp at none; 44.1 kHz headroom is 0.075203 V"
        status: pass
      - kind: integration
        ref: "grep -c 'forge::kVcoNyquistGuardFrac' >= 1 and 'forge::kVcoFreqC4' >= 1; non-comment grep for a hardcoded Hz ceiling (2183x|2376x|4752x) == 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "The tolerance is FIXED — it does not widen with samples per cycle, pitch, morph or rate, and there is exactly one tolerance constant in the file (D-20)"
    verification:
      - kind: integration
        ref: "grep -cE 'constexpr double kTracking|constexpr double k.*Tolerance' == 1; the same constant is read by the primary tier (line 584), the secondary tier (line 711) and invariant 1 (line 468). Worst samples-per-cycle in the sweep is 2.86669 and the tolerance there is the same 0.05 as at 8.18 Hz"
        status: pass
    human_judgment: false
  - id: D8
    description: "This phase measured and recorded ITS OWN worst-case figures; neither prior-milestone research figure is cited"
    verification:
      - kind: integration
        ref: "grep -cE '0\\.0048|0\\.1 cent|1e-4|1e-6' == 0. Six figures harvested from actual runs of these cases, recorded in the banner, in the tolerance provenance comment and in invariant 3's comment, each with the volt where it occurred"
        status: pass
    human_judgment: false
  - id: D9
    description: "The gate's matched case count is asserted non-empty, not merely its exit status (T-31-20)"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"*v/oct tracking*\" reports '2 | 2 passed | 0 failed | 73 skipped'. After Task 2 alone it reported 1 matched case / 285 assertions; both counts are recorded below"
        status: pass
    human_judgment: false
  - id: D10
    description: "No one-definition-rule collision with tests/test_vco_core.cpp, which defines SAMPLE_RATES and estimateFreqRising at the same names (T-31-22)"
    verification:
      - kind: integration
        ref: "grep -c '^namespace {$' == 1 and grep -c '} // namespace' == 1 — every helper, constant and stand-in lives in ONE anonymous namespace. Both TUs link into the same binary and `make test` builds and runs clean"
        status: pass
    human_judgment: false
  - id: D11
    description: "The shipped LFO's golden replay leg is untouched: only the VCO driver is included, nothing is subclassed or aliased, and the six goldens replay inside every run"
    requirement: "guardrail"
    verification:
      - kind: integration
        ref: "grep -cE '(^|[^[:alnum:]_])BlockDriver\\.hpp' == 0 (the LFO golden driver is never included); grep -c 'template' == 0; git diff --name-only over all three commits == tests/test_vco_pitch.cpp alone; make guards PASS (frozen manifest + 6 LFO .f32 goldens + negative control)"
        status: pass
    human_judgment: false
  - id: D12
    description: "A non-finite telemetry value lands on the FAILING branch rather than passing silently (T-31-01)"
    verification:
      - kind: integration
        ref: "`if (!(telHz > 0.0)) telemetryUsable = false;` runs before the logarithm, and the tolerance comparison is written negated for the same reason — every comparison against a not-a-number is false, so both a NaN and a non-positive frequency fail"
        status: pass
    human_judgment: false

# Metrics
duration: 22min
completed: 2026-07-30
status: complete
---

# Phase 31 Plan 05: TEST-02, the Phase Exit Gate Summary

**The phase's exit gate now exists and is non-vacuous by construction: 1 V/oct tracking measured on the RETURNED SAMPLES against libm-in-double, across a sweep whose upper bound is the lesser of two ceilings both derived from the `forge::` constants at runtime, at a single FIXED 0.05-cent tolerance shared by both observation tiers — with a crossing-count precondition ahead of every comparison, consecutive expectations required to differ by more than 1.4x, and all six measured worst-case figures harvested by this phase and recorded in the source rather than inherited from research that contradicted itself.**

## Performance

- **Duration:** 22 min
- **Started:** 2026-07-30T11:56Z
- **Completed:** 2026-07-30T12:18Z
- **Tasks:** 3
- **Files created:** 1 (`tests/test_vco_pitch.cpp`, 715 lines)
- **Files modified:** 0

## Task Commits

1. **Task 1: banner, preamble, anonymous-namespace helpers and invariant 1, the derived-boundary self-check** — `94cc60c` (test)
2. **Task 2: invariant 2 — the TEST-02 primary tier on the returned samples, with this phase's measured figures** — `ec2e9b6` (test)
3. **Task 3: invariant 3 — the secondary telemetry tier, labelled the weaker one, and the banner closed** — `9eba54d` (test)

## Accomplishments

- **The gate measures the signal, not the arithmetic.** The primary tier reads the `std::vector<float>` the driver returned and estimates frequency from sub-sample-interpolated rising zero crossings. `tel.freqHz` appears **exactly once in the whole file** (line 686), inside the secondary-tier case — so the primary tier structurally *cannot* take the telemetry shortcut that would stay green through a dead accumulator.
- **The expectation comes from a different implementation than the one under test, and deliberately from the decimal reference.** `expectedFreqHz(v)` returns `261.6256 * std::exp2(v)` in double. It does **not** read `forge::kVcoFreqC4`, so the float constant's `+0.0000685`-cent representation offset is *inside* the measurement rather than cancelled out of it — which is exactly what `src/dsp/VcoCore.hpp` asks the gate to do, and warns against "fixing".
- **Both apparatus limits are derived, and the `min()` earned its keep in a way the numbers make visible.** The estimator ceiling binds at all three rates and the clamp binds at none — so a clamp-only bound would have admitted `+6.5 V` at 48 kHz, a point measuring roughly minus twelve cents on a perfectly correct oscillator. Invariant 1 asserts the minimality from both sides, asserts the grid keeps clear air, and restates the clamp volt by **exponentiation** rather than by logarithm so a sign error or transposed division cannot survive both derivations.
- **The tolerance is one number, and the apparatus limit is handled by bounding the sweep instead of by widening it.** `0.05` cents at 8.18 Hz and at 2.87 samples per cycle alike. One `constexpr double` in the file, read by invariant 1, the primary tier and the secondary tier. A weaker tier with a looser number would have been two gates pretending to be one.
- **The secondary tier is labelled weaker in its name, its comment and the banner — and its own measured figures are the argument for the label.** It is an order of magnitude *tighter* than the primary tier, because it is reading an arithmetic result back out. The comment says so, and says that reading a number back accurately is not the same fact as producing the right tone, and says that if the two tiers ever disagree the primary tier is the evidence.
- **The tier's stated coverage was made real rather than left as prose.** Its comment claims it covers the band above the estimator's ceiling and below the clamp's. Measured, that band is `0.30743 V` wide — **narrower than the 0.5 V grid step**, so no lattice point can ever land in it. One derived point per rate was added, and the case **asserts** it lands inside the band. That point turned out to be the worst-measuring point at every rate (see the deviation below).
- **The gate's matched case count is asserted, not just its exit status.** A `-tc` selector that matches nothing also exits 0. `-tc="*v/oct tracking*"` reports **2 matched cases / 603 assertions**, and the count after Task 2 alone (1 case / 285 assertions) is recorded too.
- **The regression floor held exactly, and grew by one case per task.** 72 → 73 → 74 → 75 cases, 0 failed throughout. The six shipped LFO `.f32` goldens replayed byte-identical inside every one of those runs.

---

## The derived ceiling table, as observed

Printed out of invariant 1 itself during Task 1 and recorded at the precision it was printed at. Nothing in this table is typed into code — every figure is computed at runtime from `forge::kVcoNyquistGuardFrac` and `forge::kVcoFreqC4`.

| Sample rate | Clamp ceiling | Estimator ceiling | **Binding limit** | Grid steps | Top grid point | Headroom |
|---|---|---|---|---|---|---|
| 44100 | +6.38263 V | **+6.0752 V** | **estimator** | 22 | +6.0 V | **0.075203 V** |
| 48000 | +6.50489 V | **+6.19746 V** | **estimator** | 22 | +6.0 V | 0.197459 V |
| 96000 | +7.50489 V | **+7.19746 V** | **estimator** | 24 | +7.0 V | 0.197459 V |

**The estimator binds at every rate and the clamp binds at none — and the `min()` is still required**, because the clamp is the *requirement-level* boundary and must be stated rather than left implicit. 44.1 kHz has only `0.075203 V` of clear air, 1.5x the `0.05 V` invariant 1 demands; that is the rate to look at first if a constant ever moves.

## The full grid of test points, per rate

**Primary tier (invariant 2) — 72 points total, all at `morph = 0.f`, `character = 0.f`:**

| Rate | Points | Volts |
|---|---|---|
| 44100 | **24** | `-7.0` (extreme, this rate only), then `-5.0, -4.5, -4.0, -3.5, -3.0, -2.5, -2.0, -1.5, -1.0, -0.5, 0.0, +0.5, +1.0, +1.5, +2.0, +2.5, +3.0, +3.5, +4.0, +4.5, +5.0, +5.5, +6.0` |
| 48000 | **23** | `-5.0` through `+6.0` in 0.5 V steps |
| 96000 | **25** | `-5.0` through `+7.0` in 0.5 V steps |

The `-7.0 V` point is roughly 2.04 Hz and takes a **7.83-second** window (345,240 samples at 44.1 kHz) under the sixteen-period window rule. It is prepended so the list stays ascending, which means the consecutive-expectation ratio check applies to it too — two octaves is a factor of four, comfortably past the required 1.4.

**Secondary tier (invariant 3) — 104 points total, same fixed inputs:**

| Rate | Points | Volts |
|---|---|---|
| 44100 | **34** | `-10.0` through `+6.0` in 0.5 V steps (33 lattice points), plus the derived band point `+6.20392 V` |
| 48000 | **34** | `-10.0` through `+6.0` (33), plus `+6.32617 V` |
| 96000 | **36** | `-10.0` through `+7.0` (35), plus `+7.32617 V` |

`-10.0 V` is about **0.2555 Hz** — resolving that from zero crossings would need a block over a minute long at every rate, which is precisely the range this tier exists for. Every point sits strictly below `clampCeilingVolts(sr) - 0.05 V`, so the clamp fires nowhere in this tier.

## The six measured worst-case figures

All harvested from actual runs of these cases during this plan, with a temporary print that was removed before each commit. **Neither of the two contradictory prior-milestone research figures is cited anywhere in the file** (verified: `grep -cE '0\.0048|0\.1 cent|1e-4|1e-6'` → `0`).

| Tier | Rate | Worst \|cents\| | Sign | At volts | Samples/cycle there |
|---|---|---|---|---|---|
| **PRIMARY** (returned samples) | 44100 | **0.00967639** | − | **+5.5 V** | 3.72472 |
| **PRIMARY** | 48000 | **0.00870829** | − | **+6.0 V** | 2.86669 |
| **PRIMARY** | 96000 | **0.00239614** | − | **+7.0 V** | 2.86669 |
| **SECONDARY** (telemetry, weaker) | 44100 | **0.0013924** | + | **+6.20392 V** | 2.28662 |
| **SECONDARY** | 48000 | **0.00123964** | − | **+6.32617 V** | 2.28662 |
| **SECONDARY** | 96000 | **0.00123964** | − | **+7.32617 V** | 2.28662 |

**Every primary-tier worst is NEGATIVE in sign** — the measured frequency sits very slightly under the reference at the top of each rate's range.

**The two margins.** The fixed `0.05`-cent tolerance is **5.17x above** the worst measurement anywhere (`0.00967639`), so it is not brittle; and it is **20x under** the one cent PITCH-01 requires. The worst measurement is therefore **103x inside the requirement**.

**Secondary tier, lattice-only:** `0.000164011` cents at `-9.5 V` at all three rates. That figure is recorded deliberately — it is what the tier would have reported without the added band point, an order of magnitude smaller and hiding the band entirely.

## Test counts, before and after

| Run | Cases | Assertions | Failed |
|---|---|---|---|
| Pre-plan baseline (`14446ab`) | 72 | 2,616,112 | 0 |
| After Task 1 (`94cc60c`) | **73** | 2,616,142 | 0 |
| After Task 2 (`ec2e9b6`) | **74** | 2,616,427 | 0 |
| After Task 3 (`9eba54d`) | **75** | **2,616,745** | 0 |
| Phase regression floor | 72 | 2,616,112 | 0 |

Exactly **+1 case per task**, as the plan required, and **0 failed** at every step. No existing case moved: the net addition is 633 assertions across three new cases.

**The gate selector, both times it mattered:**

| Invocation | Matched cases | Assertions | Exit |
|---|---|---|---|
| `-tc="*v/oct tracking*"` after Task 2 | **1** | 285 | 0 |
| `-tc="*v/oct tracking*"` after Task 3 | **2** | **603** | 0 |
| `-tc="*PRIMARY TIER*"` (spot check) | 1 | 285 | 0 |

## Gate results

| Gate | Required | Observed |
|---|---|---|
| `make test` | exit 0, 0 failed, +1 case per task | **75 / 2,616,745 / 0** |
| Compiler warnings from this file | zero under `-Wall -Wextra` | **zero** — clean rebuild after every task, `grep -i warning` over full build output empty |
| `./build-test/test -tc="*v/oct tracking*"` | exit 0 **and** non-empty match | **exit 0, 2 cases, 603 assertions** |
| `bash tests/check_includes.sh` | exit 0 on the file's FIRST run | **exit 0** — 31-01's pre-registration did its job |
| `make guards` | exit 0, `guard suite: PASS` | **PASS** (see the transient noted in Deviations) |
| `make strict` | exit 0, unchanged from 31-04 | **PASS** — `strict C++11 gate: PASS`, unchanged (this plan touches no `src/` file) |
| Six LFO `.f32` goldens | byte-identical | replayed inside **every** `make test` run above |
| `git diff --name-only` over all 3 commits | `tests/test_vco_pitch.cpp` alone | **`tests/test_vco_pitch.cpp`** |

## Non-vacuity requirements — which are this plan's, and which are not

The validation contract names **six**. Four are this plan's and all four are asserted rather than argued:

| # | Requirement | Owner | Status |
|---|---|---|---|
| 1 | Measure the OUTPUT, not telemetry, on the primary tier | **this plan** | ✅ single `tel.freqHz` read in the file, inside the secondary tier only |
| 2 | Ground truth from libm in double | **this plan** | ✅ `261.6256 * std::exp2(v)`; zero references to the polynomial under test |
| 3 | Expectations one octave apart | **this plan** | ✅ `CHECK(expectedRatio > 1.4)` on every consecutive pair |
| 4 | `REQUIRE(nUp >= 8)` before any tolerance check | **this plan** | ✅ line 563 vs the first tolerance read at line 567, inside the same case |
| 5 | FM multiplicative negative control | **31-06** | ⬜ not implemented here and **not claimed** here |
| 6 | Clamp-boundary case proving the clamp FIRES | **31-07** | ⬜ not implemented here and **not claimed** here |

## Milestone guardrail compliance

- `git diff --name-only 94cc60c~1 9eba54d` = **`tests/test_vco_pitch.cpp`**. One file, three commits, nothing else.
- **No frozen header in the diff.** `src/dsp/LfoCore.hpp`, `src/dsp/Waveshape.hpp`, `src/dsp/RackCompat.hpp`, `src/dsp/MathConst.hpp` and `src/dsp/FROZEN.sha256` are all absent, verified by grep over the full three-commit diff (`0` matches). `check_frozen.sh` PASSes and the six LFO goldens replay byte-identical.
- **`src/AnalogLFO.cpp` absent from the diff.** So is every other `src/` file — `make strict` is therefore unchanged from the 31-04 result by construction, not by luck.
- **The LFO's golden replay driver is never included.** Only `tests/VcoBlockDriver.hpp`. Nothing is subclassed, aliased, or parameterised over a type: `grep -c 'template'` is `0` for the whole file.
- **`tests/VcoBlockDriver.hpp`, `tests/test_vco_core.cpp`, `tests/main.cpp` and `tests/check_includes.sh` all unmodified.** Zero build wiring was needed — `TEST_SOURCES := $(wildcard tests/*.cpp)` picked the new file up.
- **No deletions in any commit** (`git diff --diff-filter=D` empty for all three) and **no untracked residue** (`git status --porcelain --untracked-files=all` empty after each).
- **Zero registry packages** (T-31-SC): doctest is vendored in-tree.

## Decisions Made

1. **One derived band point per rate was added to the secondary tier.** See Deviations — the tier's own comment claimed coverage the prescribed lattice provably could not reach.
2. **The tolerance is a single constant, shared by both tiers.** `grep -cE 'constexpr double kTracking|constexpr double k.*Tolerance'` returns `1`. A looser number on the weaker tier would have been two gates wearing one name, and the weaker tier measured *tighter* anyway, so a second constant would have bought nothing at the cost of an argument nobody could win later.
3. **Invariant 1 exercises the tolerance constant rather than a bare epsilon.** Its cross-check bar is `kTrackingToleranceCents * 0.02` — the apparatus's own restatement error has to be a small fraction of the number the gate spends. This also keeps the constant referenced in the same task that declares it, which matters because an unused constant in an anonymous namespace is a warning.
4. **The minimality of the binding limit is asserted from both sides rather than by recomputing the minimum.** `binding <= clampV`, `binding <= estV`, and `binding` equals one of them. Recomputing `std::fmin` and comparing would only have echoed the helper's implementation back at itself — the same self-comparison shape the whole file exists to avoid, one level up.
5. **The grid is walked by integer step index, never by accumulating a double.** `low + 0.5 * (double)k` is exact in binary; `v += 0.5` in a loop condition is a fencepost waiting to move the top of the sweep.
6. **The `-7.0 V` extreme point is prepended rather than appended.** That keeps the point list ascending, so the consecutive-expectation ratio check covers it instead of having to skip it. Its own factor is four.
7. **The primary tier re-states `base.morph = 0.f` and `base.character = 0.f` even though `pitchBase()` already zeroes them.** The base helper is deliberately neutral; the case deliberately restates what it depends on. Both halves of that discipline exist so no grid point can inherit a value it did not name.
8. **The secondary tier reads only 8 samples per point.** Enough, because the telemetry field is written on every `step(...)` — and taking only a handful makes it obvious in the code that this tier is not measuring a signal, merely reading back a field.
9. **PITCH-01 and TEST-02 marked complete.** See below; this is the first requirement claim this phase has made, and the reasoning is recorded because the three preceding plans each declined.

### Why PITCH-01 and TEST-02 are marked complete, when 31-01, 31-02 and 31-03 each declined

The three preceding plans declined to mark anything, every time for the same reason: they landed arithmetic or structure with **no behavioral gate behind the claim**. 31-03's hand-off said so explicitly and forecast "confirm `PITCH-01` after 31-05" and "`TEST-02` after 31-05/06/07".

**This plan is that gate.** Reading the requirement texts individually:

- **PITCH-01** — *"V/Oct input tracks 1V/octave across the audio range (C4 = 0V reference), reusing `forge::exp2_taylor5` for the exponential pitch law (no new exponential; shared-core bit-identity preserved)."* The tracking claim is now measured on the output at 103x inside its own stated bar, across the audio range at three rates; the reuse-and-no-new-exponential clause was proven by construction in 31-03 (exactly one `exp2_taylor5` call, `std::exp2`/`std::pow` absent from `src/` as code) and the goldens still replay byte-identical. The jack has existed since Phase 30 and 31-04 landed the shell. Nothing in the requirement text is now unevidenced.
- **TEST-02** — *"V/Oct tracking accuracy is asserted (< 1 cent error) across the pitch range."* `31-VALIDATION.md` defines TEST-02 as **the union of the PITCH-01 rows** — the output-derived primary tier and the telemetry secondary tier — and both are green. TEST-02 is scoped to V/Oct *tracking*; COARSE, FINE and FM are PITCH-02/03 and FM-01/02/03, which have their own IDs and are **not** claimed here.

31-03's forecast of "TEST-02 after 31-05/06/07" was a conservative forward guess made before this plan existed, and it is superseded by the requirement's own text plus the validation contract's own definition. Marking it now is not a false green: the assertion exists, it runs on every `make test`, its matched case count is non-zero and recorded, and its four owned non-vacuity controls are each asserted. **Still pending and correctly so:** `PITCH-02`, `PITCH-03`, `FM-01`, `FM-02`, `FM-03` (31-06), `PITCH-04` (31-07), `PITCH-05` (its non-regression obligation is discharged but its own gate is the existing `phaseInRange` case, confirmed at the phase gate).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 — Missing Critical] The secondary tier's stated coverage of the inter-ceiling band was UNREACHABLE on the prescribed 0.5 V lattice**

- **Found during:** Task 3, while writing the case's opening comment.
- **Issue:** The plan and D-19 both give this tier two jobs: the very low end, **and** "the narrow band between the estimator's ceiling and the clamp's". The prescribed grid is `-10.0 V` upward in **0.5 V steps**. Measured, that band is **0.30743 V wide** at every rate (`0.25743 V` after `kGridHeadroomVolts`) — **narrower than one grid step**. So no lattice point can ever land inside it, at any rate, ever. Writing the comment as prescribed would have shipped a **coverage claim the grid provably could not deliver** — the same false-comment class plan 30-08 existed to remove and that 31-03 hit twice, one level up: a claim about what a test covers rather than about what a number is.
- **Fix:** one **derived** extra point per rate, the midpoint of `[estimatorCeilingVolts(sr), clampCeilingVolts(sr) - kGridHeadroomVolts]`, both ends read from the existing helpers so it moves with the constants. Two `REQUIRE`s assert the point really does land strictly inside the band, so the coverage claim is **asserted rather than assumed**. No new constant, no hardcoded volt.
- **Evidence the fix was load-bearing rather than tidy:** the added point is the **worst-measuring point at every one of the three rates** (`0.0013924` / `0.00123964` / `0.00123964` cents) and sits at **2.28662 samples per cycle** — inside the region the primary tier's derived ceiling deliberately excludes. Restricted to the lattice alone the tier's worst figure is `0.000164011` cents, an order of magnitude smaller. The band was real, it was invisible, and it is now the tier's tightest-measured evidence.
- **Files modified:** `tests/test_vco_pitch.cpp`. **Committed in:** `9eba54d`.

**2. [Rule 3 — Blocking] doctest rejects `||` inside an assertion macro**

- **Found during:** Task 1's first build.
- **Issue:** `CHECK((binding == clampV) || (binding == estV));` fails to **compile**: doctest's expression decomposition deliberately forbids `&&`/`||` inside its macros — `static assertion failed: Expression Too Complex Please Rewrite As Binary Comparison!` at `tests/doctest.h:1543`.
- **Fix:** hoisted the disjunction into a named `const bool bindingIsOneOfTheTwo` and asserted that, with a comment recording *why* the shape is what it is so the next person does not "simplify" it back into the macro. No assertion was weakened — the same three facts are checked.
- **Files modified:** `tests/test_vco_pitch.cpp`. **Committed in:** `94cc60c`.

### Prescribed but worth recording

**3. Three helpers landed with Task 2 rather than Task 1**

`pitchBase()`, `estimateFreqRising()` and `windowSamples()` are listed in Task 1's helper block, but invariant 1 uses none of them — and Task 1's own action text anticipates exactly this: *"an unused function in an anonymous namespace produces a compiler warning under this target's flags, so either exercise it here or land it with the case that first needs it."* They landed with invariant 2, the case that first needs them. The alternative was three `-Wunused-function` warnings against Task 1's "zero warnings" criterion. `kSecondaryLowVolts` and `kTelemetryBlockSamples` landed with Task 3 for the same reason.

### Verification-command notes (no code impact)

**4. Task 1 criterion `grep -c 'BlockDriver.hpp' tests/test_vco_pitch.cpp` returns `1`, not `0`.**
The one hit is the file's own include line, `#include "VcoBlockDriver.hpp"` — which **contains** `BlockDriver.hpp` as a substring, so the criterion as literally written cannot ever return `0` for a file that includes the VCO driver at all. This is the same criterion-filter artifact class 31-02 and 31-03 each documented (a grep whose mechanism is wider than the prose it encodes — Phase 30's standing gate-design lesson). The **substantive** claim is true and verified with a boundary-anchored pattern:

```
$ grep -cE '(^|[^[:alnum:]_])BlockDriver\.hpp' tests/test_vco_pitch.cpp
0
```

The LFO's golden replay driver is included nowhere, and the filename is deliberately not mentioned in any comment either, so the count stays at exactly one — the include line.

**5. Task 2's ordering criterion needs case-scoping to evaluate, and it holds.**
`grep -n 'kTrackingToleranceCents'` file-wide reports the constant's **declaration** at line 181, which is naturally before `REQUIRE(nUp >= 8)` at line 563. The criterion says *"inside the same case"*, so it was evaluated scoped from the primary-tier case's first line (481):

```
$ awk 'NR>=481 && /REQUIRE\(nUp >= 8\)/ {print NR; exit}'      -> 563
$ awk 'NR>=481 && /kTrackingToleranceCents/  {print NR; exit}' -> 567
```

563 < 567. The precondition precedes the tolerance check, and the cents value is not even **computed** until after it.

**6. One `make guards` run failed transiently, in a fixture that does not contain this file.**
The first `make guards` after Task 3 exited 1 with a single FAIL in `check_canary.sh [4/5]`'s negative control: its `-std=c++17` fixture **sanity** compile reported `src/dsp/RackCompat.hpp:14:10: fatal error: 'cmath' file not found`. That invocation is `${CXX} -std=c++17 -fsyntax-only -I src <fixture>` on a synthetic TU whose only include is `dsp/VcoCore.hpp` — **`tests/test_vco_pitch.cpp` is not in that translation unit at all.** Not reproducible:

```
$ for i in 1 2 3 4 5; do make guards >/dev/null 2>&1; echo $?; done
0 0 0 0 0                      # zero FAIL lines in every log
$ printf '#include <cmath>\nint main(){return 0;}' > /tmp/cm.cpp && c++ -std=c++17 -fsyntax-only /tmp/cm.cpp
(clean)
```

A toolchain/SDK-resolution hiccup on this machine, recorded rather than swallowed because a one-off red in a guard suite is exactly the kind of thing that gets dismissed and then turns out to matter. Nothing was changed to make it pass; it passed on its own five times running.

---

**Total deviations:** 2 auto-fixed (1 Rule 2 comment-truth-about-coverage, 1 Rule 3 build blocker), 1 prescribed-but-recorded, 3 verification-command notes.
**Impact on plan:** No scope creep and no weakened assertion. The file is the three invariants the plan specified, plus one derived grid point per rate that the plan's own stated coverage required.

## Issues Encountered

- **None blocking.**
- **`make test` runtime grew from under a second to ~12 s.** The primary tier drives roughly 2.3 million `step(...)` calls — the low end of the grid needs multi-second windows (`-7.0 V` alone is 345,240 samples) and the sixteen-period rule is what makes those windows honest. Still comfortably inside "run it after every task commit". Worth knowing before anyone assumes a slow suite means a hang.
- **Note for later plans (fourth confirmation):** `gsd-tools query state.record-metric` / `state.add-decision` take **named flags**, not the positional arguments the `execute-plan.md` workflow shows. Carried forward from 31-01, 31-02 and 31-03, confirmed again here.

## User Setup Required

None. No external service configuration, and **zero registry packages** this plan — doctest is vendored in-tree and the test target globs `tests/*.cpp`, so the new file needed no build wiring at all.

## Next Phase Readiness

- **Ready for 31-06.** Invariant numbers **4, 5, 6 and 7** are reserved in the banner with 31-06 named against each, so appending needs no renumbering. Every helper is already in the one anonymous namespace and can be reused directly: `pitchBase()` already assigns `coarse`, `fine`, `fmVolts`, `fmAtten` and `fmConnected` explicitly, so a COARSE/FINE/FM case only overrides what it means to test. Three specific hand-offs:
  - **The FM-03 negative control must be driven through the SAME helper the positive case uses**, or it proves nothing — model it on `DeliberatelyBrokenSharedStateCore` (`tests/test_vco_core.cpp:316-366`), anonymous namespace, test TU only, exactly one deliberate defect (`freq *= ...` after the pitch resolves).
  - **Do not introduce a second tolerance constant.** `grep -cE 'constexpr double k.*Tolerance'` returning `1` is an acceptance criterion of this plan and should stay true. A bit-exact identity case wants a direct `!=` comparison anyway, never `doctest::Approx`.
  - **`CHECK` cannot contain `&&` or `||`** — hoist any compound condition into a named bool (see deviation 2). This will bite immediately when writing the bipolarity case.
- **Ready for 31-07.** Invariant numbers **8 and 9** are reserved. `clampCeilingVolts(sr)` is the helper the clamp-fires case wants — drive **just above** it and expect the frequency pinned at `kVcoNyquistGuardFrac * sampleRate` while the output keeps oscillating. `<limits>` is already included for the hostile grid. **Do not write a `tel.freqHz == 0` assertion for hostile pitch** — 31-03 moved that value to `1.41828e-17` by design (D-13), and it is the one behavioral number this phase has changed.
- **The gate will need re-measuring if any of three constants moves.** `kVcoNyquistGuardFrac`, `kVcoFreqC4` or `kEstimatorMinSamplesPerCycle`. Invariant 1 is the tripwire and will fire first, in a case whose name says the problem is the ruler; the six figures in the banner and the tolerance provenance comment are then stale and must be re-harvested, not adjusted by hand. 44.1 kHz has the least headroom (`0.075203 V`) and will trip first.
- **32-morph-blep inherits a warning, not a gift.** This file's primary tier runs at `morph = 0` on purpose, and the measured reason is recorded: the estimator is roughly a hundred times worse through the morphed shapes' kinked crossings. When Phase 32 lands band-limiting and wants a morph-robustness pass, it belongs in a **separate case at the same fixed tolerance** and must never loosen this one. The banner says so.
- **No blockers.**

## Self-Check: PASSED

- `tests/test_vco_pitch.cpp` — FOUND (715 lines)
- `.planning/phases/31-pitch-tuning-exponential-fm/31-05-SUMMARY.md` — FOUND
- Commit `94cc60c` — FOUND
- Commit `ec2e9b6` — FOUND
- Commit `9eba54d` — FOUND
- No file deletions in any of the three commits (`git diff --diff-filter=D --name-only` empty for each)
- No untracked residue after any commit (`git status --porcelain --untracked-files=all` empty)
- No frozen header, no `FROZEN.sha256`, no `src/AnalogLFO.cpp` and no `src/` file at all in the three-commit diff
- Every figure in this summary was read out of an actual run's output, not computed by hand

---
*Phase: 31-pitch-tuning-exponential-fm*
*Completed: 2026-07-30*
