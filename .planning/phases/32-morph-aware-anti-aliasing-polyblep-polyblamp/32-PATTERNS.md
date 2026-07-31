# Phase 32: Morph-Aware Anti-Aliasing (polyBLEP/polyBLAMP) - Pattern Map

**Mapped:** 2026-08-01
**Files analyzed:** 8 (2 new, 6 modified) + 3 must-not-touch
**Analogs found:** 8 / 8

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/dsp/MorphBlep.hpp` **NEW** | DSP utility header (struct + free fn) | per-sample transform | `src/dsp/Waveshape.hpp` (struct/POD + banner) + `src/dsp/VcoCore.hpp` (C++11 rules, guard idiom, per-instance state) | exact (role), split across two |
| `tests/test_vco_spectrum.cpp` **NEW** | test (doctest TU) | batch measure over block driver | `tests/test_vco_core.cpp` (structure, oracle, accumulate-don't-assert) + `tests/test_vco_pitch.cpp` (measured-provenance grid rows, skip-not-clip) | exact |
| `src/dsp/VcoCore.hpp` MOD | DSP core | request-response (POD in, float out) | itself — `:484` call site; local pattern at `:363-364`, `:431-432`, `:470-472` | exact (self) |
| `src/AnalogVCO.cpp` MOD | Rack shell (module + widget) | param/CV plumbing | itself — Phase 31's `FM_ATTEN_PARAM` + `FM_INPUT` addition | exact (self) |
| `tests/test_vco_core.cpp` MOD | test | batch | itself — `:349-418` oracle, `:524-611` bound, `:735-826` hostile grid | exact (self) |
| `src/vco_compile_canary.cpp` MOD | build gate TU | compile-only | itself — `:54-56` pre-staged growth point | exact (self) |
| `tests/check_includes.sh` MOD | guard script | denylist scan | itself — `VCO_SIDE_ALLOW` at `:325-332` with the `:300-316` rationale-comment precedent | exact (self) |
| `res/AnalogVCO.svg` MOD | panel asset | static | itself — the eight existing marker rects paired 1:1 with `AnalogVCO.cpp:266-281` | exact (self) |

---

## Pattern Assignments

### `src/dsp/MorphBlep.hpp` (NEW — DSP utility header, per-sample transform)

**Analogs:** `src/dsp/Waveshape.hpp` (banner + struct shape), `src/dsp/VcoCore.hpp` (C++11 contract, NaN idiom, per-instance state), `src/dsp/DriftEngine.hpp` (banner "lifted from / bit-identity discipline" framing).

**Banner pattern** — every `src/dsp/*.hpp` opens `#pragma once`, then a `// src/dsp/X.hpp` path line, then a prose block naming provenance, the load-bearing invariant, and the include-hygiene line. From `Waveshape.hpp:1-16`:

```cpp
#pragma once
// src/dsp/Waveshape.hpp
//
// Pure waveshape math (sine/tri/saw/square/pulse + morph crossfade + bleed),
// lifted VERBATIM from src/AnalogLFO.cpp L194-388 (D-02: leaf dependency of the
// driveable LfoCore). ...
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes. Every free
// function is `inline` or a struct member (ODR, Pitfall 4).
```

Copy this shape. `MorphBlep.hpp`'s banner must additionally carry the D-01 **frozen-internals dependency note** (RESEARCH § "The header skeleton" gives the exact wording) — the analog for that kind of note is `DriftEngine.hpp:10-15`'s "Bit-identity discipline (D-03/D-07/D-08): the per-sample RNG draw ORDER is load-bearing".

**Include pattern** (`Waveshape.hpp:18-23`, `VcoCore.hpp:82-90`) — include-what-you-use, one trailing comment per include naming the symbol used and its status:

```cpp
#include <cmath>
#include "dsp/Waveshape.hpp"   // forge::Waveshape::morphedWave (FROZEN — call it, never edit it)
```

`VcoCore.hpp:89` already uses that exact trailing comment; reuse it verbatim.

**Namespace + struct pattern** (`Waveshape.hpp:25-32`): `namespace forge {` … `struct X { float member = 0.f; …` — NSDMI defaults, public POD-ish struct, no constructors, closing `} // namespace forge` (`VcoCore.hpp:496`).

**Per-instance state pattern** (`VcoCore.hpp:243-249`) — the CORE-03 / D-14 constraint has an existing comment that states it and should be echoed:

```cpp
	// Per-instance oscillator state, mirroring src/dsp/LfoCore.hpp:58-63. Both
	// members are INSTANCE state, not static or shared — which is literally what
	// CORE-03 asserts: two cores stepped interleaved must not see each other.
	double phase = 0.0;
	Waveshape wave;
```

Hold `MorphBlep` by value in `VcoCore` next to `wave`, in the same block, with the same style of comment.

**NaN / hostile-input guard pattern** (`VcoCore.hpp:363-364`, `:431-432`, `:471`) — the negated comparison, never `forge::clamp`:

```cpp
			if (!(pitchVolts > -kVcoMaxPitchVolts)) pitchVolts = -kVcoMaxPitchVolts;
			...
			if (freq > maxFreq) freq = maxFreq;
			if (!(freq > 0.f)) freq = 0.f;
			...
			if (!(deltaPhase > 0.0)) deltaPhase = 0.0;
```

with the rejection of `clamp` recorded in words (`VcoCore.hpp:357-362`):

```
			// forge::clamp IS REJECTED HERE BY NAME (deferred item 3 / CR-02). It is
			// a comparison ladder — `x < lo ? lo : (x > hi ? hi : x)` — and BOTH of
			// its comparisons are FALSE for a NaN, so a NaN passes straight through
			// it UNCHANGED.
```

`morphBlepCharFactor(w, dt)` and the `dt` guard at the top of `MorphBlep::step` must both use this idiom.

**Namespace-scope constant pattern** (`VcoCore.hpp:94-110`) — plain `constexpr` at namespace scope, never `inline constexpr`, never in-class `static constexpr` indexed at runtime; every constant carries a provenance paragraph:

```cpp
// Namespace-scope plain constexpr — the src/dsp/MathConst.hpp idiom this file's
// banner mandates above. NOT `inline constexpr` (C++17), NOT an in-class
// `static constexpr` (declaration-only under C++11 → MinGW undefined reference).
constexpr float kVcoFreqC4 = 261.6256f;
```

If the D-04 9-entry site table becomes an array, this is the only permitted form (or a function-local `const`).

**Rules inherited verbatim from `VcoCore.hpp:55-80`** (the two-standard C++11 block) — quote the list, do not restate loosely: no `inline constexpr` variables, no `if constexpr`, no `std::clamp`, no `<cmath>` pi macro (use `forge::kPi`), no in-class `static constexpr` indexed at runtime, no brace value-list init of `VcoInputs`, zero Rack includes.

---

### `src/dsp/VcoCore.hpp` (MODIFIED — DSP core, request-response)

**Analog:** itself. The insertion point is `:476-492`:

```cpp
		const float p = (float)phase;
		const float morph = clamp(in.morph, 0.f, 1.f);
		const float character = clamp(in.character, 0.f, 1.f);

		// D-12: ONE call into the frozen Waveshape — a call, never an edit.
		// bleedLfo = 0.f is the OU-layer-0 read, and 0 is correct because this
		// phase steps no OU layer (D-11: no drift stepping, no per-sample RNG).
		// Phase 34 passes the real layer-0 state here.
		const float sample = wave.morphedWave(p, morph, character, 0.f);
		tel.displayPhase = p;

		return 5.f * sample;
```

Pattern to preserve: `p`, `morph`, `character` are computed once and the SAME `p` is handed both to `morphedWave` and (now) to `MorphBlep` — RESEARCH Pattern 2 makes that identity load-bearing. `deltaPhase` (double, `:470-472`) is the `dt` source. `tel.displayPhase = p;` and the `× 5` stay exactly where they are.

**Source-shape contract — hard constraint, not style** (`VcoCore.hpp:34-43`): the `struct VcoCore {` line and the `float step(const VcoInputs& in) {` line must each stay on ONE line with their opening brace, or `make guards` hard-fails with "could not perturb src/dsp/VcoCore.hpp". Also: never quote the full `step` signature in a comment on a line that also contains `{`.

**Comment-density pattern:** every behavioral change in this file carries a MEASURED-figure paragraph above it (see `:394-430`, `:440-465`). New code must do the same, drawing figures from `32-RESEARCH.md`'s measured tables — not from prose.

---

### `src/AnalogVCO.cpp` (MODIFIED — Rack shell, param/CV plumbing)

**Analog:** itself — Phase 31's FM control addition is the exact template for Phase 32's MORPH control addition. Copy it in four places.

**1. Enum entries** (`:76-88`) — append; ID churn is explicitly free (banner `:22-28`):

```cpp
	enum ParamId {
		MORPH_PARAM,
		CHARACTER_PARAM,
		COARSE_PARAM,
		FINE_PARAM,
		FM_ATTEN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		FM_INPUT,
		INPUTS_LEN
	};
```

**2. Attenuverter param declaration** (`:132-136`) — D-17 says MORPH's attenuverter follows this precedent exactly (bipolar `-1..+1`, linear taper, default `0`, `%` display with `0.f, 100.f`):

```cpp
		// BIPOLAR by FM-02 / D-07, so a negative setting gives inverted FM. The
		// shipped module's styling is borrowed — linear taper, default off,
		// percentage readout, and the very same control name — but its RANGE is
		// NOT: that control is a unipolar attenuator over 0..1, and copying its
		// range here would silently drop half of what FM-02 asks for.
		configParam(FM_ATTEN_PARAM, -1.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f);

		configInput(VOCT_INPUT, "V/Oct");
		configInput(FM_INPUT, "FM");
```

**3. `process()` forwarding** (`:179-213`) — note the DIVERGENCE the planner must handle: Phase 31 forwards RAW and computes nothing, whereas D-17 authorises a knob + CV × attenuverter mix + clamp here. That is the *one* sanctioned exception to the banner, and it must be justified in a comment in the same voice as the existing D-09/D-17 paragraph at `:196-207`:

```cpp
		forge::VcoInputs in;
		in.pitchCV = inputs[VOCT_INPUT].getVoltage();
		in.morph = params[MORPH_PARAM].getValue();
		...
		in.fmVolts = inputs[FM_INPUT].getVoltage();
		in.fmConnected = inputs[FM_INPUT].isConnected();
		outputs[OUTPUT].setVoltage(core.step(in));
```

**4. The field-accounting comment block** (`:215-245`) must be updated in the same commit — it currently states "SEVEN of the eight VcoInputs DSP fields" and "the field-count margin is therefore down to ONE". D-17 adds no POD field, so the counts do not move, but the block's growth rule must be re-read and the no-change stated deliberately.

**5. Widget coordinates** (`:257-281`) — two new `createParamCentered<RoundBlackKnob>` / `createInputCentered<PJ301MPort>` calls, and the paired comment rule at `:257-262` binds: the SVG marker rects and these coordinates are written together in ONE commit.

```cpp
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.48f, 40.f)),
		         module, AnalogVCO::MORPH_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(45.72f, 100.f)),
		         module, AnalogVCO::FM_INPUT));
```

**Banner update:** `:15-21` says "Eight controls, no more" — that count becomes ten and the sentence must be edited, not left to rot (the Phase-31 precedent is visible in the same paragraph).

---

### `tests/test_vco_spectrum.cpp` (NEW — doctest test TU, batch measurement)

**Analogs:** `tests/test_vco_core.cpp` (TU shape, oracle struct, accumulate-then-assert), `tests/test_vco_pitch.cpp` (measured-provenance grid, skip-not-clip, tier labelling).

**TU/include pattern** (`test_vco_core.cpp:78-112`) — no `DOCTEST_CONFIG_IMPLEMENT` (that lives in `tests/main.cpp`), driver header first, then std headers each with a trailing comment naming the one symbol it supplies, then an anonymous namespace with `SAMPLE_RATES` and a `coreBase()`:

```cpp
#include "VcoBlockDriver.hpp"

#include <vector>
#include <functional>   // std::function — the interleave helper's seeder/input parameters
#include <cmath>
#include <cstdint>
#include <limits>       // std::numeric_limits<float>::quiet_NaN() — scenario four's hostile timing grid

namespace {

constexpr double SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0};

forge::VcoInputs coreBase() {
	forge::VcoInputs in;
	in.pitchCV   = 0.f;
	in.coarse    = 0.f;   // Phase 31 — unread by this step() body
	...
	return in;
}
```

Note: `check_includes.sh` `[1/7]` is a denylist, so this file needs a `VCO_SIDE_ALLOW` entry (below) before it lands.

**Driving pattern** (`test_vco_core.cpp:466-468`) — a steady tone is a constant-lambda run over `VcoBlockDriver`; the driver overwrites `sampleTime`/`sampleRate` unconditionally (`VcoBlockDriver.hpp:53-64`), which is what constrains D-10's bin-centre construction to bisecting `pitchCV`:

```cpp
					forge::VcoBlockDriver d(sr);
					std::vector<float> out = d.run(n, [=](int) { return base; });
					REQUIRE(out.size() == (size_t)n);
```

**Analysis-helper pattern** (`test_vco_core.cpp:114-149` `estimateFreqRising`) — a free helper in the anonymous namespace, preceded by a banner that (a) states why it is structurally sound rather than heuristic, with the grid size it was validated over, and (b) names the simplification that must NOT be reintroduced and the measured error it would carry. The DFT/Goertzel helper and the alias-bin classifier take this exact shape, including the "do not 'simplify' this back" closing line.

**Non-vacuity pattern** (`test_vco_core.cpp:472-480`) — assert the measurement is meaningful before asserting the value:

```cpp
					int nUp = 0;
					const double measured = estimateFreqRising(out, sr, &nUp);
					// A real tone anywhere on this grid produces at least 16
					// rising crossings in 250 ms ...
					REQUIRE(nUp >= 8);
```

This is the direct analog for D-10's leakage self-check: `REQUIRE` the achieved bin error implies a leakage floor ≥ 10 dB below the threshold being asserted, BEFORE `CHECK`ing the alias floor.

**Threshold-table pattern** (`test_vco_pitch.cpp:1131-1165`) — the per-`(morph region, note, character)` D-09 matrix is a `struct Row` table with a `role` string that is `CAPTURE`d, grouped by lettered comment sections that state what each group proves:

```cpp
	struct Row { float voct; float coarse; const char* role; };

	static const Row GRID[] = {
		// (a) THE DECLARED RANGE. -5 and +5 are exactly the bounds
		//     src/AnalogVCO.cpp declares for COARSE_PARAM, so these two rows are
		//     what proves the whole range is reachable rather than its middle.
		{ 0.f, -5.f,   "range endpoint, minus five octaves" },
		...
	};
	const size_t nRows = sizeof(GRID) / sizeof(GRID[0]);
```

Extend the row struct with `naiveDb` / `thresholdDb` and keep the `role` field — that is where D-09's "each threshold carries its measured justification in the test" lives.

**Skip-not-clip pattern** (`test_vco_pitch.cpp:1195-1201`) — for grid points where the technique legitimately cannot improve (e.g. triangle at character 1, P-6), skip and count, never soften the number:

```cpp
			// SKIPPED, NOT CLIPPED. Above the binding limit the apparatus or
			// D-10's clamp takes over and a cents assertion would fail on
			// CORRECT behavior. Clipping the row to the limit instead would
			// quietly test a different input than the grid states.
			if (summed > top) { ++skippedPoints; continue; }
```

**Capture-on-pass pattern** (`test_vco_core.cpp:549-554`) — the measured dB figures must appear in `-s` output on a PASS, because pinning what the oscillator actually does is half this suite's job:

```cpp
				// Captured so the measured figure appears in `-s` output on a PASS,
				// not only on a failure ...
				CAPTURE(maxAbs);
				CHECK(maxAbs <= kLooseBoundV);
```

**Scenario-labelling pattern** (`test_vco_core.cpp:534-537`) — use `INFO("scenario: …")` with a stream insertion, never `CAPTURE` on a bare `const char*` (doctest stringifies it as a pointer).

**Naive-path oracle pattern** (`test_vco_core.cpp:349-418`, `DeliberatelyBrokenSharedStateCore`) — this is the analog for D-08's "keep the naive path callable". An in-test struct that mirrors the real core's pitch/guard/accumulate block line for line and diverges in exactly one documented place, with a banner that (a) marks the divergence `>>> THE DELIBERATE DEFECT, AND THE ONLY ONE. <<<`, (b) records the re-observed assertion counts, and (c) says "IF ANY OF THOSE FIGURES EVER MOVES … STOP AND REPORT IT RATHER THAN UPDATING THE NUMBER" (`:344-347`). A test-only naive shim built on this pattern needs no `src/` flag and no second entry point in the core.

---

### `tests/test_vco_core.cpp` (MODIFIED — test)

**Analog:** itself, three named sites.

**a) The `:416` oracle.** `DeliberatelyBrokenSharedStateCore::step` reimplements the naive path and ends:

```cpp
		const float p = (float)sharedPhase;
		const float morph = forge::clamp(in.morph, 0.f, 1.f);
		const float character = forge::clamp(in.character, 0.f, 1.f);
		return 5.f * wave.morphedWave(p, morph, character, 0.f);
```

The mirror-maintenance rule at `:400-403` (“GUARD SEQUENCE MIRRORED FROM src/dsp/VcoCore.hpp … Kept in step with the real core deliberately”) means this line must gain the same band-limited correction, and the re-observed 512/512/1024 mismatch figures at `:336-347` must be re-captured and compared as numbers.

**b) The `:511` bound.** `kLooseBoundV = 6.0f` at `:525`, with its derivation at `:504-515` and the load-bearing `CHECK(maxAbs > 5.1f)` at `:610`. RESEARCH P-10 supplies the replacement table; the derivation paragraph is rewritten in the same voice (analytic ceiling → numerical confirmation → "a REAL bound, not a round number"), and the `> 5.1f` non-vacuity assertion must be re-derived alongside it, not dropped.

**c) Scenario four's hostile grid** (`:735-826`) — extend the two arrays in place; the surrounding structure (direct `core.step(in)` with NO driver, accumulate-don't-assert, five named CHECKs) is unchanged:

```cpp
		static const float HOSTILE_RATES[] = {
			-44100.f, 0.f, 44100.f, std::numeric_limits<float>::quiet_NaN()
		};
		static const float HOSTILE_TIMES[] = {
			-1.f / 44100.f, 0.f, 1.f / 44100.f, 1.f / 1000.f, 999.f,
			std::numeric_limits<float>::quiet_NaN()
		};
```

D-15 adds `±inf`, subnormal (`std::numeric_limits<float>::denorm_min()`) and very-large-finite entries. **Also required:** `:743-744` contains the falsified premise — "the shape Phase 32's oversampled inner loop will produce naturally" — and `VcoCore.hpp:446` carries the same falsified sentence. Both must be corrected per D-15, not inherited.

Accumulate-don't-assert idiom (`:770-807`), which the extended grid must keep or the suite gains millions of assertions:

```cpp
					// ACCUMULATED, not asserted per sample: 48 configs at 20000
					// steps would otherwise add nearly four million assertions to
					// a 2.6 million assertion suite.
					bool allFinite = true; ... int firstBadStep = -1;
```

---

### `src/vco_compile_canary.cpp` (MODIFIED — build-gate TU)

**Analog:** itself. The growth point is pre-staged at `:54-56`; activating it is a two-line edit:

```cpp
#include "dsp/VcoCore.hpp"
// D-08 growth point — Phase 32 adds the morph-BLEP header here:
// #include "dsp/MorphBlep.hpp"
```

`check_canary.sh` strips comment lines first (`:400-402`), so the commented placeholder does **not** satisfy `[5/5]` once `src/dsp/MorphBlep.hpp` exists — the include must be activated in the SAME commit as the header. The growth rule at `:23-27` names this file's obligation explicitly. D-17 adds no `VcoInputs` field, so the runtime-derived field block at `:91-99` needs no addition — state that finding, do not silently skip it.

---

### `tests/check_includes.sh` (MODIFIED — guard script)

**Analog:** itself. `[1/7]` is a **denylist**: every `src|tests|tools` file not in `VCO_SIDE_ALLOW` and not matching `src/dsp/Vco*.hpp|src/dsp/MorphBlep.hpp` is scanned for VCO includes.

```bash
VCO_SIDE_ALLOW=(
	"src/vco_compile_canary.cpp"
	"src/AnalogVCO.cpp"
	"tests/VcoBlockDriver.hpp"
	"tests/test_vco_harness.cpp"
	"tests/test_vco_core.cpp"
	"tests/test_vco_pitch.cpp"
)
```

Add `"tests/test_vco_spectrum.cpp"`. **The rationale-comment is part of the pattern**, not optional — `:300-316` is the Phase-31 precedent (D-23) and shows the required content: why the file is VCO-side, that the entry is added BEFORE the file exists (the Phase-29 precedent — reactive addition turned this section red once already), that the match is against a QUOTED right-hand side so it is exact-path, and the standing warning at `:320-322` that adding to this list is the dangerous edit.

`src/dsp/MorphBlep.hpp` itself needs **no guard edits** — it is already pre-wired in `check_includes.sh:90, 261, 317, 337` and `check_canary.sh:414, 452-453, 466`.

---

### `res/AnalogVCO.svg` (MODIFIED — throwaway panel asset)

**Analog:** itself. Eight marker rects at the final 18 HP geometry, each "sitting at its control centre minus five in both axes" (`AnalogVCO.cpp:257-262`). Add two rects at the two new control centres, in the SAME commit as the widget coordinates. Replaced wholesale in Phase 35.

---

## Shared Patterns

### C++11 two-standard contract
**Source:** `src/dsp/VcoCore.hpp:55-80`
**Apply to:** `MorphBlep.hpp`, `VcoCore.hpp`, `AnalogVCO.cpp`, `vco_compile_canary.cpp` (everything under `src/`)
No `inline constexpr` variables; no `if constexpr`/structured bindings/`[[maybe_unused]]`/nested-namespace syntax/auto return deduction/generic lambdas; no `std::clamp` (use `forge::clamp`/`forge::clampi`); no `<cmath>` pi macro (use `forge::kPi`); constant tables namespace-scope `static constexpr` only, never in-class indexed at runtime; never brace value-list init `VcoInputs`. Verified by `make strict` (`-std=c++11 -pedantic-errors -fsyntax-only` over `$(wildcard src/*.cpp)`, Makefile `:76-79`).

### NaN-safe guard idiom
**Source:** `src/dsp/VcoCore.hpp:349-364`, `:431-432`, `:471`
**Apply to:** every new divisor/bound in `MorphBlep.hpp`
```cpp
			if (!(x > 0.f)) x = 0.f;    // negated: a NaN fails the comparison and lands on the fallback
```
`forge::clamp` is rejected by name for hostile inputs (it is a comparison ladder; both comparisons are false for NaN).

### Test file guard/build wiring
**Source:** Makefile `:36-48`, `tests/check_includes.sh:325-332`
**Apply to:** `tests/test_vco_spectrum.cpp`
`TEST_SOURCES := $(wildcard $(TEST_DIR)/*.cpp)` — no Makefile edit needed. `TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -I$(TEST_DIR) -Wall -Wextra -ffp-contract=off` — `-ffp-contract=off` is load-bearing for the BLEP polynomials (P-11). The only wiring is the one `VCO_SIDE_ALLOW` line.

### Guard suite mechanics
**Source:** Makefile `:99-106`
```make
GUARD_SCRIPTS := tests/check_frozen.sh tests/check_includes.sh tests/check_canary.sh

.PHONY: guards
guards:
	@for s in $(GUARD_SCRIPTS); do echo "=== $$s ==="; bash $$s || exit 1; done
```
`make guards` is Rack-free (bash + checksum + a compiler). `check_frozen.sh` compares CR-stripped SHA-256 against `src/dsp/FROZEN.sha256`; a frozen file may change ONLY with a manifest bump in the same commit (`check_frozen.sh:17-23`). **Phase 32 bumps nothing** — no entry in the 15-line manifest is touched.

### Measured-provenance comment discipline
**Source:** `src/dsp/VcoCore.hpp:99-109`, `:394-430`; `tests/test_vco_core.cpp:504-515`, `:557-585`; `tests/test_vco_pitch.cpp:115-126`
**Apply to:** every new constant, threshold and bound in this phase
Every number carries: what it was measured against, at what grid/rate, why the neighbouring value was rejected, and — for historical figures — an explicit instruction not to recompute them. Thresholds sourced from `32-RESEARCH.md`'s `[MEASURED]` tables, never from ROADMAP prose.

### Falsified-premise correction discipline
**Source:** `tests/test_vco_core.cpp:344-347`, `:623-632` (the plan-30-08 scope correction), `:1160-1163`
**Apply to:** `VcoCore.hpp:446`, `test_vco_core.cpp:743-744` (the "Phase 32's oversampled inner loop" premise falsified by AA-05), and RESEARCH P-4 (the saw soft-reset premise)
The house pattern is to keep the conclusion, replace the premise in place, and say in the comment that the old reason was falsified — not to delete the sentence.

---

## Must-Not-Touch (verified this session)

| File | Status | Verification |
|------|--------|--------------|
| `src/dsp/Waveshape.hpp` | FROZEN, byte-pinned | `src/dsp/FROZEN.sha256:1` `e8ae0700…`; enforced by `tests/check_frozen.sh` inside `make guards` |
| `src/dsp/RackCompat.hpp` | FROZEN, byte-pinned, shipped-LFO-consumed | `FROZEN.sha256:2` `405a878d…`; D-12 forbids putting BLEP kernels here |
| `src/AnalogLFO.cpp` | FROZEN **and** must be absent from this phase's diff | `FROZEN.sha256:12` `fae084ca…`; Phase 30/31 precedent (`AnalogVCO.cpp:44-49`) |
| `src/dsp/MathConst.hpp`, `DriftEngine.hpp`, `LfoCore.hpp`, `ClockTracker.hpp`, `RatioTable.hpp`, `Swing.hpp`, `PatchParse.hpp`, `DisplayFill.hpp`, `Anim.hpp` | FROZEN | remaining `FROZEN.sha256` entries |
| `tests/BlockDriver.hpp`, `tests/test_golden.cpp`, `tests/golden/freerun_seeds.txt` | FROZEN | `FROZEN.sha256:13-15`; `VcoBlockDriver.hpp:9-18` forbids templating/subclassing/aliasing the two drivers |

**No `FROZEN.sha256` bump is required or permitted in Phase 32.**

## No Analog Found

None. Every file in scope has an exact or near-exact in-repo analog.

## Metadata

**Analog search scope:** `src/`, `src/dsp/`, `tests/`, `res/`, `Makefile`
**Files scanned:** 14 read in full or in targeted ranges (`VcoCore.hpp`, `Waveshape.hpp`, `DriftEngine.hpp`, `AnalogVCO.cpp`, `vco_compile_canary.cpp`, `VcoBlockDriver.hpp`, `test_vco_core.cpp`, `test_vco_pitch.cpp`, `check_includes.sh`, `check_canary.sh`, `check_frozen.sh`, `FROZEN.sha256`, `Makefile`, phase docs)
**Pattern extraction date:** 2026-08-01
