---
status: complete
phase: 30-vcocore-skeleton-module-registration
source: [30-VERIFICATION.md]
started: 2026-07-29T00:45:03Z
updated: 2026-07-29T01:15:00Z
---

## Current Test

[testing complete]

<!-- resolved: 1 issue (test 1, CR-01 — fix now), 2 accepted (tests 2 and 3)
number: 3
name: WR-02 — every live VCO instance in a patch is a bit-identical clone
expected: |
  Roadmap success criterion 4 states "same seed → bit-identical block; different seed
  diverges." At the `forge::VcoCore` level this is fully satisfied and proven
  non-vacuously by two passing tests. But `src/AnalogVCO.cpp:96-97` hardcodes the seed
  and spread-seed literals for every instance, so two live VCO modules in the same Rack
  patch are bit-identical clones — measured 0/2048 differing samples, reproduced
  independently by the verifier.

  This is a scope ambiguity, not a code defect. The hardcoded seed was an explicit
  planned must-have in 30-05 (T-30-02), chosen deliberately to avoid a real Rack-hang
  bug from a degenerate (0,0) seed. No requirement in REQUIREMENTS.md asks for
  per-instance shell entropy in Phase 30.

  HOWEVER: the comment at `src/AnalogVCO.cpp:83-85` asserts the seeding "produces
  per-instance analog variation," which is FALSE for the shipped module as written.
  That part is an inaccurate comment, not deferred work.

  DECISION: (a) accept SC4 as satisfied at the VcoCore level, file the follow-up so
  Phase 34/35 doesn't inherit undocumented debt, and correct the misleading comment in
  the same pass; or (b) treat per-instance module divergence as in-scope for Phase 30
  and open a gap.
awaiting: user response
-->


## Tests

### 1. CR-01 — Nyquist guard clamp ordering produces a non-finite runaway
expected: Drive `forge::VcoCore::step()` directly with `in.sampleRate = -44100.f`, `pitchCV = 0`, `morph = 0.5`, `character = 1.0` for 20000 steps. Output reaches ~1.48e38 V and goes non-finite, contradicting the guard's own "LOAD-BEARING" comment. Fix is a one-line clamp-order swap, bit-identical for all finite positive rates so no golden can move.
result: issue
reported: "fix now"
severity: major

### 2. CR-02 — `forge::clamp` is NaN-transparent, so VcoCore's defensive clamps are inert
expected: `src/dsp/VcoCore.hpp:187-188` clamps morph/character as its "only defensive validation." `forge::clamp` is a comparison ladder (`x < lo ? lo : (x > hi ? hi : x)`) — both comparisons are false for NaN, so NaN passes through unchanged and `step()` emits a non-finite sample straight to the module output. This diverges silently from `rack::math::clamp`, which is `fmax(fmin(...))` and discards NaN. Not reachable today (Rack's `ParamQuantity::setValue` sanitises NaN), but becomes reachable the moment Phase 31/34 add MORPH/CHARACTER CV inputs, since Rack does not sanitise cable voltages. **`forge::clamp` is FROZEN and consumed by the shipped LFO** (`src/dsp/LfoCore.hpp:168,212-213,216`), so any fix must be a local VcoCore-scoped helper, never an edit to the shared primitive.
result: pass
reported: "Pass"
note: "Accepted for Phase 30 — not reachable while Rack sanitises params. Becomes reachable when Phase 31/34 add MORPH/CHARACTER CV inputs; track there. Any future fix must be a local VcoCore-scoped helper, never an edit to the frozen shared `forge::clamp`."

### 3. WR-02 — every live VCO instance in a patch is a bit-identical clone
expected: Roadmap success criterion 4 states "same seed → bit-identical block; different seed diverges." At the `forge::VcoCore` level this is fully satisfied and proven non-vacuously by two passing tests. But `src/AnalogVCO.cpp:96-97` hardcodes the seed and spread-seed literals for every instance, so two live VCO modules in the same Rack patch are bit-identical clones — measured 0/2048 differing samples, reproduced independently by the verifier.

  This is a scope ambiguity, not a code defect. The hardcoded seed was an explicit planned must-have in 30-05 (T-30-02), chosen deliberately to avoid a real Rack-hang bug from a degenerate (0,0) seed. No requirement asks for per-instance shell entropy in Phase 30. However, the comment at `src/AnalogVCO.cpp:83-85` asserts the seeding "produces per-instance analog variation," which is false for the shipped module as written — an inaccurate comment, not merely a deferred feature.

  DECISION: (a) accept SC4 as satisfied at the VcoCore level, file the follow-up so Phase 34/35 doesn't inherit undocumented debt, and correct the misleading comment in the same pass; or (b) treat per-instance module divergence as in-scope for Phase 30 and open a gap.
result: pass
reported: "pass"
note: "Option (a). SC4 accepted as satisfied at the `forge::VcoCore` level — proven non-vacuously by 'vco harness: seam determinism' and 'vco core: spread seed divergence at character 1.0'. Per-instance shell entropy is NOT Phase 30 scope; deferred to Phase 34/35 and filed in deferred-items.md. One action carried into gap closure: correct the false comment at src/AnalogVCO.cpp:83-85, which claims the hardcoded seeding 'produces per-instance analog variation'."

## Summary

total: 3
passed: 2
issues: 1
pending: 0
skipped: 0
blocked: 0

## Gaps

- truth: "The Nyquist guard in `forge::VcoCore::step()` bounds the output — the header at src/dsp/VcoCore.hpp:167-172 documents it as LOAD-BEARING, and 30-02 asserted it as a must-have."
  status: failed
  reason: "User reported: fix now — the ceiling clamp runs after the floor clamp, so a negative sampleRate re-introduces a negative frequency; phase ramps unboundedly negative and output reaches ~1.48e38 V, non-finite. Reproduced independently by both the code reviewer and the verifier."
  severity: major
  test: 1
  artifacts: []
  missing: []
