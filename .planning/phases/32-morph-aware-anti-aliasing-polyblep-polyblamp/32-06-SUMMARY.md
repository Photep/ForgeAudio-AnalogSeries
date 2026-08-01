---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
plan: 06
subsystem: dsp
tags: [vco, band-limiting, polyblep, polyblamp, morph-aware, core-02, morph-01, aa-01, d-08, d-14, d-15, p-3, t-32-01, t-32-18, t-32-19]

# Dependency graph
requires:
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-04 — src/dsp/MorphBlep.hpp, forge::MorphBlep and forge::morphBlepCharFactor"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-05 — the 909-assertion unit suite, and the +infinity dt guard hole it closed in MorphBlep.hpp"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-03 — the D-08 baseline-validity tombstone this plan inverts, and the 90-cell SPECTRUM_GRID"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-02 — the MORPH CV jack that falsified the comparison-ladder parenthetical (T-32-01)"
provides:
  - "forge::VcoCore::blep — a forge::MorphBlep held BY VALUE beside `wave`, the Phase 33 hard-sync seam"
  - "The band-limited call site: 5.f * (naive + correction), the frozen Waveshape still called exactly once"
  - "NaN-safe morph and character conditioning inside the core (T-32-01)"
  - "NaiveVcoCoreMirror's five recording members — lastNaive, lastP, lastDeltaPhase, lastMorph, lastCharacter"
  - "The INVERTED D-08 tombstone: a bit-exact reconstruction proof that the divergence is the correction and nothing else"
affects: [32-07, 32-08, 32-09, 33-hard-sync, 34-output-and-drift]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Reconstruct-rather-than-approximate: prove a divergence by rebuilding the new path from the old one plus the named new component, with a direct float ==, instead of bounding the difference"
    - "Zero-correction control beside a divergence claim: assert the cells where the correction MUST be exactly zero alongside the cells where it must fire, so 'they now differ' cannot be satisfied by a correction that fires indiscriminately"
    - "Lifetime-as-reset: one instrument per grid point, default-constructed in the point's own scope, so the reset cannot be forgotten"
    - "Falsified-premise-corrected-in-place, continued from 32-04 and 32-05: keep the conclusion, replace the reason, and say in the source that the old reason was falsified and by what"

key-files:
  created: []
  modified:
    - src/dsp/VcoCore.hpp
    - tests/test_vco_spectrum.cpp

key-decisions:
  - "The morph/character guard is written as the negated-comparison pair rather than forge::clamp, and the difference from the ladder is NOT limited to a not-a-number: negative zero is normalised to positive zero. MEASURED as moving 0 of 4096 samples for either field, and recorded in the source rather than glossed"
  - "The file banner's 'NAIVE, DELIBERATELY ALIASED' self-description was corrected in place too — a third falsified sentence the plan did not name, found by reading the file the plan told this executor to read in full"
  - "Task 1 was committed with the suite RED on two tombstones by design; that commit is the OBSERVED red the D-08 inversion tombstone asks for, and deleting the red window by combining the two tasks would have removed the evidence"
  - "The plan-32-07 alias-floor tombstone stays red at the end of this plan. It is 32-07's to invert, and softening it here would have been the anti-softening clause's named failure"

requirements-completed: [CORE-02, MORPH-01, AA-01]

coverage:
  - id: D1
    description: "forge::VcoCore holds a forge::MorphBlep BY VALUE beside its Waveshape and returns 5.f * (naive + correction); the frozen Waveshape is still called exactly once and never edited"
    requirement: "CORE-02"
    verification:
      - kind: unit
        ref: "grep: 'MorphBlep blep;' = 1, 'blep.step(wave, phase, p, deltaPhase, morph, character)' = 1, 'wave.morphedWave(p, morph, character, 0.f)' = 1"
        status: pass
      - kind: integration
        ref: "make strict PASS, make guards PASS, make -j4 produces plugin.dylib"
        status: pass
    human_judgment: false
  - id: D2
    description: "The SAME float p is handed to Waveshape::morphedWave and to MorphBlep::step, and the double phase accumulator supplies the distance separately (P-3)"
    requirement: "AA-01"
    verification:
      - kind: unit
        ref: "tests/test_vco_spectrum.cpp#vco spectrum: the core now DIVERGES ... — the reconstruction is driven from the mirror's recorded lastP and its own double phase, and matches bit-exactly on all 45 cells"
        status: pass
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: RESONANT phase increments ... (P-3) — 165 rows, observed detecting a double-sourced side decision"
        status: pass
    human_judgment: false
  - id: D3
    description: "The MorphBlep member is per-instance state held by value, never static and never shared (CORE-03 / D-14 / T-32-18)"
    requirement: "CORE-02"
    verification:
      - kind: unit
        ref: "tests/test_vco_core.cpp#vco core: two-instance independence under sample-by-sample interleaving (D-17) — 1 case, 18 assertions, 0 failed with the new member live"
        status: pass
      - kind: unit
        ref: "tests/test_vco_core.cpp#vco core: independence positive control - a shared static accumulator FAILS the same check (D-17) — still green, so the invariant is still able to fail"
        status: pass
    human_judgment: false
  - id: D4
    description: "A not-a-number morph or character lands at 0.f rather than reaching the frozen (int)(morph * 4.f) cast (T-32-01)"
    requirement: "MORPH-01"
    verification:
      - kind: unit
        ref: "grep -cE 'if \\(!\\(morph > 0\\.f\\)\\)' = 1 and the same for character"
        status: pass
      - kind: other
        ref: "out-of-tree probe: both fields set to a quiet NaN, 4096 samples all std::isfinite, envelope 5.000000 V"
        status: pass
    human_judgment: false
  - id: D5
    description: "The negated pair is bit-identical to the comparison ladder for every finite input except negative zero, which is unobservable downstream"
    requirement: "MORPH-01"
    verification:
      - kind: other
        ref: "out-of-tree probe at C8/44.1 kHz over 4096 samples: morph = -0.f vs +0.f differs on 0 samples; character = -0.f vs +0.f differs on 0 samples"
        status: pass
      - kind: integration
        ref: "make test — every non-tombstone case green, no recorded figure in any suite moved"
        status: pass
    human_judgment: false
  - id: D6
    description: "The VcoCore.hpp:446 oversampled-inner-loop premise is REPLACED with the real reason, and the replacement says the old reason was falsified and names AA-05"
    requirement: "AA-01"
    verification:
      - kind: unit
        ref: "grep -c 'AA-05 forbids oversampling' = 1; grep -ciE 'falsified' = 2 (VcoCore.hpp:392 and :488)"
        status: pass
    human_judgment: false
  - id: D7
    description: "The D-08 baseline tombstone is INVERTED IN PLACE and proves the core and the naive mirror differ by EXACTLY the MorphBlep correction, bit-exactly"
    requirement: "AA-01"
    verification:
      - kind: unit
        ref: "tests/test_vco_spectrum.cpp#vco spectrum: the core now DIVERGES from NaiveVcoCoreMirror by EXACTLY the MorphBlep correction (D-08 inversion) — 1 case, 288 assertions, 0 failed; reconstruction mismatches 0 at all three rates"
        status: pass
      - kind: other
        ref: "OBSERVED RED at commit 728121e: the case failed on 39 of 45 grid points before the inversion landed"
        status: pass
    human_judgment: false
  - id: D8
    description: "The zero-correction control: the sine centre at character 0 shows EXACTLY zero differing samples at every rate, while the saw centre at character 0 shows many"
    requirement: "AA-01"
    verification:
      - kind: unit
        ref: "sineCentreChar0Differing == 0 and sawCentreChar0Differing > 0, asserted per rate; measured 0/0/0 and 778/713/358"
        status: pass
    human_judgment: false
  - id: D9
    description: "The shipped Analog LFO is untouched: no frozen header edited, FROZEN.sha256 unmoved, src/AnalogLFO.cpp absent from both commits"
    verification:
      - kind: integration
        ref: "make guards (check_frozen + check_includes + check_canary) PASS; git show --name-only over 728121e and 05ae9a5 lists only src/dsp/VcoCore.hpp and tests/test_vco_spectrum.cpp"
        status: pass
      - kind: unit
        ref: "the six .f32 LFO goldens replay bit-exact inside make test — the lfo cases are green throughout"
        status: pass
    human_judgment: false

# Metrics
duration: 27 min
completed: 2026-08-01
status: complete
---

# Phase 32 Plan 06: Wire MorphBlep Into VcoCore Summary

**`forge::VcoCore` now band-limits: it holds a `forge::MorphBlep` by value beside its `Waveshape`, returns `5.f * (naive + correction)` with the frozen waveshaper still called exactly once and never edited, conditions `morph` and `character` NaN-safely against the CV jack plan 32-02 added, and carries two corrected premises in place of two falsified ones — and the D-08 tombstone was inverted in place into a bit-exact proof that the divergence from the naive mirror IS the correction, with a reconstruction mismatch count of 0 on all 45 grid points at all three sample rates.**

## Performance

- **Duration:** ~27 min
- **Completed:** 2026-08-01
- **Tasks:** 2
- **Files:** 0 created, 2 modified (`src/dsp/VcoCore.hpp`, `tests/test_vco_spectrum.cpp`)

## Task Commits

1. **Task 1 — band-limit the core, harden morph/character, correct two falsified sentences** — `728121e` (`feat`)
2. **Task 2 — invert the D-08 tombstone into a reconstruction proof** — `05ae9a5` (`test`)

## The Measured Figures This Plan Was Asked To Record

### Reconstruction mismatch counts, per rate

| rate | grid points | reconstruction mismatches |
|---|---|---|
| 44 100 Hz | 15 | **0** |
| 48 000 Hz | 15 | **0** |
| 96 000 Hz | 15 | **0** |

Every one of the 3 × 15 × 4096 = 184 320 samples reconstructs from `5.f * (mirror.lastNaive + localBlep.step(...))` with a **direct float `==`**. Nothing other than `forge::MorphBlep` moved inside `forge::VcoCore::step`.

### Diverging-point counts

| rate | diverging points | bound |
|---|---|---|
| 44 100 Hz | **14** of 15 | ≥ 12 |
| 48 000 Hz | **13** of 15 | ≥ 12 |
| 96 000 Hz | **12** of 15 | ≥ 12 |

### The zero-correction control, and the loudest edge

| rate | sine centre, character 0 | saw centre, character 0 |
|---|---|---|
| 44 100 Hz | **0** | 778 |
| 48 000 Hz | **0** | 713 |
| 96 000 Hz | **0** | 358 |

The saw figures are **exactly 2 per cycle** at 389 / 357 / 179 cycles per block — one wrap edge, each correction spanning the sample containing it and the one after it (D-13). That arithmetic agreeing is an independent second reading that the placement is right.

### The full 44.1 kHz differing-sample grid (out of 4096)

| morph | character 0.00 | 0.50 | 1.00 |
|---|---|---|---|
| 0.00 sine | **0** | 1555 | 1549 |
| 0.25 triangle | 1546 | 1545 | 1500 |
| 0.50 saw | 778 | 1535 | 1497 |
| 0.75 square | 1556 | 1556 | 1555 |
| 1.00 pulse | 982 | 982 | 982 |

### The five cells that do NOT diverge, and why each is correct

1. **Sine centre, character 0, at all three rates.** No discontinuity, no bleed ring, every site magnitude zero. This is the control.
2. **Triangle centre, character 1, at 48 kHz and 96 kHz.** The rounded corner is 0.175 in phase units against a `2*dt` of 0.1743 and 0.0874 — wider than the kernel's own two-sample support, so the D-03 factor returns **exactly** zero. At 44.1 kHz `2*dt` is 0.18994, just wider than the corner, and the same cell diverges on 1500 samples. That rate-ordering is D-03's compact-support cutoff visible as data.
3. **Sine centre, character 1, at 96 kHz.** At morph 0 the bleed ring puts the weight on the **pulse** (the finding recorded in `src/dsp/MorphBlep.hpp`); at character 1 the pulse's hard step is fully `(1-c)`-weighted to zero and its soft edge is 0.16 wide against a `2*dt` of 0.0874 — zero again.

### Doctest counts before and after

| | cases | assertions | failed |
|---|---|---|---|
| plan start | 91 | 2 619 816 | 0 |
| after Task 1 (RED by design) | 91 | 2 619 816 | **45** (2 cases) |
| after Task 2 | **91** | **2 619 879** | **6** (1 case — the plan-32-07 tombstone) |

**The case count is unchanged at 91.** The tombstone was inverted in place, not deleted and not added alongside.

### The exact source lines where each falsified premise was replaced

| premise | file:line | replaced with |
|---|---|---|
| "`forge::clamp` is still the right tool for morph and character further down, where the inputs are already finite" | `src/dsp/VcoCore.hpp:392` | T-32-01: plan 32-02's MORPH CV jack means `in.morph` is no longer guaranteed finite; the exception is withdrawn and both fields move to the negated pair |
| "Phase 32's oversampled inner loop is the obvious future caller that decouples them on purpose" | `src/dsp/VcoCore.hpp:488` | **AA-05 forbids oversampling in v2.0 by name**; the decoupled case matters because this phase put a **divisor** behind `in.sampleTime` for the first time |
| "step() is now a NAIVE, DELIBERATELY ALIASED morphed oscillator" (file banner, **not named by the plan**) | `src/dsp/VcoCore.hpp:7-11` | the core is band-limited as of this commit; the naive path survives only as `NaiveVcoCoreMirror` in the test TU |

## Verification Results

| Check | Result |
|-------|--------|
| `make strict` | `strict C++11 gate: PASS` |
| `make guards` | `guard suite: PASS` — `check_canary.sh` did **not** report "could not perturb src/dsp/VcoCore.hpp"; [5/5] confirms `dsp/MorphBlep.hpp` is carried into both gates |
| `make -j4` | exits 0, produces `plugin.dylib` (169 328 bytes) |
| `./build-test/test -tc="vco core: two-instance independence*"` | 1 case / 1 passed / 0 failed, 18 assertions |
| `./build-test/test -tc="vco pitch*"` | 9 cases / 9 passed / **0 failed**, 1941 assertions |
| `./build-test/test -tc="vco core*"` | 5 cases / 5 passed / 0 failed, 990 assertions |
| `./build-test/test -tc="morph blep*"` + `lfo*` + core + pitch | 30 cases / 30 passed / 0 failed, 3900 assertions |
| `./build-test/test -tc="vco spectrum: the core now DIVERGES*"` | **1 case / 1 passed / 0 failed**, 288 assertions |
| `grep -c 'MorphBlep blep;' src/dsp/VcoCore.hpp` | `1` |
| `grep -c 'blep.step(wave, phase, p, deltaPhase, morph, character)'` | `1` |
| `grep -c 'wave.morphedWave(p, morph, character, 0.f)'` | `1` |
| `grep -cE 'if \(!\(morph > 0\.f\)\)'` / `character` | `1` / `1` |
| `grep -c 'AA-05 forbids oversampling'` | `1` |
| `grep -ciE 'falsified' src/dsp/VcoCore.hpp` | `2` |
| `grep -c 'D-08 inversion' tests/test_vco_spectrum.cpp` | `1` |
| `Approx` in comment-stripped `tests/test_vco_spectrum.cpp` | `0` |
| `git status --porcelain src/dsp/FROZEN.sha256` | empty |
| `src/dsp/Waveshape.hpp`, `src/dsp/RackCompat.hpp`, `src/AnalogLFO.cpp` | absent from both commits |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] The plan's bit-identity claim is not quite true: negative zero is a finite input where the negated pair and the comparison ladder DISAGREE**

- **Found during:** Task 1, while writing the guard's rationale paragraph.
- **Issue:** the plan instructs "State the bit-identity: for every finite input the negated pair returns exactly what the ladder returned, so no existing measurement can move; the two differ only for a not-a-number." That is **false for negative zero**. `forge::clamp(-0.f, 0.f, 1.f)` returns `-0.f` unchanged (`-0.f < 0.f` is false and `-0.f > 1.f` is false); the negated pair's first line, `!(morph > 0.f)`, is **true** for `-0.f`, so it lands at `+0.f`. Writing the plan's sentence verbatim would have put a claim in the source that a reader with a debugger can falsify in thirty seconds.
- **Fix:** the claim was **measured** rather than weakened or dropped. An out-of-tree probe drove two identically seeded `forge::VcoCore` instances for 4096 samples at C8 / 44.1 kHz with `morph = -0.f` against `morph = +0.f`, and again with `character = -0.f` against `+0.f`, comparing with `memcmp`. **MEASURED: 0 differing samples in both directions.** The source now states the exception, states the measurement, and states why it cannot be observed: the consumers are the float-to-int cast (0 for either sign of zero) and multiply/add chains in which `-0.f` and `+0.f` agree.
- **Why this is stronger, not weaker:** the plan's sentence asserted a property; the source now asserts a narrower property **plus** the measurement that shows the gap is inert. A future editor reading it will not be misled into thinking the two forms are interchangeable.
- **Files modified:** `src/dsp/VcoCore.hpp`
- **Committed in:** `728121e`

---

**2. [Rule 2 — Missing Critical] A THIRD falsified sentence, in the file banner, that the plan did not name**

- **Found during:** Task 1, reading `src/dsp/VcoCore.hpp` in full as `<read_first>` requires.
- **Issue:** the banner's opening paragraph describes `step()` as "now a NAIVE, DELIBERATELY ALIASED morphed oscillator … and one call into the FROZEN forge::Waveshape::morphedWave, scaled x5 and returned unconditioned", and the phase-ownership list says "Phase 32 … **owns** band-limiting … so NO assertion over this body may claim anything about spectral cleanliness". Both sentences became false at the moment Task 1's call-site change landed. Leaving a file's own self-description contradicting its body is the exact failure the house rule about falsified premises exists to prevent, and the plan's silence about the banner is not permission to leave it wrong.
- **Fix:** both corrected in place in the same voice as the other two. The banner now says the core is band-limited as of this commit, records that the sentence **used to** read "NAIVE, DELIBERATELY ALIASED", and names where the naive path survives (`NaiveVcoCoreMirror` in the test TU — there is no flag, no second entry point and no naive branch in the body). The ownership list now says Phase 32 has **delivered** band-limiting and still owns the alias floor itself, with the threshold re-pinning pointed at plan 32-07.
- **Files modified:** `src/dsp/VcoCore.hpp`
- **Committed in:** `728121e`

---

**3. [Rule 3 — Blocking] Task 2's acceptance criterion "`make test` exits 0 with 0 failures" is unsatisfiable inside this plan**

- **Found during:** Task 1's first full run.
- **Issue:** landing `forge::MorphBlep` turns **two** tombstones red at once. The first, the D-08 baseline-validity tombstone, is this plan's to invert and Task 2 inverts it. The second, `"vco spectrum: TOMBSTONE - the NAIVE core FAILS the Phase 32 alias-floor gate (D-08 RED evidence) - INVERTS IN PLAN 32-07"`, is red for the **opposite** reason after Task 1: its `CHECK(failing >= 27)` now sees **2**, and its five named subset cells now sit **below** their thresholds. That case's own banner says in capitals that **plan 32-07** flips those lines, and this plan's brief says "32-07 owns the green alias-floor gate, NOT you."
- **Decision, and why it is not a softening:** the criterion was **not** satisfied by editing that tombstone. Flipping it here would have meant re-pinning the whole 90-cell threshold column against this implementation's own output, which is precisely the anti-softening clause's named failure (T-32-15: "that edit would convert a measurement into a transcription of the result"). The red is left standing, is documented in both commit messages, and is 32-07's to close.
- **Consequence to carry forward:** `make test` is **red between this plan and 32-07** — 91 cases, 90 passed, 1 failed, 6 failed assertions. This is a deliberate phase-internal red window of exactly the same kind the D-08 tombstone itself created between 32-03 and this plan.

---

**4. [Rule 2 — Missing Critical] Task 1 was committed with the suite red, on purpose**

- **Found during:** Task 1, at the commit step.
- **Issue:** the two-task split means the D-08 tombstone is red for the duration of exactly one commit. The tempting shortcut is to squash the two tasks so no red commit exists.
- **Decision:** the tasks were committed separately. The tombstone's own text requires the inversion to be **OBSERVED** red, in the shape Phase 29's silence tombstone used and Phase 30 honoured; commit `728121e` **is** that observation, and squashing would have deleted the only evidence that the inverted assertion was ever able to fail. **OBSERVED at `728121e`: 39 of the 45 grid points failed `CHECK(differing == 0)`.**

---

**Total deviations:** 4 auto-fixed — 1 falsified claim in the plan's own wording corrected by measurement, 1 falsified sentence the plan did not name, 1 unsatisfiable acceptance criterion escalated rather than met by softening, 1 deliberate red commit. **No production behavior was changed beyond what the plan specifies.** No guard was weakened, no threshold was edited, no frozen file was touched, and `src/AnalogLFO.cpp` is absent from both commits.

## TDD Gate Compliance

Both tasks are marked `tdd="true"`. The gate sequence for this plan is a **tombstone inversion** rather than a fresh RED/GREEN pair, and it is complete and in order:

| Gate | Evidence |
|------|----------|
| **RED (observed, not argued)** | Commit `728121e`: the D-08 baseline-validity tombstone failed on **39 of 45** grid points the moment `forge::MorphBlep` reached the call site. That is the red the tombstone's own instructions demand before the inversion. |
| **GREEN** | Commit `05ae9a5` (`test`): the inverted case passes with **288 assertions, 0 failed**, and its reconstruction mismatch count is **0** at all three rates. |
| **Sensitivity — the inverted assertion can still fail** | The reconstruction comparison is a direct float `==` over 184 320 samples; the accompanying zero-correction control (`sineCentreChar0Differing == 0`) fails if the correction ever fires where there is nothing to correct. The 12-of-15 bound is met **exactly** at 96 kHz, so a correction that stopped firing on one more cell would turn the case red rather than passing quietly. |

Commit types: `feat(32-06)` for the production change, `test(32-06)` for the test-only inversion.

## Findings Recorded for Later Plans

- **Plan 32-07 (the threshold re-pin) inherits a very short list.** Against the corrected core, **only 2 of the 45 gated cells still exceed their provisional thresholds**, and both are the same cell at two notes:
  - `44100 / K=195 / morph 0.00 sine / character 0.50` — measured **-64.61 dB** against a threshold of **-65.0** (misses by 0.39 dB)
  - `44100 / K=389 / morph 0.00 sine / character 0.50` — measured **-66.11 dB** against a threshold of **-68.0** (misses by 1.89 dB)

  Both are **sine-centre with the bleed ring live**, which is the regime `src/dsp/MorphBlep.hpp`'s banner singles out ("the narrow pulse bleeds in at full intensity inside what the user hears as a pure sine"). The deferred narrow-pulse "reach" refinement recorded in that banner is the first thing to try if 32-07 decides the gap is real rather than a provisional-threshold artefact.
- **All five of the tombstone's named large-margin subset cells now sit BELOW threshold**, by 3.79 / 3.84 / 3.88 / 3.57 / 3.01 dB (triangle C8, saw C8, square C8, pulse C8, saw C9). Plan 32-07's flip of `CHECK(naiveDb > threshold + 5.0)` to `CHECK(correctedDb <= threshold)` will pass on all five with 3 dB of room.
- **The 32-05 missed-edge caveat did not bite anywhere on this grid.** No cell showed the signature (a shape's correction present at two rates and absent at a third for no D-03 reason). The three zero-correction cells are all explained by the D-03 factor's exact zero, computed by hand from `2*dt` against the edge width, and the arithmetic agrees to the decimal.
- **Phase 33 (hard sync)** plugs into `forge::VcoCore::blep` through `MorphBlep::addStep`. The member is public, held by value, and its per-instance-ness is now covered by the interleave invariant as well as by inspection.
- **Phase 34 (output conditioning)** should know the corrected output envelope: **MEASURED 5.518014 V** over a 21 × 5 morph-by-character grid at C8 / 44.1 kHz — comfortably inside `tests/test_vco_core.cpp`'s 6.0 V loose bound and inside Rack's ±12 V norms, and slightly under the 5.52 V naive figure the D-13 paragraph records.
- **Any future editor of the morph/character guard** should know that swapping it back to `forge::clamp` is not a cosmetic change: it re-opens the frozen `(int)(morph * 4.f)` cast to a not-a-number arriving through the MORPH CV jack.

## Known Stubs

None. Every line this plan added is live in the audio path or asserted by a case that runs on every invocation. There is no placeholder, no skipped case, no `TODO` and no flag.

## Threat Flags

None — no network, auth, file-access or schema surface was introduced. Every threat-register entry assigned to this plan is mitigated by a named artefact:

| Threat | Mitigation as landed |
|--------|----------------------|
| T-32-01 (the frozen float-to-int cast reached through the MORPH CV jack) | The negated-comparison pair for **both** fields inside `step()`, with the ladder's exception withdrawn in place at `VcoCore.hpp:392`. Probe: both fields NaN → 4096 finite samples, 5.000000 V envelope. |
| T-32-02 (division by `dt` newly reachable from `sampleTime`) | `forge::MorphBlep` carries its own negated guard on both divisors and does not rely on this caller (P-14); the corrected `:488` paragraph records that this phase is what put a divisor behind that field. |
| T-32-18 (voice isolation of the new `blep` member) | Held BY VALUE inside the per-instance state block; the interleave invariant **and** its permanent shared-state positive control were re-run and are green. |
| T-32-19 (the D-08 baseline silently ceasing to be a baseline) | The inverted tombstone's bit-exact reconstruction, mismatch count **0** on 184 320 samples, with the standing "stop and report" instruction written into the case banner. |
| T-32-20 (`make guards` hard-failing on the source-shape contract) | The `struct VcoCore` and `float step(...)` lines each still sit on one line with their brace; the full signature is quoted nowhere on a line containing a brace. `check_canary.sh` [2b/5] reports OK on all 8 fields. |
| T-32-12 (the shipped Analog LFO's golden bit-stability) | No frozen header edited, `FROZEN.sha256` unmoved, `src/AnalogLFO.cpp` absent from both commits, and the six `.f32` goldens replay bit-exact on every `make test`. |
| T-32-SC (package installs) | Zero packages installed in any ecosystem. |

## Issues Encountered

None beyond the four deviations above. `make strict`, `make guards` and `make -j4` were green at every commit; `make test` is green on every case except the plan-32-07 tombstone, which is red by design.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

**Ready for 32-07.**

The band-limiting is live in the shipped code path, the naive baseline is proved to be a faithful copy of everything except the correction, and the alias-floor tombstone that 32-07 owns has been driven from 32 failing gated cells down to 2 — both of them the same sine-with-bleed cell at two notes, with the deferred refinement that targets exactly that regime already recorded in `src/dsp/MorphBlep.hpp`. Nothing in the shipped LFO moved.

## Self-Check: PASSED

- `src/dsp/VcoCore.hpp` — FOUND on disk; `MorphBlep blep;` at :278, the correction at :645, the guard at :598, the two `FALSIFIED PREMISE` markers at :392 and :488
- `tests/test_vco_spectrum.cpp` — FOUND on disk; the inverted case present, `grep -c 'D-08 inversion'` = 1, `Approx` = 0 in comment-stripped source
- `.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/32-06-SUMMARY.md` — FOUND on disk
- Commits `728121e` and `05ae9a5` — both FOUND in `git log`
- All plan `<success_criteria>` re-run and green; both tasks' `<acceptance_criteria>` re-run and green, with the one documented exception (Task 2's "`make test` exits 0 with 0 failures", superseded by deviation 3 — the plan-32-07 tombstone is red by design and is that plan's to invert)

---
*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Completed: 2026-08-01*
