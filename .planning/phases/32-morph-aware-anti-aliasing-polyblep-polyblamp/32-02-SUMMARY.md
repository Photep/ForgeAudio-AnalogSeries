---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
plan: 02
subsystem: ui
tags: [vcv-rack, rack-sdk, param-plumbing, attenuverter, svg-panel, nan-safety, cpp11]

# Dependency graph
requires:
  - phase: 29-vcocore-extraction
    provides: "forge::VcoInputs::morph, declared post-CV/post-clamp [0,1] — the seam this plan makes true"
  - phase: 30-vcocore-skeleton-module-registration
    provides: "The AnalogVCO Rack shell, its throwaway panel asset, and D-07 (every visible control does something)"
  - phase: 31-pitch-tuning-exponential-fm
    provides: "FM_ATTEN_PARAM — the attenuverter styling precedent D-17 inherits wholesale; the negated-comparison NaN idiom; the compile canary's one-field margin"
provides:
  - "AnalogVCO::MORPH_ATTEN_PARAM — bipolar -1..+1 attenuverter, linear taper, default 0, displayed -100%..+100%"
  - "AnalogVCO::MORPH_CV_INPUT — the MORPH CV jack, reachable in Rack"
  - "A shell-side mix feeding forge::VcoInputs::morph: knob + CV * 0.1f * attenuverter, conditioned NaN-safely into [0,1]"
  - "Two paired marker rects in res/AnalogVCO.svg at (25.48, 75) and (55.96, 75)"
  - "The cable an operator plugs in to drive audio-rate MORPH — the hardest case the Phase 32 alias floor has to survive"
affects: [32-06 core-side morph guard, 32-09 audio-rate MORPH assertion, 32-10 operator UAT, 34-character-drift-cv, 35-panel-display]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Shell-side param conditioning as the one authorised divergence from the no-DSP rule (D-17)"
    - "Negated-comparison pair at the Rack cable boundary, negated line first, forge::clamp rejected by name"
    - "Paired widget-coordinate / SVG-marker-rect edits in a single commit"

key-files:
  created: []
  modified:
    - src/AnalogVCO.cpp
    - res/AnalogVCO.svg

key-decisions:
  - "D-16 honoured: MORPH's CV jack and attenuverter are declared in Phase 32, correcting Phase 31's CONTEXT lumping rather than following it. CHARACTER's CV and attenuverter remain Phase 34 (CHAR-01) and were deliberately not added alongside them, however symmetric the panel would have looked."
  - "D-17 honoured: forge::VcoInputs gains ZERO fields. morphCV/morphAtten were rejected — the POD's morph field was designed post-CV/post-clamp in Phase 29, Phase 31's D-05 forbids churning field semantics mid-milestone, and two new fields would overdraw the compile canary's exactly-one-field margin (Phase 31 deferred item 9)."
  - "The conditioning uses the negated-comparison pair, NOT forge::clamp. forge::clamp is a comparison ladder and both of its comparisons are false for a NaN, so it is inert against T-32-01 — the float-to-int cast inside the frozen forge::Waveshape::morphedWave that this new jack is the first route to."
  - "The attenuverter is bipolar rather than unipolar because a negative setting inverts the CV, letting one modulator sweep the crossfade backwards from pulse to sine without repatching."
  - "The new widget row reuses the y = 40 row's x positions (30.48 / 60.96), so it is symmetric about the 45.72 mm centre line and each control sits under the knob it relates to."

patterns-established:
  - "Authorised-divergence comment shape: when a file's banner forbids a class of code, the exception is justified in the source in independent points, each naming the decision that authorises it."
  - "Re-read findings are recorded, not skipped: the field-accounting block states that Phase 32 re-read the growth rule and found nothing to do, so a later reader does not re-derive it from the diff."

requirements-completed: [MORPH-02]

coverage:
  - id: D1
    description: "AnalogVCO::MORPH_ATTEN_PARAM and AnalogVCO::MORPH_CV_INPUT exist, are configured with Phase 31's D-07 attenuverter styling, and compile under the C++11 pedantic gate"
    requirement: "MORPH-02"
    verification:
      - kind: unit
        ref: "make strict — 'strict C++11 gate: PASS'"
        status: pass
      - kind: unit
        ref: "make -j4 — plugin.dylib built and linked against ../Rack-SDK"
        status: pass
    human_judgment: false
  - id: D2
    description: "in.morph is knob + CV x attenuverter conditioned NaN-safely into [0,1] with no POD field added; forge::VcoInputs is byte-unchanged and the canary still reports eight fields runtime-live"
    requirement: "MORPH-02"
    verification:
      - kind: unit
        ref: "make guards — check_frozen + check_includes + check_canary [2b/5]: 'guard suite: PASS'"
        status: pass
      - kind: unit
        ref: "make test — 83/83 doctest cases, unchanged from this plan's baseline"
        status: pass
      - kind: unit
        ref: "git status --porcelain src/dsp/VcoCore.hpp — empty"
        status: pass
    human_judgment: false
  - id: D3
    description: "The two new controls are visible and reachable in a running Rack session, and the panel's marker rects sit under the controls they mark"
    verification: []
    human_judgment: true
    rationale: "Widget placement and panel-marker alignment are visual properties of a running Rack instance. The headless gates prove the counts agree (12 rects vs 10 coordinates) and that the widget links, but only an operator can confirm the controls appear where the panel says they do. Plan 32-10's in-Rack UAT session is where this is signed off."

# Metrics
duration: 21min
completed: 2026-07-31
status: complete
---

# Phase 32 Plan 02: MORPH CV Jack and Attenuverter Summary

**The MORPH CV jack and its bipolar attenuverter are declared, wired, and reachable in Rack — the shell mixes knob + CV x attenuverter into `forge::VcoInputs::morph` behind a NaN-safe negated-comparison pair, adding zero POD fields.**

## Performance

- **Duration:** 21 min
- **Started:** 2026-07-31T14:18:00Z
- **Completed:** 2026-07-31T14:39:00Z
- **Tasks:** 3
- **Files modified:** 2

## Accomplishments

- `AnalogVCO::MORPH_ATTEN_PARAM` and `AnalogVCO::MORPH_CV_INPUT` are declared and configured with Phase 31's D-07 attenuverter styling inherited wholesale — bipolar `-1..+1`, linear taper, default `0`, displayed `-100%..+100%`.
- `in.morph` is now knob + CV x attenuverter, conditioned into the POD's documented `[0,1]` range by the negated-comparison pair with the negated line first, so a hostile cable voltage cannot reach the frozen `float`-to-`int` cast inside `forge::Waveshape::morphedWave` (T-32-01 mitigated).
- `forge::VcoInputs` is byte-unchanged. The compile canary's unique-field margin is still exactly one field, and `check_canary.sh [2b/5]` still reports eight fields runtime-live.
- The panel and the widget agree: twelve `<rect>` elements against ten `mm2px` coordinates, both edits landed in one commit per the paired-comment rule.
- Three stale comments were corrected in place rather than left to rot: the banner's control count, its panel rect count, and its "Phase 32 owns band-limiting" forward reference.

## Task Commits

Each task was committed atomically:

1. **Task 1: Declare MORPH_ATTEN_PARAM and MORPH_CV_INPUT, and update the banner counts** — `81fbb84` (feat)
2. **Task 2: Mix knob + CV x attenuverter into VcoInputs::morph with a NaN-safe conditioning pair** — `1164801` (feat)
3. **Task 3: Widget coordinates and the paired panel marker rects, in one commit** — `1df4c62` (feat)

## Files Created/Modified

- `src/AnalogVCO.cpp` — two enum entries, one `configParam`, one `configInput`, the shell-side mix and its NaN-safe conditioning pair, two widget coordinates, and four comment corrections.
- `res/AnalogVCO.svg` — two additional `10x10` `fill="#2a2a30"` marker rects at `(25.48, 75)` and `(55.96, 75)`.

## Plan Output Record

The plan's `<output>` block asks for three specific facts. Recorded here explicitly:

**The two enum entries and their positions.** `MORPH_ATTEN_PARAM` was APPENDED to `AnalogVCO::ParamId` after `FM_ATTEN_PARAM` and before `PARAMS_LEN`, giving it index 5. `MORPH_CV_INPUT` was APPENDED to `AnalogVCO::InputId` after `FM_INPUT` and before `INPUTS_LEN`, giving it index 2. Appended rather than grouped by subsystem, which is the precedent `FM_ATTEN_PARAM` set in Phase 31 and which the banner's `:22-28` paragraph licenses — nothing has shipped, so param and input ID churn is free.

**Rect and `mm2px` counts, before and after.**

| Count | Before | After |
|---|---|---|
| `grep -c '<rect' res/AnalogVCO.svg` | 10 | 12 |
| marker rects (excluding panel background and logo bar) | 8 | 10 |
| `grep -cE '^[[:space:]]*add(Param\|Input\|Output)\(create' src/AnalogVCO.cpp` | 8 | 10 |

The marker-rect count and the widget-coordinate count agree at 10, which is the invariant the paired-comment rule at `src/AnalogVCO.cpp:257-262` exists to protect.

**`forge::VcoInputs` and the canary's field-count margin are UNCHANGED.** `git status --porcelain src/dsp/VcoCore.hpp` was empty after every task, and `grep -c 'morphCV\|morphAtten' src/dsp/VcoCore.hpp` outputs `0`. This shell still feeds runtime-derived values into seven of the eight `VcoInputs` DSP fields while `src/vco_compile_canary.cpp` feeds all eight; `drift` is still the single field the canary feeds and this shell does not; the margin is still exactly one field; and `tests/check_canary.sh [2b/5]` still reports eight fields runtime-live at `-O3`. Phase 34 is still the phase that closes the gap.

## Decisions Made

- **`forge::clamp` was rejected here even though `32-RESEARCH.md:688` sketches the mix with `rack::math::clamp`.** The plan's Task 2 mandates the negated-comparison pair and the threat register's T-32-01 explains why, so the plan text was followed over the research snippet. A comparison ladder has both comparisons false for a NaN, so it would have been inert against precisely the input class the guard exists to stop. The rejection is recorded by name in the source so the research snippet cannot be copied back in later.
- **The MORPH CV jack was placed at `x = 60.96` and the depth knob at `x = 30.48`, matching the plan exactly.** This puts the depth knob directly beneath the MORPH knob it attenuates and the jack beneath CHARACTER. The alternative — jack under MORPH, knob under CHARACTER — would read better as "jack next to the knob it feeds" but breaks the vertical association between the depth control and the parameter it scales. Phase 35 owns the real layout and can revisit it.
- **The banner's panel-rect count was updated to twelve in Task 1, one commit before the SVG actually gained the rects.** The plan directs this explicitly in Task 1's action. The inconsistency is transient and internal to this plan; both files are consistent as of `1df4c62`.

## Deviations from Plan

None - plan executed exactly as written.

No deviation rule fired. Every acceptance criterion in all three tasks passed on first run, no auto-fix was required, and no file outside `src/AnalogVCO.cpp` and `res/AnalogVCO.svg` was touched by any task commit.

## Issues Encountered

None.

Two things worth flagging as verified-not-problems:

- **The RESEARCH snippet and the PLAN disagreed on the clamp form.** `32-RESEARCH.md:688` shows `rack::math::clamp(morph, 0.f, 1.f)`; the plan's Task 2 and the threat register both mandate the negated pair. The plan and threat register win — this is documented under Decisions Made rather than as a deviation, because the plan was followed as written and the research document is upstream advisory material, not the contract.
- **`.planning/research/.cache/` is untracked in the working tree.** It predates this plan (it appears in the session's opening `git status`) and is out of scope per the executor's scope boundary. Not touched, not committed, not gitignored by this plan.

## Threat Mitigations Applied

| Threat ID | Disposition | Where it landed |
|---|---|---|
| T-32-01 | mitigated | `src/AnalogVCO.cpp` `process()` — negated-comparison pair, negated line first, so a NaN lands on the `0.f` fallback and never reaches `morphedWave`'s `(int)(morph * 4.f)`. `forge::clamp` rejected by name in the source comment. Defence in depth arrives with plan 32-06's core-side guard. |
| T-32-03 | mitigated | The mix is bounded to `[0,1]` before it reaches any DSP, so no CV magnitude can widen the output envelope. |
| T-32-12 | mitigated | `src/AnalogLFO.cpp` is absent from all three task commits; no frozen header touched; `src/dsp/FROZEN.sha256` unbumped; `make test` replayed the goldens green at 83/83. |
| T-32-13 | accepted | No serialization added; the VCO still persists nothing. The two new params take Rack's default persistence. |
| T-32-SC | accepted | Zero packages installed in any ecosystem. |

No new threat surface was found beyond the register — the only new input path is the one `MORPH_CV_INPUT` jack the register already covers.

## Known Stubs

None. Both controls are fully wired: the param and the input each reach `forge::VcoInputs::morph` through the mix, and neither is a placeholder awaiting a later phase.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Plan 32-06 must still add the core-side `morph` guard.** This plan's guard defends the POD arriving from the Rack shell; it does NOT defend `forge::VcoCore::step` against the headless harness, which builds `forge::VcoInputs` directly with no shell in the way. The two guards are not redundant and the source says so.
- **Plan 32-09's audio-rate MORPH assertion now has controls to drive.** MORPH-02's headless assertion lands there; nothing in this plan asserts it.
- **Plan 32-10's operator UAT now has a cable to plug in.** The in-Rack sweep this phase signs off on is reachable.
- **Phase 34 (CHAR-01) still owns CHARACTER's CV and attenuverter,** and closing that gap also closes the compile canary's one-field margin — the growth rule in `src/AnalogVCO.cpp`'s field-accounting block binds that phase.
- **Phase 35 replaces `res/AnalogVCO.svg` wholesale.** The two rects added here are throwaway by design and at the final 18 HP geometry, so the swap stays an art change rather than a rewiring.

## Self-Check: PASSED

- `src/AnalogVCO.cpp` — FOUND
- `res/AnalogVCO.svg` — FOUND
- `plugin.dylib` — FOUND (built and linked)
- Commit `81fbb84` — FOUND
- Commit `1164801` — FOUND
- Commit `1df4c62` — FOUND

---
*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Completed: 2026-07-31*
