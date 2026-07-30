# Phase 31: Pitch, Tuning & Exponential FM - Pattern Map

**Mapped:** 2026-07-30
**Files analyzed:** 5 (2 modify `src/`, 1 modify `res/`, 1 create `tests/`, 1 modify `tests/`)
**Analogs found:** 5 / 5 (all exact or role-match; every pattern this phase needs already exists in-tree)

> ⚠ **Correction to the spawning brief.** `tests/test_vco_core.cpp` does **not** use doctest's
> BDD `SCENARIO`/`GIVEN`/`WHEN`/`THEN` macros. It uses plain `TEST_CASE(...)` + `SUBCASE(...)`
> with `CAPTURE`/`INFO`, and the word "scenario" appears only in prose and comments
> ("scenario four"). Verified by grep: zero `SCENARIO(`/`GIVEN(`/`WHEN(` invocations across the
> file; the six `TEST_CASE` sites are at lines 383, 472, 792, 938, 1055. The new
> `tests/test_vco_pitch.cpp` must use `TEST_CASE`/`SUBCASE`, not BDD macros.

---

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/dsp/VcoCore.hpp` (MODIFY) | DSP core / model (header-only, Rack-free) | per-sample transform (float in → float out) | **itself, post-Phase-30** (lines 78-95 constants, 166-233 guard sequence) + `src/dsp/LfoCore.hpp:150-190` for the `fmConnected` gate only | exact (self-extension) |
| `src/AnalogVCO.cpp` (MODIFY) | Rack shell / controller (index owner, zero arithmetic) | request-response (Rack `process()` callback per sample) | **`src/AnalogLFO.cpp:197-234`** for `configParam`/`configInput` shapes; **`src/AnalogVCO.cpp:54-165`** itself for enum/POD-forward/widget layout | exact |
| `res/AnalogVCO.svg` (MODIFY) | config / static asset | none (declarative geometry) | **`res/AnalogVCO.svg`** itself (6 rects at final 18HP geometry) | exact |
| `tests/test_vco_pitch.cpp` (CREATE) | test (headless doctest TU) | batch (drive N-sample block, measure aggregate) | **`tests/test_vco_core.cpp`** (banner, `TEST_CASE` idiom, `estimateFreqRising` at 135-149, `REQUIRE(nUp >= 8)` at 428, `DeliberatelyBrokenSharedStateCore` at 316-366) + **`tests/VcoBlockDriver.hpp`** | exact |
| `tests/check_includes.sh` (MODIFY) | guard script / config | batch (scan-set derivation) | **`tests/check_includes.sh:279-285`** itself — the existing 5-entry `VCO_SIDE_ALLOW` array | exact |

**Toolchain split, per file (binding — see Shared Patterns §C++11 and §libm):**

| File | Standard it must satisfy | libm (`std::exp2`/`std::pow`) |
|------|--------------------------|-------------------------------|
| `src/dsp/VcoCore.hpp` | **BOTH** `-std=c++11 -pedantic-errors` (`make strict`, CI MinGW) **and** `-std=c++17` | **BANNED** (bit-identity landmine) |
| `src/AnalogVCO.cpp` | **BOTH** (joins `src/*.cpp` globs automatically) | **BANNED** |
| `tests/test_vco_pitch.cpp` | `-std=c++17 -O2 -Isrc -Itests -Wall -Wextra -ffp-contract=off` only | **REQUIRED** — it is the independent ground truth (D-18) |
| `res/AnalogVCO.svg` | n/a | n/a |
| `tests/check_includes.sh` | bash | n/a |

---

## Pattern Assignments

### `src/dsp/VcoCore.hpp` (DSP core, per-sample transform)

**Analog:** itself, as shipped by Phase 30. Every pattern the new code needs is already in the
file; the phase is a four-line insertion plus one constant value change.
**Guardrail note:** `src/dsp/RackCompat.hpp`, `src/dsp/Waveshape.hpp` and `src/dsp/LfoCore.hpp`
are **excerpt-only / call-only — never edit**. Nothing in this assignment requires editing any of
them. If a plan task ever appears to, that is a guardrail event, not a VCO fix.

**Pattern 1 — namespace-scope plain `constexpr`, with the C++11 rationale inline** (lines 78-95):
```cpp
namespace forge {

// Namespace-scope plain constexpr — the src/dsp/MathConst.hpp idiom this file's
// banner mandates above. NOT `inline constexpr` (C++17), NOT an in-class
// `static constexpr` (declaration-only under C++11 → MinGW undefined reference).
constexpr float kVcoFreqC4 = 261.6256f;         // C4 = 0 V, the standard VCV V/OCT reference (PITCH-01)
constexpr float kVcoNyquistGuardFrac = 0.49f;   // PROVISIONAL — PITCH-04 (Phase 31) owns the real Nyquist policy; ...
...
constexpr double kVcoMaxDeltaPhase = 0.5;
```
The new `kVcoMaxPitchVolts` goes **here**, in this block, in this form. `0.49f` → `0.495f` and
the `PROVISIONAL` clause is replaced with settled rationale (D-11). `kVcoMaxDeltaPhase` is
**not touched** (D-12) — the 8-line comment at 86-95 exists specifically to tell this phase so.

**Pattern 2 — the exact insertion point** (line 175, with its existing comment at 171-174):
```cpp
		// D-14 pitch: exp2 off C4 = 0 V, using the frozen Rack polynomial
		// approximation forge::exp2_taylor5 and NEVER libm std::exp2/std::pow —
		// bit-identity of the FM path (Phase 31) depends on this exact function.
		// Phase 31 sums coarse/fine/FM into the volt domain BEFORE this call.
		float freq = kVcoFreqC4 * exp2_taylor5(in.pitchCV);
```
The comment already names this phase's job. Widen the argument; change nothing after it except
the constant's *value*.

**Pattern 3 — the negated-comparison NaN guard idiom** (lines 232-233 and 259-260 — copy this
shape for the D-14 bound, never `forge::clamp`):
```cpp
		if (freq > maxFreq) freq = maxFreq;
		if (!(freq > 0.f)) freq = 0.f;
		tel.freqHz = freq;
...
		double deltaPhase = (double)freq * (double)in.sampleTime;
		if (!(deltaPhase > 0.0)) deltaPhase = 0.0;
		if (deltaPhase > kVcoMaxDeltaPhase) deltaPhase = kVcoMaxDeltaPhase;
```
Both floors are negated **so NaN lands on the fallback branch**. The header states this in prose
at 203-206 and 255-257. `forge::clamp` (`RackCompat.hpp`, a comparison ladder) is
NaN-**transparent** and is rejected by D-14 by name.

**Pattern 4 — CR-01 ordering is load-bearing and is documented with its own measurement**
(lines 203-231 comment; excerpted for the "don't touch" boundary):
> `if (freq > maxFreq) freq = maxFreq;` **then** `if (!(freq > 0.f)) freq = 0.f;` — floor last,
> always the final writer. Observed with the lines swapped: `tel.freqHz = -21609.00`,
> `phase = -9800.00`, `|out| = 1.476e38 V`. **DO NOT SWAP THESE TWO LINES BACK.**
The `-21609.00` figure at line 214 is a **recorded historical measurement under the old 0.49
constant** — leave it (or annotate); do not search-and-replace it when the constant moves.

**Pattern 5 — the POD boundary is unchanged; the fields already exist** (lines 101-113):
```cpp
struct VcoInputs {
	float pitchCV = 0.f;        // V/OCT input volts (Phase 30/31)
	float coarse = 0.f;         // coarse tune, octaves (Phase 31)
	float fine = 0.f;           // fine tune, semitones (Phase 31)
	float fmVolts = 0.f;        // exponential FM input volts, summed into the pitch volt domain before the single exp2 (Phase 31)
	float fmAtten = 0.f;        // bipolar FM attenuverter (Phase 31)
	bool  fmConnected = false;  // FM jack patched
	...
	float sampleTime = 1.f / 44100.f;  // INJECTED by the harness/shell, never read from a global
	float sampleRate = 44100.f;        // INJECTED; the Nyquist clamp (PITCH-04, Phase 31) needs it
};
```
No field is added. Field units stay as documented (D-05: `coarse` octaves, `fine` semitones).

**Pattern 6 — `Telemetry` shape** (lines 132-143). `tel.freqHz` is the D-19 secondary tier and
already exists. D-22 declined a new pitch-volt telemetry field, so **this struct is unchanged.**

**Source-shape contract (hard guard, not style)** — banner lines 20-29:
`struct VcoCore {` and `float step(const VcoInputs& in) {` must each stay on **one line with the
opening brace**; `tests/check_canary.sh [2b/5]` line-matches both to build a perturbed copy, and
the step matcher is **unanchored** — writing the signature verbatim in a comment on a line that
also contains `{` makes the canary perturb the comment. Abbreviate as `step(...)` in new comments.

**C++11 excerpt-relevance:** the D-14 guard, the `in.fine * (1.f / 12.f)` term and
`constexpr float kVcoMaxPitchVolts = 64.f;` are all C++11-clean. Nothing this file gains may use
`std::clamp`, `if constexpr`, `inline constexpr`, `[[maybe_unused]]`, or an in-class
`static constexpr` table indexed at runtime (that construct got v2.0.0 rejected from the library).

---

### `src/dsp/LfoCore.hpp` — **FROZEN / SHIPPED: excerpt only, never edit**

**Borrow exactly one thing: the `fmConnected` gate (D-09).** Everything else in this block is the
explicit counter-example (lines 181-187):
```cpp
		// --- Step 6: FM via exp2_taylor5 (AnalogLFO.cpp:709-717) ---
		if (in.fmConnected) {
			float depthScale = isClocked ? 0.5f : 0.6f;
			float fmPitch = in.fmCV * in.fmAtten * depthScale;
			freq *= exp2_taylor5(fmPitch);
			freq = std::fmax(freq, 0.001f);
		}
```
| Element | VCO disposition |
|---------|-----------------|
| `if (in.fmConnected) { ... }` | **COPY** (D-09) |
| `freq *= exp2_taylor5(...)` — multiplies frequency *after* pitch resolves | **REJECT** — that is Pitfall 4; the VCO sums volts *before* one `exp2` (D-01/FM-03) |
| `depthScale = 0.5f/0.6f` | **REJECT** — VCO uses 1.0 oct/V at full CW (D-06) |
| `freq = std::fmax(freq, 0.001f)` low floor | **REJECT** — D-13, no low-end floor; the existing negated `if (!(freq > 0.f))` stays |

Also note LfoCore's `std::fmax`/`std::sqrt`/`std::round` usage: libm *is* present in the shipped
LFO core, but `std::exp2`/`std::pow` are not, and the VCO adds neither.

---

### `src/AnalogVCO.cpp` (Rack shell / controller, request-response)

**Analog A — `src/AnalogLFO.cpp:197-216`, the shipped `configParam`/`configInput` shapes:**
```cpp
	AnalogLFO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MORPH_PARAM, 0.f, 1.f, 0.f, "Morph");
		configParam(CHARACTER_PARAM, 0.f, 1.f, 0.f, "Character");
		configParam(DRIFT_PARAM, 0.f, 1.f, 0.f, "Drift");
		configParam<RateParamQuantity>(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz");
		configParam(MORPH_ATTEN_PARAM, 0.f, 1.f, 0.f, "Morph CV", "%", 0.f, 100.f);
		...
		configParam(PHASE_OFFSET_PARAM, 0.f, 1.f, 0.f, "Phase Offset", " deg", 0.f, 100.f);
		configParam(FM_ATTEN_PARAM, 0.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f);
		configInput(FM_INPUT, "FM");
		configOutput(OUTPUT, "LFO");
	}
```
*(The `PHASE_OFFSET_PARAM` line's real multiplier is `360.f`; the `"%"`/`" deg"`/`" Hz"` unit-string
convention — leading space for word units, none for `%` — is the pattern to copy.)*
**What to copy:** the positional-arg style, the linear/default-off/percentage styling, the
`"FM Depth"` name, the `configInput(FM_INPUT, "FM")` shape, the unit-string spacing convention,
and the fact that **no `displayPrecision` is ever set anywhere** (so the VCO sets none either).
**What NOT to copy:** `0.f, 1.f` unipolar range for `FM_ATTEN_PARAM` — the VCO is bipolar
`-1.f, 1.f` (D-07/FM-02). And **`src/AnalogLFO.cpp` must not appear in this phase's diff at all.**

**Anti-pattern in the same shipped file — do NOT mirror** (`src/AnalogLFO.cpp:320`):
```cpp
in.fmCV = in.fmConnected ? inputs[FM_INPUT].getVoltage() : 0.f;   // a conditional IN THE SHELL
```
D-17 makes "the shell computes nothing" load-bearing, and Rack already returns `0.f` for an
unpatched input. Forward both fields unconditionally and let `VcoCore::step` gate.

**Analog B — `src/AnalogVCO.cpp` itself, the three structural blocks to extend:**

Enums (lines 55-70) — ID churn is still free, nothing has shipped:
```cpp
	enum ParamId {
		MORPH_PARAM,
		CHARACTER_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		INPUTS_LEN
	};
```

POD forwarding in `process()` (lines 118-127) — **assignment only, no arithmetic:**
```cpp
	void process(const ProcessArgs& args) override {
		// Default-construct then assign, never a brace value list — see the
		// banner's C++11 note.
		forge::VcoInputs in;
		in.pitchCV = inputs[VOCT_INPUT].getVoltage();
		in.morph = params[MORPH_PARAM].getValue();
		in.character = params[CHARACTER_PARAM].getValue();
		in.sampleTime = args.sampleTime;
		in.sampleRate = args.sampleRate;
		outputs[OUTPUT].setVoltage(core.step(in));
```
Note `forge::VcoInputs in;` then field assignment — **never** `VcoInputs in{a, b}` (NSDMIs make it
a non-aggregate under C++11: hard error).
**Stale comment to update:** lines 129-132 currently say `coarse, fine, fmVolts, fmAtten,
fmConnected` "stay at their header defaults ... wired by the phase that lands the DSP reading
it." This phase *is* that phase — leaving it standing is the "false comment" class plan 30-08
existed to remove. Also re-check the `[2b/5]` count claim at 136-142 ("THREE of the eight
VcoInputs DSP fields") — it becomes eight-of-eight-minus-drift and the sentence's argument about
the canary must be restated, not silently invalidated.

Widget placement (lines 146-165) — SVG and widget coords are written together:
```cpp
		setPanel(createPanel(asset::plugin(pluginInstance, "res/AnalogVCO.svg")));

		// These four coordinates are the four marker rects drawn in
		// res/AnalogVCO.svg. The two files are written together; move one and
		// the panel starts lying about where its controls are.
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.48f, 40.f)),
		         module, AnalogVCO::MORPH_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48f, 100.f)),
		         module, AnalogVCO::VOCT_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(60.96f, 100.f)),
		          module, AnalogVCO::OUTPUT));
```
Stock SDK widgets only (`RoundBlackKnob`, `PJ301MPort`) — D-08/D-16. The Forge Noir knob/jack
structs are **local to `src/AnalogLFO.cpp`** and reusing them would mean extracting components
out of live released source; that is why the LFO stays out of the diff.

---

### `res/AnalogVCO.svg` (config / static asset)

**Analog:** itself. The entire current file (8 lines), which is the whole convention:
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="91.44mm" height="128.5mm" viewBox="0 0 91.44 128.5" version="1.1">
  <rect x="0" y="0" width="91.44" height="128.5" fill="#101014"/>
  <rect x="4" y="6" width="24" height="6" fill="#e85d26"/>
  <rect x="25.48" y="35" width="10" height="10" fill="#2a2a30"/>
  <rect x="55.96" y="35" width="10" height="10" fill="#2a2a30"/>
  <rect x="25.48" y="95" width="10" height="10" fill="#2a2a30"/>
  <rect x="55.96" y="95" width="10" height="10" fill="#2a2a30"/>
</svg>
```
Conventions to copy for the four new controls: `mm` units with a matching unitless `viewBox`
(`91.44 × 128.5` = final 18HP — **do not change**), a `#101014` full-panel background rect, a `#e85d26`
brand bar, and one `10×10` `#2a2a30` marker rect per control at `(cx − 5, cy − 5)` — i.e.
`(30.48, 40)` → `x="25.48" y="35"`. Four new rects, same fill, same size. `setPanel` derives
`box.size` from this file, so width/height are never hardcoded in the widget.
Compare with `res/AnalogLFO.svg` only if real art is wanted — it is not: this panel is replaced
wholesale in Phase 35 and geometry is Claude's discretion.

---

### `tests/test_vco_pitch.cpp` (test, batch) — NEW FILE

**Analog:** `tests/test_vco_core.cpp` (structure, banner discipline, all helpers) +
`tests/VcoBlockDriver.hpp` (the drive path).

**Imports / TU preamble** (`tests/test_vco_core.cpp:75-93`):
```cpp
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "VcoBlockDriver.hpp"

#include <vector>
#include <functional>   // std::function — the interleave helper's seeder/input parameters
#include <cmath>
#include <cstdint>
#include <limits>       // std::numeric_limits<float>::quiet_NaN() — scenario four's hostile timing grid

namespace {

// The three production sample rates every invariant is parametrized over.
constexpr double SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0};
```
`<cmath>` is where `std::exp2` / `std::log2` / `std::fabs` come from — **required here**, banned in
`src/`. Everything test-local lives in an **anonymous namespace** (line 87) — that is what keeps
duplicate helper names across test TUs from becoming an ODR problem.

**Neutral base-input helper** (lines 103-112) — copy this shape, extended for the new fields:
```cpp
// Deliberately NEUTRAL: every case below overrides pitch, morph and character
// explicitly, so no grid point can silently inherit a value it did not state.
forge::VcoInputs coreBase() {
	forge::VcoInputs in;
	in.pitchCV   = 0.f;
	in.coarse    = 0.f;   // Phase 31 — unread by this step() body
	in.fine      = 0.f;   // Phase 31 — unread
	in.morph     = 0.f;
	...
	return in;
}
```
The `// Phase 31 — unread` annotations become live-and-read; state the new fields
(`fmVolts`, `fmAtten`, `fmConnected`) explicitly in the new file's own base helper.

**The estimator — reuse verbatim, do not "simplify"** (lines 135-149):
```cpp
double estimateFreqRising(const std::vector<float>& o, double sr, int* nUp) {
	double first = -1.0, last = -1.0;
	int count = 0;
	for (size_t i = 1; i < o.size(); ++i) {
		if (o[i - 1] < 0.f && o[i] >= 0.f) {
			const double frac = (double)(-o[i - 1]) / ((double)o[i] - (double)o[i - 1]);
			const double t = ((double)(i - 1) + frac) / sr;
			if (count == 0) first = t;
			last = t;
			++count;
		}
	}
	*nUp = count;
	return (count < 2) ? -1.0 : (count - 1) / (last - first);
}
```
Its banner (119-134) carries the load-bearing justification: sub-sample interpolation is
mandatory — the naive `crossings / 2 / duration` form measured **−2.15 %** on a 250 ms window,
~37× the whole 1-cent budget. Because the new file is a separate TU inside an anonymous
namespace, copying it in is correct; do not try to share it out of `test_vco_core.cpp`.

**`TEST_CASE` idiom + non-degeneracy precondition + expectations one octave apart**
(lines 383-428 — the closest analog to the TEST-02 gate, and the case explicitly labelled *not*
the gate):
```cpp
TEST_CASE("vco core: naive pitch tracks the C4 reference on the OUTPUT within 1 percent (NOT the TEST-02 tracking gate)") {
	static const float PITCHES[]    = {-2.f, -1.f, 0.f, 1.f, 2.f};
	...
	for (double sr : SAMPLE_RATES) {
		const int n = (int)std::lround(sr * 0.25);   // 250 ms window
		for (float pitchCV : PITCHES) {
			...
					CAPTURE(sr);
					CAPTURE(pitchCV);
					...
					// Held CONSTANT across the whole block. This is a steady-tone
					// measurement, not a sweep: a swept input has no single
					// frequency to be right about.
					forge::VcoBlockDriver d(sr);
					std::vector<float> out = d.run(n, [=](int) { return base; });
					REQUIRE(out.size() == (size_t)n);

					const double expected = (double)forge::kVcoFreqC4 * std::pow(2.0, (double)pitchCV);

					int nUp = 0;
					const double measured = estimateFreqRising(out, sr, &nUp);

					// A real tone anywhere on this grid produces at least 16
					// rising crossings in 250 ms ... Requiring 8 says the block is
					// genuinely oscillating; a handful would mean the measurement
					// below is meaningless rather than merely wrong.
					REQUIRE(nUp >= 8);
```
Copy: `TEST_CASE("vco ...: <prose>")`, `for (double sr : SAMPLE_RATES)`, the constant-input
lambda (`[=](int) { return base; }` — steady tone, never a sweep), `REQUIRE(out.size() == ...)`,
`CAPTURE` of every loop variable before the check, and **`REQUIRE(nUp >= 8)` before any tolerance
check**. Note this analog uses `std::pow(2.0, v)` for its expectation; the new gate uses
`261.6256 * std::exp2((double)v)` per D-18 — same principle (libm, independent), the exact form
D-18 fixes.
**Do NOT copy this case's title-embedded 1 % tolerance or its `-2..+2` grid** — TEST-02 is ~17×
tighter over a derived-boundary sweep, and per Pitfall 4 the primary tier runs at
`morph = 0.f, character = 0.f` (the estimator's error is ~100× lower on a bare sine).

**Aggregated-assertion idiom for long hostile loops** (lines 718-770) — the pattern for the D-22
standing case pinned at `±kVcoMaxPitchVolts`:
```cpp
					// ACCUMULATED, not asserted per sample: 48 configs at 20000
					// steps would otherwise add nearly four million assertions to
					// a 2.6 million assertion suite. Same idiom as scenario three.
					bool  allFinite       = true;
					...
					// WR-06. The ceiling the core is SUPPOSED to have applied,
					// recomputed here from the same constant and the same
					// sanitising rule the header uses, so the test states the
					// contract independently rather than echoing whatever the
					// header happened to compute.
					const float expectedMaxFreq =
						forge::kVcoNyquistGuardFrac * ((rate > 0.f) ? rate : 0.f);
					bool  freqNyquistBounded = true;
					...
						// Negated so a NaN freqHz also counts as a failure, the
						// same way the header's own floor is written negated.
						if (!(core.tel.freqHz <= expectedMaxFreq))        { freqNyquistBounded = false; bad = true; }
					...
					CAPTURE(rate); CAPTURE(dt); CAPTURE(pitchCV); CAPTURE(maxAbs);
					CAPTURE(maxFreqSeen); CAPTURE(expectedMaxFreq); CAPTURE(firstBadStep);
					INFO("scenario: hostile timing driven straight into the core - ...");
					CHECK(allFinite);
					CHECK(freqNyquistBounded);
```
Three transferable patterns here: (a) accumulate booleans + `firstBadStep`, `CHECK` once, to keep
the assertion count sane; (b) **recompute the expected bound symbolically from
`forge::kVcoNyquistGuardFrac`, never hardcode a Hz literal** — that is also exactly how D-21's
derived clamp boundary must be written; (c) negated comparisons in the test too, so a NaN reads as
a failure. The existing `freqNyquistBounded` pin at line 753 is driven by hostile **timing**;
this phase owes a **pitch-driven** clamp-fires case (which this analog does not cover).
The comment literal `freqHz = -21609.00` near line 636 is a **historical measurement** under the
0.49 constant — leave it alone when the constant moves (Pitfall 8).

**THE negative-control pattern — `DeliberatelyBrokenSharedStateCore`** (lines 316-366). This is
the model for the FM-03 multiplicative negative control:
```cpp
struct DeliberatelyBrokenSharedStateCore {
	// Per-instance, exactly as the real core holds them. Only `sharedPhase`
	// inside step(...) below is broken.
	forge::DriftEngine drift;
	forge::Waveshape wave;

	void seed(uint64_t s0, uint64_t s1 = 0) { drift.seed(s0, s1); }

	// The D-11 five-coefficient copy, mirroring forge::VcoCore::setSpreadSeed
	// field for field so the two cores can share one seeding callable.
	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) { ... }

	// Signature matches forge::VcoCore::step(...) so runInterleaveCheck accepts
	// this type with no change whatsoever to the helper.
	float step(const forge::VcoInputs& in) {
		// >>> THE DELIBERATE DEFECT, AND THE ONLY ONE. <<<
		static double sharedPhase = 0.0;

		float freq = forge::kVcoFreqC4 * forge::exp2_taylor5(in.pitchCV);
		const float maxFreq = forge::kVcoNyquistGuardFrac * in.sampleRate;
		// GUARD SEQUENCE MIRRORED FROM src/dsp/VcoCore.hpp — ceiling first, then
		// the NaN-safe floor as the last writer, then the direct bound on the
		// increment. Kept in step with the real core deliberately (plan 30-08);
		// see the banner above.
		if (freq > maxFreq) freq = maxFreq;
		if (!(freq > 0.f)) freq = 0.f;

		double deltaPhase = (double)freq * (double)in.sampleTime;
		if (!(deltaPhase > 0.0)) deltaPhase = 0.0;
		if (deltaPhase > forge::kVcoMaxDeltaPhase) deltaPhase = forge::kVcoMaxDeltaPhase;
		sharedPhase += deltaPhase;
		if (sharedPhase >= 1.0) sharedPhase -= 1.0;

		const float p = (float)sharedPhase;
		const float morph = forge::clamp(in.morph, 0.f, 1.f);
		const float character = forge::clamp(in.character, 0.f, 1.f);
		return 5.f * wave.morphedWave(p, morph, character, 0.f);
	}
};

} // namespace
```
The transferable rules, all stated in its banner at 300-315:
1. **Anonymous namespace, test TU only — never under `src/`.**
2. **`step()` signature matches `forge::VcoCore::step` exactly**, so the *same* helper drives both.
   The FM-03 control must therefore be driven through the same measurement helper the positive
   case uses, or it proves nothing.
3. **Exactly ONE deliberate defect**, called out in shouting caps, with an explicit
   "do not 'fix' this" warning. For FM-03 the single defect is `freq *= exp2_taylor5(fm)` after
   the pitch resolves (the LFO's shape) instead of summing volts before one `exp2`.
4. **Everything else is mirrored from the real core and must be kept in step with it** — this
   phase's new pitch/FM/D-14 lines change what "mirrored" means, so the FM-03 stand-in inherits
   the *new* guard sequence. Note the existing broken core at 346-347 mirrors only the *old*
   pitch line; if the plan updates it, that is a deliberate, stated decision (it is currently
   behaviorally inert at its own inputs, per its banner).

**The drive harness — `tests/VcoBlockDriver.hpp` (unchanged this phase):**
```cpp
struct VcoBlockDriver {
	forge::VcoCore core;
	double sampleRate = 44100.0;

	// Default seeds are non-zero (never the degenerate (0,0) Xoroshiro fixed point).
	explicit VcoBlockDriver(double sr = 44100.0,
	                        uint64_t s0 = 0x1234ULL, uint64_t s1 = 0x5678ULL,
	                        uint64_t sp0 = 0x9E3779B9ULL, uint64_t sp1 = 0x7F4A7C15ULL)
		: sampleRate(sr) { core.seed(s0, s1); core.setSpreadSeed(sp0, sp1); }

	// Drive nSamples through the core. inputAt(i) supplies the per-sample
	// VcoInputs; sampleTime and sampleRate are ALWAYS overwritten (the harness
	// owns timing). This overwrite is load-bearing — it must never become
	// conditional on what the caller's functor happened to put there.
	std::vector<float> run(int nSamples, const std::function<forge::VcoInputs(int)>& inputAt) {
		std::vector<float> out;
		out.reserve(nSamples);
		const float dt = (float)(1.0 / sampleRate);
		for (int i = 0; i < nSamples; ++i) {
			forge::VcoInputs in = inputAt(i);
			in.sampleTime = dt;
			in.sampleRate = (float)sampleRate;
			out.push_back(core.step(in));
		}
		return out;
	}

	static std::function<forge::VcoInputs(int)> sweepScenario(int nSamples, forge::VcoInputs base) { ... }
};
```
**How sample rates are injected:** through the constructor's `sr` argument, and `run()`
**unconditionally overwrites** `in.sampleTime = 1/sr` and `in.sampleRate = sr` on every sample —
so a test may not set timing through its own functor. Instantiate one driver per rate
(`forge::VcoBlockDriver d(sr);`), the way `test_vco_core.cpp:414` does.
**What it exposes for measurement:** the returned `std::vector<float>` of samples (D-19 primary
tier) and `d.core.tel` (secondary tier, `tel.freqHz`). `sweepScenario` sweeps `pitchCV` −2..+2 V
with `morph`/`character` ramping — **not usable for TEST-02**, which needs a steady tone at
`morph = 0, character = 0` (a swept input has no single frequency to be right about).
**Never** template, subclass or alias this with `tests/BlockDriver.hpp` (R-2/P-4 — that file feeds
the shipped LFO's bit-exact golden replay leg).

**Banner discipline (a real pattern in this repo, not decoration).** `test_vco_core.cpp:1-75` is a
75-line banner that numbers its invariants, names the specific measured trap each case is written
against, states what is deliberately *not* in the file, and forbids softening its own labels. The
new file's banner should do the same: name TEST-02 as the gate, record the **measured** cents
figure (D-18 forbids citing research), state that no assertion touches spectral content
(Phase 32 owns aliasing), and state that libm here is deliberate and is what makes the ground
truth independent.

---

### `tests/check_includes.sh` (guard script)

**Analog:** the array itself and the comment block immediately above it (lines 262-285):
```bash
#   * tests/test_vco_core.cpp — Phase 30's CORE-01/CORE-03 behavioral suite
#     (plans 30-03 / 30-04). Same reason as the harness suite above, and it is
#     the same KIND of file: a VCO-side test TU whose entire purpose is to drive
#     forge::VcoCore through tests/VcoBlockDriver.hpp. It is not an LFO
#     translation unit, it links into no LFO build graph, and it cannot exist
#     without the include this section would otherwise flag. Adding it changes
#     the LFO-side scan set by exactly one file, and that file is VCO code.
...
# ADDING TO THE EXEMPTION LIST IS THE DANGEROUS EDIT HERE. Removing a file from
# the scan is now the only way to unguard it, which is exactly the property the
# old allowlist did not have.
# ---------------------------------------------------------------------------
echo "[1/7] No LFO translation unit includes a VCO file (directly or transitively)..."
VCO_SIDE_ALLOW=(
	"src/vco_compile_canary.cpp"
	"src/AnalogVCO.cpp"
	"tests/VcoBlockDriver.hpp"
	"tests/test_vco_harness.cpp"
	"tests/test_vco_core.cpp"
)
```
**The literal edit (D-23):** add exactly one tab-indented, double-quoted entry
`"tests/test_vco_pitch.cpp"` to `VCO_SIDE_ALLOW`, **and** a matching paragraph to the comment
block above carrying its own rationale — that prose block is part of the pattern, not optional.

**Why the entry weakens no detector** (the match, line 293-295):
```bash
	for a in "${VCO_SIDE_ALLOW[@]}"; do
		if [[ "${rel}" == "${a}" ]]; then skip=1; fi
	done
```
The RHS is **quoted**, so `==` is a literal string comparison — no glob, no substring, no
basename. The exemption is **exact-path** and removes exactly one file from the LFO-side scan set.
Contrast with deferred item 5, the *unanchored* exemption filter in `[2/7]` — a different
mechanism in the same script, and this phase now touches the script, so the planner must either
fold that fix in or re-own the deferral (CONTEXT.md `<deferred>` flags this explicitly).

---

## Shared Patterns

### C++11 / two-standard split
**Source:** `src/dsp/VcoCore.hpp:41-64` (the binding rule list) and `src/AnalogVCO.cpp:40-49`
**Apply to:** `src/dsp/VcoCore.hpp`, `src/AnalogVCO.cpp` — **not** `tests/test_vco_pitch.cpp`
```
//   - No `inline constexpr` variables (C++17 inline variables). Plain constexpr
//     at namespace scope has internal linkage per TU and is the C++11 form
//   - No `if constexpr`, no structured bindings, no [[maybe_unused]], no
//     nested-namespace definition syntax, no auto return-type deduction, no
//     generic lambdas.
//   - No std::clamp (C++17 <algorithm>) — use forge::clamp / forge::clampi
//   - Any constant table must be a namespace-scope `static constexpr`, never an
//     in-class `static constexpr` that is indexed at runtime ... That exact class
//     of bug got v2.0.0 rejected from the VCV Library.
//   - Do NOT brace-initialize VcoInputs with a value list.
```
`tests/*.cpp` builds at `-std=c++17` and may use C++17 freely — but `test_vco_core.cpp:96-99`
records that the C++11-shaped `VcoInputs` init idiom is kept in tests *anyway*, "because this same
idiom is copied into `src/` where C++11 is binding." Follow that.
`make strict` is `-fsyntax-only`, so it cannot catch the ODR/link class — only the MinGW CI
**link** leg can (Pitfall 7). Nothing in this phase's excerpts requires post-C++11 features.

### libm split (`std::exp2` / `std::pow`)
**Source:** `src/dsp/VcoCore.hpp:171-175` (banned) vs `tests/test_vco_core.cpp:418` (required)
**Apply to:** every file
```cpp
// src/  — BANNED. bit-identity of the FM path depends on this exact function.
float freq = kVcoFreqC4 * exp2_taylor5(in.pitchCV);

// tests/ — REQUIRED. Independent ground truth is what makes the gate non-vacuous.
const double expected = (double)forge::kVcoFreqC4 * std::pow(2.0, (double)pitchCV);
// TEST-02's form, per D-18:
const double expected = 261.6256 * std::exp2((double)volts);
```
`src/dsp/LfoCore.hpp` uses `std::fmax`/`std::sqrt`/`std::round` — libm generally is *not* banned
in `src/`; **`std::exp2` and `std::pow` specifically are**, because the exponential is the shared
bit-identity surface.

### Negated-comparison guard for non-finite input
**Source:** `src/dsp/VcoCore.hpp:233`, `:259`; mirrored in tests at `test_vco_core.cpp:750-753`
**Apply to:** the new D-14 bound in `VcoCore.hpp`, and every assertion about a possibly-NaN value
```cpp
if (!(freq > 0.f)) freq = 0.f;                 // NaN fails `> 0.f`, so it lands at 0
if (!(deltaPhase > 0.0)) deltaPhase = 0.0;
if (!(core.tel.freqHz <= expectedMaxFreq)) { freqNyquistBounded = false; }   // in-test form
```
**Never** `forge::clamp` for this — `RackCompat.hpp`'s ladder is NaN-transparent (both comparisons
false ⇒ NaN returned unchanged), which is inert against the exact input class D-14 exists to stop.

### Symbolic recomputation of bounds in tests, never a hardcoded literal
**Source:** `tests/test_vco_core.cpp:735-736` and `:347`
```cpp
	const float expectedMaxFreq =
		forge::kVcoNyquistGuardFrac * ((rate > 0.f) ? rate : 0.f);
```
Every reference to the Nyquist constant across `src/` and `tests/` is **symbolic**, so D-11's
value change propagates automatically. D-21's clamp boundary must follow the same rule:
`std::log2((double)forge::kVcoNyquistGuardFrac * sr / (double)forge::kVcoFreqC4)`.
Exception, and it is a real one: comment literals recording a *past* measurement
(`VcoCore.hpp:214`, `test_vco_core.cpp:636`, both `-21609.00`) are history, not expectations —
leave them (Pitfall 8).

### Load-bearing comments with recorded measurements and explicit "do not" warnings
**Source:** `src/dsp/VcoCore.hpp:203-231`, `tests/VcoBlockDriver.hpp:9-26`,
`tests/test_vco_core.cpp:300-315`, `src/AnalogVCO.cpp:86-113`
**Apply to:** every file this phase touches
The house style states the measured observation, the reproduction conditions, who reproduced it,
and the prohibition in caps (`DO NOT SWAP THESE TWO LINES BACK`, `Do not template them.`,
`Do not remove them.`). Corollary the phase must honor in the other direction: a comment whose
arithmetic the phase invalidates must be **corrected**, not left standing —
`VcoCore.hpp:249-250`'s "clears that maximum by roughly two percent" becomes ~1.0 % at `0.495`,
and `AnalogVCO.cpp:129-142`'s "stay at their header defaults" becomes false.

### Anonymous-namespace test locals
**Source:** `tests/test_vco_core.cpp:87` … `:368` (`namespace { ... } // namespace`)
**Apply to:** every helper, constant and stand-in core in `tests/test_vco_pitch.cpp`
Two test TUs in the same binary that both define `coreBase`/`estimateFreqRising`/`SAMPLE_RATES` at
external linkage would be an ODR violation; the anonymous namespace is what makes copying the
helpers into the new file correct rather than a hazard. Same family of landmine as R-9
(`forge::VcoInputs`, never `forge::Inputs` — `forge::Inputs` belongs to the LFO).

### Guard-suite tripwires any of these files can hit
**Source:** `tests/check_canary.sh [2b/5]`, `tests/check_includes.sh [1/7]`, `tests/check_frozen.sh`
| Tripwire | Triggered by | Cost |
|----------|-------------|------|
| `check_canary.sh [2b/5]` source shape | reformatting `struct VcoCore {` or the `float step(...) {` line; quoting the full signature in a comment containing `{` | `make guards` hard-fails with "could not perturb src/dsp/VcoCore.hpp" — reads like a DSP error |
| `check_canary.sh [2b/5]` field liveness | changing which `VcoInputs` fields `src/vco_compile_canary.cpp` keeps runtime-live at `-O3` | the canary is the constant-fold coverage, not `AnalogVCO.cpp` |
| `check_includes.sh [1/7]` | a new `tests/test_vco_*.cpp` without its `VCO_SIDE_ALLOW` entry | exit 1 on first run (this happened in Phase 30) |
| `check_frozen.sh` | `FROZEN.sha256` appearing in the diff — i.e. any edit to `RackCompat.hpp` / `Waveshape.hpp` / `LfoCore.hpp` | guardrail event; **no frozen header is edited this phase**, so no `FROZEN.sha256` bump |

Run `make guards` after **every** header edit, not only at the end.

---

## No Analog Found

| File / need | Role | Data Flow | Reason |
|-------------|------|-----------|--------|
| The D-14 one-shot UBSan probe (D-22) | diagnostic tooling | one-shot | **No in-tree analog exists.** The repo has no sanitizer target, no scoped-diagnostic `make` rule, and no precedent for a non-gate probe. `RESEARCH.md` §Pitfall 3 supplies the binding shape (VCO-only TU, never the `tests/*.cpp` glob, never a permanent repo-wide gate — it would light up the shipped LFO per D-24) and §The D-14 Pitch-Volt Bound supplies the literal expected diagnostic text (`RackCompat.hpp:106` float-cast-overflow, `:109` left-shift overflow). Use RESEARCH.md, not a codebase pattern. |
| A pitch-driven Nyquist-clamp-fires case (PITCH-04 / D-10) | test | batch | **Partial only.** `tests/test_vco_core.cpp:753` pins `freqNyquistBounded` symbolically, but it is driven by hostile *timing* and never observes the clamp firing on a legitimate high note, nor that the oscillator keeps sounding at the ceiling. The bound-recomputation idiom (735-736) transfers; the case itself is new. |
| A bit-exact block-equality helper for the FM-03 summation identity | test | batch | **Adjacent only.** `test_vco_core.cpp` compares blocks bit-exactly (the interleave check, and its banner at 193-195 is explicit: direct float `!=`, **never** `doctest::Approx`, whose `epsilon(0)` still applies a relative margin), but that comparison is fused into `runInterleaveCheck`'s two-instance shape and is not a reusable "two blocks are identical" helper. Copy the *rule* (direct `!=`, no `Approx`), write the helper fresh. |

---

## Metadata

**Analog search scope:** `src/`, `src/dsp/`, `tests/`, `res/`
**Files read this session:** `31-CONTEXT.md`, `31-RESEARCH.md` (full), `src/dsp/VcoCore.hpp` (full),
`src/AnalogVCO.cpp` (full), `src/AnalogLFO.cpp:185-244`, `src/dsp/LfoCore.hpp:150-194`,
`tests/VcoBlockDriver.hpp` (full), `tests/test_vco_core.cpp` (lines 1-99, 100-219, 300-429,
700-789 + `TEST_CASE`/BDD grep over the whole file), `tests/check_includes.sh:262-321`,
`res/AnalogVCO.svg` (full)
**Pattern extraction date:** 2026-07-30
