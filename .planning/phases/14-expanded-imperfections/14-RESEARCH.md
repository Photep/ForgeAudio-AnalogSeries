# Phase 14: Expanded Imperfections - Research

**Researched:** 2026-03-16
**Domain:** DSP -- analog oscillator imperfection modeling (phase jitter, DC offset wander, pitch slew, component spread)
**Confidence:** HIGH

## Summary

Phase 14 adds four distinct analog imperfections to the existing Drift knob: phase jitter (CHAR-01), DC offset wander (CHAR-02), pitch slew (CHAR-03), and component spread (CHAR-04). The codebase already contains the complete infrastructure for all four: per-module Xoroshiro128Plus RNG, normal distribution sampler, four-layer Ornstein-Uhlenbeck drift engine, TExponentialFilter-based frequency slew, progressive x^2 curve, and relaxed atomics for display bridging. No new libraries or external dependencies are needed.

The critical design distinction is that three imperfections (jitter, DC offset, pitch slew) are Drift-scaled and completely inactive at Drift=0, while component spread is always active -- representing permanent per-instance factory tolerance. Component spread requires serialization via `dataToJson()`/`dataFromJson()` to persist its RNG seed across patch saves, which is new to this module (currently no custom serialization exists). All other imperfections regenerate fresh random state on load, consistent with the existing "no OU serialization" decision.

**Primary recommendation:** Implement as four independent additions to `process()` and the constructor, each at a well-defined integration point already identified in CONTEXT.md. The pitch slew filter MUST be a separate TExponentialFilter instance layered after the existing `freqSlew` -- do not modify the existing filter's behavior.

<user_constraints>

## User Constraints (from CONTEXT.md)

### Locked Decisions
- **Phase jitter (CHAR-01):** Per-sample random phase deviation from new independent white noise source (not OU layers). Jitter is fast and uncorrelated. Authority reduced in clocked mode (2% vs 7.5%). Completely inactive at Drift=0.
- **DC offset wander (CHAR-02):** ~50-100mV maximum wander at full drift. Very slow timescale (~0.02-0.05Hz). New slow OU layer dedicated to DC offset (separate from pitch drift layers). Continuous wander that does NOT reset on clock phase resets. Completely inactive at Drift=0.
- **Pitch slew (CHAR-03):** 200-500ms time constant at full drift. Applies to ALL frequency changes (Rate knob, CV, clock tempo). Separate drift-controlled slew filter layered AFTER existing freqSlew. Completely bypassed at Drift=0.
- **Component spread (CHAR-04):** ~1-3% parameter offsets. Affects OU layer weights, character curve response, waveform shape coefficients. Serialized RNG seed persists across saves/loads. Always active regardless of Drift setting.

### Claude's Discretion
- Exact jitter magnitude scaling curve within the "subtle trace thickening" constraint
- DC offset OU layer parameters (exact theta/sigma for 0.02-0.05Hz timescale)
- Exact pitch slew time constant within the 200-500ms range
- Which specific waveform shape coefficients get component spread offsets
- Component spread RNG seed generation and serialization format
- Empirical tuning of all magnitude values

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope

</user_constraints>

<phase_requirements>

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CHAR-01 | Phase jitter adds per-sample random phase deviation scaled by Drift knob | White noise via normalDist(rng), applied to deltaPhase at line 508. Existing progressive curve and clocked authority scaling reusable. |
| CHAR-02 | DC offset drift adds slow-wandering output bias scaled by Drift knob | New 5th OU layer with theta ~0.125-0.314 (0.02-0.05Hz). Applied to outputVoltage at line 584. |
| CHAR-03 | Pitch slew adds frequency lag (component thermal response) scaled by Drift knob | New TExponentialFilter with drift-dependent lambda, placed between existing freqSlew (line 488) and FM processing (line 493). |
| CHAR-04 | Component spread applies per-instance random parameter offsets (serialized via RNG seed) | Xoroshiro128Plus state[2] serialized as two uint64_t via json_integer. Offsets generated in constructor, applied as member variable modifiers. |

</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack 2 SDK | 2.x | Module framework, DSP primitives | Project platform -- already in use |
| rack::dsp::TExponentialFilter | SDK built-in | Pitch slew filter (new instance) | Already used for freqSlew; well-understood one-pole exponential smoother |
| rack::random::Xoroshiro128Plus | SDK built-in | RNG for jitter noise and component spread seed | Already used as per-module RNG; state[2] array enables serialization |
| std::normal_distribution | C++17 stdlib | Gaussian noise for jitter sampling | Already instantiated as normalDist member |
| Jansson (json_t) | SDK bundled | Component spread seed serialization | VCV Rack's standard JSON library for dataToJson/dataFromJson |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| rack::math::clamp | SDK built-in | Parameter range enforcement | All drift-scaled calculations |
| std::random_device | C++17 stdlib | Component spread seed generation | Constructor only, for initial seed if no saved state |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| New OU layer for DC offset | Reuse existing ouLayers[0] (slowest) | CONTEXT.md explicitly requires a SEPARATE dedicated OU layer -- reusing would couple DC offset to pitch drift behavior |
| Modifying existing freqSlew lambda | Separate TExponentialFilter | CONTEXT.md explicitly requires a SEPARATE filter -- modifying freqSlew would break mode-transition slew at Drift=0 |
| Per-cycle jitter (on phase wrap) | Per-sample jitter | CONTEXT.md specifies per-sample; prior FEATURES.md suggested per-cycle but user session overrode this |

**Installation:**
No new dependencies. All components from existing VCV Rack 2 SDK and C++17 standard library.

## Architecture Patterns

### Integration Point Map
```
process() execution order (AnalogLFO.cpp):
  |
  |- processClockInput()          [existing, line 466]
  |- processResetInput()          [existing, line 467]
  |
  |- targetFreq calculation       [existing, lines 470-485]
  |- freqSlew.process()           [existing, line 488] -- mode transition slew (always on)
  +- NEW: driftSlew.process()     [CHAR-03] -- thermal pitch slew (Drift-gated)
  |- FM processing                [existing, lines 493-501]
  |
  |- deltaPhase calculation       [existing, line 508]
  |- OU drift processing          [existing, lines 517-533]
  +- NEW: phase jitter            [CHAR-01] -- added to deltaPhase after OU drift
  |- phase accumulation           [existing, line 535]
  |
  |- morph/character computation  [existing, lines 539-548]
  |- computeMorphedWave()         [existing, line 578] -- component spread affects coefficients
  |
  |- outputVoltage = 5V * sample  [existing, line 584]
  +- NEW: DC offset wander        [CHAR-02] -- added to outputVoltage
  |- crossfade processing         [existing, lines 587-595]
  |- output                       [existing, line 601]

Constructor:
  |- existing init                [existing]
  +- NEW: component spread init   [CHAR-04] -- generate offsets from RNG
  +- NEW: driftSlew init          [CHAR-03] -- initialize filter
  +- NEW: DC offset OU layer      [CHAR-02] -- initialize 5th OU layer
```

### Pattern 1: Drift-Gated Imperfection
**What:** Each new imperfection follows the same pattern: compute raw effect, scale by progressive curve, apply clocked authority scaling, gate by drift > threshold.
**When to use:** CHAR-01, CHAR-02, CHAR-03 (all drift-gated features)
**Example:**
```cpp
// Source: Existing pattern at AnalogLFO.cpp:517-533
if (drift >= 0.001f) {
    float driftAmount = progressiveCurve(drift);
    float authority = isClocked ? CLOCKED_AUTHORITY : FREE_AUTHORITY;
    float effectMagnitude = driftAmount * authority * RAW_EFFECT_VALUE;
    // Apply effect...
}
```

### Pattern 2: Component Spread (Always-Active, Serialized)
**What:** Per-instance offsets generated once from a deterministic RNG seed, persisted via JSON serialization, applied as fixed modifiers regardless of Drift setting.
**When to use:** CHAR-04
**Example:**
```cpp
// Source: VCV Rack Plugin Guide (https://vcvrack.com/manual/PluginGuide)
// Constructor: generate offsets
uint64_t spreadSeed0, spreadSeed1;
// Use a separate RNG seeded deterministically to generate spread offsets
rack::random::Xoroshiro128Plus spreadRng;
spreadRng.seed(spreadSeed0, spreadSeed1);
std::normal_distribution<float> spreadDist{0.f, 1.f};
float ouWeightSpread[NUM_OU_LAYERS]; // per-layer offsets

// Serialization:
json_t* dataToJson() override {
    json_t* rootJ = json_object();
    json_object_set_new(rootJ, "spreadSeed0", json_integer(spreadSeed0));
    json_object_set_new(rootJ, "spreadSeed1", json_integer(spreadSeed1));
    return rootJ;
}
```

### Pattern 3: Separate OU Layer for DC Offset
**What:** A dedicated Ornstein-Uhlenbeck layer with very slow timescale, independent from pitch drift layers, providing continuous wander unaffected by phase resets.
**When to use:** CHAR-02
**Example:**
```cpp
// Source: Existing OU pattern at AnalogLFO.cpp:523-528, adapted for DC offset
struct OULayer dcOffsetOU;  // separate from ouLayers[4]
// theta for 0.03Hz: 2*PI*0.03 = 0.188
// sigma chosen for stationary std = sigma/sqrt(2*theta)
// Target: ~1.0 stationary std -> sigma = sqrt(2*0.188) * 1.0 = 0.614
dcOffsetOU.theta = 2.f * M_PI * 0.03f;
dcOffsetOU.sigma = 0.614f;
dcOffsetOU.weight = 1.f;  // single layer, no weighting needed
```

### Anti-Patterns to Avoid
- **Modifying existing freqSlew behavior:** The existing filter handles mode transitions (always on, lambda=20). Drift slew is a SEPARATE concern -- use a second TExponentialFilter instance.
- **Coupling DC offset to pitch drift OU layers:** DC offset has a fundamentally different timescale and purpose. Tapping existing layers would create unwanted correlation between pitch instability and voltage offset.
- **Resetting DC offset on clock phase reset:** CONTEXT.md explicitly states DC offset "does NOT reset on clock phase resets" -- it's a component-level phenomenon, not a waveform-level one.
- **Making component spread Drift-dependent:** Component spread represents physical tolerance, not variable instability. It MUST be always active, including at Drift=0.
- **Serializing OU state:** The project has an explicit decision to NOT serialize OU state (fresh randomness on load). Only the component spread seed gets serialized.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Exponential smoothing filter | Custom one-pole lowpass | `dsp::TExponentialFilter<float>` | Already proven in freqSlew; has `setTau()` for time constant, handles sample rate automatically via deltaTime parameter |
| Gaussian noise generation | Custom Box-Muller or ziggurat | `std::normal_distribution<float>` with existing `rng` | Already instantiated and proven; thread-safe per-module |
| JSON serialization | Custom binary format | Jansson `json_integer`/`json_object_set_new` | VCV Rack's standard; handles patch save/load lifecycle automatically |
| Mean-reverting random walk | Custom random walk with manual bounds | Ornstein-Uhlenbeck process (existing pattern) | Statistically well-defined stationary distribution; existing 4-layer implementation proves the pattern |

**Key insight:** Every DSP primitive needed for this phase already exists either as a VCV Rack SDK utility or as an established pattern in the codebase. The work is purely composition -- connecting existing tools at the right integration points with the right scaling parameters.

## Common Pitfalls

### Pitfall 1: Drift Slew Interferes with Mode Transition Slew
**What goes wrong:** If you modify the existing `freqSlew` lambda based on Drift instead of adding a separate filter, mode transitions (free<->clocked) will be sluggish at high Drift and the module will feel broken at Drift=0 if the base lambda is altered.
**Why it happens:** The existing freqSlew (lambda=20, ~50ms) is tuned specifically for smooth mode transitions and must remain constant.
**How to avoid:** Add a NEW `dsp::TExponentialFilter<float> driftSlew` member. Place it in the signal chain AFTER `freqSlew.process()` and BEFORE FM processing. At Drift=0, set its lambda very high (>200) so it passes through instantly.
**Warning signs:** Mode transitions sound glitchy; frequency response feels different at Drift=0 vs pre-v1.2.

### Pitfall 2: Phase Jitter Amplitude Too Large
**What goes wrong:** Per-sample phase jitter that is too aggressive creates audible pitch modulation rather than subtle "trace thickening." Users perceive it as broken rather than analog.
**Why it happens:** Jitter magnitude is multiplicative on deltaPhase. At high LFO rates (20Hz), even small phase deviation percentages produce more absolute timing variation than at low rates.
**How to avoid:** Scale jitter conservatively. Start with ~0.1-0.3% of deltaPhase at full drift (free-running authority). The display should show subtle trace thickening, not visible wobble. Use the same authority scaling as pitch drift (0.02 clocked, 0.075 free).
**Warning signs:** Waveform trace looks obviously wobbly on display; two adjacent cycles are visibly different in length.

### Pitfall 3: DC Offset Affects Crossfade
**What goes wrong:** If DC offset is added before the crossfade logic (line 587), phase resets will capture a DC-shifted voltage as `crossfadeFrom`, causing a small click or pop during the crossfade.
**Why it happens:** The crossfade smooths between the pre-reset and post-reset output. If the DC offset is part of the captured value, the crossfade transitions between two different DC levels.
**How to avoid:** Add DC offset AFTER the crossfade processing (after line 595), just before `outputs[OUTPUT].setVoltage()`. This way the crossfade operates on the pure waveform, and DC offset is applied uniformly to the final output.
**Warning signs:** Faint clicks on phase reset when Drift is high and DC offset is away from zero.

### Pitfall 4: Component Spread Seed Lost on Module Reset
**What goes wrong:** If `onReset()` regenerates the component spread seed, the user's module loses its unique personality when they hit Initialize. This contradicts the "permanent factory tolerance" concept.
**Why it happens:** Default onReset() behavior resets parameters. If spread seed regeneration is triggered by onReset(), the module becomes a different "unit."
**How to avoid:** Do NOT regenerate the spread seed in onReset(). The seed is set once (in constructor for new modules, or from JSON for loaded modules) and persists for the module's lifetime. Only a new module instance gets a new seed.
**Warning signs:** Module character changes when user clicks Initialize; two save/load cycles produce different output.

### Pitfall 5: TExponentialFilter Lambda at Drift=0 Must Be Very High
**What goes wrong:** If the drift slew filter's lambda is merely "high" (e.g., 20-50) at Drift=0, there will be a noticeable lag on frequency changes even when the user expects digital precision.
**Why it happens:** TExponentialFilter with lambda=50 still has a 20ms time constant. Users at Drift=0 expect ZERO lag.
**How to avoid:** At Drift=0, set lambda to a very large value (500+) or bypass the filter entirely. The `setTau()` convenience method maps tau -> lambda as `lambda = 1/tau`, so a 0.002s tau gives lambda=500.
**Warning signs:** At Drift=0, rapid Rate knob changes feel slightly sluggish compared to v1.1.

### Pitfall 6: uint64_t JSON Serialization Precision
**What goes wrong:** Jansson's `json_integer()` stores values as `json_int_t` (typically `int64_t`). Xoroshiro128Plus state is `uint64_t`. Values above `INT64_MAX` (2^63-1) will overflow when cast to signed.
**Why it happens:** JSON has no unsigned 64-bit integer type. The Jansson library uses signed `json_int_t`.
**How to avoid:** Store as two separate values per seed word, OR accept the sign bit reinterpretation (cast back to uint64_t on load -- the bit pattern is preserved as long as the value round-trips through the same type width). The safest approach: store as hex string using `json_string()` and `json_string_value()`, then parse with `std::stoull(str, nullptr, 16)`.
**Warning signs:** Component spread changes after save/load cycle for some module instances.

## Code Examples

### CHAR-01: Phase Jitter Implementation
```cpp
// Source: Pattern from AnalogLFO.cpp:517-533, adapted for jitter
// Integration point: after OU drift modifies deltaPhase (line 532), before phase accumulation

// In the drift >= 0.001f block, after OU drift calculation:
// Phase jitter: independent white noise on deltaPhase
float jitterNoise = normalDist(rng);  // reuse existing RNG + distribution
float jitterAuthority = isClocked ? 0.02f : 0.075f;  // same scaling as pitch drift
float jitterScale = driftAmount * jitterAuthority * 0.003f;  // ~0.3% max deviation
deltaPhase *= (1.0 + (double)(jitterScale * jitterNoise));
```

### CHAR-02: DC Offset Wander Implementation
```cpp
// Source: OU pattern from AnalogLFO.cpp:523-528, adapted for very slow wander
// New member variables:
OULayer dcOffsetOU;
// Initialize in constructor:
dcOffsetOU.theta = 2.f * (float)M_PI * 0.03f;   // 0.03Hz center frequency (~33s wander cycle)
dcOffsetOU.sigma = 0.614f;                         // stationary std ~1.0
dcOffsetOU.weight = 1.f;
dcOffsetOU.state = 0.f;

// In process(), in the drift >= 0.001f block:
float dcNoise = normalDist(rng);
dcOffsetOU.state += dcOffsetOU.theta * (0.f - dcOffsetOU.state) * args.sampleTime
                  + dcOffsetOU.sigma * sqrtSampleTime * dcNoise;
float dcOffsetAuthority = isClocked ? 0.02f : 0.075f;
float dcOffsetV = driftAmount * dcOffsetAuthority * dcOffsetOU.state * 0.1f;
// Results in ~50-100mV max wander at full drift in free-running mode

// Applied AFTER crossfade, before output:
outputVoltage += dcOffsetV;  // or store and apply after crossfade block
```

### CHAR-03: Pitch Slew Implementation
```cpp
// Source: TExponentialFilter API (https://vcvrack.com/docs-v2/filter_8hpp_source)
// New member variable:
dsp::TExponentialFilter<float> driftSlew;

// In constructor:
driftSlew.setLambda(500.f);  // effectively instant at init
driftSlew.out = 0.7f;        // match default Rate

// In process(), between freqSlew and FM processing:
if (drift >= 0.001f) {
    float driftAmount = progressiveCurve(drift);
    // 200-500ms time constant: tau = 0.2 at full drift, tau = 0.002 at drift=0
    // lambda = 1/tau, so lambda ranges from 500 (instant) to ~3 (300ms slew)
    float slewTau = 0.002f + driftAmount * 0.298f;  // 2ms to 300ms
    driftSlew.setLambda(1.f / slewTau);
} else {
    driftSlew.setLambda(500.f);  // bypass: effectively instant
}
freq = driftSlew.process(args.sampleTime, freq);
freq = std::fmax(freq, 0.001f);
```

### CHAR-04: Component Spread with Serialization
```cpp
// Source: VCV Rack Plugin Guide (https://vcvrack.com/manual/PluginGuide)
//         Xoroshiro128Plus API (https://vcvrack.com/docs-v2/structrack_1_1random_1_1Xoroshiro128Plus)

// New member variables:
uint64_t spreadSeed[2] = {};  // persisted seed for reproducible spread
float ouWeightSpread[NUM_OU_LAYERS] = {};  // OU layer weight offsets
float characterSpread = 0.f;               // character curve offset
float sawCurvatureSpread = 0.f;            // saw exponential ramp spread
float squareDutySpread = 0.f;              // square duty cycle spread
float triAsymmetrySpread = 0.f;            // triangle asymmetry spread

void initComponentSpread() {
    rack::random::Xoroshiro128Plus spreadRng;
    spreadRng.seed(spreadSeed[0], spreadSeed[1]);
    std::normal_distribution<float> d{0.f, 1.f};
    // OU layer weight offsets: +/-2% per layer
    for (int i = 0; i < NUM_OU_LAYERS; i++) {
        ouWeightSpread[i] = d(spreadRng) * 0.02f;
    }
    // Character response: +/-1.5%
    characterSpread = d(spreadRng) * 0.015f;
    // Waveform shape coefficients: +/-1-3%
    sawCurvatureSpread = d(spreadRng) * 0.02f;
    squareDutySpread = d(spreadRng) * 0.01f;
    triAsymmetrySpread = d(spreadRng) * 0.015f;
}

// In constructor (after rng.seed()):
spreadSeed[0] = rng();  // generate from per-module RNG
spreadSeed[1] = rng();
initComponentSpread();

// Serialization:
json_t* dataToJson() override {
    json_t* rootJ = json_object();
    // Store as hex strings to avoid uint64_t -> int64_t sign issues
    char buf[32];
    snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)spreadSeed[0]);
    json_object_set_new(rootJ, "spreadSeed0", json_string(buf));
    snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)spreadSeed[1]);
    json_object_set_new(rootJ, "spreadSeed1", json_string(buf));
    return rootJ;
}

void dataFromJson(json_t* rootJ) override {
    json_t* s0J = json_object_get(rootJ, "spreadSeed0");
    json_t* s1J = json_object_get(rootJ, "spreadSeed1");
    if (s0J && s1J) {
        spreadSeed[0] = std::stoull(json_string_value(s0J), nullptr, 16);
        spreadSeed[1] = std::stoull(json_string_value(s1J), nullptr, 16);
        initComponentSpread();  // regenerate deterministic offsets from restored seed
    }
}
```

### Component Spread Application Points
```cpp
// In computeTriangle(): add triAsymmetrySpread to asymmetry
float asymmetry = c * (0.10f + triAsymmetrySpread);

// In computeSaw(): add sawCurvatureSpread to curvature blend factor
float curvedSaw = saw + c * (0.5f + sawCurvatureSpread) * (expRamp - saw);

// In computeSquare(): add squareDutySpread to duty offset
float duty = 0.5f + c * (0.04f + squareDutySpread);

// In drift block: apply OU weight spread
combinedOU += ouLayers[i].state * (ouLayers[i].weight + ouWeightSpread[i]);

// In character processing: apply character response spread
float character = rack::math::clamp(charKnob + charAtten * charCV / 5.f + characterSpread, 0.f, 1.f);
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Per-cycle jitter (on phase wrap) | Per-sample jitter (continuous) | Phase 14 CONTEXT.md decision | More authentic unstable oscillator character; trace thickening vs step changes |
| Shared OU layers for all drift effects | Dedicated OU layers per phenomenon | Phase 14 CONTEXT.md decision | DC offset decoupled from pitch drift; correct physical modeling |
| Single freqSlew serves all purposes | Separate mode-transition and drift slew filters | Phase 14 CONTEXT.md decision | Mode transitions unaffected by Drift setting |
| No state serialization | Component spread seed serialized | Phase 14 CONTEXT.md decision (CHAR-04) | Persistent module personality; exception to "no OU serialization" rule |

**Deprecated/outdated:**
- FEATURES.md suggested reusing ouLayers[0] for DC offset -- superseded by CONTEXT.md requiring a dedicated OU layer
- FEATURES.md suggested per-cycle jitter -- superseded by CONTEXT.md requiring per-sample jitter
- ARCHITECTURE.md suggested modifying existing freqSlew lambda -- superseded by CONTEXT.md requiring a separate filter

## Open Questions

1. **Exact DC offset OU parameters**
   - What we know: Timescale should be 0.02-0.05Hz. Max wander ~50-100mV at full drift.
   - What's unclear: Optimal theta and sigma for the desired subjective wander character.
   - Recommendation: Start with 0.03Hz (theta = 2*pi*0.03 = 0.188), sigma = 0.614 (for stationary std ~1.0). Tune empirically -- STATE.md already flags tuning as needed.

2. **Component spread magnitude calibration**
   - What we know: ~1-3% offsets per CONTEXT.md. STATE.md explicitly flags: "Component spread magnitudes need empirical tuning during Phase 14."
   - What's unclear: Exact percentages per coefficient that produce "subtly different" without "obviously broken."
   - Recommendation: Start conservative (1-2% for most parameters, up to 3% for OU weights). Flag empirical tuning as a verification step.

3. **Drift slew bypassing semantics at Drift=0**
   - What we know: Must be "completely bypassed" at Drift=0 per success criterion 5.
   - What's unclear: Whether to use extremely high lambda (500+) or a conditional branch.
   - Recommendation: Use high lambda (500) rather than conditional bypass. This avoids a branch in the hot path and the filter at lambda=500 converges in <2ms, which is effectively instant.

4. **Display jitter visualization for CHAR-01**
   - What we know: Success criterion 1 says "visible as slight trace thickness variation on the display." Current display uses a 256-sample buffer that shows ONE static cycle.
   - What's unclear: The display buffer is recomputed via `updateDisplayBuffer()` which calls `computeMorphedWave()` with clean phase values. Per-sample jitter affects the live output but NOT the display buffer.
   - Recommendation: Jitter trace thickening should be handled by the existing `displayDrift` visual effects (dot jitter, trail jitter) in `drawPhaseDot()`. The waveform trace itself shows the clean shape; the dot and trail show the instability. This matches the CONTEXT.md statement "Display shows subtle trace thickening at full drift, not obvious wobble." The dot trail naturally provides this effect.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Manual verification via VCV Rack runtime |
| Config file | none -- VCV Rack modules tested by running the module |
| Quick run command | `make && make install` then launch VCV Rack |
| Full suite command | `make && make install` then verify all success criteria in VCV Rack |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CHAR-01 | Phase jitter visible as trace thickening at Drift>0 | manual-only | Visual inspection in VCV Rack | N/A |
| CHAR-02 | DC offset wander observable on scope module | manual-only | Patch LFO output to scope, observe center drift | N/A |
| CHAR-03 | Frequency change lag with Drift up | manual-only | Turn Rate knob rapidly, observe settling time | N/A |
| CHAR-04 | Two identical instances produce different output | manual-only | Two AnalogLFOs side-by-side, compare on scope | N/A |
| SC-5 | Drift=0 matches pre-v1.2 precision | manual-only | Compare output with and without new code paths at Drift=0 | N/A |

**Manual-only justification:** VCV Rack DSP modules are tested by running in the host application. The module has no unit test framework. All behaviors are perceptual (visual/auditory) and require the module running in context.

### Sampling Rate
- **Per task commit:** `make && make install` -- verify module loads without crash
- **Per wave merge:** Full manual verification of all success criteria
- **Phase gate:** All 5 success criteria verified before `/gsd:verify-work`

### Wave 0 Gaps
None -- no test infrastructure needed beyond existing build system.

## Sources

### Primary (HIGH confidence)
- VCV Rack API: `rack::dsp::TExponentialFilter` -- [filter.hpp source](https://vcvrack.com/docs-v2/filter_8hpp_source) -- `setTau()`, `setLambda()`, `process(deltaTime, in)` verified
- VCV Rack API: `rack::random::Xoroshiro128Plus` -- [struct reference](https://vcvrack.com/docs-v2/structrack_1_1random_1_1Xoroshiro128Plus) -- `state[2]` public member, `seed(s0, s1)` method verified
- VCV Rack API: `rack::engine::Module` -- [struct reference](https://vcvrack.com/docs-v2/structrack_1_1engine_1_1Module) -- `dataToJson()`/`dataFromJson()` signatures verified
- VCV Rack Plugin Guide -- [PluginGuide](https://vcvrack.com/manual/PluginGuide) -- JSON serialization examples with `json_integer`, `json_object_set_new`
- Existing codebase: `AnalogLFO.cpp` -- all integration points, OU layer pattern, drift authority scaling verified by direct code reading

### Secondary (MEDIUM confidence)
- [Analog Devices AN-1067](https://www.analog.com/en/resources/app-notes/an-1067.html) -- Phase noise to jitter conversion theory
- [CCRMA Bad Circuit Modelling: Tolerances](https://ccrma.stanford.edu/~jatin/Bad-Circuit-Modelling/Tolerances.html) -- Component tolerance modeling approaches (page timed out but referenced)
- [Musicdsp.org: 1-pole LPF](https://www.musicdsp.org/en/latest/Filters/257-1-pole-lpf-for-smooth-parameter-changes.html) -- One-pole filter coefficient calculation
- Prior research documents: `.planning/research/FEATURES.md`, `.planning/research/ARCHITECTURE.md` -- domain analysis from v1.2 planning

### Tertiary (LOW confidence)
- Ornstein-Uhlenbeck sigma/theta relationship for specific timescales -- derived from mathematical properties (sigma = sqrt(2*theta) for unit stationary variance), needs empirical tuning confirmation

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all components already exist in SDK/codebase; no new dependencies
- Architecture: HIGH -- integration points explicitly identified in CONTEXT.md code_context section; verified against current source
- Pitfalls: HIGH -- derived from direct code analysis and established DSP knowledge; serialization pitfall verified against Jansson API
- OU parameters: MEDIUM -- mathematical derivation correct but optimal subjective values need empirical tuning (acknowledged in STATE.md)
- Component spread magnitudes: MEDIUM -- range is constrained (1-3%) but exact per-coefficient values need tuning

**Research date:** 2026-03-16
**Valid until:** 2026-04-16 (stable domain -- VCV Rack SDK and DSP primitives change infrequently)
