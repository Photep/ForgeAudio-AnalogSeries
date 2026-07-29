---
status: testing
phase: 30-vcocore-skeleton-module-registration
source: [30-VERIFICATION.md]
started: 2026-07-29T00:45:03Z
updated: 2026-07-29T00:45:03Z
---

## Current Test

number: 1
name: CR-01 — Nyquist guard clamp ordering produces a non-finite runaway
expected: |
  The header comment at `src/dsp/VcoCore.hpp:167-172` claims the Nyquist guard is
  "LOAD-BEARING" and bounds the output. Independently reproduced by both the code
  reviewer and the verifier: the ceiling clamp runs AFTER the floor clamp, so a
  negative `sampleRate` re-introduces a negative frequency, the phase accumulator
  ramps unboundedly negative, and the output reaches ~1.48e38 V and goes non-finite
  — the exact failure the guard's own comment says it prevents.

  Not reachable through the Phase-30 Rack shell as shipped (Rack always supplies
  sampleRate > 0), so it violates none of the four roadmap success criteria as
  literally worded. But `VcoCore` is a public seam that Phases 31-34 and future
  polyphony will call from new sites, and a 30-02 must-have explicitly asserts this
  guard is load-bearing — an assertion now proven false.

  DECISION: fix now (one-line clamp-order swap; the reviewer supplies the patch and
  a WR-03 coverage case), or accept as tracked follow-up debt before Phase 31 begins
  driving this seam from new call sites.
awaiting: user response

## Tests

### 1. CR-01 — Nyquist guard clamp ordering produces a non-finite runaway
expected: Drive `forge::VcoCore::step()` directly with `in.sampleRate = -44100.f`, `pitchCV = 0`, `morph = 0.5`, `character = 1.0` for 20000 steps. Output reaches ~1.48e38 V and goes non-finite, contradicting the guard's own "LOAD-BEARING" comment. Fix is a one-line clamp-order swap, bit-identical for all finite positive rates so no golden can move.
result: [pending]

### 2. CR-02 — `forge::clamp` is NaN-transparent, so VcoCore's defensive clamps are inert
expected: `src/dsp/VcoCore.hpp:187-188` clamps morph/character as its "only defensive validation." `forge::clamp` is a comparison ladder (`x < lo ? lo : (x > hi ? hi : x)`) — both comparisons are false for NaN, so NaN passes through unchanged and `step()` emits a non-finite sample straight to the module output. This diverges silently from `rack::math::clamp`, which is `fmax(fmin(...))` and discards NaN. Not reachable today (Rack's `ParamQuantity::setValue` sanitises NaN), but becomes reachable the moment Phase 31/34 add MORPH/CHARACTER CV inputs, since Rack does not sanitise cable voltages. **`forge::clamp` is FROZEN and consumed by the shipped LFO** (`src/dsp/LfoCore.hpp:168,212-213,216`), so any fix must be a local VcoCore-scoped helper, never an edit to the shared primitive.
result: [pending]

### 3. WR-02 — every live VCO instance in a patch is a bit-identical clone
expected: Roadmap success criterion 4 states "same seed → bit-identical block; different seed diverges." At the `forge::VcoCore` level this is fully satisfied and proven non-vacuously by two passing tests. But `src/AnalogVCO.cpp:96-97` hardcodes the seed and spread-seed literals for every instance, so two live VCO modules in the same Rack patch are bit-identical clones — measured 0/2048 differing samples, reproduced independently by the verifier.

  This is a scope ambiguity, not a code defect. The hardcoded seed was an explicit planned must-have in 30-05 (T-30-02), chosen deliberately to avoid a real Rack-hang bug from a degenerate (0,0) seed. No requirement asks for per-instance shell entropy in Phase 30. However, the comment at `src/AnalogVCO.cpp:83-85` asserts the seeding "produces per-instance analog variation," which is false for the shipped module as written — an inaccurate comment, not merely a deferred feature.

  DECISION: (a) accept SC4 as satisfied at the VcoCore level, file the follow-up so Phase 34/35 doesn't inherit undocumented debt, and correct the misleading comment in the same pass; or (b) treat per-instance module divergence as in-scope for Phase 30 and open a gap.
result: [pending]

## Summary

total: 3
passed: 0
issues: 0
pending: 3
skipped: 0
blocked: 0

## Gaps
