# Forge Audio — Analog Series

## What This Is

A VCV Rack 2 module series featuring analog-modeled oscillators. The first module is a sub-audio LFO built around a three-knob analog engine (morph, character, drift) with real-time waveform display, clock sync, FM modulation, expanded analog imperfections, and groove features. Each knob controls an independent axis: waveform shape selection, classic synth character modeling, and analog instability — all visible in real time on the display. When a clock source is patched in, the Rate knob switches to musical division/multiplication ratios with beat-aligned phase reset.

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
- ✓ Display text overlays readable via pill backgrounds — v1.2
- ✓ Incoming clock BPM alongside effective BPM — v1.2
- ✓ FM input jack with exponential frequency modulation — v1.2
- ✓ FM authority reduced in clocked mode — v1.2
- ✓ Separate RESET trigger jack with 1ms blanking — v1.2
- ✓ RESET uses existing cosine crossfade — v1.2
- ✓ Phase offset knob (0-360 degrees) at readout — v1.2
- ✓ Phase offset CV input — v1.2
- ✓ Swing/shuffle for clocked mode — v1.2
- ✓ Swing inactive in free-running mode — v1.2
- ✓ Phase jitter scaled by Drift — v1.2
- ✓ DC offset wander scaled by Drift — v1.2
- ✓ Pitch slew (thermal lag) scaled by Drift — v1.2
- ✓ Per-instance component spread with serialized seed — v1.2
- ✓ Waveform bleed (neighbor crosstalk) during morph — v1.2
- ✓ CV control of division ratio (via Rate CV in clocked mode) — v1.1

### Active

- ✓ LFO: Morph range extension — Sine → Tri → Saw → Square → Narrow Pulse (PWM integrated into morph) — v1.3
- ✓ LFO: Forge Noir panel implementation (14HP SVG panel + custom widget components) — v1.3 (Phase 19)
- [ ] LFO: Display layout per Forge Noir design (pills in left/right margins, waveform in center column)
- [ ] LFO: Animated sync badge (clock-pulse flash)

**Deferred (future milestones):**
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
- Linear FM mode — perceptually identical to exponential at LFO rates
- Through-zero FM — audio-rate timbral effect, not meaningful at LFO rates
- Surge-style modulation routing system — abandoned in favor of direct CV jacks; over-engineered for LFO use case

## Context

**Current state:** v1.3 Forge Noir in progress. ~1,600 lines of C++, 18HP Forge Noir "fresh" panel complete (Phase 20.1 — redesigned from 14HP, single-row equal secondary knobs, grouped clock section, corner bolts, widget-owned knob art) with custom SVG components, CRT-aesthetic display layout (Phase 20), three-knob analog engine with clock sync, FM modulation, expanded imperfections, waveform bleed, swing timing, and 5-shape morph sweep (sine-tri-saw-square-pulse). Animated SYNC badge (Phase 21) remaining.
**Tech stack:** VCV Rack 2 SDK, C++17, NanoVG for display, nanosvg for panel.
**Build system:** Standard VCV Rack Makefile with plugin.mk, no external dependencies.
**Brand identity:** Forge Noir — near-black panel (#0c0c0c), ember orange (#e85d26), gold accent (#daa520), warm white text (#e8e4e0). Fonts: Bebas Neue (brand/hero), Chakra Petch (labels), JetBrains Mono (data).
**Prior work:** POC LFO at `/Users/mrcbrown/Claude/Software/Forge Audio/LFO/` — clean digital implementation, no analog modeling.
**Release strategy:** v1.3 Forge Noir is the final LFO milestone. VCO module (v2.0) planned after LFO completion.
**Known tech debt:** FM, RESET, Phase Offset controls at temporary panel positions — resolved by Forge Noir panel redesign.

## Constraints

- **Platform:** VCV Rack 2 SDK, C++17, cross-platform (Mac/Windows/Linux)
- **Panel rendering:** SVG via nanosvg — limited subset (no filters, no CSS, text as paths)
- **Real-time:** All DSP in process() callback at sample rate — no allocation, no blocking
- **Display:** NanoVG on FramebufferWidget — must not drop frames
- **Panel size:** 14HP (71.12mm) × 128.5mm height (expanded from 12HP in Forge Noir redesign)
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
| Three-state clock tracker (FREE/ACQUIRING/LOCKED) | Clean separation of unclocked, learning, and tracking states | ✓ Good — predictable transitions |
| EMA period smoothing (alpha 0.3) with outlier rejection | Balances responsiveness with jitter filtering | ✓ Good — stable tracking |
| 15 discrete ratios via round(knob * 14) | No hysteresis needed, clean integer snap | ✓ Good — deterministic ratio selection |
| Cosine crossfade (3ms) on phase reset | Zero-derivative endpoints prevent clicks | ✓ Good — inaudible resets |
| Drift authority scaling (2% clocked, 7.5% free) | Prevents phase error accumulation while preserving analog character | ✓ Good — musical in both modes |
| Relaxed atomics for all display bridges | Independent per-atomic reads, no ordering needed | ✓ Good — correct and performant |
| Individual pill backgrounds per HUD overlay | Better visual integration than shared HUD pill | ✓ Good — readable at all waveform positions |
| Phase Offset applied at readout (not accumulator) | Preserves all existing timing behavior | ✓ Good — no side effects on clock/drift |
| FM processing after frequency slew filter | Preserves full modulation bandwidth | ✓ Good — FM not sluggish |
| Clocked FM depth scale 0.5f | Clock phase resets already enforce sync | ✓ Good — usable FM in clocked mode |
| DC offset applied after crossfade capture | Prevents clicks on phase reset | ✓ Good — clean resets preserved |
| Component spread seed as hex strings | Avoids uint64_t sign issues in JSON | ✓ Good — reliable serialization |
| Waveform bleed via wrapping ring topology | Modular arithmetic for neighbor access | ✓ Good — clean and extensible |
| Swing as deltaPhase multiplier after drift/jitter | Commutative, preserves groove feel | ✓ Good — MPC-style timing |
| Swing via right-click menu (not knob) | Preserves panel density | ✓ Good — functional within 12HP |
| Skip Phase 17 Panel Redesign | 12HP density at limit; Surge-style modulation routing abandoned | Closed — panel will evolve with Forge Noir design language instead |
| PWM as morph extension (not separate control) | Extends natural harmonic progression past square into pulse; no new knob needed | ✓ Good — smooth duty interpolation, no staircase artifacts |
| Forge Noir design language | Near-black panel, ember orange accents, machined metal knobs, scalloped trimpots, forge emblem | — Pending |
| Panel expansion to 14HP | Forge Noir layout needs breathing room for 5 main knobs + display | — Pending |

---
*Last updated: 2026-06-12 after Phase 20.1 (Panel Redesign 18HP Fresh Layout) completion*
