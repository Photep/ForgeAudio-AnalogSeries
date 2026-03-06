# Phase 4: Analog Character - Context

**Gathered:** 2026-03-06
**Status:** Ready for planning

<domain>
## Phase Boundary

Character knob crossfading from digital perfection to classic synth references per waveform shape. Each shape targets a specific analog reference: saw = Minimoog Model D, square = Roland SH-101/Juno-106, triangle = Moog/Prophet, sine = triangle-derived analog sine. Character CV input with attenuator for external modulation. HF rolloff (CHAR-06) deferred to VCO -- not meaningful at sub-audio LFO rates.

</domain>

<decisions>
## Implementation Decisions

### Character knob curve
- Progressive mapping: subtle analog coloring in the first half, stronger deformations in the second half
- Full character (100%) = authentic realism matching the reference synth, not exaggeration or caricature
- The useful range should reward exploration -- most of the knob travel produces musically interesting results

### Character in morph transitions
- When morph is between two shapes, how character applies (characterize-then-morph vs morph-then-characterize) is Claude's discretion -- pick what sounds and looks best through the morph range
- Whether character introduces morph bleed in transition zones is Claude's discretion -- defer to v2 (IO-03) if it adds significant complexity

### Character CV input
- Additive offset behavior: knob sets center position, CV sweeps around it (same as Morph CV)
- Hard clamp at 0-1 range when combined knob + CV exceeds bounds
- Own attenuator trimpot matching the Morph CV pattern (new param: CHARACTER_ATTEN_PARAM)
- Jack placement on panel is Claude's discretion based on available space and panel conventions

### HF rolloff
- CHAR-06 deferred to VCO module -- sub-audio LFO rates (0.01-20Hz) have no high-frequency harmonic content to roll off
- Move CHAR-06 to v2 VCO requirements

### Claude's Discretion
- Morph-character ordering (characterize-then-morph vs morph-then-characterize)
- Whether to include morph bleed scaled with character (or defer to v2 IO-03)
- Character CV jack and attenuator placement on panel
- Exact progressive curve shape (exponential, power curve, etc.)
- DSP implementation approach for each analog reference model

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `computeMorphedWave()` (AnalogLFO.cpp:40): Pure digital waveform generation -- character modeling hooks into or wraps this function
- `CHARACTER_PARAM` already declared in enum (line 9) and configured (line 72) with 0-1 range -- ready to wire up
- `updateDisplayBuffer()` (line 59): Calls computeMorphedWave -- needs to pass character param so display reflects character changes
- TODO comment at line 101: "Phase 5/6: add characterChanged and driftChanged triggers here" -- display update trigger point identified

### Established Patterns
- Lock-free double buffer for audio-to-display transfer (atomic read index + relaxed phase store)
- CV processing pattern: knob + attenuator * voltage / 10V, hard clamp (line 95) -- replicate for Character CV
- Display update triggers on phase wrap or param change with threshold (line 102-107)

### Integration Points
- `process()` function (line 81): Character processing goes between morph calculation (line 95) and waveform generation (line 112)
- `InputId` enum (line 15): Needs CHARACTER_CV_INPUT added (currently only MORPH_CV and DRIFT_CV)
- `ParamId` enum (line 7): Needs CHARACTER_ATTEN_PARAM added
- Panel SVG: Needs Character CV jack + attenuator trimpot added
- Widget constructor (line 301): Needs new input and param widget positions

</code_context>

<specifics>
## Specific Ideas

- Character knob at zero must produce bit-identical output to current digital waveforms -- no processing overhead when character is off
- The display will automatically show character deformations since it's WYSIWYG (decided in Phase 3)
- Saw is already a falling ramp matching Minimoog convention (decided in Phase 2) -- character modeling builds on this
- Phase 2 noted "Character knob will handle duty cycle variation" for square -- CHAR-03's duty cycle asymmetry (0.5-1.5%) fulfills this

</specifics>

<deferred>
## Deferred Ideas

- CHAR-06 (HF rolloff via pitch-tracking lowpass) -- deferred to VCO module where audio-rate harmonics exist
- Waveform bleed in morph transition zones (IO-03) -- may be included if Claude determines it enhances character without significant complexity, otherwise v2

</deferred>

---

*Phase: 04-analog-character*
*Context gathered: 2026-03-06*
