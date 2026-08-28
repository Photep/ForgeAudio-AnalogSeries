---
phase: 33-hard-sync
plan: 05
subsystem: tests
tags: [hard-sync, d-06, placement-measurement, sync-sub-grid, bit-exact-probe, stop-and-report, instrument-validity, falsified-prediction, integer-ratio-null-point]

# Dependency graph
requires:
  - phase: 33-hard-sync
    plan: 02
    provides: "the sync block in forge::VcoCore with the seam call DELIBERATELY WITHHELD — the core is exactly measurement leg `none`, which is what the placement probe checks itself against bit-exactly"
  - phase: 33-hard-sync
    plan: 04
    provides: "the measured facts this grid was designed around: a hard-edged master's detected fraction is inert (0.004 while g halves) while a band-limited one spans 0.678; and the one-sample-late fire at g = 0.96875"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "tests/test_vco_spectrum.cpp — fftRadix2, aliasPeakDb, impliedLeakageDb, driveSecondBlock, kSpectrumN, the four seed literals, the one-block warm-up discard and the one-measurement-function discipline, all reused verbatim"
provides:
  - "SyncMaster / makeSyncMaster — a dyadic-increment master generator with BOTH edge shapes from one parameter, an EXACTLY zero bin error, and the true wrap fraction it knows exactly"
  - "SyncCell / SYNC_GRID — 420 cells, both decibel columns UNPINNED, the achieved master frequency on every row"
  - "SyncPlacementProbe — all six legs and both mutation probes from ONE core-shaped struct, proved BIT-EXACT against forge::VcoCore on the no-correction leg over 1,720,320 samples"
  - "measureSyncCellDb — the ONE cell-measuring function, leg-parameterised, with useLiveCore mirroring measureCellDb's useMirror"
  - "fundamentalDominanceDb — the sync sub-grid's own D-10 self-check, and the generalisation of the integer-ratio null point into a per-cell instrument-validity column"
  - "THE MEASUREMENT ITSELF: 3,360 measurements, the full per-leg table, and a STOP-AND-REPORT under the three-condition rule"
  - "The snap-to-zero landmine measured at 4.99-5.61 dB on band-limited masters — SYNC-02's sub-sample clause as evidence rather than as an inherited warning"
  - "D-07's residual phantom as a NUMBER (mean 0.0569, max 0.9624 over 30,940 reset samples), discharging 33-02's deferred item 1's request"
affects: [33-06, 33-07, 33-08, 33-11, 33-12]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "A measurement instrument gated on BIT-EXACT reproduction of the shipped core BEFORE any number it produces is believed, with the gate proved able to fail by a one-line arithmetic perturbation"
    - "Eight legs out of ONE struct and ONE cell-measuring function, so the ranking is a comparison rather than a coincidence"
    - "An INSTRUMENT-VALIDITY COLUMN computed per cell on the reference leg, so cells where the metric is dividing by the wrong thing are identified by measurement rather than assumed away"
    - "A decision rule that REFUSES to pick, reported as the result rather than repaired into a winner"
    - "A recommendation labelled in source and in the SUMMARY as NOT rule-sanctioned, with an explicit prohibition on a later agent promoting it"

key-files:
  created: []
  modified:
    - tests/test_vco_spectrum.cpp

key-decisions:
  - "NO WINNER IS DECLARED BY THE D-06 RULE. All three conditions FAIL, and condition 3's own wording says what that means: the legs differ in jump MAGNITUDE, not in placement"
  - "A RECOMMENDATION is nevertheless recorded for plan 33-06, clearly labelled as evidence-based and NOT rule-sanctioned: the PAST-EDGE leg, `blep.addStep(0.f, -f*f*jump)` at the SYNC JUMP COMPLETION line, NO header change"
  - "Three of the six recommended master/slave ratios are integer NULL POINTS where the metric divides by a bin 67-78 dB down; the grid's ratio axis is replaced and the metric is left untouched"
  - "The three conditions are evaluated on the STEP-DOMINATED and INSTRUMENT-VALID population, on a physical criterion fixed before any count, with the unrestricted figures recorded alongside"
  - "The oracle leg is WORSE than the detector's own fraction on band-limited masters: the fraction that matters is the one CONSISTENT WITH THE RESET, not the physically true one"
  - "The research's ranking prediction is PARTLY CONFIRMED and PARTLY FALSIFIED, and the falsification is reported prominently rather than absorbed"
  - "SYNC-02 is DECLINED. The seam does not exist; non-comment addStep count in src/dsp/VcoCore.hpp is still 0"

patterns-established:
  - "Before a spectral metric normalised by a fundamental bin is trusted on a new signal class, MEASURE that the fundamental is the strongest bin on its own harmonic lattice — a positive alias peak is the tell that it is not"
  - "When a decision rule refuses, report the refusal AND the structure of the refusal; a stop-and-report that says only 'no' hands the next plan nothing"
  - "A mutation probe that discriminates on one axis and not on another has measured the axis, not failed"

requirements-completed: []  # SYNC-02 is in this plan's frontmatter and is DECLINED — see Decisions #7

coverage:
  - id: D1
    description: "The sync sub-grid's master is exactly periodic in the block and its bin error is EXACTLY zero, asserted directly rather than through the leakage helper's sentinel"
    requirement: "SYNC-02"  # contributes to; not completed by this plan
    verification:
      - kind: unit
        ref: "\"(D-11) the sync sub-grid...\" — binError == 0.0 and achievedCyclesPerBlock == K_m at three rates x two edge shapes; impliedLeakageDb(0.0) shown returning -999.0 as the reason NOT to use it"
        status: pass
    human_judgment: false
  - id: D2
    description: "Both master edge shapes are present and differ in what the DETECTOR can see, not merely in their sample values"
    requirement: "SYNC-02"
    verification:
      - kind: unit
        ref: "hard-edge f spread 0.0104 / 0.0209 / 0.0230; band-limited 0.955 / 0.981 / 0.983 — asserted < 0.05 and > 0.20 respectively at all three rates"
        status: pass
    human_judgment: false
  - id: D3
    description: "The placement probe IS the shipped core on the no-correction leg"
    verification:
      - kind: unit
        ref: "\"(D-06) the sync placement probe...\" — 1,720,320 samples over 420 cells, 573,440 per rate, 0 mismatches by direct float equality"
        status: pass
      - kind: other
        ref: "PROVED ABLE TO FAIL: taking the crossing solve from a double accumulator reds the case at 9,655 mismatches (44.1k 3862 / 48k 2829 / 96k 2964) and 183 failed assertions; restored and re-verified green"
        status: pass
    human_judgment: false
  - id: D4
    description: "All eight legs measured on every cell through ONE leg-parameterised cell-measuring function"
    verification:
      - kind: unit
        ref: "3,360 measurements; grep -c 'struct SyncPlacementProbe' == 1; one measureSyncCellDb; one aliasPeakDb definition"
        status: pass
    human_judgment: false
  - id: D5
    description: "The three D-06 conditions evaluated with figures, and a refusal rather than a pick when they fail"
    verification:
      - kind: unit
        ref: "cond1 FAIL (0.6296 < 0.90; deficit clause PASSES at 0.8553 dB), cond2 FAIL (22 of 38), cond3 FAIL (flat: 0.069 / -0.136 / 0.123). STOP-AND-REPORT written into the case and this SUMMARY"
        status: pass
    human_judgment: false
  - id: D6
    description: "The snap-to-zero landmine measured against the past-edge leg per rate and per master edge shape"
    requirement: "SYNC-02"
    verification:
      - kind: unit
        ref: "band-limited +5.040 / +4.985 / +5.610 dB at 44.1 / 48 / 96 kHz (snap WORSE); hard-edge -0.813 / -1.040 / -0.766 dB (snap BETTER). Sign asserted on the band-limited half, magnitudes recorded and NOT gated"
        status: pass
    human_judgment: false
  - id: D7
    description: "The oracle-versus-winner gap recorded per rate and per edge shape, decomposing fraction accuracy from placement convention"
    verification:
      - kind: unit
        ref: "oracle vs past-edge: band-limited +0.706 / +0.560 / +0.452 dB (oracle WORSE); hard-edge -0.195 / -0.210 / -0.242 dB. none vs past-edge: band-limited +1.053 / +0.996 / +1.222 dB"
        status: pass
    human_judgment: false
  - id: D8
    description: "D-07's residual carried-forward phantom recorded as a number so a later phase inherits a measurement rather than an argument"
    verification:
      - kind: unit
        ref: "|pending| carried in to a reset sample: mean 0.0569, max 0.9624, over 30,940 reset samples on the shipped core's own leg"
        status: pass
    human_judgment: false
  - id: D9
    description: "No threshold pinned and no cell gated by this plan"
    verification:
      - kind: unit
        ref: "all 420 cells carry kSyncUnpinnedDb in both columns and kProvSyncUnpinned, asserted; tier is \"measure\", never \"gated\""
        status: pass
    human_judgment: false
  - id: D10
    description: "Nothing shipped moved"
    verification:
      - kind: unit
        ref: "make test 103 cases / 2,624,784 assertions 0 failures; six LFO goldens byte-identical (9 cases / 49,188); check_frozen.sh PASS; make strict and make guards exit 0; git diff --name-only is tests/test_vco_spectrum.cpp alone"
        status: pass
    human_judgment: false

# Metrics
duration: 47min
completed: 2026-08-29
status: complete
---

# Phase 33 Plan 05: The Placement Measurement Summary

**The phase's central question was put to a measurement against the real core, and the measurement REFUSED TO ANSWER IT under the rule it was written to answer it with — all three D-06 conditions fail, and the reason is a real property of the candidates rather than a defect in the run. Three of the six recommended grid ratios turned out to be null points where the metric divides by a bin 78 dB down.**

## Performance

- **Duration:** 47 min
- **Started:** 2026-08-29T08:05:00+10:00
- **Completed:** 2026-08-29T08:52:00+10:00
- **Tasks:** 3 of 3
- **Files modified:** 1

## Accomplishments

- **Built the measurement instrument and PROVED it is the shipped core before believing a single number it produced.** 1,720,320 samples compared by direct float equality across 420 cells at three rates: **0 mismatches**. And the gate is proved able to fail — one arithmetic step perturbed reds it at **9,655 mismatches**.
- **Ran the measurement and reported that it does not support a decision.** All three conditions fail. Condition 3's failure is the informative one: the margin is **FLAT** across a factor of 2.2 in sample rate (0.897 / 0.901 / 0.849 dB on the common cell — a 0.05 dB spread), which by the rule's own wording means the legs differ in jump **magnitude**, not in placement.
- **Found a hazard the research did not anticipate, and it invalidated three of the six recommended ratios.** At an exactly integer master/slave ratio of two or more, hard sync is a near-no-op, the signal is periodic at the **slave's** period, and the master's fundamental bin — the bin `aliasPeakDb` normalises by — is **67 to 78 dB down**. The reported figure goes **positive**, which for a master-periodic signal is impossible.
- **Turned that hazard into a permanent instrument, not just a fix.** `fundamentalDominanceDb` is the sub-grid's own D-10 self-check and it partitions the grid: **210 of 420 cells are instrument-valid**, worst dominance −29.45 dB. A permanent null-point control pins the finding so no later agent restores the recommended ratios.
- **Measured the snap-to-zero landmine at 4.99–5.61 dB** on band-limited masters at all three rates — SYNC-02's sub-sample clause as evidence with a comfortable margin, and within a decibel of the research's 4.5–4.95 dB prototype prediction.
- **Falsified the oracle's premise.** The leg using the master's **TRUE** wrap fraction is **0.45–0.71 dB WORSE** than the leg using the detector's own. The fraction that matters is the one **consistent with the reset**, not the physically true one.
- **Discharged 33-02's deferred item 1** with a number: D-07's residual phantom measures **mean 0.0569, max 0.9624** over 30,940 reset samples, against the header's explicitly-labelled arithmetic estimate.

## Task Commits

| # | Task | Commit | Type |
|---|---|---|---|
| 1 | The sync sub-grid apparatus — master-bin fundamental, exact-zero bin error, two master edge shapes (D-11) | `4b8c295` | test |
| 2 | The placement probe, validated bit-exactly against the shipped core (D-06) | `9cb0f89` | test |
| 3 | Run the measurement and apply the three-condition decision rule (D-06) | `cc34df5` | test |

## Files Created/Modified

- `tests/test_vco_spectrum.cpp` — three new `TEST_CASE`s, one new core-shaped probe struct, one new master generator, one new cell struct and grid builder, one new instrument-validity helper, and one defaulted parameter added to the existing shared drive loop. **No other file in the repository was touched by any of the three commits.**

## Suite Totals, Before and After

| | Test cases | Assertions |
|---|---|---|
| Before plan 33-05 | 100 | 2,623,356 |
| After plan 33-05 | **103** | **2,624,784** |
| Delta | **+3** | **+1,428** |

### Per-case counts, with the matched-case count confirmed non-zero first

| Selector | Cases | Assertions |
|---|---|---|
| `vco spectrum: (D-11) the sync sub-grid*` | **1** | **76** |
| `vco spectrum: (D-06) the sync placement probe*` | **1** | **853** |
| `vco spectrum: (D-06 / D-11)*` | **1** | **499** |

## The Grid, as Built

| Axis | Values | Count |
|---|---|---|
| Rate × master cycle count | 44.1 kHz `K_m = 93` · 48 kHz `K_m = 85` · 96 kHz `K_m = 43` | 3 |
| Master edge shape | hard-edge, band-limited | 2 |
| Master/slave ratio | **0.5**, **0.75**, 1.0, 1.5, 2.5, 3.5, 5.5 | 7 |
| Morph | the five shape centres 0.00 / 0.25 / 0.50 / 0.75 / 1.00 | 5 |
| Character | 0.00, 1.00 | 2 |
| **Total cells** | | **420** |

**Sub-unity cells: 120** (ratios 0.5 and 0.75). **Hard-edge 210 / band-limited 210.** **Integer ratios at or above two: 0**, asserted.

### The achieved master frequencies, and why the three cannot be equal

| Rate | `K_m` | Achieved master Hz |
|---|---|---|
| 44.1 kHz | 93 | **1001.2939453125** |
| 48 kHz | 85 | **996.09375** |
| 96 kHz | 43 | **1007.8125** |

**Spread: 1.17647 %** of the lowest, asserted below 1.5 %. With `N` pinned at 4096 the achievable master frequencies are the multiples of `sr/4096` — 10.77 Hz apart at 44.1 kHz, 11.72 Hz at 48 kHz, 23.44 Hz at 96 kHz — and below about 3.4 kHz there is no common multiple. 33-RESEARCH quotes 1001.4 Hz and 1.2 %; the exact arithmetic is above and is what the case asserts. The research figure was rounded, not wrong.

### The bin error, asserted DIRECTLY

`binError == 0.0` and `achievedCyclesPerBlock == K_m` at every rate and both edge shapes, taken from the generator's own accumulated phase over the measured block. The claim is **not** routed through `impliedLeakageDb`: its negated branch returns the −999.0 sentinel for a zero bin error, which is semantically right here only by accident — the same −999.0 comes back for silence and for a negative bin error. The case **shows** the sentinel (`CHECK(impliedLeakageDb(binError) == -999.0)`) as the reason not to use it.

### Hazard two, re-measured rather than inherited

| Rate | hard-edge `f` spread | band-limited `f` spread |
|---|---|---|
| 44.1 kHz | **0.0229828** | **0.983362** |
| 48 kHz | **0.0209424** | **0.980722** |
| 96 kHz | **0.0103627** | **0.955351** |

33-04 measured 0.004 and 0.678 on its own drives. Reproduced here in kind at every rate, and asserted (`< 0.05` and `> 0.20`), so a grid that lost the band-limited axis goes red instead of going quietly green.

## The Probe's Non-Vacuity Gate (T-33-17)

| | 44.1 kHz | 48 kHz | 96 kHz | Total |
|---|---|---|---|---|
| Cells | 140 | 140 | 140 | **420** |
| Samples compared | 573,440 | 573,440 | 573,440 | **1,720,320** |
| **Mismatches** | **0** | **0** | **0** | **0** |

Every one of the 420 cells fired at least one sync reset (`cellsWithSyncActivity == 420`), asserted before the identity claim, because two blocks of a free-running oscillator would be trivially identical and would say nothing about the sync path.

### The gate is PROVED ABLE TO FAIL

One arithmetic step perturbed in the working tree — the crossing solve `f = (1 - prev)/(now - prev)` taken from a **double** accumulator instead of the float:

| | 44.1 kHz | 48 kHz | 96 kHz | Total |
|---|---|---|---|---|
| **Mismatches** | **3862** | **2829** | **2964** | **9655** |

**183 failed assertions**, 1 failed case. Restored from a pristine copy and re-verified: 0 mismatches, whole suite green. Following 33-04's deferred item 2, the mutant build was **verified before its result was read** — `make test` was not silenced and the binary timestamp was checked.

### The seed literals, quoted for comparison against `tests/VcoBlockDriver.hpp`

```cpp
probe.seed(0x1234ULL, 0x5678ULL);
probe.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);
```

`tests/VcoBlockDriver.hpp:41-42` declares `s0 = 0x1234ULL, s1 = 0x5678ULL, sp0 = 0x9E3779B9ULL, sp1 = 0x7F4A7C15ULL`. **Identical.** Never invented: a `forge::Xoroshiro128Plus` seeded (0, 0) is a fixed point whose all-zero stream makes `std::normal_distribution`'s rejection loop never terminate.

---

# >>> HAZARD THREE — THE FINDING THAT REWROTE THE GRID <<<

**33-RESEARCH and 33-VALIDATION both recommend the master/slave ratios 0.5×, 1×, 2×, 3×, 4× and 6×. Three of those turn the measurement into nonsense.**

At an exactly integer ratio of two or more the slave is **already in phase at every master wrap**, so the reset moves it by almost nothing, the emitted signal is periodic at the **slave's** period rather than the master's, and the master's fundamental bin — the bin `aliasPeakDb` **normalises by** — carries essentially no energy.

**Measured at 44.1 kHz, `K_m = 93`, the saw centre at character 0, hard-edged master, on the shipped core's own leg:**

| ratio | \|X[K_m]\| | strongest lattice bin | at n | fundamental vs it | reported alias peak | mean \|jump\| |
|---|---|---|---|---|---|---|
| 0.5 | 3259.48 | 3259.48 | 1 | 0.00 dB | −27.24 dB | 0.999998 |
| 1.0 | 6502.19 | 6502.19 | 1 | 0.00 dB | −32.22 dB | 0.00382 |
| **2.0** | **0.81** | 6452.03 | **2** | **−78.03 dB** | **+51.87 dB** | 0.00764 |
| **3.0** | **1.16** | 6368.85 | **3** | **−74.78 dB** | **+52.25 dB** | 0.01146 |
| **4.0** | **1.70** | 6256.08 | **4** | **−71.32 dB** | **+51.41 dB** | 0.01528 |
| **6.0** | **2.59** | 5936.22 | **6** | **−67.20 dB** | **+51.29 dB** | 0.02293 |

**A POSITIVE alias peak is the tell.** It says a non-harmonic bin carries more energy than the fundamental, which cannot happen for a signal that really is master-periodic. On the first run, with the recommended ratios in place, the eight legs separated by **up to 27 dB** at those cells — and it was almost entirely the **normalisation** moving.

**The fix is the grid, not the metric.** `aliasPeakDb` is called unchanged, as the derivation requires. The integer ratios at or above two are replaced with **non-integer** values, where the reset genuinely truncates the slave mid-cycle:

| ratio | fundamental vs strongest lattice bin | reported alias peak | mean \|jump\| |
|---|---|---|---|
| 0.5 | 0.00 dB | −27.24 dB | 0.999998 |
| 0.75 | 0.00 dB | −27.24 dB | 1.5 |
| 1.0 | 0.00 dB | −32.22 dB | 0.00382 |
| 1.5 | 0.00 dB | −27.39 dB | 1.00001 |
| 2.5 | −6.36 dB | −15.62 dB | 1.00002 |
| 3.5 | −9.29 dB | −9.20 dB | 1.00004 |
| 5.5 | −12.66 dB | −1.50 dB | 1.0 |

**This is also the physically representative case.** An operator sweeping a hard-synced slave passes through the integer ratios as isolated **null points** and spends all the rest of the sweep between them. Ratio 1.0 is kept because there the fundamental is still the dominant bin so the metric is sound, and the row honestly records that unity sync barely moves the waveform (mean |jump| **0.0038** against ~1.0 either side of it).

**A permanent control pins the null point**, constructed outside `SYNC_GRID` and asserted to be exactly as unusable as the banner says (`strongestHarmonicN == 2`, `fundVsStrongestDb < −60`, `nullPeakDb > 0`, `meanAbsJump < 0.01`). If a future change ever makes an integer ratio measurable, the control turns red and says so.

### The generalisation: an instrument-validity column

`fundamentalDominanceDb` reports `20·log10(|X[K_m]| / max_n |X[n·K_m]|)` on the reference leg, so it is a property of the **cell** and not of the candidate under test. A cell is instrument-valid when the master's fundamental **is** the strongest bin on its own harmonic lattice.

| | Cells |
|---|---|
| Instrument-valid | **210** |
| Instrument-INVALID | **210** |
| Worst dominance measured | **−29.4473 dB** |

The invalid half is concentrated at the high ratios, where the slave's own harmonics overtake the master's fundamental — the same degradation as hazard three, arriving gradually instead of all at once.

---

# THE FULL MEASURED TABLE

Mean alias peak in dB per (rate × master edge shape × ratio) per leg, over the ten morph-by-character cells of each group. **Lower is better.** The past-edge column is bolded as the reference the other columns are read against — **not** as a declared winner.

| rate | master Hz | edge | ratio | none | detect | pastEdge | flatHalf | oracle | snap | misMap | badSign | step | fires | late |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 44.1k | 1001.29 | hard-edge | 0.50 | -24.81 | -24.03 | **-26.99** | -22.30 | -25.95 | -26.98 | -24.40 | -23.95 | 10/10 | 930 | 0 |
| 44.1k | 1001.29 | hard-edge | 0.75 | -28.67 | -29.17 | **-29.66** | -24.90 | -30.24 | -29.85 | -28.16 | -27.63 | 10/10 | 930 | 0 |
| 44.1k | 1001.29 | hard-edge | 1.00 | -29.83 | -29.83 | **-29.84** | -29.81 | -29.83 | -29.89 | -29.83 | -29.82 | 4/10 | 930 | 0 |
| 44.1k | 1001.29 | hard-edge | 1.50 | -23.93 | -23.98 | **-24.02** | -22.11 | -24.80 | -24.51 | -23.63 | -23.34 | 10/10 | 930 | 0 |
| 44.1k | 1001.29 | hard-edge | 2.50 | -11.92 | -11.71 | **-11.57** | -11.94 | -11.96 | -12.55 | -12.12 | -12.20 | 10/10 | 930 | 0 |
| 44.1k | 1001.29 | hard-edge | 3.50 | -5.57 | -5.13 | **-4.78** | -7.08 | -4.97 | -6.39 | -5.97 | -6.23 | 10/10 | 930 | 0 |
| 44.1k | 1001.29 | hard-edge | 5.50 | 2.14 | 3.23 | **3.85** | -1.21 | 3.40 | 1.47 | 1.21 | 0.62 | 10/10 | 930 | 0 |
| 44.1k | 1001.29 | band-limited | 0.50 | -27.55 | -26.66 | **-30.81** | -25.18 | -29.36 | -26.81 | -25.83 | -26.74 | 10/10 | 930 | 110 |
| 44.1k | 1001.29 | band-limited | 0.75 | -30.58 | -30.39 | **-33.56** | -24.79 | -32.32 | -30.65 | -28.26 | -27.86 | 10/10 | 930 | 110 |
| 44.1k | 1001.29 | band-limited | 1.00 | -39.00 | -39.01 | **-39.00** | -39.02 | -39.00 | -30.12 | -39.00 | -39.01 | 0/10 | 930 | 110 |
| 44.1k | 1001.29 | band-limited | 1.50 | -29.08 | -28.47 | **-30.17** | -25.84 | -29.64 | -25.32 | -27.07 | -27.42 | 10/10 | 930 | 110 |
| 44.1k | 1001.29 | band-limited | 2.50 | -16.83 | -16.63 | **-17.39** | -15.54 | -16.77 | -13.11 | -15.61 | -16.12 | 10/10 | 930 | 110 |
| 44.1k | 1001.29 | band-limited | 3.50 | -11.59 | -11.53 | **-11.94** | -11.49 | -11.25 | -6.75 | -10.35 | -11.21 | 10/10 | 930 | 110 |
| 44.1k | 1001.29 | band-limited | 5.50 | -4.68 | -3.63 | **-3.81** | -6.43 | -3.41 | 1.37 | -4.31 | -5.58 | 10/10 | 930 | 110 |
| 48k | 996.09 | hard-edge | 0.50 | -24.48 | -23.52 | **-26.16** | -22.15 | -25.48 | -27.21 | -24.08 | -23.66 | 10/10 | 850 | 0 |
| 48k | 996.09 | hard-edge | 0.75 | -29.26 | -29.83 | **-30.25** | -25.47 | -31.01 | -30.45 | -28.57 | -27.99 | 10/10 | 850 | 0 |
| 48k | 996.09 | hard-edge | 1.00 | -30.42 | -30.41 | **-30.40** | -30.28 | -30.44 | -30.82 | -30.39 | -30.36 | 6/10 | 850 | 0 |
| 48k | 996.09 | hard-edge | 1.50 | -23.87 | -23.86 | **-23.87** | -22.18 | -24.69 | -24.58 | -23.66 | -23.35 | 10/10 | 850 | 0 |
| 48k | 996.09 | hard-edge | 2.50 | -11.84 | -11.61 | **-11.46** | -12.07 | -11.77 | -12.60 | -12.04 | -12.14 | 10/10 | 850 | 0 |
| 48k | 996.09 | hard-edge | 3.50 | -5.28 | -4.85 | **-4.51** | -6.80 | -4.70 | -6.33 | -5.69 | -5.95 | 10/10 | 850 | 0 |
| 48k | 996.09 | hard-edge | 5.50 | 2.35 | 3.20 | **3.92** | -0.66 | 3.89 | 2.00 | 1.58 | 1.04 | 10/10 | 850 | 0 |
| 48k | 996.09 | band-limited | 0.50 | -27.56 | -26.90 | **-30.62** | -25.60 | -29.41 | -27.13 | -26.12 | -26.89 | 10/10 | 850 | 100 |
| 48k | 996.09 | band-limited | 0.75 | -31.32 | -30.93 | **-34.12** | -25.52 | -33.17 | -31.50 | -29.16 | -28.66 | 10/10 | 850 | 100 |
| 48k | 996.09 | band-limited | 1.00 | -39.60 | -39.59 | **-39.63** | -39.49 | -39.63 | -31.06 | -39.60 | -39.57 | 2/10 | 850 | 100 |
| 48k | 996.09 | band-limited | 1.50 | -29.46 | -29.70 | **-31.04** | -26.37 | -30.17 | -25.32 | -27.37 | -27.72 | 10/10 | 850 | 100 |
| 48k | 996.09 | band-limited | 2.50 | -16.66 | -16.83 | **-17.31** | -15.50 | -16.84 | -12.99 | -15.46 | -15.84 | 10/10 | 850 | 100 |
| 48k | 996.09 | band-limited | 3.50 | -11.08 | -10.81 | **-11.01** | -11.04 | -10.80 | -6.42 | -10.41 | -10.99 | 10/10 | 850 | 100 |
| 48k | 996.09 | band-limited | 5.50 | -4.74 | -3.71 | **-3.65** | -6.62 | -3.44 | 1.94 | -4.29 | -5.37 | 10/10 | 850 | 100 |
| 96k | 1007.81 | hard-edge | 0.50 | -29.62 | -27.95 | **-31.11** | -27.24 | -30.81 | -32.30 | -29.28 | -28.84 | 10/10 | 430 | 0 |
| 96k | 1007.81 | hard-edge | 0.75 | -35.38 | -35.89 | **-36.31** | -31.30 | -37.29 | -36.41 | -34.63 | -33.92 | 10/10 | 430 | 0 |
| 96k | 1007.81 | hard-edge | 1.00 | -36.34 | -36.32 | **-36.31** | -36.31 | -36.34 | -36.90 | -36.35 | -36.36 | 5/10 | 430 | 0 |
| 96k | 1007.81 | hard-edge | 1.50 | -29.62 | -29.66 | **-29.70** | -27.55 | -30.36 | -30.02 | -29.38 | -29.01 | 10/10 | 430 | 0 |
| 96k | 1007.81 | hard-edge | 2.50 | -17.07 | -17.00 | **-16.94** | -16.86 | -17.25 | -17.54 | -17.13 | -17.16 | 10/10 | 430 | 0 |
| 96k | 1007.81 | hard-edge | 3.50 | -10.47 | -10.29 | **-10.14** | -11.16 | -10.26 | -10.96 | -10.64 | -10.74 | 10/10 | 430 | 0 |
| 96k | 1007.81 | hard-edge | 5.50 | -2.18 | -1.75 | **-1.40** | -3.98 | -1.29 | -3.13 | -2.60 | -2.90 | 10/10 | 430 | 0 |
| 96k | 1007.81 | band-limited | 0.50 | -33.01 | -31.58 | **-36.05** | -31.06 | -35.00 | -32.06 | -31.73 | -32.28 | 10/10 | 430 | 50 |
| 96k | 1007.81 | band-limited | 0.75 | -36.95 | -36.71 | **-39.75** | -31.15 | -39.11 | -36.89 | -34.92 | -34.26 | 10/10 | 430 | 50 |
| 96k | 1007.81 | band-limited | 1.00 | -45.39 | -45.39 | **-45.41** | -45.31 | -45.41 | -37.03 | -45.38 | -45.36 | 1/10 | 430 | 50 |
| 96k | 1007.81 | band-limited | 1.50 | -34.50 | -34.46 | **-36.00** | -31.34 | -35.37 | -30.71 | -33.04 | -32.89 | 10/10 | 430 | 50 |
| 96k | 1007.81 | band-limited | 2.50 | -23.78 | -23.73 | **-24.64** | -21.65 | -24.28 | -18.04 | -22.37 | -22.63 | 10/10 | 430 | 50 |
| 96k | 1007.81 | band-limited | 3.50 | -16.93 | -16.75 | **-17.41** | -15.82 | -17.16 | -11.24 | -16.19 | -16.29 | 10/10 | 430 | 50 |
| 96k | 1007.81 | band-limited | 5.50 | -9.28 | -9.30 | **-9.13** | -9.77 | -8.91 | -3.17 | -8.70 | -9.31 | 10/10 | 430 | 50 |

**42 groups, 420 cells, 3,360 measurements. `late` is the number of resets that landed on a sample the master did NOT wrap on** — a placement error that exists **before any seam does** and must not be attributed to one. Grid-wide: **1,820 late fires out of 30,940** (5.88 %), all of them on band-limited masters, none on hard-edge ones.

---

# THE PLATEAU / STEP-DOMINATED CLASSIFICATION

**The criterion, stated on its physical basis in the case comment BEFORE any population is enumerated:** `aliasPeakDb` reports an arg-max over roughly two thousand non-harmonic bins. When the emitted waveform carries a **true value step**, that step's spectrum is a broad 1/f skirt and the arg-max is a **genuine maximum** — 1.0 dB bound. When there is no true value step, the surviving energy is a near-flat **plateau** of near-tied bins that one libm ULP reorders — 4.0 dB bound.

**The observable:** under hard sync the value step in question **is the sync jump**, which is exactly what `forge::VcoCore::Telemetry::syncJump` records. A cell is step-dominated when its mean absolute sync jump on the shipped core's own leg is at least **0.01** in pre-scale units (0.05 V at the output).

**The population, counted AFTER the criterion was fixed:**

| Class | Cells |
|---|---|
| Step-dominated | **378** |
| Plateau | **42** |
| Total | **420** |

The 42 plateau cells are concentrated at ratio 1.0, where unity sync barely moves the waveform — which the table's `step` column shows directly (0/10 to 6/10 at ratio 1.0, 10/10 everywhere else).

---

# >>> THE DECISION: STOP AND REPORT <<<

**ALL THREE CONDITIONS FAIL. NO WINNER IS DECLARED BY THE D-06 RULE.**

The conditions are evaluated on the **step-dominated, instrument-valid 44.1 kHz** population (54 cells), for the reason stated with the criterion: a cell with no value step poses no placement question, and a cell where the metric divides by something that is not the fundamental cannot rank anything. **The unrestricted figures are recorded alongside so the restriction hides nothing.**

## Condition 1 — SIGN CONSISTENCY: **FAIL** on its first clause, **PASS** on its second

| candidate | wins (all 140) | frac | wins (step-dom 124) | frac | wins (valid+step 54) | frac | worst deficit |
|---|---|---|---|---|---|---|---|
| `none` | 20 | 0.1429 | 16 | 0.1290 | 10 | 0.1852 | **3.9259 dB** |
| `detect` | 6 | 0.0429 | 2 | 0.0161 | **0** | **0.0000** | **5.0518 dB** |
| **`pastEdge`** | **66** | **0.4714** | **55** | **0.4435** | **34** | **0.6296** | **0.8553 dB** |
| `flatHalf` | 60 | 0.4286 | 51 | 0.4113 | 10 | 0.1852 | **10.4567 dB** |

- **First clause FAILS:** the best candidate is `pastEdge` at **0.6296**, short of the 0.90 the rule demands.
- **Second clause PASSES, and it is the durable half:** `pastEdge`'s worst single-cell deficit against the best other candidate is **0.8553 dB — INSIDE register item 8's 1.0 dB step-dominated reproduction bound.** Every other candidate's worst deficit is **outside** it. `pastEdge` is the only candidate that is never materially worse than anything else, anywhere on the valid grid.

## Condition 2 — MARGIN ABOVE THE REPRODUCTION BOUND: **FAIL**

Evaluated on the sub-unity cells at 44.1 kHz, margin = (best other candidate) − (`pastEdge`), positive = `pastEdge` better.

| population | cells | over the bound | min | mean | max |
|---|---|---|---|---|---|
| step-dominated | 40 | 22 | **−0.5025 dB** | **1.4565 dB** | **3.4226 dB** |
| step-dominated **and** instrument-valid | 38 | 22 | **−0.5025 dB** | **1.5056 dB** | **3.4226 dB** |

**22 of 38 clear the bound, not all of them.** The rule requires the margin to exceed the applicable bound on *at least* the sub-unity cells; 58 % is not that. And the minimum is **negative** — there are sub-unity cells where `pastEdge` is beaten. **A decision resting on this population is not defensible cross-toolchain and MUST NOT BE TAKEN.**

## Condition 3 — RATE SIGNATURE: **FAIL — the margin is FLAT**

Mean margin over the cells at or above unity ratio, `pastEdge` against the best other candidate:

| population | 44.1 kHz | 48 kHz | 96 kHz | shrinks? |
|---|---|---|---|---|
| step-dominated | −1.3218 | −1.2773 | −0.4428 | **no** |
| step-dominated **and** instrument-valid | **0.0688** | **−0.1357** | **0.1234** | **no** |

**And on the single common cell** — band-limited master, ratio 2.5, saw centre, character 0.00, the same cell at all three rates:

| 44.1 kHz | 48 kHz | 96 kHz | spread |
|---|---|---|---|
| **0.8968 dB** | **0.9014 dB** | **0.8487 dB** | **0.0527 dB** |

**A 0.05 dB spread across a factor of 2.2 in sample rate.** Condition 3's own wording is what this means: *"a margin that is FLAT across rates means the legs differ in jump MAGNITUDE, not in placement, and the measurement has not answered D-06 — stop and report rather than picking."*

**That reading holds up physically and is not an excuse.** `pastEdge` and `flatHalf` differ by a factor of `f²`, which **is** a magnitude difference. A pure one-sample-placement signature was never going to be the thing that separated them. The only pair that differs purely in placement is `pastEdge` against `detect`, and `detect` is eliminated on condition 1's own evidence (0 wins of 54, worst deficit 5.05 dB, and worse than doing nothing).

---

# THE RECOMMENDATION FOR PLAN 33-06

> **THIS IS A RECOMMENDATION ON THE EVIDENCE. IT IS NOT A RULE-SANCTIONED DECISION, AND A LATER AGENT MUST NOT PROMOTE IT TO ONE BY DELETING THIS SENTENCE.** The three-condition rule refused, and the refusal is recorded above with its figures. What follows is what the measurement *does* support, stated so plan 33-06 has something to implement rather than nothing.

**The leg: PAST-EDGE.** The exact seam call, at the `SYNC JUMP COMPLETION` line in `src/dsp/VcoCore.hpp` (the line 33-02's source already directs 33-06 to), inside the `syncFired` condition and **ahead of** the single `blep.step` call:

```cpp
blep.addStep(0.f, -tel.syncFrac * tel.syncFrac * tel.syncJump);
```

**HEADER CHANGE REQUIRED: NONE.** The algebraic identity is written into the probe's banner and holds exactly:

```
addStep(0.f, -f*f*h)
  u        = 1 - 0 = 1
  inject  += (-f*f*h) *  0.5 * u * u  =  -h*f^2/2      <- exactly the past-edge residual
  pending += (-f*f*h) * -0.5 * 0 * 0  =  0             <- nothing owed forward
```

At forward position zero the forward-owed term vanishes identically and the current-sample term **is** the past-edge residual. The entry gate passes on its own terms (`0 >= 0` true, `0 > 1` false), so the documented `[0,1]` contract is **honoured, not reinterpreted**, and the finiteness clause still applies to the pre-scaled jump. `MorphBlep.hpp` is untouched — which matters, because `addPastStep` is currently listed among the symbols this phase creates and **this measurement says it is not needed**. Whether to add it anyway *on legibility grounds* is 33-06's call and is orthogonal to this result; the two forms are numerically identical.

**Why past-edge and not one of the others, in one line each:**

| leg | verdict |
|---|---|
| `pastEdge` | **The only candidate whose worst-case deficit (0.8553 dB) is inside the 1.0 dB reproduction bound.** Best on 34 of 54 valid step-dominated cells. Beats `none` by 1.00–1.22 dB on band-limited masters at all three rates. |
| `none` | Loses to `pastEdge` by 1.00–1.22 dB on band-limited masters. Worst deficit 3.93 dB. Doing nothing is measurably worse than the past-edge correction wherever the fraction carries information. |
| `detect` | **Eliminated.** 0 wins of 54, worst deficit 5.05 dB, and worse than `none` — confirming the research's prediction that a step-shaped correction on the wrong side of a step is new broadband energy, not a filter. |
| `flatHalf` | **Eliminated on variance.** It wins some high-ratio hard-edge cells outright, but its worst deficit is **10.4567 dB** — an order of magnitude outside the bound. A leg that is sometimes best and sometimes ten decibels worst is not a convention. |

**Two things 33-06 must carry across from this measurement:**

1. **The `tel.syncCorrection` reconstruction relationship holds under this placement and only under it.** `src/dsp/VcoCore.hpp` already says so: under the past-edge placement the correction deposits **nothing** into `pending`, so `leg_none[n] == leg_full[n] - 5.f * syncCorrection[n]` is exact per sample. Under `detect` or `flatHalf` it is not. 33-06 must **re-state** that relationship against the leg it lands, as the header instructs.
2. **Plan 33-07 must RE-ANCHOR the bit-exactness gate.** The moment 33-06 lands the seam, `forge::VcoCore` stops being measurement leg `none` and the gate's `kLegNone` argument stops describing it. The correct repair is to change the leg argument and **keep the equality exact**; the tempting repair — loosening the equality — deletes the gate. This is written into the case as well as here.

---

# WHAT ELSE THE GRID BOUGHT — "IT PAYS FOR ITSELF TWICE"

All figures are mean dB against the past-edge leg over the 70 cells of each (rate × edge) group. **Positive = worse than past-edge.**

| rate | edge | snap − pastEdge | oracle − pastEdge | none − pastEdge | misMap − pastEdge | badSign − pastEdge |
|---|---|---|---|---|---|---|
| 44.1 kHz | hard-edge | **−0.813** | −0.195 | +0.061 | +0.015 | +0.064 |
| 44.1 kHz | band-limited | **+5.040** | **+0.706** | **+1.053** | **+2.322** | **+1.821** |
| 48 kHz | hard-edge | **−1.040** | −0.210 | −0.010 | −0.017 | +0.046 |
| 48 kHz | band-limited | **+4.985** | **+0.560** | **+0.996** | **+2.138** | **+1.761** |
| 96 kHz | hard-edge | **−0.766** | −0.242 | +0.174 | +0.272 | +0.423 |
| 96 kHz | band-limited | **+5.610** | **+0.452** | **+1.222** | **+2.296** | **+2.196** |

## 1. The snap-to-zero landmine, measured — and it cuts both ways

**On band-limited masters the snap leg is 4.99 to 5.61 dB WORSE at all three rates.** That is SYNC-02's sub-sample clause turned from an inherited warning into evidence with a comfortable margin, and it lands within a decibel of 33-RESEARCH's prototype prediction of 4.5–4.95 dB. It is the **only** sync claim on this grid that clears both reproduction bounds at every rate, which is exactly what the research said it would be. The sign is asserted permanently; the magnitude is recorded and deliberately **not** gated (33-07 owns that).

**On hard-edge masters the snap leg is 0.77 to 1.04 dB BETTER.** This is hazard two arriving as a number, and it is worth stating plainly: with a single-sample master wrap there is **no sub-sample information to preserve**, the detector's fraction is a near-constant ≈0.6 that has nothing to do with the true crossing, and resetting to a "sub-sample" overshoot built from that fraction is worse than snapping to zero. 33-RESEARCH predicted the two would measure *identically* there; measured, snap is about a decibel **better**. **The sub-sample reset is a win only when the master is band-limited.** That is not an argument against it — a band-limited master is what another Forge VCO produces — but it must not be claimed generally.

## 2. The oracle — a falsified premise, and the reason is worth more than the correction

**The oracle leg, which uses the master generator's TRUE wrap fraction, is 0.45 to 0.71 dB WORSE than the detector's own fraction on band-limited masters.**

That inverts the expected decomposition. The mechanism is measured elsewhere in the same run: on a band-limited master the residual can push the wrap sample below the high threshold, so the detector fires **one sample late** — **1,820 of 30,940 resets, 5.88 %, all of them on band-limited masters**. The reset that actually happened is the one the detector's `f` describes. Feeding the correction the *physically true* `g` sizes it for an edge that is one sample further back than the reset it is correcting, and the mismatch costs more than the fraction error it removes.

**The finding, stated for the phases that inherit it:** *the fraction that matters is the one CONSISTENT WITH THE RESET, not the physically true one.* On hard-edge masters the oracle is 0.19–0.24 dB better, which is the small, honest size of the fraction-accuracy term when the detector is not firing late.

**The decomposition D-06 asked for, on band-limited masters:**

| quantity | 44.1 kHz | 48 kHz | 96 kHz | reading |
|---|---|---|---|---|
| cost of the **placement convention** (`none` − `pastEdge`) | 1.053 | 0.996 | 1.222 | about **1 dB**, stable across rates |
| cost of the **fraction's accuracy** (`oracle` − `pastEdge`) | +0.706 | +0.560 | +0.452 | **negative value** — a perfect fraction would make things worse |

**A later phase should NOT escalate to a slope-correction kernel to improve the fraction.** The measurement says the fraction is not the binding term; the late-fire detector is. If anything is worth a later phase's attention it is the **detection threshold under a band-limited master**, not the interpolation.

## 3. The mutation probes — and one of them measured an axis rather than failing

| probe | grid-wide mean vs `pastEdge` | cells worse | band-limited | hard-edge | verdict |
|---|---|---|---|---|---|
| `misMap` = `addStep(1−f, jump)` | **+1.171 dB** | 254 / 420 | **+2.14 to +2.32 dB** | −0.02 to +0.27 dB | **DISCRIMINATES** on band-limited masters; does **not** on hard-edge ones |
| `badSign` = jump as `before − after` | **+1.052 dB** | 258 / 420 | **+1.76 to +2.20 dB** | +0.05 to +0.42 dB | **DISCRIMINATES** on band-limited masters; does **not** on hard-edge ones |

Both probes separate cleanly and by more than the step-dominated bound wherever the detector's fraction carries information, and both collapse to noise where it does not. **That is not a probe failing — it is the probe measuring hazard two from a third direction.** The sign is asserted permanently on the band-limited half; the hard-edge half is deliberately not asserted, and its absence is documented in the case as the finding rather than as an omission.

`badSign` is `.planning/research/STACK.md:124`'s expression transcribed verbatim. It costs **1.76 to 2.20 dB** on a band-limited master. The sign warning in `src/dsp/VcoCore.hpp` now has a price on it.

## 4. D-07's residual phantom — 33-02's deferred item 1, discharged with a number

`src/dsp/VcoCore.hpp` names the phantom and gives an order of magnitude explicitly labelled **arithmetic, not measurement**, and asks this plan for the measurement. Measured as the magnitude of `MorphBlep`'s carried accumulator **on entry to a reset sample**, on the shipped core's own leg:

| | value |
|---|---|
| Reset samples observed | **30,940** |
| Mean \|pending\| carried in | **0.0568771** |
| Max \|pending\| carried in | **0.9623710** |

Per-group means run 0.000–0.220 and per-group maxima 0.000–0.962 (the table's `phantomMean` / `phMax` columns in the `-s` log). **The maximum is very nearly a full-scale residual** — 0.96 in the same pre-scale units the naive sample lives in — which is a good deal larger than the header's "order of one phantom site every few sync events" reads. The mean is small. **A later phase now inherits both numbers instead of a paragraph of arithmetic.**

Note the ratio-0.75 rows measure a phantom of exactly **0.000** at every rate and both edge shapes — those are cells where the reset never lands within one increment of a live site.

---

# THE PREDICTION, SCORED HONESTLY

33-RESEARCH's in-session prototype over 135 measurements predicted three things. The prototype carried **one** discontinuity site against this run's nine, so its **ranking** was the prediction and its **decibels** were explicitly not transferable.

| prediction | outcome |
|---|---|
| **`b` (past-edge) wins every cell** | **FALSIFIED.** It wins 34 of 54 valid step-dominated 44.1 kHz cells (63 %). What survives is the weaker and more useful claim: it is the only candidate never materially worse than any other. |
| **`a` (detect) is worse than applying no correction at all in most cells** | **CONFIRMED, emphatically.** `detect` wins **0** of 54; `none` wins 10. `detect`'s worst deficit is 5.05 dB against `none`'s 3.93. |
| **The rate signature shrinks by roughly half from 44.1 to 96 kHz** | **FALSIFIED.** The margin is FLAT: 0.897 / 0.901 / 0.849 dB on the common cell, a 0.05 dB spread. This is condition 3's failure and it is the reason no winner is declared. |
| **`snap` vs `b` at 4.5–4.95 dB** | **CONFIRMED and slightly exceeded** on band-limited masters: 4.99 / 4.99 / 5.61 dB. |
| **`snap` and `b` measure identically on a hard-jump master** | **FALSIFIED in direction.** Snap measures 0.77–1.04 dB **better**, not identically. |
| **`oracle` is 3.5–5.5 dB better than `b` on a hard-jump master** | **FALSIFIED in magnitude.** Measured 0.19–0.24 dB better. On a band-limited master the prototype said the two coincide to ≈0.1 dB; measured, the oracle is 0.45–0.71 dB **worse**. |

**Two of six confirmed, four falsified.** The prototype's `a`-is-worse-than-nothing result is the one that transferred, and it is the one the phase most needed.

---

# Gate Results

| Gate | Result |
|------|--------|
| `make test` | **PASS** — 103 cases, 2,624,784 assertions, 0 failures |
| `make strict` (C++11, `-pedantic-errors`) | **PASS**, exit 0 |
| `make guards` | **PASS**, exit 0 |
| `bash tests/check_frozen.sh` | **PASS** — D-05 manifest + golden fixtures + negative control |
| Six shipped-LFO goldens | **byte-identical** — 9 cases / 49,188 assertions, 0 failures |
| Compiler warnings in the changed TU | **0** (`-Wall -Wextra`) |
| `git diff --stat tests/check_includes.sh` | **empty** — no new translation unit, no `VCO_SIDE_ALLOW` entry incurred |
| `src/AnalogLFO.cpp` in the whole-plan diff | **absent** |
| `git diff --name-only` across all three commits | **`tests/test_vco_spectrum.cpp` alone** |
| `grep -c 'struct SyncPlacementProbe'` | **1** — no second core-shaped struct for sync |
| `grep -c '^double aliasPeakDb'` | **1** definition; **16** total occurrences (definition, banner mentions and call sites) — **the metric was not forked** |
| `grep -n 'binError == 0.0'` | **1**, inside the new apparatus, and not via the leakage helper |

---

# Decisions Made

1. **NO WINNER IS DECLARED BY THE D-06 RULE, and the refusal is the deliverable.** All three conditions fail. The plan's own instruction is unambiguous — *"if any condition fails, the measurement STOPS AND REPORTS rather than picking a winner"* — and the acceptance criteria require a STOP-AND-REPORT section with no winner declared. That is what this SUMMARY carries. Repairing the rule until it produced a winner would have been the one move the whole three-condition apparatus exists to prevent.

2. **A RECOMMENDATION is recorded anyway, and is labelled in the source AND here as not rule-sanctioned.** The other acceptance criterion requires the decision written in a form 33-06 can implement without re-reading the table. Those two criteria are in tension when the rule refuses, and the honest resolution is to satisfy both explicitly: the refusal is the finding, the recommendation is the evidence-based next step, and neither is dressed as the other. The case comment carries the same paragraph with an explicit prohibition on a later agent deleting it.

3. **The grid's ratio axis departs from both source documents, because three of the six recommended values are null points.** See hazard three above. The metric is untouched, as the derivation requires; the grid moved. The null point is pinned by a permanent control so the recommendation cannot be quietly reverted.

4. **The three conditions are evaluated on the step-dominated, instrument-valid population — with the unrestricted figures recorded alongside.** The physical criterion was fixed in the case comment before any count. A cell with no value step poses no placement question; a cell where the fundamental is not the strongest bin on its own lattice cannot rank anything. Both restrictions follow from register item 8's own logic rather than from which cells failed, and reporting all three populations is what makes that checkable.

5. **The grid is enumerated by a builder rather than spelled out, and 33-07 is warned in source.** `SPECTRUM_GRID` is spelled out because every row carries a pinned number with per-row provenance. This grid pins nothing, so enumerating a five-axis cross product makes the axes auditable and the cell count mechanical. **The moment 33-07 pins a per-cell threshold, that number needs a per-cell home** — the builder must grow a lookup, not a formula.

6. **`driveSecondBlock` gained one defaulted `master` pointer instead of a second sync-aware loop.** The file's banner forbids forking it, and the reason binds harder here than anywhere: this plan's central claim is that the probe **is** `forge::VcoCore`, and a comparison whose two sides ran different loops could not support that claim at all. A null `master` leaves the body byte-for-byte what it was, and `syncConnected` defaults to false, so no pre-existing caller can reach the sync block. Verified by re-running the whole suite across the change: no recorded Phase 32 figure moved.

7. **SYNC-02 is DECLINED — the eighth consecutive decline in this project's history of them.** This plan adds no shipped code at all; its entire diff is one test file. The seam does not exist (non-comment `addStep` count in `src/dsp/VcoCore.hpp` is still **0**), so the *click-free* half of SYNC-02 has nothing behind it. Marking it here would book a click-free claim on a reset that is still un-band-limited, and would do so in the plan that just measured how much that matters.

8. **The oracle leg takes a HELD wrap fraction, not a per-sample one.** 5.88 % of resets land on a sample the master did not wrap on. A strictly per-sample `g` would be absent exactly where the oracle is most needed. The lateness is not swept under that rug — it is a reported column, and it turned out to be the mechanism behind the oracle's own falsification.

---

# Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] Three of the six recommended master/slave ratios make the metric divide by a bin 78 dB down**

- **Found during:** Task 3, on the first full run
- **Issue:** 33-RESEARCH § Grid and 33-VALIDATION § Sync Sub-Grid Construction both recommend ratios 0.5×, 1×, 2×, 3×, 4×, 6×. At an exactly integer ratio of two or more the slave is already in phase at every master wrap, the reset is a near-no-op, the signal is periodic at the **slave's** period, and `|X[K_m]|` — the bin `aliasPeakDb` normalises by — sits **67 to 78 dB** below the strongest lattice bin. The reported figures went **positive** (+51.3 to +52.3 dB) and the eight legs separated by up to 27 dB in what was almost entirely the normalisation moving. Half the grid was measuring nothing.
- **Fix:** Replaced the integer ratios at or above two with non-integer ones (`0.5, 0.75, 1.0, 1.5, 2.5, 3.5, 5.5`). `aliasPeakDb` is **untouched** — the derivation requires it called unchanged, and the defect is in the stimulus, not the classifier. The measured evidence table is written into the `SYNC_RATIOS` banner, a permanent control pins the null point, and `fundamentalDominanceDb` generalises it into a per-cell validity column. Task 1's axis assertions gained an `integerRatioCellsAtOrAboveTwo == 0` clause.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** The two tables under "Hazard three" above; grid-wide 210 of 420 cells instrument-valid, worst dominance −29.45 dB.
- **Committed in:** `cc34df5`

**2. [Rule 3 — Blocking] The ratio change invalidated cell counts already committed in Tasks 1 and 2**

- **Found during:** Task 3
- **Issue:** Tasks 1 and 2 landed with `CHECK(nCells == 360)`, `subUnityCells == 60`, `hardEdgeCells == 180`, `tally[r].cells == 120` and so on. The corrected ratio axis makes the grid 420 cells, so those already-committed assertions were false.
- **Fix:** Updated in the Task 3 commit rather than by amending history: 420 cells, 120 sub-unity, 210 per edge shape, 140 per rate. The counts remain **exact equalities**, never loosened to inequalities — a count assertion softened to `>=` would stop being able to detect a grid that silently lost cells, which is the whole reason it is there.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** All three cases green at the new counts; 1,720,320 samples compared in the gate against the previous 1,474,560.
- **Committed in:** `cc34df5`

**3. [Rule 1 — Bug] The `thresholdDb` acceptance criterion counts three additions, not one**

- **Found during:** Task 3, checking acceptance
- **Issue:** The criterion is *"`grep -c 'thresholdDb'` equals its pre-plan value plus only the `SyncCell` field declaration"*. Measured: **17 before, 20 after** — plus three, not one. The three are the field declaration, the builder line that populates it with the unpinned sentinel, and the Task 1 assertion that **every cell is unpinned**. A field that is declared and never assigned is not a field, and the assertion is the criterion's own prose (*"no threshold column was pinned"*) enforced mechanically.
- **Fix:** Reported rather than adjusted, and both numbers recorded. The **prose** is satisfied exactly: no row carries a pinned decibel, every row carries `kSyncUnpinnedDb` and `kProvSyncUnpinned`, and the `tier` is `"measure"`, never `"gated"`. This is the **eighth** instance in this project of a gate mechanism narrower than the prose beside it, after the five 33-02 catalogued, the one 33-03 added and the one 33-04 added.
- **Files modified:** none — a criterion-interpretation decision
- **Verification:** `git diff 6a67c34 -- tests/test_vco_spectrum.cpp | grep '^+.*thresholdDb'` returns exactly those three lines, none of them a number.
- **Committed in:** n/a (recorded here and in `cc34df5`'s message)

**4. [Rule 2 — Missing critical correctness] The three conditions needed an instrument-validity population, or they would rank cells the metric cannot rank**

- **Found during:** Task 3
- **Issue:** The plan specifies the conditions over "the 44.1 kHz cells" and "the sub-unity-ratio cells". After hazard three it was clear that fundamental-dominance degrades **gradually** with ratio, not only at the integer null points — so the specified populations still contained cells where `aliasPeakDb` divides by a bin that is not the fundamental. Ranking four candidates there ranks the normalisation.
- **Fix:** Added `fundamentalDominanceDb` as a per-cell validity column, computed on the **reference leg** so it is a property of the cell and not of the candidate under test, and evaluated the conditions on the step-dominated **and** instrument-valid population. **All three populations are reported** — all 140, step-dominated 124, valid+step 54 — so the restriction cannot hide a result. It changes the outcome in only one direction and not the verdict: `pastEdge`'s share rises from 0.4435 to 0.6296 and its worst deficit falls from 15.9112 dB to 0.8553 dB, and all three conditions still FAIL.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** The condition tables above carry both populations side by side.
- **Committed in:** `cc34df5`

**5. [Rule 1 — Bug] The common-cell rate-signature lookup named a ratio the corrected grid no longer contains**

- **Found during:** Task 3
- **Issue:** The three-rates-on-one-cell lookup selected `ratio == 2.0`, which after the ratio fix matches nothing; `REQUIRE(commonFound == 3)` went red.
- **Fix:** Moved to ratio 2.5, with the reason (2.0 is an integer null point) written beside it and pointing at the null-point control. Caught by a `REQUIRE` that was written to bind before the values were read — the criterion-first habit working as intended.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** `commonFound == 3`; margins 0.8968 / 0.9014 / 0.8487 dB.
- **Committed in:** `cc34df5`

**6. [Rule 3 — Blocking] Two constants added a commit early would have been a `-Wunused-const-variable` warning**

- **Found during:** Task 2
- **Issue:** `kSyncLegCount` and `SYNC_LEG_NAME` are Task 3's consumers, and landing them in Task 2 produced a warning in a suite whose standing figure is zero.
- **Fix:** Deferred to the commit that first consumes them, with a comment recording the precedent — `tests/test_vco_core.cpp` deferred `makeMasterSawBandLimited` the same way in plan 33-04 for the same reason.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** `-Wall -Wextra` warnings in the changed TU: **0**, on every one of the three commits.
- **Committed in:** `9cb0f89`

---

**Total deviations:** 6 auto-fixed (3 × Rule 1, 2 × Rule 3, 1 × Rule 2)
**Impact on plan:** All six served the plan's own stated goals. One found and repaired a defect that had silently invalidated half the grid and that neither source document anticipated; one propagated that repair honestly through assertions already committed; one refused to report a criterion as met when the mechanism counts differently from the prose; one gave the decision rule a population it can legitimately rank, and reported all three populations so the restriction is auditable; two were mechanical. **No scope creep** — the whole-plan diff is `tests/test_vco_spectrum.cpp` alone.

---

# Known Stubs

**None.** Every helper this plan adds is consumed by an assertion in the same commit.

Two things are *absent by design* and belong to named later plans:

| Absence | Owner | Why it is not a stub here |
|---|---|---|
| Both decibel columns of `SYNC_GRID` are the unpinned sentinel | **plan 33-07** | This plan's stated output is a decision, not a gate. A threshold pinned in the same commit that chose the leg would be pinned from a leg no gate had yet examined. Every cell carries `kProvSyncUnpinned`, which says exactly this, and Task 1 asserts all 420 of them do. |
| No seam call in `src/dsp/VcoCore.hpp` | **plan 33-06** | The withholding is the precondition this plan was written against; non-comment `addStep` count is still **0**. |

---

# Deferred Register Items

Recorded here so plan 33-11 files them with a Resolve-at.

**1. NEW — the D-06 three-condition rule has no defined behaviour when it refuses.**
The rule as written produces either a winner or a stop-and-report, and the phase plan assumes the former: 33-06 is scheduled to "implement the decision 33-05 records". It refused. This plan discharges the gap with a clearly-labelled recommendation, but the *process* question is open — **who decides when a measurement declines to?** The candidates are an operator decision at 33-12's UAT, a second instrument (D-10's time-domain one, which register item 5 says the spectral gate is structurally blind to the click SC-3 forbids), or accepting the recommendation on its condition-1-second-clause evidence.
**Proposed Resolve-at:** plan 33-06, as the first thing it states before writing a line of seam code; escalate to the operator if it is not comfortable proceeding on a recommendation.

**2. NEW — the spectral instrument cannot rank hard-edge-master cells, and half the grid is invalid.**
210 of 420 cells fail the fundamental-dominance check, and separately, every leg measures within about a decibel of every other on hard-edge masters at any ratio. **The spectral grid is only informative where the master is band-limited.** That is not a defect to fix here — it is the same conclusion register item 5 reached about single-sample spikes reading 0.0 dB spectrally — but it means the sync BLEP's non-circular evidence lives in D-10's time-domain instrument, and this measurement has now put a second number on why.
**Proposed Resolve-at:** plan 33-08, which owns the time-domain instrument and the tighter musical tier for sync.

**3. NEW — the late-fire rate under a band-limited master is 5.88 %, and it is the binding error term, not the interpolation.**
1,820 of 30,940 resets landed on a sample the master did not wrap on, all of them on band-limited masters. The oracle measurement shows this dominates the fraction-accuracy term — a perfect fraction makes things **worse**, by 0.45–0.71 dB, because it is sized for an edge the reset did not correspond to. **Any future work aimed at improving sub-sample accuracy should target the DETECTION THRESHOLD under a band-limited master, not the interpolation.** The BLAMP escalation (register item 9) is specifically **not** indicated by this measurement.
**Proposed Resolve-at:** no code change in v2.0. Re-open only if a later phase conditions `syncVolts` before it reaches the core.

**4. CLOSED — 33-02's deferred item 1 (the residual `pending` phantom) now has a measured number.**
Mean 0.0569, max 0.9624 over 30,940 reset samples. The maximum is much larger than the header's arithmetic estimate suggested. The **disposition is unchanged** (accept and document — `MorphBlep`'s accumulator is a scalar sum and per-site cancellation needs a restructure out of proportion to the effect), but the register entry should now carry the measurement rather than the arithmetic.
**Proposed Resolve-at:** unchanged — the first phase that restructures `MorphBlep` for per-site accounting, or v2.1's oversampling work.

**5. CARRIED — 33-RESEARCH's grid recommendation and 33-VALIDATION's Threshold Policy both contain the falsified ratio set.**
Neither document is edited by this plan (both are outside its single-file scope). The corrected evidence lives in `tests/test_vco_spectrum.cpp`'s `SYNC_RATIOS` banner and here.
**Proposed Resolve-at:** plan 33-11, when it reconciles the phase's documents.

---

# Issues Encountered

- **The measurement refused, and that is uncomfortable but correct.** A large amount of apparatus was built to answer a question that the apparatus then declined to answer. The temptation to relax one clause of one condition was real and is exactly what register item 8 and the file's STOP-AND-REPORT instruction exist to resist. The condition-1 second clause — `pastEdge` is the only candidate whose worst-case deficit is inside the reproduction bound — is a genuinely strong result and it is *not* the result the rule asked for.
- **Half the grid runtime measures cells that cannot rank anything.** The 210 instrument-invalid cells are still measured, still bit-exactness-checked and still reported. They were kept because the validity column is a *measurement*, and a column that only ran on cells already believed valid would be a self-check that cannot fail. Total added runtime is about 2 seconds.
- **T-33-08 (toolchain divergence) is not discharged locally**, unchanged from 33-01/33-02/33-03/33-04. This plan adds **no shipped code at all** — its whole diff is one test file — so its exposure is nil, but every decibel above is an **Apple-clang** figure and register item 8 binds every one of them. The CI MinGW leg remains plan 33-11's.
- **Two untracked `.planning/research/.cache/*.json` files** pre-existed at session start, as they did for 33-02, 33-03 and 33-04, and were left alone.

---

# Next Phase Readiness

**The measurement is complete, the instrument is validated, and the phase's central question has an evidence-based answer that the formal rule declines to certify.**

- **Plan 33-06** should read the RECOMMENDATION section first, then deferred item 1. It has an exact seam call, a proof that no header change is needed, and an explicit statement that `addPastStep` is **not required** by the measurement. It must also re-state `tel.syncCorrection`'s reconstruction relationship against the leg it lands, as `src/dsp/VcoCore.hpp` already instructs.
- **Plan 33-07** inherits three obligations: **RE-ANCHOR the bit-exactness gate** to the leg 33-06 landed (loosening the equality deletes the gate); pin the two decibel columns, which means growing `buildSyncGrid` a per-cell lookup rather than a formula; and decide whether to gate the instrument-invalid half at all.
- **Plan 33-08** owns the time-domain instrument, and deferred item 2 says why it matters more than this plan's own instrument for the click claim. It also inherits the snap-versus-past-edge figures (4.99–5.61 dB band-limited) as the one spectral sync claim with a comfortable margin.
- **Plan 33-11** inherits register items 1, 2, 3 and 5 above, item 4 as closed-with-a-number, and 33-02/33-03/33-04's six.
- **Plan 33-12** owns the operator UAT, and deferred item 1 names it as one of the three ways the refused decision could be closed.

**Concerns carried forward:**

- **The strongest one is deferred item 1.** 33-06 is scheduled to implement a decision that was not formally taken. It should say so out loud before it starts.
- **Every decibel in this SUMMARY is an Apple-clang figure**, and register item 8's reproduction bounds are the reason condition 2 refused. Nothing here survives a toolchain crossing on its own.
- **The spectral instrument is blind on hard-edge masters and on 210 of 420 cells.** Read any hard-edge row above as "no information", not as "no difference".
- **SYNC-02 has five further contributing plans.** The seam is still absent from the source.

---

# Self-Check: PASSED

Verified against disk and git rather than asserted:

- **Files exist:** `tests/test_vco_spectrum.cpp`, `.planning/phases/33-hard-sync/33-05-SUMMARY.md` — both FOUND.
- **Commits exist:** `4b8c295`, `9cb0f89`, `cc34df5` — all FOUND in `git log`.
- **The three new cases are present in `HEAD`** and are matched by their selectors with non-zero case counts: 1 / 76, 1 / 853, 1 / 499.
- **The suite really did grow:** 100 → 103 cases, 2,623,356 → 2,624,784 assertions, 0 failures.
- **The whole-plan diff is one file:** `git diff --name-only 6a67c34 HEAD` returns `tests/test_vco_spectrum.cpp` alone; `src/AnalogLFO.cpp` is absent; `tests/check_includes.sh` shows an empty diffstat.
- **Nothing shipped moved:** six LFO goldens byte-identical (9 cases / 49,188 assertions), `check_frozen.sh` PASS, `make strict` and `make guards` exit 0, **zero** compiler warnings.
- **The seam is still genuinely absent from `src/dsp/VcoCore.hpp`:** non-comment `addStep` count is **0**, checked against the committed blob rather than the working tree.
- **`.planning/REQUIREMENTS.md`:** SYNC-01 remains `[x]` / `Complete` (33-04's); **SYNC-02 remains `[ ]` / `Pending`**, checked explicitly rather than assumed.

---
*Phase: 33-hard-sync*
*Completed: 2026-08-29*
</content>
</invoke>
