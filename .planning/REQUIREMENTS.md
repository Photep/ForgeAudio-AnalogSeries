# Requirements: Forge Audio Analog Series

**Defined:** 2026-03-13
**Core Value:** The three-knob analog engine (morph, character, drift) that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback.

## v1.2 Requirements

Requirements for v1.2 Deep Analog milestone. Each maps to roadmap phases.

### Display

- [x] **DISP-01**: Display text overlays (SYNC badge, ratio, BPM, Hz) readable over waveform in all brightness/color scenarios via soft-edged pill backgrounds
- [x] **DISP-02**: Incoming clock BPM displayed alongside effective (ratio-adjusted) BPM when clocked

### Modulation

- [x] **MOD-01**: FM input jack applies exponential frequency modulation with CV-controllable depth
- [x] **MOD-02**: FM authority reduced in clocked mode to prevent clock-phase fighting
- [x] **MOD-03**: Separate RESET trigger jack resets phase independently from CLK, with 1ms blanking to prevent double-resets
- [x] **MOD-04**: RESET uses existing anti-click cosine crossfade

### Phase

- [x] **PHASE-01**: Phase offset knob shifts waveform phase 0-360 degrees, applied at waveform readout (not accumulator)
- [x] **PHASE-02**: Phase offset CV input for external modulation
- [x] **PHASE-03**: Swing/shuffle control warps phase progression within beat pairs in clocked mode
- [x] **PHASE-04**: Swing inactive in free-running mode

### Analog Character

- [x] **CHAR-01**: Phase jitter adds per-sample random phase deviation scaled by Drift knob
- [x] **CHAR-02**: DC offset drift adds slow-wandering output bias scaled by Drift knob
- [x] **CHAR-03**: Pitch slew adds frequency lag (component thermal response) scaled by Drift knob
- [x] **CHAR-04**: Component spread applies per-instance random parameter offsets (serialized via RNG seed)
- [x] **CHAR-05**: Waveform bleed introduces adjacent-shape crosstalk during morph transitions

### Panel

- [ ] **PANEL-01**: Panel SVG updated with all new jacks and controls in a cohesive layout — deferred to modulation routing milestone
- [ ] **PANEL-02**: Panel accommodates FM jack, RESET jack, phase offset knob/CV at minimum; swing control placement determined (panel or right-click menu) — deferred to modulation routing milestone

## Future Requirements

Deferred to future milestones. Tracked but not in current roadmap.

### Modulation

- **MOD-05**: CV control of division ratio
- **MOD-06**: Animated sync badge (clock-pulse flash)

### VCO Module (v2.0)

- **VCO-01**: V/Oct pitch input with 1V/octave tracking
- **VCO-02**: FM input and through-zero FM
- **VCO-03**: Hard sync input
- **VCO-04**: Morph-aware polyBLEP antialiasing
- **VCO-05**: Phase distortion
- **VCO-06**: Tracking error modeling (right-click toggle)
- **VCO-07**: Coarse/fine tune controls
- **VCO-08**: Oversampling option (Off/2x/4x)

## Out of Scope

| Feature | Reason |
|---------|--------|
| Linear FM mode | Perceptually identical to exponential at LFO rates (0.01-20Hz); linear FM belongs in VCO module |
| Through-zero FM | Audio-rate timbral effect, not meaningful at sub-audio LFO rates |
| Individual imperfection knobs | One Drift knob with curated proportions is the design concept |
| Polyphonic operation | 16x CPU cost, complicates drift and display |
| Panel expansion beyond 12HP | Preserve existing rack footprint; use right-click menu if controls don't fit |

## Traceability

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| DISP-01 | Phase 11 | Complete |
| DISP-02 | Phase 11 | Complete |
| MOD-01 | Phase 13 | Complete |
| MOD-02 | Phase 13 | Complete |
| MOD-03 | Phase 12 | Complete |
| MOD-04 | Phase 12 | Complete |
| PHASE-01 | Phase 12 | Complete |
| PHASE-02 | Phase 12 | Complete |
| PHASE-03 | Phase 16 | Complete |
| PHASE-04 | Phase 16 | Complete |
| CHAR-01 | Phase 14 | Complete |
| CHAR-02 | Phase 14 | Complete |
| CHAR-03 | Phase 14 | Complete |
| CHAR-04 | Phase 14 | Complete |
| CHAR-05 | Phase 15 | Complete |
| PANEL-01 | Phase 17 | Deferred |
| PANEL-02 | Phase 17 | Deferred |

**Coverage:**
- v1.2 requirements: 17 total
- Mapped to phases: 17
- Unmapped: 0

---
*Requirements defined: 2026-03-13*
*Last updated: 2026-03-13 after roadmap creation*
