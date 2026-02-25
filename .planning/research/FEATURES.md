# Feature Landscape

**Domain:** Analog-modeled VCV Rack oscillator modules (LFO + VCO)
**Researched:** 2026-02-25
**Confidence:** MEDIUM (deep domain expertise in DSP/synth design from training data; web verification of current VCV module landscape unavailable this session)

---

## Table Stakes

Features users expect from any VCV Rack oscillator claiming analog modeling. Missing any of these means the module feels unfinished and gets passed over in a crowded library.

### Waveform Generation

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Four core waveforms (sine, triangle, saw, square) | Universal expectation for any analog oscillator module. Every classic synth had these. | Low | POC already generates all four from shared phase. |
| Waveform morphing / continuous sweep | Common in modern VCV modules (VCV Fundamental VCO-2 has a wave knob, Bogaudio VCO). Users expect smooth transitions, not hard switching. | Medium | Sine-to-tri via peak sharpening. Tri-to-saw via asymmetry sweep. Saw-to-square via slope steepening. Must be click-free at all morph speeds including audio-rate CV. |
| Antialiased audio output (VCO) | Any VCO without antialiasing sounds obviously digital at high pitches. Users notice immediately. Bogaudio, Befaco, Fundamental VCO all antialias. | High | PolyBLEP is the standard. See dedicated Antialiasing section below. |
| V/Oct pitch input (VCO) | Standard 1V/octave. Without this the VCO is unusable in any patch. | Low | Standard exponential pitch calculation via `exp2_taylor5`. |
| FM input | Every VCO and most LFOs have FM. Exponential FM is table stakes. | Low | Exponential FM = adding voltage to pitch before exp conversion. Already in POC. |
| Hard sync input (VCO) | Standard on nearly all analog-modeled VCOs. Users expect it for classic sync lead sounds. | Medium | Reset phase on rising edge. Must handle sub-sample timing for clean sync sweep. |
| Reset/sync input (LFO) | Standard on all LFOs. Needed for tempo-synced modulation. | Low | Already in POC via Schmitt trigger. |
| Bipolar +/-5V output | VCV Rack standard for oscillator outputs. | Low | Already in POC. |
| CV inputs for primary parameters | Any knob without a CV input feels incomplete in modular. Morph, character, and drift all need CV. | Low | Standard VCV pattern: knob value + attenuated CV input. |
| Reasonable CPU usage | Users run 50-100+ modules in a patch. An oscillator eating >1% single-core CPU gets replaced immediately. | Medium | PolyBLEP is efficient. Drift calculations should be amortized (update every N samples). Display updates at screen rate, not audio rate. |
| Right-click context menu | VCV Rack convention for secondary settings. Users expect to find options here. | Low | Framework provides this. Use for oversampling, tracking error toggle, etc. |
| Clean panel with readable labels | Users browse modules visually in the library. Poor panel design = not taken seriously. | Low | SVG panel with established Forge Audio brand identity. |

### Visual Feedback

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Real-time waveform display | Increasingly expected on modern VCV oscillators. Valley Terrorform, Bogaudio LVCO, Lindenberg Research modules all have displays. Without it, the morph/character knobs feel blind. | High | NanoVG custom drawing on FramebufferWidget. Use dirty-flag pattern: only redraw when parameters change. Buffer one cycle of the composite waveform shape. |
| Phase indicator on display | Shows current position in the cycle. Standard on LFO displays (VCV Fundamental LFO-2). | Medium | Bright dot riding the waveform curve. Update at display rate (~30-60 Hz), not audio rate. |

### Analog Modeling Core

Since the module's entire identity is analog character, these are table stakes *for this specific product* even though generic VCOs skip them.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Analog character control (digital-to-analog crossfade) | The core promise. Without convincing analog character there is no reason to use this over Fundamental VCO. | High | Per-shape reference waveform models. See Classic Reference Targets below. |
| Pitch drift | The single most recognizable analog imperfection. Every analog modeling synth includes it. Users test for this immediately. | Medium | Ornstein-Uhlenbeck process: mean-reverting random walk. Must sound like warming-up oscillator, not random noise. |
| HF rolloff on waveforms | Real analog oscillators have bandwidth-limited harmonics. A saw wave equally bright at C1 and C7 sounds digital. This is the most immediately audible warmth cue. | Medium | One-pole or two-pole lowpass whose cutoff tracks pitch. Higher notes get relatively more rolloff. Part of the character engine. |

---

## Classic Analog Reference Targets per Waveform Shape

Which specific synths to target as the "character=1.0" reference for each waveform, and exactly what makes them sound the way they do.

### Sawtooth -- Primary Reference: Minimoog Model D

**Why Minimoog:** The Minimoog sawtooth is the most iconic analog saw wave ever produced. It is the gold standard that virtually every analog modeling synth and plugin targets. When users think "fat analog saw," they hear the Minimoog in their mind.

**What makes it sound that way (circuit-level):**

- **Capacitor discharge reset:** The Minimoog oscillator uses a capacitor that charges linearly (ramp up), then discharges through a transistor switch when it hits a threshold. The discharge is not instantaneous -- it takes approximately 5-15 microseconds, creating a slightly rounded falling edge rather than a mathematically perfect vertical reset. This finite reset time naturally attenuates the highest harmonics.
- **Exponential ramp curvature:** The charging capacitor creates a slightly convex (exponential approach) ramp rather than a perfectly linear slope. The deviation from linearity is subtle (approximately 1-3% of the waveform amplitude) but contributes perceptible warmth by slightly de-emphasizing upper harmonics relative to a perfect linear ramp.
- **Natural bandwidth limitation:** The transistor reset circuit and parasitic capacitances roll off harmonics above roughly 15-20 kHz, giving the saw a "dark" quality compared to a mathematically perfect sawtooth. This rolloff increases with pitch (higher notes sound proportionally warmer).
- **DC offset:** Minimoog oscillators carry a slight DC offset that drifts with temperature, typically 10-50mV equivalent in a 10Vpp signal.

**Secondary reference (contrast, not implementation target):** Roland SH-101 saw -- slightly brighter and thinner, with a faster reset time due to the CEM3340 IC design. More "precise" while still recognizably analog.

**Implementation at character=1.0:** Apply to the base saw: (a) slight exponential curvature to the ramp (convex bend, ~2-3% deviation from linear), (b) soften the reset transition with a short polynomial or tanh rolloff (equivalent to ~10-20us at current pitch), (c) gentle one-pole lowpass tracking pitch. Crossfade all deformations smoothly from character=0 (perfect digital) to character=1.0 (full Minimoog reference).

**Confidence:** HIGH -- Minimoog circuit analysis extensively documented in Stilson & Smith (CCRMA, 1996), Valimaki et al. (2010), and Moog service manuals.

### Square/Pulse -- Primary Reference: Roland SH-101 / Juno-106

**Why Roland:** Roland's CEM3340-based square waves defined the sound of new wave, synthpop, and acid house. They have a distinctively warm, round quality that sits perfectly in a mix.

**What makes them sound that way:**

- **Comparator slew rate:** The square is derived from the saw via a comparator. Real comparators have finite slew rate, producing rounded rising/falling edges with rise/fall times of approximately 1-5 microseconds depending on load capacitance. This is the primary source of the "round" quality -- it naturally rolls off harmonics proportional to the slew rate.
- **Slight ringing at transitions:** Parasitic capacitance and inductance (PCB traces, component leads) cause subtle overshoot at transitions, typically 5-10% of amplitude that decays within 1-2 microseconds. This adds a tiny burst of high-frequency energy at each edge that gives the square a slight "snap."
- **Duty cycle asymmetry:** Different propagation delays for rising vs falling edges create slight asymmetry, typically 0.5-2% deviation from perfect 50% duty cycle. This introduces even harmonics that would be absent from a perfect symmetric square.
- **Inherited saw character:** Since the square is derived from the saw via comparison, imperfections in the saw (ramp curvature, reset shape) subtly affect the comparator switching point timing, coupling the saw's character into the square.

**Implementation at character=1.0:** Apply: (a) sigmoid/tanh edge softening with adjustable rise/fall time (~2-5us equivalent), (b) optional tiny overshoot modeled as a damped sinusoid at transitions, (c) slight duty cycle offset of 0.5-1.5%. Edge softening is by far the most important component.

**Confidence:** HIGH -- CEM3340 datasheet is publicly available; SH-101 and Juno-106 are among the most analyzed synths in existence.

### Triangle -- Primary Reference: Moog Voyager / Prophet-5 Rev 3.3

**Why these:** Triangle waves reveal analog character through peak rounding and slope asymmetry. The Moog Voyager (modern Moog, classic topology) and Prophet-5 Rev 3.3 (SSM2040-based) produce warm, characterful triangles.

**What makes them sound that way:**

- **Rounded peaks and valleys:** The integrator circuit generating the triangle has finite bandwidth. At the direction-reversal points, the integrator curves through the transition rather than changing direction instantaneously. Peak shape approximates a sinusoidal cap rather than a sharp mathematical point. The radius of curvature is determined by the integrator's bandwidth.
- **Slope asymmetry:** Charging and discharging current paths through the integrating capacitor are never perfectly matched. Typical mismatch is 1-5%, creating one slope slightly steeper than the other. This introduces even harmonics (primarily 2nd harmonic) that are absent from a perfect symmetric triangle.
- **Subtlety:** A perfect triangle has only odd harmonics at 1/n^2 amplitude, making it already close to a sine. Analog imperfections on triangle are more subtle than on saw or square -- real but requiring attentive listening to appreciate.

**Implementation at character=1.0:** Apply: (a) polynomial or sinusoidal rounding at peaks and valleys (smooth curve over the top/bottom ~5-10% of waveform amplitude), (b) slight asymmetry between rising/falling slopes (2-3% steeper on one side). Peak rounding is the more perceptually important effect.

**Confidence:** MEDIUM -- General mechanisms well-understood, but triangle character differences between specific synths are subtle and less extensively documented than saw/square. Quantitative targets are approximate.

### Sine -- Primary Reference: Minimoog (Triangle-Derived)

**Why Minimoog:** Most classic analog synths lack a dedicated sine oscillator. The "sine" is derived from the triangle via a waveshaper circuit -- typically a differential transistor pair (Moog, Sequential) or diode network. This derivation process defines the character.

**What makes it sound that way:**

- **Residual harmonics (THD):** The triangle-to-sine waveshaper intentionally cancels the triangle's harmonics, but cancellation is imperfect. Typical THD for an analog "sine" is 1-5%, primarily 3rd harmonic (residual from triangle source) plus some 2nd harmonic (from asymmetry in the shaping circuit). A perfect mathematical sine is actually a telltale sign of digital origin.
- **Asymmetry inheritance:** Triangle asymmetry carries through to the sine, appearing as even-harmonic distortion (primarily 2nd harmonic). This gives the sine a slightly "fat" quality.
- **Soft clipping at peaks:** The waveshaper saturates the triangle peaks into sinusoidal shape, but the saturation curve is not mathematically perfect. Peaks are marginally wider or narrower than a true sine depending on the specific circuit.

**Key implementation insight:** At character=0, output a pure `sin(2*pi*phase)`. At character=1.0, add 1-3% THD using Chebyshev polynomial mixing or a simple polynomial waveshaper on a triangle: `out = a1*tri + a3*tri^3 + a5*tri^5` with tuned coefficients. Do NOT add too much distortion -- real analog sines are surprisingly clean. The difference from a perfect sine is subtle.

**Confidence:** MEDIUM -- sine shaping circuits vary significantly between manufacturers. The general principle (triangle-derived with residual THD) is universal, but specific harmonic profiles vary.

---

## Analog Imperfections: Detailed Analysis

Ranked by perceptual impact. This directly informs what the Drift knob controls and in what proportions.

### Tier 1: Perceptually Critical (users notice immediately)

| Imperfection | What It Is | Perceptual Effect | Implementation | Complexity |
|-------------|------------|-------------------|----------------|------------|
| **Pitch drift** | Slow random wandering of pitch, typically +/- 2-10 cents over seconds to minutes | The "warming up" sound. The single most recognizable analog behavior. Creates natural chorusing when multiple oscillators drift apart. Makes sustained notes feel alive rather than static. | Ornstein-Uhlenbeck process: sum of 3-4 independent band-limited random walks at different time scales (0.05 Hz very slow drift, 0.2 Hz medium wander, 0.8 Hz gentle vibrato-like movement, ~2 Hz subtle fast wobble). The OU process has mean-reversion, which prevents unbounded pitch wandering and creates realistic "centered but wandering" behavior. Scale total deviation with drift knob. | Medium |
| **HF rolloff** | Progressive attenuation of harmonics above ~10-15 kHz | Warmth. Digital saws and squares sound "brittle" and "icy" without this. This is arguably the single most important contributor to perceived warmth, even more than waveshape deformation. Immediately audible in A/B comparison. | One-pole lowpass whose cutoff decreases as pitch increases. At C2 (65 Hz): cutoff ~18 kHz (barely audible). At C5 (523 Hz): cutoff ~14 kHz (noticeable warmth). At C7 (2093 Hz): cutoff ~10 kHz (significant softening). Scale with character knob (this is character, not drift). Can alternatively be baked directly into waveform generation by rolling off the higher PolyBLEP corrections. | Low-Med |
| **Waveform shape deformation** | Non-ideal waveshapes: curved saw ramp, rounded square edges, rounded triangle peaks, impure sine | This IS the analog character. When users say "it sounds analog" they primarily mean the waveforms have these specific imperfections. The waveshape difference is what makes Minimoog saw sound different from a digital saw. | Per-shape parametric deformation as detailed in the Reference Targets section. This is the character knob's primary job. | Med-High |

### Tier 2: Important for Authenticity (noticed in careful listening, felt as "aliveness")

| Imperfection | What It Is | Perceptual Effect | Implementation | Complexity |
|-------------|------------|-------------------|----------------|------------|
| **Phase jitter** | Cycle-to-cycle timing variation, typically <0.1% of the period | Subtle animation. Prevents the perfectly static, mechanical quality of digital oscillators. Each cycle is very slightly different in duration. Most audible on sustained notes and in stereo with another oscillator. | Small random perturbation added to phase increment each sample. Use band-limited noise (lowpass filtered white noise, cutoff ~100-500 Hz) scaled very small. Must be subtle enough to not sound like FM modulation. | Low |
| **DC offset drift** | Small slowly-varying voltage added to the output, typically 10-50mV in a +/-5V signal | Not directly audible as pitch or timbre change, but affects downstream modules: filters respond differently with DC at input, waveshapers become asymmetric, VCAs produce soft clicks on note edges. Adds authenticity when users scope the output. | Slow random walk (0.01-0.1 Hz) added to final output. Very small amplitude relative to signal. | Low |
| **Pitch slew** | Pitch does not change instantaneously; small lag when CV changes rapidly | Subtle portamento-like smoothing on fast pitch sequences. Prevents the perfectly instant pitch transitions that sound digital. Audible on rapid arpeggios as a very slight "slide" into each note. | One-pole lowpass on the pitch CV before exponential conversion. Time constant ~1-5ms at full drift, 0ms at drift=0. | Low |
| **Component spread** | Each physical unit has slightly different characteristics due to manufacturing tolerances | Multiple module instances sound slightly different, like buying two of the same synth from different production runs. One drifts faster, another has a slightly brighter saw. Meaningful for detuned unison patches. | Per-instance random seed from module ID. Offsets: drift rate (+/- 20%), DC offset base (+/- 30%), waveform curvature (+/- 10%), HF rolloff frequency (+/- 5%). Calculated once at creation. | Low |

### Tier 3: Subtle/Specialist (completionists value, most users will not notice)

| Imperfection | What It Is | Perceptual Effect | Implementation | Complexity |
|-------------|------------|-------------------|----------------|------------|
| **Tracking error** | Pitch deviates from perfect V/Oct at range extremes. Real VCOs go sharp in upper octaves, flat in lower. | Authentic but potentially frustrating. Detuning at extremes was a constant annoyance for real analog synth players. Noticeable when playing wide intervals. | Slight nonlinear S-curve on V/Oct: +5-15 cents at C7, -5-10 cents at C1, accurate around C3-C4 center. Must be togglable via right-click. Default: OFF. | Low |
| **Waveform bleed** | Residual signal from other waveform stages in the output | Very subtle crosstalk. See dedicated Bleed section below. For a morphed output, manifests as widened transition zones. | Mix small amounts of adjacent shapes into morph boundaries. Scale with character knob. | Low |
| **Power supply ripple** | Very low-level hum from supply coupling into oscillator | Barely audible at ~-60 dB in real synths. Not worth the CPU. | Do not implement for v1. | N/A |
| **Temperature sensitivity** | Pitch/character change with temperature over minutes/hours | Already covered perceptually by pitch drift and component spread. Modeling actual thermal curves adds complexity with zero additional audible benefit. | Do not implement separately. Drift knob covers this perception. | N/A |

### Drift Knob Scaling Recommendation

The drift knob scales all Tier 1 drift effects and Tier 2 imperfections together in musically curated proportions. HF rolloff and waveform deformation are controlled by the character knob instead.

| Drift Position | Pitch Drift | Phase Jitter | DC Offset | Pitch Slew | Subjective Quality |
|----------------|-------------|--------------|-----------|------------|-------------------|
| 0.0 | 0 cents | 0% | 0 mV | 0 ms | Perfectly stable digital |
| 0.25 | +/- 1 cent | 0.01% | 5 mV | 0.5 ms | Barely perceptible movement |
| 0.50 | +/- 3 cents | 0.03% | 15 mV | 1.5 ms | "Well-maintained studio analog" |
| 0.75 | +/- 6 cents | 0.06% | 30 mV | 3 ms | "Vintage, character-full" |
| 1.0 | +/- 10 cents | 0.1% | 50 mV | 5 ms | "Just powered on, still warming up" |

**Note:** These are starting points from literature and experience. Final values MUST be tuned by ear. The subjective "feel" matters more than hitting specific numbers. Use logarithmic or exponential scaling so most of the useful range is in the first 75% of knob travel.

---

## Waveform Bleed in Real Analog Circuits

In real analog oscillators, waveform bleed occurs because:

1. **Shared core oscillator:** All waveforms derive from the same ramp/saw core. Triangle is integrated from the square; sine is shaped from the triangle. They share circuit nodes.
2. **Parasitic capacitive coupling:** PCB traces running near each other couple signals at approximately -40 to -60 dB below the primary signal level.
3. **Op-amp crosstalk through power rails:** Output buffers in close proximity couple through shared supply rails, especially at high frequencies.
4. **Imperfect waveshaper nulling:** The triangle-to-sine shaper does not perfectly cancel triangle harmonics, leaving residual odd harmonics in the sine output.

**What it sounds like in practice:** At individual outputs of a real synth, faint ghosts of other waveforms are present. Most noticeable on the sine output where a faint "buzz" (sawtooth residual) is audible at -40 dB. On saw and square outputs, bleed is masked by rich harmonic content and is effectively inaudible.

**Relevance to this project's single morphed output:** Traditional inter-output bleed does not directly apply since there is one output. Instead, model bleed as part of the morph transition:

- **At morph positions near shape boundaries** (~0.25 where tri meets saw, ~0.50 where saw meets square), the transition should not be a perfectly clean mathematical crossfade when character > 0. A small amount of the adjacent shape should "leak" into the transition zone.
- **Scale with character knob:** At character=0, crossfade is mathematically precise. At character=1.0, transition zones are wider with ~2-5% residual from the neighboring shape.
- **Most critical zone:** The sine region. A sine with character > 0.5 should carry faint traces of triangle harmonics (3rd, 5th harmonic at -30 to -40 dB), simulating imperfect triangle-to-sine waveshaping.

---

## Antialiasing Approaches for Audio-Rate VCO

### PolyBLEP -- Recommended Primary Approach

**What:** Polynomial Band-Limited Step. Applies a low-order polynomial correction to samples near discontinuities (saw resets, square transitions).

**Used by:** VCV Fundamental VCO, Befaco EvenVCO, Bogaudio VCO, and most quality VCV Rack oscillators.

**Quality:** Good. Eliminates the worst aliasing (loud inharmonic tones). Some residual aliasing above C7, but acceptable and consistent with real analog oscillators' high-frequency behavior.

**CPU cost:** Very low. 2-4 extra multiplications per sample, only near discontinuities.

**Implementation notes:**
- For discontinuity at fractional position `t` within current sample (0 <= t < 1):
  - Apply correction to the sample containing the discontinuity and the next sample
  - 2nd-order PolyBLEP: `correction = t*t + 2*t - 1` (before) and `-(t*t) + 2*t - 1` scaled by discontinuity magnitude
- Apply to saw: one correction per cycle at reset
- Apply to square: two corrections per cycle (both edges)
- Triangle and sine: no correction needed (continuous, no discontinuities)
- Use 4th-order (or higher) PolyBLEP from the start -- marginal CPU increase, meaningful quality improvement at high pitches

**Critical morphing consideration:** As the morph sweeps, discontinuity structure changes:
- Sine/triangle region (0.0-0.25): no discontinuities, no PolyBLEP
- Tri-to-saw (0.25-0.50): discontinuity appears and grows. PolyBLEP magnitude must ramp in smoothly, proportional to discontinuity size
- Saw-to-square (0.50-0.75): one discontinuity splits into two. Must transition correction from one to two discontinuities per cycle
- Square region (0.75-1.0): two discontinuities at full PolyBLEP correction

This morph-aware PolyBLEP is the trickiest part of the VCO implementation.

### MinBLEP -- Higher Quality Alternative (Defer to v2)

**What:** Minimum-phase Band-Limited Step using a precomputed windowed sinc table. More accurate than PolyBLEP.

**Quality:** Excellent. Virtually alias-free across full frequency range.

**CPU cost:** Moderate. ~3-5x PolyBLEP due to table lookup, interpolation, and correction buffer (~32-64 samples).

**Why defer:** More complex to implement, harder to integrate with morphing (correction buffer interacts with morph changes), and quality difference over PolyBLEP is marginal for musical use. Consider for v2 if users specifically request better aliasing.

### Oversampling -- Complementary Option

**What:** Process at 2x/4x sample rate, downsample with steep lowpass (half-band FIR).

**Quality:** PolyBLEP + 2x oversampling approaches MinBLEP quality.

**CPU cost:** 2x oversampling approximately doubles oscillator CPU; 4x quadruples.

**Recommendation:** Offer as right-click menu option: "Oversampling: Off / 2x / 4x". Default Off. This is established VCV convention (Befaco and Bogaudio both do this).

### Antialiasing Recommendation

**Use PolyBLEP (4th-order) as the always-on primary method, with optional 2x/4x oversampling via right-click menu.** This matches quality expectations, keeps default CPU low, and provides an upgrade path. Defer MinBLEP to v2.

---

## Existing VCV Rack Module Landscape

### Befaco EvenVCO
- **Morphing:** None. Separate outputs per shape.
- **Analog character:** Minimal. Targets precision, not vintage.
- **Antialiasing:** PolyBLEP.
- **Competitive position:** Not a competitor in analog-character space.

### Bogaudio VCO / LVCO
- **Morphing:** No continuous morph.
- **Analog character:** Minimal. Clean digital focus.
- **Antialiasing:** PolyBLEP with optional oversampling.
- **Display:** LVCO has waveform display (good UI reference).
- **Competitive position:** Well-regarded for features/efficiency. Not analog-modeling.

### Valley Terrorform
- **Morphing:** Wavetable scanning. Different paradigm.
- **Analog character:** Modern digital aesthetic.
- **Display:** Waveform display (good reference).
- **Competitive position:** Different market segment entirely.

### Lindenberg Research (VCO, Woldemar, Alma)
- **Morphing:** Some modules have wave morphing.
- **Analog character:** Explicitly analog-modeled with drift and nonlinearity. Closest competitor.
- **Display:** Excellent oscilloscope-style with glow effects.
- **Competitive position:** Main competitor. However, Lindenberg provides "generic analog character" rather than specific classic synth references. Forge Audio differentiates with: (a) named synth targets rather than generic warmth, (b) three independent axes (morph/character/drift) rather than single analog amount, (c) character knob answers "WHICH analog" not just "HOW MUCH."

### VCV Fundamental VCO-2
- **Morphing:** Has a "wave" knob that morphs between shapes. The free baseline.
- **Analog character:** None. Pure digital.
- **Antialiasing:** PolyBLEP.
- **Competitive position:** The module Forge Audio must clearly surpass. If the character knob at zero sounds identical to VCO-2, the morph alone is insufficient differentiation. The analog character engine is what justifies the module's existence.

### Gap Analysis

**No existing VCV Rack module combines all three of:**
1. Continuous waveform morphing between four classic shapes
2. Specific classic synth reference modeling (not generic warmth)
3. Controllable, multi-dimensional analog imperfection system

This three-axis approach is genuine whitespace.

---

## Differentiators

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Three-knob analog engine (morph + character + drift)** | No other VCV module separates waveform selection, tonal reference, and imperfection into three independent CV-controllable axes. The system architecture is the differentiator. | High | Each axis independently useful; together they create a unique design space. |
| **Named classic synth references on character knob** | "Minimoog saw" is more compelling and concrete than generic "warmth." Users know what a Minimoog sounds like. The character knob has a specific, verifiable target. | High | Requires accurate per-shape modeling. Reference targets documented above. |
| **Component spread (per-instance variation)** | Multiple instances sound slightly different, like real hardware units. Meaningful for detuned unison patches. | Low | Seed from module ID. One-time random offsets to drift, DC, curvature, rolloff. |
| **Waveform display showing all three axes** | Display makes the abstract tangible: SEE the curved Minimoog ramp, SEE drift wobble, SEE DC offset. Not just a scope -- a composite view of the three control axes. | High | Dirty-flag updates. Must efficiently composite morph+character+drift. |
| **Through-zero FM (VCO)** | True TZFM for clean, stable FM tones with analog character layered on. Uncommon in VCV analog-style VCOs. | High | Signed frequency, phase decrements. Tricky with antialiasing near zero-crossing. |
| **Phase distortion (VCO)** | Casio CZ-style synthesis adds a timbral dimension orthogonal to morph. Rare in VCV Rack combined with analog modeling. | Medium | Modifies phase lookup. Interacts uniquely with character (phase-distorted analog waveforms are unexplored territory). |
| **Inverted output** | Both normal and inverted morphed output. Saves a utility module. | Low | Negate output. Trivial but useful. |
| **Oversampling option** | Power users can trade CPU for quality. Established VCV convention. | Medium | Right-click: Off/2x/4x. Half-band FIR decimation. |

---

## Anti-Features

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| **Separate waveform outputs** | Breaks the design concept. The single morphed output IS the point. Separate outputs make it "just another multi-output VCO with knobs." | Single morphed + inverted. Users wanting separate shapes have Befaco, Bogaudio, Fundamental. |
| **Polyphonic operation (v1)** | 16x CPU cost, complicates drift (N independent generators), complicates display (which voice to show?). Mono is the modular norm. | Mono for v1. Per-instance component spread gives natural detuning across multiple instances. |
| **Built-in effects** | Scope creep. Oscillators oscillate; effects process. Modular philosophy = separate concerns. | Clean output. Users choose their own downstream signal chain. |
| **Wavetable mode** | Different paradigm. Dilutes analog identity. Competitors (Valley Terrorform) own this space. | Four classic shapes with morphing. The constraint is the identity. |
| **Named synth presets** | Undercuts hands-on tweaking. "Minimoog" preset implies emulation accuracy and invites trademark issues. | Character knob position IS the preset. Descriptive labels only (no trademarked names on panel). |
| **MIDI input / quantization** | Out of scope for an oscillator. These are upstream module responsibilities. | Standard V/Oct input. |
| **Amplitude envelope** | Oscillators oscillate; envelopes shape. Combining = semi-voice that breaks modular philosophy. | Continuous output. Users patch through VCA + EG. |
| **Scope / spectrum analyzer display** | The display is a shape preview, not a measurement tool. Adding FFT, time-base, triggers bloats UI and CPU. | Single-cycle shape display, fixed zoom, phase dot. Users have Fundamental Scope for analysis. |
| **Individually exposed drift parameters on panel** | Separate knobs for drift rate, jitter, DC, slew creates option paralysis and panel clutter. The power is ONE knob scaling everything in curated proportions. | Single drift knob. Individual parameter fine-tuning available in right-click menu for advanced users. |
| **PWM as separate control** | Already subsumed by morph architecture. In the square region, morph position inherently controls duty cycle. Separate PWM input is redundant. | Modulate morph CV while knob is in square region for PWM effect. |
| **Built-in sub-oscillator** | Panel complexity, dilutes three-knob focus. Users patch a second instance one octave down. | Leave to dedicated sub-osc modules or second instance. |

---

## Feature Dependencies

```
Core Waveform Generation (sine, tri, saw, square from shared phase)
  |
  +-- Waveform Morphing (requires all four shapes)
  |     |
  |     +-- Morph CV Input
  |     |
  |     +-- PolyBLEP Antialiasing [VCO only]
  |     |     (must track morph position for discontinuity locations/magnitudes)
  |     |     |
  |     |     +-- Oversampling Option (complements PolyBLEP, independent toggle)
  |     |
  |     +-- Waveform Bleed in Morph Zones (widens transitions at high character)
  |
  +-- Analog Character Engine (deforms base waveforms toward reference targets)
  |     |
  |     +-- Minimoog Saw Reference (exponential ramp, soft reset)
  |     +-- Roland Square Reference (slewed transitions, duty asymmetry)
  |     +-- Moog/Prophet Triangle Reference (rounded peaks, slope asymmetry)
  |     +-- Analog Sine Reference (1-3% THD, residual harmonics)
  |     |
  |     +-- HF Rolloff (pitch-tracking lowpass, part of character chain)
  |     +-- Character CV Input
  |
  +-- Drift Engine (modulates oscillator, independent of character)
  |     |
  |     +-- Pitch Drift (Ornstein-Uhlenbeck, primary component)
  |     +-- Phase Jitter (band-limited noise on phase increment)
  |     +-- DC Offset Drift (slow random walk on output)
  |     +-- Pitch Slew (one-pole lowpass on pitch CV)
  |     +-- Component Spread (per-instance seed, offsets all parameters)
  |     |
  |     +-- Drift CV Input
  |
  +-- Tracking Error [VCO only] (toggle, modifies V/Oct curve)

Waveform Display
  +-- Requires: phase access, morph state, character state, drift state
  +-- Phase Tracking Dot (requires phase accumulator)
  +-- Dirty-flag redraw (requires parameter change detection)

VCO-Specific (extends shared foundation)
  +-- V/Oct Input
  +-- Hard Sync (phase reset on external trigger)
  +-- Through-Zero FM (signed frequency, phase decrements)
  |     +-- Stable antialiasing at negative frequencies
  +-- Phase Distortion (modifies phase lookup, orthogonal to morph)
```

---

## MVP Recommendation

### Phase 1: LFO Module

Validates the three-knob concept at LFO rates where antialiasing is irrelevant.

**Implementation order:**
1. **Core waveform generation with morphing** -- foundation
2. **Waveform display with phase dot** -- build early so all subsequent work is visible
3. **Drift engine** (pitch drift + phase jitter + DC offset + pitch slew) -- audible proof of analog promise
4. **Analog character engine with all four reference models** -- the key differentiator
5. **HF rolloff** (pitch-tracking lowpass) -- major warmth contributor
6. **CV inputs for morph, character, drift** -- modular integration
7. **Component spread** -- low effort, high value
8. **FM input and reset/sync** -- standard LFO functionality (proven in POC)
9. **Inverted output** -- trivial, useful
10. **Waveform bleed in morph zones** -- subtle polish

**Defer:** Tracking error (N/A for LFO), phase distortion (VCO feature), oversampling (N/A at LFO rates).

### Phase 2: VCO Module

Adds audio-rate complexity on shared foundation.

**Implementation order:**
1. **PolyBLEP antialiasing** (morph-aware) -- mandatory for audio rate
2. **V/Oct input** -- mandatory
3. **Hard sync** -- table stakes
4. **Reuse morph/character/drift from LFO** -- shared engine code
5. **Through-zero FM** -- differentiator
6. **Phase distortion** -- differentiator
7. **Tracking error toggle** -- authenticity detail
8. **Oversampling option** -- power user quality upgrade

**Defer:** MinBLEP (only if aliasing complaints), polyphony (only if significant demand).

---

## Morph Implementation Strategy

| Knob Range | Transition | Technique | Antialiasing Notes |
|------------|------------|-----------|-------------------|
| 0.00-0.25 | Sine to Triangle | Sharpen peaks progressively. Use `sign(sin) * |sin|^(1/k)` where k goes from 1.0 (sine) upward (triangle). Or: parametric blend of curvature at zero-crossings from sinusoidal to linear. | No discontinuities. No PolyBLEP needed. |
| 0.25-0.50 | Triangle to Saw | Introduce progressive asymmetry. One slope steepens while the other shallows. At 0.50, one slope is near-vertical (saw reset), other spans full cycle. | Discontinuity appears and grows. PolyBLEP ramps in proportional to discontinuity magnitude. |
| 0.50-0.75 | Saw to Square | Ramp steepens into two flat segments connected by two transitions. Parametrize as variable-slope trapezoid. | One discontinuity becomes two. PolyBLEP transitions from single to dual correction. |
| 0.75-1.00 | Square (endpoint) | Square wave at morph=1.0. Option: sweep PWM from 50% toward narrow pulse in this zone. | Two discontinuities at full PolyBLEP. |

**Critical:** Every intermediate morph position must be a musically valid waveshape, not a blend artifact. Position 0.375 should sound like an asymmetric triangle/ramp, not "triangle plus saw noise." Morph must change waveshape parameters, not crossfade two audio signals.

---

## Sources and Confidence

| Topic | Basis | Confidence |
|-------|-------|------------|
| Minimoog oscillator characteristics | Training data: Stilson & Smith (CCRMA 1996), Valimaki et al. (2010), service manuals | HIGH |
| Roland CEM3340/SH-101/Juno-106 | Training data: CEM3340 datasheet, service manuals, community analysis | HIGH |
| PolyBLEP antialiasing | Training data: Valimaki, Esqueda et al., VCV Rack source analysis | HIGH |
| Ornstein-Uhlenbeck for drift | Training data: DSP and stochastic process literature | HIGH |
| VCV module landscape (current) | Training data through May 2025 | MEDIUM -- may have changed |
| Prophet-5/Voyager triangle specifics | Training data: service manuals, community analysis | MEDIUM |
| Waveform bleed levels (-40 to -60 dB) | Training data: analog circuit design knowledge | MEDIUM -- approximate |
| MinBLEP in VCV context | Training data only | LOW -- not verified against current SDK |

**Gaps requiring future verification:**
- VCV Rack module landscape may have new analog-character competitors since May 2025
- Specific parameter values (drift amounts, rolloff frequencies, character scaling) are literature-based starting points that MUST be tuned by ear
- Morph-aware PolyBLEP behavior across shape transitions needs careful testing for click-free operation
