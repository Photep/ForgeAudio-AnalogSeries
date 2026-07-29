---
status: complete
phase: 30-vcocore-skeleton-module-registration
source: [30-VERIFICATION.md]
started: 2026-07-29T04:02:21Z
updated: 2026-07-29T04:35:00Z
note: |
  Second UAT round, opened by the gap-closure re-verification. The first round
  (2 passed, 1 issue — the issue being WR-02, closed by plan 30-09) is preserved
  at 30-UAT-pre-gap-closure.md.

  Only test 3 needed a decision. Tests 1 and 2 are closure confirmations for
  items the operator already resolved in round one; they are carried here so the
  re-verification record shows them traced through rather than silently dropped.

  OUTCOME: the operator chose option (b), fix now. WR-06 was fixed in plan 30-11
  (commits fdddb4a RED / a01921a fix), not deferred. IN-05, the remaining
  un-owned finding from the same re-review, was filed as deferred-items.md
  item 6 with owner Phase 32.
---

## Current Test

[testing complete]

## Resolved Test

number: 3
name: WR-06 — the Nyquist ceiling silently no-ops when `sampleRate` is NaN
expected: |
  Reproduced independently by both the gap-closure code review and the verifier:
  drive `forge::VcoCore::step()` with `in.sampleRate = NaN`, `in.pitchCV = +10`,
  `morph = 0.5`, `character = 1.0`. `maxFreq` becomes NaN, so the comparison
  `freq > maxFreq` is false for every value and the ceiling never fires.
  Measured: `tel.freqHz = 267904.625` Hz — unbounded relative to any Nyquist limit.

  The audio output itself stays safe (`out = 5.0`, finite, in-bound), but only
  because the independent `kVcoMaxDeltaPhase` bound added by 30-08 masks it — a
  different mechanism than the one whose declared job this is. The header comment
  calls that guard LOAD-BEARING.

  This does not fail any roadmap Success Criterion or any of CORE-01 / CORE-03 /
  PANEL-03 as worded. It is the only open finding in the phase without a named
  owner in deferred-items.md.

  Decision needed:
    (a) Accept as follow-on debt — log in deferred-items.md with a named owner
        before Phase 31 starts driving this seam from new call sites.
    (b) Fix now — the reviewer's suggested fix is one line, and is
        bit-identical for every finite positive sample rate.
result: |
  FIXED. Operator chose (b).

  Plan 30-11, RED first per this phase's standing discipline:
    fdddb4a  test — freqNyquistBounded added to scenario four, observed RED:
             12 failures, all freqNyquistBounded, ALL confined to rate := nan
             (6 hostile timings x 2 pitches). maxFreqSeen := 267905 at
             pitchCV = 10 and 261.626 at pitchCV = 0, both against
             expectedMaxFreq := 0. Every other rate class already passed,
             confirming the assertion pins WR-06 specifically.
    a01921a  fix — the rate is now sanitised BEFORE it is scaled:
             `const float safeRate = (in.sampleRate > 0.f) ? in.sampleRate : 0.f;`
             Written positively rather than negated, because here the NaN case
             wants the fallback branch, not pass-through.

  Suite 72/72, 2,616,112 assertions (the 48 added are exactly 48 configs x 1
  assertion). make strict and make guards both PASS.

  Bit-identity for every finite positive rate proven mechanically by 30-08's
  method: full doctest -s transcripts of the pre-existing suite before and
  after, 9,357,414 lines each, differ on exactly 7 lines — all one artifact,
  doctest rendering a const char* operand as a pointer whose address moved
  with the __TEXT layout. The measured side (the sha256 digests) is
  byte-identical, and they belong to test_lfo_guardrail.cpp's sha256
  self-test vectors, unrelated to VcoCore.

## Tests

### 1. CR-02 — `forge::clamp` is NaN-transparent (closure confirmation only)
expected: `forge::clamp(NaN, 0.f, 1.f)` returns NaN, so `step()`'s morph/character clamps are inert against NaN. Not reachable today — Rack sanitises param NaN before `getValue()` — and becomes reachable once Phase 31/34 add MORPH/CHARACTER CV inputs. Already accepted by the operator at round-one UAT test 2 and recorded in `deferred-items.md` item 3 with owner Phase 31 or 34, constrained so the fix must be a VcoCore-local NaN-safe helper and never an edit to the frozen shared `forge::clamp`. No new action expected.
result: [pending]

### 2. WR-02 module-level scope — every `AnalogVCO` instance is a bit-identical clone (closure confirmation only)
expected: Roadmap SC4 ("same seed → bit-identical block; different seed diverges") holds at the `forge::VcoCore` level, proven by two passing tests. `src/AnalogVCO.cpp` still hardcodes identical seed and spread-seed literals in every constructor, so two live VCO modules in one patch measure 0/2048 differing samples. Already resolved by the operator at round-one UAT test 3, option (a): SC4 accepted as satisfied at the VcoCore level, per-instance shell entropy explicitly out of Phase-30 scope, filed as `deferred-items.md` item 2 with owner Phase 34/35 and a re-validate-on-deserialize constraint. The misleading comment was corrected in the same pass by plan 30-09. No new action expected.
result: [pending]

### 3. WR-06 — Nyquist ceiling silently no-ops when `sampleRate` is NaN (decision required)
expected: See Resolved Test above. Accept-and-track with a named owner, or fix now.
result: fixed — operator chose (b); plan 30-11, commits fdddb4a (RED) and a01921a (fix)

## Summary

total: 3
passed: 3
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps

None open. WR-06 was fixed rather than deferred. IN-05, the only other finding
surfaced by the gap-closure re-review, is filed as `deferred-items.md` item 6
with owner Phase 32 — so every open finding in this phase now has a named owner.
