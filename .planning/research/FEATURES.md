# Feature Landscape: v1.2 Deep Analog

**Domain:** Eurorack LFO module -- analog character deepening, modulation inputs, groove features
**Researched:** 2026-03-13
**Overall Confidence:** HIGH
**Scope:** NEW features only -- deep analog additions to existing Analog Series LFO (v1.0 engine + v1.1 clock sync)

---

## Existing Foundation (v1.0 + v1.1 -- must not be disrupted)

- Three-knob analog engine (Morph, Character, Drift) with CV inputs
- Four-shape parametric morph (Sine-Tri-Saw-Square) with per-shape analog character modeling
- Four-layer Ornstein-Uhlenbeck drift engine with per-module RNG
- Real-time waveform display with lock-free double buffer, glow trace, phase-tracking dot
- Three-state clock tracker (FREE/ACQUIRING/LOCKED) with EMA smoothing
- 15 discrete musical ratios via dual-mode Rate knob
- Division-aware phase reset with 3ms cosine crossfade
- NanoVG text overlays (SYNC badge, ratio, BPM, Hz) with fade animations
- Drift authority scaling (2% clocked vs 7.5% free), 50ms frequency slew for mode transitions
- 12HP panel, bipolar +/-5V single output, 890 lines of C++

---

## Table Stakes

Features users expect from a mature LFO module. Missing any of these means the module feels incomplete compared to VCV Fundamental LFO, Bogaudio LFO, or Mutable Instruments Tides.

| Feature | Why Expected | Complexity | Dependencies on Existing Engine | Notes |
|---------|--------------|------------|-------------------------------|-------|
| **FM input jack** | Every serious LFO/VCO in hardware and VCV Rack has an FM input. VCV Fundamental LFO has it. Bogaudio LFO has V/Oct. Users patch LFO-to-LFO constantly for evolving modulation. | Low | Modifies frequency calculation -- same insertion point as drift. New `FM_INPUT` in InputId, new attenuverter trimpot. | Linear FM (proportional Hz scaling). At LFO rates, exponential FM is perceptually identical to linear -- no toggle needed. See detailed breakdown below. |
| **Separate RESET jack** | Standard on VCV Fundamental LFO, Mutable Tides, nearly all hardware Eurorack LFOs. Users expect to restart LFO phase independently of clock -- essential for syncing to note gates, sequences, or manual triggers. | Low | Reuses existing phase-reset + crossfade logic from clock sync (v1.1). New `RESET_INPUT` in InputId, new `dsp::SchmittTrigger`. | Rising-edge trigger resets phase. Must work in both free-running and clocked modes. Independent of CLK tracking. See detailed breakdown below. |
| **Display text overlay readability** | Current HUD text (Hz, BPM, ratio, SYNC) renders directly on the waveform trace. When waveform passes through text areas, readability drops. Users need to read values while patching. | Low | Display rendering only (NanoVG). No DSP changes. Modify `drawTextOverlays()` to add semi-transparent dark pill backgrounds behind text. | Standard UI pattern: `nvgRoundedRect` fill at ~0.6 alpha of background color before text rendering. Minimal code, immediate UX improvement. |
| **Incoming clock BPM display** | Users need to see both raw clock tempo AND ratio-adjusted LFO rate. Currently only effective BPM is shown, which is misleading at non-x1 ratios. | Low | No new atomics needed -- `displaySmoothedPeriod` already exists. Calculation: `60.f / smoothedPeriod`. New text element in display. | Show raw clock BPM (e.g., "120 CLK") in bottom-left when clocked. Effective BPM stays in bottom-right. Helps users verify clock detection is correct. |

**Confidence:** HIGH -- FM input and RESET jack are verified as standard across VCV Fundamental LFO (official manual and library page), Bogaudio LFO, Mutable Instruments Tides (manual verified), and the broader Eurorack ecosystem. Display improvements are straightforward NanoVG operations.

---

## Differentiators

Features that set this module apart from competitors. Not expected by default, but valued -- these are what make users choose this LFO over alternatives.

| Feature | Value Proposition | Complexity | Dependencies on Existing Engine | Notes |
|---------|-------------------|------------|-------------------------------|-------|
| **Phase offset knob with CV** | Enables quadrature patches (90-degree offset for stereo widening), polyrhythmic phase relationships, and dynamic phase displacement. Tides and 8FO have phase shift but few single-LFO modules offer a phase offset knob with CV. | Medium | Additive offset on phase readout, not on the accumulator itself. New `PHASE_PARAM`, `PHASE_ATTEN_PARAM`, `PHASE_CV_INPUT`. Display must show offset waveform. | See detailed breakdown below. |
| **Expanded analog imperfections** | Deepens existing drift engine beyond pitch-only drift. Real analog circuits exhibit DC offset wander, per-component parameter spread, phase jitter, and frequency slew. This is the module's core identity -- going deeper into analog authenticity where no competitor goes. | High | Extends existing OU infrastructure. Phase jitter at cycle wrap. DC offset on output. Component spread as static per-instance offsets. All bundled under existing Drift knob. | See detailed breakdown below. Four sub-features, all scaled by `progressiveCurve(drift)`. |
| **Waveform bleed during morph transitions** | In real analog waveshaper circuits, switching between outputs is never perfectly clean. Adjacent waveforms "bleed" through due to analog multiplexer crosstalk and capacitive coupling. No competing VCV LFO models this. | Medium | Modifies `computeMorphedWave()`. Adds small non-adjacent shape bleed scaled by character level. All four shapes already computed -- minimal additional computation. | See detailed breakdown below. Only active when character > 0. |
| **Swing/shuffle for clocked mode** | Adds groove to clocked LFO patterns. Straight-time modulation sounds mechanical. 4ms SCM and Impromptu Clocked offer swing, but no LFO module has built-in swing -- a genuine differentiator. | High | Modifies phase progression in clocked mode via phase warping. Interacts with clock sync, phase reset, and division-aware beat counting. | See detailed breakdown below. Only active in clocked mode. |

**Confidence:** MEDIUM for swing (novel, complex interactions), HIGH for the rest (grounded in well-documented analog phenomena and established Eurorack patterns).

---

## Anti-Features

Features to explicitly NOT build for v1.2.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| **Exponential FM mode / toggle switch** | At LFO rates (0.01-20Hz), exponential FM is perceptually identical to linear FM. Adding a toggle adds UI complexity for zero musical benefit. Exponential FM is meaningful at audio rates -- the VCO module (v2.0) will need both modes; the LFO does not. | Linear FM only. Proportional scaling: `freq *= (1.f + fmAtten * fmCV / 5.f)`. One mode, no switch. |
| **Through-zero FM** | TZ-FM is meaningful only at audio rates where frequency passes through 0 Hz and inverts. At LFO rates, frequency inversion produces confusing backward modulation with no musical utility. | Linear FM with clamping: `freq = fmax(freq, 0.001f)`. If FM pushes frequency negative, it stops rather than reverses. |
| **Individually exposed drift sub-parameters** | Exposing phase jitter, DC offset, pitch slew, and component spread as separate controls violates the v1.0 design decision: "one drift knob with curated proportions." Adding 4+ knobs breaks the three-knob engine identity. | Bundle all imperfections under existing Drift knob. Each scales with `progressiveCurve(drift)`. Right-click menu to enable/disable specific layers is acceptable for advanced users. |
| **Additional OU layers for new imperfections** | More OU layers = more per-sample RNG calls and computation. The existing four layers already provide the stochastic foundation. | Reuse existing OU layer 0 output (slow wander) with different scaling for DC offset drift. Phase jitter uses existing RNG directly at phase wrap only (once per LFO cycle, not per sample). |
| **Swing knob on front panel** | 12HP panel is already dense. Adding a visible swing knob requires expanding the panel or cramping existing controls. | Right-click context menu for swing percentage (stepped: 50%, 54%, 58%, 62%, 67%, 71%, 75%). Serialized in JSON. Display shows swing value when non-50%. |
| **Swing in free-running mode** | Without a clock, "swing" has no musical meaning. There are no beats to shuffle. | Swing parameter ignored in free-running mode. Only active when CLK is connected and clock state is ACQUIRING or LOCKED. |
| **Per-cycle random phase offset (humanize)** | Random per-cycle phase offsets produce the same perceptual effect as phase jitter from the expanded drift engine. Implementing both would be redundant. | Phase jitter under the Drift knob covers this use case. |
| **Separate phase-shifted output jack** | Some multi-output LFOs provide phase-shifted copies on additional jacks. This contradicts the single-output design philosophy ("single morphed output IS the design concept"). | Phase offset knob shifts the single output. Users wanting multiple copies should use multiple module instances. |
| **FM depth knob on front panel** | Panel real estate is limited. A dedicated FM depth knob would be a fourth "big" control breaking the three-knob aesthetic. | FM depth controlled by attenuverter trimpot in bottom section, matching existing CV attenuverter pattern (Morph, Character, Drift attenuverters). |

**Confidence:** HIGH -- all decisions align with established project philosophy (three-knob identity, 12HP constraint, "one drift knob with curated proportions").

---

## Detailed Feature Breakdowns

### FM Input Jack (Table Stakes)

**Analog synth convention:** LFO FM at sub-audio rates produces vibrato (when modulating pitch) or evolving modulation patterns (LFO-to-LFO FM). Eurorack conventions:
- **Linear FM:** Input voltage directly scales frequency proportionally. More intuitive at LFO rates.
- **Exponential FM:** Input voltage scales frequency via V/octave. Standard for VCOs, less useful for LFOs.

VCV Fundamental LFO offers both modes (LFM toggle). Bogaudio LFO uses V/Oct (exponential). At sub-audio rates where the full frequency range is 0.01-20Hz, the perceptual difference between linear and exponential FM is negligible.

**Recommended implementation:**
- Input range: +/-5V bipolar
- Attenuverter trimpot: 0 to 1 (unipolar -- FM polarity comes from the modulation source, not the attenuverter, keeping it consistent with the existing Morph/Character/Drift attenuverter pattern)
- Scaling: proportional to base frequency. `freq *= (1.f + fmAtten * fmCV / 5.f)`. At full attenuverter with +5V input, frequency doubles. With -5V, frequency goes to zero (clamped to 0.001).
- Clamp result: `freq = fmax(freq, 0.001f)`
- Integration point: AFTER base frequency calculation, BEFORE drift processing. Order in process(): base freq -> FM -> drift -> phase accumulation.
- In clocked mode: FM modulates the clock-derived frequency, allowing rhythmic frequency variation even when synced.

**Panel placement:** FM attenuverter trimpot + FM jack in bottom section, grouped near CLK jack (related modulation inputs together).

### Separate RESET Jack (Table Stakes)

**Analog synth convention:** RESET/SYNC jacks on LFOs accept a rising-edge trigger and restart the waveform from the beginning of its cycle:
- Rising-edge trigger (Schmitt: 0.1V low, 1.0V high -- VCV SDK convention)
- Phase resets to 0.0 (or to phase offset value when offset is implemented)
- No effect on frequency or clock tracking
- Works in both free-running and clocked modes

**Distinction from CLK input:** CLK serves dual purposes: (1) tempo reference, and (2) phase reset. RESET provides phase reset ONLY without affecting clock tracking. Key use cases:
- Reset LFO to note onset (gate to RESET) while keeping tempo-locked frequency (clock to CLK)
- Manual reset via button module
- Reset from sequencer step trigger

**Implementation:** New `RESET_INPUT` in InputId enum, new `dsp::SchmittTrigger resetTrigger`. In process(), after clock processing: check `resetTrigger`, if triggered, reset phase to offset value (or 0.0), apply same 3ms cosine crossfade as clock reset. Do NOT reset `clockBeatCount` -- clock tracking stays independent.

**Panel placement:** RESET jack adjacent to CLK jack (both are timing/trigger inputs).

### Phase Offset Knob with CV (Differentiator)

**Analog synth convention:** Phase offset is standard on quadrature LFO modules:
- Mutable Instruments Tides: SHIFT knob applies phase shift between outputs in cyclic mode
- Bogaudio 8FO: 8 outputs at fixed 45-degree intervals
- Cherry Audio Quadrature VLFO: Phase Adjust knob, 0-360 degrees
- Erica Synths Black Octasource: 8 syncable outputs with 45-degree shifts
- New Systems Instruments QLFO: 8 phase-shifted outputs, tunable to any relationship

Range is always 0-360 degrees. The offset shifts where in the cycle the output currently is without changing frequency.

**Key behaviors:**
- Phase offset is ADDITIVE to the running phase: `outputPhase = fmod(phase + offset, 1.0)`
- Changing offset smoothly shifts the output waveform in time
- CV modulation of offset = phase modulation (PM), producing waveform "sliding" at LFO rates
- RESET trigger resets phase to the offset value (not 0.0)
- Clock-triggered reset also resets to the offset value

**Range convention:**
- Knob: 0.0 to 1.0 internally (tooltip displays 0-360 degrees)
- CV: 0-10V unipolar with attenuverter (0V = no additional offset, 10V = +360 degrees = full cycle)

**Display implications:** Both the waveform trace and phase-tracking dot must reflect the offset. The display should show what the output actually looks like. Implementation: shift the phase lookup when reading from the display buffer, or regenerate the buffer with offset applied.

**Panel considerations:** Needs a small knob (RoundBlackKnob, matching Rate), an attenuverter trimpot, and a CV jack. The most panel-space-demanding new feature. See Panel Density Analysis section.

### Expanded Analog Imperfections (Differentiator)

Each imperfection models a real analog circuit phenomenon. All bundled under the existing Drift knob.

**1. Phase Jitter (cycle-to-cycle timing variation)**

*Real analog behavior:* Each cycle of a real analog oscillator varies slightly in duration due to transistor noise, power supply ripple, and thermal effects. Visible as edge "blurring" on an oscilloscope. Typically 0.01-0.5% of cycle period in vintage circuits.

*Implementation:* On each phase wrap (`phase >= 1.0`), apply a small random offset: `phase += progressiveCurve(drift) * 0.005f * normalDist(rng)`. Uses existing per-module RNG. No per-sample cost -- fires once per LFO cycle.

*Complexity:* Low. One random number and one addition at phase wrap.

**2. DC Offset Drift (output voltage center wander)**

*Real analog behavior:* Real oscillators rarely output perfectly symmetric waveforms centered at 0V. DC offset wanders slowly due to op-amp input offset voltage drift (~1-10 uV/degC), capacitor leakage, and aging. The drift is proportional to sqrt(elapsed time) per Analog Devices documentation. Typical wander: 10-50mV over minutes.

*Implementation:* Reuse the existing slowest OU layer (layer 0, 0.05Hz) output with different scaling. `dcOffset = ouLayers[0].state * progressiveCurve(drift) * 0.01f` (producing +/-50mV on the +/-5V output at full drift). Applied after waveform computation: `outputVoltage += dcOffset`. No new OU process needed.

*Complexity:* Very Low. One multiply and one add per sample, using existing OU state.

**3. Pitch Slew (frequency change inertia)**

*Real analog behavior:* Analog oscillator circuits cannot change frequency instantaneously. The expo converter and integrator have finite bandwidth. Slew time: 1-10ms for small changes, 10-50ms for large jumps.

*Current state:* The module already has `freqSlew` (TExponentialFilter, lambda=20, ~50ms time constant) for mode transitions. In free-running mode, the raw knob value goes directly to `targetFreq`.

*Implementation:* Make the slew filter's time constant drift-dependent. At drift=0, lambda=100 (essentially instant, 10ms). At full drift, lambda=5 (200ms, noticeable analog-style lag on rate changes). Dynamically set: `freqSlew.setLambda(rack::math::clamp(100.f - 95.f * progressiveCurve(drift), 5.f, 100.f))`.

*Complexity:* Very Low. The infrastructure already exists. Just make the lambda drift-dependent.

**4. Component Spread (per-instance parameter variation)**

*Real analog behavior:* Component tolerances (1-5% for resistors, 10-20% for capacitors) mean each synth module has slightly different characteristics. Two "identical" LFOs have slightly different frequency calibration, waveform symmetry, and output level.

*Implementation:* On module construction, generate per-instance random offsets using existing RNG:
- Frequency calibration: +/-2% (`freqSpread = 1.f + 0.04f * (normalDist(rng) * 0.5f)`)
- Square duty cycle offset: +/-1% additional asymmetry
- Output amplitude: +/-1%

These are STATIC per instance (set once in constructor). Apply only when drift > 0: `freq *= 1.f + (freqSpread - 1.f) * progressiveCurve(drift)`.

*Complexity:* Medium. Requires generating and storing multiple offsets, applying at various points.

### Waveform Bleed During Morph Transitions (Differentiator)

**Analog synth convention:** In real analog synths with waveform selectors (Minimoog, Prophet-5, Juno-106), non-selected waveforms leak through due to:
- Analog switch ON resistance (CD4066: ~50-200 ohm) allowing signal coupling
- Capacitive coupling between adjacent PCB traces
- Imperfect transistor switching in waveshaper networks

Leakage is typically 0.5-3% of non-selected waveform amplitude.

**Implementation:** After computing the main morphed wave, add a bleed contribution from all four shapes:
```cpp
float bleedAmount = progressiveCurve(character) * 0.03f;  // max 3% at full character
if (bleedAmount > 0.001f) {
    float avgAll = (sine + tri + saw + sqr) * 0.25f;
    float bleedSignal = avgAll - mainOutput;  // everything NOT in the main output
    mainOutput += bleedAmount * bleedSignal;
}
```

Computationally near-free since all four shapes are already computed. Scales with character (zero character = zero bleed = digital perfection). Display buffer should incorporate bleed, making the trace slightly more complex at high character -- visually communicates the imperfection.

### Swing/Shuffle for Clocked Mode (Differentiator)

**What swing means for a continuous LFO:** Swing timing alternates between longer and shorter half-cycles within each period. At 50% swing, both halves are equal (straight time). At 67% (triplet feel), the first half takes 2/3 of the period and the second half takes 1/3.

**Industry conventions:**
- TR-808/TR-909: shuffle delays every other 16th note by 2/96 to 12/96 of a beat
- MPC: swing values 50-75%, with 54-58% being subtle groove, 62-67% being strong
- 4ms SCM: "Slip" CV shifts specific beats forward in time
- Impromptu Clocked: swing percentage on clock outputs with CV via expander
- Standard definition: swing percentage S means the first beat takes S% of the total beat pair

**Phase-warping algorithm:**
```
Given raw linear phase p (0 to 1) and swing ratio s (0.5 to 0.75):

if p < s:
    warped = p * 0.5 / s                            // first half stretched
else:
    warped = 0.5 + (p - s) * 0.5 / (1.0 - s)       // second half compressed

output = computeMorphedWave(warped, morph, character)
```

**Feature interaction chain:** phase accumulation -> swing warp -> phase offset -> waveform computation.

**UI:** Right-click context menu with stepped presets (50%, 54%, 58%, 62%, 67%, 71%, 75%). Serialized in patch JSON. Display shows "SWG 67%" overlay when swing is non-50% and module is clocked.

---

## Feature Dependencies

```
Display text readability fix    (independent, display-only, quick win)
Incoming clock BPM display      (independent, display-only, quick win)

FM input jack                   (independent, modifies freq calculation)

RESET jack                      (independent, reuses v1.1 crossfade)
    |
    v
Phase offset knob/CV            (depends on RESET: reset target = offset value)
    |
    v
Swing/shuffle                   (depends on phase offset: swing warp composes with offset)

Expanded imperfections          (independent, extends drift infrastructure)
    - Phase jitter              (low complexity, phase wrap only)
    - DC offset drift           (low complexity, reuses OU layer 0)
    - Pitch slew                (very low, extends existing freqSlew)
    - Component spread          (medium, constructor-time offsets)

Waveform bleed                  (independent, modifies computeMorphedWave)

Panel layout                    (depends on ALL features being finalized)
```

**Critical ordering rationale:**
1. RESET before Phase Offset -- offset changes what "reset" means
2. Phase Offset before Swing -- swing warping must compose with offset
3. Display fixes are fully independent -- good first-phase quick wins
4. FM input is fully independent
5. Expanded imperfections are independent but should be grouped as one unit
6. Waveform bleed is independent
7. Panel layout must come last after all components are known

---

## Panel Density Analysis

Current panel (12HP = 60.96mm) layout:
```
y=54mm:   [MORPH]                    (RoundBigBlackKnob, x=30.48)
y=69mm:   [CHARACTER]    [DRIFT]     (RoundLargeBlackKnob, x=18, x=42.96)
y=86mm:   [RATE]         [CLK]      (RoundBlackKnob x=18, PJ301MPort x=42.96)
y=96mm:   [MATrim] [CATrim] [DATrim] (Trimpot, x=10, 24, 38)
y=108mm:  [MCV]   [CCV]   [DCV]  [OUT] (PJ301MPort, x=10, 24, 38, 52)
```

v1.2 additions needed:
- FM attenuverter trimpot + FM input jack (2 components)
- RESET input jack (1 component)
- Phase offset knob + attenuverter trimpot + Phase CV jack (3 components)

Total: 6 new components on an already well-populated 12HP panel.

**Options:**
1. **Stay at 12HP, creative layout:** Place RESET and FM near CLK. Phase offset knob replaces or shares the Rate row area. Third row of small components.
2. **Expand to 14HP (+10.16mm):** Comfortable for all components. Breaks established form factor.
3. **Defer Phase CV to right-click:** Phase offset becomes knob-only initially (no CV), reducing by 2 components.
4. **Design panel LAST:** Implement all DSP first with temporary placement, then dedicate a phase to comprehensive panel redesign.

**Recommendation:** Option 4 -- design panel once for all features. Panel rework was identified as a v1.0 inefficiency in the retrospective ("Bottom row layout went through three iterations"). Design for the final state.

---

## MVP Recommendation

**Priority order for v1.2 phases:**

1. **Display text readability fix + incoming clock BPM** -- Quick wins, immediate UX improvement, no DSP risk
2. **RESET jack** -- Table stakes, low complexity, reuses existing crossfade
3. **FM input jack** -- Table stakes, low complexity, high user value
4. **Phase offset knob/CV** -- Differentiator, medium complexity, depends on RESET
5. **Expanded imperfections** -- Differentiator, high complexity, core identity deepening
6. **Waveform bleed** -- Differentiator, medium complexity, extends character knob
7. **Swing/shuffle** -- Differentiator, high complexity, clocked-mode only
8. **Panel redesign** -- Accommodates all new components in final layout

**Defer if scope must be trimmed:** Swing/shuffle is the best deferral candidate. It has the most complex interactions (clock sync + phase reset + phase offset + division ratios), is only useful in clocked mode, and the right-click menu approach means it needs no panel changes -- easy to add in v1.2.1 or v1.3.

---

## Competitive Landscape

| Feature | VCV Fundamental LFO | Bogaudio LFO | Bogaudio 8FO | Mutable Tides | Forge Analog LFO (after v1.2) |
|---------|---------------------|--------------|--------------|---------------|-------------------------------|
| FM input | Yes (lin + expo) | Yes (V/Oct) | Yes | Yes (expo) | Yes (linear) |
| RESET jack | Yes | No | No | Yes (gate) | Yes |
| Phase offset | No | No | Yes (8 fixed) | Yes (per output) | Yes (knob + CV) |
| Swing/shuffle | No | No | No | No | Yes (clocked mode) |
| Analog character | No | No | No | Wavefolder only | Yes (per-shape modeling) |
| Multi-layer drift | No | No | No | No | Yes (4-layer OU) |
| Expanded imperfections | No | No | No | No | Yes (jitter, DC offset, slew, spread) |
| Waveform bleed | No | No | No | No | Yes |
| Clock sync | Yes | No | No | Yes | Yes (15 ratios) |
| Waveform display | No | No | No | No | Yes (real-time with dot) |
| Continuous morph | No | Wave selector | No | Shape knob | Yes (4-shape continuous) |

**Positioning:** After v1.2, Forge Analog LFO will have the most comprehensive analog-character feature set of any LFO in the VCV Rack ecosystem. The combination of three-knob analog engine + clock sync + FM + phase offset + swing + expanded imperfections + waveform display is unmatched by any single module.

---

## Backward Compatibility (Non-Negotiable)

| Requirement | How to Verify |
|-------------|--------------|
| No new inputs/params alter existing behavior at default values | FM atten=0, phase offset=0, swing=50% produce identical output to v1.1 |
| Existing patches load correctly in v1.2 | New params have safe defaults; new inputs unconnected = no effect |
| No CPU regression for basic operation | New features early-exit when params are at default values |
| Display unchanged when new features inactive | HUD backgrounds only appear behind active text |
| Output voltage range unchanged | Still bipolar +/-5V; DC offset drift adds ~50mV max (within spec) |
| Drift=0 still means digital perfection | All new imperfections gated by `drift >= 0.001f` check |

---

## Sources

### HIGH Confidence (Official documentation, verified APIs, direct inspection)
- VCV Rack SDK `dsp/digital.hpp` -- SchmittTrigger API (0.1V/1.0V thresholds), verified from local SDK
- Existing `src/AnalogLFO.cpp` source (890 lines) -- verified integration points, current enum layout, all existing patterns
- [VCV Library - VCV LFO](https://library.vcvrack.com/Fundamental/LFO) -- Fundamental LFO feature set: FM input (linear/expo toggle), RESET input, CLK input, UNI/BI switch, polyphonic
- [Mutable Instruments Tides documentation](https://pichenettes.github.io/mutable-instruments-documentation/modules/tides_2018/manual/) -- Phase shift per output in cyclic mode, FM via expo CV, trigger reset, shape/slope/smoothness

### MEDIUM Confidence (Multiple sources agree, community-verified patterns)
- [VCV Community - FM CV levels](https://community.vcvrack.com/t/fm-cv-levels-correspond-to-what-exactly/13561) -- FM voltage scaling conventions
- [KVR Audio - Implementing swing](https://www.kvraudio.com/forum/viewtopic.php?t=261858) -- Swing/shuffle DSP approaches, TR-909 shuffle tick values
- [Cycling '74 - Shuffle for phasor LFO](https://cycling74.com/forums/shuffle-or-swing-for-a-phasor-driven-lfo) -- Phase warping approaches for LFO swing
- [4ms Shuffling Clock Multiplier](https://4mscompany.com/scm.php) -- Slip/shuffle: CV shifts beats forward, 5 slipped + 3 steady outputs
- [Impromptu Clocked](https://library.vcvrack.com/ImpromptuModular/Clocked) -- Swing + clock delay + pulse width as core features
- [Bogaudio GitHub](https://github.com/bogaudio/BogaudioModules) -- LFO V/Oct tracking, 8FO phase outputs
- [Cherry Audio Quadrature VLFO](https://store.cherryaudio.com/modules/quadrature-vlfo) -- Phase Adjust knob 0-360 degrees
- [Erica Synths Black Octasource](https://www.sweetwater.com/store/detail/BkOctasource--erica-synths-black-octasource-syncable-lfo-eurorack-module-with-phase-shift) -- 8 syncable outputs, 45-degree shifts
- [New Systems Instruments QLFO](https://nsinstruments.com/modules/qlfo.html) -- 8 phase-shifted outputs, tunable
- [MOD WIGGLER - LFO with sync and reset](https://www.modwiggler.com/forum/viewtopic.php?t=222452) -- Community expectations for RESET jack behavior
- [Noise Engineering - Vibrato](https://noiseengineering.us/blogs/loquelic-literitas-the-blog/vibrato/) -- FM/vibrato: pitch modulation < 1 semitone, sine or triangle LFO source
- [Analog Devices - Temperature Drift](https://analogtoolshub.com/temperature-drift-analog-circuits/) -- DC offset drift mechanisms, temperature coefficient specs
- [AllAboutCircuits - Component Aging](https://www.allaboutcircuits.com/technical-articles/electronic-component-aging-aging-effects-of-resistors-and-operational-amps/) -- Op-amp offset drift proportional to sqrt(elapsed time)
- [Vintage Synth Forum - Analog drift](https://forum.vintagesynth.com/viewtopic.php?t=54750) -- Real-world drift: tuning drift as circuits warm, EM interference, component aging
- [MOD WIGGLER - Adding swing to clock](https://modwiggler.com/forum/viewtopic.php?t=174713) -- Modular swing: delay-based, LFO-based, alternating gate timing

### LOW Confidence (General knowledge, single source, needs validation)
- Waveform bleed percentages (0.5-3%) -- general knowledge of CD4066 analog switch characteristics, not measured from specific synths
- Component spread tolerances (1-5% resistor, 10-20% capacitor) -- standard EIA bands, not synth-specific
- Phase jitter magnitude (0.01-0.5% of cycle) -- general oscillator knowledge, not vintage synth measurements
- Panel density feasibility at 12HP with all v1.2 additions -- needs visual verification
