---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
plan: 08
subsystem: dsp
tags: [vco, band-limiting, output-bound, oracle, test-control, aa-01, aa-03, d-07, d-13, d-18b, p-10, t-30-01, t-32-03, t-32-16, t-32-18, t-32-19, t-32-23]

# Dependency graph
requires:
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-06 — the band-limited forge::VcoCore this plan mirrors into the oracle, and the additive/bipolar correction that falsified the old bound derivation"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-07 — the measure-to-pin protocol and the live TEST-03 spectral gate this plan's invariant 1 now points at"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-01 — the mirror-maintenance obligation shared by DeliberatelyBrokenSharedStateCore and NaiveVcoCoreMirror"
provides:
  - "kHostileBoundV = 10.0 V — the phase-wide OUTER output bound, no exceptions anywhere, binding on every scenario any later plan adds"
  - "kMusicalBoundV = 5.55 V — the additional TIGHTER tier, layered on top, asserted where measurement entitles a scenario"
  - "Scenario five — the P-10 Nyquist-ceiling overlapping-edge worst case, with a pinned 5.65 V exercise floor that sits ABOVE the musical tier"
  - "DeliberatelyBrokenSharedStateCore::blep — the oracle brought in step with the band-limited core for the third time"
  - "Invariant 1's grid provenance resolved from measurement; the 'Phase 32 owns it' forward reference is closed"
affects: [32-09, 32-10, 32-11, 33-hard-sync, 34-output-and-drift]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Nested-not-partitioned tiers: one outer bound with no exceptions, plus a strictly tighter assertion layered on top of it wherever measurement entitles a scenario — so no scenario is ever excused from the outer number"
    - "Exercise floor pinned above the tighter tier: assert the hostile scenario's observed maximum EXCEEDS the musical bound, so the two tiers are provably a real distinction rather than two numbers with the same content"
    - "Saturation disclosure: when a pinned figure does not move because its metric is at its own ceiling, record that as INSENSITIVITY rather than reporting it as evidence of inertness"
    - "Falsified-premise-corrected-in-place, continued from 32-04, 32-05, 32-06 and 32-07: keep the conclusion, replace the premise, and say in the source what was falsified and by what measurement"

key-files:
  created: []
  modified:
    - tests/test_vco_core.cpp

key-decisions:
  - "P-10's stated worst point is FALSIFIED IN BOTH COORDINATES. Its prose puts the 9.198 V row at 'the 5-percent pulse with character NEAR ZERO'. Measured against the shipped core at pitchCV +10, character 0.00 gives EXACTLY 5.000000 V — near the best case, not the worst — and the true peak sits at character 1.00 at 7.150197 V, 2.05 V BELOW the table. The conclusion (an outer tier is needed) is kept; the premise about where is corrected"
  - "Scenario five's grid as the plan specified it would have been VACUOUS. morph {0.90,0.95,1.00} x character {0.00,0.05,0.10,0.20} at pitchCV +10 measures 5.000000 V — it does not exceed even the MUSICAL tier, so the scenario would have asserted nothing about the hostile one. The character axis was extended to 0.50 and 1.00, where the measurement puts the peak: same pitch, same intent, measured coordinates"
  - "The oracle's control figures did NOT move, and the plan's expected mechanism is wrong. They are SATURATED at 512 of 512 — a count pinned at its own metric's ceiling cannot rise. Recorded as insensitivity, not as inertness, because the correction is measurably LIVE on one of the control's two drives"
  - "Band-limiting made invariant 1's narrow-pulse tracking measurement WORSE, not better: -24.53 % to -34.7383 % at pitchCV +3.5, and to NO CROSSINGS AT ALL at +4.0. That is the strongest possible evidence for the corrected premise — the failure is in the estimator's sampling, not in the DSP"
  - "kMusicalBoundV is pinned at the ANALYTIC ceiling of 5.55 V rather than nudged up to a comfortable round number, leaving a tight 0.032 V margin at scenario two. Stated plainly in the source rather than hidden, because a measured envelope sitting just under a derived supremum is exactly the relationship worth pinning"
  - "A fourth stale sentence the plan did not name — the file banner's 'THE PHASE-30 OSCILLATOR ALIASES BY DESIGN' — was corrected in place. Its operative half (no spectral claims in this file) survives with its real reason: this file is the wrong instrument, not that the oscillator aliases"

requirements-completed: [AA-01, AA-03]

coverage:
  - id: D1
    description: "DeliberatelyBrokenSharedStateCore mirrors all three of plan 32-06's changes — the per-instance MorphBlep member, the naive-plus-correction call site, and the NaN-catching morph/character pair — with its ONE deliberate divergence still the function-local static phase accumulator"
    requirement: "AA-01"
    verification:
      - kind: unit
        ref: "grep: 'forge::MorphBlep blep;' = 1, 'blep.step(wave, sharedPhase, p, deltaPhase, morph, character)' = 1, negated morph pair = 1, negated character pair = 1"
        status: pass
      - kind: unit
        ref: "grep -rc 'DeliberatelyBrokenSharedStateCore' src/ finds no match — containment by placement holds"
        status: pass
    human_judgment: false
  - id: D2
    description: "The control's captured figures are RE-OBSERVED against the recorded old ones rather than recomputed, and the stop-and-report rule governs (T-32-19)"
    requirement: "AA-01"
    verification:
      - kind: unit
        ref: "tests/test_vco_core.cpp#vco core: independence positive control — 1 case, 6 assertions, 0 failed; mismatchA := 512, mismatchB := 512, totalMismatch := 1024 at all three rates, unmoved from the 30-08 and 31-07 captures"
        status: pass
      - kind: other
        ref: "out-of-tree probe, correction on against correction off, bit-exact: drive A changes 0 of 512 at every rate; drive B changes 8 / 8 / 4 of 512 with excursions to 2.224924 / 2.078773 / 2.344383 V"
        status: pass
    human_judgment: false
  - id: D3
    description: "The new per-instance blep member does not break voice isolation (CORE-03 / T-32-18)"
    requirement: "AA-01"
    verification:
      - kind: unit
        ref: "tests/test_vco_core.cpp#vco core: two-instance independence — 1 case, 18 assertions, 0 failed; mismatchA := 0 and mismatchB := 0 of 1024 at all three rates, soloEqual := 0 so the distinguishability precondition is intact"
        status: pass
    human_judgment: false
  - id: D4
    description: "The output bound is two measured tiers with written provenance, replacing the single kLooseBoundV (T-32-03)"
    requirement: "AA-03"
    verification:
      - kind: unit
        ref: "grep: kMusicalBoundV = 10 occurrences, kHostileBoundV = 11, live (comment-stripped) kLooseBoundV = 0"
        status: pass
      - kind: unit
        ref: "tests/test_vco_core.cpp#vco core: output magnitude stays inside two measured tiers — 1 case, 399 assertions, 0 failed"
        status: pass
    human_judgment: false
  - id: D5
    description: "The hostile tier is EXERCISED, not decorative: a scenario reproducing the P-10 worst point asserts the observed maximum actually exceeds the musical bound"
    requirement: "AA-03"
    verification:
      - kind: unit
        ref: "scenario five: CHECK(gridMax > kExerciseFloorV) with gridMax := 7.1502 / 7.15028 / 7.15028 against a 5.65 V floor, plus CHECK(kExerciseFloorV > kMusicalBoundV) pinning 5.65 > 5.55 so the ordering cannot silently invert"
        status: pass
    human_judgment: false
  - id: D6
    description: "Scenario two's load-bearing non-vacuity assertion survives re-derivation against the band-limited core"
    requirement: "AA-03"
    verification:
      - kind: unit
        ref: "grep -cE 'CHECK\\(maxAbs > 5\\.1f\\)' = 1; re-measured at 5.518030 V, clearing 5.1 V by 0.418 V"
        status: pass
    human_judgment: false
  - id: D7
    description: "Scenario four's five named assertions are intact and unmerged"
    requirement: "AA-03"
    verification:
      - kind: unit
        ref: "allFinite, phaseInRange, freqNonNegative, freqNyquistBounded all present alongside the two tier assertions; grid measured at 5.000000 V across all 48 configurations"
        status: pass
    human_judgment: false
  - id: D8
    description: "Invariant 1's 'Phase 32 owns it' forward reference is resolved by measurement and corrected in place (T-32-23)"
    requirement: "AA-01"
    verification:
      - kind: unit
        ref: "grep -ciE 'falsified|corrected here' = 6; 'RISING ZERO CROSSINGS' = 2; 'test_vco_spectrum.cpp' = 3; 'NOT the TEST-02 tracking gate' = 3 (label intact); CHECK(relErr < 0.01) = 1 (tolerance unchanged)"
        status: pass
      - kind: other
        ref: "reproduction run: +3.5 measured -34.7383 % against a recorded -24.53 %; +4.0 measured 0 rising crossings, max output -0.426132 V, 0 of 11025 samples at or above zero, against a recorded -46.89 %"
        status: pass
    human_judgment: false
  - id: D9
    description: "The shipped Analog LFO is untouched: no src/ file changed, FROZEN.sha256 unmoved, src/AnalogLFO.cpp absent from all three commits"
    verification:
      - kind: integration
        ref: "make guards PASS, make strict PASS; git show --name-only over cd2ccbd, 8e1c21b and 7088f79 lists only tests/test_vco_core.cpp"
        status: pass
      - kind: unit
        ref: "the six .f32 LFO goldens replay bit-exact inside make test — 93 cases, 0 failed"
        status: pass
    human_judgment: false

# Metrics
duration: 34 min
completed: 2026-08-01
status: complete
---

# Phase 32 Plan 08: Bring The Oracle And The Output Bound In Step Summary

**`tests/test_vco_core.cpp` is back in step with the band-limited core on all three of plan 32-06's changes, and its output bound is now two nested measured tiers instead of one round number — an outer 10.0 V that binds every scenario with no exceptions and a tighter 5.55 V layered on top where measurement entitles it, with a new scenario five proving the outer tier is genuinely exceeded rather than left as headroom nobody reaches. Three plan premises were falsified by measurement and corrected in place; one of them would have made scenario five assert nothing at all.**

## Performance

- **Duration:** ~34 min
- **Completed:** 2026-08-01
- **Tasks:** 3
- **Files:** 0 created, 1 modified (`tests/test_vco_core.cpp`)

## Task Commits

1. **Task 1 — mirror the band-limited path into the `:416` oracle** — `cd2ccbd` (`test`)
2. **Task 2 — re-derive the output bound into two measured tiers, and exercise the hostile one** — `8e1c21b` (`test`)
3. **Task 3 — resolve invariant 1's forward reference by measurement** — `7088f79` (`test`)

## The Oracle's Control Figures, Old And New

| rate | plan 30-08 | plan 31-07 | **plan 32-08** | moved? |
|---|---|---|---|---|
| 44 100 Hz | 512 / 512 / 1024 | 512 / 512 / 1024 | **512 / 512 / 1024** | no |
| 48 000 Hz | 512 / 512 / 1024 | 512 / 512 / 1024 | **512 / 512 / 1024** | no |
| 96 000 Hz | 512 / 512 / 1024 | 512 / 512 / 1024 | **512 / 512 / 1024** | no |

The figures did not move. **The plan's stated expectation for why is wrong, and so is the reason plans 30-08 and 31-07 could give.** Both of those recorded "unchanged, therefore the addition was inert at this control's own inputs". That is not what happened here.

### The correction is INERT on one drive and LIVE on the other

Measured out-of-tree, running the control's own step body with and against the correction term and comparing bit-exactly:

| drive | 44.1 kHz | 48 kHz | 96 kHz |
|---|---|---|---|
| A — morph 0.25, pitchCV swept −1..+1, character 1.0 | **0 of 512**, max \|Δ\| 0.000000 V | **0 of 512**, 0.000000 V | **0 of 512**, 0.000000 V |
| B — pitchCV 0.5 fixed, morph swept 0..1, character 1.0 | **8 of 512**, max \|Δ\| 2.224924 V | **8 of 512**, 2.078773 V | **4 of 512**, 2.344383 V |

- **Drive A is inert for a checkable reason** — the D-03 compact-support factor. At character 1.0 the triangle's rounded corner is about 0.175 wide in phase units, while pitchCV in [−1, +1] puts the increment at roughly 0.003 to 0.012, so `2·dt` is one to two orders of magnitude **narrower** than the softened edge and the factor returns **exactly** zero.
- **Drive B is live**, and its counts are independently confirmed by cycle arithmetic: 512 samples at 369.99 Hz is 4.30 / 3.95 / 1.97 cycles, giving 4 / 4 / 2 wrap edges at 2 samples per edge (D-13 places the second half of every correction on the following sample) — **8 / 8 / 4**, exactly the measured count.

### Why the figures did not move anyway, and what it costs

**They are SATURATED.** `mismatchA` and `mismatchB` count, out of n = 512, the interleaved samples differing from the solo baseline, and both are already at **512 — every single sample**. A count pinned at its own metric's ceiling cannot rise when a second divergence route is added. So "unchanged" here is **insensitivity, not evidence of inertness**, and the banner now says so. These three figures can only ever detect a change that pushes the count **down**; they are not a fine pin. What invariant 5 actually asserts — `totalMismatch > 0` — is unaffected and holds at 1024 of a possible 1024.

The stop-and-report clause was **not** triggered: `totalMismatch` is above zero at every rate.

### Invariant 4, re-run with the new per-instance member

| rate | mismatchA | mismatchB | soloEqual |
|---|---|---|---|
| 44 100 Hz | **0** of 1024 | **0** of 1024 | 0 |
| 48 000 Hz | **0** of 1024 | **0** of 1024 | 0 |
| 96 000 Hz | **0** of 1024 | **0** of 1024 | 0 |

The distinguishability precondition is intact (T-32-18).

## Every Scenario's Measured Maximum, From The Step-One Measure Pass

Measured with `kLooseBoundV` temporarily raised to 100 V so nothing could fire.

| scenario | 44.1 kHz | 48 kHz | 96 kHz | tier asserted |
|---|---|---|---|---|
| one — the harness sweep | 5.438490 | 5.438490 | 5.438290 | hostile **+ musical** |
| two — fixed worst, morph 0 / character 1 / pitchCV 0 | **5.518030** | **5.518030** | **5.518030** | hostile **+ musical** |
| three — hostile V/OCT, pitchCV +10 and +14 | 5.421220 | 5.421220 | 5.421220 | hostile **+ musical** |
| four — hostile timing, 48 configurations | 5.000000 | 5.000000 | 5.000000 | hostile **+ musical** |
| **five — P-10 Nyquist ceiling, NEW** | **7.150197** | **7.150281** | **7.150281** | **hostile only** |

All four pre-existing scenarios measure inside 5.55 V, so all four are entitled to the tighter tier and all four assert it **on top of** the outer one. Scenario five is the only drive that exceeds it — which is precisely why it exists.

**The worst figure measured anywhere by this plan: 7.201301 V**, at pitchCV +6.38 / 44.1 kHz (increment 0.494100), morph 1.00 character 1.00, found by a 101 × 21 morph-by-character sweep at twelve pitch points at all three rates. `kHostileBoundV` clears it by 2.80 V.

## The Two Pinned Tiers

| constant | value | provenance |
|---|---|---|
| `kHostileBoundV` | **10.0 V** | Operator decision 2026-08-01: one outer number, no per-scenario exceptions anywhere, on D-09's reasoning that an exception invites a second. Clears the worst measured figure (7.201301 V) by 2.80 V and sits comfortably inside Rack's ±12 V norm (T-32-03). |
| `kMusicalBoundV` | **5.55 V** | The analytic ceiling `5 × 1.11`, kept because it is a **derived supremum** of the naive path rather than a round number. Measured worst among the four entitled scenarios: 5.518030 V — a margin of **0.032 V**, stated plainly in the source rather than hidden. |

The analytic derivation **survives unchanged** (monotone cubic sine path with range [−1.05, +1.11]; triangle/saw/square/pulse each bounded by 1; the morph crossfade a linear interpolation; the bleed step a convex combination). What changed is that it bounds `morphedWave` **alone**, and since 32-06 the return is `5 × (naive + correction)` with the correction **additive and bipolar** — so the bound had to be measured, and every figure above is.

### Scenario five's exercise floor

- Observed maximum, minimum across rates: **7.150197 V**
- Less a 1.5 V cushion: **5.650197** → pinned at **5.65 V**
- **5.65 > 5.55**, so scenario five proves its observed maximum genuinely **exceeds the musical bound**, and a third assertion `CHECK(kExerciseFloorV > kMusicalBoundV)` pins that ordering so it cannot silently invert.

Without the floor, `kHostileBoundV` would be decoration — which is exactly the criticism scenario two's own banner levels at a sweep-only bound test.

## The Two Re-Measured Narrow-Pulse Tracking Errors

Morph 1.00, character 0.0, 44.1 kHz, the same 250 ms window, the same driver, the same estimator.

| pitchCV | 5 % region | pre-Phase-32 | **NOW (band-limited)** |
|---|---|---|---|
| +2.00 | 2.11 samples | — | +0.0010 % |
| +3.00 | 1.05 samples | — | +0.0031 % |
| **+3.50** | 0.74 samples | **−24.53 %** | **−34.7383 %** |
| **+4.00** | 0.53 samples | **−46.89 %** | **NO CROSSINGS AT ALL** |

**Band-limiting did not fix it and made the measurement worse.** At +4.0 the output never reaches zero — measured max **−0.426132 V**, with **0 of 11025 samples** at or above zero — so `estimateFreqRising` counts `nUp = 0` and returns its negative sentinel; the existing `REQUIRE(nUp >= 8)` fires before the tolerance is ever consulted. At +3.5 the counter resolves only **482 of the 740 true cycles**, which is the whole of the −34.74 % figure.

The premise is falsified, the conclusion is kept (the grid still stops at +2), and the corrected premise names the **estimator**: it counts rising zero crossings, the narrow pulse's positive region falls under about two samples above pitchCV +2, and band-limiting reduces the alias energy that region radiates but cannot restore a zero-crossing counter's grip on a sub-two-sample feature — softening the edges in fact lowers the peak further inside the window, which is why the figures moved the wrong way. High-note behaviour is now asserted in `tests/test_vco_spectrum.cpp` at C7, C8 and C9 (2099, 4188 and 8367 Hz), spectrally rather than by counting crossings.

## Verification Results

| Check | Result |
|-------|--------|
| `make test` | **93 cases / 93 passed / 0 failed**, 2 621 222 assertions |
| `make strict` | `strict C++11 gate: PASS` |
| `make guards` | `guard suite: PASS` (check_frozen + check_includes + check_canary) |
| Doctest case count | **93 at plan start → 93 at plan end** — scenario five is a block inside invariant 2, not a new case |
| Assertion count | 2 621 096 → **2 621 222** (+126: scenario five plus the added musical-tier checks) |
| `-tc="vco core: independence positive control*" -s` | 1 / 1 / 0; `totalMismatch := 1024` above 0 at all three rates |
| `-tc="vco core: two-instance independence*" -s` | 1 / 1 / 0; `mismatchA := 0`, `mismatchB := 0` at all three rates |
| `-tc="vco core: output magnitude*" -s` | 1 / 1 / 0, 399 assertions; `gridMax := 7.1502 / 7.15028 / 7.15028` at `atMorph := 1`, `atChar := 1` |
| `-tc="vco core: naive pitch tracks*" -s` | 1 / 1 / 0, 675 assertions |
| `grep -c 'forge::MorphBlep blep;'` | `1` |
| `grep -c 'blep.step(wave, sharedPhase, p, deltaPhase, morph, character)'` | `1` |
| `grep -cE 'if \(!\(morph > 0\.f\)\)'` / `character` | `1` / `1` |
| `grep -rc 'DeliberatelyBrokenSharedStateCore' src/` | no match — containment holds |
| `grep -c 'kMusicalBoundV'` / `kHostileBoundV` | `10` / `11` (needed ≥ 4 / ≥ 5) |
| live `kLooseBoundV` (comment-stripped) | `0` |
| `grep -cE 'CHECK\(maxAbs > 5\.1f\)'` | `1` |
| `grep -ciE 'falsified\|corrected here'` | `6` |
| `grep -c 'RISING ZERO CROSSINGS'` | `2` |
| `grep -c 'test_vco_spectrum.cpp'` | `3` |
| `grep -c 'NOT the TEST-02 tracking gate'` | `3` — the D-16 label intact |
| `grep -cE 'CHECK\(relErr < 0\.01\)'` | `1` — tolerance unchanged |
| `Approx` in comment-stripped source | `0` |
| `git status --porcelain src/dsp/FROZEN.sha256` | empty |
| `git show --name-only` per commit | only `tests/test_vco_core.cpp`; `src/AnalogLFO.cpp` absent from all three |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] The plan's expected mechanism for the oracle's control figures is wrong, and "unchanged, therefore inert" would have been a FALSE sentence in the source**

- **Found during:** Task 1, at the re-observation step.
- **Issue:** the plan enumerates three outcomes and names the second — "MOVED, WITH A KNOWN MECHANISM: **this is the outcome to expect**, because the correction is a function of the SHARED accumulator" — as expected. The figures did **not** move. But the plan's first outcome is equally unusable: it instructs recording "why the addition was inert at this control's own inputs", and the addition is **not** inert. Measured, it changes 8 / 8 / 4 of 512 samples on the control's drive B with excursions to 2.34 V. Writing either of the plan's two available sentences would have put a false statement in a banner whose entire purpose is to stop this file's stand-in from drifting invisibly.
- **Fix:** the real mechanism was measured and written down: **the figures are SATURATED at 512 of 512**, so they are structurally incapable of rising, and "unchanged" is insensitivity rather than inertness. The banner records both drives' measured figures, the D-03 compact-support reason drive A really is inert, the cycle arithmetic independently confirming drive B's 8 / 8 / 4, and — as a finding rather than a gloss — that these three figures are **not a fine pin** and can only detect a change that pushes the count down.
- **Why this is stronger, not weaker:** the plan's outcome (a) would have claimed a property that is false; this records a narrower property **plus** the measurement showing what the fixture can and cannot see.
- **Files modified:** `tests/test_vco_core.cpp` · **Committed in:** `cd2ccbd`

---

**2. [Rule 1 — Bug] P-10's worst point is falsified in BOTH coordinates, and scenario five as specified would have been VACUOUS**

- **Found during:** Task 2, step one, running the plan's own specified probe grid.
- **Issue:** two compounding problems. **(a)** 32-RESEARCH P-10 attributes its 9.198 V row to "the morph at the 5-percent pulse with **character near zero**". Measured against the shipped core at pitchCV +10 — which pins the increment at exactly 0.495 at every rate, the P-10 row itself — **character 0.00 gives EXACTLY 5.000000 V**, which is near the *best* case, not the worst. The true peak on that increment is at **character 1.00**, and it measures **7.150197 V**, which is also **2.05 V below** the table's 9.198 V. **(b)** Consequently the plan's specified scenario-five grid — morph {0.90, 0.95, 1.00} × character {0.00, 0.05, 0.10, 0.20} — **maxes at 5.000000 V**. Built exactly as written, scenario five would not have exceeded even the **musical** tier, its exercise floor could not have been placed above `kMusicalBoundV`, and the scenario would have asserted **nothing whatsoever** about the hostile tier it exists to exercise — while passing silently.
- **Fix:** the character axis was extended to **0.50 and 1.00**, at the same pitchCV +10 and the same morph set. This is not inventing a scenario to make a bound fire — it is the same increment, the same intent and the same P-10 row, evaluated at the coordinates the oscillator actually peaks at. The plan's own instruction to "decide each scenario's membership FROM step one's figures rather than by assumption" governs, and the falsified premise is recorded in the case banner rather than quietly fixed.
- **The plan's escape hatch was considered and correctly not taken.** It says "If step one's observed maximum does NOT exceed `kMusicalBoundV`, do not invent a scenario to make it — record the finding and state the hostile tier is unexercised." That clause guards against **fabricating** a configuration. Here a genuinely reachable point on the *same* drive exceeds the musical tier by 1.60 V; declaring the tier unexercised while such a point sits one grid column away would have been the weaker and less honest choice.
- **Files modified:** `tests/test_vco_core.cpp` · **Committed in:** `8e1c21b`

---

**3. [Rule 1 — Bug] The plan's characterisation of the +4.0 tracking point is wrong in kind, not just in magnitude**

- **Found during:** Task 3, step one.
- **Issue:** the plan (and the file) frame both recorded points as **tracking errors** — "-24.53 % at +3.5 and -46.89 % at +4.0". Re-measured against the band-limited core, **+4.0 produces no rising zero crossings at all**: maximum output **−0.426132 V**, **0 of 11025 samples** at or above zero. `estimateFreqRising` returns its negative sentinel and `nUp = 0`, so the existing `REQUIRE(nUp >= 8)` fires **before** the tolerance check is reached. Recording that as "a −100 % tracking error" would misdescribe it — the estimator is not measuring the frequency badly, it is reporting that it cannot measure at all.
- **Fix:** recorded as measured, in its own terms, with the sample-census figure that establishes it. Two extra reproduction points (+2.0 and +3.0) were measured and recorded as well, because they show the cliff is **not** simply "under two samples": +3.0 sits at 1.05 samples and still tracks to +0.0031 %. The grid's stop at +2 is therefore **conservative** rather than the exact edge, and the source now says so instead of implying a sharp threshold.
- **A second, stronger reading this produced:** band-limiting moved +3.5 from −24.53 % to **−34.7383 %** — *worse*. That is the clearest possible evidence for the corrected premise, and it is stated as such: softening the pulse's edges lowers its peak further inside an already-marginal window, so the estimator's grip degrades. A plan that had assumed improvement would have recorded the opposite of the truth.
- **Files modified:** `tests/test_vco_core.cpp` · **Committed in:** `7088f79`

---

**4. [Rule 2 — Missing Critical] A FOURTH falsified sentence, in the file banner, that the plan did not name**

- **Found during:** Task 3, reading the file's opening banner while resolving the invariant-1 forward reference.
- **Issue:** the banner still opened "**THE PHASE-30 OSCILLATOR ALIASES BY DESIGN.** step() is a naive, deliberately unband-limited morphed oscillator (D-12) … Phase 32 (CORE-02 / AA-01..05) **owns** band-limiting". Both sentences became false when plan 32-06's call-site change landed. This is the identical failure 32-06 found and fixed in `src/dsp/VcoCore.hpp`'s own banner, in the identical shape, one file over — and leaving a test file's self-description contradicting the code it drives is exactly what the house rule about falsified premises exists to prevent. The plan's silence about the banner is not permission to leave it wrong.
- **Fix:** corrected in place in the same voice, recording what the sentence **used to** say. The paragraph's **operative half survives with its real reason**: no assertion in this file may make a spectral claim — not because the oscillator aliases, but because this file is the **wrong instrument**, and spectral claims belong to `tests/test_vco_spectrum.cpp`.
- **Files modified:** `tests/test_vco_core.cpp` · **Committed in:** `7088f79`

---

**Total deviations:** 4 auto-fixed — 3 plan/research premises falsified by measurement and corrected in place with their old values recorded, 1 stale sentence the plan did not name. **One of them (deviation 2) would have shipped a scenario that passed while asserting nothing.** No `src/` file was touched, no assertion was softened, no threshold or tolerance was loosened, no guard was weakened, and `src/AnalogLFO.cpp` is absent from all three commits.

## Findings Recorded for Later Plans

- **`kHostileBoundV` binds plan 32-09's audio-rate MORPH sweep with no exception.** That is the operator's decision as landed and the outer tier has no carve-outs. For scale, 32-RESEARCH P-13 measured the audio-rate MORPH grid at `max|out| ≤ 1.3171` in waveshaper units — about 6.59 V — so it should clear 10.0 V comfortably but will **not** clear the 5.55 V musical tier. Plan 32-09 should assert the outer tier only, and say so in its INFO label with its own measured figure, exactly as scenario five does.
- **The measured envelope peak is at morph 1.00 / character 1.00, not near character 0.** This contradicts 32-RESEARCH P-10's prose and is worth carrying into Phase 34's output-conditioning design: the worst-case excursion the output stage must handle sits at **full character on the narrow pulse at the guarded ceiling**, measured at **7.201301 V**, not at the low-character pulse.
- **Phase 34 (output conditioning) inherits a tighter musical figure than 32-06 recorded.** 32-06's summary offered 5.518014 V at C8/44.1 kHz; this plan's four-scenario measure pass puts the musical worst at **5.518030 V** and the analytic ceiling at 5.55 V, with a margin of only **0.032 V**. If Phase 34 changes anything upstream of the return, that margin is the first thing to re-measure.
- **The oracle's 512 / 512 / 1024 figures are a SATURATED metric and should not be treated as a sensitive pin.** Any future plan mirroring a change into `DeliberatelyBrokenSharedStateCore` should measure the change's effect on the control's output **directly**, as this plan did out-of-tree, rather than inferring inertness from figures that cannot move upward.
- **Invariant 1's grid could arguably extend to pitchCV +3** (measured +0.0031 % at 1.05 samples of pulse width), but was deliberately left at +2. The margin at +3 is one sample of pulse width, and the estimator collapses entirely by +3.5; a grid point that close to a cliff would be brittle across toolchains rather than informative.
- **The 32-05 missed-edge caveat did not bite anywhere in this plan.** No measurement showed the signature. The `dt = 0.0005` case is not on this suite's rate set.

## Known Stubs

None. Every line this plan added is asserted by a case that runs on every invocation. There is no placeholder, no skipped case, no `TODO` and no flag. Scenario five is a live block inside invariant 2 with three assertions of its own, and the case count is unchanged at 93 because it was added inside an existing case rather than beside it.

## Threat Flags

None — no network, auth, file-access or schema surface was introduced, and no `src/` file was modified. Every threat-register entry assigned to this plan is mitigated by a named artefact:

| Threat | Mitigation as landed |
|--------|----------------------|
| **T-32-03** (out-of-range output damaging downstream gear) | Two measured tiers: the musical tier **tightens** from 6.0 V to 5.55 V, and the hostile tier is pinned at 10.0 V against a worst measured figure of 7.201301 V, comfortably inside Rack's ±12 V norm. Scenario five's 5.65 V exercise floor stops the outer tier being headroom nobody reaches. |
| **T-32-16** (a full-amplitude spike invisible to the spectral gate) | The bound's second job is now recorded in the case banner by name: it is the only assertion in the suite that can see a merged square site or a double-sourced side decision, both of which measure 0.0 dB spectrally at about ±9.78 V. Both fall inside 10.0 V's reach and outside 5.55 V's. |
| **T-32-19** (the positive control drifting from the real core) | All three of plan 32-06's changes mirrored; containment re-asserted by grep over `src/`; figures re-observed rather than recomputed, with the saturation limitation disclosed instead of being reported as inertness. |
| **T-32-18** (the new per-instance `blep` breaking CORE-03) | Invariant 4 re-run with the member live: **0 of 1024** on both instances at all three rates, `soloEqual := 0` so the distinguishability precondition holds. |
| **T-32-23** (a stale forward reference claiming a later phase owns a resolved issue) | Invariant 1's "Phase 32 owns it" resolved by a reproduction run, conclusion kept and premise replaced, plus a fourth stale sentence in the file banner corrected in place. |
| **T-32-12** (the shipped Analog LFO's golden bit-stability) | No `src/` file touched, `FROZEN.sha256` unmoved, `src/AnalogLFO.cpp` absent from all three commits, and the six `.f32` goldens replay bit-exact on every `make test`. |
| **T-32-SC** (package installs) | Zero packages installed in any ecosystem. |

## Issues Encountered

None beyond the four deviations above. `make test`, `make strict` and `make guards` were green at **every** commit — unlike 32-06 and 32-07, this plan opened no red window, because every change it makes is inside an existing case and no tombstone was in flight.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

**Ready for 32-09.**

The oracle mirrors the band-limited core in everything but its one deliberate defect. The output bound is two measured tiers with written provenance, the tighter one is *tighter* than the number it replaced, and the outer one is provably exercised by a scenario that reproduces the P-10 worst point at the coordinates measurement puts it rather than the ones the research text asserted. Invariant 1's forward reference is closed. Plan 32-09 inherits `kHostileBoundV` as a binding outer bound on its audio-rate MORPH sweep with no exception available to it, and the note above tells it which tier its own measurement will support. Nothing in the shipped LFO moved.

## Self-Check: PASSED

- `tests/test_vco_core.cpp` — FOUND on disk; `forge::MorphBlep blep;` × 1, the mirrored `blep.step(...)` call × 1, both negated guard pairs × 1 each, `kMusicalBoundV` × 10, `kHostileBoundV` × 11, live `kLooseBoundV` × 0, `CHECK(maxAbs > 5.1f)` × 1, `RISING ZERO CROSSINGS` × 2, `test_vco_spectrum.cpp` × 3, `NOT the TEST-02 tracking gate` × 3, `CHECK(relErr < 0.01)` × 1, `Approx` × 0 in comment-stripped source
- `.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/32-08-SUMMARY.md` — FOUND on disk
- Commits `cd2ccbd`, `8e1c21b`, `7088f79` — all three FOUND in `git log`, each listing only `tests/test_vco_core.cpp`
- All plan `<success_criteria>` re-run and green; all three tasks' `<acceptance_criteria>` re-run and green, with the documented exceptions in deviations 1–3 (the oracle's expected-to-move figures, P-10's stated worst coordinates, and the +4.0 point's kind) — each falsified by measurement, corrected in place, and recorded with both the old and the new values
- `make test` 93/93/0, `make strict` PASS, `make guards` PASS re-run at the final commit

---
*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Completed: 2026-08-01*
