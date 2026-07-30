---
phase: 31-pitch-tuning-exponential-fm
plan: 07
subsystem: tests
tags: [doctest, non-vacuous-coverage, nyquist-clamp, derived-bounds, hostile-input, negated-comparison, static-assert, type-traits, stand-in-mirror, comment-truth, odr, anonymous-namespace]

# Dependency graph
requires:
  - phase: 31-pitch-tuning-exponential-fm
    plan: 06
    provides: "tests/test_vco_pitch.cpp with invariants 1-7, reserved numbers 8 and 9, kPitchLooseBoundV already declared with its provenance, seedLikeDriver<SeedableT>(), and TRAP 5's measured blindness rule"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 05
    provides: "clampCeilingVolts(sr), estimatorCeilingVolts(sr), pitchBase(), estimateFreqRising(), expectedFreqHz(), centsError(), the ONE tolerance constant, kTelemetryBlockSamples, and the negated-comparison in-test idiom"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 03
    provides: "the four-term volt-domain summation, the fmConnected gate, kVcoMaxPitchVolts, the UBSan RED transcript this plan's banner cites rather than reproduces, and the measured fact that tel.freqHz for hostile pitch is 1.418e-17 and NOT zero"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 04
    provides: "the declared control ranges the reachable-envelope margin is computed FROM: COARSE -5..+5 oct, FINE -1..+1 semitones, FM_ATTEN -1..+1"
  - phase: 30-vcocore-skeleton-module-registration
    provides: "tests/VcoBlockDriver.hpp, the accumulate-then-check-once idiom with a first-bad-step index, and DeliberatelyBrokenSharedStateCore with the banner rule this plan's Task 3 discharges"
provides:
  - "invariant 8 (PITCH-04 / D-10 / validation requirement 6): the Nyquist clamp OBSERVED FIRING on three legitimate high notes per rate with tel.freqHz EXACTLY equal to the guard fraction times the float sample rate, AND the oscillator observed still oscillating there - thousands of crossings, a non-constant block, a full 5.000 V peak - plus a below-ceiling control proving the clamp does not fire early"
  - "invariant 9 (D-14 / D-22): the STANDING hostile-pitch regression check, 26 rows x 3 rates = 78 configurations x 4000 steps, both bound ends PINNED symbolically, both the pitch route and the FM route, three non-finite-attenuverter rows, five accumulated properties, firstBadStep = -1 everywhere"
  - "the reachable-envelope margin: an ARITHMETIC guarantee built from four named declared-range terms that the worst legitimate patch sums to 29.083333333333332 V against a 64 V bound - ratio 2.2005730659025788 - so the bound provably cannot fire on real music"
  - "PITCH-05 pinned two ways: a static_assert on the accumulator member's DECLARED TYPE (the only place a one-word narrowing is visible) and a 100000-step high-pitch runtime range check at all three rates"
  - "THE HONEST COLLAPSE RULE, recorded the way 31-06 recorded its blind rows: a hostile grid whose guard maps every input onto two values distinguishes only THREE outcomes, and the source says so rather than implying 26 independent observations"
  - "DeliberatelyBrokenSharedStateCore brought back in step with the real core's Phase 31 pitch block, its ONE deliberate defect intact, its exponential count still exactly one, and its 512/512/1024 figures RE-OBSERVED unchanged"
  - "the measured proof that a NOT-A-NUMBER lands on the NEGATIVE plateau, which is the negated-first comparison in the core made externally visible - a tripwire on anyone 'simplifying' that pair into the NaN-transparent comparison ladder"
affects: [31-08, 31-09, 32-morph-blep, 33-hard-sync, 34-drift-output]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "An INEQUALITY is satisfied by a dead oscillator. `freqHz <= ceiling` is true at zero, so a bound alone cannot distinguish a working clamp from a silenced one - the EXACT EQUALITY plus a liveness assertion is what makes a clamp observable"
    - "A boundary needs BOTH sides. A case that only drives above a threshold is satisfied by an implementation that clamps ALWAYS; the below-threshold control is what turns a constant into a boundary"
    - "When a guard COLLAPSES a wide input grid onto a few outcomes, record the collapse. 26 hostile rows that produce 3 distinct observations are 3 facts, not 26, and saying so in the source is the same discipline as marking a blind row"
    - "Some guarantees have NO behavioral form, and that is a property of the guarantee rather than a gap in the test. When a downstream clamp dominates, whether an upstream bound fired is invisible in the output - which is exactly why the bound is inaudible, and why its evidence must be arithmetic built from declared ranges rather than a typed-in total"
    - "Update a file's own banner in the SAME COMMIT that falsifies it, even mid-plan. Two banner edits across two commits cost nothing; one commit shipping a banner that calls a landed invariant RESERVED is the false-comment class this repo has retired four times"
    - "A stand-in's drift from the core it mirrors is INVISIBLE PRECISELY WHEN IT IS INERT: the control keeps passing, so nothing fires. The obligation has to be discharged by the phase that moved the core, on the banner's own written rule, not discovered later"

key-files:
  created:
    - ".planning/phases/31-pitch-tuning-exponential-fm/31-07-SUMMARY.md"
  modified:
    - "tests/test_vco_pitch.cpp (2025 -> 2713 lines)"
    - "tests/test_vco_core.cpp (1108 -> 1160 lines, changes confined to lines 315-403: the stand-in's banner and its step body)"

key-decisions:
  - "Invariant 8 asserts an EXACT float equality on tel.freqHz, recomputed in FLOAT from the constant times the float rate through the core's own sanitising ternary - doing the recomputation in double and narrowing would land on a different value and turn the equality into a coin flip"
  - "Invariant 9 asserts NO non-constant property anywhere, and the source says why at the negative pinned point: the guarded -64 V produces a denormal-scale POSITIVE frequency, so the negated floor correctly does not fire and the output is DC by decision (D-13)"
  - "Invariant 9 uses three doctest SUBCASEs so the 78-configuration hostile loop runs ONCE rather than once per section - a plain sequence of blocks would have been equivalent, but the subcase form names the three claims separately in the failure output"
  - "The margin assertion is built from four NAMED declared-range terms in double, never a typed-in total, so it is THIS assertion that fires if a later phase widens a control range or lowers the bound"
  - "Task 3 also mirrored the SANITISED RATE, not only the pitch block - the plan's action listed it, and the stand-in had been reading in.sampleRate raw since Phase 30"
  - "The stale `// Phase 31 - unread` comments in tests/test_vco_core.cpp's coreBase() were NOT fixed: the plan explicitly forbids touching the base-input helper and confines the diff to the stand-in. Handed to 31-08, which owns deferred-items.md"
  - "PITCH-04 and PITCH-05 marked COMPLETE, with the two forward clauses (sync, band-limiting) recorded as future-phase re-confirmations rather than as gaps in what is asserted today"

patterns-established:
  - "Record what a criterion measured BEFORE the work as well as after. A criterion already satisfied at the pre-plan tip is not evidence of the plan"
  - "A stalled oscillator is not a quiet one. The largest peak magnitude in the hostile grid is on the FROZEN-output rows, which is why a magnitude bound is a real check there and a finiteness test alone would not be"

requirements-completed: [PITCH-04, PITCH-05]

coverage:
  - id: D1
    description: "PITCH-04 / D-10 / validation requirement 6: above the derived ceiling the clamp FIRES - tel.freqHz equals the guard fraction times the float sample rate EXACTLY - and the oscillator KEEPS SOUNDING rather than going silent"
    requirement: "PITCH-04"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"*Nyquist*\" - exit 0, 1 test case matched, 93 assertions, 0 failed. Nine above-ceiling points (3 offsets x 3 rates): tel.freqHz = 21829.5 / 23760.0 / 47520.0 Hz, EXACT equality on all nine; nUp = 5457 / 5939 / 11879 rising crossings; peak = 5.000 V with blockMin = -5 and blockMax = +5 exactly; every sample finite and inside kPitchLooseBoundV"
        status: pass
    human_judgment: false
  - id: D2
    description: "The clamp does NOT fire early - a control point one volt below the ceiling reads strictly under the recomputed maximum and is still the RIGHT note against the libm reference"
    requirement: "PITCH-04"
    verification:
      - kind: unit
        ref: "Three below-ceiling points: 10914.73438 Hz vs a 21829.5 ceiling (reference 10914.74967, -0.00242652678 cents), 11879.96484 vs 23760 (11879.99964, -0.00507139295 c), 23759.92969 vs 47520 (23759.99929, -0.00507139295 c). Strict `<`, not `<=`: at-or-below would be satisfied by a clamp that fired here too. Every cents figure is inside a TENTH of the file's 0.05-cent tolerance"
        status: pass
    human_judgment: false
  - id: D3
    description: "No tracking assertion exists above the clamp ceiling, where the pitch is intentionally wrong - the estimator is called for its crossing count only and its returned frequency is discarded"
    requirement: "PITCH-04"
    verification:
      - kind: integration
        ref: "the above-ceiling call site is `estimateFreqRising(out, sr, &nUp);` at line 2198 with the return value dropped - the only such call in the file (the other two, at 1757-1758, are invariant 6's and bind their results). No CHECK or REQUIRE in invariant 8 compares a measured frequency to expectedFreqHz at any above-ceiling point; the only expectedFreqHz read in the case is at the BELOW-ceiling point"
        status: pass
    human_judgment: false
  - id: D4
    description: "D-21: no Hz literal and no volt literal - the ceiling volt comes from the existing derived helper and the expected maximum from forge::kVcoNyquistGuardFrac"
    requirement: "PITCH-04"
    verification:
      - kind: integration
        ref: "grep -c 'forge::kVcoNyquistGuardFrac' == 6 (was 3); non-comment grep for a hardcoded Hz ceiling (2183x|2376x|4752x) == 0; grep -cE 'constexpr double k.*Tolerance' == 1 (no new tolerance); exactly one `constexpr float kPitchLooseBoundV` definition, now with 5 references"
        status: pass
    human_judgment: false
  - id: D5
    description: "D-14 / D-22: the STANDING hostile-pitch case keeps five properties over 78 configurations, with both bound ends pinned symbolically and the FM route covered as well as the pitch route"
    requirement: "PITCH-04"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"*hostile pitch*\" - exit 0, 1 test case matched, 395 assertions, 0 failed. 26 rows x 3 rates x 4000 steps: allFinite, peak inside kPitchLooseBoundV, accumulator inside [0,1), telemetry non-negative, telemetry at or below the symbolically recomputed ceiling. firstBadStep = -1 on every one of the 78 configurations. The last two checks are written negated so a not-a-number reads as a failure"
        status: pass
      - kind: integration
        ref: "grep -c 'forge::kVcoMaxPitchVolts' tests/test_vco_pitch.cpp == 6; non-comment grep for '64.f\\|64.0' == 0 - the bound is never a literal in code. grep -c 'if (!(' == 21"
        status: pass
    human_judgment: false
  - id: D6
    description: "T-31-24: the pitch-volt bound provably cannot fire on any reachable patch - asserted arithmetically from four named declared control ranges with at least a factor-of-two margin"
    requirement: "PITCH-04"
    verification:
      - kind: unit
        ref: "measured 12 + 5 + 0.083333333333333329 + 12 = 29.083333333333332 V against boundVolts = 64, ratio = 2.2005730659025788. CHECK(worstReachableVolts < boundVolts) and CHECK(marginRatio >= 2.0). Built from the ranges, not from a total, so it is this assertion that fires if a control widens or the bound drops"
        status: pass
    human_judgment: false
  - id: D7
    description: "PITCH-05: the phase accumulator is still a double, proven at COMPILE TIME, and the increment still casts both operands, proven at the source level"
    requirement: "PITCH-05"
    verification:
      - kind: integration
        ref: "grep -c 'static_assert' tests/test_vco_pitch.cpp == 1, at line 2465: `static_assert(std::is_same<decltype(forge::VcoCore::phase), double>::value, ...)`. grep -c 'type_traits' == 1 (the include line only). grep -c 'double deltaPhase = (double)freq * (double)in.sampleTime;' src/dsp/VcoCore.hpp == 1"
        status: pass
      - kind: unit
        ref: "100000 steps one volt above each rate's clamp ceiling - the maximal-increment, constantly-wrapping condition - with the accumulator asserted in [0,1) every step. Final phase 0.99909167672740296 (44.1 kHz), 0.99946984462440014 (48 and 96 kHz), firstBadStep = -1 at all three rates"
        status: pass
    human_judgment: false
  - id: D8
    description: "T-31-26: the other test file's stand-in mirrors the real core's new pitch block, still differs from it in EXACTLY ONE deliberate way, and its recorded mismatch figures are re-observed unchanged"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"*independence positive control*\" -s reports r.mismatchA = 512, r.mismatchB = 512, totalMismatch = 1024 at each of 44100 / 48000 / 96000 - IDENTICAL to the pre-edit capture. -tc=\"*two-instance independence*\" still exit 0 with 18 assertions. Whole-suite counts identical: 81 / 2,618,053 / 0 before and after the edit"
        status: pass
      - kind: integration
        ref: "grep -c 'static double sharedPhase = 0.0;' == 1 (still a function-local static); grep -c 'THE DELIBERATE DEFECT, AND THE ONLY ONE' == 1; grep -c 'forge::exp2_taylor5' == 1 (still exactly ONE exponential - the mirror did not acquire the multiplicative shape); grep -c 'in.fine * (1.f / 12.f)' == 1; grep -c 'if (in.fmConnected)' == 1; grep -c 'forge::kVcoMaxPitchVolts' == 2; grep -c '21609' == 1 (the historical literal untouched); grep -r 'DeliberatelyBrokenSharedStateCore' src/ == 0"
        status: pass
    human_judgment: false
  - id: D9
    description: "D-15: the hostile TIMING grids in tests/test_vco_core.cpp are not extended - the deferral to the oversampled-inner-loop phase stands, and this plan's hostile coverage is a different input class"
    verification:
      - kind: integration
        ref: "grep -c 'HOSTILE_RATES\\|HOSTILE_TIMES' == 4, unchanged from the baseline; git diff -U0 over the file shows ZERO changes inside either array (the three diff hunks are at 314, 346 and 350, all inside the stand-in's banner and struct). Invariant 9's own timing is deliberately legitimate at every point and a comment restates the deferral"
        status: pass
    human_judgment: false
  - id: D10
    description: "T-31-20: every selector's matched case count is asserted rather than only its exit status"
    verification:
      - kind: unit
        ref: "-tc=\"*Nyquist*\" -> 1 case / 93 assertions; -tc=\"*hostile pitch*\" -> 1 / 395; -tc=\"*independence positive control*\" -> 1 / 6; -tc=\"*two-instance independence*\" -> 1 / 18; -tc=\"*v/oct tracking*\" -> 2 / 603; -tc=\"*exponential FM*\" -> 2 / 494. No pre-existing case name in the suite contains 'nyquist' or 'hostile' as a substring (checked case-insensitively across all names before naming), so both new selectors are unambiguous"
        status: pass
    human_judgment: false
  - id: D11
    description: "The milestone guardrail holds: no src/ file, no frozen header, no FROZEN.sha256, no src/AnalogLFO.cpp and neither block driver in the diff, and the six LFO goldens replay byte-identical"
    requirement: "guardrail"
    verification:
      - kind: integration
        ref: "git diff --name-only d42157e~1 89a70e1 == tests/test_vco_pitch.cpp and tests/test_vco_core.cpp, nothing else; the same diff filtered for src/|FROZEN|AnalogLFO|VcoBlockDriver|BlockDriver|main.cpp returns 0. make guards PASS; make strict PASS over the same four TUs (unchanged from 31-04 by construction - this plan touches no src/ file); -tc=\"*golden*\" reports 9 cases / 49188 assertions passing inside every run"
        status: pass
    human_judgment: false

# Metrics
duration: 19min
completed: 2026-07-30
status: complete
---

# Phase 31 Plan 07: The Nyquist Clamp Firing, the Standing Hostile-Pitch Case, and the Mirror Brought In Step Summary

**The Nyquist clamp is now OBSERVED FIRING on a legitimate high note — `tel.freqHz` exactly equal to the guard fraction times the float sample rate at nine above-ceiling points, with thousands of rising crossings and a full 5.000 V peak proving the oscillator keeps sounding rather than going silent, and a below-ceiling control proving it does not fire early. The pitch-volt bound has a permanent 78-configuration regression check whose banner states honestly that it is the regression check and NOT the red, an arithmetic guarantee built from four declared control ranges that it cannot fire on any reachable patch (29.08 V against 64 V, a 2.2× margin), and PITCH-05 pinned at compile time. And the other test file's stand-in is back in step with the core it promises to mirror, with its 512/512/1024 figures re-observed unchanged and the whole-suite counts bit-stable.**

## Performance

- **Duration:** 19 min
- **Started:** 2026-07-30T02:54Z
- **Completed:** 2026-07-30T03:13Z
- **Tasks:** 3
- **Files created:** 0
- **Files modified:** 2 (`tests/test_vco_pitch.cpp` 2025 → 2713 lines; `tests/test_vco_core.cpp` 1108 → 1160 lines)

## Task Commits

1. **Task 1: invariant 8 — the Nyquist clamp fires at its derived ceiling and the oscillator keeps sounding** — `d42157e` (test)
2. **Task 2: invariant 9 — the standing hostile-pitch case, the reachable-envelope margin, and PITCH-05's two pins** — `7d09ef6` (test)
3. **Task 3: the shared-state stand-in brought back in step with the new pitch block** — `89a70e1` (test)

---

## Invariant 8 — every point, per rate, as observed

Harvested with a temporary print that was removed before the commit. Above-ceiling blocks are 250 ms at `morph = 0`, `character = 0`; the below-ceiling point uses the file's 8-sample telemetry-read idiom. **Nothing in this table is typed into code** — the ceiling volt comes from `clampCeilingVolts(sr)` and the expected maximum from `forge::kVcoNyquistGuardFrac` times the float rate.

| Rate | Derived ceiling V | Offset | Recomputed max Hz | Observed `tel.freqHz` | Verdict | nUp | Peak V |
|---|---|---|---|---|---|---|---|
| 44100 | **+6.38263152** | +0.25 | 21829.5 | **21829.5** | **EXACT** | 5457 | **5.000** |
| 44100 | +6.38263152 | +1.00 | 21829.5 | **21829.5** | **EXACT** | 5457 | 5.000 |
| 44100 | +6.38263152 | +3.00 | 21829.5 | **21829.5** | **EXACT** | 5457 | 5.000 |
| 44100 | +6.38263152 | **−1.00** | 21829.5 | **10914.73438** | **strictly under** | — | — |
| 48000 | **+6.50488727** | +0.25 | 23760.0 | **23760.0** | **EXACT** | 5939 | 5.000 |
| 48000 | +6.50488727 | +1.00 | 23760.0 | **23760.0** | **EXACT** | 5939 | 5.000 |
| 48000 | +6.50488727 | +3.00 | 23760.0 | **23760.0** | **EXACT** | 5939 | 5.000 |
| 48000 | +6.50488727 | **−1.00** | 23760.0 | **11879.96484** | **strictly under** | — | — |
| 96000 | **+7.50488727** | +0.25 | 47520.0 | **47520.0** | **EXACT** | 11879 | 5.000 |
| 96000 | +7.50488727 | +1.00 | 47520.0 | **47520.0** | **EXACT** | 11879 | 5.000 |
| 96000 | +7.50488727 | +3.00 | 47520.0 | **47520.0** | **EXACT** | 11879 | 5.000 |
| 96000 | +7.50488727 | **−1.00** | 47520.0 | **23759.92969** | **strictly under** | — | — |

**The three below-ceiling points against the libm reference**, which is the other half of the boundary claim:

| Rate | Driven V | `tel.freqHz` | libm reference | Cents |
|---|---|---|---|---|
| 44100 | +5.38263152 | 10914.73438 | 10914.74967 | **−0.00242652678** |
| 48000 | +5.50488727 | 11879.96484 | 11879.99964 | **−0.00507139295** |
| 96000 | +6.50488727 | 23759.92969 | 23759.99929 | **−0.00507139295** |

Every one is inside **a tenth** of the file's 0.05-cent tolerance, so one volt below the ceiling the oscillator is not merely under the limit — it is playing the right note.

**Three things worth reading rather than skimming.**

**The telemetry frequency, the crossing count and the peak are IDENTICAL across all three above-ceiling offsets at a given rate.** A quarter volt above and three volts above produce the very same block. That is what a *hard* clamp looks like from the outside, and it is not what a soft one would look like — which is why the `+3.00` offset earns its place: at eight times the ceiling frequency before clamping, a loosely-applied ceiling would show.

**The peak at the ceiling is the FULL five volts**, with `blockMin = −5` and `blockMax = +5` exactly. The accumulator's near-`0.495`-per-sample walk lands on the sine's own maximum, so "peaks flatten out" is a flattening of the waveform **shape**, not a loss of level. Nothing here is quiet, let alone silent. This is the specific fact that distinguishes D-10's decided behavior from the rejected amplitude-fade alternative, and it is measured rather than assumed.

**The crossing counts are in the thousands** — 5457, 5939, 11879 — so the eight this case requires is not a bar the block barely clears. `REQUIRE(nUp >= 8)` runs **before** every other assertion at each point, so a silenced oscillator is a hard failure rather than a wrong number.

### Why the below-ceiling control is load-bearing rather than tidy

Without it, **every assertion in the above-ceiling half is satisfied by a core that pinned the frequency to the ceiling ALWAYS, at every pitch** — a catastrophic bug reading as a green case. The strict `<` (never `<=`) is what excludes it. This is the same structural point 31-05 made about the inter-ceiling band and 31-06 made about the blind rows: a check that cannot fail on the plausible wrong implementation is not evidence.

### And why an inequality was never going to be enough

The suite already pinned the Nyquist bound before this plan — `tests/test_vco_core.cpp`'s scenario four recomputes the ceiling symbolically and requires `tel.freqHz <= ceiling`. Two things follow that it therefore could not see, and both are closed here:

1. It is driven by hostile **timing**, so it never observes the clamp firing on a legitimate high note reached through the **pitch**, which is the only route a user ever takes.
2. **An inequality is satisfied by a dead oscillator.** `freqHz <= ceiling` is true at zero. A core that silenced itself above the ceiling would have passed that pin forever.

That second point is validation requirement 6, and it is why the case asserts an **exact equality** plus three liveness properties rather than a bound.

## Invariant 9 — the full hostile grid and what it actually distinguishes

26 rows × 3 rates × 4000 steps = **78 configurations**, all with deliberately **legitimate** timing (D-15). Five properties accumulated per configuration with a first-bad-step index, `CHECK`ed once. **`firstBadStep = −1` on every one of the 78** — not one step of any configuration violated anything.

| # | Route | pitchCV | fmVolts | fmAtten | Jack | Outcome |
|---|---|---|---|---|---|---|
| 1 | pitch | **0** (control) | 0 | 0 | off | control |
| 2 | pitch | **quiet NaN** | 0 | 0 | off | **negative plateau** |
| 3 | pitch | **+inf** | 0 | 0 | off | positive plateau |
| 4 | pitch | **−inf** | 0 | 0 | off | negative plateau |
| 5 | pitch | **+1e30** | 0 | 0 | off | positive plateau |
| 6 | pitch | **−1e30** | 0 | 0 | off | negative plateau |
| 7 | pitch | **+200 V** | 0 | 0 | off | positive plateau |
| 8 | pitch | **−200 V** | 0 | 0 | off | negative plateau |
| 9 | pitch | **+130 V** | 0 | 0 | off | positive plateau |
| 10 | pitch | **−130 V** | 0 | 0 | off | negative plateau |
| 11 | pitch | **+`kVcoMaxPitchVolts`** | 0 | 0 | off | **PINNED (D-22)** — positive plateau |
| 12 | pitch | **−`kVcoMaxPitchVolts`** | 0 | 0 | off | **PINNED (D-22)** — negative plateau |
| 13 | **FM** | 0 | **quiet NaN** | +1.0 | **on** | negative plateau |
| 14 | FM | 0 | **+inf** | +1.0 | on | positive plateau |
| 15 | FM | 0 | **−inf** | +1.0 | on | negative plateau |
| 16 | FM | 0 | **+1e30** | +1.0 | on | positive plateau |
| 17 | FM | 0 | **−1e30** | +1.0 | on | negative plateau |
| 18 | FM | 0 | **+200 V** | +1.0 | on | positive plateau |
| 19 | FM | 0 | **−200 V** | +1.0 | on | negative plateau |
| 20 | FM | 0 | **+130 V** | +1.0 | on | positive plateau |
| 21 | FM | 0 | **−130 V** | +1.0 | on | negative plateau |
| 22 | FM | 0 | **+`kVcoMaxPitchVolts`** | +1.0 | on | **PINNED through FM** |
| 23 | FM | 0 | **−`kVcoMaxPitchVolts`** | +1.0 | on | **PINNED through FM** |
| 24 | FM | 0 | +5 V (finite) | **quiet NaN** | on | **non-finite KNOB** — negative plateau |
| 25 | FM | 0 | +5 V (finite) | **+inf** | on | **non-finite KNOB** — positive plateau |
| 26 | FM | 0 | +5 V (finite) | **−inf** | on | **non-finite KNOB** — negative plateau |

**Both pinned bound points come from `forge::kVcoMaxPitchVolts`, never a literal** — verified: a non-comment grep for `64.f\|64.0` across the whole file returns **0**.

### The honest shape of what this grid distinguishes — three outcomes, not twenty-six

Because the bound maps every hostile input onto one of exactly two values, the 26 rows **collapse** onto three observable outcomes per rate. That collapse is recorded in the source, in the same spirit as 31-06's four marked blind rows: a grid that produces three distinct observations is three facts, and claiming twenty-six would overstate it.

| Outcome | Observed `tel.freqHz` | Peak V |
|---|---|---|
| **the CONTROL** (row 1) | **261.6256104** | 5.000000 (44.1 kHz) / 4.999999523 (48 & 96 kHz) |
| **POSITIVE plateau** (rows 3, 5, 7, 9, 11, 14, 16, 18, 20, 22, 25) | the rate's ceiling **EXACTLY**: 21829.5 / 23760 / 47520 | 4.998722553 (44.1 kHz) / 4.998712063 (48 & 96 kHz) |
| **NEGATIVE plateau** (rows 2, 4, 6, 8, 10, 12, 13, 15, 17, 19, 21, 23, 24, 26) | **1.418275276e-17** at all three rates | **5.000000** |

**Two facts in that table are load-bearing.**

**A NOT-A-NUMBER lands on the NEGATIVE plateau** (rows 2, 13, 24). That is the core's **negated-first** comparison made externally visible: a NaN fails `pitchVolts > -kVcoMaxPitchVolts`, the negation is true, and it becomes minus the bound. If anyone ever "simplified" that pair into `forge::clamp` — whose *both* comparisons are false for a NaN, so it passes one straight through — **those three rows would stop landing here.** The grid is therefore a live tripwire on the one substitution D-14 rejects by name in the source, not just a survival check.

**The peak magnitude at the negative plateau is the LARGEST of the three, not the smallest.** The output there is a frozen constant at whatever the waveform's value happens to be, so **a stalled oscillator is not a quiet one.** That is precisely why the loose magnitude bound is a real check on those rows and why a finiteness test alone would not be — the same argument Phase 30 recorded when it measured −8,655,011 V with every sample still finite.

### Why no non-constant assertion appears anywhere in invariant 9

At minus the bound the frequency is denormal-scale but **positive** (1.418275276e-17 Hz measured here, matching the 1.418e-17 recorded independently in the header's own rationale), so the negated frequency floor **correctly does not fire**, the phase increment is ~3.2e-22, and the output is effectively DC. **That is D-13's stated low-end decision, not a defect** — no low-end frequency floor is added anywhere, because a VCO tuned absurdly low genuinely *is* direct current and the user asked for it. Invariant 8 is where the oscillator is *required* to keep sounding; invariant 9 is where it is allowed to stall.

**The related trap is written into the source as a prohibition:** do not write a `tel.freqHz == 0` assertion for hostile pitch. That value *used* to be zero, before the bound existed, only because the undefined behavior produced garbage the floor happened to sanitise. It is now a small **positive** number and such an assertion would fail on correct behavior. (31-03, 31-05 and 31-06 each carried this warning forward; this plan is where it would have bitten.)

### The banner says honestly what this case is and is not

Stated in the source, at length, because the label matters more here than anywhere else in the file:

- **This case is the PERMANENT REGRESSION CHECK for the pitch-volt bound. It is NOT the evidence that the bound was needed.**
- A behavioral red was **MEASURED VACUOUS**: with the guard absent the core *already* survived a quiet NaN, both infinities, very large finite magnitudes of both signs and ±200 V with every sample finite, a telemetry frequency of zero and a peak inside five volts — because the existing negated frequency floor catches the garbage *result* the undefined behavior produces. Every assertion a reasonable person would have written as the red was already green.
- The red therefore came from a **one-shot undefined-behavior sanitizer probe**, run outside the working tree during 31-03, which named both frozen sites by file, line and column — the float-to-int cast at `src/dsp/RackCompat.hpp:106` and the left shift at `:109`. Its verbatim transcript and the clean re-run live in **31-03's summary**; this plan cites it rather than reproducing it.
- **A permanent repository-wide sanitizer gate is forbidden (D-24)**, because the shipped module reaches the identical latent problem through its own FM path. Fixing that is a milestone-guardrail event nobody has signed off. The source says "do not add `-fsanitize=undefined` to the build or to CI on the strength of this case."

## The reachable-envelope margin — and why arithmetic is the RIGHT form of evidence here

| Term | Source | Volts |
|---|---|---|
| conventional max cable magnitude on V/OCT | the platform's ±12 V signal-cable norm | **12** |
| coarse tune range magnitude | `COARSE_PARAM` declared −5..+5 **octaves** | **5** |
| fine tune range magnitude ÷ 12 | `FINE_PARAM` declared −1..+1 **semitones**, divided as the core divides it (D-05) | **0.083333333333333329** |
| max cable magnitude × a full attenuverter | `FM_ATTEN_PARAM` declared −1..+1, at 1.0 oct/V with no depth constant (D-06) | **12** |
| **worst reachable total** | | **29.083333333333332** |
| **the bound** | `forge::kVcoMaxPitchVolts`, symbolic | **64** |
| **margin ratio** | | **2.2005730659025788** |

The requirement is a factor of two; the measured margin clears it by ten percent. It matches the `~2.2×` figure the header's own rationale records, independently recomputed here from the declared ranges rather than copied.

**There is no behavioral form of this assertion, and that is a property of the guarantee rather than a gap in the test.** At the volts in question the Nyquist ceiling dominates the frequency completely — a block driven at twenty-nine volts and a block driven at sixty-four volts return **the same samples**. Whether the pitch-volt bound fired is invisible in the output. **That invisibility is exactly why the bound is inaudible in normal use**, and it is exactly why the guarantee has to be stated as arithmetic.

**And it is built from the ranges rather than from a typed-in total on purpose**, so it is *this* assertion that fires if a later phase widens a control's range or lowers the constant — which is the only way T-31-24 becomes real. A typed-in `29.08` would have kept passing while the thing it summarised went stale.

## PITCH-05 — pinned in the one place a narrowing is visible

**Compile time.** `static_assert(std::is_same<decltype(forge::VcoCore::phase), double>::value, ...)` at `tests/test_vco_pitch.cpp:2465`. The rationale is in the source: the band-limiting work that follows this phase places waveform discontinuities at a **sub-sample** position, and the fraction it needs is computed from this accumulator. A float accumulator carries ~24 mantissa bits, so at audio rates it loses the low-order part of every increment and the loss **compounds** over a long block. **A one-word type change would do that silently** — nothing about the pitch, the tracking or the guards would move, and no behavioral case in this suite would notice. Compile time is the only place that regression is visible.

**Source level.** `grep -c 'double deltaPhase = (double)freq * (double)in.sampleTime;' src/dsp/VcoCore.hpp` → **1**. Both casts intact. (31-03 proved the whole region from `const float safeRate` to the closing brace byte-identical against `HEAD~2`; this plan re-asserts the specific line from the test side.)

**Runtime.** 100000 steps one volt above each rate's clamp ceiling — the ceiling fires on every sample, so the increment sits at its bound and the single-subtract wrap runs constantly, which is the hardest condition for the accumulator. Final phase **0.99909167672740296** (44.1 kHz) and **0.99946984462440014** (48 and 96 kHz), `firstBadStep = −1` at all three rates.

## The stand-in mirror — before and after, side by side

`tests/test_vco_core.cpp`'s `DeliberatelyBrokenSharedStateCore` carries a banner promising in capitals that its guard sequence is mirrored from the real core and **MUST BE KEPT IN STEP WITH IT**, and recording that a previous phase made exactly this kind of update when the guards were reordered. 31-03 changed the real core. **That sentence is the obligation this task discharges.**

**What was mirrored:** the volt-domain summation of V/OCT, coarse and the divided fine value; the `fmConnected`-gated FM contribution added into those volts (not multiplied onto a resolved frequency); the D-14 pitch-volt bound against the same `forge::` constant with the same negated comparison first; the single exponential; and the **sanitised rate** ahead of the Nyquist ceiling — which the stand-in had been reading raw as `in.sampleRate` since Phase 30.

**What was NOT touched:** the one deliberate defect is intact — `static double sharedPhase = 0.0;` is still a function-local static, with its shouting-caps callout and its do-not-fix warning verbatim.

| Figure | **Before** (pre-edit capture) | **After** (re-observed) |
|---|---|---|
| 44100 `r.mismatchA` / `r.mismatchB` / total | **512 / 512 / 1024** | **512 / 512 / 1024** |
| 48000 `r.mismatchA` / `r.mismatchB` / total | **512 / 512 / 1024** | **512 / 512 / 1024** |
| 96000 `r.mismatchA` / `r.mismatchB` / total | **512 / 512 / 1024** | **512 / 512 / 1024** |
| whole suite: cases / assertions / failed | **81 / 2,618,053 / 0** | **81 / 2,618,053 / 0** |
| `-tc="*independence positive control*"` | 1 case / 6 assertions | 1 case / 6 assertions |
| `-tc="*two-instance independence*"` | 1 case / 18 assertions | 1 case / 18 assertions |

**Every figure identical. Nothing moved, so nothing had to be reported.** The stop-and-report rule was in force and did not fire — and it is now written into the banner for the next person who has to do this.

**The figures were compared as NUMBERS, never by diffing the raw `-s` output.** Every successful-assertion line carries its own source line number, and this edit shifted the case's `CHECK` from line 1106 to 1158, so a raw diff of the two captures would have differed for reasons that mean nothing. (That trap is recorded in STATE.md §Accumulated Context from Phase 30 and the banner now restates it.)

### Why the addition is inert, stated as a mechanism rather than hoped for

Invariant 5 drives this type through `coreBase()`, which leaves `coarse` and `fine` at zero and leaves all three FM fields at their header defaults **with the jack unpatched**, over `pitchCV` in `[−1, +1]` and `0.5`. So the summation reduces to the pitch volt exactly, **the gated FM term is not evaluated at all**, the pitch-volt bound cannot fire two orders of magnitude inside its own range, and the sanitising ternary returns a legitimate positive rate unchanged. Not one sample can move.

**And that inertness is precisely why the drift would have been invisible.** Had the mirror not been updated, this type would differ from the real core in more than the one field its banner promises — and it would have gone on **passing**, because its own inputs never reach the new arithmetic. A stand-in's divergence from what it claims to mirror is undetectable exactly when it is inert, which is why the obligation has to be discharged by the phase that moved the core rather than found later.

**The mirror also still has exactly ONE exponential** (`grep -c 'forge::exp2_taylor5'` → 1), so the update did not accidentally acquire the multiplicative shape that `tests/test_vco_pitch.cpp`'s *other* stand-in exists to be.

**Diff confinement:** `git diff --stat` shows 56 insertions and 4 deletions, in three hunks at lines 314, 346 and 350 — **all inside the stand-in's banner (259–348) and its struct (349–418)**. The estimator, `coreBase()`, the interleave helper, the other five cases and both hostile timing grids are untouched, and the historical `−21609.00` measurement literal is unchanged.

## Test counts, before and after

| Run | Cases | Assertions | Failed |
|---|---|---|---|
| Pre-plan baseline (31-06 tip, `c7c7303`) | 79 | 2,617,565 | 0 |
| After Task 1 (`d42157e`) | **80** | 2,617,658 | 0 |
| After Task 2 (`7d09ef6`) | **81** | **2,618,053** | 0 |
| After Task 3 (`89a70e1`) | **81** | **2,618,053** | 0 |
| Phase regression floor | 79 | 2,617,565 | 0 |

**+1 case for each of Tasks 1 and 2, and Task 3 moved NEITHER count** — which is exactly what its criterion required and what the inertness argument predicts. No existing case moved at any step; the net addition is 488 assertions across two new cases.

**Every selector's matched case count, stated because a `-tc` filter that matches nothing also exits 0:**

| Invocation | Matched cases | Assertions | Exit |
|---|---|---|---|
| `-tc="*Nyquist*"` | **1** | **93** | 0 |
| `-tc="*hostile pitch*"` | **1** | **395** | 0 |
| `-tc="*independence positive control*"` | 1 | 6 | 0 |
| `-tc="*two-instance independence*"` | 1 | 18 | 0 |
| `-tc="*v/oct tracking*"` (regression spot check) | 2 | 603 | 0 |
| `-tc="*exponential FM*"` (regression spot check) | 2 | 494 | 0 |
| `-tc="*golden*"` (guardrail spot check) | 9 | 49,188 | 0 |

Case-insensitivity was checked **before** naming: doctest's filters are case-insensitive by default, and **no** pre-existing case name in the whole suite contains `nyquist` or `hostile` as a substring, so both new selectors are unambiguous.

## Gate results

| Gate | Required | Observed |
|---|---|---|
| `make test` | exit 0, 0 failed, +1 case for Tasks 1 and 2, unchanged for Task 3 | **81 / 2,618,053 / 0** |
| Compiler warnings from either file | zero under `-Wall -Wextra` | **zero** — clean rebuild after every task |
| `-tc="*Nyquist*"` / `-tc="*hostile pitch*"` | exit 0 **and** exactly 1 case each | **1 / 1** |
| `-tc="*independence positive control*" -s` | figures identical to the pre-edit capture | **512 / 512 / 1024 at all three rates** |
| `-tc="*two-instance independence*"` | still exit 0 | **exit 0, 18 assertions** |
| `make guards` | exit 0, `guard suite: PASS` | **PASS** |
| `make strict` | exit 0, unchanged from 31-04 | **PASS** — `strict C++11 gate: PASS`, same four TUs (this plan touches no `src/` file) |
| Six LFO `.f32` goldens | byte-identical | replayed inside **every** `make test` run above |
| `git diff --name-only` over all 3 commits | the two test files only | **`tests/test_vco_pitch.cpp`, `tests/test_vco_core.cpp`** |

**Mechanical checks from the plan's acceptance criteria, as observed:**

| Check | File | Required | Observed |
|---|---|---|---|
| `grep -c 'forge::kVcoNyquistGuardFrac'` | pitch | ≥ 4 | **6** (was 3) |
| non-comment `grep -cE '2183[0-9]\|2376[0-9]\|4752[0-9]'` | pitch | 0 | **0** |
| `grep -c 'REQUIRE(nUp >= 8)'` | pitch | ≥ 4 | **5** (was **4** — see note 1) |
| `grep -cE 'constexpr double k.*Tolerance'` | pitch | 1 | **1** |
| `grep -c 'kPitchLooseBoundV'` / definitions | pitch | reused, no second definition | **5 references / 1 definition** |
| `grep -c 'forge::kVcoMaxPitchVolts'` | pitch | ≥ 3 | **6** |
| non-comment `grep -c '64.f\|64.0'` | pitch | 0 | **0** |
| `grep -c 'if (!('` | pitch | ≥ 2 | **21** |
| `grep -c 'static_assert'` | pitch | ≥ 1 | **1**, comparing the accumulator member's declared type against `double` |
| `grep -c 'type_traits'` | pitch | 1 | **1** (the include line only — deliberately not named in any comment) |
| `grep -c 'MESSAGE('` | pitch | 0 | **0** |
| BDD macros (`SCENARIO`/`GIVEN`/`WHEN`/`THEN`) | pitch | 0 | **0** |
| `grep -c 'double deltaPhase = (double)freq * (double)in.sampleTime;'` | `src/dsp/VcoCore.hpp` | 1 | **1** |
| `grep -c 'static double sharedPhase = 0.0;'` | core | 1 | **1** |
| `grep -c 'THE DELIBERATE DEFECT, AND THE ONLY ONE'` | core | 1 | **1** |
| `grep -c 'forge::kVcoMaxPitchVolts'` | core | ≥ 1 | **2** |
| `grep -c 'in.fine * (1.f / 12.f)'` | core | 1 | **1** |
| `grep -c 'if (in.fmConnected)'` | core | 1 | **1** |
| `grep -c 'forge::exp2_taylor5'` | core | 1 | **1** |
| `grep -c '21609'` | core | 1 | **1** (historical literal unchanged) |
| `grep -c 'HOSTILE_RATES\|HOSTILE_TIMES'` | core | unchanged | **4 → 4**, and `git diff -U0` shows **zero** changes inside either array |
| `grep -r 'DeliberatelyBrokenSharedStateCore' src/` | — | 0 | **0** |

## Non-vacuity requirements — the tally is now complete

| # | Requirement | Owner | Status |
|---|---|---|---|
| 1 | Measure the OUTPUT, not telemetry, on the primary tier | 31-05 | ✅ closed there |
| 2 | Ground truth from libm in double | 31-05 | ✅ closed there |
| 3 | Expectations one octave apart | 31-05 | ✅ closed there |
| 4 | `REQUIRE(nUp >= 8)` before any tolerance check | 31-05 | ✅ closed there; **this plan added one more**, ahead of every above-ceiling assertion |
| 5 | FM multiplicative negative control | 31-06 | ✅ closed there — 5343 mismatching samples observed |
| 6 | **Clamp-boundary case proving the clamp FIRES** | **this plan** | ✅ **closed — exact equality at nine above-ceiling points, thousands of crossings, a full 5.000 V peak, plus a below-ceiling control** |

**All six of the validation contract's non-vacuity requirements are now implemented and observed.**

## Milestone guardrail compliance

- `git diff --name-only d42157e~1 89a70e1` = **`tests/test_vco_pitch.cpp`** and **`tests/test_vco_core.cpp`**. Two files, three commits, nothing else.
- **No frozen header, no `FROZEN.sha256`, no `src/AnalogLFO.cpp` and no `src/` file at all** in the three-commit diff (filtered grep returns `0`). `make strict` is therefore unchanged from the 31-04 result by construction, over the same four translation units.
- **Neither block driver is in the diff.** `tests/VcoBlockDriver.hpp` is used unchanged, untemplated, unsubclassed and unaliased; the LFO's golden replay driver is neither included nor named.
- **`tests/main.cpp` and `tests/check_includes.sh` unmodified.** Zero build wiring needed.
- **The six LFO `.f32` goldens replay byte-identical** inside every `make test` run above; `-tc="*golden*"` reports 9 cases / 49,188 assertions passing.
- **No deletions in any commit** (`git diff --diff-filter=D --name-only` empty for all three) and **no untracked residue** (`git status --porcelain --untracked-files=all` empty after each).
- **Zero registry packages** (T-31-SC): the only new include is `<type_traits>` from the toolchain.
- **No sanitizer wiring added** (D-24): the plan's banner explicitly forbids it and nothing was added to the `Makefile` or CI.

## Requirements: why PITCH-04 and PITCH-05 are marked complete

**PITCH-04** — *"Frequency is clamped just below Nyquist so extreme pitch/FM/sync never aliases via out-of-range frequency."* The clamp mechanism landed in Phase 30 and its constant was settled in 31-02. What was missing was evidence, and all of it is now in place: the clamp is **observed firing** at exactly its derived ceiling on legitimate high notes at three rates (invariant 8), a below-ceiling control proves it does not fire early, and 78 hostile configurations — covering **extreme pitch** and **extreme FM**, including both bound ends and a non-finite attenuverter — all assert the telemetry frequency at or below the symbolically recomputed ceiling (invariant 9), on top of the pre-existing timing-driven pin. The requirement's `/sync` clause names a control **Phase 33 adds**; the clamp sits downstream of the frequency, so no new pitch source can bypass it, but **Phase 33 must re-confirm the clamp still binds after sync lands** — recorded below as a hand-off rather than claimed here. The "never aliases" clause is scoped by its own qualifier, *"via out-of-range frequency"*: that is what is pinned. Alias **content** belongs to Phase 32 (CORE-02 / AA-01..05) and is not claimed.

**PITCH-05** — *"Phase accumulation uses double precision so high-frequency phase-crossing placement stays accurate for band-limiting."* The accumulator's type is pinned at **compile time**, in the only place a silent one-word narrowing is visible; both `deltaPhase` casts are pinned at the source level; and the accumulator is observed staying in its half-open range over 100000 steps at maximal increment at three rates. The *"for band-limiting"* clause is a statement of **purpose** — the mechanism exists so Phase 32's sub-sample crossing placement can be accurate — and the mechanism is now pinned three ways. Phase 32 owns whether it *uses* it well.

**Phase 31's requirement ledger is now closed:** PITCH-01 and TEST-02 (31-05), PITCH-02, PITCH-03, FM-01, FM-02, FM-03 (31-06), PITCH-04 and PITCH-05 (this plan). All nine.

## Decisions Made

1. **Invariant 8 asserts an EXACT float equality**, and the recomputation is done in **float** through the core's own sanitising ternary. Recomputing in double and narrowing afterwards lands on a different value and turns an exact equality into a coin flip. The equality is a far stronger statement than a bound: it says the ceiling was *applied*, at its *derived* value, and that nothing downstream perturbed it.
2. **The `+3.00 V` above-ceiling offset is not redundant with `+0.25 V`.** At eight times the ceiling frequency before clamping, a loosely-applied ceiling would show. That all three offsets produce byte-identical blocks is the measurement that says the clamp is *hard*.
3. **The below-ceiling control uses a strict `<`, never `<=`.** At-or-below would be satisfied by a clamp that fired there too, which is the whole failure the control exists to exclude.
4. **Invariant 9 asserts no non-constant property anywhere**, and the source states why at the negative pinned point. Adding one would turn D-13's decided low-end behavior red.
5. **Invariant 9 uses three doctest `SUBCASE`s**, so the 78-configuration hostile loop runs once rather than once per section, and the three claims (the grid, the margin, PITCH-05's runtime half) are named separately in the failure output.
6. **The margin assertion is built from four named declared-range terms**, in double, never from a typed-in total — so it is the assertion that fires when a control range widens or the bound drops.
7. **The 26-row grid's collapse onto three outcomes is RECORDED rather than glossed.** Same discipline as 31-06's four marked blind rows: a grid that produces three distinct observations is three facts, and the source says so.
8. **Task 3 also mirrored the sanitised rate**, not only the pitch block. The plan's action listed it, and the stand-in had been reading `in.sampleRate` raw since Phase 30 — so this was a second, older, inert divergence closed in the same edit.
9. **The file banner was updated TWICE, once per task, rather than once at the end.** Task 1's commit says invariants 1–8 are landed and 9 is reserved; Task 2's says all nine are landed. Two edits cost nothing; one commit shipping a banner that calls a landed invariant `RESERVED` is the false-comment class this repo has now retired five times.
10. **The banner spells `src/dsp/RackCompat.hpp:106` / `:109`** rather than describing them. Checked before writing: `check_includes.sh`'s `[2/7]` detector matches `#include` **lines** and scans VCO **headers**, so a prose mention in a test TU cannot answer it — unlike the `BlockDriver.hpp` case 31-06 hit, where a boundary-anchored grep genuinely was in force. Precision was preferable where it was safe.
11. **PITCH-04 and PITCH-05 marked complete**, with the two forward clauses recorded as future-phase re-confirmations.

## Deviations from Plan

### Auto-fixed Issues

**None.** All three tasks landed exactly as specified, and every assertion the plan asked for is asserted. This is the first plan in Phase 31 with no Rule 1/2/3 fix — which is itself worth stating, because the four preceding plans each found a real coverage or comment-truth hole in what they were handed, and the reason this one did not is that 31-05 and 31-06 had already paid down the helper and constant debt (`clampCeilingVolts`, `kPitchLooseBoundV`, the drive helper, the one tolerance) that a plan writing new cases would otherwise have had to invent.

### Out of scope, found and NOT fixed — handed to 31-08

**1. `tests/test_vco_core.cpp`'s `coreBase()` carries two comments that 31-03 falsified.**

```cpp
in.coarse    = 0.f;   // Phase 31 — unread by this step() body
in.fine      = 0.f;   // Phase 31 — unread
```

31-03 made the real core **read** both fields, so both annotations are now false — the same false-comment class plan 30-08 existed to remove and that this phase has already corrected four times elsewhere. **It was deliberately not fixed here:** Task 3's action explicitly forbids touching "its base-input helper", and its acceptance criteria require `git diff --stat tests/test_vco_core.cpp` to show changes **confined to the stand-in struct and its banner**. Fixing it would have violated a stated prohibition to correct a comment nothing asserts against.

**31-08 owns `deferred-items.md` per this phase's artifact list, and this item belongs in it** alongside the shipped-LFO latent UB that 31-03 recorded. It is a two-line comment edit with no behavioral consequence; it is recorded here so it is not rediscovered cold. (Note the *third* field's annotation, `in.drift = 0.f; // Phase 34 — unread`, is still **true** and must be left alone.)

### Verification-command notes (no code impact)

**2. Task 1's criterion `grep -c 'REQUIRE(nUp >= 8)' >= 4` was ALREADY SATISFIED at the pre-plan tip.**

Measured at `c7c7303`, before any edit: **4**, not the `3` that 31-06's own summary table records for the same pattern. So the criterion as written could not distinguish "this plan added a crossing-count precondition" from "this plan added nothing". Observed **5** after Task 1, so the substantive claim — a `REQUIRE(nUp >= 8)` ahead of every above-ceiling assertion in the new case — is true and verified by reading the case, not by the count. **Recorded because a criterion that is green before the work is not evidence of the work**, which is the same structural point this whole phase has been making about vacuous coverage, applied one level up to the criteria themselves.

**3. Task 2's criterion `grep -c 'forge::kVcoMaxPitchVolts' >= 3` returns 6, and its parenthetical undercounts by design.**

The criterion names three expected sites ("the mirror in 31-06's stand-in, the pinned grid points, and the margin assertion"). Observed 6: **two** in 31-06's multiplicative stand-in (the negated bound and the plain bound are separate lines), **two** added by Task 1 (the D-14 headroom `REQUIRE`s that assert invariant 8 stays clear of the *other* bound), and **two** in Task 2 (the two pinned grid rows share one `bound` local, and the margin subcase reads the constant once). The inequality holds comfortably; the parenthetical was a forecast, not a count.

**4. `grep -c 'forge::exp2_taylor5' tests/test_vco_core.cpp` is 1 both before and after Task 3.**

The criterion's purpose — "still exactly one exponential in the stand-in, so the mirror did not accidentally acquire the multiplicative shape" — is a **non-regression** check rather than a change check, and it is the right shape for what it guards: the failure it excludes is an *added* second exponential. Stated because a reader comparing this to 31-06's `grep -c 'forge::exp2_taylor5' == 2` (in the *other* file, where the second one is the deliberate defect) could otherwise think a count moved.

---

**Total deviations:** 0 auto-fixed, 1 out-of-scope finding recorded and deliberately not fixed, 3 verification-command notes.
**Impact on plan:** No scope creep and no weakened assertion. Every claim the plan asked for is asserted, at the figures recorded above.

## Issues Encountered

- **None blocking.**
- **`make test` runtime.** The suite's compile still dominates its execution (31-06 measured 0.82 s of run against 14.31 s of build). This plan added ~700 lines to the pitch TU and ~390 assertions; the run cost is dominated by invariant 9's 312,000 `step(...)` calls plus PITCH-05's 300,000, which together are about an eighth of the primary tracking tier's load. Nothing here is worth trimming for time.
- **doctest's `&&`/`||` limitation never bit.** Every compound condition was written either as separate negated `if` statements accumulating a named bool, or as a plain-C++ expression bound to a `const bool` before the macro. Fourth plan in a row where 31-05's warning worked.
- **Note for later plans (sixth confirmation):** `gsd-tools query state.record-metric` / `state.add-decision` take **named flags**, not the positional arguments the `execute-plan.md` workflow shows. Carried forward from 31-01 through 31-06.

## User Setup Required

None. No external service configuration, and **zero registry packages** — the only new include is `<type_traits>` from the toolchain, and the test target globs `tests/*.cpp` so no build wiring changed.

## Next Phase Readiness

- **Ready for 31-08.** Two concrete hand-offs, both for `deferred-items.md`:
  - **`coreBase()`'s two stale `// Phase 31 — unread` annotations** in `tests/test_vco_core.cpp` (deviation 1 above). A two-line comment edit; leave the third field's `// Phase 34 — unread` alone, it is still true.
  - The **shipped LFO's shared latent UB**, which 31-03 recorded and pointed at no phase (D-24). This plan's invariant 9 banner now also states, in the source, that a permanent repo-wide sanitizer gate is forbidden for that reason — so the constraint is recorded in two places, not one.
- **Ready for 31-09 / the phase gate.** All nine of Phase 31's requirements are marked complete and **all six non-vacuity requirements are closed**. The gate needs: `make test` (81 / 2,618,053 / 0), `make guards` PASS, `make strict` PASS, a real plugin link, the 3-OS CI matrix green **by SHA**, and operator in-Rack UAT after a **full `dist/` flush**.
- **Phase 32 (band-limiting) inherits three things from this plan specifically.**
  - **The compile-time accumulator pin is Phase 32's tripwire, not this phase's.** If anyone narrows `phase` to a float, `tests/test_vco_pitch.cpp` fails to **compile** with a message naming the sub-sample crossing fraction. That is deliberate: no behavioral case in the suite would have caught it.
  - **The clamp's measured behavior at the ceiling is now on record with numbers** — the frequency pins exactly, the peak is a full 5.000 V, `blockMin`/`blockMax` are −5 and +5. When Phase 32 lands band-limiting and the peaks stop flattening the same way, invariant 8's `blockIsNotConstant` and `peakAudible` are the assertions that will speak first, and the recorded figures are what a diff should be read against.
  - **Invariant 8's above-ceiling points are the one place in the suite where the pitch is intentionally wrong.** A morph-robustness or alias-floor pass must not extend that case; it belongs in a separate one, at the same fixed tolerance.
- **Phase 33 (hard sync) has one explicit obligation from this plan.** PITCH-04's requirement text names *"extreme pitch/FM/**sync**"*, and sync does not exist yet. The clamp sits downstream of the frequency so a sync-driven pitch source cannot bypass it structurally — but **Phase 33 must re-confirm it binds** by adding its sync inputs to invariant 9's grid or to its own equivalent, rather than inheriting this plan's green as coverage of an input class that was unreachable when it was written. Recorded here because that is exactly the kind of forward claim this phase has repeatedly declined to make on someone else's behalf.
- **A tripwire nobody should misread.** Rows 2, 13 and 24 of invariant 9's grid land on the **negative** plateau specifically because the core's D-14 bound writes its negated comparison **first**. If those rows ever move to the positive plateau or to a NaN, look at that comparison pair before looking anywhere else — someone has replaced it with the NaN-transparent comparison ladder the source rejects by name.
- **No blockers.**

## Self-Check: PASSED

- `tests/test_vco_pitch.cpp` — FOUND (2713 lines)
- `tests/test_vco_core.cpp` — FOUND (1160 lines)
- `.planning/phases/31-pitch-tuning-exponential-fm/31-07-SUMMARY.md` — FOUND
- Commit `d42157e` — FOUND
- Commit `7d09ef6` — FOUND
- Commit `89a70e1` — FOUND
- No file deletions in any of the three commits (`git diff --diff-filter=D --name-only` empty for each)
- No untracked residue after any commit (`git status --porcelain --untracked-files=all` empty)
- No `src/` file, no frozen header, no `FROZEN.sha256`, no `src/AnalogLFO.cpp`, no block driver and no `tests/main.cpp` in the three-commit diff
- No temporary harvest print survives in either file (`grep -c fprintf` and `grep -c cstdio` both `0` in `tests/test_vco_pitch.cpp`)
- `grep -r 'DeliberatelyBrokenSharedStateCore' src/` → 0 hits
- Every figure in this summary was read out of an actual run's output — the ceiling volts, the telemetry frequencies, the crossing counts, the peak magnitudes, the cents figures, the plateau values, the margin terms, the final accumulator values and both mismatch captures — not computed by hand

---
*Phase: 31-pitch-tuning-exponential-fm*
*Completed: 2026-07-30*
