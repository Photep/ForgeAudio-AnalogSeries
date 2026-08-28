---
phase: 33-hard-sync
plan: 02
subsystem: dsp
tags: [hard-sync, vcocore, sub-sample-placement, nan-guard, schmitt-trigger, bit-identity, d-07-ordering, cpp11]

# Dependency graph
requires:
  - phase: 33-hard-sync
    plan: 01
    provides: "MorphBlep Guards A/B/C — the hostile-parameter hardening that had to land before a second MorphBlep call site could be contemplated (deferred item 27, operator-scheduled)"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "src/dsp/MorphBlep.hpp, its nine-site union, the site loop's crossing test and wrap adjustment (:715-718), and the scalar `pending` accumulator (:233) that D-07's residual analysis rests on"
  - phase: 30-vco-core-registration
    provides: "forge::VcoCore, forge::VcoInputs, the single-subtract wrap and the negated-comparison guard idiom the sub-sample guard mirrors"
provides:
  - "forge::VcoInputs::syncVolts (float) and ::syncConnected (bool) — RAW volts cross the POD boundary by decision (D-02)"
  - "forge::VcoCore::syncTrig (forge::SchmittTrigger) and ::prevSyncVolts (float), per-instance, held by value beside blep"
  - "forge::VcoCore::Telemetry::syncFrac, ::syncJump, ::syncCorrection — recording-only; ::syncFired is now POPULATED, not duplicated"
  - "Hard-sync detection, the guarded sub-sample solve, the fractional-overshoot reset, the jump computation and the unconditional previous-voltage store, all inside forge::VcoCore::step"
  - "The core's behavior is now EXACTLY measurement leg `none` — reset applied, sync correction withheld — which is what plan 33-05's probe checks itself against"
  - "D-07 discharged as an ordering claim with its mechanism cited by line, plus the one residual it does not cover named with its window width and its accepted disposition"
affects: [33-03, 33-04, 33-05, 33-06, 33-08, sync-seam, addPastStep]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "The sub-sample solve guarded by the negated pair with a STRICT upper bound and a fallback of ZERO — the first guard in this project whose fallback VALUE is itself a landmine"
    - "Splitting a two-term difference across the point where the second term is already being computed, so the extra cost stays at one frozen-header call"
    - "Discharging a suppression rule as an ORDERING claim, with a cited mechanism, rather than as a predicate that would gate nothing"
    - "Naming the residual a claim does NOT cover, with its window width and its disposition, in the same comment that makes the claim"

key-files:
  created: []
  modified:
    - src/dsp/VcoCore.hpp

key-decisions:
  - "The jump's second term is `naive` itself, completed AFTER the naive call rather than inside the sync block — under the D-07 ordering the POST-reset value is the free one, so the roles in the plan's step list are the opposite way round from what the mandated ordering produces. One extra frozen call, as D-05 authorises; doing both calls in the block would have cost two while the comment claimed one"
  - "The fraction's upper bound is STRICT and its fallback is ZERO. A fallback of ONE would reintroduce the snap-to-zero landmine through the guard itself — the failure mode of a guard that sanitises to the nearest bound"
  - "Neither prevSyncVolts nor syncTrig is reset on a sample-rate change: a rate change does not alter which sample was the previous one, and resetting either would MANUFACTURE the stale-store case the guard rests against"
  - "The morph/character conditioning block MOVED above the sync block rather than the sync path taking raw fields; the pair itself is byte-unchanged"
  - "The seam call is withheld deliberately and the withholding is stated in the source as measurement leg `none`, so plan 33-05 has a bit-exact reference to check its probe against"
  - "No requirement marked complete — SYNC-01 has no jack (33-03) and no assertion (33-04); SYNC-02 has eight further contributing plans"

patterns-established:
  - "A guard whose FALLBACK VALUE is one of the landmines must say so beside itself, because 'sanitise to the nearest bound' is the instinct that would reintroduce the defect"
  - "When a plan's step ordering and a plan's stated cost budget conflict, the cost budget is the substance — implement to the budget and record the inversion"
  - "A claim written into source must state what it does NOT cover in the same comment, with a window width, or a later reader will read the claim as total"

requirements-completed: []  # SYNC-01 and SYNC-02 are BOTH in this plan's frontmatter and NEITHER is delivered end-to-end here — see Decisions #6 and Deviations #3

coverage:
  - id: D1
    description: "With syncConnected false, no reset ever occurs however hostile syncVolts is"
    requirement: "SYNC-01"  # contributes to; not completed by this plan
    verification:
      - kind: other
        ref: "one-shot probe, case 1: 6000 samples cycling {0, 5, NaN, 1, +inf, -inf} V — fired = 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "A master crossing the high threshold upward from below the low threshold fires exactly one reset; a master peaking at 0.9 V fires none"
    requirement: "SYNC-01"
    verification:
      - kind: other
        ref: "one-shot probe, cases 2a/2b: 0 V -> 5 V step fires exactly 1 in 20 samples; a 0.9 V peak fires 0 in 4000"
        status: pass
    human_judgment: false
  - id: D3
    description: "After a reset the phase is (1 - f) * deltaPhase and is strictly positive for every reachable fraction"
    requirement: "SYNC-02"
    verification:
      - kind: other
        ref: "one-shot probe, case 3: 41-point sweep of the previous voltage, 40 firing points, nonPositive = 0, min post-reset phase 0.00395504"
        status: pass
    human_judgment: false
  - id: D4
    description: "A master sample landing exactly on the high threshold computes a raw fraction of exactly 1, is caught by the STRICT upper bound, and lands on the fallback so the phase is deltaPhase rather than zero"
    requirement: "SYNC-02"
    verification:
      - kind: other
        ref: "one-shot probe, case 4: guarded frac 0, phase 0.0118651068 == deltaPhase; mutants A/B/C each report frac = 1 / phase = 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "A not-a-number syncVolts does not fire the trigger, is still stored, and on the next finite crossing produces a fraction caught by the negated lower comparison that never reaches the accumulator"
    requirement: "SYNC-02"
    verification:
      - kind: other
        ref: "one-shot probe, case 5 WITH a withdrawal phase: firedOnNaN 0, guarded frac 0, 0 of 200 post-withdrawal samples non-finite. Mutant A (guard removed): frac = nan, 200 of 200 non-finite, firstBad = 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "prevSyncVolts equals the previous sample's syncVolts on every sample and every branch, including samples where the jack is unpatched"
    requirement: "SYNC-01"
    verification:
      - kind: other
        ref: "one-shot probe, case 6: 400 samples, connected flag toggling every 7 samples, mismatches = 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "tel.syncFired is true exactly on samples where a reset occurred and false otherwise"
    requirement: "SYNC-01"
    verification:
      - kind: other
        ref: "one-shot probe, case 7: 441 Hz square master over 4410 samples — fired 44, backward-phase events 44, exact agreement"
        status: pass
    human_judgment: false
  - id: D8
    description: "With the jack unpatched every finite input produces bit-identical output to the pre-plan header — no recorded Phase 30/31/32 measurement moves"
    verification:
      - kind: other
        ref: "4096-sample forge::VcoCore block at 44.1 kHz / pitchCV +2 / morph 0.60 / character 0.40, pre-plan vs post-plan binary, byte-compared (SHA-256 a7bb08ec... identical, 0 differing samples)"
        status: pass
      - kind: unit
        ref: "./build-test/test -tc=\"*golden*\" (9 cases / 49,188 assertions, six shipped-LFO goldens byte-identical)"
        status: pass
    human_judgment: false

# Metrics
duration: 22min
completed: 2026-08-29
status: complete
---

# Phase 33 Plan 02: The Sync Block in VcoCore Summary

**`forge::VcoCore` now detects a master rising edge, solves the sub-sample instant behind a guard whose three landmines each have their trigger condition written beside them, and resets to a fractional overshoot that cannot be exactly zero — with the BLEP seam call deliberately withheld so plan 33-05 decides the placement convention by measurement rather than by assumption.**

## Performance

- **Duration:** 22 min
- **Started:** 2026-08-28T20:46:00Z
- **Completed:** 2026-08-28T21:08:00Z
- **Tasks:** 3 of 3
- **Files modified:** 1

## Accomplishments

- **Landed SYNC-01 in full inside the core**, with raw volts crossing the POD boundary so the whole detection path — hysteresis, previous-voltage store, sub-sample solve — is assertable headlessly. A shell-side trigger would hand the core an already-decided boolean and no test could ever see a mis-detected edge; that argument is now written in the header rather than left in the plan.
- **Measured every one of Task 2's seven behaviours rather than asserting them**, and measured **three distinct guard mutants** producing **three different reds** — including a genuine permanent-poisoning trace with a withdrawal phase.
- **Proved bit-identity by measurement.** With the jack unpatched, 4096 samples are byte-identical to the pre-plan binary. Identical SHA-256, 0 differing samples.
- **Caught and corrected a cost-accounting inversion in the plan's own step list** (see Deviations #1): implementing it literally would have cost two extra frozen-waveshaper calls while the comment claimed one.
- **Discharged D-07 honestly** — as the ordering claim it actually is, with the mechanism cited by file and line, and with the one residual it does not cover named, sized, and given a disposition.
- **Found a live insensitivity in `check_canary.sh [2b/5]`** on this host and filed it against plan 33-03, whose gate depends on it (see Deferred Register #2).

## Task Commits

1. **Task 1: The POD fields, the per-instance state, the telemetry and the two self-contradicting banners (D-02 / CORE-03)** — `809dd81` (feat)
2. **Task 2: The sync block — detection, the guarded solve, the reset and the jump (SYNC-01 / D-01 / D-03 / D-05 / D-12)** — `0019a97` (feat)
3. **Task 3: D-07's ordering claim, plus the residual phantom named (D-07)** — `85cd01e` (docs)

## Files Created/Modified

- `src/dsp/VcoCore.hpp` — 422 insertions, 19 deletions across three commits. **Nine new code lines and two moved blocks; the rest is the comment density this file holds a two-line guard to.** Two POD fields, two per-instance members, three recording-only telemetry floats, the sync block, the jump-completion line, and four banner paragraphs corrected in the commits that falsified them.

## The Landed Ordering — All Three Line Numbers Recorded

The plan requires the conditioning block to precede the sync block, which must precede the `p` snapshot. Measured on the final tip:

| Landmark | Line |
|---|---|
| `if (!(morph > 0.f))` — the conditioning pair | **689** |
| `syncTrig.process(...)` — the sync block | **853** |
| `const float p = (float)phase;` — the snapshot | **922** |

689 < 853 < 922. The `p` snapshot, the naive sample and the single `blep.step` call all see the **post-reset** phase.

## The Measured Behaviour (one-shot probe, outside the repository)

Plan 33-04 owns the permanent cases. This plan measured all seven of Task 2's stated behaviours so those cases inherit numbers rather than expectations.

| # | Property | Result |
|---|----------|--------|
| 1 | `syncConnected` false, 6000 samples cycling `{0, 5, NaN, 1, +inf, -inf}` V | **0 resets** |
| 2a | 0 V → 5 V step over 20 samples | **exactly 1 reset** |
| 2b | master peaking at 0.9 V, 4000 samples | **0 resets** |
| 3 | 41-point sweep of the previous voltage | 40 firing points, **0 non-positive** post-reset phases, minimum **0.00395504** |
| 4 | master sample landing exactly on 1.0 V | guarded frac **0**, phase **0.0118651068** = `deltaPhase` |
| 5 | NaN volts, then withdrawal over 200 samples | fired-on-NaN **0**, guarded frac **0**, non-finite **0 of 200** |
| 6 | 400 samples, connected flag toggling every 7 | store mismatches **0** |
| 7 | 441 Hz square master, 4410 samples | `syncFired` **44**, backward-phase events **44** |

### The Measured RED — three mutants, three different signatures

Each mutant was built into a scratch include directory and the probe recompiled against it. No repository artifact.

| Mutant | Change | Cases red | Signature |
|--------|--------|-----------|-----------|
| **A** | the guard line **removed entirely** | **2** | case 4 `frac = 1, phase = 0`; case 5 `frac = nan`, **200 of 200 post-withdrawal samples non-finite**, `firstBad = 0` |
| **B** | upper bound made **non-strict** (`f <= 1.f`) | **1** | case 4 only — `frac = 1, phase = 0`. Case 5 **green**: the negated lower comparison still catches the NaN |
| **C** | fallback changed **0.f → 1.f** | **2** | case 4 `frac = 1, phase = 0`; case 5 `frac = 1` — the NaN is caught and then sent to the landmine |

**Mutant B isolates landmine 2** (the strict bound alone is what stops the snap) and **mutant C isolates landmine 3** (the fallback value alone). The two share only case 4, and neither reproduces mutant A's poisoning.

### The permanent-poisoning claim is TRUE here — and that is a real difference from plan 33-01

Plan 33-01 **falsified** the "permanent poisoning" narrative for `MorphBlep`'s `jump`: exactly one of the next twenty samples went non-finite, because `step()`'s preamble drains and zeroes both accumulators unconditionally. **The same narrative is TRUE for `phase`, and mutant A measures it at 200 of 200.** The structural reason is the one now written beside the guard: `phase` has no drain. Once it is a not-a-number, `phase += deltaPhase` stays one, `phase >= 1.0` is false so the wrap never fires again, and the instance never recovers — after the hostile input has gone. That is why the guard is load-bearing rather than defensive, and it is why plan 33-04's case must carry a withdrawal phase.

## Bit-Identity, Measured Not Asserted

4096 samples through `forge::VcoCore` at 44.1 kHz, pitchCV +2, morph 0.60, character 0.40, compiled with the flags `make test` uses (`-std=c++17 -O2 -ffp-contract=off`), captured **before** any edit and again on the final tip with the jack unpatched:

| Capture | SHA-256 | Differing samples |
|---------|---------|-------------------|
| pre-plan (`c6d8bf1`) | `a7bb08ecbb2d1ba74e9dbb9c7022ed67c4e87869817239f51ccf09cc858d6c1c` | — |
| post-plan (`85cd01e`) | `a7bb08ecbb2d1ba74e9dbb9c7022ed67c4e87869817239f51ccf09cc858d6c1c` | **0 of 4096** (`cmp`, direct byte comparison) |

## Suite Totals, Before and After

| | Test cases | Assertions |
|---|---|---|
| Before plan 33-02 | 97 | 2,622,378 |
| After plan 33-02 | **97** | **2,622,378** |
| Delta | **0** | **0** |

**Zero delta is the correct result for this plan and is stated so it is not read as a gap.** The plan's `files` is `src/dsp/VcoCore.hpp` alone and all three tasks' acceptance criteria require `git diff --name-only` to list only that file; plan 33-04 owns the assertions. See Deviations #2.

## Gate Results

| Gate | Result |
|------|--------|
| `make test` | **PASS** — 97 cases, 2,622,378 assertions, 0 failures |
| `make strict` (C++11, `-pedantic-errors`) | **PASS**, exit 0 |
| `make guards` | **PASS**, exit 0 |
| `bash tests/check_canary.sh` | **PASS**, exit 0 — perturbed `src/dsp/VcoCore.hpp` successfully, no "could not perturb" error |
| `[2b/5]` field count | **8 → 9** (`syncVolts` joins the enumeration; `syncConnected` is a bool and is not enumerated) — but see Deferred Register #2 |
| `bash tests/check_frozen.sh` | **PASS**, exit 0 — **15 pinned entries** unchanged |
| `git diff --stat` on the four frozen shared headers, whole plan | **empty** |
| `src/AnalogLFO.cpp` in the whole-plan diff | **absent** |
| Six shipped-LFO goldens | **byte-identical** — 9 cases / 49,188 assertions, 0 failures |
| `grep -c '0.1f, 1.0f'` | **1** (the single `syncTrig.process` call) |
| Non-comment `addStep` count | **0** — the seam is genuinely absent from the source, not merely from the diff |
| Non-comment `blep.step` count | **1** — still exactly one band-limiter call per sample |
| Non-comment `clamp(` count | **0 before, 0 after** — unchanged, and the helper is named only in comments where it is rejected by name |
| `grep -c 'deliberately ABSENT'` | **0** — the stale forward reference is gone |
| `grep -n 'syncFired'` | **one Telemetry field** (`:345`) plus a function-local at `:850` — no parallel telemetry field was added |

### Task 3's comment-only criterion — both numbers, as required

| Mechanism | Output |
|---|---|
| The plan's literal criterion, `git diff -U0 \| grep -c '^+[^+/ ]'` | **68** |
| Added lines that are not comments | **0** |

The literal regex assumes an added comment line reads `+ ` or `+/`. Every comment inside `step()` is **tab-indented**, so each added line is `+` followed by a TAB, which the character class `[^+/ ]` matches. **The criterion as written is unsatisfiable for a tab-indented comment-only change in this file.** The corrected mechanism expresses the prose exactly ("adds no new loop, no new array and no new member") and reports 0. This is the **fifth** instance in this project of a gate whose regex is wider than the prose it encodes — after 30-08's doctest line numbers, 30-09's LFO filename zero-count, 30-10's CI run selector and 31-08's `std::pow` prohibition. In all five the prose was correct and the mechanism was not.

## Decisions Made

1. **The jump's second term is `naive` itself, and the completion sits after the naive call.** D-05 authorises **one** extra `morphedWave` call, on the argument that the other term is a value the step was going to compute anyway. Under the D-07 ordering this plan is required to implement, the term that is free is the **post-reset** one — it is `naive`, computed at the post-reset phase for the output — and the term that costs the extra call is the **pre-reset** one. Computing both inside the sync block would have made `naive` a bit-identical recomputation of the second and cost **two** extra calls. Plan 33-06's seam call therefore belongs at the completion line, which the source says explicitly in both places.

2. **The upper bound is strict and the fallback is zero.** The fallback value is itself one of the landmines: sanitising to the nearest bound would send an out-of-range fraction to **one**, which is the snap-to-zero landmine arriving through the guard that was supposed to stop it. The source says this beside the line.

3. **Neither `prevSyncVolts` nor `syncTrig` is reset on a sample-rate change.** The standing discretion item is resolved in the source rather than inherited. A rate change does not alter which sample was the previous one; resetting either would **manufacture** the stale-store case that is the only route to a zero divisor. Plan 33-04 asserts the choice.

4. **The conditioning block moved rather than the sync path taking raw fields.** The pair is byte-unchanged — same order, same wording, same comparisons — and the move is recorded in its own comment with the reason and a "do not move it back down".

5. **The withholding of the seam is itself documented as measurement leg `none`.** Plan 33-05's probe can check itself for bit-exactness against this core precisely because the core's behaviour as of this commit is one of the legs it will measure.

6. **No requirement is marked complete — and both IDs in the frontmatter are declined, not just SYNC-02.** SYNC-02 is declined for plan 33-01's recorded reason (eight further contributing plans; the last one marks it). **SYNC-01 is also declined**, and the reason is its own: it reads *"Hard sync input resets oscillator phase on a master rising edge"*, and there **is no hard sync input** — `AnalogVCO::SYNC_INPUT` is plan 33-03's, so no user can reach this code. Nor does any permanent test assert it; plan 33-04 owns that. Marking it here would repeat the PANEL-03 false green exactly, and would be the **sixth** consecutive decline in this project's history of them.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] The plan's Part D step list costs TWO extra frozen calls while claiming one**

- **Found during:** Task 2
- **Issue:** Part D orders: (1) take the pre-reset value, (2) reset, (3) *"take the post-reset value with ONE extra `morphedWave` call — this is the whole extra cost D-05 authorises"*, (4) subtract. It also requires the comment to record *"that the pre-reset value REUSES the sample this step was going to compute anyway"*. **Under the ordering the same plan mandates, that reuse is impossible.** The sync block sits *above* the `p` snapshot and above `naive` (D-07), so at that point in the sample nothing has been computed yet — the pre-reset value is **not** free. Implemented literally, a sync sample makes three `morphedWave` calls: `before`, `after`, and `naive`, where `naive` recomputes `after` bit-identically. That is **two** extra calls against a stated budget of one, with a source comment claiming one.
- **Fix:** Kept the plan's **cost budget** (the substance) over its **step ordering** (the mechanism). `before` is taken inside the sync block — it is the extra call — and the subtraction is completed on the line immediately after `naive`, which *is* the post-reset value. **One extra call**, on sync samples only. Line 608's `naive` computation is **byte-unchanged**. The inversion is recorded in the source at both ends, with an explicit direction for plan 33-06 that its seam call belongs at the completion line.
- **Files modified:** `src/dsp/VcoCore.hpp`
- **Verification:** Non-comment `morphedWave` call sites in `step()`: 2 (one unconditional, one inside the sync branch). The jump reads the emitted sample itself, so `after` and `naive` cannot drift apart — a strictly stronger guarantee than two separate calls.
- **Committed in:** `0019a97`

**2. [Rule 3 — Blocking] Every task is marked `tdd="true"` while every task forbids touching a test file**

- **Found during:** Task 1
- **Issue:** All three tasks carry `tdd="true"`, which mandates a failing-test commit first. All three tasks also carry the acceptance criterion *"`git diff --name-only` lists only `src/dsp/VcoCore.hpp`"*, the plan's `files_modified` is that one file, its `artifacts` is that one file, and the plan names **plan 33-04** as the owner of the assertions in four separate places. A RED commit would put `tests/test_vco_core.cpp` in the diff and fail the criterion on every task.
- **Fix:** Honoured the acceptance criteria — thrice-repeated, specific, and consistent with the phase's plan split — over the `tdd` attribute. **The RED was still measured, not skipped:** three guard mutants built into scratch include directories, each producing a different red, following the revert-one-only convention plan 33-01 established. Nothing was committed to `tests/`.
- **Files modified:** none — an execution-procedure decision
- **Verification:** Mutant signatures table above; `git diff --name-only c6d8bf1 HEAD` returns `src/dsp/VcoCore.hpp` alone.
- **Committed in:** n/a (procedure, recorded here and in `0019a97`'s message)

**3. [Rule 1 — Bug] The automatic requirement mark would book SYNC-01 as well as SYNC-02**

- **Found during:** state updates, after Task 3
- **Issue:** Plan 33-01's SUMMARY left a standing note that the auto-mark would fire on all eight remaining SYNC-02 plans. **This plan's frontmatter carries SYNC-01 too**, and SYNC-01 is the more tempting of the two to let stand, because this plan really does implement "resets oscillator phase on a master rising edge" in the core. It is still a false green: there is **no sync input on the module** (plan 33-03 adds `SYNC_INPUT`), so no user can reach the code, and **no permanent test asserts it** (plan 33-04). A green SYNC-01 would tell the audit-open scanner and the operator that hard sync is reachable when it is not.
- **Fix:** Neither ID marked. `.planning/REQUIREMENTS.md` is unchanged from its pre-plan state.
- **Files modified:** `.planning/REQUIREMENTS.md` (verified unchanged)
- **Verification:** `git diff .planning/REQUIREMENTS.md` produces no output.
- **Committed in:** n/a — the file is unchanged

**4. [Rule 3 — Blocking] Task 1's stale-reference criterion is a literal grep for a phrase the correction wanted to quote**

- **Found during:** Task 1
- **Issue:** The criterion is `grep -n 'deliberately ABSENT' src/dsp/VcoCore.hpp` returns no match. The first draft of the corrected paragraph followed this project's "falsified premise corrected in place" convention and **quoted the old wording verbatim**, which made the criterion fail while the stale forward reference was in fact gone.
- **Fix:** Paraphrased the old wording instead of quoting it, and recorded **why** in the paragraph itself — the file is being grepped against the phrase it would otherwise quote. Same trap class `src/AnalogVCO.cpp`'s banner already documents for the four forbidden C++ constructs.
- **Files modified:** `src/dsp/VcoCore.hpp`
- **Verification:** `grep -c 'deliberately ABSENT'` returns **0**; the correction-in-place record is intact.
- **Committed in:** `809dd81`

**5. [Rule 2 — Missing critical documentation] The strictly-positive reset claim is false at a zero increment**

- **Found during:** Task 2
- **Issue:** Task 2's `<behavior>` states *"after a reset, `phase` equals `(1 - f) * deltaPhase` and is strictly greater than zero for every reachable `f`"*. True of the fraction — but the **product** is still zero when `deltaPhase` is zero, which the guards above produce for a non-positive or non-finite sample rate and at extreme negative pitch (D-13's measured 3.2e-22 floor case). A plan-33-04 assertion of `phase > 0.0` written unconditionally from that sentence would be asserting something false and would go red on the existing hostile-timing scenarios.
- **Fix:** Stated the caveat honestly beside the reset — the claim is about the fraction, the product is still zero at a zero increment, that is not the snap this block is written against because the oscillator is frozen and silent there anyway, and **plan 33-04's assertion belongs on a live increment**.
- **Files modified:** `src/dsp/VcoCore.hpp`
- **Verification:** Probe case 3 sweeps 40 firing points at a live increment: 0 non-positive.
- **Committed in:** `0019a97`

---

**Total deviations:** 5 auto-fixed (2 × Rule 1, 2 × Rule 3, 1 × Rule 2)
**Impact on plan:** All five served the plan's own stated goals. One preserved a cost budget the plan's step list would have broken; one resolved a contradiction between the plan's TDD attribute and its own diff constraints without skipping the RED; one refused a false green; two made criteria and claims satisfiable and true. **No scope creep** — the whole-plan diff is `src/dsp/VcoCore.hpp` alone.

## Known Stubs

| Stub | File / line | Reason, and who resolves it |
|------|-------------|------------------------------|
| `tel.syncCorrection` is unconditionally `0.f` | `src/dsp/VcoCore.hpp` (sync block, Part F) | **Intentional and load-bearing.** D-06 forbids choosing the seam placement before plan 33-05 measures it, so no seam call exists to deposit anything. The field's zero value is what makes the core exactly measurement leg `none`. **Resolved by plan 33-06**, at the same line as its seam call. The source names the plan and states the condition. |

This is the plan's stated output, not an unwired path: the plan's `<objective>` requires the core's behaviour after this commit to be *"exactly measurement leg `none`: reset applied, sync BLEP withheld"*, and `<task>` Part F requires the seam's absence to be written into the source with its reason.

## Deferred Register Items

These are recorded here so plan 33-11 files them with a Resolve-at rather than losing them.

**1. The residual `pending` phantom that D-07's ordering claim does not cover.**
At sample *n−1* the site loop fired every site within one increment ahead and deposited both halves; sites the reset then jumped over were never traversed, so **both** halves are phantom and the second is still in the carried accumulator when sample *n* drains it. Window width is `(1 − f) · dt`; at C7 and 44.1 kHz the increment is ≈ 0.047 of a cycle, so a mid-crossing reset leaves a window ≈ 0.024 wide — on the order of one phantom site every few sync events at that note. `MorphBlep`'s accumulator is a **scalar sum** (`MorphBlep.hpp:233`), so per-site cancellation is impossible without restructuring a header this phase is otherwise only hardening. **Disposition: accept and document** (taken), for two reasons — the effect is partly self-cancelling because the pre-reset jump term is taken *past* the jumped-over site, and restructuring is out of proportion.
**Proposed Resolve-at:** the first phase that restructures `MorphBlep` for per-site accounting, or v2.1's oversampling work — whichever comes first; not before.
**Request to plan 33-05:** its grid should carry a **diagnostic column reporting this residual**, so a later phase inherits a measured number rather than this paragraph's arithmetic. The figure above is explicitly labelled arithmetic, not measurement, in both the source and here.

**2. NEW — `check_canary.sh [2b/5]`'s per-field check is INSENSITIVE on this host, and plan 33-03's gate depends on it.**
**Measured this session, twice.** (a) The section reports *"all 9 VcoInputs DSP field(s) stay runtime-live"* on the landed header even though `src/vco_compile_canary.cpp` **never assigns `syncVolts`** — it holds its NSDMI default of `0.f`, a compile-time constant, which is exactly the condition the section exists to detect. (b) A direct sensitivity probe replacing the canary's runtime-derived `in.morph` with the literal `0.5f` **still emits `kCanaryOdr_morph`** at `-O3`. So on Apple clang the symbol survives a constant-fed field and the check cannot distinguish the two cases; its real teeth are on the **CI GCC leg**, which the section's own INFO branch half-acknowledges for a neighbouring sub-case but not for this one.
**Consequence, and why it matters now:** Pitfall 9 makes feeding both new fields a plan-33-03 task with its own rationale, and register item 15 pins it. **That task's local gate will be green whether or not it does the work.** 33-03 must therefore either observe `[2b/5]` on the CI GCC leg for its commit, or add a sensitivity control that bites on this host, and must not read a local PASS as evidence.
**Not fixed here:** the fix lives in `tests/check_canary.sh` or `src/vco_compile_canary.cpp`, both outside this plan's single-file scope, and 33-03 owns the canary.
**Proposed Resolve-at:** plan 33-03, as part of the task that feeds the fields.

## Issues Encountered

- **`tel.syncFrac` and `tel.syncJump` are populated but nothing reads them yet.** That is by design — they are recording-only and their consumers are plans 33-04, 33-05, 33-06 and 33-08 — but it means a compiler cannot tell anyone if a later edit stops populating them. Plan 33-04's assertions are the first thing that will.
- **T-33-08 (toolchain divergence) is not discharged locally**, unchanged from plan 33-01. `make strict` runs the C++11 `-pedantic-errors` gate under Apple clang and passes, but `-fsyntax-only` never links. The new code adds no integer cast and no shift, so it is a smaller exposure than 33-01's, but the CI MinGW compile-and-link leg on the exact commit is still plan 33-11's job. Nothing here is tagged or submitted on local evidence alone.
- **Two untracked `.planning/research/.cache/*.json` files pre-existed** at session start and were left alone.

## Next Phase Readiness

**The core is now exactly measurement leg `none`, which is the precondition plan 33-05 was written against.**

- **Plan 33-03** may add the SYNC jack and the canary field feeds. **Read Deferred Register #2 first** — its local `[2b/5]` gate does not bite on this host.
- **Plan 33-04** owns every assertion this plan measured. Three specifics it should inherit rather than rediscover: its poisoned-instance case **must** carry a withdrawal phase (a case that only asserts finiteness during the hostile sample sees nothing — measured 200 of 200 with the guard removed); its strictly-positive reset assertion **must** be on a live increment (Deviations #5); and its two-instance interleave case must **drive sync voltages** for the CORE-03 window to cover `syncTrig` and `prevSyncVolts`, which the header's own comment says is not yet true.
- **Plan 33-05** may measure the three placement candidates against this ordering, and may check its probe for bit-exactness against this core. Its grid should carry the phantom's diagnostic column (Deferred Register #1).
- **Plan 33-06** lands the seam. Two things the source directs it to: the seam call belongs at the **SYNC JUMP COMPLETION** line, not in the sync block, because the jump does not exist yet at that point in the sample; and it must **re-state** `tel.syncCorrection`'s reconstruction relationship against whichever candidate 33-05 selects, because the exact subtraction is specific to the past-edge placement.

**Concerns carried forward:**

- **The `[2b/5]` insensitivity is the sharpest one** — it is a guard reporting PASS on a property it cannot currently observe on this host, in the exact section whose failure mode got v2.0.0 rejected.
- **Guard C's IEEE dependence from plan 33-01 extends here.** The new guard's negated lower comparison relies on NaN comparison semantics identically. `-ffast-math` would defeat both.
- **No requirement is complete.** SYNC-01 needs 33-03's jack and 33-04's assertions; SYNC-02 has eight further contributing plans.

## Self-Check: PASSED

Verified against disk and git rather than asserted:

- **Files exist:** `src/dsp/VcoCore.hpp`, `.planning/phases/33-hard-sync/33-02-SUMMARY.md` — both FOUND.
- **Commits exist:** `809dd81`, `0019a97`, `85cd01e` — all FOUND in `git log --all`.
- **The landed code is present in `HEAD`,** confirmed against the committed blob rather than the working tree: `syncVolts` / `syncConnected` in `struct VcoInputs`, `SchmittTrigger syncTrig` and `float prevSyncVolts` beside `blep`, the three telemetry floats, `syncTrig.process(in.syncVolts, 0.1f, 1.0f)`, the guard `if (!(f >= 0.f) || !(f < 1.f)) f = 0.f;`, the reset `phase = (double)(1.f - f) * deltaPhase;`, the unconditional `prevSyncVolts = in.syncVolts;` and the jump completion.
- **The seam is genuinely absent from the source,** not only from the diff: non-comment `addStep` count is **0**.
- **The whole-plan diff is one file:** `git diff --name-only c6d8bf1 HEAD` returns `src/dsp/VcoCore.hpp` alone; `src/AnalogLFO.cpp` is absent; the four frozen shared headers show an empty diffstat.

---
*Phase: 33-hard-sync*
*Completed: 2026-08-29*
