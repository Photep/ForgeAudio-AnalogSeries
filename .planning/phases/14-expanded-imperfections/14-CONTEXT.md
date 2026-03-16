# Phase 14: Expanded Imperfections - Context

**Gathered:** 2026-03-16
**Status:** Ready for planning

<domain>
## Phase Boundary

The Drift knob controls a richer set of analog imperfections beyond pitch drift alone: phase jitter, DC offset wander, pitch slew (thermal response), and per-instance component spread. Requirements: CHAR-01, CHAR-02, CHAR-03, CHAR-04. At Drift=0, all new imperfections except component spread are completely inactive -- output matches pre-v1.2 digital precision.

</domain>

<decisions>
## Implementation Decisions

### Phase jitter (CHAR-01)
- Unstable oscillator character -- per-sample random phase deviation, not correlated wow/flutter
- New independent white noise source from existing per-module RNG (not tapping OU layers)
- Jitter is fast and uncorrelated; drift remains slow and correlated -- both coexist authentically
- Display shows subtle trace thickening at full drift, not obvious wobble
- Authority reduced in clocked mode (same scaling philosophy as pitch drift: 2% vs 7.5%)
- Completely inactive at Drift=0

### DC offset wander (CHAR-02)
- Subtle realism: ~50-100mV maximum wander at full drift
- Very slow timescale (~0.02-0.05Hz) -- wanders over 20-50 seconds, like component thermal drift
- New slow OU layer dedicated to DC offset (separate from pitch drift OU layers)
- Continuous wander -- does NOT reset on clock phase resets (component-level phenomenon)
- No display change -- offset is below visual threshold; observable on scope module only
- Completely inactive at Drift=0

### Pitch slew (CHAR-03)
- Slow thermal settling: 200-500ms time constant at full drift
- Simulates capacitor/transistor thermal lag on frequency changes
- Applies to ALL frequency changes: Rate knob, CV, and clock tempo changes
- Separate drift-controlled slew filter layered after existing freqSlew
- Existing freqSlew handles mode transitions (always on); drift slew adds thermal character on top
- Completely bypassed at Drift=0 -- frequency response identical to v1.1

### Component spread (CHAR-04)
- Subtly different: ~1-3% parameter offsets, like factory-run component tolerance
- Parameters affected: OU layer weights, character curve response, waveform shape coefficients
- Serialized RNG seed -- spread persists across patch saves/loads (permanent module personality)
- Always active regardless of Drift setting -- component spread is a physical property, not switchable
- Two instances at Drift=0 will have slightly different waveform shapes (factory tolerance)

### Claude's Discretion
- Exact jitter magnitude scaling curve within the "subtle trace thickening" constraint
- DC offset OU layer parameters (exact theta/sigma for 0.02-0.05Hz timescale)
- Exact pitch slew time constant within the 200-500ms range
- Which specific waveform shape coefficients get component spread offsets
- Component spread RNG seed generation and serialization format
- Empirical tuning of all magnitude values (flagged in STATE.md as needing tuning)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements
- `.planning/REQUIREMENTS.md` -- CHAR-01 through CHAR-04 define the four imperfection types

### Architecture context
- `.planning/PROJECT.md` -- Key decisions table: drift engine design, OU layers, per-module RNG, no OU serialization (note: component spread seed IS serialized per CHAR-04)
- `.planning/phases/11-display-polish/11-CONTEXT.md` -- Display pill backgrounds and overlay patterns

### Prior phase decisions
- `.planning/STATE.md` -- "Component spread magnitudes need empirical tuning during Phase 14" blocker

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ouLayers[4]` (AnalogLFO.cpp:81): 4-layer OU drift engine -- phase jitter should NOT reuse these; DC offset wander adds a 5th independent OU layer
- `rng` (AnalogLFO.cpp:77): Per-module Xoroshiro128Plus -- use for both jitter noise and component spread seed generation
- `normalDist` (AnalogLFO.cpp:78): Normal distribution already configured -- reuse for jitter sampling
- `progressiveCurve()` (AnalogLFO.cpp:146): x^2 curve -- reuse for all new imperfection scaling
- `freqSlew` (AnalogLFO.cpp:115): Existing exponential filter for mode transitions -- pitch slew is a SEPARATE filter layered after this
- `displayDrift` atomic (AnalogLFO.cpp:88): Bridges drift level to display -- jitter display effects can read this

### Established Patterns
- Drift authority scaling: `isClocked ? 0.02f : 0.075f` (AnalogLFO.cpp:530) -- new imperfections follow same pattern
- Progressive x^2 curve gates all drift-scaled effects (AnalogLFO.cpp:521)
- Relaxed atomics for audio-to-display data transfer
- `computeMorphedWave()` is the waveform generation pipeline -- component spread modifies coefficients used by individual compute functions

### Integration Points
- Phase jitter: modifies `deltaPhase` in process(), alongside existing drift calculation (AnalogLFO.cpp:508-533)
- DC offset: added to `outputVoltage` after waveform computation, before crossfade (AnalogLFO.cpp:584)
- Pitch slew: new filter between `freqSlew.process()` output and FM processing (AnalogLFO.cpp:488-489)
- Component spread: applied in constructor after RNG seed init, offsets stored as member variables
- Serialization: `dataToJson()`/`dataFromJson()` needed for component spread seed (new methods)

</code_context>

<specifics>
## Specific Ideas

No specific requirements -- open to standard approaches

</specifics>

<deferred>
## Deferred Ideas

None -- discussion stayed within phase scope

</deferred>

---

*Phase: 14-expanded-imperfections*
*Context gathered: 2026-03-16*
