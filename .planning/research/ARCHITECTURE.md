# Architecture Research: v1.2 Deep Analog Integration

**Domain:** Deep analog character expansion for VCV Rack LFO
**Researched:** 2026-03-13
**Confidence:** HIGH (existing codebase thoroughly analyzed; integration points are well-defined; all features are DSP fundamentals with established implementation patterns)

## Current Architecture Snapshot (Post v1.1)

The AnalogLFO is an 890-line single-file module (`src/AnalogLFO.cpp`) with three structs:

1. **`AnalogLFO` (Module)** -- Phase accumulator, waveform engine, character engine, drift engine, clock tracker, display buffer management. All DSP in `process()`.
2. **`WaveformDisplay` (TransparentWidget)** -- NanoVG rendering of waveform trace, phase dot, text overlays (SYNC, ratio, BPM, Hz). Reads lock-free display state.
3. **`AnalogLFOWidget` (ModuleWidget)** -- Panel layout with knobs, jacks, screws, display.

### Current Data Flow (Per Sample)

```
process():
  1. processClockInput(sampleTime)          // clock edge detection, period EMA, phase reset
  2. Compute targetFreq (clocked or free)
  3. Apply frequency slew (freqSlew)
  4. deltaPhase = freq * sampleTime         // double precision
  5. Read drift params + CV -> drift amount
  6. If drift > 0: run 4 OU layers, modulate deltaPhase
  7. phase += deltaPhase; wrap [0, 1)
  8. Read morph/character params + CV
  9. Update display buffer on wrap or param change
  10. computeMorphedWave(phase, morph, character) -> sample
  11. Apply crossfade if recent phase reset
  12. Output: sample * 5V
```

### Current Enum Layout

```cpp
enum ParamId {    // 7 params
    MORPH_PARAM, CHARACTER_PARAM, DRIFT_PARAM, RATE_PARAM,
    MORPH_ATTEN_PARAM, CHARACTER_ATTEN_PARAM, DRIFT_ATTEN_PARAM
};
enum InputId {    // 4 inputs
    MORPH_CV_INPUT, DRIFT_CV_INPUT, CHARACTER_CV_INPUT, CLK_INPUT
};
enum OutputId { OUTPUT };   // 1 output
```

### Current Panel Layout (12HP = 60.96mm)

```
y=15-42mm   Waveform display (57x27mm)
y=54mm      MORPH knob (center, RoundBigBlackKnob)
y=69mm      CHARACTER (x=18mm) + DRIFT (x=42.96mm) knobs
y=86mm      RATE knob (x=18mm) + CLK jack (x=42.96mm)
y=96mm      Trimpots: Morph(x=10) Char(x=24) Drift(x=38)
y=108mm     Jacks: MorphCV(x=10) CharCV(x=24) DriftCV(x=38) OUT(x=52)
```

---

## v1.2 Feature Integration Analysis

### Feature 1: FM Input Jack

**What it does:** Allows external CV to modulate the LFO frequency. An FM input voltage shifts frequency exponentially (1V/oct standard) or linearly, scaled by an attenuator.

**Integration point:** Between frequency computation (step 2-3) and deltaPhase calculation (step 4).

**Architecture decision: Exponential FM** because the LFO already operates with a linear Hz parameter that maps to deltaPhase linearly. Exponential FM means the input voltage is added to a pitch representation before converting to frequency, which gives musically useful scaling (1V = double frequency, consistent with VCV standards). This is the approach used by the VCV Fundamental LFO.

**Implementation:**

```cpp
// After targetFreq is computed, before deltaPhase:
float fmInput = inputs[FM_INPUT].getVoltage();
float fmAtten = params[FM_ATTEN_PARAM].getValue();  // -1.0 to 1.0 (attenuverter)
float fmAmount = fmInput * fmAtten;

// Convert targetFreq to pitch space, add FM, convert back
// pitch = log2(freq / baseFreq), but simpler: just scale freq exponentially
float fmFreq = targetFreq * std::pow(2.f, fmAmount);
fmFreq = rack::math::clamp(fmFreq, 0.001f, 100.f);  // safety clamp
float freq = freqSlew.process(args.sampleTime, fmFreq);
```

**New components:**
- `FM_INPUT` enum entry
- `FM_ATTEN_PARAM` enum entry (attenuverter, -1 to 1)
- FM jack on panel
- FM attenuverter trimpot on panel

**Modified components:**
- `process()`: insert FM calculation between targetFreq and deltaPhase
- Constructor: `configInput(FM_INPUT, "FM")`, `configParam(FM_ATTEN_PARAM, ...)`
- Widget: add FM jack and trimpot

**Panel placement consideration:** The FM jack is functionally related to Rate/CLK (frequency control group). It should go near RATE knob. The current layout has RATE at (18, 86) and CLK at (42.96, 86). FM input could go at (52, 96) with its trimpot at (52, 96) -- but this conflicts with the existing trimpot row. **Alternative:** Add FM jack to the bottom jack row. See Panel Layout section below.

**Real-time safety:** `std::pow(2.f, x)` is safe for audio thread. No allocation. For performance, could use `rack::dsp::exp2_taylor5()` approximation if profiling shows need, but at LFO rates (process() is cheap overall) exact `pow` is fine.

**Interaction with clock mode:** FM should apply in BOTH free and clocked modes. In clocked mode, FM modulates around the clock-derived frequency, creating tempo-synced frequency wobble -- a musically useful effect.

---

### Feature 2: Expanded Analog Imperfections

Four new imperfection types that complement the existing drift engine. The key architectural principle: **all four are controlled by the existing Drift knob** via curated proportions, not individual controls (matches the "No individually exposed drift params" out-of-scope decision).

#### 2a. Phase Jitter

**What it does:** Adds random per-sample noise to the phase readout, simulating timing instability in analog oscillator circuits. Unlike drift (which modulates frequency/deltaPhase), jitter modulates the phase value used for waveform lookup.

**Integration point:** Between phase accumulation (step 7) and waveform generation (step 10). Applied to the phase value passed to `computeMorphedWave()`, NOT to the accumulator itself (to prevent jitter from accumulating into permanent phase error).

```cpp
// After phase accumulation, before waveform generation:
float p = (float)phase;
if (drift >= 0.001f) {
    float jitterAmount = driftAmount * 0.003f;  // max 0.3% of cycle at full drift
    float jitterNoise = normalDist(rng);
    p += jitterAmount * jitterNoise;
    p = p - std::floor(p);  // wrap to [0, 1)
}
float sample = computeMorphedWave(p, morph, character);
```

**Why not modulate deltaPhase:** Drift already modulates deltaPhase via the OU process. Phase jitter is a different phenomenon -- it is uncorrelated per-sample noise on the phase readout, not a frequency modulation. Adding it to deltaPhase would make it indistinguishable from the existing OU drift.

**New components:** None (uses existing RNG and normalDist).

**Modified components:** `process()` -- add jitter calculation between phase accumulation and waveform generation.

#### 2b. DC Offset Drift

**What it does:** Adds a slowly wandering DC offset to the output, simulating capacitor leakage and op-amp offset voltage in analog circuits. Real analog oscillators rarely sit at perfect 0V center.

**Integration point:** After waveform generation (step 10), before output scaling. A slow random process (separate OU layer or reuse of existing slow OU layer) adds a small voltage offset.

```cpp
// After sample = computeMorphedWave(...):
if (drift >= 0.001f) {
    // Reuse the slowest OU layer (0.05Hz) as DC drift source
    float dcOffset = driftAmount * 0.04f * ouLayers[0].state;  // max ~200mV offset
    outputVoltage = 5.f * sample + dcOffset;
} else {
    outputVoltage = 5.f * sample;
}
```

**Why reuse OU layer 0:** The slowest OU layer (0.05Hz wander) has exactly the right character for DC drift -- very slow, continuous wandering. Creating a separate OU process would be redundant. The layer's state is already computed in the drift section; we just read it again with a different scaling factor.

**New components:** None.

**Modified components:** `process()` -- modify output voltage computation.

#### 2c. Pitch Slew (Rate Limiting)

**What it does:** Applies a gentle slew/lag to frequency changes, simulating the thermal inertia of analog oscillator cores. When you turn the Rate knob, the frequency does not change instantly but glides toward the new value.

**Integration point:** This is already partially implemented via `freqSlew` (TExponentialFilter with lambda=20). The v1.2 enhancement ties slew rate to the Drift knob: higher drift = more slew = more "analog" response to frequency changes.

```cpp
// Modify the existing freqSlew lambda based on drift:
float slewLambda = 20.f;  // base: 50ms (existing)
if (drift >= 0.001f) {
    // At full drift, slow down to lambda=5 (~200ms slew)
    slewLambda = 20.f - driftAmount * 15.f;
    slewLambda = std::fmax(slewLambda, 5.f);
}
freqSlew.setLambda(slewLambda);
float freq = freqSlew.process(args.sampleTime, targetFreq);
```

**New components:** None.

**Modified components:** `process()` -- make freqSlew lambda drift-dependent.

#### 2d. Component Spread

**What it does:** Simulates manufacturing tolerance in analog components by applying unique per-instance offsets to character parameters. Two modules with identical knob settings will produce subtly different waveforms, just like two units of the same analog synth.

**Integration point:** In the constructor (one-time initialization), generate per-instance random offsets. Apply these in `process()` when reading morph, character, and rate parameters.

```cpp
// In AnalogLFO struct:
float componentSpread[3] = {};  // morph, character, rate offsets

// In constructor:
componentSpread[0] = normalDist(rng) * 0.02f;  // +/- 2% morph offset
componentSpread[1] = normalDist(rng) * 0.03f;  // +/- 3% character offset
componentSpread[2] = normalDist(rng) * 0.01f;  // +/- 1% rate offset

// In process(), when reading params:
float morph = rack::math::clamp(morphKnob + morphAtten * morphCV / 5.f
              + componentSpread[0] * drift, 0.f, 1.f);
```

**Scaling with Drift knob:** Component spread is multiplied by the drift amount. At drift=0 (digital), there is zero component spread. At drift=1 (full analog), the offsets are fully applied. This keeps the "drift=0 means pristine digital" contract.

**Serialization:** Component spread values should NOT be serialized. They regenerate on module creation, giving each instance unique character. This matches the existing OU state non-serialization philosophy.

**New components:** `componentSpread[3]` array.

**Modified components:** Constructor (generate spreads), `process()` (apply spreads).

---

### Feature 3: Waveform Bleed in Morph Transitions

**What it does:** In real analog crossfader circuits, adjacent waveforms bleed slightly into the output even when not selected, due to imperfect switching and capacitive coupling. At morph=0.0 (pure sine), a tiny amount of triangle is audible. At morph=0.33 (pure triangle), traces of sine and saw are present.

**Integration point:** Inside `computeMorphedWave()` (step 10 of the pipeline). The existing linear crossfade between adjacent pairs becomes a wider crossfade where non-selected waveforms contribute a small amount.

**Current morph implementation:**
```cpp
float scaled = morph * 3.f;
int segment = std::min((int)scaled, 2);
float frac = scaled - (float)segment;
// Only two adjacent waveforms contribute at any point
```

**Modified implementation with bleed:**
```cpp
float computeMorphedWave(float phase, float morph, float character, float bleedAmount) {
    float sine = computeSine(phase, character);
    float tri  = computeTriangle(phase, character);
    float saw  = computeSaw(phase, character);
    float sqr  = computeSquare(phase, character);

    float scaled = morph * 3.f;
    int segment = std::min((int)scaled, 2);
    float frac = scaled - (float)segment;

    // Standard crossfade (existing behavior)
    float primary;
    switch (segment) {
        case 0: primary = sine + frac * (tri - sine); break;
        case 1: primary = tri  + frac * (saw - tri);  break;
        case 2: primary = saw  + frac * (sqr - saw);  break;
        default: primary = sqr; break;
    }

    if (bleedAmount < 0.001f) return primary;

    // Bleed: non-adjacent waveforms contribute a small amount
    // Weight falls off with distance from morph position
    float waves[4] = { sine, tri, saw, sqr };
    float bleed = 0.f;
    for (int i = 0; i < 4; i++) {
        float pos = (float)i / 3.f;  // normalized position of this waveform
        float dist = std::fabs(morph - pos);
        if (dist < 0.001f) continue;  // skip the primary contributor
        // Inverse-distance bleed, capped at bleedAmount
        float weight = bleedAmount * std::fmax(0.f, 1.f - dist * 3.f);
        bleed += weight * waves[i];
    }
    return primary * (1.f - bleedAmount * 0.5f) + bleed;
}
```

**Bleed amount source:** Tied to the Character knob (not Drift). Character already models analog circuit imperfections per-waveform; bleed is a natural extension of analog crossfader circuitry.

```cpp
float bleedAmount = progressiveCurve(character) * 0.08f;  // max 8% bleed
```

**Performance note:** All four waveforms are already computed in `computeMorphedWave()`. The bleed calculation adds a loop of 4 iterations with simple arithmetic -- negligible cost.

**Display buffer impact:** `updateDisplayBuffer()` also calls `computeMorphedWave()`. It must pass the bleed amount too. Since `updateDisplayBuffer` is called with morph and character, this is straightforward.

**New components:** None.

**Modified components:**
- `computeMorphedWave()` -- add bleed parameter and calculation
- `updateDisplayBuffer()` -- pass bleed amount through
- `process()` -- compute bleedAmount from character, pass to computeMorphedWave

---

### Feature 4: Separate RESET Jack

**What it does:** Provides a trigger input that resets the LFO phase to 0 (or to the phase offset value), independent of the CLK input. In v1.1, phase reset is tied to clock edges. A separate RESET jack allows manual/sequencer-driven phase resets without affecting clock tracking.

**Integration point:** Inside `process()`, after `processClockInput()` but before phase accumulation. Uses the same anti-click crossfade mechanism as clock-driven resets.

**Implementation:**

```cpp
// New member:
dsp::SchmittTrigger resetTrigger;

// In process(), after processClockInput():
if (inputs[RESET_INPUT].isConnected()) {
    float resetVoltage = inputs[RESET_INPUT].getVoltage();
    if (resetTrigger.process(resetVoltage, 0.1f, 1.0f)) {
        crossfadeFrom = lastOutputVoltage;
        crossfadeProgress = 0.f;
        phase = 0.0;  // or phase = phaseOffset if phase offset is implemented
        clockBeatCount = 0;  // reset division counter too
    }
}
```

**Interaction with CLK:** Both CLK and RESET can trigger phase resets independently. CLK continues to measure period and drive frequency in clocked mode. RESET only resets phase -- it does not affect clock tracking state (clockState, smoothedPeriod, etc.). This is the standard Eurorack convention (VCV Fundamental LFO has separate CLK and RESET inputs).

**Interaction with division counting:** When RESET fires, the division beat counter (`clockBeatCount`) resets to 0. This ensures the LFO re-aligns with the clock after a manual reset, starting a fresh division cycle on the next clock edge.

**New components:**
- `RESET_INPUT` enum entry
- `dsp::SchmittTrigger resetTrigger` member
- RESET jack on panel

**Modified components:**
- `process()` -- add reset processing after clock processing
- Constructor: `configInput(RESET_INPUT, "Reset")`
- Widget: add RESET jack

---

### Feature 5: Phase Offset Knob/CV

**What it does:** Adds a constant offset to the phase used for waveform generation. At offset=0, the waveform starts at the beginning. At offset=0.5, it starts halfway through. CV-modulatable for animated phase shifting.

**Integration point:** Between phase accumulation (step 7) and waveform generation (step 10). Applied to the phase readout, NOT the accumulator (same principle as phase jitter).

```cpp
// New params:
// PHASE_OFFSET_PARAM: 0.0 to 1.0 (knob)
// PHASE_OFFSET_ATTEN_PARAM: attenuator trimpot
// PHASE_OFFSET_CV_INPUT: CV input

// In process(), after phase accumulation:
float phaseOffsetKnob = params[PHASE_OFFSET_PARAM].getValue();
float phaseOffsetAtten = params[PHASE_OFFSET_ATTEN_PARAM].getValue();
float phaseOffsetCV = inputs[PHASE_OFFSET_CV_INPUT].getVoltage();
float phaseOffset = phaseOffsetKnob + phaseOffsetAtten * phaseOffsetCV / 10.f;
// Note: /10.f because 0-10V unipolar maps to 0-1 cycle offset

float p = (float)phase + phaseOffset;
p = p - std::floor(p);  // wrap to [0, 1)

// Apply jitter (if drift enabled)
// ... then pass p to computeMorphedWave(p, morph, character, bleedAmount)
```

**Why divide by 10 (not 5):** Phase offset is a unipolar 0-1 parameter (fraction of a cycle). A 0-10V unipolar CV should sweep the full cycle. Using /5 would mean a bipolar +/-5V signal sweeps 0-2 cycles, which wraps and works but is less intuitive for the common case of unipolar CV.

**Interaction with RESET:** When RESET triggers, phase resets to 0.0. The phase offset is added downstream, so after a reset, the output waveform starts at the offset position. This is correct behavior -- a module with phase offset 0.25 and a reset will always restart at the 90-degree point of the waveform.

**Display impact:** The display buffer shows one cycle of the waveform without phase offset applied (it is a shape preview, not a real-time phase-accurate view). The phase dot position should reflect the offset: `displayPhase.store(p, ...)` where `p` includes the offset. This way the dot shows where in the waveform the output currently sits.

**New components:**
- `PHASE_OFFSET_PARAM` enum entry
- `PHASE_OFFSET_ATTEN_PARAM` enum entry (optional -- panel space dependent)
- `PHASE_OFFSET_CV_INPUT` enum entry
- Phase offset knob and CV jack on panel

**Modified components:**
- `process()` -- add phase offset calculation between accumulation and waveform generation
- Constructor: config new params/inputs

---

### Feature 6: Swing/Shuffle Control

**What it does:** In clocked mode, alternately shortens and lengthens consecutive half-cycles, creating a swing/shuffle feel. At 50% swing = straight time. At 66% swing = classic triplet shuffle (the "standard" swing feel). At 75% swing = heavy dotted-eighth swing.

**Integration point:** Modifies the phase accumulation rate on alternate half-cycles. The swing control changes deltaPhase dynamically based on which half of the LFO cycle is active.

**Algorithm:**

Swing is defined as the percentage of a beat pair occupied by the first beat. At 50%, both beats are equal (straight). At 66%, the first beat takes 2/3 of the time and the second takes 1/3 (triplet feel).

For an LFO, one "cycle" is the unit. Swing alternates the speed during the first half (phase 0 to 0.5) vs second half (phase 0.5 to 1.0):

```cpp
// New param:
// SWING_PARAM: 50 to 75 (percentage), default 50

// In process(), after computing freq but before deltaPhase:
float swingParam = params[SWING_PARAM].getValue();  // 50.0 to 75.0
if (isClocked && swingParam > 50.5f) {
    float swingRatio = swingParam / 100.f;  // 0.5 to 0.75
    // In first half of cycle, slow down; in second half, speed up
    // So that first half takes swingRatio of the total period
    // and second half takes (1-swingRatio)
    bool inFirstHalf = (phase < 0.5);
    float speedMultiplier;
    if (inFirstHalf) {
        speedMultiplier = 0.5f / swingRatio;      // < 1.0, slows down
    } else {
        speedMultiplier = 0.5f / (1.f - swingRatio);  // > 1.0, speeds up
    }
    deltaPhase *= (double)speedMultiplier;
}
```

**At 66% swing:** First half: speedMultiplier = 0.5/0.66 = 0.757 (slower). Second half: speedMultiplier = 0.5/0.34 = 1.47 (faster). The output waveform spends more time in the first half, creating the "lazy" feel of swing.

**Clocked-only:** Swing only makes musical sense in clocked mode. In free-running mode, the swing parameter should be ignored (or displayed as inactive). The total period remains unchanged -- swing redistributes time within the cycle without changing the overall frequency.

**Interaction with division ratios:** Swing applies per LFO cycle regardless of division ratio. At /4 (one LFO cycle per 4 clock beats), the swing creates an uneven distribution of the LFO's first vs second half across the 4-beat span. This is musically correct -- the LFO output has a "swung" shape in time.

**Interaction with phase offset:** Phase offset shifts where in the cycle the LFO reads its waveform. Swing changes the speed of phase advancement through the cycle. These are independent and compose correctly. However, the `inFirstHalf` test should use the raw `phase` (before offset), because swing is about temporal distribution of the cycle, not about where the output reads.

**New components:**
- `SWING_PARAM` enum entry (50-75 range)
- Swing knob on panel (small knob, only active in clocked mode)

**Modified components:**
- `process()` -- add swing speed modulation after freq computation, before phase accumulation

---

### Feature 7: Display Text Overlay Readability

**What it does:** Adds semi-transparent HUD backgrounds behind text overlays so they remain readable when the waveform trace passes behind them.

**Integration point:** In `WaveformDisplay::drawTextOverlays()`, render a rounded rect background behind each text element before drawing the text.

```cpp
void drawTextBackground(NVGcontext* vg, float x, float y,
                        const char* text, float fontSize, int align, float alpha) {
    // Measure text bounds
    float bounds[4];
    nvgTextBounds(vg, x, y, text, NULL, bounds);
    float pad = 2.f;
    nvgBeginPath(vg);
    nvgRoundedRect(vg, bounds[0] - pad, bounds[1] - pad,
                   bounds[2] - bounds[0] + 2*pad, bounds[3] - bounds[1] + 2*pad, 2.f);
    nvgFillColor(vg, nvgRGBAf(0.051f, 0.051f, 0.102f, alpha * 0.7f));
    nvgFill(vg);
}
```

**Modified components:**
- `drawGlowText()` or `drawTextOverlays()` -- add background rect before text rendering

**No new DSP components.** Pure display change.

---

### Feature 8: Incoming Clock BPM Display

**What it does:** Shows the raw incoming clock BPM alongside the effective (ratio-adjusted) BPM. Currently only effective BPM is shown.

**Integration point:** In `WaveformDisplay::drawTextOverlays()`, add a second BPM readout.

```cpp
// In drawTextOverlays(), after effective BPM:
if (bpmFadeAlpha > 0.001f) {
    float period = module->displaySmoothedPeriod.load(std::memory_order_relaxed);
    if (period > 0.f) {
        float clockBPM = 60.f / period;
        std::string clockBpmText = rack::string::f("%d", (int)std::round(clockBPM));
        // Draw at bottom-left: "CLK: 120"
        drawGlowText(vg, font->handle, margin, box.size.y - margin,
                     clockBpmText.c_str(), fontSize,
                     NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM, bpmFadeAlpha);
    }
}
```

**No new atomics needed:** `displaySmoothedPeriod` already exists and stores the raw clock period.

**Modified components:**
- `drawTextOverlays()` -- add clock BPM text rendering

---

## Revised Data Flow (v1.2)

```
process():
  1.  processClockInput(sampleTime)            // clock edges, period EMA
  2.  Process RESET input                       // NEW: independent phase reset
  3.  Compute targetFreq (clocked or free)
  4.  Apply FM modulation to targetFreq         // NEW: exponential FM
  5.  Apply drift-dependent pitch slew          // MODIFIED: variable slew rate
  6.  Compute swing speed multiplier            // NEW: clocked-mode only
  7.  deltaPhase = freq * sampleTime * swing
  8.  Apply drift: OU layers modulate deltaPhase
  9.  phase += deltaPhase; wrap [0, 1)
  10. Compute phase offset                      // NEW: knob + CV
  11. Apply phase jitter                        // NEW: per-sample noise on readout
  12. p = phase + phaseOffset + jitter; wrap
  13. Read morph/character params + CV
  14. Apply component spread to params          // NEW: per-instance offsets
  15. Compute bleed amount from character        // NEW
  16. computeMorphedWave(p, morph, character, bleed)  // MODIFIED signature
  17. Apply DC offset drift                     // NEW: slow wander on output
  18. Apply crossfade if recent phase reset
  19. Output voltage
```

### Display Data Flow (v1.2 Additions)

```
Audio Thread                               GUI Thread
-----------                               ----------
process()                                  WaveformDisplay::drawLayer()
  -> displayPhase.store(p)                   // p now includes offset
                                             -> drawTextBackground() before text  // NEW
                                             -> draw clock BPM text               // NEW
```

No new atomics needed for v1.2. All new features either use existing atomics or are internal to the audio thread (jitter, bleed, FM, swing, DC offset, component spread, pitch slew). The display already has all the state it needs.

---

## Component Boundaries

| Component | Responsibility | v1.2 Changes |
|-----------|---------------|--------------|
| Phase accumulator | `phase += deltaPhase` with wrap | Swing modulates deltaPhase; reset jack adds second reset source |
| Frequency computation | Rate knob or clock-derived freq | FM input added; pitch slew made drift-dependent |
| Drift engine (OU layers) | Frequency modulation via OU process | DC offset drift reuses OU layer 0 output; no new OU layers needed |
| Waveform engine | `computeMorphedWave()` | Bleed parameter added; phase jitter applied before lookup |
| Character engine | Per-shape analog deformation | Component spread adds per-instance parameter offsets |
| Clock tracker | Edge detection, period EMA, phase reset | Unchanged; RESET jack is separate from CLK logic |
| Display bridge (atomics) | Lock-free audio-to-GUI state | displayPhase now includes phase offset; text backgrounds added |
| Display renderer | NanoVG waveform + overlays | HUD backgrounds, clock BPM readout added |

---

## Panel Layout Considerations

v1.2 adds significant new panel components:
- **FM jack** (input)
- **FM attenuverter trimpot**
- **RESET jack** (input)
- **PHASE OFFSET knob** (with CV input and trimpot)
- **SWING knob** (small, clocked-mode only)

The current 12HP panel is moderately populated. Options:

### Option A: Stay at 12HP (recommended)

Rearrange bottom section to accommodate new components. The current bottom section has 4 trimpots and 4 jacks in two rows. Adding FM and Phase Offset CV inputs plus a RESET jack requires careful layout.

Proposed layout:
```
y=15-42mm   Waveform display (57x27mm) -- unchanged
y=54mm      MORPH knob (center) -- unchanged
y=69mm      CHARACTER (x=18) + DRIFT (x=42.96) -- unchanged
y=80mm      RATE (x=10) + PHASE OFFSET (x=30, small) + SWING (x=50, small)
y=90mm      CLK (x=10) + RST (x=25) + FM (x=40) + OUT (x=52)
y=100mm     Trimpots: Morph(x=8) Char(x=20) Drift(x=32) FM(x=44) PhOff(x=56)
y=112mm     CVs: MorphCV(x=8) CharCV(x=20) DriftCV(x=32) FMCV(x=44) PhOffCV(x=56)
```

This is tight but achievable at 12HP. The SWING knob does not need a CV input (it is a performance control, not a modulation target -- and adding CV would require yet another jack).

### Option B: Expand to 14HP

More breathing room, but breaks existing patch layouts and panel SVG. Not recommended unless 12HP proves too cramped after prototyping.

**Recommendation: Start with Option A at 12HP.** The panel can be iterated during visual verification. If it is too cramped, the FM attenuverter trimpot could be removed (fixed-depth FM is simpler but less flexible) or Phase Offset CV could be deferred.

---

## Patterns to Follow

### Pattern 1: Phase Readout Modification (not Accumulator Modification)

**What:** Features that affect where the waveform is "read" (phase offset, jitter) modify a local copy of the phase, not the accumulator itself.

**Why:** The accumulator is the single source of truth for cycle timing. Modifying it with transient effects (jitter) would cause permanent phase error accumulation. Phase offset is a constant shift in reading position, not a change in oscillation rate.

```cpp
// CORRECT: modify readout phase
float p = (float)phase;           // copy from accumulator
p += phaseOffset + jitter;        // modify copy
p = p - std::floor(p);            // wrap
sample = computeMorphedWave(p, ...);

// WRONG: modify accumulator
phase += jitter;  // jitter accumulates! phase drifts permanently
```

### Pattern 2: Drift-Scaled Imperfections

**What:** New analog imperfections (jitter, DC offset, pitch slew, component spread) all scale with the Drift knob value.

**Why:** The Drift knob is the established "analog-ness" axis. Drift=0 must always produce pristine digital output. Adding imperfections that ignore the Drift knob would break this contract.

```cpp
float driftAmount = progressiveCurve(drift);  // 0 to 1, x^2 curve
float jitter = driftAmount * 0.003f * noise;     // scales with drift
float dcDrift = driftAmount * 0.04f * ouState;    // scales with drift
float slewMod = driftAmount * 15.f;               // scales with drift
float spread = componentSpread[i] * drift;        // scales with drift (linear, not curved)
```

### Pattern 3: Reuse Existing OU Layers for New Effects

**What:** DC offset drift reuses the existing OU layer 0 state rather than creating a new random process.

**Why:** OU layer 0 (0.05Hz slow wander) has the right temporal character for DC drift. Adding a fifth OU layer would increase per-sample CPU cost. Reading the existing state a second time with a different scaling factor costs nothing.

### Pattern 4: Swing as Phase Rate Modulation

**What:** Swing modifies deltaPhase, not the waveform shape. The same waveform is produced but with uneven temporal distribution.

**Why:** Swing is a timing effect, not a waveshaping effect. The waveform should look the same when plotted against phase -- only the time spent at each phase point changes.

---

## Anti-Patterns to Avoid

### Anti-Pattern 1: FM on the Phase Accumulator Directly

**What people do:** Add FM voltage directly to `deltaPhase` without converting to proper frequency scaling.

**Why it is wrong:** At LFO rates, linear FM causes the output frequency to go negative when the modulation exceeds the base frequency, producing phase reversal. Exponential FM (multiply frequency by `pow(2, fmVoltage)`) keeps frequency positive and gives musically useful scaling.

**Do this instead:** Convert FM voltage to a frequency multiplier via `pow(2, fmAmount)` and multiply the base frequency.

### Anti-Pattern 2: Applying Phase Offset to the Accumulator

**What people do:** `phase += phaseOffset` in the accumulator, then try to subtract it back out for cycle counting.

**Why it is wrong:** The accumulator must wrap at 1.0 to mark cycle boundaries. Adding a constant offset shifts when wrapping occurs, breaking beat counting, display phase tracking, and division-aware reset logic.

**Do this instead:** Keep the accumulator clean. Apply offset to a local readout copy.

### Anti-Pattern 3: Separate Controls for Each Imperfection

**What people do:** Add individual knobs for jitter amount, DC offset amount, slew time, component spread.

**Why it is wrong for this module:** The three-knob design (morph, character, drift) IS the identity. Adding 4 more knobs for imperfections destroys panel simplicity and creates option paralysis.

**Do this instead:** Tie all imperfections to the Drift knob with curated proportions. Power users can modulate Drift via CV for dynamic control over all imperfections simultaneously.

### Anti-Pattern 4: Swing in Free-Running Mode

**What people do:** Apply swing even when there is no clock reference.

**Why it is wrong:** Without a clock, "swing" has no musical meaning. A free-running LFO at 0.3Hz with swing just produces an irregular waveform with no rhythmic context. Users expecting swing to create a groove feel will be confused.

**Do this instead:** Only apply swing in clocked mode (ACQUIRING or LOCKED state). In free mode, the swing parameter is ignored.

---

## Suggested Build Order

Based on dependency analysis and testing efficiency:

### Phase 1: Display Polish (HUD Backgrounds + Clock BPM)

**Add:** Text background rects in `drawTextOverlays()`, incoming clock BPM text.
**Modify:** `drawGlowText()` or add `drawTextBackground()` helper.
**Test:** Visual verification -- text readable over bright waveform peaks, clock BPM shows correct value.
**Why first:** Purely visual, no DSP changes, no new jacks/params. Quick win that improves existing functionality. Establishes the display baseline before other features add new display elements.

### Phase 2: RESET Jack + Phase Offset

**Add:** `RESET_INPUT`, `resetTrigger`, `PHASE_OFFSET_PARAM`, `PHASE_OFFSET_CV_INPUT`.
**Modify:** `process()` -- add reset processing and phase offset calculation.
**Test:** Trigger reset, verify phase snaps to offset position with anti-click crossfade. Modulate phase offset with CV, verify smooth animation.
**Why second:** These two features are tightly coupled (reset-to-offset behavior) and should be designed together. They modify the phase readout path, which must be stable before other features layer onto it.
**Dependencies:** None (uses existing crossfade mechanism).

### Phase 3: FM Input

**Add:** `FM_INPUT`, `FM_ATTEN_PARAM`, FM processing in `process()`.
**Modify:** Frequency computation section.
**Test:** Patch an LFO into FM input, verify frequency wobble. Test in both free and clocked modes. Verify extreme FM does not cause phase explosion (safety clamp).
**Why third:** Independent of Phase 2 features. Modifies frequency computation, which is upstream of all phase/waveform logic.
**Dependencies:** None.

### Phase 4: Expanded Analog Imperfections

**Add:** Phase jitter, DC offset drift, drift-dependent pitch slew, component spread.
**Modify:** `process()` at four insertion points, constructor for component spread generation.
**Test:** Sweep drift knob 0 to 1, verify progressive imperfection introduction. Verify drift=0 is perfectly clean. Compare two module instances for different component spread. Verify DC offset on scope.
**Why fourth:** These features layer onto the phase readout and output paths established in Phases 2-3. They should be added together because they share the drift-scaling pattern and are tested as a group.
**Dependencies:** Phase 2 (phase readout path must be established for jitter to layer correctly).

### Phase 5: Waveform Bleed in Morph

**Add:** Bleed calculation in `computeMorphedWave()`.
**Modify:** `computeMorphedWave()` signature and implementation, `updateDisplayBuffer()`, all call sites.
**Test:** Set morph to pure sine (0.0), sweep character from 0 to 1, verify subtle triangle bleed appears. Check display reflects bleed. Verify no amplitude change at morph boundaries.
**Why fifth:** Modifies the waveform engine signature, which affects both audio and display paths. Best done after other waveform-path changes (jitter, phase offset) are stable.
**Dependencies:** None strictly, but changing `computeMorphedWave()` signature touches a core function -- better to do after other modifications to that pipeline are settled.

### Phase 6: Swing/Shuffle

**Add:** `SWING_PARAM`, swing speed calculation in `process()`.
**Modify:** deltaPhase computation in clocked mode.
**Test:** Set clock to steady tempo, sweep swing from 50% to 75%, verify asymmetric timing on scope. Verify swing has no effect in free mode. Test with various division ratios.
**Why sixth:** Swing modifies deltaPhase computation, which is upstream of phase accumulation. It interacts with the clock system and must be tested with real clock input. Saving it for late ensures the clock system and phase path are fully stable.
**Dependencies:** Clock system (existing), Phase 2 (phase offset must not interfere with swing's half-cycle detection).

### Phase 7: Panel Layout + Visual Verification

**Add:** Updated SVG panel with all new jacks, knobs. Widget positions for all new components.
**Test:** Full visual verification in VCV Rack. Check panel density, label readability, knob spacing. Verify all features interact correctly with the display.
**Why last:** Panel layout depends on knowing the final component count. v1.0 retrospective specifically noted that incremental panel changes caused rework. Design for the final state.
**Dependencies:** All prior phases (need to know exact jack/knob count).

---

## Interaction Matrix

How v1.2 features interact with each other and existing systems:

| Feature | Phase Accum | Freq Comp | Drift Engine | Waveform Engine | Clock Tracker | Display |
|---------|------------|-----------|-------------|-----------------|---------------|---------|
| FM Input | - | MODIFIES freq | - | - | - | - |
| Phase Jitter | reads phase | - | uses RNG | - | - | - |
| DC Offset | - | - | reads OU[0] | - | - | - |
| Pitch Slew | - | MODIFIES slew | reads drift | - | - | - |
| Component Spread | - | - | reads drift | reads morph/char | - | - |
| Waveform Bleed | - | - | - | MODIFIES output | - | MODIFIES buffer |
| RESET Jack | RESETS phase | - | - | - | resets beatCount | - |
| Phase Offset | reads phase | - | - | - | - | modifies dotPos |
| Swing | MODIFIES dPhase | reads freq | - | - | reads clockState | - |
| HUD Backgrounds | - | - | - | - | - | MODIFIES render |
| Clock BPM | - | - | - | - | reads period | MODIFIES render |

Key observations:
- **No feature modifies the clock tracker itself.** This is good -- the clock system remains a stable foundation.
- **Phase accumulator has three modifiers:** swing (deltaPhase), drift (deltaPhase), and reset (snap to 0). These are well-ordered in the pipeline.
- **Frequency computation has two modifiers:** FM and pitch slew. FM is applied before slew, which is correct (slew smooths the FM-modulated frequency).
- **No circular dependencies.** The build order respects the unidirectional data flow.

---

## Scalability Considerations

| Concern | Current (890 LOC) | Post-v1.2 (~1100 LOC) | Mitigation |
|---------|-------------------|----------------------|------------|
| File size | Single file, manageable | Still single file but getting long | Consider splitting WaveformDisplay into separate file at ~1200 LOC |
| process() length | ~115 lines | ~160 lines | Group related operations with comments; consider helper methods for FM, swing, imperfections |
| CPU per sample | 4 OU layers + waveform compute | Same + jitter noise + DC offset read + swing branch | Minimal impact; all additions are O(1) arithmetic |
| Param/Input count | 7 params, 4 inputs | ~11 params, ~7 inputs | Approaching limits of 12HP panel density |
| Display atomics | 5 atomics | 5 atomics (no new ones) | Clean; all new features are audio-thread-internal |

---

## Sources

- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) -- FM input scaling, trigger thresholds (HIGH confidence)
- [VCV Fundamental LFO source (GitHub)](https://github.com/VCVRack/Fundamental/blob/v2/src/LFO.cpp) -- Reference FM implementation, RESET input pattern (HIGH confidence)
- [VCV Community: FM CV levels](https://community.vcvrack.com/t/fm-cv-levels-correspond-to-what-exactly/13561) -- FM scaling conventions (MEDIUM confidence)
- [KVR Audio: Implementing swing](https://www.kvraudio.com/forum/viewtopic.php?t=261858) -- Swing/shuffle algorithm patterns (MEDIUM confidence)
- [JMK Music Pedals: MIDI Clock Swing](https://jmkmusicpedals.com/blogs/jmk/midi-clock-swing) -- TR-909 shuffle timing values (MEDIUM confidence)
- [Mod Wiggler: LFO with sync and reset](https://www.modwiggler.com/forum/viewtopic.php?t=222452) -- Eurorack RESET jack conventions (MEDIUM confidence)
- [Mod Wiggler: Crossfading vs wave shaping](https://www.modwiggler.com/forum/viewtopic.php?t=283479) -- Waveform bleed in analog crossfaders (MEDIUM confidence)
- Existing codebase: `src/AnalogLFO.cpp` at 890 lines -- Direct inspection (HIGH confidence)
- Project retrospective and milestone audit -- Architectural patterns and lessons learned (HIGH confidence)

---
*Architecture research for: v1.2 Deep Analog integration into Analog Series LFO*
*Researched: 2026-03-13*
