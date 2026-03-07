# Roadmap: Forge Audio Analog Series (LFO)

## Overview

This roadmap delivers the Forge Audio Analog Series LFO module for VCV Rack 2 -- a sub-audio oscillator built around a three-knob analog engine (morph, character, drift) with real-time waveform display. The build follows a strict dependency chain: plugin scaffold and panel first, then the morph waveform engine, then visual feedback via the display, then analog character modeling against specific classic synth references, and finally the drift engine that brings the module to life with authentic analog imperfections. Each phase delivers a verifiable capability that the next phase builds on.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Plugin Scaffold and Panel** - Buildable VCV Rack 2 plugin with branded SVG panel and module registration
- [x] **Phase 2: Waveform Engine** - Four-shape morph oscillator with rate control and bipolar output
- [x] **Phase 3: Waveform Display** - Real-time single-cycle waveform visualization with phase-tracking dot (completed 2026-02-26)
- [x] **Phase 4: Analog Character** - Character knob crossfading from digital perfection to classic synth references per shape
- [x] **Phase 5: Drift Engine** - Layered analog imperfections via Ornstein-Uhlenbeck pitch drift
- [ ] **Phase 6: Polish & Cleanup** - Tech debt closure: documentation fixes, drift visual tuning, panel refinement

## Phase Details

### Phase 1: Plugin Scaffold and Panel
**Goal**: Users can install and load a visible, branded LFO module in VCV Rack that builds cross-platform
**Depends on**: Nothing (first phase)
**Requirements**: INFR-01, PANL-01, PANL-02
**Success Criteria** (what must be TRUE):
  1. Plugin compiles on macOS with `make` and loads in VCV Rack 2 without errors
  2. Module appears in VCV Rack's module browser under "Forge Audio" brand
  3. Panel displays at correct HP width with Forge Audio brand identity (navy background, amber accents, lavender labels)
  4. SVG panel file is structured with documented coordinates and nanosvg-compatible elements ready for designer handoff
**Plans**: 2 plans

Plans:
- [x] 01-01-PLAN.md -- Plugin scaffold and branded SVG panel (INFR-01, PANL-01)
- [x] 01-02-PLAN.md -- Designer handoff documentation and visual verification (PANL-02)

### Phase 2: Waveform Engine
**Goal**: Users can generate sound and continuously sweep through four waveform shapes with a morph knob
**Depends on**: Phase 1
**Requirements**: WAVE-01, WAVE-02, WAVE-03, OUT-01, OUT-02, PTCH-01
**Success Criteria** (what must be TRUE):
  1. Morph knob continuously sweeps Sine to Triangle to Saw to Square with musically valid intermediate shapes (no clicks or discontinuities during sweep)
  2. Rate knob controls LFO frequency across the full sub-audio range (approximately 0.01Hz to 20Hz)
  3. Morph CV input modulates morph position when patched from an external source
  4. Module produces bipolar +/-5V output from a single output jack (inverted output removed by design decision)
  5. Phase accumulator uses double precision (verified: no LFO stall below 0.01Hz)
**Plans**: 2 plans

Plans:
- [x] 02-01-PLAN.md -- Panel restructuring: remove INV output, add Morph CV attenuator, linear rate knob (OUT-02, PTCH-01, WAVE-03)
- [x] 02-02-PLAN.md -- DSP engine: phase accumulator, waveform generation, morph interpolation, CV processing, output (WAVE-01, WAVE-02, OUT-01)

### Phase 3: Waveform Display
**Goal**: Users see exactly what the oscillator is outputting in real time with a phase-tracking indicator
**Depends on**: Phase 2
**Requirements**: DISP-01, DISP-02, DISP-03, DISP-04, DISP-05
**Success Criteria** (what must be TRUE):
  1. Single-cycle waveform trace renders in the upper portion of the module panel, updating in real time as morph knob moves
  2. Bright amber dot tracks the current phase position along the waveform trace, visibly moving at LFO rate
  3. Display renders with amber trace on dark navy background matching Forge Audio brand identity
  4. Display uses lock-free double buffer -- no audio dropouts or clicks when display is visible and animating
**Plans**: 2 plans

Plans:
- [x] 03-01-PLAN.md -- Lock-free double buffer and WaveformDisplay widget with NanoVG rendering (DISP-01, DISP-02, DISP-03, DISP-04, DISP-05)
- [x] 03-02-PLAN.md -- Visual verification of display in VCV Rack (DISP-01, DISP-02, DISP-04)

### Phase 4: Analog Character
**Goal**: Users can dial in authentic vintage analog tone per waveform shape using the character knob
**Depends on**: Phase 2
**Requirements**: CHAR-01, CHAR-02, CHAR-03, CHAR-04, CHAR-05, CHAR-06, CHAR-07
**Success Criteria** (what must be TRUE):
  1. Character knob at zero produces mathematically perfect digital waveforms; at full produces recognizable analog character for each shape
  2. Saw reference exhibits Minimoog-style exponential ramp curvature and soft capacitor reset behavior
  3. Square reference exhibits Roland SH-101/Juno-106-style sigmoid edge softening and duty cycle asymmetry
  4. Triangle reference exhibits Moog/Prophet-style rounded peaks and slope asymmetry
  5. Sine reference exhibits triangle-derived analog sine characteristics with audible residual harmonic distortion
**Plans**: 2 plans

Plans:
- [x] 04-01-PLAN.md -- Character DSP engine: per-shape analog models, CV input, panel updates (CHAR-01, CHAR-02, CHAR-03, CHAR-04, CHAR-05, CHAR-06, CHAR-07)
- [x] 04-02-PLAN.md -- Visual verification of character in VCV Rack (CHAR-01, CHAR-02, CHAR-03, CHAR-04, CHAR-05, CHAR-07)

### Phase 5: Drift Engine
**Goal**: Users can add authentic analog instability that makes the oscillator sound alive and unique
**Depends on**: Phase 4
**Requirements**: DRFT-01, DRFT-02
**Success Criteria** (what must be TRUE):
  1. Drift knob at zero produces perfectly stable output; increasing drift introduces progressively more pitch variation with audible multi-timescale movement (slow wander plus faster jitter)
  2. Drift CV input modulates drift amount when patched from an external source
  3. The complete three-knob engine works together: morph selects shape, character adds analog tone, drift adds instability, and the display reflects all three in real time
**Plans**: 2 plans

Plans:
- [x] 05-01-PLAN.md -- Drift DSP engine: OU process, CV input, panel layout update, dot instability (DRFT-01, DRFT-02)
- [x] 05-02-PLAN.md -- Visual verification of drift and complete three-knob engine in VCV Rack (DRFT-01, DRFT-02)

### Phase 6: Polish & Cleanup
**Goal**: Close tech debt from milestone audit — fix stale documentation, tune drift visuals, refine panel design language
**Depends on**: Phase 5
**Requirements**: None (tech debt closure — all requirements already satisfied)
**Gap Closure**: Closes 4 tech debt items from v1.0 audit
**Success Criteria** (what must be TRUE):
  1. OUT-02 description in REQUIREMENTS.md accurately reflects the design decision (INV output removed)
  2. Drift dot instability visual is perceptible at high drift levels (trail jitter and halo variation)
  3. Dot instability visual responds to CV-modulated drift value, not just knob position
  4. Bottom row panel layout uses consistent design language
**Plans**: 2 plans

Plans:
- [ ] 06-01-PLAN.md -- Drift visual tuning, bottom row restructure, SVG panel update, documentation fixes
- [ ] 06-02-PLAN.md -- Visual verification of all Phase 6 changes in VCV Rack

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5 -> 6
Note: Phases 3 and 4 both depend on Phase 2 but not on each other.

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Plugin Scaffold and Panel | 2/2 | Complete    | 2026-02-25 |
| 2. Waveform Engine | 2/2 | Complete    | 2026-02-25 |
| 3. Waveform Display | 2/2 | Complete    | 2026-02-26 |
| 4. Analog Character | 2/2 | Complete | 2026-03-07 |
| 5. Drift Engine | 2/2 | Complete | 2026-03-07 |
| 6. Polish & Cleanup | 0/2 | Planned | -- |
