---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
plan: 03
subsystem: testing
tags: [alias-floor, spectral-analysis, threshold-policy, tombstone, d-08, d-09, d-11, doctest, vco]

# Dependency graph
requires:
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-01 — fftRadix2, aliasPeakDb, binCentredPitchCV, binCentredSampleTime, impliedLeakageDb, NaiveVcoCoreMirror"
  - phase: 30-vco-core-skeleton-and-registration
    provides: "forge::VcoCore with the naive morphed oscillator; the tombstone-inverted-in-place precedent (D-15/D-19)"
  - phase: 29-vco-test-harness-and-lfo-guardrail
    provides: "tests/VcoBlockDriver.hpp and its four proven non-degenerate seed literals"
provides:
  - "SPECTRUM_GRID — 90 cells indexed by (morph region, note, character), 45 gated / 15 diagnostic / 30 cross-rate, each with a threshold and a written provenance string"
  - "measureCellDb + driveSecondBlock — one shared drive loop for the naive mirror and the live core, with the warm-up discard and a per-cell bin-centre solver escalation"
  - "kThresholdFloorDb = -75 dB — the tightest threshold this apparatus can honestly assert, derived from measurement"
  - "The recorded 90-cell naive alias baseline (D-08)"
  - "The alias-floor gate, OBSERVED RED against the naive core (32 of 45 gated cells) and landed as a tombstone naming plan 32-07 as its inversion owner"
affects: [32-04, 32-05, 32-06, 32-07, alias-floor-thresholds, morph-blep]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Threshold table indexed by (morph region, note, character) with a per-cell written provenance string (D-09 / P-6)"
    - "Static threshold floor derived from the instrument's measured noise floor, never computed from the measurement it gates (D-10)"
    - "Per-cell solver escalation: method one unless that cell's own D-10 bar demands method two; the self-check stays live because the threshold column is static"
    - "Tombstone-with-inversion-contract carrying a counter floor plus a named large-margin subset, so the pin is specific rather than statistical"

key-files:
  created: []
  modified:
    - tests/test_vco_spectrum.cpp

key-decisions:
  - "The plan's 'sine at character 0 below -140 dB' bound is unreachable through this apparatus and was replaced by the stronger floor-relative form; measured, the sine reports its cell's own leakage floor to within 0.078 dB at all six rows"
  - "kThresholdFloorDb = -75 dB: no threshold is tighter than the apparatus can assert, derived from plan 32-01's worst measured leakage row (-91.95 dB at 44.1 kHz C9)"
  - "measureCellDb escalates from method one to method two per cell, driven by that cell's own D-10 bar, rather than the plan's single-method text — the sine cells cannot be measured any other way"
  - "kNaiveFailuresFloor = 27, the observed 32 minus 5, so a cell flipping across its threshold on another toolchain cannot redden the build"
  - "The 13 gated cells that already pass against the naive core are recorded as expected P-6 behavior and are NOT to be 'fixed' by tightening their thresholds"

patterns-established:
  - "Measure-then-pin: the whole 90-cell baseline is recorded and CAPTUREd before a single threshold is asserted"
  - "Observe-the-RED-then-invert-in-place: the gate is written in final form, run against the naive core, its transcript transcribed, and the same slot converted to a tombstone"

requirements-completed: []

coverage:
  - id: D1
    description: "SPECTRUM_GRID holds 90 cells across 45 gated, 15 diagnostic and 30 cross-rate regression entries, each with a threshold and a written provenance"
    requirement: "TEST-03"
    verification:
      - kind: unit
        ref: "tests/test_vco_spectrum.cpp#vco spectrum: the NAIVE alias floor... (REQUIRE nCells == 90; gatedCells/diagnosticCells/regressionCells == 45/15/30)"
        status: pass
      - kind: other
        ref: "grep -c 'PROVISIONAL' tests/test_vco_spectrum.cpp -> 7; REQUIRE(cell.provenance != 0) on every cell"
        status: pass
    human_judgment: false
  - id: D2
    description: "The naive alias floor is recorded for every cell with a per-cell D-10 leakage self-check asserted before the alias value is read"
    requirement: "TEST-03"
    verification:
      - kind: unit
        ref: "tests/test_vco_spectrum.cpp#vco spectrum: the NAIVE alias floor... (REQUIRE impliedLeakage <= threshold - 10.0, 90 times)"
        status: pass
      - kind: other
        ref: "mutation probe: disabling the method-two escalation fires the REQUIRE immediately at -61.97 dB against a required -85"
        status: pass
    human_judgment: false
  - id: D3
    description: "The alias-floor gate has been observed RED against the naive core and landed as a tombstone naming plan 32-07 as its inversion owner"
    requirement: "TEST-03"
    verification:
      - kind: unit
        ref: "tests/test_vco_spectrum.cpp#vco spectrum: TOMBSTONE - the NAIVE core FAILS the Phase 32 alias-floor gate (D-08 RED evidence) - INVERTS IN PLAN 32-07"
        status: pass
      - kind: other
        ref: "step-one transcript: 32 of 45 gated cells failed, 32 failed assertions, reproduced verbatim below"
        status: pass
    human_judgment: false
  - id: D4
    description: "The apparatus is proven able to see the two structural properties it asserts, rather than merely observed green"
    requirement: "TEST-03"
    verification:
      - kind: other
        ref: "mutation probe: planting a 1e-3 spur in every measured block fails both sine bounds on all 6 sine cells (12 failed assertions)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The shipped Analog LFO is untouched: no frozen header edited, FROZEN.sha256 unmoved, src/AnalogLFO.cpp absent from the plan diff"
    verification:
      - kind: integration
        ref: "make guards + make strict + git diff --name-only HEAD~2 HEAD"
        status: pass
    human_judgment: false

# Metrics
duration: 40 min
completed: 2026-08-01
status: complete
---

# Phase 32 Plan 03: The Naive Alias Baseline and the D-08 Tombstone Summary

**The naive alias floor is now recorded across all 90 cells of a (morph region, note, character) grid with a per-cell leakage self-check, and the Phase 32 alias-floor gate has been written in its final form, observed failing on 32 of 45 gated cells against the live naive `forge::VcoCore`, and pinned as a tombstone that names plan 32-07 as the plan obliged to invert it.**

## Performance

- **Duration:** ~40 min
- **Completed:** 2026-08-01
- **Tasks:** 2
- **Files modified:** 1 (`tests/test_vco_spectrum.cpp`)

## Task Commits

1. **Task 1: The 90-cell SPECTRUM_GRID and the recorded naive baseline (D-08 / D-09 / D-11)** — `13ea1c7` (test)
2. **Task 2: Observe the alias-floor gate RED against the naive core, then land it as a tombstone (D-08)** — `1513d5b` (test)

## Accomplishments

- **The threshold is now set from measurement rather than inherited.** Every one of the 90 cells carries a `thresholdDb` and a `provenance` string naming its source and its provisional status, and every cell's trailing comment records the prototype's naive→corrected pair so plan 32-07's re-pinning has something to move against.
- **The RED is genuine.** The gate was written in its final form — `CHECK(measureCellDb(cell, useMirror=false) <= cell.thresholdDb)` over all 45 gated cells — and run against the live, still-naive `forge::VcoCore`. It failed on 32 of 45. The transcript is reproduced verbatim below.
- **The measurement reproduces `32-RESEARCH`'s prototype closely wherever the two are comparable.** Triangle C7 character 0: −41.6325 measured against −41.7 predicted. Saw C8 character 0: −15.5630 against −15.6. Pulse C8 character 0: −1.2931 against −1.3. Square C8 character 0: −16.9030 against −16.9. Assumption A3 ("the prototype's alias numbers will be reproduced to within ~1 dB") holds on the method-one cells.
- **D-11's like-with-like construction is visibly working.** The 48 kHz C8 row reproduces the 44.1 kHz C8 row to within 0.01 dB on every non-sine cell (saw character 0: −15.5651 against −15.5630), while the 96 kHz row is 0.4 to 15 dB cleaner because the same note carries 11 harmonics below Nyquist there instead of 5. A `dt`-scaling bug would show up as the 48 kHz row diverging from the 44.1 kHz one, and that comparison now exists.
- **The gate's own noise floor is asserted 90 times, per cell, before any alias value is read**, and the assertion was proven to bite by mutation probe rather than by having been seen green.

## The D-08 RED Evidence

The gate in its final form, run against the naive `forge::VcoCore`. **This state was not committed.** Verbatim `./build-test/test -tc="vco spectrum: the Phase 32 alias-floor gate*"` output, head and tail:

```
[doctest] doctest version is "2.4.11"
[doctest] run with "--help" for options
===============================================================================
tests/test_vco_spectrum.cpp:1538:
TEST CASE:  vco spectrum: the Phase 32 alias-floor gate (D-09)

tests/test_vco_spectrum.cpp:1563: ERROR: CHECK( measuredDb <= (double)threshold ) is NOT correct!
  values: CHECK( -56.5921 <= -65 )
  logged: i := 1
          sr := 44100
          K := 195
          note := C7
          morph := 0
          region := sine
          character := 0.5
          threshold := -65
          method := 2
          impliedLeakage := -125.513
          measuredDb := -56.5921
          aliasRmsDb := -82.6504

tests/test_vco_spectrum.cpp:1563: ERROR: CHECK( measuredDb <= (double)threshold ) is NOT correct!
  values: CHECK( -41.6325 <= -47 )
  logged: i := 3
          sr := 44100
          K := 195
          note := C7
          morph := 0.25
          region := triangle
          character := 0
          threshold := -47
          method := 1
          impliedLeakage := -61.9683
          measuredDb := -41.6325
          aliasRmsDb := -70.8422

  ... 29 further failure blocks, identical in shape ...

tests/test_vco_spectrum.cpp:1563: ERROR: CHECK( measuredDb <= (double)threshold ) is NOT correct!
  values: CHECK( -0.446148 <= -6 )
  logged: i := 43
          sr := 44100
          K := 777
          note := C9
          morph := 1
          region := pulse 5%
          character := 0.5
          threshold := -6
          method := 1
          impliedLeakage := -70.8615
          measuredDb := -0.446148
          aliasRmsDb := -24.8088

===============================================================================
[doctest] test cases:   1 |   0 passed |  1 failed | 84 skipped
[doctest] assertions: 135 | 103 passed | 32 failed |
[doctest] Status: FAILURE!
```

### The complete failing cell list, reported versus expected

| # | i | rate | K | note | region | character | reported dB | expected dB | over by |
|---|---|------|---|------|--------|-----------|------------:|------------:|--------:|
| 1 | 1 | 44100 | 195 | C7 | sine | 0.5 | −56.5921 | −65 | +8.408 |
| 2 | 3 | 44100 | 195 | C7 | triangle | 0 | −41.6325 | −47 | +5.367 |
| 3 | 6 | 44100 | 195 | C7 | saw | 0 | −20.8277 | −26 | +5.172 |
| 4 | 7 | 44100 | 195 | C7 | saw | 0.5 | −20.6087 | −26 | +5.391 |
| 5 | 8 | 44100 | 195 | C7 | saw | 1 | −20.0959 | −25 | +4.904 |
| 6 | 9 | 44100 | 195 | C7 | square | 0 | −20.8290 | −26 | +5.171 |
| 7 | 10 | 44100 | 195 | C7 | square | 0.5 | −21.9570 | −27 | +5.043 |
| 8 | 11 | 44100 | 195 | C7 | square | 1 | −49.8461 | −57 | +7.154 |
| 9 | 12 | 44100 | 195 | C7 | pulse 5% | 0 | −4.8327 | −10 | +5.167 |
| 10 | 13 | 44100 | 195 | C7 | pulse 5% | 0.5 | −5.3471 | −10 | +4.653 |
| 11 | 16 | 44100 | 389 | C8 | sine | 0.5 | −52.0938 | −68 | +15.906 |
| 12 | 17 | 44100 | 389 | C8 | sine | 1 | −67.2686 | −73 | +5.731 |
| 13 | 18 | 44100 | 389 | C8 | triangle | 0 | −33.8085 | −45 | +11.191 |
| 14 | 19 | 44100 | 389 | C8 | triangle | 0.5 | −33.2317 | −35 | +1.768 |
| 15 | 21 | 44100 | 389 | C8 | saw | 0 | −15.5630 | −22 | +6.437 |
| 16 | 22 | 44100 | 389 | C8 | saw | 0.5 | −15.3786 | −22 | +6.621 |
| 17 | 23 | 44100 | 389 | C8 | saw | 1 | −14.7858 | −20 | +5.214 |
| 18 | 24 | 44100 | 389 | C8 | square | 0 | −16.9030 | −28 | +11.097 |
| 19 | 25 | 44100 | 389 | C8 | square | 0.5 | −17.4632 | −30 | +12.537 |
| 20 | 26 | 44100 | 389 | C8 | square | 1 | −38.8452 | −44 | +5.155 |
| 21 | 27 | 44100 | 389 | C8 | pulse 5% | 0 | −1.2931 | −8 | +6.707 |
| 22 | 28 | 44100 | 389 | C8 | pulse 5% | 0.5 | −1.5352 | −8 | +6.465 |
| 23 | 33 | 44100 | 777 | C9 | triangle | 0 | −19.0850 | −25 | +5.915 |
| 24 | 34 | 44100 | 777 | C9 | triangle | 0.5 | −18.9745 | −21 | +2.026 |
| 25 | 36 | 44100 | 777 | C9 | saw | 0 | −9.5424 | −16 | +6.458 |
| 26 | 37 | 44100 | 777 | C9 | saw | 0.5 | −9.3696 | −15 | +5.630 |
| 27 | 38 | 44100 | 777 | C9 | saw | 1 | −8.9488 | −14 | +5.051 |
| 28 | 39 | 44100 | 777 | C9 | square | 0 | −9.5424 | −16 | +6.458 |
| 29 | 40 | 44100 | 777 | C9 | square | 0.5 | −9.6485 | −16 | +6.351 |
| 30 | 41 | 44100 | 777 | C9 | square | 1 | −15.3574 | −18 | +2.643 |
| 31 | 42 | 44100 | 777 | C9 | pulse 5% | 0 | −0.2887 | −6 | +5.711 |
| 32 | 43 | 44100 | 777 | C9 | pulse 5% | 0.5 | −0.4461 | −6 | +5.554 |

**Observed `failing` count: 32 of 45 gated cells.**
**Derived `kNaiveFailuresFloor`: 27** (the observed count minus 5). The gap absorbs the two cells sitting within 2.1 dB of their thresholds — i = 19 (triangle C8, character 0.5, +1.768 dB) and i = 34 (triangle C9, character 0.5, +2.026 dB) — either of which could flip on a different toolchain. **The observed number is a measurement; the constant is a floor derived from it, and it must not later be "tightened" to the observed count.**

### The five named large-margin subset cells

Asserted individually at `CHECK(naiveDb > threshold + 5.0)`. These are what make the tombstone specific rather than statistical.

| cell | measured dB | threshold dB | margin | cushion over the +5 dB bar |
|------|------------:|-------------:|-------:|---------------------------:|
| 44100 / K=389 / morph 0.25 triangle / character 0.00 | −33.8085 | −45 | **+11.1915** | 6.19 dB |
| 44100 / K=389 / morph 0.50 saw / character 0.00 | −15.5630 | −22 | **+6.4370** | 1.44 dB |
| 44100 / K=389 / morph 0.75 square / character 0.00 | −16.9030 | −28 | **+11.0970** | 6.10 dB |
| 44100 / K=389 / morph 1.00 pulse / character 0.00 | −1.2931 | −8 | **+6.7069** | 1.71 dB |
| 44100 / K=777 / morph 0.50 saw / character 0.00 | −9.5424 | −16 | **+6.4576** | 1.46 dB |

### Why 13 gated cells already pass, and why that is correct

The 13 non-failing gated cells are the P-6 population, not a defect in the grid, and **must not be "fixed" by tightening their thresholds**:

- **Six are sine cells** (i = 0, 2, 30, 31, 32 and the character-0 rows). A sine at character 0 has no discontinuity of its own and no bleed ring, so there is nothing for band-limiting to correct; at C9 the sine's bleed ring at character 0.5 and 1.0 is one of RESEARCH's two recorded ~1.5 dB regressions.
- **Four are high-character cells where the D-03 factor correctly returns zero** — triangle C8 character 1 (−33.6598 measured against −33.5 predicted, an improvement of exactly 0.0 dB by design because the corner is already 7.7 samples wide), triangle C9 character 1, pulse C7 character 1, pulse C9 character 1.
- **Three are cells whose naive value already sits within a decibel of the threshold** (pulse C8 character 1 at −7.837 against −7).

A grid where every cell were red would mean the thresholds had been set to fail rather than to measure.

## The Full 90-Cell Naive Baseline

`method` 1 is `binCentredPitchCV` (bisect pitch, shared harness); `method` 2 is `binCentredSampleTime` (the nudge, local drive loop). `leakage dB` is that cell's achieved `impliedLeakageDb(binError)` — the gate's own noise floor for that cell, which the D-10 self-check requires to sit at least 10 dB below the threshold column.

| i | rate | K | note | region | char | tier | method | leakage dB | naive peak dB | naive RMS dB | threshold dB | vs threshold |
|--:|-----:|--:|------|--------|-----:|------|-------:|-----------:|--------------:|-------------:|-------------:|-------------|
| 0 | 44100 | 195 | C7 | sine | 0 | gated | 2 | −125.513 | −125.435 | −153.329 | −75 | green −50.435 |
| 1 | 44100 | 195 | C7 | sine | 0.5 | gated | 2 | −125.513 | −56.5921 | −82.6504 | −65 | **RED +8.408** |
| 2 | 44100 | 195 | C7 | sine | 1 | gated | 2 | −125.513 | −98.8753 | −128.556 | −75 | green −23.875 |
| 3 | 44100 | 195 | C7 | triangle | 0 | gated | 1 | −61.9683 | −41.6325 | −70.8422 | −47 | **RED +5.367** |
| 4 | 44100 | 195 | C7 | triangle | 0.5 | gated | 1 | −61.9683 | −40.68 | −69.3584 | −39 | green −1.680 |
| 5 | 44100 | 195 | C7 | triangle | 1 | gated | 1 | −61.9683 | −47.592 | −77.0703 | −44 | green −3.592 |
| 6 | 44100 | 195 | C7 | saw | 0 | gated | 1 | −61.9683 | −20.8277 | −43.3027 | −26 | **RED +5.172** |
| 7 | 44100 | 195 | C7 | saw | 0.5 | gated | 1 | −61.9683 | −20.6087 | −43.0689 | −26 | **RED +5.391** |
| 8 | 44100 | 195 | C7 | saw | 1 | gated | 1 | −61.9683 | −20.0959 | −42.8979 | −25 | **RED +4.904** |
| 9 | 44100 | 195 | C7 | square | 0 | gated | 1 | −61.9683 | −20.829 | −46.1168 | −26 | **RED +5.171** |
| 10 | 44100 | 195 | C7 | square | 0.5 | gated | 1 | −61.9683 | −21.957 | −48.0136 | −27 | **RED +5.043** |
| 11 | 44100 | 195 | C7 | square | 1 | gated | 2 | −125.513 | −49.8461 | −73.0733 | −57 | **RED +7.154** |
| 12 | 44100 | 195 | C7 | pulse 5% | 0 | gated | 1 | −61.9683 | −4.83274 | −29.9147 | −10 | **RED +5.167** |
| 13 | 44100 | 195 | C7 | pulse 5% | 0.5 | gated | 1 | −61.9683 | −5.34705 | −31.2044 | −10 | **RED +4.653** |
| 14 | 44100 | 195 | C7 | pulse 5% | 1 | gated | 1 | −61.9683 | −20.041 | −48.8258 | −17 | green −3.041 |
| 15 | 44100 | 389 | C8 | sine | 0 | gated | 2 | −101.55 | −101.541 | −129.481 | −75 | green −26.541 |
| 16 | 44100 | 389 | C8 | sine | 0.5 | gated | 2 | −101.55 | −52.0938 | −79.9344 | −68 | **RED +15.906** |
| 17 | 44100 | 389 | C8 | sine | 1 | gated | 2 | −101.55 | −67.2686 | −100.116 | −73 | **RED +5.731** |
| 18 | 44100 | 389 | C8 | triangle | 0 | gated | 1 | −56.5394 | −33.8085 | −64.41 | −45 | **RED +11.191** |
| 19 | 44100 | 389 | C8 | triangle | 0.5 | gated | 1 | −56.5394 | −33.2317 | −63.1405 | −35 | **RED +1.768** |
| 20 | 44100 | 389 | C8 | triangle | 1 | gated | 1 | −56.5394 | −33.6598 | −65.6076 | −30 | green −3.660 |
| 21 | 44100 | 389 | C8 | saw | 0 | gated | 1 | −56.5394 | −15.563 | −40.5181 | −22 | **RED +6.437** |
| 22 | 44100 | 389 | C8 | saw | 0.5 | gated | 1 | −56.5394 | −15.3786 | −40.3023 | −22 | **RED +6.621** |
| 23 | 44100 | 389 | C8 | saw | 1 | gated | 1 | −56.5394 | −14.7858 | −39.8966 | −20 | **RED +5.214** |
| 24 | 44100 | 389 | C8 | square | 0 | gated | 1 | −56.5394 | −16.903 | −43.9333 | −28 | **RED +11.097** |
| 25 | 44100 | 389 | C8 | square | 0.5 | gated | 1 | −56.5394 | −17.4632 | −45.294 | −30 | **RED +12.537** |
| 26 | 44100 | 389 | C8 | square | 1 | gated | 1 | −56.5394 | −38.8452 | −67.92 | −44 | **RED +5.155** |
| 27 | 44100 | 389 | C8 | pulse 5% | 0 | gated | 1 | −56.5394 | −1.2931 | −26.1491 | −8 | **RED +6.707** |
| 28 | 44100 | 389 | C8 | pulse 5% | 0.5 | gated | 1 | −56.5394 | −1.53521 | −26.839 | −8 | **RED +6.465** |
| 29 | 44100 | 389 | C8 | pulse 5% | 1 | gated | 1 | −56.5394 | −7.83655 | −37.1262 | −7 | green −0.837 |
| 30 | 44100 | 777 | C9 | sine | 0 | gated | 2 | −91.9452 | −91.9421 | −119.882 | −75 | green −16.942 |
| 31 | 44100 | 777 | C9 | sine | 0.5 | gated | 1 | −70.8615 | −37.3824 | −69.7521 | −31 | green −6.382 |
| 32 | 44100 | 777 | C9 | sine | 1 | gated | 1 | −70.8615 | −23.8352 | −56.9386 | −19 | green −4.835 |
| 33 | 44100 | 777 | C9 | triangle | 0 | gated | 1 | −70.8615 | −19.085 | −51.4425 | −25 | **RED +5.915** |
| 34 | 44100 | 777 | C9 | triangle | 0.5 | gated | 1 | −70.8615 | −18.9745 | −51.2143 | −21 | **RED +2.026** |
| 35 | 44100 | 777 | C9 | triangle | 1 | gated | 1 | −70.8615 | −18.5499 | −50.6764 | −16 | green −2.550 |
| 36 | 44100 | 777 | C9 | saw | 0 | gated | 1 | −70.8615 | −9.54241 | −37.1438 | −16 | **RED +6.458** |
| 37 | 44100 | 777 | C9 | saw | 0.5 | gated | 1 | −70.8615 | −9.36961 | −36.9494 | −15 | **RED +5.630** |
| 38 | 44100 | 777 | C9 | saw | 1 | gated | 1 | −70.8615 | −8.94884 | −36.5279 | −14 | **RED +5.051** |
| 39 | 44100 | 777 | C9 | square | 0 | gated | 1 | −70.8615 | −9.54244 | −39.4225 | −16 | **RED +6.458** |
| 40 | 44100 | 777 | C9 | square | 0.5 | gated | 1 | −70.8615 | −9.64852 | −39.9683 | −16 | **RED +6.351** |
| 41 | 44100 | 777 | C9 | square | 1 | gated | 1 | −70.8615 | −15.3574 | −48.1795 | −18 | **RED +2.643** |
| 42 | 44100 | 777 | C9 | pulse 5% | 0 | gated | 1 | −70.8615 | −0.2887 | −24.3047 | −6 | **RED +5.711** |
| 43 | 44100 | 777 | C9 | pulse 5% | 0.5 | gated | 1 | −70.8615 | −0.446148 | −24.8088 | −6 | **RED +5.554** |
| 44 | 44100 | 777 | C9 | pulse 5% | 1 | gated | 1 | −70.8615 | −2.52971 | −31.0051 | −2 | green −0.530 |
| 45 | 44100 | 97 | C6 | sine | 0 | diagnostic | 2 | −116.186 | −116.141 | −144.071 | −75 | green −41.141 |
| 46 | 44100 | 97 | C6 | sine | 0.5 | diagnostic | 2 | −116.186 | −64.0666 | −86.554 | −73 | RED +8.933 |
| 47 | 44100 | 97 | C6 | sine | 1 | diagnostic | 2 | −116.186 | −114.099 | −138.993 | −75 | green −39.099 |
| 48 | 44100 | 97 | C6 | triangle | 0 | diagnostic | 1 | −71.2485 | −54.4646 | −81.0529 | −61 | RED +6.535 |
| 49 | 44100 | 97 | C6 | triangle | 0.5 | diagnostic | 1 | −71.2485 | −55.1872 | −82.6534 | −52 | green −3.187 |
| 50 | 44100 | 97 | C6 | triangle | 1 | diagnostic | 1 | −71.2485 | −60.2646 | −87.3691 | −57 | green −3.265 |
| 51 | 44100 | 97 | C6 | saw | 0 | diagnostic | 1 | −71.2485 | −26.848 | −46.394 | −32 | RED +5.152 |
| 52 | 44100 | 97 | C6 | saw | 0.5 | diagnostic | 1 | −71.2485 | −26.5895 | −46.1583 | −32 | RED +5.410 |
| 53 | 44100 | 97 | C6 | saw | 1 | diagnostic | 1 | −71.2485 | −26.5101 | −46.0577 | −32 | RED +5.490 |
| 54 | 44100 | 97 | C6 | square | 0 | diagnostic | 1 | −71.2485 | −27.2355 | −49.5063 | −33 | RED +5.765 |
| 55 | 44100 | 97 | C6 | square | 0.5 | diagnostic | 1 | −71.2485 | −29.4214 | −51.905 | −35 | RED +5.579 |
| 56 | 44100 | 97 | C6 | square | 1 | diagnostic | 2 | −116.186 | −56.7366 | −76.3003 | −65 | RED +8.263 |
| 57 | 44100 | 97 | C6 | pulse 5% | 0 | diagnostic | 1 | −71.2485 | −13.2439 | −33.1722 | −23 | RED +9.756 |
| 58 | 44100 | 97 | C6 | pulse 5% | 0.5 | diagnostic | 1 | −71.2485 | −14.9306 | −35.2916 | −24 | RED +9.069 |
| 59 | 44100 | 97 | C6 | pulse 5% | 1 | diagnostic | 1 | −71.2485 | −36.9925 | −60.9526 | −33 | green −3.992 |
| 60 | 48000 | 357 | C8 | sine | 0 | regression | 2 | −97.7123 | −97.7016 | −125.643 | −75 | green −22.702 |
| 61 | 48000 | 357 | C8 | sine | 0.5 | regression | 2 | −97.7123 | −52.0938 | −79.9346 | −68 | RED +15.906 |
| 62 | 48000 | 357 | C8 | sine | 1 | regression | 2 | −97.7123 | −67.2685 | −100.108 | −73 | RED +5.731 |
| 63 | 48000 | 357 | C8 | triangle | 0 | regression | 1 | −56.2187 | −33.8042 | −64.3993 | −45 | RED +11.196 |
| 64 | 48000 | 357 | C8 | triangle | 0.5 | regression | 1 | −56.2187 | −33.2309 | −63.1404 | −35 | RED +1.769 |
| 65 | 48000 | 357 | C8 | triangle | 1 | regression | 1 | −56.2187 | −33.658 | −65.5988 | −30 | green −3.658 |
| 66 | 48000 | 357 | C8 | saw | 0 | regression | 1 | −56.2187 | −15.5651 | −40.5163 | −22 | RED +6.435 |
| 67 | 48000 | 357 | C8 | saw | 0.5 | regression | 1 | −56.2187 | −15.3806 | −40.3005 | −22 | RED +6.619 |
| 68 | 48000 | 357 | C8 | saw | 1 | regression | 1 | −56.2187 | −14.7874 | −39.8977 | −20 | RED +5.213 |
| 69 | 48000 | 357 | C8 | square | 0 | regression | 1 | −56.2187 | −16.9039 | −43.9317 | −28 | RED +11.096 |
| 70 | 48000 | 357 | C8 | square | 0.5 | regression | 1 | −56.2187 | −17.4639 | −45.2938 | −30 | RED +12.536 |
| 71 | 48000 | 357 | C8 | square | 1 | regression | 1 | −56.2187 | −38.8492 | −67.8886 | −44 | RED +5.151 |
| 72 | 48000 | 357 | C8 | pulse 5% | 0 | regression | 1 | −56.2187 | −1.28153 | −26.1093 | −8 | RED +6.718 |
| 73 | 48000 | 357 | C8 | pulse 5% | 0.5 | regression | 1 | −56.2187 | −1.52641 | −26.8073 | −8 | RED +6.474 |
| 74 | 48000 | 357 | C8 | pulse 5% | 1 | regression | 1 | −56.2187 | −7.83528 | −37.1222 | −7 | green −0.835 |
| 75 | 96000 | 179 | C8 | sine | 0 | regression | 2 | −102.875 | −102.852 | −130.793 | −75 | green −27.852 |
| 76 | 96000 | 179 | C8 | sine | 0.5 | regression | 2 | −102.875 | −58.3074 | −83.6065 | −68 | RED +9.693 |
| 77 | 96000 | 179 | C8 | sine | 1 | regression | 2 | −102.875 | −100.189 | −127.905 | −73 | green −27.189 |
| 78 | 96000 | 179 | C8 | triangle | 0 | regression | 1 | −72.8699 | −44.5581 | −73.2967 | −45 | RED +0.442 |
| 79 | 96000 | 179 | C8 | triangle | 0.5 | regression | 1 | −72.8699 | −43.6157 | −71.4744 | −35 | green −8.616 |
| 80 | 96000 | 179 | C8 | triangle | 1 | regression | 1 | −72.8699 | −47.5728 | −78.6871 | −30 | green −17.573 |
| 81 | 96000 | 179 | C8 | saw | 0 | regression | 1 | −72.8699 | −21.5835 | −43.6997 | −22 | RED +0.416 |
| 82 | 96000 | 179 | C8 | saw | 0.5 | regression | 1 | −72.8699 | −21.3726 | −43.465 | −22 | RED +0.627 |
| 83 | 96000 | 179 | C8 | saw | 1 | regression | 1 | −72.8699 | −20.952 | −43.3357 | −20 | green −0.952 |
| 84 | 96000 | 179 | C8 | square | 0 | regression | 1 | −72.8699 | −22.2787 | −46.9021 | −28 | RED +5.721 |
| 85 | 96000 | 179 | C8 | square | 0.5 | regression | 1 | −72.8699 | −23.6685 | −48.9611 | −30 | RED +6.331 |
| 86 | 96000 | 179 | C8 | square | 1 | regression | 1 | −72.8699 | −51.1848 | −73.533 | −44 | green −7.185 |
| 87 | 96000 | 179 | C8 | pulse 5% | 0 | regression | 1 | −72.8699 | −5.91965 | −30.66 | −8 | RED +2.080 |
| 88 | 96000 | 179 | C8 | pulse 5% | 0.5 | regression | 1 | −72.8699 | −6.47988 | −32.0743 | −8 | RED +1.520 |
| 89 | 96000 | 179 | C8 | pulse 5% | 1 | regression | 1 | −72.8699 | −22.3537 | −50.6991 | −7 | green −15.354 |

*(The "vs threshold" column for the diagnostic and regression tiers is informational only — neither tier is asserted by the tombstone, which walks `tier == "gated"` alone.)*

### The sine at character 0 measures the instrument, not the oscillator

The six sine / character-0 cells report their own cell's leakage floor and nothing more:

| rate | K | note | cell leakage dB | reported alias dB | excess |
|-----:|--:|------|----------------:|------------------:|-------:|
| 44100 | 195 | C7 | −125.513 | −125.435 | +0.0780 |
| 44100 | 389 | C8 | −101.550 | −101.541 | +0.0090 |
| 44100 | 777 | C9 | −91.9452 | −91.9421 | +0.0031 |
| 44100 | 97 | C6 | −116.186 | −116.141 | +0.0450 |
| 48000 | 357 | C8 | −97.7123 | −97.7016 | +0.0107 |
| 96000 | 179 | C8 | −102.875 | −102.852 | +0.0230 |

A sine at character 0 has no discontinuity and no bleed ring, so it emits no alias energy at all; everything the classifier reports for it is the rectangular-window leakage of a drive frequency that could not be placed *exactly* on its bin centre. This is what made the plan's `-140 dB` bound unsatisfiable — see Deviations.

### Cross-rate observations for plan 32-07

- **48 kHz reproduces 44.1 kHz cell for cell.** Every non-sine 48 kHz C8 cell lands within 0.01 dB of its 44.1 kHz C8 twin. D-11's like-with-like construction is doing exactly what it was designed to do, and a `dt`-scaling bug in `MorphBlep` will show up as this agreement breaking.
- **96 kHz is 0.4 to 15 dB cleaner at the same note**, because C8 there carries 11 harmonics below Nyquist instead of 5 and much less folds back. **The 96 kHz thresholds, transferred from the 44.1 kHz C8 row, are therefore conservative** — cells 78, 81, 82, 87 and 88 miss by under 2.1 dB naive, and cells 79, 80, 83 and 86 already pass. Plan 32-07 should expect the 96 kHz rows to be the least discriminating in the grid, and must not tighten them to compensate: the anti-softening clause runs in both directions.

## Decisions Made

1. **`kThresholdFloorDb = -75.0f`.** No threshold in the grid is tighter than the apparatus can honestly assert. Derived from measurement: plan 32-01's worst method-two leakage row is −91.95 dB (44.1 kHz C9), so the tightest assertable threshold anywhere on the grid is −81.95 dB, and −75 leaves that worst row 16.95 dB of margin. Six cells' prototype figures are tighter than the floor and carry a provenance string saying so. The floor is a **static** constant, deliberately — a threshold computed from the binError the self-check reads would be a self-check that can never fail.
2. **`measureCellDb` escalates the bin-centre solver per cell.** Method one (bisect `pitchCV`, `forge::VcoBlockDriver` untouched) is used whenever its implied leakage already clears that cell's own D-10 bar; otherwise the cell escalates to method two, exactly as `32-RESEARCH`'s switching rule prescribes. 76 of the 90 cells use method one; 14 escalate. The escalation cannot hide a failure because the threshold column is static and the self-check `REQUIRE` still runs against whatever leakage was actually achieved.
3. **Method two drives the live core through the shared `driveSecondBlock` loop, not through `forge::VcoBlockDriver`.** The driver's per-sample `sampleTime` overwrite is unconditional and documented as load-bearing; a nudged `dt` is unreachable through it, and making that overwrite conditional is forbidden (R-2 / P-4). Method-one cells still drive the live core through the driver, as the plan asks. That the two paths agree is not assumed — plan 32-01's identity case `REQUIRE`s the driver's output to be bit-identical to a local loop's over a 45-point grid at these very frequencies.
4. **`kSelfCheckDb` was left at −62 dB.** Plan 32-01 flagged it as pinned from plan text rather than measurement and asked plan 32-07 to re-derive it. Nothing in this plan required touching it, so nothing did. The new grid's own per-cell D-10 bar is independent of that constant.
5. **The C6 tier is `diagnostic`, not `gated`, and the tombstone walks `tier == "gated"` only.** C6 is where the naive path is already clean at high character (square, character 1: −56.74 dB measured), so it is the row that would expose an over-correcting factor as a P-1 regression rather than as a miss. Recording it costs 15 cells of runtime and buys the only evidence that would catch damage rather than absence of improvement.
6. **TEST-03 is deliberately NOT marked complete**, continuing the discipline plan 32-01 applied and Phase 31 applied five consecutive times. TEST-03 reads "an alias-floor / spectral invariant asserts high-note aliasing stays below a defined threshold". This plan asserts the *opposite* — that the naive core fails that threshold on 32 of 45 cells. The requirement is satisfied when plan 32-07 inverts the tombstone and the gate goes green against the real `forge::MorphBlep`. Marking it here would reproduce the PANEL-03 false green Phase 30 recorded as deferred item 1.
7. **The three label columns are copied into `std::string` before `CAPTURE`.** doctest renders a bare `const char*` as a pointer unless `DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING` is defined project-wide, and that macro is a global switch that would change how every other TU renders — including the shipped LFO's cases. For a case whose entire job is to *record*, an unreadable `-s` dump would defeat the case.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] The plan's "sine at character 0 below −140 dB" structural bound is unsatisfiable through this apparatus; replaced with the stronger floor-relative form**

- **Found during:** Task 1, on the first full run — the assertion failed on all six sine / character-0 cells.
- **Issue:** The plan's `<behavior>` block requires `measureCellDb` on the sine cell at character 0 to return "an alias peak below −140 dB at every note", citing `32-RESEARCH`'s prototype figure of −150.7 dB. Measured here: **−125.435 / −101.541 / −91.9421 / −116.141 / −97.7016 / −102.852 dB**. Six failed assertions.
- **Root cause, measured:** it is not the oscillator. −150.7 dB sits **25 to 59 dB below this gate's own leakage floor** on every row of the grid — plan 32-01's measured method-two column is −91.95 to −125.51 dB. An instrument cannot report a number quieter than its own noise. The reported figure is, to within 0.078 dB at the worst row and 0.003 dB at the best, *exactly* the cell's own `impliedLeakageDb(binError)`: the sine contributes nothing at all, and everything the classifier sees is the residual of a drive frequency that could not be placed exactly on its bin centre. A −140 dB `CHECK` here would not be a stronger claim about the DSP — it would be a claim the measurement cannot carry, which is precisely what the D-10 self-check three lines above it exists to forbid.
- **Fix:** two assertions in place of one, both stating the plan's actual intent more strongly than the number did.
  - `CHECK(naiveDb <= impliedLeakage + 1.0)` — the sine adds nothing measurable *above whatever floor the instrument has*, which holds on any apparatus rather than on this one. The 1.0 dB margin is a 12× cushion on the worst measured excess of 0.078 dB.
  - `CHECK(naiveDb < -85.0)` — an absolute bound pinning the floor far below every threshold on the grid, so a future apparatus regression that lifted *both* the floor and this cell together (keeping the delta small while the whole measurement went soft) still fails. Worst measured is −91.94 dB, so −85.0 leaves 6.9 dB.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** observed RED, not argued — a one-shot mutation probe planting a 1e-3 spur in every measured block fails **both** bounds on **all six** sine cells (12 failed assertions, reported at ≈−74 dB against floors of −90.9 to −124.5).
- **Committed in:** `13ea1c7` (Task 1 commit)

---

**2. [Rule 2 — Missing Critical] The plan's single-method `measureCellDb` cannot satisfy its own per-cell D-10 self-check; added the per-cell solver escalation**

- **Found during:** Task 1, while deriving the threshold column — the achievable leakage per row was known from plan 32-01 before any code was written.
- **Issue:** The plan specifies `measureCellDb` resolving `binCentredPitchCV` and driving `forge::VcoBlockDriver`, i.e. method one only. It *also* requires a per-cell `REQUIRE` that `impliedLeakageDb(binError)` sits at least 10 dB below that cell's threshold. Method one's measured leakage is −56.22 to −72.87 dB across the six rows, which supports thresholds no tighter than about −46 dB at 44.1 kHz C8. Fourteen cells in the grid — every sine cell whose threshold is tighter than −60 dB, plus C6 square at character 1 and C7 square at character 1 — carry thresholds the method cannot support. The two requirements contradict each other, in the same shape plan 32-01 recorded for its own Part C.
- **Fix:** `measureCellDb` compares method one's implied leakage against that cell's own D-10 bar (`thresholdDb - 10`) and escalates to `binCentredSampleTime` where it falls short — the second method already prescribed by `32-RESEARCH.md` § Validation Architecture and `32-VALIDATION.md`:103, whose switching rule reads "If any threshold ends up tighter than about −50 dB (only the sine rows do), switch that case to the second method." The chosen method is reported through `methodOut` and `CAPTURE`d, so a red cell names the instrument it was measured with.
- **Why this does not weaken the self-check:** the threshold column is **static**. The escalation changes only *which instrument* measures the cell, never *what is asserted about it*. If method two also fails the bar for some cell, the `REQUIRE` fires — and that is a finding about the apparatus, which is what the self-check is for.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** observed RED — a one-shot mutation probe disabling the escalation (`if (false && ...)`) fires the per-cell `REQUIRE` immediately on the first sine cell: `REQUIRE( -61.9683 <= -85 )`. Restored, then committed green.
- **Committed in:** `13ea1c7` (Task 1 commit)

---

**Total deviations:** 2 auto-fixed (1 bug, 1 missing critical).
**Impact on plan:** both deviations make the plan's own stated behavior *achievable* rather than expanding it. No production code was touched, no guard was weakened, no driver was changed, and the plan's file list (`tests/test_vco_spectrum.cpp` alone) is unchanged. Deviation 2 is the same class of finding plan 32-01 recorded and uses the same remedy the phase's own research and validation documents prescribe.

### Threshold column note (not a deviation, recorded for plan 32-07)

The plan says to fill `thresholdDb` from `32-VALIDATION.md`'s Threshold Policy matrix. That matrix covers C7, C8 and C9 only. The C6 diagnostic row's thresholds come from `32-RESEARCH.md`'s § "D-08 baseline and D-09 threshold evidence" table, which does carry C6 rows at the same three characters; the 48 kHz and 96 kHz rows take the 44.1 kHz threshold **for the same note**, which is the entire reason D-11 lands them on C8. No cell required interpolation, and every one of those three cases has its own distinct provenance string.

## Verification Results

| Check | Result |
|-------|--------|
| `make test` | **85 cases / 85 passed / 0 failed**, 2,618,907 assertions (from 83 / 2,618,339 at plan start) |
| `./build-test/test -tc="vco spectrum: the NAIVE alias floor*" -s` | 1 case, 1 passed, 0 failed, 469 assertions |
| `./build-test/test -tc="vco spectrum: TOMBSTONE*" -s` | 1 case, 1 passed, 0 failed, 99 assertions |
| `failing` reported by the tombstone | **32**, against `kNaiveFailuresFloor` = 27 |
| `gatedWalked` / `subsetChecked` | 45 / 5, both `REQUIRE`d |
| five subset `marginDb` values | 11.1915, 11.097, 6.7069, 6.45759, 6.43695 — all > 5.0 |
| `sizeof(SPECTRUM_GRID)/sizeof(SPECTRUM_GRID[0])` | **90**, `REQUIRE`d in the baseline case |
| tier census | 45 gated / 15 diagnostic / 30 regression, all `CHECK`ed |
| `grep -c 'PROVISIONAL' tests/test_vco_spectrum.cpp` | **7** |
| `grep -c 'INVERTS IN PLAN 32-07' tests/test_vco_spectrum.cpp` | **1** |
| `grep -c 'kNaiveFailuresFloor' tests/test_vco_spectrum.cpp` | **2** |
| `make guards` | `guard suite: PASS` — `check_frozen.sh`, `check_includes.sh`, `check_canary.sh` |
| `make strict` | `strict C++11 gate: PASS` over all four `src/` TUs |
| `git diff --name-only` (per task, and `HEAD~2 HEAD`) | exactly `tests/test_vco_spectrum.cpp` |
| `src/AnalogLFO.cpp` in the diff | absent |
| `src/dsp/FROZEN.sha256` | unmoved (`git status --porcelain` clean of it) |

Doctest counts across the plan:

| Point | Cases | Assertions |
|-------|------:|-----------:|
| Plan start (after 32-01, 32-02) | 83 | 2,618,339 |
| After Task 1 | 84 | 2,618,808 |
| After Task 2 | 85 | 2,618,907 |

Delta: **+2 cases, +568 assertions**, 0 failures.

## TDD Gate Compliance

Task 1 is marked `tdd="true"`, but its deliverable **is** a test file — there is no production code for a failing test to drive out, and a synthetic RED commit would have to fabricate a failure rather than observe one. RED was therefore established the way this repository establishes it everywhere else, and the way plan 32-01 established it: each detector was **observed failing** under a one-shot mutation probe, then the probe was reverted and the green state committed.

| Detector | Mutation applied | Observed RED |
|----------|------------------|--------------|
| The per-cell D-10 leakage self-check | Disable the method-two escalation (`if (false && ...)`) | `REQUIRE( -61.9683 <= -85 )` fires on the first sine cell; 1 failed assertion |
| The sine structural bounds | Plant a 1e-3 spur at bin `K+1` in every measured block | Both bounds fail on all 6 sine cells; 12 failed assertions, reported ≈−74 dB against floors of −90.9 to −124.5 |
| The alias-floor gate itself (Task 2, step one) | *None — run in its final form against the live naive core* | **32 of 45 gated cells fail; 32 failed assertions.** This is not a probe, it is the D-08 RED observation, and it is the reason the tombstone exists |

Commit types are `test(32-03)` for both tasks, the correct conventional type for a test-only change.

## Issues Encountered

None beyond the two deviations above. `make test`, `make guards` and `make strict` were green on every task commit.

## Next Phase Readiness

**Ready for 32-04.**

What plan 32-07 inherits and must honour:

- **The inversion obligation is written into the case name and the case banner.** `CHECK(failing >= kNaiveFailuresFloor)` becomes `CHECK(failing == 0)`; the five `CHECK(naiveDb > threshold + 5.0)` lines become `CHECK(correctedDb <= threshold)`. Same slot, same grid, same cells. Both lines carry an inline `>>> PLAN 32-07 FLIPS THIS LINE <<<` marker.
- **`measureCellDb`'s `useMirror = false` branch is already the live-core path the corrected measurement will use.** Nothing in the comparator needs to change; only the assertions do.
- **The threshold column is PROVISIONAL and is plan 32-07's to re-pin** from this repository's own measurement of the real `forge::MorphBlep`, per every provenance string in the grid. The anti-softening clause is written into both the grid banner and the tombstone banner.
- **Cells to expect trouble from, ranked:** the 96 kHz regression rows (naive already within 2.1 dB of threshold on five cells, already passing on four); the 44.1 kHz C9 rows (the phase's hardest, where RESEARCH records its only ~1.5 dB regressions, all on the sine); and triangle C8 / C9 at character 0.5, which miss by only 1.768 and 2.026 dB naive.
- **`kSelfCheckDb` is still −62 dB** in the plan-32-01 apparatus case, still flagged for re-derivation. This plan did not need it and did not touch it.
- **`kThresholdFloorDb = -75 dB` bounds how tight any future threshold can be** without a corresponding improvement to the bin-centre solver. If plan 32-07's corrected measurement wants to assert tighter than −75 dB anywhere, the solver — not the floor — is what has to move.
- **The mirror-maintenance rule now has a third dependant:** `measureCellDb` drives `NaiveVcoCoreMirror` and `forge::VcoCore` through the same `driveSecondBlock` template. Any change to `forge::VcoCore::step`'s pitch / guard / accumulate sequence must be mirrored in `NaiveVcoCoreMirror`, or this comparator's two sides stop measuring the same thing.

## User Setup Required

None — no external service configuration required.

## Self-Check: PASSED

- `tests/test_vco_spectrum.cpp` — FOUND on disk
- `.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/32-03-SUMMARY.md` — FOUND on disk
- Commit `13ea1c7` — FOUND in `git log`
- Commit `1513d5b` — FOUND in `git log`
- All plan `<success_criteria>` re-run and green; all task `<acceptance_criteria>` re-run and green except the one superseded by Deviation 1, which is recorded above with its measurement and its replacement.

---
*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Completed: 2026-08-01*
