# Technology Stack: v1.2 Deep Analog Additions

**Project:** Forge Audio Analog Series -- v1.2 Deep Analog milestone
**Researched:** 2026-03-13
**Confidence:** HIGH (all additions use VCV Rack SDK built-ins, C++ standard library, and hand-rolled DSP algorithms with no external dependencies)

## Scope

This document covers ONLY stack additions and changes for v1.2 features. The validated v1.0/v1.1 stack (VCV Rack 2 SDK, C++17, NanoVG display, nanosvg panel, GNU Make/plugin.mk, lock-free double buffer, OU drift engine, SchmittTrigger/Timer clock tracking, ExponentialFilter period smoothing) is unchanged. No new dependencies are introduced.

---

## New SDK Components Needed

### FM Input: Exponential Frequency Modulation

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `std::pow(2.f, ...)` (C++ `<cmath>`) | C++17 standard | Convert FM CV voltage to exponential frequency multiplier | FM at LFO rates should be exponential (V/Oct style), not linear. Exponential FM modulates pitch by equal musical intervals up and down: `freq *= std::pow(2.f, fmVoltage * fmDepth)`. This matches VCV Rack voltage standards where 1V = 1 octave. At sub-audio rates, exponential FM produces musically useful vibrato/warble. Linear FM (`freq += voltage * index`) is for audio-rate timbral FM -- inappropriate for an LFO where users expect pitch-symmetric modulation. |

**Implementation pattern (exponential FM for LFO):**
```cpp
// In process(), after base frequency calculation:
float fmVoltage = inputs[FM_INPUT].getVoltage();  // +/-5V typical
float fmDepth = params[FM_ATTEN_PARAM].getValue(); // 0..1 attenuverter
float fmMultiplier = std::pow(2.f, fmVoltage * fmDepth);
freq *= fmMultiplier;
freq = rack::math::clamp(freq, 0.001f, 100.f);  // safety clamp
```

**Why NOT `dsp::exp2_taylor5()`:** The SDK's fast approximation is designed for SIMD polyphonic paths where accuracy trade-offs matter. Our LFO is monophonic and already uses `std::pow` elsewhere. One `std::pow(2.f, x)` per sample at LFO rates is negligible CPU cost. Use the standard library for correctness.

**Why NOT linear FM:** At sub-audio rates, linear FM shifts the mean frequency upward (asymmetric modulation). With exponential FM, +1V doubles frequency and -1V halves it -- musically symmetric intervals. This matches how every VCV Rack V/Oct input works and what users expect from a CV-controlled frequency input.

**FM depth attenuverter vs. attenuator:** Use an attenuator (0 to 1), not an attenuverter (-1 to 1). The FM input voltage is already bipolar. An attenuverter on FM depth would invert the modulation direction, which is confusing for LFO FM (unlike VCO FM where inversion enables through-zero FM effects). Keep it simple: depth 0 = no FM, depth 1 = full FM range.

### Separate RESET Jack: Trigger Detection

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `rack::dsp::SchmittTrigger` (second instance) | SDK built-in | Detect rising edge on RESET input independently from CLK | Already used for CLK input. Add a second `SchmittTrigger` instance for the RESET jack. Standard VCV pattern -- modules like Fundamental VCO use separate SchmittTrigger instances for sync and reset inputs. Use same thresholds (0.1V low, 1.0V high). |

**Implementation pattern:**
```cpp
dsp::SchmittTrigger resetTrigger;  // NEW member

// In process():
if (resetTrigger.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.0f)) {
    crossfadeFrom = lastOutputVoltage;  // reuse existing anti-click crossfade
    crossfadeProgress = 0.f;
    phase = 0.0 + (double)phaseOffset;  // reset to phase offset position
    clockBeatCount = 0;                 // reset division counter too
}
```

**Interaction with CLK:** RESET is independent from CLK. CLK resets phase on beat boundaries for sync; RESET forces immediate phase reset regardless of clock state. Both share the existing anti-click cosine crossfade mechanism. No new crossfade code needed.

### Phase Offset Knob/CV

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| Phase offset as additive constant | No dependency | Shift LFO output phase by 0-360 degrees | Phase offset is trivially implemented by adding a constant to the phase accumulator before waveform computation. No SDK utility needed. The offset applies to the read phase, not the accumulator, to avoid drift: `float p = std::fmod((float)phase + phaseOffset, 1.f)`. |

**Implementation pattern:**
```cpp
// In constructor:
configParam(PHASE_OFFSET_PARAM, 0.f, 1.f, 0.f, "Phase Offset", " deg", 0.f, 360.f);

// In process(), after phase accumulation:
float offsetKnob = params[PHASE_OFFSET_PARAM].getValue();
float offsetCV = inputs[PHASE_OFFSET_CV_INPUT].getVoltage();
float phaseOffset = rack::math::clamp(offsetKnob + offsetCV / 10.f, 0.f, 1.f);
float p = std::fmod((float)phase + phaseOffset, 1.f);
float sample = computeMorphedWave(p, morph, character);
```

**Why additive read-offset, not accumulator modification:** Modifying the phase accumulator with the offset would cause discontinuities when the offset changes (knob turn = phase jump = click). Instead, add the offset only at the point of waveform evaluation. The accumulator runs cleanly; the offset shifts the "window" into the waveform. This also means the display phase dot tracks the offset naturally.

---

## New Algorithms: No External Libraries

All v1.2 DSP features are hand-rolled algorithms using only C++ standard library math. No external DSP libraries are needed or recommended.

### Expanded Analog Imperfections

These four new imperfection layers integrate into the existing drift architecture. They are all simple per-sample computations.

#### 1. Phase Jitter

| Algorithm | Source | Purpose | Why This Approach |
|-----------|--------|---------|-------------------|
| OU process on phase accumulator | Extension of existing 4-layer OU drift | Random per-sample phase perturbation simulating capacitor noise | The existing drift engine applies OU noise to deltaPhase (frequency domain). Phase jitter adds a separate OU noise source directly to the phase readout. This models a different physical phenomenon: not pitch instability but timing uncertainty in the oscillator core. Use a single fast OU layer (~5-10 Hz bandwidth) with small amplitude (0.001-0.005 of a cycle). Scale with drift knob. |

```cpp
// New member:
OULayer phaseJitterLayer;  // ~8Hz, small sigma

// In process():
float jitter = phaseJitterLayer.state * phaseJitterLayer.weight;
float p = std::fmod((float)phase + phaseOffset + jitter * driftAmount, 1.f);
if (p < 0.f) p += 1.f;  // handle negative jitter wrapping
```

#### 2. DC Offset Drift

| Algorithm | Source | Purpose | Why This Approach |
|-----------|--------|---------|-------------------|
| Slow OU process on output voltage | Hand-rolled | Wandering DC offset simulating capacitor coupling and op-amp bias drift | Real analog oscillators have slowly wandering DC offset from aging capacitors and thermal drift in op-amp bias currents. Add a very slow OU layer (~0.02-0.05 Hz) that adds a small voltage offset to the final output. Maximum offset: ~50-100mV at full drift (1-2% of 5V output). This is subtle but measurable -- exactly how analog gear behaves. |

```cpp
// New member:
OULayer dcOffsetLayer;  // ~0.03Hz, very slow wander

// In process(), after waveform computation:
float dcOffset = dcOffsetLayer.state * dcOffsetLayer.weight * driftAmount;
float outputVoltage = 5.f * sample + dcOffset * 0.1f;  // max ~50mV offset
```

#### 3. Pitch Slew (Slew-Rate Limited Frequency)

| Algorithm | Source | Purpose | Why This Approach |
|-----------|--------|---------|-------------------|
| Exponential slew on frequency target | `b1 = exp(-1 / slewTime)` one-pole filter | Simulates VCO core that cannot change frequency instantaneously | Real analog VCOs have integrating capacitors that slew between frequencies. When the rate knob changes quickly or FM modulation is fast, the frequency should lag slightly behind. The existing `freqSlew` (ExponentialFilter with lambda=20) handles clock/free transitions. For analog pitch slew, add a second, lighter slew (lambda ~100-200, i.e. 5-10ms time constant) that runs always, simulating the capacitor charging time of an analog VCO core. Scale the slew effect with the character knob (digital = instant, analog = slewed). |

```cpp
// Reuse existing SDK utility:
dsp::TExponentialFilter<float> analogPitchSlew;

// In constructor:
analogPitchSlew.setLambda(150.f);  // ~6.7ms time constant

// In process():
float slewedFreq;
if (character > 0.001f) {
    float slewAmount = progressiveCurve(character);
    float lambda = rack::math::rescale(slewAmount, 0.f, 1.f, 1000.f, 100.f);
    analogPitchSlew.setLambda(lambda);
    slewedFreq = analogPitchSlew.process(args.sampleTime, freq);
} else {
    slewedFreq = freq;
    analogPitchSlew.out = freq;  // keep filter primed
}
```

**Why tie to character knob, not drift knob:** Pitch slew is about the oscillator core's response characteristics (hardware design), not random instability. Character models "what kind of hardware is this" while drift models "how unstable is it." A Minimoog's VCO core has inherent slew; that is character, not drift.

#### 4. Component Spread (Per-Instance Tolerance Variation)

| Algorithm | Source | Purpose | Why This Approach |
|-----------|--------|---------|-------------------|
| Truncated Gaussian parameter offsets, initialized once per module instance | Jatin Chowdhury's "Bad Circuit Modelling" approach | Each module instance gets slightly different waveform characteristics | Real analog synths have component tolerances (resistors at +/-5%, capacitors at +/-10%). Two "identical" oscillators sound slightly different. Model this by generating per-instance random offsets at module construction using the existing Xoroshiro128Plus RNG. Apply as small multipliers to character parameters: duty cycle offset, triangle asymmetry offset, saw curvature offset, etc. These are fixed for the module lifetime (persist across saves via seed serialization), making each instance unique. |

```cpp
// New member:
struct ComponentSpread {
    float dutyOffset;      // +/-0.02 (2% variation)
    float triangleAsym;    // +/-0.015
    float sawCurvature;    // +/-0.02
    float sineThd;         // +/-0.01
};
ComponentSpread spread;

// In constructor, after RNG seeding:
auto gaussRand = [&]() -> float {
    return rack::math::clamp(normalDist(rng) * 0.33f, -1.f, 1.f);
};
spread.dutyOffset = gaussRand() * 0.02f;
spread.triangleAsym = gaussRand() * 0.015f;
spread.sawCurvature = gaussRand() * 0.02f;
spread.sineThd = gaussRand() * 0.01f;
```

**Why truncated Gaussian, not uniform:** Real component values cluster around the nominal with a Gaussian distribution, but extreme outliers are rejected by quality control (the "truncation"). Clamp at +/-1 sigma times the tolerance. Uniform distribution would over-represent extreme values.

**Serialization consideration:** To make each module instance consistently unique across patch saves, serialize the RNG seed (two uint64_t values) via `dataToJson`/`dataFromJson`. This is a deliberate departure from the v1.0 decision of "no OU state serialization" because component spread represents fixed hardware characteristics, not random runtime behavior. The OU drift layers should remain uninitialized on load (analog-authentic randomness).

### Waveform Bleed During Morph Transitions

| Algorithm | Source | Purpose | Why This Approach |
|-----------|--------|---------|-------------------|
| Widened crossfade window with secondary shape mixing | Hand-rolled | Adjacent waveforms partially leak into morph position | Currently, morphing uses a linear crossfade between adjacent shapes with a hard boundary at integer morph positions. Real analog morphers (like the Prophet VS vector joystick) have cross-coupling where adjacent shapes bleed into each other. Implement by widening the crossfade region so that at morph=0.5 (tri position), a trace of sine and saw is present. This is NOT equal-power crossfade -- it is a wider transition window where secondary shapes contribute at reduced amplitude. |

```cpp
// Modified computeMorphedWave with bleed:
float computeMorphedWave(float phase, float morph, float character, float bleedAmount) {
    float sine = computeSine(phase, character);
    float tri  = computeTriangle(phase, character);
    float saw  = computeSaw(phase, character);
    float sqr  = computeSquare(phase, character);
    float shapes[4] = {sine, tri, saw, sqr};

    if (bleedAmount < 0.001f) {
        // Original behavior (no bleed)
        // ... existing segment crossfade code ...
    }

    // With bleed: each shape gets a weight based on distance from morph position
    float scaled = morph * 3.f;
    float result = 0.f;
    float totalWeight = 0.f;
    for (int i = 0; i < 4; i++) {
        float dist = std::fabs(scaled - (float)i);
        float weight = std::fmax(0.f, 1.f - dist / (1.f + bleedAmount * 2.f));
        weight *= weight;  // quadratic falloff for natural bleed
        result += shapes[i] * weight;
        totalWeight += weight;
    }
    return result / totalWeight;  // normalize to prevent amplitude change
}
```

**Why tie to character knob:** Waveform bleed is a property of the morphing circuit's impedance matching and buffer isolation. More "analog" character = more bleed. At character=0 (digital), morphing is crisp crossfades. At character=1, adjacent shapes leak ~5-10%.

### Swing/Shuffle for Clocked Mode

| Algorithm | Source | Purpose | Why This Approach |
|-----------|--------|---------|-------------------|
| Even-beat phase offset within LFO cycle | Standard swing algorithm (TR-909 pattern) | Delay every other beat to create rhythmic groove | Swing works by alternating the timing of even-numbered subdivisions. At 50% swing = straight time. At 66.7% = triplet feel. At 75% = dotted-note feel. For a clocked LFO, swing modifies the phase accumulation rate: advance faster during the first half of each beat pair, slower during the second half. This creates the characteristic "long-short" pattern without changing the overall cycle length. |

**Implementation approach:**

Swing at the LFO level is conceptually different from swing on a sequencer. The LFO produces a continuous waveform, not discrete steps. Swing should subtly warp the phase progression so that the waveform "lingers" on the first half and "rushes" through the second half of each beat pair.

```cpp
// New member:
float swingAmount = 0.5f;  // 0.5 = straight, 0.67 = triplet, 0.75 = dotted

// In process(), when clocked:
if (isClocked && swingAmount > 0.501f) {
    // Determine position within beat pair (0..1 over two beats)
    float beatPairPhase = std::fmod((float)phase * 2.f, 1.f);
    // Phase warping: stretch first half, compress second half
    float warpedRate;
    if (beatPairPhase < swingAmount) {
        warpedRate = 0.5f / swingAmount;       // slower (stretch)
    } else {
        warpedRate = 0.5f / (1.f - swingAmount); // faster (compress)
    }
    deltaPhase *= warpedRate;
}
```

**Constraint:** Swing only applies in clocked mode. In free-running mode, there are no "beats" to swing against. The swing parameter should have no effect (or be hidden/grayed) when CLK is not connected.

**Why NOT a separate swing clock:** Some modular approaches use a gate delay on alternate triggers. That pattern is for sequencers and drum machines. An LFO produces a continuous signal -- swing must warp the phase progression, not delay triggers.

---

## Display Enhancements

### Text Overlay Readability (HUD Backgrounds)

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `nvgTextBounds()` + `nvgRoundedRect()` | NanoVG (SDK bundled) | Measure text extents, draw semi-transparent pill backgrounds | The current text overlays (SYNC badge, ratio label, BPM readout, Hz readout) use glow rendering but can be hard to read when the waveform trace passes behind them. Solution: measure each text string's bounds with `nvgTextBounds()`, then draw a small semi-transparent rounded rectangle behind it before rendering the text. This is the standard "HUD pill" pattern used throughout VCV Rack's own modules. |

```cpp
void drawHudText(NVGcontext* vg, int fontHandle, float x, float y,
                 const char* text, float fontSize, int align, float alpha) {
    nvgFontFaceId(vg, fontHandle);
    nvgFontSize(vg, fontSize);
    nvgTextAlign(vg, align);

    // Measure text bounds
    float bounds[4];
    nvgTextBounds(vg, x, y, text, NULL, bounds);

    // Draw semi-transparent pill background
    float pad = 2.f;
    nvgBeginPath(vg);
    nvgRoundedRect(vg,
        bounds[0] - pad, bounds[1] - pad,
        (bounds[2] - bounds[0]) + 2.f * pad,
        (bounds[3] - bounds[1]) + 2.f * pad,
        2.f);
    nvgFillColor(vg, nvgRGBAf(0.051f, 0.051f, 0.102f, 0.75f * alpha));
    nvgFill(vg);

    // Render text on top (keep existing glow technique)
    drawGlowText(vg, fontHandle, x, y, text, fontSize, align, alpha);
}
```

**Background color:** Use the display background color (`0.051, 0.051, 0.102`) at ~75% opacity. This makes the pill blend seamlessly with the display when no waveform is behind it, but provides contrast when the amber waveform trace overlaps. Avoid pure black (looks harsh) or panel color (looks disconnected).

### Incoming Clock BPM Display

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| No new technology | -- | Show raw clock BPM alongside effective (ratio-adjusted) BPM | Use existing `displaySmoothedPeriod` atomic. Calculate `clockBPM = 60.f / smoothedPeriod`. Display as "CLK: 120" or similar alongside the existing ratio-adjusted BPM. Both use the same NanoVG text rendering. The only new code is a second BPM text string in `drawTextOverlays()`. |

---

## New Enum Members and Parameters

### Inputs (add to InputId enum)

```cpp
enum InputId {
    MORPH_CV_INPUT,
    DRIFT_CV_INPUT,
    CHARACTER_CV_INPUT,
    CLK_INPUT,
    FM_INPUT,              // NEW: FM frequency modulation
    RESET_INPUT,           // NEW: independent phase reset
    PHASE_OFFSET_CV_INPUT, // NEW: phase offset CV
    INPUTS_LEN
};
```

### Parameters (add to ParamId enum)

```cpp
enum ParamId {
    MORPH_PARAM,
    CHARACTER_PARAM,
    DRIFT_PARAM,
    RATE_PARAM,
    MORPH_ATTEN_PARAM,
    CHARACTER_ATTEN_PARAM,
    DRIFT_ATTEN_PARAM,
    PHASE_OFFSET_PARAM,   // NEW: 0..1 (0..360 degrees)
    FM_ATTEN_PARAM,       // NEW: FM depth attenuator (0..1)
    SWING_PARAM,          // NEW: swing amount (0.5..0.75, default 0.5)
    PARAMS_LEN
};
```

### Panel Space Consideration

Adding 3 new jacks (FM, RESET, PHASE_OFFSET_CV) and 3 new controls (PHASE_OFFSET knob, FM_ATTEN trimpot, SWING knob/trimpot) to a 12HP panel is tight but feasible. The existing layout has:
- 3 large knobs (morph, character, drift) in the mid-section
- 1 medium knob (rate) + 1 jack (CLK) in the rate row
- 3 trimpots + 3 CV jacks + 1 output jack in the bottom section

**Recommended layout approach:** Add PHASE_OFFSET as a small knob near the RATE knob. Add FM and RESET jacks in a new row or alongside existing jacks. FM_ATTEN as a trimpot near FM jack. SWING as a trimpot in the bottom section (only meaningful in clocked mode). This may require rearranging the bottom section or using smaller widget sizes.

If 12HP becomes too crowded, consider: (a) making SWING a right-click menu parameter instead of a panel control, or (b) expanding to 14HP. Research the actual mm spacing during implementation.

---

## Components NOT Needed

These were considered and explicitly rejected:

| Technology | Why NOT Needed |
|------------|----------------|
| External DSP libraries (STK, Faust, etc.) | All v1.2 algorithms are 5-15 lines of C++ each. Adding a library dependency for simple OU noise, exponential FM, or phase offset is over-engineering. The existing codebase has zero external dependencies and should stay that way. |
| `dsp::exp2_taylor5()` for FM | SIMD-optimized approximation for polyphonic modules. Our monophonic LFO benefits more from `std::pow(2.f, x)` accuracy at negligible CPU cost difference. |
| Linear FM / through-zero FM | Designed for audio-rate VCOs where harmonic sidebands are the goal. At LFO rates, exponential FM gives musically useful vibrato/warble. Linear FM would produce asymmetric pitch modulation that feels wrong at sub-audio rates. Defer TZFM to the VCO module (v2.0). |
| FM attenuverter (bipolar depth) | FM input voltage is already bipolar. Attenuverting the depth would invert the modulation direction, which is confusing for an LFO. Use a unipolar attenuator (0 to 1). Reserve attenuverters for the VCO module where through-zero FM makes inversion meaningful. |
| PLL for phase tracking | Still overkill. Simple edge measurement + EMA continues to be sufficient for LFO clock tracking. The v1.2 features do not change this assessment. |
| `rack::dsp::SlewLimiter` for pitch slew | `ExponentialFilter` is already in use and provides the correct exponential convergence behavior. SlewLimiter has asymmetric rate capping which would sound wrong for analog pitch response. |
| External random number generators | The existing `Xoroshiro128Plus` RNG + `std::normal_distribution` handles all noise generation (OU layers, component spread, phase jitter). No need for additional RNG infrastructure. |
| `dsp::Decimator` or oversampling | Not relevant for sub-audio LFO signals. No aliasing concerns at 0.01-20 Hz output frequencies. Reserve for VCO module. |
| JSON serialization for OU state | v1.0 decision stands: OU drift state stays fresh on patch load. However, the NEW `ComponentSpread` offsets SHOULD be serialized (see below). |
| `std::mutex` or `std::condition_variable` | All new features run in the `process()` callback. New display atomics follow the established lock-free pattern. No synchronization primitives needed. |

---

## Serialization Changes

### New: Serialize Component Spread Seed

```cpp
json_t* dataToJson() override {
    json_t* root = json_object();
    // Serialize RNG seed for reproducible component spread
    json_object_set_new(root, "rngSeed0", json_integer((json_int_t)rngSeed0));
    json_object_set_new(root, "rngSeed1", json_integer((json_int_t)rngSeed1));
    return root;
}

void dataFromJson(json_t* root) override {
    json_t* s0 = json_object_get(root, "rngSeed0");
    json_t* s1 = json_object_get(root, "rngSeed1");
    if (s0 && s1) {
        rngSeed0 = (uint64_t)json_integer_value(s0);
        rngSeed1 = (uint64_t)json_integer_value(s1);
        rng.seed(rngSeed0, rngSeed1);
        regenerateComponentSpread();  // deterministic from saved seed
    }
}
```

Store the seed, not the spread values. This keeps serialization compact and allows the spread generation algorithm to evolve without breaking saved patches.

---

## Build Changes

**None.** No new source files, no new dependencies, no Makefile changes. All code goes into the existing `src/AnalogLFO.cpp`. The SVG panel file (`res/AnalogLFO.svg`) needs updating to add new jack and knob positions, following the same workflow as v1.1's CLK jack addition.

---

## Integration Order Recommendation

Based on dependency analysis, implement in this order:

1. **Display text HUD backgrounds** -- No DSP changes, pure display fix. Quick win, immediately visible improvement.
2. **Phase offset knob/CV** -- Simple additive offset, no interaction with other features.
3. **Separate RESET jack** -- Second SchmittTrigger instance, reuses existing crossfade. Independent from all other features.
4. **FM input** -- Exponential multiplier on frequency. Must integrate with existing frequency slew chain.
5. **Expanded imperfections** (phase jitter, DC offset, pitch slew, component spread) -- Four parallel features that all integrate into the existing drift/character pipeline. Implement together to test interactions.
6. **Waveform bleed** -- Modifies `computeMorphedWave()` signature. Must be done after imperfections are stable since both affect the output simultaneously.
7. **Swing/shuffle** -- Most complex interaction with clock system. Implement last when all other clock-interacting features (reset, phase offset) are settled.
8. **Clock BPM display** -- Pure display, depends on no DSP changes. Can slot anywhere but naturally last since it is the simplest remaining task.

---

## CPU Budget Assessment

Current module at v1.1: ~890 lines, primary cost is 4 OU layers + 4 waveform computations + crossfade per sample.

v1.2 additions per sample:
- FM: 1x `std::pow(2.f, x)` -- moderate (~15-20 cycles)
- Phase jitter: 1 additional OU layer -- negligible
- DC offset drift: 1 additional OU layer -- negligible
- Pitch slew: 1 ExponentialFilter update -- negligible
- Component spread: lookup of pre-computed offsets -- negligible
- Waveform bleed: 4 distance calculations + normalization -- light
- Swing phase warp: 1 conditional branch + multiply -- negligible
- Phase offset: 1 fmod + addition -- negligible

**Total estimated overhead:** ~5-10% increase over v1.1. The `std::pow` call is the most expensive single addition. All other features are cheaper than a single OU layer update. Well within CPU budget for a monophonic LFO module.

If profiling reveals `std::pow` is a concern (unlikely), replace with the polynomial approximation: `float exp2_fast(float x) { float y = 1.f + x * (0.6931472f + x * (0.2402265f + x * 0.0558011f)); return y; }` -- but only if measured, not preemptively.

---

## Sources

- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) -- 1V/Oct standard, trigger thresholds (0.1V/1.0V), cable delay model
- [VCV Rack API: dsp namespace](https://vcvrack.com/docs-v2/namespacerack_1_1dsp) -- SchmittTrigger, ExponentialFilter, exp2_taylor5 reference
- [VCV Rack DSP Manual](https://vcvrack.com/manual/DSP) -- general DSP guidance for module development
- [VCV Community: Oscillators with Phase Control](https://community.vcvrack.com/t/oscillators-with-phase-control/16534) -- phase offset implementation patterns in VCV modules
- [VCV Community: Reset Sequencers Timing](https://community.vcvrack.com/t/reset-sequencers-are-one-step-off/12898) -- cable delay and reset timing considerations
- [Frap Tools: Exponential FM Explained](https://frap.tools/frequency-modulation-part-1-exponential-fm/) -- why exponential FM is correct for pitch modulation
- [DAFx2020: Practical Linear and Exponential FM](https://dafx2020.mdw.ac.at/proceedings/papers/DAFx2020_paper_61.pdf) -- academic treatment of FM synthesis variants
- [Jatin Chowdhury: Bad Circuit Modelling -- Component Tolerances](https://ccrma.stanford.edu/~jatin/Bad-Circuit-Modelling/Tolerances.html) -- truncated Gaussian distribution for per-instance variation
- [Jatin Chowdhury: Bad Circuit Modelling Paper](https://ccrma.stanford.edu/~jatin/papers/BadCircuitModels.pdf) -- academic paper on analog circuit imperfection modeling
- [Gearspace: Simulating Analog Oscillator Drift](https://gearspace.com/board/electronic-music-instruments-and-electronic-music-production/852329-simulating-analog-oscillator-drift.html) -- community techniques for drift and jitter simulation
- [KVR: Implementing Swing/Shuffle](https://www.kvraudio.com/forum/viewtopic.php?t=261858) -- DSP implementation patterns for swing timing
- [SuperCollider: Pattern Guide Swing](https://doc.sccode.org/Tutorials/A-Practical-Guide/PG_Cookbook08_Swing.html) -- algorithmic swing implementation reference
- [MOD WIGGLER: Adding Swing to Clock](https://modwiggler.com/forum/viewtopic.php?t=174713) -- modular synth swing approaches
- [NanoVG API: nanovg.h](https://github.com/memononen/nanovg/blob/master/src/nanovg.h) -- nvgTextBounds, nvgRoundedRect, nvgFontBlur API reference
- [NanoVG: Text Bounds Issues](https://github.com/memononen/nanovg/issues/41) -- layout calculation with nvgTextBounds
- Existing codebase: `src/AnalogLFO.cpp` (directly verified, 890 lines, v1.1)

---
*Stack research for: Forge Audio Analog Series v1.2 Deep Analog*
*Researched: 2026-03-13*
