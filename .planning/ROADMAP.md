# Roadmap: Forge Audio Analog Series

## Overview

The Forge Audio Analog Series is a collection of VCV Rack 2 modules featuring analog-modeled oscillators with a three-knob engine (morph, character, drift) and real-time waveform display.

## Milestones

- ✅ **v1.0 Analog Series LFO** -- Phases 1-6 (shipped 2026-03-07)
- ✅ **v1.1 Clock Sync** -- Phases 7-10 (shipped 2026-03-13)
- ✅ **v1.2 Deep Analog** -- Phases 11-17 (shipped 2026-03-17)
- 🚧 **v1.3 Forge Noir** -- Phases 18-21 (in progress)

## Phases

<details>
<summary>✅ v1.0 Analog Series LFO (Phases 1-6) -- SHIPPED 2026-03-07</summary>

- [x] Phase 1: Plugin Scaffold and Panel (2/2 plans) -- completed 2026-02-25
- [x] Phase 2: Waveform Engine (2/2 plans) -- completed 2026-02-25
- [x] Phase 3: Waveform Display (2/2 plans) -- completed 2026-02-26
- [x] Phase 4: Analog Character (2/2 plans) -- completed 2026-03-07
- [x] Phase 5: Drift Engine (2/2 plans) -- completed 2026-03-07
- [x] Phase 6: Polish & Cleanup (2/2 plans) -- completed 2026-03-07

See: `.planning/milestones/v1.0-ROADMAP.md` for full details.

</details>

<details>
<summary>✅ v1.1 Clock Sync (Phases 7-10) -- SHIPPED 2026-03-13</summary>

- [x] Phase 7: Clock Input and Period Tracking (2/2 plans) -- completed 2026-03-07
- [x] Phase 8: Frequency Override and Ratio Table (1/1 plans) -- completed 2026-03-10
- [x] Phase 9: Phase Reset and Drift Integration (1/1 plans) -- completed 2026-03-11
- [x] Phase 10: Display and Panel (2/2 plans) -- completed 2026-03-13

See: `.planning/milestones/v1.1-ROADMAP.md` for full details.

</details>

<details>
<summary>✅ v1.2 Deep Analog (Phases 11-17) -- SHIPPED 2026-03-17</summary>

- [x] Phase 11: Display Polish (1/1 plan) -- completed 2026-03-13
- [x] Phase 12: RESET and Phase Offset (2/2 plans) -- completed 2026-03-15
- [x] Phase 13: FM Input (1/1 plan) -- completed 2026-03-15
- [x] Phase 14: Expanded Imperfections (2/2 plans) -- completed 2026-03-16
- [x] Phase 15: Waveform Bleed (1/1 plan) -- completed 2026-03-17
- [x] Phase 16: Swing and Shuffle (1/1 plan) -- completed 2026-03-17
- [ ] ~~Phase 17: Panel Redesign~~ -- SKIPPED (deferred to modulation routing milestone)

See: `.planning/milestones/v1.2-ROADMAP.md` for full details.

</details>

### 🚧 v1.3 Forge Noir (In Progress)

**Milestone Goal:** Extend morph sweep through pulse/PWM, replace the 12HP panel with the 14HP Forge Noir design language, restructure the display into a three-column layout with CRT aesthetic, and add animated SYNC badge flash.

- [x] **Phase 18: PWM DSP Extension** - Extend morph sweep past square into variable-width pulse with analog character (completed 2026-03-28)
- [x] **Phase 19: Forge Noir Panel + Custom Components** - 14HP Forge Noir SVG panel with custom knobs, jacks, trimpots, and brand elements (completed 2026-04-01)
- [ ] **Phase 20: Display Layout + CRT Aesthetic** - Three-column display with left/right pills, center waveform, CRT scanlines, and border glow
- [ ] **Phase 21: Animated SYNC Badge** - Clock-pulse flash on SYNC badge with exponential decay

## Phase Details

### Phase 18: PWM DSP Extension
**Goal**: Users can sweep the morph knob through a fifth waveform shape -- variable-width pulse -- with full analog character (backward compatibility dropped per D-02)
**Depends on**: Phase 16 (existing morph engine)
**Requirements**: WAVE-01, WAVE-02, WAVE-03, WAVE-04, WAVE-05
**Success Criteria** (what must be TRUE):
  1. Sweeping the Morph knob from 0 to 1 produces Sine, Triangle, Saw, Square, then progressively narrower Pulse -- the full five-shape progression is audible and visible on the display
  2. Existing patches saved with morph at any position (0 to 0.75) produce identical output to v1.2 -- no timbral shift on load
  3. Turning the Character knob while in the pulse region visibly softens the pulse edges (tanh rounding) and varies the duty cycle spread per instance
  4. Sweeping morph through the square-to-pulse boundary and through pulse-to-sine wrap (bleed) produces smooth transitions with no clicks or discontinuities
**Plans**: 1 plan

Plans:
- [x] 18-01-PLAN.md -- Add computePulse() and extend computeMorphedWave() to 5 shapes with bleed ring wrap

### Phase 19: Forge Noir Panel + Custom Components
**Goal**: The module appears in VCV Rack with the 14HP Forge Noir visual identity -- near-black panel, custom machined-metal knobs, scalloped trimpots, accent-ring jacks, forge emblem, and path-rendered brand typography
**Depends on**: Phase 18 (PWM must work before panel locks positions)
**Requirements**: PANEL-01, PANEL-02, PANEL-03, PANEL-04, PANEL-05, PANEL-06, PANEL-07
**Success Criteria** (what must be TRUE):
  1. The module loads in VCV Rack as a 14HP panel with the Forge Noir color scheme (near-black background, ember orange accent bars, warm white labels) -- no black rectangles or missing elements
  2. All controls (5 main knobs, 5 CV trimpots, 5 CV jacks, CLK, RST, output jack) are interactive and positioned per the Forge Noir mockup layout
  3. Three distinct knob sizes are visible (hero MORPH, secondary RATE/CHAR/DRIFT, utility PHASE) with machined-metal appearance, and trimpots are visually distinct scalloped attenuverters
  4. Jacks show two distinct sizes (standard input and larger output) with ember accent rings on the output jack
  5. The forge emblem is visible as a subtle background element and brand text (Forge Audio, Analog LFO) renders correctly as SVG paths
**Plans**: 4 plans
**UI hint**: yes

Plans:
- [x] 19-01-PLAN.md -- Create custom SVG component files (knobs, trimpots, jacks, hex bolt)
- [x] 19-02-PLAN.md -- Create 14HP Forge Noir panel SVG with all decorative elements and typography
- [x] 19-03-PLAN.md -- Wire custom widgets into C++ codebase, update plugin.json and PANEL-SPEC.md
- [ ] 19-04-PLAN.md -- Install and visual verification checkpoint in VCV Rack

### Phase 20: Display Layout + CRT Aesthetic
**Goal**: The waveform display uses a structured three-column layout with data readouts in dedicated margins, CRT-inspired visual treatment, and the Forge Noir color palette
**Depends on**: Phase 19 (display box position and size depend on 14HP panel layout)
**Requirements**: DISP-01, DISP-02, DISP-03, DISP-04, DISP-05
**Success Criteria** (what must be TRUE):
  1. Data pills (ratio, Hz, swing, SYNC, BPM) appear in dedicated left and right columns -- they never overlap the waveform trace in the center column
  2. The waveform trace is confined to roughly the center 60% of the display width, with consistent margins on both sides
  3. Corner bracket decorations are visible at all four corners of the display border
  4. Faint horizontal scanline overlay is visible across the display, giving a CRT monitor aesthetic
  5. The display border pulses with a subtle breathing glow animation that is visible when watching the module idle
**Plans**: 3 plans
**UI hint**: yes

Plans:
- [x] 20-01-PLAN.md -- Download JetBrains Mono NL font, overhaul display container (background, border, brackets, zero-crossing)
- [ ] 20-02-PLAN.md -- Three-column layout with phaseToX constraint, pill restyling, waveform/dot color update
- [ ] 20-03-PLAN.md -- CRT scanline overlay with image pattern + visual verification checkpoint

### Phase 21: Animated SYNC Badge
**Goal**: The SYNC badge visually pulses on each incoming clock edge, giving immediate feedback that the module is receiving and responding to clock signals
**Depends on**: Phase 20 (SYNC badge must be in its final three-column position)
**Requirements**: ANIM-01, ANIM-02
**Success Criteria** (what must be TRUE):
  1. Each clock edge triggers a visible bright flash on the SYNC badge that is distinct from the steady-state badge appearance
  2. The flash decays smoothly back to the normal SYNC badge brightness over roughly 200ms with no abrupt transitions
**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 18 -> 19 -> 20 -> 21

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
| 11. Display Polish | v1.2 | 1/1 | Complete | 2026-03-13 |
| 12. RESET and Phase Offset | v1.2 | 2/2 | Complete | 2026-03-15 |
| 13. FM Input | v1.2 | 1/1 | Complete | 2026-03-15 |
| 14. Expanded Imperfections | v1.2 | 2/2 | Complete | 2026-03-16 |
| 15. Waveform Bleed | v1.2 | 1/1 | Complete | 2026-03-17 |
| 16. Swing and Shuffle | v1.2 | 1/1 | Complete | 2026-03-17 |
| 17. Panel Redesign | v1.2 | - | Skipped | 2026-03-17 |
| 18. PWM DSP Extension | v1.3 | 1/1 | Complete    | 2026-03-28 |
| 19. Forge Noir Panel + Custom Components | v1.3 | 3/4 | Complete    | 2026-04-01 |
| 20. Display Layout + CRT Aesthetic | v1.3 | 1/3 | In Progress|  |
| 21. Animated SYNC Badge | v1.3 | 0/0 | Not started | - |
