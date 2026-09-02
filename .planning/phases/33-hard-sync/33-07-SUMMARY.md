---
phase: 33-hard-sync
plan: 07
subsystem: tests
tags: [hard-sync, sync-02, d-01, d-06, d-11, register-item-8, threshold-pinning, per-cell-lookup, provenance, reproduction-check, mutation-probe, refused-gate-shape, snap-to-zero, sync-02-declined]

# Dependency graph
requires:
  - phase: 33-hard-sync
    plan: 05
    provides: "the sync sub-grid, SyncPlacementProbe, measureSyncCellDb, fundamentalDominanceDb, the plateau/step-dominated jump criterion, and the 420-cell instrument this plan turns into a gate"
  - phase: 33-hard-sync
    plan: 06
    provides: "the SHIPPED past-edge leg (forge::MorphBlep::addPastStep called from forge::VcoCore), and the bit-exactness gate ALREADY re-anchored to kLegPastEdge with the equality still exact"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "kThresholdFloorDb, the six kProv* strings, the measure-then-pin protocol, the derivation-plus-reproduction coupling, register item 8's 1.0 / 4.0 dB split and the measured 3.02596 dB toolchain divergence"
provides:
  - "SYNC_PINS — a 420-row PER-CELL lookup, each row carrying its measured decibel, its outward-rounded threshold, its tier and its written provenance; buildSyncGrid grows the lookup 33-05 said it would need"
  - "Nine NEW kProvSync* constants, asserted new rather than claimed new (non-empty, mutually distinct, distinct from Phase 32's six by CONTENT, each naming plan 33-07 and its rate)"
  - "THE GATE: 210 instrument-valid cells CHECKed against their pinned thresholds, worst headroom 1.00245 dB"
  - "The reproduction pass, PROVED to bite on both halves of its coupling"
  - "A discriminating mutation probe that fails its STATED populations EXACTLY: +2.0 dB fires 192 of 192 step-dominated and 0 of 18 plateau; +5.0 dB fires all 210"
  - "The two-clause plateau/step-dominated criterion, stated physically before any count, with both clauses measured to do work (192 jump-only, 24 shape-only, 186 both)"
  - "The IMPROVEMENT GATE REFUSED IN WRITING, with its measured reason (+0.5827 dB grid-wide) and the ratio table showing the correction is WORSE than none at 5.5"
  - "The snap-to-zero comparison as a permanent, non-circular case that consults no pinned threshold: 5.31-5.58 dB on informative masters, both halves and the discrimination asserted"
  - "The CLAIM 1 / CLAIM 2 split of the bit-exactness case, and the 12-cell exactly-zero-jump population it found"
affects: [33-08, 33-10, 33-11, 33-12]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Splitting one assertion that was true for two independent reasons into two separately labelled claims, so a future failure can say WHICH claim moved"
    - "A per-cell pin table keyed by all of a cell's axes, with the SENTINEL as the default, so a grid row added without a pin fires fourteen assertions instead of passing"
    - "Re-deriving a recorded TIER and a recorded CLASS from the run's own measurements, so a label edited to move a failing cell into a wider bound goes red"
    - "A mutation probe whose stated population is DERIVED from the threshold-derivation rule's own headroom interval, not observed and then written down"
    - "Refusing a gate shape IN THE SOURCE, with the measurement that makes it unwritable, rather than writing it and loosening it"
    - "Asserting the population where an effect is ABSENT alongside the population where it is present, so a restriction is evidenced rather than declared"

key-files:
  created: []
  modified:
    - tests/test_vco_spectrum.cpp

key-decisions:
  - "THE RE-ANCHOR WAS ALREADY DONE BY 33-06 AND WAS VERIFIED, NOT REPEATED. The gate reads kLegPastEdge at both call sites with a direct float ==; that acceptance criterion was already satisfied on arrival and is reported as such"
  - "The plateau/step-dominated criterion has TWO clauses, because a hard-synced cell has two independent sources of a value step. Clause (i) alone would have called a unity-ratio SAW cell a plateau"
  - "33-05's open question is answered: the instrument-INVALID half is NOT gated, decided on the instrument rather than on the numbers. 70 gated / 140 regression / 210 diagnostic"
  - "The improvement gate is REFUSED IN WRITING with its measured reason, and the refusal names the two warning signs the file is to be read against"
  - "The snap claim is a POPULATION mean per rate, not a per-cell inequality, because on 19 of 105 gated informative cells the snap leg measures BETTER"
  - "SYNC-02 is DECLINED — the TENTH consecutive decline. This plan closed exactly one of the two things 33-06 named as missing"

patterns-established:
  - "When a gate can be satisfied by disabling the thing on both sides of it, the non-vacuity partner belongs in the same case and must be labelled as a DIFFERENT claim"
  - "A threshold's derivation and its reproduction check are a PAIR; report which of the two a given edit reds, rather than describing the pair as one mechanism"
  - "A count assertion that is inverted by a later plan should have its DIRECTION inverted and its MECHANISM kept — a deleted count cannot detect the thing the count was for"

requirements-completed: []  # SYNC-02 is in this plan's frontmatter and is DECLINED — see Decisions #7

coverage:
  - id: D1
    description: "The bit-exactness gate measures the core that now ships, and the two claims it can support are separately labelled"
    requirement: "SYNC-02"  # contributes to; not completed by this plan
    verification:
      - kind: unit
        ref: "\"(D-06) the sync placement probe...past-edge leg\" — CLAIM 1: 1,720,320 samples, 0 mismatches, 140 cells / 573,440 samples per rate. CLAIM 2: 29,863 samples moved by the correction (12,523 / 11,496 / 5,844), both sides the PROBE"
        status: pass
      - kind: other
        ref: "PROVED ABLE TO FAIL: perturbing the probe's past-edge coefficient by 1.0001 reds it at 28,541 mismatches (11,849 / 11,076 / 5,616) and 411 failed assertions; restored and re-verified green"
        status: pass
    human_judgment: false
  - id: D2
    description: "The improvement gate this instrument cannot honestly carry is refused in writing, with its measured reason"
    verification:
      - kind: unit
        ref: "SyncCell banner lines 1312-1382; the measured mean +0.5827 dB is ASSERTED in range (0,1) by the D-11 gate case, and the ratio-5.5 sign (correction WORSE than none) is asserted permanently"
        status: pass
    human_judgment: false
  - id: D3
    description: "Every gated sync cell carries a measured value, an outward-pinned threshold, a tier and a non-empty provenance, asserted inside the test"
    requirement: "SYNC-02"
    verification:
      - kind: unit
        ref: "\"(SYNC-02 / D-11)...\" — 420 rows checked for provenance non-empty / not the sentinel / naming 33-07, measuredDb and thresholdDb not the sentinel, tier in the allowed set, the derivation asserted mechanically, and the floor asserted per cell"
        status: pass
      - kind: other
        ref: "PROVED: breaking ONE pin row's key so it matches no cell fires 14 assertions across 2 cases; deleting the row instead reds the pin-count REQUIRE"
        status: pass
    human_judgment: false
  - id: D4
    description: "The classification is stated on the physical criterion ABOVE the first population count, and both populations are asserted exactly"
    verification:
      - kind: unit
        ref: "criterion at lines 5452-5510, first population count at 5581 and its assertion at 5593. nStepDominated == 402 and nPlateau == 18, both exact; both clauses measured to do work (192 / 24 / 186)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The reproduction pass bites, on both halves of its coupling"
    verification:
      - kind: other
        ref: "Loosening ONE threshold alone (-31.0 -> -29.0, measured column untouched) reds the DERIVATION assertion at line 5685 (-29 == -31) and the mutation probe (191 of a stated 192). Loosening BOTH together (-32.2312/-31.0 -> -30.2312/-29.0) reds the REPRODUCTION check proper (delta 2 <= bound 1) and the probe. Restored and re-verified green both times"
        status: pass
    human_judgment: false
  - id: D6
    description: "The mutation probe fails a STATED population EXACTLY"
    verification:
      - kind: unit
        ref: "stated as literals before the loop: +2.0 dB fires 192 step-dominated and 0 plateau; +5.0 dB fires all 210. OBSERVED: 192 / 0 / 210. Equal"
        status: pass
    human_judgment: false
  - id: D7
    description: "SYNC-02's sub-sample clause is a permanent, non-circular measurement, with the instrument's own blind spot asserted alongside it"
    requirement: "SYNC-02"
    verification:
      - kind: unit
        ref: "\"(SYNC-02 / D-01) snap...\" — informative 5.5811 / 5.3051 / 5.4749 dB against a 1.0 dB pinned floor; hard-edge -0.2124 / -0.3832 / -0.3007 dB against a 5.0 dB closeness bound; DISCRIMINATION 5.79 / 5.69 / 5.78 dB against a 4.0 dB bound. Zero `thresholdDb` matches in lines 5931-6167"
        status: pass
      - kind: other
        ref: "PROVED ABLE TO FAIL: forcing the probe's past-edge reset to snap reds 4 cases at 750 assertions; this case fails exactly its 6 claim assertions of 226. Restored and re-verified green"
        status: pass
    human_judgment: false
  - id: D8
    description: "Nothing shipped moved"
    verification:
      - kind: unit
        ref: "make test 106 cases / 2,631,627 assertions 0 failures; six LFO goldens byte-identical (9 cases / 49,188); check_frozen.sh PASS; make strict and make guards exit 0; git diff --name-only across all three commits is tests/test_vco_spectrum.cpp alone; src/AnalogLFO.cpp absent"
        status: pass
    human_judgment: false

# Metrics
duration: 78min
completed: 2026-09-02
status: complete
---

# Phase 33 Plan 07: The Sync Sub-Grid Becomes a Gate Summary

**420 cells that measured and gated nothing now gate on thresholds pinned outward from this plan's own measurement of the leg that ships, each with its provenance, its reproduction check and a discriminating probe that fails a stated population exactly — and the gate shape this instrument CANNOT honestly carry is refused in the source with the measurement that makes it unwritable, rather than written and then loosened.**

## Performance

- **Duration:** 78 min
- **Tasks:** 3 of 3
- **Files modified:** 1

## The Re-Anchor: ALREADY DONE, VERIFIED RATHER THAN REPEATED

Task 1's first acceptance criterion asked this plan to re-anchor 33-05's bit-exactness gate from `kLegNone` to the leg 33-06 landed. **It was already done.** Landing the seam turned that gate red in 33-06's own commit at 412 failed assertions, and a red gate cannot be handed across a plan boundary, so 33-06 did it there and said so in its deferred register item 2.

**Verified against the file on arrival, not assumed:**

| Check | State found |
|---|---|
| Leg argument at both `measureSyncCellDb` call sites | **`kLegPastEdge`** |
| The comparison | **`probeBlock[i] != coreBlock[i]`** — a direct float `!=`, never doctest's approximate comparator |
| 33-05's original instruction | **quoted verbatim in the file**, not deleted |
| Result | **1,720,320 samples, 0 mismatches, 420 of 420 cells firing** |

**That criterion was satisfied on arrival and nothing about it was re-done or "improved".** The equality was not touched by one character in this plan either.

---

# TASK 1 — THE TWO CLAIMS, AND THE GATE SHAPE THAT IS REFUSED

## The assertion that was carrying two claims

Before 33-06 the case held a single assertion — probe on `kLegNone` against the live core, zero mismatches — and it happened to be true for **two independent reasons at once**: the probe reproduced the core's arithmetic (an **identity** statement), *and* the shipped core applied no correction (a **magnitude** statement, that the correction was zero). Landing the seam falsified the second while leaving the first true, and the assertion went red **without being able to say which claim had moved**.

The two are now separately labelled, and the labels are at distinct sites:

| Claim | Source line | The two sides |
|---|---|---|
| **CLAIM 1 — IDENTITY** | `tests/test_vco_spectrum.cpp:4111` | the **PROBE** on `kLegPastEdge` against the **LIVE CORE** |
| **CLAIM 2 — MAGNITUDE CONTROL** | `tests/test_vco_spectrum.cpp:4009` | the probe's own `kLegNone` leg against the probe's own `kLegPastEdge` leg — **both sides the probe** |
| **THE MAGNITUDE CLAIM, IN DECIBELS** | `tests/test_vco_spectrum.cpp:4563` | the same comparison expressed as a mean dB, in the 33-05 measurement case |

`grep -n 'no-correction\|noCorrection'` returns **942, 1715, 4009, 4111, 4563**. The label lines, quoted:

```
4111:	// This is the claim that used to be anchored to the no-correction leg and is
4009:		// CLAIM 2's second side: the probe's OWN no-correction leg.
4563:			// compares the probe's no-correction leg against the probe's shipped
```

**Why CLAIM 2 has to exist at all:** a bit-exactness gate is trivially satisfiable by disabling the thing on both sides of it. Remove the seam from `src/dsp/VcoCore.hpp` and from the probe's `kLegPastEdge` arm in the same edit and CLAIM 1 goes green on 1,720,320 samples while saying nothing. CLAIM 2 is what goes red then.

The comparison the pre-33-06 assertion literally was — probe `kLegNone` against the **live core** — is now the *sum* of the two claims and is deliberately **not** asserted, because a failure of it could not be attributed to either.

## CLAIM 2's numbers, and a population it found

| | 44.1 kHz | 48 kHz | 96 kHz | Total |
|---|---|---|---|---|
| Cells | 140 | 140 | 140 | **420** |
| Samples compared (CLAIM 1) | 573,440 | 573,440 | 573,440 | **1,720,320** |
| **Mismatches (CLAIM 1)** | **0** | **0** | **0** | **0** |
| **Samples moved by the correction (CLAIM 2)** | **12,523** | **11,496** | **5,844** | **29,863** |

Largest single-sample correction: **4.86088 V**.

**And CLAIM 2 found something.** The correction moves nothing at all on **12 of 420 cells** — and the physics predicts exactly which twelve, so the criterion is stated in the source before the count and both populations are asserted exactly:

> The seam deposits `-f*f*jump`. When the reset produces a jump of **exactly zero** it deposits exactly zero and the two legs are bit-identical — not approximately, identically. That needs the pre-reset and post-reset phases to land inside the **same flat segment of a piecewise-constant waveform**, which needs two things at once: a ratio at which the reset barely moves the phase, and a shape with a flat top to move within.

| Population | Cells |
|---|---|
| Correction moves samples | **408** |
| Sync jump exactly zero | **12** |
| …and all 12 in the predicted class (unity ratio × square/pulse centre × character 0.00, 3 rates × 2 edge shapes) | **12** |

A cell in that class is a cell with **nothing to correct**, and reporting it as a correction failure would be reporting the waveform.

## The gate is PROVED ABLE TO FAIL after the re-anchor

Perturbation: the probe's past-edge magnitude `-f*f*jump` → `-f*f*jump * 1.0001f`. A coefficient typo, deliberately tiny.

| | 44.1 kHz | 48 kHz | 96 kHz | Total |
|---|---|---|---|---|
| **Mismatches** | **11,849** | **11,076** | **5,616** | **28,541** |

**411 failed assertions, 1 failed case.** Restored from a pristine copy and re-verified: 0 mismatches, whole suite green.

## The improvement gate, REFUSED IN WRITING — `tests/test_vco_spectrum.cpp:1312-1382`

The obvious move for hard sync is to copy the milestone's existing improvement gate: TEST-03's `naiveDb − correctedDb >= 8.0`, which is Phase 32's strongest non-circular evidence precisely because it consults no pinned number. **It does not work here, and the reason is a property of the instrument.**

**The measured figure the refusal paragraph carries, quoted from 33-05's own leg table** (mean dB, `none` − `pastEdge`, positive = the shipped correction is better):

| rate | hard-edge | band-limited |
|---|---|---|
| 44.1 kHz | +0.061 | +1.053 |
| 48 kHz | −0.010 | +0.996 |
| 96 kHz | +0.174 | +1.222 |

**Grid-wide mean: +0.5827 dB.** 33-VALIDATION's Threshold Policy predicted "a mean of about 0.5 dB" from the research prototype and named the consequence in advance — *"a gate written in Phase 32's shape … therefore FAILS, and the failure is a property of the instrument, not of the implementation."* **Confirmed to within a twentieth of a decibel**, and this plan's own independent measurement of all 420 cells reproduces it at **+0.58268**.

**Measured by this plan, and it is worse than a small number — it is negative at the top of the sweep.** Mean `none` − `pastEdge` per ratio, over all 60 cells of each:

| ratio | mean | ratio | mean |
|---|---|---|---|
| 0.50 | **+2.4495** | 2.50 | +0.2051 |
| 0.75 | **+1.9150** | 3.50 | **−0.1911** |
| 1.00 | +0.0037 | 5.50 | **−1.0281** ← WORSE THAN NONE |
| 1.50 | +0.7247 | | |

At ratio 5.5 the shipped leg measures **worse than applying no correction at all on 47 of 60 cells, by up to 7.0218 dB on the worst single cell**. Grid-wide, 167 of 420 cells are worse. That is the forfeited pre-edge half showing up where the detected fraction is largest, it is **33-06's deferred register item 3**, and an improvement gate would have to carve those cells out — which is the anti-reclassification clause's forbidden move performed on a whole ratio.

**Where the sync correction's own non-circular evidence lives instead:** the time domain, plan 33-08's instrument (D-10). Register item 5 **measured** that a single-sample full-amplitude spike reads **0.0 dB spectrally** — the alias-floor metric is structurally blind to the artefact SC-3 exists to forbid. A spectral improvement gate for a click is not a weak gate, it is the wrong instrument.

**The two warning signs the file is now to be read against**, named in the source so a later reader can check for them rather than being told they do not happen:

1. **A gate that has to be loosened repeatedly.** One loosening is a mistake in the pin; a second is evidence the gate was written in a shape the instrument cannot support, and the response is to escalate and re-shape it.
2. **A threshold and its measured sibling drifting together.** That edit passes both the derivation assertion and the reproduction check while recording *agreement* rather than *measurement*.

## The provenance constants are NEW, and that is asserted rather than claimed

| | Before | After |
|---|---|---|
| `grep -c 'kProv'` (lines) | **101** | **121** |
| `grep -o 'kProv[A-Za-z0-9]*'` (occurrences) | **101** | **130** |
| **Distinct `kProv*` names** | **7** | **16** — exactly **+9** |

The nine new constants: `kProvSync441Step`, `kProvSync441Plateau`, `kProvSync48Step`, `kProvSync48Plateau`, `kProvSync96Step`, `kProvSync96Plateau`, `kProvSync441Invalid`, `kProvSync48Invalid`, `kProvSync96Invalid`.

**Why Phase 32's six could not be reused:** every one of them opens *"MEASURED by plan 32-07"*, names that plan's own run, and states the derivation `thresholdDb = ceil(measuredDb + 3.0)` with a flat 3 dB margin. **All three clauses are false of the sync rows** — measured by 33-07, on a different signal class, with register item 8's per-class bound rather than a flat 3 dB. `kProvSync441Step` as landed:

> "MEASURED by plan 33-07 in this repository AT 44.1 kHz - the BINDING rate - driving the SHIPPED past-edge leg through measureSyncCellDb with useLiveCore=false, on the cell named by this row's own five axes; STEP-DOMINATED class, so thresholdDb = ceil(measuredDb + 1.0), register item 8's step-dominated reproduction bound. **This is the alias floor the shipped leg REACHES here; it is not a claim that the correction improves this cell**."

That last sentence is on **all nine**, and it is 33-06's deferred item 3 honoured in the data rather than in a document.

The `(D-11)` sub-grid case now asserts the constants are new: each is non-empty, names `33-07`, names a rate, does **not** name `32-07`, and is distinct from all eight others **and from Phase 32's six by CONTENT** — a copy-paste of `kProvMeasured`'s text under a new name would defeat a pointer comparison and is exactly the edit worth catching.

---

# TASK 2 — THE GATE

## `SYNC_PINS` — the per-cell lookup 33-05 said would be needed

33-05 wrote, in the `SYNC_GRID` banner: *"PLAN 33-07 IS WARNED: the moment a per-cell threshold is pinned, that number needs a per-cell home and a per-cell provenance, and this builder must grow a **LOOKUP** rather than a **FORMULA**."* **420 rows**, keyed by all five axes, each carrying `measuredDb`, `thresholdDb`, `tier`, `provenance`, plus a trailing comment with the three physical quantities the class and the tier were decided from (`fundDom`, `jump`, `none`).

`buildSyncGrid` defaults to the **sentinel** and overwrites only on a five-axis match. A cell with no matching pin keeps `kSyncUnpinnedDb` and the tier `"UNPINNED"`.

**PROVED that a cell with no pin cannot pass silently.** Breaking one pin row's key (ratio `0.50` → `0.60`, so it matches no cell) fires **14 assertions across 2 cases**:

```
CHECK( unpinnedCells == 0 )                        CHECK( prov != std::string(kProvSyncUnpinned) )
CHECK( cell.measuredDb != kSyncUnpinnedDb )        CHECK( cell.thresholdDb != kSyncUnpinnedDb )
CHECK( (isDiagnostic || isAsserted) )              CHECK( (tier == "gated") == binding )
CHECK( namesStepBound == (stepDom[ci] != 0) )      CHECK( (double)cell.thresholdDb == expected )
CHECK( delta <= bound )                            CHECK( nGated == 70 )
CHECK( nAsserted == 210 )                          CHECK( nGatedStep == 192 )
CHECK( probe2Step == ... )                         CHECK( probe5All == ... )
```

*Deleting* the row instead reds the `REQUIRE(kSyncPinCount == 420)` first, which aborts the case — also correct, and also loud.

**33-05's `unpinnedCells` assertion was INVERTED, not deleted.** It counted unpinned cells and required all 420; it now counts them and requires **zero**. The direction of the claim changed; the mechanism did not, and a deleted count cannot detect an unpinned row.

## The classification — two clauses, stated physically, before any count

**Source line numbers, and the criterion is above the count:**

| | Line |
|---|---|
| The criterion, stated on its physical basis | **5452–5510** |
| Clause (i), the sync jump | **5474–5482** |
| Clause (ii), the slave's own discontinuity | **5484–5490** |
| The anti-reclassification clause, extended to the sync rows **by name** | **5502–5510** |
| **The first population count** | **5581** |
| **Its assertion** | **5593** |

**A hard-synced cell has TWO independent sources of a value step, and the question the bound asks is whether the CELL carries one — not whether the RESET does:**

- **(i) The sync jump.** Mean `|syncJump|` on the correction-free reference leg ≥ **0.01** pre-scale units. **That floor was fixed in plan 33-05's source, before any cell in this file was gated, and is inherited here unchanged** — which is the strongest possible answer to "produced by renaming the cells that failed": it predates the existence of a threshold to fail against.
- **(ii) The slave's own discontinuity.** Saw and pulse at every character, and square below full character. **Phase 32 measured exactly this partition** on the standing grid; sine and triangle carry a corner, and the square at character 1.00 was measured with its jump collapsed to −0.001661.

**A cell is step-dominated when EITHER holds.** Clause (i) alone would have called a unity-ratio **saw** cell a plateau — a waveform with a full-scale discontinuity every cycle — and granted it a 4.0 dB bound it has not earned.

**Both clauses do work, and that is asserted:**

| Population | Cells |
|---|---|
| Step-dominated by the **jump** alone | **192** |
| Step-dominated by the **shape** alone | **24** |
| By **both** | **186** |
| **STEP-DOMINATED total** | **402** |
| **PLATEAU** | **18** |
| Total | **420** |

Both totals asserted **exactly**. Under 33-05's single-clause criterion the split was 378/42; the 24 shape-only cells are the ratio-1.0 saw, pulse and hard-square rows that clause (ii) correctly reclaims.

## The tiers — and 33-05's open question, answered

33-05 asked 33-07 to *"decide whether to gate the instrument-invalid half at all."*

> **THE DECISION IS: NO, AND IT IS TAKEN ON THE INSTRUMENT RATHER THAN ON THE NUMBERS.** A cell that fails `fundamentalDominanceDb` is a cell where `aliasPeakDb` is normalising by a bin that is **not** the master's fundamental, so its decibel figure is not an alias floor and gating it would be gating a quantity the instrument does not produce.

| Tier | Cells | Behaviour |
|---|---|---|
| `"gated"` — 44.1 kHz, instrument-valid, the **BINDING** rate | **70** | measured, pinned, reproduction-checked, **CHECKed** |
| `"regression"` — 48 and 96 kHz, instrument-valid | **140** | measured, pinned, reproduction-checked, **CHECKed** |
| `"diagnostic"` — instrument-invalid, all rates | **210** | measured, pinned, **reproduction-checked**, never CHECKed |
| **Asserted against a threshold** | **210** | of which **192 step-dominated / 18 plateau** |

Every count exact. **The tier is a measurement, not a label:** it is re-derived from this run's own `fundamentalDominanceDb` result and the row's rate and must match what the table recorded — so a tier edited to move a failing cell into the diagnostic half goes red. The instrument-validity partition reproduces 33-05's exactly: **210 valid / 210 invalid, worst dominance −29.4473 dB**.

## The derivation, and the floor

```
thresholdDb == max(ceil(measuredDb + bound), kThresholdFloorDb)
    bound = 1.0 dB  (step-dominated)  |  4.0 dB  (plateau)     — register item 8
```

Asserted **mechanically per cell**, over all 420. The provenance string also names the bound its class earns, and the class is re-derived from the physical criterion — so a provenance swapped to buy a cell the wider bound reds too.

| | Value |
|---|---|
| Tightest pinned threshold on the whole sync grid | **−49.0 dB** |
| `kThresholdFloorDb` | **−75.0 dB** |
| **Cells where the floor BINDS** | **0** |

**The floor does not bind anywhere here, and that is recorded rather than left to be assumed.** No sync row is silently floored and no sync provenance has to claim it was. The per-cell floor assertion is still made over the whole grid, because a later re-pin at a quieter cell could reach it.

## The reproduction pass, and the coupling PROVED to bite on BOTH halves

> A threshold pinned from the implementation's own output **cannot, on its own, fail** — every gated cell passes by construction with at least its class bound of room. That is why the table is not the evidence on its own.

Two experiments, both run, both restored:

**Experiment A — loosen ONE threshold alone, measured column untouched.** Row `44100.0, hard-edge, 0.50, sine, char 0.00`: `thresholdDb` −31.0 → −29.0 (2.0 dB, more than its 1.0 dB class bound).

| Assertion | Result |
|---|---|
| `CHECK((double)cell.thresholdDb == expected)` — the **DERIVATION**, line 5685 | **RED: `-29 == -31`** |
| `CHECK(probe2Step == kStatedProbe2FiresStep)` — the **mutation probe** | **RED: `191 == 192`** |
| `CHECK(delta <= bound)` — the reproduction check *proper* | green, correctly |

**Experiment B — loosen BOTH together**, keeping the derivation satisfied: `measuredDb` −32.2312 → −30.2312 and `thresholdDb` −31.0 → −29.0.

| Assertion | Result |
|---|---|
| `CHECK(delta <= bound)` — the **REPRODUCTION CHECK**, line 5761 | **RED: `2 <= 1`** |
| `CHECK(probe2Step == ...)` | **RED: `191 == 192`** |

> **REPORTED PRECISELY RATHER THAN GLOSSED.** Task 2's acceptance criterion says *"loosen ONE threshold … and confirm the reproduction check goes RED."* What goes red on that edit is the **derivation assertion inside the reproduction pass**, plus the mutation probe — not the `delta <= bound` line. The two are a **PAIR** and the criterion's intent is fully met by running both halves: one edit is caught by the derivation, the other by the reproduction check, and there is no edit of either column that is caught by neither. The numbers are given rather than the verdict, following 33-05's deviation 3.

**Worst reproduction departure on this host: 5.05137e-05 dB** (the residue of storing `measuredDb` to four decimals as a float). Worst gate headroom: **1.00245 dB**.

## The mutation probe fails its STATED population EXACTLY

**Stated before the loop ran, as literals in the source, and DERIVED rather than observed:** every gated cell's headroom against its own pinned threshold is `ceil(measuredDb + bound) − measuredDb`, which lies in `[bound, bound+1)` by construction. Step-dominated headroom is therefore in `[1.0, 2.0)` and plateau in `[4.0, 5.0)`. An offset of +2.0 dB is above **every** step-dominated headroom and below **every** plateau one.

| Probe | STATED | OBSERVED | Equal? |
|---|---|---|---|
| +2.0 dB, step-dominated gated cells | **192** | **192** | **yes** |
| +2.0 dB, plateau gated cells | **0** | **0** | **yes** |
| +5.0 dB, all gated cells | **210** | **210** | **yes** |

**The stated and observed counts are equal, so no STOP-AND-REPORT is required here.** Measured headroom bounds confirm the derivation: step-dominated `[1.00245, 1.99488]`, plateau `[4.34388, 4.93598]`.

---

# TASK 3 — THE SNAP-TO-ZERO COMPARISON, LANDED PERMANENTLY

`.planning/research/STACK.md:149` carried the landmine as a table row and nothing more — *"Snapping sync reset to exactly `phase = 0` | Destroys sub-sample sync timing → sync itself aliases even with a BLEP | Reset to fractional overshoot."* That is an inherited assertion which stays true in the document whether or not the shipped core still honours it. **It is now a measurement that runs on every invocation.**

## It consults no pinned threshold

| Check | Result |
|---|---|
| Case + banner line range | **5931–6167** |
| `grep -c 'thresholdDb'` inside that range | **0** |
| How the gated population is selected | **by `tier`**, never by a decibel column |

The name of the threshold column is deliberately not written out even in the banner, so the grep stays clean. It compares **two measurements of the same apparatus** — the shipped leg's alias floor against the snap leg's on the same cell — exactly as the standing grid's 8 dB minimum-improvement CHECK does.

## The restriction is stated on its physical criterion BEFORE the rows are enumerated

| | Line |
|---|---|
| The informative-master criterion, with 33-RESEARCH's worked expression | **5964–5989** |
| **The enumeration of qualifying rows** (`if (cell.edge == kMasterBandLimited)`) | **6073** |

> `f = (HIGH − prev)/(now − prev)` recovers a **time** fraction only when the master's edge spans the threshold over a sample or more. For a single-sample full-scale wrap, the same expression returns the **voltage** fraction: for a ±5 V falling saw wrapping at HIGH = 1.0 V with true fraction `g`, `f ≈ 0.6 − g·dt_m` — `g` enters only at order `dt_m` and `f` is a nearly constant ≈0.6 with nothing to do with the crossing time.

**The observable, measured rather than assumed:** the sub-grid's apparatus case measures the spread of detected `f` and asserts `< 0.05` hard-edge and `> 0.20` band-limited. This run reproduces **0.0104–0.0230** against **0.9554–0.9834**.

## Both halves asserted, and the discrimination

**HALF ONE — the informative masters. The claim.** Mean `snap − shipped` over the 35 gated band-limited cells at each rate; positive = snap is worse:

| rate | mean margin | pinned floor | margin over the floor |
|---|---|---|---|
| 44.1 kHz | **+5.5811** | 1.0 | 4.58 |
| 48 kHz | **+5.3051** | 1.0 | 4.31 |
| 96 kHz | **+5.4749** | 1.0 | 4.47 |

**The pinning rule, and it is mechanical:** the least favourable of the three per-rate means (5.3051 at 48 kHz), rounded outward by register item 8's **plateau** bound — the widest present in either population, since both contain plateau cells — and then outward to the whole decibel: `floor(5.3051 − 4.0) = 1.0 dB`. **The floor is deliberately five times looser than the measurement**, and that is the comfortable margin being spent on the toolchain problem rather than on the claim.

> **THE PER-CELL SPREAD, RECORDED BECAUSE THE CLAIM IS A POPULATION CLAIM AND SAYING SO IS THE HONEST WAY TO MAKE IT.** On **19 of the 105** gated informative cells the snap leg measures **BETTER**, the worst by **3.2022 dB**; the best cell for the shipped leg is **+12.3463 dB**. A per-cell inequality here would be **false**, and a case that asserted one would have to be loosened later — which is warning sign (1), arriving in advance.

**HALF TWO — the hard-edge masters. Asserted, not omitted.** Mean `snap − shipped` over the 35 gated hard-edge cells at each rate:

| rate | mean | closeness bound | worst single cell |
|---|---|---|---|
| 44.1 kHz | **−0.2124** | 5.0 | |
| 48 kHz | **−0.3832** | 5.0 | |
| 96 kHz | **−0.3007** | 5.0 | **2.6497** (abs) |

Pinned by the same rule: `ceil(0.3832 + 4.0) = 5.0 dB`. **This is not a failure** — it is the measurement-design hazard rendered as an assertion. 33-RESEARCH predicted the two would measure *identically* there; measured, snap is **0.21 to 0.38 dB BETTER**. That direction is **recorded and not gated**: it is a two-tenths-of-a-decibel effect on an instrument Phase 32 measured toolchain-dependent by up to 3.02596 dB per cell, and gating a sign at that size would be gating rounding.

**THE DISCRIMINATION — what makes the two halves a result rather than two observations:**

| rate | informative − hard-edge | bound | |
|---|---|---|---|
| 44.1 kHz | **5.7934** | 4.0 | ✔ |
| 48 kHz | **5.6883** | 4.0 | ✔ |
| 96 kHz | **5.7757** | 4.0 | ✔ |

The fractional-overshoot reset buys something **exactly where there is sub-sample information to buy it with, and nothing where there is not** — which is what a sub-sample mechanism should do and what a coincidence would not. Without this, "we only assert on the band-limited rows" would be indistinguishable from "we only assert where it passed".

## The industry reference, recorded

VCV's own Fundamental VCO carries `syncSubsample = -lastSync / deltaSync` — **the identical linear crossing solve, interpolated to zero rather than to a hysteresis threshold**. The difference is deliberate here: this core drives the reset from the same hysteresis detector the gate uses, so the fraction it recovers is the one **consistent with the reset that actually fired**. 33-05 measured why that coupling matters — the leg fed the master's *true* wrap fraction is 0.45–0.71 dB **worse**, because 5.88 % of resets on a band-limited master fire one sample late.

## PROVED ABLE TO FAIL

Perturbation: the probe's reset forced to snap on the past-edge leg as well —
`phase = (leg == kLegSnap || leg == kLegPastEdge) ? 0.0 : ...`

| | Result |
|---|---|
| Cases red | **4** |
| Total failed assertions | **750** |
| **This case** | **6 failed of 226** — exactly its 3 `infMean > floor` and 3 `separation > bound` assertions |
| `(D-06)` bit-exactness gate | 420 cells mismatching |
| `(SYNC-02 / D-11)` gate | 229 reproduction + 73 gate failures |
| `(D-06 / D-11)` measurement | the snap and both probe sign claims |

Restored and re-verified: **106 cases, 2,631,627 assertions, 0 failures.**

---

# Suite Totals, Before and After

| | Test cases | Assertions |
|---|---|---|
| Before plan 33-07 (33-06's recorded totals) | 104 | 2,625,138 |
| After plan 33-07 | **106** | **2,631,627** |
| Delta | **+2** | **+6,489** |

### Per-case counts, matched-case count confirmed non-zero first

| Selector | Cases | Assertions |
|---|---|---|
| `vco spectrum: (D-11) the sync sub-grid*` | **1** | **204** |
| `vco spectrum: (D-06) the sync placement probe*` | **1** | **1,702** |
| `vco spectrum: (D-06 / D-11)*` | **1** | **499** |
| **`vco spectrum: (SYNC-02 / D-11)*`** | **1** | **5,286** |
| **`vco spectrum: (SYNC-02 / D-01) snap*`** | **1** | **226** |

## Task Commits

| # | Task | Commit | Type |
|---|---|---|---|
| 1 | The two claims separated, the improvement gate refused, the provenance constants (D-06 / D-11) | `7a87c5a` | test |
| 2 | The per-cell thresholds pinned with provenance (D-11 / register item 8) | `14507e4` | test |
| 3 | The snap-to-zero comparison, landed permanently (SYNC-02 / D-01) | `1c495c3` | test |

## Files Created/Modified

- `tests/test_vco_spectrum.cpp` — the `SYNC_PINS` lookup and its struct, nine provenance constants, two new `TEST_CASE`s, the refusal banner, the CLAIM 1 / CLAIM 2 split, and three comment corrections. **No other file in the repository was touched by any of the three commits.**

---

# Gate Results

| Gate | Result |
|------|--------|
| `make test` | **PASS** — 106 cases, 2,631,627 assertions, 0 failures |
| `make strict` (C++11, `-pedantic-errors`) | **PASS**, exit 0 |
| `make guards` | **PASS**, exit 0 (all three scripts) |
| `bash tests/check_frozen.sh` | **PASS** — D-05 manifest + goldens + negative control |
| Six shipped-LFO goldens | **byte-identical** — 9 cases / 49,188 assertions, 0 failures |
| Compiler warnings in the changed TU | **0** (`-Wall -Wextra`) |
| `src/AnalogLFO.cpp` in the whole-plan diff | **absent** |
| `git diff --name-only` across all three commits | **`tests/test_vco_spectrum.cpp` alone** |
| `tests/check_includes.sh` diffstat | **empty** — no new translation unit |
| Four frozen shared headers in the diff | **none** |

---

# Decisions Made

1. **THE RE-ANCHOR WAS VERIFIED, NOT REPEATED.** 33-06 discharged it one plan early because the gate reds in the landing commit. The leg argument, the exact float comparison and the preserved 33-05 instruction were all checked against the file before anything was written, and the criterion is reported as **already satisfied**. Re-doing it or "improving" it would have risked the one edit 33-05 warned against.

2. **The plateau/step-dominated criterion has TWO clauses.** The question register item 8's bound asks is whether the **cell** carries a true value step, and a hard-synced cell has two independent sources of one. Both clauses are properties of the waveform, both come from prior measurement (33-05's jump floor and Phase 32's shape partition), and both are measured to do work here (192 jump-only, 24 shape-only, 186 both) so neither is a decoration.

3. **The instrument-invalid half is NOT gated, and the decision is taken on the instrument.** Gating a cell where `aliasPeakDb` normalises by a bin 2.4–29.4 dB below the strongest lattice bin would be gating a quantity the instrument does not produce. Those 210 cells are still measured and still **reproduction-checked**, so a drift there is still visible; they are simply never CHECKed against a threshold. The alternative — gating them for coverage's sake — is coverage that means nothing, which is the failure this phase's register exists to prevent.

4. **The improvement gate is refused in the SOURCE, with its measurement.** Writing it and loosening it later is the documented failure mode; refusing it and naming where the evidence lives instead (33-08's time-domain instrument) is the honest alternative. 33-VALIDATION predicted the refusal in advance at "about 0.5 dB" and this plan measured **+0.58268** independently.

5. **Every threshold is pinned from the SHIPPED leg's own figure and asserts nothing about improvement.** All nine provenance strings say so in terms. 33-06's deferred item 3 required exactly this, and the ratio-5.5 sign (`mean none − pastEdge < 0`) is now **asserted permanently** so the region where the correction is worse than nothing cannot quietly stop being reported.

6. **The snap claim is a POPULATION mean per rate, not a per-cell inequality.** On 19 of 105 gated informative cells the snap leg measures better; a per-cell claim would be false and would be loosened later. The per-cell spread is recorded beside the means so the shape of the claim is auditable.

7. **SYNC-02 is DECLINED — the TENTH consecutive decline, and this plan closed exactly one of the two things 33-06 named as still missing.**

   33-06's decline named two gaps precisely:
   - **"No sync alias threshold is pinned."** — **CLOSED BY THIS PLAN.** All 420 cells carry a measured value, an outward-pinned threshold, a tier and a provenance; 210 are gated; the coupling is proved to bite on both halves; the probe fails its stated populations exactly.
   - **"'Click-free' has no instrument."** — **STILL OPEN, and it is plan 33-08's.** Register item 5 measured that a single-sample full-amplitude spike reads **0.0 dB spectrally**. The instrument this plan just finished gating is **structurally blind** to the artefact SC-3 forbids, and this plan wrote that blindness into the source as the reason its own improvement gate is refused. **Booking a click-free claim on it now would be the exact move the refusal paragraph argues against, in the plan that wrote the paragraph.**

   And the third reason 33-06 gave stands unchanged and is now **asserted rather than documented**: at master/slave ratio 5.5 the shipped correction measures **worse than no correction at all** (mean −1.0281 dB, 47 of 60 cells, worst 7.0218 dB).

   **`.planning/REQUIREMENTS.md` was CHECKED, not assumed, after this plan finished:** line 39 `- [x] **SYNC-01**` and line 134 `| SYNC-01 | Phase 33 | Complete |`; line 40 `- [ ] **SYNC-02**` and line 135 `| SYNC-02 | Phase 33 | Pending |`. **SYNC-02 remains `[ ]` / `Pending`.**

---

# Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] CLAIM 2's first form asserted a population that is physically false**

- **Found during:** Task 1, on the first `make test` after adding the magnitude control
- **Issue:** The control was written as `CHECK(cellsWhereCorrectionMoved == (int)nCells)` — "the correction moves something on every cell". Measured: **408 of 420**. Twelve cells carry a sync jump of **exactly zero**, so `addPastStep(f, 0)` deposits exactly zero and the two legs are bit-identical.
- **Fix:** The physics was worked out *before* the population was written down, and both are now in the source: a reset that stays inside one flat segment of a piecewise-constant waveform produces no jump, which needs a ratio at which the reset barely moves the phase **and** a shape with a flat top. This grid has exactly one such corner — unity ratio × the square and pulse centres at character 0.00. The assertion is now `cellsWithZeroJump == 12`, `zeroJumpCellsAtUnityFlatTop == 12` and `cellsWhereCorrectionMoved == 408`, all exact, plus a per-cell `CHECK((correctionDiff > 0) == !zeroJump)`.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** 420 per-cell checks green; the three population equalities green.
- **Committed in:** `7a87c5a`

**2. [Rule 1 — Bug] `kProvSync48Step`'s own text named plan 32-07 and tripped the constant's own "not a reuse" assertion**

- **Found during:** Task 1, first build
- **Issue:** The string explained that the cross-rate transfer "was FALSIFIED in plan 32-07", which is true — and made `CHECK(si.find("32-07") == std::string::npos)` fire. The assertion is right and the string was wrong: a provenance that names 32-07 anywhere is a provenance a reader can mis-attribute.
- **Fix:** Reworded to "was FALSIFIED in **phase 32**". The assertion was **not** relaxed to accommodate the string.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** All nine constants pass all six of their checks.
- **Committed in:** `7a87c5a`

**3. [Rule 1 — Bug] Two comments falsified by 33-06's case rename**

- **Found during:** Task 1, running the acceptance grep for `no-correction`
- **Issue:** The `SyncPlacementProbe` mirror banner quoted the case name *"…bit-exactly on the **no-correction** leg"*, which 33-06 renamed; and `driveSecondBlock`'s `master`-parameter banner said 33-05's claim is that the probe is the core "on the no-correction leg" without recording that it is now the past-edge leg. A banner quoting a case name that no longer exists is a banner nobody can follow.
- **Fix:** Corrected in place, both keeping the history rather than erasing it ("it said 'no-correction leg' until plan 33-06 landed the seam and re-anchored it").
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** Comment-only; suite green at unchanged totals.
- **Committed in:** `7a87c5a`

**4. [Rule 2 — Missing correctness] `CAPTURE` of a `const char*` prints a POINTER**

- **Found during:** Task 2, reading experiment A's failure log
- **Issue:** `CAPTURE(cell.edgeName)` logged `0x10503a511`. A per-cell gate whose failure log cannot name the cell that failed is a gate that costs a debugging session every time it fires — and this one has 420 cells.
- **Fix:** The two label columns go through `std::string` at all three CAPTURE sites.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** Re-ran experiment A; the failing cell now names itself.
- **Committed in:** `14507e4`

### Reported, not fixed

**5. [Reported] Task 2's reproduction-check acceptance criterion is narrower than its own prose — the TENTH such instance in this project**

- **Found during:** Task 2, running the acceptance experiment
- **Issue:** The criterion reads *"temporarily loosen ONE threshold by more than its class's bound without touching its measured sibling, and confirm the **reproduction check** goes RED."* On that edit the assertion that reds is the **derivation** assertion (line 5685) and the **mutation probe** — not the `delta <= bound` reproduction check, which is by design the half that catches the *other* edit.
- **Fix:** **Reported rather than satisfied.** Nothing was reshaped to make the literal wording true. Both experiments were run instead, and both are recorded with their assertions and values: the derivation catches a threshold moved alone, the reproduction check catches both columns moved together, and there is no edit of either column that is caught by neither. Following 33-05's deviation 3 and 33-06's deviation 5, the numbers are given rather than the verdict.
- **Files modified:** none — a criterion-interpretation decision
- **Verification:** The two experiment tables under "The reproduction pass" above.
- **Committed in:** n/a

**6. [Reported] Task 1's acceptance criterion assumed the re-anchor was still outstanding**

- **Found during:** Task 1, before any edit
- **Issue:** Part A asks this plan to change the leg argument. 33-06 had already done it, one plan early and for a reason it recorded in writing (the gate reds in the landing commit at 412 assertions).
- **Fix:** **Verified rather than re-done.** The prior-wave brief's instruction was followed exactly: the state was checked first, found already correct, and reported. What this plan added at that site is the part 33-06 did **not** do — the separation of the identity and magnitude claims, which is the other half of Part A's prose.
- **Files modified:** none for the re-anchor itself
- **Verification:** The re-anchor table at the top of this SUMMARY.
- **Committed in:** n/a

---

**Total deviations:** 4 auto-fixed (3 × Rule 1, 1 × Rule 2) + 2 reported
**Impact on plan:** One found a physically real population the plan's own assertion would have denied, and turned it into two exact counts. One caught a provenance string that would have misattributed a number — by the assertion written to catch exactly that, on its first run. Two were mechanical. Both reported items are criterion-interpretation calls where reshaping the code to fit the literal wording would have been the weaker outcome. **The whole-plan diff is `tests/test_vco_spectrum.cpp` alone.**

---

# Known Stubs

**None.** Every constant, row and helper this plan adds is consumed by an assertion in the same commit.

One thing is *absent by design* and belongs to a named later plan:

| Absence | Owner | Why it is not a stub here |
|---|---|---|
| No time-domain click instrument, so SC-3's click-free clause still has no gate | **plan 33-08** | Register item 5 measured that a single-sample full-amplitude spike reads 0.0 dB spectrally. This plan wrote that blindness into the source as the reason its own improvement gate is refused; building a spectral gate for a click here would contradict the paragraph in the same file. |

---

# Deferred Register Items

**1. NEW — the 1.0 dB step-dominated reproduction bound is INHERITED on this signal class, never measured on it.**
Register item 8's split was measured on Phase 32's **free-running** cells. The physical argument transfers — a true value step gives a broad skirt and a genuine arg-max — but the **number** has never been measured on hard-synced cells, and 192 of the 210 gated sync rows now depend on it with only 1.00245 to 1.99488 dB of headroom. **The SyncCell banner says this out loud and names the escalation:** if a step-dominated sync cell reproduces outside 1.0 dB on another toolchain, that is a **finding about the bound**, escalated per the anti-softening rule, and not absorbed by widening the column.
**Proposed Resolve-at:** plan 33-11, on the CI MinGW leg — that run is the first real measurement of this bound on this signal class.

**2. CARRIED and now ASSERTED — 33-06's item 3: the landed leg is measurably WORSE than no correction at high ratios.**
Measured by this plan across all 420 cells: mean `none − pastEdge` is **−0.1911 dB at ratio 3.5** and **−1.0281 dB at ratio 5.5**, with 47 of 60 ratio-5.5 cells worse and a worst single cell of **7.0218 dB**. 33-06 asked that 33-07 not pin a threshold there assuming the correction helps; **no threshold anywhere on this grid assumes it** — every one is pinned from the shipped leg's own figure and all nine provenance strings say so. The sign at ratio 5.5 is now asserted permanently, so the region cannot quietly stop being reported.
**Proposed Resolve-at:** unchanged — no code change in v2.0; a ratio-conditional correction is not available to a core that cannot know the master's frequency.

**3. NEW — 33-05's plateau population changed shape under the two-clause criterion, and the older figure is still in 33-05's SUMMARY.**
33-05 recorded 378 step-dominated / 42 plateau under clause (i) alone. Under the criterion this plan states, it is **402 / 18**: the 24 ratio-1.0 saw, pulse and hard-square cells are step-dominated because the **slave** carries the step. Both numbers are correct about their own criterion and neither document is wrong; a reader comparing them without the criterion in front of them will think one is.
**Proposed Resolve-at:** plan 33-11, when it reconciles the phase's documents.

**4. CARRIED — every decibel in `SYNC_PINS`, in this SUMMARY and in the new banners is an Apple-clang figure.**
Unchanged in kind from 33-01 through 33-06, but the exposure has changed shape: this is the first plan in the phase to **GATE** on absolute decibels from the sync path. 420 pinned numbers now exist that did not before, 210 of them CHECKed. `make strict` passes locally at C++11 `-pedantic-errors`; T-33-08 is not discharged locally and the CI MinGW leg remains plan 33-11's.

**5. CARRIED — 33-05's items 2, 3 and 5, 33-06's items 1, 4 and 5, and 33-02/03/04's six, are unchanged by this plan.**
33-05's item 2 (the spectral instrument is blind on hard-edge masters and on 210 of 420 cells) is now **built into the tier partition** rather than only recorded: the 210 invalid cells are diagnostic by decision, and the hard-edge blindness is **asserted** as the second half of the snap case. 33-06's item 2 (the re-anchor, closed early) is confirmed closed.

---

# Issues Encountered

- **The plan's own acceptance criteria assumed a state the previous plan had already changed, twice.** The re-anchor was done; the reproduction-check experiment reds a different assertion than the wording predicts. Both were resolved by checking the file first and reporting the numbers, which is what the prior-wave brief asked for in the first case and what 33-05's and 33-06's deviation precedents ask for in the second. The tempting move in both was to write something that made the criterion literally true.
- **The first version of CLAIM 2 asserted a population the physics denies.** Twelve cells were "wrong" for about four minutes before it became clear the assertion was wrong and the cells were right. The habit that caught it is the one this file keeps insisting on — work out the physical criterion, *then* look at the count.
- **The gate is honest but not comfortable at 1.0 dB.** 192 of 210 gated cells run on an inherited bound with under 2 dB of headroom, on an instrument that was measured 3.02596 dB toolchain-dependent on its older sibling — and that 3.02596 was a *plateau* cell, which is exactly why the split exists. This is the largest open risk in the table and it is registered as item 1 rather than hedged by silently widening the column.
- **`check_canary.sh [2b/5]` remains measurement-blind on this host** (33-03's finding). Irrelevant here — this plan adds no shipped code — but `make guards` going green was again treated as evidence of nothing about behaviour.
- **Two untracked `.planning/research/.cache/*.json` files** pre-existed at session start, as they did for 33-02 through 33-06, and were left alone.

---

# Next Phase Readiness

**The spectral half of SYNC-02 is closed. The instrument now gates, its numbers carry their provenance, and the gate it cannot honestly carry is refused in writing with the measurement that makes it unwritable.**

- **Plan 33-08 owns the other half, and it is the whole remaining half.** It inherits the snap figures (5.31–5.58 dB informative, now permanently asserted at a 1.0 dB floor) and, more usefully, the **written statement in `tests/test_vco_spectrum.cpp` of what the spectral instrument cannot see** — 0.0 dB for a single-sample full-amplitude spike, and 210 of 420 cells that cannot rank anything. Its D-10 per-sample delta bound is the only thing that can evidence SC-3.
- **Plan 33-10** is unaffected by this plan; its reconstruction relationship and its one-ulp error bar come from 33-06 unchanged.
- **Plan 33-11 inherits register item 1 as the phase's largest open risk.** Its CI MinGW leg is the first measurement of register item 8's step-dominated bound on hard-synced cells, and 192 gated rows depend on it. It also inherits items 2, 3, 4 and 5 above, plus 33-05's 2/3/5, 33-06's 1/4/5, and 33-02/03/04's six.
- **Plan 33-12** owns the operator UAT. It should read the SyncCell banner's refusal paragraph before writing the expected-results block: the sync correction's own spectral improvement is **half a decibel**, and an operator told to expect a dramatic difference would be told something the measurement does not support.

**Concerns carried forward:**

- **SYNC-02's remaining gap is now exactly one item, and it is named:** no instrument that can see a click. The mechanism is complete, the mechanism's spectral behaviour is gated, and the perceptual clause has nothing behind it until 33-08.
- **192 gated cells run on an inherited 1.0 dB bound with under 2 dB of headroom.** Register item 1.
- **The correction is worse than nothing at the top of the ratio sweep**, now asserted rather than documented.

---

# Self-Check: PASSED

Verified against disk and git rather than asserted:

- **Files exist:** `tests/test_vco_spectrum.cpp`, `.planning/phases/33-hard-sync/33-07-SUMMARY.md` — both **FOUND**.
- **Commits exist:** `7a87c5a`, `14507e4`, `1c495c3` — all **FOUND** in `git log`.
- **The two new cases are present in `HEAD`** and are matched by their selectors with non-zero counts: `(SYNC-02 / D-11)` **1 case / 5,286 assertions**, `(SYNC-02 / D-01) snap` **1 case / 226 assertions**.
- **The suite really did grow:** 104 → **106** cases, 2,625,138 → **2,631,627** assertions, 0 failures.
- **The pin table is genuinely there and genuinely consumed:** `kSyncPinCount == 420` is a `REQUIRE`, and `unpinnedCells == 0` over the built grid.
- **The re-anchor is genuinely on the shipped leg:** both `measureSyncCellDb` call sites in the bit-exactness case read `kLegPastEdge`, and the comparison is still a direct float `!=`.
- **The snap case consults no threshold column:** `grep -c 'thresholdDb'` over lines 5931–6167 is **0**.
- **Nothing shipped moved:** six LFO goldens byte-identical (9 cases / 49,188), `check_frozen.sh` PASS, `make strict` and `make guards` exit 0, **zero** compiler warnings, `src/AnalogLFO.cpp` absent from the whole-plan diff, `git diff --name-only` across all three commits is `tests/test_vco_spectrum.cpp` alone.
- **`.planning/REQUIREMENTS.md` was CHECKED, not assumed:** SYNC-01 remains `[x]` / `Complete`; **SYNC-02 remains `[ ]` / `Pending`.**

---
*Phase: 33-hard-sync*
*Completed: 2026-09-02*
