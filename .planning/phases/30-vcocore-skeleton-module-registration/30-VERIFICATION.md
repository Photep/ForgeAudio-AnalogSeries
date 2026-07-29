---
phase: 30-vcocore-skeleton-module-registration
verified: 2026-07-29T00:45:03Z
status: human_needed
score: 4/4 must-haves verified
behavior_unverified: 0
overrides_applied: 0
human_verification:
  - test: "CR-01 (30-REVIEW.md): drive forge::VcoCore::step() directly with in.sampleRate = -44100.f, pitchCV = 0, morph = 0.5, character = 1.0 for 20000 steps and observe the output."
    expected: "Header comment at src/dsp/VcoCore.hpp:167-172 claims the Nyquist guard is 'LOAD-BEARING' and bounds the output. Independently reproduced: the ceiling clamp runs after the floor clamp, so a negative sampleRate re-introduces a negative frequency, the phase accumulator ramps unboundedly negative, and the output reaches ~1.48e38 V and goes non-finite — the exact failure the guard's own comment says it prevents."
    why_human: "Not reachable through the Phase-30 Rack shell as shipped (Rack always supplies sampleRate > 0), so it does not violate any of the 4 roadmap success criteria as literally worded. But VcoCore is a public seam Phases 31-34 and future polyphony will call from new sites, and a PLAN 30-02 must-have explicitly asserts this guard is load-bearing — an assertion now proven false. Operator decision needed: fix now (one-line clamp-order swap, reviewer supplies the patch and a WR-03 coverage case), or accept as tracked follow-up debt before Phase 31 begins driving this seam from new call sites."
  - test: "CR-02 (30-REVIEW.md): call forge::clamp(NaN, 0.f, 1.f) directly, and separately drive step() with morph = NaN, character = NaN."
    expected: "src/dsp/VcoCore.hpp:187-188 clamps morph/character as its 'only defensive validation.' Independently reproduced: forge::clamp is a comparison ladder that is NaN-transparent (returns NaN unchanged), so the clamp does not clamp, and step() emits a non-finite sample straight to the module output."
    why_human: "Not reachable today — Rack's ParamQuantity::setValue sanitises NaN before params[].getValue() is read. It becomes reachable the moment Phase 31/34 add MORPH/CHARACTER CV inputs, since Rack does not sanitise cable voltages. forge::clamp itself is FROZEN and shared with the shipped LFO (src/dsp/LfoCore.hpp:168,212-213,216), so any fix must be a local VcoCore-scoped helper, not an edit to the shared primitive. Operator decision needed: fix now (reviewer supplies a NaN-safe local helper + a failing-today pinning test) or track for Phase 31/34 when CV inputs land."
  - test: "WR-02 (30-REVIEW.md), independently reproduced by this verification: construct two forge::VcoCore instances seeded with the exact literals src/AnalogVCO.cpp:96-97 hardcodes (0x1234/0x5678 seed, 0x9E3779B9/0x7F4A7C15 spread seed), step both 2048 samples at morph=0.25, character=1.0, and diff the outputs sample by sample."
    expected: "Roadmap success criterion 4 states 'Fixed-seed determinism holds: same seed -> bit-identical block; different seed diverges.' The forge::VcoCore level fully satisfies this (see truth 4 below, proven by two passing behavioral tests: 'vco harness: seam determinism' and 'vco core: spread seed divergence at character 1.0'). But because AnalogVCO's constructor hardcodes the same seed and spread-seed literals for every instance, two live VCO modules in the same Rack patch are bit-identical clones today — measured 0/2048 differing samples, reproduced independently by this verifier (matches 30-REVIEW.md's own count)."
    why_human: "This is a genuine ambiguity in scope, not a code defect: the roadmap's SC4 wording and SC1's 'driven headlessly by the Phase-29 harness' framing both read most naturally as a claim about forge::VcoCore itself, which the test suite proves rigorously and non-vacuously. src/AnalogVCO.cpp's hardcoded seed was also an explicit, planned must-have in 30-05-PLAN.md (T-30-02), chosen deliberately to avoid a real Rack-hang bug from a degenerate (0,0) seed — not an oversight. No requirement in REQUIREMENTS.md asks for per-instance shell entropy in Phase 30; that pattern (random_device seeding + patch persistence) exists today only in the shipped LFO and is architecturally a Phase 34/35 concern. However, the in-source comment at src/AnalogVCO.cpp:83-85 currently asserts the seeding 'produces per-instance analog variation,' which is false for the shipped module as written — an inaccurate comment, not merely a deferred feature. Operator decision needed: (a) accept SC4 as satisfied at the VcoCore level and file a deferred-items.md / follow-up plan entry so Phase 34/35 doesn't silently inherit undocumented debt, correcting the misleading comment in the same pass; or (b) treat per-instance module divergence as in-scope for Phase 30 and open a gap."
---

# Phase 30: VcoCore Skeleton & Module Registration Verification Report

**Phase Goal:** A pitch-accurate but intentionally aliased `VcoCore` behind the proven POD boundary, registered as a second module so it appears and sounds (crudely) in Rack — proving the architecture before any hard DSP.
**Verified:** 2026-07-29T00:45:03Z
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

All four are the roadmap's literal Success Criteria for Phase 30 (`.planning/ROADMAP.md` § Phase 30).

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | `forge::VcoCore` exposes a POD `Inputs` → `step()` → output+telemetry boundary mirroring `LfoCore`, driven headlessly by the Phase-29 harness | ✓ VERIFIED | `src/dsp/VcoCore.hpp:90-204` — `VcoInputs` POD, `step(const VcoInputs&)` returning `float`, `Telemetry tel` struct populated every step. Driven headlessly by `tests/VcoBlockDriver.hpp` / `tests/test_vco_harness.cpp` / `tests/test_vco_core.cpp` with zero libRack link (`make guards` dependency-direction audit passes; `check_includes.sh` all 7 sections green — independently re-run, PASS). |
| 2 | Naive aliased morphed waveform at audio rate via `exp2_taylor5`, no static/global mutable voice state | ✓ VERIFIED | `step()` computes `freq = kVcoFreqC4 * exp2_taylor5(in.pitchCV)` (`VcoCore.hpp:164`) and calls the frozen `wave.morphedWave(...)` (`VcoCore.hpp:194`) — one call, never an edit, confirmed by `git diff --stat 93cca2f..HEAD -- src/dsp/` showing only `VcoCore.hpp` touched (108 insertions / 14 deletions; zero touches to `Waveshape.hpp`, `RackCompat.hpp`, `DriftEngine.hpp`, `MathConst.hpp`). "No static/global mutable state" is a behavior-dependent invariant, not provable by presence alone — it is proven behaviorally by `tests/test_vco_core.cpp`'s `"vco core: two-instance independence under sample-by-sample interleaving (D-17)"` (independently re-run: 1/1 passed, 18/18 assertions) plus its REQUIRED-to-fail positive control `"vco core: independence positive control - a shared static accumulator FAILS the same check"` (independently re-run: 1/1 passed — i.e. the control correctly demonstrates failure under a real shared-static core, proving the independence test is non-vacuous). |
| 3 | The VCO appears as a second selectable module via a second `addModel` + `plugin.hpp` extern + `plugin.json` `modules[]` entry, LFO registration and slug untouched | ✓ VERIFIED | `git diff 93cca2f..HEAD -- plugin.json src/plugin.cpp src/plugin.hpp` shows exactly 11 insertions, 0 deletions, entirely additive (new `modules[1]` block in `plugin.json`; one new `extern` line in `plugin.hpp`; one new `p->addModel(...)` line in `plugin.cpp`). LFO's existing lines byte-unchanged. Slug `ForgeAnalogVCO` matches `src/AnalogVCO.cpp:159`'s `createModel<AnalogVCO, AnalogVCOWidget>("ForgeAnalogVCO")` exactly. Operator UAT (`30-07-SUMMARY.md` Task 3) recorded verbatim sign-off `"Approved"` after confirming the module appears in Rack's browser as "Analog VCO" beside "Analog LFO," all four controls are audibly live, and the shipped LFO is unchanged in the same session. |
| 4 | Fixed-seed determinism holds: same seed → bit-identical block; different seed diverges | ✓ VERIFIED (at the `forge::VcoCore` level — see human-verification note on module-level scope below) | Independently re-run: `"vco harness: seam determinism (same seeds produce bit-identical blocks)"` (1/1 passed, 6/6 assertions, direct `float ==` comparison, not `Approx`) and `"vco core: spread seed divergence at character 1.0 (D-18a)"` (1/1 passed, 18/18 assertions, asserting `maxAbsDiff > 0.01 V` and >90% of samples differing between two differently-seeded cores, with a zero-character in-test negative control). Both are genuine DSP-level claims — the sweep drives real pitch/morph/character through a real double-precision accumulator and a real seeded `Waveshape`, not silent/degenerate seams. **Caveat surfaced to human verification:** `src/AnalogVCO.cpp:96-97` hardcodes identical seed literals in every `AnalogVCO` constructor, so two live module instances in the same Rack patch are bit-identical clones today (independently reproduced: 0/2048 differing samples). |

**Score:** 4/4 truths verified (0 present, behavior-unverified)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/dsp/VcoCore.hpp` | POD boundary + naive step body | ✓ VERIFIED | Exists, substantive (204 lines of real DSP + documentation), wired (included by `src/AnalogVCO.cpp`, `tests/test_vco_harness.cpp`, `tests/test_vco_core.cpp`, `src/vco_compile_canary.cpp`) |
| `tests/test_vco_harness.cpp` | Rack-free harness invariants | ✓ VERIFIED | Exists, wired into `make test` glob, 6 cases pass |
| `tests/test_vco_core.cpp` | CORE-01 + CORE-03 behavioral cases | ✓ VERIFIED | Exists, wired into `make test` glob, 5 cases pass (independently re-run individually, not just as part of the full suite) |
| `src/AnalogVCO.cpp` | Minimum-viable Rack shell, 4 controls | ✓ VERIFIED | Exists, compiles clean under `make strict` (`-std=c++11 -pedantic-errors -Wall -Wextra`), delegates 100% of DSP to `core.step(in)` — no calculation in the shell |
| `res/AnalogVCO.svg` | Throwaway 18HP panel, final geometry | ✓ VERIFIED | `91.44mm × 128.5mm` viewBox, header line byte-identical to shipped `res/AnalogLFO.svg` (confirmed via `diff`) |
| `plugin.json`, `src/plugin.hpp`, `src/plugin.cpp` | Additive registration | ✓ VERIFIED | 11 insertions / 0 deletions across all three, confirmed via `git diff` |

### Key Link Verification

| From | To | Via | Status | Details |
|------|-----|-----|--------|---------|
| `src/AnalogVCO.cpp` widget coords | `res/AnalogVCO.svg` marker rects | Four `mm2px(Vec(...))` calls | ✓ WIRED | `AnalogVCO.cpp:139-146` coordinates (30.48/40, 60.96/40, 30.48/100, 60.96/100) — reviewer independently confirmed match against the panel's marker rects; not re-measured pixel-by-pixel by this verifier but no contrary evidence found |
| `src/AnalogVCO.cpp` `modelAnalogVCO` | `src/plugin.cpp` `addModel` | Symbol resolution at link time | ✓ WIRED | `make strict` links `src/AnalogLFO.cpp src/AnalogVCO.cpp src/plugin.cpp src/vco_compile_canary.cpp` together with `-fsyntax-only`; `nm` export of both `modelAnalogLFO`/`modelAnalogVCO` was independently confirmed by the code reviewer against a scratch build with the real `../Rack-SDK` (not re-run by this verifier, no `../Rack-SDK` link-capable toolchain invoked here — covered instead by the CI `win-x64 leg reproduction` step, independently re-confirmed green below) |
| `plugin.json` slug | `src/AnalogVCO.cpp` `createModel<...>(...)` slug | String literal match | ✓ WIRED | Both are `ForgeAnalogVCO`, confirmed by direct read of both files |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Full local test suite | `make test` | 72 cases / 72 passed / 0 failed, 2,615,872 assertions | ✓ PASS |
| Guard suite | `make guards` | All sections OK, `guard suite: PASS` | ✓ PASS |
| Strict C++11 compile | `make strict` | `strict C++11 gate: PASS` over 4 TUs incl. `AnalogVCO.cpp` | ✓ PASS |
| Golden replay (LFO non-regression) | `./build-test/test --test-case="golden*"` | 6/6 passed, 49,164 assertions | ✓ PASS |
| Seam determinism (SC4, same-seed half) | `--test-case="vco harness: seam determinism*"` | 1/1 passed, 6/6 assertions | ✓ PASS |
| Spread-seed divergence (SC4, different-seed half) | `--test-case="vco core: spread seed divergence*"` | 1/1 passed, 18/18 assertions | ✓ PASS |
| Two-instance independence (CORE-03) | `--test-case="vco core: two-instance independence*"` | 1/1 passed, 18/18 assertions | ✓ PASS |
| Independence positive control | `--test-case="vco core: independence positive control*"` | 1/1 passed, 6/6 assertions | ✓ PASS |
| Hardcoded-seed clone probe (WR-02) | Standalone probe: two `VcoCore`s seeded with `AnalogVCO`'s exact literals, 2048 samples | 0/2048 differing samples | Confirms WR-02 — surfaced above |
| CI on the exact pushed commit | `gh run view 30407971115` (SHA `7933fae3`) | `toolchain-gate`, `test (windows/macos/ubuntu-latest)` all ✓ success; `win-x64 leg reproduction` step individually ✓ success | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| CORE-01 | 30-01, 30-02, 30-03, 30-05, 30-07 | New Rack-free `forge::VcoCore` mirrors `LfoCore`'s POD-`Inputs` → `step()` → output+telemetry boundary | ✓ SATISFIED | Truth 1 + Truth 2 above |
| CORE-03 | 30-04, 30-07 | `VcoCore` is self-contained per-voice, no static/global mutable state, polyphony-ready | ✓ SATISFIED | Truth 2 above (interleave independence test + REQUIRED-to-fail positive control) |
| PANEL-03 | 30-01, 30-05, 30-06, 30-07 | VCO registered as second module (`addModel` + `plugin.hpp` extern + `plugin.json` entry) without altering LFO registration | ✓ SATISFIED | Truth 3 above; also explicitly re-confirmed at the plan 30-07 phase gate per `deferred-items.md` item 1 (resolves an earlier premature `[x]` mark in REQUIREMENTS.md — confirmed genuine, not a false green) |

No orphaned requirements: `.planning/REQUIREMENTS.md` maps exactly CORE-01, CORE-03, PANEL-03 to Phase 30, and all three appear in at least one plan's `requirements:` frontmatter field.

### Anti-Patterns Found

None. Scanned all 10 files the code review covered (`src/dsp/VcoCore.hpp`, `src/AnalogVCO.cpp`, `src/plugin.cpp`, `src/plugin.hpp`, `plugin.json`, `res/AnalogVCO.svg`, `tests/test_vco_core.cpp`, `tests/test_vco_harness.cpp`, `tests/check_includes.sh`, `.github/workflows/test.yml`) for `TBD`/`FIXME`/`XXX`/`TODO`/`HACK`/`PLACEHOLDER` and "not yet implemented" language — zero hits. The unconditioned >5V output, the throwaway unlabelled panel, and the deliberately aliased waveform are documented, decided behaviors (explicitly deferred to Phases 32/34/35 in-header) rather than incomplete stubs, and are correctly excluded from the anti-pattern classification per the phase's own guardrail framing.

### Independent Code Review Findings (30-REVIEW.md) — Unresolved as of this verification

The phase's own code review (`30-REVIEW.md`, committed `717a6b0`) found 2 Critical + 5 Warning issues. This verifier independently reproduced the two Criticals and the one Warning most relevant to a roadmap success criterion (WR-02); no fix commits exist after `717a6b0`.

- **CR-01** (Critical): Nyquist guard clamp ordering bug — a non-positive `sampleRate` reintroduces a negative frequency after the floor clamp runs, producing an unbounded phase ramp and a non-finite, ~1.48e38 V output. Independently reproduced. Not reachable through the Phase-30 Rack shell as shipped (Rack always supplies `sampleRate > 0`). Routed to human verification above.
- **CR-02** (Critical): `forge::clamp` is NaN-transparent, so `VcoCore`'s defensive morph/character clamps are inert against NaN. Independently reproduced. Not reachable today (Rack sanitises param NaN before `getValue()`); becomes reachable once Phase 31/34 add MORPH/CHARACTER CV inputs (Rack does not sanitise cable voltages). `forge::clamp` itself is frozen and shared with the shipped LFO, so any fix must be local to `VcoCore`. Routed to human verification above.
- **WR-02** (Warning): `src/AnalogVCO.cpp:96-97` hardcodes the spread seed identically for every instance — independently reproduced (0/2048 differing samples between two identically-constructed `AnalogVCO`s). This bears directly on roadmap success criterion 4; see Truth 4's caveat and the human-verification item above.
- WR-01, WR-03, WR-04, WR-05, IN-01..04 were read but not independently re-reproduced by this verifier; none of them contradicts a roadmap success criterion or a phase requirement, and they are recorded in `30-REVIEW.md` with fixes proposed. Not gating.

### Human Verification Required

See frontmatter `human_verification` for the full detail (test / expected / why-human for each). Summary:

1. **CR-01 — Nyquist guard clamp ordering.** Fix now or track as pre-Phase-31 debt.
2. **CR-02 — NaN-transparent clamp on morph/character.** Fix now or track for when Phase 31/34 add CV inputs to these controls.
3. **WR-02 — hardcoded per-instance seed / SC4 module-level scope.** Confirm SC4 is satisfied at the `VcoCore` level (which is proven) as the roadmap intended, and either (a) log a `deferred-items.md` entry pointing at Phase 34/35 plus correct the now-inaccurate `AnalogVCO.cpp:83-85` comment, or (b) open a gap if per-instance shell divergence was intended to be in-scope for Phase 30.

### Gaps Summary

No must-have failed. All four roadmap Success Criteria for Phase 30 and all three requirement IDs (CORE-01, CORE-03, PANEL-03) are verified with independently-reproduced evidence: 72/72 tests passing, `make guards` and `make strict` both green, the LFO golden suite intact at 6/6 (49,164 assertions, no regression), CI green on the exact pushed commit including the win-x64 link leg, and additive-only registration diffs. The phase's own code review surfaced 2 unresolved Critical findings and 1 Warning that bears on the wording of success criterion 4; none of them falsifies a stated roadmap truth, but all three are real, reproduced, and undecided. This phase is routed to `human_needed` rather than `passed` so an operator explicitly decides whether to fix, defer-and-track, or accept these findings before Phase 31 begins driving `VcoCore` from new call sites (pitch/tuning/FM) that make CR-01/CR-02 more reachable and before Phase 34/35 wire per-instance shell entropy that would resolve WR-02.

---

_Verified: 2026-07-29T00:45:03Z_
_Verifier: Claude (gsd-verifier)_
