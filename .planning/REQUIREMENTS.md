# Requirements: Forge Audio Analog LFO — v1.4 Tempered

**Defined:** 2026-06-14
**Core Value:** The three-knob analog engine (morph, character, drift) that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback.

**Milestone goal:** Take the feature-complete Analog LFO to a publishable, VCV-Library-ready plugin — bugs fixed, tested, package compliant, manual written, source published. The LFO is feature-frozen; no new DSP features.

## v1 Requirements

Requirements for the v1.4 release. Each maps to exactly one roadmap phase.

### Testing

- [x] **TEST-01**: A `make test` target builds and runs a standalone doctest binary without disturbing the existing plugin build
- [x] **TEST-02**: Pure DSP logic is extracted into a Rack-independent header-only core (`src/dsp/*.hpp`) consumed by the plugin shell
- [ ] **TEST-03**: Unit tests cover waveshaping output ranges, the clock ratio/alignment table, consecutive-outlier clock recovery, and swing math
- [x] **TEST-04**: A headless block-driver harness runs the DSP core over sample blocks and asserts on output invariants (frequency accuracy, phase continuity at reset, ±5V output bounds, fixed-seed determinism)
- [ ] **TEST-05**: Each functional bug fix (#1–#3) is pinned by a regression test that fails before the fix and passes after
- [ ] **TEST-06**: The full test suite runs green and is wired as a GitHub Actions CI check on push

### Bug Fixes

- [ ] **BUG-01**: Clock tracker recovers from >3× / <⅓× tempo jumps via consecutive-outlier counting — no permanent lockout (CODE-REVIEW #1)
- [ ] **BUG-02**: x1.5 and ÷1.5 ratios align on correct beat boundaries without mid-cycle truncation, after an in-Rack audition confirms the change is desirable (CODE-REVIEW #2)
- [ ] **BUG-03**: Phase dot tracks the trace and audio in free-running mode when swing is set (store effective display swing) (CODE-REVIEW #3)
- [ ] **BUG-04**: Patch load survives malformed/corrupt JSON without crashing (type-guard + non-throwing parse, fall back to existing seed) (CODE-REVIEW #4)

### Cleanup

- [ ] **CLEAN-01**: Dead code removed — `drawZeroCrossing()` and the unused `scanlineImage` member (CODE-REVIEW #8)
- [ ] **CLEAN-02**: Unreachable `isStill` dim condition resolved (CODE-REVIEW #9)
- [ ] **CLEAN-03**: Ratio and BPM pills fade out symmetrically with the SYNC badge on clock disconnect (CODE-REVIEW #10)
- [ ] **CLEAN-04**: Display animations are frame-rate-independent via wall-clock timing (`getLastFrameDuration`) (CODE-REVIEW #11)
- [ ] **CLEAN-05**: Display buffer regeneration runs on the GUI thread, off the audio thread (CODE-REVIEW #12)

### Packaging

- [ ] **PKG-01**: GPL-3.0 LICENSE file present at repo root (CODE-REVIEW #5)
- [ ] **PKG-02**: plugin.json `authorUrl`, `pluginUrl`, and `sourceUrl` (GitHub project page) are populated (CODE-REVIEW #6)
- [ ] **PKG-03**: Manifest validated for submission — permanent slug confirmed, version stays `2.0.0`, tags valid, module slug/name correct
- [ ] **PKG-04**: A NOTICES/credits file inventories third-party asset licenses (shipped fonts, SVG panel art)
- [ ] **PKG-05**: `make dist` produces a verified `.vcvplugin` artifact (binary + plugin.json + `res/` + LICENSE)

### IP Hardening

- [ ] **IP-01**: Trial/proprietary fonts (`BCBarellTEST-Regular.otf`, `FoundationLogo.ttf`) removed from the working tree and gitignored (CODE-REVIEW #7)
- [ ] **IP-02**: Trial fonts purged from full git history via `git filter-repo`, force-pushed to the still-private remote, and verified clean via a fresh clone (CODE-REVIEW #7)
- [ ] **IP-03**: SVG/panel art font-outline and asset provenance confirmed acceptable for public GPL release

### Publication

- [ ] **PUB-01**: GitHub repository flipped to public only after IP-02 purge verification passes
- [ ] **PUB-02**: VCV Library submission issue opened — titled with the plugin slug, containing sourceUrl and the exact commit hash

### Documentation

- [ ] **DOC-01**: User manual published in Notion as a new top-level page with a subpage-per-section structure
- [ ] **DOC-02**: Manual covers all table-stakes sections — 3-axis engine concept, annotated panel, control-reference table, I/O reference, context-menu options, clock/sync behavior, patch examples, install (Library + manual), changelog, license/credits
- [ ] **DOC-03**: Manual is shared to web (public) and linked from `pluginUrl`

## v2 Requirements

Deferred to the v2.0 VCO milestone. Tracked but not in this roadmap.

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

Explicitly excluded from v1.4. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| New DSP features for the LFO | Module is feature-frozen; this milestone is hardening + release only |
| Manifest version "fix" to 1.4.x | VCV requires MAJOR = Rack major (2); `2.0.0` is correct and must not change |
| Linking the test harness against libRack | Drags in window/GL/`APP->` globals; extract a Rack-independent core instead |
| Booting a full Rack instance for integration tests | Unsupported and unnecessary; block-driver over the extracted core suffices |
| Lowering Rate param min to 0 for the `isStill` fix | Wider blast radius than needed; resolve #9 by comparing effective frequency or deleting the condition |
| Renaming the plugin/module slug | Permanent once published; lock the current slug pre-submission |
| Custom Notion site / custom domain hosting | Public Notion share is sufficient for v1.4; revisit later |

## Traceability

Which phases cover which requirements. Populated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| TEST-01 | Phase 22 | Complete |
| TEST-02 | Phase 22 (scaffold) + Phase 24 (full extraction) | Complete |
| TEST-03 | Phase 23 | Pending |
| TEST-04 | Phase 22 | Complete |
| TEST-05 | Phase 23 | Pending |
| TEST-06 | Phase 26 | Pending |
| BUG-01 | Phase 23 | Pending |
| BUG-02 | Phase 23 (audition-gated) | Pending |
| BUG-03 | Phase 23 | Pending |
| BUG-04 | Phase 23 | Pending |
| CLEAN-01 | Phase 24 | Pending |
| CLEAN-02 | Phase 24 | Pending |
| CLEAN-03 | Phase 24 | Pending |
| CLEAN-04 | Phase 24 | Pending |
| CLEAN-05 | Phase 24 | Pending |
| PKG-01 | Phase 25 | Pending |
| PKG-02 | Phase 26 | Pending |
| PKG-03 | Phase 26 | Pending |
| PKG-04 | Phase 25 | Pending |
| PKG-05 | Phase 26 | Pending |
| IP-01 | Phase 25 | Pending |
| IP-02 | Phase 25 | Pending |
| IP-03 | Phase 25 | Pending |
| PUB-01 | Phase 28 | Pending |
| PUB-02 | Phase 28 | Pending |
| DOC-01 | Phase 27 | Pending |
| DOC-02 | Phase 27 | Pending |
| DOC-03 | Phase 27 | Pending |

**Coverage:**
- v1 requirements: 28 total
- Mapped to phases: 28 ✓
- Unmapped: 0

**Note on TEST-02:** The Rack-independent core extraction is staged across two phases — Phase 22 extracts the minimal core needed to drive the harness; Phase 24 completes the full `src/dsp/*.hpp` layer and thins the Rack shell. TEST-02 is considered satisfied when Phase 24 completes. Primary owning phase: Phase 24.

---
*Requirements defined: 2026-06-14*
*Last updated: 2026-06-14 — roadmap created, traceability populated (28/28 mapped to Phases 22-28)*
