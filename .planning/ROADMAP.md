# Roadmap: Forge Audio Analog Series

## Overview

The Forge Audio Analog Series is a collection of VCV Rack 2 modules featuring analog-modeled oscillators with a three-knob engine (morph, character, drift) and real-time waveform display.

## Milestones

- ✅ **v1.0 Analog Series LFO** — Phases 1-6 (shipped 2026-03-07)
- ✅ **v1.1 Clock Sync** — Phases 7-10 (shipped 2026-03-13)
- ✅ **v1.2 Deep Analog** — Phases 11-17 (shipped 2026-03-17)
- ✅ **v1.3 Forge Noir** — Phases 18-21 (shipped 2026-06-13)
- 🚧 **v1.4 Tempered** — Phases 22-28 (in progress)

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

### 🚧 v1.4 Tempered (In Progress)

**Milestone Goal:** Take the feature-complete Analog LFO to a publishable, VCV-Library-ready plugin — bugs fixed, tested, package compliant, manual written, source published. The LFO is feature-frozen; no new DSP features.

- [x] **Phase 22: Test Harness Foundation** — Stand up the doctest target, the Rack-independent DSP core scaffold, and the headless block-driver harness before any code changes (completed 2026-06-14)
- [x] **Phase 23: Functional Bug Fixes** — Fix the four CODE-REVIEW functional bugs, each pinned by a regression test; #2 (x1.5/÷1.5) is audition-gated (completed 2026-06-14)
- [ ] **Phase 24: DSP Extraction + Display Refactors** — Complete the `src/dsp/*.hpp` core extraction, thin the Rack shell, and land the display/thread cleanups behind the green harness
- [ ] **Phase 25: Release IP Hardening (PRIVATE)** — Purge trial fonts from git history while the repo is still private, add LICENSE + NOTICES, confirm asset provenance
- [ ] **Phase 26: VCV Library Compliance + Packaging** — Populate/validate the manifest, build the verified `.vcvplugin`, wire CI green
- [ ] **Phase 27: Notion Manual** — Author and publish the user manual in Notion with full table-stakes sections
- [ ] **Phase 28: Publish + Submit** — Flip the repo public (gated on purge verification), then open the VCV Library submission issue

## Phase Details

### Phase 22: Test Harness Foundation
**Goal**: A standalone C++ test target builds and runs against a Rack-independent DSP core, with a headless block-driver harness — the safety net every later phase depends on.
**Depends on**: Nothing (first phase of v1.4)
**Requirements**: TEST-01, TEST-02 (initial core scaffold), TEST-04
**Success Criteria** (what must be TRUE):
  1. `make test` builds and runs a standalone doctest binary green, without touching or breaking the existing plugin build
  2. A Rack-independent `src/dsp/` core (with `RackCompat.hpp` shims) exists and is consumed by the plugin shell — the plugin still builds and loads unchanged
  3. A headless block-driver harness (`tests/BlockDriver.hpp`) runs the DSP core over sample blocks and asserts invariants: ±5V output bounds, frequency accuracy, phase continuity at reset, fixed-seed determinism
  4. Invariant tests pass at 44.1 / 48 / 96 kHz, establishing the behavioral baseline for later refactors
**Plans**: 4 plans
- [x] 22-01-PLAN.md — Rack-free doctest harness skeleton (`make test` + vendored doctest + smoke test) (TEST-01)
- [x] 22-02-PLAN.md — Extract RackCompat primitives + pure leaf headers (Waveshape w/ bleedLfo lift, RatioTable, Swing) (TEST-02)
- [x] 22-03-PLAN.md — Extract ClockTracker/DriftEngine/LfoCore, capture goldens, delegate shell, delete inline DSP (TEST-02, D-08)
- [x] 22-04-PLAN.md — BlockDriver + invariants @ 44.1/48/96 kHz + golden replay + GitHub Actions CI (TEST-04, D-09)
**Notes**: Per CONTEXT D-03/D-04 the FULL DSP core extraction (incl. DriftEngine) now lands in THIS phase, not Phase 24 — TEST-02 is effectively satisfied here. ⚠ Requirement-ownership update for Phase 24 / TEST-02 is surfaced for human confirmation in the 22-03 SUMMARY (do not assume re-scope silently). Load-bearing prerequisite per research Constraint A: the harness must exist before any DSP refactor or bug fix.

### Phase 23: Functional Bug Fixes
**Goal**: All four CODE-REVIEW functional bugs are fixed, each pinned by a regression test that fails on the old code and passes on the new — with the x1.5/÷1.5 behavior change confirmed by an in-Rack listening test first.
**Depends on**: Phase 22 (harness must exist so each fix lands as a regression test)
**Requirements**: BUG-01, BUG-02, BUG-03, BUG-04, TEST-03, TEST-05
**Success Criteria** (what must be TRUE):
  1. Clock tracker re-acquires after a >3× / <⅓× tempo jump (consecutive-outlier counting) — no permanent lockout; regression test asserts re-lock
  2. Phase dot tracks the trace and audio in free-running mode with swing set (effective display swing stored); patch load survives malformed/corrupt JSON without crashing (type-guard + non-throwing parse)
  3. **AUDITION GATE:** an in-Rack listening session for x1.5 / ÷1.5 is completed and the keep-current-vs-adopt-table decision is recorded in the milestone log BEFORE the alignment change and its regression test land
  4. x1.5 / ÷1.5 align per the auditioned decision (no mid-cycle truncation if the table is adopted), pinned by a deterministic regression test
  5. Unit tests cover waveshaping output ranges, the clock ratio/alignment table, consecutive-outlier clock recovery, and swing math (TEST-03); every fix #1–#3 is pinned by a regression test (TEST-05)
**Plans**: 5 plans
- [x] 23-01-PLAN.md — BUG-01 clock consecutive-outlier recovery + RED→GREEN re-lock regression (Wave A)
- [x] 23-02-PLAN.md — BUG-04 non-throwing parseSeedHex + dataFromJson hardening + BUG-03 gated display-swing store (Wave A)
- [x] 23-03-PLAN.md — TEST-03 unit coverage: waveshape range grid, ratio/alignment table, swing math (Wave A)
- [x] 23-04-PLAN.md — BUG-02 in-Rack x1.5/÷1.5 audition gate (BLOCKING human-verify; logs decision to STATE.md) (Wave B)
- [x] 23-05-PLAN.md — BUG-02 BEATS_PER_ALIGN alignment change + cadence regression, gated by the audition decision (Wave C)
**Human verification gate**: yes — BUG-02 cannot be implemented until the in-Rack audition decision is logged (research Pitfall 7).

### Phase 24: DSP Extraction + Display Refactors
**Goal**: The full DSP layer is extracted into `src/dsp/*.hpp`, the Rack shell is a thin pass-through, and the display/thread cleanups land — all verified output-preserving by the green harness.
**Depends on**: Phase 22 (harness green), Phase 23 (bugs fixed first, so refactors introduce no new variables)
**Requirements**: TEST-02 (full extraction), CLEAN-01, CLEAN-02, CLEAN-03, CLEAN-04, CLEAN-05
**Success Criteria** (what must be TRUE):
  1. All pure DSP logic lives in `src/dsp/*.hpp` (`Waveshape`, `Swing`, `DriftEngine`, `LfoCore`, etc.); `AnalogLFO.cpp` `process()` is a ~20-line pass-through; the `ouLayers[0].state` bleed dependency is lifted to an explicit `bleedLfo` parameter
  2. Headless harness output is identical (within epsilon) pre/post extraction at 44.1 / 48 / 96 kHz — proof the refactor preserved behavior
  3. Display animations are frame-rate-independent via clamped wall-clock `dt` (CLEAN-04); a pathological large-`dt` test confirms animation advances only by the clamped amount
  4. Display buffer regeneration runs on the GUI thread behind the existing atomics + double-buffer (CLEAN-05); the swing-zeroing gate moves with it
  5. Dead code removed (`drawZeroCrossing`, `scanlineImage` — CLEAN-01), unreachable `isStill` resolved (CLEAN-02), and ratio/BPM pills fade out symmetrically with the SYNC badge on clock disconnect (CLEAN-03)
**Plans**: 4 plans
- [x] 24-01-PLAN.md — Extract pure Rack-free helpers `src/dsp/DisplayFill.hpp` + `src/dsp/Anim.hpp` and pin them headless (`tests/test_display.cpp`, `tests/test_anim.cpp`) (CLEAN-05, CLEAN-04 — Wave 1)
- [x] 24-02-PLAN.md — CLEAN-05: move the display fill off the audio thread behind a seqlock snapshot (capture-at-trigger `bleedLfo`); GUI `step()` runs `fillFromSnapshot` (Wave 2)
- [ ] 24-03-PLAN.md — CLEAN-04 dt animations + CLEAN-03 ratio/BPM pill fade cache + CLEAN-01/02 dead-code/`isStill` removal (Wave 3)
- [ ] 24-04-PLAN.md — Manual in-Rack UAT (animation feel, pill symmetry, audio-thread relief, `getLastFrameDuration` probe) logged to STATE.md (BLOCKING human-verify — Wave 4)
**UI hint**: yes
**Scope note**: ROADMAP criteria #1 (full extraction / `process()` thinning) and #2 (DSP output identical) were already satisfied in Phase 22 (D-02/D-04/D-05, golden harness green) — TEST-02 owned there. Phase 24 = CLEAN-01..05 + the D-06 verification only. The "RNG strategy" open decision is STALE (resolved Phase 22 D-07).

### Phase 25: Release IP Hardening (PRIVATE)
**Goal**: The repository is legally clean for public GPL release — trial fonts purged from all git history while still private and verified gone, with LICENSE, NOTICES, and confirmed asset provenance.
**Depends on**: Phase 24 (history rewrite happens on a quiet tree with no code in flight). MUST execute while the GitHub repo is still PRIVATE.
**Requirements**: IP-01, IP-02, IP-03, PKG-01, PKG-04
**Success Criteria** (what must be TRUE):
  1. `BCBarellTEST-Regular.otf` and `FoundationLogo.ttf` are removed from the working tree and gitignored (IP-01)
  2. Both trial fonts are purged from ALL git history via `git filter-repo --invert-paths` on a fresh clone, force-pushed to the still-private remote with `--all --tags` (IP-02)
  3. Purge is VERIFIED clean via a fresh re-clone of the remote: `git rev-list --all --objects | grep -iE 'Barell|FoundationLogo'` returns empty (IP-02 — hard gate for Phase 28)
  4. A GPL-3.0 `LICENSE` file exists at repo root and the Makefile `DISTRIBUTABLES` picks it up (PKG-01)
  5. A NOTICES/credits file inventories every shipped third-party asset's license (JetBrains Mono OFL, panel SVG font outlines), and SVG/panel art font-outline provenance is confirmed acceptable for public GPL release (PKG-04, IP-03)
**Plans**: TBD
**Open question at phase start**: SVG font-outline provenance (Bebas Neue / Chakra Petch baked into `res/AnalogLFO.svg`) — confirm OFL Google Fonts cuts, not trial/commercial cuts (research HIGH-stakes open question).

### Phase 26: VCV Library Compliance + Packaging
**Goal**: The manifest is validated and submission-ready, a verified `.vcvplugin` artifact is produced, and the full test suite runs green in CI.
**Depends on**: Phase 24 (final code state), Phase 25 (LICENSE + clean assets present)
**Requirements**: PKG-02, PKG-03, PKG-05, TEST-06
**Success Criteria** (what must be TRUE):
  1. `plugin.json` `authorUrl`, `pluginUrl`, and `sourceUrl` (GitHub project page) are populated and reachable (PKG-02)
  2. Manifest validated field-by-field for submission: permanent slug confirmed, `version` stays `2.0.0` (MAJOR = Rack major; NOT "fixed" to 1.4.x), Makefile `VERSION` matches, tags valid, module slug/name correct, `minRackVersion` decision documented, no trademarked strings (PKG-03)
  3. `make dist` produces a `.vcvplugin` artifact that, when unzipped, contains the binary, `plugin.json` with populated URLs, `res/`, and `LICENSE` — and no trial fonts (PKG-05)
  4. The full test suite runs green as a GitHub Actions CI check on push (TEST-06)
**Plans**: TBD
**Open decision at phase start**: `minRackVersion` — lower from 2.6.0 to 2.0.0 unless a Rack 2.6 API is confirmed required (one-time grep). Also: "Forge" brand/slug collision check against the VCV Library.

### Phase 27: Notion Manual
**Goal**: A complete, publicly shared user manual lives in Notion and is linked from the manifest.
**Depends on**: Conceptually independent — drafting may begin during any earlier phase; must be published before Phase 28. Listed as a discrete phase. CV voltage ranges verified against `AnalogLFO.cpp` before publishing.
**Requirements**: DOC-01, DOC-02, DOC-03
**Success Criteria** (what must be TRUE):
  1. A new top-level Notion page exists with a subpage-per-section structure (DOC-01)
  2. The manual covers all table-stakes sections: 3-axis engine concept, annotated panel, control-reference table, I/O reference, context-menu options, clock/sync behavior, patch examples, install (Library + manual), changelog, license/credits (DOC-02)
  3. The manual is shared to web (public) and its URL is linked from `pluginUrl`/`manualUrl` in the manifest (DOC-03)
  4. No trademarked synth names appear as feature names in the manual prose (Pitfall 4)
**Plans**: TBD
**UI hint**: yes

### Phase 28: Publish + Submit
**Goal**: The source repo is public and the plugin is submitted to the VCV Library via the correct issue-based mechanism with an exact commit hash.
**Depends on**: Phase 25 (purge verification PASSED — hard gate), Phase 26 (manifest + dist ready), Phase 27 (manual published and linked)
**Requirements**: PUB-01, PUB-02
**Success Criteria** (what must be TRUE):
  1. The GitHub repo is flipped to public ONLY after the Phase 25 purge verification passed; a final fresh-clone grep confirms history is clean (PUB-01)
  2. A release commit is tagged (`v2.x.y`) and its exact `git rev-parse HEAD` hash is recorded
  3. One VCV Library submission issue is opened, titled with the plugin slug `ForgeAudio-AnalogSeries` (not the name), containing `sourceUrl`, license, manual URL, and the exact commit hash (not a branch name) (PUB-02)
  4. The submission issue URL is recorded for all future update comments
**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 22 → 23 → 24 → 25 → 26 → 27 → 28
(Phase 27 Notion Manual may be drafted in parallel but must publish before 28.)

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 22. Test Harness Foundation | v1.4 | 4/4 | Complete    | 2026-06-14 |
| 23. Functional Bug Fixes | v1.4 | 5/5 | Complete   | 2026-06-14 |
| 24. DSP Extraction + Display Refactors | v1.4 | 2/4 | In Progress|  |
| 25. Release IP Hardening (PRIVATE) | v1.4 | 0/TBD | Not started | - |
| 26. VCV Library Compliance + Packaging | v1.4 | 0/TBD | Not started | - |
| 27. Notion Manual | v1.4 | 0/TBD | Not started | - |
| 28. Publish + Submit | v1.4 | 0/TBD | Not started | - |
