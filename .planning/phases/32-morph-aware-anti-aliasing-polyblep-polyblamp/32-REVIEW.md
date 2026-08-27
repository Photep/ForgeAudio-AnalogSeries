---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
reviewed: 2026-08-27T10:58:50Z
depth: standard
files_reviewed: 9
files_reviewed_list:
  - src/dsp/MorphBlep.hpp
  - src/dsp/VcoCore.hpp
  - src/AnalogVCO.cpp
  - src/vco_compile_canary.cpp
  - res/AnalogVCO.svg
  - tests/test_morph_blep.cpp
  - tests/test_vco_core.cpp
  - tests/test_vco_spectrum.cpp
  - tests/check_includes.sh
findings:
  critical: 2
  warning: 7
  info: 2
  total: 11
status: issues_found
---

# Phase 32: Code Review Report

**Reviewed:** 2026-08-27T10:58:50Z
**Depth:** standard
**Files Reviewed:** 9
**Status:** issues_found

## Summary

I traced the `MorphBlep` weight algebra, geometry and site table line by line against
the frozen `forge::Waveshape` it mirrors, re-derived the sign convention and the
BLEP/BLAMP residuals from first principles, and drove the header directly with hostile
inputs under AddressSanitizer.

**The kernel algebra is correct.** The nine-site table checks out against the frozen
code at every entry I could verify by hand: the `(1-c)` hard / `c` soft split matches
`sqr + c*(analog - sqr)`; the bleed-ring indices, weights and folded `1/(1+bi)`
normalization match `Waveshape.hpp:188-212` exactly; `triBrk = 2/valley + 2/(1-valley)`
reduces to the correct `±8` at `c = 0` and carries the correct sign at each corner;
`r(x)`/`R(x)` and the `pending` split are self-consistent and continuous across the
sample boundary; the double-sourced distance / float-sourced side split is right, and
the direction that would double-fire is provably impossible (round-to-nearest is
monotone and every `pos[i]` is exactly representable as a float, so `(float)phase < pos`
implies `phase < pos`). The spectral suite's anti-circularity assertion
(`improvementDb >= 8.0`, consulting no pinned number) and the D-08 bit-exact
reconstruction case are genuinely strong instruments, and the "cannot fail" review lens
found no vacuous assertion in `test_morph_blep.cpp` or `test_vco_spectrum.cpp`.

**Where it breaks is the guard perimeter, and the break is asymmetric in a way the
header's own banner denies.** `src/dsp/MorphBlep.hpp:282-304` states in capitals that
"MorphBlep REFUSES TO RELY ON ITS CALLER" and carries a negated, both-sided guard on
`dt`. It carries **no guard whatsoever on `morph` or `character`**, and both of those
reach constructs that are undefined behaviour out of range. I confirmed a
**stack-buffer-underflow under ASan** from a negative `morph`, and a **NaN-poisoned
output stream** from a NaN `character` that slips past `morphBlepCharFactor`'s NaN trap
because three of the nine sites carry a literal `0.f` width. Both are latent behind
`VcoCore`'s conditioning today, but the header is a public API with a stated
caller-independence contract, its tests call it directly, and Phase 33 is documented as
a new caller that will plug straight into it.

Two further findings sit in the "invisible on Apple clang, live on the shipping
toolchain" class this repository was burned by at v2.0.0: the NaN-`morph` index is `0`
on arm64 and `INT_MIN` on x86-64, and `-ffp-contract=off` — declared load-bearing for
this exact header — is present only in `TEST_CXXFLAGS` and absent from the plugin build,
which instead inherits `-O3 -funsafe-math-optimizations` from the Rack SDK.

## Orchestrator verification note — REACHABILITY OF CR-01 AND CR-02

Added by the execute-phase orchestrator after the review returned, because severity
claims in this project have to survive the same measure-first standard as everything
else this phase asserted (T-32-27).

**Both CR-01 and CR-02 were verified as real defects in `MorphBlep.hpp` — and as NOT
reachable through the shipped call path.** Checked, not assumed:

- `blep.step` has exactly ONE call site in the whole of `src/`: `VcoCore.hpp:645`
  (`grep -rn "blep\." src/` returns that line and nothing else executable).
- Immediately above it, `VcoCore.hpp:598-602` conditions both arguments with the
  NaN-safe negated pair: `if (!(morph > 0.f)) morph = 0.f; if (morph > 1.f) morph = 1.f;`
  and the same for `character`. A NaN fails `> 0.f`, so the negation fires and it
  becomes `0.f`; a negative likewise. `morph` and `character` therefore arrive at
  `MorphBlep::step` already in [0, 1].
- `AnalogVCO.cpp:286-288` applies the same conditioning a second time at the shell
  boundary, before `in.morph` is ever populated.

**Consequence for severity.** The reviewer reproduced both by calling `MorphBlep::step`
directly with hostile arguments — something no shipping code path does. These are
defence-in-depth gaps, not live faults: nothing that VCV Rack can drive reaches them
today. Read them as **High-priority hardening**, not as ship blockers, and note that
the phase's green gates and CI legs are NOT invalidated by them.

**Why they still matter and should be fixed.** The header's own banner claims
caller-independence in capitals but defends only `dt` — so the contract it advertises
is not the contract it enforces. `src/dsp/MorphBlep.hpp` is a shared header, and
Phases 33 (hard sync) and 34 (output and drift) are both likely to add call sites.
The first unguarded one turns CR-01 into a live out-of-bounds write, and on the x86
MinGW/Linux builds that actually ship, `(int)NaN` is `INT_MIN` rather than the benign
`0` this arm64 host happens to produce — the same "invisible on Apple clang" class
that got v2.0.0 rejected from the VCV Library.

Recommended disposition: fix in a Phase 32 gap-closure plan or as the first task of
Phase 33, before any second call site exists.

## Critical Issues

### CR-01: `MorphBlep::step` writes out of bounds on a negative `morph`, and the failure is platform-divergent for a NaN `morph`

**File:** `src/dsp/MorphBlep.hpp:318-334`, `src/dsp/MorphBlep.hpp:364`
**Issue:**

```cpp
const float scaled = morph * 4.f;
int segment = (int)scaled;
if (segment > 3) segment = 3;            // clamps ONLY the top
...
	W[segment]     += 1.f - frac;
	W[segment + 1] += frac;
...
	W[(segment - 1 + 5) % 5] += bi * (1.f - frac);
	W[(segment + 2) % 5]     += bi * frac;
```

`segment` is clamped above but never below. `float W[5]` is a function-local array.

* **Negative `morph`.** `morph = -1.f` gives `segment = -4`, so `W[segment]` and
  `W[segment + 1]` are `W[-4]` and `W[-3]`, and `(segment + 2) % 5` is `-2` in C++
  (`%` on a negative left operand yields a non-positive result), so the bleed ring
  writes `W[-2]`. **Reproduced**, calling `MorphBlep::step` directly with
  `morph = -1.f, character = 0.5f`:

  ```
  AddressSanitizer: stack-buffer-underflow on address 0x00016f7d6230
  READ of size 4 at ... in forge::MorphBlep::step MorphBlep.hpp:332
    This frame has 5 object(s):
      [32, 52) 'W' (line 325) <== Memory access at offset 16 underflows this variable
  ```

* **NaN `morph`.** `(int)scaled` on a NaN is undefined behaviour ([conv.fpint]/1). It
  is not merely theoretical divergence: on Apple arm64 `fcvtzs` saturates NaN to `0`
  (measured on this host: `(int)(NaN*4.f) = 0`, which lands harmlessly on segment 0),
  while on x86-64 `cvttss2si` returns the integer indefinite value `INT_MIN`. On the
  MinGW/Linux x86 builds that ship through the VCV Library, `segment` would be
  `-2147483648` and `W[segment]` a wild write — an immediate crash in the audio thread.
  This is exactly the "clean locally, detonates on the library toolchain" class recorded
  in this file's own C++11 contract block.

This directly contradicts `src/dsp/MorphBlep.hpp:282-291` ("MorphBlep REFUSES TO RELY
ON ITS CALLER … any caller — the headless harness, a future polyphonic shell, Phase 33
— cannot reach the divisor below with hostile timing"). The `dt` divisor is defended;
the two arguments that reach an integer cast and an array index are not. `VcoCore` and
`AnalogVCO` both condition `morph` today, so this is latent through the shipped call
chain — but the contract the header states is a contract about the *header*, Phase 33 is
named as a future direct caller, and `tests/test_morph_blep.cpp` already calls
`step()`/`addStep()` with no core in the way.

**Fix:** apply the same negated pair the header already mandates elsewhere, at the top
of `step()`, before `scaled` is formed. Keep the negation first so NaN lands on the
fallback branch; do not use `forge::clamp` (rejected by name here and at
`VcoCore.hpp:386-390`), and do not use `std::clamp` (C++17).

```cpp
// T-32-01's pair, applied HERE as well: the frozen (int)(morph * 4.f) cast and the
// W[] index below are both undefined for a NaN or an out-of-range morph, and this
// header does not rely on its caller (D-15 / P-14). Negation first is the NaN catcher.
if (!(morph > 0.f)) morph = 0.f;
if (morph > 1.f) morph = 1.f;
if (!(character > 0.f)) character = 0.f;
if (character > 1.f) character = 1.f;
```

Belt-and-braces, also floor the segment so a future arithmetic change cannot reopen it:

```cpp
int segment = (int)scaled;
if (segment < 0) segment = 0;
if (segment > 3) segment = 3;
```

Add hostile-`morph` / hostile-`character` rows to `tests/test_morph_blep.cpp` case 5,
alongside the existing hostile-`dt` rows — that subcase currently proves the guard
discipline for one of the three arguments that needs it. An ASan leg over `make test`
would have caught this; consider one (scoped to the test binary, not repo-wide — D-24
keeps the shipped LFO out of a sanitizer gate).

---

### CR-02: A NaN `character` produces NaN corrections, bypassing `morphBlepCharFactor`'s NaN trap at the three literal-zero-width sites

**File:** `src/dsp/MorphBlep.hpp:317`, `:396-397`, `:454-464`, `:513-514`
**Issue:** The header's NaN defence for the site loop is `morphBlepCharFactor`, which is
written negated so `k` returns `0.f` for a NaN width and the site is skipped
(`:513-514`). That defence has a hole: three of the nine widths are the **literal**
`0.f` (`wid[0]`, `wid[3]`, `wid[5]`), so `morphBlepCharFactor(0.f, fdt)` returns exactly
`1.f` no matter what `character` was — and the *magnitudes* at those sites are NaN:

```cpp
const float c = (character < 0.001f) ? 0.f : character * character;   // NaN < 0.001 is FALSE -> c = NaN
...
const float hardSq = W[3] * 2.f * (1.f - c);   // 0.f * 2.f * NaN == NaN, even at zero weight
const float hardPl = W[4] * 2.f * (1.f - c);
...
mag[0] = W[2] * 2.f + hardSq + hardPl;         // NaN
```

`if (mag[i] == 0.f) continue;` is false for a NaN, `k == 1.f`, so `h = NaN` flows into
both `now` and `pending`. **Reproduced** by calling `step()` directly at
`morph = 0.5f, character = NaN, dt = 0.02` over 200 samples: **16 of 200 returned
corrections were non-finite** (one per wrap crossing). In the real chain
`naive + correction` makes the whole sample NaN.

Note the second-order defect this exposes: `0.f * NaN` is `NaN`, so the "magnitudes fall
to zero when a shape carries no weight" skip at `:473` — the thing that is supposed to
make a fixed nine-site union cost nothing where a shape is absent — is silently defeated
for every site whose magnitude touches `c`. Also note that the bleed block at `:359` is
gated on `character >= 0.001f`, which is *false* for a NaN, so `c` is NaN while the
weight vector was built as if character were inert: the two branches disagree about what
a NaN character means.

**Fix:** the `character` half of CR-01's guard closes this at the source and is the
recommended fix. If a defence in depth is also wanted at the site loop, make the skip
NaN-aware rather than equality-based:

```cpp
if (!(mag[i] != 0.f)) continue;   // negated: skips exact zero AND a not-a-number
```

Then add a `character = NaN` row to `tests/test_morph_blep.cpp` case 5 subcase C and
assert `std::isfinite` on the returned correction, exactly as the hostile-`dt` rows do.

## Warnings

### WR-01: The frozen `fmax(edgeWidth, 0.001f)` sharpness floor is the one frozen clamp the width derivation does not mirror

**File:** `src/dsp/MorphBlep.hpp:389`, `:392`
**Issue:** `src/dsp/MorphBlep.hpp:308-316` states the mirroring doctrine in capitals —
"Every expression here mirrors `forge::Waveshape::morphedWave`'s own, INCLUDING its
early-return threshold … the `< 0.001f` threshold below and the `>= 0.001f` gate on the
bleed block must be the FROZEN CODE'S EXACT COMPARISONS". The header duly mirrors
`fmax(0.f, 0.04f + bleedSpread)` (`:360-361`) and `fmax(0.f, bleedIntensity)`
(`:363`). It does **not** mirror the third frozen clamp in the path it derives widths
from:

```
Waveshape.hpp:111   float sharpness = 1.f / std::fmax(edgeWidth, 0.001f);   // square
Waveshape.hpp:136   float sharpness = 1.f / std::fmax(edgeWidth, 0.001f);   // pulse
```

so the tanh edge the frozen path actually produces never gets narrower than a half-width
of `0.001` (equivalent ramp width `0.002`), while

```cpp
const float wSq = 2.f * (c * 0.08f);
const float wPl = 2.f * (c * capPl / (1.f + wv.pulseEdgeSpread));
```

go to zero with `c`. Below `character ≈ 0.112` (square) / `≈ 0.158` (pulse) the width fed
to the D-03 factor is smaller than the edge that exists, so the soft-edge site keeps more
authority than the frozen geometry warrants. **Measured** at C4 (`dt = 0.00593`):

```
char=0.02  w_used=0.000064  w_frozen=0.002000  k_used=0.98924  k_true=0.69129   +43.1%
char=0.05  w_used=0.000400  w_frozen=0.002000  k_used=0.93371  k_true=0.69129   +35.1%
char=0.08  w_used=0.001024  w_frozen=0.002000  k_used=0.83484  k_true=0.69129   +20.8%
char=0.11  w_used=0.001936  w_frozen=0.002000  k_used=0.70029  k_true=0.69129    +1.3%
```

The absolute error is small — the soft-edge magnitude is `W*2*c ≈ 0.0008` in that band —
so this is a fidelity/doctrine defect rather than an audible one, and the spectral grid's
`character` column (0.00 / 0.50 / 1.00) steps straight over the affected band, so nothing
in the suite can see it. That is precisely the shape of the "narrow character band the
spectral grid sweeps straight through" the header's own P-12 paragraph warns about.

**Fix:**

```cpp
// Waveshape.hpp:111 / :136 floor the SHARPNESS, so the realised tanh half-width never
// goes below 0.001. Mirror that floor rather than the unfloored product (P-12).
const float ewSq = (c * 0.08f > 0.001f) ? (c * 0.08f) : 0.001f;
const float wSq  = 2.f * ewSq;
const float ewPl = (c * capPl > 0.001f) ? (c * capPl) : 0.001f;
const float wPl  = 2.f * (ewPl / (1.f + wv.pulseEdgeSpread));
```

Note this changes measured output in that character band, so it must go through the
MEASURE-TO-PIN protocol rather than be landed silently.

---

### WR-02: `-ffp-contract=off` is declared load-bearing for `MorphBlep.hpp` but is absent from the only build that ships

**File:** `src/dsp/MorphBlep.hpp:79-83`, `Makefile:8`, `Makefile:40`
**Issue:** The header says:

> `-ffp-contract=off` IS LOAD-BEARING FOR THIS FILE SPECIFICALLY … The residuals below
> are chains of `a*b+c`, so contraction into fused multiply-adds changes the result bit
> for bit. Do not add `-ffast-math`, do not drop the flag …

The flag appears in exactly one place: `TEST_CXXFLAGS` (`Makefile:40`). The plugin build
sets `CXXFLAGS +=` (empty, `Makefile:8`) and inherits `$(RACK_DIR)/plugin.mk`, whose
`compile.mk:17` reads:

```
FLAGS += -O3 -funsafe-math-optimizations -fno-omit-frame-pointer
```

So the shipped `plugin.dylib` / `.so` / `.dll` compiles `MorphBlep.hpp` with GCC's
default `-ffp-contract=fast` **and** `-funsafe-math-optimizations` (which enables
`-fassociative-math`, `-freciprocal-math`, `-fno-signed-zeros`). Two consequences:

1. The bit-exact reconstruction assertion at `tests/test_vco_spectrum.cpp:1547`
   (`CHECK(reconstructionMismatches == 0)`) — described in-file as the case that proves
   "nothing else in `step()` moved" — is evidence about a binary that is never
   distributed. So is every `!=`-based divergence count in that file.
2. The header's instruction "do not add `-ffast-math`" reads as a warning against a
   future change, when in fact half of `-ffast-math` is already applied to the shipped
   artifact. A future editor following the banner will believe the flag is enforced.

The NaN guards themselves survive — `-funsafe-math-optimizations` does *not* imply
`-ffinite-math-only`, so `!(x > 0.f)` still catches a NaN — which is the one thing that
keeps this out of the Critical tier.

**Fix:** either enforce the flag on the plugin build

```make
# MorphBlep.hpp's residuals are a*b+c chains; contraction changes them bit for bit,
# and the Rack SDK adds -funsafe-math-optimizations. Pin both for OUR sources.
CXXFLAGS += -ffp-contract=off -fno-unsafe-math-optimizations
```

or correct the header's banner to say the flag is a *test-target* property and that the
shipped binary may differ in the last ULP, and drop the "load-bearing for this file
specifically" claim. Silently keeping the claim while shipping without the flag is the
worse of the three options. Note the shipped LFO's goldens have the same exposure, so
whichever way this goes it should be an operator-visible decision, not an inline fix.

---

### WR-03: The site-crossing test can MISS an edge, so "each site fires EXACTLY ONCE PER CYCLE" is not exact

**File:** `src/dsp/MorphBlep.hpp:475-511`
**Issue:** The banner asserts the tiling is exact ("this distance decreases by exactly
`dt` and each site fires EXACTLY ONCE PER CYCLE", "the double-sourced distance already
tiles exactly"). It is not, because the *side* comes from the float and the *distance*
from the double, and those two can disagree by up to half a float ULP.

Concretely, for a site at `P` and a sample where `phase < P` (double) but
`(float)phase == P` (rounded up):

* this sample: `!(P > p)` is true, so `d = P - phase + 1 ≈ 1`, `s ≈ 1/dt` → **skipped** —
  while the naive path, which uses the same `p`, has **already flipped**;
* previous sample: `d = P - phase + dt = dt + δ` with `δ ≤ ulp(P)/2`, so
  `s = 1 + δ/dt`, which exceeds `1.0f` whenever `δ/dt > 2^-24` → also **skipped**.

The edge is band-limited by nothing that cycle. The opposite direction (double-fire) is
provably impossible, because rounding to nearest is monotone and every `pos[i]` is
exactly representable as a float, so `(float)phase < P` implies `phase < P` — worth
recording, since the banner's rejected-shortcut (3) reasons about both directions as if
they were symmetric.

Rate, for triage: only the five sites at non-zero positions are exposed (`pos == 0.f`
takes the `+1` branch unconditionally). The window is `ulp(P)/2 ≈ 2.98e-8` out of `dt`
per cycle, and cycles per second is `dt * fs`, so the miss rate is
`2.98e-8 * fs ≈ 1.3e-3` per site per second at 44.1 kHz — **independent of pitch** —
roughly one missed edge every 2.5 minutes across the five exposed sites. Each one is a
single uncorrected edge, not a spike. Low audible impact; the finding is that the
header's stated exactness claim is wrong and nothing tests the window.

**Fix:** no code change recommended (the header is right that widening the fire gate
trades a miss for a double-fire, and this is the safer failure direction). Correct the
banner: state that the mixed float/double test is exact in the double-fire direction and
can miss at most once per cycle within half a float ULP of a site, give the measured
rate, and record why that is the accepted trade. If a test is wanted, `case 6` in
`tests/test_morph_blep.cpp` already walks resonant increments and could count
fires-per-cycle per site rather than only bounding the envelope.

---

### WR-04: `MorphBlep::reset()` has zero call sites and zero test coverage, and no reset path reaches it

**File:** `src/dsp/MorphBlep.hpp:236`, `src/dsp/VcoCore.hpp:246-278`, `src/AnalogVCO.cpp:92-226`
**Issue:** `grep` across `src/` and `tests/` finds exactly one occurrence of
`MorphBlep::reset` — its own definition. `forge::VcoCore` exposes no reset entry point,
and `AnalogVCO` overrides neither `onReset` nor `dataToJson`/`dataFromJson`, so
`blep.pending` (and `phase`) survive a Rack "Initialize" and a patch reload. That is a
single stale residual sample, not a real audio defect — but the structural consequence
is worse than the behavioural one: **no test asserts that `reset()` clears state**, so if
Phase 33 adds a third accumulator field for sync and forgets it in `reset()`, nothing in
the suite goes red. The header treats `pending`/`inject` as load-bearing carried state
everywhere else; the one function that is supposed to clear it is unverified.

**Fix:** add a two-line assertion to `tests/test_morph_blep.cpp` case 3, where a primed
instance already exists:

```cpp
// reset() clears EVERY accumulator member. This is the assertion that goes red when a
// future phase adds carried state and forgets it here.
forge::MorphBlep rb;
rb.addStep(0.25f, 2.f);
REQUIRE(rb.inject != 0.0f);          // non-vacuity: something was primed
rb.reset();
CHECK(rb.pending == 0.0f);
CHECK(rb.inject  == 0.0f);
```

Separately, decide whether `VcoCore` should gain a `reset()` that zeroes `phase` and
calls `blep.reset()`, wired to `AnalogVCO::onReset`. If the answer is no, say so in
`VcoCore.hpp` beside the `blep` member so the omission reads as a decision.

---

### WR-05: `addStep` guards `xAhead` but not `jump`, so a non-finite jump reaches per-instance state

**File:** `src/dsp/MorphBlep.hpp:257-262`, `tests/test_morph_blep.cpp:1036-1055`
**Issue:**

```cpp
void addStep(float xAhead, float jump) {
	if (!(xAhead >= 0.f) || xAhead > 1.f) return;
	const float u = 1.f - xAhead;
	inject  += jump * ( 0.5f) * u * u;
	pending += jump * (-0.5f) * xAhead * xAhead;
}
```

The banner at `:253-256` justifies the gate as: "a not-a-number `xAhead` is REJECTED
rather than accumulated … before touching per-instance state that would then poison
every following sample". The identical argument applies verbatim to `jump`, which is
unguarded: `jump = NaN` (or `±inf`) writes NaN straight into both members. `step()`
drains and re-zeros them, so the poisoning is bounded to one sample rather than
permanent — but the gate is half a gate, and the seam's whole purpose is that Phase 33
supplies `jump` from a caller this header does not control.

The test only exercises the guarded argument: `tests/test_morph_blep.cpp:1043-1044`
feeds `{-0.1f, 1.1f, NaN}` as `xAhead` with `jump` fixed at `2.f`. There is no row with
a hostile `jump`.

**Fix:**

```cpp
void addStep(float xAhead, float jump) {
	// Negated on BOTH arguments: a not-a-number or infinite jump reaches the same
	// per-instance state the xAhead gate exists to protect (D-14 / D-15).
	if (!(xAhead >= 0.f) || xAhead > 1.f) return;
	if (!(jump > -1e30f) || !(jump < 1e30f)) return;   // rejects NaN and +/-inf
	...
}
```

and add `{0.5f, NaN}`, `{0.5f, +inf}`, `{0.5f, -inf}` rows to case 5 subcase B.

---

### WR-06: The saw slope-break dismissal carries an extra factor of `dt`

**File:** `src/dsp/MorphBlep.hpp:431-437`, `tests/test_morph_blep.cpp:587-589`
**Issue:** Entry 1's P-4 paragraph justifies omitting the saw's soft-reset slope break:

> Its slope-break magnitude is about `4*dt*dt/6` against the value step's roughly 1 —
> about three and a half orders of magnitude smaller at `dt = 0.02`. IGNORE IT.

The BLAMP contribution the header itself implements is `mag * fdt * R(x)` with
`R ≤ 1/6` (`:537-539`), i.e. `slopeChangePerUnitPhase * dt / 6` — **linear** in `dt`, not
quadratic. Re-deriving the slope change at `phase 0` from `Waveshape.hpp:88-97`: after
the wrap the blended reset has derivative `smoothT'(0)*(curvedSaw(0)-1) + smoothT(0)*…`,
and both terms vanish, so the slope is exactly `0`; before the wrap
`curvedSaw'(1⁻) = -2 + c*(0.5+spread)*(2 - 6e⁻³/(1-e⁻³)) ≈ -1.157` at `c = 1`. Slope
change `≈ +1.157` per unit phase, giving `1.157 * 0.02 / 6 ≈ 0.0039` against a value step
of `2` — about **2.7 orders of magnitude**, not 3.5. `tests/test_morph_blep.cpp:588-589`
repeats the same "three and a half orders of magnitude" claim.

The conclusion (ignore it) is probably still right at `-54 dB` relative to the wrap, but
this repository's own house rule is that a stated premise which does not survive
re-derivation gets corrected in place rather than inherited. A later phase sizing a
refinement off `4*dt*dt/6` would be off by `1/dt`.

**Fix:** correct both the header paragraph and the test comment to the linear-in-`dt`
form, state the re-derived slope change (`≈ 1.16` per unit phase at character 1, `0` at
character 0 where `resetWidth <= 0.001` disables the reset entirely), and record the
resulting `≈ -54 dB` ratio rather than an order-of-magnitude count. If the omission is
worth keeping, it is worth keeping for the right number.

---

### WR-07: `check_includes.sh` [1/7]'s missing-file detector cannot fire

**File:** `tests/check_includes.sh:96-115`, `:232-253`, `:391-401`
**Issue:** The banner explains at length why the "files opened" count is derived from a
log rather than from an array length ("With one LFO header moved aside the gate still
printed 'OK: 25 LFO-side files scanned'"), and `SCAN_MISSING` exists to report any path
handed to the detector that does not exist. But the scan set is no longer the raw file
list — it is `LFO_CLOSURE`, produced by `expand_include_closure`, which already filters:

```bash
while [[ "${idx}" -lt "${#work[@]}" ]]; do
	cur="${work[${idx}]}"
	...
	[[ -f "${cur}" ]] || continue      # non-existent paths are dropped here
	...
	printf '%s\n' "${cur}"             # only existing paths are ever echoed
```

Every element of `LFO_CLOSURE` therefore passed `[[ -f ]]` by construction, so
`detect_vco_includes` can never append to `SCAN_MISSING`, the `if [[ -s "${SCAN_MISSING}" ]]`
branch at `:398` is unreachable, and `scanned` is always exactly `${#LFO_CLOSURE[@]}`.
This is the same "a check that has only ever been observed green is unvalidated" failure
mode the file's own [6/7] section exists to prevent — applied to one of its own branches.

**Fix:** move the existence report to the point where the scope is actually decided —
inside `expand_include_closure`, where a root or a resolved include is dropped:

```bash
[[ -f "${cur}" ]] || { printf '%s\n' "${cur#${ROOT}/}" >> "${SCAN_MISSING}"; continue; }
```

and reset `SCAN_MISSING` before the closure call rather than after it. Alternatively add a
[6/7]-style negative control: hand `expand_include_closure` a root that does not exist and
require the missing-file branch to fire.

## Info

### IN-01: `AnalogVCO.cpp`'s closing comment tells a future editor not to fix something already fixed

**File:** `src/AnalogVCO.cpp:417-420`
**Issue:** The comment reads "Nothing registers this symbol with the plugin yet —
`src/plugin.hpp` and `src/plugin.cpp` are plan 30-06's. After this file lands the symbol
exists and the plugin links, and the module still does not appear in Rack's browser. That
is the intended intermediate state; **do not 'fix' it here**." That state ended:
`src/plugin.cpp:8` calls `p->addModel(modelAnalogVCO)`, `src/plugin.hpp:8` declares it,
and `plugin.json:26` carries the `ForgeAnalogVCO` slug. The instruction now actively
misleads.

**Fix:** replace with a note that the model is registered in `src/plugin.cpp` and that
the slug is a one-way door pinned in `plugin.json`.

---

### IN-02: The TEST-03 gate loses the identity of a failing cell

**File:** `tests/test_vco_spectrum.cpp:2270`, `:2323`
**Issue:** Per-cell threshold misses are tallied (`if (correctedDb > (double)threshold) ++failing;`)
and only the aggregate is asserted (`CHECK(failing == 0)`) after the loop. doctest's
`CAPTURE` context is scoped to the loop body, so when the aggregate fires the operator
gets a bare `failing == 0` with no `sr`/`K`/`morph`/`character`/`correctedDb` — for a
90-cell grid whose escalation path (`(1)` reach refinement, `(2)` surface to operator)
depends on knowing which regime missed. Contrast the sibling case at `:2458`, which
asserts per cell and keeps the context.

**Fix:** keep the aggregate and add an `INFO`-bearing per-cell record, e.g. capture the
worst offending cell's coordinates into locals inside the loop and `CAPTURE` them after
it, the same way the no-regression case tracks `worstRegressionCell`.

---

_Reviewed: 2026-08-27T10:58:50Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
