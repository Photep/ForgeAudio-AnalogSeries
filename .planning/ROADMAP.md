# Roadmap: Forge Audio Analog Series (LFO)

## Overview

This roadmap delivers the Forge Audio Analog Series LFO module for VCV Rack 2 -- a sub-audio oscillator built around a three-knob analog engine (morph, character, drift) with real-time waveform display. The build follows a strict dependency chain: plugin scaffold and panel first, then the morph waveform engine, then visual feedback via the display, then precise pitch controls, then analog character modeling against specific classic synth references, and finally the drift engine that brings the module to life with authentic analog imperfections. Each phase delivers a verifiable capability that the next phase builds on.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Plugin Scaffold and Panel** - Buildable VCV Rack 2 plugin with branded SVG panel and module registration
- [x] **Phase 2: Waveform Engine** - Four-shape morph oscillator with rate control and bipolar output
- [x] **Phase 3: Waveform Display** - Real-time single-cycle waveform visualization with phase-tracking dot (completed 2026-02-26)
- [ ] **Phase 4: Pitch Controls** - Octave snap and semitone selector for precise LFO rate tuning
- [ ] **Phase 5: Analog Character** - Character knob crossfading from digital perfection to classic synth references per shape
- [ ] **Phase 6: Drift Engine** - Layered analog imperfections via Ornstein-Uhlenbeck pitch drift

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
  4. Module produces bipolar +/-5V output and inverted output simultaneously from the output jacks
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
- [ ] 03-01-PLAN.md -- Lock-free double buffer and WaveformDisplay widget with NanoVG rendering (DISP-01, DISP-02, DISP-03, DISP-04, DISP-05)
- [ ] 03-02-PLAN.md -- Visual verification of display in VCV Rack (DISP-01, DISP-02, DISP-04)

### Phase 4: Pitch Controls
**Goal**: Users can precisely set LFO rate using octave snap and semitone selection
**Depends on**: Phase 2
**Requirements**: PTCH-02, PTCH-03
**Success Criteria** (what must be TRUE):
  1. Octave snap knob quantizes rate in octave steps across a +/-4 octave range from the base rate
  2. Semitone selector with up/down buttons adjusts pitch in semitone increments, with a small digital display showing the current semitone value
**Plans**: TBD

Plans:
- [ ] 04-01: TBD

### Phase 5: Analog Character
**Goal**: Users can dial in authentic vintage analog tone per waveform shape using the character knob
**Depends on**: Phase 2
**Requirements**: CHAR-01, CHAR-02, CHAR-03, CHAR-04, CHAR-05, CHAR-06, CHAR-07
**Success Criteria** (what must be TRUE):
  1. Character knob at zero produces mathematically perfect digital waveforms; at full produces recognizable analog character for each shape
  2. Saw reference exhibits Minimoog-style exponential ramp curvature and soft capacitor reset behavior
  3. Square reference exhibits Roland SH-101/Juno-106-style sigmoid edge softening and duty cycle asymmetry
  4. Triangle reference exhibits Moog/Prophet-style rounded peaks and slope asymmetry
  5. Sine reference exhibits triangle-derived analog sine characteristics with audible residual harmonic distortion
**Plans**: TBD

Plans:
- [ ] 05-01: TBD
- [ ] 05-02: TBD

### Phase 6: Drift Engine
**Goal**: Users can add authentic analog instability that makes the oscillator sound alive and unique
**Depends on**: Phase 5
**Requirements**: DRFT-01, DRFT-02
**Success Criteria** (what must be TRUE):
  1. Drift knob at zero produces perfectly stable output; increasing drift introduces progressively more pitch variation with audible multi-timescale movement (slow wander plus faster jitter)
  2. Drift CV input modulates drift amount when patched from an external source
  3. The complete three-knob engine works together: morph selects shape, character adds analog tone, drift adds instability, and the display reflects all three in real time
**Plans**: TBD

Plans:
- [ ] 06-01: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5 -> 6
Note: Phases 3, 4, and 5 all depend on Phase 2 but not on each other.

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Plugin Scaffold and Panel | 2/2 | Complete    | 2026-02-25 |
| 2. Waveform Engine | 2/2 | Complete    | 2026-02-25 |
| 3. Waveform Display | 0/TBD | Complete    | 2026-02-26 |
| 4. Pitch Controls | 0/TBD | Not started | - |
| 5. Analog Character | 0/TBD | Not started | - |
| 6. Drift Engine | 0/TBD | Not started | - |
