# Phase 33: Hard Sync — Pattern Map

**Mapped:** 2026-08-28
**Files analyzed:** 13 (11 modified, 2 created)
**Analogs found:** 12 / 13 (one created file, `tools/render_sync_ab.cpp`, has a *structural* analog only — see §"No Analog Found")

> **Read this first.** Every excerpt below is quoted from the file named at the top of the block, at
> the line range given. Nothing here is invented shape. Where a pattern is *forbidden*, the excerpt is
> included so the planner can see what is being rejected rather than inferring it.

---

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/dsp/MorphBlep.hpp` (Task 1 guards; optional `addPastStep`) | DSP utility (header-only) | transform | itself — `MorphBlep::step`'s `dt` guard `:282-306`, `addStep`'s gate `:253-262` | exact (self-analog; the guard idiom already exists two lines away) |
| `src/dsp/VcoCore.hpp` (two POD fields, sync state, reset, seam call) | DSP core (header-only) | event-driven → transform | `src/dsp/LfoCore.hpp:134-145` (reset trigger + `resetConnected` gate) **for the detection half**; `VcoCore.hpp:536-542` + `:597-602` **for the guard/reset half** | exact (role) / role-match (LFO's *action* is the rejected crossfade) |
| `src/AnalogVCO.cpp` (SYNC jack + 2 POD assignments) | Rack shell (module) | request-response (per-sample marshalling) | `src/AnalogVCO.cpp:102-107, :186-188, :316-317, :401-406` — the FM jack, wired end to end | exact |
| `src/vco_compile_canary.cpp` (2 runtime-derived feeds) | build-gate TU | transform | `src/vco_compile_canary.cpp:104-112` — the nine existing feeds | exact |
| `res/AnalogVCO.svg` (one jack rect) | config/asset | n/a | `res/AnalogVCO.svg:4-13` — the eleven existing `<rect …10×10 fill="#2a2a30"/>` jack/knob placeholders | exact |
| `tests/test_morph_blep.cpp` (Task 1 RED + permanent assertions) | test | transform | `tests/test_morph_blep.cpp:1036-1055` (subcase B, the entry-gate rejection) and `:1057-1076` (subcase C, the poisoned-instance `dt` story) | exact |
| `tests/test_vco_core.cpp` (SC-3 delta bound, D-09 ceiling, sync rows, re-derived tiers) | test | event-driven | `tests/test_vco_core.cpp:1153-1239` (scenario four's hostile grid), `:818-989` (the two tiers), `:1529-1560` (the interleave invariant) | exact |
| `tests/test_vco_spectrum.cpp` (D-06 legs + D-11 sync sub-grid) | test | batch/transform | `tests/test_vco_spectrum.cpp:611-622` (`SpectrumCell`), `:937-949` (`driveSecondBlock`), `:1001-1055` (`measureCellDb`), `:186-223` (`aliasPeakDb`) | exact |
| `tests/test_vco_pitch.cpp` (PITCH-04 third input class) | test | event-driven | `tests/test_vco_pitch.cpp:2138` (the PITCH-04 Nyquist case) and `:2448` (the hostile-input case) | exact |
| `tests/check_includes.sh` (`VCO_SIDE_ALLOW`) | config/guard | batch | `tests/check_includes.sh:355-364` — the existing eight-entry array | exact |
| `Makefile` (skip filter + render target) | config/build | batch | `Makefile:56-64` — the `capture` target, and `:24` the filter | exact |
| `tools/render_sync_ab.cpp` **(new)** | tool TU (standalone `main`) | file-I/O | `tools/capture_golden.cpp` — the *only* non-test standalone TU that links the Rack-free core | role-match (structure exact; it links `LfoCore`, not `VcoCore`, and writes `.f32`, not `.wav`) |
| *(optional)* new sync test TU | test | batch | any of `tests/test_vco_*.cpp` | exact — but see §"Shared Patterns → New-TU cost" |

**Must remain absent from the diff:** `src/AnalogLFO.cpp`.
**FROZEN — call, never edit:** `src/dsp/RackCompat.hpp`, `src/dsp/Waveshape.hpp`, `src/dsp/DriftEngine.hpp`, `src/dsp/MathConst.hpp`.

---

## Pattern Assignments

### `src/dsp/MorphBlep.hpp` (DSP utility, transform) — Task 1's three guards + the optional seam

**Analog:** itself. Every guard Task 1 adds has a sibling within 60 lines, in the exact idiom required.

**The negated-guard-with-early-return pattern** — copy this shape verbatim (`MorphBlep.hpp:282-306`):

```cpp
	// D-15 / P-14: MorphBlep REFUSES TO RELY ON ITS CALLER. forge::VcoCore
	// already guards its own deltaPhase, but this header carries its own guard so
	// that any caller — the headless harness, a future polyphonic shell, Phase 33
	// — cannot reach the divisor below with hostile timing. Written NEGATED so a
	// not-a-number `dt` lands here too: `fdt > 0.f` is false for a NaN, zero,
	// a negative and a subnormal that flushed to zero alike. NEVER a
	// comparison-ladder helper (forge::clamp), which is inert against a NaN.
	// Draining the accumulator BEFORE this guard is deliberate: an already-owed
	// residual is still owed even on a sample the caller mistimed.
	const float fdt = (float)dt;
	if (!(fdt > 0.f) || !(fdt <= 1.f)) return now;
```

Two structural properties to carry into the new guards:
1. **Drain first, guard second.** `now = inject + pending; inject = 0.f; pending = 0.f;` happens at `:278-280`, *before* the `dt` guard returns. A guard that returns before the drain would strand an owed residual. Any new early-return Task 1 adds must sit **after** the drain for the same reason.
2. **The comment carries the measurement.** `:292-304` records the exact failure (`+infinity dt` → `pending = nan` → instance poisoned forever) and which test found it. Task 1's three guards each need the same treatment: the RED that found it, named in the comment.

**The `addStep` entry gate — the pattern the `jump` guard extends** (`MorphBlep.hpp:253-262`):

```cpp
	// The entry gate is written with the NEGATED comparison FIRST so a
	// not-a-number `xAhead` is REJECTED rather than accumulated: `xAhead >= 0.f`
	// is false for a NaN, so the negation fires and the function returns before
	// touching per-instance state that would then poison every following sample.
	void addStep(float xAhead, float jump) {
		if (!(xAhead >= 0.f) || xAhead > 1.f) return;
		const float u = 1.f - xAhead;
		inject  += jump * ( 0.5f) * u * u;
		pending += jump * (-0.5f) * xAhead * xAhead;
	}
```

⚠️ **`jump` is unguarded here.** D-04's third item closes it. The guard belongs on the same line as the existing gate (one combined early return), because the reason is identical: reject before touching `inject`/`pending`. There is no `std::isfinite` in this header today and adding `<cmath>` is not required — a negated pair on `jump` (e.g. `!(jump > -kBig) || !(jump < kBig)`) or an explicit `jump == jump` NaN test plus an infinity bound both stay closed-form; **the planner owns which**, but it must be written negated and must not be `forge::clamp`.

**The CR-01 site — the one-sided clamp and the `float[5]` write** (`MorphBlep.hpp:317-334`):

```cpp
	const float c = (character < 0.001f) ? 0.f : character * character;
	const float scaled = morph * 4.f;
	int segment = (int)scaled;
	if (segment > 3) segment = 3;                    // mirrors the frozen minimum-of-3 (:166)
	const float frac = scaled - (float)segment;
	...
	float W[5] = { 0.f, 0.f, 0.f, 0.f, 0.f };
	if (segment == 3) {
		W[4] = 1.f;
	} else {
		W[segment]     += 1.f - frac;
		W[segment + 1] += frac;
	}
```

`segment` is clamped only from **above**. A negative `morph` gives a negative `segment` and `W[segment]` underflows the array; `(int)NaN` is `0` on this arm64 host but `INT_MIN` under x86 `cvttss2si`. The fix is a lower clamp *plus* the CR-02 entry guard on `morph`/`character` — and the entry guard must use the negated idiom, not the ternary shape at `:317`.

**The `addPastStep` seam (D-06 candidate (b), if the plan takes the named entry point).** Place it directly beneath `addStep` and mirror its gate exactly. RESEARCH §"The additive past-edge entry point" gives the body; the *banner* obligation is what the analog sets: `:238-256` spends 19 comment lines on a 6-line function stating the boundary and the reason for the gate ordering. A bare `addPastStep` with no forfeited-half paragraph would be out of character for this file and would invite the "simplification" back to candidate (a) that RESEARCH warns about.

**Source-shape note:** `MorphBlep::step` is declared in-struct at `:268` and defined out of line at `:273` as `inline`. If Task 1 or D-06 adds a function long enough to be a wall of arithmetic, follow that split.

---

### `src/dsp/VcoCore.hpp` (DSP core, event-driven → transform)

**Analog for the POD fields:** `VcoCore.hpp:232-244`.

```cpp
struct VcoInputs {
	float pitchCV = 0.f;        // V/OCT input volts (Phase 30/31)
	...
	float fmVolts = 0.f;        // exponential FM input volts, summed into the pitch volt domain before the single exp2 (Phase 31)
	float fmAtten = 0.f;        // bipolar FM attenuverter (Phase 31)
	bool  fmConnected = false;  // FM jack patched
	...
	float sampleTime = 1.f / 44100.f;  // INJECTED by the harness/shell, never read from a global
	float sampleRate = 44100.f;        // INJECTED; the Nyquist clamp (PITCH-04, Phase 31) needs it
};
```

Copy: NSDMI defaults, one trailing `//` comment per field naming the owning phase, `fmVolts`/`fmConnected` as the exact naming precedent for `syncVolts`/`syncConnected`.
⚠️ **`VcoCore.hpp:60-62` currently says the sync fields are "deliberately ABSENT" and names Phase 33 as the phase that adds them.** That banner paragraph must be updated in the same commit, or the header contradicts itself.

**Analog for the per-instance state block:** `VcoCore.hpp:253-278`.

```cpp
	// Per-instance oscillator state, mirroring src/dsp/LfoCore.hpp:58-63. ALL
	// THREE members — the double phase accumulator, the Waveshape and now the
	// MorphBlep — are INSTANCE state, not static and not shared. That is
	// literally what CORE-03 asserts and what D-14 binds by name: two cores
	// stepped interleaved must not see each other.
	...
	double phase = 0.0;
	Waveshape wave;
	MorphBlep blep;
```

The new `SchmittTrigger syncTrig;` and `float prevSyncVolts = 0.f;` go **here**, by value, beside `blep`, and the banner's "ALL THREE members" sentence extends to five. The banner also states the CORE-03 proof is behavioral (`:268-275`) — the interleave case at `tests/test_vco_core.cpp:1529` is the thing that must be extended to drive sync inside its window.

**Analog for the telemetry field — already reserved** (`VcoCore.hpp:285`):

```cpp
		bool  syncFired = false;   // a hard-sync reset fired this sample (Phase 33)
```

Populate it. Do **not** add a parallel field. Note RESEARCH's §"Don't Hand-Roll" proposes one *additional* recording-only telemetry float (the deposited sync correction) so D-14's withheld leg is reconstructible from the same pass — that is additive to this same `Telemetry` struct, which the banner at `:280-281` already declares is "NOT part of the audio path".

**Analog for the reset site — the advance/wrap the overwrite lands after** (`VcoCore.hpp:536-542`):

```cpp
		double deltaPhase = (double)freq * (double)in.sampleTime;
		if (!(deltaPhase > 0.0)) deltaPhase = 0.0;
		if (deltaPhase > kVcoMaxDeltaPhase) deltaPhase = kVcoMaxDeltaPhase;
		phase += deltaPhase;
		if (phase >= 1.0) phase -= 1.0;

		const float p = (float)phase;
```

⚠️ **`deltaPhase` is guarded; `phase` is not.** The sync block goes strictly between `:540` and `:542`, so `p`, `naive` and `blep.step` all see the post-reset phase (D-07). The `p` snapshot at `:542` must not be hoisted above the sync block, and `blep.step` at `:645` must stay the single call it is.

**Analog for the guard idiom — the worked example D-04 and the `f` guard both mirror** (`VcoCore.hpp:597-602`):

```cpp
		float morph = in.morph;
		if (!(morph > 0.f)) morph = 0.f;
		if (morph > 1.f) morph = 1.f;
		float character = in.character;
		if (!(character > 0.f)) character = 0.f;
		if (character > 1.f) character = 1.f;
```

Negated comparison **first** (the NaN catcher), plain comparison second. The comment block above it (`:544-596`, 53 lines) is the depth this file documents a two-line guard at: what the guard protects, why it is here and not in the frozen header, why the core guards even though the shell does, the bit-identity argument, and the measured evidence of it firing. The `f` guard is a *new divisor* and warrants at least that much.
⚠️ `morph`/`character` are conditioned at `:597-602`, **after** the wrap at `:540`. D-05's two `morphedWave` calls need conditioned values, so the plan owns moving the conditioning block above the sync block (see RESEARCH's parenthetical note under §"Code Examples").

**Anti-pattern excerpt — what SYNC-02 rejects** (`src/dsp/LfoCore.hpp:134-145`):

```cpp
	// processResetInput (AnalogLFO.cpp:546-565): blanking advances every sample.
	bool blanking = resetBlanking.process(sampleTime);
	if (in.resetConnected) {
		if (resetTrigger.process(in.resetVoltage, 0.1f, 1.0f)) {
			if (!blanking) {
				crossfadeFrom = lastOutputVoltage;   // <-- the 3 ms cosine crossfade
				crossfadeProgress = 0.f;             //     SYNC-02 forbids by name
				phase = 0.0;                         // <-- and the snap-to-zero
				resetBlanking.trigger(0.001f);       //     STACK.md:149 forbids
			}
		}
	}
```

**Copy** the `in.resetConnected` outer gate and the `0.1f, 1.0f` literals. **Reject** the crossfade body and the `phase = 0.0` snap. This one excerpt contains both the precedent and the anti-pattern, which is why CONTEXT.md tells the planner to read it.

**Threshold-convention corroboration** (`src/dsp/ClockTracker.hpp:109-112`):

```cpp
		// Edge detection (AnalogLFO.cpp:446-447)
		float clkVoltage = clkV;
		if (clockTrigger.process(clkVoltage, 0.1f, 1.0f)) {
			r.edgeFired = true;
```

Two in-house sites, same literals. D-03 inherits; nothing to invent.

**The primitive being called (FROZEN — read only)** (`src/dsp/RackCompat.hpp:44-58`):

```cpp
// SchmittTrigger — float specialization (digital.hpp:82-161).
// UNINITIALIZED handling is load-bearing.
struct SchmittTrigger {
	enum State : uint8_t { LOW, HIGH, UNINITIALIZED };
	State s = UNINITIALIZED;
	void reset() { s = UNINITIALIZED; }
	bool process(float in, float lowThreshold = 0.f, float highThreshold = 1.f) {
		if (s == LOW && in >= highThreshold)        { s = HIGH; return true; }
		else if (s == HIGH && in <= lowThreshold)   { s = LOW; }
		else if (s == UNINITIALIZED && in >= highThreshold) { s = HIGH; }
		else if (s == UNINITIALIZED && in <= lowThreshold)  { s = LOW; }
		return false;
	}
	bool isHigh() { return s == HIGH; }
};
```

This is the source for two of RESEARCH's findings and both are checkable right here: `process` returns `true` only from the `LOW → HIGH` arm (so `prev < 1.0 <= now` and the divisor is non-zero **in steady state**, Pitfall 5), and all three `in`-comparisons are false for a NaN (so a NaN cannot fire but *is* stored, Pitfall 6). Also note `reset()` exists — it is the mechanism available for the sample-rate-change discretion item.

**The NaN-transparent helper that must NOT be used** (`RackCompat.hpp:97`):

```cpp
inline float clamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
```

Both comparisons false for NaN → returns the NaN unchanged. This is why the negated pair exists.

---

### `src/AnalogVCO.cpp` (Rack shell, request-response)

**Analog:** the FM jack, which is wired through all four required places. Copy each.

1. **Enum** (`:102-107`):
```cpp
	enum InputId {
		VOCT_INPUT,
		FM_INPUT,
		MORPH_CV_INPUT,
		INPUTS_LEN
	};
```
⚠️ Append `SYNC_INPUT` **before** `INPUTS_LEN`. Appending rather than inserting keeps existing indices stable (no patch-compat concern yet, but it is the file's convention).

2. **`configInput`** (`:186-188`):
```cpp
		configInput(VOCT_INPUT, "V/Oct");
		configInput(FM_INPUT, "FM");
		configInput(MORPH_CV_INPUT, "Morph CV");
```

3. **The POD assignment — two lines, no arithmetic** (`:316-317`):
```cpp
		in.fmVolts = inputs[FM_INPUT].getVoltage();
		in.fmConnected = inputs[FM_INPUT].isConnected();
```
The banner at `:302-312` states *why* both fields cross unconditionally and names the shipped LFO's conditional as the **anti-pattern**. `syncVolts`/`syncConnected` follow this exactly: unconditional, no `if`, no zeroing.

4. **The widget** (`:401-406`):
```cpp
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(45.72f, 100.f)),
		         module, AnalogVCO::FM_INPUT));
```

5. ⚠️ **The field-count sentence at `:320-331`** ("Exactly ONE forge::VcoInputs field is still left at its header default: drift" / "SEVEN of the eight VcoInputs DSP fields") is a hand-maintained count that this phase moves. Update it in the same commit or the file misstates its own coverage.

---

### `src/vco_compile_canary.cpp` (build-gate TU, transform)

**Analog:** `:104-112`, the nine existing runtime-derived feeds.

```cpp
	in.pitchCV     = (float)(i & 7) - 4.f;
	in.coarse      = (float)((i >> 3) & 3);
	in.fine        = (float)((i >> 5) & 3);
	in.fmVolts     = (float)((i >> 7) & 7) * 0.25f;
	in.fmAtten     = (float)((i >> 10) & 3) * 0.5f - 1.f;
	in.fmConnected = ((i >> 12) & 1) != 0;
	in.morph       = (float)(i & 15) / 15.f;
	in.character   = (float)((i >> 4) & 15) / 15.f;
	in.drift       = (float)((i >> 8) & 15) / 15.f;
```

`in.fmConnected` at `:109` is the **exact** shape for `in.syncConnected` — a bit test, not a literal. Add two non-overlapping bit slices (RESEARCH suggests `(i>>13)&7` and `(i>>16)&1`; verify no collision with the slices above, which currently occupy bits 0-15 with deliberate overlap between the two naming groups).
The banner at `:89-103` is the rationale block — its "LOAD-BEARING — DO NOT REPLACE THESE WITH LITERALS" paragraph plus the measured `kTable` anecdote. The new feeds need one added sentence: a constant-`false` `syncConnected` would fold the entire sync branch away at `-O3` while `[2b/5]` still reported PASS on field presence.
Also note `:85` — `VcoInputs in;` then per-field assignment. **Never** a brace value-list (C++11 NSDMI/non-aggregate rule, P-8). Same idiom at `tests/VcoBlockDriver.hpp:78-81` and `tests/test_vco_spectrum.cpp:1025-1034`.

---

### `res/AnalogVCO.svg` (config/asset)

**Analog:** the file is 14 lines of flat `<rect>`s with no groups, ids or classes (`:4-13`):

```xml
  <rect x="25.48" y="95" width="10" height="10" fill="#2a2a30"/>
  <rect x="55.96" y="95" width="10" height="10" fill="#2a2a30"/>
  <rect x="40.72" y="95" width="10" height="10" fill="#2a2a30"/>
```

One more `10×10 fill="#2a2a30"` rect at a free position. The `y="95"` row is the jack row (`VOCT` at 30.48, `FM` at 45.72 in mm2px centres → the SVG rects at x = 25.48 / 40.72 are those two, less the 5 mm half-width). Panel width is 91.44 mm. **Do not** apply Forge Noir design language here — CONTEXT.md D-18 says this SVG is a throwaway replaced wholesale in Phase 35; physical form and placement are Phase 35's call.

---

### `tests/test_morph_blep.cpp` (test, transform) — Task 1's RED + permanent assertions

**Analog A — the entry-gate rejection case** (`:1036-1055`), the exact template for the `jump` guard's permanent assertion:

```cpp
	SUBCASE("B: the seam's entry gate rejects a bad event without touching state") {
		// The gate is written with the NEGATED comparison FIRST so a not-a-number
		// `xAhead` is REJECTED rather than accumulated. That ordering is the whole
		// point: a not-a-number that reached per-instance state would poison every
		// following sample, because `pending` carries it forward forever ...
		const float bad[3] = {
			-0.1f, 1.1f, std::numeric_limits<float>::quiet_NaN()
		};
		for (int i = 0; i < 3; ++i) {
			CAPTURE(bad[i]);
			forge::MorphBlep b;
			b.addStep(bad[i], 2.f);
			CAPTURE(b.inject);
			CAPTURE(b.pending);
			CHECK(b.inject  == 0.0f);
			CHECK(b.pending == 0.0f);
		}
	}
```

The `jump` version is this case with the hostile array moved to the second argument and `{+inf, -inf, NaN}` as the population. **Exact-equality `== 0.0f` on the accumulators** is the assertion shape — not a tolerance.

**Analog B — the pinned seam equivalence** (`:984-1008`), which the planner must not disturb and which the D-06 seam must remain consistent with:

```cpp
		{
			const double dt = 0.02;
			const double targetS = 0.5;

			forge::MorphBlep site;
			double phase = 1.0 - targetS * dt - dt;
			const float emitted = driveOneSite(site, wv, phase, dt, 0.50f, 0.f);

			forge::MorphBlep seam;
			seam.addStep((float)targetS, 2.f);
			...
			// MEASURED: both pairs are (+0.250000, -0.250000).
			CHECK(std::fabs((double)emitted - (double)seam.inject) < 1e-5);
			CHECK(std::fabs((double)site.pending - (double)seam.pending) < 1e-5);
		}
```

If D-06 lands `addPastStep`, its own equivalence assertion belongs here in the same shape: `addPastStep(f, h)` and `addStep(0.f, -f*f*h)` must produce identical `(inject, pending)` pairs — that identity is the header change's own non-circular check.

**Analog C — the poisoned-instance narrative** (`:1057-1076` and the `+infinity` paragraph at `:1076`). This subcase is the model for the `jump` guard's RED: drive the hostile value, *withdraw it*, and show the instance still returning NaN. The banner at `MorphBlep.hpp:292-304` records that exact story for `dt`; the `jump` version needs the same pair (a test that found it, a header comment that records it).

**Also note** `:956` — the TEST_CASE title names the decisions it discharges (`(D-14 / D-15)`). Every case in this file does. Task 1's cases should name `(D-04 / CR-01)`, `(D-04 / CR-02)`, `(D-04 third item)`.

**Sanitizer constraint:** the ASan RED for CR-01 is a **scoped one-shot probe**, run to produce evidence, never wired into `make test`, `make guards` or CI (register item 12). There is no ASan target in the Makefile today and one must not be added.

---

### `tests/test_vco_core.cpp` (test, event-driven) — SC-3, D-09, the sync rows, the tiers

**Analog A — the hostile grid the sync voltages join** (`:1153-1165, :1176-1239`):

```cpp
		static const float HOSTILE_TIMES[] = {
			-1.f / 44100.f,                                    // the negative of the one legitimate value
			0.f,                                               // the increment-zeroing case
			1.f / 44100.f,                                     // paired with 44100: the one legitimate CONTROL point
			...
			std::numeric_limits<float>::quiet_NaN(),           // a mis-wired host or an uninitialised ProcessArgs
			std::numeric_limits<float>::infinity(),            // the same, non-finite
			...
		};
```

Every entry carries a trailing comment naming the *physical* case it models. A `HOSTILE_SYNC[]` array follows that discipline exactly: a steady 5 V (the stale-store `0/0` case), a NaN cable voltage, an exact `1.0 V` sample (the `f == 1` snap-to-zero landmine), ±inf, and one legitimate control edge.

**The accumulate-then-assert idiom** — mandatory at this grid size (`:1190-1230`):

```cpp
					// ACCUMULATED, not asserted per sample: 176 configs at
					// 20000 steps would otherwise add roughly twenty-one million
					// assertions to a suite already past 2.6 million.
					bool  allFinite       = true;
					...
					for (int i = 0; i < nHostile; ++i) {
						const float s = core.step(in);
						const float a = std::fabs(s);
						if (a > maxAbs) maxAbs = a;
						...
						bool bad = false;
						if (!std::isfinite(s))                        { allFinite = false;    bad = true; }
						if (a > kHostileBoundV)                       {                       bad = true; }
						if (!(core.phase >= 0.0 && core.phase < 1.0)) { phaseInRange = false; bad = true; }
						...
						if (bad && firstBadStep < 0) firstBadStep = i;
					}
```

⚠️ `if (!(core.phase >= 0.0 && core.phase < 1.0))` at `:1224` is **already** an assertion on `phase` that a NaN `f` would break — the Pitfall 6 poisoning would surface here. That makes this loop the natural home for the new-divisor RED, and it also means the RED must show the failure *after* the hostile input is withdrawn (`firstBadStep` alone will not distinguish "bad during" from "bad forever" — a withdrawal phase must be added).

**Analog B — the two measured output tiers** (`:133-134`, asserted at `:862-863`, `:921-922`, `:988-989`, `:1269-1270`):

```cpp
constexpr float kHostileBoundV = 10.0f;
constexpr float kMusicalBoundV = 5.55f;
```
```cpp
			CHECK(maxAbs <= kHostileBoundV);
			CHECK(maxAbs <= kMusicalBoundV);
```

The provenance block at `:680-790` records, per scenario, the measured maxima that justify each number — including the audio-rate-MORPH scenario reaching 6.289864 V and the *withholding* of the tighter tier there being itself asserted. **Re-derive for sync; do not assume.** If sync exceeds `kMusicalBoundV` the pattern is to withhold the tighter tier for that scenario and say so in the provenance, not to widen the constant.

**Analog C — the interleave invariant, which the sync state must land inside** (`:1529-1560`): two cores, identical drift seeds, *different* spread seeds, driven sample-by-sample with two genuinely different input drives, each required to reproduce its solo block bit-exactly. `runInterleaveCheck` (`:170-200`) returns mismatch counts plus `soloEqual` as an explicit non-vacuity precondition. The permanent positive control at `:1646` proves the check can fail. CORE-03 for the sync trigger and `prevSyncVolts` means the interleave drives must carry **sync voltages**, or the new state sits outside the window the invariant covers.

**Analog D — the sub-sample interpolation helper, and its stated blindness** (`:176-190`, banner `:160-175`, corrected premise at `:594`): `estimateFreqRising` counts rising zero crossings and is blind under ~2 samples per cycle. **Do not use it for sync high-note claims** — the D-09 structural-ceiling assertion is the register-item-6 move applied instead.

---

### `tests/test_vco_spectrum.cpp` (test, batch) — the D-06 legs and the D-11 sub-grid

**Analog A — the cell struct every threshold hangs off** (`:611-622`):

```cpp
struct SpectrumCell {
	double sr;
	int K;
	const char* note;
	float morph;
	const char* region;
	float character;
	float measuredDb;
	float thresholdDb;
	const char* tier;
	const char* provenance;
};
```

A sync cell needs the same shape plus a master axis (`Kmaster`, master edge shape, master/slave ratio). ⚠️ **The `measuredDb` and `provenance` columns are not optional.** The banner at `:600-609` states that loosening `thresholdDb` alone breaks the reproduction CHECK in the measure pass, and that pair is what makes "pinned from measurement" defensible. The provenance-string constants at `:661-676` (`kProvMeasured`, `kProvFloored`, `kProvCrossRate`) are the templates; a sync sub-grid needs its own, naming the plan that measured it and the rate it was measured at.

**Analog B — the threshold floor** (`:648`):

```cpp
constexpr float kThresholdFloorDb = -75.0f;
```
The banner `:624-647` derives it from measured leakage and states explicitly that it is **static, deliberately** — "a threshold derived from the measurement it is checked against is a self-check that can never fail." Sync cells inherit this bound.

**Analog C — the alias metric, called unchanged with `K_master`** (`:186-223`): the harmonic-skip loop is `if ((i % K) == 0 && (i / K) <= maxHarmonic) continue;` — substituting `K_master` for `K` is the *only* change RESEARCH's derivation requires. Note the `-999.0` sentinel returns at `:191, :200, :221`.

**Analog D — the warm-up-and-measure loop** (`:937-949`):

```cpp
template <typename CoreT>
void driveSecondBlock(CoreT& core, const forge::VcoInputs& base, float dt, double sr,
                      std::vector<float>& out) {
	out.clear();
	out.reserve((std::size_t)kSpectrumN);
	for (int i = 0; i < 2 * kSpectrumN; ++i) {
		forge::VcoInputs in = base;
		in.sampleTime = dt;
		in.sampleRate = (float)sr;
		const float s = core.step(in);
		if (i >= kSpectrumN) out.push_back(s);
	}
}
```

It is a **template on `CoreT`** — which is exactly how the mirror and the live core share one loop, and how a sync-driving variant should take its per-sample master voltage (a functor parameter, not a fork). ⚠️ `:951-964` forbids a second measurement function outright: *"If a later agent adds a second measurement function for the corrected path, the phase's central claim stops being a measurement and becomes a coincidence."* The six D-06 legs must therefore come out of **one** `measureCellDb`-shaped function parameterised by leg, not six near-copies.

**Analog E — the seeds and the copy-assign POD build** (`:1025-1050`):

```cpp
	forge::VcoInputs base;
	base.pitchCV   = pitchCV;
	base.coarse    = 0.f;
	...
	base.drift     = 0.f;
	...
		forge::VcoCore core;
		core.seed(0x1234ULL, 0x5678ULL);
		core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);
		driveSecondBlock(core, base, dt, cell.sr, block);
```

The four seed literals are copied verbatim from `tests/VcoBlockDriver.hpp:42-43` and `:966-970` says they must never be invented — `(0,0)` is a Xoroshiro fixed point that hangs `std::normal_distribution`. **The renderer must copy these four too.**

**Analog F — the driver, and the overwrite that must stay unconditional** (`tests/VcoBlockDriver.hpp:49-64`):

```cpp
	// Drive nSamples through the core. inputAt(i) supplies the per-sample
	// VcoInputs; sampleTime and sampleRate are ALWAYS overwritten (the harness
	// owns timing). This overwrite is load-bearing — it must never become
	// conditional on what the caller's functor happened to put there.
	std::vector<float> run(int nSamples, const std::function<forge::VcoInputs(int)>& inputAt) {
```

The `inputAt(i)` functor is exactly the hook a per-sample master voltage goes through — **no driver change is needed** for sync, which is what D-02's raw-volts boundary buys. `:9-18` forbids templating or subclassing against `tests/BlockDriver.hpp` forever.

---

### `tests/test_vco_pitch.cpp` (test, event-driven) — PITCH-04's third input class

**Analog:** `:2138` (`PITCH-04 / D-10: the Nyquist clamp FIRES … and the oscillator KEEPS SOUNDING`) and `:2448` (the hostile pitch/FM standing regression). The file's naming convention is the pattern to copy: every TEST_CASE title states the requirement ID, the mechanism, **and** whether it is the RED or the standing check — `:1929`'s negative-control title (*"this case passing means the control DETECTED the defect"*) is the shape D-12's non-vacuity claim needs. D-12's obligation is that sync is observed **firing** behind the claim: the analog for "observed firing" is `tel.syncFired` accumulated across the drive and asserted non-zero, in the same spirit as `:989`'s telemetry-reading secondary tier being explicitly labelled the **weaker** tier.

---

### `tests/check_includes.sh` and `Makefile` (config/build, batch)

**`VCO_SIDE_ALLOW`** (`tests/check_includes.sh:355-364`):

```bash
VCO_SIDE_ALLOW=(
	"src/vco_compile_canary.cpp"
	"src/AnalogVCO.cpp"
	"tests/VcoBlockDriver.hpp"
	"tests/test_vco_harness.cpp"
	"tests/test_vco_core.cpp"
	"tests/test_vco_pitch.cpp"
	"tests/test_vco_spectrum.cpp"
	"tests/test_morph_blep.cpp"
)
```

The scan source is `find "${ROOT}/src" "${ROOT}/tests" "${ROOT}/tools"` (`:378-379`) — **`tools/` is scanned**, so `tools/render_sync_ab.cpp` is LFO-side by default and `make guards` exits 1 the moment it lands. One line, plus a rationale comment. Same cost for any new test TU.

**The Makefile skip filter** (`Makefile:24`):

```make
ifeq ($(filter test capture guards,$(MAKECMDGOALS)),)
include $(RACK_DIR)/plugin.mk
endif
```

**The `capture` target — the exact template for the render target** (`Makefile:50-64`):

```make
CAPTURE_BIN := build-test/capture

.PHONY: capture
capture: $(CAPTURE_BIN)
	./$(CAPTURE_BIN)

$(CAPTURE_BIN): tools/capture_golden.cpp $(TEST_HEADERS)
	@mkdir -p build-test
	$(CXX) $(TEST_CXXFLAGS) tools/capture_golden.cpp -o $@
```

Copy verbatim with names substituted. Three constraints the analog encodes: the new goal name **must join the `:24` filter list**, it compiles with `TEST_CXXFLAGS` (`-std=c++17 -O2 -ffp-contract=off`) so the rendered audio matches what `make test` measures, and it must stay **GNU Make 3.81-compatible** (`:96-97`: plain shell loops, no `$(file …)`, no `::=`).
`TEST_SOURCES := $(wildcard tests/*.cpp)` at `:36` is why the renderer must **not** live in `tests/` — it would link into `make test` and run every invocation, contradicting D-15.
`.gitignore` already contains `build-test/`, so `build-test/audition/` costs **zero** `.gitignore` edits.

---

## Shared Patterns

### Negated-comparison guards for hostile numeric input
**Source:** `src/dsp/VcoCore.hpp:597-602`, `src/dsp/MorphBlep.hpp:180, :183, :306, :511`
**Apply to:** every new guard in this phase — `morph`/`character` at `addStep`'s entry, `jump`, `f`, and any new divisor.

```cpp
	if (!(x > lo)) x = lo;      // negated FIRST — a NaN fails the comparison and lands here
	if (x > hi)    x = hi;      // plain second — leaves the fallback untouched
```
Early-return variant: `if (!(x >= 0.f) || x > 1.f) return;` (`MorphBlep.hpp:258`).
**Never** `forge::clamp` (`RackCompat.hpp:97`, NaN-transparent). **Never** a clamp ladder (both comparisons false for NaN — the exact reason `VcoCore` uses the pair).

### The measure → pin → prove-it-bites protocol
**Source:** `tests/test_vco_spectrum.cpp:600-676` (the `SpectrumCell` banner, `kThresholdFloorDb` derivation, the three provenance strings), `tests/test_vco_core.cpp:680-790` (the tier provenance block)
**Apply to:** the D-06 candidate ranking, the D-11 per-cell sync thresholds, the D-10 delta bound, the re-derived output tiers.
Every pinned number carries (a) the measured value in a sibling column, (b) a written provenance naming the plan and the rate it was measured at, (c) a reproduction CHECK comparing the measured column against this run, and (d) a discriminating mutation probe. Loosening a threshold alone breaks (c) — that coupling is the point.

### POD construction: copy-and-assign, never a brace value list
**Source:** `src/vco_compile_canary.cpp:83-87`, `tests/VcoBlockDriver.hpp:78-81`, `tests/test_vco_spectrum.cpp:1025-1034`
**Apply to:** every site that builds a `VcoInputs`.
```cpp
	// Copy + assign, never a brace value-list: VcoInputs has NSDMIs, so
	// under C++11 it is not an aggregate and a value-list init is a hard error (P-8).
	forge::VcoInputs in = base;
```

### The four seed literals
**Source:** `tests/VcoBlockDriver.hpp:41-43` — `0x1234 / 0x5678 / 0x9E3779B9 / 0x7F4A7C15`
**Apply to:** every new core/mirror/renderer construction. `(0,0)` is a Xoroshiro fixed point that hangs `std::normal_distribution` — a hung suite here, a hang on patch load in Rack.

### Comment density is a project convention, not decoration
**Source:** every file read. `VcoCore.hpp:544-596` is 53 comment lines for a 6-line guard; `MorphBlep.hpp:238-256` is 19 lines for a 6-line function; `Makefile:66-74` is 9 lines for a 3-line target.
**Apply to:** every new guard, seam, threshold and target. Each block states: what it protects, the measured evidence, the rejected alternative and why, and the named test that would go red. A plan that lands correct code with thin comments will read as out-of-character in review.

### New-TU cost (a plan task, not a gate-time discovery)
**Source:** `tests/check_includes.sh:355-379`, `Makefile:24, :36`
Any new `tests/*.cpp` costs one `VCO_SIDE_ALLOW` line. `tools/render_sync_ab.cpp` costs one `VCO_SIDE_ALLOW` line **and** one `MAKECMDGOALS` filter entry **and** one target. Both were flagged by CONTEXT.md's Phase 31 D-23 lesson: make it an explicit task with its own rationale.

### The source-shape contract (hard-fails `make guards`)
**Source:** `src/dsp/VcoCore.hpp:43-52`
```
// Source-shape contract (tests/check_canary.sh [2b/5]): the `struct VcoCore`
// declaration line and the `float step(...)` signature line must each stay on
// ONE line together with their opening brace. ... Allman braces, a wrapped
// parameter list or an added `noexcept` make `make guards` hard-fail with
// "could not perturb src/dsp/VcoCore.hpp" — a guard error, not a DSP error.
// NOTE ...: the step matcher is UNANCHORED, so quoting the full signature
// verbatim in a comment ON A LINE THAT ALSO CONTAINS `{` makes the canary
// perturb the COMMENT and the guard fails with unrelated compile errors.
```
⚠️ The second half bites this phase specifically: sync comments in `VcoCore.hpp` must not quote `float step(` on a line that also contains `{`.

### Zero `rack/` includes in `src/dsp/*.hpp`; C++11 `-pedantic-errors` for `src/`
**Source:** `src/dsp/VcoCore.hpp:64-84`, `Makefile:75-79`, `tests/check_includes.sh [2/7]`
No `inline constexpr` variables, no `if constexpr`, no `std::clamp`, no `[[maybe_unused]]`, no structured bindings, no nested-namespace syntax, **no in-class `static constexpr` indexed at runtime** (`MorphBlep.hpp:400-404` records that this is the construct that got v2.0.0 rejected — its nine-site tables are function-local `const` arrays for exactly this reason). `-ffp-contract=off`, never `-ffast-math`.

---

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `tools/render_sync_ab.cpp` — the **`.wav` writer** | tool | file-I/O | **No WAV writer exists anywhere in the repo.** The only binary writer is `tools/capture_golden.cpp:67-74`, which writes raw little-endian float32 with `f.write(reinterpret_cast<const char*>(&x), sizeof x)` — no header, no chunk layout, no int16 quantization. RESEARCH §"Don't Hand-Roll" is the source: ~30 lines of fixed RIFF layout, no dependency. Take the *scaling and reporting* discipline from RESEARCH Open Question 4 (fixed stated scale, never per-leg normalisation, report the clip count to stdout). |
| `tools/render_sync_ab.cpp` — the **grid/config parameterisation** (D-16) | tool | batch | No existing tool is parameterised by anything; `capture_golden.cpp` has a hardcoded 3-line `main()`. The closest in-repo pattern for "a grid driven through one function" is `measureCellDb` + `SpectrumCell` in `tests/test_vco_spectrum.cpp:611-622, :1001-1055` — **use that shape**, not a new one. |

**Structural analog for the tool TU as a whole:** `tools/capture_golden.cpp`, in full. It is the only precedent and it establishes every non-audio property the renderer needs:

```cpp
// Build:  make capture   (Rack-free; same TEST_CXXFLAGS as `make test`, so the
//                          captured bytes are bit-identical to what make test replays)
// Run from the repo root — output paths are relative to CWD.

#include "dsp/LfoCore.hpp"
#include <fstream>
...
namespace { ... }   // everything but main() in an anonymous namespace

int main() {
	capture(44100.0, "tests/golden/freerun_44100_driftoff.f32");
	...
	return 0;
}
```

Copy: the banner naming the build command and the CWD assumption, the anonymous namespace, the `std::ofstream` + open-failure `fprintf(stderr, …)` + early return, and the `std::printf` confirmation line naming the path and byte count. Substitute `#include "dsp/VcoCore.hpp"` and the `build-test/audition/` output path.
⚠️ `capture_golden.cpp`'s banner also carries a **critical landmine** paragraph (`:11-17`) about *not* using `BlockDriver`'s constructor because it seeds the spread path. The renderer's equivalent landmine is different but must be stated with the same prominence: the two legs must come from the **same pass** (D-14 / register item 26's binding constraint), which RESEARCH's telemetry-subtraction shortcut satisfies **only under candidate (b)**.

---

## Metadata

**Analog search scope:** `src/`, `src/dsp/`, `tests/`, `tools/`, `res/`, `Makefile`, `.gitignore`
**Files opened this session:** 17 (`RackCompat.hpp`, `VcoCore.hpp`, `MorphBlep.hpp`, `LfoCore.hpp`, `ClockTracker.hpp`, `AnalogVCO.cpp`, `vco_compile_canary.cpp`, `VcoBlockDriver.hpp`, `test_morph_blep.cpp`, `test_vco_core.cpp`, `test_vco_spectrum.cpp`, `test_vco_pitch.cpp` (index only), `check_includes.sh`, `Makefile`, `capture_golden.cpp`, `AnalogVCO.svg`, `.gitignore`)
**Project instructions:** no `./CLAUDE.md`, no `.claude/skills/`, no `.agents/skills/` — confirmed absent this session, matching RESEARCH §"Project Constraints".
**Pattern extraction date:** 2026-08-28
