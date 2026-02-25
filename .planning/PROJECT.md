# Forge Audio — Analog Series

## What This Is

A pair of VCV Rack modules (LFO and VCO) that model the sound and behavior of classic analog synthesizer oscillators. Each module features three core controls: a continuous waveform morph knob, an analog character knob that crossfades between mathematically perfect digital waveforms and modeled classic synth references, and a drift knob that introduces authentic analog imperfections. A real-time waveform display shows the exact output shape with a phase-tracking indicator.

## Core Value

The three-knob analog engine — morph, character, drift — that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback showing exactly what's happening.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] Continuous waveform morph: Sine → Triangle → Saw → Square
- [ ] Analog character knob: crossfade from digital perfection to classic analog reference waveforms
- [ ] Drift knob: pitch drift, phase jitter, component spread, DC offset drift, HF rolloff, pitch slew
- [ ] Real-time single-cycle waveform display with phase-tracking dot
- [ ] Display reflects all three knobs (morph, character, drift) in real time
- [ ] Morphed output and inverted morphed output
- [ ] CV inputs for all three main knobs (morph, character, drift)
- [ ] Tracking error modeling (toggleable via right-click context menu)
- [ ] 14-16HP panel with Forge Audio brand identity
- [ ] SVG panel built for designer handoff (template files, documented coordinates)
- [ ] LFO module: rate control covering sub-audio LFO range
- [ ] LFO module: reset/sync input
- [ ] LFO module: FM input
- [ ] VCO module: V/Oct pitch input with standard 1V/octave tracking
- [ ] VCO module: FM input
- [ ] VCO module: through-zero FM option
- [ ] VCO module: hard sync input
- [ ] VCO module: shape CV input (controls morph position)
- [ ] VCO module: phase distortion
- [ ] Antialiasing strategy for audio-rate VCO (research determines approach)
- [ ] Waveform bleed modeling (research determines approach)
- [ ] Classic analog reference waveforms per shape (research determines targets — e.g., Minimoog saw, Roland square)

### Out of Scope

- Individual waveform outputs — single morphed output keeps the design focused
- Built-in effects (chorus, reverb) — these are oscillator modules, not voices
- Polyphonic operation — mono oscillators for v1
- Final production panel artwork — placeholder branding, designer will create final SVG

## Context

- **Prior work:** Proof-of-concept LFO built at `/Users/mrcbrown/Claude/Software/Forge Audio/LFO/` — 71-line C++ module with exponential pitch scaling, four simultaneous waveform outputs (sine/tri/saw/square), FM input, reset input, Schmitt trigger sync. Clean digital implementation with no analog modeling.
- **Brand identity established:** Deep navy (#1a1a2e) background, forge amber (#e8a838) accents, muted lavender labels, bright white-gray text. Professional Eurorack aesthetic.
- **VCV Rack 2 SDK:** Standard plugin architecture — Module class with process() callback, ModuleWidget with SVG panel, mm2px coordinate system.
- **Build system:** Standard VCV Rack Makefile with plugin.mk, no external DSP dependencies.
- **Waveform display inspiration:** Arturia Pigments LFO display — single-cycle view with real-time shape updates. Amber trace on dark navy, bright dot riding the waveform to show current phase position.
- **Release strategy:** LFO module first (v1), VCO module second (v2). LFO is familiar territory; VCO adds complexity with audio-rate antialiasing, pitch tracking, sync, and through-zero FM.

## Constraints

- **Platform:** VCV Rack 2 SDK, C++17, cross-platform (Mac/Windows/Linux)
- **Panel rendering:** SVG via nanosvg — limited subset of SVG features (no filters, no CSS, text as paths)
- **Real-time:** All DSP in process() callback at sample rate — no allocation, no blocking
- **Display:** VCV Rack's `LightWidget` / `FramebufferWidget` / custom draw via NanoVG — must be efficient enough to not drop frames
- **Panel size:** 14-16HP (70.96–81.28mm width) × 128.5mm height
- **Designer handoff:** SVG panel must be structured for easy redesign — clear layers, documented coordinates, template files

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Three-knob analog engine (morph, character, drift) | Separates waveform selection, tonal shaping, and imperfection modeling into independent axes | — Pending |
| Morph order: Sine → Tri → Saw → Square | Smooth shapes to sharp edges — natural progression of harmonic content | — Pending |
| Character knob targets specific classic synths | More authentic than generic "warmth" — research identifies Minimoog, Roland, etc. as reference targets | — Pending |
| Drift knob bundles all analog imperfections | Single control for pitch drift, jitter, component spread, DC offset, HF rolloff, pitch slew | — Pending |
| Tracking error as right-click toggle | Authentic but potentially frustrating — users can opt in without cluttering the panel | — Pending |
| Single morphed output + inverted | Keeps panel focused on the three character controls rather than individual waveform jacks | — Pending |
| Real-time waveform display | Visual feedback makes the three knobs intuitive — see the character changes as you dial them | — Pending |
| LFO first, VCO second | LFO is familiar territory from POC; VCO adds audio-rate complexity (aliasing, tracking, sync) | — Pending |
| 14-16HP panels | Room for three knobs, CV inputs, display, and outputs with visual breathing room | — Pending |
| Placeholder branding for designer handoff | Use existing Forge Audio identity now, hire designer for production artwork later | — Pending |

---
*Last updated: 2025-02-25 after initialization*
