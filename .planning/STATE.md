---
gsd_state_version: 1.0
milestone: v2.0
milestone_name: Forge Analog VCO
current_phase: 29
current_phase_name: vco-test-harness-lfo-non-regression-guardrail
status: executing
stopped_at: Completed 29-02-PLAN.md
last_updated: "2026-07-28T05:35:23.186Z"
last_activity: 2026-07-28
last_activity_desc: Phase 29 execution started
progress:
  total_phases: 8
  completed_phases: 0
  total_plans: 5
  completed_plans: 1
  percent: 0
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-06-14)

**Core value:** The three-knob analog engine (morph, character, drift) that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback.
**Current focus:** Phase 29 — vco-test-harness-lfo-non-regression-guardrail

> **⚠ MILESTONE GUARDRAIL — protect the shipped LFO.** No breaking/behavioral changes to the Analog LFO (live in VCV Library, golden-pinned) while adding the VCO. Prefer additive code over editing shared `src/dsp/` headers. Any LFO-regression risk (shared-header edits, plugin.json/version/registration) → surface to operator with impact + remediation options + a recommendation before acting. Tripwires: LFO `.f32` goldens + `make strict` + CI MinGW link leg. See PROJECT.md Constraints.

## Current Position

Phase: 29 (vco-test-harness-lfo-non-regression-guardrail) — EXECUTING
Plan: 3 of 5
Status: Ready to execute
Last activity: 2026-07-28 — Phase 29 execution started

## Performance Metrics

**Velocity (cumulative):**

- v1.0: 12 plans in 58 min (4.8 min avg)
- v1.1: 6 plans in 6 days (includes human verification sessions)
- v1.2: 8 plans in ~97 min
- v1.3: 14 plans, 20 tasks (2026-03-28 → 2026-06-13)
- v1.4: 27 plans, 54 tasks (2026-06-14 → 2026-07-10)

## Accumulated Context

### Roadmap Evolution

- v1.3 Forge Noir shipped: Phases 18-21 (24/24 requirements). LFO feature-complete.
- v1.4 Tempered shipped: Phases 22-28, continuous numbering from Phase 21. Release-hardening milestone, no new DSP features. 28/28 requirements. LFO live in VCV Library (#929, tag v2.0.0, commit 4d7b0a8).
- **v2.0 Forge Analog VCO roadmap created (2026-07-20): 8 phases (29-36), continuous numbering from Phase 28. Fine granularity. 40/40 v1 requirements mapped (source file's "37" was a stale miscount — corrected to 40 enumerated IDs during traceability update).**
- Phase order honors the unanimous four-agent research sequence: (1) test-harness + LFO-golden guardrail FIRST [29], (2) VcoCore skeleton + registration [30], (3) pitch/tuning/FM [31], (4) morph-aware polyBLEP/polyBLAMP — the isolated linchpin with its own iteration budget [32], (5) hard sync reusing the BLEP machinery [33], (6) audio-rate analog engine + drift recalibration + output stage [34], (7) shell/panel/display [35], (8) goldens/CI/library update [36].
- Guardrail encoded into phase design: only Phase 34 touches a shared header (`DriftEngine.hpp`, additive authority members, defaults = LFO literals, gated by byte-identical golden replay). All other VCO work is new files (`VcoCore.hpp`, `MorphBlep.hpp`, `VcoBlockDriver.hpp`). The standing LFO-golden + strict + MinGW canary is wired in Phase 29 and runs at the end of every later phase.
- Reconciliation note: research (PITFALLS + ARCHITECTURE invariant 1) assigns the < 1-cent V/Oct tracking test (TEST-02) to the pitch phase as its exit gate, so TEST-02 is mapped to Phase 31 (not the final ship phase) — the pitch feature is tested where it is delivered, not five phases later.

### Decisions pending at phase start (from research)

- **Phase 32 (RESOLVED-AT-PLAN):** alias-floor test threshold (target ≈ −60 dB rel. fundamental) to be pinned empirically once naive vs. band-limited renders exist to compare. Exact CHARACTER↔BLEP magnitude tuning is MEDIUM-confidence — flag `--research-phase` if the first spectral iteration is ambiguous.
- **Phase 34 (audition-gated, operator):** VCO drift-depth value (single-digit cents, ARCHITECTURE estimate ~0.3–1.8% max deltaPhase) — needs in-Rack audition, not calculation (DRIFT-03), matching the v1.4 x1.5/÷1.5 precedent.
- **Phase 34 (operator decision):** DC-blocker policy — accept DC on the audio output (some real analog VCOs do) vs. add a light ~5–20 Hz high-pass. Deliberate call, not a default inheritance of the LFO's DC-positive stance (OUT-02). Also decide whether the VCO uses drift `dcOffsetV` at all.
- **Phase 30 (operator confirm):** permanent VCO slug (immutable once users have patches) chosen at registration.
- **Phase 36 (verify at release):** precise VCV Library *feature-update* mechanics for adding a module to an already-live plugin (auto-pickup from manifest version vs. fresh action on #929) — MEDIUM-confidence; verify against current library docs before tagging. Manifest version stays Rack-major 2.x with a fresh tag.

### Decisions

All decisions logged in PROJECT.md Key Decisions table.
v1.0–v1.3 phase-level decisions archived in `milestones/` ROADMAP files.

Prior-milestone (v1.4) phase decisions retained below for reference:

- Phase 22: vendored doctest 2.4.11 harness; make test additive/Rack-free; pure DSP leaf headers (RackCompat/Waveshape/RatioTable/Swing) extracted to src/dsp/ verbatim/rack-free (D-05 bleed lifted to bleedLfo param); full LfoCore extraction proven bit-exact vs inline (D-08 gate); goldens frozen from the validated core.
- Phase 23: BUG-01 consecutive-outlier counter (threshold 3) in ClockTracker.hpp; BUG-04 non-throwing forge::parseSeedHex in dataFromJson; BUG-03 phase-dot swing gated to effective value; BUG-02 adopt-table BEATS_PER_ALIGN[15] two-cell swap (idx 6 /1.5 → 3, idx 8 x1.5 → 2), operator-auditioned, 13 other ratios bit-identical.
- Phase 24: fillDisplayBuffer + clampFrameDt/flashDecay pure headers; 256x display fill moved off audio thread via tear-free seqlock snapshot; three GUI cleanups; manual in-Rack UAT APPROVED 2026-06-30.
- Phase 25: trial fonts purged from all git history while PRIVATE (IP-02), verified clean via fresh mirror; res/AnalogLFO.svg text re-exported from confirmed-OFL Chakra Petch (IP-03).
- Phase 26: plugin.json submission-ready (manifest URLs, minRackVersion 2.0.0, version 2.0.0 Rack-major); portable drift-off goldens for 3-OS CI, drift-on macOS-gated.
- Phase 27: docs/ GitHub-Markdown manual (hub + 4 code-fact sections + install/changelog/license); manualUrl added.
- Phase 28: release ff-only to main, tag v2.0.0 (commit 4d7b0a81f7aabed83626a11951956fff173b6ad7); public flip gated on fresh-mirror CLEAN verdict; VCV Library submission #929 filed with full 40-char hash — the PERMANENT update thread (all future bumps are comments on #929).
- [Phase 29]: VCO POD is forge::VcoInputs, never a second forge::Inputs (cross-TU ODR hazard, R-9) — A duplicate forge::Inputs compiles silently in TUs including only one header and detonates on the CI MinGW link leg — the class that got v2.0.0 rejected
- [Phase 29]: tests/BlockDriver.hpp and tests/VcoBlockDriver.hpp stay independent files forever — never templated or subclassed — BlockDriver feeds the macOS bit-exact drift-ON golden leg of the shipped LFO; any change moves tests/golden/freerun_*.f32 (R-2/P-4)
- [Phase 29]: Phase 29 VcoCore::step() returns silence and a TOMBSTONE test asserts it; Phase 30 must delete that test — D-01 scopes Phase 29 to the boundary contract only; the tombstone forces Phase 30 to consciously revisit the weak-by-construction invariants
- [Phase 29]: D-04 golden digests are pinned as source literals in tests/test_lfo_guardrail.cpp, not in a data file — Changing a golden then requires a reviewed CODE diff; a data-file manifest could be regenerated silently alongside the fixtures
- [Phase 29]: The SHA-256 hasher is vendored in tests/Sha256.hpp and validated by a permanent negative control, never by a green run — Three published FIPS 180-4 vectors plus a one-byte-perturbed in-memory copy of a real golden; no external hashing tool (sha256sum is absent on macOS) and no new dependency

### Carried Forward (deferred from v1.3, non-blockers)

- `swingIndex` GUI→audio non-atomic write (pre-existing, predates Phase 18; common VCV menu-param pattern).
- Manual-only Nyquist validation on phases 18/19/20.1/21 (inherently human-gated visual/audio behaviors).

### Pending Todos

None — all v1.3/v1.4 todos resolved (see `.planning/todos/done/`).

### Blockers/Concerns

- None open for v2.0. The v1.4 IP/public-flip gates all CLEARED (repo PUBLIC 2026-07-10; #929 live). Full v1.4 blocker history archived in `milestones/v1.4-ROADMAP.md`.

## Deferred Items

| Category | Item | Status | Deferred At |
|----------|------|--------|-------------|
| Tech debt | `swingIndex` non-atomic GUI→audio write | Carried (non-blocker) | v1.3 close |
| Verification | Manual-only Nyquist validation (Phases 18/19/20.1/21) | Carried (human-gated) | v1.3 close |
| Verification (UAT) | Phase 23 BUG-03 manual in-Rack check — phase dot tracks trace in free-run with swing (automated regression covers the fix) | Acknowledged / deferred | v1.4 close (2026-07-10) |
| Verification (UAT) | Phase 23 BUG-04 manual in-Rack check — corrupt spreadSeed hex patch loads without crashing (automated red→green regression covers the fix) | Acknowledged / deferred | v1.4 close (2026-07-10) |
| Scope (v2.1) | Through-zero FM, phase distortion, oversampling (Off/2×/4×), tracking-error modeling | Deferred to v2.1 | v2.0 scoping |
| Scope (v2.1) | Polyphony (up to 16 voices) + per-voice drift seeding — enabled by CORE-03, additive shell change | Deferred to v2.1 | v2.0 scoping |
| Phase 29 P01 | 12min | 3 tasks | 3 files |
| Phase 29 P02 | 6 min | 3 tasks | 3 files |

## Session Continuity

**Resume file:** None

Last session: 2026-07-28T05:35:23.181Z
Stopped at: Completed 29-02-PLAN.md
Resume: run `/gsd-plan-phase 29` to plan the VCO test harness + LFO guardrail phase.

## Operator Next Steps

- Review the v2.0 roadmap (`.planning/ROADMAP.md` Phase Details, Phases 29-36).
- Note the TEST-02 reconciliation (mapped to Phase 31, the pitch phase, per research) and confirm or adjust.
- Then run `/gsd-plan-phase 29` to begin.
