---
gsd_state_version: 1.0
milestone: v2.0
milestone_name: Forge Analog VCO
current_phase: 30
current_phase_name: vcocore-skeleton-module-registration
status: executing
stopped_at: Completed 30-08-PLAN.md
last_updated: "2026-07-29T03:09:06.382Z"
last_activity: 2026-07-29
last_activity_desc: Phase 30 execution started
progress:
  total_phases: 8
  completed_phases: 1
  total_plans: 15
  completed_plans: 13
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
Plan: 8 of 10 (30-08 complete; 30-09 and 30-10 remain)
Status: Ready to execute 30-09
Last activity: 2026-07-29 — 30-08 gap closure complete (CR-01 guard fix + WR-03 coverage)

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
- [Phase 30]: CORE-01 invariants are measured on the OUTPUT, never on tel telemetry — and the D-16 pitch case is labelled NOT the TEST-02 tracking gate in both the file banner and the case name — A telemetry assertion only re-reads the number step() computed three lines earlier and stays green through a dead accumulator. Measured results reproduce the researcher's figures to six digits (divergence 0.233229/0.233235/0.233187 V, worst-case magnitude 5.51803 V at all three rates), confirming the landed step() body is the prototype the margins were measured against. Worst pitch error in the grid is 0.0078 %, better than one cent, which is exactly why the NOT-TEST-02 label is written twice.
- [Phase 30]: tests/test_vco_core.cpp had to be registered in check_includes.sh [1/7] VCO_SIDE_ALLOW — a new VCO test TU is LFO-side by default and make guards exits 1 the moment it lands — Section [1/7] derives its LFO-side scan set as everything under src/, tests/ and tools/ MINUS the named VCO-side files. The new TU includes VcoBlockDriver.hpp -> VcoCore.hpp by construction, so the guard's own failure text names the remedy. Same kind of file and same documented reason as tests/test_vco_harness.cpp since Phase 29. Exact-path match proved by a near-miss fixture that still fails. Unlike plan 30-01's [2/7] change this weakens no detector, but it was NOT operator-checkpointed — flagged for 30-07's phase gate.
- [Phase 30]: The researcher's 'sweepScenario maxes at exactly 5.0000 V' figure is BLOCK-LENGTH DEPENDENT, measured this session, and the source now records the table instead of repeating the sentence — Measured at all three rates: 5.000000 V at n=1024 and 0.05 s, 5.2104-5.2114 V at 0.25 s, 5.4383-5.4385 V at the 1 s block plan 30-03 specifies. Root cause is research's own explanation — sweepScenario anti-correlates morph=t with character=1-t, so whether the accumulator reaches peak phase while morph is still near zero depends on block length. This strengthens the case for the fixed morph 0/character 1 scenario: a sweep-only bound test has a margin that silently changes when anyone edits the block length.
- [Phase 30]: The VCO has a Rack body — src/AnalogVCO.cpp is a four-control shell (V/OCT, MORPH, CHARACTER, OUT) that computes NOTHING: it fills forge::VcoInputs by field assignment, delegates every sample to core.step(in), and writes the voltage back, which is the only reason make test remains evidence about what Rack produces. Stock RoundBlackKnob/PJ301MPort widgets (D-08) keep src/AnalogLFO.cpp out of the milestone diff entirely. Model factory under the permanent slug ForgeAnalogVCO (D-01); registration is plan 30-06's, so the symbol exists, the plugin links, and the browser stays empty — the intended intermediate state. Durable control geometry: MORPH (30.48, 40), CHARACTER (60.96, 40), V/OCT (30.48, 100), OUT (60.96, 100) mm on a 91.44 x 128.5 mm (18.00 HP) panel.
- [Phase 30]: The new TU joined make strict (3 -> 4 translation units), the plugin link and the CI MinGW compile-and-link loop with ZERO Makefile or CI wiring added — src/AnalogVCO.cpp was pre-registered in check_includes.sh VCO_SIDE_ALLOW (line 281) in Phase 29 before the file existed, so make guards was green on its first run, unlike 30-03's tests/test_vco_core.cpp. No guard-script edit was needed or made, and check_frozen.sh's FROZEN_EXPECTED_ENTRIES stayed at 15 for the new res/ asset.
- [Phase 30]: The Phase-29 compile canary SURVIVED the one moment retiring it looked reasonable (T-30-10) — the three-of-eight VcoInputs field asymmetry (the shell feeds pitchCV/morph/character; src/vco_compile_canary.cpp feeds all eight, which is what makes check_canary.sh [2b/5] report eight fields runtime-live at -O3) is now written into BOTH src/AnalogVCO.cpp::process() and the .github/workflows/test.yml canary comment, so the argument survives either file being read alone. The stale CI sentence that scoped the canary to a world without src/AnalogVCO.cpp is gone; the workflow diff was comment-only, all 10 steps unrenamed and unreordered.
- [Phase 30]: src/AnalogVCO.cpp's banner names the four forbidden C++ constructs by DESCRIPTION, not literal spelling — the plan requires the banner to state the C++11 rules AND requires a negative grep for those exact literals to return 0. Same trap class as 30-02's canary-matcher collision: a file that must document a rule it is simultaneously being grepped against. Same reason dataToJson is called "patch-state serialization" and the coordinates are not repeated in prose (mm2px must appear exactly four times).
- [Phase 30]: CORE-03 is closed BEHAVIORALLY: two differently-seeded VcoCore instances driven interleaved sample by sample each reproduce their solo block bit-exactly (0/1024 mismatches, both instances, all three rates), and the check is validated on every run by a permanent DeliberatelyBrokenSharedStateCore measured at 512/512 — Absence of shared state is what a source-text guard proves badly - a grep for static misses a function-local static, a shared reference member, a singleton behind an accessor and a shared pointer. Sensitivity is MEASURED not argued: with phase made a shared static the case exits 1 with 1024/1024 mismatches on both instances at all three rates. v2.1 POLY-01 is now an evidenced additive shell change
- [Phase 30]: The interleave helper is TEMPLATED over the core type so the positive control exercises byte-identically the drive loop the real check uses, and the case REQUIREs the helper's solo block bit-identical to VcoBlockDriver::run() before asserting anything else — A control that runs its own copy of the loop proves nothing about the loop under test - the check_includes.sh [6/7] argument (every negative control calls the SAME function its section calls). The validity-first REQUIRE is the nc2_direct habit: a helper that quietly stopped overwriting sampleTime/sampleRate would otherwise pass everything below it. Proved: the static-phase probe made that REQUIRE fire first
- [Phase 30]: The D-17 sensitivity probe used 'static inline double phase', and the control asserts totalMismatch > 0 rather than an exact count — A bare in-class 'static double phase = 0.0;' is ill-formed in BOTH C++11 and C++17 (in-class initializers on static members need const integral or inline), so the plan's literal splice would have produced a compile error - which proves nothing about whether the assertions can see the defect. And the control's own solo baselines are polluted by design (all four helper runs share the one static), so an exact count would pin an accident of run order: measured 512/512 here vs the researcher's 511/512
- [Phase 30]: The VCO is REGISTERED: extern in src/plugin.hpp, addModel in src/plugin.cpp init(), and a second plugin.json modules[] element under the operator-approved permanent slug ForgeAnalogVCO
- [Phase 30]: Assert byte identity by reading BYTES, not by counting git diff markers
- [Phase 30]: Phase 30 closed on OBSERVED CI evidence, not local green: run 30407971115 on SHA 7933fae36ad98882ac8964f17d6c1b15f60087fd, toolchain-gate = success AND its 'win-x64 leg reproduction' step's OWN conclusion = success ('win-x64 link gate: PASS') — The job's conclusion is never sufficient: a step that fail-fasts upstream is reported 'skipped', which scans as 'not red' in a job summary. The run was located BY SHA, never by recency. Phase 36 stands on this record for the tag and the #929 update, and must re-observe on whatever commit IT tags. Full local gate (72/72/0, strict, guards, canary, goldens) was recorded as a PRECONDITION — Phase 29 measured that exact combination green on code that could not link.
- [Phase 30]: Every future in-Rack UAT must flush the WHOLE extracted plugin directory from dist/, not just plugin.dylib and res/ — a stale install is a stale PLUGIN VERSION, not a stale binary — Measured at the 30-07 gate: the extracted install was a complete v2.0.0 LFO-only plugin — plugin.json at version 2.0.0 with ONE modules[] entry, res/ without AnalogVCO.svg, and a Jul 9 dylib exporting only _modelAnalogLFO. The plan's Step 2 refreshes only res/ and plugin.dylib; executing it literally would have satisfied the hash assertion while Rack kept reading a one-module manifest, so the VCO still would not have appeared — the exact false negative the flush exists to prevent, reached THROUGH the safeguard. Correct form: rsync -a dist/ForgeAudio-AnalogSeries/ into plugins-mac-arm64/ForgeAudio-AnalogSeries/, then cp plugin.dylib. Phases 31-35 all end in an in-Rack check.
- [Phase 30]: Research assumption A5 is CLOSED — the Apple-clang-only CORE-01/CORE-03 tolerances hold under GCC/libstdc++ and MinGW g++ — The three-OS matrix reported 69 cases on Ubuntu/Windows vs 72 on macOS. The gap is exactly the three #if defined(__APPLE__) drift-ON bit-exact golden cases in tests/test_golden.cpp (Phase-26 decision: portable drift-off goldens for 3-OS CI, drift-on macOS-gated because std::normal_distribution is not portable across standard libraries). No Phase-30 case was dropped: all 7 'vco harness:' and all 5 'vco core:' cases ran and passed on all three OSes, so 30-03's 1% pitch tolerance, its 6.0 V bound with the >5.1 V exercise assertion, its 0.01 V divergence threshold and 30-04's 0/1024 and 512/512 figures are confirmed cross-toolchain.
- [Phase 30]: Operator UAT sign-off 'Approved' — all four controls audibly live (D-07) and the shipped Analog LFO visually and audibly unchanged in the same session; NO timbre or output-level observation was raised — Recorded explicitly so Phase 32 reads its starting point as the crude aliased baseline AS DESIGNED rather than as an unreported problem, and so Phase 34 inherits no reported level problem. The expected-results block (crude/buzzy/harsh timbre; excursions above 5 V at high CHARACTER, measured 5.51803 V against a 5.55 V analytic ceiling; the deliberately ugly unlabelled panel) was presented in full before the reply, so its absence from the answer is an absence of complaint, not an absence of exposure. No observations were invented.
- [Phase 30]: All four items flagged for the phase gate resolved: [1/7] VCO_SIDE_ALLOW entry confirmed exact-path; PANEL-03 confirmed genuinely satisfied (not un-checked); the plugin.json diff-shape discrepancy reproduced and judged; invariant 5 confirmed green BY DETECTING — 1) tests/check_includes.sh:294 matches with [[ "${rel}" == "${a}" ]] — a QUOTED RHS, so literal comparison: no glob, no substring, no basename. It weakens no detector, unlike 30-01's [2/7] change. 2) All three PANEL-03 edits present (plugin.hpp:8, plugin.cpp:8, plugin.json:26); manifest still 2 modules, LFO first, version 2.0.1. 3) All four git diff algorithms render 0 deleted lines, not the predicted 1; the byte invariant holds directly (lines 1-23 identical, new[23] == old[23] + ','). 0 < 1 is the SAFE direction — the assertion was a ceiling on damage. 4) Invariant 5's -s output shows mismatchA/B := 512 and totalMismatch := 1024 at all three rates, so it passes BECAUSE it detected its own defect — which is what makes invariant 4 (0/1024) meaningful.
- [Phase 30]: CR-01 closed: the Nyquist floor now runs LAST so it is always the final writer
- [Phase 30]: forge::kVcoMaxDeltaPhase = 0.5 bounds the phase increment DIRECTLY, and is a different KIND of constant from kVcoNyquistGuardFrac
- [Phase 30]: WR-03 closed by a DRIVERLESS scenario - the bypass IS the coverage
- [Phase 30]: Green after a multi-part fix is not evidence that either part bites - each guard was proven load-bearing by a revert-one-only probe producing a DIFFERENT red
- [Phase 30]: A doctest -s before/after capture cannot be diffed raw - every SUCCESS line carries its own source line number

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
| Phase 30 P03 | 10 min | 3 tasks | 2 files |
| Phase 30 P05 | 3 min | 3 tasks | 3 files |
| Phase 30 P04 | 6 min | 2 tasks | 1 files |
| Phase 30 P06 | 4 min | 3 tasks | 3 files |
| Phase 30 P07 | 27 min | 3 tasks | 0 files |
| Phase 30 P08 | 9 | 3 tasks | 2 files |

## Session Continuity

**Resume file:** None

Last session: 2026-07-29T03:09:06.375Z
Stopped at: Completed 30-08-PLAN.md
Resume: run `/gsd-verify-work 29`, then `/gsd-discuss-phase 30` for VcoCore skeleton + module registration.

## Operator Next Steps

- Review the v2.0 roadmap (`.planning/ROADMAP.md` Phase Details, Phases 29-36).
- Note the TEST-02 reconciliation (mapped to Phase 31, the pitch phase, per research) and confirm or adjust.
- Then run `/gsd-plan-phase 29` to begin.
