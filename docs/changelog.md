# Changelog

Structured by shipped milestone. The plugin's `plugin.json` manifest version is `2.0.0` (VCV requires the MAJOR version to match the Rack major version — Rack 2); the milestone names below are the marketing/release history and are independent of that manifest number.

## v1.0 Analog Series LFO

*Shipped 2026-03-07.*

- Three-knob analog engine: Morph, Character, Drift
- Four-shape morph sweep: sine → triangle → saw → square
- Real-time single-cycle waveform display with phase-tracking dot
- Bipolar ±5V output
- CV inputs for Morph, Character, and Drift
- Sub-audio rate range 0.01–20 Hz
- Lock-free double buffer for audio-to-display transfer

## v1.1 Clock Sync

*Shipped 2026-03-13.*

- CLK input with edge detection and period tracking
- Dual-mode Rate knob: free Hz or 15 snapped musical ratios
- Division-aware phase reset on clock edges
- 3ms cosine anti-click crossfade on reset
- EMA period smoothing with outlier rejection
- Display: SYNC badge, ratio label, BPM readout with fade animations
- Reduced drift authority in clocked mode

## v1.2 Deep Analog

*Shipped 2026-03-17.*

- FM input with exponential frequency modulation
- Separate RESET trigger jack with 1ms blanking
- Phase Offset knob (0–360°) with CV input
- Swing/shuffle for clocked mode (inactive when free-running)
- Expanded imperfections: phase jitter, DC-offset wander, thermal pitch slew
- Per-instance component spread with serialized seed
- Waveform bleed (neighbor crosstalk) during morph

## v1.3 Forge Noir

*Shipped 2026-06-13.*

- 18HP Forge Noir panel with custom machined-metal SVG components
- Fifth morph shape: narrow pulse (PWM folded into the morph sweep)
- Three-column CRT display: corner brackets, scanlines, breathing border glow
- Per-edge animated SYNC badge flash while LOCKED

## v1.4 Tempered

*In progress (2026-07).*

- Release hardening: functional bug fixes and code cleanups
- Automated test harness (Rack-independent DSP core, integration harness, golden regression) with cross-platform CI
- IP hardening: GPL-3.0 license, trial-font purge
- VCV Library compliance and `.vcvplugin` packaging
- This user manual
- Public source release
