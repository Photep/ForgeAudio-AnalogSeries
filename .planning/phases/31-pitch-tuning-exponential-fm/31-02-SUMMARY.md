---
phase: 31-pitch-tuning-exponential-fm
plan: 02
subsystem: dsp
tags: [cpp11, constexpr, nyquist, comment-truth, load-bearing-comments, pitch]

# Dependency graph
requires:
  - phase: 30-vcocore-skeleton-module-registration
    provides: "the implemented, correctly-ordered Nyquist clamp (CR-01 ceiling-then-negated-floor, WR-06 rate sanitising) and the PROVISIONAL constant whose own comment named Phase 31 as its replacement"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 01
    provides: "a green `make guards` and `make test` at its exact measured floor, so this plan's identical post-edit counts are meaningful rather than coincidental"
provides:
  - "forge::kVcoNyquistGuardFrac = 0.495f — PITCH-04's SETTLED policy, with derived ceilings, derived crossover volts and D-10's hard-clamp decision recorded in the source"
  - "the corrected kVcoMaxDeltaPhase margin arithmetic (coupled-rate maximum 0.495, margin ~1 %) — no comment in the header asserts arithmetic the new constant falsifies"
  - "the -21609.00 CR-01 figures annotated as pre-Phase-31 historical observations, digits intact (Pitfall 8 discharged for this file)"
affects: [31-03, 31-05, 31-07, 32-morph-blep]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "A constant's value change and every comment its new value falsifies land in the SAME plan — a stale arithmetic claim is part of the change's blast radius, not follow-up work"
    - "Recorded MEASURED figures are annotated with the constant they were measured under, never recomputed (Pitfall 8)"
    - "Retiring a placeholder means retiring its forward-reference too: the banner, the constant's own comment and the neighbouring constant's cross-reference all pointed at 'Phase 31 will'"

key-files:
  created: []
  modified:
    - "src/dsp/VcoCore.hpp"

key-decisions:
  - "kVcoNyquistGuardFrac = 0.495f exactly as D-11/D-12 specified — no substituted value"
  - "The new rationale is written as WHOLE-LINE comments above the declaration, with a short trailing comment on the declaration line, because the acceptance criteria negative-grep the retired marker word"
  - "kVcoMaxDeltaPhase's own nine-line rationale was de-tensed (Rule 2 deviation) — it asserted Phase 31 'replaces' the Nyquist constant and 'must leave this one alone', both falsified by Task 1"
  - "PITCH-04 deliberately NOT marked complete: this plan settles the policy constant, it asserts no clamp behavior (31-07 owns that)"

patterns-established:
  - "Comment-only commits carry their own proof: `git diff -U0 | grep ';[[:space:]]*$'` returning empty is the evidence, stated in the commit body"
  - "A grep-based acceptance criterion that scans comments as well as code is a plan-authoring artifact, not a finding — re-run it filtered to non-comment lines and record both numbers"

requirements-completed: []

coverage:
  - id: D1
    description: "kVcoNyquistGuardFrac is 0.495f as a namespace-scope plain constexpr float, with the settled PITCH-04 rationale (policy, derived ceilings, derived crossover volts, D-10 hard clamp, D-13 no floor) in the source"
    requirement: "PITCH-04"
    verification:
      - kind: integration
        ref: "grep -c 'constexpr float kVcoNyquistGuardFrac = 0.495f;' == 1; grep -c '= 0\\.49f' == 0; grep -c PROVISIONAL == 0; grep -c 21829 == 1; grep -ci 'hard clamp|pins at the ceiling|keeps sounding' == 2"
        status: pass
      - kind: integration
        ref: "make strict — 'strict C++11 gate: PASS' (the -std=c++11 -pedantic-errors leg the VCV Library build uses)"
        status: pass
    human_judgment: false
  - id: D2
    description: "No comment in the header makes an arithmetic claim 0.495f falsifies; the kVcoMaxDeltaPhase margin is restated at ~1 % with every conclusion preserved"
    verification:
      - kind: integration
        ref: "grep -c 'roughly two percent' == 0; grep -ci 'roughly one percent|about one percent' == 2; grep -c '0.495' == 5"
        status: pass
    human_judgment: false
  - id: D3
    description: "kVcoMaxDeltaPhase's value and type are unchanged (D-12) and the guard sequence, the negated floor and the pitch expression are provably untouched"
    verification:
      - kind: integration
        ref: "git diff 46c1129..070af28 non-comment delta is exactly ONE line (the constant); `constexpr double kVcoMaxDeltaPhase = 0.5;` byte-identical, only its line number moved 95 -> 133"
        status: pass
      - kind: integration
        ref: "CR-01 ordering intact — ceiling at :268 strictly before negated floor at :269; grep -c 'DO NOT SWAP' == 1; grep -c 'if (!(freq > 0.f)) freq = 0.f;' == 1"
        status: pass
    human_judgment: false
  - id: D4
    description: "The recorded -21609.00 measurement literals were annotated, not recomputed (Pitfall 8)"
    verification:
      - kind: integration
        ref: "grep -c '21609' == 1 and that line still contains tel.freqHz; grep -ci 'pre-Phase-31' == 1"
        status: pass
    human_judgment: false
  - id: D5
    description: "The shipped LFO is untouched and the regression floor holds exactly, with every live reference to the constant symbolic so the value change propagated without a test edit"
    requirement: "PITCH-04"
    verification:
      - kind: unit
        ref: "make test — 72 cases / 2616112 assertions / 0 failed, IDENTICAL to the pre-edit baseline captured before Task 1; test_vco_core.cpp recompiled against the new value and no assertion moved"
        status: pass
      - kind: integration
        ref: "make guards — 'guard suite: PASS' (frozen manifest + 6 LFO .f32 goldens + dependency audit + canary); git diff --name-only 46c1129..HEAD == src/dsp/VcoCore.hpp alone"
        status: pass
    human_judgment: false

# Metrics
duration: 7min
completed: 2026-07-30
status: complete
---

# Phase 31 Plan 02: Nyquist Policy Settled Summary

**`kVcoNyquistGuardFrac` retired from placeholder to `0.495f` with PITCH-04's settled rationale — derived ceilings, derived crossover volts and D-10's hard-clamp decision recorded in the source — and every comment the new value falsified corrected in the same plan, at a total non-comment delta of ONE line.**

## Performance

- **Duration:** 7 min
- **Started:** 2026-07-30T01:11Z
- **Completed:** 2026-07-30T01:17Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments

- **PITCH-04's policy stops being provisional.** The constant is `0.495f`, the value D-11 settled, and the comment that used to promise "Phase 31 replaces this constant" is gone because Phase 31 just did. The rationale now carries the three things that make the choice auditable rather than arbitrary: the policy expression (`0.5 x sampleRate x 0.99`), the derived ceilings at the three rates users actually run, and the crossover volts those ceilings correspond to off C4.
- **D-10 is recorded where the next reader will hit it.** The clamp is a HARD clamp: frequency pins at the ceiling and the oscillator keeps sounding. The flattened peaks under deep FM are the chosen sound. Both rejected alternatives — amplitude fade, pitch fold-back — are named with *why* they were rejected, so a future phase re-litigates from the actual reasoning instead of rediscovering it.
- **The one non-obvious consequence landed with the change, not after it.** The `kVcoMaxDeltaPhase` rationale argued that `0.5` was chosen because it clears the coupled-rate increment maximum "by roughly two percent". At `0.495` the maximum rises and the margin becomes ~1 %. Leaving that standing would have reproduced exactly the false-comment class plan 30-08 existed to remove — in the very file 30-08 cleaned.
- **Three separate forward-references to "Phase 31 will" were retired, not one.** The constant's own trailing comment, the file banner's "non-provisional Nyquist policy" clause, and `kVcoMaxDeltaPhase`'s cross-reference all pointed at this phase. Only the first was in the plan's task list; the other two were falsified the moment Task 1 landed.
- **The regression floor held exactly, and that is a result rather than a formality.** `tests/test_vco_core.cpp` references the constant symbolically in two places and recompiled against the new value, yet the assertion count is bit-stable at 2,616,112 — which is the direct evidence that no Hz literal is hardcoded anywhere in the suite.

## Task Commits

1. **Task 1: Set `kVcoNyquistGuardFrac` to `0.495f` and replace its placeholder comment with the settled PITCH-04 rationale (D-10 / D-11)** — `2c39f0f` (feat)
2. **Task 2: Correct the `kVcoMaxDeltaPhase` margin arithmetic and annotate the historical measurements (D-12 / Pitfall 8)** — `070af28` (docs, comment-only)

## Files Created/Modified

- `src/dsp/VcoCore.hpp` — one constant value; four comment regions (the banner, the constant's new rationale, the `kVcoMaxDeltaPhase` cross-reference, the CR-01 narrative annotation + the wrap-margin paragraph)

## Test counts, pre-edit vs post-edit

Captured with `make test` **before** any edit, and again after each task:

| Run | Cases | Assertions | Failed |
|---|---|---|---|
| Pre-edit baseline (at `46c1129`) | 72 | 2,616,112 | 0 |
| After Task 1 (`2c39f0f`) | 72 | **2,616,112** | 0 |
| After Task 2 (`070af28`) | 72 | **2,616,112** | 0 |
| Phase regression floor | 72 | 2,616,112 | 0 |

Identical, and identical is the requirement rather than a coincidence: `tests/test_vco_core.cpp` **was** recompiled (it includes this header and reads the constant at `:347` and `:736`), so the counts holding proves the references are symbolic and no assertion encodes a frequency literal.

## Every live reference is symbolic

`grep -rn 'kVcoNyquistGuardFrac' src tests` — twelve hits, of which exactly **three** are non-comment, and all three are a multiplication against a rate rather than a literal:

```
src/dsp/VcoCore.hpp:15:// PITCH-04's Nyquist policy is SETTLED as of this commit: kVcoNyquistGuardFrac
src/dsp/VcoCore.hpp:89:// POLICY: clamp the oscillator frequency to kVcoNyquistGuardFrac * sampleRate,
src/dsp/VcoCore.hpp:97:// INTENTIONALLY stops — log2(kVcoNyquistGuardFrac * sampleRate / kVcoFreqC4) —
src/dsp/VcoCore.hpp:120:constexpr float kVcoNyquistGuardFrac = 0.495f;  // 0.5 x sampleRate x 0.99 (PITCH-04 / D-11)
src/dsp/VcoCore.hpp:123:// This is a DIFFERENT KIND OF CONSTANT from kVcoNyquistGuardFrac above and must
src/dsp/VcoCore.hpp:124:// not be confused with it. kVcoNyquistGuardFrac is a Nyquist POLICY bound on the
src/dsp/VcoCore.hpp:239:		const float maxFreq = kVcoNyquistGuardFrac * safeRate;
src/dsp/VcoCore.hpp:264:		// symbolically, through kVcoNyquistGuardFrac itself.)
src/dsp/VcoCore.hpp:295:		// WHY 0.5 AND NOT kVcoNyquistGuardFrac. At a COUPLED rate
src/dsp/VcoCore.hpp:309:		// rather than absorbed. A future phase that raises kVcoNyquistGuardFrac
tests/test_vco_core.cpp:347:		const float maxFreq = forge::kVcoNyquistGuardFrac * in.sampleRate;
tests/test_vco_core.cpp:736:						forge::kVcoNyquistGuardFrac * ((rate > 0.f) ? rate : 0.f);
```

The two `tests/` sites are the broken-control mirror and scenario four's `expectedMaxFreq` — both followed the value automatically, which is why this plan edited no test file. The only remaining `0.49` text in `src/` is two deliberate comment mentions (the historical annotation and the "moved from 0.49 to 0.495" narrative); no `0.49f` literal survives anywhere.

## Comment region 1 — the constant

**Before** (one line, trailing, pointing at this phase):

```cpp
constexpr float kVcoNyquistGuardFrac = 0.49f;   // PROVISIONAL — PITCH-04 (Phase 31) owns the real Nyquist policy; Phase 31 replaces this constant rather than rediscovering the intent
```

**After** — whole-line rationale above the declaration (the plan's instruction, since the acceptance criteria negative-grep the retired marker word), with a short trailing comment left on the declaration line:

```cpp
// PITCH-04's Nyquist policy, SETTLED (D-11; .planning/research/STACK.md:122).
// POLICY: clamp the oscillator frequency to kVcoNyquistGuardFrac * sampleRate,
// i.e. 0.5 x sampleRate x 0.99 — half the sample rate with a one-percent guard
// band. The DERIVED ceilings are what make the choice auditable:
//     44100 Hz -> 21829.5 Hz    48000 Hz -> 23760.0 Hz    96000 Hz -> 47520.0 Hz
// Every one of those is above human hearing, so the clamp is inaudible in
// normal use.
//
// Off the C4 reference those ceilings are the volts at which 1V/oct tracking
// INTENTIONALLY stops — log2(kVcoNyquistGuardFrac * sampleRate / kVcoFreqC4) —
// about +6.3826 V at 44100, +6.5049 V at 48000, +7.5049 V at 96000. Phase 31's
// TEST-02 gate DERIVES those volts from this constant instead of hardcoding
// them (D-21), so moving this constant moves the gate with it.
//
// D-10: this is a HARD CLAMP. The frequency PINS AT THE CEILING and the
// oscillator KEEPS SOUNDING — it does not mute, fade or fold. Under deep FM
// (1.0 oct/V at a full attenuverter means a +/-5 V audio-rate modulator swings
// +/-5 octaves) the clamp fires on most cycles and the peaks flatten out at the
// top. That flattening is the CHOSEN SOUND, not a defect. Amplitude fade above
// the threshold was considered and REJECTED because it adds a gain stage that
// collides with Phase 34's OUT-01..03; pitch fold-back was REJECTED because it
// is a deliberate effect, not the guard PITCH-04 asks for.
//
// D-13 is the counterpart at the LOW end: NO floor is added. Extreme negative
// pitch freezes the phase and the output becomes effectively DC — MEASURED at
// -64 V of pitch: freq = 1.418e-17 Hz, deltaPhase ~ 3.2e-22, an accumulator
// advancing by a denormal-scale amount. That is honest, and it is the decided
// behavior: PITCH-04 speaks only to the top end.
//
// This constant is a Nyquist POLICY bound on the FREQUENCY. It remains a
// different KIND of constant from the wrap-correctness bound on the phase
// INCREMENT declared just below, which D-12 leaves untouched.
constexpr float kVcoNyquistGuardFrac = 0.495f;  // 0.5 x sampleRate x 0.99 (PITCH-04 / D-11)
```

The `.planning/research/STACK.md:122` citation was **verified by reading that line** before writing it — it is Q2, which is where `min(freq, 0.5·sampleRate·0.99)` originates. The declaration stayed a namespace-scope plain `constexpr float` in the same block (31-PATTERNS Pattern 1); no `inline constexpr`, no in-class `static constexpr`.

## Comment region 2 — the wrap margin

**Before** (the arithmetic `0.495f` falsifies, in bold):

```cpp
		// WHY 0.5 AND NOT kVcoNyquistGuardFrac. At a COUPLED rate the guarded
		// frequency yields an increment of 0.49 plus float rounding, so a 0.49
		// ceiling could fire on a legitimate input and MOVE SAMPLES. 0.5 clears
		// that maximum by roughly two percent, leaves every existing measurement
		// bit-identical, and still satisfies the wrap (any bound < 1.0 does).
```

**After** — every conclusion preserved because none of them changed, plus the sentence the plan asked for:

```cpp
		// WHY 0.5 AND NOT kVcoNyquistGuardFrac. At a COUPLED rate
		// (in.sampleTime == 1 / in.sampleRate) the guarded frequency yields an
		// increment of 0.495 plus float rounding, so a 0.495 ceiling could fire
		// on a legitimate input and MOVE SAMPLES. 0.5 still clears that maximum
		// — by roughly one percent — leaves every existing measurement
		// bit-identical, and still satisfies the wrap (any bound < 1.0 does).
		//
		// That margin NARROWED, from about two percent to roughly one percent,
		// when PITCH-04's policy moved the guard fraction from 0.49 to 0.495.
		// D-12 FORBIDS widening this bound in response, and nothing is lost by
		// leaving it: 0.5 still clears the coupled-rate maximum, so the bound
		// still cannot fire on a legitimate coupled-rate input and still cannot
		// move a sample — which is why every existing measurement stayed
		// bit-identical across that constant change. The margin is DOCUMENTED
		// rather than absorbed. A future phase that raises kVcoNyquistGuardFrac
		// further must RE-CHECK this specific margin rather than assume it
		// survived, because at a guard fraction >= 0.5 this bound WOULD start
		// firing on a legitimate coupled-rate input.
```

The phrase "about two percent" is deliberate: the acceptance criterion negative-greps the exact string `roughly two percent`, so the historical value is stated without reproducing the retired claim verbatim.

## Comment region 3 — the historical measurement, annotated not recomputed

The CR-01 narrative's figures are untouched (`grep -c '21609'` returns `1`, and that line still reads `Observed tel.freqHz = -21609.00,`). What was added is a parenthetical immediately below:

```cpp
		// (Those four figures were measured under the pre-Phase-31 guard
		// fraction of 0.49. They are a HISTORICAL OBSERVATION of a reproduction
		// run, NOT a current expectation, so they are deliberately left at the
		// digits that were actually observed: do not recompute them against
		// 0.495 and do not search-and-replace them when the constant moves. No
		// assertion anywhere reads these numbers — the suite pins this behavior
		// symbolically, through kVcoNyquistGuardFrac itself.)
```

This is Pitfall 8 discharged explicitly: the annotation tells the *next* editor not to do the thing Pitfall 8 predicts, rather than relying on this summary to be read.

## `kVcoMaxDeltaPhase` is provably untouched (D-12)

Confirmed by reading and by diff, not by assumption:

```
$ git show 46c1129:src/dsp/VcoCore.hpp | grep -n 'kVcoMaxDeltaPhase = '
95:constexpr double kVcoMaxDeltaPhase = 0.5;
$ grep -n 'kVcoMaxDeltaPhase = ' src/dsp/VcoCore.hpp
133:constexpr double kVcoMaxDeltaPhase = 0.5;
```

Same type (`double`), same value (`0.5`), same spelling. Only the line number moved (95 → 133), and only because the new rationale above it is 38 lines longer.

## Line-level evidence: Task 2 is comment-only

The criterion asked for line-level evidence that Task 2 changed zero code. Filtering the diff to added/removed lines that end in a semicolon:

```
$ git diff -U0 src/dsp/VcoCore.hpp | grep -E '^[-+]' | grep -v '^[-+][-+]' | grep -E ';[[:space:]]*$'
   (empty)
```

And across the **whole plan**, the non-comment delta is exactly one line:

```
$ git diff 46c1129..070af28 -- src/dsp/VcoCore.hpp | grep -E '^[-+]' | grep -v '^[-+][-+]' \
    | grep -vE '^[-+][[:space:]]*//' | grep -vE '^[-+][[:space:]]*$'
-constexpr float kVcoNyquistGuardFrac = 0.49f;   // PROVISIONAL — ...
+constexpr float kVcoNyquistGuardFrac = 0.495f;  // 0.5 x sampleRate x 0.99 (PITCH-04 / D-11)
```

## Gate results

| Gate | Required | Observed |
|---|---|---|
| `make guards` | exit 0, `guard suite: PASS` | **PASS** — frozen manifest + LFO goldens + dependency audit + canary, all three scripts green after **each** task |
| `make strict` | exit 0, `strict C++11 gate: PASS` | **PASS** — `-std=c++11 -pedantic-errors` over all four TUs incl. `src/vco_compile_canary.cpp` |
| `make test` | 0 failed, unchanged counts | **72 / 2,616,112 / 0** — exact |
| CR-01 ordering | ceiling line < floor line | `:268` ceiling, `:269` floor |
| Source shape `[2b/5]` | `^struct VcoCore {$` == 1, step signature == 1 | **1 / 1** — signature never quoted into a comment |

`make guards` was run after Task 1 **and** after Task 2, per the plan's Pitfall-5 instruction, not only at the end.

## Milestone guardrail compliance

- `git diff --name-only 46c1129..HEAD` = `src/dsp/VcoCore.hpp`. Nothing else, in either commit.
- **No frozen header edited.** `src/dsp/LfoCore.hpp`, `src/dsp/Waveshape.hpp`, `src/dsp/RackCompat.hpp`, `src/dsp/MathConst.hpp` and `src/dsp/FROZEN.sha256` are all absent from the diff; `check_frozen.sh` PASSes inside `make guards` and the six LFO `.f32` goldens replay byte-identical inside `make test`.
- **`src/AnalogLFO.cpp` absent from the diff** (D-16). It was recompiled by `make strict` and stayed clean.
- **C++11 clean:** no `inline constexpr` definition and no in-class `static constexpr` introduced (non-comment count for both: **0**); `std::exp2` / `std::pow` still absent from `src/`; the constant remains namespace-scope plain `constexpr`.
- **No test file touched**, so nothing weakened a gate to accommodate the change.
- **PITCH-04 not marked complete** — see Deviations.

## Decisions Made

1. **`0.495f` exactly as decided.** D-11/D-12 fixed the value; no independent re-derivation was attempted. The derived ceilings and crossover volts written into the comment were taken from `31-RESEARCH.md` §Nyquist Policy's MEASURED table rather than recomputed here, so the source and the research cannot disagree.
2. **Rationale as whole-line comments, not a long trailing comment.** The plan called this out and it is the right shape independently: the block is 31 lines, and a trailing comment that long is unreadable and would have carried the retired marker word past the negative grep.
3. **The `STACK.md:122` citation was verified by reading it** before writing it into the source. A load-bearing comment that cites a line number is only as good as the line, and this repo has now been bitten four times by prose that outlived its mechanism.
4. **The margin sentence names the failure condition, not just the number.** "A future phase must re-check this margin" is weak guidance on its own, so the comment states *what* would go wrong and *when*: at a guard fraction ≥ 0.5 the increment bound starts firing on a legitimate coupled-rate input and moves samples. That is the actionable form.
5. **PITCH-04 deliberately NOT marked complete** (see Deviations), following 31-01's precedent with TEST-02.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 — Missing Critical] De-tensed `kVcoMaxDeltaPhase`'s cross-reference, which Task 1 falsified**

- **Found during:** Task 2.
- **Issue:** The nine-line `kVcoMaxDeltaPhase` rationale asserted that `kVcoNyquistGuardFrac` is a policy bound *"and Phase 31 (PITCH-04) **replaces it**"*, and closed with *"Phase 31 **must leave this one alone** when it retires the Nyquist constant."* Both are forward-looking instructions to a phase that, as of `2c39f0f`, has already acted. Leaving them standing is the same false-comment class the plan's own objective names — a comment telling a future reader to do something already done, in the exact file plan 30-08 was created to clean.
- **Tension with the plan, stated plainly:** Task 2's action text says to confirm this comment "survived Task 1 and this task **untouched**". I edited it anyway, because the plan's *objective* is explicit that "no comment in the file is falsified by the change", and the `must_haves` entry requires only that the comment "still explains why it is a different KIND of constant" — not byte-identity. The prohibition list constrains only the constant's **value and type**, both of which are untouched and verified above.
- **Fix:** Minimal tense correction. *"has now settled it"* replaces *"replaces it"*; *"Phase 31 left this one alone when it retired the Nyquist constant, exactly as D-12 required; the margin that narrowed as a result is DOCUMENTED at the wrap below rather than absorbed by widening this bound"* replaces the closing instruction. The DIFFERENT-KIND-OF-CONSTANT paragraph, the wrap-validity argument, the `< 1.0` rule and the double-type note are all preserved verbatim.
- **Files modified:** `src/dsp/VcoCore.hpp` (comment only).
- **Verification:** `grep -c 'constexpr double kVcoMaxDeltaPhase = 0.5;'` → `1`; declaration byte-identical vs `46c1129`; the KIND distinction still present at `:123-124`; `make guards` / `make strict` / `make test` all green.
- **Committed in:** `070af28` (Task 2 commit).

**2. [Rule 2 — Missing Critical] Banner clause corrected as part of Task 1**

The plan prescribed this, so it is not strictly a deviation — recorded because it is a *second* forward-reference the value change falsified. The banner listed *"the non-provisional Nyquist policy"* among what Phase 31 still owes. It now states the policy is settled at `0.495f` (PITCH-04 / D-11) and names the volt-domain summation as the remaining Phase 31 pitch work, landed by the following plan. Kept truthful **at this commit**; 31-03 rewrites it again when the summation lands.

### Deferred / not done, deliberately

**3. `PITCH-04` left unchecked in `REQUIREMENTS.md`, against the plan frontmatter**

- **Found during:** the `requirements mark-complete` step.
- **Issue:** `31-02-PLAN.md` declares `requirements: [PITCH-04]`, and PITCH-04 reads that *frequency is clamped just below Nyquist so extreme pitch/FM/sync never aliases via out-of-range frequency*. This plan settles the **policy constant** and documents it; it adds no assertion that the clamp fires. `31-RESEARCH.md:1027` grades PITCH-04's current coverage **⚠ partial** on exactly this point: `tests/test_vco_core.cpp:753`'s `freqNyquistBounded` is driven by hostile *timing*, not hostile *pitch*, so nothing yet observes the clamp engaging on a legitimate high note. Marking it complete here would reproduce the false green that Phase 30 deferred item 1 recorded for `PANEL-03`, and that 31-01 declined to repeat for TEST-02.
- **Action:** PITCH-04 stays `- [ ]` / `Pending`. **31-07** owns the pitch-driven Nyquist case (`-tc="*Nyquist*"`) that proves the clamp fires and the oscillator keeps sounding (D-10). The phase gate should confirm PITCH-04 only after 31-07.
- **Recorded in:** STATE.md § Accumulated Context.

### Verification-command note (no code impact)

Task 1's criterion `grep -cE 'inline constexpr' src/dsp/VcoCore.hpp` returns **`2`**, not `0`. Both hits are **pre-existing comment lines** that *name the banned construct in order to forbid it* — `:49` (`// - No \`inline constexpr\` variables (C++17 inline variables)`) and `:84` (`// banner mandates above. NOT \`inline constexpr\` (C++17)...`). The count at the pre-plan commit `46c1129` is also `2`, so this plan introduced neither.

The substantive claim the criterion encodes is **true and verified** via the non-comment form the sibling criterion already uses:

```
$ grep -v '^[[:space:]]*//' src/dsp/VcoCore.hpp | grep -cE 'inline constexpr'
0
```

Independently corroborated by `make strict` (a `-pedantic-errors` C++11 leg would reject a real `inline constexpr` variable) and by `check_canary.sh [4/5]`, whose negative control confirms that a namespace-scope `inline constexpr` variable *is* rejected for the expected reason. This is a plan-authoring artifact in the criterion's filter — the same class as 31-01's BSD-`grep` note — not a finding about the header. **Future plans writing this criterion should scope it to non-comment lines**, since this file deliberately quotes every construct it bans.

---

**Total deviations:** 2 auto-fixed (both Rule 2, comment-truth), 1 deliberate non-action (requirement not falsely marked), 1 criterion artifact documented.
**Impact on plan:** No scope creep. Both prescribed edits landed exactly as specified. Every deviation is comment-only; the plan's entire non-comment footprint remains the single constant line.

## Issues Encountered

- None blocking. Note for later plans in this phase: `gsd-tools query state.record-metric` / `state.add-decision` / `state.record-session` take **named flags**, not the positional arguments the `execute-plan.md` workflow shows (carried forward from 31-01's finding, and confirmed again here).

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- **Ready for 31-03.** `make guards`, `make strict` and `make test` are all green at the exact regression floor, and the constants block is in its final Phase-31 shape apart from the `kVcoMaxPitchVolts` addition 31-03 makes. 31-03 lands `kVcoMaxPitchVolts = 64.f` in the same block, in the same namespace-scope plain `constexpr` form.
- **31-03 must rewrite the banner again.** It is truthful *at this commit* and deliberately says the volt-domain summation is still owed. When 31-03 widens the `exp2_taylor5` argument, that clause becomes false — the same trap this plan just cleared twice.
- **The pitch expression is untouched and waiting.** `float freq = kVcoFreqC4 * exp2_taylor5(in.pitchCV);` is byte-identical, with its existing comment still naming this phase's remaining job.
- **31-05/31-07 inherit a moving target on purpose.** The crossover volts are now `+6.3826 / +6.5049 / +7.5049 V`, and D-21 requires the gate to *derive* them from the constant. The rationale comment states this, so a future constant move drags the gate with it rather than silently diverging.
- **Open for the phase gate:** PITCH-04 remains unchecked by design and must be confirmed after **31-07**, not assumed. TEST-02 likewise (31-01's decision stands).
- **No blockers.**

## Self-Check: PASSED

- `src/dsp/VcoCore.hpp` — FOUND
- `.planning/phases/31-pitch-tuning-exponential-fm/31-02-SUMMARY.md` — FOUND
- Commit `2c39f0f` — FOUND
- Commit `070af28` — FOUND
- No file deletions in either task commit (`git diff --diff-filter=D` empty for both); no untracked residue (`git status --short` empty after each commit).

---
*Phase: 31-pitch-tuning-exponential-fm*
*Completed: 2026-07-30*
