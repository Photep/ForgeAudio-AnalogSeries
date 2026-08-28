---
phase: 33-hard-sync
plan: 04
subsystem: tests
tags: [hard-sync, sync-01, doctest, structural-ceiling, withdrawal-phase, core-03, interleave, mutation-probe, falsified-premise]

# Dependency graph
requires:
  - phase: 33-hard-sync
    plan: 02
    provides: "the sync block in forge::VcoCore — detection, the guarded sub-sample solve, the fractional-overshoot reset, the unconditional store and the three recording-only telemetry floats. This plan is the permanent assertion of every one of them"
  - phase: 33-hard-sync
    plan: 03
    provides: "AnalogVCO::SYNC_INPUT — the jack that made SYNC-01 reachable by a user, which is the other half of the requirement this plan marks"
  - phase: 30-vco-core-registration
    provides: "tests/test_vco_core.cpp, tests/VcoBlockDriver.hpp, runInterleaveCheck and DeliberatelyBrokenSharedStateCore — the four helpers this plan extends and the one mirror it brings in step"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "kHostileBoundV / kMusicalBoundV, the two nested measured tiers, and scenario four's hostile-grid discipline (a named array, one physical case per entry, accumulate-then-assert)"
provides:
  - "Invariants 7, 8 and 9 in tests/test_vco_core.cpp — the FIRST permanent assertions of the hard-sync path in this repository"
  - "MasterBlock / makeMasterSaw / makeMasterSawBandLimited — a dyadic-increment master generator that knows its own TRUE wrap fraction, which plan 33-05 needs as a placement oracle"
  - "SyncTrace / driveTraced — per-sample sync telemetry through forge::VcoBlockDriver with that file byte-unchanged"
  - "HOSTILE_SYNC[] — nine sync voltages, each a named physical case, each driven twice and each WITHDRAWN"
  - "The CORE-03 interleave window extended to cover syncTrig and prevSyncVolts, with a per-member shared-static probe proving it can fail"
  - "DeliberatelyBrokenSharedStateCore mirrors the sync block, the moved conditioning and the post-sync `p` snapshot"
  - "SYNC-01 COMPLETE — the first requirement this phase closes, after three consecutive declines that each named this plan as the owner"
affects: [33-05, 33-06, 33-07, 33-08, 33-11, 33-12]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Observing per-sample core telemetry THROUGH a frozen block driver by recording at the top of the input functor — the functor runs immediately before step(i), so what it sees is step(i-1)"
    - "A hostile-input case with a WITHDRAWAL PHASE: the same instance driven with legitimate input afterwards, asserted separately, because a first-bad-step index cannot distinguish bad-during from bad-forever"
    - "Discharging a coverage clause by asserting an instrument's STRUCTURAL CEILING, with the limitation named in prose before the first assertion is written against it"
    - "Comparing a cross-rate invariant as three CONCRETE DESCRIPTIONS (count, count, pattern hash) rather than as a boolean, so a green result says what agreed"
    - "A mutation probe per NEW MEMBER rather than one per case, when the existing positive control shares a different member and therefore proves nothing about the new one"

key-files:
  created: []
  modified:
    - tests/test_vco_core.cpp
    - src/dsp/VcoCore.hpp

key-decisions:
  - "SYNC-01 IS MARKED COMPLETE. Three prior plans declined it and each named 33-04's assertions as the last missing piece; they now exist, at three sample rates, through the real POD boundary, with four discriminating mutants"
  - "SYNC-02 is DECLINED. Its sub-sample half is delivered and asserted here; its sync-BLEP half does not exist — the seam is deliberately withheld until plan 33-06"
  - "33-RESEARCH Pitfall 7 is FALSIFIED by measurement: a band-limited master cannot push the fraction out of [0,1], because the trigger only returns true when now >= 1.0 and only from LOW, so prev < 1.0 <= now by construction"
  - "The plan's own prescribed non-vacuity probe is INSENSITIVE and is reported rather than adjusted: both halves of the negated pair catch a not-a-number, so removing the lower one is green across the whole suite"
  - "DeliberatelyBrokenSharedStateCore mirrors the sync block even though the omission would have been behaviourally inert, following the plan-31-07 precedent that an inert drift is invisible exactly when it matters"
  - "src/dsp/VcoCore.hpp was edited against three tasks' single-file criteria, because this plan falsified a premise written in that header — comment-only, measured at zero non-comment lines"
  - "The tighter musical tier is withheld on every sync drive, with the measured headroom recorded, because the reset is un-band-limited until plan 33-06 and T-33-07 makes plan 33-08 the owner"

patterns-established:
  - "A hostile-input case must carry a withdrawal phase, or it books coverage it does not have — measured here at 0 reds during the hostile block and 21 after it"
  - "When a negated-comparison pair is used as a NaN guard, its two halves are REDUNDANT against NaN; a mutation probe aimed at one half comes back green and will be mistaken for a passing test"
  - "A behavioural independence check only covers the state its DRIVES exercise; extending the covered set means extending the drives, and the extension needs its own non-vacuity precondition or it is unfalsifiable"
  - "When a research pitfall is falsified, keep the conclusion and correct the premise in place with the structural reason — the reason is usually worth more than the correction"

requirements-completed: [SYNC-01]

coverage:
  - id: D1
    description: "A master rising edge through the real forge::VcoInputs boundary fires exactly one reset per master cycle at 44.1, 48 and 96 kHz"
    requirement: "SYNC-01"
    verification:
      - kind: unit
        ref: "invariant 7 subcase A — 32 resets per 4096-sample block, both master offsets, all three rates"
        status: pass
    human_judgment: false
  - id: D2
    description: "On every reset sample the phase equals (1 - f) * deltaPhase to double precision, and is strictly greater than zero on a live increment"
    requirement: "SYNC-01"
    verification:
      - kind: unit
        ref: "invariant 7 subcase A — phaseMismatch 0, nonPositive 0; minimum post-reset phase 0.00109 (96 kHz) to 0.00239 (44.1 kHz)"
        status: pass
    human_judgment: false
  - id: D3
    description: "syncConnected false fires nothing; a master peaking below the high threshold fires nothing; the hysteresis band is observed rather than assumed"
    requirement: "SYNC-01"
    verification:
      - kind: unit
        ref: "invariant 7 subcase B (0 / 0 / 32) and subcase B2 (a 0.5 V dip does not re-arm, a 0.05 V dip does — 2 fires, 0 in the middle rise)"
        status: pass
    human_judgment: false
  - id: D4
    description: "A master sample landing exactly on the high threshold makes the raw quotient exactly 1 and still leaves the phase strictly positive"
    requirement: "SYNC-02"
    verification:
      - kind: unit
        ref: "invariant 7 subcase C — rawQuotient == 1.0f asserted, recorded fraction == 0.f, phase == deltaPhase, all three rates"
        status: pass
    human_judgment: false
  - id: D5
    description: "SC-3's at-most-one-event-per-sample clause discharged by asserting the detector's structural ceiling, with the missed set identical at all three rates"
    requirement: "SYNC-01"
    verification:
      - kind: unit
        ref: "invariant 8 — eight masters from 1/16x to 4x the sample rate; firedSamples == firedCycles everywhere; equality with the wrap count below 0.4 cycles/sample; three per-rate descriptions (wraps, fired, FNV-1a pattern hash) identical at every dtm"
        status: pass
    human_judgment: false
  - id: D6
    description: "The instance is finite AFTER a hostile sync input is withdrawn, not merely during it"
    requirement: "SYNC-02"
    verification:
      - kind: unit
        ref: "invariant 9 subcase 1 — nine entries x two arming states x three rates, each followed by a 512-sample legitimate master on the SAME instance; the poisoning path pinned specifically (wFired > 0 and the first withdrawal fraction == the fallback)"
        status: pass
    human_judgment: false
  - id: D7
    description: "prevSyncVolts holds the immediately preceding sample's voltage on every branch, including unpatched samples and the first sample after construction"
    requirement: "SYNC-01"
    verification:
      - kind: unit
        ref: "invariant 9 subcase 2 — 400 samples, connected flag toggling every 7, storeMismatch 0; the NSDMI checked before the first step"
        status: pass
    human_judgment: false
  - id: D8
    description: "Neither the store nor the trigger is reset on a sample-rate change — the stated choice asserted, not inherited"
    requirement: "SYNC-01"
    verification:
      - kind: unit
        ref: "invariant 9 subcase 3 — 44.1 -> 96 kHz mid-drive; the first post-change sample FIRES (a reset trigger would return false from UNINITIALIZED) and its fraction is 0.322034 not 0.333333 (a reset store would give the latter)"
        status: pass
    human_judgment: false
  - id: D9
    description: "The recorded fraction stays inside its guarded range under a band-limited master, and the accumulator never leaves its own"
    requirement: "SYNC-02"
    verification:
      - kind: unit
        ref: "invariant 9 subcase 4 — five dyadic wrap fractions with 33-RESEARCH's own two-point residual; fracOutOfRange 0, rawOutOfRange 0, guardFired 0, spread 0.677579"
        status: pass
    human_judgment: false
  - id: D10
    description: "The new per-instance state sits inside the CORE-03 window, and that window is proved able to fail for those members specifically"
    requirement: "SYNC-01"
    verification:
      - kind: unit
        ref: "invariant 4 with sync drives — mismatchA 0/1024, mismatchB 0/1024 at all three rates; invariant 5 unmoved at 512/512/1024"
        status: pass
      - kind: other
        ref: "two out-of-tree shared-static probes: prevSyncVolts shared -> 961/982 of 1024; syncTrig shared -> 630/976 of 1024. Header restored and re-verified byte-identical"
        status: pass
    human_judgment: false
  - id: D11
    description: "Nothing shipped moved — the six LFO goldens, the frozen manifest and every recorded Phase 30/31/32 figure"
    verification:
      - kind: unit
        ref: "make test 100 cases / 2,623,356 assertions 0 failures; goldens 9 cases / 49,188 assertions byte-identical; check_frozen.sh PASS at 15 pinned entries; make strict and make guards exit 0"
        status: pass
    human_judgment: false
  - id: D12
    description: "In-Rack audition of hard sync"
    requirement: "SYNC-01"
    verification:
      - kind: other
        ref: "NOT VERIFIED HERE — no operator session in this plan, and judging by ear before plan 33-06 lands the seam would be judging an un-band-limited reset. Plan 33-12 owns the UAT"
        status: deferred
    human_judgment: true

# Metrics
duration: 39min
completed: 2026-08-29
status: complete
---

# Phase 33 Plan 04: SYNC-01 Asserted Summary

**`make test` gained its first hard-sync assertion in this project's history — 951 of them across three new invariants — and the two research premises those assertions were written against both turned out to be wrong, including the plan's own prescribed non-vacuity probe, which is green.**

## Performance

- **Duration:** 39 min
- **Started:** 2026-08-29T07:25:52+10:00
- **Completed:** 2026-08-29T08:04:40+10:00
- **Tasks:** 3 of 3
- **Files modified:** 2

## Accomplishments

- **Closed SYNC-01.** Three consecutive plans declined it, and each named this one as the owner of the missing piece. The piece now exists: 951 permanent assertions across three sample rates, through the real `forge::VcoInputs` boundary, with **eight discriminating mutants** behind them.
- **Falsified 33-RESEARCH Pitfall 7 by measurement, and the reason is worth more than the correction.** A band-limited master cannot push the sub-sample fraction out of `[0,1]`: the trigger only returns true when `now >= 1.0` and only from `LOW`, so `prev < 1.0 <= now` and `f ∈ (0,1]` **by construction**. Measured 0.150582 … 0.828161 where the research predicted 1.2.
- **Falsified the plan's own non-vacuity probe, and reported it rather than adjusting it.** Removing the negated **lower** comparison from the fraction guard is **green across the whole suite**, because both halves of the pair are negated and a not-a-number fails both. The discriminating mutant is the whole guard line — and its red lands **entirely in the withdrawal block**, which is the argument for the withdrawal phase measured rather than argued.
- **Measured the fact that makes plan 33-05's grid design non-negotiable.** For a **hard-edged** master the detector's fraction moves by **0.004** while the true wrap fraction **halves**; for a band-limited one it spans **0.678**. That is 33-RESEARCH Pitfall 10 arriving as a number, reproduced by the suite on every run.
- **Put the new per-instance state inside a CORE-03 window proved able to fail** — not by the existing control, which shares a different member, but by **one shared-static probe per new member**, both producing real reds on both instances at all three rates.
- **Brought `DeliberatelyBrokenSharedStateCore` in step a fourth time** when the omission would have been inert, because that is exactly the case the file's own banner says a mirror must not skip.

## Task Commits

| # | Task | Commit | Type |
|---|---|---|---|
| 1 | SYNC-01's reset, gate, hysteresis band and never-zero guarantee (D-01 / D-03) | `242a0f1` | test |
| 2 | The detector's structural ceiling and the new divisor's withdrawal phase (D-09 / D-12) | `7f2eeb3` | test |
| 3 | The CORE-03 interleave window extended to carry sync (CORE-03 / D-17) | `660fb70` | test |
| — | The falsified CORE-03 premise in the header, corrected in place (Deviations #1) | `863648a` | docs |

## Files Created/Modified

- `tests/test_vco_core.cpp` — three new `TEST_CASE`s (invariants 7, 8 and 9), five new anonymous-namespace helpers, the interleave invariant extended with requirement (vi), and the deliberately-broken control's mirror brought in step. The file banner's invariant list gained 7–9 and its item 4 records the sync extension.
- `src/dsp/VcoCore.hpp` — **comment only**, measured: 0 added non-comment lines, 0 removed non-comment lines. One paragraph, which this plan falsified.

## Suite Totals, Before and After

| | Test cases | Assertions |
|---|---|---|
| Before plan 33-04 | 97 | 2,622,378 |
| After plan 33-04 | **100** | **2,623,356** |
| Delta | **+3** | **+978** |

Plans 33-01, 33-02 and 33-03 added **zero** between them and each said so. This is the plan that pays that bill.

### Per-selector counts, with the matched-case count confirmed non-zero first

A selector that matches **zero** cases also exits 0 and prints `SUCCESS`, so the case count is read before the result. Verified directly: `-tc="vco sync: NO SUCH CASE*"` reports `test cases: 0 | 0 passed | 0 failed | 98 skipped` and `Status: SUCCESS!`.

| Selector | Cases | Assertions |
|---|---|---|
| `vco sync: (SYNC-01 / D-01*` | **1** | **99** |
| `vco sync: (SYNC-01 / D-09*` | **1** | **225** |
| `vco sync: (D-12*` | **1** | **639** |
| `*interleaving*` | **1** | **33** (was 18) |
| `*positive control*` | **1** | **6** |

`*interleave*` matches **nothing** — the case name reads *"interleaving"*. The selector in the plan's acceptance criteria is one character short of matching, which is worth recording because it would have reported a green zero-case run.

## Invariant 7 — SYNC-01's Reset, Measured

| Subcase | What it drives | Result |
|---|---|---|
| A | ±5 V falling saw, 32 master cycles per 4096 samples, two phase offsets, three rates | **32 resets** each; `phaseMismatch` **0**; `nonPositive` **0** |
| B | unpatched / 0.9 V peak / both thresholds crossed | **0 / 0 / 32** |
| B2 | 0.05 → 3 → **0.5** → 3 → 0.05 → 3 V | **2** fires, **0** on the middle rise |
| C | a sample landing exactly on 1.0 V | raw quotient **exactly 1.0f**, recorded fraction **0.f**, phase **== deltaPhase** |

**Post-reset phase minima** (the strictly-positive claim, on a live increment): 0.00239171 / 0.00219738 / 0.00109869 at 44.1 / 48 / 96 kHz.

**Envelope:** 4.920715 / 4.920976 / 4.921710 V.

### The measurement that changes plan 33-05's grid design

| Master offset | TRUE wrap fraction `g` | Detector's fraction `f` |
|---|---|---|
| `phi0 = 0` | **1.000000** at every wrap | **0.596850** |
| `phi0 = 0.5/128` | **0.500000** at every wrap | **0.600787** |

`g` halves; `f` moves by **0.004**. Linear interpolation across a discontinuity has no information about where inside the sample the discontinuity was — so a placement grid built on hard-edged masters alone measures nothing about placement. Both offsets are driven on every run, so the number is reproduced rather than quoted.

## Invariant 8 — The Structural Ceiling

The limitation is written in the case banner **before the first assertion**, together with the rejected alternative (inferring the master's rate) and both reasons for rejecting it. Measured, identical at all three rates:

| dtm | master vs. rate | wraps | observed | what the ceiling costs |
|---|---|---|---|---|
| 0.0625 | 1/16× | 512 | **512** | nothing |
| 0.125 | 1/8× | 1024 | **1024** | nothing |
| 0.25 | 1/4× | 2048 | **2048** | nothing |
| 0.75 | 3/4× | 6144 | **2048** | 2 of every 3 |
| 1.0 | **1×** | 8192 | **0** | **every edge** |
| 1.5 | 3/2× | 12288 | **4096** | 2 of every 3 |
| 2.5 | 5/2× | 20480 | **4096** | 4 of every 5 |
| 4.0 | **4×** | 32768 | **0** | **every edge** |

### The three concrete descriptions, as required — not a boolean

Per `dtm`, the description is the triple *(total wraps, fired cycles, FNV-1a hash of the whole fired/missed pattern indexed by the MASTER's cycle)*. The hash is what stops two rates agreeing on *how many* edges were missed while disagreeing on *which*.

| dtm | 44.1 kHz | 48 kHz | 96 kHz |
|---|---|---|---|
| 0.0625 | (512, 512, 9852883976742657411) | (512, 512, 9852883976742657411) | (512, 512, 9852883976742657411) |
| 0.75 | (6144, 2048, 4295785712143866755) | (6144, 2048, 4295785712143866755) | (6144, 2048, 4295785712143866755) |
| 1.0 | (8192, 0, 2089881884828795779) | (8192, 0, 2089881884828795779) | (8192, 0, 2089881884828795779) |
| 2.5 | (20480, 4096, 14049534513171092355) | (20480, 4096, 14049534513171092355) | (20480, 4096, 14049534513171092355) |

*(0.125, 0.25, 1.5 and 4.0 agree identically and are omitted for width; all eight are asserted.)*

**The `1×` and `4×` rows are the honest face of the ceiling and are kept for that reason.** At an integer ratio the master is a **constant +5 V** and the trigger never re-arms, so the edges are not under-sampled, they are **invisible**. Nothing in the case pretends otherwise.

**Why the agreement is evidence and not a tautology:** the masters are parametrised by cycles-per-**sample**, never by Hz, so the voltage sequence is bit-identical across rates. The assertion is a permanent tripwire on the design boundary D-09 rejects — a detector that ever started reading `sampleRate` would part the three descriptions. Proved: a mutant making the high threshold rate-dependent reds exactly `CHECK(descHash[1] == descHash[2])`.

## Invariant 9 — The Divisor, and the Half That Only Appears After Withdrawal

### `HOSTILE_SYNC[]`, quoted in full as the acceptance criterion requires

```cpp
static const float HOSTILE_SYNC[] = {
	5.f,                                        // a +5 V gate ALREADY HIGH when the cable is patched — Pitfall 5's stale-store zero-divisor case, verbatim
	2.f,                                        // THE LEGITIMATE CONTROL EDGE: an ordinary +2 V gate. It MUST fire, or this grid passes by never detecting anything
	1.f,                                        // EXACTLY the 1.0 V high threshold — the raw quotient is exactly 1 and lands on the guard's STRICT upper bound
	0.1f,                                       // EXACTLY the 0.1 V low threshold — the arming edge of the hysteresis band, held rather than crossed
	std::numeric_limits<float>::quiet_NaN(),    // a mis-wired host, an uninitialised port read, or an upstream 0/0 — the ONLY entry that can poison `phase` PERMANENTLY
	std::numeric_limits<float>::infinity(),     // an upstream overflow on a cable: it DOES fire, with a divisor of +infinity, giving a fraction of exactly zero
	-std::numeric_limits<float>::infinity(),    // its sign partner: it cannot fire, but it IS stored, and inf/inf on the next real crossing is a not-a-number
	1e30f,                                      // finite and enormous: passes a naive std::isfinite check while dwarfing every threshold
	std::numeric_limits<float>::denorm_min(),   // an upstream underflow: below the low threshold, so it ARMS rather than fires
};
```

`grep -c 'HOSTILE_SYNC' tests/test_vco_core.cpp` → **3** (the definition, its banner and its use). Every entry carries a trailing comment. Every entry is **held constant** across its block — that is how the equal-consecutive-samples case is delivered — and each is driven **twice**, once from sample 0 and once after a 0 V arming prefix, because only the second state can reach the divisor at all.

**Non-vacuity:** the legitimate control edge fires exactly once (`hFired == 1`, asserted), and the grid-wide `totalFiredAnywhere > 0` is asserted per rate.

### The withdrawal phase, and the mutation that proves it is load-bearing

| Assertion | reds during the hostile block | reds after withdrawal |
|---|---|---|
| `CHECK(hFinite)` / `CHECK(hRange)` | **0** | — |
| `CHECK(wFinite)` | — | **9** |
| `CHECK(wRange)` | — | **9** |
| `CHECK(wFirstFrac == 0.f)` | — | **3** |

**Zero reds during, twenty-one after.** A case that stopped at the hostile block would have reported `SUCCESS` on a core whose instances were permanently dead — which is precisely 33-02's measured 200-of-200 poisoning trace, now pinned permanently.

### Store invariant, and the sample-rate choice

- **Store:** 400 samples, connected flag toggling every 7 (so unpatched stretches are covered), `storeMismatch` **0**; the NSDMI checked on a fresh core before the first step. A mutant gating the store on `syncConnected` produces **3 reds, on that one assertion, at that one line**.
- **Rate change:** 44.1 → 96 kHz mid-drive. A **surviving trigger** is what makes the first post-change sample fire at all (a reset trigger takes `SchmittTrigger`'s UNINITIALIZED arm, which sets HIGH and returns **false**). A **surviving store** is what makes the fraction **0.322034** rather than **0.333333**. Both asserted bit-exactly. A mutant resetting both on a rate change produces **6 reds** across exactly those two assertions.

### The band-limited master — Pitfall 7 falsified, and one fact for plan 33-05

| true wrap fraction `g` | detector's fraction `f` | raw quotient | fires |
|---|---|---|---|
| 0.03125 | 0.249813 | 0.249813 | 16 / 16 |
| 0.25 | 0.466092 | 0.466092 | 16 / 16 |
| 0.5 | 0.634737 | 0.634737 | 16 / 16 |
| 0.75 | 0.828161 | 0.828161 | 16 / 16 |
| **0.96875** | **0.150582** | 0.150582 | 16 / 16 |

`f == raw` on **every** firing sample: the guard never had to intervene, and that is asserted (`guardFired == 0`) rather than assumed. Spread **0.677579**, against the hard-edged master's 0.004.

**The g = 0.96875 row is a finding plan 33-05 should inherit rather than rediscover.** The residual pushes the wrap sample to **0.31 V**, below the high threshold, so the detector fires **one sample late** with a fraction of 0.150582 instead of the ≈0.99 the geometry suggests. That is a one-sample placement error under a band-limited master, present *before any seam exists*.

## Invariant 4 — CORE-03 Extended, and Proved Able to Fail

**The two drives, as the acceptance criterion requires them recorded:**

| | master cycles across the 1024-sample block | master Hz at 44.1 / 48 / 96 kHz | jack |
|---|---|---|---|
| Instance A | 16 (`dtm = 16/1024`) | 689.06 / 750.00 / 1500.00 | patched throughout |
| Instance B | 24 (`dtm = 24/1024`) | 1033.59 / 1125.00 / 2250.00 | **unpatched over samples 384…639** |

Non-vacuity, `REQUIRE`d ahead of every value assertion: **firedA 16, firedB 18** (24 cycles less the 6 inside the unpatched window), **firedBWhileUnpatched 0**, and `firedA != firedB`. Without this line a functor that stopped setting `syncVolts` would leave the case passing exactly as it did before Phase 33 while still claiming to cover the new members.

**The property, at all three sample rates:** `mismatchA` **0 / 1024**, `mismatchB` **0 / 1024**, `soloEqual` **0 / 1024**. The solo-equality precondition against `forge::VcoBlockDriver` still runs first and still passes.

### The positive control, re-run — figures compared as numbers, not re-typed

| | 44.1 kHz | 48 kHz | 96 kHz |
|---|---|---|---|
| Recorded in the file before this plan | 512 / 512 / 1024 | 512 / 512 / 1024 | 512 / 512 / 1024 |
| **Measured after this plan** | **512 / 512 / 1024** | **512 / 512 / 1024** | **512 / 512 / 1024** |
| Difference | **none** | **none** | **none** |

**No difference to report** — the fourth consecutive capture at those numbers. And the file's own saturation caveat is restated beside it rather than glossed: these three figures are pinned at the ceiling of their own metric and can only detect a change that pushes the count **down**. "Unchanged" there is insensitivity, not evidence of inertness. The evidence of inertness is separate and checkable: invariant 5 drives `coreBase()`, which leaves `syncConnected` at its header default of `false`, so the mirrored sync branch is **not evaluated at all**.

### Non-vacuity for the NEW state specifically

The permanent control shares a **phase accumulator**, which says nothing about whether the check can see a shared **trigger**. One probe per new member, each turning exactly one member into a process-wide static and changing nothing else:

| Probe | mismatchA | mismatchB |
|---|---|---|
| `prevSyncVolts` shared | **961 / 1024** | **982 / 1024** |
| `syncTrig` shared | **630 / 1024** | **976 / 1024** |

Both at all three rates. **Neither figure is saturated**, so unlike the permanent control's 512/512 these can move in either direction and are a real pin.

**A near-miss worth recording.** The first attempt at the store probe deleted the member outright. The build **failed**, `make test >/dev/null 2>&1` swallowed the error, and the **stale binary ran and reported all green**. That green was mistaken for "the probe does not discriminate" for about a minute. The corrected probe keeps the member and redirects only the read. **A mutation probe must have its build verified before its result is read** — the same class of trap as the local gate returning exit 0 on a commit that could not link, which is the lesson `tests/test_vco_core.cpp`'s own banner opens with.

## The Full Mutation Register — Eight Probes, Six Distinct Signatures

All built against a scratch copy of `src/dsp/VcoCore.hpp`, restored and re-verified byte-identical afterwards. No repository artifact.

| # | Mutation | Reds | Where |
|---|---|---|---|
| M1 | fraction fallback `0.f → 1.f` | 12 | invariant 7 subcase C |
| M2 | upper bound made non-strict (`f <= 1.f`) | 12 | invariant 7 subcase C — **identical to M1** |
| M3 | low threshold `0.1 → 0.6` | 6 | invariant 7 subcase B2 |
| M4 | connected gate removed | 3 | invariant 7 subcase B, unpatched drive |
| **N1** | **negated LOWER comparison removed** | **0** | **nowhere — see Deviations #2** |
| N2 | whole guard line removed | 21 (+12 in inv. 7) | invariant 9 **withdrawal block only** |
| N4 | store gated on `syncConnected` | 3 | invariant 9 subcase 2 |
| N5 | trigger + store reset on a rate change | 6 | invariant 9 subcase 3 |
| N6 | detection made rate-dependent | 1 | invariant 8's cross-rate pattern hash |
| P1 | `prevSyncVolts` shared static | 6 | invariant 4, both instances |
| P2 | `syncTrig` shared static | 6 | invariant 4, both instances |

M1 and M2 share a signature, which reproduces exactly what plan 33-02 recorded for its mutants B and C on the same guard.

## Gate Results

| Gate | Result |
|------|--------|
| `make test` | **PASS** — 100 cases, 2,623,356 assertions, 0 failures |
| `make strict` (C++11, `-pedantic-errors`) | **PASS**, exit 0 |
| `make guards` | **PASS**, exit 0 |
| `bash tests/check_frozen.sh` | **PASS** — 15 pinned entries |
| Six shipped-LFO goldens | **byte-identical** — 9 cases / 49,188 assertions, 0 failures |
| Compiler warnings in the changed TU | **0** (`-Wall -Wextra`) |
| `git diff --stat tests/VcoBlockDriver.hpp` | **empty** — the driver is byte-unchanged |
| `src/AnalogLFO.cpp` in the whole-plan diff | **absent** |
| `grep -n 'estimateFreqRising'` | 5 occurrences — `:176` (definition), `:706/:717/:722` (invariant 1's banner) and `:777` (its single call). **None inside any new case** |
| `grep -c 'HOSTILE_SYNC'` | **3** |

## Decisions Made

1. **SYNC-01 IS MARKED COMPLETE, and this is the first requirement Phase 33 closes.** Its three clauses are now each delivered *and* asserted: *"hard sync input"* — `AnalogVCO::SYNC_INPUT`, wired through five sites and marked on the panel (33-03); *"resets oscillator phase"* — the sync block (33-02), asserted bit-exactly against `(1 - f) * deltaPhase` at three rates here; *"on a master rising edge"* — the `LOW → HIGH` arm, asserted together with its gate, its hysteresis band and its structural ceiling here. Plans 33-01, 33-02 and 33-03 declined it and **each named this plan's assertions as the only thing standing between the requirement and complete** (33-03 Decisions #5, verbatim). They exist. The seventh consecutive decline in this project's history of them is where the streak ends, and it ends because the evidence arrived rather than because the standard moved.

2. **SYNC-02 is DECLINED.** Its **sub-sample fractional placement** half is delivered and is asserted here from four directions (the bit-exact reset, the exactly-1 landmine, the band-limited fraction, the withdrawal). Its **sync-BLEP (click-free)** half does not exist: the seam is deliberately withheld so plan 33-05 can measure the placement, and plan 33-06 lands it. Marking it now would book a click-free claim on a reset that is currently un-band-limited.

3. **The tighter musical tier is withheld on every sync drive — with the measured headroom recorded, which is what makes the withholding auditable.** Every sync scenario measures ≈4.92–5.00 V and would clear 5.55 V with more than half a volt to spare. It is withheld because the reset is un-band-limited until 33-06, so a tier asserted now would pin a transient about to change, and T-33-07 makes plan 33-08 the owner. The outer `kHostileBoundV` binds every new scenario with no exception, as plan 32-08 requires. **The headroom figures are written into the source so 33-08 does not start from nothing.**

4. **`DeliberatelyBrokenSharedStateCore` mirrors the sync block, when omitting it would have been inert.** Invariant 5's drives leave the jack unpatched, so a mirror without the block would have been behaviourally exact *today*. That is precisely the argument the plan-31-07 paragraph in that banner **rejects** — the FM addition was inert on the same drives and was mirrored anyway, because *"a stand-in drifting from what it claims to mirror is invisible precisely when the drift is inert"*. The one line **not** mirrored (the pre-reset `morphedWave` call, which is `const` and feeds telemetry only) is named in the banner rather than left to be noticed.

5. **The interleave extension gets its own non-vacuity precondition, `REQUIRE`d before every value assertion.** Without `firedA > 0 && firedB > 0 && firedA != firedB`, a functor that quietly stopped setting `syncVolts` would leave invariant 4 passing exactly as it did before Phase 33 while its banner claimed to cover the two new members. That is the same failure shape the file's own validity-first habit exists to prevent, arriving through the extension rather than through the helper.

6. **The masters in the interleave drives are closed-form, never accumulated.** `runInterleaveCheck` calls each functor **four** times over the block — solo A, solo B and the two interleaved instances. A stateful generator would hand the four runs four different masters and the case would fail for a reason that has nothing to do with the core.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 — Missing critical documentation] This plan falsified a premise written in `src/dsp/VcoCore.hpp`, against three tasks' single-file criteria**

- **Found during:** Task 3
- **Issue:** The header's per-instance-state banner said the sync members were inside the CORE-03 window *"ONLY ONCE THE INVARIANT DRIVES SYNC, AND THAT IS NOT TRUE YET (plan 33-02)"*, and named plan 33-04 as what would make the claim true. Commit `660fb70` made it true, which makes the sentence **false**. All three tasks' acceptance criteria require `git diff --name-only` to list only `tests/test_vco_core.cpp`, so honouring them literally would have left the shipped header stating that CORE-03 does **not** cover sync when it now does — the exact failure this project's house rule about falsified premises exists to prevent, and one 33-02 wrote the forward reference specifically to avoid.
- **Fix:** Corrected in place in a **separate `docs` commit** (`863648a`), quoting what the paragraph used to say and carrying the two shared-static probe figures so the "proved able to fail" claim carries its own evidence. Three other forward references to plan 33-04 in the same header (`:748`, `:798`, `:890`) were **left alone** — they read as statements of fact and are all still true; only the one containing *"NOT TRUE YET"* was false.
- **Files modified:** `src/dsp/VcoCore.hpp`
- **Verification:** Added lines that are not comments: **0**. Removed lines that are not comments: **0**. Suite totals unchanged from `660fb70` (100 cases, 2,623,356 assertions). Six LFO goldens byte-identical. `make strict`, `make guards`, `check_frozen.sh` all exit 0. `src/AnalogLFO.cpp` absent from the diff.
- **Committed in:** `863648a`

**2. [Rule 1 — Bug] Task 2's prescribed non-vacuity probe is INSENSITIVE, and the reason is a real property of the guard**

- **Found during:** Task 2
- **Issue:** The criterion reads *"temporarily remove the negated lower comparison from the fraction guard … and confirm CASE TWO goes RED specifically in the withdrawal block"*. **Measured: it is green.** Not merely green in that case — green across the entire 100-case suite. Both halves of `if (!(f >= 0.f) || !(f < 1.f))` are **negated comparisons**, and a not-a-number fails `f >= 0.f` **and** `f < 1.f`, so **either half alone catches it**. The pair is redundant against the one input class the lower half was written for. Each half is individually load-bearing only for its own *finite* out-of-range direction, and `f < 0` is unreachable at a firing sample (see Deviation #3). Reporting the criterion as met would have booked a discrimination that does not exist.
- **Fix:** Reported rather than adjusted, and the discriminating mutant found and measured instead: the **whole guard line removed**, which reds **0 times during the hostile block and 21 times after withdrawal** — a sharper demonstration of the withdrawal phase's value than the prescribed probe could ever have given. The finding is written into the case banner with the full mutation table, so a future reader aiming a probe at the lower comparison does not mistake its green for a passing test. The lower comparison is **kept**: it costs one comparison on sync samples only, it is the file's standing idiom, and a future master-conditioning stage could make `f < 0` reachable.
- **Files modified:** `tests/test_vco_core.cpp` (the banner)
- **Verification:** N1 whole-suite result `100 cases / 2,623,341 assertions / 0 failures`. N2 result tabulated above.
- **Committed in:** `7f2eeb3`

**3. [Rule 1 — Bug] 33-RESEARCH Pitfall 7's premise is false; its conclusion is kept**

- **Found during:** Task 2, writing the band-limited subcase against it
- **Issue:** Pitfall 7 states that a band-limited master pushes the fraction **out of** `[0,1]`, working it to **1.2 at g = 1**, and the plan's `<behavior>` inherits that. **Measured across the research's own two-point residual at five wrap fractions and three rates: 0.150582 … 0.828161.** It never approaches 1. The structural reason: `f > 1` requires `now < 1.0`, but the trigger only returns true when `now >= 1.0`, and only from `LOW`, which means the previous sample failed the same comparison — so `prev < 1.0 <= now` and `f ∈ (0,1]` **by construction**. Pitfall 7's worked value of 1.2 is taken at `g = 1`, where the residual drives the wrap sample to **0 V** — and at 0 V the trigger does not fire, so that fraction is never computed.
- **Fix:** The subcase was **kept and made stronger** rather than deleted. It now asserts what is actually true and checkable — `fracOutOfRange == 0`, `rawOutOfRange == 0`, and `guardFired == 0` (the recorded fraction **is** the raw quotient, bit for bit, so the guard demonstrably never intervened) — plus the fraction-spread contrast against the hard-edged master, which is the thing the subcase turns out to be genuinely good for. The falsified premise, the structural reason, and what the guard's lower bound is therefore *actually* for (a not-a-number, by two reachable routes, both in the grid) are recorded in the case banner. **The conclusion — guard both ends — is unchanged**, and is why the pitfall is kept.
- **Files modified:** `tests/test_vco_core.cpp` (the banner and the subcase)
- **Verification:** The five-row table above, at all three rates.
- **Committed in:** `7f2eeb3`

**4. [Rule 3 — Blocking] The plan's interleave selector matches zero cases**

- **Found during:** Task 3
- **Issue:** Task 3's criterion is `./build-test/test -tc="*interleave*" -s`. The case is named *"…sample-by-sample **interleaving** (D-17)"*, and doctest's matcher reports `test cases: 0 | 0 passed | 0 failed | 100 skipped` and `Status: SUCCESS!`. **The criterion as written is a green zero-case run** — the precise failure its own neighbouring clause ("confirmed to have matched a non-zero case count before its result is read") exists to prevent, arriving in the criterion itself.
- **Fix:** Used `-tc='*interleaving*'`, confirmed the matched-case count is 1 before reading the result, and recorded both the failing selector and the working one. This is the **seventh** instance in this project of a gate mechanism narrower than the prose beside it, after the five 33-02 catalogued and the one 33-03 added.
- **Files modified:** none — a criterion-interpretation decision
- **Verification:** `-ltc` lists the exact case name; the corrected selector reports 1 case / 33 assertions.
- **Committed in:** n/a (recorded here and in `660fb70`'s message)

**5. [Rule 2 — Missing critical correctness] `DeliberatelyBrokenSharedStateCore` would have silently stopped being a mirror**

- **Found during:** Task 3
- **Issue:** The plan's Task 3 asks only to extend invariant 4's drives. It says nothing about the deliberately-broken control, whose banner carries an explicit, thrice-honoured rule: *"THE GUARD SEQUENCE IS MIRRORED DELIBERATELY AND MUST BE KEPT IN STEP WITH THE REAL CORE"*, with the plan-31-07 paragraph adding that an **inert** drift is the dangerous kind. Phase 33 gave the real core a sync block, a moved conditioning pair and a relocated `p` snapshot. Omitting all three would have been inert **today** and invisible **exactly when a later plan gave invariant 5 sync voltages**.
- **Fix:** Mirrored the sync block, the moved conditioning and the post-sync snapshot, with `syncTrig` and `prevSyncVolts` **per-instance** so the control still carries exactly one defect. The single line not mirrored is named with its reason.
- **Files modified:** `tests/test_vco_core.cpp`
- **Verification:** Positive control **512 / 512 / 1024 at all three rates, unmoved**; whole-suite totals unmoved by the mirror change alone (2,623,341 before and after).
- **Committed in:** `660fb70`

---

**Total deviations:** 5 auto-fixed (3 × Rule 1/2 correctness, 1 × Rule 2 documentation, 1 × Rule 3 blocking)
**Impact on plan:** All five served the plan's own stated goals. Two falsified premises the plan asked assertions to be written against, and in both cases the conclusion was kept and the mechanism corrected with a measurement. One honoured an in-source invariant the task boundaries would have broken. One kept a shipped header from stating a falsehood about its own test coverage. One replaced a zero-matching selector. **No scope creep** — the whole-plan diff is `tests/test_vco_core.cpp` plus a comment-only paragraph in `src/dsp/VcoCore.hpp`.

## Known Stubs

**None.** Every helper this plan adds is consumed by an assertion in the same commit, and every assertion is behind a measured mutant. `makeMasterSawBandLimited` was deliberately deferred from Task 1's commit to Task 2's rather than landing as an unused function, so no commit in this plan's history carries a `-Wunused-function` warning.

Two things are *absent by design* and belong to named later plans:

| Absence | Owner | Why it is not a stub here |
|---|---|---|
| No spectral or time-domain assertion of the sync BLEP | **plans 33-07 / 33-08** | The seam does not exist yet (33-06). This suite is the wrong instrument for spectral claims by its own banner |
| The tighter musical tier on sync drives | **plan 33-08** (T-33-07) | Withheld with the measured headroom recorded — see Decisions #3 |

## Deferred Register Items

Recorded here so plan 33-11 files them with a Resolve-at.

**1. NEW — the fraction guard's two halves are REDUNDANT against a not-a-number, and one of them has no reachable duty on this call site.**
Measured this session (Deviations #2 and #3). `!(f >= 0.f)` and `!(f < 1.f)` are each individually true for a NaN, so either alone catches it. Their *non-redundant* duties are the finite out-of-range directions, and only one of those is reachable: `f >= 1` (the exactly-on-threshold case) is reachable and asserted; `f < 0` is **unreachable at a firing sample** because the trigger only fires from `LOW` with `now >= 1.0`, so `prev < 1.0 <= now`.
**Why this is filed rather than acted on:** the correct action is almost certainly *none* — the guard costs one comparison on sync samples only, it is the file's standing idiom, and a future master-conditioning stage or a different caller could make `f < 0` reachable. What must not happen is a later reader aiming a mutation probe at the lower half, seeing green, and concluding the case is passing. That reader is warned in the case banner; this entry is so the register carries it too.
**Proposed Resolve-at:** no code change. Re-check the reachability argument in any phase that adds a second caller of the sync block or conditions `syncVolts` before it reaches the core.

**2. NEW — a mutation probe whose build failed reported a full green, and it was nearly believed.**
The first `prevSyncVolts` shared-static probe deleted the member. Two test-file references to it stopped compiling, `make test >/dev/null 2>&1` swallowed the error, and `./build-test/test` ran the **stale binary** and reported 33 of 33 assertions passing. For about a minute that was read as "the probe does not discriminate". This is the local-gate-exits-0-on-a-commit-that-cannot-link failure, in miniature, inside the very technique this repository uses to validate its own guards.
**Proposed Resolve-at:** plan 33-11, as one line in whatever mutation-probe convention it writes down: **a probe's build must be verified before its result is read.** Cheap — drop the `>/dev/null 2>&1` on the mutant build, or assert a non-empty rebuild.

**3. CARRIED — 33-02's deferred item 1 (the residual `pending` phantom) is untouched by this plan** and still awaits plan 33-05's diagnostic column. Nothing measured here bears on it: this plan adds no seam call and the core is still exactly measurement leg `none`.

## Issues Encountered

- **`estimateFreqRising` is unusable for any sync claim, exactly as the plan warned**, and nothing in the three new cases calls it. Worth restating: it counts rising zero crossings and is blind under about two samples per cycle, and invariant 8 deliberately drives masters **four times** the sample rate. The structural-ceiling assertion is the instrument used instead.
- **T-33-08 (toolchain divergence) is not discharged locally**, unchanged from 33-01/33-02/33-03. This plan adds no shipped code at all — its only `src/` change is a comment — so its exposure is the smallest of the four so far, but `make strict` is still `-fsyntax-only` and never links. The CI MinGW leg on the exact commit remains plan 33-11's.
- **No operator session ran**, and none is in this plan's scope. Judging hard sync by ear before plan 33-06 lands the seam would be judging an un-band-limited reset; plan 33-12 owns the UAT.
- **Two untracked `.planning/research/.cache/*.json` files** pre-existed at session start, as they did for 33-02 and 33-03, and were left alone.

## Next Phase Readiness

**SYNC-01 is complete. The sync path is now defended by 951 permanent assertions and eleven measured mutants, and the core is still exactly measurement leg `none`.**

- **Plan 33-05** gains two things it should use rather than rebuild. `MasterBlock::wrapG` is the **true wrap fraction oracle** its placement grid needs. And two measurements bear directly on its grid design: a **hard-edged master's fraction is inert** (moves 0.004 while `g` halves), so a grid built on hard edges alone measures nothing about placement — that is Pitfall 10 with a number on it; and at `g = 0.96875` a band-limited master's residual makes the detector fire **one sample late**, which is a placement error already present before any seam exists and should be a column in the grid.
- **Plan 33-06** should know that invariant 7's envelope figures (4.9207 / 4.9210 / 4.9217 V) and invariant 8's grid worst (4.999978 V) are the **pre-seam** baseline. Its seam will move them, and the outer tier is what will notice.
- **Plan 33-08** owns the tighter musical tier for sync (T-33-07) and inherits the measured headroom above rather than a blank page.
- **Plan 33-11** inherits register items 1 and 2 above, plus 33-02's and 33-03's four.
- **Plan 33-12** owns the only SYNC-01 evidence that is still `deferred`: the in-Rack audition (coverage row D12).

**Concerns carried forward:**

- **`make guards` is still blind to the compile-canary failure class** 33-03 spent its measurement on. Unchanged by this plan — the fix lives in `tests/check_canary.sh` and is plan 33-11's.
- **Guard C's IEEE dependence** from 33-01, extended by 33-02's negated pair, is unchanged and is now *doubly* relied on: Deviation #2 establishes that both halves of the fraction guard depend on NaN comparison semantics. `-ffast-math` would defeat the whole guard, not half of it.
- **SYNC-02 has six further contributing plans.** The seam is still absent from the source; non-comment `addStep` count in `src/dsp/VcoCore.hpp` is still **0**.

## Self-Check: PASSED

Verified against disk and git rather than asserted:

- **Files exist:** `tests/test_vco_core.cpp`, `src/dsp/VcoCore.hpp`, `.planning/phases/33-hard-sync/33-04-SUMMARY.md` — all FOUND.
- **Commits exist:** `242a0f1`, `7f2eeb3`, `660fb70`, `863648a` — all FOUND in `git log`.
- **The three new cases are present in `HEAD`** and are matched by their selectors with non-zero case counts: 1 / 99, 1 / 225, 1 / 639.
- **The suite really did grow:** 97 → 100 cases, 2,622,378 → 2,623,356 assertions, 0 failures.
- **`tests/VcoBlockDriver.hpp` is byte-unchanged:** `git diff --stat` against the pre-plan tip produces no output.
- **The `src/dsp/VcoCore.hpp` change is comment-only, measured not claimed:** 0 added non-comment lines, 0 removed non-comment lines.
- **Nothing shipped moved:** six LFO goldens byte-identical (9 cases / 49,188 assertions), `check_frozen.sh` PASS at 15 pinned entries, `src/AnalogLFO.cpp` absent from `git diff --name-only 60e6728 HEAD`.
- **`.planning/REQUIREMENTS.md`:** SYNC-01 now `[x]` / `Complete`; **SYNC-02 still `[ ]` / `Pending`**, checked explicitly rather than assumed.

---
*Phase: 33-hard-sync*
*Completed: 2026-08-29*
