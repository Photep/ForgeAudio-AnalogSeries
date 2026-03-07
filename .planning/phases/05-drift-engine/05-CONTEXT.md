# Phase 5: Drift Engine - Context

**Gathered:** 2026-03-07
**Status:** Ready for planning

<domain>
## Phase Boundary

Drift knob introduces authentic analog pitch instability via a multi-timescale Ornstein-Uhlenbeck process (layered at 0.05Hz, 0.2Hz, 0.8Hz, ~2Hz). Drift CV input with attenuator trimpot for external modulation. At zero, drift is fully bypassed with zero CPU overhead. This completes the three-knob analog engine (morph, character, drift).

</domain>

<decisions>
## Implementation Decisions

### Drift range and curve
- Moderate pitch variation at full drift: 5-10% of base frequency
- Progressive x^2 curve matching character knob: subtle first half, stronger second half
- At zero: full bypass -- no OU computation, no random generation, zero CPU overhead (matches character-at-zero pattern)
- Each module instance gets a unique random seed -- two LFOs with identical settings drift independently, like two real analog synths

### Drift CV input
- Add DRIFT_ATTEN_PARAM trimpot matching the Morph CV and Character CV pattern
- Additive offset behavior: knob sets center position, CV sweeps around it (same as Morph/Character CV)
- Hard clamp at 0-1 range when combined knob + CV exceeds bounds
- Attenuator defaults to 0 (no CV effect until turned up) -- consistent with Morph/Character attenuators

### Bottom row layout
- 7 components organized as grouped pairs: [MATrim MCV] [CATrim CCV] [DATrim DCV] [OUT]
- Each attenuator trimpot sits directly next to its CV jack for visual clarity
- Panel SVG needs updating to accommodate the new trimpot

### Display behavior
- Drift modulates pitch (phase increment rate), not waveform shape -- display waveform trace does NOT change with drift
- Drift is visible through the phase dot's speed variation (speeding up and slowing down)
- No display buffer refresh needed for drift changes -- only phase dot position changes (already updated every sample via displayPhase atomic)
- Subtle dot instability at higher drift levels: increased glow variation or trail jitter as a visual hint that drift is active
- Remove/resolve the TODO comment at line 198 ("Phase 5: add driftChanged trigger here") -- no buffer trigger needed

### State persistence
- OU drift state reinitializes fresh on each patch load -- no save/restore of random state
- Real analog synths don't resume their drift state; fresh randomness on load is more authentic

### Claude's Discretion
- Exact OU process parameters (mean reversion rates, noise amplitudes per layer)
- How the four OU layers are combined and scaled
- Implementation of subtle dot instability visual (glow variation vs trail jitter vs other)
- Exact bottom row spacing to fit 7 components in ~57mm usable panel width
- Random seed generation method (module ID, random_device, etc.)

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `DRIFT_PARAM` (AnalogLFO.cpp:10): Already declared in enum, configured 0-1 range (line 161) -- ready to wire up
- `DRIFT_CV_INPUT` (AnalogLFO.cpp:18): Already declared in enum, configured (line 166) -- ready to wire up
- `progressiveCurve()` (line 44): x^2 curve function -- reuse directly for drift knob mapping
- CV processing pattern (lines 183-192): knob + attenuator * voltage / 10V, clamp -- replicate for Drift CV

### Established Patterns
- Lock-free double buffer for audio-to-display transfer (atomic read index + relaxed phase store)
- CV processing: `clamp(knob + atten * voltage / 10.f, 0.f, 1.f)` -- replicate exactly for Drift CV
- Early-out guard: `if (character < 0.001f) return` -- replicate as `if (drift < 0.001f)` for zero-overhead bypass
- Display phase dot already reads `displayPhase` atomic every frame -- drift speed variation is automatically visible

### Integration Points
- `process()` function (line 172): Drift processing goes before phase accumulation (line 178-180) to modulate `deltaPhase`
- `ParamId` enum (line 7): Needs `DRIFT_ATTEN_PARAM` added
- Panel SVG: Needs Drift attenuator trimpot added, bottom row respaced for 7 components
- Widget constructor (line 404): Needs new trimpot widget position
- TODO at line 198: Remove -- no driftChanged display trigger needed

</code_context>

<specifics>
## Specific Ideas

- Drift at zero must produce perfectly stable output -- no overhead, no randomness, matching the character-at-zero guarantee
- Phase dot speed variation IS the visual feedback for drift -- physically accurate (real analog drift changes timing, not waveshape)
- Subtle dot instability at higher drift (glow/trail variation) provides an additional visual cue without cluttering the display
- Per-instance unique drift gives multi-LFO patches organic life -- no two modules wander the same way

</specifics>

<deferred>
## Deferred Ideas

None -- discussion stayed within phase scope

</deferred>

---

*Phase: 05-drift-engine*
*Context gathered: 2026-03-07*
