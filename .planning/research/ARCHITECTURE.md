# Architecture Research: Clock Sync Integration

**Domain:** Clock-synced LFO in VCV Rack 2
**Researched:** 2026-03-07
**Confidence:** HIGH (VCV Rack SDK patterns well-documented; clock sync is a solved problem in the module ecosystem; existing codebase is small and well-understood)

## Current Architecture Snapshot

The existing AnalogLFO is a single 538-line file (`src/AnalogLFO.cpp`) containing three components:

1. **`AnalogLFO` (Module struct)** -- Phase accumulator, waveform generation, drift engine, display buffer management. All DSP in `process()` at sample rate.
2. **`WaveformDisplay` (Widget)** -- NanoVG rendering of waveform trace + phase dot with glow effects. Reads from double buffer.
3. **`AnalogLFOWidget` (ModuleWidget)** -- Panel layout: knobs, jacks, screws, display placement.

### Current Data Flow (Per Sample)

```
process():
  1. Read RATE_PARAM -> freq (Hz)
  2. Compute deltaPhase = freq * sampleTime
  3. Read drift params + CV -> drift amount
  4. If drift > 0: run OU layers, modulate deltaPhase
  5. phase += deltaPhase; wrap to [0, 1)
  6. Read morph/character params + CV
  7. Update display buffer on phase wrap or param change
  8. computeMorphedWave(phase, morph, character) -> sample
  9. Output: sample * 5V
```

### Current Enum Layout

```cpp
enum ParamId {
    MORPH_PARAM, CHARACTER_PARAM, DRIFT_PARAM, RATE_PARAM,
    MORPH_ATTEN_PARAM, CHARACTER_ATTEN_PARAM, DRIFT_ATTEN_PARAM,
    PARAMS_LEN  // = 7
};
enum InputId {
    MORPH_CV_INPUT, DRIFT_CV_INPUT, CHARACTER_CV_INPUT,
    INPUTS_LEN  // = 3
};
enum OutputId { OUTPUT, OUTPUTS_LEN };
enum LightId  { LIGHTS_LEN };
```

### Current Panel Layout (12HP = 60.96mm)

```
y=15-42mm   Waveform display (57x27mm, 2mm side margins)
y=54mm      MORPH knob (center, RoundBigBlackKnob)
y=69mm      CHARACTER knob (x=18mm) + DRIFT knob (x=42.96mm)
y=86mm      RATE knob (x=18mm, RoundBlackKnob)
            -- empty space at x=42.96, y=86mm --
y=92mm      Section divider line
y=96mm      Trimpots: Morph(x=10) Char(x=24) Drift(x=38)
y=108mm     Jacks: MorphCV(x=10) CharCV(x=24) DriftCV(x=38) OUT(x=52)
```

## Clock Sync Integration Architecture

### System Overview

```
                          CLK INPUT (new)
                              |
                              v
                   +--------------------+
                   |  Clock Tracker     | (new component)
                   |                    |
                   |  SchmittTrigger    |
                   |  Period measurement|
                   |  Period smoothing  |
                   |  Validity tracking |
                   +--------+-----------+
                            |
                            | clockPeriod (seconds)
                            | clockValid (bool)
                            v
    +--------------------------------------------------+
    |              process() (modified)                  |
    |                                                    |
    |  if (clockValid):                                  |
    |    freq = divisionRatio / clockPeriod              |
    |    phase reset on clock edge                       |
    |  else:                                             |
    |    freq = RATE_PARAM (existing free-run)           |
    |                                                    |
    |  deltaPhase = freq * sampleTime                   |
    |  ... drift, morph, character unchanged ...         |
    +--------------------------------------------------+
                            |
                            v
                   +--------------------+
                   |  WaveformDisplay   | (modified)
                   |                    |
                   |  + sync badge      | (new overlay)
                   |  + division label  | (new overlay)
                   +--------------------+
```

### New Components

#### 1. Clock Tracker (inline in AnalogLFO struct)

This is NOT a separate class -- it is a set of member variables and a helper method on the `AnalogLFO` struct. Introducing a separate class for ~30 lines of logic would be over-engineering given the existing monolithic style.

**New member variables:**

```cpp
// Clock tracking state
rack::dsp::SchmittTrigger clockTrigger;
float clockPeriod = 0.f;           // Measured period (seconds)
float clockTimer = 0.f;            // Time since last edge
float smoothedClockPeriod = 0.f;   // EMA-smoothed period
bool clockValid = false;           // Have we seen >= 2 edges?
int clockEdgeCount = 0;            // Edges seen since CLK connected
bool clockWasConnected = false;    // For detecting disconnect
```

**Clock processing logic (called at top of `process()`):**

```cpp
void processClockInput(float sampleTime) {
    bool clkConnected = inputs[CLK_INPUT].isConnected();

    // Handle disconnect: revert to free-run immediately
    if (!clkConnected) {
        if (clockWasConnected) {
            clockValid = false;
            clockEdgeCount = 0;
            clockTimer = 0.f;
            smoothedClockPeriod = 0.f;
        }
        clockWasConnected = false;
        return;
    }
    clockWasConnected = true;

    // Accumulate time
    clockTimer += sampleTime;

    // Edge detection (VCV standard: low=0.1V, high=1.0V)
    float clkVoltage = inputs[CLK_INPUT].getVoltage();
    if (clockTrigger.process(clkVoltage, 0.1f, 1.f)) {
        clockEdgeCount++;

        if (clockEdgeCount >= 2 && clockTimer > 0.001f) {
            // Valid period measurement
            clockPeriod = clockTimer;

            // EMA smoothing (alpha=0.3 for responsive yet stable tracking)
            if (smoothedClockPeriod <= 0.f) {
                smoothedClockPeriod = clockPeriod;  // First valid: snap
            } else {
                smoothedClockPeriod += 0.3f * (clockPeriod - smoothedClockPeriod);
            }
            clockValid = true;
        }
        clockTimer = 0.f;

        // Phase reset on clock edge (snap to 0)
        phase = 0.0;
    }
}
```

**Rationale for inline approach:**
- The existing module is 538 lines with everything in one struct. Clock tracking adds ~50 lines of state and one method. Extracting a class would add indirection without benefit at this scale.
- If/when a VCO module is built, the clock tracker could be refactored into a shared utility -- but that is a v2.0 concern, not a v1.1 concern.

#### 2. Division/Multiplication Ratio

The RATE knob behavior changes when a clock is connected. Instead of mapping to Hz directly, it maps to a division/multiplication table.

**Approach: Discrete ratio table indexed by knob position.**

```cpp
// Division/multiplication ratios (musical values)
// Rate knob [0..1] maps to index in this table
static constexpr int NUM_RATIOS = 13;
static constexpr float RATIOS[NUM_RATIOS] = {
    1.f/8, 1.f/6, 1.f/4, 1.f/3, 1.f/2,
    2.f/3, 1.f,
    3.f/2, 2.f, 3.f, 4.f, 6.f, 8.f
};
// Labels for display
static const char* RATIO_LABELS[NUM_RATIOS] = {
    "/8", "/6", "/4", "/3", "/2",
    "x2/3", "x1",
    "x3/2", "x2", "x3", "x4", "x6", "x8"
};

int getRatioIndex() {
    float knob = params[RATE_PARAM].getValue();
    // Map continuous knob to nearest discrete ratio
    float normalized = (knob - 0.01f) / (20.f - 0.01f);  // Normalize from Rate range
    int idx = (int)(normalized * (NUM_RATIOS - 1) + 0.5f);
    return rack::math::clamp(idx, 0, NUM_RATIOS - 1);
}
```

**Why a discrete table instead of continuous scaling:**
- Musical clock divisions are inherently discrete (1/4, 1/2, x2, x4).
- Continuous scaling between divisions creates non-musical ratios that fight the clock.
- Discrete snapping gives predictable, repeatable behavior -- users can reliably dial in "half time" or "double time."
- The Rate knob's existing range (0.01-20Hz) maps naturally to 13 positions without needing reconfiguration.

**Alternative considered and rejected: Separate division param.** Adding a dedicated switch or encoder would require panel space that does not exist at 12HP without a redesign. Repurposing the Rate knob is the standard Eurorack convention (see: VCV Fundamental LFO, Mutable Instruments Tides).

### Modified Components

#### 1. `AnalogLFO` struct -- process() Changes

The `process()` method changes in two places:

**Frequency computation (lines 218-219 currently):**

```cpp
// BEFORE (v1.0):
float freq = params[RATE_PARAM].getValue();
freq = std::fmax(freq, 0.001f);

// AFTER (v1.1):
processClockInput(args.sampleTime);
float freq;
if (clockValid && smoothedClockPeriod > 0.f) {
    int ratioIdx = getRatioIndex();
    freq = RATIOS[ratioIdx] / smoothedClockPeriod;
} else {
    freq = params[RATE_PARAM].getValue();
}
freq = std::fmax(freq, 0.001f);
```

**Phase reset is handled inside `processClockInput()` (sets `phase = 0.0` on edge).**

Everything after frequency computation -- drift, morph, character, waveform generation, display buffer, output -- remains UNCHANGED.

#### 2. Enum Extensions

```cpp
enum InputId {
    MORPH_CV_INPUT,
    DRIFT_CV_INPUT,
    CHARACTER_CV_INPUT,
    CLK_INPUT,          // NEW
    INPUTS_LEN          // now = 4
};
```

No new params, outputs, or lights needed for the core feature.

#### 3. Constructor Changes

```cpp
// In AnalogLFO() constructor, add:
configInput(CLK_INPUT, "Clock");
```

#### 4. WaveformDisplay -- Sync Indicator Overlay

The display adds two visual elements when clocked:

1. **Sync badge:** Small "SYNC" text or icon in corner of display
2. **Division label:** Current ratio (e.g., "x2", "/4") shown in display

**Implementation approach:** Read new atomic state from the module:

```cpp
// New atomics in AnalogLFO struct:
std::atomic<bool> displayClockValid{false};
std::atomic<int> displayRatioIndex{6};  // Default: x1

// In process(), after clock processing:
displayClockValid.store(clockValid, std::memory_order_relaxed);
if (clockValid) {
    displayRatioIndex.store(getRatioIndex(), std::memory_order_relaxed);
}
```

The display reads these atomics and renders text overlays using NanoVG. This follows the existing pattern of `displayPhase` and `displayDrift` atomics for audio-to-GUI transfer.

**NanoVG text rendering note:** The existing display uses only NanoVG drawing primitives (paths, circles, gradients) -- no text rendering. For the sync badge and division label, there are two options:

- **Option A: NanoVG font rendering.** Use `nvgText()` with a loaded font. Requires bundling a font file and calling `nvgCreateFont()`. Standard in VCV Rack modules.
- **Option B: Path-based text like the SVG panel.** Consistent with existing approach but tedious for runtime text that changes (division labels).

**Recommendation: Option A (NanoVG font).** The division label changes based on knob position, making path-based rendering impractical. Use the built-in VCV Rack font (`asset::system("res/fonts/ShareTechMono-Regular.ttf")`) which is already available and commonly used by other modules for display text.

#### 5. Panel SVG Update

The CLK jack needs a physical location on the panel. Looking at the current layout:

```
y=86mm: RATE knob at x=18mm -- right side (x=42.96) is EMPTY
```

**Place the CLK jack at x=42.96, y=86mm** -- directly across from the Rate knob, in the currently empty space. This creates a natural visual association: Rate knob on left, Clock input on right, both at the same vertical position.

The SVG needs:
- CLK jack hole indicator in the components layer
- "CLK" label above the jack (at ~y=79mm, matching RATE label style)

Widget addition:
```cpp
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.96, 86.0)), module, AnalogLFO::CLK_INPUT));
```

#### 6. Serialization (dataToJson/dataFromJson)

Clock tracking state should NOT be serialized. The clock period is derived from incoming signal -- on patch load, the module will re-acquire the clock within 2 edges (typically <1 second at musical tempos). This matches the existing decision not to serialize OU drift state ("fresh randomness on patch load").

However, the module should serialize **whether it was in clocked mode** so that the Rate knob can display the correct behavior on load. Actually, this is unnecessary -- clocked mode is determined by `inputs[CLK_INPUT].isConnected()`, which VCV Rack handles automatically via cable state in the patch file.

**No serialization changes needed.**

## Data Flow: Clocked vs Free-Run

### Free-Run Mode (CLK disconnected -- existing behavior)

```
RATE_PARAM -> freq (Hz)
  |
  v
deltaPhase = freq * sampleTime
  |
  v
[drift modulation]
  |
  v
phase += deltaPhase; wrap [0,1)
  |
  v
computeMorphedWave(phase, morph, character) -> output
```

### Clocked Mode (CLK connected -- new behavior)

```
CLK_INPUT voltage
  |
  v
SchmittTrigger (0.1V / 1.0V thresholds)
  |
  +--> on rising edge:
  |      measure period since last edge
  |      smooth via EMA (alpha=0.3)
  |      reset phase = 0.0
  |
  v
RATE_PARAM -> ratio index -> RATIOS[idx]
  |
  v
freq = RATIOS[idx] / smoothedClockPeriod
  |
  v
deltaPhase = freq * sampleTime
  |
  v
[drift modulation -- same as free-run]
  |
  v
phase += deltaPhase; wrap [0,1)
  |
  v
computeMorphedWave(phase, morph, character) -> output
```

### Display Data Flow (Modified)

```
Audio Thread                               GUI Thread
-----------                               ----------
processClockInput()
  -> displayClockValid.store()
  -> displayRatioIndex.store()             WaveformDisplay::drawLayer()
                                             -> read displayClockValid
process()                                    -> if synced: draw badge + ratio label
  -> displayPhase.store()                    -> read displayBuffer[readIdx]
  -> displayBuffers[writeIdx] fill           -> render waveform trace
  -> displayReadIdx.store()                  -> render phase dot
```

## Architectural Patterns

### Pattern 1: Dual-Mode Parameter (Rate Knob)

**What:** A single knob that means different things depending on module state (CLK connected vs not).

**When to use:** When panel space is constrained and the two modes are mutually exclusive.

**Implementation:**

```cpp
float freq;
if (clockValid && smoothedClockPeriod > 0.f) {
    // Clocked: Rate knob selects division ratio
    int ratioIdx = getRatioIndex();
    freq = RATIOS[ratioIdx] / smoothedClockPeriod;
} else {
    // Free: Rate knob is direct Hz
    freq = params[RATE_PARAM].getValue();
}
```

**Trade-offs:**
- Pro: No panel redesign, no new params, backward compatible.
- Con: The knob's value display in the tooltip will show Hz even in clocked mode. Consider a custom `ParamQuantity` subclass to show the ratio label instead.

### Pattern 2: EMA Period Smoothing

**What:** Exponential moving average on measured clock periods to reject jitter while tracking tempo changes.

**When to use:** Any clock tracking where the input may have timing jitter (common in VCV Rack due to sample-accurate processing and cable delays).

**Implementation:**

```cpp
// On each clock edge:
smoothedClockPeriod += alpha * (measuredPeriod - smoothedClockPeriod);
```

**Alpha selection:**
- `alpha = 0.3` -- responsive to tempo changes (settles in ~5 edges), smooths single-sample jitter.
- `alpha = 0.1` -- more stable but slow to track tempo changes. Too sluggish for live tempo automation.
- `alpha = 0.5` -- too reactive, passes through jitter to frequency.

**Trade-offs:**
- Pro: Simple, O(1) per edge, no buffer needed.
- Con: Cannot distinguish intentional tempo change from jitter. Alpha is a compromise.

### Pattern 3: Phase Reset on Clock Edge

**What:** Snap the LFO phase to 0.0 on each clock rising edge, ensuring the waveform aligns with the beat.

**When to use:** Always in clocked mode. This is the defining behavior of a clock-synced LFO -- without phase reset, the LFO frequency would track the clock but phase would drift.

**Implementation:**

```cpp
if (clockTrigger.process(clkVoltage, 0.1f, 1.f)) {
    // ... period measurement ...
    phase = 0.0;
}
```

**Trade-offs:**
- Pro: Perfect beat alignment, predictable modulation timing.
- Con: At division ratios (e.g., /4), the LFO completes one cycle per 4 clock beats, so phase resets happen mid-cycle on intermediate beats. This causes a visible "snap" in the waveform. **Solution: Only reset phase on the Nth clock edge for /N divisions.**

**Refined approach for divisions:**

```cpp
int clockBeatCount = 0;  // counts edges within one LFO cycle

if (clockTrigger.process(clkVoltage, 0.1f, 1.f)) {
    clockBeatCount++;
    // ... period measurement ...

    int ratioIdx = getRatioIndex();
    float ratio = RATIOS[ratioIdx];

    if (ratio < 1.f) {
        // Division: reset only every N beats (e.g., /4 = every 4th beat)
        int divisor = (int)(1.f / ratio + 0.5f);
        if (clockBeatCount >= divisor) {
            phase = 0.0;
            clockBeatCount = 0;
        }
    } else {
        // Unity or multiplication: reset every beat
        phase = 0.0;
        clockBeatCount = 0;
    }
}
```

### Pattern 4: Lock-Free Atomic Display State

**What:** Use `std::atomic` variables to pass simple display state from audio thread to GUI thread without locks.

**Already established in v1.0** with `displayPhase`, `displayDrift`, `displayReadIdx`. Clock sync extends this pattern with `displayClockValid` and `displayRatioIndex`.

**Guideline:** Only use atomics for small, independently meaningful values. The display buffer uses double-buffering (not atomics on each sample) because it is an array.

## Anti-Patterns to Avoid

### Anti-Pattern 1: Locking in process()

**What people do:** Use `std::mutex` to protect shared state between audio and GUI threads.

**Why it is wrong:** `process()` runs at sample rate (e.g., 48,000 times/second). Any lock contention causes audio glitches. VCV Rack's threading model makes mutex priority inversion likely.

**Do this instead:** Lock-free atomics and double buffering, as the existing codebase already does.

### Anti-Pattern 2: Continuous Division Ratios

**What people do:** Map the Rate knob to a continuous multiplier in clocked mode (e.g., 0.1x to 10x).

**Why it is wrong:** Non-integer ratios like 1.37x have no musical meaning and create confusion. The LFO will not align with beats at arbitrary ratios.

**Do this instead:** Snap to a discrete table of musically meaningful ratios (/8, /6, /4, /3, /2, x2/3, x1, x3/2, x2, x3, x4, x6, x8).

### Anti-Pattern 3: Phase Reset on Every Clock Edge Regardless of Division

**What people do:** Always set `phase = 0.0` on every clock edge, even when the LFO is running at 1/4 speed.

**Why it is wrong:** At /4, the LFO needs 4 beats to complete one cycle. Resetting every beat means the waveform never gets past the first quarter, producing a truncated saw-like output regardless of morph setting.

**Do this instead:** Count clock edges and only reset when the division count is reached. See Pattern 3 above.

### Anti-Pattern 4: Serializing Clock State

**What people do:** Save and restore clockPeriod, smoothedClockPeriod, clockValid in the patch file.

**Why it is wrong:** The clock source may have changed tempo while the patch was closed. Restoring stale clock state causes a brief period of wrong frequency until new edges arrive and override it. Worse, if the clock source is removed before loading, the module will appear to be clocked with a ghost tempo.

**Do this instead:** Start fresh on patch load. The clock tracker will acquire valid state within 2 edges (~1 beat). This matches the existing philosophy of not serializing OU drift state.

## Integration Points

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| Clock tracker to frequency computation | Member variables (`clockValid`, `smoothedClockPeriod`) | Same struct, same thread, no synchronization needed |
| Clock tracker to phase accumulator | Direct `phase = 0.0` assignment | Same struct, same thread |
| Clock tracker to display | `std::atomic<bool>` and `std::atomic<int>` | Audio thread writes, GUI thread reads |
| Rate knob to ratio selection | `getRatioIndex()` reads `params[RATE_PARAM]` | Same as existing param reads |

### VCV Rack SDK Integration

| SDK Component | Usage | Notes |
|---------------|-------|-------|
| `dsp::SchmittTrigger` | Edge detection on CLK input | Standard thresholds: low=0.1V, high=1.0V |
| `inputs[].isConnected()` | Detect CLK cable presence | Determines free-run vs clocked mode |
| `inputs[].getVoltage()` | Read CLK signal | Standard VCV Rack input reading |
| `configInput()` | Register CLK input | Called in constructor |

### Backward Compatibility

The module is **fully backward compatible**:
- No CLK cable connected = identical behavior to v1.0.
- No new parameters that could break existing patch files.
- Enum extension adds CLK_INPUT at the end, so existing input indices are unchanged.
- No serialization changes.

Existing patches saved with v1.0 will load and run identically in v1.1.

## Build Order (Suggested Phase Sequence)

Based on dependency analysis:

### Phase 1: Clock Input Infrastructure
**Add:** CLK_INPUT enum entry, `configInput()` call, `SchmittTrigger` member, `processClockInput()` method with edge detection and period measurement.
**Test:** Connect a clock source, verify edges are detected and period is measured (log output or debug display).
**Why first:** Everything else depends on clock detection working correctly.

### Phase 2: Frequency Computation in Clocked Mode
**Add:** Ratio table, `getRatioIndex()`, dual-mode frequency selection in `process()`.
**Test:** Connect clock, verify LFO frequency tracks clock at x1. Turn Rate knob, verify divisions and multiplications.
**Why second:** Depends on Phase 1 clock detection. This is the core functional change.

### Phase 3: Phase Reset Logic
**Add:** Phase reset on clock edge with division-aware counting.
**Test:** Connect clock at slow tempo, verify waveform starts at beginning on beat. Test /4 division -- verify full cycle completes over 4 beats without premature reset.
**Why third:** Refines Phase 2 behavior. Phase reset is tightly coupled to division logic.

### Phase 4: Display Integration
**Add:** `displayClockValid` and `displayRatioIndex` atomics, sync badge rendering, division label rendering in `WaveformDisplay`.
**Test:** Visual verification -- badge appears when clocked, disappears when disconnected. Label shows correct ratio as Rate knob turns.
**Why fourth:** Pure visual layer, no functional dependency except reading state from Phase 1-3.

### Phase 5: Panel SVG Update
**Add:** CLK jack hole, "CLK" label at (42.96, 86mm), widget `addInput()` call.
**Test:** Visual verification in VCV Rack module browser and running module.
**Why fifth:** Can be done in parallel with Phases 1-4 but listed last because it is trivial and independent.

### Phase 6: Edge Cases and Polish
**Refine:** Clock disconnect/reconnect behavior, timeout for stale clock (e.g., if no edge for 10s, revert to free-run), Rate knob tooltip in clocked mode (custom ParamQuantity).
**Test:** Stress testing -- rapid connect/disconnect, tempo changes, extreme divisions, very slow clocks, very fast clocks.
**Why last:** Polish depends on all prior phases being functionally complete.

## Scalability Considerations

| Concern | Impact | Mitigation |
|---------|--------|------------|
| CPU cost of clock tracking | Negligible: one SchmittTrigger::process() + one float comparison per sample | No concern |
| Multiple instances with same clock | Each module tracks independently -- no shared state | Correct by design |
| Very fast clocks (>100 Hz) | Phase reset creates discontinuities in output, may cause clicking if used as audio-rate modulation | Acceptable: LFO is sub-audio by design (max 20Hz native), fast clocks with multipliers would exceed this |
| Very slow clocks (<0.1 Hz) | Long period between edges means EMA takes many seconds to converge | First edge snaps directly; subsequent edges smooth. Acceptable trade-off |
| Clock jitter | EMA smoothing handles typical 1-sample jitter | Tested by VCV Rack's cable delay model |

## Sources

- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) -- SchmittTrigger thresholds (0.1V/1.0V), trigger duration (1ms), clock/reset timing (HIGH confidence)
- [VCV Rack API: dsp::TSchmittTrigger](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TSchmittTrigger) -- process() method signature (HIGH confidence)
- [VCV Rack API: dsp::TTimer](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TTimer) -- Timer utility for debouncing (HIGH confidence)
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) -- configInput, dataToJson/dataFromJson patterns (HIGH confidence)
- [VCV Fundamental LFO](https://library.vcvrack.com/Fundamental/LFO) -- Reference behavior: CLK syncs frequency, FREQ knob becomes multiplier at 1x default (MEDIUM confidence -- behavior observed, source code not inspected)
- [VCV Community: Clock Dividers discussion](https://community.vcvrack.com/t/clock-dividers-comparison-and-doubts/5940) -- Phase reset behavior at division boundaries (MEDIUM confidence)
- Existing codebase: `src/AnalogLFO.cpp` -- Direct inspection of current architecture (HIGH confidence)

---
*Architecture research for: Clock Sync integration into Analog Series LFO (v1.1)*
*Researched: 2026-03-07*
