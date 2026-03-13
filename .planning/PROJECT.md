# Forge Audio — Analog Series

## What This Is

A VCV Rack 2 module series featuring analog-modeled oscillators. The first module is a sub-audio LFO built around a three-knob analog engine (morph, character, drift) with real-time waveform display and clock sync. Each knob controls an independent axis: waveform shape selection, classic synth character modeling, and analog instability — all visible in real time on the display. When a clock source is patched in, the Rate knob switches to musical division/multiplication ratios with beat-aligned phase reset.

## Core Value

The three-knob analog engine — morph, character, drift — that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback showing exactly what's happening.

## Requirements

### Validated

- ✓ Continuous waveform morph: Sine → Triangle → Saw → Square — v1.0
- ✓ Analog character knob: crossfade from digital perfection to classic analog references (Minimoog saw, Roland square, Moog/Prophet triangle, analog sine) — v1.0
- ✓ Drift knob: multi-timescale Ornstein-Uhlenbeck pitch drift — v1.0
- ✓ Real-time single-cycle waveform display with phase-tracking dot — v1.0
- ✓ Display reflects all three knobs (morph, character, drift) in real time — v1.0
- ✓ Bipolar ±5V morphed output (inverted output removed by design) — v1.0
- ✓ CV inputs for all three main knobs (morph, character, drift) — v1.0
- ✓ 12HP panel with Forge Audio brand identity — v1.0
- ✓ SVG panel structured for designer handoff — v1.0
- ✓ LFO rate control covering sub-audio range (0.01-20Hz) — v1.0
- ✓ Lock-free double buffer for audio-to-display transfer — v1.0
- ✓ CLK trigger input with edge detection and period tracking — v1.1
- ✓ Dual-mode Rate knob (free Hz / 15 snapped musical ratios) — v1.1
- ✓ Phase reset on clock edge with division-aware counting — v1.1
- ✓ Anti-click 3ms cosine crossfade on phase reset — v1.1
- ✓ Clock period smoothing via EMA with outlier rejection — v1.1
- ✓ Display: SYNC badge, ratio label, BPM readout with fade animations — v1.1
- ✓ Panel SVG updated with CLK jack and label — v1.1
- ✓ Drift authority reduced in clocked mode (2% vs 7.5%) — v1.1
- ✓ Smooth frequency slew during clock/free transitions — v1.1

### Active

(None — next milestone not yet defined)

**Deferred (future milestones):**
- [ ] LFO: FM input
- [ ] LFO: Phase jitter, DC offset drift, pitch slew, component spread
- [ ] LFO: Waveform bleed in morph transitions
- [ ] LFO: CV control of division ratio
- [ ] LFO: Separate RESET jack (independent from CLK)
- [ ] LFO: Animated sync badge (clock-pulse flash)
- [ ] LFO: Phase offset knob/CV
- [ ] LFO: Swing/shuffle control
- [ ] LFO: Display incoming clock BPM alongside effective BPM
- [ ] VCO module: V/Oct pitch input with 1V/octave tracking
- [ ] VCO module: FM input and through-zero FM
- [ ] VCO module: Hard sync input
- [ ] VCO module: Morph-aware polyBLEP antialiasing
- [ ] VCO module: Phase distortion
- [ ] VCO module: Tracking error modeling (right-click toggle)
- [ ] VCO module: Coarse/fine tune controls
- [ ] VCO module: Oversampling option (Off/2x/4x)

### Out of Scope

- Individual waveform outputs — single morphed output IS the design concept
- Polyphonic operation — 16x CPU cost, complicates drift and display
- Built-in effects (chorus, reverb) — oscillators oscillate, effects process
- Wavetable mode — different paradigm, dilutes analog identity
- Named synth presets — undercuts hands-on tweaking, invites trademark issues
- MIDI input / quantization — upstream module responsibilities
- Amplitude envelope — oscillators oscillate, envelopes shape
- Scope / spectrum analyzer — display is shape preview, not measurement tool
- Individually exposed drift params — one drift knob with curated proportions
- Built-in sub-oscillator — panel complexity, dilutes three-knob focus
- Octave snap / semitone selector — not meaningful for sub-audio LFO rates
- PLL-based clock tracking — overkill for LFO rates; simple edge measurement + EMA is sufficient
- Continuous (non-snapped) clock ratios — anti-pattern; produces non-musical results

## Context

**Current state:** v1.1 Clock Sync shipped. 890 lines of C++, 12HP panel, three-knob analog engine with clock sync.
**Tech stack:** VCV Rack 2 SDK, C++17, NanoVG for display, nanosvg for panel.
**Build system:** Standard VCV Rack Makefile with plugin.mk, no external dependencies.
**Brand identity:** Deep navy (#1a1a2e), forge amber (#e8a838), muted lavender labels (#8888aa), white-gray text (#c0c0d0).
**Prior work:** POC LFO at `/Users/mrcbrown/Claude/Software/Forge Audio/LFO/` — clean digital implementation, no analog modeling.
**Release strategy:** LFO shipped (v1.0 + v1.1). VCO module next (v2.0). LFO validates the three-knob engine; VCO adds audio-rate complexity.

## Constraints

- **Platform:** VCV Rack 2 SDK, C++17, cross-platform (Mac/Windows/Linux)
- **Panel rendering:** SVG via nanosvg — limited subset (no filters, no CSS, text as paths)
- **Real-time:** All DSP in process() callback at sample rate — no allocation, no blocking
- **Display:** NanoVG on FramebufferWidget — must not drop frames
- **Panel size:** 12HP (60.96mm) × 128.5mm height
- **Designer handoff:** SVG panel structured for easy redesign

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Three-knob analog engine (morph, character, drift) | Separates waveform selection, tonal shaping, and imperfection modeling into independent axes | ✓ Good — clean separation, each knob has clear purpose |
| Morph order: Sine → Tri → Saw → Square | Smooth shapes to sharp edges — natural progression of harmonic content | ✓ Good — intuitive knob sweep |
| Character targets specific classic synths | More authentic than generic "warmth" — Minimoog, Roland, Moog/Prophet as references | ✓ Good — recognizable character per shape |
| Drift bundles all analog imperfections | Single control for pitch drift scaling everything in curated proportions | ✓ Good — simple UX, sounds alive |
| Single morphed output (no INV) | Keeps panel focused on three character controls | ✓ Good — cleaner panel, no user confusion |
| Real-time waveform display | Visual feedback makes three knobs intuitive | ✓ Good — users see character changes as they dial |
| LFO first, VCO second | LFO validates engine at sub-audio rates before VCO adds complexity | ✓ Good — clean foundation |
| 12HP panel | Room for three knobs, CV inputs, display, and outputs | ✓ Good — balanced density |
| Falling saw ramp | Matches Minimoog/SH-101/Juno convention, eliminates morph crossfade amplitude dip | ✓ Good — solved morph artifact |
| Characterize-then-morph ordering | Analog deformation per-shape before morph crossfade for coherent transitions | ✓ Good — clean morphing |
| Progressive x² character curve | Character at 0.5 = 25% effect, rewards exploration | ✓ Good — subtle to aggressive range |
| Four-layer OU drift (0.05/0.2/0.8/2Hz) | Musical multi-timescale pitch instability | ✓ Good — natural analog feel |
| Per-module Xoroshiro128Plus RNG | Independent drift per instance, no shared state | ✓ Good — each module unique |
| No OU state serialization | Fresh randomness on patch load — authentic analog behavior | ✓ Good — matches real hardware |
| Lock-free double buffer for display | No mutexes in audio thread | ✓ Good — zero audio impact |
| displayDrift atomic for CV-responsive visuals | Drift visuals respond to CV, not just knob position | ✓ Good — visual accuracy |
| Two-row bottom layout (trimpots above jacks) | Standard Eurorack convention, clean grouping | ✓ Good — improved readability |
| Three-state clock tracker (FREE/ACQUIRING/LOCKED) | Clean separation of unclocked, learning, and tracking states | ✓ Good — predictable transitions, fast-track re-acquisition |
| EMA period smoothing (alpha 0.3) with outlier rejection | Balances responsiveness with jitter filtering | ✓ Good — stable tracking even with noisy clocks |
| 15 discrete ratios via round(knob * 14) | No hysteresis needed, clean integer snap | ✓ Good — simple, deterministic ratio selection |
| Cosine crossfade (3ms) on phase reset | Zero-derivative endpoints prevent clicks | ✓ Good — inaudible resets confirmed |
| Drift authority scaling (2% clocked, 7.5% free) | Prevents phase error accumulation while preserving analog character | ✓ Good — musical in both modes |
| Relaxed atomics for all display bridges | Independent per-atomic reads, no ordering needed | ✓ Good — correct and performant |
| CLK label as SVG paths (not text element) | nanosvg compatibility — text elements not supported | ✓ Good — renders correctly |
| RATE label stays white-gray (#c0c0d0) | Matches knob label convention; CLK gets lavender as jack label | ✓ Good — visual consistency |

---
*Last updated: 2026-03-13 after v1.1 milestone completed*
