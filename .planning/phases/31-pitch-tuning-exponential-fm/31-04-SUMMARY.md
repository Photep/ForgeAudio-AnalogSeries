---
phase: 31-pitch-tuning-exponential-fm
plan: 04
subsystem: rack-shell
tags: [cpp11, rack-sdk, configParam, param-display-units, bipolar-attenuverter, panel-geometry, comment-truth, no-dsp-in-shell]

# Dependency graph
requires:
  - phase: 30-vcocore-skeleton-module-registration
    provides: "the shell itself — its no-DSP banner, the four durable widget coordinates, the six-rect throwaway panel at final 18HP geometry, and the seeding block whose literals this plan leaves byte-unchanged"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 03
    provides: "the core-side pitch chain that reads coarse, fine, fmVolts, fmAtten and fmConnected, including the fmConnected gate placed in the CORE and the kVcoMaxPitchVolts bound that contains the raw cable voltage this plan makes reachable"
provides:
  - "ParamId::COARSE_PARAM (-5..+5 oct, default 0, continuous), ParamId::FINE_PARAM (raw SEMITONES, display multiplier 100 -> cents), ParamId::FM_ATTEN_PARAM (BIPOLAR -1..+1, percentage), InputId::FM_INPUT"
  - "five bare POD forwards in process() — coarse, fine, fmAtten, fmVolts, fmConnected — with BOTH FM fields unconditional, so the shell still performs zero arithmetic (region-scoped operator count = 0)"
  - "eight widget placements against eight marker rects, correspondence verified mechanically in BOTH directions; the four Phase-30 coordinates and the panel's width/height/viewBox unmoved"
  - "the shell's field-count argument restated on the ground that survives Phase 34 closing the margin — the canary is the TU the guard compiles against a perturbed header and the only VCO TU link-checkable without the SDK"
  - "an in-Rack-auditable module: every control a user can move is a control the 31-03 DSP consumes, which is what makes 31-09's operator session honest"
affects: [31-05, 31-06, 31-07, 31-08, 31-09, 35-shell-panel-display]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "A shell that forwards RAW param values in the POD's documented units keeps every unit conversion in one place; the SDK's display multiplier handles the tooltip so no arithmetic is needed to show a different unit than the field carries"
    - "When a comment's numeric MARGIN narrows to one, restate the conclusion on the structural property that holds it up — an argument resting on a margin is one phase from becoming false again"
    - "Two files that encode the same geometry are verified by mechanical set-comparison in BOTH directions (no coordinate without a rect, no rect without a coordinate), not by reading them side by side"
    - "grep -c on a bare `IDENT,` pattern cannot isolate an enum entry: `configParam(IDENT, ...)` matches the same pattern. Anchor with ^[[:space:]]*IDENT,$ when the claim is about the enum"

key-files:
  created: []
  modified:
    - "src/AnalogVCO.cpp"
    - "res/AnalogVCO.svg"

key-decisions:
  - "Rack's default display precision is LEFT ALONE, and the divergence from D-04's illustrative digit count is recorded in the source beside the FINE declaration rather than silently absorbed"
  - "Both FM fields forwarded UNCONDITIONALLY; the shipped LFO shell's zeroing ternary is named in the comment as the anti-pattern, with three independent reasons"
  - "The five forwards were placed AFTER the existing assignments exactly as the plan directed, rather than regrouped next to pitchCV — zero churn on Phase-30 lines"
  - "The rewritten canary argument leads with the STRUCTURAL reason (perturbed-header compile + SDK-free link check) and demotes the field count to corroboration, because the count margin is now one field"
  - "Two further banner claims falsified by this phase were auto-corrected (Rule 2); one candidate was checked and deliberately left because it is still true as written"
  - "No requirement marked complete — FOURTH consecutive plan in this phase making that call, for the reason 31-03 recorded and 31-06 discharges"

patterns-established:
  - "The eight-way widget/rect correspondence check is a reusable two-direction script; Phase 35 replaces the art and can re-run it unchanged against the real panel"
  - "A stale-claim sweep (grep for every number and phase reference in the file's prose) runs AFTER the last code edit of a plan, because the last edit is what falsifies the prose"

requirements-completed: []

coverage:
  - id: D1
    description: "All four controls are DECLARED with the exact ranges, defaults, names, unit strings and display multipliers the decisions fix (D-02/D-03/D-04/D-07, PITCH-02/PITCH-03/FM-02)"
    requirement: "PITCH-02"
    verification:
      - kind: integration
        ref: "exact-string grep == 1 for each of the four declarations, quoted verbatim below; enum entries anchored ^[[:space:]]*IDENT,$ == 1 each for COARSE_PARAM/FINE_PARAM/FM_ATTEN_PARAM/FM_INPUT; PARAMS_LEN == 2 and INPUTS_LEN == 2 (sentinels still last and still passed to config)"
        status: pass
      - kind: integration
        ref: "grep -c 'displayPrecision' == 0 — Rack's default is left alone and the divergence from D-04's illustration is recorded in source instead"
        status: pass
    human_judgment: false
  - id: D2
    description: "The shell computes NOTHING: five bare assignments, no ternary, no arithmetic operator anywhere in the delegation body (D-17)"
    verification:
      - kind: integration
        ref: "region-scoped, comments stripped: sed -n '/void process(...)/,/^\\t}$/p' | grep -v '^[[:space:]]*//' | grep -cE '\\?|\\*|/|\\+|(^|[^-])-[^->]' == 0; core.step(in) == 1; forge::VcoInputs in; == 1 (default-construct then assign, never a brace value list)"
        status: pass
      - kind: integration
        ref: "whole-plan non-comment delta is 21 ADDED lines and 0 REMOVED — purely additive in code; no Phase-30 code line was rewritten"
        status: pass
    human_judgment: false
  - id: D3
    description: "Both FM fields are forwarded unconditionally, so the gate stays in the core where D-09 put it and the shipped LFO's shell-side ternary is not mirrored"
    requirement: "FM-01"
    verification:
      - kind: integration
        ref: "grep -c 'in.fmVolts' == 1 on a line containing inputs[FM_INPUT].getVoltage(); grep -c 'in.fmConnected' == 1 on a line containing inputs[FM_INPUT].isConnected(); zero ternaries in the region (see D2). src/AnalogLFO.cpp absent from all four commits"
        status: pass
    human_judgment: false
  - id: D4
    description: "Every control is reachable on the panel, and the widget coordinates correspond one-to-one with the marker rects in both directions (D-16)"
    requirement: "FM-02"
    verification:
      - kind: integration
        ref: "mm2px == 8; rects == 10; #2a2a30 rects == 8; forward check (every widget coord has a rect at cx-5, cy-5) 8/8 YES with control names; reverse check (every rect has a widget) 8/8 OK; sorted set diff of expected-vs-actual EMPTY"
        status: pass
      - kind: integration
        ref: "the four durable coordinates == 4 and the root width=\"91.44mm\" height=\"128.5mm\" viewBox=\"0 0 91.44 128.5\" == 1 — geometry and Phase-30 placements unmoved; box.size = == 0 (never hardcoded); struct .*Knob|struct .*Port == 0 (no custom widget)"
        status: pass
    human_judgment: false
  - id: D5
    description: "Both comments this phase falsified state numbers re-derived from the source this session, and the conclusion no longer rests on the field-count margin"
    verification:
      - kind: integration
        ref: "field sets derived programmatically from struct VcoInputs (float fields minus injected sampleTime/sampleRate = 8) and cross-checked against process() and src/vco_compile_canary.cpp: shell-fed 7/8, canary-fed 8/8, drift the sole gap. check_canary.sh [2b/5] independently prints 'all 8 VcoInputs DSP field(s) stay runtime-live through step() at -O3'"
        status: pass
      - kind: integration
        ref: "grep -c 'coarse, fine, fmVolts, fmAtten and fmConnected' == 0 and grep -c 'THREE of the eight' == 0 (both falsified strings gone); grep -ci 'canary' == 7 (rewritten, not deleted)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The regression floor holds exactly and the plugin still links against the real SDK"
    verification:
      - kind: unit
        ref: "make test after every task and after the comment commit: 72 / 2,616,112 / 0 — IDENTICAL to the phase floor at all four points"
        status: pass
      - kind: integration
        ref: "make guards PASS and make strict PASS after every task; RACK_DIR=../Rack-SDK make links plugin.dylib (169,072 bytes) after Task 3 and again after the final comment commit"
        status: pass
    human_judgment: false
  - id: D7
    description: "The milestone guardrail is untouched and the seeding literals are byte-unchanged"
    verification:
      - kind: integration
        ref: "git diff --name-only over all four commits == res/AnalogVCO.svg + src/AnalogVCO.cpp; filtered for 'AnalogLFO|FROZEN|src/dsp/|tests/' == 0; grep -c 'AnalogLFO.cpp' in the shell held at its baseline of 2; ForgeAnalogLFO|modelAnalogLFO == 0"
        status: pass
      - kind: integration
        ref: "core.seed(0x1234ULL, 0x5678ULL) == 1 and core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL) == 1; the prohibition comment above them is untouched (T-31-08)"
        status: pass
    human_judgment: false

# Metrics
duration: 7min
completed: 2026-07-30
status: complete
---

# Phase 31 Plan 04: Give the Pitch Chain a Body a Human Can Reach Summary

**COARSE, FINE, FM DEPTH and FM IN are declared, forwarded and placed — the shell grew four enum entries, four declarations, five bare assignments and four widget placements against four new marker rects, with a region-scoped operator count of ZERO proving it still computes nothing, and with both comments this phase falsified restated on numbers re-derived from the source rather than trusted from the plan.**

## Performance

- **Duration:** 7 min
- **Started:** 2026-07-30T01:44:51Z
- **Completed:** 2026-07-30T01:51:47Z
- **Tasks:** 3 (plus one Rule-2 comment-truth commit)
- **Files modified:** 2

## Accomplishments

- **The phase became auditable in Rack.** Before this plan the 31-03 pitch chain was real arithmetic that nothing in Rack could move. Now every term in `pitchVolts = in.pitchCV + in.coarse + in.fine/12 (+ gated FM)` has a control attached to it, which is precisely what makes 31-09's operator session a test rather than a demonstration.
- **The shell still computes nothing, and that is MEASURED rather than asserted.** The region-scoped check over `process()` with comments stripped finds **zero** ternaries and **zero** arithmetic operators. The whole-plan non-comment delta is **21 added lines and 0 removed** — no Phase-30 code line was rewritten, so D-17's banner claim is true by construction and not by care.
- **The anti-pattern was named in the source, not just avoided.** The shipped LFO's shell zeroes its FM voltage behind a conditional (`src/AnalogLFO.cpp:320`). The comment beside the new forwards says so, and gives **three independent reasons** the placement is wrong here: Rack already returns 0 V unpatched; the core's gate is strictly stronger (it does not *evaluate* the term, rather than substituting zero); and a conditional here would be the first computation in a file whose banner promises none.
- **The falsified comment numbers were re-derived, not copied from the plan.** The plan said to count the POD and the canary myself. Doing so programmatically produced **7 of 8** for the shell and **8 of 8** for the canary, with **`drift`** as the sole gap — and `check_canary.sh [2b/5]` independently prints *"all 8 VcoInputs DSP field(s) stay runtime-live through step() at -O3"*, which corroborates the denominator from a guard this plan never touched.
- **The canary argument was moved off the margin it was resting on.** With the field count now 7-vs-8, an argument built on "eight beats three" would have been one phase from collapsing. The rewritten paragraph leads with the structural reasons instead — the canary is the TU the guard **compiles against a deliberately perturbed copy** of the VCO header, and the only VCO TU that is **link-checkable without the Rack SDK** — and notes explicitly that neither property is one this shell could ever acquire.
- **The two-file geometry was verified mechanically in BOTH directions.** Not by eye: every widget coordinate was checked for a rect at `(cx−5, cy−5)` **and** every rect was checked for a widget, with the sorted set difference empty. Eight for eight, both ways.
- **The stale-claim sweep caught two more falsified banner claims** that the plan did not predict, and one candidate that was checked and correctly left alone (below).

## Task Commits

1. **Task 1: Declare the four controls — enum entries, three `configParam` calls and one `configInput`** — `ac26bfe` (feat)
2. **Task 2: Forward the five POD fields with zero arithmetic, and correct the two comments this phase falsifies** — `2ddd4ba` (feat)
3. **Task 3: Place the four widgets and draw their four marker rects, written together** — `52c2a2c` (feat, both files in one commit)
4. **Rule-2 follow-up: retire two further banner claims this phase falsified** — `4bb5c9c` (docs, comment-only)

## Files Created/Modified

- `src/AnalogVCO.cpp` — three param IDs, one input ID, four declarations with their rationale comments, five POD forwards, four widget placements, and five comment corrections
- `res/AnalogVCO.svg` — four appended `10 × 10` `#2a2a30` marker rects (six rects → ten); the existing six byte-unchanged and in their original order

---

## The four declarations, verbatim

```cpp
configParam(COARSE_PARAM, -5.f, 5.f, 0.f, "Coarse Tune", " oct");
configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine Tune", " cents", 0.f, 100.f);
configParam(FM_ATTEN_PARAM, -1.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f);
configInput(FM_INPUT, "FM");
```

Each matched by exact-string `grep -c` returning `1`. The unit-string convention is the shipped
module's: a **leading space** for word units (`" oct"`, `" cents"`), **none** for `"%"`.

| Control | Raw range → POD field | Display | Decisions |
|---|---|---|---|
| COARSE | −5..+5 **octaves** → `in.coarse` | `+2.0000 oct` | D-02, D-04, D-05, PITCH-02 |
| FINE | −1..+1 **semitones** → `in.fine` | `-14.000 cents` (×100 in the SDK) | D-00, D-03, D-04, D-05, PITCH-03 |
| FM DEPTH | −1..+1 **bipolar** → `in.fmAtten` | `-100.00%` .. `+100.00%` | D-07, FM-02 |
| FM IN | volts → `in.fmVolts` + `in.fmConnected` | — | D-09, FM-01 |

**FINE's raw range is semitones and its readout is cents, and that pair is the whole point.** It is
the only shape that satisfies D-05 (the POD is documented in semitones and the shell forwards raw)
and D-04 (the tooltip reads in cents) at the same time. The `100.f` is the SDK's **display**
multiplier, so the conversion happens inside the quantity object and *never* in this file — doing it
in the shell would have been the exact D-17 violation the plan's own pitfall table warns about.

**FM DEPTH borrows the shipped module's styling and explicitly not its range.** Same control name,
same linear taper, same default-off, same percentage readout — but `-1..+1` rather than the shipped
`0..1`, because that control is a unipolar attenuator and FM-02 asks for an attenuverter. The
comment says so, so the next editor comparing the two files sees a decision rather than a typo.

## The five forwards, verbatim

```cpp
in.coarse = params[COARSE_PARAM].getValue();
in.fine = params[FINE_PARAM].getValue();
in.fmAtten = params[FM_ATTEN_PARAM].getValue();
in.fmVolts = inputs[FM_INPUT].getVoltage();
in.fmConnected = inputs[FM_INPUT].isConnected();
```

No division, no scaling, no summation, no bound, no smoothing, no conditional. Placed **after** the
existing assignments and before the delegation call, exactly where the plan directed, so no
Phase-30 line moved.

### The D-17 proof, region-scoped and comment-stripped

```
$ sed -n '/void process(const ProcessArgs& args) override {/,/^	}$/p' src/AnalogVCO.cpp \
    | grep -v '^[[:space:]]*//' | grep -cE '\?|\*|/|\+|(^|[^-])-[^->]'
0
$ sed -n '/void process(const ProcessArgs& args) override {/,/^	}$/p' src/AnalogVCO.cpp \
    | grep -c 'core.step(in)'
1
```

Zero ternaries, zero arithmetic operators, exactly one delegation. `forge::VcoInputs in;` appears
once — default-construct then assign, never a brace value list, because the POD's NSDMIs make it a
non-aggregate under C++11 and that is a hard error rather than a style question.

## Comment correction one — the header-default list

**BEFORE** (false the moment Task 2 landed — it names five fields this plan just wired):

```
// The remaining forge::VcoInputs fields stay at their header defaults:
// coarse, fine, fmVolts, fmAtten and fmConnected are Phase 31's, drift
// is Phase 34's, and each is wired by the phase that lands the DSP
// reading it.
```

**AFTER** (names exactly one field, and preserves the rule that generated the list):

```
// Exactly ONE forge::VcoInputs field is still left at its header
// default: drift, which belongs to Phase 34. The rule that produced the
// longer list this sentence used to carry has not changed — each field
// is wired by the phase that lands the DSP reading it — and Phase 31 is
// that phase for the five directly above, so they moved out of this
// sentence and into the block above.
```

## Comment correction two — the canary argument

**BEFORE** (both numbers false, and the conclusion resting on the arithmetic):

```
// Consequence worth stating here, because this file's arrival is the
// moment it looks wrong: this shell feeds runtime-derived values into
// only THREE of the eight VcoInputs DSP fields, while
// src/vco_compile_canary.cpp feeds all EIGHT. That is what keeps
// check_canary.sh [2b/5] reporting eight fields runtime-live at -O3.
// This file therefore does NOT make the canary redundant — swapping one
// for the other would silently cut constant-fold coverage from eight
// fields to three, in exactly the fields Phases 31, 33 and 34 are about
// to make load-bearing.
```

**AFTER** (true numbers, `drift` named, conclusion re-founded, growth rule preserved):

```
// Consequence worth stating here, with numbers this phase moved: this
// shell now feeds runtime-derived values into SEVEN of the eight
// VcoInputs DSP fields, while src/vco_compile_canary.cpp feeds all
// EIGHT. The single field the canary feeds and this shell does not is
// drift. That is what keeps check_canary.sh [2b/5] reporting eight
// fields runtime-live at -O3.
//
// The field-count margin is therefore down to ONE, so read what follows
// as the actual load-bearing argument rather than as arithmetic — it
// never was the arithmetic, and saying so now matters because Phase 34
// closes the gap entirely. The canary is the translation unit that guard
// COMPILES against a deliberately perturbed copy of the VCO header, and
// it is the only VCO translation unit that is link-checkable WITHOUT the
// Rack SDK. Neither property is one this file can acquire: this shell
// cannot be compiled at all without the SDK, and it cannot be perturbed
// by a guard whose whole discipline is to leave the real source alone.
// So it could not substitute for the canary even if it fed every field
// twice over, and retiring the canary once the counts converge would
// delete the only local gate that sees the constant-fold class at all —
// the class that got v2.0.0 rejected from the library.
//
// Growth rule, unchanged and still binding: the next phase that adds a
// VcoInputs field must give the canary a runtime value for it, or
// [2b/5] quietly stops covering that field while still reporting PASS.
```

### How the new numbers were verified — derived, not trusted

The plan said to count the POD and the canary myself rather than trust its numbers. The field set
was extracted **from the header**, filtered exactly as `tests/check_canary.sh:197-204` filters it
(float fields, minus the injected `sampleTime`/`sampleRate`), then cross-referenced against
`process()` and against `src/vco_compile_canary.cpp`:

| DSP field | Fed by the shell | Fed by the canary |
|---|---|---|
| `pitchCV` | yes | yes |
| `coarse` | yes (**new**) | yes |
| `fine` | yes (**new**) | yes |
| `fmVolts` | yes (**new**) | yes |
| `fmAtten` | yes (**new**) | yes |
| `morph` | yes | yes |
| `character` | yes | yes |
| **`drift`** | **no — header default, Phase 34's** | yes |
| **total** | **7 of 8** | **8 of 8** |

`fmConnected` is a `bool` and therefore outside the eight, but the shell now feeds it too.
Independent corroboration of the denominator, from a guard this plan did not touch:

```
$ make guards | grep 'runtime-live'
  OK: all 8 VcoInputs DSP field(s) stay runtime-live through step() at -O3
```

## The eight-way widget-to-rect correspondence

Forward direction — every widget coordinate has a rect at `(cx − 5, cy − 5)`:

| Control | Widget `mm2px(Vec(cx, cy))` | Expected rect | Present in SVG? |
|---|---|---|---|
| `MORPH_PARAM` | (30.48, 40) | `x="25.48" y="35"` | YES |
| `CHARACTER_PARAM` | (60.96, 40) | `x="55.96" y="35"` | YES |
| `COARSE_PARAM` | (20.32, 60) | `x="15.32" y="55"` | YES |
| `FINE_PARAM` | (45.72, 60) | `x="40.72" y="55"` | YES |
| `FM_ATTEN_PARAM` | (71.12, 60) | `x="66.12" y="55"` | YES |
| `VOCT_INPUT` | (30.48, 100) | `x="25.48" y="95"` | YES |
| `FM_INPUT` | (45.72, 100) | `x="40.72" y="95"` | YES |
| `OUTPUT` | (60.96, 100) | `x="55.96" y="95"` | YES |

Reverse direction — every marker rect has a widget, **8 of 8 OK**, no orphan rect. The sorted set
difference between "coordinates minus five" and "rect origins" is **empty**, so the correspondence
is a bijection rather than merely a covering.

Widget types are stock SDK only: five `RoundBlackKnob`, two `PJ301MPort` inputs, one `PJ301MPort`
output. `struct .*Knob|struct .*Port` returns **0** — no custom widget struct, so nothing was
extracted out of the shipped module's translation unit.

**Geometry unmoved:** the root `width="91.44mm" height="128.5mm" viewBox="0 0 91.44 128.5"` is
byte-identical, the four Phase-30 coordinates all still match, and `box.size =` returns `0` because
the panel call derives the widget's box size from the asset and always must.

## Gate results

| Gate | Required | Task 1 | Task 2 | Task 3 | After `4bb5c9c` |
|---|---|---|---|---|---|
| `make strict` | exit 0, `strict C++11 gate: PASS` | **PASS** | **PASS** | **PASS** | **PASS** |
| `make guards` | exit 0, `guard suite: PASS` | **PASS** | **PASS** | **PASS** | **PASS** |
| `make test` | 0 failed, unchanged counts | **72 / 2,616,112 / 0** | **72 / 2,616,112 / 0** | **72 / 2,616,112 / 0** | **72 / 2,616,112 / 0** |
| `RACK_DIR=../Rack-SDK make` | links `plugin.dylib` | — | — | **169,072 bytes** | **169,072 bytes** |

The phase regression floor is **72 / 2,616,112 / 0** and it is met **exactly** at every point. That
is the expected result and it is worth saying why it is not a weakness: this plan adds no test and
no core arithmetic, and the existing grid drives `forge::VcoCore` through
`tests/VcoBlockDriver.hpp` rather than through the Rack shell, which cannot be compiled without the
SDK. The gate that exercises this file end to end is the **real link**, and the gate that will
exercise these controls behaviorally is **31-06**.

## Milestone guardrail compliance

- `git diff --name-only ac26bfe~1 4bb5c9c` = **`res/AnalogVCO.svg`** and **`src/AnalogVCO.cpp`**. Nothing else, in any of the four commits.
- **`src/AnalogLFO.cpp` and `res/AnalogLFO.svg` are absent from every commit** — the cleanest position against the guardrail, held for the third phase running. The shell's textual references to that filename stayed pinned at their **baseline of 2** (captured before Task 1 and re-checked after every task); the D-08 banner needs them and gained none.
- `ForgeAnalogLFO` / `modelAnalogLFO`: **0** occurrences.
- **No frozen header edited.** `src/dsp/` and `src/dsp/FROZEN.sha256` are absent from the diff; `check_frozen.sh` PASSes inside `make guards` and the **six LFO `.f32` goldens replay byte-identical** inside every `make test` above.
- **No `tests/` file touched**, so nothing was weakened to accommodate the change.
- **Seeding literals byte-unchanged** (`0x1234/0x5678` and `0x9E3779B9/0x7F4A7C15`, each `grep -c` = 1) and their prohibition comment untouched — T-31-08's degenerate-seed hang stays out of reach.
- **C++11 clean:** `make strict` (`-std=c++11 -pedantic-errors`) PASSes over all four TUs after every task, and the real build compiles this file at `-std=c++11 -O3`. No `std::exp2`/`std::pow`, no standard clamp helper, no C++17 construct, no brace value-list init of the POD.
- **No display precision set anywhere** (`grep -c 'displayPrecision'` = 0).

## Threat mitigations applied

| Threat ID | Disposition | Evidence |
|---|---|---|
| T-31-01 | mitigate (upstream) | `in.fmVolts = inputs[FM_INPUT].getVoltage();` is indeed the line that first makes an unsanitised cable voltage reachable from the exponent argument. **No sanitising was added here, by design** — the containment is 31-03's `kVcoMaxPitchVolts` bound with its NaN-rejecting negated comparison, evidenced by that plan's clean UBSan transcript over an FM-connected non-finite grid |
| T-31-16 | mitigate | Region-scoped operator count = **0**; the shipped shell's conditional is named in the source comment as the anti-pattern, so the temptation is documented rather than merely resisted |
| T-31-04 | mitigate | Stock widgets only (no struct extracted from released source); LFO filename reference count pinned at 2; diff-name check for `AnalogLFO|FROZEN|src/dsp/|tests/` returns 0; `make guards` and the six goldens green after every task |
| T-31-17 | mitigate | Both files in **one** commit (`52c2a2c`); bijective correspondence verified in both directions; `box.size =` = 0 |
| T-31-08 | mitigate | Both seeding literals byte-unchanged, prohibition comment intact |
| T-31-18 | mitigate | `make strict` after every task **plus** a real `-O3` compile-and-link of this exact TU; the local link is the only gate Phase 29 measured as catching the ODR/link class, and 31-08 still owns observing the CI MinGW leg by SHA |
| T-31-SC | accept | Zero registry packages; no install command run at any point |

## Decisions Made

1. **Rack's default display precision is left alone, and the divergence is written into the source.**
   The default yields five significant digits, so the tooltips read `+2.0000 oct` and `-14.000 cents`
   rather than D-04's illustrative `+2.00 oct` / `-14.0 cents`. D-04 fixes the **units**, not the
   digit count; the shipped module tightens the digit count nowhere either; and the phase constraint
   states this explicitly. Recording the divergence beside the FINE declaration — with the one-line
   remedy on the pointer the declaration returns — means the next reader who notices the mismatch
   finds a decision instead of a bug.
2. **Both FM fields forwarded unconditionally, with the anti-pattern named and three reasons given.**
   Any one of the three would justify it: Rack already returns 0 V unpatched; the core's gate does
   not evaluate the term at all, which is strictly stronger than substituting zero; and a conditional
   here would be the first computation in a file whose banner promises none. Writing all three down
   means the next editor who "simplifies" this by copying the shipped module cannot do so accidentally.
3. **The five forwards were placed after the existing assignments, exactly as directed.** Grouping
   them next to `in.pitchCV` would have read marginally better and would have touched Phase-30 lines
   for cosmetics. The result is a **purely additive** non-comment delta: 21 lines added, 0 removed.
4. **The canary argument leads with structure and demotes the count to corroboration.** The plan
   warned that an argument resting on the field-count margin "would be one phase from becoming false
   again", and it was right — the margin is now a single field and Phase 34 closes it. The two
   structural properties (compiled against a perturbed header; link-checkable without the SDK) are
   ones this shell **cannot** acquire, which makes the conclusion permanent rather than current.
5. **The unit-string spacing convention was copied from the shipped module rather than guessed:**
   leading space for word units, none for the percent sign. Verified by reading
   `src/AnalogLFO.cpp:200-214` (read-only; that file is absent from the diff).
6. **No requirement marked complete** — see Deviations. Fourth consecutive plan making that call.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 — Missing Critical] The banner's "six rectangles" claim was falsified by Task 3's own edit**

- **Found during:** Task 3, immediately after the SVG edit.
- **Issue:** The banner read *"res/AnalogVCO.svg is **six** rectangles at the FINAL 18 HP geometry"*. Task 3 appended four, making it ten. A file that documents its own asset by rect count cannot be allowed to miscount it — this is the same comment-truth class 31-02 cleared twice and 31-03 cleared twice more.
- **Fix:** `six rectangles` → `ten rectangles`. Nothing else in the paragraph changed; the throwaway-on-purpose argument and the Phase-35 art-swap framing are preserved verbatim.
- **Files modified:** `src/AnalogVCO.cpp` (comment only).
- **Committed in:** `52c2a2c` (same commit as the SVG edit that falsified it, deliberately).

**2. [Rule 2 — Missing Critical] The banner's opening line still described the file as the Phase-30 skeleton's shell**

- **Found during:** the post-edit stale-claim sweep (grep over every number and phase reference in the file's prose), run after the last code edit because the last edit is what falsifies prose.
- **Issue:** Line 3 read *"The minimum-viable Rack shell for the **Phase-30 VCO skeleton**."* After this plan the file carries four Phase-31 controls and forwards five Phase-31 fields, so the one-line self-description is no longer what the file is.
- **Fix:** *"The minimum-viable Rack shell for the VCO — Phase 30 built it around the skeleton core, Phase 31 added the tune and FM controls."* The ownership sentence that follows (Rack indices here, arithmetic in the core, the same POD the Phase-29 harness drives) is untouched.
- **Committed in:** `4bb5c9c`.

**3. [Rule 2 — Missing Critical] The serialization sentence was scoped to Phase 30 and read as stale from inside Phase 31**

- **Found during:** the same sweep.
- **Issue:** *"...as is any patch-state serialization: the VCO persists nothing in **Phase 30**."* The claim was *technically still true* as a historical statement, which is what makes it the more dangerous variety — a reader in Phase 31 cannot tell whether it was left deliberately or simply not revisited, and four controls were just added whose values a user will reasonably expect a patch to remember.
- **Fix:** *"...the VCO persisted nothing in Phase 30 and still persists nothing in Phase 31, so no control declared above survives a patch save yet."* This states the current fact and names the user-visible consequence of it.
- **Committed in:** `4bb5c9c`.

Both `4bb5c9c` edits are comment-only, verified: `git diff -U0` over that commit contains **zero**
non-comment changed lines, and all four gates re-ran green afterwards.

### Considered and deliberately NOT changed

**4. The seeding block's "the four literals" phrasing** was checked during the sweep and left
untouched. There are still exactly four literals, the sentence is true, and the block carries an
explicit prohibition against editing it — including against "fixing" the clone behavior by
hand-picking different values. Leaving a true comment alone is as much a part of comment-truth
discipline as correcting a false one.

### Deferred / not done, deliberately

**5. No requirement marked complete, against the plan frontmatter's `[PITCH-02, PITCH-03, FM-01, FM-02]`**

- **Found during:** the `requirements mark-complete` step.
- **Issue:** This plan lands the **controls**. It adds **no behavioral assertion**, and the phase's
  own artifact list assigns the assertions that make these requirements non-vacuous to **31-06**:
  the `PITCH-02 COARSE` and `PITCH-03 FINE` test cases and the `*exponential FM*` case with its
  `DeliberatelyMultiplicativeFmCore` broken control. Reading the requirement texts confirms it
  individually — PITCH-02 says the knob *"sweeps ±5 octaves"* and PITCH-03 that it *"trims ±1
  semitone"*, both of which are claims about how far the **pitch** moves when the knob turns, not
  merely about a declared range. FM-01 (*"modulates pitch at audio rate"*) needs 31-06's audio-rate
  case. FM-02 (*"a dedicated bipolar attenuverter sets FM depth"*) is the closest to provable by
  construction here, and this summary's D1/D4 entries record the source assertions — but marking it
  would claim a gate that does not yet exist.
- **Action:** all four stay `- [ ]` / `Pending`. 31-03's hand-off said precisely this: *"confirm
  PITCH-02, PITCH-03, FM-01, FM-02, FM-03 after 31-04 **and** 31-06."* **This is the fourth
  consecutive plan in this phase declining to mark a requirement for the same reason**, after 31-01
  (TEST-02), 31-02 (PITCH-04) and 31-03 (seven of them) — which makes it a standard rather than a
  judgment call, and avoids reproducing the false green Phase 30 recorded for PANEL-03.
- **For the phase gate:** unchanged from 31-03 — `PITCH-01` after 31-05; `PITCH-02`, `PITCH-03`,
  `FM-01`, `FM-02`, `FM-03` after **31-06**; `PITCH-04` after 31-07; `PITCH-05` after 31-05;
  `TEST-02` after 31-05/06/07.
- **Recorded in:** STATE.md § Accumulated Context.

**6. `deferred-items.md` was NOT created — 31-08 owns it**, and this plan's diff-name criterion
requires exactly two files. Three items surfaced here for that plan to collect:
  - **Octave detents on COARSE.** PITCH-02 says "continuously" and D-02 honours it literally, so
    nothing here enables integer stepping or offers a right-click menu. The idea is reasonable and
    is recorded in the source as a deferred item rather than as an omission.
  - **The FM depth control's physical form** — full knob vs scalloped trimpot — is Phase 35's, which
    owns the real layout and the whole control budget. This phase declares the param and gives it a
    stock widget. Noted in the banner.
  - **Patch persistence for the four new controls.** The module still serializes nothing, so none of
    these knob positions survives a patch save. This is Phase 35's `serialization` work and it now
    has four more reasons to happen; the banner names the consequence.

---

**Total deviations:** 3 auto-fixed (all Rule 2, comment-truth), 1 considered-and-left, 2 deliberate
non-actions (requirements not falsely marked; `deferred-items.md` left to its owning plan).
**Impact on plan:** No scope creep. Every prescribed edit landed exactly as specified, and the
non-comment footprint is exactly what the plan predicted — three `configParam` calls, one
`configInput`, five field assignments, four widget placements, four rectangles.

## Issues Encountered

- **None blocking.**
- **Two acceptance criteria are grep-pattern artifacts and cannot return the stated value.** Both are
  documented rather than worked around, and the substantive claim behind each is verified a different
  way:
  - `grep -c 'COARSE_PARAM,' src/AnalogVCO.cpp` returns **2**, not `1`. The criterion intends "the
    enum entry exists once", but `configParam(COARSE_PARAM, -5.f, ...)` contains the same
    `COARSE_PARAM,` substring, so the pattern cannot distinguish the enum entry from the declaration.
    The substantive claim is **true and verified** by anchoring:
    `grep -cE '^[[:space:]]*COARSE_PARAM,$'` returns **1**. Same for `FINE_PARAM`, `FM_ATTEN_PARAM`.
  - `grep -c 'FM_INPUT,' src/AnalogVCO.cpp` returns **2**, not `1`, for the identical reason
    (`configInput(FM_INPUT, "FM");`). Anchored: `grep -cE '^[[:space:]]*FM_INPUT,$'` returns **1**.
  - The sibling criterion `grep -c 'COARSE_PARAM'` returning `2` at the end of Task 1 was **exact**.
  - **This is the same criterion-filter class 31-02 and 31-03 each documented**, in a new variety: not
    comment lines inflating a code grep, but a **trailing comma shared between an enum entry and a
    function call**. Any future criterion asserting an enum-entry count should be anchored.
- **`make guards` is worth running immediately after each save**, per this phase's Pitfall 5 — the
  canary's `[2b/5]` section is the only thing that would have caught a field the shell forwards but
  the canary does not, and it now reports the corroborating `8` this plan's comment depends on.

## User Setup Required

None. No external service, no registry package, no install command — the only external dependency is
the locally pinned SDK at `../Rack-SDK`, and `plugin.dylib` links against it.

## Next Phase Readiness

- **Ready for 31-05/31-06/31-07 (the gates).** Every control the assertions describe now exists with
  the exact declared range, so `PITCH-02 COARSE` and `PITCH-03 FINE` have real params to talk about.
  Note that those tests drive `forge::VcoCore` through `tests/VcoBlockDriver.hpp` and **not** through
  this shell — the shell cannot be compiled without the SDK. So the tests assert against the POD's
  documented units (`coarse` in **octaves**, `fine` in **semitones**), and the declared param ranges
  above are what make those POD ranges the ones a user can actually reach.
- **Ready for 31-09 (the operator session).** All eight controls are on the panel with stock widgets
  at the final 18HP geometry, and `plugin.dylib` links. Two things the operator should be told
  up front so neither reads as a bug: the tooltips carry **five significant digits** (`+2.0000 oct`)
  rather than two decimals, by decision; and **no knob position survives a patch save**, because the
  module still serializes nothing.
- **31-08 inherits three deferred items** (COARSE detents, the FM depth control's physical form,
  patch persistence for the four new controls) plus the shipped LFO's shared latent UB that 31-03
  handed it.
- **Phase 35 inherits a bijective geometry check it can re-run unchanged.** The two-direction
  correspondence script is described above; when the real panel replaces the ten rects, that check is
  the cheap way to prove no control drifted from its art.
- **The comment-truth debt this plan created is zero, but it will accrue again.** The banner now
  claims eight controls and ten rectangles, and the delegation body's comment claims **seven of
  eight** DSP fields with `drift` as the gap. **Phase 34 falsifies both of the latter** when it wires
  `drift`: the count becomes eight of eight and the sentence naming a single header-default field
  becomes false. The rewritten canary paragraph was deliberately built to survive that, but the two
  numbers in it were not — they will need one more pass.
- **No blockers.**

## Self-Check: PASSED

- `src/AnalogVCO.cpp` — FOUND
- `res/AnalogVCO.svg` — FOUND
- `.planning/phases/31-pitch-tuning-exponential-fm/31-04-SUMMARY.md` — FOUND
- Commit `ac26bfe` — FOUND
- Commit `2ddd4ba` — FOUND
- Commit `52c2a2c` — FOUND
- Commit `4bb5c9c` — FOUND
- No file deletions in any of the four commits (`git diff --diff-filter=D` empty for each)
- No untracked residue after any commit (`git status --porcelain --untracked-files=all` clean)
- No frozen header, no `FROZEN.sha256`, no `src/dsp/` file, no `tests/` file and no `src/AnalogLFO.cpp` in the four-commit diff
- `make strict`, `make guards`, `make test` (72 / 2,616,112 / 0) and `RACK_DIR=../Rack-SDK make` all green at HEAD

---
*Phase: 31-pitch-tuning-exponential-fm*
*Completed: 2026-07-30*
