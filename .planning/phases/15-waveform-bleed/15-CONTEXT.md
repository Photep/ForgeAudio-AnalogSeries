# Phase 15: Waveform Bleed - Context

**Gathered:** 2026-03-17
**Status:** Ready for planning

<domain>
## Phase Boundary

Morph transitions show analog crossfader crosstalk influenced by the Character knob. When Character is above zero, the clean linear crossfade between adjacent waveform shapes gains subtle bleed from neighboring shapes. At Character=0, morph is a crisp crossfade with no bleed — identical to v1.1 behavior. Requirement: CHAR-05.

</domain>

<decisions>
## Implementation Decisions

### Bleed source & reach
- Nearest neighbors only — bleed comes from the two shapes adjacent to the current morph segment (not all four shapes)
- Tri→saw crossfade gets a hint of sine (left neighbor) and square (right neighbor)
- Proximity-weighted: the closer neighbor bleeds more (at morph=0.4 in the tri→saw segment, sine side bleeds more than square side because you're closer to the tri/sine boundary)
- Subtle magnitude: 3-5% of output from neighbor shapes at maximum Character, at the morph midpoint
- Consistent with Phase 14's conservative imperfection philosophy

### Morph endpoint behavior
- Claude's discretion on whether endpoints (morph=0, morph=1) and segment boundaries (0.333, 0.667) are pure or have bleed
- Must satisfy success criterion #2: at Character=0, morph is crisp crossfade identical to v1.1

### Bleed texture
- Slightly modulated over time — bleed amount fluctuates slowly, like real capacitive crosstalk that changes with temperature
- Claude's discretion on modulation source (existing OU drift layers, component spread, or dedicated modulator — pick what's most elegant given existing architecture)
- Component spread (Phase 14) DOES affect bleed — per-instance offset to bleed magnitude, consistent with "every instance is slightly different" philosophy
- Uses existing component spread infrastructure (serialized RNG seed)

### Amplitude safety
- Claude's discretion on whether to add explicit +/-5V clamp — analyze the math given that bleed mixes small amounts of waveforms already normalized to +/-1
- Must satisfy success criterion #3: output stays within +/-5V at all morph/character/bleed combinations

### Claude's Discretion
- Edge wrapping at morph strip endpoints (sine↔square as neighbors or one-sided bleed)
- Whether segment boundaries are pure points or have bleed
- Whether morph endpoints have bleed when Character is up
- Modulation source for bleed fluctuation (existing OU layers vs. new source)
- Exact bleed magnitude within 3-5% range
- Whether explicit output clamping is needed
- Integration point within computeMorphedWave() vs. separate post-processing

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements
- `.planning/REQUIREMENTS.md` — CHAR-05 defines waveform bleed requirement

### Architecture context
- `.planning/PROJECT.md` — Key decisions table: drift engine design, OU layers, per-module RNG, component spread
- `.planning/phases/14-expanded-imperfections/14-CONTEXT.md` — Phase 14 imperfection patterns: progressiveCurve(), authority scaling, component spread infrastructure, OU layer design

### Prior phase decisions
- `.planning/STATE.md` — Phase 14 decisions on component spread magnitudes and OU parameters

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `computeMorphedWave()` (AnalogLFO.cpp:264): Current morph pipeline — computes all four shapes then does linear crossfade by segment. This is the primary integration point for bleed.
- `computeSine/Triangle/Saw/Square()` (AnalogLFO.cpp:162-262): Individual shape generators already called within computeMorphedWave — bleed needs access to neighbor shapes that are already computed.
- `progressiveCurve()` (AnalogLFO.cpp:146): x^2 character scaling — reuse for bleed intensity gating.
- `ouLayers[4]` (AnalogLFO.cpp:81): Existing 4-layer OU drift engine — potential modulation source for bleed fluctuation.
- Component spread variables: `triAsymmetrySpread`, `sawCurvatureSpread`, `squareDutySpread` (AnalogLFO.cpp) — pattern for adding `bleedSpread` offset.
- `rng` + `normalDist` (AnalogLFO.cpp:77-78): Per-module RNG for component spread seed generation.

### Established Patterns
- Character scaling: `progressiveCurve(character)` gates all analog effects — bleed follows same pattern
- At character < 0.001f, early-return with digital precision (existing pattern in all compute functions)
- Component spread offsets stored as member variables, initialized from serialized RNG seed
- Drift authority scaling: `isClocked ? 0.02f : 0.075f` — may apply to bleed modulation if using OU layers

### Integration Points
- `computeMorphedWave()` already computes all four waveforms — bleed modifies the crossfade mix, not the individual shapes
- Display buffer renders via `updateDisplayBuffer()` which calls `computeMorphedWave()` — bleed will automatically appear on display
- Output path: `computeMorphedWave() → 5V scaling → crossfade → DC offset` — bleed is pre-scaling so +/-1 normalization holds

</code_context>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches within the constraints above.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 15-waveform-bleed*
*Context gathered: 2026-03-17*
