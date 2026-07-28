# Requirements: Forge Analog VCO (v2.0)

**Defined:** 2026-07-20
**Core Value:** The three-knob analog engine (morph, character, drift) — now at audio rate as a morph/macro-style oscillator — that lets users dial anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback.
**Milestone scope:** Lean core VCO, added as a **second module** inside the shipped `ForgeAudio-AnalogSeries` plugin. Reuses the LFO's extracted analog core. Monophonic. Advanced DSP (through-zero FM, phase distortion, oversampling, tracking-error) deferred to v2.1.

> **⚠ LFO NON-REGRESSION GUARDRAIL (milestone-wide):** The shipped Analog LFO — live in the VCV Library, pinned by bit-exact `.f32` goldens — must not get breaking or behavioral changes. All VCO work is additive (new files); shared `src/dsp/` headers stay frozen. Any change risking LFO behavior is surfaced to the operator with impact + remediation options + a recommendation before proceeding. Tripwires: LFO goldens + `make strict` + the CI MinGW link leg.

## v1 Requirements (v2.0)

Requirements for the initial VCO release. Each maps to a roadmap phase.

### Pitch & Tuning

- [ ] **PITCH-01**: V/Oct input tracks 1V/octave across the audio range (C4 = 0V reference), reusing `forge::exp2_taylor5` for the exponential pitch law (no new exponential; shared-core bit-identity preserved)
- [ ] **PITCH-02**: COARSE tune knob sweeps ±5 octaves continuously
- [ ] **PITCH-03**: FINE tune knob trims ±2 semitones for detuning/beating
- [ ] **PITCH-04**: Frequency is clamped just below Nyquist so extreme pitch/FM/sync never aliases via out-of-range frequency
- [ ] **PITCH-05**: Phase accumulation uses double precision so high-frequency phase-crossing placement stays accurate for band-limiting

### FM

- [ ] **FM-01**: Exponential FM input modulates pitch at audio rate
- [ ] **FM-02**: A dedicated bipolar attenuverter sets FM depth
- [ ] **FM-03**: FM sums into the volt domain before the single exponential (musical exponential FM)

### Morph Engine & Anti-Aliasing (the linchpin)

- [ ] **MORPH-01**: The morph engine (`Waveshape`) runs at audio rate, reused verbatim from the frozen shared core
- [ ] **MORPH-02**: MORPH knob + CV + attenuverter sweep the continuous 5-shape crossfade (sine→triangle→saw→square→narrow-pulse) at audio rate
- [ ] **AA-01**: Morph-aware polyBLEP band-limits the value-step discontinuities (saw wrap, square edge, variable-width pulse edges) of the continuous crossfade, scaled by the morph weights
- [ ] **AA-02**: polyBLAMP band-limits the triangle slope-corner discontinuities
- [ ] **AA-03**: Anti-aliasing correctly handles multiple/overlapping discontinuities within one sample at narrow pulse widths
- [ ] **AA-04**: BLEP/BLAMP magnitude is driven by the characterized (actual) jump so CHARACTER edge-softening auto-scales the correction
- [ ] **AA-05**: Anti-aliasing is table-free and Rack-free (closed-form arithmetic) — preserving C++11-strict compilation and golden bit-stability; no minBLEP, no oversampling in v2.0

### Sync

- [ ] **SYNC-01**: Hard sync input resets oscillator phase on a master rising edge
- [ ] **SYNC-02**: Sync reset uses sub-sample fractional placement plus a sync-BLEP (click-free), reusing the anti-aliasing machinery — not the LFO's 3 ms cosine crossfade

### Analog Character & Drift

- [ ] **CHAR-01**: CHARACTER knob + CV + attenuverter shape oscillator timbre at audio rate (reused engine)
- [ ] **DRIFT-01**: DRIFT knob + CV + attenuverter apply analog pitch/timbre instability at audio rate
- [ ] **DRIFT-02**: The VCO uses a **separate** `DriftEngine` instance with VCO-specific authority (few-cents range) set via parameters — the shared `DriftEngine` defaults and RNG draw-order are unchanged (LFO untouched, bit-identical)
- [ ] **DRIFT-03**: VCO drift depth is calibrated by in-Rack audition to read as "alive," not "detuned" (audition-gated decision)

### Output

- [ ] **OUT-01**: Single morphed audio output at ±5V (bipolar), consistent with the single-output design philosophy
- [ ] **OUT-02**: A DC blocker (~5–20 Hz high-pass) strips DC from asymmetric / narrow-pulse morph positions (VCO-only, additive; the LFO's intentional DC path is untouched)
- [ ] **OUT-03**: Output is soft-saturated/clamped to stay within Rack voltage norms under extreme FM/sync

### Display

- [ ] **DISP-01**: The display shows a static single-cycle morph preview reflecting MORPH/CHARACTER, regenerated off the audio thread from parameters (not captured from live audio-rate samples)
- [ ] **DISP-02**: The readout shows note / frequency (Hz) instead of the LFO's BPM pill
- [ ] **DISP-03**: The spinning phase dot is dropped or decoupled (it blurs into a smear at audio pitch)

### Panel & Registration

- [ ] **PANEL-01**: An 18HP Forge Noir panel for the VCO, consistent with the LFO's design language
- [ ] **PANEL-02**: Panel exposes V/OCT in, SYNC in, FM in + attenuverter, MORPH/CHARACTER/DRIFT + their CV + attenuverters, COARSE, FINE, and OUT
- [x] **PANEL-03**: The VCO is registered as a second module (`addModel` + `plugin.hpp` extern + `plugin.json` `modules[]` entry) without altering the LFO's registration

### Core & Test (boundary + guardrails)

- [x] **CORE-01**: A new Rack-free `forge::VcoCore` (`src/dsp/VcoCore.hpp`) mirrors the `LfoCore` POD-`Inputs` → `step()` → output+telemetry boundary
- [ ] **CORE-02**: Anti-aliasing lives in a new additive header (`MorphBlep.hpp`) that *calls* the frozen `Waveshape.hpp` — zero edits to shared headers
- [x] **CORE-03**: `VcoCore` is a self-contained per-voice unit with no static/global mutable voice state — **polyphony-ready** so a future v2.1 polyphony is an additive shell change, not a rewrite
- [x] **TEST-01**: A Rack-free test harness drives `VcoCore` over sample blocks (mirrors `BlockDriver`), runnable via `make test` with no libRack
- [ ] **TEST-02**: V/Oct tracking accuracy is asserted (< 1 cent error) across the pitch range
- [ ] **TEST-03**: An alias-floor / spectral invariant asserts high-note aliasing stays below a defined threshold
- [x] **TEST-04**: The shipped LFO's `.f32` goldens are replayed byte-identical in the same `make test` run as a standing non-regression canary
- [ ] **TEST-05**: New VCO goldens — drift-off fixtures are cross-platform portable; drift-on fixtures are macOS-gated (matching the LFO policy)
- [x] **TEST-06**: The strict C++11 gate (`make strict`) and the CI MinGW link leg cover the new `AnalogVCO.cpp` translation unit (ODR / C++17-ism protection)

### Release

- [ ] **REL-01**: The VCO ships as a feature update to the live plugin via the existing VCV Library thread (#929), with correct multi-module version discipline (update procedure confirmed before tagging)

## v2 Requirements (deferred to v2.1+)

Tracked, not in this roadmap.

### Advanced Oscillator DSP

- **TZFM-01**: Through-zero FM (linear FM through zero Hz) with DC-offset/sign handling
- **PD-01**: Phase distortion (Casio-CZ-style) as a distinct synthesis mode
- **OS-01**: Oversampling option (Off / 2× / 4×) as the escalation path if polyBLEP alone proves insufficient at extreme brightness
- **TRK-01**: Tracking-error modeling (right-click toggle) for analog pitch-scaling imperfection

### Polyphony

- **POLY-01**: Polyphonic operation (up to 16 voices) via an array of `VcoCore` instances + per-channel processing (enabled by CORE-03; additive shell change)
- **POLY-02**: Per-voice drift seeding for authentic analog unison detune

## Out of Scope

Explicitly excluded from the Analog Series VCO by design.

| Feature | Reason |
|---------|--------|
| Individual per-shape waveform outputs | Single morphed output IS the design concept (carried from the LFO) |
| Wavetable mode | Different paradigm; dilutes the analog morph identity |
| Built-in sub-oscillator | Panel complexity; dilutes the three-knob focus |
| Named synth presets | Undercuts hands-on tweaking; invites trademark issues |
| minBLEP anti-aliasing | SDK-coupled + builds a startup table → breaks Rack-free `make test` and golden bit-stability |
| Scope / spectrum analyzer | Display is a shape preview, not a measurement tool |
| Individually exposed drift params | One drift knob with curated proportions (carried from the LFO) |

## Traceability

Every v1 requirement maps to exactly one phase. Phases 29-36 (v2.0 milestone; numbering continues from v1.4's Phase 28).

| Requirement | Phase | Status |
|-------------|-------|--------|
| PITCH-01 | Phase 31 | Pending |
| PITCH-02 | Phase 31 | Pending |
| PITCH-03 | Phase 31 | Pending |
| PITCH-04 | Phase 31 | Pending |
| PITCH-05 | Phase 31 | Pending |
| FM-01 | Phase 31 | Pending |
| FM-02 | Phase 31 | Pending |
| FM-03 | Phase 31 | Pending |
| MORPH-01 | Phase 32 | Pending |
| MORPH-02 | Phase 32 | Pending |
| AA-01 | Phase 32 | Pending |
| AA-02 | Phase 32 | Pending |
| AA-03 | Phase 32 | Pending |
| AA-04 | Phase 32 | Pending |
| AA-05 | Phase 32 | Pending |
| SYNC-01 | Phase 33 | Pending |
| SYNC-02 | Phase 33 | Pending |
| CHAR-01 | Phase 34 | Pending |
| DRIFT-01 | Phase 34 | Pending |
| DRIFT-02 | Phase 34 | Pending |
| DRIFT-03 | Phase 34 | Pending |
| OUT-01 | Phase 34 | Pending |
| OUT-02 | Phase 34 | Pending |
| OUT-03 | Phase 34 | Pending |
| DISP-01 | Phase 35 | Pending |
| DISP-02 | Phase 35 | Pending |
| DISP-03 | Phase 35 | Pending |
| PANEL-01 | Phase 35 | Pending |
| PANEL-02 | Phase 35 | Pending |
| PANEL-03 | Phase 30 | Complete |
| CORE-01 | Phase 30 | Complete |
| CORE-02 | Phase 32 | Pending |
| CORE-03 | Phase 30 | Complete |
| TEST-01 | Phase 29 | Complete |
| TEST-02 | Phase 31 | Pending |
| TEST-03 | Phase 32 | Pending |
| TEST-04 | Phase 29 | Complete |
| TEST-05 | Phase 36 | Pending |
| TEST-06 | Phase 29 | Complete |
| REL-01 | Phase 36 | Pending |

**Coverage:**

- v1 requirements: 40 total (enumerated IDs; supersedes the earlier "37" summary miscount)
- Mapped to phases: 40 ✓
- Unmapped: 0 ✓

**Phase → requirement rollup:**

- Phase 29 (Test Harness & Guardrail): TEST-01, TEST-04, TEST-06
- Phase 30 (Skeleton & Registration): CORE-01, CORE-03, PANEL-03
- Phase 31 (Pitch, Tuning & FM): PITCH-01..05, FM-01..03, TEST-02
- Phase 32 (Anti-Aliasing): MORPH-01..02, AA-01..05, CORE-02, TEST-03
- Phase 33 (Hard Sync): SYNC-01..02
- Phase 34 (Analog Engine, Drift & Output): CHAR-01, DRIFT-01..03, OUT-01..03
- Phase 35 (Shell, Panel & Display): PANEL-01..02, DISP-01..03
- Phase 36 (Goldens, CI & Library Update): TEST-05, REL-01

---
*Requirements defined: 2026-07-20*
*Last updated: 2026-07-20 — traceability populated by roadmapper (Phases 29-36); coverage corrected from 37 to 40 enumerated IDs*
