# Roadmap: Forge Audio Analog Series

## Overview

The Forge Audio Analog Series is a collection of VCV Rack 2 modules featuring analog-modeled oscillators with a three-knob engine (morph, character, drift) and real-time waveform display.

## Milestones

- ✅ **v1.0 Analog Series LFO** — Phases 1-6 (shipped 2026-03-07)
- ✅ **v1.1 Clock Sync** — Phases 7-10 (shipped 2026-03-13)
- ✅ **v1.2 Deep Analog** — Phases 11-17 (shipped 2026-03-17)
- ✅ **v1.3 Forge Noir** — Phases 18-21 (shipped 2026-06-13)
- ✅ **v1.4 Tempered** — Phases 22-28 (shipped 2026-07-10)
- 🔨 **v2.0 Forge Analog VCO** — Phases 29-36 (in progress)

## Phases

**Phase Numbering:**

- Integer phases (29, 30, 31): Planned milestone work (continuous from v1.4 which ended at Phase 28)
- Decimal phases (29.1, 29.2): Urgent insertions (marked INSERTED), execute between integers

<details>
<summary>✅ v1.0 Analog Series LFO (Phases 1-6) — SHIPPED 2026-03-07</summary>

- [x] Phase 1: Plugin Scaffold and Panel (2/2 plans) — completed 2026-02-25
- [x] Phase 2: Waveform Engine (2/2 plans) — completed 2026-02-25
- [x] Phase 3: Waveform Display (2/2 plans) — completed 2026-02-26
- [x] Phase 4: Analog Character (2/2 plans) — completed 2026-03-07
- [x] Phase 5: Drift Engine (2/2 plans) — completed 2026-03-07
- [x] Phase 6: Polish & Cleanup (2/2 plans) — completed 2026-03-07

See: `.planning/milestones/v1.0-ROADMAP.md` for full details.

</details>

<details>
<summary>✅ v1.1 Clock Sync (Phases 7-10) — SHIPPED 2026-03-13</summary>

- [x] Phase 7: Clock Input and Period Tracking (2/2 plans) — completed 2026-03-07
- [x] Phase 8: Frequency Override and Ratio Table (1/1 plans) — completed 2026-03-10
- [x] Phase 9: Phase Reset and Drift Integration (1/1 plans) — completed 2026-03-11
- [x] Phase 10: Display and Panel (2/2 plans) — completed 2026-03-13

See: `.planning/milestones/v1.1-ROADMAP.md` for full details.

</details>

<details>
<summary>✅ v1.2 Deep Analog (Phases 11-17) — SHIPPED 2026-03-17</summary>

- [x] Phase 11: Display Polish (1/1 plan) — completed 2026-03-13
- [x] Phase 12: RESET and Phase Offset (2/2 plans) — completed 2026-03-15
- [x] Phase 13: FM Input (1/1 plan) — completed 2026-03-15
- [x] Phase 14: Expanded Imperfections (2/2 plans) — completed 2026-03-16
- [x] Phase 15: Waveform Bleed (1/1 plan) — completed 2026-03-17
- [x] Phase 16: Swing and Shuffle (1/1 plan) — completed 2026-03-17
- [ ] ~~Phase 17: Panel Redesign~~ — SKIPPED (Surge-style modulation routing abandoned; panel evolved via Forge Noir instead)

See: `.planning/milestones/v1.2-ROADMAP.md` for full details.

</details>

<details>
<summary>✅ v1.3 Forge Noir (Phases 18-21) — SHIPPED 2026-06-13</summary>

- [x] Phase 18: PWM DSP Extension (1/1 plan) — completed 2026-03-28
- [x] Phase 19: Forge Noir Panel + Custom Components (4/4 plans) — completed 2026-04-01
- [x] Phase 20: Display Layout + CRT Aesthetic (3/3 plans) — completed 2026-06-11
- [x] Phase 20.1: Panel Redesign 18HP Fresh Layout (5/5 plans, INSERTED) — completed 2026-06-12
- [x] Phase 21: Animated SYNC Badge (1/1 plan) — completed 2026-06-13

See: `.planning/milestones/v1.3-ROADMAP.md` for full details.

</details>

<details>
<summary>✅ v1.4 Tempered (Phases 22-28) — SHIPPED 2026-07-10</summary>

**Milestone Goal:** Take the feature-complete Analog LFO to a publishable, VCV-Library-ready plugin — bugs fixed, tested, package compliant, manual written, source published. The LFO is feature-frozen; no new DSP features.

- [x] Phase 22: Test Harness Foundation (4/4 plans) — completed 2026-06-14
- [x] Phase 23: Functional Bug Fixes (5/5 plans) — completed 2026-06-14
- [x] Phase 24: DSP Extraction + Display Refactors (4/4 plans) — completed 2026-06-30, verified 2026-07-08
- [x] Phase 25: Release IP Hardening — PRIVATE (4/4 plans) — completed 2026-07-08
- [x] Phase 26: VCV Library Compliance + Packaging (4/4 plans) — completed 2026-07-09
- [x] Phase 27: User Manual — GitHub Markdown (4/4 plans) — completed 2026-07-09
- [x] Phase 28: Publish + Submit (3/3 plans) — completed 2026-07-10

Shipped: public repo `Photep/ForgeAudio-AnalogSeries`, release commit `4d7b0a8` tagged `v2.0.0`, VCV Library submission issue [#929](https://github.com/VCVRack/library/issues/929).

See: `.planning/milestones/v1.4-ROADMAP.md` for full details.

</details>

### 🔨 v2.0 Forge Analog VCO (Phases 29-36) — IN PROGRESS

**Milestone Goal:** Ship the second module in the Analog Series — an analog-modeled *morphing* VCO that brings the LFO's morph/character/drift identity to audio rate, as a second module inside the existing `ForgeAudio-AnalogSeries` plugin. Lean core scope (through-zero FM, phase distortion, oversampling, tracking-error deferred to v2.1).

> **⚠ MILESTONE GUARDRAIL — the shipped LFO must not regress.** The Analog LFO is live in the VCV Library and pinned by bit-exact `.f32` goldens. All VCO work is additive (new files: `VcoCore.hpp`, `MorphBlep.hpp`, a separate `DriftEngine` instance). The four shared `src/dsp/` headers stay frozen; any unavoidable additive touch (only `DriftEngine.hpp`, Phase 34) is gated by a byte-identical LFO golden replay. Any LFO-regression risk — shared-header edits, `plugin.json`/version/registration changes — is surfaced to the operator with impact + remediation options + a recommendation before proceeding. Tripwires: LFO goldens + `make strict` (C++11) + the CI MinGW link leg, wired as a standing canary in Phase 29.

- [x] **Phase 29: VCO Test Harness & LFO Non-Regression Guardrail** - Stand up the Rack-free VcoCore harness and wire the LFO golden + strict/MinGW canary before any VCO DSP lands (completed 2026-07-28)
- [x] **Phase 30: VcoCore Skeleton & Module Registration** - Pitch-accurate (aliased-on-purpose) VcoCore behind the POD boundary, registered as a second module (completed 2026-07-29)
- [x] **Phase 31: Pitch, Tuning & Exponential FM** - Accurate 1V/oct + coarse/fine tune + audio-rate expo FM, all summed before one exp2, proven to <1 cent (completed 2026-07-30)
- [ ] **Phase 32: Morph-Aware Anti-Aliasing (polyBLEP/polyBLAMP)** - The linchpin: band-limit the continuous character-deformed morph crossfade in an isolated wrapper header
- [ ] **Phase 33: Hard Sync** - Click-free hard sync reusing the anti-aliasing machinery at the master's sub-sample fraction
- [ ] **Phase 34: Audio-Rate Analog Engine, Drift & Output Stage** - Morph/character/drift at audio rate with VCO-appropriate drift authority and a clean bounded output
- [ ] **Phase 35: Shell, Panel & Display** - The Rack shell, 18HP Forge Noir panel, and audio-rate-safe CRT display
- [ ] **Phase 36: Goldens, Cross-Platform CI & Library Update** - Portable/gated VCO goldens, strict/MinGW green, ship the feature update via #929

## Phase Details

### Phase 29: VCO Test Harness & LFO Non-Regression Guardrail

**Goal**: Before any VCO DSP exists, stand up the Rack-free VCO test harness and lock in the shipped-LFO guardrail as a standing, always-green canary — so no later phase can silently threaten the live LFO.
**Depends on**: Nothing (first VCO phase; builds on the shipped v1.4 test infrastructure)
**Requirements**: TEST-01, TEST-04, TEST-06
**Success Criteria** (what must be TRUE):

  1. A single `make test` run replays the shipped LFO's existing `.f32` goldens byte-identical *and* drives a new Rack-free VCO harness — the LFO canary is proven green before any VCO DSP lands.
  2. A `VcoBlockDriver` (mirroring `BlockDriver`) drives a `forge::VcoCore` over sample blocks at 44.1 / 48 / 96 kHz with no libRack linked, using non-degenerate default seeds (no `(0,0)` Xoroshiro fixed point).
  3. `make strict` (C++11, `-pedantic-errors`) and the CI MinGW **link** leg both cover the new `AnalogVCO` translation unit and VCO headers, failing on any ODR / C++17-ism (the exact class that rejected v2.0.0).
  4. The full test + strict + MinGW canary runs in CI on every push and is green.

**Plans**: 5/5 plans complete
Plans:
**Wave 1**

- [x] 29-01-PLAN.md — Bare POD `VcoCore` seam (D-01/D-03) + Rack-free `VcoBlockDriver` + TEST-01 harness invariants at 44.1/48/96 kHz
- [x] 29-02-PLAN.md — Test-scope SHA-256 + D-04 golden-byte checksum lock with published-vector and flipped-bit negative controls

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 29-03-PLAN.md — D-07/D-08 compile canary that ODR-uses the VCO headers, placement decision checkpoint, and `check_canary.sh` wired into CI

**Wave 3** *(blocked on Wave 2 completion)*

- [x] 29-04-PLAN.md — D-05 frozen-header manifest + D-06 dependency-direction audit, `make guards`, and the standing guard-wiring assertion

**Wave 4** *(blocked on Wave 3 completion)*

- [x] 29-05-PLAN.md — Full local phase gate + CI-only ODR link-gate negative control (push red, revert, confirm green)

**Guardrail**: This phase *is* the guardrail — the standing LFO-golden replay + strict/MinGW canary it wires (TEST-04/06) runs automatically at the end of every subsequent phase.

### Phase 30: VcoCore Skeleton & Module Registration

**Goal**: A pitch-accurate but intentionally aliased `VcoCore` behind the proven POD boundary, registered as a second module so it appears and sounds (crudely) in Rack — proving the architecture before any hard DSP.
**Depends on**: Phase 29
**Requirements**: CORE-01, CORE-03, PANEL-03
**Success Criteria** (what must be TRUE):

  1. `forge::VcoCore` (`src/dsp/VcoCore.hpp`) exposes a POD-`Inputs` → `step()` → output+telemetry boundary mirroring `LfoCore`, driven headlessly by the Phase-29 harness.
  2. The core produces a naive (aliased-on-purpose) morphed waveform at audio rate via basic V/oct through `exp2_taylor5`, with **no static/global mutable voice state** — polyphony-ready so a future v2.1 is an additive shell change, not a rewrite.
  3. The VCO appears as a second selectable module in Rack via a second `addModel` + `plugin.hpp` extern + `plugin.json` `modules[]` entry, with the LFO's registration and slug untouched.
  4. Fixed-seed determinism holds: same seed → bit-identical block; different seed diverges.

**Plans**: 10/10 plans complete
Plans:
**Wave 1**

- [x] 30-01-PLAN.md — `check_includes.sh [2/7]` exact-path exemption for the Rack-free shim + a two-direction negative control, opened by the D-05 blocking operator checkpoint on the permanent slug and the guard weakening

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 30-02-PLAN.md — The real `VcoCore::step()` body (D-11/D-12/D-13/D-14) + D-15 tombstone inversion in place + D-19 re-evidencing of the two green-but-weak rows

**Wave 3** *(blocked on Wave 2 completion)*

- [x] 30-03-PLAN.md — New `tests/test_vco_core.cpp`: the three CORE-01 cases (output-measured pitch, 6.0 V magnitude bound, spread-seed divergence) with measured non-vacuity
- [x] 30-05-PLAN.md — `src/AnalogVCO.cpp` minimum-viable Rack shell (four live controls, stock widgets, non-degenerate seeding) + the throwaway 18 HP `res/AnalogVCO.svg`

**Wave 4** *(blocked on Wave 3 completion)*

- [x] 30-04-PLAN.md — CORE-03 two-instance interleave independence + the permanent deliberately-broken-core positive control
- [x] 30-06-PLAN.md — Additive registration under slug `ForgeAnalogVCO`: `plugin.hpp` extern, `plugin.cpp` `addModel`, second `plugin.json` `modules[]` entry, LFO entry proven byte-unchanged

**Wave 5** *(blocked on Wave 4 completion)*

- [x] 30-07-PLAN.md — Phase gate: full local gate at 72 cases, CI `toolchain-gate` MinGW **link**-leg observed green on the pushed commit, and the in-Rack UAT opening with a stale-install flush

**Gap closure — Wave 1** *(from `30-UAT.md`: CR-01 marked "fix now"; WR-02 comment correction carried out of test 3, option a)*

- [x] 30-08-PLAN.md — CR-01 Nyquist guard clamp-order fix (ceiling before the NaN-safe floor) + the WR-01 phase-increment bound the hostile grid requires, with the WR-03 driverless coverage case OBSERVED RED first and bit-identity proven by a before/after `-s` diff
- [x] 30-09-PLAN.md — Correct the false per-instance-variation claim in `src/AnalogVCO.cpp` (the seed literals themselves stay byte-unchanged, T-30-02) + file WR-02 / CR-02 / WR-04 / WR-05 into `deferred-items.md` with owning phases

**Gap closure — Wave 2** *(blocked on both wave-1 gap plans)*

- [x] 30-10-PLAN.md — Gap-closure gate: combined local gate, whole-range four-file diff audit, one push, and the CI `win-x64 leg reproduction` **step's own conclusion** observed on the exact pushed SHA

**Guardrail**: Registration touches `plugin.cpp` / `plugin.hpp` / `plugin.json` additively (a second module entry). A permanent VCO slug is chosen here — surface the slug and the registration diff to the operator before committing; the LFO entry stays byte-unchanged.

### Phase 31: Pitch, Tuning & Exponential FM

**Goal**: Musical, accurate pitch — 1V/octave tracking with coarse/fine tune and audio-rate exponential FM, all summed in the volt domain before a single exponential, proven correct to within a cent.
**Depends on**: Phase 30
**Requirements**: PITCH-01, PITCH-02, PITCH-03, PITCH-04, PITCH-05, FM-01, FM-02, FM-03, TEST-02
**Success Criteria** (what must be TRUE):

  1. The V/OCT input tracks 1V/octave from a standard `C4 = 0V → 261.6256 Hz` reference, reusing `forge::exp2_taylor5` verbatim; measured tracking error stays **< 1 cent** across the pitch range (TEST-02 is the phase gate).
  2. COARSE sweeps ±5 octaves continuously and FINE trims ±1 semitone (±100 cents) for detuning/beating.
  3. Exponential FM (input × bipolar attenuverter × depth) sums into the pitch volt domain **before** the single `exp2_taylor5` call — musical exponential FM, not multiplicative.
  4. Frequency is clamped just below Nyquist and phase accumulates in double precision, so extreme pitch/FM never aliases via an out-of-range frequency and high-note sub-sample crossing placement stays accurate for later band-limiting.

**Plans**: 9/9 plans complete
Plans:
**Wave 1**

- [x] 31-01-PLAN.md — Pre-register `tests/test_vco_pitch.cpp` in `check_includes.sh` `VCO_SIDE_ALLOW` (D-23) and fold in deferred item 5's `[2/7]` anchoring fix with a new `[6/7]` control
- [x] 31-02-PLAN.md — PITCH-04's settled Nyquist policy: `kVcoNyquistGuardFrac` → `0.495f` (D-11) with derived ceilings, D-10's hard-clamp rationale, and the `kVcoMaxDeltaPhase` margin arithmetic corrected

**Wave 2** *(blocked on Wave 1 — same header)*

- [x] 31-03-PLAN.md — The volt-domain summation, the `fmConnected` gate and the `kVcoMaxPitchVolts` bound before the single `exp2_taylor5`, with D-22's one-shot UBSan RED→GREEN transcript

**Wave 3**

- [x] 31-04-PLAN.md — Shell + panel: COARSE, FINE, FM DEPTH and FM IN declared with exact ranges/units (D-02/03/04/07/16), five bare POD assignments (D-17), four widgets and four rects
- [x] 31-05-PLAN.md — NEW `tests/test_vco_pitch.cpp`: derived-boundary self-check plus the TEST-02 tracking gate and its secondary telemetry tier, libm ground truth, this phase's own measured figures (D-18/19/20/21)

**Wave 4** *(blocked on Wave 3 — same test TU)*

- [x] 31-06-PLAN.md — COARSE/FINE range + continuity cases, the FM summation identity/bipolarity/connected gate/one-octave-per-volt, and the multiplicative negative control that must FAIL

**Wave 5** *(blocked on Wave 4 — same test TU)*

- [x] 31-07-PLAN.md — PITCH-04's pitch-driven clamp-fires case (D-10), D-22's standing hostile-pitch case pinned at the bound, PITCH-05 compile-time pin, and the `test_vco_core.cpp` stand-in mirror brought in step

**Wave 6** *(blocked on Waves 3-5)*

- [x] 31-08-PLAN.md — Phase gate: TEST-02 as a hard gate with per-selector matched counts, whole-diff prohibition sweep, CI observed green BY SHA, and the deferred register incl. D-24

**Wave 7** *(blocked on Wave 6)*

- [x] 31-09-PLAN.md — Full `dist/` flush, install-freshness proof, and the blocking operator in-Rack UAT checkpoint

### Phase 32: Morph-Aware Anti-Aliasing (polyBLEP/polyBLAMP)

**Goal**: Band-limit the continuous, character-deformed morph crossfade so the oscillator stays clean across the whole keyboard — the single dominant-risk subsystem, fully isolated in its own wrapper header with its own spectral iteration budget.
**Depends on**: Phase 31 (proven pitch path + skeleton core; wraps the frozen `Waveshape.hpp`)
**Requirements**: MORPH-01, MORPH-02, AA-01, AA-02, AA-03, AA-04, AA-05, CORE-02, TEST-03
**Success Criteria** (what must be TRUE):

  1. A new additive `MorphBlep.hpp` *calls* (never edits) the frozen `Waveshape.hpp`, applying polyBLEP at value-step discontinuities (saw wrap, square edge, variable-width pulse edges) and polyBLAMP at triangle slope corners — the frozen shared header takes zero edits.
  2. The MORPH control (knob + CV + attenuverter) sweeps the continuous 5-shape crossfade (sine→triangle→saw→square→narrow-pulse) at audio rate with band-limited output; BLEP/BLAMP magnitude is driven by the *characterized* jump so CHARACTER edge-softening auto-scales the correction.
  3. Multiple / overlapping discontinuities within a single sample (narrow-pulse duty edges) are each placed at their own sub-sample position and summed, not overwritten — narrow pulse keeps its body at high notes.
  4. A spectral alias-floor invariant (a small DFT/Goertzel helper) asserts high-note aliasing stays below **per-shape thresholds pinned from measurement** (D-09 evidence-set thresholds, each carrying its measured justification) at **C7, C8 and C9** — TEST-03. The earlier "≈ −60 dB" figure was a target, not a reachable floor: 2-sample polyBLEP attenuates by `sinc²`, which is only ≈ −8 dB at Nyquist, so the achievable floor sits well above −60 dB (measured best at C8/44.1 kHz: saw −25.8, square −31.9, narrow pulse −11.6 dB) — corroborated by DAFx-16. Per-shape measured thresholds are the honest gate.
  5. All anti-aliasing is table-free and Rack-free (closed-form arithmetic), preserving C++11-strict compilation and golden bit-stability — no minBLEP, no oversampling in v2.0.

**Plans**: 11 plans

Plans:
**Wave 1**

- [ ] 32-01-PLAN.md — Guard pre-registration, the exact-integer-cycle spectral apparatus with its own leakage self-check, and the test-only naive mirror (D-08 / D-10)
- [ ] 32-02-PLAN.md — MORPH CV jack, bipolar attenuverter, NaN-safe shell mix and the paired panel rects (MORPH-02 / D-16 / D-17)

**Wave 2** *(blocked on Wave 1 completion)*

- [ ] 32-03-PLAN.md — The 90-cell naive alias baseline, then the gate observed RED and landed as a tombstone (D-08 / D-09 / D-11)

**Wave 3** *(blocked on Wave 2 completion)*

- [ ] 32-04-PLAN.md — `src/dsp/MorphBlep.hpp`: kernels, compact-support character factor, nine-site union, split-source crossing test, plus the canary include (CORE-02 / AA-01..05)

**Wave 4** *(blocked on Wave 3 completion)*

- [ ] 32-05-PLAN.md — Unit suite: characterized jumps probed against the frozen `Waveshape`, the D-03 limits, overlapping edges, the sync seam, hostile `dt`, and the P-3 resonant-tiling regression (AA-02/03/04)
- [ ] 32-06-PLAN.md — Wire `MorphBlep` into `VcoCore.hpp:484`, harden morph/character, correct two falsified premises, invert the baseline tombstone (CORE-02 / MORPH-01)

**Wave 5** *(blocked on Wave 4 completion)*

- [ ] 32-07-PLAN.md — Close the measure→pin loop: pin D-09 thresholds, invert the tombstone into the live TEST-03 gate, add the no-regression and cross-rate invariants
- [ ] 32-08-PLAN.md — Bring the oracle in step, re-derive the output bound into two measured tiers, exercise the hostile tier (AA-01 / AA-03)

**Wave 6** *(blocked on Wave 5 completion)*

- [ ] 32-09-PLAN.md — Extend the hostile-timing grid on the corrected D-15 rationale, and add the audio-rate MORPH invariant (MORPH-01/02)

**Wave 7** *(blocked on Wave 6 completion)*

- [ ] 32-10-PLAN.md — Phase gate: full local suite, CI link leg observed by SHA, requirement traceability, deferred register

**Wave 8** *(blocked on Wave 7 completion)*

- [ ] 32-11-PLAN.md — Operator in-Rack UAT of the audio-rate MORPH sweep and the shipped-LFO guardrail (blocking checkpoint)

**Cross-cutting constraints:**

- D-09: thresholds are per (morph region, note, character) and each carries its measured justification in the test; a single global threshold and a global-floor-with-pulse-carve-out are both rejected.

**Research flag**: RESOLVED at plan time. `32-RESEARCH.md` measured the D-03 factor (`k = max(0, 1 − w/(2·dt))²`, compact support load-bearing) and the achievable per-shape floors against the real frozen header; the MEDIUM-confidence item that remains is the exact threshold numbers, which D-08's measure→pin loop closes in plans 32-03 and 32-07. This is the risk concentrator — budget iteration here.

### Phase 33: Hard Sync

**Goal**: Classic buzzy hard sync that resets the oscillator on a master edge without clicks, reusing the Phase-32 band-limiting machinery rather than the LFO's slow cosine crossfade.
**Depends on**: Phase 32 (reuses the sync-BLEP / discontinuity machinery)
**Requirements**: SYNC-01, SYNC-02
**Success Criteria** (what must be TRUE):

  1. A hard sync input resets oscillator phase on the master's rising edge (Schmitt-triggered).
  2. The reset uses sub-sample fractional placement plus a sync-BLEP applied at the **master's** wrap fraction, reusing the anti-aliasing machinery — explicitly **not** the LFO's 3 ms cosine crossfade — producing the sharp, buzzy sync timbre without clicks.
  3. A sync-continuity invariant bounds the per-sample step across a reset (no full-scale artifact) and correctly handles ≥1 sync event within a single sample.

**Plans**: TBD

### Phase 34: Audio-Rate Analog Engine, Drift & Output Stage

**Goal**: The morph/character/drift analog engine running at audio rate — with VCO-appropriate drift authority (a few cents, not the LFO's semitone) and a clean, bounded audio output — the headline "alive" character without detuning the oscillator or regressing the LFO.
**Depends on**: Phase 32
**Requirements**: CHAR-01, DRIFT-01, DRIFT-02, DRIFT-03, OUT-01, OUT-02, OUT-03
**Success Criteria** (what must be TRUE):

  1. CHARACTER and DRIFT (each knob + CV + attenuverter) shape oscillator timbre and apply analog pitch/timbre instability at audio rate through the reused engine.
  2. The VCO uses a **separate, independently-seeded** `DriftEngine` instance with VCO-specific few-cents authority set via parameters; the shared `DriftEngine` defaults and RNG draw-order are unchanged, and the LFO's drift-on goldens replay byte-identical.
  3. VCO drift depth reads as "alive," not "detuned" — calibrated by in-Rack audition (operator-gated decision, DRIFT-03).
  4. A single morphed audio output at ±5V (bipolar) passes through a DC blocker (~5–20 Hz high-pass) and soft saturation/clamp, so asymmetric / narrow-pulse morph positions and extreme FM/sync stay within Rack voltage norms.

**Plans**: TBD
**Guardrail**: This is the *only* phase that touches a shared header — `DriftEngine.hpp` gains configurable authority members whose defaults equal today's LFO literals (IEEE-bit-identical). The additive edit is surfaced to the operator and gated by a byte-identical LFO drift-on golden replay before commit.
**Human gate**: DRIFT-03 drift-depth value is audition-gated (operator listens in Rack), matching the v1.4 x1.5/÷1.5-ratio precedent. The DC-blocker policy (accept DC vs. light high-pass) is a deliberate operator call, not a default inheritance of the LFO's DC-positive stance.

### Phase 35: Shell, Panel & Display

**Goal**: Wrap the proven headless core in its Rack shell — an 18HP Forge Noir panel and an audio-rate-safe CRT display — so in-Rack UAT validates wiring and feel, not DSP correctness.
**Depends on**: Phases 30–34
**Requirements**: PANEL-01, PANEL-02, DISP-01, DISP-02, DISP-03
**Success Criteria** (what must be TRUE):

  1. An 18HP Forge Noir panel, consistent with the LFO's design language, exposes V/OCT in, SYNC in, FM in + attenuverter, MORPH/CHARACTER/DRIFT + their CV + attenuverters, COARSE, FINE, and OUT.
  2. The display shows a static single-cycle morph preview reflecting MORPH/CHARACTER, regenerated off the audio thread from parameters (not captured from live audio-rate samples) — so the trace stays clean at 2–5 kHz.
  3. The readout shows note / frequency (Hz) instead of the LFO's BPM pill.
  4. The spinning phase dot is dropped or decoupled (it would smear into a blur at audio pitch).

**Plans**: TBD
**UI hint**: yes

### Phase 36: Goldens, Cross-Platform CI & Library Update

**Goal**: Lock the VCO's golden fixtures and cross-platform CI, confirm the strict/MinGW gate green for the final shell, and ship the VCO as a feature update on the live plugin's 2.x line.
**Depends on**: Phase 35
**Requirements**: TEST-05, REL-01
**Success Criteria** (what must be TRUE):

  1. New VCO goldens are captured and byte-replayed: **drift-off** fixtures are cross-platform portable and gate all 3 OS; **drift-on** fixtures are macOS-gated (matching the LFO policy), captured with `-ffp-contract=off`.
  2. `make strict` (C++11) and the CI MinGW **link** leg are confirmed green for the final `AnalogVCO` TU and all new headers *before* any tag is cut — never ship on green `make strict` alone.
  3. The manifest `version` is bumped on the Rack-major 2.x line with a fresh tag, and the VCO ships as a feature update via the existing VCV Library thread (#929) — the multi-module feature-update procedure confirmed against current library docs before tagging.

**Plans**: TBD
**Research flag**: MEDIUM-confidence on the precise VCV Library *feature-update* mechanics for adding a module to an already-live plugin (auto-pickup from manifest version vs. a fresh action on #929) — verify against current library docs at release time.

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 29. VCO Test Harness & LFO Guardrail | 5/5 | Complete    | 2026-07-28 |
| 30. VcoCore Skeleton & Registration | 10/10 | Complete    | 2026-07-29 |
| 31. Pitch, Tuning & Exponential FM | 9/9 | Complete    | 2026-07-30 |
| 32. Morph-Aware Anti-Aliasing | 0/? | Not started | - |
| 33. Hard Sync | 0/? | Not started | - |
| 34. Analog Engine, Drift & Output | 0/? | Not started | - |
| 35. Shell, Panel & Display | 0/? | Not started | - |
| 36. Goldens, CI & Library Update | 0/? | Not started | - |
