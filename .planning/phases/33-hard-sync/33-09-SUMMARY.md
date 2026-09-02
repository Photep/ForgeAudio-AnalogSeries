---
phase: 33-hard-sync
plan: 09
subsystem: tests
tags: [hard-sync, pitch-04, d-12, third-input-class, deferred-item-11, non-vacuity, observed-firing, estimator-blindness, master-lock, new-divisor, apple-clang-only, sync-02-not-touched]

# Dependency graph
requires:
  - phase: 33-hard-sync
    plan: 06
    provides: "THE SEAM ITSELF — forge::VcoCore is measurement leg pastEdge, so every reading in this plan is taken on the SHIPPED core rather than on a withheld leg"
  - phase: 33-hard-sync
    plan: 04
    provides: "the sync detector's measured behaviour (the g-versus-f study, the falsified Pitfall 7) and the accumulate-then-assert idiom at grid scale"
  - phase: 33-hard-sync
    plan: 08
    provides: "the measure-the-distribution-BEFORE-pinning discipline, the two-sided bound derivation, and the stated-cushion habit this plan's two new constants copy"
  - phase: 31-pitch-tuning-exponential-fm
    provides: "PITCH-04's existing evidence (invariants 8 and 9), kTrackingToleranceCents, kEstimatorMinSamplesPerCycle, kPitchLooseBoundV, and DEFERRED ITEM 11 — the debt this plan discharges"
provides:
  - "INVARIANT 10 in tests/test_vco_pitch.cpp: PITCH-04's THIRD input class — 783 cells of extreme pitch x extreme FM x hostile sync, driven together rather than pairwise"
  - "THE MEASUREMENT PHASE 31 DEFERRED ITEM 11 DECLINED TO MAKE AS AN ARGUMENT: tel.freqHz on every patched sync shape is EXACTLY EQUAL to the unpatched row's, by float ==, on all 783 cells"
  - "The sync firing count ASSERTED IN BOTH DIRECTIONS per cell — six rows must fire, three provably cannot — proved able to fail at 522 assertions in one subcase and 27 in the other"
  - "The MASTER-LOCK measurement: hard sync replaces the slave's period with the master's over a MEASURED rho window [0.320, 1.310] at this shape point, and the output frequency is the master's to 0.020443 cents"
  - "kSyncLockToleranceCents = 0.10 cents, pinned from a two-sided interval and asserted to be BOTH looser than the v/oct tolerance and stricter than PITCH-01's one cent"
  - "The estimator's blindness at the Nyquist ceiling stated as ARITHMETIC from the forge:: constant — 1/0.495 = 2.0202 samples per cycle against a 2.5 cutoff — and asserted ABOVE the enumeration of qualifying cells"
affects: [33-10, 33-11, 33-12]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Asserting a control's firing count in BOTH directions — must-fire AND cannot-fire — from the detector's own state machine rather than from what the run produced, so 'the counter counts the right thing' is evidenced rather than assumed"
    - "Closing an inherited STRUCTURAL argument by measuring the thing it argued about: exact float equality of a downstream field across every hostile shape of the new upstream input"
    - "Placing a claim's population inside a measured window with STATED MARGIN when the window's edges are SHARP rather than a gap — the answer to a knife edge is margin, not a tighter pin"
    - "Naming the neighbouring cases a case does NOT claim, by title, and CHECKING each title resolves as a selector before writing it down"
    - "A deliberately-looser second tolerance for a genuinely different quantity, asserted to be looser than the file's own tolerance so it can never be read as a widening of it"

key-files:
  created: []
  modified:
    - tests/test_vco_pitch.cpp

key-decisions:
  - "A NEW TEST_CASE IN THE SAME FILE, not an extension of invariant 9 — Task 1's literal instruction contradicts the plan's own acceptance criterion, artifacts list and Task 2, and its stated reason (two files) does not apply to a same-file case"
  - "The pitch claim is split into a UNISON row that is explicitly NOT discriminating for sync, and MASTER-LOCKED rows that are — and the non-discriminating half says so in the source"
  - "kSyncLockToleranceCents is a SEPARATE constant, deliberately looser than kTrackingToleranceCents, because it measures a different quantity; the file's v/oct tolerance was NOT widened by one digit"
  - "The window length was MEASURED (0.25 / 0.50 / 1.00 s) and 0.50 chosen at the knee — the quarter-second window would have forced a tolerance three times looser for no reason but a shorter run"
  - "The rho window's edges are SHARP (0.315 does not lock, 0.320 does) and that is RECORDED rather than smoothed; the population sits 0.18 and 0.31 inside them"
  - "NO REQUIREMENT IS TICKED BY THIS PLAN. PITCH-04 was already [x] from Phase 31 and stays [x]; SYNC-02 is NOT touched; the re-tick decision is plan 33-11's, and it now has evidence to decide on"

requirements-completed: []  # SYNC-01 was earned by 33-04; SYNC-02 is NOT this plan's to close; PITCH-04 is Phase 31's tick, re-evidenced not re-ticked

coverage:
  - id: D1
    description: "The three input classes are driven TOGETHER — extreme pitch x extreme FM x hostile sync — rather than pairwise"
    requirement: "PITCH-04"  # re-evidenced; the tick itself is Phase 31's and the re-tick is 33-11's
    verification:
      - kind: unit
        ref: "\"vco pitch: (PITCH-04 / D-12) ...\" subcase one — 29 pitch/FM rows (invariant 9's 26 verbatim + invariant 8's three ceiling-relative points) x 9 sync shapes x 3 rates = 783 cells, 4000 steps each, 3,132,000 core steps; 6,266 assertions, 0 failures"
        status: pass
    human_judgment: false
  - id: D2
    description: "Sync is OBSERVED FIRING behind the claim, asserted in both directions, and the assertion is proved able to fail"
    requirement: "PITCH-04"
    verification:
      - kind: unit
        ref: "tests/test_vco_pitch.cpp:3234 CHECK(firingAsClassified) and :3426 / :3544 CHECK(fireCount > 0). Six sync rows must fire, three provably cannot; both directions asserted per cell"
        status: pass
      - kind: other
        ref: "PROVED ABLE TO FAIL: unpatching the whole grid reds 522 assertions at :3234 (6 must-fire rows x 29 x 3); a targeted probe on subcase two reds all 27 of its CHECK(fireCount > 0). Restored byte-identical and re-verified green"
        status: pass
    human_judgment: false
  - id: D3
    description: "Phase 31 deferred item 11's STRUCTURAL argument is replaced by a MEASUREMENT"
    requirement: "PITCH-04"
    verification:
      - kind: unit
        ref: "tests/test_vco_pitch.cpp:3233 CHECK(telemetryUnmovedBySync) — tel.freqHz on each of the 8 patched sync shapes compared to the UNPATCHED row's by EXACT float equality, per pitch point per rate, 783 cells"
        status: pass
      - kind: other
        ref: "PROVED ABLE TO FAIL: a one-line `if (in.syncConnected) freq *= 1.0001f;` inserted above the clamp in src/dsp/VcoCore.hpp reds this assertion 384 times (and two neighbouring cases). Header restored, `git diff` on it empty"
        status: pass
    human_judgment: false
  - id: D4
    description: "The pitch claim is made ONLY where the estimator can see, restricted on the instrument's blindness stated ABOVE the enumeration"
    requirement: "PITCH-04"
    verification:
      - kind: unit
        ref: "the restriction is at tests/test_vco_pitch.cpp:3253 with its assertion at :3265 (CHECK(samplesPerCycleAtCeiling < kEstimatorMinSamplesPerCycle), 2.0202 < 2.5); the qualifying population is first enumerated at :3352 and :3357 — 87 lines BELOW the restriction"
        status: pass
      - kind: unit
        ref: "twelve of the 29 pitch rows are driven past the ceiling and carry NO pitch assertion at all; the rho = 0.20 control fires 28/55/110 times and the estimator still returns its negative sentinel, asserted at :3550-3551"
        status: pass
    human_judgment: false
  - id: D5
    description: "The v/oct law survives hard sync, at this file's UNWIDENED tolerance"
    requirement: "PITCH-04"
    verification:
      - kind: unit
        ref: "the unison rows (rho = 1.00, 9 cells) against the libm reference at kTrackingToleranceCents = 0.05, unchanged. MEASURED worst 0.006936 cents — a 7.2x margin, better than the primary tier's own 5.17x"
        status: pass
    human_judgment: false
  - id: D6
    description: "The DISCRIMINATING pitch claim: the output period is the MASTER's, on a population stated on a measured physical criterion"
    requirement: "PITCH-04"
    verification:
      - kind: unit
        ref: "18 master-locked cells at rho 0.50 and 0.75, worst 0.020443 cents against the master, pinned at kSyncLockToleranceCents = 0.10. The un-synced alternative is asserted at least 100 tolerances away; MEASURED smallest separation 498.0449 cents"
        status: pass
      - kind: other
        ref: "the rho window [0.320, 1.310] was MEASURED over a 0.005-step sweep at both master rates and all three rates before the population was chosen; two out-of-window controls (rho 0.20 and 1.60) assert what the instrument does outside it"
        status: pass
    human_judgment: false
  - id: D7
    description: "The phase's NEW DIVISOR is exercised behind the pitch input as well as in the core suite"
    requirement: "PITCH-04"
    verification:
      - kind: unit
        ref: "five of the nine sync rows are divisor cases: idling above the threshold (cannot fire, 0/0 unreachable), a held not-a-number cable (cannot fire), a sample EXACTLY on the 1.0 V threshold (quotient exactly 1.0), a not-a-number GLITCH stored against a legitimate master (quotient NaN/NaN), and LOW-then-steady (fires exactly once). All 783 cells finite, bounded, accumulator in range"
        status: pass
    human_judgment: false
  - id: D8
    description: "The case states its own scope and its own limits in the source"
    verification:
      - kind: unit
        ref: "a 114-line banner at tests/test_vco_pitch.cpp:2829-2942 in five ordered items; the three neighbouring claims named BY TITLE and every title checked to resolve as a selector (5286 / 32 / 576 assertions), plus a fourth sibling (639)"
        status: pass
    human_judgment: false
  - id: D9
    description: "Nothing shipped moved"
    verification:
      - kind: unit
        ref: "make test 109 cases / 2,638,713 assertions 0 failures; six LFO goldens byte-identical (9 cases / 49,188); check_frozen.sh PASS; make strict and make guards exit 0; zero compiler warnings; git diff --name-only across both commits is tests/test_vco_pitch.cpp alone; src/AnalogLFO.cpp absent"
        status: pass
    human_judgment: false

# Metrics
duration: 40min
completed: 2026-09-02
status: complete
---

# Phase 33 Plan 09: PITCH-04's Third Input Class Summary

**Phase 31 marked PITCH-04 complete on two of the three input classes its own text names, and wrote down exactly why it would not close the gap by argument: *"the clamp sits downstream of the frequency, so a sync-driven pitch source cannot bypass it STRUCTURALLY — and that structural argument is exactly the kind of forward claim this phase has repeatedly declined to make on another phase's behalf."* That argument is now a MEASUREMENT. On all 783 cells of the new grid, `forge::VcoCore::Telemetry::freqHz` under every hostile sync shape is EXACTLY EQUAL — by float `==`, not a tolerance — to the same core's value with the jack unpatched. Sync does not move the clamped frequency by one bit, and that is observed rather than reasoned.**

**And it is observed with the detector firing.** The plan's load-bearing half was that a re-confirmation on a path where sync never fires is worth nothing. The firing count is asserted per cell in **both** directions — six sync shapes must fire, three provably cannot — and both assertions were proved able to fail before this SUMMARY was written: unpatching the grid reds **522** of them.

**No requirement is ticked by this plan.** PITCH-04 was already `[x]` from Phase 31 and remains so; SYNC-02 is untouched; `.planning/REQUIREMENTS.md` was checked against disk, not assumed.

## Performance

- **Duration:** 40 min
- **Tasks:** 2 of 2
- **Files modified:** 1

## Task Commits

| # | Task | Commit | Type |
|---|---|---|---|
| 1 | PITCH-04's third input class, with sync observed firing (D-12) | `ccf8b22` | test |
| 2 | State what the re-confirmation does and does not establish (D-12) | `13a9c2d` | docs |

## Files Created/Modified

- `tests/test_vco_pitch.cpp` — invariant 10 and its 114-line banner, a `SYNC SUPPORT (plan 33-09)` helper block, and the file's header invariant list extended by one entry. **No other file in the repository was touched by either commit.**

## Suite Totals, Before and After

| | Test cases | Assertions |
|---|---|---|
| Before plan 33-09 (33-08's recorded totals) | 108 | 2,632,235 |
| After plan 33-09 | **109** | **2,638,713** |
| Delta | **+1** | **+6,478** |

### Per-selector counts, matched-case count confirmed non-zero first

| Selector | Cases | Assertions |
|---|---|---|
| `vco pitch: (PITCH-04*` | **1** | **6,478** |
| …its subcase `the three input classes*` | — | **6,266** |
| …its subcase `the PITCH claim*` | — | **212** |

6,266 + 212 = **6,478**, exactly the suite delta. Nothing else moved.

**PER LEG, and the honest scope of that phrase.** Every figure in this SUMMARY is from the **local Apple-clang leg** (`-std=c++17 -O2 -ffp-contract=off`). `make strict` passes locally at C++11 `-pedantic-errors`, which is a *syntax* leg and measures no behaviour. **The CI MinGW leg has not run on this commit and remains plan 33-11's**, so T-33-08 is not discharged here. Two of the constants landed by this plan (`kSyncLockToleranceCents` and the two `kMeasuredLockRho*` figures) have never been measured on another toolchain; their cushions are stated below for exactly that reason.

**Run cost:** the whole suite went from 6.62 s to 7.01 s. The new case alone is **0.44 s** for 4,542,750 core steps.

---

# TASK 1 — THE THIRD INPUT CLASS

## The grid

**783 cells** = 29 pitch/FM rows × 9 sync shapes × 3 sample rates, at 4000 steps each — **3,132,000 core steps**.

The pitch/FM axis is **invariant 9's own population, row for row** (all 26 of them, including both D-22 bound pins and the three non-finite-attenuverter rows), plus **invariant 8's own three boundary points** — one volt below the derived clamp ceiling, one volt above, three volts above. That is deliberate: this is the *same* two input classes with a third axis, not a fresh selection that happens to look similar, and the clamp points are there because a re-confirmation of a clamp has to include the places where the clamp is the thing being observed.

The `expect` column is **stated per row, not recomputed from the core's summation**. Deriving it here from `pitchCV + fmVolts * fmAtten` would be TRAP 2 — a mirror of the code under test, green even if that code were replaced by a constant.

## The sync axis, and what each shape is for

| # | Shape | Can it fire? | Why |
|---|---|---|---|
| 0 | the SYNC jack **UNPATCHED** | **no** | invariant 9's own evidence, preserved as a ROW rather than replaced |
| 1 | a legitimate master at 110 Hz | yes | a normal cable |
| 2 | a legitimate master at 55 Hz | yes | a normal cable an octave down |
| 3 | a master at **70 kHz — far above the sample rate** | yes | the aliased-master case |
| 4 | **idling at a steady 5 V from sample zero** | **no** | the trigger's `UNINITIALIZED` arm sees ≥ 1.0 V, goes HIGH and returns **false**; it never returns to LOW, so it never fires and **the divisor is never evaluated** |
| 5 | a **held not-a-number** cable voltage | **no** | every comparison against a NaN is false, so the trigger cannot leave `UNINITIALIZED` |
| 6 | a sample landing **EXACTLY on the 1.0 V threshold** | yes | the quotient is **exactly 1.0**, the open end of the `[0,1)` contract |
| 7 | a **not-a-number GLITCH** on an otherwise legitimate master | yes | the NaN is **stored** unconditionally, and the next legitimate edge divides `(1 − NaN)/(v − NaN)` |
| 8 | **LOW, then a steady 5 V** held against the store | yes, **once** | the stale-store case: it fires on the transition, then holds 5 V against a HIGH trigger forever |

**Rows 4-8 are the NEW DIVISOR rows.** Row 7 is the one that actually drives a not-a-number into `f = (1 - prevSyncVolts) / (syncVolts - prevSyncVolts)`; row 5 cannot, and the difference between them is worth having in the file, because "a NaN cable voltage" reads like one case and is two.

**Row 4 is the physical case behind the core's own claim that a 0/0 quotient is UNREACHABLE**, and the measurement agrees with the header: the trigger goes HIGH from `UNINITIALIZED` without firing, so the store and the trigger can never be in the configuration a 0/0 needs.

## The firing counts, MEASURED — the numbers the plan asked to be recorded per cell

Over 4000 steps, as (minimum … maximum) across all **87 cells** of each row (29 pitch/FM rows × 3 rates):

| Sync row | min | max |
|---|---|---|
| the jack UNPATCHED | **0** | **0** |
| musical master 110 Hz | **5** | **10** |
| musical master 55 Hz | **3** | **5** |
| master far above the sample rate | **84** | **167** |
| idling at a steady 5 V | **0** | **0** |
| not-a-number cable, held | **0** | **0** |
| exactly on the high threshold | **62** | **62** |
| not-a-number glitch | **5** | **10** |
| low then steady 5 V | **1** | **1** |

**Every must-fire row fires on every one of its 87 cells; every cannot-fire row fires on none of them.** The counts vary with rate because the block is a fixed 4000 samples and the masters are fixed in hertz — 4000 samples is 90.7 ms at 44.1 kHz and 41.7 ms at 96 kHz.

## Why BOTH directions are asserted

A firing counter that is only ever checked for "greater than zero" has not been shown to be counting the right thing — a counter wired to `syncConnected` rather than to `tel.syncFired` would pass that check on all six must-fire rows *and* on the three that cannot fire. Asserting the zeros is what makes the non-zeros mean something.

## THE ASSERTION DEFERRED ITEM 11 ASKED FOR

```cpp
bool telemetryUnmovedBySync = true;
if (!(telFreq == unpatchedTelFreq)) telemetryUnmovedBySync = false;
```

`tests/test_vco_pitch.cpp:3233`. **Exact float equality, never a tolerance.** For each pitch point at each rate, the unpatched row's clamped frequency is captured on the first pass of the inner loop and every one of the eight patched shapes is compared against it.

**Item 11's own words are quoted in the banner** so a reader sees both the argument and the fact that it was replaced by a measurement rather than adopted.

## The clamp classification, and a near-miss caught by measuring first

| Class | Cells | Assertion | Measured |
|---|---|---|---|
| driven **above** the ceiling | 12 rows | `telFreq == expectedMaxFreq` **exactly** | 21829.5 / 23760.0 / 47520.0 Hz, identical on all nine sync shapes |
| driven **below** the ceiling | 14 rows | `telFreq < expectedMaxFreq` **strictly** | — |
| a sanitised **not-a-number** path | 3 rows | `telFreq < 1.f` | **1.418275e-17 Hz** at every rate and every sync shape |

> **The obvious reading of the third row would have been `== 0.f`, and it is WRONG.** The sanitised NaN path does not floor to zero — it lands at `0x1.05a028p-56`, about 1.4e-17 Hz. Written as an equality against zero, **81 of the 783 cells would have gone red on correct behaviour.** It was caught by running the whole grid out of tree and looking at the numbers before any assertion existed, which is the habit plan 33-08 established when its own "physically obvious" threshold turned out to have a boundary gap of one part in a million. The assertion landed is `< 1.f` — seventeen orders of magnitude of margin, and a statement that means something: **the not-a-number path does not produce an audible frequency.**

## The envelope, and the scope of the claim

Grid-worst `|out|` = **5.000000 V** against this file's `kPitchLooseBoundV = 6.0f`.

> **AND THAT IS A SCOPED CLAIM, STATED RATHER THAN LEFT TO BE DISCOVERED.** This grid holds `morph = 0.50, character = 1.00` — invariant 9's own shape point — and 5.000000 V is the worst excursion **there**. Plan 33-08 measured the sync class reaching **8.218569 V** at other shape centres, which is above this file's outer tier. **The bound asserted here is not a phase-wide sync envelope claim and must not be cited as one.** Moving this grid's morph or character without re-measuring would turn it red on correct behaviour.

## PROVED ABLE TO FAIL — two experiments, both run

**Experiment A — unpatch the whole grid** (`base.syncConnected = false` everywhere in the working tree):

| | Result |
|---|---|
| **`CHECK(firingAsClassified)` at :3234** | **RED — 522 failures**, exactly 6 must-fire rows × 29 pitch rows × 3 rates |
| Also red | **1** `REQUIRE(nUp >= 16)` in subcase two — see the reported deviation below |
| Suite-wide | 1 case red, **523** failed assertions |
| After restoring from a pristine copy | **byte-identical**, 109 cases / 2,638,713 assertions / 0 failures |

**Experiment B — a sync → frequency coupling in the SHIPPED header.** `if (in.syncConnected) freq *= 1.0001f;` inserted immediately above the clamp in `src/dsp/VcoCore.hpp`:

| | Result |
|---|---|
| **`CHECK(telemetryUnmovedBySync)` at :3233** | **RED — 384 failures** (the 16 pitch rows whose frequency is not already pinned at the ceiling or floored to silence, × 8 patched shapes × 3 rates) |
| Collateral, expected | `vco spectrum: (D-06)` 420 + 4; `vco sync: (SC-3 / D-10)` populations 3 |
| After restoring | `git diff src/dsp/VcoCore.hpp` **empty**; suite green at unchanged totals |

**Experiment B is the one that matters for deferred item 11.** It is the defect class the assertion exists for — a sync input that reaches the frequency chain — and the assertion detects it.

---

# TASK 1, SECOND HALF — THE PITCH CLAIM, WHERE THE INSTRUMENT CAN SEE

## The restriction, stated on the instrument's blindness, ABOVE the enumeration

| | Line |
|---|---|
| The restriction, on `estimateFreqRising`'s known limitation | **3253** (banner) |
| **Its assertion** | **3265** — `CHECK(samplesPerCycleAtCeiling < kEstimatorMinSamplesPerCycle)` |
| The first enumeration of qualifying cells | **3352** (`SYNC_MASTER_HZ`) and **3357** (`SYNC_LOCK_RHO`) |

**87 lines of separation, and the restriction is arithmetic rather than an opinion:** a frequency pinned at `kVcoNyquistGuardFrac × sampleRate` is `1 / 0.495 = 2.020202` samples per cycle at **every** rate, which is below the measured 2.5 cutoff. So **twelve of the 29 pitch rows are driven past the ceiling and carry no pitch assertion at all** — not because they produced awkward numbers, but because the ruler provably cannot read there.

**And the other half of the same honesty is in the source.** Band-limiting (Phase 32) made this estimator's narrow-pulse tracking measurably worse at high pitch; that was measured to be a property of the estimator's *sampling* rather than a defect in the DSP, and **hard sync is not blamed for it here either**. The smallest samples-per-cycle count anywhere in this subcase's population is **200.45** — eighty times the cutoff. Nothing is asserted where that effect lives.

## The physical criterion, MEASURED before the population was chosen

Hard sync replaces the slave's period with the master's when the reset arrives before the slave has finished a second cycle **and** after the slave has passed its own rising zero crossing. Writing ρ = slaveHz / masterHz, that is a window in ρ, and it was swept in steps of **0.005** at both master rates and all three sample rates:

| | ρ |
|---|---|
| **The lock region** | **[0.320, 1.310]**, identical at every rate and both master rates |
| Below it | the slave never reaches its own rising crossing between resets; the estimator returns its **negative sentinel** |
| Above it | a second crossing appears inside each master period; the reading walks toward **twice** the master rate |

> **THE EDGES ARE SHARP, NOT A GAP — ρ = 0.315 does not lock and ρ = 0.320 does — AND THAT IS RECORDED RATHER THAN SMOOTHED.** Plan 33-08 measured a threshold in this phase whose physically obvious value had a boundary gap of one part in a million and would plausibly have gone red on another toolchain; it moved that threshold into a measured 0.28-wide gap. **There is no gap to move into here.** The answer to a knife edge is *margin*, not a tighter pin: this subcase's ρ values are 0.50, 0.75 and 1.00, which sit **0.18** inside the lower edge and **0.31** inside the upper one, and both margins are asserted in the source.

## The window length, measured rather than chosen

Worst |cents| against the master over the whole population, at three window lengths:

| window | 0.25 s | **0.50 s** | 1.00 s |
|---|---|---|---|
| worst \|cents\| | 0.148484 | **0.020443** | 0.015707 |

**Half a second is the knee** — doubling it again buys 0.005 cents. Taking the quarter-second window would have forced a tolerance three times looser for no reason other than a shorter run.

## The two halves of the pitch claim, and only one of them discriminates

### (a) UNISON — the v/oct law survives hard sync

ρ = 1.00, nine cells. Measured against the **libm reference** at this file's own `kTrackingToleranceCents = 0.05`, **unwidened**.

| | Value |
|---|---|
| Worst \|cents\| over the nine cells | **0.006936** |
| Margin against the file's tolerance | **7.2×** — better than the 5.17× the primary tier records for itself |

> **AND THIS ROW IS EXPLICITLY NOT DISCRIMINATING FOR SYNC, WHICH IS SAID IN THE SOURCE.** An unpatched slave at the same volts reads the same frequency, so this row alone cannot evidence that sync did anything. It is a **non-regression** claim — hard sync does not break the pitch law at unison — and the firing assertion beside it is what carries the sync half.

### (b) MASTER-LOCKED — the output pitch IS the master's

ρ = 0.50 and 0.75, eighteen cells.

| | Value |
|---|---|
| Worst \|cents\| against the master | **0.020443** |
| Pinned `kSyncLockToleranceCents` | **0.10** |
| Cushion | **4.89×** |
| Smallest un-synced separation, MEASURED | **498.0449 cents** |
| …in units of the pinned tolerance | **4,980×** |

**The separation is what makes this discriminating, and it is ASSERTED rather than described:** an unsynced slave at these volts would read `reference`, which is ρ of the master — 1200 cents away at ρ = 0.50 and 498 at ρ = 0.75. `CHECK(separationCents > 100.0 * kSyncLockToleranceCents)` runs on every cell. **There is no reading that satisfies both.**

### The tolerance's derivation, two-sided

`kSyncLockToleranceCents` is a **separate constant and the file's v/oct tolerance was not widened by one digit**, because this measures a different quantity: the *master's* period recovered through a reset discontinuity, not the frozen exponential's accuracy.

- **(a)** at or above the measured worst, **0.020443 cents** over all 18 cells;
- **(b)** strictly below the **one cent** PITCH-01 actually asks for, so passing it is a musically meaningful statement rather than a bound wide enough to admit an audible error.

Admissible interval **[0.020443, 1.0)**. Pinned at **0.10** — 4.89× above the measurement (the Apple-clang-only cushion this phase has been spending explicitly: 33-07 spent 5×, 33-08 spent 2.38×, both said so) and 10× under PITCH-01. **Both sides are asserted in the source**, in the shape 33-08 used to reject its analytic bound:

```cpp
CHECK(kSyncLockToleranceCents > kTrackingToleranceCents);   // deliberately LOOSER, never a widening of it
CHECK(kSyncLockToleranceCents < 1.0);                       // and still strictly inside what PITCH-01 asks for
```

## The two out-of-window controls

Without these the ρ window is just a set of values that happened to work.

| Control | Fires? | What the instrument does | Asserted |
|---|---|---|---|
| **ρ = 0.20** (below) | **yes — 28 / 55 / 110 times** | `nUp = 0` at every rate and master; the estimator returns its **negative sentinel** | `nUp < 2` and `measured < 0.0` |
| **ρ = 1.60** (above) | yes | locks to **twice** the master; departure from an exact octave 0.003 … 3.99 cents | `\|cents vs 2×master\| < 10.0` |

> **The ρ = 0.20 control is the whole of this subcase's discipline in one row.** The detector fires between 28 and 110 times per block and the estimator still cannot read the result. Sync works; the ruler does not. **No pitch claim is made there**, and the row exists so that absence is a stated measurement rather than a silent omission.

---

# TASK 2 — THE BANNER

**114 comment lines**, `tests/test_vco_pitch.cpp:2829-2942`, in the five items the plan required and in that order:

| Item | Lines | Content |
|---|---|---|
| **(1)** | **2836-2844** | PITCH-04 quoted in the requirement's own words, and **Phase 31** named as the phase that ticked it |
| **(2)** | **2846-2862** | Phase 31 **deferred item 11**, quoted verbatim including its refusal to lean on the structural argument, and its Resolve-at naming Phase 33 |
| **(3)** | **2864-2886** | what the case asserts — finiteness, the outer tier, the accumulator range, the clamp classification, the item-11 equality, the firing counts, and the tracking accuracy on the estimator-valid subset |
| **(4)** | **2888-2933** | what it deliberately does **NOT** assert, with each neighbour named by title |
| **(5)** | **2935-2941** | the non-vacuity condition in one sentence, with the two measured red counts |

## The three neighbours, named by title — and every title CHECKED to resolve

| Named claim | Selector that resolves | Cases | Assertions |
|---|---|---|---|
| the sync correction's **spectral quality** | `vco spectrum: (SYNC-02 / D-11) the sync alias floor*` | **1** | **5,286** |
| the **per-sample step** across a reset | `vco sync: (SC-3 / D-10) the per-sample step*` | **1** | **32** |
| its **anti-circularity** half | `vco sync: (SC-3 / D-10) the corrected reset delta*` | **1** | **576** |
| *(sibling, not a boundary)* the divisor in the core suite | `vco sync: (D-12) the new divisor*` | **1** | **639** |

> **A CRITERION'S MECHANISM CAUGHT BEFORE IT WAS TRUSTED — the twelfth instance in this project.** Task 2's criterion is that *"each named title resolves: running each as a selector reports a non-zero matched case count"*. Run as the **full title**, the spectral neighbour matches **ZERO cases**: its title contains a **comma**, and doctest treats a comma inside `-tc=` as a **filter separator**, so the title splits into two filters and neither matches anything. The criterion is satisfied by the wildcard prefix form, and **the trap is written into the banner beside the title** rather than quietly worked around, because a selector that silently matches nothing is exactly the failure this project keeps logging.

## The fourth neighbour, and the divisor rationale applied rather than inherited

The banner names `vco sync: (D-12) the new divisor cannot poison the phase accumulator` as a **sibling**, and states why the divisor is exercised in both places: a new division appearing behind a new input field is the rationale that moved a hostile grid into the preceding phase under that phase's D-15 — **and that rationale was itself a correction of a falsified one**, so it is re-argued here rather than copied. What the core suite cannot say is what this case says: that the divisor's hostile inputs cannot move the **pitch**.

## The file's own invariant list

The header's numbered list said *"A later plan that adds one appends as invariant 10 and renumbers nothing here."* Invariant 10 was appended, **nothing was renumbered**, and the note now reads that plan 33-09 did so and that a later plan appends as 11. That is the only edit anywhere above line 2713 other than the two-line list preamble — the whole-plan diff is otherwise a pure insertion.

---

# Gate Results

| Gate | Result |
|------|--------|
| `make test` | **PASS** — 109 cases, 2,638,713 assertions, 0 failures |
| `make strict` (C++11, `-pedantic-errors`) | **PASS**, exit 0 |
| `make guards` | **PASS**, exit 0 (all three scripts) |
| `bash tests/check_frozen.sh` | **PASS** — D-05 manifest + goldens + negative control |
| Six shipped-LFO goldens | **byte-identical** — 9 cases / 49,188 assertions, 0 failures |
| Compiler warnings in the changed TU | **0** (`-Wall -Wextra`, verified by `-fsyntax-only`) |
| `src/AnalogLFO.cpp` in the whole-plan diff | **absent** (grep count 0) |
| `git diff --name-only` across both commits | **`tests/test_vco_pitch.cpp` alone** |
| `tests/check_includes.sh` diffstat | **empty** — no new translation unit |
| Four frozen shared headers in the diff | **none** |
| Whole-plan diffstat | **857 insertions, 2 deletions** — four hunks, three of them pure insertions |

**`.planning/REQUIREMENTS.md` was CHECKED against disk, not assumed:** line 18 `- [x] **PITCH-04**` and line 122 `| PITCH-04 | Phase 31 | Complete |` — **unchanged, and unchanged by this plan on purpose**; line 39 `- [x] **SYNC-01**` / line 134 `Complete`; line 40 `- [ ] **SYNC-02**` / line 135 `Pending`. **SYNC-02 was not touched.**

---

# Decisions Made

1. **A NEW `TEST_CASE` IN THE SAME FILE, NOT AN EXTENSION OF INVARIANT 9 — and this is a deliberate departure from Task 1's literal wording, reported rather than silently taken.** Task 1 says *"Extend the existing hostile pitch and FM case… Do not add a separate case: splitting one requirement's evidence across two files is the alternative D-12 rejects by name."* **The stated reason does not support the instruction.** D-12's actual rejection is *"a dedicated separate case (splits one requirement's evidence across two files)"* — a claim about **files**, aimed at putting the case in `tests/test_vco_core.cpp`. A new case in `tests/test_vco_pitch.cpp` splits nothing. Meanwhile the literal instruction is **incompatible with three other parts of the same plan**: its artifacts list says this plan produces *"the `vco pitch: (PITCH-04 / D-12) …` TEST_CASE"*; its acceptance criterion requires `-tc="vco pitch: (PITCH-04*"` to report a non-zero matched case count (invariant 9's title begins `vco pitch D-14 / D-22 …` and matches it not at all); and Task 2 requires *"the case's own title and banner"* to state which requirement it discharges. **Renaming invariant 9 to satisfy the selector was considered and rejected outright**: its title carries the hard-won honest label *"this case is explicitly NOT the RED"*, and it also holds D-14's arithmetic margin and both of PITCH-05's non-regression pins. Retitling it around PITCH-04 would mis-file three requirements' evidence to satisfy a wildcard. **What was landed honours D-12's real constraint** — one requirement, one file, immediately below its two existing invariants, driving invariant 9's own population — **and D-12's second rejection too**, since the pitch claim is *not* folded into a grid that asserts only finiteness and boundedness.

2. **The pitch claim is split into a half that discriminates for sync and a half that does not, and the non-discriminating half says so.** The unison row would pass with the jack unpatched; it is a non-regression claim about the v/oct law and is labelled as one in the source. The master-locked rows cannot pass without the reset, and their separation from the un-synced alternative is asserted per cell rather than described. **Publishing the unison row as evidence that sync works would have been the vacuity this plan exists to prevent.**

3. **`kTrackingToleranceCents` was NOT widened, and the second tolerance is asserted to be looser than it.** The file's own rule is that its tolerance does not move — *"a tolerance that moves with the measurement is a gate wider than the prose it encodes, and this project has been bitten four times in one phase by exactly that."* The master-lock measures a different quantity, so it gets its own constant, with `CHECK(kSyncLockToleranceCents > kTrackingToleranceCents)` in the source so it can never be read as, or quietly merged into, the v/oct figure.

4. **The window length was measured at three values before one was chosen.** 0.25 s would have forced a tolerance three times looser; 1.00 s buys 0.005 cents for double the run. The table is in the source next to the constant, so the choice is auditable rather than arbitrary.

5. **The ρ window's SHARP edges are recorded rather than smoothed, and the population is placed with margin instead.** A 0.005-step sweep found no gap at either edge. Following 33-08's finding — that a physically obvious threshold can sit one ulp from being a coin flip — the response here was margin (0.18 and 0.31, both asserted) plus two out-of-window controls, not a tighter pin.

6. **The `kClampSilent` expectation was MEASURED, not written from the obvious reading.** `== 0.f` would have redded 81 of 783 cells on correct behaviour. The sanitised NaN path lands at 1.418275e-17 Hz.

7. **NO REQUIREMENT IS TICKED BY THIS PLAN, AND PITCH-04 IS NOT RE-TICKED HERE.** PITCH-04 has been `[x]` since Phase 31 and stays `[x]`; what this plan changes is not its checkbox but the evidence under it. The plan's own acceptance criteria say the re-tick decision *"belongs to the phase gate and depends on this evidence being non-vacuous"* — that is **plan 33-11**, and it now has, for the first time, a measured basis: the third input class exists, the detector is observed firing on 522 assertions' worth of cells, and the structural argument deferred item 11 refused to lean on has been replaced by an exact-equality measurement. **SYNC-02 was not touched.** Its remaining gap is 33-08's register item 1 — the residual-versus-intended-step separation — which nothing in this plan measures and which this plan does not claim to.

---

# Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] `syncMasterVolts` was missing its `kSyncStaleStore` arm, and the compiler warning that said so was not read**

- **Found during:** Task 1, on the first `make test` — 87 red at `CHECK(firingAsClassified)`, one full sync row at zero fires across all 29 pitch rows and all three rates.
- **Issue:** the `switch` in `syncMasterVolts` handled six of the seven `SyncKind` enumerators; `kSyncStaleStore` fell through to the trailing `return 0.f;`, so that row presented a constant zero volts and never fired. **`-Wall -Wextra` DID emit `warning: enumeration value 'kSyncStaleStore' not handled in switch [-Wswitch]` on that build** — verified afterwards by reconstructing the defect and recompiling. It was missed because the build output was read with `tail`, which shows the doctest summary and hides everything above it.
- **Fix:** the missing arm added, with the comment the other six carry.
- **Process note worth more than the bug:** this project holds a standing *"zero compiler warnings in the changed TU"* gate, and that gate is only as good as whether anyone reads the warnings. `make test` prints them **above** the doctest summary, so any `tail`-based check swallows them silently. The warning-free state of the committed code was re-verified explicitly with `-fsyntax-only`, not by `tail`.
- **Files modified:** `tests/test_vco_pitch.cpp`
- **Verification:** all nine sync rows at their classified firing counts; suite green.
- **Committed in:** `ccf8b22`

**2. [Rule 2 — Missing correctness] Diagnostics for a 783-cell grid were unreadable, because doctest renders a `const char*` capture as its address**

- **Found during:** Task 1, reading the 87-failure output of deviation 1 — every row name printed as `role := 0x100f45e5c`.
- **Issue:** invariant 9 uses `CAPTURE(role)` with a `const char*`, and doctest prints the pointer. With 78 cells that is survivable; with 783 cells across two named axes it makes a red unidentifiable, which defeats the purpose of carrying role strings at all.
- **Fix:** the two role strings are **streamed** through `INFO("pitch/FM row: " << role)` instead, which renders the text. `CAPTURE` is kept for the numeric fields. **Invariant 9 was not edited** — it is out of this plan's scope and its own diff must stay clean.
- **Files modified:** `tests/test_vco_pitch.cpp`
- **Verification:** the deviation-1 failure output names both axes in text.
- **Committed in:** `ccf8b22`

### Reported, not fixed

**3. [Reported] Task 1's instruction to extend the existing case contradicts three other parts of the same plan, and its stated reason does not apply — the TWELFTH instance in this project of a mechanism at odds with its own prose**

- **Found during:** Task 1, reconciling the action text against the acceptance criteria before writing any code.
- **Issue:** see Decision 1. *"Do not add a separate case"* is justified by a **file**-splitting argument that a same-file case does not trigger, while the plan's own selector criterion, artifacts list and Task 2 all require a separately-titled case.
- **Fix:** **Reported rather than resolved by retyping either side.** A new case was landed in the same file, driving invariant 9's own population, and both of D-12's real rejections are honoured. Following 33-05's deviation 3, 33-06's deviation 5 and 33-08's deviations 4 and 5, the reasoning is given rather than the verdict.
- **Files modified:** none — a criterion-interpretation decision
- **Verification:** `-tc="vco pitch: (PITCH-04*"` reports 1 case / 6,478 assertions; invariant 9's title, body and assertion count are untouched.
- **Committed in:** n/a

**4. [Reported] Task 1's description of the estimator's blindness is BACKWARDS, and applying it literally would have selected the blind cells**

- **Found during:** Task 1, deriving the qualifying population.
- **Issue:** the plan says to assert *"on the subset of cells where the slave's frequency is **high enough above** the estimator's blindness threshold"*. The estimator's threshold is expressed in **samples per cycle** (`kEstimatorMinSamplesPerCycle = 2.5`), and it is blind where samples per cycle are **few** — that is, where the **frequency is HIGH**. Taken literally, the instruction selects exactly the cells at the Nyquist ceiling, which are the twelve rows where the estimator returns a plausible wrong answer on a perfectly correct oscillator.
- **Fix:** **Reported and applied in the correct direction.** The restriction is written on the quantity the file's own constant is expressed in — samples per cycle — and asserted as arithmetic from `forge::kVcoNyquistGuardFrac`.
- **Files modified:** none — a criterion-interpretation decision
- **Verification:** `CHECK(samplesPerCycleAtCeiling < kEstimatorMinSamplesPerCycle)` at :3265; smallest samples-per-cycle in the asserted population 200.45.
- **Committed in:** n/a

**5. [Reported] The "unpatch the whole grid" acceptance experiment reds the firing assertion 522 times in subcase one but NOT in subcase two, where an earlier precondition aborts first**

- **Found during:** Task 1's acceptance experiment.
- **Issue:** the criterion asks that unpatching *"confirm the case goes RED on the firing assertion specifically."* In subcase one it does, 522 times. In subcase two it does **not reach** the firing assertion: at ρ = 0.50 an unsynced slave produces half as many rising crossings, so `REQUIRE(nUp >= 16)` — the oscillation precondition — fires first and aborts the subcase. That ordering is correct and deliberate (a block the estimator cannot read makes every number after it meaningless rather than merely wrong), but it means the criterion's literal signature is not what the experiment produces.
- **Fix:** **Reported with both signatures, and a second targeted experiment run** rather than the criterion retyped. Unpatching subcase two while holding ρ at 1.00 keeps the crossing count above the precondition and reds `CHECK(fireCount > 0)` on **all 27** of its cells. Both firing assertions are therefore proved able to fail; the numbers are given rather than the verdict.
- **Files modified:** none — restored byte-identical from a pristine copy, verified by `cmp`
- **Verification:** experiment A 522 + 1; experiment B (targeted) 27; restore byte-identical; suite green.
- **Committed in:** n/a

**6. [Reported] The spectral neighbour's case title matches ZERO cases when used as its own selector, because it contains a comma**

- **Found during:** Task 2, checking the criterion *"each named title resolves"* before writing the titles into the banner.
- **Issue:** `vco spectrum: (SYNC-02 / D-11) the sync alias floor stays below its per-cell pinned threshold, and every pinned number reproduces` contains a comma, and doctest treats a comma inside `-tc=` as a **filter separator**. The full title splits into two filters and matches nothing — `test cases: 0 | 0 passed`.
- **Fix:** **Reported, and written into the banner beside the title** along with the wildcard form that resolves. Not worked around silently: a selector that matches nothing is the exact shape of failure this project's register keeps recording, and the next reader of that banner will be the person running it.
- **Files modified:** `tests/test_vco_pitch.cpp` (the banner note)
- **Verification:** the wildcard form reports 1 case / 5,286 assertions.
- **Committed in:** `13a9c2d`

---

**Total deviations:** 2 auto-fixed (1 × Rule 1, 1 × Rule 2) + 4 reported
**Impact on plan:** One was a real bug the compiler had already caught and a `tail` had hidden — the more useful half of it is the process note. One made a 783-cell grid diagnosable. Of the four reported, two are plan-text problems that would have produced either a case that could not be selected or a claim asserted exactly where its instrument is blind; one is an acceptance experiment whose literal signature the correct assertion ordering prevents; and one is a doctest selector trap. **In none of them was an assertion retyped to make a criterion true.** The whole-plan diff is `tests/test_vco_pitch.cpp` alone.

---

# Known Stubs

**None.** Every constant, helper, enum and struct field this plan adds is consumed by an assertion in the same commit.

One thing is *absent by design*:

| Absence | Owner | Why it is not a stub here |
|---|---|---|
| No pitch claim anywhere above the Nyquist clamp ceiling | **nobody, and that is the correct answer** | The rising-crossing estimator is structurally blind at 2.0202 samples per cycle. There is no instrument in this repository that can make a pitch claim there, and inventing one is not what PITCH-04 asks for — the requirement is that the frequency is CLAMPED, which is asserted exactly, on every one of those rows, on every sync shape. |

---

# Deferred Register Items

**1. NEW — the output-envelope claim in invariant 10 is SCOPED to one shape point, and the phase already has a measurement that exceeds it.**
Invariant 10 asserts `|out| <= kPitchLooseBoundV = 6.0 V` and measures **5.000000 V** — but only at `morph = 0.50, character = 1.00`, which is invariant 9's shape point. Plan 33-08 measured the sync class reaching **8.218569 V** at other shape centres. **The two are not in conflict and must not be read as a phase-wide envelope claim either way.** A later plan that sweeps morph or character in this grid will turn it red on correct behaviour, and the right response is to re-derive the tier for that population (as 33-08 did), never to widen `kPitchLooseBoundV`, which four other scenarios in the pitch suite depend on.
**Proposed Resolve-at:** whichever plan next widens invariant 10's shape coverage; noted for 33-11's register.

**2. NEW — the ρ lock window has NO measured gap at either edge, unlike every other classifier this phase has pinned.**
33-07 and 33-08 both placed their population criteria inside measured empty gaps (a 1.44× window and a 0.28-wide one). **Here there is none:** ρ = 0.315 does not lock and ρ = 0.320 does, at a sweep resolution of 0.005. The claim is protected by **margin** (0.18 and 0.31, both asserted) and by two out-of-window controls, which is a weaker guarantee than a gap. If a future change to the waveform's shape moves the lower edge above 0.32, the ρ = 0.50 cells fail — and that would be a finding about the shape, not a reason to move the window.
**Proposed Resolve-at:** plan 33-11, alongside the CI MinGW leg.

**3. NEW — `kSyncLockToleranceCents` and the two `kMeasuredLockRho*` figures are Apple-clang-only and have a 4.89× cushion, not a two-sided measurement.**
The tolerance's lower constraint is a measurement (0.020443) and its upper one is a requirement figure (1.0 cent), so the interval is genuinely two-sided — but unlike 33-08's `kSyncResetDeltaBoundV`, **the upper constraint is not something another toolchain could falsify**, so this bound cannot go red for the reason that one can. Its real protection is the 4,980× separation from the un-synced alternative, which is asserted per cell.
**Proposed Resolve-at:** plan 33-11, on the CI MinGW leg.

**4. CARRIED — every cent, volt and count in this SUMMARY is an Apple-clang figure.**
Unchanged in kind from 33-01 through 33-08. The exposure's shape here: **two new pinned tolerance-scale constants**, **two pinned ρ edges**, and **two pinned population sizes** (29 and 9, both structural rather than measured, so both are safe). `make strict` passes locally at C++11 `-pedantic-errors`; T-33-08 is not discharged locally.

**5. CARRIED — 33-08's items 1, 2 and 3, 33-07's 1, 3 and 5, 33-06's 1, 4 and 5, 33-05's 2/3/5, and 33-02/03/04's six, are unchanged by this plan.**
**33-08's item 1 in particular is untouched and is still the whole of SYNC-02's remaining gap:** the residual-versus-intended-step separation, measured by neither instrument in this phase, with a resolve-at of 33-11. Nothing in this plan measures it and nothing in this plan claims to.

**6. Phase 31 deferred item 11 — the EVIDENCE is landed; the closure is a phase-gate decision.**
Item 11's Resolve-at reads *"Phase 33 (hard sync). That phase must re-confirm the clamp still binds — by adding its sync inputs to invariant 9's hostile grid in `tests/test_vco_pitch.cpp`, or to an equivalent case of its own."* **The equivalent case exists** and it drives invariant 9's own population. Whether that discharges the item is 33-11's call, not this plan's, and the item's own file under `.planning/phases/31-pitch-tuning-exponential-fm/` was deliberately **not edited** — that would be a cross-phase document change this plan has no mandate for.
**Proposed Resolve-at:** plan 33-11, as a phase-gate decision taken alongside PITCH-04's re-tick.

---

# Issues Encountered

- **The plan's own text pointed the pitch claim at the cells where its instrument is blind.** Deviation 4. It was caught only because the estimator's cutoff is expressed in samples per cycle rather than in hertz, so the direction of "above the threshold" had to be worked out rather than read off. That is the second time in this phase a criterion's wording has been narrower than or contrary to its own mechanism, and the twelfth in the project.
- **The obvious expectation for the not-a-number rows would have redded 81 cells on correct behaviour.** `== 0.f` is what the phrase "floors the frequency" suggests; 1.418275e-17 Hz is what the core does. The habit that caught it is the one 33-08 insisted on: run the whole grid out of tree, look at the distribution, *then* write the assertion.
- **A compiler warning that named the defect exactly was on screen and unread**, because the build output was inspected with `tail`. The bug cost one build cycle; the lesson is that this project's zero-warning gate has no teeth if the warnings are scrolled past.
- **`check_canary.sh [2b/5]` remains measurement-blind on this host** (33-03's finding). Irrelevant here — this plan adds no shipped code — but `make guards` going green was again treated as evidence of nothing about behaviour.
- **Two untracked `.planning/research/.cache/*.json` files** pre-existed at session start, as they have since 33-02, and were left alone.

---

# Next Phase Readiness

**PITCH-04 has a third input class, the detector is observed firing behind it, and the argument Phase 31 refused to make is now a measurement.**

- **Plan 33-10** is unaffected. It inherits `sweepSyncDeltaGrid` and friends from 33-08 unchanged; nothing in this plan touches `tel.syncCorrection` or the reconstruction relationship.
- **Plan 33-11 inherits the PITCH-04 re-tick decision with evidence for the first time**, plus register items 1, 2 and 3 above and Phase 31 deferred item 11's closure question. It should read Decision 7 before deciding: the checkbox does not move as a result of this plan, and what it can now say is that the tick rests on three input classes rather than two. **It should also note that this plan touched neither SYNC-02 nor 33-08's register item 1**, which remains the phase's only open requirement gap.
- **Plan 33-12 owns the operator UAT**, and it gains one concrete, audible fact from this plan that its expected-results block can use honestly: **at a master/slave ratio between about 0.32 and 1.31 the output pitch is the MASTER's, to within a fiftieth of a cent.** That is something an operator can hear and check — unlike the 2 % step reduction 33-08 warned it not to promise.

**Concerns carried forward:**

- **The envelope claim here is shape-scoped and the phase has a larger measurement elsewhere.** Register item 1.
- **The ρ window has no gap, only margin.** Register item 2 — the first classifier in this phase pinned without one.
- **SYNC-02's remaining gap is untouched and unchanged:** 33-08's register item 1.

---

# Self-Check: PASSED

Verified against disk and git rather than asserted:

- **Files exist:** `tests/test_vco_pitch.cpp`, `.planning/phases/33-hard-sync/33-09-SUMMARY.md` — both **FOUND**.
- **Commits exist:** `ccf8b22`, `13a9c2d` — both **FOUND** in `git log`.
- **The new case is present in `HEAD`** and its selector matches with a non-zero count: **1 case / 6,478 assertions / 0 failures**, split 6,266 + 212 across its two subcases.
- **The suite really did grow:** 108 → **109** cases, 2,632,235 → **2,638,713** assertions, 0 failures; the delta equals the new case's own count exactly.
- **The restriction really is above the enumeration:** banner at **3253**, its assertion at **3265**, the first qualifying-cell enumeration at **3352** — 87 lines below.
- **The firing assertions really are able to fail:** 522 at `:3234` on the unpatched grid, 27 at `:3426` on the targeted probe; the item-11 equality reds **384** times under a shipped-header coupling probe. All three working trees restored and re-verified — `tests/test_vco_pitch.cpp` byte-identical by `cmp`, `git diff src/dsp/VcoCore.hpp` empty.
- **Every named neighbour title resolves:** 5,286 / 32 / 576 / 639 assertions, all with a matched case count of 1 — and the one that does **not** resolve as a full title is recorded as such in the banner.
- **The whole-plan diff is `tests/test_vco_pitch.cpp` alone:** 857 insertions, 2 deletions, four hunks, three of them pure insertions.
- **Nothing shipped moved:** six LFO goldens byte-identical (9 cases / 49,188), `check_frozen.sh` PASS, `make strict` and `make guards` exit 0, **zero** compiler warnings (`-fsyntax-only` with `-Wall -Wextra`), `src/AnalogLFO.cpp` absent from the whole-plan diff.
- **`.planning/REQUIREMENTS.md` was CHECKED, not assumed:** PITCH-04 `[x]` / `Complete` (Phase 31's, unchanged); SYNC-01 `[x]` / `Complete`; **SYNC-02 `[ ]` / `Pending`, not touched.**

---
*Phase: 33-hard-sync*
*Completed: 2026-09-02*
