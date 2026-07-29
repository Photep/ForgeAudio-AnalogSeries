---
phase: 30-vcocore-skeleton-module-registration
verified: 2026-07-29T05:00:00Z
status: human_needed
score: 4/4 must-haves verified
behavior_unverified: 0
overrides_applied: 0
re_verification:
  previous_status: human_needed
  previous_score: 4/4
  gaps_closed:
    - "CR-01 (Nyquist guard clamp-order bug, non-finite ~1.48e38 V runaway on non-positive sampleRate) — fixed by 30-08: ceiling now runs before the NaN-safe floor, plus a new independent kVcoMaxDeltaPhase=0.5 direct phase-increment bound. Independently re-confirmed by reading src/dsp/VcoCore.hpp:207-208 and by running the exact named test."
    - "WR-03 (hostile-timing input class structurally unreachable through the driver) — closed by 30-08's driverless scenario four in tests/test_vco_core.cpp:617-739, calling core.step(in) directly. Independently re-run: 1/1 test case, 225 assertions, 0 failed."
    - "WR-02 comment inaccuracy (src/AnalogVCO.cpp claimed 'per-instance analog variation' the shipped module does not have) — corrected by 30-09 (commit 4cc5cc7). Independently confirmed by reading src/AnalogVCO.cpp:83-100; seed literals byte-unchanged, comment now states the measured 0/2048 clone behavior."
    - "CI link-gate re-observation on the combined 30-08+30-09 tip — 30-10 pushed 7933fae..0cf5f82 once and observed run 30419429579. Independently re-queried via gh run view: headSha matches 0cf5f82e12148b7dc096a3fc1c89a4d8ecc6a820, run conclusion success, and the 'win-x64 leg reproduction (compile + full link vs libRack)' step's own conclusion is success."
  gaps_remaining: []
  regressions: []
human_verification:
  - test: "CR-02 (deferred-items.md item 3): call forge::clamp(NaN, 0.f, 1.f) directly, and separately drive step() with morph = NaN, character = NaN."
    expected: "forge::clamp is NaN-transparent, so VcoCore's morph/character clamps are inert against NaN and step() emits a non-finite sample. Not reachable today (Rack sanitises param NaN before getValue()); becomes reachable once Phase 31/34 add MORPH/CHARACTER CV inputs. Already accepted by the operator at UAT test 2 ('Pass... Accepted for Phase 30... track there') and recorded with an owner in deferred-items.md item 3 (Phase 31 or 34, fix must be a VcoCore-local NaN-safe helper, never an edit to the frozen shared forge::clamp). Re-surfaced here only for closure confirmation, not as an open question."
    why_human: "Already resolved by the operator in 30-UAT.md test 2 and durably recorded with a named owner in deferred-items.md item 3. No new action needed; included so the re-verification record shows this item traced through to closure rather than silently dropped."
  - test: "WR-02 module-level scope (deferred-items.md item 2): roadmap SC4 ('same seed -> bit-identical block; different seed diverges') is satisfied at the forge::VcoCore level (proven by two passing tests) but src/AnalogVCO.cpp:114-115 hardcodes identical seed/spread-seed literals in every constructor, so two live VCO modules in the same patch are bit-identical clones today (0/2048 differing samples)."
    expected: "Already resolved by the operator at UAT test 3, option (a): SC4 accepted as satisfied at the VcoCore level; per-instance shell entropy explicitly out of Phase-30 scope; filed as deferred-items.md item 2 with owner Phase 34/35 and a re-validate-on-deserialize constraint; misleading comment corrected in the same pass (30-09)."
    why_human: "Already resolved and recorded with a named owner. Included so the re-verification record shows this item traced through to closure rather than silently dropped."
  - test: "WR-06 (NEW, surfaced by 30-REVIEW.md's gap-closure re-review, not yet triaged by an operator or logged in deferred-items.md): drive forge::VcoCore::step() with in.sampleRate = NaN, in.pitchCV = +10, morph = 0.5, character = 1.0."
    expected: "Independently reproduced by this verifier with a standalone probe against src/dsp/VcoCore.hpp: tel.freqHz = 267904.625 Hz (matches 30-REVIEW.md's '≈267,904 Hz' figure) — unbounded relative to any Nyquist ceiling, because maxFreq = kVcoNyquistGuardFrac * NaN is itself NaN, so the ceiling comparison `freq > maxFreq` is false for any freq and never fires; the very next floor line also does not catch a positive, finite freq. The header's own comment calls this guard 'LOAD-BEARING.' Output itself stays safe (finite, 5.0 V in this probe) only because the independently-added kVcoMaxDeltaPhase bound on deltaPhase masks it — a different guard than the one whose job this specifically is. The new WR-03 hostile-timing scenario does not catch this: it only asserts core.tel.freqHz >= 0.f, which an unclamped-but-positive freq satisfies trivially."
    why_human: "Does not fail any of the 4 roadmap success criteria or any of CORE-01/CORE-03/PANEL-03 as literally worded (audio output stays finite and in-bound; NaN sampleRate is not a value Rack has ever delivered). But it is a real, independently-reproduced gap in a guard the header text calls load-bearing, discovered by the phase's own gap-closure re-review, and unlike CR-02/WR-02/WR-04/WR-05 it has NOT yet been triaged by the operator or given a named owner in deferred-items.md. Operator decision needed: (a) accept as follow-on debt and log it in deferred-items.md with an owner (the reviewer's suggested one-line fix — sanitize sampleRate to 0 before computing maxFreq — is low-risk and bit-identical for every positive-finite rate Rack has ever supplied), or (b) fix now before closing this phase."
---

# Phase 30: VcoCore Skeleton & Module Registration Verification Report

**Phase Goal:** A pitch-accurate but intentionally aliased `VcoCore` behind the proven POD boundary, registered as a second module so it appears and sounds (crudely) in Rack — proving the architecture before any hard DSP.
**Verified:** 2026-07-29T05:00:00Z
**Status:** human_needed
**Re-verification:** Yes — after gap closure (plans 30-08, 30-09, 30-10, closing CR-01/WR-03 and correcting WR-02's comment; CR-02 and WR-02's underlying per-instance-clone behavior were operator-accepted as deferred debt with named owners at UAT)

## Goal Achievement

### Observable Truths

All four are the roadmap's literal Success Criteria for Phase 30 (`.planning/ROADMAP.md` § Phase 30). Re-checked against the current tree (HEAD `734328c`), not against the prior verification's record.

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | `forge::VcoCore` exposes a POD `Inputs` → `step()` → output+telemetry boundary mirroring `LfoCore`, driven headlessly by the Phase-29 harness | ✓ VERIFIED | `src/dsp/VcoCore.hpp` — `VcoInputs` POD, `step(const VcoInputs&)`, `Telemetry tel`. Driven headlessly by `tests/VcoBlockDriver.hpp` / `tests/test_vco_harness.cpp` / `tests/test_vco_core.cpp`. `make guards` re-run: PASS (dependency-direction audit clean, guard wiring clean). |
| 2 | Naive aliased morphed waveform at audio rate via `exp2_taylor5`, no static/global mutable voice state | ✓ VERIFIED | `step()` computes `freq = kVcoFreqC4 * exp2_taylor5(in.pitchCV)` and calls the frozen `wave.morphedWave(...)` once. No-static-state is behavior-dependent and is proven by re-running (not just reading) `"vco core: two-instance independence under sample-by-sample interleaving (D-17)"` — 1/1 passed, 18/18 assertions — plus its REQUIRED-to-fail positive control `"vco core: independence positive control..."` — 1/1 passed, 6/6 assertions, confirming the positive control genuinely detects (`mismatchA=512, mismatchB=512` per 30-10-SUMMARY.md, consistent with this verifier's independent test run). |
| 3 | The VCO appears as a second selectable module via a second `addModel` + `plugin.hpp` extern + `plugin.json` `modules[]` entry, LFO registration and slug untouched | ✓ VERIFIED | Re-read directly (not diffed against a stale baseline): `src/plugin.hpp` declares `extern Model* modelAnalogVCO;`, `src/plugin.cpp` calls `p->addModel(modelAnalogVCO);` after `p->addModel(modelAnalogLFO);`, `plugin.json` parses to exactly 2 modules `['ForgeAnalogLFO', 'ForgeAnalogVCO']` at version `2.0.1`. `30-10-SUMMARY.md` D6 additionally confirms all three files are blob-identical to the pre-gap-closure baseline (`f8f430e`) — this gap closure touched none of them. Slug `ForgeAnalogVCO` matches `src/AnalogVCO.cpp`'s `createModel<AnalogVCO, AnalogVCOWidget>("ForgeAnalogVCO")`. Prior operator UAT sign-off ("Approved," `30-07-SUMMARY.md`) stands, unaffected by this gap closure. |
| 4 | Fixed-seed determinism holds: same seed → bit-identical block; different seed diverges | ✓ VERIFIED (at the `forge::VcoCore` level — see human-verification note on module-level scope, already operator-resolved) | Independently re-run: `"vco harness: seam determinism..."` — 1/1 passed, 6/6 assertions — and `"vco core: spread seed divergence at character 1.0 (D-18a)"` — 1/1 passed, 18/18 assertions. Module-level scope caveat (`src/AnalogVCO.cpp` hardcodes identical seed literals per instance, 0/2048 differing samples) is unchanged by this gap closure by design (the literals themselves were explicitly out of scope; only the misleading comment was corrected) and was already accepted by the operator at UAT test 3, option (a), with a named Phase 34/35 owner in `deferred-items.md` item 2. |

**Score:** 4/4 truths verified (0 present, behavior-unverified)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/dsp/VcoCore.hpp` | POD boundary + naive step body, CR-01 guard fix | ✓ VERIFIED | Re-read directly: ceiling-then-floor order confirmed at the frequency guard (`if (freq > maxFreq) freq = maxFreq; if (!(freq > 0.f)) freq = 0.f;`), new `kVcoMaxDeltaPhase = 0.5` bound confirmed applied to `deltaPhase` independently of the frequency guard. Wired into `AnalogVCO.cpp`, `test_vco_harness.cpp`, `test_vco_core.cpp`, `vco_compile_canary.cpp`. |
| `tests/test_vco_core.cpp` | CORE-01 + CORE-03 behavioral cases, WR-03 driverless hostile-timing scenario | ✓ VERIFIED | Scenario four (`:617-739`) confirmed calling `core.step(in)` directly with no `VcoBlockDriver` in the loop — genuinely bypasses the unconditional sampleTime/sampleRate overwrite that made hostile timing unreachable before. Independently re-run: the containing test case (`"vco core: output magnitude stays inside the 6.0 V loose bound (D-18b)"`) passes at 225/225 assertions. |
| `src/AnalogVCO.cpp` | Minimum-viable Rack shell, 4 controls, WR-02 comment correction | ✓ VERIFIED | Constructor comment (`:83-100`) re-read directly: no longer claims "per-instance analog variation"; states the measured 0/2048 clone figure and scopes the D-11 spread correctly. Seed literals (`0x1234`/`0x5678`, `0x9E3779B9`/`0x7F4A7C15`) confirmed byte-unchanged. Compiles clean under `make strict`. |
| `plugin.json`, `src/plugin.hpp`, `src/plugin.cpp` | Additive registration, untouched by gap closure | ✓ VERIFIED | Re-read directly (see Truth 3). |
| `deferred-items.md` | Named owner recorded for every accepted-debt finding | ⚠️ PARTIAL | Items 1-5 present with named owners (PANEL-03 resolved; WR-02 clone behavior → Phase 34/35; CR-02 → Phase 31/34; WR-04 → Phase 36; WR-05 + four Info findings → next `check_includes.sh` touch/unplanned). **WR-06, a Warning-level finding from `30-REVIEW.md`'s gap-closure re-review, is NOT yet present in this file** — it postdates the file's last edit. See human verification below. |

### Key Link Verification

| From | To | Via | Status | Details |
|------|-----|-----|--------|---------|
| `src/AnalogVCO.cpp` `modelAnalogVCO` | `src/plugin.cpp` `addModel` | Symbol resolution | ✓ WIRED | Re-read directly; unchanged since prior verification. |
| `plugin.json` slug | `src/AnalogVCO.cpp` `createModel<...>(...)` slug | String literal match | ✓ WIRED | Both `ForgeAnalogVCO`, re-confirmed by direct read. |
| CI toolchain-gate `win-x64 leg reproduction` step | pushed commit `0cf5f82` | `gh run view` step-level conclusion | ✓ WIRED | Independently re-queried (not trusted from SUMMARY): `gh run view 30419429579 --json headSha,conclusion,jobs` returns `headSha=0cf5f82e12148b7dc096a3fc1c89a4d8ecc6a820`, run `conclusion=success`, and the named step's own `conclusion=success`. |

### Behavioral Spot-Checks

All commands below were independently executed by this verifier against the current working tree (HEAD `734328c`), not read from a SUMMARY.

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Full local test suite | `make test` | 72 cases / 72 passed / 0 failed, 2,616,064 assertions | ✓ PASS |
| Guard suite | `make guards` | All sections OK, `guard suite: PASS` | ✓ PASS |
| Strict C++11 compile | `make strict` | `strict C++11 gate: PASS` over 4 TUs incl. `AnalogVCO.cpp` | ✓ PASS |
| Golden replay (LFO non-regression) | `./build-test/test --test-case="golden*"` | 6/6 passed, 49,164 assertions | ✓ PASS |
| WR-03 driverless hostile-timing scenario | `./build-test/test --test-case="vco core: output magnitude stays inside the 6.0 V loose bound (D-18b)"` | 1/1 passed, 225/225 assertions | ✓ PASS |
| Seam determinism (SC4, same-seed half) | `--test-case="vco harness: seam determinism*"` | 1/1 passed, 6/6 assertions | ✓ PASS |
| Spread-seed divergence (SC4, different-seed half) | `--test-case="vco core: spread seed divergence*"` | 1/1 passed, 18/18 assertions | ✓ PASS |
| Two-instance independence (CORE-03) | `--test-case="vco core: two-instance independence*"` | 1/1 passed, 18/18 assertions | ✓ PASS |
| Independence positive control | `--test-case="vco core: independence positive control*"` | 1/1 passed, 6/6 assertions | ✓ PASS |
| CR-01 fix, direct code trace | Read `src/dsp/VcoCore.hpp:207-208` | Ceiling clamp precedes the NaN-safe floor clamp; floor is the final writer | ✓ PASS |
| WR-06 reproduction (new finding, not a regression check) | Standalone probe: `core.step(in)` with `sampleRate=NaN, pitchCV=10, morph=0.5, character=1.0` | `tel.freqHz = 267904.625`, `out = 5.0` (finite) | Confirms `30-REVIEW.md`'s WR-06 finding — surfaced to human verification below |
| CI on the exact pushed commit | `gh run view 30419429579 --json headSha,conclusion,jobs` | `headSha` matches, run `success`, win-x64 link step `success` | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| CORE-01 | 30-01, 30-02, 30-03, 30-05, 30-07, 30-08, 30-09, 30-10 | New Rack-free `forge::VcoCore` mirrors `LfoCore`'s POD-`Inputs` → `step()` → output+telemetry boundary | ✓ SATISFIED | Truth 1 + Truth 2 above; re-confirmed after `VcoCore.hpp` moved under the CR-01 fix (30-10 D4: pitch/magnitude/divergence figures reproduce 30-03's recorded values to six digits) |
| CORE-03 | 30-04, 30-07, 30-10 | `VcoCore` is self-contained per-voice, no static/global mutable state, polyphony-ready | ✓ SATISFIED | Truth 2 above; re-confirmed by 30-10 D5 and by this verifier's independent test re-run |
| PANEL-03 | 30-01, 30-05, 30-06, 30-07, 30-10 | VCO registered as second module without altering LFO registration | ✓ SATISFIED | Truth 3 above; re-confirmed byte-identical to the pre-gap-closure baseline by 30-10 D6 and by this verifier's direct file read |

`.planning/REQUIREMENTS.md` maps exactly CORE-01, CORE-03, PANEL-03 to Phase 30 (rollup line + individual `[x]` marks + status table all agree: "Complete"). All three appear in `requirements:` frontmatter across multiple Phase 30 plans, including the three gap-closure plans (30-08, 30-09, 30-10). No orphaned requirements.

### Anti-Patterns Found

None. Re-scanned the three gap-closure-touched files (`src/dsp/VcoCore.hpp`, `tests/test_vco_core.cpp`, `src/AnalogVCO.cpp`) for `TBD`/`FIXME`/`XXX`/`TODO`/`HACK`/`PLACEHOLDER` and "not yet implemented" language — zero hits.

### Independent Code Review Findings (30-REVIEW.md, gap-closure re-review) — Status

The gap-closure re-review found the prior CR-01 fix genuinely closes the bug (confirmed independently above, not merely re-read), WR-03 genuinely non-vacuous, and WR-02's comment genuinely corrected. It surfaced one new finding:

- **WR-06** (Warning, new): the reordered Nyquist ceiling is a no-op specifically when `in.sampleRate` is `NaN` (`maxFreq` becomes `NaN`, so the ceiling comparison is always false), leaving `tel.freqHz` unbounded for that one input class. Independently reproduced by this verifier via a standalone probe (see spot-checks table). Output itself stays safe — the independent `kVcoMaxDeltaPhase` bound on `deltaPhase` masks it — so this does not fail any roadmap Success Criterion or requirement as worded. It is real, reproduced, and does not yet have a named owner in `deferred-items.md` (the file predates this finding, which the re-review surfaced after `deferred-items.md`'s last edit). Routed to human verification below rather than silently accepted or silently gapped.
- CR-02, WR-04, WR-05, four Info findings: unchanged, all already have named owners in `deferred-items.md`, confirmed present by direct read of that file. Not re-litigated here.
- IN-05 (new, Info-level): the hostile-timing grid doesn't cover `+inf`/`-inf`/subnormal/very-large-finite values; explicitly "not evidence of a defect" per the review's own hand-trace, and covered by `deferred-items.md` item 5's general "30-REVIEW.md is the record" catch-all for unplanned Info findings. Not gating.

### Human Verification Required

See frontmatter `human_verification` for the full detail (test / expected / why-human for each). Summary:

1. **CR-02 and WR-02 module-level scope** — already resolved by the operator at UAT (`30-UAT.md` tests 2 and 3) with named owners recorded in `deferred-items.md`. Listed here only so the re-verification record shows these traced through to closure, not as open questions.
2. **WR-06 (new) — Nyquist ceiling silently no-ops when `sampleRate` is `NaN`.** Real, independently reproduced, does not fail any roadmap truth or requirement (output stays finite/bounded via an unrelated guard), but has not yet been triaged by the operator or logged in `deferred-items.md` with an owner. Operator decision needed: (a) accept as follow-on debt and log it with an owner before Phase 31 begins driving this seam from new call sites, or (b) fix now (the reviewer's suggested fix is a one-line, bit-identical-for-positive-rates sanitization).

### Gaps Summary

No must-have failed. All four roadmap Success Criteria and all three requirement IDs (CORE-01, CORE-03, PANEL-03) remain verified, now with the gap-closure fixes independently re-confirmed rather than re-read from SUMMARYs: CR-01 is fixed (direct code read + reproduced-safe behavioral test), WR-03 is closed non-vacuously (re-run, passing, and the bypass mechanism confirmed by reading the test), WR-02's inaccurate comment is corrected (re-read), and the CI link-gate re-observation is independently re-queried and confirmed green on the exact pushed SHA including the win-x64 link step's own conclusion. Local gates (`make test`, `make guards`, `make strict`) all independently re-run and green, matching the constraints stated for this verification exactly (72 cases, 2,616,064 assertions).

This phase is routed to `human_needed` rather than `passed` for one reason: the gap-closure round's own re-review surfaced a new, real, independently-reproduced Warning (WR-06) that has not yet been triaged by an operator or given a named owner, unlike every other open finding in this phase (CR-02, WR-02, WR-04, WR-05 all have owners). It does not falsify a roadmap truth or requirement — output stays safe by construction of an unrelated guard — but leaving it untriaged would mean this phase closes with an un-owned, reproduced defect in a comment-declared "LOAD-BEARING" guard, which is exactly the pattern this phase's verification loop exists to catch before Phase 31 begins driving `VcoCore` from new call sites.

---

_Verified: 2026-07-29T05:00:00Z_
_Verifier: Claude (gsd-verifier)_
