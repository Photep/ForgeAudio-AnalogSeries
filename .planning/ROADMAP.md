# Roadmap: Forge Audio Analog Series

## Overview

The Forge Audio Analog Series is a collection of VCV Rack 2 modules featuring analog-modeled oscillators with a three-knob engine (morph, character, drift) and real-time waveform display.

## Milestones

- ✅ **v1.0 Analog Series LFO** — Phases 1-6 (shipped 2026-03-07)
- ✅ **v1.1 Clock Sync** — Phases 7-10 (shipped 2026-03-13)
- ✅ **v1.2 Deep Analog** — Phases 11-17 (shipped 2026-03-17)
- ✅ **v1.3 Forge Noir** — Phases 18-21 (shipped 2026-06-13)
- ✅ **v1.4 Tempered** — Phases 22-28 (shipped 2026-07-10)

## Phases

**Phase Numbering:**
- Integer phases (22, 23, 24): Planned milestone work (continuous from v1.3 which ended at Phase 21)
- Decimal phases (24.1, 24.2): Urgent insertions (marked INSERTED), execute between integers

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
