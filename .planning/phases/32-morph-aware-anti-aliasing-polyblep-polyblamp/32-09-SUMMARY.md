---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
plan: 09
subsystem: dsp
tags: [vco, morph, hostile-timing, band-limiting, output-bound, d-13, d-15, d-16, p-13, p-14, morph-01, morph-02, aa-05, t-32-02, t-32-23, t-32-24, t-32-25]

# Dependency graph
requires:
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-08 — kHostileBoundV / kMusicalBoundV as two nested measured tiers, and the note by name telling this plan which tier its measurement would support"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-05 — the MorphBlep divisor guards (both bounds negated) and the hostile-input findings"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-06 — the band-limited forge::VcoCore::step and its core-side morph/character guard"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-02 — the MORPH CV jack whose audio-rate drive invariant 6 is the headless counterpart of"
provides:
  - "Scenario four extended from 48 to 176 configurations, covering +/-infinity, subnormal and very-large-finite on BOTH timing fields, each entry carrying its own reason"
  - "The measured MorphBlep guard-reachability census from the forge::VcoCore call site: 140 reach the guard, 36 pass it, the upper bound fires 0 times and provably cannot"
  - "Invariant 6 — audio-rate MORPH across 27 configurations, non-vacuity asserted first, outer tier asserted, modulated excess pinned"
  - "kHostileBoundV / kMusicalBoundV as anonymous-namespace constants — one definition read by two cases"
  - "The measured static-vs-swept split at C9/44.1 kHz: 5.518032 V static, 6.289864 V swept, 0.771832 V of modulated excess"
affects: [32-10, 32-11, 33-hard-sync, 34-output-and-drift]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Test what the CALL SITE can actually reach, and say so: a guard proved unreachable from the driving path is documented as unreachable rather than claimed as covered"
    - "Non-vacuity preconditions REQUIREd before any value assertion, counted and CAPTUREd, so a modulation case that stopped modulating fails loudly instead of passing quietly"
    - "Withholding the tighter tier is itself asserted: CHECK(gridWorstV > kMusicalBoundV) turns an exemption into an examined claim"
    - "Falsified-premise-corrected-in-place, continued from 32-04 through 32-08: keep the conclusion, replace the premise, record what was falsified and by what measurement"
    - "One definition for a bound that binds two cases — hoist rather than hand-mirror, because this suite has already watched a hand-kept mirror drift"

key-files:
  created: []
  modified:
    - tests/test_vco_core.cpp

key-decisions:
  - "The plan's Task 1 behavior claim is FALSIFIED in two of its three named classes. A POSITIVE INFINITE sampleTime does NOT reach MorphBlep's dt guard when the rate is legitimate — forge::VcoCore's kVcoMaxDeltaPhase ceiling clamps deltaPhase to an ordinary 0.5. A POSITIVE SUBNORMAL sampleTime does not reach it either; it passes both bounds and is stopped by the NEXT guard down, the crossing gate, where d/dt overflows the float to +infinity"
  - "The MorphBlep upper bound !(dt <= 1.f) fires 0 times in 176 configurations and provably cannot fire from this call site, exactly as 32-05 said. The grid tests what it can reach and records the unreachability rather than asserting a guard it cannot touch"
  - "morph = 1.00 is NOT a crossable segment boundary. The frozen path clamps (int)(morph*4) at 3, so 1.00 is the top ENDPOINT of segment 3; the plan's four-boundary crossing assertion would have been RED on correct behavior at every configuration. Replaced with a strictly stronger occupancy-plus-endpoint precondition"
  - "The audio-rate MORPH excess over the musical tier is REAL but NARROW: only 2 of 27 configurations exceed 5.55 V, both C9 at 2000 Hz, and at 96 kHz the excess vanishes entirely. The assertion is therefore grid-wide; a per-rate form would be RED at 96 kHz on correct behavior"
  - "No cushioned exercise floor is pinned for invariant 6. Scenario five clears the musical tier by 1.60 V and affords a 1.5 V cushion; this grid clears it by 0.74 V, so any cushion large enough to matter would sit below kMusicalBoundV and assert nothing"
  - "A stale per-configuration figure in scenario four was corrected while re-measuring: 42 of the original 48 sit at exactly 5.000000 V and SIX are strictly below because the oscillator genuinely runs there. The grid MAXIMUM was right; the per-cell claim was not"

requirements-completed: [MORPH-01, MORPH-02, AA-05]

coverage:
  - id: D1
    description: "The hostile-timing grid covers infinite, subnormal and very-large-finite timing on BOTH fields, each entry carrying its own inline reason, with the five named assertions intact and unmerged"
    requirement: "AA-05"
    verification:
      - kind: unit
        ref: "tests/test_vco_core.cpp#vco core: output magnitude stays inside two measured tiers (D-18b) — 1 case, 1167 assertions, 0 failed; 176 configurations x 6 assertions = 1056 from scenario four alone, firstBadStep := -1 at all 1056"
        status: pass
      - kind: unit
        ref: "grep: infinity() = 4, denorm_min() = 3, 1e30f = 2; HOSTILE_RATES 8 entries, HOSTILE_TIMES 11 entries"
        status: pass
    human_judgment: false
  - id: D2
    description: "The falsified D-15 rationale is corrected IN PLACE where it is used, naming what was falsified and by what (T-32-23)"
    requirement: "AA-05"
    verification:
      - kind: unit
        ref: "tests/test_vco_core.cpp:1097-1152 — 'THE FALSIFIED PREMISE THAT POINTED THIS WORK HERE, CORRECTED IN PLACE'; grep -c 'AA-05 forbids oversampling' = 1"
        status: pass
    human_judgment: false
  - id: D3
    description: "The MorphBlep guard-reachability claim is MEASURED from this call site rather than assumed, and the unreachable upper bound is recorded as unreachable (T-32-02 / P-14)"
    requirement: "AA-05"
    verification:
      - kind: other
        ref: "out-of-tree probe over all 176 configurations: guard census LOWER=140, UPPER=0, passes=36; direct MorphBlep probe at dt = 3.66616e-43 gives guardFired 0/4096 and nonzeroCorrection 0/4096, at dt = 0.5 gives guardFired 0/4096 and nonzeroCorrection 4096/4096"
        status: pass
    human_judgment: false
  - id: D4
    description: "Audio-rate MORPH through every segment boundary is asserted finite and inside the outer tier across 27 configurations (MORPH-01 / MORPH-02 / T-32-24)"
    requirement: "MORPH-02"
    verification:
      - kind: unit
        ref: "tests/test_vco_core.cpp#vco core: audio-rate MORPH sweeping through every segment boundary stays finite and bounded (MORPH-01 / MORPH-02) — 1 case, 326 assertions, 0 failed; 27 per-configuration maxima from 5.508759 to 6.289864 V, all <= kHostileBoundV"
        status: pass
    human_judgment: false
  - id: D5
    description: "The modulation is proven non-vacuous BEFORE any value is asserted (T-32-25)"
    requirement: "MORPH-01"
    verification:
      - kind: unit
        ref: "REQUIRE(boundaryCrossings >= 4) with measured counts 61 / 124 / 136 / 625 / 1249 / 1360 / 2500 / 4999 / 5441; plus all four segments occupied, both endpoints reached, block non-constant and 100 % non-zero at all 27"
        status: pass
    human_judgment: false
  - id: D6
    description: "The modulated excess over the static-input envelope is pinned and explained rather than absorbed (D-13)"
    requirement: "MORPH-02"
    verification:
      - kind: unit
        ref: "CHECK(gridWorstV > kMusicalBoundV) with gridWorstV := 6.28986 at atRate := 44100, atPitchCV := 5, atModHz := 2000"
        status: pass
      - kind: other
        ref: "out-of-tree split at the worst configuration: swept 6.289864 V vs a 201-point static-morph scan worst of 5.518032 V at the same note and rate; peak sample at morph 0.7485, on the 0.75 square-to-pulse boundary"
        status: pass
    human_judgment: false
  - id: D7
    description: "The shipped Analog LFO is untouched: no src/ file changed, FROZEN.sha256 unmoved, src/AnalogLFO.cpp absent from both commits"
    verification:
      - kind: integration
        ref: "make guards PASS, make strict PASS; git show --name-only over aab56f7 and d5aeb31 lists only tests/test_vco_core.cpp; git status --porcelain src/dsp/FROZEN.sha256 empty"
        status: pass
      - kind: unit
        ref: "the six .f32 LFO goldens replay bit-exact inside make test — 94 cases, 0 failed"
        status: pass
    human_judgment: false

# Metrics
duration: 22 min
completed: 2026-08-01
status: complete
---

# Phase 32 Plan 09: Extend The Hostile-Timing Grid And Add The Audio-Rate MORPH Invariant Summary

**Scenario four's hostile-timing grid grew from 48 configurations to 176 on a corrected rationale — the premise that pointed the work here (Phase 32's "oversampled inner loop") is falsified by AA-05's own wording, and the surviving reason is that this phase put a division by `dt` behind `sampleTime` — and a new invariant 6 bounds audio-rate MORPH across 27 configurations with its non-vacuity proven first and its measured excess over the static-input envelope pinned rather than hidden. Three further premises were falsified by measurement and corrected in place; one of them would have made an assertion red on correct behavior, and two of them were the plan's own.**

## Performance

- **Duration:** ~22 min
- **Completed:** 2026-08-01
- **Tasks:** 2
- **Files:** 0 created, 1 modified (`tests/test_vco_core.cpp`)

## Task Commits

1. **Task 1 — extend the hostile-timing grid on the corrected D-15 rationale** — `aab56f7` (`test`)
2. **Task 2 — invariant 6, audio-rate MORPH through every segment boundary** — `d5aeb31` (`test`)

## Scenario Four: 48 Configurations To 176

| | before | after |
|---|---|---|
| `HOSTILE_RATES` | 4 entries | **8** |
| `HOSTILE_TIMES` | 6 entries | **11** |
| `HOSTILE_PITCH_T4` | 2 entries | 2 (deliberately not widened) |
| configurations | 48 | **176** |
| assertions from this scenario | 288 | **1056** (6 per configuration) |
| `nHostile` | 20000 | 20000 (unchanged) |
| grid maximum `max\|out\|` | 5.000000 V | **5.000000 V — unmoved** |
| `firstBadStep` | `-1` throughout | **`-1` at all 1056** |

The four new classes on each field are `+infinity`, `-infinity`, the smallest positive subnormal float, and `1e30f`; `HOSTILE_TIMES` additionally gains the **negative** subnormal. The suite's whole-file assertion count moved 2 621 222 -> 2 621 990, exactly **+768 = 128 new configurations x 6**.

### The exact source line where the falsified premise was replaced

`tests/test_vco_core.cpp:1097`, opening `THE FALSIFIED PREMISE THAT POINTED THIS WORK HERE, CORRECTED IN PLACE (plan 32-09, D-15)`, running to `:1152` and sitting directly above `HOSTILE_TIMES` at `:1153`. The sentence it replaces described the decoupled entries as "the shape Phase 32's **oversampled inner loop** will produce naturally". AA-05's own wording is *"no minBLEP, no oversampling in v2.0"*, so no such loop exists in this phase and none will be added. The entries all stay; the reason changed.

## The Guard-Reachability Census, MEASURED Rather Than Assumed

The plan asserted that "a plus-or-minus infinite or subnormal `sampleTime` reaches `forge::MorphBlep::step`'s `dt` guard and returns the drained accumulator rather than dividing". **Measured from this call site, that is true for one of the three classes and false for the other two.**

| class on `sampleTime` | what actually happens | reaches the guard? |
|---|---|---|
| zero, negative, negative subnormal, `-infinity`, NaN | `forge::VcoCore`'s own `!(deltaPhase > 0.0)` floor has already driven `dt` to `0.0` | **yes** — 140 of 176 |
| `+infinity`, legitimate rate | `deltaPhase = freq * inf` is `+infinity`, which the core's **own** `kVcoMaxDeltaPhase` ceiling clamps to **0.5** — an entirely ordinary value to the band-limiter, correcting on **4096 of 4096** samples in a direct probe | **no** |
| `+infinity`, hostile rate | `freq` is already 0, so `0 * inf` is a NaN and the floor catches it | yes |
| **positive subnormal** | clears **both** bounds: `dt = 3.66616e-43` (pitchCV 0) and `3.05896e-41` (pitchCV +10). Stopped by the **next** guard down — `d / dt` overflows the float to `+infinity`, `!(s <= 1.f)` fires, and **0 of 4096** samples receive any correction | **no** |

**The guard's upper bound `!(dt <= 1.f)` fires 0 times in 176 configurations**, and provably cannot fire from this call site: `forge::VcoCore` clamps at `kVcoMaxDeltaPhase = 0.5`, a full factor of two below it. Plan 32-05 reached that bound through `MorphBlep`'s own unit tests, not through the core. That is recorded in the source as an unreachability rather than papered over — per 32-05's note to this plan by name.

Direct census of `morphBlepCharFactor(w = 0, dt)` across the hostile classes, for the record: `0 -> 0`, `-0 -> 0`, `denorm_min -> 1`, `-denorm_min -> 0`, `+inf -> 0`, `-inf -> 0`, `NaN -> 0`, `1e30 -> 0`, `0.5 -> 1`.

## Invariant 6: The 27 Audio-Rate MORPH Configurations

Three sample rates x three notes derived from `forge::kVcoFreqC4` (pitchCV +3/+4/+5 -> **2093.00 / 4186.01 / 8372.02 Hz**, i.e. C7/C8/C9) x three morph modulation rates (50 / 500 / 2000 Hz), character 1.0, morph a full-range sinusoid, 20000 samples each.

| rate | note | 50 Hz | 500 Hz | 2000 Hz |
|---|---|---|---|---|
| 44 100 | C7 | 5.516624 | 5.514303 | 5.517566 |
| 44 100 | C8 | 5.516128 | 5.517966 | 5.514354 |
| 44 100 | C9 | 5.515751 | 5.516174 | **6.289864** |
| 48 000 | C7 | 5.515609 | 5.518027 | 5.516409 |
| 48 000 | C8 | 5.514686 | 5.516407 | 5.518031 |
| 48 000 | C9 | 5.517793 | 5.517908 | **6.006541** |
| 96 000 | C7 | 5.514891 | 5.517333 | 5.517462 |
| 96 000 | C8 | 5.515077 | 5.517800 | 5.508759 |
| 96 000 | C9 | 5.517523 | 5.515464 | 5.517825 |

**Grid-wide worst: 6.289864 V** at 44.1 kHz / C9 / 2000 Hz. All 27 clear `kHostileBoundV = 10.0 V` by at least 3.71 V. **Only two of the 27 exceed `kMusicalBoundV`**, both C9 at 2000 Hz.

### Boundary-crossing counts (the non-vacuity precondition)

| morph rate | 44.1 kHz | 48 kHz | 96 kHz |
|---|---|---|---|
| 50 Hz | 136 | 124 | **61** |
| 500 Hz | 1360 | 1249 | 625 |
| 2000 Hz | **5441** | 4999 | 2500 |

The asserted floor is 4 — two orders of magnitude below the slowest configuration, because it is a **non-vacuity** floor, not a characterisation. All four segments are occupied and both endpoints are reached at every configuration; every block is non-constant and **100.00 % non-zero**.

### The excess, and where it comes from

| quantity | measured |
|---|---|
| grid-wide worst, **swept** morph | **6.289864 V** (1.257973 waveshaper units) |
| same note and rate, morph held **static**, scanned over 201 points of the morph axis | **5.518032 V** |
| **the modulated excess** | **0.771832 V** |
| naive path's own contribution at the worst run | 5.517806 V |
| morph value at the peak sample | **0.7485** (44.1 kHz), **0.7500** (48 kHz) |
| 32-RESEARCH P-13's envelope for this grid | `<= 1.3171` units = 6.5855 V, worst at 8372 Hz / 2 kHz |

The excess is produced by the **modulation**, not by the note: the identical oscillator at the identical note and rate stays inside the musical tier at **every** static morph value, and only moves outside it when morph is swept. The peak sample lands **on the 0.75 boundary** — the square-to-pulse switch, where the frozen path stops crossfading two rectangles and takes the direct-duty special case. That is D-13's design as landed: the `pending` accumulator deliberately delivers the second half of a correction computed with the **previous** sample's weight vector, site position and phase increment, because recomputing at the next sample would use a moved jump magnitude, a moved site position and a moved `dt`. This run's 1.257973 units sits inside P-13's 1.3171, at the same worst coordinates.

## Verification Results

| Check | Result |
|-------|--------|
| `make test` | **94 cases / 94 passed / 0 failed**, 2 622 316 assertions |
| `make strict` | `strict C++11 gate: PASS` |
| `make guards` | `guard suite: PASS` (check_frozen + check_includes + check_canary) |
| Doctest case count | **93 at plan start -> 93 after Task 1 -> 94 after Task 2** (invariant 6 is a new case; scenario four's extension is inside an existing one) |
| Assertion count | 2 621 222 -> 2 621 990 (**+768**, Task 1) -> 2 622 316 (**+326**, Task 2) |
| `-tc="vco core: output magnitude*" -s` | 1 / 1 / 0, **1167** assertions; `firstBadStep := -1` at all 1056 scenario-four assertions |
| scenario four `maxAbs` census in `-s` | 146 configurations at `5`, 23 at `1.77018`, 3 at `4.99791`, 3 at `3.38944`, 1 at `2.39022` |
| `-tc="vco core: audio-rate MORPH*" -s` | 1 / 1 / 0, **326** assertions; `gridWorstV := 6.28986` at `atRate := 44100`, `atPitchCV := 5`, `atModHz := 2000` |
| `grep -c 'infinity()'` | `4` (needed >= 2) |
| `grep -c 'denorm_min()'` | `3` (needed >= 2) |
| `grep -c '1e30f'` | `2` (needed >= 2) |
| `grep -c 'AA-05 forbids oversampling'` | `1` (needed >= 1) |
| `HOSTILE_RATES` / `HOSTILE_TIMES` entries | `8` / `11`, each with an inline reason |
| file banner invariant list | **six** entries; 1-5 unrenumbered |
| `Approx` in comment-stripped source | `0` |
| `git status --porcelain src/dsp/FROZEN.sha256` | empty |
| `git diff --name-only` / `git show --name-only` per commit | only `tests/test_vco_core.cpp`; `src/AnalogLFO.cpp` absent from both |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] Task 1's behavior claim about which classes reach `MorphBlep`'s `dt` guard is FALSIFIED in two of its three named classes**

- **Found during:** Task 1, at the measure-first step, before a line of the grid was written.
- **Issue:** the plan states "a plus-or-minus infinite or subnormal `sampleTime` reaches `forge::MorphBlep::step`'s `dt` guard and returns the drained accumulator rather than dividing". Measured through `forge::VcoCore` — which is the only path this scenario has — **a positive infinite `sampleTime` does not reach it at all** when the rate is legitimate: `deltaPhase = freq * inf` is `+infinity`, and the core's own `kVcoMaxDeltaPhase` ceiling clamps it to **0.5**, a value the band-limiter treats as entirely ordinary and corrects on 4096 of 4096 samples. **A positive subnormal `sampleTime` does not reach it either**: at `sampleRate 44100` it yields `dt = 3.66616e-43`, which clears both `dt > 0.f` and `dt <= 1.f`. Writing the plan's sentence into the source would have put a claim in a banner that the code contradicts.
- **Fix:** the census was measured over all 176 configurations plus a direct four-class probe of `MorphBlep::step`, and the result is written into the source as measured: 140 configurations reach the guard, 36 pass it, the positive-subnormal class is stopped by the **next** guard down (`d / dt` overflows the float to `+infinity` and `!(s <= 1.f)` fires, with 0 of 4096 samples corrected), and the guard's upper bound fires **0 of 176** times and provably cannot fire from this call site.
- **Why this is stronger, not weaker:** it names the guard that actually does the work in each case, and it follows 32-05's instruction to this plan by name — test what the call site can reach, and say so, rather than asserting a guard the grid cannot touch.
- **Files modified:** `tests/test_vco_core.cpp` · **Committed in:** `aab56f7`

---

**2. [Rule 1 — Bug] Scenario four's recorded per-configuration figure is wrong; the grid maximum is right**

- **Found during:** Task 1, re-measuring the pre-existing 48 before extending them.
- **Issue:** the source said "MEASURED across all 48 configurations at 5.000000 V exactly — the frozen-phase DC value the guards produce when a hostile rate or time zeroes the increment". Measured, **42** of the 48 sit at exactly 5.000000 V; the other **six** are strictly below it because the oscillator genuinely **runs** there — all six at `sampleRate 44100`, at increments 0.005933 / 0.495 / 0.261626 / 0.5 / 0.5 / 0.5, measuring 4.997915 / 2.390221 / 3.389438 / 1.770179 / 1.770179 / 1.770179 V. The **maximum** was right; the per-cell claim was not, and the frozen-phase explanation describes the majority of the grid rather than all of it.
- **Fix:** corrected in place with both censuses recorded — 42 of 48 before, 146 of 176 after, with only four distinct values beneath the maximum. The doctest `-s` output independently confirms the split (876 assertions at `maxAbs := 5`, i.e. 146 x 6).
- **Files modified:** `tests/test_vco_core.cpp` · **Committed in:** `aab56f7`

---

**3. [Rule 1 — Bug] Task 2's segment-boundary premise would have made the assertion RED on correct behavior**

- **Found during:** Task 2, at the measure-first step.
- **Issue:** the plan specifies `REQUIRE`ing that morph "crosses all four segment boundaries at 0.25, 0.50, 0.75 and **1.00**". There are only **three** crossable boundaries. The frozen `forge::Waveshape::morphedWave` computes `segment = (int)(morph * 4)` and clamps `segment > 3` to 3 (`Waveshape.hpp:165-166`, mirrored in `MorphBlep.hpp`), so **morph = 1.00 is the top endpoint of segment 3**, not a division between two segments — a full-range sinusoid reaches it and turns around without crossing it. Measured, `maxMorph` is 0.999997 at 44.1 kHz and exactly 1.000000 at 48 and 96 kHz, and the segment index never becomes 4 at any configuration. An assertion counting a crossing at 1.00 would have been **red at all 27** on correct behavior.
- **Fix:** crossings are counted over the three real interior boundaries (measured 61 to 5441), and the 1.00 claim is replaced by a **strictly stronger** pair: all four segments must be **occupied**, and the sweep must reach both endpoints (`minMorph < 0.01`, `maxMorph > 0.99`). Together those prove more than the four-crossing claim did — they pin that the sweep visits the sine centre *and* the narrow pulse, not merely that it moves. The falsified premise is recorded in the case banner.
- **Files modified:** `tests/test_vco_core.cpp` · **Committed in:** `d5aeb31`

---

**4. [Rule 2 — Missing Critical] `kHostileBoundV` / `kMusicalBoundV` hoisted to the anonymous namespace**

- **Found during:** Task 2, writing invariant 6.
- **Issue:** plan 32-08 pinned both constants as `const float` **local to invariant 2's `TEST_CASE`**, while recording `kHostileBoundV` as "the PHASE-WIDE OUTER output bound, no exceptions anywhere, **binding on every scenario any later plan adds**". Invariant 6 is the first such later scenario and could not see either name, so the only options were to re-declare `10.0f` and `5.55f` by hand in a second case — creating exactly the hand-maintained mirror this suite has already watched drift once (`DeliberatelyBrokenSharedStateCore`) — or to hoist.
- **Fix:** both constants moved to the anonymous namespace beside `SAMPLE_RATES` as `constexpr float`, with a pointer to invariant 2's banner where the full provenance stays and is **not** duplicated. Invariant 2's declaration site carries a note recording that only the declaration moved. **Behaviour-neutral, proved mechanically:** the whole-suite assertion count was 2 621 990 immediately before and immediately after the hoist, with 0 failures.
- **Files modified:** `tests/test_vco_core.cpp` · **Committed in:** `d5aeb31`

---

**5. [Rule 2 — Missing Critical] Invariant 2's forward reference to this plan, resolved in place**

- **Found during:** Task 2, after invariant 6 was green.
- **Issue:** invariant 2's nesting paragraph said the outer tier applies to every scenario "including the audio-rate MORPH sweep plan 32-09 **adds**" — a live forward reference that this plan had just landed. This suite's house rule is that a resolved forward reference gets resolved where it is written, not only in the register (T-32-23, the identical failure 32-08 fixed for invariant 1).
- **Fix:** the paragraph now records that the reference has landed, names invariant 6, states that it asserts `kHostileBoundV` at all 27 configurations with a measured grid-wide worst of 6.289864 V, and notes it is the first scenario in the suite to decline the tighter tier and why that is examined rather than assumed.
- **Files modified:** `tests/test_vco_core.cpp` · **Committed in:** `d5aeb31`

---

### Recorded, not fixed

- **The plan's assertion-count arithmetic undercounts by one per configuration.** It says "the assertion count rises by the extended configuration count times **five**", from a banner that lists five *named* assertions. Scenario four in fact carries **six** `CHECK`s per configuration — plan 32-08 split the single magnitude bound into the two nested tiers. Measured, the rise is `128 x 6 = 768`, and the doctest total moved 2 621 222 -> 2 621 990 exactly. No code change: the assertion set is correct as it stands, only the plan's count of it was stale.

---

**Total deviations:** 5 auto-fixed — 3 premises falsified by measurement and corrected in place with their old values recorded (two of them the plan's own, one of which would have been red on correct behavior), 1 structural change preventing a hand-mirrored constant, 1 resolved forward reference — plus 1 arithmetic correction recorded without a code change. **No `src/` file was touched, no assertion was softened, no threshold or tolerance was loosened, no guard was weakened, no bound was pinned merely because the implementation currently produces it, and `src/AnalogLFO.cpp` is absent from both commits.**

## Findings Recorded for Later Plans

- **Plan 32-10's operator UAT has its headless counterpart's numbers.** The same control surface at the same three sample rates and the same three modulation rates measures 5.508759 to 6.289864 V. The two are deliberately **not** substitutes: invariant 6 bounds the numbers, the UAT judges whether it *sounds* like zipper noise at a crossfade seam. Nothing in `tests/test_vco_core.cpp` can see the latter.
- **Plan 32-10 should record the corrected D-15 rationale in the phase's deferred register** so no later phase inherits the falsified "oversampled inner loop" premise. The source correction is at `tests/test_vco_core.cpp:1097`; `src/dsp/VcoCore.hpp` corrected its own copy of the same sentence in plan 32-06.
- **Phase 34 (output conditioning) inherits a NEW worst figure, and it is not the one 32-08 handed over.** 32-08's worst measured anywhere was 7.201301 V under *static* inputs at the guarded Nyquist ceiling. Under **audio-rate MORPH at a musical note** the envelope reaches **6.289864 V** — lower in absolute terms, but reached at C9 with a perfectly ordinary pitch, which is a patch a user will actually build. If Phase 34 changes anything upstream of the return, both figures need re-measuring.
- **The excess over the musical tier is a 44.1/48 kHz phenomenon at the top note and the top modulation rate only.** At 96 kHz the whole 9-configuration slice stays inside 5.55 V, because the phase increment at C9 is half as large and the swept peak coincides with the naive envelope. Any future plan tempted to write a per-rate form of invariant 6's excess assertion will find it red at 96 kHz on correct behavior.
- **`MorphBlep`'s `dt` upper bound is unreachable through `forge::VcoCore` and is covered only by `tests/test_morph_blep.cpp`.** If a future phase ever raises `kVcoMaxDeltaPhase` above 1.0, or adds a caller that does not clamp, that guard changes from unreachable to live and scenario four's census must be re-measured rather than assumed to still read `UPPER=0`.
- **`morph = 1.00` is not a segment boundary, and this will be tempting to get wrong again.** Any later case reasoning about the crossfade's segments should mirror the frozen `(int)(morph * 4)` with the minimum-of-3 clamp, as invariant 6 does, rather than treating the morph axis as four equal open intervals.
- **The 32-05 measure-zero missed-edge caveat did not bite anywhere in this plan.** `dt = 0.0005` with a 0.374 duty is not on any grid this plan drives, and no measurement showed the signature. It was not re-fixed and was not re-derived.

## Known Stubs

None. Every line this plan added is asserted by a case that runs on every invocation. There is no placeholder, no skipped case, no `TODO` and no flag. Invariant 6 is a live `TEST_CASE` contributing 326 assertions, and scenario four's extension contributes 768.

## Threat Flags

None — no network, auth, file-access or schema surface was introduced, and no `src/` file was modified. Every threat-register entry assigned to this plan is mitigated by a named artefact:

| Threat | Mitigation as landed |
|--------|----------------------|
| **T-32-02** (division by `dt` newly reachable from `sampleTime`) | 176 configurations driven straight into `core.step(in)` with no driver in the way, asserting finiteness, both tiers, the phase range and both frequency properties. The guard census is measured and recorded, including which classes reach the divisor's guard and which are stopped one guard further down. |
| **T-32-24** (unbounded transient through the pending accumulator under audio-rate modulation) | Invariant 6 bounds 27 configurations at a worst of 6.289864 V against a 10.0 V outer tier, with the boundary crossings proven to occur first and the 0.771832 V excess over the static envelope pinned and attributed to D-13 by measurement rather than absorbed. |
| **T-32-03** (out-of-range output damaging downstream gear) | Both tasks assert `kHostileBoundV` at every configuration with no exception. Scenario four's grid maximum is unmoved at 5.000000 V; invariant 6's is 6.289864 V, clearing 10.0 V by 3.71 V and sitting well inside Rack's +/-12 V norm. |
| **T-32-23** (a falsified rationale inherited by a later phase) | The premise directly above `HOSTILE_TIMES` is replaced in place at `:1097` and names what was falsified and by what; invariant 2's forward reference to this plan is resolved in place; two further plan premises are corrected in the source with their old wording recorded. |
| **T-32-25** (a modulation case that never modulates) | Invariant 6 `REQUIRE`s counted boundary crossings, four-segment occupancy, both endpoints, block non-constancy and a 90 % non-zero floor **before** any value assertion — the same validity-first habit invariant 4 uses. |
| **T-32-12** (the shipped Analog LFO's golden bit-stability) | No `src/` file touched, `FROZEN.sha256` unmoved, `src/AnalogLFO.cpp` absent from both commits, and the six `.f32` goldens replay bit-exact on every `make test`. |
| **T-32-SC** (package installs) | Zero packages installed in any ecosystem. |

## Issues Encountered

None beyond the five deviations above. `make test`, `make strict` and `make guards` were green at **both** commits — no red window was opened at any point, because every change is additive to a passing suite.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

**Ready for 32-10.**

The hostile-timing grid covers infinite, subnormal and very-large-finite timing on both fields with per-entry reasons, and the rationale that pointed the work there is corrected where it is used rather than only in the register. The guard census is measured rather than assumed, including the honest statement that one of the two guards this plan was sent to exercise is unreachable from this call site. Audio-rate MORPH is bounded across 27 configurations with its non-vacuity proven before any value is asserted, and the one number a reader would trip over — a modulated envelope 0.771832 V above the static one — is pinned, attributed and explained. Plan 32-10 inherits the headless figures its operator UAT is the judgment-side counterpart of, and a note telling it to record the corrected D-15 rationale in the phase's deferred register. Nothing in the shipped LFO moved.

## Self-Check: PASSED

- `tests/test_vco_core.cpp` — FOUND on disk; `HOSTILE_RATES` 8 entries at `:1087`, `HOSTILE_TIMES` 11 entries at `:1153`, the corrected premise at `:1097`, `infinity()` x 4, `denorm_min()` x 3, `1e30f` x 2, `AA-05 forbids oversampling` x 1, `Approx` x 0 in comment-stripped source, six entries in the banner's invariant list with 1-5 unrenumbered
- `.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/32-09-SUMMARY.md` — FOUND on disk
- Commits `aab56f7`, `d5aeb31` — both FOUND in `git log`, each listing only `tests/test_vco_core.cpp`
- All plan `<success_criteria>` re-run and green; both tasks' `<acceptance_criteria>` re-run and green, with the documented exceptions in deviations 1-3 and the recorded arithmetic correction (the plan's "times five" is measured as times six) — each falsified item corrected in place with both the old and the new values recorded
- `make test` 94/94/0 at 2 622 316 assertions, `make strict` PASS, `make guards` PASS, `git status --porcelain src/dsp/FROZEN.sha256` empty — all re-run at the final commit

---
*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Completed: 2026-08-01*
