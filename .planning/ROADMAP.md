# Roadmap: Forge Audio Analog Series

## Overview

The Forge Audio Analog Series is a collection of VCV Rack 2 modules featuring analog-modeled oscillators with a three-knob engine (morph, character, drift) and real-time waveform display.

## Milestones

- ✅ **v1.0 Analog Series LFO** -- Phases 1-6 (shipped 2026-03-07)
- ✅ **v1.1 Clock Sync** -- Phases 7-10 (shipped 2026-03-13)
- **v1.2 Deep Analog** -- Phases 11-17 (in progress)

## Phases

<details>
<summary>v1.0 Analog Series LFO (Phases 1-6) -- SHIPPED 2026-03-07</summary>

- [x] Phase 1: Plugin Scaffold and Panel (2/2 plans) -- completed 2026-02-25
- [x] Phase 2: Waveform Engine (2/2 plans) -- completed 2026-02-25
- [x] Phase 3: Waveform Display (2/2 plans) -- completed 2026-02-26
- [x] Phase 4: Analog Character (2/2 plans) -- completed 2026-03-07
- [x] Phase 5: Drift Engine (2/2 plans) -- completed 2026-03-07
- [x] Phase 6: Polish & Cleanup (2/2 plans) -- completed 2026-03-07

See: `.planning/milestones/v1.0-ROADMAP.md` for full details.

</details>

<details>
<summary>v1.1 Clock Sync (Phases 7-10) -- SHIPPED 2026-03-13</summary>

- [x] Phase 7: Clock Input and Period Tracking (2/2 plans) -- completed 2026-03-07
- [x] Phase 8: Frequency Override and Ratio Table (1/1 plans) -- completed 2026-03-10
- [x] Phase 9: Phase Reset and Drift Integration (1/1 plans) -- completed 2026-03-11
- [x] Phase 10: Display and Panel (2/2 plans) -- completed 2026-03-13

See: `.planning/milestones/v1.1-ROADMAP.md` for full details.

</details>

### v1.2 Deep Analog (Phases 11-17)

**Milestone Goal:** Deepen the LFO's analog character with FM modulation, expanded drift/imperfection modeling, new modulation jacks, groove features, and display polish.

- [ ] **Phase 11: Display Polish** - Fix HUD text readability and add incoming clock BPM display
- [ ] **Phase 12: RESET and Phase Offset** - Independent reset jack and CV-modulatable phase shift
- [ ] **Phase 13: FM Input** - Exponential frequency modulation with clocked-mode authority management
- [ ] **Phase 14: Expanded Imperfections** - Phase jitter, DC offset drift, pitch slew, component spread
- [ ] **Phase 15: Waveform Bleed** - Adjacent-shape crosstalk during morph transitions
- [ ] **Phase 16: Swing and Shuffle** - Beat-pair phase warping for clocked groove
- [ ] **Phase 17: Panel Redesign** - Final 12HP layout for all new jacks and controls

## Phase Details

### Phase 11: Display Polish
**Goal**: Display overlays are always readable and show complete clock status information
**Depends on**: Phase 10 (v1.1 display foundation)
**Requirements**: DISP-01, DISP-02
**Success Criteria** (what must be TRUE):
  1. SYNC badge, ratio label, BPM, and Hz text remain readable when the waveform trace passes directly through overlay areas at any morph/character/drift setting
  2. When clocked at a non-x1 ratio, the display shows both the raw incoming clock BPM and the ratio-adjusted effective BPM with clear visual distinction between them
  3. When clocked at x1 ratio, only one BPM value is displayed (no redundant duplicate)
**Plans**: TBD

### Phase 12: RESET and Phase Offset
**Goal**: Users can independently reset LFO phase and shift waveform output by a controllable offset
**Depends on**: Phase 11
**Requirements**: MOD-03, MOD-04, PHASE-01, PHASE-02
**Success Criteria** (what must be TRUE):
  1. A rising-edge trigger at the RESET jack snaps the LFO to its start position with a click-free crossfade, independent of clock state
  2. Simultaneous triggers on CLK and RESET within 1ms do not cause double-resets or corrupt clock period tracking
  3. The Phase Offset knob shifts the waveform output 0-360 degrees, and the waveform display dot tracks the offset-inclusive position
  4. Phase Offset CV input modulates offset in real time, enabling quadrature (90-degree) patches when driven by a constant voltage
  5. Changing the Phase Offset knob while running produces smooth waveform changes with no clicks or discontinuities
**Plans**: TBD

### Phase 13: FM Input
**Goal**: Users can frequency-modulate the LFO from external CV sources in both free and clocked modes
**Depends on**: Phase 12
**Requirements**: MOD-01, MOD-02
**Success Criteria** (what must be TRUE):
  1. Patching a bipolar CV source into the FM jack modulates LFO frequency exponentially, with an attenuator controlling modulation depth
  2. FM never drives frequency negative -- LFO remains stable at any FM depth and input voltage
  3. In clocked mode, FM is usable without destroying clock sync -- authority is reduced so clock-edge phase resets remain clean at moderate FM depths
  4. With FM attenuator at zero (default), output is identical to v1.1 behavior
**Plans**: TBD

### Phase 14: Expanded Imperfections
**Goal**: The Drift knob controls a richer set of analog imperfections beyond pitch drift alone
**Depends on**: Phase 12 (phase readout path must be established)
**Requirements**: CHAR-01, CHAR-02, CHAR-03, CHAR-04
**Success Criteria** (what must be TRUE):
  1. With Drift above zero, waveform timing shows subtle per-sample jitter visible as slight trace thickness variation on the display
  2. With Drift above zero, the output center drifts slowly away from 0V (DC offset wander), observable on a scope module
  3. Rapid frequency changes (via Rate knob or CV) show slight lag when Drift is up, simulating component thermal response
  4. Two instances of the module with identical knob positions produce audibly different output character due to per-instance component spread
  5. At Drift = 0, all new imperfections are completely inactive -- output matches pre-v1.2 digital precision
**Plans**: TBD

### Phase 15: Waveform Bleed
**Goal**: Morph transitions show analog crossfader crosstalk influenced by the Character knob
**Depends on**: Phase 14 (pipeline changes to computeMorphedWave must follow imperfections)
**Requirements**: CHAR-05
**Success Criteria** (what must be TRUE):
  1. With Character above zero and Morph at intermediate positions, the waveform trace shows subtle influence from adjacent shapes (visible on display)
  2. At Character = 0, morph is a crisp crossfade with no bleed -- identical to v1.1 behavior
  3. Output stays within the +/-5V range at all morph/character/bleed combinations -- no amplitude spikes
**Plans**: TBD

### Phase 16: Swing and Shuffle
**Goal**: Clocked LFO output can be grooved with swing timing for rhythmic modulation patches
**Depends on**: Phase 13 (FM and phase features must be stable before swing layers on)
**Requirements**: PHASE-03, PHASE-04
**Success Criteria** (what must be TRUE):
  1. In clocked mode, a swing setting above 50% audibly shifts alternate beat timing -- the LFO "grooves" rather than playing straight
  2. Swing is adjustable via right-click menu with musically useful presets (50% through 75%)
  3. In free-running mode, swing has no effect on output regardless of its setting
  4. At swing = 50% (default), clocked output is identical to v1.1 behavior
**Plans**: TBD

### Phase 17: Panel Redesign
**Goal**: The panel accommodates all v1.2 components in a clean, cohesive 12HP layout
**Depends on**: Phase 16 (all DSP features finalized before panel design)
**Requirements**: PANEL-01, PANEL-02
**Success Criteria** (what must be TRUE):
  1. Panel SVG includes FM jack, RESET jack, and Phase Offset CV jack with appropriate labels
  2. Phase Offset knob and FM attenuator trimpot are positioned following Eurorack layout conventions (trimpots above jacks, logical grouping)
  3. All jack and control spacing meets minimum 7mm center-to-center, and the layout works at 12HP (or a documented fallback has been applied)
  4. Panel is structured for designer handoff with consistent layer naming and grouped elements
**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 11 -> 12 -> 13 -> 14 -> 15 -> 16 -> 17

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Plugin Scaffold and Panel | v1.0 | 2/2 | Complete | 2026-02-25 |
| 2. Waveform Engine | v1.0 | 2/2 | Complete | 2026-02-25 |
| 3. Waveform Display | v1.0 | 2/2 | Complete | 2026-02-26 |
| 4. Analog Character | v1.0 | 2/2 | Complete | 2026-03-07 |
| 5. Drift Engine | v1.0 | 2/2 | Complete | 2026-03-07 |
| 6. Polish & Cleanup | v1.0 | 2/2 | Complete | 2026-03-07 |
| 7. Clock Input and Period Tracking | v1.1 | 2/2 | Complete | 2026-03-07 |
| 8. Frequency Override and Ratio Table | v1.1 | 1/1 | Complete | 2026-03-10 |
| 9. Phase Reset and Drift Integration | v1.1 | 1/1 | Complete | 2026-03-11 |
| 10. Display and Panel | v1.1 | 2/2 | Complete | 2026-03-13 |
| 11. Display Polish | v1.2 | 0/? | Not started | - |
| 12. RESET and Phase Offset | v1.2 | 0/? | Not started | - |
| 13. FM Input | v1.2 | 0/? | Not started | - |
| 14. Expanded Imperfections | v1.2 | 0/? | Not started | - |
| 15. Waveform Bleed | v1.2 | 0/? | Not started | - |
| 16. Swing and Shuffle | v1.2 | 0/? | Not started | - |
| 17. Panel Redesign | v1.2 | 0/? | Not started | - |
