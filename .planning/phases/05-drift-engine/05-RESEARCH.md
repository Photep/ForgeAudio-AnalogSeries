# Phase 5: Drift Engine - Research

**Researched:** 2026-03-07
**Domain:** Ornstein-Uhlenbeck drift modeling, VCV Rack random API, panel layout
**Confidence:** HIGH

## Summary

Phase 5 implements pitch drift via a multi-timescale Ornstein-Uhlenbeck (OU) process that modulates the phase accumulation rate. The OU process is a mean-reverting stochastic process, ideal for modeling analog oscillator thermal drift: it produces smooth, bounded random wander rather than unbounded random walks. By layering four OU processes at different timescales (0.05Hz, 0.2Hz, 0.8Hz, ~2Hz), we achieve the characteristic multi-timescale instability of real analog oscillators where slow thermal drift coexists with faster component-level jitter.

The implementation is straightforward: each OU layer maintains a single float state variable updated via the Euler-Maruyama discretization. VCV Rack provides `rack::random::Xoroshiro128Plus` which can be instantiated per-module with unique seeding from the module's `id` field, ensuring independent drift per instance. The existing codebase already declares `DRIFT_PARAM`, `DRIFT_CV_INPUT`, and has the `progressiveCurve()` function ready for reuse. The primary new additions are: `DRIFT_ATTEN_PARAM` in the enum, four OU state variables, per-module RNG, drift CV processing, and the panel SVG update for the new trimpot.

**Primary recommendation:** Use four independent OU processes with per-layer theta (mean reversion rate) and sigma (noise amplitude), combined additively, scaled by `progressiveCurve(drift)`, applied as a multiplicative factor on `deltaPhase`. Seed a per-module Xoroshiro128Plus from `std::random_device` in the constructor.

<user_constraints>

## User Constraints (from CONTEXT.md)

### Locked Decisions
- Moderate pitch variation at full drift: 5-10% of base frequency
- Progressive x^2 curve matching character knob: subtle first half, stronger second half
- At zero: full bypass -- no OU computation, no random generation, zero CPU overhead (matches character-at-zero pattern)
- Each module instance gets a unique random seed -- two LFOs with identical settings drift independently, like two real analog synths
- Add DRIFT_ATTEN_PARAM trimpot matching the Morph CV and Character CV pattern
- Additive offset behavior: knob sets center position, CV sweeps around it (same as Morph/Character CV)
- Hard clamp at 0-1 range when combined knob + CV exceeds bounds
- Attenuator defaults to 0 (no CV effect until turned up) -- consistent with Morph/Character attenuators
- 7 components organized as grouped pairs: [MATrim MCV] [CATrim CCV] [DATrim DCV] [OUT]
- Each attenuator trimpot sits directly next to its CV jack for visual clarity
- Panel SVG needs updating to accommodate the new trimpot
- Drift modulates pitch (phase increment rate), not waveform shape -- display waveform trace does NOT change with drift
- Drift is visible through the phase dot's speed variation (speeding up and slowing down)
- No display buffer refresh needed for drift changes -- only phase dot position changes (already updated every sample via displayPhase atomic)
- Subtle dot instability at higher drift levels: increased glow variation or trail jitter as a visual hint that drift is active
- Remove/resolve the TODO comment at line 198 ("Phase 5: add driftChanged trigger here") -- no buffer trigger needed
- OU drift state reinitializes fresh on each patch load -- no save/restore of random state

### Claude's Discretion
- Exact OU process parameters (mean reversion rates, noise amplitudes per layer)
- How the four OU layers are combined and scaled
- Implementation of subtle dot instability visual (glow variation vs trail jitter vs other)
- Exact bottom row spacing to fit 7 components in ~57mm usable panel width
- Random seed generation method (module ID, random_device, etc.)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope

</user_constraints>

<phase_requirements>

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| DRFT-01 | Drift knob controls pitch drift via multi-timescale Ornstein-Uhlenbeck process (layered random walks at 0.05Hz, 0.2Hz, 0.8Hz, ~2Hz) | OU discrete-time formula verified, four-layer architecture designed, parameter recommendations provided, zero-bypass pattern documented |
| DRFT-02 | Drift CV input modulates drift amount | CV processing pattern replicated from Morph/Character, DRIFT_ATTEN_PARAM addition documented, additive offset + clamp pattern confirmed |

</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack SDK | 2.6.6 | Module framework, DSP, panel rendering | Project's target platform |
| rack::random::Xoroshiro128Plus | (SDK built-in) | Per-module RNG for OU noise generation | Fast, non-cryptographic, VCV-native, can be instantiated per-module |
| std::random_device | C++11 | Seed generation for per-module RNG | Platform-provided entropy for unique seeds |
| std::normal_distribution | C++11 | Gaussian noise for OU process | Standard Box-Muller transform; compatible with Xoroshiro128Plus as engine |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| rack::math::clamp | (SDK built-in) | CV signal clamping | Already used for Morph/Character CV clamping |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Per-module Xoroshiro128Plus | rack::random::normal() (global) | Global RNG is simpler but shared state means two identical modules would NOT produce independent drift sequences; per-module RNG is required by user decision |
| std::normal_distribution | Manual Box-Muller | std::normal_distribution is cleaner and caches the second variate; manual Box-Muller is what rack::random::normal() already does |
| Euler-Maruyama OU update | Exact OU sampling formula | Euler-Maruyama is simpler and sufficient at audio sample rates; exact formula uses exp() per-sample which is more expensive |

## Architecture Patterns

### Integration Points in Existing Code

```
src/AnalogLFO.cpp
  Line 14: ParamId enum -- ADD DRIFT_ATTEN_PARAM before PARAMS_LEN
  Line 30: Module state  -- ADD OU state variables, per-module RNG
  Line 44: progressiveCurve() -- REUSE for drift knob mapping
  Line 172: process()     -- ADD drift CV processing, OU update, deltaPhase modulation
  Line 198: TODO comment  -- REMOVE (no driftChanged trigger needed)
  Line 309-353: drawPhaseDot() -- MODIFY for subtle dot instability at high drift
  Line 404+: Widget constructor -- ADD drift attenuator trimpot, RESPACE bottom row

res/AnalogLFO.svg
  Bottom row labels   -- ADD "DATrim" label (or reposition existing labels)
  Component positions -- RESPACE 7 components
  Components layer    -- ADD drift attenuator trimpot reference
```

### Pattern 1: Ornstein-Uhlenbeck Discrete-Time Update (Euler-Maruyama)

**What:** The OU process is a mean-reverting random walk defined by the SDE: `dx = theta * (mu - x) * dt + sigma * dW`, where `dW = sqrt(dt) * N(0,1)`.

**When to use:** Every audio sample when drift > 0.

**Discretization (Euler-Maruyama):**
```cpp
// Per-layer OU state update
// theta: mean reversion rate (controls how fast the process returns to zero)
// sigma: noise amplitude (controls how far the process wanders)
// mu = 0 (drift centered around zero)
// dt = args.sampleTime
//
// x[n+1] = x[n] + theta * (0 - x[n]) * dt + sigma * sqrt(dt) * gaussian_noise
// Simplified: x[n+1] = x[n] * (1 - theta * dt) + sigma * sqrt(dt) * noise

float noise = normalDist(rng);  // N(0,1) from per-module RNG
ouState += theta * (0.f - ouState) * dt + sigma * sqrtDt * noise;
```

**Key property:** The process is bounded in expectation. Stationary variance = sigma^2 / (2 * theta). The process will hover around zero with excursions proportional to sigma/sqrt(2*theta).

### Pattern 2: Multi-Timescale Layering

**What:** Four OU layers at different frequencies summed to create naturalistic multi-timescale drift.

**Design:**
```cpp
// Four OU layers with different characteristics:
//
// Layer 0: Very slow thermal drift   (0.05Hz) - large excursions, slow wander
// Layer 1: Medium thermal drift      (0.2Hz)  - component-level variation
// Layer 2: Fast component noise      (0.8Hz)  - rapid subtle fluctuations
// Layer 3: Jitter                    (~2Hz)   - cycle-level timing variation
//
// theta = 2*pi*freq (mean reversion rate proportional to target frequency)
// sigma chosen so each layer contributes appropriate amount to total drift
//
// Combined drift = sum of all layers, scaled by progressiveCurve(driftKnob)
// Applied as: deltaPhase *= (1.0 + driftScale * combinedDrift)
```

### Pattern 3: Zero-Overhead Bypass

**What:** When drift knob is at zero (after CV), skip all OU computation entirely.

**When to use:** Always -- matches the established character-at-zero pattern.

```cpp
// Source: existing pattern from computeSine/Triangle/Saw/Square
float drift = rack::math::clamp(driftKnob + driftAtten * driftCV / 10.f, 0.f, 1.f);
if (drift < 0.001f) {
    // Skip OU update entirely -- zero CPU overhead
    // deltaPhase remains unmodified
} else {
    float driftAmount = progressiveCurve(drift);
    // Update all 4 OU layers, combine, apply to deltaPhase
}
```

### Pattern 4: Per-Module Unique RNG

**What:** Each module instance gets its own Xoroshiro128Plus seeded uniquely.

```cpp
// In module struct declaration:
rack::random::Xoroshiro128Plus rng;
std::normal_distribution<float> normalDist{0.f, 1.f};

// In constructor (AnalogLFO()):
std::random_device rd;
rng.seed(rd(), rd());  // Two 32-bit seeds from hardware entropy
```

**Why not module `id`:** The module `id` is only assigned when added to the engine (it is -1 in the constructor). Using `std::random_device` in the constructor is simpler and guarantees unique seeds without needing to wait for `onAdd()`. The decision specifies "fresh randomness on load" anyway -- no reproducibility needed.

### Pattern 5: Drift Applied to Phase Increment

**What:** Drift modulates the phase accumulation rate, not the waveform shape.

```cpp
// BEFORE phase accumulation:
double deltaPhase = (double)freq * (double)args.sampleTime;

// Apply drift as multiplicative pitch modulation:
// At full drift, combined OU output is scaled to produce 5-10% frequency deviation
if (drift >= 0.001f) {
    float driftAmount = progressiveCurve(drift);
    float combinedOU = /* sum of 4 OU layers */;
    // Scale so full drift produces ~7.5% peak deviation (middle of 5-10% range)
    float driftScale = driftAmount * 0.075f;
    deltaPhase *= (1.0 + (double)(driftScale * combinedOU));
}

phase += deltaPhase;
```

### Anti-Patterns to Avoid
- **Modulating waveform shape with drift:** Drift is a pitch phenomenon, not a shape change. The display trace should NOT change with drift (per user decision).
- **Using global rack::random:** Two modules with identical settings would share the same random stream and drift identically. Per-module RNG is required.
- **Saving/restoring OU state:** Real analog synths don't resume their drift state. Fresh randomness on patch load is more authentic (per user decision).
- **Computing OU when drift is zero:** Must skip entirely for zero CPU overhead (per user decision).
- **Using exp() per-sample for exact OU:** Euler-Maruyama is accurate enough at 44.1kHz+ sample rates and avoids expensive transcendentals.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Gaussian random numbers | Manual Box-Muller transform | `std::normal_distribution<float>` with per-module Xoroshiro128Plus | Standard library implementation caches second variate (2x efficiency), handles edge cases |
| Random seeding | Hash-based seed from module properties | `std::random_device` | Platform-provided entropy, guaranteed unique, simple |
| Signal clamping | Manual min/max | `rack::math::clamp()` | Already used throughout codebase, consistent API |
| CV processing | Custom voltage scaling | Replicate exact pattern from lines 183-192 | Proven pattern, tested in Morph/Character CV |

**Key insight:** The OU process itself IS the custom DSP here -- it's simple enough that a library would be overkill. The four-line Euler-Maruyama update is the entire algorithm. Everything around it (RNG, clamping, CV processing) uses existing tools.

## Common Pitfalls

### Pitfall 1: OU Parameter Tuning -- Too Much or Too Little Drift
**What goes wrong:** OU sigma too high causes audible pitch warbling; too low is imperceptible. theta too low causes the process to wander far and not return; too high kills all variation.
**Why it happens:** The stationary variance of an OU process is sigma^2 / (2*theta). If sigma is large relative to theta, excursions will be large.
**How to avoid:** Design each layer so its stationary standard deviation contributes appropriately to the total drift range. The combined output should peak around +/-1.0 at typical excursions, then the master scaling applies `driftAmount * 0.075` to get 7.5% frequency deviation at full drift. Use the relationship: std_dev = sigma / sqrt(2 * theta).
**Warning signs:** Drift sounds like vibrato (too fast/deep) or is inaudible (too subtle).

### Pitfall 2: Numerical Stability at Low Sample Rates
**What goes wrong:** If `theta * dt > 1.0`, the Euler-Maruyama update becomes unstable (overshoots and diverges).
**Why it happens:** At very high theta values or very low sample rates, the discrete step is too large.
**How to avoid:** With our highest theta = 2*pi*2Hz ~ 12.6 and lowest typical sample rate = 44100Hz, `theta * dt = 12.6 / 44100 = 0.000286` -- well within stability bounds. No concern for this application.
**Warning signs:** OU state values growing unbounded, NaN/Inf output.

### Pitfall 3: sqrt(dt) Precomputation
**What goes wrong:** Computing `std::sqrt(args.sampleTime)` every sample is wasteful.
**Why it happens:** `sqrt(dt)` is constant as long as sample rate doesn't change.
**How to avoid:** Compute `sqrtSampleTime` once and cache it. Update in `onSampleRateChange()` or compute once per process block. Since sample rate changes are rare, caching in a member variable updated on first use or rate change is clean.
**Warning signs:** Unnecessary CPU usage in process() hotpath.

### Pitfall 4: Bottom Row Component Spacing
**What goes wrong:** 7 components in 57mm usable width creates tight spacing; components overlap or look cramped.
**Why it happens:** Panel is 60.96mm (12HP), minus ~2mm margins each side = ~57mm usable. Trimpot diameter ~6mm, PJ301MPort diameter ~8mm. Seven components at equal spacing = ~8.14mm center-to-center, which is tight but feasible since trimpots are smaller than jacks.
**How to avoid:** Use grouped pairs with tighter within-pair spacing (~7mm trimpot-to-jack) and slightly wider between-pair gaps (~9mm). The OUTPUT jack at the end gets its own space.
**Warning signs:** Components visually overlapping, labels unreadable.

### Pitfall 5: Display Dot Instability Overdone
**What goes wrong:** Too much visual instability makes the display look broken rather than subtly alive.
**Why it happens:** Applying drift value directly to visual parameters without attenuation.
**How to avoid:** Keep the instability very subtle -- small glow radius variation or slight trail position jitter, scaled so it's barely noticeable at mid-drift and only clearly visible at full drift.
**Warning signs:** Dot appears to jump erratically rather than gently wobble.

## Code Examples

### OU Layer State Structure
```cpp
// Recommended: compact struct for each OU layer
struct OULayer {
    float state = 0.f;     // Current OU process value
    float theta;           // Mean reversion rate
    float sigma;           // Noise amplitude
    float weight;          // Contribution weight to combined output
};
```

### Recommended OU Parameters (Claude's Discretion)
```cpp
// Four OU layers targeting the specified frequencies
// theta = 2*pi*freq (mean reversion rate)
// sigma chosen for stationary std_dev ~ 1.0 per layer (before weighting)
// sigma = desired_std_dev * sqrt(2 * theta)
//
// Layer 0: 0.05Hz slow wander -- dominant contributor
//   theta = 0.314, sigma = sqrt(2*0.314)*1.0 = 0.793
//   weight = 0.50 (50% of total drift character)
//
// Layer 1: 0.2Hz medium -- secondary contributor
//   theta = 1.257, sigma = sqrt(2*1.257)*1.0 = 1.586
//   weight = 0.25
//
// Layer 2: 0.8Hz fast -- texture
//   theta = 5.027, sigma = sqrt(2*5.027)*1.0 = 3.170
//   weight = 0.15
//
// Layer 3: ~2Hz jitter -- sparkle
//   theta = 12.566, sigma = sqrt(2*12.566)*1.0 = 5.013
//   weight = 0.10
//
// Combined: weights sum to 1.0, so combined output has ~1.0 std dev
// Final scaling: combinedOU * progressiveCurve(drift) * 0.075
// => at full drift: ~7.5% peak frequency deviation (within 5-10% spec)

static constexpr int NUM_OU_LAYERS = 4;

// Initialize in constructor:
ouLayers[0] = {0.f, 2.f * M_PI * 0.05f,  0.793f, 0.50f};  // slow wander
ouLayers[1] = {0.f, 2.f * M_PI * 0.2f,   1.586f, 0.25f};  // medium drift
ouLayers[2] = {0.f, 2.f * M_PI * 0.8f,   3.170f, 0.15f};  // fast texture
ouLayers[3] = {0.f, 2.f * M_PI * 2.0f,   5.013f, 0.10f};  // jitter
```

### Process Function Integration Point
```cpp
void process(const ProcessArgs& args) override {
    float freq = params[RATE_PARAM].getValue();
    freq = std::fmax(freq, 0.001f);

    // Phase accumulation
    double deltaPhase = (double)freq * (double)args.sampleTime;

    // --- DRIFT PROCESSING (new) ---
    float driftKnob = params[DRIFT_PARAM].getValue();
    float driftAtten = params[DRIFT_ATTEN_PARAM].getValue();
    float driftCV = inputs[DRIFT_CV_INPUT].getVoltage();
    float drift = rack::math::clamp(driftKnob + driftAtten * driftCV / 10.f, 0.f, 1.f);

    if (drift >= 0.001f) {
        float driftAmount = progressiveCurve(drift);
        float dt = args.sampleTime;
        float combinedOU = 0.f;

        for (int i = 0; i < NUM_OU_LAYERS; i++) {
            float noise = normalDist(rng);
            ouLayers[i].state += ouLayers[i].theta * (0.f - ouLayers[i].state) * dt
                               + ouLayers[i].sigma * sqrtSampleTime * noise;
            combinedOU += ouLayers[i].state * ouLayers[i].weight;
        }

        // Apply drift: 7.5% max frequency deviation at full drift
        float driftScale = driftAmount * 0.075f;
        deltaPhase *= (1.0 + (double)(driftScale * combinedOU));
    }

    phase += deltaPhase;
    if (phase >= 1.0) phase -= 1.0;

    // ... rest of process() unchanged ...
}
```

### Per-Module RNG Initialization
```cpp
// In struct AnalogLFO:
rack::random::Xoroshiro128Plus rng;
std::normal_distribution<float> normalDist{0.f, 1.f};

// In constructor:
AnalogLFO() {
    // ... existing config calls ...

    // Seed per-module RNG from hardware entropy
    std::random_device rd;
    rng.seed(rd(), rd());
}
```

### Drift Attenuator Param Configuration
```cpp
// In ParamId enum:
DRIFT_ATTEN_PARAM,  // Add before PARAMS_LEN
// PARAMS_LEN now becomes 7

// In constructor:
configParam(DRIFT_ATTEN_PARAM, 0.f, 1.f, 0.f, "Drift CV", "%", 0.f, 100.f);
```

### Bottom Row Layout (7 Components, Claude's Discretion)
```cpp
// Panel usable width: ~57mm (margins 2mm each side of 60.96mm panel)
// Grouped pairs with tight pair spacing, wider gaps between pairs:
//
// [MATrim(7) MCV(15)] [CATrim(23) CCV(31)] [DATrim(39) DCV(47)] [OUT(55)]
//
// Within pair: ~8mm center-to-center (trimpot ~6mm + jack ~8mm, no overlap)
// Between pairs: ~8mm gap
// Final OUT: ~8mm from DCV
//
// Alternative tighter spacing:
// [MATrim(6) MCV(14)] [CATrim(22) CCV(30)] [DATrim(38) DCV(46)] [OUT(55)]

// Widget constructor additions:
addParam(createParamCentered<Trimpot>(mm2px(Vec(7.0, 104.0)), module, AnalogLFO::MORPH_ATTEN_PARAM));
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.0, 104.0)), module, AnalogLFO::MORPH_CV_INPUT));
addParam(createParamCentered<Trimpot>(mm2px(Vec(23.0, 104.0)), module, AnalogLFO::CHARACTER_ATTEN_PARAM));
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.0, 104.0)), module, AnalogLFO::CHARACTER_CV_INPUT));
addParam(createParamCentered<Trimpot>(mm2px(Vec(39.0, 104.0)), module, AnalogLFO::DRIFT_ATTEN_PARAM));
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(47.0, 104.0)), module, AnalogLFO::DRIFT_CV_INPUT));
addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(55.0, 104.0)), module, AnalogLFO::OUTPUT));
```

### Subtle Dot Instability Visual (Claude's Discretion)
```cpp
// Recommended approach: trail jitter + glow variation at high drift
// Read drift value from module (need to expose or pass drift level to display)
// Apply small random offset to trail positions proportional to drift

// In drawPhaseDot():
float driftLevel = 0.f;
if (module) {
    driftLevel = module->params[AnalogLFO::DRIFT_PARAM].getValue();
    // Could also read actual combined drift amount for more accuracy
}

// Trail jitter: add small random offset to trail Y positions
// Scale with drift level so zero drift = perfect trails
if (driftLevel > 0.01f) {
    float jitterAmount = driftLevel * 0.3f;  // pixels of Y jitter at full drift
    // Use frame-to-frame variation (breathePhase already provides oscillation)
    float trailJitter = jitterAmount * std::sin(breathePhase * 3.7f + (float)i * 1.3f);
    ty += trailJitter;
}

// Glow variation: modulate halo radius slightly with drift
float haloJitter = 1.f + driftLevel * 0.15f * std::sin(breathePhase * 2.3f);
float haloRadius = dotRadius * 3.f * haloJitter;
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Random walk (unbounded) | Ornstein-Uhlenbeck (mean-reverting) | Standard since ~2010s DSP literature | Bounded variance, natural sound, no runaway pitch |
| Single timescale drift | Multi-timescale layered OU | VCV Fundamental VCO pattern | More naturalistic: slow wander + fast jitter simultaneously |
| Global random state | Per-module RNG instances | VCV Rack community best practice | Independent drift per module instance |
| rack::random (thread-local) | rack::random (global, v2.2.0+) | VCV Rack 2.2.0 | Per-module Xoroshiro128Plus recommended for independent sequences |

**VCV Rack Fundamental VCO reference:** Uses a leaky integrator (first-order lowpass) of Gaussian noise, which is mathematically equivalent to a single OU process. Our approach extends this with four timescales for richer character.

## Open Questions

1. **Optimal drift scaling factor**
   - What we know: User specified 5-10% of base frequency at full drift. 7.5% center is recommended.
   - What's unclear: Whether 7.5% sounds right at all LFO rates (0.01-20Hz). At very low rates, 7.5% pitch variation may be imperceptible; at high rates it may be too obvious.
   - Recommendation: Start with 7.5%, verify by ear. The scaling factor is a single constant that's easy to tune during verification.

2. **sqrt(dt) caching strategy**
   - What we know: Computing sqrt(sampleTime) per-sample is wasteful.
   - What's unclear: Whether to use onSampleRateChange() callback or lazy initialization.
   - Recommendation: Compute in constructor and update in onSampleRateChange(). Simple and correct.

3. **Display drift level communication**
   - What we know: The display widget needs to read the drift level for dot instability visual.
   - What's unclear: Whether to read the raw knob value, the CV-processed value, or the actual OU output.
   - Recommendation: Read the raw drift knob param value (simple, no threading concerns, good enough for visual scaling). The display already reads params directly for other purposes.

## Sources

### Primary (HIGH confidence)
- VCV Rack SDK 2.6.6 headers: `random.hpp`, `engine/Module.hpp` -- verified Xoroshiro128Plus API, Module::id field, normal distribution pattern
- VCV Rack API docs (vcvrack.com/docs-v2) -- rack::random namespace, Xoroshiro128Plus struct
- NESTML OU noise tutorial (nestml.readthedocs.io) -- exact OU discrete-time update formula verified
- FSU C++ OU implementation (people.sc.fsu.edu) -- Euler-Maruyama formula: `dx = theta*(mu-x)*dt + sigma*dW`

### Secondary (MEDIUM confidence)
- VCV Community discussion on rack::random usage -- per-module RNG best practice confirmed by multiple developers
- KVR Audio forum on emulating pitch drift -- leaky integrator approach confirmed as standard
- VCV Rack Fundamental VCO -- uses leaky integrator of gaussian noise (equivalent to single OU process)

### Tertiary (LOW confidence)
- Component dimensions (Trimpot ~6mm, PJ301MPort ~8mm) -- from general knowledge, not verified against SVG sources in this session

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - VCV Rack SDK headers directly verified, API confirmed
- Architecture (OU process): HIGH - mathematical foundation well-established, discrete-time formula verified from multiple sources
- Architecture (bottom row layout): MEDIUM - component dimensions from general knowledge; spacing needs visual verification
- OU parameters: MEDIUM - mathematically derived but audio perception needs tuning by ear
- Pitfalls: HIGH - based on OU process mathematics and established VCV Rack patterns
- Display visual: MEDIUM - approach is sound but exact visual parameters need tuning

**Research date:** 2026-03-07
**Valid until:** 2026-04-07 (stable domain -- OU process math and VCV Rack SDK are mature)
