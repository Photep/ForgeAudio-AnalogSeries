---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
verified: 2026-08-28T00:00:00Z
status: human_needed
score: 9/9 must-haves verified (all qualified by two open hardening defects and one honestly-unresolved perceptual claim)
behavior_unverified: 0
overrides_applied: 0
human_verification:
  - test: "Decide disposition of 32-REVIEW.md CR-01 (OOB write on negative/NaN morph in MorphBlep::step) and CR-02 (NaN character bypasses the NaN trap at three literal-zero-width sites)"
    expected: "An explicit operator decision: fix in a Phase 32 gap-closure plan, or accept as the first task of Phase 33 before its second call site (hard-sync addStep) lands"
    why_human: "Both are verified-real, verified-unreachable-today defects in a shared header two future phases will call into; whether to gate Phase 33 on this fix (as the review and this verifier recommend) versus proceeding is a scope/sequencing decision, not a code-correctness question"
  - test: "Confirm the MORPH-02 shell-side knob+CV×attenuverter mix qualification (operator-attested on absence of fault, not headlessly measured) is an acceptable standing state, or decide it needs a headless harness / POD field before Phase 34 closes the gap"
    expected: "Explicit accept-as-is or a follow-up plan; REQUIREMENTS.md already carries the qualification honestly and should not be silently upgraded to an unqualified Complete"
    why_human: "No test can reach in.morph's pre-clamp CV×attenuverter path without new POD fields, which reopens the compile-canary margin question (deferred item 15) — a design tradeoff, not a gap this phase can close"
  - test: "Acknowledge that the audible-improvement half of the operator UAT (Q1(b)) is unevidenced by construction (no A/B reference existed) before treating Phase 32's perceptual claim as closed"
    expected: "The audible-improvement claim rests on automated spectral evidence only (TEST-03 `failing==0`, the anti-circularity `improvementDb>=8.0` assertion); the ear-evidence half is open and filed as deferred item 26, owned by Phase 36 or the next perceptual phase"
    why_human: "Only a human can decide whether automated-only evidence is sufficient to ship on, or whether an A/B rendering harness should be built before Phase 34's audition-gated DRIFT-03 repeats the same unanswerable-by-construction shape"
---

# Phase 32: Morph-Aware Anti-Aliasing (polyBLEP/polyBLAMP) Verification Report

**Phase Goal:** Band-limit the continuous, character-deformed morph crossfade so the oscillator stays clean across the whole keyboard — the single dominant-risk subsystem, fully isolated in its own wrapper header with its own spectral iteration budget.
**Verified:** 2026-08-28
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

This phase's own discipline (T-32-27) requires a named test case in a named file to discharge a requirement, or a written Pending reason. I held this verification to the same standard: every truth below is checked against the actual code and, where a requirement is claimed discharged, against the literal `TEST_CASE` string that discharges it — not against SUMMARY prose.

### Observable Truths

| # | Truth (from ROADMAP Success Criteria) | Status | Evidence |
|---|---|---|---|
| 1 | A new additive `MorphBlep.hpp` *calls* (never edits) the frozen `Waveshape.hpp`, applying polyBLEP at value-step sites and polyBLAMP at triangle slope corners — zero edits to the frozen header | ✓ VERIFIED | `src/dsp/MorphBlep.hpp` is a new file; `git log` shows 0 modifying commits to `src/dsp/Waveshape.hpp` in the phase range. Independently re-verified: `shasum -a 256 -c src/dsp/FROZEN.sha256` reports all 15 pinned paths match on the current tree (I ran this myself, not from a SUMMARY claim). `MorphBlep.hpp` reads `wv.squareDutySpread` etc. and calls no mutating method on `Waveshape`. |
| 2 | MORPH knob+CV+attenuverter sweep the continuous 5-shape crossfade at audio rate with band-limited output; BLEP/BLAMP magnitude driven by the characterized jump | ✓ VERIFIED (crossfade half) / ⚠ QUALIFIED (shell-mix half) | Crossfade-at-audio-rate: `TEST_CASE("vco core: audio-rate MORPH sweeping through every segment boundary stays finite and bounded (MORPH-01 / MORPH-02)")` at `tests/test_vco_core.cpp:1814`, confirmed present, 27 configurations. Characterized-jump magnitude: `mag[]` array at `MorphBlep.hpp:453-463` derived from `W[]`/`c` (character-squared), asserted against a direct probe of frozen `Waveshape` in `tests/test_morph_blep.cpp:446` (`TEST_CASE("morph blep: the site magnitudes ARE the characterized jumps of the frozen Waveshape (AA-04 / D-01)")`, confirmed present). **The knob+CV×attenuverter mix itself is asserted by NO test case** — confirmed by reading `src/AnalogVCO.cpp:282-288`: the mix happens in `process()` against `params[]`/`inputs[]`, which no headless driver constructs, and D-17 added zero `VcoInputs` POD fields for it. This is honestly recorded in `REQUIREMENTS.md`'s 2026-08-27 footer and `deferred-items.md` item 24 as operator-attested-on-absence-of-fault, not measured. I confirm the qualification is accurate, not overstated. |
| 3 | Multiple/overlapping discontinuities within a single sample are each placed at their own sub-sample position and summed, not overwritten — narrow pulse keeps its body at high notes | ✓ VERIFIED | `MorphBlep.hpp:469` site loop uses `+=` accumulation (`now +=`/`pending +=`, confirmed at `:516-524` region), never assignment. Discharged by `TEST_CASE("morph blep: overlapping pulse edges SUM rather than overwrite at a narrow duty (AA-03 / D-07)")` at `tests/test_morph_blep.cpp:846` — read in full: it independently derives expected `alone0`/`alone1` contributions from the kernel formula (not read back from the implementation) and checks the emitted value equals their sum, a genuine non-circular assertion. |
| 4 | A spectral alias-floor invariant asserts high-note aliasing stays below per-shape measured thresholds at C7/C8/C9 (TEST-03) | ✓ VERIFIED | `TEST_CASE("vco spectrum: TEST-03 - the alias floor stays below its per-shape pinned threshold at C7, C8 and C9 ...")` at `tests/test_vco_spectrum.cpp:2159`; `CHECK(failing == 0)` at `:2323` over 45 gated cells (`REQUIRE(gatedWalked == 45)`); anti-circularity assertion `CHECK(improvementDb >= kMinImprovementDb)` at named large-margin cells consults no pinned number. I independently ran `make test`: **94/94 cases, 0 failed, 2,622,319 assertions** — matches the orchestrator's pre-verification gate measurement exactly, and includes this case passing. |
| 5 | All anti-aliasing is table-free and Rack-free (closed-form arithmetic), preserving C++11-strict compilation and golden bit-stability — no minBLEP, no oversampling | ✓ VERIFIED | `MorphBlep.hpp` uses only function-local `const` arrays (never `static constexpr` in-class, confirmed at `:400-402` banner and by inspection of `pos[9]`/`mag[9]`/`wid[9]`/`kind[9]`), no `<vector>`/table lookups, no Rack includes (`check_includes.sh [2/7]`/`[3/7]` reported `OK` in the pre-verification gate run, re-confirmed by me: `make guards` → `guard suite: PASS`). `make strict` re-run by me: `strict C++11 gate: PASS`. No oversampling loop exists in the header or `VcoCore::step` (confirmed by reading the single `wave.morphedWave` call and single `blep.step` call at `VcoCore.hpp:614`/`:645`). |

**Score:** 9/9 phase requirement IDs have a named discharging test case (table below); 0 are behavior-dependent-unverified. Two of the nine truths above carry an honestly-recorded qualification that is not a code gap this phase can close alone (see Gaps Summary).

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `src/dsp/MorphBlep.hpp` | New additive header, calls-not-edits `Waveshape.hpp` | ✓ VERIFIED | Exists, 500+ lines, substantive (kernel algebra, 9-site union, character factor); wired via `VcoCore.hpp:645` |
| `src/dsp/VcoCore.hpp` | Wires `MorphBlep` into `step()`, hardens morph/character | ✓ VERIFIED | `blep` member present; morph/character conditioned at `:598-602` before use; single call site confirmed by `grep -rn "blep\." src/` |
| `src/AnalogVCO.cpp` | MORPH CV jack + attenuverter, NaN-safe shell mix | ✓ VERIFIED (wiring) / ⚠ (no headless test reaches it) | `:282-288` implements `MORPH_CV_INPUT * 0.1 * MORPH_ATTEN_PARAM` mix with NaN-safe negated-pair clamp on the result |
| `res/AnalogVCO.svg` | Paired MORPH CV/atten panel rects | ✓ VERIFIED | 32-11-SUMMARY.md's installed-asset pin measured 12 `<rect>` (10→12 delta), independently plausible given the described change; not re-derived by me from raw SVG but the delta is corroborated by two independent artefacts (repo file and installed file) agreeing |
| `tests/test_morph_blep.cpp`, `tests/test_vco_core.cpp`, `tests/test_vco_spectrum.cpp` | Unit + spectral suites | ✓ VERIFIED | All three exist; the specific named `TEST_CASE`s cited in this report were located by direct grep against the files, not accepted from SUMMARY quotation |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|
| `VcoCore.hpp:645` | `MorphBlep::step` | direct call, single site | ✓ WIRED | `grep -rn "blep\." src/` returns exactly one executable call site — confirmed myself, not taken from the review's word |
| `VcoCore.hpp:598-602` | `blep.step`'s `morph`/`character` args | NaN-safe negated-pair conditioning immediately upstream | ✓ WIRED | Read directly: `if (!(morph > 0.f)) morph = 0.f; if (morph > 1.f) morph = 1.f;` and the same for `character`, both before the call at `:645` |
| `AnalogVCO.cpp:286-288` | `in.morph` | shell-boundary NaN-safe clamp | ✓ WIRED | Confirmed present; note this conditions **`morph` only** — `in.character` (`:289`) is unconditioned at the shell because CHARACTER's CV jack does not exist yet (Phase 34), so a knob-only value is inherently range-bound by Rack's param widget and cannot be user-driven to NaN today |
| `MorphBlep.hpp:step` (unguarded `morph`/`character`) | out-of-bounds `W[]` write / NaN propagation | direct call bypassing VcoCore's guard | ✗ NOT WIRED (defense-in-depth gap, confirmed real) | See Anti-Patterns / Gaps below — this is CR-01/CR-02, reproduced in the code by direct inspection, not merely cited from the review |

### Requirements Coverage

| Requirement | Source Plan(s) | Description | Status | Evidence |
|---|---|---|---|---|
| MORPH-01 | 32-06, 32-09 | Morph engine (`Waveshape`) runs at audio rate, reused verbatim | ✓ SATISFIED | `tests/test_vco_core.cpp:1814`, confirmed present and passing |
| MORPH-02 | 32-02, 32-09 | MORPH knob+CV+atten sweep the crossfade at audio rate | ⚠ SATISFIED WITH RECORDED QUALIFICATION | Crossfade half: same case above. Shell-mix half: no test case exists (confirmed — `AnalogVCO.cpp::process()` reachable only via `make strict`/link + operator attestation). REQUIREMENTS.md's footer already states this qualification accurately; I did not find it overstated anywhere. |
| AA-01 | 32-06, 32-07, 32-08 | polyBLEP at value-step discontinuities | ✓ SATISFIED | `tests/test_vco_spectrum.cpp:2159` (TEST-03) + `:1387` (D-08 reconstruction) confirmed present |
| AA-02 | 32-05, 32-07 | polyBLAMP at triangle slope corners | ✓ SATISFIED | `tests/test_morph_blep.cpp:446` part B (slope-break magnitudes with `REQUIRE` that triangle contributes no value jump) confirmed present |
| AA-03 | 32-05, 32-07, 32-08 | Overlapping discontinuities summed, not overwritten | ✓ SATISFIED | `tests/test_morph_blep.cpp:846`, confirmed present and read in full — genuinely non-circular (independently derives expected values) |
| AA-04 | 32-05 | BLEP/BLAMP magnitude driven by characterized jump | ✓ SATISFIED | `tests/test_morph_blep.cpp:270` (`morphBlepCharFactor` limits) + `:446` (site magnitudes vs. frozen probe), both confirmed present |
| AA-05 | 32-05, 32-09, 32-10 | Table-free, Rack-free, C++11-strict, no minBLEP/oversampling | ✓ SATISFIED | `make strict` PASS (re-run by me), `make guards` PASS (re-run by me), CI MinGW link leg observed green by SHA per 32-10-SUMMARY (not independently re-run by me, but the claim is corroborated by the fact this exact header ships through the compile canary per `check_canary.sh [5/5]` which I did re-run) |
| CORE-02 | 32-06 | Anti-aliasing in additive header, zero edits to shared headers | ✓ SATISFIED | `shasum -a 256 -c src/dsp/FROZEN.sha256` — I ran this myself; all 15 pinned paths verified byte-identical on the current tree |
| TEST-03 | 32-01, 32-03, 32-07 | Spectral alias-floor invariant at C7/C8/C9 | ✓ SATISFIED | `tests/test_vco_spectrum.cpp:2159`, `CHECK(failing==0)`; re-ran `make test` myself — 94/94 pass, includes this case |

**No orphaned requirements.** All 9 phase requirement IDs (MORPH-01, MORPH-02, AA-01..05, CORE-02, TEST-03) appear in at least one plan's `requirements:` frontmatter and each has a corresponding `requirements-completed:` entry in the plan(s) that landed its assertion (32-02, 32-05, 32-06, 32-07, 32-09), matching REQUIREMENTS.md's traceability table exactly.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---|---|---|---|
| `src/dsp/MorphBlep.hpp` | `:319-320`, `:332-333`, `:364-365` | `segment` clamped only above (`if (segment > 3) segment = 3`), never below; negative `morph` produces `segment < 0` and an out-of-bounds `W[segment]`/`W[(segment+2)%5]` access | 🛑 BLOCKER-CLASS DEFECT, but confirmed NOT REACHABLE via the shipped call path today | Reproduced by direct code reading (not merely cited): `blep.step`'s only call site (`VcoCore.hpp:645`) is preceded by the NaN-safe negated-pair clamp at `:598-602`, and `AnalogVCO.cpp:286-288` clamps `in.morph` a second time before it ever reaches `VcoCore`. This is CR-01 from `32-REVIEW.md`, and I independently verified the reachability claim rather than accepting it: `grep -rn "blep\." src/` returns exactly one executable call site, and both upstream guards are present and correctly ordered (negation-first, so NaN fails `> 0.f` and falls to the safe branch). **Open, unfixed, correctly classified as hardening not a live fault.** |
| `src/dsp/MorphBlep.hpp` | `:317`, `:464` (`wid[9]` has three literal `0.f` entries at indices 0, 3, 5), `:453-463` | A NaN `character` makes `c = character*character = NaN` (the `< 0.001f` guard at `:317` is false for NaN, so it doesn't trip), and `mag[i]` at the three zero-width sites goes NaN even though `morphBlepCharFactor(0.f, fdt)` returns exactly `1.f` regardless — the site's NaN-trap only guards non-zero-width sites | 🛑 BLOCKER-CLASS DEFECT, confirmed NOT REACHABLE via shipped call path | This is CR-02. Same upstream guards as above apply, so it is latent, not live. Confirmed by direct inspection of the `wid[9]` array and the `mag[i] == 0.f` skip test at `:473`, which is false for NaN. |

Both items are correctly disclosed in `32-REVIEW.md`, `32-10-SUMMARY.md`, and `32-11-SUMMARY.md` as open, unreachable-today, high-priority hardening — not silently absorbed into a clean-phase narrative. I independently confirmed both the defects and the unreachability claim by reading the code rather than accepting the review's or the SUMMARY's word. No debt markers (`TBD`/`FIXME`/`XXX`/`TODO`/`HACK`/`PLACEHOLDER`) were found in any phase-modified file.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Full test suite passes at the exact count the orchestrator measured | `make test` | 94 cases / 94 passed / 0 failed, 2,622,319 assertions | ✓ PASS (independently re-run, matches exactly) |
| Strict C++11 gate | `make strict` | `strict C++11 gate: PASS` | ✓ PASS (independently re-run) |
| Guard suite (frozen-header + canary + include-direction) | `make guards` | `guard suite: PASS` | ✓ PASS (independently re-run) |
| Frozen-header byte-identity | `shasum -a 256 -c src/dsp/FROZEN.sha256` | All 15 paths report OK | ✓ PASS (independently re-run, not accepted from SUMMARY) |
| MORPH-01/02 named case exists | `grep -n "TEST_CASE(\"vco core: audio-rate MORPH sweeping..." tests/test_vco_core.cpp` | Found at `:1814` | ✓ PASS |
| CORE-02/AA-01..05 named cases exist | grep against `tests/test_morph_blep.cpp`, `tests/test_vco_spectrum.cpp` | All 6 named cases located verbatim | ✓ PASS |

CI (four-job matrix, MinGW link leg) was not re-run by me — the orchestrator's pre-verification state block already reports it green on the exact shipped SHA (`262e5c5`, run `30681442134`), and 32-10-SUMMARY.md's transcript of that run (job/step conclusions, `win-x64 link gate: PASS` verbatim line) is internally consistent with the local guard/strict results I did re-run. I did not re-trigger CI myself; this is the one gate taken on the orchestrator's word rather than independently reproduced, which is reasonable since CI state was explicitly supplied as pre-measured verification_state.

### Human Verification Required

See frontmatter `human_verification`. Summarized:

1. **Disposition of CR-01/CR-02** (`32-REVIEW.md`). Both are real, both are confirmed unreachable through the single shipped call site today, but Phase 33 is documented to add a second call site (`addStep` at the hard-sync seam) that the deferred register itself flags as "what makes the ordering matter." This is a sequencing decision — fix now vs. fix as Phase 33's first task — not a code-quality question this verifier can resolve.
2. **MORPH-02's shell-mix qualification.** Correctly and honestly recorded as operator-attested-on-absence-of-fault rather than measured. Whether this is an acceptable standing state through Phase 34, or whether a headless harness/POD field should be added now, is a judgment call with a real cost (reopens the compile-canary margin, deferred item 15).
3. **The audible-improvement claim (Q1(b) of the UAT).** Genuinely unanswerable by the session as designed (no A/B reference existed) — not a failure of execution, but a design gap in every remaining perceptual audition this milestone. Filed as deferred item 26. A human should decide whether to accept the automated-only evidence as sufficient for now or prioritize the A/B rendering harness before Phase 34's audition-gated DRIFT-03.

None of these are gaps in what this phase *built* — they are open questions this phase correctly refused to paper over. That refusal is itself evidence of the phase's discipline, but per the adversarial stance of this verification, an unqualified "passed" would misrepresent that state.

### Gaps Summary

No requirement lacks a named, located, independently-confirmed discharging test case. No debt markers exist in phase-modified files. No frozen header was touched (verified by checksum, not by trusting `git diff --name-only`). The two code-review criticals are real defects, but the shipped call path is genuinely guarded against them today — I re-derived this myself rather than trusting the review's or SUMMARY's reachability claim, and found the guard chain (`VcoCore.hpp:598-602`, `AnalogVCO.cpp:286-288`, single call site) intact and correctly ordered.

The phase does not have "gaps" in the sense of missing or stubbed work. It has three items that were deliberately surfaced rather than closed, each with a named owner and remedy, which is exactly what this project's own escalation discipline calls for. Routing them to human verification rather than either rubber-stamping a clean pass or manufacturing a false "gaps_found" is the correct classification: the artifacts exist, are substantive, and are wired; what remains open are judgment calls about defense-in-depth timing and about how much evidence a perceptual claim needs before the next phase relies on it.

---

_Verified: 2026-08-28_
_Verifier: Claude (gsd-verifier)_
