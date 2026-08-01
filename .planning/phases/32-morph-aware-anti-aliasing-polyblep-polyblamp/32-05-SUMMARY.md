---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
plan: 05
subsystem: testing
tags: [doctest, polyblep, polyblamp, band-limiting, morph-aware, aa-02, aa-03, aa-04, d-03, d-07, d-13, d-14, d-15, p-3, p-4, vco]

# Dependency graph
requires:
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-04 — src/dsp/MorphBlep.hpp, forge::morphBlepCharFactor and struct forge::MorphBlep, plus the 22-assertion out-of-tree probe inventory this plan makes permanent"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-01 — tests/test_morph_blep.cpp pre-registered in check_includes.sh VCO_SIDE_ALLOW before the file existed"
  - phase: 30-vco-core-skeleton-and-registration
    provides: "forge::VcoCore::step's phase-update order, which driveOneSite mirrors; the accumulate-don't-assert and validity-first REQUIRE idioms"
provides:
  - "tests/test_morph_blep.cpp — the permanent AA-02/AA-03/AA-04 unit suite, 6 cases / 909 assertions"
  - "probeJump, probeSlopeBreak, frozenPulseDuty, frozenCurve, driveOneSite, walkResonant — the test-only instruments"
  - "A CLOSED hole in MorphBlep's D-15 guard: a +infinity dt no longer reaches the divisor"
  - "The P-3 resonant-tiling regression case, PROVED to detect a double-sourced side decision"
affects: [32-06, 32-07, 32-08, 33-hard-sync, morph-blep]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Probe-the-frozen-header: assert an analytic table against a direct evaluation of the code it was derived from, never against a restatement of itself (AA-04)"
    - "Per-row expected values instead of a blanket guard claim, so a legitimate exception is documented rather than hidden"
    - "Sparse-stream distinguishability: a correction stream's resting value is zero, so 'few samples equal' is the wrong non-vacuity metric — count DIFFERING samples and require each stream to be active"
    - "Prove-by-breaking: temporarily invert the property under test, record the observed failure count and magnitude, then revert and confirm the source is clean"
    - "Compute-never-type a frozen-derived constant: 0.5f - 0.45f and 0.05f are different floats, and a strict comparison against the float p can put them on opposite sides of an edge"

key-files:
  created:
    - tests/test_morph_blep.cpp
  modified:
    - src/dsp/MorphBlep.hpp

key-decisions:
  - "The plan's square-split probe at morph 0.75 was relocated to morph 0.70, where W[3] is genuinely live — at 0.75 the frozen direct-duty special case zeroes W[3] and the check would have passed on the PULSE's hard step, which does not split"
  - "A +infinity dt was found to pass MorphBlep's lower-bound-only guard, reach the divisor and leave pending = NaN permanently; both guards gained a negated upper bound of 1.0"
  - "The saw's morphedWave wrap jump is NOT +2 above character 0 — D-05's bleed normalization divides by (1 + bleedIntensity) — so the assertion moved to the closed form 2/(1+bi), which is stricter than the constant would have been"
  - "The sine-centre bleed probe uses eps = 1e-6, not the plan's 1e-4, because the sine's own 2*pi slope contributes 1.26e-3 across a 1e-4 bracket — two orders of magnitude above the tolerance"
  - "probeSlopeBreak uses eps = 1e-3 while probeJump uses 1e-4: dividing by eps amplifies morphedWave's float rounding by 1/eps, so the two probes have DIFFERENT optimal eps and collapsing them to one constant silently weakens one"

patterns-established:
  - "Falsified-premise-corrected-in-place, continued from 32-04: keep the conclusion, replace the reason, and say the old reason was measured false — applied five times in this plan, all in the plan text rather than the implementation"
  - "Non-vacuity guards are load-bearing, not ceremony: two of this plan's three test-construction defects were found by its own REQUIREs rather than by inspection"

requirements-completed: [AA-02, AA-03, AA-04, AA-05]

coverage:
  - id: D1
    description: "The D-03 character factor reaches its two limits EXACTLY — 1.0f bit-exact at a true hard step, 0.0f bit-exact at and beyond twice dt — over eight phase increments from 1/96000 to 0.495"
    requirement: "AA-04"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: morphBlepCharFactor hits D-03's limits EXACTLY ... / A: the two exact limits"
        status: pass
    human_judgment: false
  - id: D2
    description: "The factor is strictly monotone over (0, 2*dt) and its first difference at the cutoff is 2.5e-5 — the discriminator that fixes the exponent at 2 rather than 1 (the un-squared form measures 5.0e-3)"
    requirement: "AA-04"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: morphBlepCharFactor hits D-03's limits EXACTLY ... / B: monotone decreasing, and the SLOPE argument"
        status: pass
    human_judgment: false
  - id: D3
    description: "Hostile dt and w reach no divisor: an 18-row grid over {0, negative, subnormal, +inf, -inf, NaN} x {0, 0.01, NaN}, each row carrying its own expected value and a finiteness assertion"
    requirement: "AA-05"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: morphBlepCharFactor hits D-03's limits EXACTLY ... / C: hostile dt and hostile w"
        status: pass
      - kind: other
        ref: "RED observed before the fix: 4 failing assertions, both +infinity rows returning nan"
        status: pass
    human_judgment: false
  - id: D4
    description: "Every hard site magnitude is the CHARACTERIZED jump of the frozen Waveshape, probed directly: +2.000000 at the saw centre, +2/-2 at morph 0.75, and +1.999920/-1.600080 at morph 0.70 where the weight algebra is genuinely exercised"
    requirement: "AA-04"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: the site magnitudes ARE the characterized jumps ... / A: the HARD sites at character 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "The triangle's slope breaks are -8.000016 and +8.000016, with a non-vacuity REQUIRE that it contributes NO value jump at either site — the reason AA-02 is a separate requirement from AA-01"
    requirement: "AA-02"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: the site magnitudes ARE the characterized jumps ... / B: the SLOPE breaks at character 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "P-4 made permanent: the raw computeSaw wrap jump is +2 to within 4e-4 at character 0, 0.25, 0.5, 0.75 and 1.0, and the morphed path matches the closed form 2/(1+bi) at all five"
    requirement: "AA-04"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: the site magnitudes ARE the characterized jumps ... / C: P-4, the falsified premise, made permanent"
        status: pass
    human_judgment: false
  - id: D7
    description: "The bleed ring is band-limited inside what the user hears as a pure sine (0.0148632 against a closed form of 0.0148515), and the excess is proved to be smooth slope by its linear convergence in eps"
    requirement: "AA-04"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: the site magnitudes ARE the characterized jumps ... / D: the bleed ring, and the square's TWO distinct positions"
        status: pass
    human_judgment: false
  - id: D8
    description: "The square's hard step at 0.5 and soft edge at dutySq are genuinely distinct positions (-1.201655 against -0.002073 at character 0.5), with a character-1.00 complement proving the discriminator vanishes at full character"
    requirement: "AA-02"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: the site magnitudes ARE the characterized jumps ... / D (square split rows at morph 0.70)"
        status: pass
    human_judgment: false
  - id: D9
    description: "The pending accumulator delivers the second half at zero latency and drains exactly once — the third sample is a bit-exact 0.0f with both members zeroed"
    requirement: "AA-04"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: the pending accumulator delivers the second half at zero latency, and drains exactly once (D-13)"
        status: pass
    human_judgment: false
  - id: D10
    description: "Overlapping pulse edges SUM rather than overwrite at a 5-percent duty and a C9 phase increment: emitted matches alone0+alone1 to 1e-5 on all 13 overlap rows and is provably neither one alone on the 11 with room"
    requirement: "AA-03"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: overlapping pulse edges SUM rather than overwrite at a narrow duty (AA-03 / D-07)"
        status: pass
    human_judgment: false
  - id: D11
    description: "The D-14 sync seam feeds the SAME accumulator (a driven morph site at s = 0.5 and addStep(0.5, 2) both produce +0.250000/-0.250000), events compose by summation, and the entry gate rejects a negative, over-range or not-a-number position without touching state"
    requirement: "AA-03"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: the D-14 sync seam feeds the SAME accumulator ... / A and B"
        status: pass
    human_judgment: false
  - id: D12
    description: "All six hostile dt classes driven STRAIGHT into step() return exactly the primed 0.25 and leave both members at zero, with no driver in the way to absorb them"
    requirement: "AA-05"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: the D-14 sync seam feeds the SAME accumulator ... / C: hostile dt"
        status: pass
    human_judgment: false
  - id: D13
    description: "Two MorphBlep instances stepped interleaved reproduce their solo streams BIT-EXACTLY, with a sparse-stream distinguishability precondition ahead of the property"
    requirement: "AA-05"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: the D-14 sync seam feeds the SAME accumulator ... / D: two instances stepped interleaved"
        status: pass
    human_judgment: false
  - id: D14
    description: "At 165 resonant phase increments the band-limited envelope stays inside 1.11 with every row firing at least once per cycle — and the case is PROVED to detect a double-sourced side decision (20 failures, observed maximum 1.999949)"
    requirement: "AA-04"
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: RESONANT phase increments do not spike the envelope ... (P-3)"
        status: pass
      - kind: other
        ref: "sensitivity probe: side line temporarily sourced from the double phase -> 20 failed assertions, maxAbs 1.999949; reverted, git status --porcelain src/dsp/MorphBlep.hpp empty"
        status: pass
    human_judgment: false
  - id: D15
    description: "The shipped Analog LFO is untouched: no frozen header edited, FROZEN.sha256 unmoved, src/AnalogLFO.cpp absent from every commit in this plan"
    verification:
      - kind: integration
        ref: "make guards (check_frozen + check_includes + check_canary) PASS; make strict PASS; git show --name-only over all four commits"
        status: pass
    human_judgment: false

# Metrics
duration: 78 min
completed: 2026-08-01
status: complete
---

# Phase 32 Plan 05: The MorphBlep Unit Suite Summary

**`tests/test_morph_blep.cpp` now pins every claim `src/dsp/MorphBlep.hpp` makes — the D-03 factor's two exact limits, every site magnitude probed against the FROZEN `Waveshape` rather than against the header's own table, the zero-latency accumulator, overlapping-edge summation, the D-14 seam, hostile timing, and a 165-row resonant-tiling regression whose sensitivity is measured rather than argued — and it found and closed a real hole in the header's D-15 guard on its first run.**

## Performance

- **Duration:** ~78 min
- **Completed:** 2026-08-01
- **Tasks:** 3
- **Files:** 1 created (`tests/test_morph_blep.cpp`, 6 cases / 909 assertions), 1 modified (`src/dsp/MorphBlep.hpp`)

## Task Commits

1. **Task 1 — RED gate (the fix the suite forced)** — `51ae09c` (fix): reject a non-finite positive `dt` in the D-03 factor and in `step()`
2. **Task 1 — the D-03 factor's limits, monotonicity, slope continuity and hostile inputs** — `ee9d809` (test)
3. **Task 2 — every site magnitude probed against the frozen `Waveshape`** — `69818f1` (test)
4. **Task 3 — the accumulator, overlapping edges, the seam, hostile timing and the P-3 resonant tiling** — `902b539` (test)

## Accomplishments

- **AA-04 is asserted the way AA-04 means it.** Every magnitude in `MorphBlep`'s nine-site table is now checked against a direct probe of `Waveshape::morphedWave`, not against a restatement of the table. That is the half of D-01's trade plan 32-04 could not close on its own: the header duplicates frozen internals, and this file is what detects the duplication going stale if the byte-pin is ever lifted.
- **A real defect was found, proved and fixed.** A `+infinity` `dt` PASSED the header's `fdt > 0.f` guard, reached the divisor, and left `pending = NaN` — poisoning the instance **permanently**, not for one sample. RED was 4 failing assertions on the very first run of Task 1's hostile grid. The other five hostile classes were already correct.
- **The two spectrally-invisible defects each have a dedicated case, and the second one's sensitivity is measured.** Case two part D pins the square's split positions; case six walks 165 resonant increments and was **observed failing** — 20 assertions, envelope 1.999949 against a 1.11 bound — with the side decision temporarily sourced from the double.
- **Five falsified premises in the plan text were corrected in place rather than weakened**, and each correction made the assertion *stronger* than the plan's wording would have been. None of them touched the implementation.
- **The suite's own non-vacuity guards did the work they exist for.** Two of the three test-construction defects in this plan were caught by its own `REQUIRE`s — the overlap discriminator running where a contribution was 0.00277, and a sparse-stream distinguishability check that would have passed on two nearly-silent signals.

## Verification Results

| Check | Result |
|-------|--------|
| `make test` | **91 cases / 91 passed / 0 failed**, 2,619,816 assertions (85 / 2,618,907 at plan start) |
| `./build-test/test -tc="morph blep: *"` | **6 cases / 6 passed / 0 failed**, 909 assertions |
| `./build-test/test -tc="morph blep: RESONANT*"` | 1 case / 1 passed, 664 assertions |
| `make strict` | `strict C++11 gate: PASS` |
| `make guards` | `guard suite: PASS` |
| `check_includes.sh [1/7]` | OK — 29 LFO-side roots, zero VCO includes, this file already exempt via 32-01 |
| `check_frozen.sh` | PASS — `src/dsp/FROZEN.sha256` unmoved |
| `grep -c 'DOCTEST_CONFIG_IMPLEMENT'` | 0 |
| `Approx` in comment-stripped source | **0** — every limit assertion is a bit-exact float comparison |
| `grep -c 'kMorphBlepPreScaleEnvelope'` | 2 (>= 2 required) |
| `grep -c '748'` | 7 (>= 1 required) |
| `src/AnalogLFO.cpp`, `src/dsp/Waveshape.hpp` | absent from all four commits |

## The Measured Figures This Plan Was Asked To Record

**The five saw-wrap probe values (P-4), on `computeSaw` itself:**

| character | 0.00 | 0.25 | 0.50 | 0.75 | 1.00 |
|---|---|---|---|---|---|
| raw wrap jump | 1.999600 | 1.999805 | 1.999821 | 1.999847 | 1.999884 |

The residual is the `eps` bracket, not character. On the **morphed** path the same probe reads 1.999600 / 1.994818 / 1.980021 / 1.955841 / 1.922966, matching `2/(1 + bleedIntensity)` = 2.000000 / 1.995012 / 1.980198 / 1.955990 / 1.923077 to within 4e-4.

**The two square positions and their difference (morph 0.70):**

| character | jump at 0.5 | `dutySq` | jump at `dutySq` | difference |
|---|---|---|---|---|
| 0.50 | -1.201655 | 0.510000 | -0.002073 | **1.199583** |
| 0.71 | -0.795097 | 0.520164 | -0.002064 | **0.793033** |
| 1.00 | -0.001661 | 0.540000 | -0.002048 | 0.000386 (the complement) |

The -1.201655 is itself a D-07 summation — the square's hard step (-1.188119) plus the pulse's, brought to the same position by the bleed ring (-0.011881) — the exact pair plan 32-04 measured.

**The sine-centre bleed step:** 0.0148632 at `eps = 1e-6`, against a closed form of 0.0148515. Convergence toward it: 1.19e-4 at `eps = 1e-5`, 1.17e-5 at 1e-6, 1.34e-6 at 1e-7 — a clean factor of ten per decade, which is the signature of a smooth slope rather than a second discontinuity.

**The overlapping-edge magnitudes (morph 1.00, character 0, dt 0.19):** 13 of 41 swept start phases overlap. At `start = 0.80` the two single contributions are +0.897507 and -0.468144 and the emitted correction is **0.429363** — their sum to 1.6e-7, and 0.468 away from one and 0.898 away from the other.

**The resonant grid:**

| quantity | value |
|---|---|
| computed duty at morph 0.82 | 0.374000013 — **bit-identical to `0.374f`** |
| nominal `0.374 / 0.0005` | 748.000000000000 (exact) |
| realised `duty / 0.0005` | 748.0000257 — the ULP-scale offset the case hunts |
| rows walked | 165 (3 characters x 11 base increments x 5 ULP steps) |
| per-row maximum, character 0.0 | **1.000000** |
| per-row maximum, character 0.5 | **0.997545** |
| per-row maximum, character 1.0 | **0.988873** |
| bound | 1.11 |
| tightest non-vacuity margin | `fired - cycles = 0` — every row fires at least once per cycle |
| **observed maximum under the double-sourced-side probe** | **1.999949**, with **20 failed assertions** |

**Doctest counts before and after:** 85 cases / 2,618,907 assertions -> **91 cases / 2,619,816 assertions**, 0 failed throughout.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] A `+infinity` `dt` passed MorphBlep's guard, reached the divisor and poisoned the instance permanently**

- **Found during:** Task 1, on the first run of the hostile-input grid.
- **Issue:** Both guards were written with a lower bound only (`!(fdt > 0.f)`). `+infinity` satisfies `> 0.f`, so it passed. In `morphBlepCharFactor` the division became `inf/inf` and returned a not-a-number; in `step()` the same input drove `d / dt` to 0, fired **every** live site at sub-sample position 0, and multiplied that not-a-number into `pending`. **MEASURED:** `step()` returned `nan` **and left `pending = nan`**, so every later sample returned `nan` even after timing recovered. A one-sample glitch would have been survivable; this was not.
- **RED, observed:** 4 failing assertions, both `+infinity` rows, before any change.
- **Fix:** a negated upper bound, `!(fdt <= 1.f)`, added alongside the existing lower bound in both places. The bound is 1 rather than `kVcoMaxDeltaPhase` because `dt > 1` is more than a full cycle per sample — the single-subtract wrap is already meaningless there — and `forge::VcoCore` clamps at 0.5, so it has a factor of two of margin and provably cannot fire on a legitimate input. Written as a literal so the header keeps its deliberate independence from `dsp/VcoCore.hpp`.
- **Files modified:** `src/dsp/MorphBlep.hpp`
- **Verification:** GREEN immediately after; `make test` 86/86, `make strict` PASS, `make guards` PASS. No measurement anywhere in the phase moved, because the bound cannot fire for `0 < dt <= 1`.
- **Committed in:** `51ae09c`
- **Note on the plan's file list:** plan 32-05 states "No `src/` file is modified by this plan", and each task's acceptance criteria expect `git diff --name-only` to list only the test file. That expectation rests on the header already being correct. The test proved otherwise, and leaving a permanently-poisoning path in the audio code to satisfy a file-list criterion would have inverted the point of the plan. The threat register's `T-32-02` (division by `dt`, high, **mitigate**) is the disposition this fix honours.

---

**2. [Rule 1 - Bug] The plan's square-split probe at `morph = 0.75` would have been VACUOUS, not merely unsatisfiable**

- **Found during:** Task 2, part D.
- **Issue:** At exactly `morph = 0.75`, `scaled` is exactly 3.0, so `segment == 3` and the frozen direct-duty special case (`Waveshape.hpp:179-182`) sets `W[4] = 1` and leaves `W[3] = 0`. The square carries **no weight at all**. **MEASURED at 0.75 / character 0.50:** the probe at 0.5 reads -1.487628 and at `dutySq` reads -0.001951, a difference of 1.485677 — the check would have **PASSED**. But it would have passed on the **PULSE's** hard step, and the pulse does **not** split (its branch and its soft edge both derive from the same duty). The assertion would have been green against a merged square entry, which is the exact defect it exists to catch. This is worse than the plan's premise being false; the premise being false is what made the case vacuous.
- **Fix:** relocated to `morph = 0.70` (`scaled = 2.8`, `segment = 2`, `frac = 0.8`, so `W[3] = 0.8` is live) — the same relocation plan 32-04 made for the same reason. The assertion is **unchanged in strength**; only its morph position moved. Two character rows are asserted (0.50 and 0.71, the measured peak of the `(1-c)`-weighted spike) plus a character-1.00 **complement** row proving the discriminator vanishes at full character, so nobody generalises the case to a character where it cannot fail.
- **Files modified:** `tests/test_morph_blep.cpp`
- **Committed in:** `69818f1`

---

**3. [Rule 1 - Bug] The plan asserts the saw's morphed wrap jump is +2.0 at every character; MEASURED 1.922966 at character 1**

- **Found during:** Task 2, part C.
- **Issue:** unsatisfiable as written. The saw itself is unchanged (+1.999884 at character 1) — the difference is **D-05's bleed normalization**, which divides the whole result by `(1 + bleedIntensity)`.
- **Fix:** the literal P-4 claim is now asserted on `computeSaw` **directly** at all five characters, and the morphed path is asserted against the closed form `2/(1 + bi)`. That is **stricter** than the constant would have been — it pins the normalization as well as the saw — not weaker.
- **Committed in:** `69818f1`

---

**4. [Rule 1 - Bug] The plan fixes `eps = 1e-4` for the sine-centre bleed probe, where the sine's own slope swamps the measurement**

- **Found during:** Task 2, part D.
- **Issue:** a `+/-eps` probe returns (true discontinuity) + (local slope x 2*eps). At the sine centre the slope is `2*pi`, so a 1e-4 bracket contributes 1.26e-3 — two orders of magnitude above the 1e-4 tolerance the plan asks for. **MEASURED 0.016046** against a target of 0.0148.
- **Fix:** `eps = 1e-6` there, and the case now **asserts the linear eps-convergence** (1.19e-4 -> 1.17e-5 -> 1.34e-6) that proves the residual is slope rather than a missed discontinuity. Separately, `probeSlopeBreak` uses `eps = 1e-3` while `probeJump` uses 1e-4, because dividing by `eps` amplifies float rounding by `1/eps`: **MEASURED -7.998944 at 1e-4 against -8.000016 at 1e-3**. The three-sided `eps` envelope is written into the helper banner.
- **Committed in:** `69818f1`

---

**5. [Rule 3 - Blocking] The plan's `driveOneSite` signature takes `startPhase` BY VALUE, which cannot express the plan's own instruction to call it twice**

- **Found during:** Task 3, case three.
- **Issue:** a by-value phase resets on every call, so the second sample re-walks the first and D-13's entire claim — that the second half arrives on the **following** sample — becomes invisible.
- **Fix:** `double& phase`. Documented in the helper banner as a deliberate departure.
- **Committed in:** `902b539`

---

**6. [Rule 2 - Missing Critical] Two of this plan's own non-vacuity guards were mis-specified, and the guards themselves caught it**

- **Found during:** Task 3, first full run — 2 failures, both in the new cases.
- **Issue (a):** the sum-versus-overwrite discriminator ran on all 13 overlap rows, but at the two window edges the second contribution is 0.00277 and 0.01108 — **smaller than any threshold that could separate a sum from an overwrite**. Asserting there would not catch more; it would only fail for a reason unrelated to the property.
- **Issue (b):** "interleaved == solo" used `soloEqual < n/10`, the metric `tests/test_vco_core.cpp`'s invariant 4 uses. That case compares **dense audio**; this one compares a **sparse correction stream** whose resting value is zero. **MEASURED at 64 samples: the two streams agreed on 59 of 64, purely because both were silent** — 3 non-zero samples each.
- **Fix (a):** the discriminator now runs on the 11 rows where both contributions clear 0.02, with the excluded rows still covered by the sum and cancellation assertions, and `strongOverlaps >= 8` asserted so the restriction cannot silently empty the set.
- **Fix (b):** raised to 256 samples and restated as "differing samples >= 8 **AND** each stream non-zero >= 4". **MEASURED: 23 / 11 / 13.**
- **Committed in:** `902b539`

---

**Total deviations:** 6 auto-fixed (4 bugs in the plan's stated behavior, 1 blocking signature defect, 1 missing-critical non-vacuity correction). **One production-code change** — deviation 1, the `+infinity` guard hole, which is a genuine defect the suite was written to find. Every other correction strengthened an assertion rather than weakening one. No guard was weakened, no frozen file was touched, and `src/AnalogLFO.cpp` is absent from all four commits.

## TDD Gate Compliance

This plan is `type: tdd` and its gate sequence is complete and in order:

| Gate | Evidence |
|------|----------|
| **RED** | Task 1's hostile grid run against the header exactly as plan 32-04 landed it: **4 failing assertions**, both `+infinity` rows, `k := nan`, `std::isfinite(k)` false. Observed, not argued. |
| **GREEN** | `51ae09c` (`fix`) — `make test` 86/86, 0 failed, immediately after. |
| **RED (second cycle)** | Task 3's first full run: **2 failing assertions**, both raised by this plan's own non-vacuity guards rather than by the header. |
| **GREEN (second cycle)** | 91/91, 0 failed after the two guard corrections. |
| **Sensitivity (prove-by-breaking)** | Case six observed FAILING with the side decision temporarily sourced from the double `phase`: **20 failed assertions, observed maximum 1.999949** against a 1.11 bound. Probe reverted; `git status --porcelain src/dsp/MorphBlep.hpp` empty and the probe marker absent before the commit. |

Commit types: `fix(32-05)` for the header guard (a production behavior change), `test(32-05)` for the three test commits.

## Decisions Made

Recorded in the frontmatter `key-decisions`. The load-bearing one for later plans: **`MorphBlep::step` and `morphBlepCharFactor` now reject `dt > 1` as well as `dt <= 0` and non-finite `dt`.** Nothing legitimate is affected — `forge::VcoCore` clamps its increment at `kVcoMaxDeltaPhase = 0.5` — but a future caller that decouples `sampleTime` from `sampleRate` will now get the drained accumulator rather than a poisoned instance.

## Known Stubs

None. Every case in the file asserts a measured value; there is no placeholder, no skipped case and no `TODO`.

## Threat Flags

None. No network, auth, file-access or schema surface was introduced. Every threat-register entry assigned to this plan is mitigated by a named case:

| Threat | Mitigation as landed |
|--------|----------------------|
| T-32-02 (division by `dt`) | Case five part C drives all six hostile classes straight into `step()` — and **found the `+infinity` hole**, now closed |
| T-32-17 (out-of-range sync event) | Case five part B: three rejected `addStep` calls leave both members bit-exactly zero |
| T-32-16 (spectrally invisible spike) | Case two parts A and D: every hard magnitude probed against the frozen header, and the square's two positions proved distinct at a morph where the square is genuinely live |
| T-32-31 (resonant full-amplitude spike) | Case six: 165 rows, envelope bounded at 1.11, **sensitivity proved by observing 1.999949 under a deliberately broken side decision** |
| T-32-18 (shared voice state) | Case five part D: bit-exact interleave equality behind a sparse-stream distinguishability precondition |
| T-32-14 (a case that cannot fail) | Every case carries a non-vacuity `REQUIRE` ahead of its value assertion; two of them fired during development |
| T-32-SC (package installs) | Zero packages installed in any ecosystem |

## Issues Encountered

None beyond the six deviations above. `make test`, `make strict` and `make guards` were green at every commit.

## Findings Recorded for Later Plans

- **Plan 32-06 (the `VcoCore` wiring)** must pass the **same** float `p` to `morphedWave` and to `MorphBlep::step`, and the **double** `phase` and `deltaPhase` alongside it. Case six is what will catch a caller that breaks that identity — but only if the wiring is exercised at a resonant increment, so 32-06 should not assume the unit suite covers it end to end.
- **Plan 32-07 (the threshold re-pin)** inherits a measured caveat: **at `dt = 0.0005` exactly, the pulse edge at 0.374000013 is MISSED once per cycle.** The preceding sample sits 1.0000257 samples away — just outside the `s <= 1` gate — and the next sample is already past the site, so the correction is never placed. This is a lost half-jump on a measure-zero set of increments, not an envelope spike, so case six stays green. It is **recorded rather than "fixed"**: widening the gate trades a missed edge for a double-fired one, which `src/dsp/MorphBlep.hpp` explicitly rejects as the companion anti-pattern. If a single grid cell in 32-07 misses its threshold at a suspiciously round sample rate, this is the first thing to check.
- **Plan 32-08 (the output envelope)** can rely on the measured pre-scale maxima recorded above — 1.000000, 0.997545 and 0.988873 at characters 0, 0.5 and 1 at morph 0.82 — all inside the 1.1047 naive envelope P-10 records.
- **Phase 33 (hard sync)** plugs into `addStep(xAhead, jump)`, whose split and entry gate are now permanently pinned by case five parts A and B. No header change is needed.
- **Any future editor of `src/dsp/MorphBlep.hpp`** should know that `frozenPulseDuty` in the test file exists because `0.5f - 0.45f` and `0.05f` are **different floats**, 1.5 ULP apart, and the strict comparison against `p` puts a site on opposite sides of a sample depending which one is used. That cost a debugging cycle here.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

**Ready for 32-06.**

`forge::MorphBlep` is now characterised by 909 permanent assertions across 6 cases, its D-15 guard is complete for all six hostile timing classes, and the two defects the spectral metric cannot see each have a dedicated case — the second of which has been observed detecting rather than merely passing. Nothing in the shipped LFO moved.

## Self-Check: PASSED

- `tests/test_morph_blep.cpp` — FOUND on disk, 6 cases / 909 assertions
- `src/dsp/MorphBlep.hpp` — FOUND on disk, guard fix present, probe marker absent
- `.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/32-05-SUMMARY.md` — FOUND on disk
- Commits `51ae09c`, `ee9d809`, `69818f1`, `902b539` — all FOUND in `git log`
- All plan `<success_criteria>` re-run and green; all three tasks' `<acceptance_criteria>` re-run and green, with the two documented exceptions (the `src/` file-list criterion, superseded by deviation 1; and the plan's "exactly 5 test cases" arithmetic, which omits Task 2's case — the file has 6)

---
*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Completed: 2026-08-01*
