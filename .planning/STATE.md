---
gsd_state_version: 1.0
milestone: v2.0
milestone_name: Forge Analog VCO
current_phase: 32
current_phase_name: morph-aware-anti-aliasing-polyblep-polyblamp
status: executing
stopped_at: Completed 32-06-PLAN.md
last_updated: "2026-08-01T00:34:24.554Z"
last_activity: 2026-07-31
last_activity_desc: Phase 32 execution started
progress:
  total_phases: 8
  completed_phases: 3
  total_plans: 35
  completed_plans: 30
  percent: 38
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-06-14)

**Core value:** The three-knob analog engine (morph, character, drift) that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback.
**Current focus:** Phase 32 — morph-aware-anti-aliasing-polyblep-polyblamp

> **⚠ MILESTONE GUARDRAIL — protect the shipped LFO.** No breaking/behavioral changes to the Analog LFO (live in VCV Library, golden-pinned) while adding the VCO. Prefer additive code over editing shared `src/dsp/` headers. Any LFO-regression risk (shared-header edits, plugin.json/version/registration) → surface to operator with impact + remediation options + a recommendation before acting. Tripwires: LFO `.f32` goldens + `make strict` + CI MinGW link leg. See PROJECT.md Constraints.

## Current Position

Phase: 32 (morph-aware-anti-aliasing-polyblep-polyblamp) — EXECUTING
Plan: 7 of 11
Status: Ready to execute
Last activity: 2026-07-31 — Phase 32 execution started

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
- [Phase 30]: The T-30-02 seed literals are byte-unchanged (sha256 ba3ec29a identical before/after) — only the comment above them claimed a property the module does not have. Every AnalogVCO in a patch is a bit-identical clone at 0 of 2048 differing samples, and the corrected comment now scopes tests/test_vco_core.cpp's divergence invariants as evidence about forge::VcoCore, NOT about the shipped module. Per-instance entropy plus patch persistence is Phase 34/35's, tracked as deferred item 2 with the shipped LFO's draw/reject-(0,0)/persist/non-throwing-parse pattern and a MUST re-validate-on-deserialize requirement attached.
- [Phase 30]: Plan 30-09's Task-1 gate folded the LFO SOURCE FILENAME into the same zero-count as the LFO model symbol and slug, which is unsatisfiable by construction — plan 30-05 deliberately wrote two AnalogLFO.cpp mentions into the D-08 banner explaining why stock SDK widgets are used. Split into ForgeAnalogLFO|modelAnalogLFO = 0 (the landmine's canonical list) plus AnalogLFO.cpp unchanged at its baseline of 2 (the landmine's actual prose: do not ADD a mention). Same failure class as 30-08's doctest line-number diff — a gate whose regex is wider than the prose it encodes.
- [Phase 30]: Phase 30's gap closure is CLOSED on OBSERVED CI evidence: run 30419429579 on SHA 0cf5f82e12148b7dc096a3fc1c89a4d8ecc6a820, toolchain-gate job success AND its 'win-x64 leg reproduction (compile + full link vs libRack)' step's OWN conclusion = success ('win-x64 link gate: PASS') — This is the first gate measured on the COMBINED tip of 30-08 and 30-09 — 30-08 gated its own two commits and 30-09 its own two, and the composition had never been measured. The run was located BY SHA, never by recency, and every returned run's headSha was compared to the pushed SHA before any conclusion was read. The job conclusion is recorded but never sufficient. The full local gate (72/72/0 at 2,616,064 assertions, guards with and without a real RACK_DIR, strict over four TUs, frozen, canary, and a real Rack-SDK link) is recorded as a PRECONDITION: Phase 29 measured that exact combination green on code that could not link.
- [Phase 30]: The plan's own CI-run selector was unsatisfiable: it matched select(.name|test("toolchain";"i")) against the RUN name, but toolchain-gate is a JOB inside the single workflow named 'test' — The selector returns an empty RID, so the block's own 'test -n $RID' fails regardless of CI health. Corrected to select by headSha equality — strictly MORE faithful to landmine 1 (locate BY SHA, never by name or recency) and what the task prose already asks for ('extract, across all jobs, the step whose name is ...'). Third occurrence in one gap-closure wave of the same failure class as 30-08's doctest line-number diff and 30-09's LFO-filename zero-count: a gate whose mechanism does not match the prose it encodes. The prose was correct all three times; the mechanism was not.
- [Phase 30]: The three-OS matrix gap is discharged by ACCOUNTING for it, not by asserting it is expected: 72 macOS vs 69 Ubuntu/Windows is exactly 3 cases AND exactly 24,582 assertions — tests/test_golden.cpp contains exactly three #if defined(__APPLE__) TEST_CASEs (the drift-ON bit-exact goldens at 44.1k/48k/96k, Phase-26 decision: std::normal_distribution is not portable across standard libraries), and those three cases were measured locally at exactly 24,582 assertions — matching both matrix deltas on both axes. Nothing else was dropped, so all 5 'vco core:' and all 7 'vco harness:' cases ran on all three legs. That is what confirms 30-08's hostile-timing scenario four — the first NaN and negative-zero-adjacent float comparisons the VCO suite has ever run, kept stable by -ffp-contract=off — cross-toolchain rather than on Apple clang alone.
- [Phase 31]: Phase 30 deferred item 5 (WR-05) folded into plan 31-01 rather than re-deferred — The item's own Resolve-at named 'the next phase that touches tests/check_includes.sh', and D-23 made Phase 31 that phase. Re-owning the deferral would have left it pointed at a condition already satisfied - the same false-comment class plan 30-08 existed to remove.
- [Phase 31]: The anchored [2/7] exclusion deliberately permits a trailing // or /* comment — src/dsp/VcoCore.hpp:74 carries '// forge::exp2_taylor5, forge::clamp' on its real exempted line, so forbidding trailing comments would have turned [2/7] red on the LIVE VCO header. The anchor forbids only a PRECEDING directive, which is the whole of the WR-05 evasion. It also permits the grep -n line-number prefix because the exclusion runs downstream of grep -nE.
- [Phase 31]: TEST-02 deliberately NOT marked complete by plan 31-01 despite being in its frontmatter — TEST-02 reads 'V/Oct tracking accuracy is asserted (< 1 cent error) across the pitch range'. Plan 31-01 only pre-registers the test file's allowlist entry; the assertions land in 31-05/31-06/31-07. Marking it now would reproduce exactly the false green Phase 30 deferred item 1 recorded for PANEL-03. The phase gate confirms TEST-02 after 31-07.
- [Phase 31]: kVcoNyquistGuardFrac settled at 0.495f (PITCH-04 / D-11) with derived ceilings, crossover volts and D-10's hard-clamp decision in the source — The constant's own comment named Phase 31 as its replacement; leaving it provisional would have kept PITCH-04's policy undecided while three separate comments pointed forward at work already done
- [Phase 31]: PITCH-04 NOT marked complete at 31-02 — 31-07 owns the pitch-driven Nyquist assertion — 31-02 settles the policy constant but asserts no clamp behavior; 31-RESEARCH grades PITCH-04 coverage partial because the existing freqNyquistBounded pin is driven by hostile timing, not hostile pitch. Marking it here would repeat the PANEL-03 false green
- [Phase 31]: kVcoMaxDeltaPhase's cross-reference de-tensed alongside the margin arithmetic (Rule 2), value and type untouched per D-12 — The comment instructed a future Phase 31 to leave the constant alone; once Phase 31 had, that instruction was a false forward-reference in the very file plan 30-08 existed to clean
- [Phase 31]: kVcoMaxPitchVolts = 64.f bounds the summed pitch volts before the single exp2 — power of two (no rounding in the comparisons), 2.2x outside the reachable musical worst case (29.08 V), 2.0x inside the frozen helper's UB boundary, and FINITE at both extremes where 120/126 hand the downstream ceiling an infinity
- [Phase 31]: the D-14 bound uses the negated-comparison idiom with the negated line FIRST as the NaN catcher; forge::clamp is rejected BY NAME in the source because both of its comparisons are false for a NaN (closes deferred item 3 / CR-02)
- [Phase 31]: the D-22 RED evidence is a one-shot UBSan probe (RackCompat.hpp:106:24 float-cast-overflow + :109:11 left-shift), NOT a behavioral case — the same probe run reproduced the measurement that every behavioral assertion was already GREEN pre-fix. No permanent sanitizer gate, because the SHIPPED LFO shares the identical latent UB via AnalogLFO.cpp:320 -> LfoCore.hpp:183-184 (D-24)
- [Phase 31]: no requirement marked complete — third consecutive plan in phase 31 declining a false green. PITCH-01/05 await 31-05; PITCH-02/03 and FM-01/02/03 await 31-04's controls plus 31-06's assertions; PITCH-04 awaits 31-07
- [Phase 31]: tel.freqHz for hostile pitch moved from 0 to 1.41828e-17 — the guarded value is POSITIVE so the negated frequency floor correctly does not fire (D-13's stated intent). A 'tel.freqHz == 0 for hostile pitch' assertion in 31-07 would now FAIL
- [Phase 31]: 31-04 — Rack's default display precision left alone; the divergence from D-04's illustrative digit count (5 significant digits, +2.0000 oct) is recorded in src/AnalogVCO.cpp beside the FINE declaration rather than silently absorbed
- [Phase 31]: 31-04 — both FM fields forwarded UNCONDITIONALLY from the shell; the shipped LFO's zeroing ternary (src/AnalogLFO.cpp:320) is named in the source as the anti-pattern with three independent reasons (D-09/D-17)
- [Phase 31]: 31-04 — the shell's canary argument now leads with STRUCTURE (the canary is the TU the guard compiles against a perturbed VcoCore header, and the only VCO TU link-checkable without the Rack SDK) and demotes the field count to corroboration — the margin narrowed to 7-of-8 vs 8-of-8 with drift the sole gap, and Phase 34 closes it
- [Phase 31]: 31-04 — no requirement marked complete (PITCH-02, PITCH-03, FM-01, FM-02 stay Pending) — FOURTH consecutive plan in phase 31 declining, because 31-06 owns the behavioral assertions that make them non-vacuous
- [Phase 31]: 31-08 phase gate: TEST-02 discharged as a HARD gate - all six selectors green AND each proven to have matched its exact expected case count (2/1/1/2/1/1) — A negative control on the same binary shows a selector matching ZERO cases also exits 0 and prints Status: SUCCESS!, so exit status cannot distinguish a gate that passed from a gate that never ran
- [Phase 31]: 31-08: the three-OS matrix and the Windows link leg observed green BY HASH EQUALITY - run 30511183170 on 80fb90a, toolchain-gate success AND step 6 win-x64 leg reproduction OWN conclusion success — The pre-phase commit da266bb had its OWN green run sitting one line above it in gh run list, which is exactly why recency would have been wrong; and a job conclusion alone is insufficient because a step that fail-fasts upstream reports skipped
- [Phase 31]: 31-08: the matrix case-count gap is ACCOUNTED FOR by measuring BOTH sides - +9 cases and +1,941 assertions on every leg identically (macOS 72->81, Ubuntu/Windows 69->78) — The macOS-vs-others gap is unchanged at exactly 3 cases / 24,582 assertions before AND after, matching the three macOS-gated drift-ON goldens measured locally. A per-leg BEFORE/AFTER delta is evidence that all nine new cases ran everywhere; an absent #if in the new file is only an argument
- [Phase 31]: 31-08: the std::pow-under-src prohibition is UNSATISFIABLE as written and must be scoped to the VCO seam or to a baseline comparison in future plans — src/dsp/Anim.hpp:40 is a frozen, byte-pinned, shipped-LFO header containing std::pow, absent from the phase diff and unchanged from baseline. Three further criterion artifacts were reported the same way: inline constexpr (3 raw hits, all comment lines forbidding it), static constexpr (14 non-comment hits, mostly the PERMITTED namespace-scope form), and #if defined(__APPLE__) (2 directives gating 3 cases)
- [Phase 31]: 31-08: D-24 recorded in deferred-items.md item 1 pointed at NO PHASE, marked a guardrail event and unfixed by decision; the clamp-helper item recorded HALF closed — Both shipped call sites read this session: AnalogLFO.cpp:320 -> LfoCore.hpp:183-186 -> RackCompat.hpp:106/:109. Consequence recorded: a permanent repo-wide UBSan gate cannot be adopted while it stands. The clamp helper is half closed - the pitch-volt half honoured the local-to-VcoCore constraint, the morph/character half stays pointed at the phase adding their CV inputs
- [Phase 32]: D-16 honoured: MORPH CV jack + attenuverter declared in Phase 32, correcting Phase 31's CONTEXT lumping; CHARACTER's CV stays in Phase 34 (CHAR-01)
- [Phase 32]: D-17 honoured: forge::VcoInputs gains ZERO fields; the shell conditions knob + CV x attenuverter into the POD's documented [0,1], protecting the compile canary's one-field margin
- [Phase 32]: forge::clamp rejected by name at the MORPH CV boundary: both of a ladder's comparisons are false for a NaN, so the negated-comparison pair is used instead (T-32-01)
- [Phase ?]: 32-03: kThresholdFloorDb = -75 dB pins the tightest threshold the spectral apparatus can honestly assert, derived from plan 32-01's worst measured leakage row (-91.95 dB at 44.1 kHz C9); it is a static constant so the D-10 self-check can still fail
- [Phase ?]: 32-03: measureCellDb escalates the bin-centre solver per cell (method one unless that cell's own D-10 bar demands method two); 76 of 90 cells use method one, 14 escalate
- [Phase ?]: 32-03: the alias-floor gate was observed RED on 32 of 45 gated cells against the live naive forge::VcoCore and pinned as a tombstone; kNaiveFailuresFloor = 27 is the observed 32 minus 5 and must not be tightened to the observed count
- [Phase ?]: 32-03: the 13 gated cells that already pass naive are the P-6 population (sine, and high-character cells where the D-03 factor correctly returns zero) and must not be 'fixed' by tightening their thresholds
- [Phase 32]: MorphBlep's dt guard gained an upper bound: a +infinity dt passed the lower-bound-only guard, reached the divisor, and left pending = NaN permanently — Measured RED against the 32-04 header; the other five hostile dt classes were already correct. Bound is 1.0, not kVcoMaxDeltaPhase, so it provably cannot fire on a legitimate input.
- [Phase ?]: 32-06: forge::VcoCore band-limits through a per-instance forge::MorphBlep held by value; morph and character are conditioned with the negated-comparison pair, not forge::clamp (T-32-01), because plan 32-02's MORPH CV jack made 'already finite' false
- [Phase ?]: 32-06: the negated pair differs from forge::clamp for negative zero as well as for a not-a-number - MEASURED as moving 0 of 4096 samples for either field, and recorded in the source rather than glossed
- [Phase ?]: 32-06: the D-08 baseline tombstone was inverted in place into a bit-exact reconstruction proof - reconstruction mismatches 0 across 184,320 samples at three rates, with the sine-centre zero-correction control at exactly 0
- [Phase ?]: 32-06: the plan-32-07 alias-floor tombstone is left RED (failing gated cells fell from 32 to 2, both sine-centre character 0.5); flipping it here would have re-pinned thresholds against this implementation's own output

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
| Phase 30 P09 | 14 min | 2 tasks | 2 files |
| Phase 30 P10 | 7 min | 2 tasks | 1 files |
| Phase 31 P01 | 7m | 2 tasks | 2 files |
| Phase 31 P02 | 7min | 2 tasks | 1 files |
| Phase 31 P03 | 12min | 3 tasks | 1 files |
| Phase 31 P04 | 7min | 3 tasks | 2 files |
| Phase 31 P05 | 22min | 3 tasks | 1 files |
| Phase 31 P06 | 26min | 3 tasks | 1 files |
| Phase 31 P07 | 19min | 3 tasks | 2 files |
| Phase 31 P08 | 12min | 3 tasks | 1 files |
| Phase 31 P09 | 12min | 2 tasks | 1 files |
| Phase 32 P01 | 20 min | 3 tasks | 2 files |
| Phase 32 P02 | 21min | 3 tasks | 2 files |
| Phase 32 P03 | 40 min | 2 tasks | 1 files |
| Phase 32 P04 | 35 min | 2 tasks | 2 files |
| Phase 32 P05 | 78 min | 3 tasks | 2 files |
| Phase 32 P06 | 27 min | 2 tasks | 2 files |

## Session Continuity

**Resume file:** None

Last session: 2026-08-01T00:34:50.000Z
Stopped at: Completed 32-06-PLAN.md
Resume: run plan 32-07 — it owns the green alias-floor gate. `make test` is RED between 32-06 and 32-07 BY DESIGN: the alias-floor tombstone's `CHECK(failing >= 27)` now sees 2, and its five named subset cells all sit 3.0 to 3.9 dB BELOW threshold. Only 2 of 45 gated cells still exceed, both sine-centre at character 0.50 (C7 by 0.39 dB, C8 by 1.89 dB).

## Operator Next Steps

- **Phase 31 is complete and operator-approved.** Next: `/gsd-verify-work 31`, then `/gsd-discuss-phase 32`.
- **Optional housekeeping, your call.** `~/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio` is a stale separate plugin (slug `ForgeAudio`, v2.0.0, module `ForgeAudioLFO`, Feb 14) under the pre-rename slug. It is harmless — a different slug, so it cannot shadow the current `ForgeAudio-AnalogSeries` install — but it puts a **second Forge LFO** in the module browser, which is why Phase 31's step-9 guardrail sign-off is recorded with a subject that is inferred rather than pinned. Removing it would make every future in-Rack audition unambiguous. Not done for you: deleting a plugin from your Rack installation is not an executor's call. See `deferred-items.md` item 15.
- **Phase 35 now holds your first piece of VCO panel feedback** — the FM depth knob's affordance, `deferred-items.md` item 14. FM-02's bipolar behavior is locked and verified; only the widget is open.
