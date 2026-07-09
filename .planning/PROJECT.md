# Forge Audio — Analog Series

## What This Is

A VCV Rack 2 module series featuring analog-modeled oscillators. The first module is a sub-audio LFO built around a three-knob analog engine (morph, character, drift) with real-time waveform display, clock sync, FM modulation, expanded analog imperfections, and groove features. Each knob controls an independent axis: waveform shape selection, classic synth character modeling, and analog instability — all visible in real time on the display. When a clock source is patched in, the Rate knob switches to musical division/multiplication ratios with beat-aligned phase reset.

## Core Value

The three-knob analog engine — morph, character, drift — that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback showing exactly what's happening.

## Current Milestone: v1.4 Tempered

**Goal:** Take the feature-complete Analog LFO to a publishable, VCV-Library-ready plugin — bugs fixed, tested, package compliant, manual written, source published.

**Target features:**
- Automated testing: C++ test target (unit tests on extracted DSP logic) plus a headless `process()`-driver integration harness; bug fixes land as regression tests
- Functional bug fixes from CODE-REVIEW-FINDINGS.md: clock tracker >3× lockout (#1), x1.5/÷1.5 mid-cycle truncation (#2), free-run phase-dot swing desync (#3), patch-load crash guard (#4)
- Code cleanup: dead code (#8), unreachable `isStill` (#9), pill fade-out symmetry (#10), frame-rate-independent animations (#11), display buffer off the audio thread (#12)
- VCV Library compliance: GPL-3.0 LICENSE (#5), populated plugin.json URLs (#6), trial-font removal incl. git-history purge (#7)
- Source publication: public GitHub repo (provides sourceUrl/pluginUrl)
- User manual authored as GitHub-flavored Markdown under `docs/` (linked from plugin.json manualUrl)
- Release packaging: verified build + `.vcvplugin` artifact ready for submission

**Out of milestone scope:** No new DSP features — the LFO is feature-frozen. VCO module remains deferred to v2.0.

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
- ✓ Morph range extension: Sine → Tri → Saw → Square → Narrow Pulse (PWM integrated into morph, even 5-shape rescale per D-02) — v1.3
- ✓ Forge Noir panel: near-black 18HP SVG panel with custom machined-metal knobs, scalloped trimpots, ember-ring jacks, forge emblem, path typography — v1.3
- ✓ Three-column CRT display: pills in left/right margins, ember waveform in center, corner brackets, scanlines, breathing border glow — v1.3
- ✓ Animated SYNC badge: per-edge white-hot flash while LOCKED with exponential decay — v1.3
- ✓ Automated `make test` target with a Rack-independent header-only DSP core (`src/dsp/*.hpp` + `RackCompat.hpp` shims) consumed by the plugin shell; unit tests on the extracted DSP — v1.4 (Phase 22)
- ✓ Headless `BlockDriver` integration harness asserting output invariants (±5V bounds, frequency accuracy, phase continuity at reset, fixed-seed determinism) over sample blocks at 44.1/48/96 kHz, plus a bit-exact golden-output regression and cross-platform GitHub Actions CI — v1.4 (Phase 22)

### Active

**v1.4 Tempered — release hardening (LFO feature-frozen):**
- [ ] Clock tracker recovers from >3× tempo jumps (no permanent lockout)
- [ ] x1.5 / ÷1.5 ratios align without mid-cycle truncation (audition-gated)
- [ ] Phase dot tracks trace in free-running mode with swing set
- [ ] Patch load survives malformed/corrupt JSON without crashing
- [ ] GPL-3.0 LICENSE file at repo root
- [ ] plugin.json URLs populated (author/plugin/source)
- [ ] Trial/proprietary fonts removed from repo and git history
- [ ] Public GitHub source repository published
- [ ] User manual published as Markdown under `docs/`
- [ ] `.vcvplugin` release artifact built and verified for VCV Library submission
- [ ] Display/code cleanups: dead code, unreachable `isStill`, pill fade symmetry, frame-rate-independent animations, display buffer off audio thread

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

**Current state:** v1.3 Forge Noir SHIPPED (2026-06-13). 1,641 lines of C++. 18HP Forge Noir "fresh" panel (Phase 20.1 — redesigned from 14HP, single-row equal secondary knobs, grouped clock section, corner bolts, widget-owned knob art) with custom SVG components, three-column CRT-aesthetic display (Phase 20), per-edge animated SYNC badge flash (Phase 21), three-knob analog engine with clock sync, FM modulation, expanded imperfections, waveform bleed, swing timing, and 5-shape morph sweep (sine-tri-saw-square-pulse). The LFO is feature-complete; the VCO module (v2.0) is the next milestone.
**Tech stack:** VCV Rack 2 SDK, C++17, NanoVG for display, nanosvg for panel.
**Build system:** Standard VCV Rack Makefile with plugin.mk, no external dependencies.
**Brand identity:** Forge Noir — near-black panel (#0c0c0c), ember orange (#e85d26), gold accent (#daa520), warm white text (#e8e4e0). Fonts: Bebas Neue (brand/hero), Chakra Petch (labels), JetBrains Mono (data).
**Prior work:** POC LFO at `/Users/mrcbrown/Claude/Software/Forge Audio/LFO/` — clean digital implementation, no analog modeling.
**Release strategy:** v1.3 Forge Noir is the final LFO milestone — shipped. VCO module (v2.0) is next.
**Known tech debt:** `swingIndex` is a plain int written from the GUI context-menu lambda and read from the audio thread (pre-existing non-atomic write, predates Phase 18; worst case one-frame latency on swing change — common VCV menu-param pattern). Four v1.3 phases (18/19/20.1/21) carry manual-only Nyquist validation (inherently human-gated visual/audio behaviors, no automated harness). Both deferred from v1.3 as non-blockers.

## Constraints

- **Platform:** VCV Rack 2 SDK, C++17, cross-platform (Mac/Windows/Linux)
- **Panel rendering:** SVG via nanosvg — limited subset (no filters, no CSS, text as paths)
- **Real-time:** All DSP in process() callback at sample rate — no allocation, no blocking
- **Display:** NanoVG on FramebufferWidget — must not drop frames
- **Panel size:** 18HP (91.44mm) × 128.5mm height (12HP → 14HP → 18HP across the Forge Noir redesign)
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
| Forge Noir design language | Near-black panel, ember orange accents, machined metal knobs, scalloped trimpots, forge emblem | ✓ Good — cohesive premium identity, shipped v1.3 |
| Panel expansion to 18HP | Forge Noir layout needs breathing room for 5 main knobs + display; 14HP too tight in practice | ✓ Good — 18HP fresh layout resolved all density issues |
| Even 5-shape morph rescale (morph×4), drop v1.2 backward compat (D-02) | Clean 20%-per-shape sweep beats preserving old patch positions for a niche LFO | ✓ Good — smooth sweep; existing patches shift on load (accepted) |
| Widget-owned knob art, strip metal bodies from SVG (D-01) | Eliminates double-rendered knob bodies; SVG keeps only recessed-socket shadows + scallop ticks | ✓ Good — clean single source of knob rendering |
| Promote fresh.svg to production res/AnalogLFO.svg, no plugin.json width (D-03/D-05) | Rack auto-derives 18HP from viewBox; one canonical panel file | ✓ Good — no width drift between art and code |
| SYNC flash via lock-free atomic edge counter, color/glow not alpha (Phase 21 D-01) | Audio thread increments, widget reads; white-hot lerp + bloom reads better than alpha fade | ✓ Good — per-edge flash, zero audio-thread coupling |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd:complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-06-14 — Phase 22 (Test Harness Foundation) complete: Rack-independent DSP core extracted, `make test` + BlockDriver harness + golden regression + CI in place*
