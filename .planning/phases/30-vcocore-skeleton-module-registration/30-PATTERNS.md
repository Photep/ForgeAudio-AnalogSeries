# Phase 30: VcoCore Skeleton & Module Registration - Pattern Map

**Mapped:** 2026-07-28
**Files analyzed:** 8 (2 new source, 1 new asset, 1 new test, 4 modified)
**Analogs found:** 8 / 8 (all exact or role-match — this phase is a mirror of a shipped module)

---

## File Classification

| New/Modified File | Status | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|--------|------|-----------|----------------|---------------|
| `src/dsp/VcoCore.hpp` | MODIFIED | DSP core (Rack-free header-only) | transform (POD in → float + telemetry out), per-sample | `src/dsp/LfoCore.hpp` | **exact** |
| `src/AnalogVCO.cpp` | NEW | Rack shell (Module + ModuleWidget + Model) | request-response per audio sample | `src/AnalogLFO.cpp` (read-only reference — **never edited**) | **exact** (role), stripped scope |
| `res/AnalogVCO.svg` | NEW | asset / panel | static resource | `res/AnalogLFO.svg` (header line only) | role-match |
| `src/plugin.hpp` | MODIFIED | config / registration | declaration | its own existing `extern Model* modelAnalogLFO;` | **exact** |
| `src/plugin.cpp` | MODIFIED | config / registration | init-time | its own existing `p->addModel(modelAnalogLFO);` | **exact** |
| `plugin.json` | MODIFIED | manifest | static config | its own existing `modules[0]` entry | **exact** |
| `tests/test_vco_core.cpp` | NEW | test | batch / property | `tests/test_vco_harness.cpp` | **exact** |
| `tests/test_vco_harness.cpp` | MODIFIED | test | batch / property | itself (cases 5, 6, 7 banners) | n/a |
| `tests/check_includes.sh` | MODIFIED | guard script | batch / static analysis | `tests/check_includes.sh [6/7]` + `tests/check_canary.sh [5b/5]` | **exact** |

---

## Pattern Assignments

### `src/dsp/VcoCore.hpp` (DSP core, transform) — MODIFIED IN PLACE

**Analog:** `src/dsp/LfoCore.hpp`

**Import pattern** — `src/dsp/LfoCore.hpp:23-34`. Note it includes `dsp/RackCompat.hpp` **explicitly** at
line 29 even though `dsp/DriftEngine.hpp` (line 34) would supply it transitively. This is the repo's
include-what-you-use practice and is the precedent for D-14's explicit include (which triggers the
`check_includes.sh [2/7]` patch below):

```cpp
#include <cmath>
#include <cstdint>
#include <algorithm>

#include "dsp/MathConst.hpp"   // forge::kPi (D-06, rack-free pi constant)

#include "dsp/RackCompat.hpp"    // forge::OnePole, forge::exp2_taylor5, forge::clamp
#include "dsp/Waveshape.hpp"     // forge::Waveshape::morphedWave
...
#include "dsp/DriftEngine.hpp"   // forge::DriftEngine
```

Current `VcoCore.hpp:48-50` has only `<cstdint>` + `dsp/DriftEngine.hpp`. Phase 30 adds
`dsp/RackCompat.hpp` and `dsp/Waveshape.hpp`, each with a trailing `// forge::symbol` comment matching the
LfoCore style. Both new basenames are already on `check_canary.sh [5b/5]`'s allow-list
(`check_canary.sh:470`) — no canary edit is needed.

**Member layout pattern** — `src/dsp/LfoCore.hpp:58-63`. `phase` is `double`, `Waveshape wave` and
`DriftEngine drift` are **instance members** (this is exactly what CORE-03 asserts):

```cpp
struct LfoCore {
	// --- orchestration state (mirrors the AnalogLFO members process() touches) ---
	double phase = 0.0;
	ClockTracker clock;
	DriftEngine drift;
	Waveshape wave;
```

`VcoCore.hpp:72-77` already has `DriftEngine drift;`. Add `double phase = 0.0;` and `Waveshape wave;`
alongside it. **Keep `struct VcoCore {` on one line** — `check_canary.sh [2b/5]` line-matches
`"struct VcoCore"*"{"*` to build a perturbed copy.

**THE D-11 PATTERN — `setSpreadSeed` coefficient copy, verbatim** — `src/dsp/LfoCore.hpp:102-112`.
This is the whole of D-11; copy it field-for-field:

```cpp
	void seed(uint64_t s0, uint64_t s1 = 0) { drift.seed(s0, s1); }
	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) {
		drift.setSpreadSeed(s0, s1);
		// Surface the spread coefficients into Waveshape (the shell wires these the
		// same way; the inline code shared them as instance members).
		wave.triAsymmetrySpread = drift.triAsymmetrySpread;
		wave.sawCurvatureSpread = drift.sawCurvatureSpread;
		wave.squareDutySpread   = drift.squareDutySpread;
		wave.pulseEdgeSpread    = drift.pulseEdgeSpread;
		wave.bleedSpread        = drift.bleedSpread;
	}
```

The five destination fields exist at `src/dsp/Waveshape.hpp:29-33`. **`characterSpread` is deliberately
NOT copied** — `LfoCore` does not copy it either (the LFO shell folds it into `in.character`), and copying
it would silently change what `character = 1.0` means. The current stub to replace is
`VcoCore.hpp:94-95` (`setSpreadSeed` is currently a one-line forward to `drift`).

**Constant pattern** — namespace-scope plain `constexpr`, per the `src/dsp/MathConst.hpp` idiom the
`VcoCore.hpp:26-28` banner already mandates. NOT `inline constexpr` (C++17), NOT in-class
`static constexpr` (C++11 declaration-only → MinGW undefined reference; the class of bug that got v2.0.0
rejected — `VcoCore.hpp:35-40`):

```cpp
namespace forge {
constexpr float kVcoFreqC4 = 261.6256f;         // C4 = 0 V (PITCH-01 reference)
constexpr float kVcoNyquistGuardFrac = 0.49f;   // PROVISIONAL — PITCH-04 (Phase 31) owns the real one
}
```

**Core step() pattern** — the verified body is RESEARCH § Code Example 1 (lines 694-715). Replace
`VcoCore.hpp:99-109`. **Keep `float step(const VcoInputs& in) {` on one line with its opening brace** —
`check_canary.sh [2b/5]` line-matches `*"float step(const VcoInputs& in)"*"{"*`. The existing telemetry
prologue (`VcoCore.hpp:100-102`) stays byte-identical; only the body after it changes.

**Banner-rewrite pattern:** `VcoCore.hpp:4-9` currently declares the seam silent by design and names
Phase 30 as the phase that lands DSP. Rewrite that paragraph; leave the R-9 ODR note (11-15), the sync
note (17-19) and the entire two-standard C++11 rule block (21-46) **untouched** — they are still binding.

---

### `src/AnalogVCO.cpp` (Rack shell, request-response) — NEW

**Analog:** `src/AnalogLFO.cpp` — **read-only. D-08 requires this file not appear in the Phase-30 diff.**
Extract the minimum viable shape only; everything below the "strip" list is Phase 31-35 work.

**Imports pattern** — `src/AnalogLFO.cpp:1-2` (strip the rest of its include block; the stub needs none of
`PatchParse`/`DisplayFill`/`Anim`/`<atomic>`/`<array>`/`<random>`):

```cpp
#include "plugin.hpp"
#include "dsp/LfoCore.hpp"   // forge::LfoCore — the extracted DSP core the shell delegates to
```

**Module skeleton pattern** — `src/AnalogLFO.cpp:13-43`. Four enums, `PARAMS_LEN`/`INPUTS_LEN`/
`OUTPUTS_LEN`/`LIGHTS_LEN` sentinels last, `LightId` present-but-empty:

```cpp
struct AnalogLFO : Module {
	enum ParamId {
		MORPH_PARAM,
		CHARACTER_PARAM,
		...
		PARAMS_LEN
	};
	enum InputId { ... INPUTS_LEN };
	enum OutputId {
		OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
```

D-07 caps the VCO stub at `MORPH_PARAM, CHARACTER_PARAM, PARAMS_LEN` / `VOCT_INPUT, INPUTS_LEN` /
`OUTPUT, OUTPUTS_LEN` / `LIGHTS_LEN`. **Do not** copy `RATIO_TABLE` (`AnalogLFO.cpp:49`) or any other
in-class `static constexpr` array — Pitfall 4.

**Config pattern** — `src/AnalogLFO.cpp:197-216`. `config(...)` first with all four sentinels, then
`configParam`, then `configInput`, then `configOutput`. The two knobs the VCO needs already exist with the
exact ranges D-07 wants (`AnalogLFO.cpp:199-200`) — copy these two lines verbatim:

```cpp
	AnalogLFO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MORPH_PARAM, 0.f, 1.f, 0.f, "Morph");
		configParam(CHARACTER_PARAM, 0.f, 1.f, 0.f, "Character");
		...
		configOutput(OUTPUT, "LFO");
	}
```

Add `core.seed(...)` / `core.setSpreadSeed(...)` in the ctor with **non-`(0,0)`** literals — the seeding
discipline is documented at `tests/VcoBlockDriver.hpp:20-26` and its four default values
(`0x1234, 0x5678, 0x9E3779B9, 0x7F4A7C15`) are the already-proven-non-degenerate set.

**Process pattern** — the shell marshals Rack indices into the POD and delegates; the core never sees an
index. Build the POD by **default construction + field assignment**, never a brace value-list
(`VcoCore.hpp:41-44` — NSDMIs make it a non-aggregate under C++11):

```cpp
	void process(const ProcessArgs& args) override {
		forge::VcoInputs in;
		in.pitchCV    = inputs[VOCT_INPUT].getVoltage();
		...
		outputs[OUTPUT].setVoltage(core.step(in));
	}
```

**Widget skeleton pattern** — `src/AnalogLFO.cpp:1131-1134` + `1153-1154` + `1177-1178` + `1193-1194`,
with the widget types swapped to stock Rack per D-08 and the hex bolts / display / context menu
(`1136-1151`, `1197-1209`) **stripped**:

```cpp
struct AnalogLFOWidget : ModuleWidget {
	AnalogLFOWidget(AnalogLFO* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/AnalogLFO.svg")));
		...
		addParam(createParamCentered<ForgeKnobHero>(mm2px(Vec(45.72f, 61.00f)),
		         module, AnalogLFO::MORPH_PARAM));
		...
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(7.70f, 119.50f)),
		         module, AnalogLFO::MORPH_CV_INPUT));
		...
		addOutput(createOutputCentered<ForgeJackOutput>(mm2px(Vec(83.74f, 119.50f)),
		          module, AnalogLFO::OUTPUT));
	}
};
```

Substitutions for Phase 30: `ForgeKnobHero`/`ForgeKnobSecondary` → `RoundBlackKnob`;
`ForgeJackInput`/`ForgeJackOutput` → `PJ301MPort`; `"res/AnalogLFO.svg"` → `"res/AnalogVCO.svg"`.
Never hardcode `box.size` — `setPanel` derives it from the SVG.

**Registration tail pattern** — `src/AnalogLFO.cpp:1212`, last line of the file, no trailing newline
ceremony, no namespace:

```cpp
Model* modelAnalogLFO = createModel<AnalogLFO, AnalogLFOWidget>("ForgeAnalogLFO");
```

Phase 30's line: `Model* modelAnalogVCO = createModel<AnalogVCO, AnalogVCOWidget>("ForgeAnalogVCO");`
(D-01 — the slug is a one-way door).

---

### `res/AnalogVCO.svg` (asset) — NEW

**Analog:** `res/AnalogLFO.svg:1` — mirror this header line byte-for-byte (the 128.5 mm → 379.4291 px
vs `RACK_GRID_HEIGHT = 380` discrepancy is shipped and live; do not "fix" it):

```svg
<svg xmlns="http://www.w3.org/2000/svg" width="91.44mm" height="128.5mm" viewBox="0 0 91.44 128.5" version="1.1">
```

Body is throwaway: 1-2 `<rect>` elements (RESEARCH Pattern 6). **No `<text>` element** — the SDK's
vendored nanosvg has no text parser and silently drops it. Do not copy the LFO's `<defs>`/gradient block
(`res/AnalogLFO.svg:2-5`) — that is Forge Noir design work owned by Phase 35.

---

### `src/plugin.hpp` / `src/plugin.cpp` / `plugin.json` (config, registration) — MODIFIED ADDITIVELY

**Analog:** each file's own existing LFO entry. Every edit is a single appended sibling; the LFO lines
stay byte-unchanged (D-05).

`src/plugin.hpp:6-7`:
```cpp
extern Plugin* pluginInstance;
extern Model* modelAnalogLFO;
```
→ append `extern Model* modelAnalogVCO;`. (Not an `#include`, so `check_includes.sh [1/7]`'s detector
does not match it — verified.)

`src/plugin.cpp:5-8`:
```cpp
void init(Plugin* p) {
	pluginInstance = p;
	p->addModel(modelAnalogLFO);
}
```
→ append `p->addModel(modelAnalogVCO);` (tab-indented, matching).

`plugin.json:16-24` — the `modules[]` element shape to mirror (2-space indent, key order
`slug, name, description, tags`):
```json
    {
      "slug": "ForgeAnalogLFO",
      "name": "Analog LFO",
      "description": "Sub-audio oscillator with waveform morphing, analog character, and drift modeling",
      "tags": [
        "Low-frequency oscillator",
        "Waveshaper"
      ]
    }
```
→ add a comma and a second element per D-01/D-02/D-03. `"version": "2.0.1"` (line 4) is **not** touched
(D-04).

---

### `tests/test_vco_core.cpp` (test, batch/property) — NEW

**Analog:** `tests/test_vco_harness.cpp` — copy its file-level idioms exactly.

**Banner pattern** — `tests/test_vco_harness.cpp:1-27`: file path, what the suite proves, a numbered
invariant list, an explicit "Deliberately NOT here" paragraph, and the "This TU does NOT define the
doctest impl macro (tests/main.cpp owns it)" line. The new file's banner must carry D-16's mandatory
**"NOT the TEST-02 tracking gate"** label in this block *and* in the pitch case name.

**Imports pattern** — `tests/test_vco_harness.cpp:29-35`:
```cpp
#include "doctest.h"

#include "VcoBlockDriver.hpp"

#include <vector>
#include <cmath>
#include <cstdint>
```

**Anonymous-namespace helper pattern** — `tests/test_vco_harness.cpp:37-56`. Both the `SAMPLE_RATES`
parametrization constant and the base-input builder live here. Note the builder is **field assignment,
never a brace value-list** (`VcoInputs` has NSDMIs → not a C++11 aggregate, P-8). The D-17 positive
control (`DeliberatelyBrokenSharedStateCore`) belongs in this same anonymous namespace:

```cpp
namespace {

// The three production sample rates every invariant is parametrized over.
constexpr double SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0};

// Baseline harness input. Built by default construction + field assignment,
// never a brace value-list (VcoInputs has NSDMIs, so it is not a C++11
// aggregate — P-8).
forge::VcoInputs harnessBase() {
	forge::VcoInputs in;
	in.pitchCV   = 0.f;
	...
	return in;
}

} // namespace
```

**Case pattern (parametrized + CAPTURE + REQUIRE-then-CHECK)** — `tests/test_vco_harness.cpp:63-72`.
`REQUIRE` for the structural precondition (a wrong-sized block makes every later assertion meaningless),
`CHECK` for the actual invariant so all sample rates report:

```cpp
TEST_CASE("vco harness: drives VcoCore over blocks at 44.1 / 48 / 96 kHz Rack-free") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const int n = 1024;
		forge::VcoBlockDriver d(sr);
		auto out = d.run(n, forge::VcoBlockDriver::sweepScenario(n, harnessBase()));
		REQUIRE(out.size() == (size_t)n);
		CHECK(d.core.tel.stepCount == (uint32_t)n);
	}
}
```

Case names use the `"<suite>: <what>"` convention so `-tc="vco core*"` filters cleanly, mirroring
`-tc="vco harness*"`.

**Bit-exact comparison pattern** — `tests/test_vco_harness.cpp:158-165`. This is the comment to carry
into the D-17 independence case and the D-18a divergence case; **never** `doctest::Approx`:

```cpp
		// Bit-exact comparison via a direct float == (NOT doctest::Approx, whose
		// epsilon(0) still applies a relative-scaling margin and is not a true
		// bit-exact comparator).
		bool identical = true;
		for (size_t i = 0; i < oa.size(); ++i) {
			if (oa[i] != ob[i]) { identical = false; break; }
		}
		CHECK(identical);
```

**Explicit-seed driver-pair pattern** — `tests/test_vco_harness.cpp:152-153`. Two drivers constructed
with all four seeds spelled out is the shape D-17/D-18a reuse (varying only the two spread seeds):

```cpp
		forge::VcoBlockDriver a(sr, 0xC0FFEEULL, 0xBADF00DULL, 0x9E3779B9ULL, 0x7F4A7C15ULL);
		forge::VcoBlockDriver b(sr, 0xC0FFEEULL, 0xBADF00DULL, 0x9E3779B9ULL, 0x7F4A7C15ULL);
```

**Non-vacuity-precondition pattern** — `tests/test_vco_harness.cpp:83-90` and `101-108` feed a
deliberately bogus value (`in.sampleTime = 999.f`, `in.sampleRate = -1.f`) so the assertion fails the
moment the mechanism under test is removed. The D-17 "the two solos are distinguishable" precondition and
the D-18a divergence threshold are the same idea; state the measured margin in the comment the way
`tests/test_vco_harness.cpp:112-117` states the hang-guard rationale.

**Direct-core-access pattern** — `d.core.tel.stepCount` (`:70`), `d.core.drift.rng.state[0]` (`:122`).
`VcoBlockDriver::core` is a public member, so the D-17 interleave loop can call `ia.core.step(a)`
directly without adding anything to the harness.

---

### `tests/test_vco_harness.cpp` (test) — MODIFIED

Three surgical edits, no structural change:

1. **Case 7 inversion (D-15)** — `:189-207`. Same slot, opposite assertion. Rewrite the banner
   (`:190-195`, currently "Phase 30 is REQUIRED to DELETE this test case") to say the core is now real and
   that reverting it to a stub must fail loudly, rename the case off the word TOMBSTONE per D-15's
   framing, and flip `allSilent` to a not-all-zero **and** not-constant check.
2. **Case 5 banner (D-19)** — `:139-145`, the "WEAK BY CONSTRUCTION TODAY … revisit it in Phase 30"
   paragraph. Replace with a statement that it is now load-bearing under real DSP over the varying sweep.
   The case body needs no change (verified green on the prototype).
3. **Case 6 banner (D-19)** — `:170-173`, same treatment.
4. **File banner** — `:8-25`, the invariant list line 15 and the "Known coverage caveat (P-7)" paragraph
   (`:17-22`) both name the weakness; update both, and point the "Deliberately NOT here" paragraph
   (`:24-25`) at the new `tests/test_vco_core.cpp` for pitch and bounds.

---

### `tests/check_includes.sh` (guard script, static analysis) — MODIFIED

**Section to patch:** `[2/7]`, `tests/check_includes.sh:281-295`. The detector at line 287 is:

```bash
		rack_hits="$(grep -nE '^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"][^">]*[Rr]ack[^">]*[>"]' "${h}" || true)"
```

**Precedent to mirror for the exemption** — `tests/check_canary.sh:464-472`, which already allow-lists
`RackCompat.hpp` by exact basename with the reason written in place. Copy its posture: a `case` over
exact names, a comment explaining *why each name* is allowed, and an explicit "anything else still
fails":

```bash
		case "${b}" in
			# The VCO's own headers.
			Vco*|MorphBlep.hpp) ;;
			# The D-05 frozen shared headers the VCO is allowed to consume. This
			# list is the four D-05 names; anything else under src/dsp/ is LFO
			# internals and the VCO must not couple to it.
			DriftEngine.hpp|MathConst.hpp|RackCompat.hpp|Waveshape.hpp) ;;
			*) seam_bad="${seam_bad} ${inc}" ;;
		esac
```

The exact `[2/7]` patch (exact-path `grep -v` filter + rationale comment) is RESEARCH § Guard Script
Impact, Option A. Because it weakens a standing guard it goes on the **same operator surface as the D-05
registration diff**.

**Negative-control idiom for the companion control** — `tests/check_includes.sh:424-446`. Copy this
shape exactly: a heredoc-written synthetic fixture that is deliberately bad, the **same detector function
the real section runs** applied to it, and a `note_fail` whose message states that the real section's
PASS is meaningless if the control does not fire:

```bash
echo "[6/7] NEGATIVE CONTROL — the audit must detect a synthetic violation..."
cat > "${TMP}/nc_lfo_leak.cpp" <<'EOF'
// Synthetic fixture: an LFO-side translation unit that pulls in VCO code.
// This is exactly the breach [1/7] exists to prevent.
#include "dsp/LfoCore.hpp"
#include "dsp/VcoCore.hpp"
float ncLeakProbe() { return 0.f; }
EOF
nc_hits="$(detect_vco_includes "${TMP}/nc_lfo_leak.cpp")"
if [[ -n "${nc_hits}" ]]; then
	echo "  OK: direct (one-hop) violation detected by the same detector [1/7] uses:"
	printf '%s\n' "${nc_hits}" | sed 's/^/    /'
else
	note_fail "negative control DID NOT FIRE: detect_vco_includes reported a fixture that plainly includes dsp/VcoCore.hpp as clean. The detector is broken, so [1/7]'s PASS above is meaningless."
fi
```

Phase 30's control: a synthetic `dsp/VcoNegControl.hpp`-shaped fixture in `${TMP}` carrying
`#include <rack.hpp>`, run through the **patched** `[2/7]` grep pipeline, required to produce a hit. The
"validity check" idiom at `:471-474` (assert the fixture is actually testing what it claims before
asserting it fires) is worth mirroring: assert the fixture would be *missed* by a naive detector, or at
minimum that a `dsp/RackCompat.hpp`-only fixture is *not* flagged, so both directions of the exemption are
proven.

**Closing-note idiom** — `:482-483`:
```bash
echo "  NOTE: these negative controls are what make this audit VALIDATED rather than"
echo "        merely green. They run on every invocation. Do not remove them."
```

**No edit needed** to `VCO_SIDE_ALLOW` — `src/AnalogVCO.cpp` was pre-registered in Phase 29 at
`tests/check_includes.sh:220`.

---

## Shared Patterns

### C++11 two-standard discipline
**Source:** `src/dsp/VcoCore.hpp:21-44` (the binding banner)
**Apply to:** `src/dsp/VcoCore.hpp`, `src/AnalogVCO.cpp` (both are in the `make strict`
`-std=c++11 -pedantic-errors` set; the tests are not).
No `inline constexpr` variables, no `if constexpr`, no `std::clamp` (use `forge::clamp`,
`src/dsp/RackCompat.hpp:97`), no in-class `static constexpr` indexed at runtime, no brace value-list init
of `VcoInputs`.

### POD-boundary construction
**Source:** `tests/VcoBlockDriver.hpp:78-86`
**Apply to:** every site that builds a `forge::VcoInputs` — `src/AnalogVCO.cpp::process`,
`tests/test_vco_core.cpp` helpers, `tests/test_vco_harness.cpp::harnessBase`
```cpp
			// Copy + assign, never a brace value-list: VcoInputs has NSDMIs, so
			// under C++11 it is not an aggregate and a value-list init is a hard
			// error (P-8). Kept C++11-shaped even though tests build at C++17.
			forge::VcoInputs in = base;
```

### Non-degenerate seeding
**Source:** `tests/VcoBlockDriver.hpp:20-26` + `:41-47`
**Apply to:** `src/AnalogVCO.cpp` ctor, every new test driver
```cpp
	explicit VcoBlockDriver(double sr = 44100.0,
	                        uint64_t s0 = 0x1234ULL, uint64_t s1 = 0x5678ULL,
	                        uint64_t sp0 = 0x9E3779B9ULL, uint64_t sp1 = 0x7F4A7C15ULL)
		: sampleRate(sr) {
		core.seed(s0, s1);
		core.setSpreadSeed(sp0, sp1);
	}
```
`(0,0)` is a Xoroshiro fixed point → `std::normal_distribution` never terminates → **Rack hangs on patch
load** (`tests/test_vco_harness.cpp:112-116`).

### Comment-the-load-bearing-line
**Source:** `tests/VcoBlockDriver.hpp:49-52`, `:9-18`; `tests/check_includes.sh:184-215`
**Apply to:** every file in this phase.
House style is that a constraint which a future refactor could plausibly undo carries an in-place comment
naming the consequence ("Do not template them. Do not subclass. Do not alias."). Phase 30's instances:
the one-line `struct VcoCore {` / `step()` signature shapes (`check_canary.sh [2b/5]`), the provisional
`kVcoNyquistGuardFrac`, and the `check_includes.sh [2/7]` exemption's scope.

### Frozen-header discipline
**Source:** `check_canary.sh:464-472` allow-list; `src/dsp/Waveshape.hpp:158` signature
**Apply to:** `src/dsp/VcoCore.hpp`
`Waveshape`, `DriftEngine`, `RackCompat`, `MathConst` are **called, never edited**
(`check_frozen.sh [1/3]` pins their bytes). The Phase-30 call site is
`wave.morphedWave(phase, morph, character, 0.f)` — signature at `src/dsp/Waveshape.hpp:158`
(`float morphedWave(float phase, float morph, float character, float bleedLfo) const`).

---

## No Analog Found

None. Every file in this phase mirrors an existing, shipped counterpart. The two places where the analog
is deliberately **not** followed are decisions, not gaps:

| File | Divergence from analog | Reason |
|------|------------------------|--------|
| `src/AnalogVCO.cpp` | Stock `RoundBlackKnob`/`PJ301MPort` instead of the `ForgeKnob*`/`ForgeJack*` structs at `src/AnalogLFO.cpp:1125-1194` | D-08 — the Forge components are local structs inside the shipped LFO TU; reusing them would put `src/AnalogLFO.cpp` in the diff |
| `res/AnalogVCO.svg` | 2 rects instead of the LFO's 172-shape Forge Noir art | D-06 — throwaway; Phase 35 is an art swap at the same filename and HP |

---

## Metadata

**Analog search scope:** `src/`, `src/dsp/`, `tests/`, `res/`, repo root (`plugin.json`)
**Files read:** `src/dsp/VcoCore.hpp`, `src/dsp/LfoCore.hpp` (20-149), `src/AnalogLFO.cpp` (1-50,
1125-1212 + symbol index), `src/plugin.cpp`, `src/plugin.hpp`, `plugin.json`,
`tests/test_vco_harness.cpp`, `tests/VcoBlockDriver.hpp`, `tests/check_includes.sh` (168-308, 424-485),
`tests/check_canary.sh` (430-484), `res/AnalogLFO.svg` (header), `src/dsp/Waveshape.hpp` (symbol index)
**Pattern extraction date:** 2026-07-28
