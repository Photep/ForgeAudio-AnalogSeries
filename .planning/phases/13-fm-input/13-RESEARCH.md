# Phase 13: FM Input - Research

**Researched:** 2026-03-15
**Domain:** Exponential frequency modulation for LFO, FM+clock authority management, VCV Rack DSP patterns
**Confidence:** HIGH

## Summary

Phase 13 adds an FM (frequency modulation) input jack with an attenuator to the LFO. The FM signal modulates frequency exponentially -- each volt of FM CV shifts the frequency by a configurable number of octaves, following the standard 1V/octave convention used by VCV Rack's Fundamental modules. The exponential approach is natural because `freq = baseFreq * 2^(cv * depth)` can never produce negative frequencies (2^x is always positive), which directly satisfies success criterion 2.

The primary design challenge is managing FM authority in clocked mode (MOD-02). When clocked, the LFO's frequency is derived from clock period, and clock edges trigger phase resets. If FM pushes the frequency far from the clock-derived value, the LFO phase at the next clock edge will be wildly wrong, making the reset crossfade audible and destroying the "locked to clock" feel. The solution, already foreshadowed by the drift authority pattern in the codebase (line 514: `float maxDrift = isClocked ? 0.02f : 0.075f`), is to reduce the FM depth scaling factor in clocked mode. In free mode, the FM attenuator at full allows +/-3 octaves of modulation per 5V input. In clocked mode, this is reduced to approximately +/-0.5 octaves (a semitone-scale wobble), keeping the LFO frequency close enough to the clock-derived frequency that phase resets at clock edges remain clean.

**Primary recommendation:** Add FM_INPUT and FM_ATTEN_PARAM using the established knob+CV pattern. Apply FM as `freq *= dsp::exp2_taylor5(fmCV * fmDepth * depthScale)` where `depthScale` is 0.6 in free mode and 0.1 in clocked mode. Place FM controls at temporary panel positions (Phase 17 finalizes layout). The attenuator defaults to 0.0 (zero FM depth), ensuring v1.1-identical behavior with no FM patched or attenuator at minimum.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| MOD-01 | FM input jack applies exponential frequency modulation with CV-controllable depth | `dsp::exp2_taylor5(fmCV * fmAtten * depthScale)` applied as frequency multiplier; Trimpot attenuator controls depth; exponential FM guarantees no negative frequency |
| MOD-02 | FM authority reduced in clocked mode to prevent clock-phase fighting | Dual-scale pattern matching existing drift authority: `depthScale = isClocked ? 0.1f : 0.6f`; limits clocked FM to ~+/-0.5 octaves so clock-edge phase resets remain clean |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack 2 SDK | bundled | All DSP, param/input/output infrastructure | Already the project's sole dependency |
| `dsp::exp2_taylor5(float)` | SDK (`dsp/approx.hpp`) | Fast 2^x approximation for exponential FM | Used by VCV Fundamental LFO/VCO for FM; works with scalar float; <6e-06 relative error; included via `plugin.hpp` -> `rack.hpp` -> `dsp/approx.hpp` (line 117) |

### Supporting
| API | Purpose | When to Use |
|-----|---------|-------------|
| `rack::math::clamp` | Clamp FM depth scaling to safe range | Standard pattern for all bounded values in codebase |
| `configParam` | Configure FM attenuator with 0.0 default | Default 0.0 ensures zero FM when not in use (success criterion 4) |
| `configInput` | Register FM input jack | Standard input registration |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `dsp::exp2_taylor5(x)` | `std::pow(2.f, x)` | `exp2_taylor5` is 5-10x faster than `std::pow`; at LFO rates the difference is negligible per-sample, but it's the VCV convention and costs nothing to use |
| Exponential FM | Linear FM | Linear FM can produce negative frequencies requiring extra clamping; exponential is perceptually natural (pitch-linear) and inherently safe; linear FM explicitly out of scope per REQUIREMENTS.md |
| Trimpot attenuator (0-1) | Full attenuverter (-1 to +1) | Unipolar attenuator matches all existing CV patterns in codebase (Morph, Character, Drift, Phase Offset); bipolar not needed since FM input signal itself is bipolar |

**No installation needed.** All APIs available through existing `#include "plugin.hpp"`.

## Architecture Patterns

### Pattern 1: Exponential FM Frequency Multiplier

**What:** FM input voltage is scaled by an attenuator and a mode-dependent depth factor, then converted to a frequency multiplier via `2^x`.

**When to use:** In `process()`, after base frequency calculation but before phase accumulation.

**Example:**
```cpp
// Source: VCV Fundamental LFO pattern (github.com/VCVRack/Fundamental/blob/v2/src/LFO.cpp)
// Adapted for our non-SIMD monophonic architecture

// After targetFreq calculation (line ~476 in current code):
float fmMult = 1.f;
if (inputs[FM_INPUT].isConnected()) {
    float fmCV = inputs[FM_INPUT].getVoltage();  // bipolar, typically +/-5V
    float fmAtten = params[FM_ATTEN_PARAM].getValue();  // 0..1, default 0
    // Mode-dependent depth: how many octaves per volt at full attenuator
    float depthScale = isClocked ? 0.1f : 0.6f;
    float fmPitch = fmCV * fmAtten * depthScale;
    fmMult = dsp::exp2_taylor5(fmPitch);
}
targetFreq *= fmMult;
```

**Key properties:**
- `2^0 = 1` when attenuator is 0 or no cable connected -- frequency unchanged
- `2^x > 0` for all x -- frequency can never go negative
- In free mode: 5V * 1.0 * 0.6 = 3.0 octaves (8x frequency) -- dramatic modulation
- In clocked mode: 5V * 1.0 * 0.1 = 0.5 octaves (1.41x frequency) -- gentle wobble

### Pattern 2: FM Before Frequency Slew

**What:** FM multiplication is applied to `targetFreq` before the `freqSlew` filter, so the slew filter smooths both mode transitions AND FM modulation changes.

**When to use:** FM should be applied to `targetFreq`, not to the post-slew `freq` value.

**Example:**
```cpp
// Current flow (lines 466-484):
//   1. Calculate targetFreq (from knob or clock)
//   2. Clamp: targetFreq = std::fmax(targetFreq, 0.001f)
//   3. Slew: freq = freqSlew.process(sampleTime, targetFreq)
//   4. Clamp: freq = std::fmax(freq, 0.001f)
//   5. Phase accumulate: deltaPhase = freq * sampleTime

// NEW flow:
//   1. Calculate targetFreq (from knob or clock)
//   2. Apply FM: targetFreq *= fmMult
//   3. Clamp: targetFreq = std::fmax(targetFreq, 0.001f)
//   4. Slew: freq = freqSlew.process(sampleTime, targetFreq)
//   5. Clamp: freq = std::fmax(freq, 0.001f)
//   6. Phase accumulate: deltaPhase = freq * sampleTime
```

**Why before slew:** The slew filter has a 50ms time constant (lambda=20), which would smooth out fast FM changes. For FM to be responsive, it should bypass the slew. However, applying FM before slew means the slew acts as a gentle lowpass on the FM effect, which is actually desirable -- it prevents extreme FM from causing sudden frequency jumps.

**Alternative: FM after slew.** If the planner decides FM should be maximally responsive (instant frequency changes), apply FM multiplication after the slew filter. This means FM bypasses the 50ms smoothing. The tradeoff is sharper FM response vs. potential for abrupt frequency changes. For an LFO (not a VCO), the 50ms slew is negligible compared to LFO rates, so either position works. Applying before slew is slightly safer.

### Pattern 3: Dual-Scale Authority (Matches Drift Pattern)

**What:** FM depth scaling uses a binary mode switch, matching the drift authority pattern already in the codebase.

**When to use:** When calculating FM depth.

**Example:**
```cpp
// Existing drift authority pattern (line 514):
float maxDrift = isClocked ? 0.02f : 0.075f;

// Analogous FM authority pattern:
float depthScale = isClocked ? 0.1f : 0.6f;
```

**Design rationale for the specific values:**
- **Free mode (0.6):** At full attenuator with +/-5V input, this gives +/-3 octaves of FM. At 1Hz base, the frequency swings from 0.125Hz to 8Hz. This is dramatic but musically useful for LFO-rate vibrato-on-vibrato effects.
- **Clocked mode (0.1):** At full attenuator with +/-5V input, this gives +/-0.5 octaves. At 2Hz clock-derived frequency, FM swings from ~1.41Hz to ~2.83Hz. The LFO completes roughly one cycle per clock period (+/-41% variation), so clock-edge phase resets land within ~0.3-0.7 of phase -- the crossfade handles this smoothly.
- **Tuning note:** These values should be validated empirically. If clocked FM at 0.1 still causes audible phase-reset artifacts, reduce to 0.05. If free-mode FM at 0.6 feels insufficient, increase to 1.0 (1V/octave).

### Pattern 4: New Enum Entries and Constructor Config

**What:** Add FM input and attenuator to the module's parameter and input enums, with constructor configuration.

**When to use:** Module setup.

**Example:**
```cpp
// In ParamId enum (add before PARAMS_LEN):
FM_ATTEN_PARAM,

// In InputId enum (add before INPUTS_LEN):
FM_INPUT,

// In constructor:
configParam(FM_ATTEN_PARAM, 0.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f);
configInput(FM_INPUT, "FM");
```

**Critical: default value is 0.0**, not 1.0. This differs from the other attenuators (Morph/Character/Drift/Phase Offset CV attenuators default to 1.0). The FM attenuator defaults to 0 because:
1. Success criterion 4 requires identical v1.1 behavior with attenuator at zero
2. FM is an "opt-in" effect -- patching a cable into FM jack should not immediately modulate frequency until the user turns up the attenuator
3. This matches VCV Fundamental LFO convention where FM params default to 0

### Pattern 5: Temporary Panel Placement

**What:** FM jack and attenuator need temporary positions. Phase 17 finalizes layout.

**When to use:** Widget setup in `AnalogLFOWidget` constructor.

**Example:**
```cpp
// FM controls at temporary positions (Phase 17 finalizes layout)
// Place near CLK/RESET area since FM is a modulation input
addParam(createParamCentered<Trimpot>(mm2px(Vec(42.96, 96.0)),
         module, AnalogLFO::FM_ATTEN_PARAM));
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.96, 108.0)),
         module, AnalogLFO::FM_INPUT));
```

**Note:** These positions may overlap with existing controls. The planner must check current panel occupancy and find non-colliding temporary positions. The current occupancy at y=96 and y=108:
- x=10: Morph atten/CV
- x=24: Character atten/CV
- x=38: Drift atten/CV
- x=52: Phase Offset atten/CV

x=42.96 at y=96/108 is near Drift atten/CV (x=38) -- may be too close. Planner should verify 7mm minimum center-to-center spacing.

### Anti-Patterns to Avoid

- **Linear FM for LFO:** Linear FM (`freq += fmCV * depth`) can produce negative frequencies, requiring extra clamping that creates flat spots at the frequency floor. Exponential FM is inherently safe and perceptually natural. Linear FM is explicitly out of scope per REQUIREMENTS.md.
- **Full FM authority in clocked mode:** Without authority reduction, strong FM pushes the LFO phase far from where the clock edge expects it, making reset crossfades audible as rhythmic "hiccups." The reduced authority is essential for MOD-02.
- **FM attenuator defaulting to 1.0:** Would immediately modulate frequency when any cable is patched into FM, surprising users. Default 0.0 matches user expectation ("plug in, then turn up").
- **Applying FM to phase instead of frequency:** Phase modulation (PM) is a different effect. FM modulates the rate of phase change, not the phase value itself. PM would also interact badly with the phase offset feature from Phase 12.
- **Separate frequency clamping for FM:** The existing `std::fmax(targetFreq, 0.001f)` floor already prevents near-zero frequencies. Exponential FM cannot produce negative values. No additional clamping is needed beyond what exists.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| 2^x computation | Manual polynomial or `std::pow` | `dsp::exp2_taylor5(float)` | SDK-provided, fast, accurate (<6e-06 error), used by all VCV Fundamental modules for FM |
| Trigger detection for FM input | N/A | N/A | FM is a continuous CV signal, not a trigger -- no trigger detection needed |
| Frequency floor clamping | Custom negative-frequency detection | Exponential FM itself (`2^x > 0 always`) | The math guarantees positive frequency; existing `std::fmax(0.001f)` handles edge case of extremely negative FM pitch values pushing toward 0 |

**Key insight:** Exponential FM is self-clamping. The formula `baseFreq * 2^(cv * depth)` produces a positive frequency for any input values because `2^x > 0` for all real x. This eliminates an entire class of stability bugs that would plague linear FM.

## Common Pitfalls

### Pitfall 1: FM Depth Scale Too Aggressive in Clocked Mode
**What goes wrong:** With FM depth too high in clocked mode, each clock edge triggers a crossfade from an unexpected phase position. At fast clock rates, these crossfades overlap and create a continuous warbling artifact instead of clean rhythmic sync.
**Why it happens:** The depth scale is set to match free-running mode, ignoring that clocked mode relies on phase position being near-zero (or near the expected division count position) at each clock edge.
**How to avoid:** Use a conservative clocked-mode depth scale (0.1). Validate by patching a 5V bipolar LFO into FM with full attenuator while clocked -- the output should show rhythmic modulation without losing clock sync.
**Warning signs:** Audible rhythmic hiccups or clicks at clock edges when FM is active.

### Pitfall 2: FM Applied After Frequency Slew Creates Latency
**What goes wrong:** If FM multiplication is applied after the exponential slew filter, the slew does not smooth FM -- FM acts instantaneously on the post-slew frequency. This is not necessarily wrong (it gives maximum FM responsiveness), but if the intent is for FM to be slew-smoothed, applying after slew defeats that.
**Why it happens:** Ambiguity about where in the signal chain FM belongs.
**How to avoid:** Decide intentionally: before slew (FM is smoothed, 50ms lag) or after slew (FM is instant). For an LFO, 50ms is a tiny fraction of most cycle periods, so the distinction is minor. Recommendation: apply to `targetFreq` before slew for consistency with how drift also modifies `deltaPhase` (which is derived from post-slew freq).
**Warning signs:** FM feels sluggish or unresponsive -- check if slew is over-dampening.

### Pitfall 3: FM Breaks Serialization / Preset Behavior
**What goes wrong:** Adding new params and inputs changes the enum ordering, which breaks existing patches and presets if enum values shift.
**Why it happens:** Inserting new enum entries before `PARAMS_LEN`/`INPUTS_LEN` changes the integer values of existing enums if placed incorrectly.
**How to avoid:** Always add new enum entries immediately before `PARAMS_LEN` or `INPUTS_LEN`. The existing code already follows this pattern (Phase 12 added `PHASE_OFFSET_PARAM`, `PHASE_OFFSET_ATTEN_PARAM` before `PARAMS_LEN`). Check the current enum ordering and append, never insert in the middle.
**Warning signs:** Loading old patches causes parameters to map to wrong controls.

### Pitfall 4: Forgetting to Check `isConnected()` for FM Input
**What goes wrong:** Reading an unconnected input returns 0V, so `2^(0 * depth) = 1.0` and there is no functional problem. But processing the exp2 calculation every sample when FM is unpatched wastes CPU.
**Why it happens:** FM code runs unconditionally without checking cable connection.
**How to avoid:** Wrap FM processing in `if (inputs[FM_INPUT].isConnected()) { ... }`. When not connected, `fmMult` stays 1.0 (no frequency change). This is a minor optimization but follows the existing pattern used for Phase Offset CV (line 553: `if (inputs[PHASE_OFFSET_CV_INPUT].isConnected())`).
**Warning signs:** No functional bug, just unnecessary CPU usage.

### Pitfall 5: FM Depth Discontinuity at Mode Transition
**What goes wrong:** When the clock cable is disconnected (transitioning from clocked to free mode), the FM depth scale jumps from 0.1 to 0.6 instantly. If FM is active with high attenuator, this causes a sudden 6x increase in FM effect, producing an audible frequency jump.
**Why it happens:** `isClocked` is a binary state that flips instantly on cable disconnect.
**How to avoid:** The existing `freqSlew` filter (50ms time constant) smooths the resulting frequency change, which mitigates this. For extra safety, the FM depth scale itself could be slewed, but this adds complexity for a rare edge case (disconnecting clock cable while FM is active). Recommendation: rely on `freqSlew` for now; add depth-scale slew only if testing reveals audible artifacts.
**Warning signs:** Pop or click when disconnecting clock cable while FM is patched.

## Code Examples

### Example 1: Complete FM Processing Block

```cpp
// Source: Adapted from VCV Fundamental LFO FM pattern
// Position: In process(), after targetFreq calculation (line ~476)

// FM processing (MOD-01, MOD-02)
if (inputs[FM_INPUT].isConnected()) {
    float fmCV = inputs[FM_INPUT].getVoltage();
    float fmAtten = params[FM_ATTEN_PARAM].getValue();
    // Authority: reduced in clocked mode to prevent clock-phase fighting
    float depthScale = isClocked ? 0.1f : 0.6f;
    float fmPitch = fmCV * fmAtten * depthScale;
    targetFreq *= dsp::exp2_taylor5(fmPitch);
}
targetFreq = std::fmax(targetFreq, 0.001f);
```

### Example 2: Enum and Constructor Updates

```cpp
// New enum entries (append before LEN sentinels):
enum ParamId {
    // ... existing ...
    PHASE_OFFSET_PARAM,
    PHASE_OFFSET_ATTEN_PARAM,
    FM_ATTEN_PARAM,         // NEW
    PARAMS_LEN
};
enum InputId {
    // ... existing ...
    PHASE_OFFSET_CV_INPUT,
    FM_INPUT,               // NEW
    INPUTS_LEN
};

// Constructor config:
configParam(FM_ATTEN_PARAM, 0.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f);
configInput(FM_INPUT, "FM");
```

### Example 3: Exponential FM Math Verification

```cpp
// Verification of FM behavior at key points:
//
// Free mode (depthScale = 0.6):
//   fmCV=0V:  2^(0 * 1.0 * 0.6) = 2^0.0 = 1.0x (no change)
//   fmCV=+5V: 2^(5 * 1.0 * 0.6) = 2^3.0 = 8.0x (3 octaves up)
//   fmCV=-5V: 2^(-5 * 1.0 * 0.6) = 2^-3.0 = 0.125x (3 octaves down)
//   fmCV=+5V, atten=0.5: 2^(5 * 0.5 * 0.6) = 2^1.5 = 2.83x
//
// Clocked mode (depthScale = 0.1):
//   fmCV=+5V: 2^(5 * 1.0 * 0.1) = 2^0.5 = 1.414x (~+0.5 octave)
//   fmCV=-5V: 2^(-5 * 1.0 * 0.1) = 2^-0.5 = 0.707x (~-0.5 octave)
//   fmCV=+10V: 2^(10 * 1.0 * 0.1) = 2^1.0 = 2.0x (1 octave, worst case)
//
// With attenuator at 0 (default):
//   fmCV=anything: 2^(cv * 0.0 * scale) = 2^0 = 1.0x (NO CHANGE)
//   This guarantees success criterion 4 (identical to v1.1)
```

### Example 4: Widget Placement

```cpp
// FM controls at temporary positions (Phase 17 finalizes layout)
// FM attenuator trimpot and jack in the bottom section
addParam(createParamCentered<Trimpot>(mm2px(Vec(X_POS, 96.0)),
         module, AnalogLFO::FM_ATTEN_PARAM));
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(X_POS, 108.0)),
         module, AnalogLFO::FM_INPUT));
// Note: X_POS must be determined by planner to avoid collision
// with existing controls. Current occupancy at y=96/108:
//   x=10 (Morph), x=24 (Character), x=38 (Drift), x=52 (Phase Offset)
```

## State of the Art

| Old Approach (current code) | New Approach (this phase) | Impact |
|------------------------------|---------------------------|--------|
| No FM input | Exponential FM with attenuator | Users can frequency-modulate the LFO for vibrato-on-vibrato, complex rhythmic modulation |
| Drift already has dual authority (0.075 free / 0.02 clocked) | FM has parallel dual authority (0.6 free / 0.1 clocked) | Consistent pattern: modulation effects are tamed in clocked mode to preserve sync |
| `targetFreq` computed from knob or clock only | `targetFreq` modified by FM multiplier before slew | Modulation chain grows: knob/clock -> FM -> slew -> drift -> phase accumulation |

**Nothing deprecated:** All existing behavior remains unchanged. With FM attenuator at 0 (default), output is bit-identical to v1.1.

## Open Questions

1. **Exact FM depth scale values**
   - What we know: Free mode should allow dramatic modulation; clocked mode should keep FM gentle enough for clean clock resets.
   - What's unclear: The proposed values (0.6 free, 0.1 clocked) are informed by the VCV Fundamental LFO convention and the existing drift authority ratio, but optimal values require empirical testing in VCV Rack.
   - Recommendation: Implement with proposed values, test with clock + FM patch, adjust if needed. The values are parameters in the code, easy to tune.

2. **FM application point relative to slew filter**
   - What we know: FM before slew means slew smooths FM; after slew means FM is instant.
   - What's unclear: Which sounds better for LFO use cases.
   - Recommendation: Apply FM to `targetFreq` (before slew). The slew's 50ms time constant is negligible at LFO rates and provides a safety buffer against extreme FM jumps. If testing shows FM feels sluggish, move after slew.

3. **FM interaction with drift**
   - What we know: Drift currently modifies `deltaPhase` (line 516: `deltaPhase *= (1.0 + driftScale * combinedOU)`). FM modifies `targetFreq` (before slew, before delta phase computation).
   - What's unclear: Whether FM and drift interact well or create unexpected combined effects.
   - Recommendation: FM and drift are independent modulation sources operating at different points in the chain. FM modifies the base frequency; drift modifies the per-sample phase increment. They should combine naturally: FM sets the "intended" frequency, and drift adds organic wandering around that frequency. No special interaction handling needed.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Manual verification in VCV Rack (DSP behavior) + compile-time check |
| Config file | none -- no automated test framework for VCV Rack modules |
| Quick run command | `cd "/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series" && make -j4` |
| Full suite command | Manual: build, install, load in VCV Rack, test with FM source and clock |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| MOD-01 | FM input applies exponential frequency modulation with attenuator depth control | manual-only | Patch: LFO -> FM input of module. Turn FM attenuator from 0 to full. Observe frequency modulation on scope. Verify modulation depth increases with attenuator. At attenuator=0, verify no modulation. | N/A |
| MOD-02 | FM authority reduced in clocked mode to prevent clock-phase fighting | manual-only | Patch: clock source (e.g. 120 BPM) into CLK. Patch LFO into FM with full attenuator. Compare modulation depth in free mode vs. clocked mode. Clocked should show significantly less FM effect. Verify clock sync is maintained (waveform still resets at clock edges). | N/A |

### Sampling Rate
- **Per task commit:** Build compiles without errors (`make -j4`)
- **Per wave merge:** Manual test in VCV Rack with FM + clock test patch
- **Phase gate:** Full verification of all 4 success criteria in VCV Rack

### Wave 0 Gaps
None -- this phase modifies existing C++ source only. No test framework applies to real-time audio module behavior beyond compile verification. All behavioral verification is manual in VCV Rack.

## Sources

### Primary (HIGH confidence)
- Project source `src/AnalogLFO.cpp` (local, 1101 lines) -- complete current implementation including frequency calculation pipeline, clock tracking, drift authority pattern, existing parameter/input enums
- VCV Rack SDK `include/dsp/approx.hpp` (local) -- `exp2_taylor5()` API verified: template function, works with scalar `float`, <6e-06 relative error, included via `rack.hpp` line 117
- VCV Rack SDK `include/rack.hpp` (local) -- confirmed `dsp/approx.hpp` is included at line 117, available through `plugin.hpp`
- VCV Fundamental LFO source (`github.com/VCVRack/Fundamental/blob/v2/src/LFO.cpp`) -- FM pattern: `pitch += FM_INPUT * FM_PARAM; freq = clockFreq/2 * exp2(pitch)`; FM_PARAM configured as (-1, 1, 0.f); FM applies uniformly regardless of clock state
- VCV Fundamental VCO source (`github.com/VCVRack/Fundamental/blob/v2/src/VCO.cpp`) -- exponential FM: `pitch += FM_INPUT * FM_PARAM; freq = C4 * exp2(pitch)`; frequency clamped to `[0, sampleRate/2]`

### Secondary (MEDIUM confidence)
- `.planning/REQUIREMENTS.md` -- MOD-01, MOD-02 requirement definitions; "Linear FM mode" and "Through-zero FM" explicitly out of scope
- `.planning/STATE.md` -- blocker noted: "FM clocked-mode authority design decision needed before Phase 13 begins"
- `.planning/phases/12-reset-and-phase-offset/12-RESEARCH.md` -- established research format, drift authority pattern reference

### Tertiary (LOW confidence)
- VCV Community forum discussions on FM CV levels -- general consensus that 1V/octave is standard for exponential FM in Eurorack context
- Bogaudio LLFO source -- FM via `PITCH_INPUT` port, but parent class implementation not fully inspected

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- `dsp::exp2_taylor5` verified in local SDK headers; exponential FM math is well-established
- Architecture: HIGH -- FM-before-slew pattern follows VCV Fundamental convention; dual authority pattern matches existing drift code in codebase
- Pitfalls: HIGH -- clock-phase fighting understood from drift authority experience; enum ordering verified against current source; mode transition smoothing covered by existing freqSlew

**Research date:** 2026-03-15
**Valid until:** Indefinite -- VCV Rack 2 SDK is stable, exponential FM is a settled technique, project codebase patterns are established
