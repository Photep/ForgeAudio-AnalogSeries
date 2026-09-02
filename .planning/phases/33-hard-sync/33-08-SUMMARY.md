---
phase: 33-hard-sync
plan: 08
subsystem: tests
tags: [hard-sync, sc-3, d-10, sync-02-declined, time-domain-instrument, measured-envelope, anti-circularity, mutation-probe, output-tier-withheld, correction-worse-than-none, apple-clang-only]

# Dependency graph
requires:
  - phase: 33-hard-sync
    plan: 04
    provides: "makeMasterSaw / makeMasterSawBandLimited, SyncTrace / driveTraced, the accumulate-then-assert idiom at grid scale, and the g-versus-f measurement this plan's band-limited half rests on"
  - phase: 33-hard-sync
    plan: 06
    provides: "THE SEAM ITSELF (forge::MorphBlep::addPastStep called from forge::VcoCore), tel.syncCorrection populated, and the reconstruction relationship WITH ITS MEASURED ONE-ULP ERROR BAR"
  - phase: 33-hard-sync
    plan: 07
    provides: "the written, measured statement that the spectral instrument is structurally blind to a single-sample spike (0.0 dB), which is why this plan's instrument is time-domain; and the ratio-5.50 finding this plan reproduces independently"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "kHostileBoundV / kMusicalBoundV, the two-tier nesting rule, the withhold-and-assert precedent (invariant 6) and the cushioned-exercise-floor rule (scenario five)"
provides:
  - "THE TIME-DOMAIN INSTRUMENT SC-3 HAS NEVER HAD: a 420-cell sync sweep measuring the per-sample step across every reset, on reset samples identified from tel.syncFired"
  - "kSyncResetDeltaBoundV = 9.90 V, pinned from a TWO-SIDED interval — at or above the measured worst AND strictly below what a seam-free core measures — so the bound is falsifiable rather than analytic"
  - "kSyncAntiCircularityMarginV = 0.04 V asserted on 277 cells of a population stated on a PHYSICAL criterion above its own enumeration, both legs from ONE pass, consulting no pinned number from anywhere else in the suite"
  - "Three mutation probes, each failing a STATED population exactly: quarter deposit 69/69, half deposit 0/0, inverted sign 277/277"
  - "THE MEASURED FACT THAT THE CORRECTION IS NOT A UNIFORM IMPROVEMENT, now asserted in a SECOND instrument: 56 of 420 cells negative, worst -0.246492 V AT RATIO 5.50"
  - "The sync scenario's output-tier row, re-derived rather than inherited: 8.218569 V, the largest envelope anywhere in this suite; outer tier asserted, tighter tier WITHHELD with the withholding asserted PER RATE"
affects: [33-09, 33-10, 33-11, 33-12]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Pinning a bound from a TWO-SIDED interval — at or above the measurement, and strictly below what the un-corrected mechanism measures — so the bound is provably able to fail without a source mutation"
    - "Rejecting the free analytic bound IN AN ASSERTION (`bound < 2 * outer tier`) rather than only in prose, so the rejection cannot rot"
    - "Choosing a classifier threshold inside a MEASURED EMPTY GAP in the classified quantity's distribution, and asserting both edges of the gap, so the population provably does not depend on the threshold's value"
    - "A mutation probe applied to the RECONSTRUCTION rather than to the shipped header, with the licence to do so (pure additivity, nothing owed forward) itself asserted per cell"
    - "A probe magnitude that fires NOTHING, asserted alongside one that fires a subset, so 'discriminating' is evidenced rather than claimed"
    - "Appending a re-measurement beside a falsified figure instead of overwriting it, when the original paragraph is still a correct record of what it measured and why"

key-files:
  created: []
  modified:
    - tests/test_vco_core.cpp

key-decisions:
  - "THE BOUND IS PINNED FROM A TWO-SIDED INTERVAL, not rounded outward from the measurement alone. Constraint (b) — strictly below the 10.000000 V a seam-free core measures — is what stops it becoming the vacuous analytic bound"
  - "THE MEASURED ENVELOPE IS ONLY 2 % TIGHTER THAN THE NAIVE FULL-SCALE STEP, and that is reported as the headline finding rather than buried: the seam buys 0.206 V of the worst cell's 10 V step"
  - "The gated population's criterion is max |tel.syncJump| >= 0.75 pre-multiply units, chosen inside a MEASURED empty gap 0.639500..0.921976 and with both gap edges asserted"
  - "The margin is pinned at 0.04 V — the measured minimum 0.095148 V rounded outward to 0.09 and then HALVED, a stated 2.4x cushion spent on the Apple-clang-only exposure"
  - "The negative-margin COUNT is deliberately NOT pinned as an equality (20 of the 56 are within a millionth of a volt of zero); the worst value and its ratio are pinned instead"
  - "The tighter output tier is WITHHELD for sync and the withholding is asserted PER RATE — the per-rate form was checked first, because invariant 6 records exactly where that form would be red on correct behaviour"
  - "SYNC-02 is DECLINED — the ELEVENTH consecutive decline. The gap 33-06 and 33-07 named is CLOSED: the instrument exists. What the instrument REPORTS is the new reason"

requirements-completed: []  # SYNC-02 is in this plan's frontmatter and is DECLINED — see Decisions #7

coverage:
  - id: D1
    description: "The per-sample step across a reset is bounded by a measured envelope, on reset samples identified from telemetry"
    requirement: "SYNC-02"  # contributes to; not completed by this plan
    verification:
      - kind: unit
        ref: "\"(SC-3 / D-10) the per-sample step...\" — 420 cells, 1,720,320 core steps, 13,230 resets, worst 9.793601 V against a pinned 9.90 V; reset samples read from tel.syncFired (tests/test_vco_core.cpp:3816) and the recorder's linkage to the live core asserted first"
        status: pass
      - kind: other
        ref: "PROVED ABLE TO FAIL: the seam call commented out of src/dsp/VcoCore.hpp reds this case at 4 of 32 assertions (`10 <= 9.9`, grid-wide and at all three rates) and the suite at 2 cases / 416 assertions. Restored and re-verified green"
        status: pass
    human_judgment: false
  - id: D2
    description: "The bound is not the analytic excursion bound, and that is asserted rather than argued"
    verification:
      - kind: unit
        ref: "CHECK(kSyncResetDeltaBoundV < kSyncAnalyticDeltaBoundV) where the analytic bound is written as 2 * kHostileBoundV = 20.0 V; plus CHECK(gridWorstWithheld > kSyncResetDeltaBoundV) — the withheld leg measures 10.000000 V, ABOVE the bound"
        status: pass
    human_judgment: false
  - id: D3
    description: "The uncorrected reset delta minus the corrected one is at or above a pinned margin on every cell of a stated population, consulting no pinned number from elsewhere"
    requirement: "SYNC-02"
    verification:
      - kind: unit
        ref: "\"(SC-3 / D-10) the corrected reset delta...\" lines 3979-4303 — 277 gated cells, minimum margin 0.095148 V against a pinned 0.04 V. grep over that range for kSyncResetDeltaBoundV / kSyncExerciseFloorV / kHostileBoundV / kMusicalBoundV / thresholdDb / measuredDb returns 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "Both legs come from the SAME pass of the SAME core"
    verification:
      - kind: unit
        ref: "sweepSyncDeltaGrid records outPrev/outNow and corrPrev/corrNow per reset from ONE drive; worstResetDeltaAt(c, k) derives every leg from those four numbers. No second core, no mirror, no flag in the shipped body, no second pass"
        status: pass
    human_judgment: false
  - id: D5
    description: "The restriction is stated on a physical criterion ABOVE the enumeration, the population is asserted exactly, and the criterion is proved not to be a tuned value"
    verification:
      - kind: unit
        ref: "criterion at tests/test_vco_core.cpp:4016 and its constant at 4101; first enumeration at 4117; assertion at 4130. nGated == 277 and nUngated == 143, both exact. The MEASURED empty gap 0.639500..0.921976 is asserted at both edges, so any floor in a 1.44x window selects the same 277"
        status: pass
    human_judgment: false
  - id: D6
    description: "A discriminating mutation probe fails a STATED population exactly"
    verification:
      - kind: unit
        ref: "STATED before running, DERIVED from the additivity identity (measured departure exactly 0.000e+00 over 277 cells): quarter deposit 69, half deposit 0, inverted sign 277. OBSERVED 69 / 0 / 277. All three equal"
        status: pass
    human_judgment: false
  - id: D7
    description: "The output tiers are re-derived for sync; the tighter tier is withheld and the withholding is asserted; neither constant widened"
    verification:
      - kind: unit
        ref: "sync scenario worst 8.218569 V grid-wide, 8.218569 / 8.216589 / 8.216589 per rate. kHostileBoundV asserted grid-wide and per rate; kMusicalBoundV withheld with an exercise floor of 6.70 V asserted grid-wide AND per rate, plus CHECK(kSyncExerciseFloorV > kMusicalBoundV)"
        status: pass
      - kind: unit
        ref: "`git diff HEAD~3 -U0 tests/test_vco_core.cpp | grep -E '^[-+].*(kHostileBoundV = |kMusicalBoundV = )'` returns nothing; the whole-plan diff is 970 insertions and ZERO deletions"
        status: pass
    human_judgment: false
  - id: D8
    description: "Nothing shipped moved"
    verification:
      - kind: unit
        ref: "make test 108 cases / 2,632,235 assertions 0 failures; six LFO goldens byte-identical (9 cases / 49,188); check_frozen.sh PASS; make strict and make guards exit 0; git diff --name-only across all three commits is tests/test_vco_core.cpp alone; src/AnalogLFO.cpp absent"
        status: pass
    human_judgment: false

# Metrics
duration: 86min
completed: 2026-09-02
status: complete
---

# Phase 33 Plan 08: SC-3 Gets an Instrument That Can See the Artefact Summary

**SC-3 now has a time-domain instrument, and the first thing it reports is uncomfortable: the worst per-sample step across a hard-sync reset is 9.793601 V, against the 10.000000 V a core with NO sync correction at all measures on the same grid. The shipped sync BLEP removes about two percent of the worst-case step. That number is the headline of this plan, it is pinned into a two-sided bound so it cannot be widened into comfort, and it is why SYNC-02 is declined for the eleventh time — for a reason that has changed shape again.**

The gap plans 33-06 and 33-07 both named — *"'click-free' has no instrument"* — is **CLOSED**. The instrument exists, it is asserted on every run, it is proved able to fail by removing the seam from the shipped header, and it consults no spectral number anywhere. What replaces that gap is not an absence but a **measurement**, and the measurement does not support the word "click-free".

## Performance

- **Duration:** 86 min
- **Tasks:** 3 of 3
- **Files modified:** 1

## Task Commits

| # | Task | Commit | Type |
|---|---|---|---|
| 1 | The measured per-sample reset delta bound (SC-3 / D-10) | `8934703` | test |
| 2 | The anti-circularity margin and three discriminating probes (SC-3 / D-10) | `b2573b0` | test |
| 3 | Re-derive the output tiers for sync, and withhold rather than widen (SC-3) | `603b226` | test |

## Files Created/Modified

- `tests/test_vco_core.cpp` — two new `TEST_CASE`s (invariants 10 and 11), the 420-cell sweep and its helpers, two appended `SyncTrace` fields, the provenance block's sync row, and one re-measurement appended beside a figure the seam falsified. **No other file in the repository was touched by any of the three commits.**

## Suite Totals, Before and After

| | Test cases | Assertions |
|---|---|---|
| Before plan 33-08 (33-07's recorded totals) | 106 | 2,631,627 |
| After plan 33-08 | **108** | **2,632,235** |
| Delta | **+2** | **+608** |

### Per-case counts, matched-case count confirmed non-zero first

| Selector | Cases | Assertions |
|---|---|---|
| `vco sync: (SC-3 / D-10) the per-sample step*` | **1** | **32** |
| `vco sync: (SC-3 / D-10) the corrected reset delta*` | **1** | **576** |

32 + 576 = **608**, exactly the suite delta. Nothing else moved.

---

# TASK 1 — THE MEASURED ENVELOPE

## The grid

420 cells, deliberately the SAME five axes as `tests/test_vco_spectrum.cpp`'s `SYNC_GRID`, so a finding in one instrument can be looked up cell-for-cell in the other: three sample rates × two master edge shapes × seven master/slave ratios (0.50, 0.75, 1.00, 1.50, 2.50, 3.50, 5.50) × the five shape centres × character at both ends. 4096 samples per cell at a dyadic master increment of 1/128 — **1,720,320 core steps, 13,230 resets** (32 per cell on the 210 hard-edge cells, 31 on the 210 band-limited ones, where the polyBLEP residual applied to the sample *before* the first wrap moves that wrap's detection out of the block). Measured cost of one pass: **0.12 s**.

## Reset samples are identified from telemetry, and the linkage is asserted rather than trusted

`grep -n 'syncFired' tests/test_vco_core.cpp` → **799** (`SyncTrace::record`), 3160 (an unrelated 33-04 comment), **3784** and **3816**, both inside this case's range (3765–3977). Line 3816 is the assertion:

```cpp
CHECK(tv.fired[(size_t)(nv - 1)]      == (dv.core.tel.syncFired ? (char)1 : (char)0));
CHECK(tv.correction[(size_t)(nv - 1)] == dv.core.tel.syncCorrection);
CHECK(tv.jump[(size_t)(nv - 1)]       == dv.core.tel.syncJump);
```

The recorder writes with a deliberate off-by-one, so its final entry — recorded from the live core *after* `run()` returns — is compared against the core's own `tel` by exact equality before anything is measured through it. Inferring resets from the waveform ("a large step means a reset") would have been circular in a case whose entire subject is how large the step at a reset is.

## THE CONSTANT AND ITS COMMENT, QUOTED

```cpp
	// THE BOUND. Local to this case, exactly as scenario five's exercise floor
	// is, and deliberately NOT hoisted to namespace scope: invariant 11 must be
	// able to say it consults no pinned number, and the cheapest way to make
	// that structural rather than a promise is for this constant not to be in
	// scope there at all.
	const float kSyncResetDeltaBoundV = 9.90f;
```

Its provenance lives in the case banner, in the shape invariant 2 uses for its tiers:

> MEASURED by plan 33-08 in this repository, on the SHIPPED past-edge leg (`forge::VcoCore` calling `forge::MorphBlep::addPastStep` since plan 33-06), over all 420 cells of the sweep below with the bound temporarily raised so nothing could fire:
>
> - grid worst `|x[n] - x[n-1]|` on a reset sample — **9.793601 V**
> - the cell it came from — **44.1 kHz, band-limited master, ratio 0.50, the 5 % pulse centre, character 0.00**
> - per rate (44.1 / 48 / 96 kHz) — **9.793601 / 9.793601 / 9.793601 V**
>
> PINNED AT **9.90 V**, and the derivation is TWO-SIDED: (a) it must be AT OR ABOVE the measured worst, 9.793601 V; and (b) it must be STRICTLY BELOW what a seam-free core measures on this same grid, 10.000000 V. The admissible interval is `[9.793601, 10.000000)`, whose midpoint is 9.896800; **ROUNDED OUTWARD — upward, since this is an upper bound — to the nearest hundredth of a volt.**

## The measured worst per rate, alongside the pinned bound

| | 44.1 kHz | 48 kHz | 96 kHz | pinned bound | margin |
|---|---|---|---|---|---|
| **Shipped (corrected) leg** | **9.793601** | **9.793601** | **9.793601** | 9.90 V | **0.106 V** |
| Withheld leg (diagnostic) | 10.000000 | 10.000000 | 10.000000 | — | −0.100 V (**above** the bound, by design) |

**The envelope is rate-independent to six decimal places, and that is recorded rather than assumed.** Same cell, same value at all three rates, because the grid is parametrised by master cycles *per sample* rather than by hertz — the same construction invariant 8 uses.

## What the criterion is NOT, asserted rather than only written

The case opens by stating that this is **not** a smallness claim. A legitimate hard-sync reset at a slave at or below its master's rate genuinely steps the output by nearly its full peak-to-peak range in one sample, and that is hard sync working. So the case asserts a **floor** as well as a bound:

```cpp
CHECK(gridWorstCorrected > kSyncResetDeltaFloorV);   // 9.0 V
```

If a future change brought the worst reset step under 9.0 V this goes red and asks whether hard sync is still resetting the phase.

## The analytic bound, rejected in an assertion

`|x[n] - x[n-1]| <= 2 * kHostileBoundV = 20.0 V` is available for free, permanently valid, and **close to vacuous**: a full-scale artefact — a ±5 V waveform jumping to the opposite rail in one sample — is a step of about 10 V, which a 20 V bound admits. The rejection is mechanical, not prose:

```cpp
const float kSyncAnalyticDeltaBoundV = 2.f * kHostileBoundV;   // 20.0 V
CHECK(kSyncResetDeltaBoundV < kSyncAnalyticDeltaBoundV);
```

## PROVED ABLE TO FAIL — the acceptance experiment, run

**Perturbation:** the seam call commented out of the shipped header —
`src/dsp/VcoCore.hpp:1101` `blep.addPastStep(syncFrac, tel.syncJump);` → `//PROBE blep.addPastStep(...)`.

| | Result |
|---|---|
| **This case** | **RED — 4 failed of 32**: the grid-wide bound and all three per-rate bounds, each reporting **`CHECK( 10 <= 9.9 )`** |
| Suite-wide | **2 cases red, 416 failed assertions** (this case's 4 plus the `(D-06)` bit-exactness gate's 412) |
| After `cp` restore | **108 cases, 2,632,235 assertions, 0 failures** |

**The bound fires. It is not a number the implementation cannot miss.** That is what constraint (b) in the derivation buys, and it is the reason the interval was not widened to a round 10.0 V.

---

# TASK 2 — THE ANTI-CIRCULARITY MARGIN

## It consults no pinned number, and that is structural

| Check | Result |
|---|---|
| Case + banner line range | **3979–4303** |
| `grep -cE 'kSyncResetDeltaBoundV\|kSyncResetDeltaFloorV\|kSyncAnalyticDeltaBoundV\|kSyncExerciseFloorV\|kHostileBoundV\|kMusicalBoundV\|thresholdDb\|measuredDb'` over that range | **0** |
| Why it is structural rather than a promise | Task 1's bound is declared **inside** its own `TEST_CASE` (line 3771), so it is **not in scope** here at all |

The only pinned numbers in the case are the margin itself and the classifier's floor, and both are derived from this run's own measurements.

## Both legs come from the SAME pass — the reconstruction expression and the telemetry member it reads

```cpp
const double now  = (double)o.outNow  + 5.0 * (k - 1.0) * (double)o.corrNow;
const double prev = (double)o.outPrev + 5.0 * (k - 1.0) * (double)o.corrPrev;
```

`o.corrNow` and `o.corrPrev` are **`forge::VcoCore::Telemetry::syncCorrection`**, recorded per sample by `SyncTrace::record`. At `k = 1` this is the shipped leg unchanged; at `k = 0` it is `out[n] - 5.f * syncCorrection[n]`, exactly the reconstruction `src/dsp/VcoCore.hpp` states. **There is no second `forge::VcoCore`, no `NaiveVcoCoreMirror`, no `bool bandLimit` flag in the shipped body and no second drive.**

**No bit-exact equality is written against the reconstruction**, and the file says why: plan 33-06 measured it exact on 49,136 of 49,152 samples with 16 off by exactly one ulp, worst 4.77e-07 V. The comparisons here are envelope-versus-envelope against volt-scale numbers, and the one identity that *is* asserted (below) is asserted at 1e-5 V rather than by `==`.

## The physical criterion, stated ABOVE the enumeration

| | Line |
|---|---|
| The criterion, on its physical basis | **4016** (banner) |
| Its constant | **4101** |
| **The first population count** | **4117** |
| **Its assertion** | **4130** |

> **A CELL IS STEP-CARRYING WHEN THE LARGEST JUMP ITS RESETS PRODUCE IS AT LEAST 0.75 PRE-MULTIPLY UNITS** — three quarters of the naive waveform's own unit amplitude, or 3.75 V at the output.

**The physics is the seam's own arithmetic.** `addPastStep` deposits `-f*f*jump/2` — a correction *proportional to the jump* and to at most half of it. On a cell whose resets produce a large jump that deposit is a genuine correction to a genuine discontinuity; on a cell whose resets barely move the waveform there is no step for a *step* blep to correct, and the surviving after-edge half is a small additive perturbation that can land either way. `tel.syncJump` is computed **before the seam runs** and is identical on the shipped leg, the withheld leg and all three probes, so it cannot be a restatement of the result — the same argument plan 33-07 gives for inheriting 33-05's jump floor.

## The restriction is NOT "exclude the cells that failed", and the measurement says so

**The floor sits inside a wide EMPTY GAP in the measured jump distribution.** No cell anywhere on the 420-cell grid has a largest jump between **0.639500** and **0.921976** — a window a factor of **1.44** wide. Any floor in it selects the same 277 cells. Both edges are asserted:

```cpp
CHECK(largestJumpBelowFloor  <= 0.65f);   // MEASURED 0.639500
CHECK(smallestJumpAboveFloor >= 0.92f);   // MEASURED 0.921976
```

A floor tuned to exclude particular failures would have to be tuned to a particular *value*, and there is no value in that window that changes the answer.

| Population | Cells | Asserted |
|---|---|---|
| **STEP-CARRYING (gated)** | **277** | exactly |
| Not step-carrying | **143** | exactly |
| Total | 420 | `REQUIRE(grid.size() == 420u)` |

## The margin distribution — the shape of the evidence, not one number

Margin ≡ (worst withheld reset step) − (worst shipped reset step), over the 277 gated cells:

| | Value | Cell |
|---|---|---|
| **minimum** | **0.095148 V** | 48 kHz, band-limited, ratio 5.50, square centre, character 1.00 |
| median | **0.874437 V** | |
| maximum | **1.781152 V** | |

**Per rate:**

| rate | n | min | median | max |
|---|---|---|---|---|
| 44.1 kHz | 91 | 0.095156 | 0.206400 | 1.781152 |
| 48 kHz | 95 | 0.095148 | 0.882165 | 1.781152 |
| 96 kHz | 91 | 0.095148 | 0.206400 | 1.781152 |

**Per master edge shape:**

| edge | n | min | median | max |
|---|---|---|---|---|
| hard-edge | 139 | **0.874437** | 1.597610 | 1.781152 |
| band-limited | 138 | **0.095148** | 0.173168 | 0.206400 |

**The binding cells are the band-limited ones, by an order of magnitude, and that is the expected direction.** On a hard-edged master the detected fraction is a nearly constant 0.5968 (invariant 7 measures it), so `f*f` is a stable 0.36 and the deposit is predictable; on a band-limited master `f` ranges across most of the unit interval, so the deposit is small wherever `f` is small.

## The margin's provenance

| | Value |
|---|---|
| Gated population size | **277** |
| Measured **minimum** over it | **0.095148 V** |
| Rounded OUTWARD (downward, it is a floor) to a hundredth | **0.09 V** |
| Then **halved** and rounded outward again | **0.04 V** ← `kSyncAntiCircularityMarginV` |
| Cushion factor | **2.38×** |

Step two is deliberate and is spent on **one** problem: every volt and every decibel in this phase is an **Apple-clang figure** and this margin has never been measured on another toolchain. Plan 33-07 spent a factor of **five** on the same problem for its snap floor and said so; this spends 2.38 and says so. The gate is still 0.04 V against a measured 0.095148 V over 277 cells, and the distribution above is the evidence rather than the constant.

## THE CORRECTION IS NOT A UNIFORM IMPROVEMENT — asserted, in a second instrument

| | Value |
|---|---|
| Cells with a **negative** margin (whole grid) | **56 of 420** |
| …how many of those are inside the gated population | **0** |
| **Worst negative margin** | **−0.246492 V** |
| …at | **44.1 kHz, hard-edge master, RATIO 5.50, square centre, character 1.00** |
| Negatives by ratio | 0.50 → 6, 0.75 → 0, 1.00 → 17, 1.50 → 6, 2.50 → 9, 3.50 → 9, **5.50 → 9** |

**This is the same region, found again in a different instrument on a different metric.** Plan 33-06 measured the shipped leg **0.15 to 1.09 dB worse than no correction at ratio 5.5** spectrally; plan 33-07 reproduced it at a mean of **−1.0281 dB** over 60 cells and asserted the sign permanently. This plan finds it in the **time domain** and pins it:

```cpp
CHECK(nNegativeAnywhere > 0);
CHECK(worstNegativeAtRatio55 <= -0.20);
CHECK(nNegativeInGated == 0);
```

> **THE COUNT IS DELIBERATELY NOT PINNED AS AN EQUALITY, and that is a decision rather than an omission.** Twenty of the 56 negative margins are within a **millionth of a volt** of zero (measured: −1.2e-05, −6.0e-06, −3.2e-06, … −6e-09). An exact count there would pin float rounding rather than physics, and would be the kind of fragile assertion this project's register keeps recording. What is pinned instead are the two robust facts: the population is non-empty, and its worst member is at ratio 5.50 and at least two tenths of a volt.

## The mutation probes — three, each failing a STATED population EXACTLY

**The derivation the stated populations rest on, asserted per cell rather than assumed.** The seam's deposit is purely additive and nothing is owed forward (33-06 measured 93 resets and exactly 93 differing samples), so a leg whose seam deposited `k` times the correction differs from the shipped leg by exactly `5*(k-1)*syncCorrection` on every sample — which makes the margin **exactly `k` times** the unprobed margin.

```
MEASURED departure of margin(0.25) from 0.25 * margin(1), over all 277 gated cells:
    exactly 0.000e+00
```

Asserted at `derivErr < 1e-5` rather than by float equality, for the error-bar reason above.

| Probe | Defect class | STATED | OBSERVED | Equal? |
|---|---|---|---|---|
| **Quarter deposit** (`* 0.5f` mistyped as `* 0.125f`) | coefficient typo | **69** of 277 | **69** | **yes** |
| **Half deposit** (`* 0.25f`) | coefficient typo, smaller | **0** of 277 | **0** | **yes** |
| **Inverted sign** (`k = -1`) | 33-05's `kProbeBadSign` class | **277** of 277 | **277** | **yes** |

**No STOP-AND-REPORT is required: all three stated and observed populations are equal.**

**The half-deposit probe is the control that makes the quarter-deposit probe discriminating rather than trivial.** A probe that fires on everything proves nothing about sensitivity. Half the deposit halves every margin, and the smallest gated margin is 0.095148 V, so every halved margin is at least 0.047574 V — above the pinned 0.04 V, and **nothing fires**. Plan 33-07's +2.0 dB probe firing on 192 step-dominated cells and **0** plateau cells is the same shape.

**And the quarter-deposit cut's own robustness is measured and asserted.** The cut at four times the pinned margin (0.16 V) sits in a gap: the nearest gated margins are **0.155046** below and **0.173168** above, so the stated population of 69 does not turn on the last digit of the pin.

```cpp
CHECK(largestGatedMarginBelowCut  <= 0.156);
CHECK(smallestGatedMarginAboveCut >= 0.172);
```

---

# TASK 3 — THE OUTPUT TIERS, RE-DERIVED FOR SYNC

## The two definition lines, quoted and confirmed unchanged

```cpp
constexpr float kHostileBoundV = 10.0f;
constexpr float kMusicalBoundV = 5.55f;
```

`tests/test_vco_core.cpp:170-171`, byte-identical to the pre-plan commit.

```
$ git diff HEAD~3 -U0 tests/test_vco_core.cpp | grep -E '^[-+].*(kHostileBoundV = |kMusicalBoundV = )'
(no output)
```

The only occurrence of either name in the whole-plan diff is a **comment** in invariant 10's banner, `//     kHostileBoundV, so |x[n] - x[n-1]| <= 2 * kHostileBoundV = 20.0 V,`.

## The measurement

| | 44.1 kHz | 48 kHz | 96 kHz | grid-wide |
|---|---|---|---|---|
| **Worst \|out\| on the SC-3 sweep** | **8.218569** | **8.216589** | **8.216589** | **8.218569 V** |
| the cell | hard-edge, ratio **1.50**, square centre, character 0.00 | hard-edge, ratio **5.50**, square, char 0.00 | hard-edge, ratio **5.50**, square, char 0.00 | 44.1 kHz row |

**This is the largest envelope measured anywhere in this suite.** Scenario five's Nyquist-ceiling worst case reaches 7.150281 V; the audio-rate MORPH sweep reaches 6.289864 V; hard sync reaches **8.218569 V**, 1.07 V above the previous maximum and still 1.78 V inside `kHostileBoundV`.

## Which tier is asserted

| Tier | Decision | Assertions |
|---|---|---|
| `kHostileBoundV` (10.0 V) | **ASSERTED unconditionally**, grid-wide **and** per rate | `CHECK(gridWorstOut <= kHostileBoundV)` + 3 per-rate |
| `kMusicalBoundV` (5.55 V) | **WITHHELD, and the withholding ASSERTED** | `CHECK(gridWorstOut > kSyncExerciseFloorV)`, `CHECK(kSyncExerciseFloorV > kMusicalBoundV)` + 3 per-rate |

**Neither constant was widened.** Widening one to admit this scenario would have destroyed what the tier means for the four scenarios that assert it.

## The per-rate form was CHECKED before it was written

Invariant 6 records the trap explicitly: its own excess assertion is grid-wide and **not** per-rate, because *"at 96 kHz the excess vanishes entirely … and a per-rate form of this assertion would be RED there on correct behavior."*

**Checked here first.** The sync scenario's per-rate worsts are **8.218569 / 8.216589 / 8.216589 V**, so **all three** exceed the musical tier — by at least **2.67 V** at the least favourable rate. The per-rate form is therefore true, is the form written, and is **strictly stronger** than the grid-wide one.

**And a cushioned exercise floor is affordable here where invariant 6 says it is not.** Invariant 6 clears the tier by 0.74 V, so a 1.5 V cushion would have put its floor *below* the tier where it would assert nothing. This scenario clears it by 2.67 V, so scenario five's rule applies unchanged: the minimum of the three per-rate worsts (8.216589 V) less a 1.5 V cushion, pinned at **6.70 V**.

## The provenance block gained exactly one row, and no existing row was edited

```
$ git diff HEAD~3 -U0 tests/test_vco_core.cpp | grep '^@@'
@@ -1317,0 +1318,66 @@ TEST_CASE("vco core: naive pitch tracks the C4 reference on the OUTPUT ...
@@ -2769,0 +2836,20 @@ TEST_CASE("vco sync: (SYNC-01 / D-01 / D-03) a master rising edge resets ...
@@ -3748,0 +3835,6 @@ ...
@@ -3768,0 +3861,7 @@ ...
@@ -3837,0 +3937,40 @@ ...
```

Every hunk is of the form `-N,0 +M,K` — a **pure insertion with zero deleted lines**. The provenance block's hunk is `@@ -1317,0 +1318,66 @@`: **66 added lines, 0 removed.** The new section is headed `THE SYNC SCENARIO — ADDED BY PLAN 33-08, RE-DERIVED AND NOT INHERITED` at line 1320, and opens by stating that nothing above it was edited.

**Whole-plan diffstat: `tests/test_vco_core.cpp | 970 ++++`, 970 insertions(+), 0 deletions.**

---

# Gate Results

| Gate | Result |
|------|--------|
| `make test` | **PASS** — 108 cases, 2,632,235 assertions, 0 failures |
| `make strict` (C++11, `-pedantic-errors`) | **PASS**, exit 0 |
| `make guards` | **PASS**, exit 0 (all three scripts) |
| `bash tests/check_frozen.sh` | **PASS** — D-05 manifest + goldens + negative control |
| Six shipped-LFO goldens | **byte-identical** — 9 cases / 49,188 assertions, 0 failures |
| Compiler warnings in the changed TU | **0** (`-Wall -Wextra`) |
| `src/AnalogLFO.cpp` in the whole-plan diff | **absent** (grep count 0) |
| `git diff --name-only` across all three commits | **`tests/test_vco_core.cpp` alone** |
| `tests/check_includes.sh` diffstat | **empty** — no new translation unit |
| Four frozen shared headers in the diff | **none** |

---

# Decisions Made

1. **THE BOUND IS PINNED FROM A TWO-SIDED INTERVAL, NOT ROUNDED OUTWARD FROM THE MEASUREMENT ALONE.** Rounding 9.793601 V outward gives 9.80 V (0.006 V of headroom — unusable across toolchains) or 10.0 V (which *deletes* the property that makes the bound evidence). The second constraint — strictly below the 10.000000 V a seam-free core measures — is what makes it falsifiable, and the acceptance experiment proved it fires. 9.90 V is the interval's midpoint rounded outward.

2. **THE HEADLINE FINDING IS REPORTED, NOT BURIED: THE MEASURED ENVELOPE IS ONLY ABOUT TWO PERCENT TIGHTER THAN THE UNCORRECTED ONE.** There are 0.206 V between what the shipped leg does at the grid's worst cell and what a core with no sync correction does there. The case banner says so in terms and forbids widening the bound into comfort. A plan that wanted a reassuring number here could have pinned 11.0 V and reported "comfortably inside the bound"; that number would have been true and worthless.

3. **The gated population's criterion is placed inside a MEASURED EMPTY GAP, and both gap edges are asserted.** The first threshold considered — a jump of 1.0 unit, the waveform's own full amplitude — turned out to have a boundary gap of **1.0e-06**: cells at 0.999999762 and 1.000000715 sit either side of it, so the population count would have been at the mercy of one ulp on another toolchain. Measuring the distribution first and *then* choosing 0.75, which sits in a gap 0.28 wide, is the difference between a population assertion that means something and one that pins rounding.

4. **The margin's cushion is a stated factor, not a hidden one.** 0.095148 → 0.09 (outward) → 0.04 (halved, outward again). The reason is named — Apple-clang-only exposure — and the comparison to 33-07's five-times snap floor is made explicitly so a reader can see this phase's cushion policy is consistent rather than ad hoc.

5. **The negative-margin count is deliberately NOT pinned as an equality.** Twenty of the 56 are within a millionth of a volt of zero. Pinning `== 56` would have looked more rigorous and would have been a fragile assertion about float rounding. The worst value and its ratio are pinned instead, which is the robust half of the same claim — and it is the half that matters, because it is the half that reproduces 33-06's and 33-07's spectral finding.

6. **The per-rate form of the withholding was checked before it was written.** Invariant 6 records exactly where the same form would be red at 96 kHz on correct behaviour. Here all three rates exceed the tighter tier by at least 2.67 V, so the stronger form is true and is the one landed. **This is the third plan in this phase to check a precedent's trap rather than inherit its shape**, and the first where the check came back permitting the stronger claim.

7. **SYNC-02 IS DECLINED — THE ELEVENTH CONSECUTIVE DECLINE, AND THE REASON HAS CHANGED SHAPE AGAIN.**

   **The gap this plan was assigned is CLOSED.** 33-06 and 33-07 both named the same missing thing: *"'click-free' has no instrument."* It has one now — time-domain by necessity, on reset samples identified from telemetry, proved able to fail by removing the seam from the shipped header, consulting no spectral number, with a naive-versus-corrected margin from one pass and three probes that fail stated populations exactly. **That is not what is missing any more.**

   **What is missing is what the instrument REPORTS.** Three measured facts, none of which supports the word "click-free":

   - **The worst per-sample step across a reset is 9.793601 V**, against 10.000000 V for a core with no sync correction at all. The shipped sync BLEP removes about **2 %** of the worst-case step. That is a real improvement, it is asserted, and it is not what "click-free" describes.
   - **On 56 of 420 cells the correction makes the worst reset step LARGER**, worst −0.246492 V at ratio 5.50 — the same region 33-06 measured at 0.15–1.09 dB worse and 33-07 at a mean −1.0281 dB. **A requirement whose text says "click-free" cannot be ticked while the mechanism meant to deliver it measurably worsens the metric on thirteen percent of the measured parameter space.** The prior-wave brief named this case explicitly, and this is it.
   - **AND THE INSTRUMENT HAS A NAMED LIMIT OF ITS OWN, STATED HERE RATHER THAN DISCOVERED LATER.** It bounds the *total* step and compares corrected against uncorrected. It does **not** separate the INTENDED full-scale step — which is correct behaviour at a slave at or below its master's rate — from the RESIDUAL discontinuity the band-limiting failed to absorb, which is what SC-3 actually forbids. **Both of its legs contain the intended step**, so their difference measures the correction's *size*, not the residual's. Closing "click-free" needs an instrument that can measure the second quantity, and nothing in this phase has one.

   **`.planning/REQUIREMENTS.md` was CHECKED against disk, not assumed, after this plan finished:** line 39 `- [x] **SYNC-01**` and line 134 `| SYNC-01 | Phase 33 | Complete |`; line 40 `- [ ] **SYNC-02**` and line 135 `| SYNC-02 | Phase 33 | Pending |`. **SYNC-02 remains `[ ]` / `Pending`.**

---

# Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] Invariant 7's three recorded sync-envelope figures were falsified by plan 33-06's seam and were still in the file**

- **Found during:** Task 3, re-deriving the tiers — the paragraph that hands this plan its obligation also quotes three numbers
- **Issue:** `tests/test_vco_core.cpp`'s invariant 7 records *"this drive measures 4.920715 / 4.920976 / 4.921710 V at the three rates"*, measured by plan 33-04 against the **pre-seam** core. RE-MEASURED on the shipped past-edge leg: **4.908170 / 4.910800 / 4.920170 V**. The correction reduces the envelope there by 0.012545 / 0.010176 / 0.001540 V. The figures had been stale since 33-06 and are the figures a later plan would have inherited when deciding invariant 7's own tier.
- **Fix:** **Appended, not overwritten.** Nothing plan 33-04 wrote was deleted, because that paragraph is a correct record of what it measured and of *why* it withheld the tier; a new block beside it marks the figures PRE-SEAM, records the re-measurement, and states that plan 33-08's obligation is discharged in invariant 10 rather than here.
- **Files modified:** `tests/test_vco_core.cpp`
- **Verification:** Extracted from `-s` output per rate and offset; suite green.
- **Committed in:** `603b226`

**2. [Rule 2 — Missing correctness] Invariant 7 still declines the tighter tier, and that had to become a decision rather than an omission**

- **Found during:** Task 3
- **Issue:** Invariant 7's drive measures 4.92 V and would individually qualify for `kMusicalBoundV`. Its stated reason for withholding — *"the seam is withheld until plan 33-06"* — became false when the seam landed. Left alone, the file would have carried a withholding whose only recorded justification no longer applied.
- **Fix:** The appended block states the *current* reason and makes it a decision: granting the tighter tier to one favourable sync drive while the sync class as a whole measures 8.218569 V would be a tier claimed by the scenario that needs it least. The assertions were **not** changed.
- **Files modified:** `tests/test_vco_core.cpp`
- **Verification:** Suite green at unchanged assertion counts for that case.
- **Committed in:** `603b226`

**3. [Rule 3 — Blocking] `maxAbsJumpOf` was written in Task 1 and consumed only in Task 2, so Task 1 did not compile warning-free**

- **Found during:** Task 1, first build
- **Issue:** `-Wall -Wextra` emitted `unused function 'maxAbsJumpOf'`. **Zero compiler warnings in the changed TU is a standing gate of this project**, and shipping a warning across a commit boundary would have made Task 1's own gate result false.
- **Fix:** The helper was moved into Task 2's commit, where its first consumer lives. No behaviour changed.
- **Files modified:** `tests/test_vco_core.cpp`
- **Verification:** Both commits build at zero warnings.
- **Committed in:** `8934703` / `b2573b0`

### Reported, not fixed

**4. [Reported] The first classifier threshold considered was one ulp from being a coin flip — measured, then replaced**

- **Found during:** Task 2, before any assertion was written
- **Issue:** The obvious physical threshold for "the reset produces a step" is a jump of **1.0** unit — the naive waveform's own full amplitude. MEASURED, the grid has cells at `maxAbsJump` = 0.999999762 and 1.000000715, a boundary gap of **1.0e-06**. A `nGated == 257` assertion on that threshold would have been an assertion about float rounding, and would plausibly red on the CI MinGW leg.
- **Fix:** **Reported and designed around rather than absorbed.** The full distribution was measured first, a gap of 0.28 was located between 0.639500 and 0.921976, and the threshold was placed inside it at 0.75 — with **both gap edges asserted in the source**, so the robustness is itself a measurement rather than a claim in this SUMMARY.
- **Files modified:** none beyond the chosen threshold
- **Verification:** `CHECK(largestJumpBelowFloor <= 0.65f)` and `CHECK(smallestJumpAboveFloor >= 0.92f)`, both green.
- **Committed in:** `b2573b0`

**5. [Reported] The plan's Task-2 wording anticipates a margin that is "too small to be a useful gate"; the measured problem was the SIGN, not the size — the eleventh instance of a mechanism narrower than its own prose**

- **Found during:** Task 2, on the first measurement
- **Issue:** Task 2 says *"Pin the margin from the MINIMUM over the gated population, rounded outward. If that minimum is too small to be a useful gate, restrict the assertion…"*, and 33-RESEARCH Pitfall 4 warns the margin *"can be as small as 0.003 V"*. MEASURED, the minimum over the **whole grid** is **−0.246492 V** — not small, **negative**. The restriction was not an optimisation for a tighter gate; without it the assertion is simply false.
- **Fix:** **Reported rather than glossed.** The restriction was taken for the reason the measurement gives, stated on a physical criterion above its enumeration as the plan requires, and the population where the effect is *absent or reversed* is asserted alongside the population where it is present — so the restriction is evidenced rather than declared. Following 33-05's deviation 3, 33-06's deviation 5 and 33-07's deviations 5 and 6, the numbers are given rather than the verdict.
- **Files modified:** none — a criterion-interpretation decision
- **Verification:** The negative-margin table above.
- **Committed in:** n/a

---

**Total deviations:** 3 auto-fixed (1 × Rule 1, 1 × Rule 2, 1 × Rule 3) + 2 reported
**Impact on plan:** One removed a stale figure the seam falsified two plans ago, without deleting the record that explains it. One turned a withholding whose reason had expired back into a decision. One kept a standing zero-warning gate honest across a commit boundary. Both reported items are measurement-first calls where writing the obvious thing would have produced a fragile or false assertion. **The whole-plan diff is `tests/test_vco_core.cpp` alone, 970 insertions and zero deletions.**

---

# Known Stubs

**None.** Every constant, helper and struct field this plan adds is consumed by an assertion in the same commit or the next.

One thing is *absent by design*, and unlike the previous two plans it does not have an owner yet:

| Absence | Owner | Why it is not a stub here |
|---|---|---|
| No instrument separates the INTENDED full-scale reset step from the RESIDUAL the band-limiting failed to absorb | **unassigned — see deferred register item 1** | This plan's two legs both contain the intended step, so their difference measures the correction's size rather than the residual's. That is a limit of the instrument this plan built, named here rather than left to be discovered by whichever plan tries to tick SYNC-02. |

---

# Deferred Register Items

**1. NEW — SC-3's "click-free" clause needs a quantity NEITHER instrument in this phase measures, and no plan owns it.**
The spectral gate cannot see a single-sample spike (0.0 dB, register item 5). This plan's time-domain gate can see the *step*, but a legitimate reset genuinely produces a near-full-scale step, so seeing the step is not seeing the artefact. The quantity SC-3 is actually about is the **residual discontinuity after band-limiting**, separated from the intended jump — plausibly `|x[n] - x[n-1]| - |jump|` evaluated against what a correctly band-limited step *should* leave, which needs an oracle this phase does not have. **This is the whole of SYNC-02's remaining gap and it is now precisely stated for the first time in the phase.**
**Proposed Resolve-at:** plan 33-11, as a phase-gate decision — either an operator decision that SYNC-02 is discharged on the mechanism plus the perceptual UAT of plan 33-12, or an explicit carry to a later milestone. It should NOT be absorbed by widening this plan's bound.

**2. NEW — the SC-3 envelope is only 2 % tighter than the uncorrected one, and the admissible interval for the bound is 0.206 V wide.**
`kSyncResetDeltaBoundV` has 0.106 V of headroom above the measurement and 0.100 V of clearance below the withheld leg. **There is no room to widen it without deleting the property that makes it evidence.** If another toolchain measures the shipped leg above 9.90 V, that is a **finding about the seam**, escalated per the anti-softening rule, and must not be absorbed by moving the constant toward 10.0.
**Proposed Resolve-at:** plan 33-11, on the CI MinGW leg.

**3. CARRIED and now ASSERTED IN A SECOND INSTRUMENT — 33-06's item 3 / 33-07's item 2: the landed leg is measurably WORSE than no correction at high ratios.**
Measured here in the **time domain**: 56 of 420 cells have a negative anti-circularity margin, worst **−0.246492 V at ratio 5.50**. The spectral measurements (33-06: 0.15–1.09 dB worse; 33-07: mean −1.0281 dB over 60 cells) and this one agree on the region and the sign. **Two independent instruments now report it and both assert it permanently.**
**Proposed Resolve-at:** unchanged — no code change in v2.0; a ratio-conditional correction is not available to a core that cannot know the master's frequency.

**4. CARRIED — every volt in this SUMMARY, in the new banners and in both pinned constants is an Apple-clang figure.**
Unchanged in kind from 33-01 through 33-07. The exposure's shape here: **two new pinned volt-scale constants** and **three pinned population counts** (277 / 143 / 69), of which the counts are protected by measured distribution gaps (1.44× and 0.018 V respectively) and the constants are not. `make strict` passes locally at C++11 `-pedantic-errors`; T-33-08 is not discharged locally and the CI MinGW leg remains plan 33-11's.

**5. CARRIED — 33-07's items 1, 3 and 5, 33-06's items 1, 4 and 5, 33-05's 2/3/5, and 33-02/03/04's six, are unchanged by this plan.**
33-07's item 1 (the inherited 1.0 dB step-dominated bound, 192 gated rows riding on it) is untouched — this plan gates on volts, not decibels, and adds no dependency on it.

---

# Issues Encountered

- **The measurement said something less comfortable than the plan anticipated, twice.** Task 2's wording expects a small positive margin; the grid-wide minimum is negative. Task 1's implicit expectation is that a measured envelope will be usefully tighter than the analytic one; it is 2 % tighter than the *uncorrected* leg and half the analytic bound only because the analytic bound is 20 V. Both were written down as they came out. The tempting move in each case was to widen a number until the sentence sounded better.
- **The first classifier threshold was one ulp from being arbitrary, and only measuring the distribution first revealed it.** Choosing 1.0 because it is the waveform's amplitude is a *physical* argument, and it was still the wrong number — 20 cells sit within 3e-05 of it. The habit that caught it is the one this file keeps insisting on: work out the physical criterion, then look at the distribution *before* pinning the count.
- **`check_canary.sh [2b/5]` remains measurement-blind on this host** (33-03's finding). Irrelevant here — this plan adds no shipped code — but `make guards` going green was again treated as evidence of nothing about behaviour.
- **Two untracked `.planning/research/.cache/*.json` files** pre-existed at session start, as they have since 33-02, and were left alone.

---

# Next Phase Readiness

**SC-3 has an instrument. SYNC-02 does not have a tick, and for the first time the reason is a measurement rather than an absence.**

- **Plan 33-09** inherits a suite at 108 cases / 2,632,235 assertions and two new namespace-scope helpers in `tests/test_vco_core.cpp` (`sweepSyncDeltaGrid`, `worstResetDeltaAt`, `maxAbsJumpOf`) that any later time-domain sync work should reuse rather than re-derive.
- **Plan 33-10** is unaffected. Its reconstruction relationship and one-ulp error bar come from 33-06 unchanged — and this plan is a second consumer of that relationship that also declined to write a bit-exact equality against it, which is the behaviour 33-06 asked for.
- **Plan 33-11 inherits register items 1 and 2 as the phase's newest open risks**, alongside 33-07's item 1. Item 1 is the one that decides SYNC-02. Its CI MinGW leg is also the first cross-toolchain measurement of `kSyncResetDeltaBoundV`, which has 0.106 V of headroom and no room to be widened.
- **Plan 33-12 owns the operator UAT, and it should read Decision 7 before writing its expected-results block.** 33-07 already warned it not to promise a dramatic spectral difference; this plan adds the time-domain half of the same warning. The measured facts an operator's ears will be asked to judge are: the reset step is nearly full-scale and is *supposed* to be, the correction removes about 2 % of it, and on some ratio settings the correction is measurably worse than none. **An operator told to expect an obvious "click disappearing" would be told something neither instrument in this phase supports.**

**Concerns carried forward:**

- **SYNC-02's remaining gap is now precisely named for the first time:** the residual-versus-intended-step separation, which no instrument in this phase measures and no plan owns. Register item 1.
- **The SC-3 bound has 0.206 V of total room and cannot be widened without becoming vacuous.** Register item 2.
- **The correction is worse than nothing at the top of the ratio sweep**, now asserted in two independent instruments.

---

# Self-Check: PASSED

Verified against disk and git rather than asserted:

- **Files exist:** `tests/test_vco_core.cpp`, `.planning/phases/33-hard-sync/33-08-SUMMARY.md` — both **FOUND**.
- **Commits exist:** `8934703`, `b2573b0`, `603b226` — all **FOUND** in `git log`.
- **The two new cases are present in `HEAD`** and are matched by their selectors with non-zero counts: `(SC-3 / D-10) the per-sample step*` **1 case / 32 assertions**; `(SC-3 / D-10) the corrected reset delta*` **1 case / 576 assertions**. 32 + 576 = 608 = the suite delta exactly.
- **The suite really did grow:** 106 → **108** cases, 2,631,627 → **2,632,235** assertions, 0 failures.
- **Reset samples really are identified from telemetry:** `grep -n 'syncFired' tests/test_vco_core.cpp` returns 799, 3160, 3784 and **3816** — the last two inside invariant 10's range (3765–3977), and 3816 is the assertion comparing the recorder against the live core's `tel`.
- **Invariant 11 really consults nothing:** `grep -cE 'kSyncResetDeltaBoundV|kSyncResetDeltaFloorV|kSyncAnalyticDeltaBoundV|kSyncExerciseFloorV|kHostileBoundV|kMusicalBoundV|thresholdDb|measuredDb'` over lines 3979–4303 returns **0**, and Task 1's bound is declared inside its own `TEST_CASE` so it is not in scope there.
- **Neither tier constant moved:** `tests/test_vco_core.cpp:170-171` still read `constexpr float kHostileBoundV = 10.0f;` and `constexpr float kMusicalBoundV = 5.55f;`, and the whole-plan diff touches neither definition line.
- **The whole-plan diff is a pure insertion:** `970 insertions(+)`, **0 deletions**, in `tests/test_vco_core.cpp` alone.
- **The bound is genuinely able to fail:** commenting the seam call out of `src/dsp/VcoCore.hpp` reds this case at 4 of 32 assertions (`10 <= 9.9`) and the suite at 2 cases / 416 assertions; restored from a pristine copy and re-verified green.
- **Nothing shipped moved:** six LFO goldens byte-identical (9 cases / 49,188), `check_frozen.sh` PASS, `make strict` and `make guards` exit 0, **zero** compiler warnings, `src/AnalogLFO.cpp` absent from the whole-plan diff.
- **`.planning/REQUIREMENTS.md` was CHECKED, not assumed:** SYNC-01 remains `[x]` / `Complete`; **SYNC-02 remains `[ ]` / `Pending`.**

---
*Phase: 33-hard-sync*
*Completed: 2026-09-02*
