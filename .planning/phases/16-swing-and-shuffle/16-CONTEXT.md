# Phase 16: Swing and Shuffle - Context

**Gathered:** 2026-03-17
**Status:** Ready for planning

<domain>
## Phase Boundary

Clocked LFO output can be grooved with swing timing for rhythmic modulation patches. In clocked mode, a swing setting above 50% audibly shifts alternate beat timing via MPC-style beat pairs. Swing is adjustable via right-click menu with musically useful presets (50% through 75%). In free-running mode, swing has no effect. At swing = 50% (default), clocked output is identical to v1.1 behavior. Requirements: PHASE-03, PHASE-04.

</domain>

<decisions>
## Implementation Decisions

### Swing timing model
- MPC-style beat pairs: alternate beats get different durations (even = long, odd = short)
- Swing modifies deltaPhase (phase accumulation rate), not readout
- Even beat accumulates phase slower (takes more time), odd beat accumulates faster (takes less time)
- Within each LFO cycle: first half = even beat (long), second half = odd beat (short)
- Beat parity determined by phase position within cycle (0→0.5 = even, 0.5→1.0 = odd)
- Range: 50% (straight) through 75% (extreme dotted-note swing)
- At 50%: both halves equal duration — identical to v1.1 behavior
- At 66%: classic triplet swing — even beat takes 2/3 of cycle time, odd beat takes 1/3

### Non-x1 ratio behavior (resolves STATE.md blocker)
- Swing always warps the individual LFO cycle regardless of ratio
- At x2: each of the 2 LFO cycles per clock period is independently swung
- At x/2: the single LFO cycle spanning 2 clock beats is swung within itself
- Consistent model: swing = "first half of each cycle is long, second half is short"
- No special cases for different ratios — swing is a per-cycle phase distortion

### Phase Offset interaction
- Swing and Phase Offset are independent layers
- Swing modifies deltaPhase (accumulation rate) — affects timing
- Phase Offset applied at readout (per Phase 12) — shifts position
- They stack naturally with no special interaction

### Right-click menu
- First context menu on this module (appendContextMenu override on AnalogLFOWidget)
- 6 named presets: Straight (50%), Light (54%), Medium (58%), Triplet (66%), Heavy (71%), Max (75%)
- Checkmark next to active preset (standard VCV menu pattern)
- Swing setting serialized via dataToJson/dataFromJson (same pattern as component spread seed from Phase 14)
- Default: Straight (50%) — no swing on fresh instances

### Display: swing text overlay
- Show swing preset name + percentage in display when swing > 50% (e.g., "TRIPLET 66%")
- Uses Phase 11 pill-background pattern for readability
- Positioned bottom-left of display area (opposite from BPM stack in bottom-right)
- Hidden at 50% (straight) — no clutter when swing is off
- Only visible in clocked mode (swing has no effect in free-running, so no overlay)
- Follows existing 200ms fade animation pattern for show/hide

### Display: phase-warped waveform
- Display shows asymmetric cycle reflecting swing timing
- 256 display samples distributed non-uniformly: more samples in first (long) half, fewer in second (short) half
- Uses same swing phase mapping as audio path for visual accuracy
- Phase dot position reflects swing-warped timing
- At 50% swing: uniform distribution — display identical to v1.1

### Claude's Discretion
- Exact deltaPhase multiplier formula for even/odd beats within the 50-75% range
- Whether swing phase boundary (0.5) needs smoothing to avoid discontinuity at the even/odd transition
- Exact display sample redistribution algorithm for phase-warped waveform
- Swing text overlay font size and exact pill dimensions (match existing overlay style)
- Whether swing overlay should dim like CLK line (60% alpha) or full brightness
- Implementation of appendContextMenu (VCV Rack MenuItem/createSubmenu patterns)

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements
- `.planning/REQUIREMENTS.md` — PHASE-03 (swing/shuffle warps phase in clocked mode), PHASE-04 (swing inactive in free-running)

### Architecture context
- `.planning/PROJECT.md` — Key decisions table: clock tracking, ratio table, authority scaling, serialization patterns
- `.planning/phases/11-display-polish/11-CONTEXT.md` — Display pill background pattern, text overlay rendering, fade animations, dual BPM layout
- `.planning/phases/14-expanded-imperfections/14-CONTEXT.md` — dataToJson/dataFromJson serialization pattern for component spread seed

### Prior phase decisions
- `.planning/STATE.md` — "Swing subdivision semantics at non-x1 ratios need resolution" blocker (RESOLVED by this context: swing always warps individual LFO cycle)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `clockBeatCount` (AnalogLFO.cpp:109): Tracks beats for division-aware resets — not used for swing parity (swing uses phase position instead)
- `phase` (AnalogLFO.cpp:67): Double-precision accumulator, 0→1 per cycle — swing modifies deltaPhase before `phase += deltaPhase`
- `deltaPhase` calculation (AnalogLFO.cpp:625): `freq * sampleTime` — swing multiplier applied here
- `drawGlowText()` (AnalogLFO.cpp:724): 2-pass amber glow text renderer — reuse for swing overlay
- `drawTextOverlays()` (AnalogLFO.cpp:741): Overlay renderer with pill backgrounds — add swing text here
- Fade animation system (syncFadeAlpha, ratioFadeAlpha, bpmFadeAlpha): Pattern for swing overlay fade
- `updateDisplayBuffer()`: Fills 256-sample buffer — needs swing phase mapping for non-uniform distribution
- `dataToJson()`/`dataFromJson()`: Already exists for component spread seed — add swing serialization here

### Established Patterns
- `isClocked` flag (AnalogLFO.cpp:568): Gates clocked-mode behavior — swing gated by this
- Phase 11 pill backgrounds: dark navy pill (#1a1a2e ~80%), 3px feather, individual per text element
- Text overlay color: amber (0.91, 0.66, 0.22) with 2-pass glow rendering
- 200ms fade transitions for overlay show/hide
- Relaxed atomics for audio→display data bridges
- Right-click menu: No existing appendContextMenu — this is the first one

### Integration Points
- Phase accumulation (AnalogLFO.cpp:664-665): `phase += deltaPhase; if (phase >= 1.0) phase -= 1.0;` — swing multiplier on deltaPhase before this line
- Display buffer (updateDisplayBuffer): Currently renders uniform phase distribution — needs swing-aware phase mapping
- AnalogLFOWidget (AnalogLFO.cpp:1204): Add `appendContextMenu()` override for swing presets
- Serialization: dataToJson/dataFromJson for swing value persistence

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

*Phase: 16-swing-and-shuffle*
*Context gathered: 2026-03-17*
