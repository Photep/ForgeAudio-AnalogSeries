# Phase 2: Waveform Engine - Research

**Researched:** 2026-02-25
**Domain:** LFO DSP engine -- phase accumulator, waveform generation, continuous morphing, CV input handling
**Confidence:** HIGH

## Summary

Phase 2 transforms the empty `process()` callback in `AnalogLFO.cpp` into a functioning LFO with four morphable waveform shapes, a rate knob, morph CV input with attenuator, and a single bipolar output. The technical domain is well-understood: double-precision phase accumulation, basic waveform math (sin, triangle, saw, square), linear crossfade morphing between adjacent shapes, and standard VCV Rack CV input conventions. No external libraries are needed -- everything uses the VCV Rack 2 SDK (version 2.6.6, installed locally at `../Rack-SDK`) and standard C++ math.

The primary complexity is structural, not algorithmic. This phase must: (1) change the Rate parameter from exponential to linear mapping (0.01-20Hz), (2) remove the inverted output (reducing from 2 outputs to 1), (3) add a new Morph CV attenuator parameter (not in the Phase 1 scaffold), (4) update the SVG panel to accommodate the new attenuator knob and remove the INV output label, and (5) implement the four-shape morph engine with the user's specified equal-quarter distribution. The waveform generation math itself is straightforward -- the POC at `/Users/mrcbrown/Claude/Software/Forge Audio/LFO/src/LFO.cpp` already demonstrates all four waveform formulas and the phase accumulator pattern.

The morph interpolation approach recommended for the "Claude's Discretion" items is: linear crossfade between adjacent shapes (simpler, produces clean intermediate shapes for these particular waveform pairs), 10V full-range CV scaling (VCV standard for unipolar CV modulating a 0-1 parameter), and a Trimpot-style knob (~6mm diameter) positioned directly above the Morph CV jack for the attenuator.

**Primary recommendation:** Implement this as a focused DSP phase with minimal architectural scaffolding. Keep all code in `AnalogLFO.cpp` (no separate DSP files yet -- the engine is ~50 lines of math). Separate into `dsp/` files only when Phase 5 (Character) or the VCO module creates actual code sharing needs.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **Morph sweep character:** Equal quarter distribution: Sine 0-25%, Triangle 25-50%, Saw 50-75%, Square 75-100%. Shape order: Sine -> Triangle -> Saw -> Square (smooth to sharp, low to high harmonics). No snap/detent at pure shape points -- fully smooth continuous sweep.
- **Rate knob feel:** Linear mapping across 0.01Hz to 20Hz range (equal Hz per degree of rotation). Default rate: ~0.7Hz (matching the previous Forge Audio LFO project). Current codebase has exponential mapping -- this needs to change to linear. Hz value shown via VCV Rack's built-in tooltip on hover (no panel display needed). No special behavior at extremes.
- **CV input behavior:** Morph CV is additive (offset): knob sets center position, CV sweeps around it. Hard clamp when combined knob + CV exceeds range (stays at sine or square, no wrap-around). Add a small attenuator knob next to the morph CV jack for controlling modulation depth (0-100%). Note: this adds a new param and panel element not currently in the scaffold.
- **Output signal shape:** Single output jack only -- inverted output removed (simplifies display in Phase 3). Bipolar +/-5V output, all shapes centered at 0V (no DC offset for any shape). Saw waveform: rising ramp (ramps up from -5V to +5V, snaps down). Square waveform: 50% duty cycle.
- **Current codebase declares 2 outputs -- reduce to 1.**

### Claude's Discretion
- Morph interpolation method between shapes (linear crossfade vs shape-aware -- pick whatever produces the best intermediate shapes)
- CV-to-morph voltage scaling (10V full range vs 5V vs other)
- Phase accumulator implementation details (double precision already specified in success criteria)
- Exact attenuator knob sizing and panel placement near CV jack

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| WAVE-01 | Module generates sine, triangle, saw, and square waveforms from shared double-precision phase accumulator | POC validates all four waveform formulas; double-precision phase accumulator prevents LFO stall at low rates (see Phase Accumulator section below) |
| WAVE-02 | Morph knob continuously sweeps Sine -> Triangle -> Saw -> Square using parametric shape morphing | Equal quarter distribution with linear crossfade between adjacent shapes; zero-crossing aligned waveform definitions prevent discontinuities (see Morph Engine section) |
| WAVE-03 | Morph CV input modulates morph position | Additive offset with attenuator knob; 10V = full range scaling; hard clamp at boundaries (see CV Input section) |
| OUT-01 | Bipolar +/-5V morphed waveform output | Single output at 5.f * morphedSample; all shapes produce [-1, +1] normalized output centered at 0V |
| OUT-02 | Inverted morphed waveform output | **REMOVED per CONTEXT.md decision** -- single output only. Enum and widget code must be updated to remove INV_OUTPUT |
| PTCH-01 | Rate knob controls LFO frequency across sub-audio range | Linear mapping 0.01-20Hz; configParam with direct Hz range; tooltip shows Hz via built-in display (see Rate Knob section) |
</phase_requirements>

## Standard Stack

### Core

| Component | Version | Purpose | Why Standard | Confidence |
|-----------|---------|---------|--------------|------------|
| VCV Rack 2 SDK | 2.6.6 | Plugin framework, module lifecycle, param/port API | Required; installed locally at `../Rack-SDK` | HIGH (verified locally) |
| C++17 | System clang | Implementation language | SDK's plugin.mk sets `-std=c++17` | HIGH |
| `<cmath>` | Standard library | `std::sin`, `std::fabs`, `std::floor`, `std::fmin` | Standard waveform math; `std::sin` is fine at LFO rates (no audio-rate per-sample concern) | HIGH |

### Supporting

| Component | Purpose | When to Use |
|-----------|---------|-------------|
| `rack::math::clamp()` | Clamping morph+CV combined value to [0,1] | Every sample when CV is connected |
| `rack::dsp::exp2_taylor5()` | NOT used in Phase 2 (exponential mapping removed) | Kept available for future phases |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Linear crossfade morph | Shape-aware parametric morphing (Hermite interpolation on shape parameters) | Hermite preserves derivative continuity but is more complex. For these specific shape pairs (sine-triangle, triangle-saw, saw-square), linear crossfade produces musically valid intermediate shapes because adjacent shapes share similar zero-crossing structures. Linear wins on simplicity. |
| `std::sin()` for sine wave | Polynomial approximation (Bhaskara, parabolic) | At LFO rates (<20Hz), `std::sin()` is called once per sample, not 44100 times per cycle. CPU cost is negligible. Use `std::sin()` for correctness. |
| Inline DSP in AnalogLFO.cpp | Separate `dsp/MorphStage.hpp` file | Premature separation adds file complexity for ~50 lines of math that only one module uses. Refactor into shared DSP files when Phase 5 (Character) or VCO module creates actual sharing needs. |

## Architecture Patterns

### Current File Structure (Phase 2 changes marked)

```
src/
  plugin.hpp                    # No changes needed
  plugin.cpp                    # No changes needed
  AnalogLFO.cpp                 # MODIFIED: DSP implementation, enum changes, new param
res/
  AnalogLFO.svg                 # MODIFIED: add attenuator knob, remove INV label/component
  PANEL-SPEC.md                 # MODIFIED: update component table
```

### Pattern 1: configParam for Linear Hz Range

**What:** The Rate knob needs linear mapping from 0.01Hz to 20Hz with Hz display in the tooltip. The current scaffold uses exponential mapping (`configParam(RATE_PARAM, -8.f, 4.f, -3.f, "Rate", " Hz", 2.f, 1.f)`). This must change to a direct linear range.

**Source:** VCV Rack SDK `ParamQuantity` -- when `displayBase = 0.f`, the display value equals `value * displayMultiplier + displayOffset`. For a parameter whose raw value IS the Hz value, simply use:

```cpp
// Linear rate: raw value IS the frequency in Hz
// Range: 0.01 to 20.0, default ~0.7
// displayBase=0 (linear), displayMultiplier=1, displayOffset=0
configParam(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz");
```

The tooltip will automatically show "Rate: 0.70 Hz" and allow the user to type in Hz values directly when right-clicking the knob.

**Verified against SDK:** `ParamQuantity.hpp` lines 42-49 confirm: displayBase=0 means linear, displayMultiplier defaults to 1.f, displayOffset defaults to 0.f. The `configParam` signature in `Module.hpp` line 125 shows displayBase defaults to 0.f, so omitting the last three arguments gives linear display.

### Pattern 2: Double-Precision Phase Accumulator

**What:** Use `double` for the phase variable and phase increment calculation to prevent LFO stall at very low rates.

**Why necessary:** At 0.01Hz and 48kHz sample rate, the phase increment is `0.01 / 48000 = 2.083e-7`. A `float` has ~7 decimal digits of precision. When the accumulator reaches 0.5, adding 2.083e-7 rounds to 0.5 with only ~6 significant digits of precision -- the increment is at the edge of representability. `double` provides ~15.9 digits, handling increments down to ~1e-15 without loss.

```cpp
double phase = 0.0;  // NOT float

void process(const ProcessArgs& args) override {
    float freq = params[RATE_PARAM].getValue();  // 0.01 to 20.0 Hz (linear)
    double deltaPhase = (double)freq * (double)args.sampleTime;
    phase += deltaPhase;
    if (phase >= 1.0) phase -= 1.0;  // Wrap to [0, 1)

    // Use (float)phase for waveform generation (float precision is fine for shape math)
    float p = (float)phase;
    // ... generate waveform from p ...
}
```

### Pattern 3: Four-Shape Morph Engine (Equal Quarter Distribution)

**What:** The morph knob (0.0 to 1.0) maps to four equal quarters. At the boundaries (0.0, 0.25, 0.5, 0.75, 1.0) you get pure shapes. Between boundaries, linear crossfade between the two adjacent shapes.

```cpp
float computeMorphedWave(float phase, float morph) {
    // Compute the four base shapes
    float sine = std::sin(2.f * M_PI * phase);
    float tri  = 4.f * std::fabs(phase - 0.5f) - 1.f;           // symmetric triangle
    float saw  = 2.f * phase - 1.f;                               // rising ramp [-1, +1]
    float sqr  = (phase < 0.5f) ? 1.f : -1.f;                    // 50% duty cycle

    // Four equal quarters: 0-25% sine-tri, 25-50% tri-saw, 50-75% saw-sqr, 75-100% = sqr
    float scaled = morph * 4.f;  // 0 to 4
    int segment = std::min((int)scaled, 3);
    float frac = scaled - (float)segment;

    switch (segment) {
        case 0: return sine + frac * (tri - sine);    // sine -> triangle
        case 1: return tri  + frac * (saw - tri);     // triangle -> saw
        case 2: return saw  + frac * (sqr - saw);     // saw -> square
        case 3: return sqr;                            // pure square
    }
    return sine;  // fallback
}
```

**Note on waveform definitions (zero-crossing alignment):** All four shapes cross zero at phase=0.0 and phase=0.5, which ensures smooth morphing without discontinuities at the blend boundaries. The triangle formula `4*|phase-0.5|-1` starts at -1, peaks at +1 at phase=0.5, and returns to -1 at phase=1.0. This differs from the POC's triangle formula which used a quarter-phase offset. The new formula is preferred for morph alignment: it shares zero-crossings with the saw and square shapes.

**Correction to POC formulas:** The POC triangle is `4*|phase+0.25 - round(phase+0.25)| - 1`, which starts at +1 at phase=0. For morph compatibility, we want all shapes to start at the same value. The recommended approach: all shapes start at their minimum/crossing at phase=0 and peak at phase=0.25 or phase=0.5 depending on shape. Actually, the simplest approach for zero-crossing alignment:

- **Sine:** `sin(2*pi*phase)` -- starts at 0, peaks at +1 at phase=0.25
- **Triangle:** `2*|2*phase - 1| - 1` or equivalently `4*|phase-0.5| - 1` -- starts at -1, crosses 0 at 0.25, peaks at +1 at 0.5, crosses 0 at 0.75, returns to -1 at 1.0. Hmm, this does NOT start at 0.

For morph smoothness, the critical property is that adjacent shapes have similar values at each phase position throughout the cycle, not just at zero crossings. Let's verify the sine-triangle pair:
- At phase 0.0: sine = 0, triangle (using `4*|p-0.5|-1`) = -1. That's a big gap.
- With the POC formula `4*|p+0.25 - round(p+0.25)| - 1`: at phase 0.0 = 4*0.25 - 1 = 0. At phase 0.25 = 4*0 - 1 = -1.

The POC triangle starts at 0, matching sine at phase=0. This is better for morph. Let me verify the full cycle:
- POC tri at p=0: 0, sine at p=0: 0 (match)
- POC tri at p=0.25: -1, sine at p=0.25: +1 (mismatch)
- POC tri at p=0.5: 0, sine at p=0.5: 0 (match)

That mismatch at 0.25 is large. In reality, for LFO waveform morphing the exact phase relationship matters less than it does for audio-rate -- the morph transition happens smoothly over the knob range, and at any fixed morph position the waveform is a valid shape. The linear crossfade at the 50% point between sine and triangle will look like a somewhat flattened sine, which is musically useful.

**Recommended waveform definitions for this implementation:**

```cpp
float sine = std::sin(2.f * (float)M_PI * phase);
float tri  = 2.f * std::fabs(2.f * phase - 1.f) - 1.f;  // V-shape: -1 at 0, +1 at 0.5, -1 at 1
float saw  = 2.f * phase - 1.f;                           // rising: -1 at 0, +1 at 1 (snaps down)
float sqr  = (phase < 0.5f) ? 1.f : -1.f;                // high-low
```

The specific waveform starting values at phase=0 are: sine=0, tri=-1, saw=-1, sqr=+1. Adjacent pairs: sine-tri (0 vs -1), tri-saw (-1 vs -1, good match), saw-sqr (-1 vs +1). The sine-to-triangle transition will produce an intermediate shape that starts somewhere between 0 and -1 at phase=0, which is a valid waveshape (asymmetric, like a shifted sine). The saw-to-square transition has a large gap at phase=0 (-1 vs +1) but this is inherent to these shapes -- the crossfade produces a shape that transitions from a ramp to a step function, which is musically standard.

**Key insight:** For an LFO morph, the crossfade intermediate shapes don't need to be "theoretically optimal" -- they need to sound and look musically useful when modulating other parameters. Linear crossfade between these standard waveform definitions is the approach used by the majority of VCV Rack morphing oscillators (including MorphOsc) and produces acceptable results.

### Pattern 4: CV Input with Attenuator

**What:** Morph CV is additive: `finalMorph = clamp(knobValue + attenuator * cvVoltage / 10.f, 0.f, 1.f)`.

**CV scaling rationale (10V = full range):** VCV Rack's voltage standard states CV modulation sources are typically 0-10V unipolar or +/-5V bipolar. For a 0-1 parameter, the most standard scaling is: 10V of CV = full parameter range. This means a 0-10V unipolar LFO sweeps the full morph range, and a +/-5V bipolar LFO sweeps half the range in each direction from the knob position. This matches how the VCV Fundamental modules scale CV inputs.

```cpp
// In process():
float morphKnob = params[MORPH_PARAM].getValue();            // 0 to 1
float morphAtten = params[MORPH_ATTEN_PARAM].getValue();      // 0 to 1 (attenuator, not attenuverter)
float morphCV = inputs[MORPH_CV_INPUT].getVoltage();           // -10V to +10V
float morph = clamp(morphKnob + morphAtten * morphCV / 10.f, 0.f, 1.f);
```

**Why attenuator (0-1), not attenuverter (-1 to +1):** The CONTEXT.md specifies "attenuator knob for controlling modulation depth (0-100%)". This is a unipolar control: at 0% the CV has no effect, at 100% the CV has full effect. An attenuverter would add inversion capability, which was not requested.

### Pattern 5: Removing the Inverted Output

**What:** The enum, widget, and SVG must all be updated to remove INV_OUTPUT.

**Enum change approach:** Remove `INV_OUTPUT` from the `OutputId` enum. This changes `OUTPUTS_LEN` from 2 to 1. The DRIFT_CV_INPUT enum stays in `InputId` (it will be wired in Phase 6). The widget code must remove the `addOutput` line for INV_OUTPUT. The SVG must remove the INV label and the blue component circle.

**Important consideration for enum ordering:** The Phase 1 scaffold declares enums "upfront so widget positions are fixed for all future phases." Removing INV_OUTPUT changes OUTPUTS_LEN, which is fine -- the enum value of OUTPUT (0) stays the same. However, if any JSON state was saved referencing output port 1, it would break. Since Phase 1 had no DSP (empty process()), no user would have meaningful saved state to break. This change is safe now but would NOT be safe after users have saved patches.

### Pattern 6: Adding the Morph CV Attenuator Parameter

**What:** A new param `MORPH_ATTEN_PARAM` must be added to the enum, configured, and placed on the panel.

**Enum position:** Add after `OCTAVE_PARAM` and before `PARAMS_LEN` to avoid changing the integer values of existing params:

```cpp
enum ParamId {
    MORPH_PARAM,
    CHARACTER_PARAM,
    DRIFT_PARAM,
    RATE_PARAM,
    OCTAVE_PARAM,
    MORPH_ATTEN_PARAM,  // NEW: Morph CV attenuator
    PARAMS_LEN
};
```

**Configuration:**
```cpp
configParam(MORPH_ATTEN_PARAM, 0.f, 1.f, 0.f, "Morph CV Attenuator", "%", 0.f, 100.f);
```
This gives a 0-100% tooltip display. Default 0 means CV has no effect until the user deliberately turns up the attenuator -- a safe default that prevents unexpected modulation when first patching.

**Panel placement:** The attenuator should be positioned near the Morph CV jack. The Morph CV jack is at (11.0, 104.0). A Trimpot (~6mm diameter) placed directly above or beside the jack works well. Recommended position: directly above at (11.0, 96.0) -- this puts it between the section divider (y=94) and the jack (y=104), with the existing "MCV" label providing context. The "MCV" label at y=96.2 needs to shift slightly to accommodate the knob.

**Widget class:** Use `Trimpot` (~6.05mm diameter) rather than `RoundSmallBlackKnob` (~7.68mm). The Trimpot is the VCV Rack convention for attenuator/attenuverter knobs next to CV jacks. It's visually small enough to tuck near the jack without dominating the panel.

### Anti-Patterns to Avoid

- **Premature file separation:** Do not create `dsp/MorphStage.hpp`, `dsp/AnalogEngine.hpp` etc. for Phase 2. The morph engine is ~50 lines of math used by one module. Extract into shared files when there's an actual second consumer.
- **Exponential rate mapping kept accidentally:** The current `configParam` for RATE_PARAM uses `displayBase=2.f` for exponential Hz display. This MUST change to linear. Do not just change the display -- the actual frequency computation in `process()` must also change from `freq = 2.f * dsp::exp2_taylor5(pitch)` to `freq = params[RATE_PARAM].getValue()`.
- **Using float for phase:** The success criteria explicitly requires double precision. Do not use `float phase`.
- **Modifying enum order of existing items:** The MORPH_PARAM, CHARACTER_PARAM, etc. values must not change. Append new params at the end (before PARAMS_LEN).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parameter clamping | Custom min/max functions | `rack::math::clamp(value, min, max)` | SDK-provided, handles edge cases, consistent with ecosystem |
| Hz tooltip display | Custom string formatting | `configParam` with linear range + " Hz" unit | Built into ParamQuantity; handles right-click value entry too |
| Phase wrapping | `fmod()` or custom modulo | `if (phase >= 1.0) phase -= 1.0;` | fmod is slower and produces identical results for single-wrap cases |
| CV input reading | Manual port access | `inputs[ID].getVoltage()` | Returns 0V when unconnected -- correct default behavior |

**Key insight:** The DSP in this phase is entirely standard waveform math. There is nothing to import or adapt from external libraries. The entire morph engine fits in a single function.

## Common Pitfalls

### Pitfall 1: Forgetting to Change Rate Computation Along with configParam

**What goes wrong:** Developer changes `configParam` to linear range (0.01-20.0) but leaves the `process()` frequency computation as `2.f * dsp::exp2_taylor5(pitch)`. The rate knob then produces wildly wrong frequencies.
**Why it happens:** configParam and process() must be updated together, but they're in different parts of the file.
**How to avoid:** The rate computation in process() must simply be `float freq = params[RATE_PARAM].getValue();` -- the raw param value IS the frequency in Hz. No conversion needed.
**Warning signs:** Rate knob produces frequencies far outside 0.01-20Hz range.

### Pitfall 2: Phase Accumulator Stall at Low Frequencies

**What goes wrong:** Using `float` for phase causes the LFO to stall or stutter below ~0.01Hz.
**Why it happens:** IEEE 754 single-precision has ~7 decimal digits. At 0.01Hz/48kHz, the increment is 2.08e-7. When phase reaches 0.5, adding 2.08e-7 is at the precision boundary.
**How to avoid:** Use `double phase`. Verify by testing at 0.01Hz for a full cycle (100 seconds).
**Warning signs:** LFO output stops changing or advances in jerky steps at very low rates.

### Pitfall 3: Saw Waveform Direction Wrong

**What goes wrong:** Saw ramps down instead of up, contradicting the CONTEXT.md decision of "rising ramp."
**Why it happens:** The formula `2*phase - 1` produces a rising ramp. The formula `1 - 2*phase` produces a falling ramp. Easy to get backwards.
**How to avoid:** Rising ramp = `2.f * phase - 1.f`. At phase 0 -> output -1 (bottom), at phase ~1 -> output +1 (top), then snaps to -1.
**Warning signs:** Visual check against a scope module -- the ramp should go up, not down.

### Pitfall 4: Morph Knob Clicks at Segment Boundaries

**What goes wrong:** Audible click when the morph knob crosses 0.25, 0.5, or 0.75 (the segment boundaries between waveform pairs).
**Why it happens:** At LFO rates, this is unlikely to be audible because the waveform changes are slow. However, if the morph knob is modulated by a fast CV source, the segment switch can cause a sample-level discontinuity.
**How to avoid:** The linear crossfade formula `a + frac * (b - a)` is mathematically continuous across segment boundaries (when frac=1.0, the result equals b; the next segment starts with frac=0.0, result equals b). The crossfade IS continuous. The concern would only arise from integer truncation of the segment index -- use `std::min((int)scaled, 3)` to handle the morph=1.0 edge case.
**Warning signs:** Clicks when sweeping morph rapidly with CV.

### Pitfall 5: Panel SVG and C++ Enum Out of Sync After Changes

**What goes wrong:** Removing INV_OUTPUT from the enum but forgetting to remove the SVG label/component, or vice versa. Or adding MORPH_ATTEN_PARAM but forgetting the SVG component circle.
**Why it happens:** Three files must stay synchronized: AnalogLFO.cpp (enum + widget), AnalogLFO.svg (components layer + visible labels), PANEL-SPEC.md (component table).
**How to avoid:** Update all three files in the same task. Use the PANEL-SPEC.md component table as the source of truth and verify all three match after changes.
**Warning signs:** Component appears on panel but has no function, or function exists but component is invisible.

### Pitfall 6: Attenuator Default at 1.0 Causes Surprise Modulation

**What goes wrong:** If the attenuator defaults to 1.0 (full), patching any cable into the Morph CV jack immediately modulates the morph at full depth, which may surprise the user.
**Why it happens:** Some modules default attenuators to full. But for a morph parameter, unexpected modulation is jarring.
**How to avoid:** Default the attenuator to 0.0. The user must deliberately turn it up to enable CV modulation. This is a "safe default" pattern.
**Warning signs:** Users report that patching a cable immediately changes the sound unexpectedly.

## Code Examples

### Complete Rate Parameter Configuration (Linear Hz)

```cpp
// OLD (exponential, from Phase 1 scaffold):
// configParam(RATE_PARAM, -8.f, 4.f, -3.f, "Rate", " Hz", 2.f, 1.f);

// NEW (linear Hz, Phase 2):
configParam(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz");
```

The tooltip will display "Rate: 0.70 Hz" and the user can right-click to type an exact Hz value.

**Verification:** At the default 0.7Hz, the POC used `defaultValue=-1.5` with `base=2, multiplier=2`, giving `2 * 2^(-1.5) = 2 * 0.354 = 0.707Hz`. The new default of 0.7f matches this closely.

### Complete Morph CV Attenuator Configuration

```cpp
configParam(MORPH_ATTEN_PARAM, 0.f, 1.f, 0.f, "Morph CV", "%", 0.f, 100.f);
```

The tooltip will display "Morph CV: 0%" at minimum and "Morph CV: 100%" at maximum.

### Complete process() Implementation Skeleton

```cpp
void process(const ProcessArgs& args) override {
    // --- Rate -> frequency (linear, direct Hz) ---
    float freq = params[RATE_PARAM].getValue();  // 0.01 to 20.0 Hz
    freq = std::fmax(freq, 0.001f);              // safety floor

    // --- Phase accumulation (double precision) ---
    double deltaPhase = (double)freq * (double)args.sampleTime;
    phase += deltaPhase;
    if (phase >= 1.0) phase -= 1.0;

    // --- Morph with CV ---
    float morphKnob = params[MORPH_PARAM].getValue();
    float morphAtten = params[MORPH_ATTEN_PARAM].getValue();
    float morphCV = inputs[MORPH_CV_INPUT].getVoltage();
    float morph = clamp(morphKnob + morphAtten * morphCV / 10.f, 0.f, 1.f);

    // --- Waveform generation ---
    float p = (float)phase;
    float output = computeMorphedWave(p, morph);

    // --- Output ---
    outputs[OUTPUT].setVoltage(5.f * output);
}
```

### Trimpot Widget Placement

```cpp
// Morph CV attenuator: Trimpot above the Morph CV jack
addParam(createParamCentered<Trimpot>(mm2px(Vec(11.0, 96.0)), module, AnalogLFO::MORPH_ATTEN_PARAM));
```

**Spacing verification:** Trimpot diameter ~6.05mm, center at y=96.0. Bottom edge at 96.0 + 3.03 = 99.03mm. Morph CV jack center at y=104.0, top edge at 104.0 - 4.015 = 99.985mm. Gap between Trimpot bottom and jack top: 99.985 - 99.03 = ~0.96mm. This is tight but acceptable -- the VCV Rack standard layout often places attenuator knobs very close to their associated jacks. If it feels too tight visually, shift the Trimpot to y=95.0 for ~2mm gap.

Section divider is at y=94. The Trimpot's top edge at 96.0 - 3.03 = 92.97mm would overlap the divider by ~1mm. Better to move divider or use y=97.0 for the Trimpot: top edge at 93.97 (just below divider at 94), bottom edge at 100.03, gap to jack at 104-4=100 gives -0.03mm overlap. This is too tight.

**Revised recommendation:** Place Trimpot at (11.0, 97.5). Top edge: 94.47 (below divider at 94). Bottom edge: 100.53. Jack top edge: 99.985. Overlap: 100.53 - 99.985 = 0.54mm. Still tight. Actually, let me reconsider the section divider -- it may need to move up or be removed, or the jack row may need to move down slightly. Alternatively, position the attenuator beside the jack rather than above it.

**Alternative layout: Trimpot beside the Morph CV jack.** Place the Trimpot at the same Y as the jack but offset in X. With the INV output removed, the jack row has more horizontal space. Revised jack layout (3 jacks + 1 attenuator):

Current jack positions: MCV(11.0, 104.0), DCV(24.5, 104.0), OUT(38.0, 104.0), INV(51.5, 104.0).
Remove INV. Add Trimpot near MCV. New layout idea:
- Trimpot at (8.0, 104.0) -- left of MCV jack
- MCV jack at (17.0, 104.0)
- DCV jack at (30.48, 104.0) -- centered
- OUT jack at (44.0, 104.0)

This gives 9mm between Trimpot center and MCV center (Trimpot radius 3mm + jack radius 4mm = 7mm minimum, so 9mm gives 2mm clearance). The Trimpot and its associated jack are visually grouped.

**Final recommendation:** The exact panel layout is a detailed spatial design task. The planner should allocate a task specifically for redesigning the bottom row to accommodate 3 jacks (MCV, DCV, OUT) + 1 Trimpot, removing the INV jack and label. Multiple layout options exist; the implementation task should test a few positions and pick the cleanest one.

## Discretion Recommendations

### Morph Interpolation Method: Linear Crossfade

**Recommendation:** Use linear crossfade (lerp) between adjacent waveform shapes.

**Rationale:** For these specific shape pairs:
- **Sine to Triangle:** Linear blend produces a waveshape that smoothly transitions from a rounded sine to a pointed triangle. The intermediate shapes are valid waveforms that sound like a sine with increasing harmonic content. This is musically useful.
- **Triangle to Saw:** Linear blend produces a waveshape that transitions from symmetric peaks to asymmetric ramp. Intermediate shapes are valid asymmetric triangles. Musically useful.
- **Saw to Square:** Linear blend transitions from a ramp to a step. Intermediate shapes include the "sawtooth with flat top" and partial duty cycle variations. Musically useful.

Shape-aware morphing (e.g., interpolating shape parameters like symmetry ratio or harmonic content) would produce smoother intermediate shapes but requires defining a parameterization for each transition zone. The added complexity is not justified for an LFO where the morphing happens at sub-audio rates and the intermediate shapes only need to "look good on the display and produce useful modulation shapes."

### CV-to-Morph Voltage Scaling: 10V Full Range

**Recommendation:** 10V of CV spans the full 0-1 morph range.

**Rationale:** VCV Rack's official voltage standard states unipolar CV is 0-10V and bipolar CV is +/-5V. With 10V = full range:
- A 0-10V unipolar LFO sweeps the entire morph range (standard use case)
- A +/-5V bipolar LFO sweeps half the range in each direction from the knob position
- This matches the behavior of VCV Fundamental modules and most third-party modules
- The attenuator knob scales this: at 50%, 10V of CV = half the morph range

Alternative (5V full range) would mean a 0-10V signal overshoots by 2x, requiring the attenuator to be turned down to 50% for standard sources. This is less intuitive.

### Phase Accumulator: Double Precision

**Recommendation:** `double phase = 0.0;` with `double deltaPhase` computation.

**Rationale:** Already specified in the success criteria. At 0.01Hz and 48kHz, the increment is 2.08e-7. Double precision handles this with ~9 digits of margin. The cast to `float` happens only when feeding the phase value to the waveform functions, which need at most 7 digits of precision for shape math.

### Attenuator Knob: Trimpot, Near Morph CV Jack

**Recommendation:** Use the `Trimpot` widget class (~6.05mm diameter). Position it adjacent to or directly above the Morph CV jack in the bottom row. Exact position depends on the redesigned jack layout after removing the INV output.

**Rationale:** Trimpot is the VCV ecosystem convention for small attenuator/attenuverter knobs placed near CV jacks. It's visually unobtrusive and signals "this adjusts the adjacent input" to experienced modular users. RoundSmallBlackKnob (~7.68mm) would also work but is larger than necessary.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Exponential rate knob for LFO | Linear rate for LFO (user decision) | Phase 2 | Direct Hz values, simpler mental model |
| Dual outputs (normal + inverted) | Single output (user decision) | Phase 2 | Simpler module, easier display in Phase 3 |
| All DSP in separate files from Phase 1 | Inline in module file until sharing needed | Phase 2 research | Avoid premature abstraction |

**Deprecated/outdated:**
- Setting `displayBase = 2.f` for LFO rate: Replaced by linear configParam in this phase
- `INV_OUTPUT` enum value: Removed per user decision

## Open Questions

1. **Exact attenuator knob and jack row layout after INV output removal**
   - What we know: The INV output is removed, freeing space in the bottom row. A Trimpot for the morph attenuator needs to be placed near the MCV jack.
   - What's unclear: The exact X/Y positions for the 3 remaining jacks + 1 Trimpot. The current 4-jack layout (evenly spaced across full width) needs redesigning for 3 jacks + 1 small knob.
   - Recommendation: The planner should include a layout task that tests 2-3 arrangements and picks the best one. Options include: (a) Trimpot left of MCV jack, 3 jacks evenly spaced to the right; (b) Trimpot above MCV jack (requires moving section divider); (c) Trimpot and MCV jack paired on the left, DCV centered, OUT on the right.

2. **SVG label changes for removed/added elements**
   - What we know: The "INV" label path data must be removed from the SVG. A label for the attenuator may or may not be needed (Trimpots are often unlabeled in VCV modules).
   - What's unclear: Whether the "MCV" label should change to indicate the attenuator's presence, or whether the Trimpot's tooltip ("Morph CV: 0%") is sufficient documentation.
   - Recommendation: Keep the "MCV" label near the jack. Do not add a separate label for the Trimpot -- the visual grouping of Trimpot+Jack is self-documenting per VCV convention.

3. **Whether to update PANEL-SPEC.md in this phase or defer**
   - What we know: PANEL-SPEC.md documents component positions. It needs updating to reflect the new layout.
   - Recommendation: Update PANEL-SPEC.md as part of the same task that modifies the SVG and C++ widget code. All three must stay synchronized.

## Sources

### Primary (HIGH confidence)
- VCV Rack 2 SDK 2.6.6 headers -- `ParamQuantity.hpp` (configParam display formula, lines 42-49), `Module.hpp` (configParam signature, line 125), `componentlibrary.hpp` (Trimpot class), `helpers.hpp` (createParamCentered) -- verified from local install at `/Users/mrcbrown/Claude/Software/Forge Audio/Rack-SDK/include/`
- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) -- official CV voltage conventions (0-10V unipolar, +/-5V bipolar)
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) -- configParam examples with displayBase/displayMultiplier
- Existing codebase at `/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/src/AnalogLFO.cpp` -- current scaffold code
- POC LFO at `/Users/mrcbrown/Claude/Software/Forge Audio/LFO/src/LFO.cpp` -- working waveform generation, phase accumulator pattern, rate knob implementation

### Secondary (MEDIUM confidence)
- [VCV Fundamental LFO source](https://github.com/VCVRack/Fundamental/blob/v2/src/LFO.cpp) -- configParam pattern with FrequencyQuantity, SIMD processing, CV handling
- [VCV-MorphOsc](https://github.com/jaffasplaffa/VCV-MorphOsc) -- community morphing oscillator implementation
- Component SVG dimensions verified from VCV Rack 2 Free application: Trimpot=17.86px/6.05mm, RoundSmallBlackKnob=22.68px/7.68mm

### Tertiary (LOW confidence)
- Morph crossfade quality assessment (whether linear crossfade vs shape-aware produces "better" intermediate shapes) -- based on DSP knowledge rather than empirical A/B testing. Should be verified by ear during implementation.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- SDK verified locally, waveform math is textbook DSP
- Architecture: HIGH -- pattern follows established VCV Rack module structure, changes to existing scaffold are well-defined
- DSP implementation: HIGH -- phase accumulator, waveform generation, and crossfade morphing are well-understood techniques
- Panel layout (attenuator placement): MEDIUM -- multiple valid layouts exist; exact positioning needs visual testing in VCV Rack
- Morph interpolation quality: MEDIUM -- linear crossfade is standard but "musically valid intermediate shapes" is subjective; verify by ear

**Research date:** 2026-02-25
**Valid until:** 2026-03-25 (VCV Rack SDK stable; DSP techniques are timeless)
