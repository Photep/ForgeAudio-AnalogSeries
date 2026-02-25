# Phase 3: Waveform Display - Context

**Gathered:** 2026-02-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Real-time single-cycle waveform visualization with a phase-tracking dot in the upper portion of the module panel. The display shows exactly what the DSP engine is producing and updates continuously. Lock-free double buffer ensures no audio impact.

</domain>

<decisions>
## Implementation Decisions

### Display Size & Placement
- Generous window: ~25-35% of panel height, the display is a featured visual element
- Starts right below the module title with no gap — maximize display real estate
- Near full width with small margins (~2mm each side) — maximize waveform visibility
- Darker navy background inside the display — creates visual depth like a real screen
- Subtle inset frame with thin border — embedded hardware screen feel
- No grid lines, no axis markers, no center line — clean background only
- Purely visual — no text labels or frequency readouts inside the display

### Waveform Trace Style
- Line weight: Claude's discretion — pick based on display size and visual balance
- Subtle amber glow/bloom around the trace line — adds warmth without looking retro
- Continuous morph update — display morphs smoothly in sync with the knob, every intermediate shape visible
- Exact DSP output — display shows precisely what the engine computes (WYSIWYG); future character/drift will be visible here
- Clean modern aesthetic (Mutable Instruments style), not retro oscilloscope

### Phase Dot Behavior
- Small and precise dot — just a few pixels bigger than the line width
- Both glow halo AND short comet-like trail — gives the dot life and shows direction of travel
- Brighter amber / white-amber center — hotter color than the trace for contrast
- At very slow rates: subtle pulse/breathe animation so the dot shows it's active even when movement is imperceptible

### Display Edge Cases
- On startup: show waveform immediately — pre-compute current morph shape, display is never empty
- Fast morph knob changes: Claude's discretion — pick what looks best (instant vs slight smoothing)
- When bypassed or rate=0: dim the entire display — waveform and dot fade to lower brightness, visually reads as inactive
- Target monitor refresh rate (typically 60fps) for smoothest possible animation

### Claude's Discretion
- Exact line weight for waveform trace
- Visual smoothing behavior during fast knob changes
- Exact dimensions and margins (within the "generous" and "near full width" guidelines)
- Trail length and glow intensity details
- Pulse/breathe animation timing at slow rates

</decisions>

<specifics>
## Specific Ideas

- Clean modern look like Mutable Instruments / modern Eurorack — not retro oscilloscope
- The display is WYSIWYG: when Character and Drift are added in later phases, their effects should be visible in the waveform trace
- Dot with both glow AND trail creates a sense of liveliness — the display should feel alive, not static
- Darker navy inset creates the feeling of looking into a screen embedded in hardware

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 03-waveform-display*
*Context gathered: 2026-02-25*
