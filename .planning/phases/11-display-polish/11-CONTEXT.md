# Phase 11: Display Polish - Context

**Gathered:** 2026-03-13
**Status:** Ready for planning

<domain>
## Phase Boundary

Make HUD text overlays (SYNC badge, ratio label, BPM, Hz) always readable when the waveform trace passes through them, and add incoming clock BPM display alongside the effective (ratio-adjusted) BPM. Requirements: DISP-01, DISP-02.

</domain>

<decisions>
## Implementation Decisions

### Pill background styling
- Dark navy pill backgrounds (#1a1a2e at ~80% opacity) behind each text overlay
- Feathered gradient edges (3-4px fade from full opacity to transparent) — no hard boundary, text emerges from a dark cloud
- Each text element gets its own individual pill (SYNC, ratio, Hz, BPM stack each have separate pills)
- Comfortable padding (4-5px) around text within pills

### Dual BPM layout
- Raw incoming clock BPM and effective BPM stacked vertically in the bottom-right corner
- Raw clock BPM on top (labeled "CLK"), effective BPM below (labeled "BPM")
- Both lines share one pill background (grouped as related info)
- Raw CLK line: smaller font (~8px) and dimmer (~60% alpha) — supplemental context
- Effective BPM line: standard font (10px) and full alpha — primary readout
- At x1 ratio: only one line shown ("120 BPM") — no redundant CLK line
- At non-x1 ratios: both lines shown (e.g., "120 CLK" / "30 BPM")

### Text color and glow
- All text stays amber (0.91, 0.66, 0.22) — same color family as waveform trace
- Pills provide the contrast; text color stays cohesive with display identity
- Keep existing 2-pass glow rendering (blur pass + sharp pass) on all text
- Raw CLK line gets proportionally dimmed glow (60% alpha on glow pass too)

### Information density
- Free-running mode: no changes — single Hz readout in top-left with pill background added
- Clocked mode: ratio (top-left), SYNC (top-right), BPM stack (bottom-right) — no Hz shown
- No additional info elements added beyond what DISP-01/DISP-02 require
- BPM stack appears immediately during ACQUIRING state (alongside blinking SYNC), not waiting for LOCKED

### Claude's Discretion
- Exact gradient implementation (NVG box gradient vs radial gradient for pill edges)
- Precise pill corner radius
- Fine-tuning of feather distance if 3-4px doesn't look right at runtime
- Exact vertical spacing between CLK and BPM lines in the stacked pair

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `drawGlowText()` (AnalogLFO.cpp:724): 2-pass amber glow text renderer — extend to draw pill background before text
- `drawTextOverlays()` (AnalogLFO.cpp:741): Current overlay renderer — all modifications happen here
- Fade animation system (syncFadeAlpha, ratioFadeAlpha, bpmFadeAlpha, hzFadeAlpha): Reuse for new CLK line fade
- `displaySmoothedPeriod` atomic: Already bridges clock period to display thread — raw BPM = 60/period

### Established Patterns
- NanoVG rendering on FramebufferWidget layer 1 — all display code runs in drawLayer()
- Relaxed atomics for all audio→display bridges — no ordering constraints needed
- 200ms fade transitions for text overlay show/hide — new CLK element should match
- ShareTechMono-Regular font loaded per frame from asset::system()

### Integration Points
- `drawTextOverlays()` is the single function to modify — called from drawLayer()
- New pill backgrounds render before text in the same NVG context
- CLK BPM calculation: `60.f / period` (raw clock), already have `effectiveBPM = 60.f / period * RATIO_TABLE[ratioIdx]`
- x1 ratio detection: `ratioIdx == 7` (RATIO_TABLE[7] = 1.0)

</code_context>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 11-display-polish*
*Context gathered: 2026-03-13*
