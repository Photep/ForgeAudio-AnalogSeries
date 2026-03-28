# Phase 18: PWM DSP Extension - Context

**Gathered:** 2026-03-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Extend the morph sweep past square into a fifth waveform shape -- variable-width pulse -- with full analog character modeling. The morph knob sweeps Sine -> Triangle -> Saw -> Square -> Pulse with even spacing across 5 shapes. This is a DSP-only phase; no panel or display layout changes.

</domain>

<decisions>
## Implementation Decisions

### Morph Mapping
- **D-01:** Even 5-shape rescale -- `morph * 4.0` gives 4 segments across [0, 1.0], each shape getting 20% of the knob range
- **D-02:** WAVE-05 (backward compatibility) is DROPPED -- existing patches will shift morph positions. Breaking change accepted.
- **D-03:** Morph order: Sine -> Triangle -> Saw -> Square -> Pulse (natural harmonic progression, smooth to sharp to narrow)

### Bleed Ring Topology
- **D-04:** Full 5-shape wrapping ring: sine-tri-saw-sqr-pulse-sine (`% 5` modular arithmetic). Pulse neighbors sine, consistent with existing ring pattern.

### Pulse Character
- **D-05:** Tanh edge softening, same approach as `computeSquare` -- at character=0 crisp digital pulse, at character=1 rounded analog edges
- **D-06:** Component spread affects edge softening intensity only, NOT duty cycle. Duty cycle stays where morph puts it.

### Duty Cycle Range
- **D-07:** Duty ranges from 50% (at square->pulse boundary) to 5% (at morph=1.0) -- classic PWM territory
- **D-08:** Linear duty mapping: `duty = 0.50 - 0.45 * frac` where frac is the position within the pulse segment

### Claude's Discretion
- Pulse generation implementation details (threshold comparison, phase-based construction)
- How to structure `computePulse()` function relative to existing per-shape compute functions
- Any anti-aliasing or edge-case handling for extreme narrow pulse widths at LFO rates

### Folded Todos
- **Pulse width modulation** (from 2026-03-17) -- directly addressed by this phase's scope
- **Separate display pills from waveform visualiser** (from 2026-03-17) -- folded into v1.3 scope; primarily addressed by Phase 20 three-column layout but tracked here as user request

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Waveform Engine
- `.planning/REQUIREMENTS.md` -- WAVE-01 through WAVE-04 define the pulse requirements (WAVE-05 dropped per D-02)
- `.planning/PROJECT.md` -- Key Decisions table, especially "PWM as morph extension" and "Waveform bleed via wrapping ring topology"

### Existing Implementation
- `src/AnalogLFO.cpp` lines 299-343 -- `computeMorphedWave()` with 4-shape array, segment crossfade, and bleed ring (this is the primary code to modify)
- `src/AnalogLFO.cpp` lines 270-297 -- `computeSquare()` with tanh character modeling (reference for pulse character implementation)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `computeSquare()` tanh edge softening pattern: directly reusable for `computePulse()` with variable duty center
- `progressiveCurve()` character scaling: already used by bleed, available for pulse character
- Component spread infrastructure (`bleedSpread`, per-instance seed): ready for pulse edge variation

### Established Patterns
- Per-shape compute functions: `computeSine()`, `computeTriangle()`, `computeSaw()`, `computeSquare()` -- pulse follows same pattern
- `shapes[]` array indexed by segment for crossfade and bleed neighbor access
- Bleed uses `% N` wrapping with proximity-weighted neighbor mixing

### Integration Points
- `computeMorphedWave()`: shapes array expands from `[4]` to `[5]`, scaling from `* 3.f` to `* 4.f`
- Bleed ring: `% 4` becomes `% 5` throughout neighbor identification
- `updateDisplayBuffer()`: calls `computeMorphedWave()` -- will automatically render pulse shape
- All morph CV processing unchanged -- just wider range maps to more shapes

</code_context>

<specifics>
## Specific Ideas

No specific requirements -- open to standard approaches within the decisions above.

</specifics>

<deferred>
## Deferred Ideas

None -- discussion stayed within phase scope.

</deferred>

---

*Phase: 18-pwm-dsp-extension*
*Context gathered: 2026-03-28*
