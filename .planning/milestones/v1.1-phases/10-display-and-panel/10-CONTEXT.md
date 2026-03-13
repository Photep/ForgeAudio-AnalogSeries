# Phase 10: Display and Panel - Context

**Gathered:** 2026-03-12
**Status:** Ready for planning

<domain>
## Phase Boundary

The user can see at a glance whether the LFO is clock-synced and at what ratio. This phase adds NanoVG text overlays to the waveform display (SYNC badge, ratio label, BPM readout, Hz readout), updates the panel SVG with a CLK jack label and RATE label, and implements visual state transitions. No DSP changes -- all display/panel work.

Requirements: DISP-01, DISP-02, DISP-03, DISP-06

</domain>

<decisions>
## Implementation Decisions

### SYNC badge
- Position: top-right corner of waveform display
- Style: amber text (#e8a838) with subtle glow, no background pill -- matches waveform trace aesthetic
- Rendering: NanoVG font (nvgText), not geometric paths -- simpler for dynamic text
- State behavior:
  - FREE: no badge shown
  - ACQUIRING: "SYNC" blinks at ~2Hz (fade in/out cycle)
  - LOCKED: "SYNC" solid amber

### Ratio label
- Position: top-left corner of waveform display
- Shows current ratio from RATIO_LABELS[] (e.g., "/4", "x2", "x1")
- Only visible when clocked (ACQUIRING or LOCKED state)
- Style: amber (#e8a838) with subtle glow, same as SYNC badge
- Updates immediately when Rate knob snaps to new ratio (no crossfade between values)

### BPM readout
- Position: bottom-right corner of waveform display
- Shows effective BPM: (clock BPM x ratio), not source clock BPM
  - E.g., 120 BPM clock at /4 = "30 BPM", at x2 = "240 BPM"
- Only visible when clocked (ACQUIRING or LOCKED state)
- Style: amber (#e8a838) with subtle glow, uniform with other overlays
- Updates immediately with ratio changes

### Hz readout (free-running mode)
- Position: top-left corner of waveform display (same position as ratio label)
- Shows current frequency in free-running mode (e.g., "0.70 Hz")
- Visible when NOT clocked -- extends v1.0 display with rate information
- Style: amber (#e8a838) with subtle glow, same visual language

### Display layout summary
```
FREE mode:
┌──────────────────────────┐
│ 0.70 Hz                  │
│     ╱╲      ╱╲           │
│    ╱  ╲    ╱  ╲          │
│───╱────╲──╱────╲─────────│
│         ╲╱      ╲╱       │
│                          │
└──────────────────────────┘

LOCKED mode:
┌──────────────────────────┐
│ /4                 SYNC  │
│     ╱╲      ╱╲           │
│    ╱  ╲    ╱  ╲          │
│───╱────╲──╱────╲─────────│
│         ╲╱      ╲╱       │
│                 120 BPM  │
└──────────────────────────┘
```

### Display state transitions
- Clock connect: Hz fades out (~200ms), SYNC badge starts blinking (ACQUIRING), ratio + BPM fade in when LOCKED
- Clock loss (timeout, cable still connected): SYNC/ratio/BPM fade out (~200ms), Hz fades back in
- Cable disconnect: same 200ms fade transition (smooth visuals even though state change is instant)
- Ratio knob change: immediate text swap, no crossfade between ratio values
- All transitions ~200ms fade, consistent with Phase 9's smooth frequency slew philosophy

### Panel SVG updates
- "CLK" label above the CLK jack at (42.96, 86.0) -- lavender (#8888aa) geometric path letterforms
- "RATE" label above the Rate knob at (18.0, 86.0) -- same lavender style, added for consistency
- Both labels match existing panel label style (geometric paths, nanosvg compatible)

### Claude's Discretion
- Font choice for NanoVG text overlays (bundled font or VCV built-in)
- Exact font sizes for SYNC badge, ratio, BPM, Hz text
- Glow implementation details (multi-pass or blur)
- Exact fade easing curve for 200ms transitions
- BPM number formatting (integer vs one decimal place)
- SYNC blink easing (sine wave, linear, etc.)

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `displayClockState` (std::atomic<int>): Lock-free clock state for display (FREE=0, ACQUIRING=1, LOCKED=2) -- drives badge visibility
- `displayRatioIndex` (std::atomic<int>): Lock-free ratio index (-1 = free-running, 0-14 = ratio) -- drives ratio label via RATIO_LABELS[]
- `smoothedPeriod` (float): Clock period measurement -- needed for BPM calculation (60.0 / smoothedPeriod)
- `WaveformDisplay` class: Existing NanoVG display widget with drawLayer(), drawBackground(), drawInsetFrame(), drawWaveformTrace(), drawPhaseDot()
- `breathePhase` animation: Existing ~0.8Hz animation state in WaveformDisplay -- could drive SYNC blink timing
- Four-pass glow rendering pattern: Used for waveform trace (widths/alphas arrays) -- adaptable for text glow

### Established Patterns
- Atomic display transfer: All display-facing values use std::atomic with relaxed memory ordering
- Display update in drawLayer() layer 1: All NanoVG rendering happens here
- Color palette: amber #e8a838 = nvgRGBAf(0.91f, 0.66f, 0.22f, ...), navy background #0d0d1a = nvgRGBAf(0.051f, 0.051f, 0.102f, ...)
- Panel SVG: All text as geometric paths, fill="#8888aa", nanosvg compatible (no text/defs/style/filters)

### Integration Points
- `WaveformDisplay::drawLayer()` at line 717: Add text overlay drawing after drawPhaseDot()
- `AnalogLFO` struct: Add std::atomic<float> for smoothed period (for BPM calculation in display thread)
- Panel SVG `res/AnalogLFO.svg`: Add CLK and RATE label path groups
- Font loading: In WaveformDisplay constructor or step(), load font via `APP->window->loadFont()`

</code_context>

<specifics>
## Specific Ideas

- The three-corner layout (ratio top-left, SYNC top-right, BPM bottom-right) keeps the waveform center clear for the trace and phase dot
- FREE/ACQUIRING/LOCKED maps cleanly to hidden/blinking/solid -- three distinct visual states for three internal states
- Hz readout in free-running mode is a v1.0 display enhancement that adds value even without clock sync
- All text overlays use the same amber color for a unified "data layer" on top of the waveform

</specifics>

<deferred>
## Deferred Ideas

None -- discussion stayed within phase scope

</deferred>

---

*Phase: 10-display-and-panel*
*Context gathered: 2026-03-12*
