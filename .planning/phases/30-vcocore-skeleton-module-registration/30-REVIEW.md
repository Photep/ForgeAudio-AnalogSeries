---
phase: 30-vcocore-skeleton-module-registration
reviewed: 2026-07-29T00:00:00Z
depth: standard
files_reviewed: 3
files_reviewed_list:
  - src/dsp/VcoCore.hpp
  - src/AnalogVCO.cpp
  - tests/test_vco_core.cpp
findings:
  critical: 0
  warning: 1
  info: 1
  total: 2
status: issues_found
---

# Phase 30: Code Review Report (gap-closure re-review)

**Reviewed:** 2026-07-29
**Depth:** standard
**Files Reviewed:** 3
**Status:** issues_found

## Summary

Re-review of commits `679ef0e` (WR-03 test), `a518345` (CR-01 fix), `4cc5cc7`
(WR-02 comment) against `30-REVIEW-pre-gap-closure.md`. Verification method: full
static trace of every guard branch in `src/dsp/VcoCore.hpp` against nine input
classes per POD field (finite-in-range, zero, negative, `NaN`, `+inf`, `-inf`,
subnormal, very-large-finite) on `sampleRate` and `sampleTime` independently, plus
`make test` (72 test cases, 2,616,064 assertions, 0 failures on the current tree).
Per the review's read-only constraint, no source files were edited to empirically
force a red run; the CR-01 revert-and-observe claim in the header/test comments
was cross-checked mathematically instead of by mutating and rebuilding.

**CR-01 — CONFIRMED FIXED.** The new order (`if (freq > maxFreq) freq = maxFreq;`
then `if (!(freq > 0.f)) freq = 0.f;`, `VcoCore.hpp:207-208`) makes the NaN-safe
floor the last writer unconditionally. Traced by hand for `in.sampleRate ∈
{-44100, 0, -inf, subnormal-negative, NaN}`: in every case `freq` ends at either a
non-negative clamp of `maxFreq` or `0.f`, never negative. The reproduced CR-01
scenario (`sampleRate = -44100`, `pitchCV = 0`) now yields `freq = 0` immediately
(ceiling first writes a negative `maxFreq`, floor immediately zeros it), matching
the fix's own claim.

**The new `kVcoMaxDeltaPhase = 0.5` bound is correctly enforced on the only path
that advances `phase`.** `deltaPhase`'s own guard order (floor-then-ceiling,
`VcoCore.hpp:234-235`) is safe specifically because the floor's target (`0.0`) is
below the ceiling (`0.5`) — unlike the frequency guard's original bug, applying
the ceiling after the floor here can never reintroduce a bad value. Traced for
`in.sampleTime ∈ {-1/44100, 0, 999, NaN, +inf, -inf}` crossed against the
`sampleRate` classes above: `deltaPhase` always lands in `[0, 0.5]`, so
`phase += deltaPhase; if (phase >= 1.0) phase -= 1.0;` always leaves `phase ∈
[0, 1)`. This closes WR-01 as a side effect, which the plan's own summaries
(`30-08-SUMMARY.md:89`, `30-10-SUMMARY.md:229`) already claim — confirmed here
independently rather than re-reported as still-open.

**WR-03 — CONFIRMED CLOSED, non-vacuously.** Scenario four (`test_vco_core.cpp:
617-739`) constructs `forge::VcoInputs` directly and calls `core.step(in)` with no
driver in the loop, so nothing overwrites the hostile `sampleTime`/`sampleRate`
before `step()` sees them — genuinely bypassing the unconditional-overwrite
pattern in `VcoBlockDriver::run` and `runInterleaveCheck` that made hostile timing
structurally unreachable before this plan. Hand-traced against the pre-fix guard
order for the reproduced case (`rate = -44100`, any `dt`, either pitch): `freq`
would end negative regardless of `dt`, so `freqNonNegative`, `phaseInRange`,
`allFinite` and the magnitude bound would all fail — the assertions are not
vacuous for the case they were written to catch.

**WR-02 — CONFIRMED CLOSED, comment-only.** `git diff` confirms
`src/AnalogVCO.cpp` changed in comment text only; the seed literals at what is now
`AnalogVCO.cpp:114-115` are byte-identical to before. The corrected comment
(`AnalogVCO.cpp:86-100`) now states the divergence is "from an UNSPREAD default
core — not divergence from the next VCO the user adds," which matches the
0-of-2048 clone measurement recorded in `deferred-items.md` item 2 and no longer
claims "per-instance analog variation" the code does not have.

One new gap survived the fix, recorded below as WR-06: the reordered frequency
ceiling is a no-op specifically when `in.sampleRate` is `NaN`, so `tel.freqHz` can
carry an un-Nyquist-limited value in that one case — audio safety is preserved
only because it is masked by the independent `deltaPhase` bound, and the new test
does not check for it because `freqNonNegative` is satisfied trivially.

CR-02, WR-04, WR-05, IN-01, IN-02, IN-03, IN-04 are unchanged by this gap closure
and remain open, tracked in `deferred-items.md`; not re-reported here.

## Warnings

### WR-06: the reordered frequency ceiling silently no-ops when `in.sampleRate` is `NaN`, leaving `tel.freqHz` unbounded and untested

**File:** `src/dsp/VcoCore.hpp:176, 207-209`

**Issue:** `maxFreq = kVcoNyquistGuardFrac * in.sampleRate`. When `in.sampleRate`
is `NaN`, `maxFreq` is `NaN`. The ceiling comparison `freq > maxFreq` is then
`false` for any `freq` (every comparison against `NaN` is `false`), so the
ceiling never fires and `freq` passes through **unclamped** — it keeps whatever
value `kVcoFreqC4 * exp2_taylor5(in.pitchCV)` produced, with no relationship to
any Nyquist limit. Because that raw `freq` is always finite and positive for the
pitch values this phase reads, it also clears the very next line, `if (!(freq >
0.f)) freq = 0.f;` (positive fails the "not greater than zero" test), so the
floor does not catch it either. `tel.freqHz` is assigned this unclamped value one
line later.

Traced with `in.sampleRate = NaN`, `in.pitchCV = 10`: `freq ≈ 267,904 Hz` reaches
`tel.freqHz` completely unguarded — a "PROVISIONAL … Nyquist policy" the header
calls "LOAD-BEARING" is, for this one input class, not applied at all.

This is not a BLOCKER because the output stays safe: `deltaPhase = freq *
in.sampleTime` is bounded independently by `kVcoMaxDeltaPhase` (floor-then-ceiling
on `deltaPhase` itself, `VcoCore.hpp:234-235`), so `phase` and the returned sample
stay finite and in-bound regardless of how large the unguarded `freq` gets. But
the frequency guard's own job — bounding `freq`/`tel.freqHz` to a Nyquist-relative
ceiling — is not done for this case, and the new hostile-timing scenario does not
notice: it only asserts `core.tel.freqHz >= 0.f` (`test_vco_core.cpp:721`), which
this case satisfies trivially since the unclamped `freq` is positive, not
negative. Phase 35 is the named future consumer of `tel.freqHz` for a display
(`VcoCore.hpp:133`); a `NaN` `sampleRate` reaching a display through
`tel.freqHz` would show an arbitrary, non-Nyquist-relative number rather than the
`0` a caller reading the "guard is LOAD-BEARING" comment would reasonably expect.

**Fix:** add a `NaN`-safe fallback for `maxFreq` itself, or floor `freq` to
`kVcoNyquistGuardFrac` times a sanitized rate, e.g.:

```cpp
const float safeRate = (in.sampleRate > 0.f) ? in.sampleRate : 0.f;  // NaN and negative -> 0
const float maxFreq = kVcoNyquistGuardFrac * safeRate;
if (freq > maxFreq) freq = maxFreq;
if (!(freq > 0.f)) freq = 0.f;
```

With this change a `NaN` `in.sampleRate` yields `maxFreq = 0`, so `freq` (and
`tel.freqHz`) is correctly zeroed rather than left unguarded. Add a fifth
assertion to scenario four that would catch this directly, e.g. `CHECK(maxFreqSeen
<= 0.49 * std::max(0.f, rate))` captured per-step, or more simply extend
`freqNonNegative`'s companion check to also require `core.tel.freqHz <=
kVcoNyquistGuardFrac * std::max(0.0f, rate)` whenever `rate` is finite.

## Info

### IN-05: the hostile-timing grid (scenario four) does not include `+inf`/`-inf`/subnormal/very-large-finite values on either `sampleRate` or `sampleTime`

**File:** `tests/test_vco_core.cpp:672-681`

**Issue:** `HOSTILE_RATES` covers `{-44100, 0, 44100, NaN}` and `HOSTILE_TIMES`
covers `{-1/44100, 0, 1/44100, 1/1000, 999, NaN}`. Neither includes `+inf`,
`-inf`, a subnormal, or a very-large finite value (e.g. `FLT_MAX`). Hand-tracing
those classes through the current guard order shows them handled safely (the
`deltaPhase` floor/ceiling pair catches every combination regardless of how
`freq` or `maxFreq` behaves at the extremes), so this is not evidence of a
defect — but it is untested rather than proven-safe-by-a-passing-assertion, and
`+inf`/`-inf` are exactly the values a malformed host `ProcessArgs` or a future
oversampled inner loop (the same Phase-32 shape this plan's own comments cite)
could plausibly produce.

**Fix:** append `std::numeric_limits<float>::infinity()`,
`-std::numeric_limits<float>::infinity()`, and
`std::numeric_limits<float>::denorm_min()` to both `HOSTILE_RATES` and
`HOSTILE_TIMES` the next time this grid is touched, so the safety property is
demonstrated rather than only reasoned about.

---

_Reviewed: 2026-07-29_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
