# Phase 4: Analog Character - Research

**Researched:** 2026-03-06
**Domain:** DSP waveform deformation, analog oscillator modeling, CV input processing in VCV Rack
**Confidence:** HIGH

## Summary

Phase 4 implements the character knob -- the core differentiator of this module. At zero, output is the mathematically perfect digital morph waveform already implemented in Phase 2. At full, each waveform shape exhibits the specific analog characteristics of its reference synth: Minimoog saw (exponential ramp curvature, soft capacitor reset), Roland SH-101/Juno-106 square (sigmoid edge softening, duty cycle asymmetry), Moog/Prophet triangle (rounded peaks, slope asymmetry), and triangle-derived analog sine (residual harmonic distortion). The character knob progressively crossfades between these extremes, with a progressive curve that rewards knob exploration.

This phase also adds Character CV input with its own attenuator trimpot, following the identical pattern established by Morph CV in Phase 2. CHAR-06 (HF rolloff via pitch-tracking lowpass) is deferred to the VCO module per the CONTEXT.md decision -- sub-audio LFO rates (0.01-20Hz) have no high-frequency harmonic content to roll off.

**Primary recommendation:** Implement character as "characterize-then-morph" -- apply per-shape analog deformation to each base waveform, then morph between the characterized versions. This produces the most visually and sonically coherent result across the morph range and requires no special handling at morph transition zones. Keep all DSP inline in AnalogLFO.cpp (no separate files) following the established single-file pattern.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Progressive mapping: subtle analog coloring in the first half, stronger deformations in the second half
- Full character (100%) = authentic realism matching the reference synth, not exaggeration or caricature
- The useful range should reward exploration -- most of the knob travel produces musically interesting results
- Character CV input: additive offset behavior (knob sets center position, CV sweeps around it, same as Morph CV)
- Hard clamp at 0-1 range when combined knob + CV exceeds bounds
- Own attenuator trimpot matching the Morph CV pattern (new param: CHARACTER_ATTEN_PARAM)
- Character knob at zero must produce bit-identical output to current digital waveforms -- no processing overhead when character is off
- CHAR-06 (HF rolloff) deferred to VCO module -- sub-audio LFO rates have no high-frequency content to roll off

### Claude's Discretion
- Morph-character ordering (characterize-then-morph vs morph-then-characterize)
- Whether to include morph bleed scaled with character (or defer to v2 IO-03)
- Character CV jack and attenuator placement on panel
- Exact progressive curve shape (exponential, power curve, etc.)
- DSP implementation approach for each analog reference model

### Deferred Ideas (OUT OF SCOPE)
- CHAR-06 (HF rolloff via pitch-tracking lowpass) -- deferred to VCO module
- Waveform bleed in morph transition zones (IO-03) -- may be included if it enhances character without significant complexity, otherwise v2
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CHAR-01 | Character knob crossfades each waveform from digital perfection to classic analog reference | Progressive curve mapping, per-shape analog deformation functions, zero-cost bypass at character=0 |
| CHAR-02 | Saw reference targets Minimoog Model D (exponential ramp curvature ~2-3%, soft capacitor reset ~10-20us) | Exponential ramp bend formula, soft reset transition via short polynomial curve, pitch-relative reset width |
| CHAR-03 | Square reference targets Roland SH-101/Juno-106 (sigmoid edge softening ~2-5us, duty cycle asymmetry 0.5-1.5%) | tanh-based edge softening, duty cycle offset application, pitch-relative edge width |
| CHAR-04 | Triangle reference targets Moog/Prophet (rounded peaks via polynomial caps, slope asymmetry 2-3%) | Polynomial peak rounding at direction reversals, asymmetric slope implementation |
| CHAR-05 | Sine reference models triangle-derived analog sine (1-3% residual THD, primarily 3rd harmonic) | Chebyshev polynomial mixing for controlled harmonic injection, triangle-derived waveshaping approach |
| CHAR-06 | HF rolloff via pitch-tracking lowpass | DEFERRED to VCO -- not meaningful at sub-audio LFO rates per CONTEXT.md |
| CHAR-07 | Character CV input modulates character amount | Additive offset + attenuator pattern (matches Morph CV), new CHARACTER_ATTEN_PARAM + CHARACTER_CV_INPUT |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack 2 SDK | 2.5.x | Plugin framework | Only option for VCV Rack modules |
| C++17 / `<cmath>` | compiler | Math functions (std::tanh, std::pow, std::fabs) | Already used throughout codebase |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `rack::math::clamp` | SDK | CV clamping to 0-1 range | Character CV processing |
| `std::tanh` | C++17 | Sigmoid edge softening for square wave | Called per-sample only when character > 0 |
| `std::pow` | C++17 | Progressive curve mapping, exponential ramp curvature | Knob-to-parameter conversion (not per-sample critical) |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Inline character functions | Separate CharacterStage class in dsp/ folder | Architecture research suggested separate files, but actual codebase uses single-file pattern (all in AnalogLFO.cpp). Follow established pattern. |
| tanh for sigmoid | Polynomial approximation (x / (1 + abs(x))) | tanh is more accurate; at LFO rates CPU cost is negligible. Use real tanh. |
| Per-sample character computation | Wavetable-based character | Wavetables require interpolation artifacts management and can't smoothly vary with morph position. Procedural computation is simpler and more flexible for a morphing oscillator. |

**Installation:**
```bash
# No new dependencies -- all math is standard C++ and VCV Rack SDK
make
```

## Architecture Patterns

### Recommended Code Organization
All character code goes in `src/AnalogLFO.cpp` following the established single-file pattern:
```
src/AnalogLFO.cpp
  struct AnalogLFO : Module {
    // New: CHARACTER_ATTEN_PARAM in ParamId enum
    // New: CHARACTER_CV_INPUT in InputId enum
    // Modified: computeMorphedWave() -- now takes character param
    // New: applyCharacter() helper functions per shape
    // Modified: process() -- Character CV processing, display update trigger
    // Modified: updateDisplayBuffer() -- passes character to wave computation
  }
  struct WaveformDisplay : TransparentWidget { ... }  // unchanged
  struct AnalogLFOWidget : ModuleWidget { ... }  // new widgets added
```

### Pattern 1: Characterize-Then-Morph (Recommended)
**What:** Apply analog deformation to each individual waveform shape BEFORE morphing between them.
**When to use:** Always -- this is the recommended approach for this module.
**Why:** Each shape gets its own accurate analog reference deformation. The morph crossfade naturally blends between characterized shapes, producing smooth transitions without special handling. The display shows coherent waveform evolution.

```cpp
// Recommended approach: each base shape has its own analog version
float computeMorphedWave(float phase, float morph, float character) {
    // Generate analog-characterized versions of each base shape
    float sine = computeSine(phase, character);
    float tri  = computeTriangle(phase, character);
    float saw  = computeSaw(phase, character);
    float sqr  = computeSquare(phase, character);

    // Morph between characterized shapes (same segmentation as before)
    float scaled = morph * 4.f;
    int segment = std::min((int)scaled, 3);
    float frac = scaled - (float)segment;

    switch (segment) {
        case 0: return sine + frac * (tri - sine);
        case 1: return tri  + frac * (saw - tri);
        case 2: return saw  + frac * (sqr - saw);
        case 3: return sqr;
    }
    return sine;
}
```

**Alternative rejected: morph-then-characterize.** This would apply a single deformation to the already-morphed waveform, but there is no single analog reference for arbitrary morph positions. It cannot produce Minimoog-specific saw character alongside Juno-specific square character.

### Pattern 2: Zero-Cost Bypass
**What:** Skip all character processing when character param is zero.
**When to use:** Always check at the top of character-related functions.
**Why:** Preserves bit-identical output to current digital waveforms (locked decision from CONTEXT.md) and avoids unnecessary CPU when character is off.

```cpp
float computeSaw(float phase, float character) {
    float saw = 1.f - 2.f * phase;  // perfect digital saw (falling ramp)
    if (character < 0.001f) return saw;  // fast path: bit-identical to current

    // Apply Minimoog-style deformations scaled by character...
    float c = progressiveCurve(character);
    // ... deformation code ...
    return saw + c * (analogSaw - saw);
}
```

### Pattern 3: Progressive Curve Mapping
**What:** Map the raw character knob (0-1) through a progressive curve so subtle coloring occupies the first half and stronger deformations the second half.
**When to use:** Apply to the character parameter before using it to scale deformations.
**Why:** Locked decision -- rewards exploration, most of the knob travel produces musically interesting results.

```cpp
// Power curve: x^2 gives subtle first half, aggressive second half
// character 0.0 -> 0.0 (no effect)
// character 0.5 -> 0.25 (25% of full effect -- subtle)
// character 0.7 -> 0.49 (49% -- noticeable but tasteful)
// character 1.0 -> 1.0 (full analog reference)
float progressiveCurve(float character) {
    return character * character;  // simplest progressive mapping
}
```

**Alternative curves considered:**
- `x^3` -- too subtle in the first 75%, too aggressive in the last 25%
- `x^1.5` -- reasonable but `x^2` is simpler and sufficient
- `exp(k*x)-1` -- overly complex for marginal benefit

**Recommendation:** Use `x^2` (quadratic). It provides the desired progressive feel, is computationally trivial, and can be tuned easily by adjusting the exponent if needed during implementation.

### Pattern 4: CV Processing (Established Pattern)
**What:** Replicate the Morph CV pattern for Character CV.
**When to use:** For Character CV input processing in process().

```cpp
// Exact same pattern as Morph CV (line 91-95 in current code)
float charKnob = params[CHARACTER_PARAM].getValue();
float charAtten = params[CHARACTER_ATTEN_PARAM].getValue();
float charCV = inputs[CHARACTER_CV_INPUT].getVoltage();
float character = rack::math::clamp(charKnob + charAtten * charCV / 10.f, 0.f, 1.f);
```

### Anti-Patterns to Avoid
- **Over-deformation at character=1.0:** The analog references should sound authentic, not exaggerated. A Minimoog saw is subtly curved, not dramatically bent. Less is more.
- **Per-shape state in audio thread:** All character deformations should be stateless functions of phase and character. No filters, no accumulators, no memory allocation. Each sample computes independently.
- **Separate file for character code:** The codebase uses a single-file pattern. Do not create `src/dsp/CharacterStage.hpp` -- inline everything in AnalogLFO.cpp.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| CV clamping | Custom min/max logic | `rack::math::clamp()` | Already used in codebase, handles edge cases |
| Sigmoid function | Custom polynomial | `std::tanh()` | At LFO rates, `tanh` is fast enough; no need for approximation |
| Progressive knob curve | Complex exponential formula | `character * character` (x^2) | Simple, tunable, matches the requirement perfectly |
| Display update trigger | Custom change detection | Same threshold pattern from Phase 3 | `std::fabs(character - prevDisplayCharacter) > 0.002f` |

**Key insight:** The character modeling is pure math -- no external libraries, no state machines, no filters (at LFO rates). Each analog deformation is a mathematical function of phase and character amount. This keeps the implementation simple and stateless.

## Common Pitfalls

### Pitfall 1: Uncanny Valley -- Character That Sounds Wrong, Not Warm
**What goes wrong:** Analog deformations that are theoretically correct but perceptually unconvincing. Over-driven curvature, excessive asymmetry, or distortion amounts that make the waveform sound "broken" rather than "analog."
**Why it happens:** Literature values (2-3% curvature, 0.5-1.5% asymmetry) are starting points, not gospel. Real analog character is subtle.
**How to avoid:** Start with conservative values (half the documented ranges), listen, and increase only if the effect is too subtle. The display provides immediate visual feedback -- use it to verify deformations look natural.
**Warning signs:** If character at 50% sounds worse than character at 0%, the modeling is wrong.

### Pitfall 2: Phase-Dependent Artifacts in Saw Reset
**What goes wrong:** The soft capacitor reset for the saw wave creates a visible "bump" or discontinuity at the reset point (phase=0 or phase=1) that looks wrong on the display and may create a click at higher LFO rates.
**Why it happens:** The transition from the ramp portion to the reset portion needs to be smooth -- both the value and the first derivative should be continuous at the transition point.
**How to avoid:** Use a smooth blending function (e.g., smoothstep or cosine interpolation) for the reset region, not a hard switch between "ramp" and "reset" sections.
**Warning signs:** Visible corner or kink at the saw reset point on the waveform display.

### Pitfall 3: Morph Transition Artifacts With Character
**What goes wrong:** When morphing between two characterized shapes (e.g., analog saw to analog square), the linear crossfade produces intermediate shapes that have odd-looking or -sounding artifacts.
**Why it happens:** The analog deformations for each shape are different in nature (curvature vs. edge softening), and naive linear blending of these different deformation types can produce non-physical intermediate shapes.
**How to avoid:** The "characterize-then-morph" approach naturally handles this because each shape is a valid, complete waveform and the crossfade blends between two valid waveforms. Verify visually across the full morph range at character=1.0.
**Warning signs:** The display shows visually jarring waveforms at specific morph positions (e.g., halfway between saw and square).

### Pitfall 4: Character CV Breaking Zero-Cost Bypass
**What goes wrong:** When Character CV is connected but the attenuator is at zero AND the knob is at zero, the character processing still runs because the bypass check only looks at the knob, not the final computed character value.
**How to avoid:** Check the bypass condition on the FINAL computed character value (after knob + CV + clamp), not on the raw knob value.

### Pitfall 5: Display Not Reflecting Character Changes
**What goes wrong:** The display shows digital waveforms even when the character knob is turned up, because the display update trigger (line 101-107 in current code) does not detect character parameter changes.
**Why it happens:** The existing TODO comment at line 101 explicitly notes this: "Phase 5/6: add characterChanged and driftChanged triggers here."
**How to avoid:** Add character change detection to the display update trigger, exactly matching the morph change pattern.

## Code Examples

Verified patterns derived from existing codebase and DSP literature:

### Minimoog Saw Reference (CHAR-02)
```cpp
// Source: Stilson & Smith (CCRMA, 1996) + Valimaki et al. (2010)
// Exponential ramp curvature + soft capacitor reset
float computeSaw(float phase, float character) {
    float saw = 1.f - 2.f * phase;  // current digital falling ramp
    if (character < 0.001f) return saw;

    float c = character * character;  // progressive curve

    // 1. Exponential ramp curvature (~2-3% deviation at full character)
    //    Makes the ramp slightly convex (exponential approach)
    //    The falling ramp goes from +1 to -1 over phase 0 to 1
    //    Apply subtle exponential bend: mix linear ramp with exponential version
    float expRamp = 1.f - 2.f * (1.f - std::exp(-3.f * phase)) / (1.f - std::exp(-3.f));
    float curvedSaw = saw + c * 0.03f * (expRamp - saw);  // ~3% deviation at full

    // 2. Soft capacitor reset (~5% of cycle width at full character)
    //    At the reset point (phase near 0), smooth the sharp transition
    //    The saw jumps from -1 back to +1 at phase=0/1
    float resetWidth = c * 0.05f;  // 5% of cycle at full character
    if (phase < resetWidth && resetWidth > 0.001f) {
        // Cosine interpolation from reset value to ramp value
        float t = phase / resetWidth;
        float smoothT = 0.5f - 0.5f * std::cos(t * (float)M_PI);
        // Blend from "just after reset" (+1 region) to normal ramp
        float resetValue = 1.f;  // saw starts at +1
        curvedSaw = resetValue + smoothT * (curvedSaw - resetValue);
    }

    return curvedSaw;
}
```

### Roland SH-101/Juno-106 Square Reference (CHAR-03)
```cpp
// Source: CEM3340 datasheet, SH-101/Juno-106 service manuals
// Sigmoid edge softening + duty cycle asymmetry
float computeSquare(float phase, float character) {
    float sqr = (phase < 0.5f) ? 1.f : -1.f;  // current digital square
    if (character < 0.001f) return sqr;

    float c = character * character;  // progressive curve

    // 1. Duty cycle asymmetry (0.5-1.5% at full character)
    float dutyOffset = c * 0.015f;  // 1.5% at full
    float duty = 0.5f + dutyOffset;

    // 2. Sigmoid edge softening via tanh
    //    Edge width: ~3% of cycle at full character
    float edgeWidth = c * 0.03f;
    float sharpness = (edgeWidth > 0.001f) ? 1.f / edgeWidth : 1000.f;

    // Rising edge at phase=0, falling edge at phase=duty
    float rising = std::tanh(sharpness * phase);
    float falling = std::tanh(sharpness * (duty - phase));
    float softSquare = rising + falling - 1.f;

    // Normalize to [-1, +1] range
    float peak = std::tanh(sharpness * duty * 0.5f);
    if (peak > 0.001f) softSquare /= peak;

    return softSquare;
}
```

### Moog/Prophet Triangle Reference (CHAR-04)
```cpp
// Source: Moog Voyager / Prophet-5 Rev 3.3 analysis
// Rounded peaks + slope asymmetry
float computeTriangle(float phase, float character) {
    float tri = 2.f * std::fabs(2.f * phase - 1.f) - 1.f;  // current digital triangle
    if (character < 0.001f) return tri;

    float c = character * character;  // progressive curve

    // 1. Slope asymmetry (2-3% at full character)
    //    Rising slope slightly steeper, falling slope slightly shallower
    float asymmetry = c * 0.03f;  // 3% at full
    float midpoint = 0.25f + asymmetry * 0.5f;  // shift the peak slightly
    float analogTri;
    if (phase < midpoint) {
        analogTri = -1.f + 2.f * phase / midpoint;
    } else if (phase < (1.f - midpoint)) {
        analogTri = 1.f - 2.f * (phase - midpoint) / (1.f - 2.f * midpoint);
    } else {
        analogTri = -1.f + 2.f * (phase - (1.f - midpoint)) / midpoint;
    }

    // 2. Rounded peaks via polynomial smoothing
    //    Soften the sharp direction reversals at peaks and valleys
    float roundAmount = c * 0.15f;  // peak rounding radius (% of amplitude)
    if (analogTri > (1.f - roundAmount)) {
        float t = (analogTri - (1.f - roundAmount)) / roundAmount;
        analogTri = (1.f - roundAmount) + roundAmount * std::sin(t * (float)M_PI * 0.5f);
    } else if (analogTri < -(1.f - roundAmount)) {
        float t = (-(1.f - roundAmount) - analogTri) / roundAmount;
        analogTri = -(1.f - roundAmount) - roundAmount * std::sin(t * (float)M_PI * 0.5f);
    }

    return analogTri;
}
```

### Triangle-Derived Analog Sine Reference (CHAR-05)
```cpp
// Source: Chebyshev polynomial waveshaping literature
// Triangle-derived sine with 1-3% residual THD (primarily 3rd harmonic)
float computeSine(float phase, float character) {
    float sine = std::sin(2.f * (float)M_PI * phase);  // current digital sine
    if (character < 0.001f) return sine;

    float c = character * character;  // progressive curve

    // Add residual harmonics as if derived from triangle waveshaper
    // Primary: 3rd harmonic (residual from triangle source)
    // Secondary: 2nd harmonic (from asymmetry in shaping circuit)
    // Chebyshev polynomials: T2(x) = 2x^2 - 1, T3(x) = 4x^3 - 3x
    float h2 = 2.f * sine * sine - 1.f;  // 2nd harmonic (Chebyshev T2)
    float h3 = 4.f * sine * sine * sine - 3.f * sine;  // 3rd harmonic (Chebyshev T3)

    // Scale harmonics: 3rd harmonic dominant (1-3%), 2nd harmonic subtle (0.5-1%)
    float thd3 = c * 0.02f;   // 2% 3rd harmonic at full character
    float thd2 = c * 0.008f;  // 0.8% 2nd harmonic at full character

    return sine + thd3 * h3 + thd2 * h2;
}
```

### Character CV Processing (CHAR-07)
```cpp
// Source: Existing Morph CV pattern in AnalogLFO.cpp lines 91-95
// In process():
float charKnob = params[CHARACTER_PARAM].getValue();
float charAtten = params[CHARACTER_ATTEN_PARAM].getValue();
float charCV = inputs[CHARACTER_CV_INPUT].getVoltage();
float character = rack::math::clamp(charKnob + charAtten * charCV / 10.f, 0.f, 1.f);
```

### Display Update Trigger (addressing TODO at line 101)
```cpp
// Source: Existing pattern in AnalogLFO.cpp lines 102-107
// Add character change detection alongside morph change detection:
bool phaseWrapped = (phase < prevPhaseForDisplay);
bool morphChanged = (std::fabs(morph - prevDisplayMorph) > 0.002f);
bool characterChanged = (std::fabs(character - prevDisplayCharacter) > 0.002f);
if (phaseWrapped || morphChanged || characterChanged) {
    updateDisplayBuffer(morph, character);
    prevDisplayMorph = morph;
    prevDisplayCharacter = character;
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Generic "warmth" knob (single filter) | Per-shape analog reference modeling | 2020s | Much more convincing; each shape targets a specific synth |
| Wavetable-based character | Procedural mathematical deformation | Current | No interpolation artifacts, continuous with morph position |
| Circuit-level SPICE modeling | Parametric approximation (exponential, tanh, polynomial) | Current | 99% of the perceptual result at 1% of the CPU cost |

**Deprecated/outdated:**
- Adding white noise for "analog character" -- sounds like hiss, not warmth
- Generic single-filter approach -- makes everything sound muffled rather than specifically "analog"

## Panel Layout: Character CV Placement

### Current Bottom Row (y=104mm)
```
x=9mm    x=21mm   x=35mm   x=51mm
Trimpot  MCV      DCV      OUT
(Morph   (Morph   (Drift   (Output)
 Atten)   CV)      CV)
```

### Proposed Bottom Row (with Character CV additions)
The bottom row needs CHARACTER_ATTEN_PARAM (Trimpot) and CHARACTER_CV_INPUT (PJ301MPort). The most logical placement groups Character CV controls between Morph CV and Drift CV, maintaining left-to-right signal flow matching the knob order above.

**Option A -- Expand existing row (recommended):**
```
x=5mm    x=14mm   x=23mm   x=32mm   x=41mm   x=51mm   x=56mm
MATrim   MCV      CATrim   CCV      DCV      OUT      (tight)
```
This is tight for 12HP (60.96mm). Minimum jack spacing is ~8mm center-to-center.

**Option B -- Use y=97mm for CV labels, shift components:**
Add "CCV" label at the appropriate position. Place the Character attenuator trimpot and Character CV jack in the space between Morph CV and Drift CV.

```
x=9mm    x=18mm   x=27mm   x=36mm   x=46mm   x=55mm
MATrim   MCV      CATrim   CCV      DCV      OUT
```

This distributes 6 components across 46mm of usable width (margins at 5mm and 56mm), giving ~9.4mm between centers -- comfortable spacing for jacks (PJ301MPort is ~6.7mm diameter) and trimpots (~5.8mm diameter).

**Recommendation:** Option B with slight position adjustments to maintain visual rhythm. The exact x-coordinates are Claude's discretion per CONTEXT.md.

## Open Questions

1. **Morph-character interaction at transitions**
   - What we know: Characterize-then-morph is recommended; linear crossfade between characterized shapes
   - What's unclear: Whether intermediate morph positions between characterized shapes look/sound natural at high character values
   - Recommendation: Implement characterize-then-morph, verify visually across full morph range at character=1.0. If transitions look odd, the deformation amounts can be tuned down.

2. **Morph bleed (IO-03)**
   - What we know: CONTEXT.md says "may be included if it enhances character without significant complexity"
   - What's unclear: Whether adding bleed is worth the complexity for v1
   - Recommendation: Defer to v2. The character deformations themselves provide substantial value. Bleed adds code complexity for a very subtle effect that most users won't notice at LFO rates.

3. **Exact deformation parameter values**
   - What we know: Literature provides ranges (2-3% curvature, 0.5-1.5% asymmetry, 1-3% THD)
   - What's unclear: Whether these values sound right when combined with the progressive curve
   - Recommendation: Use the code example values as starting points. The display provides immediate visual feedback for tuning. Adjust by ear if needed.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | VCV Rack 2 live testing (visual + auditory) |
| Config file | Makefile (standard VCV Rack build) |
| Quick run command | `make && make install` (builds and installs to VCV Rack plugins) |
| Full suite command | `make` + manual verification in VCV Rack |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CHAR-01 | Character knob crossfades digital to analog | manual | Build and test in VCV Rack -- sweep character 0-100% on each shape | N/A |
| CHAR-02 | Saw: exponential ramp + soft reset | manual | Set morph to saw (0.5-0.67), sweep character, observe display | N/A |
| CHAR-03 | Square: sigmoid edges + duty asymmetry | manual | Set morph to square (0.75-1.0), sweep character, observe display | N/A |
| CHAR-04 | Triangle: rounded peaks + slope asymmetry | manual | Set morph to triangle (0.25-0.33), sweep character, observe display | N/A |
| CHAR-05 | Sine: residual THD | manual | Set morph to sine (0.0), sweep character, observe display for slight thickening | N/A |
| CHAR-06 | HF rolloff | DEFERRED | N/A -- deferred to VCO module | N/A |
| CHAR-07 | Character CV input modulates character | manual | Patch LFO to Character CV, verify display responds | N/A |

### Sampling Rate
- **Per task commit:** `make` (compile check)
- **Per wave merge:** `make && make install` + visual verification in VCV Rack
- **Phase gate:** All waveform shapes display correct character deformations

### Wave 0 Gaps
None -- existing build infrastructure covers all phase requirements. No test framework needed beyond `make` + VCV Rack visual verification.

## Sources

### Primary (HIGH confidence)
- Existing AnalogLFO.cpp source -- current code structure, patterns, integration points
- CONTEXT.md -- locked decisions, discretion areas, deferred items
- `.planning/research/FEATURES.md` -- analog reference targets (Minimoog, Roland, Moog/Prophet)
- `.planning/research/PITFALLS.md` -- Pitfall 7 (uncanny valley), Pitfall 15 (testing by ear)
- `.planning/research/ARCHITECTURE.md` -- CharacterStage design patterns

### Secondary (MEDIUM confidence)
- [Stilson & Smith, CCRMA 1996](https://www.researchgate.net/publication/220057893_Discrete-Time_Modelling_of_the_Moog_Sawtooth_Oscillator_Waveform) -- Minimoog sawtooth oscillator analysis
- [Valimaki et al., 2010](https://www.researchgate.net/publication/220386519_Oscillator_and_Filter_Algorithms_for_Virtual_Analog_Synthesis) -- Virtual analog oscillator and filter algorithms
- [Chebyshev polynomial waveshaping](https://kennypeng.com/2022/06/18/chebyshev_harmonics.html) -- T2, T3 formulas for controlled harmonic injection
- [MOD WIGGLER: Moog sawtooth waveform discussion](https://modwiggler.com/forum/viewtopic.php?t=169797) -- community knowledge on Minimoog saw characteristics
- [KVR Audio: Chebyshev waveshaper discussion](https://www.kvraudio.com/forum/viewtopic.php?t=70372) -- practical implementation patterns

### Tertiary (LOW confidence)
- Exact deformation parameter values (curvature percentages, asymmetry amounts) -- these are literature-derived starting points that need perceptual tuning by ear

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all standard C++ math, no new dependencies
- Architecture: HIGH -- follows established single-file pattern, clear integration points in existing code
- DSP implementation: HIGH -- well-documented analog modeling techniques (tanh, exponential, Chebyshev)
- Parameter values: MEDIUM -- literature-derived starting points need tuning
- Panel layout: MEDIUM -- spacing calculations are reasonable but exact positions need visual verification

**Research date:** 2026-03-06
**Valid until:** Indefinite -- DSP math and analog modeling techniques are stable knowledge
