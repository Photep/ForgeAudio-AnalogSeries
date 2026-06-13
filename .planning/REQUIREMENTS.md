# Requirements: Forge Audio Analog Series

**Defined:** 2026-03-28
**Core Value:** The three-knob analog engine (morph, character, drift) that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback.

## v1.3 Requirements

Requirements for Forge Noir milestone. Each maps to roadmap phases.

### Waveform Engine

- [x] **WAVE-01**: Morph sweep extends past square into variable-width pulse (Sine -> Tri -> Saw -> Square -> Pulse)
- [x] **WAVE-02**: Pulse duty cycle ranges from 50% (square) to 5% (narrow pulse) across morph range
- [x] **WAVE-03**: Character knob applies analog deformation to pulse shape (tanh edge softening, component spread)
- [x] **WAVE-04**: Waveform bleed ring wraps to 5 shapes (pulse neighbors sine)
- [x] **WAVE-05**: Existing morph positions preserved for backward compatibility ([0, 0.75] = original 4 shapes)

### Panel Design

- [x] **PANEL-01**: 14HP Forge Noir SVG panel with near-black background, ember accent bars, hex bolt screws
- [x] **PANEL-02**: All controls repositioned per Forge Noir mockup (5 main knobs, 5 CV trimpots+jacks, CLK/RST/OUTPUT)
- [x] **PANEL-03**: Custom SVG knob components (3 sizes: hero/secondary/utility) with machined metal appearance
- [x] **PANEL-04**: Custom SVG trimpot components (bright scalloped attenuverters)
- [x] **PANEL-05**: Custom SVG jack components (2 sizes: standard/output with ember accent ring)
- [x] **PANEL-06**: Forge emblem background element
- [x] **PANEL-07**: Brand typography rendered as SVG paths (Forge Audio header, Analog LFO name)
- [x] **PANEL-08**: Module renders at 18HP (91.44mm) using the new `res/AnalogLFO-fresh.svg` panel artwork
- [x] **PANEL-09**: MORPH hero knob + four equal-sized secondary knobs (CHARACTER/DRIFT/RATE/PHASE) repositioned to the new single-row layout
- [x] **PANEL-10**: Trimpot attenuverters, CV input jacks, and CLK/RST/OUTPUT repositioned to the new grouped clock-box layout
- [x] **PANEL-11**: Waveform display repositioned/resized (81.44×26.00mm) with internal three-column rendering re-tuned, no Phase 20 regressions
- [x] **PANEL-12**: All controls align exactly with artwork anchors at 100%/200% zoom; existing functionality unchanged

### Display

- [x] **DISP-01**: Three-column display layout (left pills, center waveform, right pills)
- [x] **DISP-02**: Waveform rendering constrained to center ~60% of display width
- [x] **DISP-03**: Corner bracket decorations on display border
- [x] **DISP-04**: CRT scanline aesthetic overlay
- [x] **DISP-05**: Breathing display border glow animation

### Animation

- [x] **ANIM-01**: SYNC badge flashes on each clock edge with bright pulse
- [x] **ANIM-02**: Flash uses exponential decay (~0.92x per frame) back to steady state

## Future Requirements

Deferred to VCO module (v2.0).

### VCO Module

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
| Surge-style modulation routing | Abandoned -- over-engineered for LFO; direct CV jacks sufficient |
| Separate PWM knob/CV | PWM integrated into morph sweep -- no new controls needed |
| Individual waveform outputs | Single morphed output IS the design concept |
| Polyphonic operation | 16x CPU cost, complicates drift and display |
| NanoVG premium knobs at runtime | NanoVG knob rendering deferred if GPU cost too high; SVG knobs as fallback |
| Panel expansion beyond 14HP | Forge Noir layout validated at 14HP |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| WAVE-01 | Phase 18 | Complete |
| WAVE-02 | Phase 18 | Complete |
| WAVE-03 | Phase 18 | Complete |
| WAVE-04 | Phase 18 | Complete |
| WAVE-05 | Phase 18 | Complete |
| PANEL-01 | Phase 19 | Complete |
| PANEL-02 | Phase 19 / 20.1 | Complete |
| PANEL-03 | Phase 19 | Complete |
| PANEL-04 | Phase 19 | Complete |
| PANEL-05 | Phase 19 | Complete |
| PANEL-06 | Phase 19 | Complete |
| PANEL-07 | Phase 19 | Complete |
| DISP-01 | Phase 20 | Complete |
| DISP-02 | Phase 20 | Complete |
| DISP-03 | Phase 20 | Complete |
| DISP-04 | Phase 20 | Complete |
| DISP-05 | Phase 20 | Complete |
| PANEL-08 | Phase 20.1 | Complete |
| PANEL-09 | Phase 20.1 | Complete |
| PANEL-10 | Phase 20.1 | Complete |
| PANEL-11 | Phase 20.1 | Complete |
| PANEL-12 | Phase 20.1 | Complete |
| ANIM-01 | Phase 21 | Complete |
| ANIM-02 | Phase 21 | Complete |

**Coverage:**
- v1.3 requirements: 24 total
- Mapped to phases: 24
- Unmapped: 0

---
*Requirements defined: 2026-03-28*
*Last updated: 2026-03-28 after roadmap creation*
