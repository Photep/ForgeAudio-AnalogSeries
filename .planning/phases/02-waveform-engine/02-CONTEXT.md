# Phase 2: Waveform Engine - Context

**Gathered:** 2026-02-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Four-shape morph oscillator with rate control, morph CV input, and single bipolar output. Users can generate LFO signals and continuously sweep through Sine, Triangle, Saw, and Square shapes with the morph knob. This phase delivers the core DSP engine that all later phases build on.

</domain>

<decisions>
## Implementation Decisions

### Morph sweep character
- Equal quarter distribution: Sine 0-25%, Triangle 25-50%, Saw 50-75%, Square 75-100%
- Shape order: Sine -> Triangle -> Saw -> Square (smooth to sharp, low to high harmonics)
- No snap/detent at pure shape points -- fully smooth continuous sweep
- Intermediate shapes between any two neighbors are Claude's discretion (pick the most musically useful interpolation method)

### Rate knob feel
- Linear mapping across 0.01Hz to 20Hz range (equal Hz per degree of rotation)
- Default rate: ~0.7Hz (matching the previous Forge Audio LFO project)
- Current codebase has exponential mapping -- this needs to change to linear
- Hz value shown via VCV Rack's built-in tooltip on hover (no panel display needed)
- No special behavior at extremes -- 0.01Hz (100-second cycle) is just a valid slow rate

### CV input behavior
- Morph CV is additive (offset): knob sets center position, CV sweeps around it
- CV-to-morph scaling: Claude's discretion (pick whatever is most standard in VCV Rack ecosystem)
- Hard clamp when combined knob + CV exceeds range (stays at sine or square, no wrap-around)
- Add a small attenuator knob next to the morph CV jack for controlling modulation depth (0-100%)
- Note: this adds a new param and panel element not currently in the scaffold

### Output signal shape
- Single output jack only -- inverted output removed (simplifies display in Phase 3)
- Bipolar +/-5V output, all shapes centered at 0V (no DC offset for any shape)
- Saw waveform: rising ramp (ramps up from -5V to +5V, snaps down)
- Square waveform: 50% duty cycle for Phase 2 (Character knob in Phase 5 will handle duty cycle variation)
- Current codebase declares 2 outputs -- reduce to 1

### Claude's Discretion
- Morph interpolation method between shapes (linear crossfade vs shape-aware -- pick whatever produces the best intermediate shapes)
- CV-to-morph voltage scaling (10V full range vs 5V vs other)
- Phase accumulator implementation details (double precision already specified in success criteria)
- Exact attenuator knob sizing and panel placement near CV jack

</decisions>

<specifics>
## Specific Ideas

- Default rate should match the previous Forge Audio LFO project (~0.7Hz, from param default -1.5 with base 2, multiplier 2)
- The single output simplifies Phase 3's display -- one waveform to visualize, no ambiguity about which output the display tracks
- The CV attenuator knob is a new panel element that wasn't in the Phase 1 scaffold -- panel SVG will need updating

</specifics>

<deferred>
## Deferred Ideas

None -- discussion stayed within phase scope

</deferred>

---

*Phase: 02-waveform-engine*
*Context gathered: 2026-02-25*
