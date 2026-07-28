---
gsd_state_version: 1.0
milestone: v2.0
milestone_name: Forge Analog VCO
current_phase: 30
current_phase_name: vcocore-skeleton-module-registration
status: executing
stopped_at: Completed 30-02-PLAN.md
last_updated: "2026-07-28T22:12:22.305Z"
last_activity: 2026-07-28
last_activity_desc: Phase 30 execution started
progress:
  total_phases: 8
  completed_phases: 1
  total_plans: 12
  completed_plans: 7
  percent: 13
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-06-14)

**Core value:** The three-knob analog engine (morph, character, drift) that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback.
**Current focus:** Phase 30 — vcocore-skeleton-module-registration

> **⚠ MILESTONE GUARDRAIL — protect the shipped LFO.** No breaking/behavioral changes to the Analog LFO (live in VCV Library, golden-pinned) while adding the VCO. Prefer additive code over editing shared `src/dsp/` headers. Any LFO-regression risk (shared-header edits, plugin.json/version/registration) → surface to operator with impact + remediation options + a recommendation before acting. Tripwires: LFO `.f32` goldens + `make strict` + CI MinGW link leg. See PROJECT.md Constraints.

## Current Position

Phase: 30 (vcocore-skeleton-module-registration) — EXECUTING
Plan: 3 of 7
Status: Ready to execute
Last activity: 2026-07-28 — Phase 30 execution started

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
- [Phase 29]: Phase 29 D-07 compile canary lives at src/vco_compile_canary.cpp (operator: option-a) — Covered for free by all four C++11/ODR gates via the existing src/*.cpp globs, so no build wiring can silently rot; identical to how Phase 30's AnalogVCO.cpp will be gated. Cost: one unused namespaced symbol forge::vcoCompileCanaryProbe ships in the released plugin binary, disclosed in the file banner.
- [Phase 29]: The compile canary must ODR-USE the VCO headers, not merely #include them — An include-only TU emits no code and is ODR-used by nothing, leaving the CI MinGW link leg nothing to resolve — permanently and silently green (P-1). A forward declaration plus a runtime-derived loop trip count ((i & 3) + 1) defeats dead-symbol elimination and constant folding; tests/check_canary.sh [2/5] asserts the emitted symbol via nm.
- [Phase 29]: The CI MinGW link gate is PROVEN to bite — run 30339957128 failed with 'undefined reference to forge::VcoCore::ODR_PROBE_TBL', green again after revert on run 30340075121 — ROADMAP criterion 3 is now demonstrated rather than asserted; the referencing object was vco_compile_canary.cpp.o, proving the canary's ODR-use design (P-1) works
- [Phase 29]: P-2 CORRECTED and widened — the ENTIRE local gate returned exit 0 on the deliberately broken commit, and the strict gate reported success on the Ubuntu runner too — make test, make strict, make guards and check_canary.sh all passed on code that could not link; -fsyntax-only never links, so no syntax-only gate on any platform can catch a link-class defect. Only the real-link step 6 caught it.
- [Phase 29]: No tag or VCV Library resubmission may be cut on local evidence alone — the CI toolchain-gate link leg must be observed green on the exact commit being tagged — Green local plus green make strict was precisely the state in which v2.0.0 was tagged and rejected; this phase reproduced that state deliberately and measured it
- [Phase 29]: The two P-7 TEST-01 rows (seam determinism, output finiteness) are recorded as green-but-weak, NOT coverage — They pass only because VcoCore::step() is silent by D-01 — determinism compares two all-zero blocks and isfinite(0.f) is trivially true. Phase 30 must re-evidence both when it deletes the TOMBSTONE case.
- [Phase 30]: Operator selected option-a (exact-path exemption) for the check_includes.sh [2/7] guard weakening, and confirmed the permanent slug ForgeAnalogVCO as specified — Both approvals were given on one surface before any Phase 30 commit existed (D-05). option-a keeps VcoCore.hpp including what it uses, ships a two-direction negative control in the same commit, ends the [2/7] vs check_canary.sh [5b/5] contradiction about RackCompat.hpp, and disarms the identical trap waiting for Phase 32 MorphBlep.hpp. The slug is a one-way door: display name "Analog VCO", tags "Voltage-controlled oscillator" + "Waveshaper", plugin.json version held at 2.0.1 (D-04). Plan 30-06 acts on this; Phase 36 needs it for the #929 update.
- [Phase 30]: The [2/7] Rack-free exemption is exact-path and is pinned by two mutation-proved controls inside [6/7], not by inspection — A widened-exemption mutant (bare [Rr]ack substring) and a removed-exemption mutant each make the guard exit 1 at the matching control. The fire-direction control alone would still pass under a substring widening; the ignore-direction control alone would still pass if the detector were deleted. Both are required to pin the exemption to its documented width.
- [Phase 30]: The VcoCore seam carries the researcher-measured naive oscillator verbatim (CORE-01): kVcoFreqC4 * exp2_taylor5(pitchCV), a NaN-safe zero test, a Nyquist clamp at 0.49 * sampleRate, a double-precision accumulate with a single-subtract wrap, one call into the frozen Waveshape::morphedWave with bleedLfo = 0, and an unconditioned x5 — the guard and the wrap are ONE invariant: without the clamp, pitchCV = +10 reaches phase 1,014,986 and -8,655,011 V while every sample stays isfinite, so no finiteness test can see it and plan 30-03's magnitude bound is what does
- [Phase 30]: D-11 divergence is the five-coefficient setSpreadSeed copy into Waveshape and nothing else — no OU drift stepping, no per-sample RNG draw, characterSpread deliberately not copied — which is exactly why all six shipped-LFO goldens stayed byte-identical through the DSP landing
- [Phase 30]: D-15 and D-19 are closed: the Phase-29 silence tombstone was INVERTED in place (same slot, still 7 harness cases) and OBSERVED red against a silenced core, failing both the not-silent and not-constant scans; the two rows Phase 29 booked as green-but-weak are re-evidenced under real DSP with the reason written in place
- [Phase 30]: check_canary.sh [2b/5]'s step matcher is UNANCHORED — quoting the full step() signature in a comment on a line that also contains a brace makes the canary perturb the COMMENT, and make guards hard-fails with unrelated 'unknown type name VcoInputs' errors. VcoCore.hpp's banner abbreviates the signature as float step(...) and documents the trap for future editors

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
| Phase 29 P03 | 6 min | 3 tasks | 3 files |
| Phase 29 P04 | 8 min | 3 tasks | 6 files |
| Phase 29 P05 | 16 min | 2 tasks | 2 files |
| Phase 30 P01 | 9 min | 3 tasks | 1 files |
| Phase 30 P02 | 6 min | 3 tasks | 2 files |

## Session Continuity

**Resume file:** None

Last session: 2026-07-28T22:12:22.298Z
Stopped at: Completed 30-02-PLAN.md
Resume: run `/gsd-verify-work 29`, then `/gsd-discuss-phase 30` for VcoCore skeleton + module registration.

## Operator Next Steps

- Review the v2.0 roadmap (`.planning/ROADMAP.md` Phase Details, Phases 29-36).
- Note the TEST-02 reconciliation (mapped to Phase 31, the pitch phase, per research) and confirm or adjust.
- Then run `/gsd-plan-phase 29` to begin.
