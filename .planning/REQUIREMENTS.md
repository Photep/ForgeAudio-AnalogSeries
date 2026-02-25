# Requirements: Forge Audio Analog Series (LFO)

**Defined:** 2026-02-25
**Core Value:** The three-knob analog engine (morph, character, drift) that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback.

## v1 Requirements

### Waveform Engine

- [x] **WAVE-01**: Module generates sine, triangle, saw, and square waveforms from shared double-precision phase accumulator
- [x] **WAVE-02**: Morph knob continuously sweeps Sine → Triangle → Saw → Square using parametric shape morphing (each intermediate position is a musically valid waveshape, not a blended crossfade)
- [x] **WAVE-03**: Morph CV input modulates morph position

### Analog Character

- [ ] **CHAR-01**: Character knob crossfades each waveform from digital perfection to classic analog reference
- [ ] **CHAR-02**: Saw reference targets Minimoog Model D (exponential ramp curvature ~2-3% deviation, soft capacitor reset ~10-20us equivalent)
- [ ] **CHAR-03**: Square reference targets Roland SH-101/Juno-106 (sigmoid edge softening ~2-5us, duty cycle asymmetry 0.5-1.5%)
- [ ] **CHAR-04**: Triangle reference targets Moog/Prophet (rounded peaks via polynomial/sinusoidal caps, slope asymmetry 2-3%)
- [ ] **CHAR-05**: Sine reference models triangle-derived analog sine (1-3% residual THD, primarily 3rd harmonic)
- [ ] **CHAR-06**: HF rolloff via pitch-tracking lowpass (higher pitches get proportionally warmer)
- [ ] **CHAR-07**: Character CV input modulates character amount

### Drift

- [ ] **DRFT-01**: Drift knob controls pitch drift via multi-timescale Ornstein-Uhlenbeck process (layered random walks at 0.05Hz, 0.2Hz, 0.8Hz, ~2Hz)
- [ ] **DRFT-02**: Drift CV input modulates drift amount

### Display

- [ ] **DISP-01**: Real-time waveform display shows single cycle of current output waveform in upper portion of module
- [ ] **DISP-02**: Bright amber dot tracks current phase position along the waveform trace
- [ ] **DISP-03**: Display reflects all three knobs (morph shape + character deformation + drift effects) in real time
- [ ] **DISP-04**: Amber trace on dark navy background matching Forge Audio brand identity
- [ ] **DISP-05**: Lock-free double buffer architecture for audio-to-display data transfer (no mutexes in audio thread)

### Pitch Control

- [x] **PTCH-01**: Rate knob controls LFO frequency across sub-audio range
- [ ] **PTCH-02**: Octave snap knob with ±4 octave range
- [ ] **PTCH-03**: Semitone selector with up/down buttons and small digital display showing current value

### Output

- [x] **OUT-01**: Bipolar ±5V morphed waveform output
- [x] **OUT-02**: Inverted morphed waveform output

### Panel & Infrastructure

- [x] **PANL-01**: 12HP SVG panel with Forge Audio brand identity (deep navy #1a1a2e, amber #e8a838, lavender labels, white-gray text)
- [x] **PANL-02**: Panel SVG structured for designer handoff (template files, documented coordinates, nanosvg-compatible)
- [x] **INFR-01**: VCV Rack 2 plugin with proper plugin.json, Makefile, module registration, and cross-platform build

## v2 Requirements

### Drift Expansion

- **DRFT-03**: Phase jitter (cycle-to-cycle timing variation, band-limited noise on phase increment)
- **DRFT-04**: DC offset drift (slow random walk on output center, 10-50mV range)
- **DRFT-05**: Pitch slew (one-pole lowpass on rate changes, 1-5ms settling time)
- **DRFT-06**: Component spread (per-instance variation seeded from module ID)

### LFO I/O Expansion

- **IO-01**: Reset/sync input (Schmitt trigger phase reset)
- **IO-02**: FM input (exponential frequency modulation)
- **IO-03**: Waveform bleed in morph transition zones (scaled with character knob)

### VCO Module (Separate Milestone)

- **VCO-01**: Audio-rate operation with morph-aware polyBLEP antialiasing (4th order)
- **VCO-02**: V/Oct pitch input with standard 1V/octave tracking
- **VCO-03**: FM input (exponential)
- **VCO-04**: Through-zero FM option
- **VCO-05**: Hard sync input (sub-sample accurate BLEP compensation)
- **VCO-06**: Shape CV input (controls morph position)
- **VCO-07**: Phase distortion
- **VCO-08**: Tracking error modeling (toggleable via right-click context menu, default OFF)
- **VCO-09**: Oversampling option (Off/2x/4x) via right-click context menu
- **VCO-10**: Coarse/fine tune controls

## Out of Scope

| Feature | Reason |
|---------|--------|
| Separate waveform outputs | Single morphed output IS the design concept — users wanting separate shapes have Befaco, Bogaudio, Fundamental |
| Polyphonic operation | 16x CPU cost, complicates drift and display. Mono is the modular norm |
| Built-in effects (chorus, reverb) | Oscillators oscillate, effects process. Modular philosophy = separate concerns |
| Wavetable mode | Different paradigm. Dilutes analog identity. Valley Terrorform owns this space |
| Named synth presets | Undercuts hands-on tweaking and invites trademark issues. Character knob position IS the preset |
| MIDI input / quantization | Upstream module responsibilities. Standard V/Oct input |
| Amplitude envelope | Oscillators oscillate, envelopes shape. Combining = semi-voice that breaks modular philosophy |
| Scope / spectrum analyzer | Display is a shape preview, not a measurement tool. Users have Fundamental Scope |
| Individually exposed drift params | One drift knob scaling everything in curated proportions. Right-click for advanced tuning |
| Built-in sub-oscillator | Panel complexity, dilutes three-knob focus |
| MinBLEP antialiasing | Defer to v2+ only if polyBLEP aliasing proves insufficient |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| INFR-01 | Phase 1 | Complete |
| PANL-01 | Phase 1 | Complete |
| PANL-02 | Phase 1 | Complete |
| WAVE-01 | Phase 2 | Complete |
| WAVE-02 | Phase 2 | Complete |
| WAVE-03 | Phase 2 | Complete |
| OUT-01 | Phase 2 | Complete |
| OUT-02 | Phase 2 | Complete |
| PTCH-01 | Phase 2 | Complete |
| DISP-01 | Phase 3 | Pending |
| DISP-02 | Phase 3 | Pending |
| DISP-03 | Phase 3 | Pending |
| DISP-04 | Phase 3 | Pending |
| DISP-05 | Phase 3 | Pending |
| PTCH-02 | Phase 4 | Pending |
| PTCH-03 | Phase 4 | Pending |
| CHAR-01 | Phase 5 | Pending |
| CHAR-02 | Phase 5 | Pending |
| CHAR-03 | Phase 5 | Pending |
| CHAR-04 | Phase 5 | Pending |
| CHAR-05 | Phase 5 | Pending |
| CHAR-06 | Phase 5 | Pending |
| CHAR-07 | Phase 5 | Pending |
| DRFT-01 | Phase 6 | Pending |
| DRFT-02 | Phase 6 | Pending |

**Coverage:**
- v1 requirements: 25 total
- Mapped to phases: 25
- Unmapped: 0

---
*Requirements defined: 2026-02-25*
*Last updated: 2026-02-25 after roadmap creation*
