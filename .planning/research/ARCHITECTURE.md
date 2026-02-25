# Architecture Patterns

**Domain:** Analog-modeled VCV Rack oscillator modules (LFO + VCO)
**Researched:** 2026-02-25
**Confidence:** HIGH (VCV Rack 2 SDK patterns are well-established; DSP techniques for analog modeling are mature; prior POC validates core approach)

## Recommended Architecture

### High-Level Overview

```
                    +---------------------------+
                    |     Shared Analog Engine   |
                    |       (AnalogEngine)       |
                    |                            |
                    |  +---------+  +---------+  |
           phase -->|  | Morph   |->| Charac- |  |
                    |  | Stage   |  | ter     |  |
                    |  +---------+  | Stage   |  |
                    |               +---------+  |
                    |                    |        |
                    |               +---------+  |
                    |               | Drift   |  |--> output sample
                    |               | Stage   |  |--> display buffer
                    |               +---------+  |
                    +---------------------------+
                              ^           ^
                              |           |
                    +--------+--+   +-----+-------+
                    | LFO Module |   | VCO Module  |
                    | (host)     |   | (host)      |
                    +------------+   +-------------+
```

The architecture separates concerns into three layers:

1. **Host Module** -- VCV Rack Module subclass that handles I/O, parameter mapping, phase accumulation, and module-specific features (sync, FM, frequency range).
2. **Analog Engine** -- A standalone C++ class (no VCV Rack dependencies) that takes a phase value and control parameters, and produces a shaped output sample. Shared identically between LFO and VCO.
3. **Waveform Display** -- A NanoVG-based custom widget that reads a snapshot buffer from the engine and renders a single-cycle waveform with a phase-tracking dot.

### Component Boundaries

| Component | Responsibility | Communicates With | Thread |
|-----------|---------------|-------------------|--------|
| `ForgeAnalogLFO` (Module) | Phase accumulation, rate knob, reset/sync, FM input, CV routing, output scaling | AnalogEngine (owns), Display (writes buffer) | Audio thread |
| `ForgeAnalogVCO` (Module) | Phase accumulation, V/Oct tracking, through-zero FM, hard sync, antialiasing | AnalogEngine (owns), Display (writes buffer) | Audio thread |
| `AnalogEngine` | Morph stage, character stage, drift stage, display buffer generation | Called by host module | Audio thread (called from process()) |
| `MorphStage` | Continuous waveform morphing sine-tri-saw-square | Called by AnalogEngine | Audio thread |
| `CharacterStage` | Crossfade from digital reference to analog reference waveforms | Called by AnalogEngine | Audio thread |
| `DriftProcessor` | Pitch drift, phase jitter, component spread, DC offset, HF rolloff, pitch slew | Called by AnalogEngine | Audio thread |
| `WaveformDisplay` (Widget) | NanoVG rendering of single-cycle waveform + phase dot | Reads snapshot buffer from Module | GUI thread |
| `LFOWidget` / `VCOWidget` | Panel layout, knobs, jacks, screws, display placement | Owns WaveformDisplay, references Module | GUI thread |

### Critical Boundary: Audio Thread vs GUI Thread

VCV Rack runs `Module::process()` on the audio thread and `Widget::draw()` on the GUI thread. These must never share data without proper synchronization. The architecture uses a **lock-free snapshot buffer** pattern:

- Audio thread writes a completed single-cycle waveform buffer (128-256 samples) and the current phase position into a double-buffered structure.
- GUI thread reads the most recently completed buffer without blocking the audio thread.
- An `std::atomic<int>` flag indicates which buffer is current.

## Data Flow

### Per-Sample DSP Pipeline

```
Host Module process():
  1. Read params + CV inputs
  2. Compute frequency (host-specific: rate knob for LFO, V/Oct for VCO)
  3. Apply pitch drift from DriftProcessor (modifies frequency)
  4. Apply pitch slew from DriftProcessor (smooths frequency changes)
  5. Advance phase accumulator: phase += freq * sampleTime
  6. Apply phase jitter from DriftProcessor (modifies phase)
  7. [VCO only] Handle hard sync (reset phase on sync trigger)
  8. [VCO only] Apply through-zero FM (add FM directly to phase increment)
  9. Wrap phase to [0, 1)
  10. Call engine.process(phase, morphPos, characterPos, driftAmount) -> output
  11. Scale output to voltage (+/-5V LFO, +/-5V VCO)
  12. Write to display buffer (every Nth sample or on phase wrap)

AnalogEngine::process(phase, morph, character, drift):
  1. MorphStage: generate morphed waveform from phase + morph position
  2. CharacterStage: crossfade between digital morph result and analog reference
  3. DriftProcessor: apply DC offset, HF rolloff to output
  4. Write to display snapshot buffer (if phase wrapped this sample)
  5. Return final output sample
```

### Display Data Flow

```
Audio Thread                          GUI Thread
-----------                          ----------
engine.process()
  -> fills displayBuffer[writeIdx]
  -> on phase wrap:
     atomic writeIdx flip            WaveformDisplay::draw()
                                       -> read displayBuffer[readIdx]
                                       -> render 128-256 point polyline
                                       -> draw phase dot at currentPhase
```

## Detailed Component Designs

### 1. AnalogEngine

```cpp
// src/dsp/AnalogEngine.hpp
#pragma once
#include <cmath>
#include <array>
#include <atomic>

struct AnalogEngineParams {
    float morph;      // 0.0 = sine, 0.33 = tri, 0.67 = saw, 1.0 = square
    float character;  // 0.0 = digital, 1.0 = full analog reference
    float drift;      // 0.0 = none, 1.0 = full drift
};

class AnalogEngine {
public:
    static constexpr int DISPLAY_SAMPLES = 256;

    // Process a single sample. phase is [0, 1).
    // Returns output in [-1, +1] range.
    float process(float phase, const AnalogEngineParams& params, float sampleRate);

    // Get display buffer for GUI thread (lock-free read)
    const std::array<float, DISPLAY_SAMPLES>& getDisplayBuffer() const;
    float getDisplayPhase() const;

    // Called once per cycle (on phase wrap) to snapshot the display buffer
    void updateDisplayBuffer(const AnalogEngineParams& params);

    // Reset all internal state
    void reset();

private:
    MorphStage morphStage;
    CharacterStage characterStage;
    DriftProcessor driftProcessor;

    // Double-buffered display data
    std::array<float, DISPLAY_SAMPLES> displayBuffers[2];
    std::atomic<int> displayWriteIdx{0};
    std::atomic<float> displayPhase{0.f};
};
```

**Design rationale:** The engine is a pure DSP class with no VCV Rack dependencies. This makes it testable in isolation, reusable across modules, and potentially portable to other plugin formats later. It takes normalized inputs (phase 0-1, params 0-1) and returns normalized output (-1 to +1). The host module handles all VCV-specific concerns.

### 2. MorphStage -- Continuous Waveform Morphing

The morph knob maps to a 0-1 range divided into three segments:

```
morph = 0.0        0.33       0.67       1.0
        |  sine   |  tri     |  saw     |
        |   <->   |   <->    |   <->    |
        |   tri   |   saw    |   square |
```

Each transition crossfades between adjacent waveform shapes.

```cpp
// src/dsp/MorphStage.hpp
class MorphStage {
public:
    // Generate morphed waveform from phase and morph position.
    // phase: [0, 1), morph: [0, 1]
    // Returns [-1, +1]
    float process(float phase, float morph) const;

private:
    // Individual waveform generators (pure math, no state)
    static float sine(float phase);
    static float triangle(float phase);
    static float saw(float phase);
    static float square(float phase);

    // Crossfade helper
    static float crossfade(float a, float b, float mix);
};
```

**Implementation approach for morphing:**

```cpp
float MorphStage::process(float phase, float morph) const {
    // Scale morph to segment index + fractional position
    float scaled = morph * 3.f;  // 0 to 3
    int segment = std::min((int)scaled, 2);
    float frac = scaled - (float)segment;

    float a, b;
    switch (segment) {
        case 0: a = sine(phase);     b = triangle(phase); break;
        case 1: a = triangle(phase); b = saw(phase);      break;
        case 2: a = saw(phase);      b = square(phase);   break;
    }
    return crossfade(a, b, frac);
}
```

**Why linear crossfade, not equal-power:** For waveform morphing, linear crossfade produces the correct intermediate shapes. Equal-power crossfade would boost the midpoint amplitude, which is wrong here -- we want the waveform shape to smoothly interpolate, not the energy level. The perceptual "dip" that equal-power solves in audio mixing doesn't apply to single-cycle waveshaping.

### 3. CharacterStage -- Analog Reference Modeling

The character knob crossfades between the "digital" waveform (output of MorphStage) and an "analog reference" waveform that models specific classic synth characteristics.

```cpp
// src/dsp/CharacterStage.hpp
class CharacterStage {
public:
    // Apply analog character to a digital waveform.
    // digitalSample: output from MorphStage
    // phase: current phase [0, 1) for generating analog reference
    // morph: current morph position (determines which analog reference to use)
    // character: 0.0 = pure digital, 1.0 = full analog reference
    float process(float digitalSample, float phase, float morph, float character) const;

private:
    // Analog reference waveform generators
    // Each models characteristics of specific classic synths
    float analogSine(float phase) const;      // Slightly clipped, mild even harmonics
    float analogTriangle(float phase) const;   // Soft corners, slight asymmetry
    float analogSaw(float phase) const;        // Leaky integrator shape, soft top
    float analogSquare(float phase) const;     // Soft edges, slight duty cycle offset

    // Morphed analog reference (same morph segmentation as digital)
    float analogMorphed(float phase, float morph) const;
};
```

**Analog reference waveform modeling approach:**

Rather than wavetable lookups, model the analog deviations mathematically. This keeps the character continuous with the morph position and avoids wavetable interpolation artifacts.

- **Analog sine:** Add small 2nd and 3rd harmonic content (~2-5% amplitude), slight positive DC bias. Models transformer and op-amp coloration.
- **Analog triangle:** Soften the peaks with a tanh-like saturation at the tips, add slight asymmetry (rising slope ~2% faster than falling). Models integrator capacitor non-linearity.
- **Analog saw:** Apply a one-pole leaky integrator shape: the linear ramp gets slightly exponential. Soften the reset with a brief RC-like transition (~5% of cycle). Models capacitor discharge.
- **Analog square:** Apply tanh soft-clipping to the transitions (rise/fall time ~3-5% of cycle), add slight duty cycle offset (~1-2%). Models transistor switching speed.

```cpp
float CharacterStage::process(float digitalSample, float phase,
                               float morph, float character) const {
    if (character < 0.001f) return digitalSample;  // fast path

    float analogSample = analogMorphed(phase, morph);
    return digitalSample + character * (analogSample - digitalSample);
    // Equivalent to: lerp(digitalSample, analogSample, character)
}
```

### 4. DriftProcessor -- Analog Imperfections

The drift knob scales six independent imperfection sources simultaneously. Each source has its own internal state (slow random processes).

```cpp
// src/dsp/DriftProcessor.hpp
class DriftProcessor {
public:
    struct DriftState {
        float pitchDrift;       // Slow random pitch wander (Hz offset)
        float phaseJitter;      // Fast random phase noise (phase offset)
        float componentSpread;  // Per-instance fixed random offsets (set on init)
        float dcOffset;         // Slow random DC wander (voltage offset)
        float hfRolloffFreq;    // Cutoff frequency for output LP filter
        float pitchSlew;        // Slew rate limit on pitch changes
    };

    // Initialize component spread (called once on module creation)
    void initComponentSpread(uint64_t seed);

    // Update drift sources. Call once per sample.
    // driftAmount: 0.0 = no drift, 1.0 = full drift
    void update(float driftAmount, float sampleRate);

    // Apply pitch drift to frequency (call before phase accumulation)
    float applyPitchDrift(float freq) const;

    // Apply pitch slew to frequency (call before phase accumulation)
    float applyPitchSlew(float freq, float sampleRate);

    // Apply phase jitter to phase (call after phase accumulation)
    float applyPhaseJitter(float phase) const;

    // Apply DC offset to output (call after waveform generation)
    float applyDCOffset(float sample) const;

    // Apply HF rolloff to output (call after waveform generation)
    float applyHFRolloff(float sample, float sampleRate);

    void reset();

private:
    DriftState state{};

    // Slow random generators (low-frequency noise sources)
    // Use filtered white noise or Perlin-like approach
    float pitchLFO = 0.f;       // ~0.1-0.5 Hz random walk
    float dcLFO = 0.f;          // ~0.05-0.2 Hz random walk

    // One-pole LP filter state for HF rolloff
    float hfFilterState = 0.f;

    // Pitch slew state
    float slewedFreq = 0.f;

    // Component spread (fixed per instance)
    float spreadPitch = 0.f;    // Fixed pitch offset, ~few cents
    float spreadDuty = 0.f;     // Fixed duty cycle offset
    float spreadGain = 0.f;     // Fixed gain offset

    // Internal noise source
    uint32_t noiseState = 12345; // xorshift PRNG
    float nextRandom();          // Returns [-1, +1]

    // Filtered noise for slow drift
    float filteredNoise(float input, float& state, float cutoff, float sampleRate);
};
```

**Drift source details and scaling:**

| Source | Rate | At drift=0.25 | At drift=0.5 | At drift=1.0 | Implementation |
|--------|------|--------------|-------------|-------------|----------------|
| Pitch drift | 0.1-0.5 Hz | +/- 2 cents | +/- 5 cents | +/- 15 cents | Filtered noise -> freq multiplier |
| Phase jitter | per-sample | +/- 0.0001 | +/- 0.0005 | +/- 0.002 | Filtered noise -> phase offset |
| Component spread | fixed | small | medium | large | Random seed -> fixed offsets |
| DC offset | 0.05-0.2 Hz | +/- 5mV | +/- 15mV | +/- 50mV | Filtered noise -> voltage offset |
| HF rolloff | static | 18 kHz | 14 kHz | 8 kHz | One-pole LP filter cutoff |
| Pitch slew | on change | 50ms | 100ms | 200ms | One-pole LP on frequency |

**Why xorshift PRNG:** The drift system needs random numbers every sample, but they don't need to be cryptographically strong. A xorshift32 is 3 instructions, cache-friendly, and produces sufficiently random results for perceptual analog drift. The filtered noise generator then shapes the raw random values into the appropriate frequency band.

### 5. WaveformDisplay -- NanoVG Real-Time Rendering

```cpp
// src/ui/WaveformDisplay.hpp
#pragma once
#include <rack.hpp>
#include <array>

class WaveformDisplay : public rack::widget::TransparentWidget {
public:
    // Reference to module for reading display data (may be null in module browser)
    rack::Module* module = nullptr;

    // Display buffer access (set by widget constructor)
    // These point into the module's double-buffered display data
    std::function<const std::array<float, 256>&()> getBuffer;
    std::function<float()> getPhase;

    // Visual config
    static constexpr float TRACE_WIDTH = 1.5f;
    static constexpr float DOT_RADIUS = 3.f;

    // Colors (Forge Audio brand)
    NVGcolor traceColor = nvgRGBAf(0.91f, 0.66f, 0.22f, 0.9f);   // Amber
    NVGcolor dotColor = nvgRGBAf(1.f, 0.85f, 0.4f, 1.f);          // Bright amber
    NVGcolor bgColor = nvgRGBAf(0.08f, 0.08f, 0.14f, 1.f);        // Deep navy
    NVGcolor gridColor = nvgRGBAf(0.15f, 0.15f, 0.25f, 0.5f);     // Subtle grid

    void draw(const DrawArgs& args) override;

private:
    void drawBackground(NVGcontext* vg, float w, float h);
    void drawGrid(NVGcontext* vg, float w, float h);
    void drawWaveform(NVGcontext* vg, float w, float h,
                      const std::array<float, 256>& buffer);
    void drawPhaseDot(NVGcontext* vg, float w, float h,
                      const std::array<float, 256>& buffer, float phase);
    void drawPlaceholder(NVGcontext* vg, float w, float h);
};
```

**Display rendering approach:**

```cpp
void WaveformDisplay::draw(const DrawArgs& args) {
    float w = box.size.x;
    float h = box.size.y;
    NVGcontext* vg = args.vg;

    drawBackground(vg, w, h);
    drawGrid(vg, w, h);

    if (!module || !getBuffer) {
        drawPlaceholder(vg, w, h);
        return;
    }

    const auto& buffer = getBuffer();
    float phase = getPhase();

    drawWaveform(vg, w, h, buffer);
    drawPhaseDot(vg, w, h, buffer, phase);
}

void WaveformDisplay::drawWaveform(NVGcontext* vg, float w, float h,
                                    const std::array<float, 256>& buffer) {
    float padX = 4.f, padY = 6.f;
    float plotW = w - 2 * padX;
    float plotH = h - 2 * padY;
    float midY = padY + plotH * 0.5f;

    nvgBeginPath(vg);
    for (int i = 0; i < 256; i++) {
        float x = padX + plotW * (float)i / 255.f;
        float y = midY - buffer[i] * plotH * 0.45f;  // +/-1 maps to 90% of plot height
        if (i == 0) nvgMoveTo(vg, x, y);
        else nvgLineTo(vg, x, y);
    }
    nvgStrokeColor(vg, traceColor);
    nvgStrokeWidth(vg, TRACE_WIDTH);
    nvgStroke(vg);
}

void WaveformDisplay::drawPhaseDot(NVGcontext* vg, float w, float h,
                                    const std::array<float, 256>& buffer,
                                    float phase) {
    float padX = 4.f, padY = 6.f;
    float plotW = w - 2 * padX;
    float plotH = h - 2 * padY;
    float midY = padY + plotH * 0.5f;

    // Interpolate buffer at current phase position
    float idx = phase * 255.f;
    int i0 = (int)idx;
    int i1 = std::min(i0 + 1, 255);
    float frac = idx - (float)i0;
    float val = buffer[i0] + frac * (buffer[i1] - buffer[i0]);

    float x = padX + plotW * phase;
    float y = midY - val * plotH * 0.45f;

    nvgBeginPath(vg);
    nvgCircle(vg, x, y, DOT_RADIUS);
    nvgFillColor(vg, dotColor);
    nvgFill(vg);
}
```

**Why TransparentWidget, not FramebufferWidget:** The waveform changes continuously (phase dot moves, drift affects shape), so caching via FramebufferWidget provides no benefit -- it would need to be dirtied every frame anyway. `TransparentWidget::draw()` is called every GUI frame (~60Hz), which is exactly what we want. The rendering cost of a 256-point polyline + circle is trivial for NanoVG.

**Why 256 samples for display buffer:** 256 is enough resolution for a smooth visual curve at typical display sizes (80-120px wide). More points add GPU draw cost without visible improvement. Fewer points create visible faceting on smooth waveforms. 256 also aligns nicely with power-of-2 for index math.

### 6. Host Module Structure (LFO)

```cpp
// src/ForgeAnalogLFO.cpp
struct ForgeAnalogLFO : Module {
    enum ParamId {
        RATE_PARAM,
        MORPH_PARAM,
        CHARACTER_PARAM,
        DRIFT_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        RESET_INPUT,
        FM_INPUT,
        MORPH_CV_INPUT,
        CHARACTER_CV_INPUT,
        DRIFT_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        MAIN_OUTPUT,
        INV_OUTPUT,
        OUTPUTS_LEN
    };

    AnalogEngine engine;
    float phase = 0.f;
    float prevPhase = 0.f;
    dsp::SchmittTrigger resetTrigger;

    // Display buffer (double-buffered, lock-free)
    std::array<float, 256> displayBuffers[2];
    std::atomic<int> displayReadIdx{0};
    std::atomic<float> displayPhase{0.f};

    void process(const ProcessArgs& args) override {
        // 1. Read parameters with CV
        float morph = clamp(params[MORPH_PARAM].getValue()
                     + inputs[MORPH_CV_INPUT].getVoltage() / 5.f, 0.f, 1.f);
        float character = clamp(params[CHARACTER_PARAM].getValue()
                         + inputs[CHARACTER_CV_INPUT].getVoltage() / 5.f, 0.f, 1.f);
        float driftAmt = clamp(params[DRIFT_PARAM].getValue()
                        + inputs[DRIFT_CV_INPUT].getVoltage() / 5.f, 0.f, 1.f);

        // 2. Compute frequency
        float pitch = params[RATE_PARAM].getValue();
        pitch += inputs[FM_INPUT].getVoltage();
        float freq = 2.f * dsp::exp2_taylor5(pitch);

        // 3. Apply drift to frequency
        engine.driftProcessor.update(driftAmt, args.sampleRate);
        freq = engine.driftProcessor.applyPitchDrift(freq);
        freq = engine.driftProcessor.applyPitchSlew(freq, args.sampleRate);
        freq = clamp(freq, 0.001f, 0.5f * args.sampleRate);

        // 4. Advance phase
        prevPhase = phase;
        phase += freq * args.sampleTime;
        // Apply phase jitter
        float jitteredPhase = engine.driftProcessor.applyPhaseJitter(phase);
        phase -= std::floor(phase);  // Wrap clean phase
        float displayP = jitteredPhase - std::floor(jitteredPhase); // Wrap jittered

        // 5. Handle reset
        if (resetTrigger.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 2.f)) {
            phase = 0.f;
            displayP = 0.f;
        }

        // 6. Generate output through engine
        AnalogEngineParams engineParams{morph, character, driftAmt};
        float output = engine.process(displayP, engineParams, args.sampleRate);

        // 7. Scale and output
        outputs[MAIN_OUTPUT].setVoltage(5.f * output);
        outputs[INV_OUTPUT].setVoltage(-5.f * output);

        // 8. Update display (on phase wrap or periodically)
        displayPhase.store(displayP, std::memory_order_relaxed);
        if (prevPhase > phase) { // phase wrapped
            engine.updateDisplayBuffer(engineParams);
            int writeIdx = 1 - displayReadIdx.load(std::memory_order_relaxed);
            displayBuffers[writeIdx] = engine.getDisplayBuffer();
            displayReadIdx.store(writeIdx, std::memory_order_release);
        }
    }

    json_t* dataToJson() override {
        json_t* root = json_object();
        json_object_set_new(root, "phase", json_real(phase));
        engine.driftProcessor.toJson(root);  // Save component spread seed
        return root;
    }

    void dataFromJson(json_t* root) override {
        json_t* phaseJ = json_object_get(root, "phase");
        if (phaseJ) phase = json_real_value(phaseJ);
        engine.driftProcessor.fromJson(root);
    }
};
```

### 7. Host Module Structure (VCO) -- Key Differences

The VCO host module differs from LFO in these specific ways:

```cpp
// src/ForgeAnalogVCO.cpp -- key differences from LFO
struct ForgeAnalogVCO : Module {
    enum ParamId {
        FREQ_PARAM,       // Coarse tune (-3 to +3 octaves)
        FINE_PARAM,       // Fine tune (-100 to +100 cents)
        MORPH_PARAM,
        CHARACTER_PARAM,
        DRIFT_PARAM,
        FM_AMOUNT_PARAM,  // FM depth attenuverter
        PARAMS_LEN
    };
    enum InputId {
        VOCT_INPUT,       // 1V/octave pitch tracking
        FM_INPUT,
        SYNC_INPUT,       // Hard sync
        MORPH_CV_INPUT,
        CHARACTER_CV_INPUT,
        DRIFT_CV_INPUT,
        INPUTS_LEN
    };

    // Through-zero FM flag (context menu toggle)
    bool throughZeroFM = false;

    // Antialiasing: polyBLEP state
    float prevPhaseIncrement = 0.f;

    void process(const ProcessArgs& args) override {
        // ... param reading same as LFO ...

        // Pitch: V/Oct standard
        float pitch = params[FREQ_PARAM].getValue();
        pitch += params[FINE_PARAM].getValue() / 1200.f; // cents to octaves
        pitch += inputs[VOCT_INPUT].getVoltage();
        float freq = dsp::FREQ_C4 * dsp::exp2_taylor5(pitch);

        // FM
        float fmInput = inputs[FM_INPUT].getVoltage()
                       * params[FM_AMOUNT_PARAM].getValue();

        float phaseInc;
        if (throughZeroFM) {
            // Through-zero: FM modulates phase increment directly
            // Allows negative frequencies (phase runs backward)
            float fmFreq = freq * dsp::exp2_taylor5(fmInput);
            phaseInc = fmFreq * args.sampleTime;
            // No clamping -- negative values = backward phase
        } else {
            // Linear FM: add FM voltage to frequency
            freq += fmInput * freq;  // Exponential FM
            freq = std::fmax(freq, 0.f);  // Prevent negative for non-TZ
            phaseInc = freq * args.sampleTime;
        }

        // Hard sync
        if (syncTrigger.process(inputs[SYNC_INPUT].getVoltage(), 0.1f, 2.f)) {
            phase = 0.f;
        }

        // Phase accumulation
        prevPhase = phase;
        phase += phaseInc;
        phase -= std::floor(phase);

        // Generate output
        AnalogEngineParams engineParams{morph, character, driftAmt};
        float output = engine.process(phase, engineParams, args.sampleRate);

        // Apply polyBLEP antialiasing for audio-rate operation
        output = applyPolyBLEP(output, phase, phaseInc);

        outputs[MAIN_OUTPUT].setVoltage(5.f * output);
        outputs[INV_OUTPUT].setVoltage(-5.f * output);
    }
};
```

**Antialiasing strategy for VCO:**

Use **polyBLEP** (polynomial bandlimited step) for the VCO. PolyBLEP works by detecting discontinuities in the waveform (the vertical edges in saw and square waves) and replacing the naive step with a polynomial approximation of the bandlimited step function. This is:

- Computationally cheap (a few multiplies per discontinuity per sample)
- Compatible with waveform morphing (the morph crossfade naturally blends the polyBLEP contributions)
- Compatible with FM and sync (polyBLEP adapts to the current phase increment)
- Well-proven in VCV Rack ecosystem (VCV's own Fundamental VCO uses this approach)

The LFO does not need antialiasing because it operates below audio rate where aliasing is inaudible.

**Through-zero FM implementation:**

Through-zero FM allows the phase increment to go negative, meaning the oscillator runs backward through its cycle. This creates the distinctive clangorous FM timbres. Implementation is straightforward: don't clamp the phase increment to positive values, and ensure the phase wrapping works correctly for negative values (use `phase -= std::floor(phase)` which handles negatives correctly).

**Hard sync implementation:**

On a rising edge of the sync input, reset phase to 0. The polyBLEP antialiasing should account for the sync discontinuity by inserting a polyBLEP correction at the sync point, treating it like a waveform reset.

## File Structure

```
src/
  plugin.hpp                    // Plugin declarations
  plugin.cpp                    // Plugin registration

  dsp/
    AnalogEngine.hpp            // Engine class declaration
    AnalogEngine.cpp            // Engine implementation
    MorphStage.hpp              // Waveform morphing
    MorphStage.cpp
    CharacterStage.hpp          // Analog character modeling
    CharacterStage.cpp
    DriftProcessor.hpp          // Drift/imperfection system
    DriftProcessor.cpp
    PolyBLEP.hpp                // Antialiasing utilities (VCO only)

  ui/
    WaveformDisplay.hpp         // NanoVG waveform widget
    WaveformDisplay.cpp

  modules/
    ForgeAnalogLFO.cpp          // LFO module (host)
    ForgeAnalogVCO.cpp          // VCO module (host)

res/
  ForgeAnalogLFO.svg            // LFO panel
  ForgeAnalogVCO.svg            // VCO panel
```

**Why this file organization:** The `dsp/` directory contains pure C++ with no VCV Rack dependencies, making it unit-testable and potentially reusable. The `ui/` directory contains VCV Rack widget code. The `modules/` directory contains the VCV Rack Module subclasses that wire everything together. This separation ensures the DSP code can be developed and tested independently from the UI.

## Patterns to Follow

### Pattern 1: Lock-Free Double Buffer for Audio-to-GUI Communication

**What:** Use two buffers and an atomic index to pass data from audio thread to GUI thread without locks.

**When:** Any time the audio thread needs to send data to the GUI for display (waveform shape, phase position, level meters).

**Why:** Mutexes in the audio thread cause priority inversion and audio glitches. Lock-free patterns guarantee the audio thread never blocks.

```cpp
// Writer (audio thread):
int writeIdx = 1 - readIdx.load(std::memory_order_relaxed);
fillBuffer(buffers[writeIdx]);
readIdx.store(writeIdx, std::memory_order_release);

// Reader (GUI thread):
int idx = readIdx.load(std::memory_order_acquire);
renderFrom(buffers[idx]);
```

### Pattern 2: Normalized Internal Representation

**What:** All engine internals work in normalized ranges: phase [0,1), waveform output [-1,+1], knob positions [0,1].

**When:** Always. The host module handles conversion to/from VCV Rack voltage conventions.

**Why:** Makes the engine reusable, testable, and mathematically clean. Voltage scaling (+/-5V, +/-10V) is a presentation concern, not a DSP concern.

### Pattern 3: Display Buffer Generation on Phase Wrap

**What:** Generate the 256-sample display buffer by evaluating the engine at 256 evenly-spaced phase values (0/256, 1/256, ..., 255/256) when the oscillator's phase wraps from ~1.0 back to ~0.0.

**When:** Once per cycle of the oscillator.

**Why:** This captures the exact current waveform shape (including morph, character, and some drift effects) without adding per-sample cost. For LFO, this updates at the LFO rate (could be every few seconds at low rates). For VCO at audio rate, this updates at the fundamental frequency (hundreds of times per second -- far more than the 60Hz display refresh).

**Handling slow LFO rates:** When the LFO rate is very slow (e.g., 0.01 Hz = 100 second cycle), waiting for a phase wrap would mean the display only updates every 100 seconds. Solution: also regenerate the display buffer whenever a knob changes by more than a small threshold. Use a simple change-detection comparison on the morph/character/drift params.

```cpp
// In process(), additionally:
bool paramsChanged = (std::fabs(morph - prevMorph) > 0.005f) ||
                     (std::fabs(character - prevCharacter) > 0.005f) ||
                     (std::fabs(driftAmt - prevDrift) > 0.01f);
if (paramsChanged || phaseWrapped) {
    regenerateDisplayBuffer(engineParams);
    prevMorph = morph;
    prevCharacter = character;
    prevDrift = driftAmt;
}
```

### Pattern 4: Component Spread via Seed

**What:** Each module instance gets a random seed (from VCV Rack's `random::u32()`) that deterministically generates its "component spread" -- fixed deviations that make each instance slightly unique, like real analog circuits.

**When:** On module creation, saved/restored via JSON serialization.

**Why:** Real analog oscillators vary unit-to-unit due to component tolerances. By seeding from a random value and persisting it, each module instance has its own "personality" that survives save/load. The drift knob scales how much this personality affects the output.

### Pattern 5: Param Smoothing for Zipper-Free Knob Movement

**What:** Apply one-pole smoothing to knob values before using them in DSP, to prevent audible zipper noise from quantized parameter changes.

**When:** For the morph, character, and drift knobs when used at audio rate (VCO). Less critical for LFO.

```cpp
// Simple one-pole smoother
float smoothParam(float target, float& state, float coeff) {
    state += coeff * (target - state);
    return state;
}
// coeff = 1 - exp(-2*pi * cutoffHz / sampleRate)
// ~20Hz cutoff gives smooth knob response without sluggishness
```

## Anti-Patterns to Avoid

### Anti-Pattern 1: Allocating Memory in process()

**What:** Using `new`, `malloc`, `std::vector::push_back`, or any allocating operation inside `Module::process()`.

**Why bad:** Memory allocation can block on a mutex in the allocator, causing audio dropouts. The audio thread must be allocation-free.

**Instead:** Pre-allocate all buffers in the constructor. Use fixed-size `std::array` for display buffers. Pre-size any containers.

### Anti-Pattern 2: Mutex-Based Audio/GUI Synchronization

**What:** Using `std::mutex` to protect data shared between audio and GUI threads.

**Why bad:** If the GUI thread holds the lock when the audio thread needs it, the audio thread blocks, causing an audible dropout. Priority inversion is the #1 cause of audio glitches in plugin development.

**Instead:** Use lock-free double buffering (Pattern 1) or `std::atomic` for simple values.

### Anti-Pattern 3: Computing Display Buffer Every Sample

**What:** Running the 256-point display buffer generation on every audio sample.

**Why bad:** At 48kHz, that's 48000 * 256 = 12.3 million extra waveform evaluations per second. Completely unnecessary since the display only refreshes at 60Hz.

**Instead:** Generate display buffer once per cycle (on phase wrap) or on parameter change (Pattern 3).

### Anti-Pattern 4: Putting VCV Rack Types in the Engine

**What:** Including `<rack.hpp>` in the DSP engine classes, or using `rack::dsp::*` utilities inside the engine.

**Why bad:** Couples the DSP to the VCV Rack SDK, making it impossible to unit test without the full SDK. Also prevents reuse in other contexts (VST, standalone, etc.).

**Instead:** The engine uses only standard C++ and `<cmath>`. The host module translates between VCV Rack conventions and the engine's normalized interface.

### Anti-Pattern 5: Using FramebufferWidget for Continuously Animated Displays

**What:** Wrapping the waveform display in a `FramebufferWidget` expecting it to cache rendering.

**Why bad:** Since the waveform changes every frame (the phase dot moves), the framebuffer would need to be dirtied and re-rendered every frame anyway, adding overhead (render to texture, then render texture) compared to direct drawing.

**Instead:** Use `TransparentWidget` and draw directly each frame. The 256-point polyline + circle is trivially cheap for NanoVG.

## Scalability Considerations

| Concern | Single instance | 10 instances | 50+ instances |
|---------|----------------|--------------|---------------|
| CPU (DSP) | Negligible. Engine is ~50 instructions/sample. | Comfortable. ~500 instr/sample total. | Fine. Analog modeling is lightweight math. |
| CPU (Display) | Trivial. 256-point polyline at 60fps. | Moderate. 10 displays at 60fps. | Consider reducing display update rate or LOD for off-screen modules. |
| Memory | ~2KB per display double buffer. Negligible. | ~20KB. Negligible. | ~100KB. Still negligible. |
| Drift PRNG | One xorshift per sample. Negligible. | 10 xorshifts. Negligible. | 50 xorshifts. Still negligible. |

The architecture has no scalability bottlenecks for realistic usage. The most expensive operation is the NanoVG polyline rendering, which VCV Rack already throttles by only calling draw() on visible widgets.

## Build Order (Dependencies)

The components should be built in this order due to dependencies:

```
Phase 1: Foundation
  MorphStage          (no dependencies, pure math)
  WaveformDisplay     (no dependency on engine -- can show static placeholder)

Phase 2: Engine Core
  AnalogEngine        (depends on MorphStage)
  ForgeAnalogLFO      (depends on AnalogEngine, WaveformDisplay)
  -> Milestone: Working LFO with morph knob and display

Phase 3: Character
  CharacterStage      (depends on MorphStage for structure)
  Wire into AnalogEngine
  -> Milestone: Morph + Character working together

Phase 4: Drift
  DriftProcessor      (independent, but tested via engine)
  Wire into AnalogEngine + host modules
  -> Milestone: Full three-knob LFO complete

Phase 5: VCO
  PolyBLEP            (independent utility)
  ForgeAnalogVCO      (depends on AnalogEngine + PolyBLEP)
  Through-zero FM     (extends VCO)
  Hard sync           (extends VCO)
  -> Milestone: Full VCO complete
```

**Phase ordering rationale:**

- MorphStage first because it is the foundation all other stages build on. It can be tested and heard immediately.
- WaveformDisplay early because visual feedback accelerates development of all subsequent stages -- you can see what the morph/character/drift is doing.
- CharacterStage before DriftProcessor because character defines the "target sound" that drift then destabilizes. Building drift first would mean tuning drift parameters without knowing the target.
- VCO last because it reuses the entire engine and only adds host-level concerns (antialiasing, sync, FM, pitch tracking). The engine is battle-tested from the LFO by this point.

## Sources

- VCV Rack 2 SDK documentation (vcvrack.com/docs-v2) -- Module/Widget architecture, threading model, NanoVG integration
- Existing LFO POC at `/Users/mrcbrown/Claude/Software/Forge Audio/LFO/src/LFO.cpp` -- validated phase accumulation, waveform generation, reset/FM patterns
- PROJECT.md at `/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/.planning/PROJECT.md` -- requirements, constraints, brand identity
- polyBLEP antialiasing: Valimaki & Franck, "Polynomial bandlimited step functions for antialiased oscillator waveforms" -- standard technique used by VCV Fundamental modules
- Lock-free programming patterns for real-time audio: Ross Bencina, "Real-time audio programming 101" -- canonical reference for audio thread safety
- NanoVG API documentation -- TransparentWidget drawing, path rendering, color handling

**Confidence notes:**
- HIGH confidence on VCV Rack SDK patterns (verified against POC code and official documentation)
- HIGH confidence on DSP architecture (morph/character/drift pipeline is standard waveshaping practice)
- HIGH confidence on lock-free display buffer (well-established real-time audio pattern)
- HIGH confidence on polyBLEP for VCO antialiasing (industry standard, used by VCV Fundamental)
- MEDIUM confidence on exact analog reference waveform shapes (the mathematical models for CharacterStage will need tuning by ear during implementation -- the architecture supports this but the specific parameter values are empirical)
- MEDIUM confidence on drift parameter scaling (the ranges in the drift table are reasonable starting points but will need perceptual tuning)
