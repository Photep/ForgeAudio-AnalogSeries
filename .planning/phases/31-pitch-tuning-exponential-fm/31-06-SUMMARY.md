---
phase: 31-pitch-tuning-exponential-fm
plan: 06
subsystem: tests
tags: [doctest, non-vacuous-coverage, negative-control, bit-exact, libm-ground-truth, cents, exponential-fm, connected-gate, odr, anonymous-namespace, templated-drive-helper]

# Dependency graph
requires:
  - phase: 31-pitch-tuning-exponential-fm
    plan: 05
    provides: "tests/test_vco_pitch.cpp with its banner, its ONE tolerance constant, the estimator, the libm reference, the derived-ceiling helpers, the reserved invariant numbers 4-7, and the doctest `&&`/`||` limitation it predicted this plan would hit"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 04
    provides: "the shell that made COARSE (-5..+5 oct), FINE (-1..+1 semitone) and the bipolar FM attenuverter (-1..+1) reachable, with both FM fields forwarded UNCONDITIONALLY"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 03
    provides: "the four-term volt-domain summation through exactly one exp2, the fmConnected gate's placement, and kVcoMaxPitchVolts - the sequence the stand-in mirrors"
  - phase: 30-vcocore-skeleton-module-registration
    provides: "tests/VcoBlockDriver.hpp (unconditional per-sample timing overwrite, proven non-degenerate seed defaults) and DeliberatelyBrokenSharedStateCore, the model this plan's stand-in follows wholesale"
provides:
  - "invariant 4 (PITCH-02/D-02): COARSE measured over its whole declared -5..+5 range at all three rates, with FOUR non-integer values as the control for the word 'continuously' and three combined opposite-sign rows so a SUMMED expectation is required"
  - "invariant 5 (PITCH-03/D-03/D-00): FINE measured at two V/OCT values, plus the RELATIVE hundred-cent assertion at both range ends that pins the /12 divisor against both plausible wrong implementations"
  - "runBlockOn<CoreT>() - a templated drive helper owning timing exactly as the VCO block driver does, PINNED bit-exact against that driver before any FM claim, and fed NONSENSE timing so a helper that stopped overwriting would diverge"
  - "invariant 6 (FM-01/02/03/D-06/D-09): the summation identity bit-exact over an 8-row grid, bipolarity (inversion AND observable sign), the zero-attenuverter no-op, the connected gate against NaN/+-inf/1e30 in BOTH FM fields, one volt measured as 1200 cents, and per-sample FM asserted finite/bounded/non-constant/audibly-patched"
  - "invariant 7 (FM-03 non-vacuity, validation requirement 5): DeliberatelyMultiplicativeFmCore OBSERVED failing the same identity through the same helper - 5343 mismatching samples across the three rates, per-row table recorded"
  - "THE MEASURED BLINDNESS RULE, sharper than the plan's model: the FM-03 identity cannot distinguish summing from multiplying whenever EITHER pitch term is a WHOLE number of volts. Zero is whole, so the obvious test (V/OCT at default, sweep FM) is vacuous at EVERY FM voltage"
  - "kPitchLooseBoundV = 6.0f with its provenance and its explicit NOT-a-five-volt-assertion note (31-07 no longer needs to introduce it)"
  - "helpers 31-07 can reuse in the SAME anonymous namespace: runBlockOn<CoreT>(), seedLikeDriver<SeedableT>(), diffBlocks(), BlockDiff, kDriverSeed0/1, kDriverSpread0/1, kPitchLooseBoundV, FM_IDENTITY_GRID"
affects: [31-07, 31-08, 31-09, 32-morph-blep, 33-hard-sync]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "An identity can be SATISFIABLE BY THE WRONG IMPLEMENTATION over part of its own input space. Establishing WHICH part, by measurement, is a prerequisite to designing the grid - not a refinement of it"
    - "Keep the BLIND rows in the grid and MARK them, rather than deleting them. A deleted blind row leaves the next person to rediscover the hole; a marked one shows them its shape"
    - "When a plan prescribes a mechanism (skip out-of-range points) whose prescribed grid can never reach it, ADD an input that reaches it and ASSERT the mechanism fired. An unexercised mechanism is a coverage claim nothing observes"
    - "A comment that SPELLS a filename a standing grep is anchored on answers that grep with a false positive. Describe the file instead - the same discipline 30-05 applied to forbidden C++ constructs"
    - "Two templates, two parameter names: the drive helper is the one pinned by a uniqueness grep, so the seeding helper uses a different parameter name rather than weakening the pin"

key-files:
  created: []
  modified:
    - "tests/test_vco_pitch.cpp (715 -> 2025 lines)"

key-decisions:
  - "The blindness rule was MEASURED and then DERIVED, and the grid was rebuilt around the measured rule rather than the plan's 'integer volt sums' model - which would have left the sharpest hole (V/OCT at 0 V) uncovered and unrecorded"
  - "One row deliberately sums to +7.0 V so the skip path fires (skipped at 44.1/48 kHz, measured at 96 kHz), and CHECK(totalSkipped > 0) asserts it"
  - "The stand-in's accumulator is a per-instance MEMBER - the shared-static defect stays in tests/test_vco_core.cpp's control, so each stand-in proves one specific thing"
  - "The hundred-cent assertion is RELATIVE (core run vs core run), not absolute against the expectation, so it does not inherit the /12 it exists to pin"
  - "PITCH-02, PITCH-03, FM-01, FM-02 and FM-03 marked COMPLETE - the first requirements in this phase whose central claim has a control observed FIRING behind it"

patterns-established:
  - "A negative control's value is proportional to how much of the input space it shows to be blind, not to whether it fires somewhere"
  - "Record a control's ZEROES as prominently as its hits: the zeroes are the map of what the check cannot see"

requirements-completed: [PITCH-02, PITCH-03, FM-01, FM-02, FM-03]

coverage:
  - id: D1
    description: "PITCH-02 / D-02: a coarse value of n octaves shifts the MEASURED output pitch by exactly n octaves across the full -5..+5 declared range, and non-integer values track just as tightly"
    requirement: "PITCH-02"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"*COARSE*\" - exit 0, 1 test case matched, 170 assertions, 0 failed. 19-row grid at three rates. Worst |cents|: 0.004658187 (44.1 kHz @ coarse +5.0), 0.0033790525 (48 kHz @ coarse -2.37), 0.00337679175 (96 kHz @ coarse -2.37) - all inside the 0.05-cent tolerance by at least 10.7x and inside PITCH-02's practical one-cent expectation by 215x"
        status: pass
    human_judgment: false
  - id: D2
    description: "Continuity is MEASURED rather than assumed: four non-integer coarse values exclude both an octave snap and a semitone snap"
    requirement: "PITCH-02"
    verification:
      - kind: unit
        ref: "coarse -2.37, -0.5, +0.5 and +3.75 all track inside 0.0034 cents. An octave snap would move them 444, 600, 600 and 300 cents; a semitone snap would move -2.37 by 44 cents and is excluded by that row ALONE (the other three are whole numbers of semitones), which is recorded in the case comment so the row is not treated as interchangeable"
        status: pass
    human_judgment: false
  - id: D3
    description: "COARSE composes with V/OCT rather than replacing it - three combined rows with both terms non-zero and of opposite signs"
    requirement: "PITCH-02"
    verification:
      - kind: unit
        ref: "(+2.0, -3.5) sum -1.5 V; (-4.0, +2.25) sum -1.75 V; (+3.5, -5.0) sum -1.5 V. An implementation where one term overwrote the other passes every one-at-a-time row and fails these"
        status: pass
    human_judgment: false
  - id: D4
    description: "PITCH-03 / D-03 / D-00: fine = +/-1 semitone shifts the measured pitch by exactly +/-100 cents, pinning the semitone-to-octave divisor"
    requirement: "PITCH-03"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"*FINE*\" - exit 0, 1 test case matched, 156 assertions, 0 failed. Twelve measured shifts (3 rates x 2 V/OCT x up/down), every one inside 0.007 cents of a hundred: +100.003243/-100.002971 at V/OCT 0.0 and +99.9940002/-99.9938641 at V/OCT +2.0 (44.1 kHz), same shape at 48 and 96 kHz. No conversion at all would read 1200; a divisor of a hundred would read 12"
        status: pass
    human_judgment: false
  - id: D5
    description: "FM-03: a static FM voltage at a given attenuverter is BIT-IDENTICAL to the same volts added onto V/OCT with the jack unpatched, compared with a direct float inequality"
    requirement: "FM-03"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"*exponential FM*\" - exit 0, 2 test cases matched, 494 assertions, 0 failed. 8-row grid x 3 rates x 512-sample blocks, 0 mismatching samples on every row. grep -c 'doctest::Approx' == 0 across the file"
        status: pass
    human_judgment: false
  - id: D6
    description: "NON-VACUITY REQUIREMENT 5: a multiplicative stand-in mirroring the real guard sequence with exactly ONE deliberate defect FAILS the same identity through the SAME drive helper"
    requirement: "FM-03"
    verification:
      - kind: unit
        ref: "invariant 7 REQUIREs a non-zero mismatch total over the sighted rows and measures 1809 (44.1 kHz) + 1817 (48 kHz) + 1717 (96 kHz) = 5343 mismatching samples, with firstBad at sample 0 on three of the four rows and sample 2 on the fourth. Health asserted alongside (finite, inside kPitchLooseBoundV) so failing-by-garbage is distinguishable from failing-by-multiplying"
        status: pass
      - kind: integration
        ref: "grep -c 'struct DeliberatelyMultiplicativeFmCore' == 1 and it sits inside the file's SINGLE anonymous namespace (lines 147-713, struct at 640); grep -r under src/ finds 0 hits; grep -c 'freq \\*=' == 1; grep -c 'static double' == 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "The blindness of the identity is MEASURED and RECORDED, not merely avoided: four grid rows sit at exactly zero mismatches and are marked, including the V/OCT-at-0 V row a reasonable test author would have used alone"
    requirement: "FM-03"
    verification:
      - kind: unit
        ref: "rows 1, 3, 5 and 8 measure 0/512 at all three rates. Rule: blind whenever EITHER term is a whole number of volts. Confirmed independently outside doctest on this toolchain: at V/OCT 0.0 the two forms agree bit-for-bit at FM +0.75, +0.481 and -4.9; at V/OCT +0.25 they differ at FM +0.75"
        status: pass
    human_judgment: false
  - id: D8
    description: "FM-02: the attenuverter is bipolar - a full negative setting inverts the shift bit-exactly AND produces a different block from the full positive setting; zero is a bit-exact no-op"
    requirement: "FM-02"
    verification:
      - kind: unit
        ref: "at V/OCT +0.5 with fmVolts +1.5: the -1.0 block is 0/512 mismatches against the negated shift (-1.0 V) and >0 mismatches against the +1.0 block, at all three rates. Both halves asserted - the inversion claim alone is satisfied by an implementation that ignored the sign. With fmVolts +2.75 and atten 0.0 the patched block is 0/512 against unpatched"
        status: pass
    human_judgment: false
  - id: D9
    description: "D-09: with the jack UNPATCHED, ANY finite or non-finite FM voltage and attenuverter is a bit-exact no-op - which is what proves the connected gate does work"
    requirement: "FM-03"
    verification:
      - kind: unit
        ref: "7 hostile fmVolts (NaN, +inf, -inf, 1e30, -1e30, 3.7, 0) x 5 hostile fmAtten (NaN, +inf, -1e30, -1.0, 0.5) x 3 rates = 105 blocks, every one 0/256 mismatches against the both-fields-zero reference. If the gate were removed, a NaN would poison the summed volts, the negated D-14 bound would drive them to -64 V and the block would change completely"
        status: pass
    human_judgment: false
  - id: D10
    description: "FM-01 / D-06: one volt of FM at a full clockwise attenuverter measures exactly one octave on the OUTPUT, and per-sample-varying FM keeps the output finite, bounded and non-constant"
    requirement: "FM-01"
    verification:
      - kind: unit
        ref: "measured |1200-cent difference| inside the 0.05-cent tolerance at all three rates, behind REQUIRE(nUp >= 8) on BOTH blocks. Audio-rate leg: fmVolts alternating +2/-2 V EVERY sample at full attenuverter over 2048 samples - allFinite, |out| <= 6.0 V, non-constant, and >0 mismatches against the same block unpatched"
        status: pass
    human_judgment: false
  - id: D11
    description: "The drive helper is PINNED bit-exact against the real block driver BEFORE any FM claim is asserted"
    verification:
      - kind: integration
        ref: "REQUIRE(vd.mismatches == 0) at line 1498; the first FM subcase opens at line 1501 and every FM claim is below it. The validity functor injects sampleTime = 1.f and sampleRate = 1.f, so a helper that stopped overwriting timing would produce a visibly different block rather than an equal one. grep -c 'template <typename CoreT>' == 1"
        status: pass
    human_judgment: false
  - id: D12
    description: "The skip-out-of-range path is EXERCISED rather than being a mechanism the grid never reaches"
    verification:
      - kind: unit
        ref: "the (+2.0 V/OCT, +5.0 coarse) row sums to +7.0 V: skipped at 44.1 and 48 kHz (18 measured / 1 skipped each) and MEASURED at 96 kHz (19 measured / 0 skipped). CHECK(totalSkipped > 0) asserts the path fired"
        status: pass
    human_judgment: false
  - id: D13
    description: "The milestone guardrail holds: no src/ file, no frozen header, no block driver and no shipped-LFO source in the diff, and the six LFO goldens replay byte-identical"
    requirement: "guardrail"
    verification:
      - kind: integration
        ref: "git diff --name-only 0c61dea~1 ad8e7f4 == tests/test_vco_pitch.cpp alone; the same diff filtered for src/|VcoBlockDriver|BlockDriver|test_vco_core|main.cpp|FROZEN|AnalogLFO returns 0; boundary-anchored grep for the LFO golden driver's filename returns 0; make guards PASS; make strict PASS over the same four TUs; -tc=\"*golden*\" reports 9 cases passed inside every run"
        status: pass
    human_judgment: false

# Metrics
duration: 26min
completed: 2026-07-30
status: complete
---

# Phase 31 Plan 06: COARSE, FINE and Exponential FM Summary

**COARSE and FINE are now measured on the returned samples across their whole declared ranges — with non-integer values as the control for the word "continuously" and a RELATIVE hundred-cent assertion that pins the semitone divisor against both plausible wrong implementations — and FM-03's summation identity is bit-exact over a grid designed around a MEASURED fact the plan had modelled too weakly: the identity is blind to a multiplicative implementation whenever EITHER pitch term is a whole number of volts, which means the obvious test (V/OCT left at its 0 V default, sweep the FM jack) distinguishes nothing at any FM voltage. The permanent multiplicative stand-in is observed failing the identity on 5343 samples across the three rates, through the same drive helper, with its four ZERO rows recorded as the map of what the check cannot see.**

## Performance

- **Duration:** 26 min
- **Started:** 2026-07-30T02:24Z
- **Completed:** 2026-07-30T02:50Z
- **Tasks:** 3
- **Files created:** 0
- **Files modified:** 1 (`tests/test_vco_pitch.cpp`, 715 → 2025 lines)

## Task Commits

1. **Task 1: invariants 4 and 5 — coarse tune across its full range, fine tune pinning the semitone divisor** — `0c61dea` (test)
2. **Task 2: the templated drive helper and invariant 6, the FM summation identity** — `8d93945` (test)
3. **Task 3: invariant 7, the multiplicative stand-in that must FAIL the identity** — `ad8e7f4` (test)

---

## The finding that changed the plan: WHICH inputs the FM-03 identity is blind on

The plan warned that an integer-only FM grid would pass a multiplicative implementation, and it was right that the trap exists. **It modelled the trap one step too narrowly**, and the measurement is worth stating precisely because the real rule is both simpler and considerably more dangerous:

> **A row of the FM-03 identity grid is BLIND — a multiplicative implementation satisfies it BIT-EXACTLY — whenever EITHER pitch term is a WHOLE NUMBER of volts.**

Not "when the SUM is an integer", which is the shape the trap first looks like. Not "when BOTH terms are whole", which is what the plan prescribed. Either one suffices.

**Why it is exact rather than statistical.** The frozen exponential splits its argument into an exponent field and a fractional remainder (`x += 127.f; xi = (int32_t)x; yii = xi << 23;` then a Horner polynomial in the remainder). Adding a whole number of volts changes only the exponent field and leaves the polynomial's argument untouched — so its value at `a + b` with `a` whole is **exactly** `2^a` times its value at `b`. Scaling a float by an exact power of two is itself exact, so it commutes with the rounding of the multiply, and the two implementations land on identical bits. For every `b`, fractional or not.

**Why that is dangerous rather than merely interesting.** Zero is a whole number. So this test —

```
V/OCT unpatched at 0 V; sweep fmVolts; assert the identity
```

— which is the first test almost anyone would write for FM-03, **is vacuous at every FM voltage**. Confirmed numerically outside doctest on this toolchain: at V/OCT `0.0` the summing and multiplying forms agree bit-for-bit at FM `+0.75`, `+0.481` and `-4.9` alike, while at V/OCT `+0.25` they differ at FM `+0.75`. Grid rows 1 and 2 differ **only** in the V/OCT term, by a quarter of a volt, and that quarter volt is the entire difference between a vacuous test and a decisive one.

**What was done about it.** The grid was rebuilt around the measured rule rather than the modelled one, the classification column was renamed from `integerTerms` to `blindRow`, the V/OCT-at-zero row was **added and kept**, row 8 was reclassified, and the file banner gained a fifth named trap. The blind rows are deliberately **not** deleted: a deleted blind row leaves the next person to rediscover the hole, whereas a marked one shows them its shape.

## The FM-03 identity grid, and the control's per-row mismatch table

Eight rows, shared unchanged by invariant 6 (where the real core must satisfy the identity) and invariant 7 (where the stand-in must fail it) — one table, two consumers, so the control is provably driven over the same inputs the check is. Blocks are 512 samples; "mismatches" counts samples on which the patched block differs from the shifted-V/OCT block.

| row | V/OCT | fmVolts | atten | product | **summed volts** | 44.1 kHz | 48 kHz | 96 kHz | |
|---|---|---|---|---|---|---|---|---|---|
| 1 | **0.00** | +0.75 | +1.00 | +0.750 | **+0.750** | **0** | **0** | **0** | ← **BLIND — the obvious test** |
| 2 | +0.25 | +0.75 | +1.00 | +0.750 | **+1.000** | 492 | 491 | 469 | integer SUM, still decisive |
| 3 | +1.00 | +2.00 | +1.00 | +2.000 | **+3.000** | **0** | **0** | **0** | ← **BLIND — both terms whole** |
| 4 | -1.50 | +0.50 | +0.50 | +0.250 | **-1.250** | 328 | 342 | 297 | |
| 5 | +2.00 | -3.00 | +1.00 | -3.000 | **-1.000** | **0** | **0** | **0** | ← **BLIND — both terms whole** |
| 6 | +0.50 | +1.30 | +0.37 | +0.481 | **+0.981** | 487 | 483 | 463 | product not exactly representable |
| 7 | +3.25 | -1.75 | +1.00 | -1.750 | **+1.500** | 502 | 501 | 488 | |
| 8 | -2.00 | +0.60 | -0.50 | -0.300 | **-2.300** | **0** | **0** | **0** | ← **BLIND on the V/OCT term ALONE** |

**Sighted-row totals: 1809 (44.1 kHz) + 1817 (48 kHz) + 1717 (96 kHz) = 5343**, which is the number `REQUIRE(fractionalMismatchTotal > 0)` observes. Every sighted row diverges from **sample 0** (rows 2, 6, 7) or **sample 2** (row 4), so detection is immediate rather than something a long block has to accumulate.

**Four rows non-integer in the summed volts:** `+0.750`, `-1.250`, `+0.981`, `+1.500`, `-2.300` — five, in fact. **The pure-integer point is marked:** rows 3 and 5, both terms whole, both zero at all three rates. Note that row 8 is fractional in its sum *and* in its product and is blind anyway, which is exactly the fact the plan's model would have missed.

**If any of the four zeroes ever becomes non-zero, the frozen polynomial changed.** The exponent-field path is what makes whole-number arguments exact and it is byte-pinned by `check_frozen.sh`; the shipped LFO's goldens would move in the same commit. That instruction is written into invariant 7's banner, pointing at the frozen manifest rather than at this test.

## Invariant 4 — the COARSE grid, in full

19 rows, run at each of 44.1 / 48 / 96 kHz, all at `morph = 0.f`, `character = 0.f`, measured on the returned samples against `261.6256 * std::exp2(summed)` in double.

| tier | rows | V/OCT | coarse | summed volts |
|---|---|---|---|---|
| declared range + integers | 11 | `0.0` | `-5, -4, -3, -2, -1, 0, +1, +2, +3, +4, +5` | same |
| **non-integer** (the continuity control) | 4 | `0.0` | `-2.37, -0.5, +0.5, +3.75` | same |
| **combined, opposite signs** | 3 | `+2.0` / `-4.0` / `+3.5` | `-3.5` / `+2.25` / `-5.0` | `-1.5` / `-1.75` / `-1.5` |
| **deliberately above the binding limit** | 1 | `+2.0` | `+5.0` | `+7.0` |

**Per-rate worst absolute cents, and the coverage split:**

| Rate | Worst \|cents\| | At | Rows measured | Rows skipped |
|---|---|---|---|---|
| 44100 | **0.004658187** | coarse **+5.0** | 18 | **1** |
| 48000 | **0.0033790525** | coarse **-2.37** | 18 | **1** |
| 96000 | **0.00337679175** | coarse **-2.37** | 19 | **0** |

**The two range endpoints specifically, signed, at zero V/OCT** — the figures the plan asked to be stated per rate:

| coarse | 44.1 kHz | 48 kHz | 96 kHz |
|---|---|---|---|
| **-5.0** | +0.0000385655 | +0.0000493257 | +0.0000521421 |
| **+5.0** | +0.004658187 | **-0.00186869** | +0.000283846 |

Two things in that table are worth reading rather than skimming. **The `-2.37` row is the worst-measuring row at two of the three rates** — the non-integer values are not only the snap control, they are also where the polynomial works hardest, since a whole-number argument is bit-exact by construction in the exponent-field path (the same fact that makes the FM grid's blind rows blind). And **the low endpoint measures four orders of magnitude better than the high one**, because the estimator has whole cycles to work with down there: at `coarse -5.0` (8.18 Hz) the 44.1 kHz block yields **15 rising crossings**, the tightest crossing count anywhere in this case and still comfortably past the required eight.

**The snap hypotheses, and which row excludes which.** An octave snap would move `-2.37` by 444 cents, `+/-0.5` by 600 and `+3.75` by 300 — any of the four would fail by four orders of magnitude. A **semitone** snap is subtler and only **one** of the four catches it: `-0.5`, `+0.5` and `+3.75` octaves are whole numbers of semitones (`-6`, `+6`, `+45`) and a semitone snap would not move them at all, whereas `-2.37` octaves is `-28.44` semitones and moves 44 cents. That row is therefore **not interchangeable** with the others, and the case comment says so.

## Invariant 5 — the FINE grid and the twelve hundred-cent shifts

Fine values `-1, -0.5, -0.25, 0, +0.25, +0.5, +1` **semitones** (the POD's documented unit, D-05), each at V/OCT `0.0` **and** `+2.0`, at all three rates — 42 measurements.

**The measurements the case exists for**, all twelve:

| Rate | V/OCT | fine 0 → **+1** | fine 0 → **-1** |
|---|---|---|---|
| 44100 | 0.0 | **+100.003243** | **-100.002971** |
| 44100 | +2.0 | **+99.9940002** | **-99.9938641** |
| 48000 | 0.0 | **+100.003244** | **-100.002972** |
| 48000 | +2.0 | **+99.9941645** | **-99.9937555** |
| 96000 | 0.0 | **+100.003237** | **-100.002974** |
| 96000 | +2.0 | **+99.9940819** | **-99.9937695** |

Every one is inside **0.007 cents** of a hundred — a seven-thousandth of the shift being measured — at both V/OCT values. Part of that residual is the core rather than the estimator and is **accounted for rather than absorbed**: `(1.f / 12.f)` as a float is `0.08333333582`, so the core's own shift is `100.0000029` cents by construction. The rest is the estimator, and it **changes sign with the V/OCT value** — slightly over a hundred at concert pitch, slightly under two octaves up — which is the signature of apparatus error rather than of a wrong divisor.

**Why this specific number is the load-bearing one.** No conversion at all (treating the semitone field as octaves) would read **1200**; a divisor of a hundred (reading the field as the knob's *displayed* cents) would read **12**. A hundred separates the correct implementation from both. And the assertion is deliberately **RELATIVE** — core run against core run — because the tracking check alone could not do this job: it compares against an expectation this test computes with the same `/12` it is trying to pin, so the two would agree on any consistent divisor.

**Per-rate worst absolute tracking cents:** `0.00628057135` (44.1 kHz), `0.006281158` (48 kHz), `0.00633285261` (96 kHz) — all at V/OCT `+2.0`, fine `-0.25`.

## Test counts, before and after

| Run | Cases | Assertions | Failed |
|---|---|---|---|
| Pre-plan baseline (31-05 tip) | 75 | 2,616,745 | 0 |
| After Task 1 (`0c61dea`) | **77** | 2,617,071 | 0 |
| After Task 2 (`8d93945`) | **78** | 2,617,446 | 0 |
| After Task 3 (`ad8e7f4`) | **79** | **2,617,565** | 0 |
| Phase regression floor | 75 | 2,616,745 | 0 |

**+2 for Task 1** (invariants 4 and 5, as its criterion required), **+1** for each of Tasks 2 and 3, **0 failed** at every step. No existing case moved: the net addition is 820 assertions across four new cases.

**Every selector's matched case count, stated because a `-tc` filter that matches nothing also exits 0:**

| Invocation | Matched cases | Assertions | Exit |
|---|---|---|---|
| `-tc="*COARSE*"` | **1** | 170 | 0 |
| `-tc="*FINE*"` | **1** | 156 | 0 |
| `-tc="*exponential FM*"` after Task 2 | **1** | 375 | 0 |
| `-tc="*exponential FM*"` after Task 3 | **2** | **494** | 0 |
| `-tc="*golden*"` (guardrail spot check) | 9 | — | 0 |

Case-insensitivity was checked before naming: doctest's filters are case-insensitive by default, and **no** pre-existing case name in the whole suite contains `coarse` or `fine` as a substring (verified across all 79 names), so both new selectors are unambiguous. The word "defined" was deliberately kept out of every new case name — it contains `fine`.

## Gate results

| Gate | Required | Observed |
|---|---|---|
| `make test` | exit 0, 0 failed, +1 case per task (+2 for Task 1) | **79 / 2,617,565 / 0** |
| Compiler warnings from this file | zero under `-Wall -Wextra` | **zero** — clean rebuild after every task |
| `-tc="*COARSE*"` / `-tc="*FINE*"` | exit 0 **and** exactly 1 case each | **1 / 1** |
| `-tc="*exponential FM*"` | exit 0 **and** exactly 2 cases | **2** |
| `make guards` | exit 0, `guard suite: PASS` | **PASS** |
| `make strict` | exit 0, unchanged from 31-04 | **PASS** — `strict C++11 gate: PASS`, same four TUs (this plan touches no `src/` file) |
| Six LFO `.f32` goldens | byte-identical | replayed inside **every** `make test` run above |
| `git diff --name-only` over all 3 commits | `tests/test_vco_pitch.cpp` alone | **`tests/test_vco_pitch.cpp`** |

**Mechanical checks from the plan's acceptance criteria, as observed:**

| Check | Required | Observed |
|---|---|---|
| `grep -c 'template <typename CoreT>'` | 1 | **1** |
| `grep -c 'doctest::Approx'` | 0 | **0** |
| `grep -c 'struct DeliberatelyMultiplicativeFmCore'` | 1 | **1** (inside the single anonymous namespace, lines 147–713, struct at 640) |
| `grep -r 'DeliberatelyMultiplicativeFmCore' src/` | 0 | **0** |
| `grep -c 'forge::exp2_taylor5'` | exactly 2 | **2** (base pitch + the deliberate second exponential, both inside the stand-in) |
| `grep -c 'freq \*='` | 1 | **1** (the defect line) |
| `grep -c 'static double'` | 0 | **0** (the stand-in's accumulator is a member) |
| `grep -c 'forge::kVcoMaxPitchVolts'` | ≥ 1 | **2** |
| `grep -ciE 'DO NOT\|THE DELIBERATE DEFECT'` | ≥ 2 | **8** |
| `grep -cE 'constexpr double k.*Tolerance'` | 1 | **1** |
| `grep -c 'REQUIRE(nUp >= 8)'` | ≥ 3 | **3** |
| `grep -c 'fmConnected = true'` / `= false` | ≥ 1 each | **8 / 7** |
| `grep -cE '1200'` | ≥ 2 | **6** |
| `grep -c '0x1234ULL\|0x9E3779B9ULL'` | ≥ 1 | **2** |
| `grep -c '0, 0)'` / `'0x0ULL, 0x0ULL'` | 0 | **0 / 0** |
| `grep -cE '2\.37\|0\.5f\|3\.75'` | ≥ 3 | **12** |
| `grep -c 'MESSAGE('` | 0 | **0** |
| CHECK/REQUIRE lines containing alias/spectral/harmonic | 0 | **0** |
| BDD macros (`SCENARIO`/`GIVEN`/`WHEN`/`THEN`) | 0 | **0** |
| validity REQUIRE below every FM claim | line-ordered | **REQUIRE at 1498; first FM subcase at 1501** |

## Non-vacuity requirements — the running tally

| # | Requirement | Owner | Status |
|---|---|---|---|
| 1 | Measure the OUTPUT, not telemetry, on the primary tier | 31-05 | ✅ closed there |
| 2 | Ground truth from libm in double | 31-05 | ✅ closed there |
| 3 | Expectations one octave apart | 31-05 | ✅ closed there |
| 4 | `REQUIRE(nUp >= 8)` before any tolerance check | 31-05 | ✅ closed there, and **this plan added two more** (invariants 4 and 5) plus one on both blocks of the one-octave-per-volt measurement |
| 5 | **FM multiplicative negative control** | **this plan** | ✅ **closed — 5343 mismatching samples observed, per-row table recorded, four blind rows measured and marked** |
| 6 | Clamp-boundary case proving the clamp FIRES | 31-07 | ⬜ not implemented here and **not claimed** here |

## Milestone guardrail compliance

- `git diff --name-only 0c61dea~1 ad8e7f4` = **`tests/test_vco_pitch.cpp`**. One file, three commits, nothing else.
- **No frozen header, no `FROZEN.sha256`, no `src/AnalogLFO.cpp` and no `src/` file at all** in the three-commit diff (filtered grep returns `0`). `make strict` is therefore unchanged from the 31-04 result by construction, over the same four translation units.
- **The shipped LFO's golden replay driver is never included, and its filename is not even spelled in a comment** — see deviation 3. The boundary-anchored grep for it returns `0`, restoring the mechanical check 31-05 established.
- **`tests/VcoBlockDriver.hpp` is untouched, untemplated, unsubclassed and unaliased.** `runBlockOn<CoreT>()` is a separate function in the test TU.
- **`tests/test_vco_core.cpp`, `tests/main.cpp` and `tests/check_includes.sh` all unmodified.** Zero build wiring needed — the test target globs `tests/*.cpp`.
- **The stand-in never leaks into shipped source:** `grep -r 'DeliberatelyMultiplicativeFmCore' src/` → 0. It has internal linkage in this file's single anonymous namespace, so `check_includes.sh`, `check_canary.sh` and the strict C++11 gate never see it.
- **No deletions in any commit** (`git diff --diff-filter=D` empty across all three) and **no untracked residue** (`git status --porcelain --untracked-files=all` empty after each).
- **Zero registry packages** (T-31-SC): doctest is vendored in-tree.

## Decisions Made

1. **The blindness rule was measured first and the grid rebuilt around it.** See the finding above and deviation 1. The plan's `integerTerms` model was a genuine but narrower version of the truth, and building the grid on it would have left the sharpest hole — V/OCT at its `0 V` default — uncovered *and* unrecorded.
2. **The blind rows stay in the grid and are marked, rather than being removed.** Four of eight rows measure zero. That is not dead weight: rows 1 and 2 differ only in a quarter volt of V/OCT and bracket the entire distinction, which is the clearest possible statement of what the grid is for.
3. **One coarse row deliberately sums to `+7.0 V` so the skip path fires**, and `CHECK(totalSkipped > 0)` asserts it. See deviation 2.
4. **The hundred-cent assertion is RELATIVE, not absolute.** Core run versus core run, so it does not inherit the `/12` it exists to pin.
5. **The stand-in's accumulator is a per-instance member.** The shared-static defect stays in `tests/test_vco_core.cpp`'s control. Two defects in one stand-in would make it prove nothing specific: a control that fails an identity because it *also* shares state across instances does not tell you the identity is sensitive to the FM ordering.
6. **The stand-in's one consequence is named as a consequence, not counted as a second defect.** Because the FM contribution no longer enters the summed volts, it no longer passes through the D-14 bound either, so a hostile FM voltage would reach the stand-in's frequency unbounded. That is downstream of the single divergence. The banner says so, and the control is driven only over the benign identity grid — the hostile-input claim belongs to invariant 6's gate subcase over the *real* core.
7. **The validity functor injects nonsense timing (`sampleTime = 1.f`, `sampleRate = 1.f`).** A helper that stopped overwriting timing would then produce a visibly different block from the driver's rather than an equal one — which turns the validity REQUIRE from a shape check into a real one.
8. **Two templates, two parameter names.** `seedLikeDriver` is `template <typename SeedableT>` so the plan's uniqueness grep on the drive helper's `template <typename CoreT>` stays meaningful. One seeding callable still drives both core types, which is what the plan actually asked for.
9. **`kPitchLooseBoundV` landed here rather than in 31-07**, as the plan's artifact list permitted, with its provenance and its explicit *not*-a-five-volt-assertion note.
10. **PITCH-02, PITCH-03, FM-01, FM-02 and FM-03 marked COMPLETE.** Reasoning below.

### Why five requirements are marked complete after four consecutive plans declined

31-01 through 31-04 each declined to mark anything, every time for the same stated reason: they landed structure, constants or controls with **no behavioral gate behind the claim**. 31-04's summary was explicit that "31-06 owns the behavioral assertions that make them non-vacuous." **This plan is those assertions.** Reading the requirement texts individually:

- **PITCH-02** — *"COARSE tune knob sweeps ±5 octaves continuously."* The knob exists at `-5..+5` (31-04). Both endpoints and every integer between them are measured at three rates, inside `0.0047` cents; and *continuously* is measured rather than assumed by four non-integer values that exclude an octave snap and — via `-2.37` specifically — a semitone snap.
- **PITCH-03** — *"FINE tune knob trims ±1 semitone (±100 cents) for detuning/beating."* The knob exists at `-1..+1` semitones with a ×100 cents display (31-04). Twelve measured shifts land within `0.007` cents of a hundred at two V/OCT values, and the assertion is relative so it pins the divisor rather than echoing it.
- **FM-01** — *"Exponential FM input modulates pitch at audio rate."* The jack exists (31-04). One volt at a full attenuverter is measured at 1200 cents on the output; a modulator changing on **every sample** at four octaves of swing keeps the output finite, inside the bound, non-constant and bit-wise different from unpatched. Per the source comment, audio-rate operation is structural — the FM path is per-sample arithmetic with no rate limit anywhere in the chain — so there was nothing to switch on.
- **FM-02** — *"A dedicated bipolar attenuverter sets FM depth."* Declared bipolar `-1..+1` (31-04, D-07). Inversion is bit-exact against the negated shift, the two signs produce different blocks, and zero is a bit-exact no-op.
- **FM-03** — *"FM sums into the volt domain before the single exponential (musical exponential FM)."* The identity holds bit-exactly over all eight rows at three rates, compared with a direct float inequality — **and the multiplicative alternative is observed failing it through the same helper on 5343 samples.** This is the one requirement in the phase whose central claim now has a control observed *firing* behind it, and the map of where that control is blind is recorded alongside.

**Still pending and correctly so:** `PITCH-04` (31-07 owns the pitch-driven clamp-fires case), `PITCH-05` (its non-regression obligation is discharged; its own gate is the existing `phaseInRange` case, confirmed at the phase gate). `PITCH-01` and `TEST-02` were completed by 31-05.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 — Missing Critical] The plan's model of the FM-03 trap was narrower than the measured truth, and the grid built on it would have left the sharpest hole uncovered**

- **Found during:** Task 3, on the first run of the control — row 7 (`V/OCT -2.0`, product `-0.3`) measured **zero** mismatches at all three rates despite having a fractional product and a fractional sum, which the plan's `integerTerms` classification predicted would be a detecting row.
- **Issue:** The plan modelled blindness as "both terms are whole numbers" (equivalently, it warned about integer volt *sums*). Measured, blindness follows from **either** term being whole. Derived afterwards and confirmed independently outside doctest: the frozen exponential's exponent-field path makes its value at `a + b` with `a` whole exactly `2^a` times its value at `b`, and power-of-two scaling commutes with the multiply's rounding. **Zero is whole**, so a grid holding V/OCT at its default and sweeping the FM jack is blind at *every* FM voltage — the single most likely grid a test author would write, and 100 % vacuous.
- **Fix:** the classification column renamed `integerTerms` → `blindRow` with the measured rule stated where the grid is declared; a **new row 1** added at `V/OCT 0.0` with a fractional FM voltage, kept in the grid and marked BLIND so the hole's shape is visible; row 8 reclassified; invariant 6's banner rewritten to state the measured rule instead of the integer-sum model; and the file banner's trap list extended from four traps to five, with TRAP 5 carrying the rule and pointing at the measured table.
- **Evidence the fix was load-bearing rather than tidy:** under the plan's classification, row 8 would have been counted as a sighted row and its zero mismatches would have made `REQUIRE(fractionalMismatchTotal > 0)` weaker while a `CHECK(mismatches == 0)` on it would have **failed outright** — the plan as literally written produces a red case. More importantly, the grid would have carried no row at `V/OCT 0.0`, so the file would never have recorded that the obvious test is vacuous.
- **Files modified:** `tests/test_vco_pitch.cpp`. **Committed in:** `ad8e7f4`.

**2. [Rule 2 — Missing Critical] The coarse case's skip path could not fire on the prescribed grid**

- **Found during:** Task 1, while writing the skip logic the plan specifies.
- **Issue:** The plan requires out-of-range points to be *skipped, not clipped*, and requires the skipped points to be captured "so the coverage per rate is visible". But the prescribed grid maxes out at coarse `+5.0` with V/OCT at zero, and its combined rows are of *opposite* signs and therefore reduce the sum — so the largest summed volt on the prescribed grid is `+5.0`, comfortably below the binding limit of `+6.0752 V` at the tightest rate. **No point could ever be skipped, at any rate.** Shipping that would have left a mechanism, a comment about per-rate coverage, and a capture that nothing ever exercised — the same false-coverage class as 31-05's deviation 1, one level down.
- **Fix:** one extra row, `V/OCT +2.0` with coarse `+5.0`, summing to `+7.0 V` — above the binding limit at 44.1 and 48 kHz and *below* it at 96 kHz. So the row is skipped at two rates and measured at one, per-rate coverage genuinely differs (18/18/19 measured, 1/1/0 skipped), and `CHECK(totalSkipped > 0)` asserts the path fired rather than assuming it.
- **Files modified:** `tests/test_vco_pitch.cpp`. **Committed in:** `0c61dea`.

**3. [Rule 3 — Blocking] A comment naming the LFO golden driver's filename broke a standing mechanical check**

- **Found during:** Task 2's verification sweep.
- **Issue:** the drive helper's banner explained, correctly, that the *other* block driver under `tests/` is not included here — and named it. That lifted the boundary-anchored grep `grep -cE '(^|[^[:alnum:]_])BlockDriver\.hpp'` from **0 to 1**. The substantive claim was true (the file is not included; only a comment mentioned it), but the check 31-05 established as the mechanical form of the guardrail no longer read zero, so future plans would have inherited a check that could not distinguish a comment from an include.
- **Fix:** the comment now **describes** the file — "the OTHER block driver under `tests/`, the one that feeds the shipped LFO's bit-exact golden replay leg" — and states explicitly that its filename is deliberately not spelled because a standing grep is anchored on it. Anchored count back to **0**. This is the same trap class 30-05 documented (a banner that must document a rule it is simultaneously being grepped against, resolved by describing rather than spelling) and the same class as 30-09's LFO-filename zero-count.
- **Files modified:** `tests/test_vco_pitch.cpp`. **Committed in:** `8d93945`.

### Prescribed but worth recording

**4. A second template was required, and it deliberately uses a different parameter name.**
The plan asks the stand-in for "a seeding entry point and a spread-seeding entry point mirroring the real core field for field, so one seeding callable can drive both types", and separately requires `grep -c 'template <typename CoreT>'` to return exactly `1`. A single seeding callable over two unrelated types is necessarily a template, so `seedLikeDriver` is declared `template <typename SeedableT>`. Both requirements are satisfied as written: one seeding callable drives both core types, and the uniqueness grep still pins the *drive helper* specifically, which is what it exists to pin.

**5. The file banner was closed rather than left with a forward reference.**
The banner opened with "once plan 31-06 lands, that coarse tune, fine tune and exponential FM move it by exactly the stated amount" and described invariants 4–9 as RESERVED. Once this plan landed, both sentences were false in the very file whose organising principle is that a comment must not claim something the code does not do. Corrected to present tense, with 1–7 recorded as landed and 8–9 still reserved for 31-07, and invariants 4 and 5 given their actual claims in the list rather than bare labels.

### Verification-command notes (no code impact)

**6. `grep -c 'forge::exp2_taylor5'` is `0` after Task 1 and `2` after Task 3 — both criteria are satisfied, sequentially.**
Task 1's criterion requires `0` and Task 3's requires exactly `2`. They are not simultaneous: the polynomial appears in this file only inside the stand-in, which Task 3 introduces. Observed `0` at Task 1's commit and `2` at Task 3's. No test expectation anywhere is computed from the polynomial.

**7. `grep -c 'BlockDriver.hpp'` returns `2`, not `0`, and cannot return `0`.**
The two hits are `#include "VcoBlockDriver.hpp"` (line 139) and one comment naming `tests/VcoBlockDriver.hpp` as the source of the seed literals (line 420) — both contain `BlockDriver.hpp` as a *substring* of `VcoBlockDriver.hpp`. This is the criterion-filter artifact 31-05 already documented for the same pattern. The substantive claim is verified with the boundary-anchored form, which returns **0**: see deviation 3.

**8. `grep -c '5.f'` returns `4`, and both range endpoints are present.**
The `.` in that pattern is a wildcard, so it matches `-5.f` as well as `5.f`. The four hits are the coarse `-5.f` endpoint, the `+5.f` endpoint, the `-5.f` inside the combined row `(+3.5, -5.0)`, and the `+5.f` inside the deliberately-skipped row `(+2.0, +5.0)`.

**9. Doctest case-insensitivity was checked before the selectors were named.**
`-tc` filters are case-insensitive by default, so `*COARSE*` and `*FINE*` would match lowercase occurrences too. All 79 case names were checked: none contains `coarse` or `fine` as a substring. The word "defined" was kept out of every new case name for the same reason — `de-FINE-d`.

---

**Total deviations:** 3 auto-fixed (2 Rule 2 missing-coverage, 1 Rule 3 mechanical-check regression), 2 prescribed-but-recorded, 4 verification-command notes.
**Impact on plan:** No scope creep and no weakened assertion. Every claim the plan asked for is asserted, and the FM grid is one row larger and correctly classified because the trap turned out to be broader than the plan modelled.

## Issues Encountered

- **None blocking.**
- **A correction to 31-05's stated cause for the suite's runtime, measured this session.** 31-05 recorded that `make test` grew "from under a second to ~12 s" and attributed it to the primary tier's ~2.3 M `step(...)` calls. Measured here: the **test binary itself runs in 0.82 s** for all 79 cases and 2,617,565 assertions, while a clean `make test` takes **14.31 s** — so the cost is almost entirely the `-O2` **compile** of a now-2000-line translation unit, not the DSP. That matters for anyone deciding whether to trim coverage to save time: trimming grid points would buy essentially nothing, and the file's length is the lever. This plan roughly tripled the file and added only ~0.1 s of execution.
- **doctest's `&&`/`||` limitation was anticipated by 31-05 and never bit.** Its hand-off predicted the bipolarity case would hit it. Every compound condition in this plan was written as separate assertions from the outset (`CHECK(inversion.mismatches == 0)` and `CHECK(signDiff.mismatches > 0)` rather than one conjunction), and the boolean-accumulator idiom covered the long loops. Recorded because a warning that works is worth confirming.
- **Note for later plans (fifth confirmation):** `gsd-tools query state.record-metric` / `state.add-decision` take **named flags**, not the positional arguments the `execute-plan.md` workflow shows. Carried forward from 31-01 through 31-05.

## User Setup Required

None. No external service configuration, and **zero registry packages** — doctest is vendored in-tree and the test target globs `tests/*.cpp`, so no build wiring changed.

## Next Phase Readiness

- **Ready for 31-07.** Invariant numbers **8 and 9** are reserved in the banner with 31-07 named against them, so appending needs no renumbering. Four specific hand-offs:
  - **`kPitchLooseBoundV` already exists** in the anonymous namespace with its provenance comment, so 31-07 must **not** introduce it again (the plan's artifact list anticipated either owner).
  - **`runBlockOn<CoreT>()`, `seedLikeDriver<SeedableT>()`, `diffBlocks()` and `BlockDiff` are available** and already pinned against the real driver. The hostile-pitch case can drive the core directly through the helper without instantiating a driver per configuration.
  - **`clampCeilingVolts(sr)` is the helper the clamp-fires case wants** — drive just *above* it and expect the frequency pinned at `kVcoNyquistGuardFrac * sampleRate` while the output keeps oscillating (D-10). Note that invariant 4 already exercises a point above the binding limit by **skipping** it; 31-07's job is the opposite, to go there deliberately.
  - **Do NOT write a `tel.freqHz == 0` assertion for hostile pitch** — 31-03 moved that value to `1.41828e-17` by design (D-13). Carried forward from 31-05, still true.
- **A finding 32-morph-blep and any future FM work should inherit.** The blindness rule is a property of the **frozen exponential**, not of the VCO: any future test that asserts a bit-exact identity across two paths through `exp2_taylor5` is blind wherever one of the two arguments is a whole number. Phase 33's hard sync and any through-zero FM work in v2.1 will be writing exactly that shape of test.
- **The four zero rows in invariant 7 are a tripwire on the frozen header.** If any becomes non-zero, the frozen polynomial's exponent-field path changed; look at `check_frozen.sh` and the frozen manifest, and expect the shipped LFO's goldens to have moved in the same commit. That instruction is written into the source, not only here.
- **No blockers.**

## Self-Check: PASSED

- `tests/test_vco_pitch.cpp` — FOUND (2025 lines)
- `.planning/phases/31-pitch-tuning-exponential-fm/31-06-SUMMARY.md` — FOUND
- Commit `0c61dea` — FOUND
- Commit `8d93945` — FOUND
- Commit `ad8e7f4` — FOUND
- No file deletions in any of the three commits (`git diff --diff-filter=D --name-only 0c61dea~1 ad8e7f4` empty)
- No untracked residue after any commit (`git status --porcelain --untracked-files=all` empty)
- No `src/` file, no frozen header, no `FROZEN.sha256`, no `src/AnalogLFO.cpp`, no block driver and no other test TU in the three-commit diff
- `grep -r 'DeliberatelyMultiplicativeFmCore' src/` → 0 hits
- Every figure in this summary was read out of an actual run's output — the mismatch table, the cents figures, the crossing counts and both runtimes — not computed by hand

---
*Phase: 31-pitch-tuning-exponential-fm*
*Completed: 2026-07-30*
