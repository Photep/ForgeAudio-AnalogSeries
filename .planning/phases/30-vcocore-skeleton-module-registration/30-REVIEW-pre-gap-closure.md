---
phase: 30-vcocore-skeleton-module-registration
reviewed: 2026-07-29T00:00:00Z
depth: standard
files_reviewed: 10
files_reviewed_list:
  - src/dsp/VcoCore.hpp
  - src/AnalogVCO.cpp
  - src/plugin.cpp
  - src/plugin.hpp
  - plugin.json
  - res/AnalogVCO.svg
  - tests/test_vco_core.cpp
  - tests/test_vco_harness.cpp
  - tests/check_includes.sh
  - .github/workflows/test.yml
findings:
  critical: 2
  warning: 5
  info: 4
  total: 11
status: issues_found
---

# Phase 30: Code Review Report

**Reviewed:** 2026-07-29
**Depth:** standard
**Files Reviewed:** 10
**Status:** issues_found

## Summary

Ten files reviewed at standard depth against the Phase 30 diff (`93cca2f..HEAD`). The
deliberate behaviors named in the phase context — aliasing, the >5 V unconditioned
output, the stub panel, the four-control shell, the `DeliberatelyBrokenSharedStateCore`
positive control, and the `[2/7]` `dsp/RackCompat.hpp` exemption — were treated as
decided and are **not** reported.

Findings concentrate in `src/dsp/VcoCore.hpp`. Both Critical findings are the same
shape: **code whose only job is to be a guard does not guard.** Each was reproduced
numerically, not inferred:

| Probe | Result |
|---|---|
| `sampleRate = -44100`, `morph = 0.5`, 20 000 steps | `freqHz = -21609`, `phase = -9800`, `maxAbs = 1.48e38 V`, **`isfinite = 0`** |
| `morph = NaN`, `character = NaN` | `forge::clamp(NaN,0,1) = NaN`, **output non-finite** |
| `sampleTime = 1/1000` with `sampleRate = 44100`, `pitchCV = +6` | `phase = 314 880` (unbounded ramp) |
| Two cores seeded exactly as `AnalogVCO()` seeds them | **0 / 2048 samples differ** — bit-identical clones |

Neither Critical is reachable through the *Rack shell as shipped today* (Rack always
supplies `sampleRate > 0`, and `rack::math::clamp` is `fmax(fmin(...))` so Rack
sanitises NaN before a param reaches `getValue()`). They are classified Critical anyway
because (a) the header documents both mechanisms as load-bearing safety and both claims
are provably false, (b) `VcoCore` is a public seam that Phases 31/32/33/34 and v2.1
polyphony will drive from call sites other than `VcoBlockDriver`, and (c) each fix is
one or two lines with zero effect on finite-input results — so no golden can move.

Positive checks that came back clean, recorded so they are not re-litigated:
`src/AnalogVCO.cpp` compiles silently under `-std=c++11 -pedantic-errors -Wall -Wextra
-Wshadow -Wconversion`; `src/vco_compile_canary.cpp` likewise; `bash
tests/check_includes.sh` returns PASS with all seven sections green; no frozen shared
header was touched (`git diff --stat` shows `src/dsp/VcoCore.hpp` as the only file under
`src/dsp/`), so no LFO regression vector was found; the four widget coordinates in
`src/AnalogVCO.cpp:139-146` match the four marker rect centres in `res/AnalogVCO.svg`
exactly; `plugin.json`'s slug matches `createModel<...>("ForgeAnalogVCO")` character for
character; and `-ffp-contract=off` in `TEST_CXXFLAGS` means the suite's bit-exact float
comparisons are not a cross-toolchain flake risk.

## Critical Issues

### CR-01: Nyquist guard applies the zero-floor before the ceiling, so a non-positive sample rate produces a negative frequency and an unbounded phase ramp

**File:** `src/dsp/VcoCore.hpp:167-184`

**Issue:** The two clamps run in the wrong order.

```cpp
if (!(freq > 0.f)) freq = 0.f;   // floor first
if (freq > maxFreq) freq = maxFreq;   // ceiling second — can REINTRODUCE a negative
```

`maxFreq = kVcoNyquistGuardFrac * in.sampleRate`. For any `in.sampleRate < 0`, `maxFreq`
is negative, `freq > maxFreq` is true, and the ceiling clamp writes a **negative**
frequency straight over the value the floor just sanitised. The floor is therefore not a
floor at all.

The consequence is not cosmetic, because the wrap has no negative branch:

```cpp
phase += deltaPhase;
if (phase >= 1.0) phase -= 1.0;   // only wraps upward
```

A negative `deltaPhase` makes `phase` an unbounded negative ramp. Reproduced with
`sampleRate = -44100`, `pitchCV = 0`, `morph = 0.5`, `character = 1.0`, 20 000 steps:

```
freqHz = -21609.00   phase = -9800.00   maxAbs = 1.476e38 V   isfinite = 0
```

The output leaves the ±6 V bound by 38 orders of magnitude and goes **non-finite**. This
is precisely the runaway the header calls out at lines 167-172 ("The guard is
LOAD-BEARING, not cosmetic") and that
`tests/test_vco_core.cpp:540-582` says the magnitude bound exists to catch — and neither
the guard nor the test catches it (see WR-03).

The same lines also make the header's inline claim at 177-181 ("correct ONLY because the
guard above bounds deltaPhase at kVcoNyquistGuardFrac (0.49) < 1.0") false as written:
the guard bounds `freq`, not `deltaPhase`, and it does not bound it below.

**Fix:** clamp the ceiling first, then the floor, so the floor is always last-writer.
This is bit-identical for every finite, positive-rate input, so no golden or measured
figure in the suite can move.

```cpp
// Ceiling FIRST, then the NaN-safe floor, so the floor is the last writer and a
// non-positive sampleRate can never reintroduce a negative frequency.
const float maxFreq = kVcoNyquistGuardFrac * in.sampleRate;
if (freq > maxFreq) freq = maxFreq;   // NaN maxFreq leaves freq alone; floor catches it
if (!(freq > 0.f)) freq = 0.f;        // NaN and every negative land at zero
```

Add the missing coverage alongside it (WR-03), driving `VcoCore::step()` directly rather
than through `VcoBlockDriver`:

```cpp
forge::VcoInputs in; in.pitchCV = 0.f; in.morph = 0.5f; in.character = 1.f;
in.sampleRate = -44100.f; in.sampleTime = 1.f / 44100.f;
for (int i = 0; i < 20000; ++i) {
    const float o = core.step(in);
    REQUIRE(std::isfinite(o));
    CHECK(std::fabs(o) <= 6.0f);
}
CHECK(core.phase >= 0.0);
CHECK(core.phase <  1.0);
```

---

### CR-02: `forge::clamp` is NaN-transparent, so `step()`'s morph/character clamps do not clamp and a non-finite output reaches the patch

**File:** `src/dsp/VcoCore.hpp:187-188` (helper at `src/dsp/RackCompat.hpp:97`)

**Issue:** `forge::clamp` is written as a comparison ladder:

```cpp
inline float clamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
```

For `x = NaN` both comparisons are false, so it returns `NaN`. The two calls at
`VcoCore.hpp:187-188` are the core's only defensive validation of `morph` and
`character`, and they are inert for the single input class that a defensive clamp exists
to stop. Reproduced:

```
forge::clamp(NaN, 0.f, 1.f) -> nan
morph = NaN, character = NaN  ->  step() output allFinite = 0
```

This is a silent behavioural divergence from the SDK primitive the shim is standing in
for. `rack::math::clamp` (`../Rack-SDK/include/math.hpp:106`) is
`std::fmax(std::fmin(x, b), a)`, and `fmin`/`fmax` discard a NaN operand — so Rack's
clamp always returns a finite value while `forge::clamp` does not.
`src/dsp/RackCompat.hpp:96` admits this in passing ("Bit-identical … **for finite
inputs**"), but `VcoCore` then relies on it in exactly the non-finite case.

Live reachability through the Phase-30 shell is nil, because Rack sanitises NaN in
`ParamQuantity::setValue` before `params[].getValue()` ever sees it. It stops being nil
the moment MORPH/CHARACTER **CV inputs** land in Phase 31/34: any upstream module may
emit NaN on a cable, Rack does not sanitise cable voltages, and a non-finite sample
handed to `outputs[OUTPUT].setVoltage()` propagates through the user's whole patch.
The clamps at 187-188 are pre-installed for precisely that future, and as written they
will not fire.

**Fix:** make the clamp NaN-safe **locally in `VcoCore`**. Do **not** edit
`forge::clamp` in `src/dsp/RackCompat.hpp` — that header is pinned by
`tests/check_frozen.sh` and is consumed by the shipped LFO at
`src/dsp/LfoCore.hpp:168,212-213,216`; changing it is an LFO-guardrail event, not a VCO
fix. A namespace-scope helper in `VcoCore.hpp` is bit-identical to `forge::clamp` for
every finite input and C++11-clean:

```cpp
// NaN-safe clamp, local to the VCO. Identical to forge::clamp for finite inputs;
// unlike forge::clamp (a comparison ladder) a NaN lands on `lo` rather than passing
// through. forge::clamp itself is frozen and shared with the shipped LFO — do not
// change it here.
inline float vcoClampFinite(float x, float lo, float hi) {
    return x >= lo ? (x <= hi ? x : hi) : lo;   // NaN fails `x >= lo` -> lo
}
```

then at `VcoCore.hpp:187-188`:

```cpp
const float morph     = vcoClampFinite(in.morph, 0.f, 1.f);
const float character = vcoClampFinite(in.character, 0.f, 1.f);
```

Pin it with a case that fails today:

```cpp
TEST_CASE("vco core: a non-finite morph/character cannot produce a non-finite sample") {
    forge::VcoBlockDriver d(48000.0);
    forge::VcoInputs base = coreBase();
    base.morph = std::numeric_limits<float>::quiet_NaN();
    base.character = std::numeric_limits<float>::quiet_NaN();
    std::vector<float> out = d.run(256, [=](int) { return base; });
    for (size_t i = 0; i < out.size(); ++i) REQUIRE(std::isfinite(out[i]));
}
```

## Warnings

### WR-01: `sampleTime` and `sampleRate` are independent POD fields, but the single-subtract wrap silently requires `sampleTime == 1/sampleRate`

**File:** `src/dsp/VcoCore.hpp:165, 177-184`

**Issue:** The Nyquist ceiling is computed from `in.sampleRate` while the phase
increment is computed from `in.sampleTime`. Nothing in `VcoCore` couples, asserts or
even documents the invariant that ties them together, yet the correctness of the
one-subtract wrap depends entirely on it. Reproduced with `sampleRate = 44100`,
`sampleTime = 1/1000`, `pitchCV = +6`, 20 000 steps:

```
freqHz = 16744   deltaPhase = 16.7   phase = 314880.80   (unbounded ramp)
```

Every driver in the repo happens to set both fields together
(`tests/VcoBlockDriver.hpp:59-60`, `tests/test_vco_core.cpp:214-216, 226-228, 241-247`,
`src/AnalogVCO.cpp:107-108`), so no existing test can observe this. The obvious future
trigger is Phase 32's anti-aliasing: an oversampled inner loop naturally advances
`sampleTime` at the oversampled rate while the Nyquist policy still reasons about the
host `sampleRate`, which is exactly the mismatch above.

A secondary consequence once `|phase|` is large: `(float)phase` at `VcoCore.hpp:186`
loses all fractional resolution. At `phase = 3e7` consecutive `step()` calls with
identical inputs return `2.98664, -4.01467, -4.01467` — the waveform quantises and then
freezes.

**Fix:** bound the quantity the wrap actually depends on, so the wrap is provably
correct regardless of what a caller puts in the two fields. Keep the `freq` guard for
telemetry accuracy.

```cpp
double deltaPhase = (double)freq * (double)in.sampleTime;
// The single-subtract wrap below is only correct for deltaPhase in [0, 1). Bound
// deltaPhase ITSELF rather than relying on the caller having set sampleTime ==
// 1/sampleRate — nothing in this POD enforces that coupling.
if (!(deltaPhase > 0.0)) deltaPhase = 0.0;                            // NaN and negatives -> 0
if (deltaPhase > (double)kVcoNyquistGuardFrac) deltaPhase = (double)kVcoNyquistGuardFrac;
phase += deltaPhase;
if (phase >= 1.0) phase -= 1.0;
```

Then state the invariant on the POD itself at `VcoCore.hpp:100-101`, so Phase 31/32
authors read it before they wire an oversampled path.

---

### WR-02: every `AnalogVCO` instance is seeded with the same four literals, so all instances in a patch are bit-identical clones

**File:** `src/AnalogVCO.cpp:96-97`

**Issue:** The constructor hardcodes the spread seed:

```cpp
core.seed(0x1234ULL, 0x5678ULL);
core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);
```

Because `setSpreadSeed` is the *whole* of this phase's per-instance divergence
mechanism (D-11), a constant seed means two Analog VCOs in the same patch are exact
clones. Measured with the literals above, `morph = 0.25`, `character = 1.0`, 2048
samples: **0 / 2048 samples differ**.

That contradicts the comment sitting directly above those two lines
(`src/AnalogVCO.cpp:83-85`), which asserts the second call produces "per-instance analog
variation … which is precisely the divergence D-11 exists to produce." It produces
divergence from a *default-constructed* core, not between instances — and between
instances is the property that matters to a user stacking two oscillators, and the
property `tests/test_vco_core.cpp` invariants 3 and 4 spend 200 lines proving. The suite
proves divergence for *differently-seeded* cores, which the shell never creates, so the
test evidence does not describe the shipped module.

It also diverges from the established convention in this very plugin: the shipped LFO
seeds per instance from `std::random_device` and persists the drawn spread seed in the
patch (`src/AnalogLFO.cpp:224-232, 238, 255-270`).

**Fix:** if the Phase 34/35 deferral stands, correct the comment so it does not claim a
property the code does not have — replace "per-instance analog variation" with an
explicit statement that all instances are currently identical and why. If the deferral
does not stand, mirror the LFO exactly (`std::random_device` → `Xoroshiro128Plus` →
`core.seed` / `spreadSeed[0..1]`), reusing the LFO's `{0,0}` rejection and its
non-throwing `forge::parseSeedHex` restore path, both of which already exist and are
already validated.

---

### WR-03: no test in either suite ever drives the core with a hostile `sampleRate` or `sampleTime` — the coverage that would have caught CR-01 and WR-01

**File:** `tests/test_vco_harness.cpp:103-115`; `tests/test_vco_core.cpp:540-582`;
`tests/VcoBlockDriver.hpp:59-60`

**Issue:** `tests/test_vco_harness.cpp:109` injects `in.sampleRate = -1.f` and
`tests/test_vco_harness.cpp:91` injects `in.sampleTime = 999.f`. Both are immediately
and unconditionally overwritten by `VcoBlockDriver::run` (`VcoBlockDriver.hpp:59-60`) —
which is the point of those two cases, and they are correct as *injection* tests. But
the effect on the suite as a whole is that hostile timing is the one input class
`VcoCore::step()` is never exposed to, while the file reads as though it has been.
`runInterleaveCheck` (`test_vco_core.cpp:214-216, 226-228, 241-247`) performs the same
unconditional overwrite, so it closes the same door.

`tests/test_vco_core.cpp:540-582` ("hostile V/OCT") then states that the magnitude bound
"is the only invariant in the suite that can" see a runaway accumulator — but it only
ever varies `pitchCV`, so it can only see a runaway caused by pitch. CR-01's runaway is
caused by the sample rate, produces `1.48e38 V` and a non-finite sample, and passes the
entire suite untouched.

**Fix:** add a fourth scenario to the D-18b bound case that constructs `VcoInputs`
directly and calls `core.step()` without a driver, sweeping the hostile timing grid
`sampleRate ∈ {-44100, 0, NaN}` × `sampleTime ∈ {-1/44100, 0, 999, NaN}` and requiring
`isfinite(out)`, `|out| <= 6.0f` and `0.0 <= core.phase < 1.0` at every step. Written
against the current body this case fails, which is what makes it evidence.

---

### WR-04: `plugin.json` ships a second module under version `2.0.1`, which is already tagged and published in the VCV Library

**File:** `plugin.json:4, 25-33`

**Issue:** `git tag` lists `v2.0.1`, and the phase context records that version as
ACCEPTED and live in the VCV Library with a single module. `plugin.json` now declares
two modules while `"version"` is still `"2.0.1"`. Two artifacts with the same version
string therefore describe different plugins, which is the ambiguity a manifest version
exists to prevent: the library builds from git tags, and a user building from source
gets a plugin that self-identifies as the published 2.0.1 but exposes an extra module.
`.planning/phases/30-.../deferred-items.md` records "`version` still `2.0.1`" as a
*confirmation* that the LFO registration was unaltered — but leaving the LFO entry
untouched and bumping the plugin version are independent, and only the first was checked.

**Fix:** bump `"version"` to the next minor (`2.1.0`) in the same commit that adds a
module, and keep the changelog entry with it. If the bump is deliberately being held for
a release phase, add an explicit line to `deferred-items.md` naming the phase that owns
it — a version that is silently stale is indistinguishable from one that was forgotten.

---

### WR-05: the `[2/7]` exemption filter is unanchored, so a genuine `<rack.hpp>` include evades the Rack-free guard when its line also carries the exempted text

**File:** `tests/check_includes.sh:161-169` (controls at `:561-596`)

**Issue:** The exemption is applied as a line-level `grep -v` with no anchors:

```bash
| grep -vE '#[[:space:]]*include[[:space:]]*"dsp/RackCompat\.hpp"'
```

Any output line containing that text *anywhere* is discarded — including a line whose
actual directive is a real SDK include. Reproduced against the live function body:

```
#include <rack.hpp>   // unlike #include "dsp/RackCompat.hpp" this is the real SDK
```

→ detector output empty, i.e. a VCO header pulling in the Rack SDK is reported clean.

This does not question the exemption itself, which is operator-approved and is validated
in both directions by `[6/7]`. It is that the two `[6/7]` fixtures
(`tests/check_includes.sh:570-575, 584-589`) are each a single-directive line, so
neither covers this shape — and `[6/7]`'s banner at `:561-569` claims the pair
"pin the exemption to exactly the width it is documented to have." Measurably, they do
not.

**Fix:** anchor the exclusion to a whole line so it can only ever match a line whose sole
directive is the shim. The incoming records are `grep -n` output, so the anchor must
allow the line-number prefix:

```bash
| grep -vE '^[0-9]+:[[:space:]]*#[[:space:]]*include[[:space:]]*"dsp/RackCompat\.hpp"[[:space:]]*(//.*)?$' \
```

and add the evasion fixture above as a third `[6/7]` control that must produce a hit.

## Info

### IN-01: `Telemetry::stepCount` is `uint32_t` and wraps after ~2^32 samples

**File:** `src/dsp/VcoCore.hpp:130, 158`
**Issue:** `++tel.stepCount` per sample wraps after roughly 12.4 hours at 96 kHz / 27
hours at 44.1 kHz. Harmless today (`tests/test_vco_harness.cpp:76` only compares it
against a short block length), but it is a monotonic counter with a reachable wrap, and
Phase 35's display work is the natural first consumer.
**Fix:** widen to `uint64_t`, or document the wrap in the field comment so Phase 35 does
not build an elapsed-time display on it.

### IN-02: last-step telemetry is written from the audio thread with no synchronisation note beyond a one-line comment

**File:** `src/dsp/VcoCore.hpp:119-132`
**Issue:** `tel.freqHz` / `tel.displayPhase` are plain `float`s written every sample by
`process()` on the audio thread. The comment says "the shell reads these to feed display
atomics", which is the safe pattern (audio thread reads `tel`, publishes to atomics) —
but it is one clause of prose guarding a UI/audio boundary that Phase 35 will actually
cross. Nothing is racing today: no widget reads the module.
**Fix:** state the rule as a contract on the struct — "read only from `process()` on the
audio thread; never from a widget's `draw`/`step`" — so Phase 35 cannot read it the
other way by accident.

### IN-03: ~40 lines of near-verbatim duplication between invariants 4 and 5

**File:** `tests/test_vco_core.cpp:757-791` vs `tests/test_vco_core.cpp:871-899`
**Issue:** The `seedInstance` lambda and the `inA`/`inB` functors are duplicated between
the real-core case and the positive control, differing only in the block length `n`. The
file argues persuasively that `runInterleaveCheck` must be *shared* between the two
cases; the same argument applies to the input functors, since a control that feeds
different inputs than the case is weaker evidence about the case. The types differ
(`forge::VcoCore` vs `DeliberatelyBrokenSharedStateCore`), so only the seeder needs to
stay per-case.
**Fix:** hoist `inA`/`inB` into a small `makeInterleaveInputs(const VcoInputs& base, int
n)` helper in the anonymous namespace and call it from both cases. Do not merge the
seeders or the two `TEST_CASE`s.

### IN-04: file banner miscounts the helpers it is describing

**File:** `tests/test_vco_core.cpp:90-91`
**Issue:** "plan 30-04 appends its CORE-03 helpers … into this SAME anonymous namespace,
and owns none of the three helpers defined here" — two helpers (`coreBase`,
`estimateFreqRising`) plus one constant (`SAMPLE_RATES`) are defined above that line.
Trivial, but this file's comments are load-bearing documentation that later phases are
told to read literally, so a miscount invites a reader to hunt for a third helper.
**Fix:** "the two helpers and one constant defined here".

---

_Reviewed: 2026-07-29_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
