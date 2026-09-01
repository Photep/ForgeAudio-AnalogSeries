---
phase: 33-hard-sync
plan: 06
subsystem: dsp
tags: [hard-sync, sync-blep, d-06, d-07, d-08, past-edge, operator-decision, evidence-not-rule-sanctioned, additive-header-extension, gate-re-anchor, sync-02-declined]

# Dependency graph
requires:
  - phase: 33-hard-sync
    plan: 05
    provides: "the placement measurement, its REFUSAL under the three-condition rule, and the labelled evidence-based recommendation the operator decision rests on"
  - phase: 33-hard-sync
    plan: 01
    provides: "the three MorphBlep guards — this is the SECOND call site and the `jump` finiteness clause (GUARD C, D-04's third item) was written for exactly this caller"
  - phase: 33-hard-sync
    plan: 02
    provides: "the sync block, the jump completion line, and the boundary paragraph naming this plan as the one that lands the seam"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "forge::MorphBlep, its D-14 seam, its D-13 accumulator and the rejected output delay buffer whose rejection this plan's forfeit rests on"
provides:
  - "forge::MorphBlep::addPastStep(float, float) — the named additive past-edge entry point, bit-exactly identical to addStep(0.f, -f*f*jump)"
  - "THE SYNC BLEP ITSELF: the seam call in forge::VcoCore::step, so the shipped core band-limits its hard-sync reset for the first time"
  - "forge::VcoCore::Telemetry::syncCorrection populated, read back OFF the accumulator, with its reconstruction relationship RE-STATED against the landed leg and MEASURED rather than asserted"
  - "the pinned numerical identity in tests/test_morph_blep.cpp — the header extension's own non-circular check"
  - "the RE-ANCHORED bit-exactness gate: 33-05's probe now checks itself against kLegPastEdge with the equality still an exact float =="
affects: [33-07, 33-08, 33-10, 33-11, 33-12]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "A named entry point chosen over a numerically identical inline trick PURELY so that a later 'simplification' back to a measurably worse candidate becomes a visible edit rather than an invisible one"
    - "An additive header extension whose safety argument is a bit-exact IDENTITY with an already-pinned neighbour, rather than a fresh table of values computed from its own formula"
    - "Gating the value that ACTUALLY REACHES the accumulator rather than the raw argument, so two call forms agree even where the intermediate scaling overflows"
    - "An inherited documentation claim ('the subtraction is exact') MEASURED on landing and replaced with its real error bar instead of being repeated"

key-files:
  created: []
  modified:
    - src/dsp/MorphBlep.hpp
    - src/dsp/VcoCore.hpp
    - tests/test_morph_blep.cpp
    - tests/test_vco_spectrum.cpp

key-decisions:
  - "THE D-06 RULE REFUSED AND AN OPERATOR DECISION CLOSED THE GAP. The past-edge leg is landed on 33-05's evidence, labelled in BOTH shipped headers as EVIDENCE-BASED AND NOT RULE-SANCTIONED, with the refusal's figures carried into the source"
  - "The NAMED entry point was landed, not the numerically identical pre-scaled trick, on legibility grounds — an operator decision, and 33-05's proof that no header change is REQUIRED is preserved rather than contradicted"
  - "The gate on addPastStep tests the PRE-SCALED value, so its rejection behaviour is identical to the pre-scaled call's for every input pair including scaling overflow"
  - "[Rule 3] 33-05's bit-exactness gate was RE-ANCHORED here rather than in 33-07: landing the seam turns it red in the same commit, and a red gate cannot be deferred. The equality was not loosened by one character"
  - "The withheld-leg reconstruction shortcut is re-stated against past-edge AND measured: 93 resets, 93 differing samples, 49,136 of 49,152 bit-exact, 16 at exactly one ulp"
  - "SYNC-02 is DECLINED — the NINTH consecutive decline, but for a different and much smaller reason than the previous eight"

patterns-established:
  - "When a landed change turns a prior plan's gate red in the same commit, re-anchor it THERE and record that the later plan's obligation has been consumed — a red gate is not a deferrable item"
  - "A measured claim inherited from a plan written before the measurement should be re-measured on landing, not re-typed"
  - "An identity assertion is a stronger non-circular check for an additive extension than any table of expected values, because the neighbour it is checked against is already pinned"

requirements-completed: []  # SYNC-02 is in this plan's frontmatter and is DECLINED — see Decisions #7

coverage:
  - id: D1
    description: "The named past-edge entry point exists, deposits only the after-edge half, and owes nothing forward"
    requirement: "SYNC-02"  # contributes to; not completed by this plan
    verification:
      - kind: unit
        ref: "\"morph blep: (D-06) the past-edge entry point...\" subcase `nothing owed forward` — pending == 0.0f by EXACT float equality over 54 (position, magnitude) pairs, with a non-vacuity CHECK that inject actually moved"
        status: pass
    human_judgment: false
  - id: D2
    description: "The extension is provably the same arithmetic the already-pinned seam performs"
    verification:
      - kind: unit
        ref: "subcase `identity` — 54 pairs, two fresh instances each, asserted both in the neighbouring pinned subcase's 1e-5f tolerance shape AND by exact float equality"
        status: pass
      - kind: other
        ref: "PROVED ABLE TO FAIL: coefficient perturbed 0.5f -> 0.5001f reds 2 cases at 514 assertions (102 of 354 here, 412 of 1261 in the spectrum gate). Restored, re-verified green"
        status: pass
    human_judgment: false
  - id: D3
    description: "The new gate rejects an out-of-range position and a non-finite magnitude without touching state, identically to the pre-scaled call"
    requirement: "SYNC-02"
    verification:
      - kind: unit
        ref: "subcase `gate` — the same three-value population and exact-equality shape as the existing rejection subcases, plus nine edge pairs (+/-FLT_MAX and 1e-30 at three positions) asserting the rejection matches addStep's where the scaling itself overflows"
        status: pass
    human_judgment: false
  - id: D4
    description: "The existing seam is byte-unchanged and its pinned residual split is unmoved"
    verification:
      - kind: unit
        ref: "git diff -U0 src/dsp/MorphBlep.hpp is ONE insertion hunk @@ -334,0 +335,126 @@ with zero deletions; tests/test_morph_blep.cpp likewise @@ -1949,0 +1950,194 @@; the seam selector reports 4 cases / 420 assertions 0 failures and reproduces (+0.250000, -0.250000)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The seam is called exactly once per sync sample, ahead of the single band-limiter call, which still sees the post-reset phase"
    requirement: "SYNC-02"
    verification:
      - kind: unit
        ref: "comment-stripped counts in src/dsp/VcoCore.hpp: blep.step == 1, addPastStep == 1, addStep == 0. Ordering: reset 866 < snapshot 933 < naive 939 < seam 1101 < blep.step 1208"
        status: pass
    human_judgment: false
  - id: D6
    description: "The output changes on and around sync samples ONLY"
    verification:
      - kind: other
        ref: "Out-of-tree A/B against the pre-task binary, 98,304 samples, three rates, both master edge shapes: UNPATCHED 0 differing samples of 49,152 by direct float equality; PATCHED 93 differing of 49,152, largest |diff| 0.924669 V"
        status: pass
    human_judgment: false
  - id: D7
    description: "The withheld-leg reconstruction relationship re-stated against the landed leg and given a measured error bar"
    verification:
      - kind: other
        ref: "93 resets fired and EXACTLY 93 output samples differed — one per reset. leg_none[n] == leg_full[n] - 5.f*syncCorrection[n] reproduces bit-exactly on 49,136 of 49,152 samples; the 16 exceptions differ by EXACTLY ONE ULP, worst absolute 4.77e-07 V. Every non-reset sample exact"
        status: pass
    human_judgment: false
  - id: D8
    description: "The bit-exactness gate re-anchored without loosening the equality"
    verification:
      - kind: unit
        ref: "kLegNone -> kLegPastEdge at both call sites; comparison still a direct float ==; 1,720,320 samples, 0 mismatches, 420 of 420 cells firing"
        status: pass
    human_judgment: false
  - id: D9
    description: "Nothing shipped that was not meant to move"
    verification:
      - kind: unit
        ref: "make test 104 cases / 2,625,138 assertions 0 failures; six LFO goldens byte-identical (9 cases / 49,188); check_frozen.sh PASS at 15 entries; make strict and make guards exit 0; src/AnalogLFO.cpp absent from the whole-plan diff"
        status: pass
    human_judgment: false

# Metrics
duration: 95min
completed: 2026-09-01
status: complete
---

# Phase 33 Plan 06: The Sync BLEP Lands Summary

**Plan 33-05's decision, restated in one sentence before anything else: the D-06 three-condition rule REFUSED — all three conditions fail and NO WINNER WAS DECLARED — and this plan lands the PAST-EDGE leg on 33-05's labelled, evidence-based recommendation under an OPERATOR DECISION taken on 2026-08-30, not under the rule.** Every claim below is consistent with that sentence: the placement is described as evidence-based and not rule-sanctioned in both shipped headers, 33-05's recommendation paragraph is quoted rather than promoted, and the refusal's own figures travel into the source alongside the choice.

**From this commit the shipped `forge::VcoCore` band-limits its hard-sync reset.** It was measurement leg `none` for four plans; it is now leg `pastEdge`.

## Performance

- **Duration:** 95 min
- **Tasks:** 3 of 3
- **Files modified:** 4 (three planned, one Rule 3)

## Accomplishments

- **Landed the placement the measurement supports, and wrote the refusal into the source beside it.** Both shipped headers now carry, in capitals, that the rule refused and that an operator decision closed the gap — with condition 1's 0.6296 win fraction, condition 2's 22-of-38, condition 3's 0.0527 dB spread, and the four legs' worst deficits (pastEdge 0.8553, none 3.9259, detect 5.0518, flatHalf 10.4567). A reader can audit the choice from the source without opening the planning tree.
- **Landed the NAMED entry point rather than the free trick, and made that choice checkable.** `addPastStep(f, h)` is BIT-EXACTLY `addStep(0.f, -f*f*h)` — not to a tolerance, operation for operation — and that identity is now pinned twice: over 54 constructed argument pairs in the unit suite, and over **1,720,320 real samples** in 33-05's bit-exactness gate, where the probe deliberately keeps the pre-scaled form so the two sides reach the same arithmetic by different routes.
- **Re-anchored a gate that went red in the same commit that landed the seam.** 33-05 wrote a paragraph telling plan 33-07 to do this. It could not wait for 33-07: the seam turns the gate red at **412 failed assertions** immediately. Re-anchored to `kLegPastEdge` with the equality still a direct float `==`, and the original instruction is quoted in place rather than deleted.
- **Measured an inherited claim instead of repeating it.** 33-02's header said the withheld-leg subtraction is "EXACT". Measured on landing: **49,136 of 49,152 samples reconstruct bit-exactly; 16 differ by exactly one ulp** (worst 4.77e-07 V), and every non-reset sample is exact so nothing accumulates. The source now hands plan 33-10 an error bar rather than an adjective.
- **Confirmed nothing owed forward at system level, not just in a unit test.** 93 resets fired in the A/B block and **exactly 93 output samples differed** — one per reset. That is the D-13 accumulator property observed end to end.
- **Wrote D-08 as a decision with both of its reasons**, and recorded that 33-05's own oracle measurement argues against escalating to a slope kernel at all: the binding error term is the detection threshold under a band-limited master, not the interpolation.

## Task Commits

| # | Task | Commit | Type |
|---|---|---|---|
| 1 | The additive past-edge entry point, with the forfeited half stated (D-06) | `ab198da` | feat |
| 2 | The seam call in the core, and D-08 recorded as a decision (SYNC-02 / D-06 / D-08) | `755874e` | feat |
| 3 | Pin the header extension's own non-circular identity (D-06) | `4fc4a86` | test |

## Files Created/Modified

- `src/dsp/MorphBlep.hpp` — one insertion hunk. `addPastStep` plus a 119-line banner.
- `src/dsp/VcoCore.hpp` — the seam call, the correction telemetry, the rewritten boundary paragraph and the seam's decision record.
- `tests/test_morph_blep.cpp` — one insertion hunk. The `(D-06)` identity case.
- `tests/test_vco_spectrum.cpp` — **[Rule 3, unplanned]** the bit-exactness gate re-anchor and three stale-comment corrections.

## Suite Totals, Before and After

| | Test cases | Assertions |
|---|---|---|
| Before plan 33-06 (33-05's recorded totals) | 103 | 2,623,356 → **2,624,784** |
| After plan 33-06 | **104** | **2,625,138** |
| Delta | **+1** | **+354** |

`./build-test/test -tc="morph blep: (D-06)*" -s` reports **1 test case, 1 passed, 0 failed, 354 assertions.**

---

# TASK 1 — THE HEADER EXTENSION

## The landed function

```cpp
void addPastStep(float xBehind, float jump) {
    const float pastJump = -xBehind * xBehind * jump;
    if (!(xBehind >= 0.f) || xBehind > 1.f || !(pastJump - pastJump == 0.f)) return;
    inject += pastJump * 0.5f;
    // `pending` is deliberately NOT touched — see item 2 above.
}
```

**The gate is plan 33-01's idiom, unaltered in shape**: negated comparison first so a not-a-number position is rejected rather than accumulated, then `jump - jump == 0.f` — GUARD C, D-04's third item, written for exactly this caller. **No fourth idiom was invented and no include was added.**

**One deliberate refinement over a literal copy, and it is what makes the identity total:** the finiteness clause tests `pastJump`, the value that ACTUALLY REACHES the accumulator, not the raw `jump`. A gate on the raw argument would accept `FLT_MAX` at position 0.5 while `addStep(0.f, -0.5*0.5*FLT_MAX)` — the form it must be identical to — sees an overflowed infinity and rejects. Gating the scaled value makes the two forms agree on **every input pair**, including the ones where the scaling itself overflows. Nine such pairs are asserted in Task 3's gate subcase.

## The banner, item by item, with line ranges

Total **119 comment lines** for a 5-line function (the plan required at least fifteen). `src/dsp/MorphBlep.hpp`:

| Lines | Content |
|---|---|
| 335–338 | Opening rule; the extension is ADDITIVE and the seam above is untouched |
| **339–357** | **How the placement was chosen** — the rule REFUSED, the operator decision of 2026-08-30, the four legs' worst deficits, and the prohibition on promoting the recommendation |
| **358–363** | **Item 1** — the existing seam's edge is AHEAD, so both halves are deliverable |
| **364–374** | **Item 2** — this edge is BEHIND; the residual is symmetric on [-1,1]; the pre-edge half belonged on an already-emitted sample; nothing owed forward |
| **375–383** | **Item 3** — that half is FORFEITED deliberately and permanently; the one-sample delay buffer and D-13's two grounds for rejecting it |
| **384–402** | **Item 4** — the MEASURED cost, quoted per ratio region |
| **403–413** | **Item 5** — why a minimum-phase kernel would not pay this cost, and that AA-05 forbids minBLEP BY NAME |
| **414–421** | **Item 6** — the `[0,1]` contract EXTENDED additively, never reinterpreted |
| **422–443** | **Item 7** — the numerical identity written out in prose, and why the named form was chosen over the trick |
| 444–452 | The gate paragraph — same negated shape, same reason, no new idiom, no include |
| 453–459 | Closing rule and the function |

## Item 4 as landed — the cost, per ratio region

The plan asked for "the MEASURED cost of the forfeit, from plan 33-05's own table, quoted per ratio region rather than as one number". **The banner states honestly that the forfeit's own cost is NOT separately measurable on 33-05's grid** — no leg there recovers the forfeited half, because recovering it needs the delay buffer item 3 rejects. What is measurable is the benefit the surviving half still buys over applying no correction at all, and that is what is quoted (`none` − `pastEdge`, mean dB, band-limited master, all three rates; positive = past-edge better):

| ratio | 44.1 kHz | 48 kHz | 96 kHz | range in the banner |
|---|---|---|---|---|
| 0.50 | +3.26 | +3.06 | +3.04 | **+2.80 to +3.26** |
| 0.75 | +2.98 | +2.80 | +2.80 | (same row) |
| 1.00 | 0.00 | −0.03 | +0.02 | **−0.03 to +0.02** |
| 1.50 | +1.09 | +1.58 | +1.50 | **+1.09 to +1.58** |
| 2.50 | +0.56 | +0.65 | +0.86 | **+0.56 to +0.86** |
| 3.50 | +0.35 | −0.07 | +0.48 | **−0.07 to +0.48** |
| 5.50 | −0.87 | −1.09 | −0.15 | **−1.09 to −0.15 — WORSE than none** |

**The top of the sweep is where the forfeit shows.** Above about 4× the surviving half no longer pays for itself. 33-RESEARCH's prototype figures (0.2–0.6 dB at slave ≥ 2×, 4.6–6.0 dB at slave < master) are recorded in the banner as SUPERSEDED by this table.

## Acceptance criteria, checked

| Criterion | Result |
|---|---|
| `make test` / `make strict` / `make guards` | **all exit 0**, 0 failures |
| Existing seam byte-unchanged | `git diff -U0 src/dsp/MorphBlep.hpp` → **`@@ -334,0 +335,126 @@`**, a single pure-insertion hunk, **zero deleted lines** |
| No new include | `git diff -U0 ... \| grep -c '^+#include'` → **`0`** |
| Banner ≥ 15 comment lines carrying all seven items | **119 lines**, enumerated above |
| Pinned seam equivalence unmoved | `-tc="*seam*" -s` → **4 matched cases, 420 assertions, 0 failures**; the recorded split reproduces at **(+0.250000, −0.250000)** and the five-position table at (1, 0) / (0.5625, −0.0625) / (0.25, −0.25) / (0.0625, −0.5625) |
| `check_frozen.sh` | **PASS, 15 pinned entries** |
| Four frozen headers in the diff | **no output** |
| `src/AnalogLFO.cpp` in the diff | **absent** |

---

# TASK 2 — THE SEAM IN THE SHIPPED CORE

## What landed

```cpp
tel.syncJump = syncFired ? (naive - syncBefore) : 0.f;
// ... 130 lines of decision record ...
if (syncFired) {
    const float injectBefore = blep.inject;
    blep.addPastStep(syncFrac, tel.syncJump);
    tel.syncCorrection = blep.inject - injectBefore;
}
```

## The sign, quoted beside the convention

| | Expression |
|---|---|
| `src/dsp/MorphBlep.hpp`'s documented convention | `h = value_after - value_before` |
| **The landed expression** | **`tel.syncJump = syncFired ? (naive - syncBefore) : 0.f;`** |
| `.planning/research/STACK.md:124` | `out_preReset - morphedWave(newPhase)` — **the negation** |

`naive` is the post-reset value and `syncBefore` the pre-reset one, so **the landed sign is the seam's**. The mutation probe that priced the alternative is **33-05's `kProbeBadSign`**, the research expression transcribed verbatim: **1.76 to 2.20 dB worse** on band-limited masters at all three rates, collapsing to noise on hard-edge ones. That figure is now in the source.

## Ordering — all three line numbers, and a criterion that is narrower than its own prose

| Landmark | Line |
|---|---|
| The reset / wrap: `phase = (double)(1.f - f) * deltaPhase;` | **866** |
| `tel.syncCorrection = 0.f;` (the unconditional default) | 931 |
| The phase snapshot: `const float p = (float)phase;` | **933** |
| `const float naive = wave.morphedWave(p, ...)` | 939 |
| `tel.syncJump = ...` (SYNC JUMP COMPLETION) | 954 |
| **`blep.addPastStep(syncFrac, tel.syncJump);`** | **1101** |
| The single band-limiter call: `blep.step(...)` | **1208** |

> **REPORTED, NOT SILENTLY SATISFIED — the ninth instance in this project of a gate mechanism narrower than the prose beside it.** Task 2's acceptance criterion asks that the seam's line number sit *"between the wrap line and the phase-snapshot line"*. **It does not, and it must not.** The seam is fed `tel.syncJump`, and the jump **does not exist** before the snapshot — its post-reset term IS `naive`, which is computed at line 939 from the snapshot at 933. A seam call before 933 would have to recompute `naive` and would cost the second frozen `morphedWave` call D-05 authorises exactly one of. `src/dsp/VcoCore.hpp` says so in capitals in the paragraph plan 33-02 left there (*"PLAN 33-06'S SEAM CALL BELONGS AT THE COMPLETION LINE, NOT HERE"*), 33-05's recommendation says so, and this plan's own prior-wave brief says so.
>
> **The invariant the criterion was reaching for is fully satisfied:** reset (866) **<** snapshot (933) **<** seam (1101) **<** the single band-limiter call (1208). The band-limiter still sees the post-reset phase, and the residual the seam deposits is drained on this sample because `step()`'s preamble consumes `inject` unconditionally. Nothing was moved to satisfy the literal wording.

## Every function called from the sync path

Enumerated from the comment-stripped source, as the criterion requires:

| Call | Where | Note |
|---|---|---|
| `syncTrig.process(in.syncVolts, 0.1f, 1.0f)` | inside the sync block | FROZEN primitive |
| `wave.morphedWave((float)phase, ...)` | inside the sync block | the pre-reset term — the ONE extra frozen call D-05 authorises |
| `wave.morphedWave(p, ...)` | outside the condition | `naive`, unconditional and unchanged |
| **`blep.addPastStep(syncFrac, tel.syncJump)`** | inside `if (syncFired)` | the seam, once |
| `blep.step(wave, phase, p, deltaPhase, morph, character)` | outside the condition | the single band-limiter call |

**No slope-correction kernel and no polyBLAMP entry point is called from this block.** Comment-stripped counts in `src/dsp/VcoCore.hpp`: `blep.step` = **1**, `addPastStep` = **1**, `addStep` = **0**.

## The output moves only where it should

Out-of-tree A/B against a binary built from the pre-task headers. 98,304 samples: 2 legs × 3 rates × 2 master edge shapes × 8192, seeded from `VcoBlockDriver`'s literals.

| Leg | Differing samples | Largest difference |
|---|---|---|
| **Jack UNPATCHED** | **0 of 49,152** by direct float equality | **0** |
| Sync active | **93 of 49,152** | **0.924669 V** |

**93 resets fired and exactly 93 samples differed.** One sample per reset, which is the nothing-owed-forward property visible at system level rather than argued from the kernel. The largest correction recorded in telemetry was **0.184934** in the pre-multiply domain; **×5 = 0.924669 V**, exactly the largest output difference.

## The reconstruction relationship, RE-STATED against the landed leg and then MEASURED

**Re-stated, not inherited.** Under `pastEdge` the seam deposits into `inject` only and leaves `pending` untouched, so the correction is confined to the sample that carries it and

> `leg_none[n] = leg_full[n] − 5.f * syncCorrection[n]`

holds per sample. Under `detect` or `flatHalf` it would not: both deposit into `pending` as well, and plan 33-10's renderer would need both halves. **The header's conditional sentence is now resolved to its true branch.**

**And then measured, because "exactly" is a claim.** Over the 49,152-sample patched block:

| | Value |
|---|---|
| Samples checked | **49,152** |
| Reconstructed **bit-exactly** | **49,136** |
| Off by **exactly one ulp** | **16** |
| Worst absolute departure | **4.77e-07 V** |
| Non-reset samples exact | **all 49,059** |

The residue is `MorphBlep`'s own `inject + pending` float addition rounding when one addend changes. **It cannot accumulate, precisely because nothing is owed forward.** Plan 33-10 now inherits an error bar instead of an adjective. The source carries these numbers.

## Acceptance criteria, checked

| Criterion | Result |
|---|---|
| `make test` / `make strict` / `make guards`, incl. `check_canary.sh` perturbing `VcoCore.hpp` | **all exit 0** |
| `blep.step` comment-stripped count | **1** |
| Selected entry point comment-stripped count, and its position | **1**, at 1101, after the snapshot and ahead of the band-limiter — see the reported criterion mismatch above |
| No slope kernel added | **confirmed**, all five calls enumerated above |
| Sign is the seam's | **confirmed**, quoted beside the convention above |
| Unpatched output bit-identical | **0 differing of 49,152** |
| Sync-active output | **93 differing, max 0.924669 V** |
| `check_frozen.sh` 15 entries; `src/AnalogLFO.cpp` absent | **PASS / absent** |
| Six shipped-LFO goldens | **byte-identical**, 9 cases / 49,188 assertions |

---

# TASK 3 — THE IDENTITY, PINNED

`TEST_CASE("morph blep: (D-06) the past-edge entry point is exactly the pre-scaled seam call")` — **1 case, 354 assertions, 0 failures.**

| Subcase | What it pins |
|---|---|
| **identity** | 54 (position, magnitude) pairs — 9 positions across the OPEN unit interval including both 1e-6 near-endpoints, × 6 signed magnitudes including ±1e30. Two FRESH instances per pair. Asserted **both** in case five part A's `< 1e-5f` tolerance shape (so the file's two equivalence claims read alike) **and** by **exact float equality** (the stronger claim, and the one the spectral gate's direct `==` depends on) |
| **nothing owed forward** | `CHECK(b.pending == 0.0f)` — **exact float equality, never a tolerance**, plus `CHECK(b.inject != 0.0f)` for non-vacuity |
| **gate** | The same three-value bad-position population and exact-equality shape as the existing rejection subcases, the same three-value non-finite magnitude population, **plus nine edge pairs** (±FLT_MAX and 1e-30 at three positions) asserting the rejection is IDENTICAL to the pre-scaled call's where the scaling itself overflows |

**The forward-owed assertion, quoted as required:**

```cpp
CHECK(b.pending == 0.0f);
```

## Proved able to fail

Perturbation: `inject += pastJump * 0.5f;` → `inject += pastJump * 0.5001f;`. A **coefficient typo**, deliberately tiny — a sign flip would prove much less.

| | Result |
|---|---|
| Cases red | **2** |
| Total failed assertions | **514** |
| `morph blep: (D-06)` | **102 failed of 354** |
| `vco spectrum: (D-06)` bit-exactness gate | **412 failed of 1261** |

Restored with `git checkout --` and re-verified: **104 cases, 2,625,138 assertions, 0 failures.** The second number is the useful half of the signature — the identity is not only asserted over constructed arguments, it is exercised against 1,720,320 real samples.

## The existing pinned subcases are unmoved

`git diff -U0 tests/test_morph_blep.cpp` → **`@@ -1949,0 +1950,194 @@`**, a single pure-insertion hunk at the end of the file, **zero deletions**. Case five parts A and B are untouched, and their recorded values reproduce:

| x | inject | pending |
|---|---|---|
| 0.00 | 1 | 0 |
| 0.25 | 0.5625 | −0.0625 |
| 0.50 | **0.25** | **−0.25** |
| 0.75 | 0.0625 | −0.5625 |
| 1.00 | 0 | −1 |

and the site-versus-seam pair reproduces at **(+0.250000, −0.250000)**.

---

# THE RE-ANCHORED GATE (Rule 3 — the plan's biggest unplanned event)

**Landing the seam turned plan 33-05's bit-exactness gate red in the same commit: 412 failed assertions, 1 failed case.** 33-05 wrote a paragraph naming plan **33-07** as the owner of the repair. That was not survivable — a red gate cannot be handed to a later plan, and `make test` exiting 0 is an acceptance criterion of both Task 2 and Task 3.

**What was done, and what was NOT done:**

- `kLegNone` → `kLegPastEdge` at both `measureSyncCellDb` call sites.
- The case name now says `...on the past-edge leg`.
- **The equality was not loosened by one character.** It is still a direct float `==`, never doctest's approximate comparator. 33-05's own warning — *"the tempting repair — loosening the equality — would delete the gate"* — is quoted verbatim in the file rather than deleted.
- Result: **1,720,320 samples, 0 mismatches, 420 of 420 cells firing.**

**And the re-anchored gate is not a tautology.** The probe leg deliberately keeps the **pre-scaled** form, `addStep(0.f, -f*f*jump)`, while the shipped core calls **`addPastStep(f, jump)`**. The two sides reach the same arithmetic by **different routes**, so the gate is the identity of Task 3 exercised against real audio. A comment at the leg's `switch` arm now says so, to stop a later editor "unifying" the probe onto the core's call and quietly hollowing the gate out.

**Consequence for plan 33-07: its re-anchor obligation is DISCHARGED, and it should not repeat it.** Its other two obligations from 33-05 (pin the two decibel columns via a per-cell lookup; decide whether to gate the instrument-invalid half) are untouched.

---

# Gate Results

| Gate | Result |
|------|--------|
| `make test` | **PASS** — 104 cases, 2,625,138 assertions, 0 failures |
| `make strict` (C++11, `-pedantic-errors`) | **PASS**, exit 0 |
| `make guards` | **PASS**, exit 0 (all three scripts) |
| `bash tests/check_frozen.sh` | **PASS** — 15 pinned entries, D-05 manifest + goldens + negative control |
| Six shipped-LFO goldens | **byte-identical** — 9 cases / 49,188 assertions, 0 failures |
| Compiler warnings in the changed TUs | **0** (`-Wall -Wextra`) |
| Four frozen shared headers in the diff | **none** |
| `src/AnalogLFO.cpp` in the whole-plan diff | **absent** |
| `git diff --name-only` across all three commits | `src/dsp/MorphBlep.hpp`, `src/dsp/VcoCore.hpp`, `tests/test_morph_blep.cpp`, `tests/test_vco_spectrum.cpp` |
| `tests/check_includes.sh` diffstat | **empty** — no new translation unit |

---

# Decisions Made

1. **The past-edge leg is landed under an OPERATOR DECISION, and the refusal is preserved rather than papered over.** 33-05's rule refused; its recommendation paragraph carries an explicit prohibition on a later agent promoting it. That prohibition is honoured: the recommendation is not restyled as a decision anywhere. Instead both shipped headers record that the rule REFUSED, quote its three conditions' figures, and name the operator decision of 2026-08-30 as what closed the gap — citing the evidence it rested on (pastEdge worst deficit **0.8553 dB**, the only candidate inside the 1.0 dB reproduction bound; `none` **3.9259**; `detect` **5.0518**, eliminated at 0 wins of 54; `flatHalf` **10.4567**, eliminated on variance).

2. **The NAMED entry point was landed, and 33-05's finding that no header change is REQUIRED stands unchallenged.** The two forms are numerically identical — that is now a pinned fact rather than an assertion — so 33-05's claim is true and is quoted as true. The named form was chosen on **legibility**, which 33-05 itself called orthogonal to its result and left to this plan. The reason is preserved in the header: a pre-scaled `-f*f*jump` at the call site reads as a forward-facing seam called at position zero with a mysterious magnitude, and its obvious "simplification" is `addStep(f, jump)` — the `detect` candidate that measured **worse than doing nothing**. Naming the function makes that edit visible.

3. **The new gate tests the PRE-SCALED value, not the raw argument.** A literal copy of `addStep`'s gate would have tested the raw `jump` and diverged from the pre-scaled call wherever the scaling overflows (e.g. FLT_MAX at position 0.5: `addStep` rejects the overflowed infinity, a raw-argument gate would accept). Gating the value that actually reaches the accumulator makes the identity **total** rather than merely typical, and nine edge pairs in Task 3 assert exactly that. This is the one place the new function is not a character-for-character copy of its neighbour, and it is deliberate.

4. **[Rule 3] The bit-exactness gate was re-anchored HERE, not in 33-07.** See the section above. The alternative — leaving 412 assertions red across a plan boundary — is not a thing this project does, and both remaining tasks have `make test` exiting 0 as an acceptance criterion.

5. **The reconstruction claim was MEASURED on landing rather than re-typed.** 33-02's header said the subtraction is "EXACT" and 33-05's recommendation repeated it. It is exact in the sense that matters — nothing is owed forward, nothing accumulates, every non-reset sample reconstructs bit-exactly — but **not bit-exact on 16 of 49,152 samples**, each off by one ulp, because `MorphBlep` sums `inject` and `pending` in float. Writing "exact" without qualification would have handed plan 33-10 a false invariant to write an equality assertion against.

6. **`tel.syncCorrection` is read back OFF the accumulator, not recomputed.** A recomputed expression would report a correction on the path where the seam's own entry gate REJECTS the event — silently claiming a deposit that never landed, in the one field plan 33-10 uses to reconstruct the withheld leg. The read-back is exact here because `inject` is provably zero at that point in `step()`: it is drained unconditionally in `MorphBlep::step`'s preamble and this seam is its only external writer in the shipped core.

7. **SYNC-02 is DECLINED — the ninth consecutive decline, and the reason has finally changed shape.** The previous eight declined because *the sync-BLEP did not exist*. It exists now. What is still missing is narrower and is owned by two named plans:
   - **No sync alias threshold is pinned.** All 420 cells of `SYNC_GRID` still carry `kSyncUnpinnedDb` and `kProvSyncUnpinned`. **Plan 33-07** owns that.
   - **"Click-free" has no instrument.** 33-05's deferred item 2 states it plainly: the spectral grid is structurally blind to the single-sample click SC-3 forbids, and half its cells cannot rank anything. The time-domain per-sample delta bound (D-10) is **plan 33-08's**. Booking a click-free claim on a spectral instrument that cannot see clicks is precisely the move this phase's register exists to prevent.
   - **And the correction is not uniformly an improvement.** At master/slave ratio 5.5 the landed leg measures **0.15 to 1.09 dB WORSE** than applying no correction at all. A requirement claiming click-freeness cannot be ticked while a measured region of the parameter space is worse than the baseline and no threshold has been pinned over it.

   `.planning/REQUIREMENTS.md` was **checked, not assumed**, after this plan finished: SYNC-01 remains `[x]` / `Complete` (33-04's), **SYNC-02 remains `[ ]` / `Pending`**.

8. **D-08 is written as a decision with both of its reasons.** No slope kernel, and the source says why: the seam carries a value jump only so a slope seam would be a second header change; and the slope difference across the reset has no closed form, so it would need a numerical derivative of the crossfade at two phases — new transcendental cost and a fresh cross-library divergence risk on a path this milestone has kept free of one. It remains the documented **first** escalation, ahead of any kernel-order change, and that escalation is an operator decision with an impact assessment. 33-05's oracle result is recorded alongside as evidence against escalating at all.

---

# Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 — Blocking] Landing the seam turned plan 33-05's bit-exactness gate red in the same commit**

- **Found during:** Task 2, on the first `make test` after the seam landed
- **Issue:** `tests/test_vco_spectrum.cpp`'s `(D-06)` probe gate compares `SyncPlacementProbe` against the shipped `forge::VcoCore` on leg `kLegNone`. The shipped core stopped being leg `none` the instant the seam landed. **412 failed assertions, 1 failed case.** 33-05 anticipated this exactly and assigned the repair to plan **33-07** — but the gate goes red in *this* commit, and `make test` exiting 0 is an acceptance criterion of Tasks 2 and 3.
- **Fix:** Re-anchored to `kLegPastEdge` at both `measureSyncCellDb` call sites, renamed the case, and rewrote the warning paragraph to **quote** 33-05's original instruction rather than delete it, recording that the re-anchor happened one plan early and why. **The equality was not loosened**; it is still a direct float `==`. Added a comment at the `kLegPastEdge` switch arm explaining that the probe deliberately keeps the pre-scaled form so the gate stays a comparison of two routes rather than a tautology.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** 1,720,320 samples, **0 mismatches**, 420 of 420 cells firing; whole suite green.
- **Committed in:** `755874e`

**2. [Rule 1 — Bug] Three comments in `tests/test_vco_spectrum.cpp` became false the moment the seam landed**

- **Found during:** Task 2, immediately after the re-anchor
- **Issue:** `kLegNone`'s enum comment said *"This is the shipped core as of plan 33-02"*; the `switch` arm said *"withheld ENTIRELY — this is the shipped core"*; and the diagnostics block said the per-cell diagnostics are taken *"on the shipped core's own leg"*. All three describe leg `none`, which is no longer the shipped core. Leaving them would have made the file assert one thing and document another.
- **Fix:** Corrected in place. `kLegNone` now reads as the leg that WAS the shipped core until 33-06 and is now the withheld audition leg and the diagnostics reference; `kLegPastEdge` is labelled as the shipped core from 33-06 onward. The diagnostics comment now states the actual reason the correction-free leg is the right reference — every diagnostic there (jump, fires, late fires, phantom, fundamental dominance) is a property of the **reset**, which is identical on all eight legs, so taking them there keeps them properties of the **cell** rather than of whichever candidate ships.
- **Files modified:** `tests/test_vco_spectrum.cpp`
- **Verification:** Comment-only; whole suite green at unchanged totals.
- **Committed in:** `755874e`

**3. [Rule 2 — Missing correctness] The new gate had to test the pre-scaled value, or the identity would be false where the scaling overflows**

- **Found during:** Task 1, while designing the gate
- **Issue:** The plan's behavior line says the gate rejects "a non-finite magnitude", and the obvious reading is a character-for-character copy of `addStep`'s clause on the raw `jump`. That would break the identity the whole extension rests on: `addPastStep(0.5f, FLT_MAX)` would be ACCEPTED while `addStep(0.f, -0.5f*0.5f*FLT_MAX)` sees an overflowed infinity and REJECTS. Two forms advertised as identical would then disagree on a reachable input class.
- **Fix:** The clause tests `pastJump`, the value that actually reaches the accumulator. Same idiom, same negated-first ordering, same single gate line, no state touched on any rejected path. Nine edge pairs in Task 3's gate subcase assert the two forms agree there.
- **Files modified:** `src/dsp/MorphBlep.hpp`, `tests/test_morph_blep.cpp`
- **Verification:** The gate subcase's nine edge pairs pass; whole suite green.
- **Committed in:** `ab198da` / `4fc4a86`

**4. [Rule 1 — Bug] The inherited "the subtraction is EXACT" claim is not bit-exact**

- **Found during:** Task 2, verifying the reconstruction relationship rather than restating it
- **Issue:** `src/dsp/VcoCore.hpp`'s Telemetry paragraph (plan 33-02) and 33-05's recommendation both say the withheld leg is the full leg minus five times `syncCorrection` **exactly**. Measured, it is exact on 49,136 of 49,152 samples and **one ulp off on 16** — `MorphBlep::step`'s preamble sums `inject` and `pending` in float and that addition rounds differently when one addend changes. Plan 33-10 is the consumer and could reasonably have written a bit-exact equality assertion against the un-qualified claim.
- **Fix:** The re-stated paragraph carries the measurement instead of the adjective: 93 resets, 93 differing samples, 49,136 of 49,152 bit-exact, 16 at exactly one ulp, worst absolute 4.77e-07 V, every non-reset sample exact so nothing accumulates.
- **Files modified:** `src/dsp/VcoCore.hpp`
- **Verification:** The out-of-tree reconstruction check above.
- **Committed in:** `755874e`

**5. [Reported, not fixed] Task 2's placement acceptance criterion is narrower than its own prose — the ninth such instance in this project**

- **Found during:** Task 2, checking acceptance
- **Issue:** The criterion requires the seam's line number to be *"between the wrap line and the phase-snapshot line"*. It is at **1101**, after the snapshot at **933**. It cannot be otherwise: the seam is fed the jump, and the jump's post-reset term IS `naive`, computed from the snapshot. A call before 933 would need a second frozen `morphedWave` call, which D-05 forbids and which 33-02's source forbids by name in capitals.
- **Fix:** **Reported rather than satisfied.** Nothing was moved. All three line numbers are recorded above along with the invariant the criterion was reaching for — reset (866) < snapshot (933) < seam (1101) < band-limiter (1208) — which holds exactly. Following 33-05's deviation 3, the numbers are given rather than the verdict.
- **Files modified:** none — a criterion-interpretation decision
- **Verification:** The ordering table above; `blep.step` comment-stripped count is 1.
- **Committed in:** n/a

---

**Total deviations:** 4 auto-fixed (2 × Rule 1, 1 × Rule 2, 1 × Rule 3) + 1 reported
**Impact on plan:** One was forced by the plan's own success — the gate 33-05 built goes red the moment its recommendation is implemented, and repairing it a plan early was the only option that leaves the tree green. Two corrected claims this plan inherited rather than repeating them. One strengthened the new gate so the identity the extension rests on is total rather than typical. **The only unplanned file is `tests/test_vco_spectrum.cpp`, and every edit in it is either the re-anchor or a comment the seam falsified.**

---

# Known Stubs

**None.** Every symbol this plan adds is called and asserted in the same commit or the next.

Two things are *absent by design* and belong to named later plans:

| Absence | Owner | Why it is not a stub here |
|---|---|---|
| No sync alias threshold is pinned; both `SYNC_GRID` decibel columns remain the unpinned sentinel | **plan 33-07** | This plan's output is a placement, not a gate. 33-05 declined to pin a threshold from a leg no gate had examined, and that reasoning is unchanged by landing the leg. |
| No time-domain click instrument | **plan 33-08** | D-10's per-sample delta bound. 33-05's deferred item 2 records that the spectral grid is structurally blind to the click SC-3 forbids, which is exactly why SYNC-02 is declined here. |

---

# Deferred Register Items

**1. CLOSED — 33-05's deferred item 1: "the D-06 three-condition rule has no defined behaviour when it refuses."**
It was resolved by the route 33-05 proposed first: **an operator decision**, taken on 2026-08-30 with the refusal, its figures, the four-leg deficit table and the Apple-clang caveat in front of it. The decision chose to proceed on condition 1's second clause. **The process gap itself is not closed for future rules** — what closed was this instance. If a later measurement refuses, the precedent is now on the record: escalate with the figures, decide explicitly, and label the result as evidence-based rather than rule-sanctioned in the source. Both shipped headers carry that label.
**Resolve-at:** recorded as precedent; plan 33-11 should file it as a resolved process question.

**2. CLOSED-EARLY — 33-05's obligation on plan 33-07 to re-anchor the bit-exactness gate.**
Discharged here, one plan early, because it could not be deferred. **Plan 33-07 must NOT repeat it.** Its other two obligations stand: pin the two decibel columns (which needs `buildSyncGrid` to grow a per-cell lookup rather than a formula), and decide whether to gate the instrument-invalid half at all.

**3. NEW — the landed leg is measurably WORSE than no correction at high master/slave ratios.**
At ratio 5.5 the past-edge correction measures **0.15 to 1.09 dB worse** than applying none, and at 3.5 it is within noise at one rate. This is the forfeited pre-edge half showing up where the fraction is largest. It is **not** a defect to fix here — the alternative candidates are all worse overall, and 33-05's condition 1 second clause is exactly the finding that past-edge is never *materially* worse anywhere. But **plan 33-07 must not pin a threshold at those ratios that assumes the correction helps**, and any future work should note that a conditional correction (gated on ratio) is not available to the core, which cannot know the master's frequency.
**Proposed Resolve-at:** plan 33-07, when it pins the per-cell columns.

**4. CARRIED — every decibel in this SUMMARY and in both banners is an Apple-clang figure.**
Unchanged from 33-01/02/03/04/05, but the exposure is materially larger now: this is the first plan in the phase to put **shipped DSP arithmetic** on the sync path. `make strict` passes locally at C++11 `-pedantic-errors`; the CI MinGW **link** leg remains plan 33-11's and T-33-08 is not discharged locally.

**5. CARRIED — 33-05's deferred items 2, 3 and 5, and 33-02/03/04's six, are unchanged by this plan.**
Item 2 (the spectral instrument is blind on hard-edge masters and on 210 of 420 cells) is the direct reason SYNC-02 is declined above.

---

# Issues Encountered

- **The gate re-anchor was the plan's real risk, and it arrived exactly where 33-05 said it would.** What 33-05 could not have known is the *timing*: it assigned the repair to 33-07 while the breakage is unavoidably in 33-06's own commit. The temptation in the moment is the one 33-05 named in writing — loosen the equality and move on. It was not taken, and the warning paragraph is preserved in the file so the next reader sees both the instruction and the fact that it was followed.
- **Writing "exact" would have been easier than measuring it.** The claim was inherited from two documents and would have passed review. Sixteen samples out of 49,152 say otherwise, and plan 33-10 is the one that would have discovered it.
- **`check_canary.sh [2b/5]` remains measurement-blind on this host** (33-03's finding: `forge::VcoCore::step` is not inlined into the canary probe at stock `-O3`). `make guards` going green was treated as evidence of nothing about this seam's behavior; every behavioral claim above comes from `make test` or from the out-of-tree A/B.
- **Two untracked `.planning/research/.cache/*.json` files** pre-existed at session start, as they did for 33-02 through 33-05, and were left alone.

---

# Next Phase Readiness

**The phase's central question is answered in the shipped source, and the answer carries its own provenance.**

- **Plan 33-07** should read deferred items **2** and **3** first. **Its re-anchor obligation is already discharged** — the gate is on `kLegPastEdge` with an exact `==`. Its remaining work is to pin the two decibel columns via a per-cell lookup and to decide about the instrument-invalid half, and it must not pin a threshold at ratios 3.5–5.5 that assumes the correction helps there.
- **Plan 33-08** owns the time-domain instrument and therefore owns the half of SYNC-02 this plan declines. It also inherits the snap-versus-past-edge figures (4.99–5.61 dB band-limited) as the one spectral sync claim with a comfortable margin.
- **Plan 33-10** inherits the reconstruction relationship **with an error bar**: `leg_none[n] = leg_full[n] − 5.f * syncCorrection[n]`, bit-exact on 49,136 of 49,152 measured samples and one ulp off on 16. It should NOT write a bit-exact equality assertion against it.
- **Plan 33-11** inherits register items 1 (closed as precedent), 3 (new), 4 and 5 above, plus 33-05's 2/3/5 and 33-02/03/04's six.
- **Plan 33-12** owns the operator UAT. The decision it would have been asked to close has already been taken, on 2026-08-30, and is labelled as such in two shipped headers.

**Concerns carried forward:**

- **SYNC-02's remaining gap is small and precisely named:** no pinned threshold (33-07) and no instrument that can see a click (33-08). The mechanism is complete.
- **The correction is worse than nothing at the top of the ratio sweep.** Recorded, banner'd, and deferred to 33-07's thresholds.
- **This is the first shipped DSP arithmetic on the sync path**, so T-33-08's exposure is real for the first time in this phase and remains undischarged locally.

---

# Self-Check: PASSED

Verified against disk and git rather than asserted:

- **Files exist:** `src/dsp/MorphBlep.hpp`, `src/dsp/VcoCore.hpp`, `tests/test_morph_blep.cpp`, `tests/test_vco_spectrum.cpp`, `.planning/phases/33-hard-sync/33-06-SUMMARY.md` — all **FOUND**.
- **Commits exist:** `ab198da`, `755874e`, `4fc4a86` — all **FOUND** in `git log`.
- **The new case is present in `HEAD`** and its selector matches with a non-zero count: **1 case / 354 assertions / 0 failures**.
- **The suite really did grow:** 103 → **104** cases, 2,624,784 → **2,625,138** assertions, 0 failures.
- **The seam is genuinely present:** comment-stripped `addPastStep` count in `src/dsp/VcoCore.hpp` is **1** (it was 0 through plan 33-05), `blep.step` is still **1**, `addStep` is **0**.
- **The existing seam is byte-unchanged:** both source diffs are single pure-insertion hunks with **zero deleted lines**.
- **Nothing shipped moved that should not have:** six LFO goldens byte-identical (9 cases / 49,188), `check_frozen.sh` PASS at 15 entries, `make strict` and `make guards` exit 0, **zero** compiler warnings, `src/AnalogLFO.cpp` absent from the whole-plan diff.
- **`.planning/REQUIREMENTS.md`:** SYNC-01 remains `[x]` / `Complete`; **SYNC-02 remains `[ ]` / `Pending`** — checked explicitly against the file, not assumed.

---
*Phase: 33-hard-sync*
*Completed: 2026-09-01*
