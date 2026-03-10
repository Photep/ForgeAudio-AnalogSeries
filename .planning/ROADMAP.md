# Roadmap: Forge Audio Analog Series

## Overview

The Forge Audio Analog Series is a collection of VCV Rack 2 modules featuring analog-modeled oscillators with a three-knob engine (morph, character, drift) and real-time waveform display.

## Milestones

- ✅ **v1.0 Analog Series LFO** -- Phases 1-6 (shipped 2026-03-07)
- **v1.1 Clock Sync** -- Phases 7-10 (in progress)

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

### v1.1 Clock Sync

**Milestone Goal:** Add clock-synced operation to the LFO -- a CLK input that tracks incoming tempo and converts the Rate knob into a clock divider/multiplier, fully backward compatible.

- [x] **Phase 7: Clock Input and Period Tracking** - Detect clock edges, measure period, handle loss/outliers (completed 2026-03-07)
- [ ] **Phase 8: Frequency Override and Ratio Table** - Dual-mode Rate knob with 15 snapped musical ratios
- [ ] **Phase 9: Phase Reset and Drift Integration** - Beat-aligned phase reset with anti-click crossfade and drift scaling
- [ ] **Phase 10: Display and Panel** - Sync badge, ratio label, and CLK jack on panel SVG

## Phase Details

### Phase 7: Clock Input and Period Tracking
**Goal**: The LFO reliably tracks incoming clock tempo from any VCV Rack clock source
**Depends on**: Phase 6 (v1.0 complete)
**Requirements**: CLK-01, CLK-02, CLK-03, CLK-04, CLK-05, CLK-06
**Success Criteria** (what must be TRUE):
  1. Patching a clock source into the CLK jack causes the module to enter clocked mode; unplugging reverts to free-running
  2. The module measures stable clock period after receiving two or more edges, even with jittery sources
  3. If the clock signal stops, the module automatically reverts to free-running mode within a few seconds
  4. Sudden tempo jumps (e.g., cable hot-swap) do not cause wild frequency spikes -- outliers are filtered
  5. The first clock pulse resets phase but does not attempt to set frequency (waits for second edge)
**Plans**: 2 plans

Plans:
- [x] 07-01-PLAN.md — Implement CLK input, clock state machine, and period tracking DSP
- [x] 07-02-PLAN.md — Verify clock tracking in VCV Rack with real clock source

### Phase 8: Frequency Override and Ratio Table
**Goal**: The Rate knob seamlessly switches between free-running Hz and musical clock divisions/multiplications
**Depends on**: Phase 7
**Requirements**: RATE-01, RATE-02, RATE-03, RATE-06
**Success Criteria** (what must be TRUE):
  1. With no CLK cable connected, the Rate knob behaves identically to v1.0 (continuous Hz, same range)
  2. With CLK connected, the Rate knob snaps to 15 discrete musical ratios from /16 to x16
  3. Sweeping the Rate knob while clocked steps through ratios with no intermediate values -- the LFO frequency jumps cleanly between divisions
  4. Hovering over the Rate knob shows the current ratio label (e.g., "x4 (synced)") in the tooltip when clocked
**Plans**: 1 plan

Plans:
- [x] 08-01-PLAN.md — Implement ratio table, frequency override, and custom RateParamQuantity tooltip

### Phase 9: Phase Reset and Drift Integration
**Goal**: The LFO stays locked to the beat with inaudible phase resets and musically appropriate drift behavior
**Depends on**: Phase 8
**Requirements**: RATE-04, RATE-05, DISP-04, DISP-05
**Success Criteria** (what must be TRUE):
  1. The LFO waveform resets to the start of its cycle on each clock edge (or every Nth edge for /N ratios), maintaining beat alignment
  2. Phase resets produce no audible clicks when the LFO modulates a filter cutoff or VCA -- the crossfade is transparent
  3. With the Drift knob turned up in clocked mode, the LFO has subtle analog wobble but does not accumulate enough phase error to cause clicks at the next reset
  4. Connecting or disconnecting the CLK cable produces a smooth frequency transition with no audible jump
**Plans**: 1 plan

Plans:
- [ ] 09-01-PLAN.md — Implement division-aware phase reset, anti-click crossfade, drift scaling, and frequency slew

### Phase 10: Display and Panel
**Goal**: The user can see at a glance whether the LFO is clock-synced and at what ratio
**Depends on**: Phase 9
**Requirements**: DISP-01, DISP-02, DISP-03, DISP-06
**Success Criteria** (what must be TRUE):
  1. The panel SVG shows a CLK jack with label, and patching into it works correctly in VCV Rack
  2. The waveform display shows a "SYNC" badge when a clock is connected and tracking
  3. The display shows the current division/multiplication ratio label (e.g., "/4", "x2") updating in real time as the Rate knob is turned
  4. The display shows the BPM calculated from the clock source and the currently selected rate divider
**Plans**: TBD

Plans:
- [ ] 10-01: TBD
- [ ] 10-02: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 7 -> 8 -> 9 -> 10

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
| 9. Phase Reset and Drift Integration | v1.1 | 0/1 | Not started | - |
| 10. Display and Panel | v1.1 | 0/? | Not started | - |
