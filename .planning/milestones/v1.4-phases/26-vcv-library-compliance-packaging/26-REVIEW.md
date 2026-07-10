---
phase: 26-vcv-library-compliance-packaging
reviewed: 2026-07-09T02:27:57Z
depth: standard
files_reviewed: 10
files_reviewed_list:
  - Makefile
  - plugin.json
  - src/AnalogLFO.cpp
  - src/dsp/DriftEngine.hpp
  - src/dsp/LfoCore.hpp
  - src/dsp/MathConst.hpp
  - src/dsp/Waveshape.hpp
  - tests/test_extraction.cpp
  - tests/test_golden.cpp
  - tools/capture_golden.cpp
findings:
  critical: 0
  warning: 2
  info: 3
  total: 5
status: issues_found
---

# Phase 26: Code Review Report

**Reviewed:** 2026-07-09T02:27:57Z
**Depth:** standard
**Files Reviewed:** 10
**Status:** issues_found

## Summary

Phase 26 does three things: (1) replaces `M_PI` with a portable `forge::kPi` constexpr across
the DSP core, (2) adds a cross-platform drift-OFF golden capture/replay path, and (3) finalizes
`plugin.json` for VCV Library submission. The three focus areas the task flagged all check out:

- **Constant substitution — correct and complete.** Every `M_PI` site was replaced 1:1 preserving
  the exact cast structure (`(float)M_PI` → `(float)forge::kPi`; `-0.75 * M_PI` → `-0.75 * forge::kPi`).
  Because `forge::kPi` is the identical IEEE-754 double as the cmath macro (`0x400921FB54442D18`),
  every substitution is **bit-for-bit unchanged** — no float-vs-double intermediate-precision drift
  was introduced, so the golden fixtures are genuinely unperturbed. A full-tree grep confirms **zero**
  `M_PI` remaining in `src/`, and every one of the 5 files that references `kPi` includes
  `dsp/MathConst.hpp`. No missed sites, no type mismatches.
- **Golden determinism — sound.** The capture tool (`tools/capture_golden.cpp`) and the replay
  (`test_golden.cpp::replayGoldenDriftOff`) both construct `forge::LfoCore` directly and call ONLY
  `core.seed(...)`, correctly bypassing `BlockDriver`'s constructor which would seed the
  `std::normal_distribution` spread path. With `drift = 0.0`, `DriftEngine::step` early-returns
  before any RNG draw and `*Spread` stay at their `0.f` defaults, so the two paths are byte-identical
  on the same platform. `-ffp-contract=off` in `TEST_CXXFLAGS` (shared by both `test` and `capture`)
  removes FMA-contraction divergence. The scenario parameters match between generator and replay.
- **Manifest — valid.** Slugs, SPDX license, tags, and required fields are all well-formed for VCV
  Library. See WR-01 for the one compliance risk worth verifying.

No blocking defects. Two warnings (one compliance-verification gap, one load-bearing documentation
inaccuracy about the cross-platform epsilon rationale) and three minor items.

## Structural Findings (fallow)

No structural pre-pass was provided for this review.

## Narrative Findings (AI reviewer)

## Warnings

### WR-01: minRackVersion lowered to 2.0.0 without a documented API-surface audit

**File:** `plugin.json:5`
**Issue:** `minRackVersion` was changed from `"2.6.0"` to `"2.0.0"`. This assertion tells the VCV
Library and the Rack loader that the module is compatible with every Rack 2.x back to 2.0.0. But the
plugin is built against `../Rack-SDK` (a 2.6-era SDK) and uses several APIs whose exact introduction
version must be confirmed to be ≤ 2.0.0 — e.g. `ParamQuantity::getScaledValue()`
(`AnalogLFO.cpp:279`), `Module::isBypassed()` (`AnalogLFO.cpp:1019`), `createIndexSubmenuItem`
(`AnalogLFO.cpp:1179`), `drawLayer` self-illumination (`AnalogLFO.cpp:996`),
`Window::getLastFrameDuration()` (`AnalogLFO.cpp:419`), and `Widget::addChildBelow`
(`AnalogLFO.cpp:1048`). If ANY of these was introduced after 2.0.0, the plugin will fail to
load or crash on Rack 2.0–2.5 while still advertising support for them. The change looks intentional
(RESEARCH references it), but nothing in the phase records an audit proving the used API set exists in
2.0.0.
**Fix:** Either (a) compile and smoke-test against the Rack 2.0.0 SDK headers to prove the API surface
resolves, or (b) raise `minRackVersion` to the lowest version actually validated (a conservative
`"2.0.0"` is only safe once the audit is on record). Document the audit result in the phase notes so
the claim is defensible at VCV submission time.

### WR-02: Cross-platform epsilon rationale misstates the operations that produce the divergence

**File:** `tests/test_golden.cpp:16-17` (and mirrored in `tests/golden/freerun_seeds.txt`)
**Issue:** The header comment justifies the `1e-6` drift-off tolerance by claiming the path "touches
ONLY the portable Xoroshiro uniform + libm sin/cos". Tracing the drift-off scenario
(`drift = 0.0`, `character = 0.6`, `morph = 0.4`) shows this is inaccurate in two load-bearing ways:
1. **Xoroshiro is never touched.** With `drift < 0.001f`, `DriftEngine::step` returns before any
   `normalDist(rng)` draw, and the clock path is disconnected — so the portable integer RNG the
   rationale leans on plays no role at all.
2. **The path additionally exercises `std::exp` and `std::tanh`, not just sin/cos.** `computeSaw`
   calls `std::exp` (`Waveshape.hpp:88`) and `computeSquare`/`computePulse` call `std::tanh`
   (`Waveshape.hpp:120,148`); both are reached every sample because `character = 0.6 > 0.001` and the
   morph position folds saw/square/pulse into the output. `exp` and `tanh` are NOT correctly-rounded
   in libm and diverge across libstdc++/libc++/MinGW by more than `sin`/`cos` typically do. The `1e-6`
   margin may still hold empirically, but the documented reasoning for WHY it holds is wrong, which
   undermines the guard: a future toolchain change that widens `exp`/`tanh` error past `1e-6` would
   break the test with a rationale that never accounted for those functions.
**Fix:** Correct the comment (both here and in `freerun_seeds.txt`) to state the actual transcendental
surface: portable phase accumulation (double, bit-exact) + libm `sin`/`cos`/`exp`/`tanh`/`sqrt`, and
tie the `1e-6` choice to the worst-case of those (exp/tanh), not sin/cos alone. If the intent was to
keep the guard on a narrow, well-behaved op set, consider a `character = 0.0` scenario (which bypasses
exp/tanh entirely and leaves only sin), which would make the stated rationale true.

## Info

### IN-01: Mixing `test`/`capture` with plugin goals silently breaks the build

**File:** `Makefile:22`
**Issue:** `ifeq ($(filter test capture,$(MAKECMDGOALS)),)` skips the `plugin.mk` include whenever
`test` or `capture` appears anywhere in the goal list. Invoking `make test install` (or
`make capture all`) matches the filter, skips `plugin.mk`, and leaves `install`/`all` undefined —
make errors out confusingly rather than doing either job. Standalone `make test` / `make capture` /
`make` / `make dist` all behave correctly; only mixed goals are affected.
**Fix:** Document that `test`/`capture` must be invoked alone, or detect a mixed goal set and emit a
clear `$(error ...)` explaining the two target families are mutually exclusive in one invocation.

### IN-02: `make capture` has no canonical-OS guard and overwrites committed fixtures

**File:** `tools/capture_golden.cpp:79-84`, `Makefile:56-62`
**Issue:** The drift-off fixtures are documented as captured on macOS (`freerun_seeds.txt`), but
`make capture` will happily regenerate `tests/golden/freerun_*_driftoff.f32` on any OS, overwriting
the committed canonical bytes with locally-produced ones. Because the fixtures are only expected to
agree to `~1e-6` cross-platform, a well-meaning regeneration on Linux/Windows silently swaps the
canonical baseline for a non-canonical one. Low blast radius (replay still passes within epsilon), but
it erodes provenance.
**Fix:** Gate the writer on the canonical OS (e.g. `#if !defined(__APPLE__)` → print a refusal and
exit non-zero), or print a prominent warning that the output is non-canonical when built off macOS.

### IN-03: Per-sample `CHECK` in an 8192-iteration loop floods output on a fixture mismatch

**File:** `tests/test_golden.cpp:107` (drift-off) and `:129` (drift-on)
**Issue:** The comparison uses `CHECK` inside a loop over all 8192 samples. `CHECK` does not abort, so
a genuinely broken fixture or regression produces up to 8192 individual failure lines, burying the
signal. This is a pre-existing pattern carried into the new drift-off leg.
**Fix:** Track a mismatch count / first-offending index and assert once after the loop (or `break` on
first failure with a `CHECK` that reports the index), so a regression reports a single actionable line.

---

_Reviewed: 2026-07-09T02:27:57Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
