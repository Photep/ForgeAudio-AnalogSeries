# Phase 6: Polish & Cleanup - Context

**Gathered:** 2026-03-07
**Status:** Ready for planning

<domain>
## Phase Boundary

Close tech debt from milestone audit: fix stale OUT-02 documentation, tune drift dot instability visuals to be perceptible, make drift visuals respond to CV-modulated value, and restructure the bottom row panel layout to follow standard Eurorack design language. No new features -- pure polish on existing functionality.

</domain>

<decisions>
## Implementation Decisions

### Drift dot instability intensity
- Unmistakable visual feedback at high drift -- 4-5x current intensity
- Trail jitter AND halo pulsing both amplified equally (two visual channels reinforcing each other)
- The dot center stays on the waveform trace (no center wobble), but trail and halo make the instability clearly visible
- At full drift the visual should "sell the alive feeling" -- not subtle, not distracting, but clearly present

### Drift instability CV response
- Drift visual must read the combined drift value (knob + attenuator * CV / 10V, clamped 0-1), not just `params[DRIFT_PARAM].getValue()`
- This matches how drift audio actually works -- visual feedback tracks real drift amount

### Bottom row layout restructure
- Adopt standard Eurorack two-row convention (reference: VCV Rack Fundamental LFO):
  - **Upper row**: 3 attenuator trimpots (Morph, Character, Drift) vertically above their jacks
  - **Lower row**: 3 CV input jacks + 1 output jack, each with label above
  - **Connection lines**: thin vertical lines from each trimpot down to its associated jack
- Rate knob stays at its current position (y=86mm, own row) -- clean separation between main controls above and CV/IO section below
- Output jack sits in the bottom jack row alongside the 3 CV inputs, but visually distinct (different label style or accent color to distinguish input vs output)

### OUT-02 documentation fix
- Verify REQUIREMENTS.md OUT-02 description accurately reflects "inverted output removed by design decision"
- Fix any stale references in ROADMAP.md success criteria that still mention inverted output

### Claude's Discretion
- Connection line style (solid vs amber accent, weight) -- pick what looks best in the Forge Audio brand palette
- Drift visual animation source (OU-driven vs independent breathePhase) -- pick whichever is cleanest and looks best
- Drift visual intensity curve mapping (x^2 vs linear vs other) -- pick the mapping that feels best
- Exact vertical spacing for the two-row bottom section (trimpots ~96mm, jacks ~108mm or similar)
- Output jack visual distinction treatment (amber ring, different label color, etc.)

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `drawPhaseDot()` (AnalogLFO.cpp:374): Current drift instability implementation -- reads `params[DRIFT_PARAM].getValue()` at line 378, needs to read combined CV-modulated drift value instead
- `progressiveCurve()` (line 60): x^2 mapping function used for audio drift -- available for visual intensity mapping
- Current trail jitter: `driftLevel * 0.3f` (line 398) and halo variation: `driftLevel * 0.15f` (line 415) -- both need 4-5x amplification
- `breathePhase` (line 292): Existing animation timer at ~0.8Hz -- drives current jitter/halo sine waves

### Established Patterns
- Lock-free atomic for audio-to-display data transfer (displayPhase, displayReadIdx)
- Four-pass glow rendering for waveform trace (widths 6/4/2.5/1.5, alphas 0.04/0.08/0.15/0.85)
- CV processing: `clamp(knob + atten * voltage / 10.f, 0.f, 1.f)` -- same pattern needed for drift visual

### Integration Points
- Widget constructor (line 481): Bottom row component positions need restructuring from single row to two rows
- Panel SVG (res/AnalogLFO.svg): Needs connection lines added, labels repositioned, bottom section restructured
- `drawPhaseDot()`: Needs drift level source changed and intensity multipliers increased

</code_context>

<specifics>
## Specific Ideas

- Reference image: VCV Rack Fundamental LFO lower section -- trimpots above with thin lines down to their labeled jacks below
- Current single-row layout at y=104mm [MATrim(7) MCV(14) CATrim(21) CCV(28) DATrim(35) DCV(42) OUT(54)] must split into two vertical rows
- Drift visual is the "alive" knob's visual identity -- the instability effect should be the most visually dynamic element on the module after the waveform trace itself

</specifics>

<deferred>
## Deferred Ideas

None -- discussion stayed within phase scope

</deferred>

---

*Phase: 06-polish-and-cleanup*
*Context gathered: 2026-03-07*
