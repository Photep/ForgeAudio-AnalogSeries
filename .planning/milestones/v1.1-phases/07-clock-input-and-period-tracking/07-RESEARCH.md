# Phase 7: Clock Input and Period Tracking - Research

**Researched:** 2026-03-07
**Domain:** Clock edge detection, period measurement, state machine, outlier filtering (VCV Rack 2 SDK)
**Confidence:** HIGH

## Summary

Phase 7 implements the clock tracking infrastructure for the Analog Series LFO. The scope is deliberately narrow: detect clock edges, measure the inter-edge period, smooth it with EMA, handle clock loss, and filter outliers. There is no ratio table, no frequency override, no anti-click crossfade, and no display changes -- those belong to Phases 8-10.

The VCV Rack 2 SDK provides all necessary primitives: `dsp::SchmittTrigger` for edge detection with standard 0.1V/1.0V thresholds, and `dsp::Timer` for float-precision time accumulation. The core algorithm is approximately 60 lines of C++ added to the existing `AnalogLFO` struct and `process()` callback. No new files, no new dependencies, no Makefile changes.

The main architectural decision is a three-state model (FREE / ACQUIRING / LOCKED) that governs clock tracking behavior. This model handles first-pulse gracefully, enables fast-track re-acquisition when a known clock returns, and gates outlier rejection to only apply in steady-state (LOCKED). The state is exposed to the GUI thread via `std::atomic<int>` following the established lock-free pattern.

**Primary recommendation:** Implement the three-state clock tracker as inline members and a `processClockInput()` method on the existing `AnalogLFO` struct. Store the smoothed period and clock state but do NOT override frequency or reset phase beyond what is specified in CONTEXT.md decisions. Phase 7 stores the measured period; Phase 8 uses it.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- First clock pulse resets phase to exactly 0.0 (hard reset, no crossfade -- Phase 9 adds anti-click later)
- LFO continues at free-running Rate knob frequency between pulse 1 and pulse 2
- On pulse 2, snap instantly to clock-derived frequency (no slew)
- On clock loss timeout, snap back to Rate knob frequency instantly (Phase 9 adds smooth slew)
- Three states: FREE -> ACQUIRING -> LOCKED
- ACQUIRING starts on pulse 1 (phase reset, but frequency stays free-running)
- Use measured period immediately from pulse 2 onward, even during ACQUIRING
- Transition to LOCKED after 3-4 consistent edges (EMA convergence)
- Outlier filter only active in LOCKED state -- ACQUIRING accepts all measurements
- Clock state exposed as std::atomic<int> for lock-free display access (same pattern as displayPhase/displayDrift)
- On clock loss, remember last smoothed period
- If new clock arrives within ~20% of last known period, skip ACQUIRING and fast-track to LOCKED
- If new clock is outside 20% tolerance, full re-acquisition from ACQUIRING
- Symmetric 3x threshold (rejects both speedups and slowdowns exceeding 3x current period)
- Outliers silently ignored -- discard measurement, keep current smoothed period
- Let timeout handle intentional tempo changes (reject outliers -> timeout -> FREE -> re-acquire new tempo)
- Base timeout: 3x smoothed period
- Floor: 1 second (prevents false timeout at fast tempos like 300 BPM)
- Ceiling: 5 seconds (prevents excessive wait at slow tempos like 20 BPM)
- Formula: clamp(3.0 * smoothedPeriod, 1.0, 5.0)
- Instant revert to FREE when CLK jack is physically disconnected (using VCV isConnected())
- Timeout only applies when cable is connected but pulses stop

### Claude's Discretion
- Exact EMA alpha value (requirements say ~0.3, Claude can tune)
- Exact edge count for ACQUIRING -> LOCKED transition (3 or 4, based on convergence math)
- Internal struct organization for clock state
- Timer implementation details

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| CLK-01 | LFO accepts clock trigger input via CLK jack | Add `CLK_INPUT` to `InputId` enum, call `configInput()` in constructor, add jack widget. SDK `dsp::SchmittTrigger` and `dsp::Timer` provide all primitives. |
| CLK-02 | Edge detection uses SchmittTrigger with VCV standard thresholds (0.1V/1.0V) | Verified against SDK source: `TSchmittTrigger<float>::process(float in, float lowThreshold = 0.f, float highThreshold = 1.f)`. Call with `process(voltage, 0.1f, 1.0f)` per VCV Voltage Standards. |
| CLK-03 | Clock period measured with float timer and EMA smoothing (alpha ~0.3) | `dsp::Timer` accumulates `float time` via `process(deltaTime)`. EMA formula: `smoothed += alpha * (raw - smoothed)`. Recommend alpha = 0.3 (settles in ~5 edges, good compromise). |
| CLK-04 | Clock-loss timeout (3x smoothed period) reverts to free-running mode | Check `clockTimer > clamp(3.0 * smoothedPeriod, 1.0, 5.0)` each process() call. On timeout, transition to FREE state. |
| CLK-05 | Outlier rejection filters tempo jumps exceeding 3x current period | In LOCKED state only: if `rawPeriod > 3.0 * smoothedPeriod` or `rawPeriod < smoothedPeriod / 3.0`, discard measurement. |
| CLK-06 | First clock pulse resets phase without setting frequency (waits for second edge) | Three-state model: pulse 1 enters ACQUIRING (phase = 0.0, freq unchanged), pulse 2 computes first period and applies it. |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `dsp::SchmittTrigger` (float specialization) | VCV Rack 2 SDK (stable since 2.0) | Detect rising edges on CLK input | SDK built-in, standard VCV approach, correct hysteresis with 0.1V/1.0V thresholds. Handles UNINITIALIZED state correctly (no false trigger on cable connect while signal is high). |
| `dsp::Timer` | VCV Rack 2 SDK (stable since 2.0) | Accumulate elapsed time between clock edges | Float-precision time accumulator. `process(deltaTime)` adds time, `getTime()` reads, `reset()` clears. Avoids integer sample-counting quantization error. |
| `std::atomic<int>` | C++17 standard | Expose clock state (FREE/ACQUIRING/LOCKED) to GUI thread | Established pattern in this codebase: `displayReadIdx`, `displayPhase`, `displayDrift` all use atomics for lock-free audio-to-GUI transfer. |

### Not Needed
| Library | Why NOT Needed |
|---------|----------------|
| `dsp::ExponentialFilter` | Its API (`process(deltaTime, in)`) is designed for per-sample smoothing with a lambda/tau parameter. For clock period smoothing that happens only on edge events (not every sample), a direct EMA one-liner is simpler and more readable: `smoothed += alpha * (raw - smoothed)`. |
| `dsp::ClockDivider` | This is a sample-counting utility for reducing callback frequency. It does NOT implement musical clock division. |
| `dsp::PulseGenerator` | For generating trigger outputs. We are only receiving clock input. |
| Any external library | Clock tracking is ~60 lines of arithmetic using SDK primitives. |
| New header includes | Everything needed is already included: `<cmath>`, `<atomic>`. No new includes required. |

**Installation:** No changes. No new dependencies. No Makefile changes.

## Architecture Patterns

### Recommended: Inline Clock State on AnalogLFO Struct

The clock tracker is NOT a separate class. It is a set of member variables and one helper method on the existing `AnalogLFO` struct. At ~60 lines of state + logic, extracting a class adds indirection without benefit given the existing monolithic style.

### New Enum Member

```cpp
// In InputId enum -- add CLK input at the end (preserves backward compatibility)
enum InputId {
    MORPH_CV_INPUT,
    DRIFT_CV_INPUT,
    CHARACTER_CV_INPUT,
    CLK_INPUT,          // NEW -- index 3
    INPUTS_LEN          // now = 4
};
```

Adding CLK_INPUT at the end means existing input indices (0, 1, 2) are unchanged. VCV Rack's serialization handles new inputs gracefully.

### New Member Variables

```cpp
// Clock tracking state
enum ClockState { FREE = 0, ACQUIRING = 1, LOCKED = 2 };

dsp::SchmittTrigger clockTrigger;
dsp::Timer clockTimer;              // Time since last edge
float smoothedPeriod = 0.f;         // EMA-smoothed clock period (seconds)
float lastSmoothedPeriod = 0.f;     // Remembered period for fast-track re-acquisition
int clockEdgeCount = 0;             // Edges received since entering ACQUIRING
ClockState clockState = FREE;
bool prevClkConnected = false;      // For detecting cable disconnect

// Display state (lock-free audio -> GUI)
std::atomic<int> displayClockState{0};  // 0=FREE, 1=ACQUIRING, 2=LOCKED
```

### Pattern 1: Three-State Clock Tracker

**What:** FREE -> ACQUIRING -> LOCKED state machine governing clock behavior.

**State transitions:**
```
                 cable connect + first edge
FREE  ────────────────────────────────> ACQUIRING
  ^                                         |
  |  timeout / cable disconnect             | 3-4 consistent edges
  |                                         |
  +──────────── LOCKED <────────────────────+
                  |
                  | timeout / cable disconnect
                  +──────> FREE (remembers lastSmoothedPeriod)

  Fast-track: FREE -> LOCKED (if new period within 20% of lastSmoothedPeriod)
```

**Behavior per state:**

| State | Frequency Source | Phase Reset | Period Measurement | Outlier Filter |
|-------|-----------------|-------------|-------------------|----------------|
| FREE | Rate knob (free-running) | No | No | No |
| ACQUIRING | Rate knob (pulse 1), then clock-derived (pulse 2+) | Yes, on each edge | Yes, raw EMA | No (accept all) |
| LOCKED | Clock-derived | Yes, on each edge | Yes, EMA smoothed | Yes (3x threshold) |

**When to use:** Always. This is the only clock tracking pattern for this module.

### Pattern 2: EMA Period Smoothing

**What:** Exponential moving average on measured inter-edge periods.

**Formula:**
```cpp
smoothedPeriod += alpha * (rawPeriod - smoothedPeriod);
```

**Recommended alpha: 0.3**

Convergence analysis for alpha = 0.3:
- After 1 edge: 30% of true value captured
- After 2 edges: 51% captured
- After 3 edges: 66% captured
- After 5 edges: 83% captured
- After 8 edges: 94% captured (effectively converged)

This means ACQUIRING -> LOCKED transition at edge count 4 gives ~76% convergence, which is sufficient for stable operation. The remaining 24% converges during LOCKED state.

**Why 0.3 over other values:**
- alpha = 0.1: Too slow. Takes ~15 edges to converge. Sluggish tempo tracking.
- alpha = 0.2: Conservative. Takes ~8 edges. Viable but slower than necessary.
- alpha = 0.3: Balanced. ~5 edges for practical convergence. Matches CLK-03 requirement spec ("alpha ~0.3").
- alpha = 0.5: Too reactive. Passes through jitter to frequency. Unstable at slow tempos.

### Pattern 3: Outlier Rejection (LOCKED state only)

**What:** Discard period measurements that deviate more than 3x from the current smoothed period.

**Implementation:**
```cpp
bool isOutlier = (rawPeriod > 3.0f * smoothedPeriod) ||
                 (rawPeriod < smoothedPeriod / 3.0f);
if (isOutlier && clockState == LOCKED) {
    // Silently discard -- keep current smoothedPeriod
    return;
}
```

**Why 3x and not 2x or 4x:**
- 2x: Too aggressive. Rejects normal tempo ramps (e.g., BPM doubling over several beats).
- 3x: Filters cable hot-swap spikes while accepting gradual tempo changes. A 3x jump means going from 120 BPM to 40 BPM or 360 BPM in one beat -- clearly an error or cable swap, not an intentional tempo change.
- 4x: Too permissive. Lets through spikes that cause visible frequency jumps.

**Why LOCKED only:** During ACQUIRING, we do not yet have a reliable reference period to compare against. Rejecting early measurements could prevent lock-on to valid tempos.

### Pattern 4: Fast-Track Re-Acquisition

**What:** When clock returns after timeout, skip ACQUIRING if the new period matches the remembered period.

**Implementation:**
```cpp
// On first edge after FREE state:
if (lastSmoothedPeriod > 0.f) {
    float ratio = rawPeriod / lastSmoothedPeriod;
    if (ratio > 0.8f && ratio < 1.2f) {
        // Fast-track: same clock source reconnected
        clockState = LOCKED;
        smoothedPeriod = lastSmoothedPeriod;
        // Apply EMA update with new measurement
        smoothedPeriod += 0.3f * (rawPeriod - smoothedPeriod);
        return;
    }
}
// Otherwise: full ACQUIRING path
```

**Why 20% tolerance:** Accounts for natural clock jitter and small BPM adjustments between disconnect and reconnect. A clock at 120 BPM (+/-20%) covers 96-144 BPM, which is a reasonable "same source" range.

### Pattern 5: Scaled Timeout

**What:** Clock-loss timeout adapts to measured tempo with floor and ceiling.

**Formula:** `timeout = clamp(3.0f * smoothedPeriod, 1.0f, 5.0f)`

| BPM | Period | Raw 3x Timeout | Clamped Timeout |
|-----|--------|---------------|-----------------|
| 300 | 0.2s | 0.6s | 1.0s (floor) |
| 120 | 0.5s | 1.5s | 1.5s |
| 60 | 1.0s | 3.0s | 3.0s |
| 30 | 2.0s | 6.0s | 5.0s (ceiling) |
| 20 | 3.0s | 9.0s | 5.0s (ceiling) |

### Anti-Patterns to Avoid

- **Do NOT use integer sample counting for period measurement.** Use `dsp::Timer` (float accumulation). Integer counting introduces quantization error that accumulates over long sessions.
- **Do NOT override LFO frequency in this phase.** Phase 7 measures and stores the period. Phase 8 uses it to derive frequency. Mixing concerns creates testing confusion.
- **Do NOT add crossfade/slew on phase reset.** CONTEXT.md explicitly says "hard reset, no crossfade -- Phase 9 adds anti-click later." Phase 7 does `phase = 0.0` directly.
- **Do NOT serialize clock state.** Transient by design, matching OU drift philosophy. Re-acquire on patch load.
- **Do NOT gate the frequency change.** CONTEXT.md says "On pulse 2, snap instantly to clock-derived frequency (no slew)." Phase 9 adds smooth slew later.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Edge detection with hysteresis | Custom comparator with state tracking | `dsp::SchmittTrigger` | Handles UNINITIALIZED state, avoids false triggers on noise/ringing, matches VCV Voltage Standards |
| Time accumulation between events | Integer sample counter (`samplesSinceEdge++`) | `dsp::Timer` | Float precision avoids quantization error. Timer API is clean: `process()`, `getTime()`, `reset()` |
| Lock-free state transfer to GUI | Mutex or condition variable | `std::atomic<int>` with `memory_order_relaxed` | Established pattern in codebase. Zero audio thread contention. |

**Key insight:** The VCV Rack 2 SDK provides exactly the primitives needed. The custom code is the state machine logic and EMA smoothing -- both are pure arithmetic, not infrastructure.

## Common Pitfalls

### Pitfall 1: SchmittTrigger UNINITIALIZED State on Cable Connect
**What goes wrong:** When a cable is first connected, the SchmittTrigger is in UNINITIALIZED state. If the signal is already HIGH (e.g., a gate module outputting 10V), the trigger transitions to HIGH but does NOT fire a trigger event. The first actual trigger only fires when the signal goes LOW then HIGH.
**Why it happens:** The SDK SchmittTrigger requires seeing a LOW-to-HIGH transition. UNINITIALIZED-to-HIGH is not a trigger. This is correct behavior (prevents false triggers) but surprising.
**How to avoid:** Accept this. The first real trigger event will occur on the first actual pulse. Do not try to "prime" the trigger state. The user experience is identical -- the LFO starts tracking on the first real clock pulse.
**Warning signs:** "Clock sync doesn't start until the second pulse" -- this is expected behavior, not a bug.

### Pitfall 2: Timer Overflow at Very Slow Tempos
**What goes wrong:** If no clock edge arrives for a very long time, `dsp::Timer::time` keeps accumulating. At 48kHz, float precision starts degrading after ~2^24 samples (~349 seconds). The timer value becomes less precise.
**Why it happens:** `dsp::Timer` uses `float` internally. Float has ~7 significant digits. At time=300 seconds, adding sampleTime=0.0000208 seconds loses precision.
**How to avoid:** The 5-second timeout ceiling prevents this. Once timeout triggers (max 5s), the state transitions to FREE and timer behavior is irrelevant. But also call `clockTimer.reset()` when transitioning to FREE to prevent stale accumulation.
**Warning signs:** Irrelevant with the 5s ceiling, but good hygiene to reset.

### Pitfall 3: Cable Disconnect Not Detected Until Next process() Call
**What goes wrong:** `inputs[CLK_INPUT].isConnected()` is checked in `process()`. If the user disconnects the cable between process calls, the module detects it on the next call. This is instantaneous for practical purposes (sample rate granularity).
**Why it happens:** VCV Rack updates cable state synchronously. `isConnected()` is always current within the process callback.
**How to avoid:** No issue. Just check `isConnected()` at the top of `processClockInput()` every call. The cost is negligible (a pointer comparison).

### Pitfall 4: EMA Smoothing on First Valid Period
**What goes wrong:** Applying EMA smoothing to the very first period measurement (when `smoothedPeriod` is 0 or uninitialized) produces a nonsensical result.
**Why it happens:** `smoothedPeriod += 0.3 * (rawPeriod - 0.0)` gives only 30% of the true period.
**How to avoid:** On the first valid measurement (edge count = 2), snap directly: `smoothedPeriod = rawPeriod`. Apply EMA only from the third edge onward. This is critical for fast acquisition.

### Pitfall 5: Phase Reset Without Frequency Override Creates Drift
**What goes wrong:** In Phase 7, pulse 1 resets phase to 0.0 but keeps the Rate knob frequency. Between pulse 1 and pulse 2, the LFO runs at the knob frequency, which may differ from the clock tempo. When pulse 2 arrives, the phase may not be at 0.0 -- it could be anywhere.
**Why it happens:** The Rate knob frequency and clock frequency are typically different.
**How to avoid:** This is expected and acceptable per CONTEXT.md decisions. Pulse 2 also resets phase to 0.0. The one-cycle "mismatch" between pulses 1 and 2 is inherent to the acquisition model. Phase 9 will add crossfade to smooth transitions.

## Code Examples

### Complete processClockInput() Method

```cpp
// Source: Synthesized from VCV Rack 2 SDK headers (dsp/digital.hpp)
// and CONTEXT.md decisions

void processClockInput(float sampleTime) {
    bool clkConnected = inputs[CLK_INPUT].isConnected();

    // Instant revert on cable disconnect
    if (!clkConnected) {
        if (prevClkConnected) {
            clockState = FREE;
            clockEdgeCount = 0;
            clockTimer.reset();
            // Remember period for fast-track re-acquisition
            if (smoothedPeriod > 0.f) {
                lastSmoothedPeriod = smoothedPeriod;
            }
            smoothedPeriod = 0.f;
            displayClockState.store(FREE, std::memory_order_relaxed);
        }
        prevClkConnected = false;
        return;
    }
    prevClkConnected = true;

    // Accumulate time
    clockTimer.process(sampleTime);

    // Check timeout (only when connected but no pulses arriving)
    if (clockState != FREE && smoothedPeriod > 0.f) {
        float timeout = std::fmax(1.0f, std::fmin(3.0f * smoothedPeriod, 5.0f));
        if (clockTimer.getTime() > timeout) {
            lastSmoothedPeriod = smoothedPeriod;
            clockState = FREE;
            clockEdgeCount = 0;
            smoothedPeriod = 0.f;
            clockTimer.reset();
            displayClockState.store(FREE, std::memory_order_relaxed);
        }
    }

    // Edge detection
    float clkVoltage = inputs[CLK_INPUT].getVoltage();
    if (clockTrigger.process(clkVoltage, 0.1f, 1.0f)) {
        float rawPeriod = clockTimer.getTime();
        clockTimer.reset();
        clockEdgeCount++;

        // Phase reset on every edge (hard reset per CONTEXT.md)
        phase = 0.0;

        if (clockEdgeCount == 1) {
            // First edge: enter ACQUIRING, no period measurement possible
            clockState = ACQUIRING;
            displayClockState.store(ACQUIRING, std::memory_order_relaxed);
        }
        else if (rawPeriod > 0.001f) {
            // Second edge onward: we have a period measurement

            // Outlier rejection (LOCKED state only)
            if (clockState == LOCKED && smoothedPeriod > 0.f) {
                bool isOutlier = (rawPeriod > 3.0f * smoothedPeriod) ||
                                 (rawPeriod < smoothedPeriod / 3.0f);
                if (isOutlier) {
                    // Silently discard
                    return;
                }
            }

            // Fast-track re-acquisition check
            if (clockState == FREE && clockEdgeCount == 2 && lastSmoothedPeriod > 0.f) {
                float ratio = rawPeriod / lastSmoothedPeriod;
                if (ratio > 0.8f && ratio < 1.2f) {
                    smoothedPeriod = lastSmoothedPeriod;
                    smoothedPeriod += 0.3f * (rawPeriod - smoothedPeriod);
                    clockState = LOCKED;
                    displayClockState.store(LOCKED, std::memory_order_relaxed);
                    return;
                }
            }

            // EMA smoothing
            if (smoothedPeriod <= 0.f) {
                smoothedPeriod = rawPeriod;  // First measurement: snap
            } else {
                smoothedPeriod += 0.3f * (rawPeriod - smoothedPeriod);
            }

            // State transition: ACQUIRING -> LOCKED after 4 consistent edges
            if (clockState == ACQUIRING && clockEdgeCount >= 4) {
                clockState = LOCKED;
                displayClockState.store(LOCKED, std::memory_order_relaxed);
            } else if (clockState == ACQUIRING) {
                displayClockState.store(ACQUIRING, std::memory_order_relaxed);
            }
        }
    }
}
```

### Integration into process()

```cpp
void process(const ProcessArgs& args) override {
    // Clock detection (new -- Phase 7)
    processClockInput(args.sampleTime);

    // Rate (Phase 7: UNCHANGED from v1.0)
    // Phase 8 will add: if (clockState >= ACQUIRING && smoothedPeriod > 0) use clock freq
    float freq = params[RATE_PARAM].getValue();
    freq = std::fmax(freq, 0.001f);

    // ... rest of process() unchanged ...
}
```

Note: The above shows Phase 7's integration point. Phase 7 calls `processClockInput()` (which handles phase reset) but does NOT change frequency computation. Phase 8 will add the frequency override conditional.

### Constructor Changes

```cpp
AnalogLFO() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
    // ... existing param configs ...
    configInput(CLK_INPUT, "Clock");  // NEW
    // ... rest unchanged ...
}
```

### Widget Changes

```cpp
// In AnalogLFOWidget constructor, add CLK jack
// Position TBD (Phase 10 handles panel layout), but needs to exist for Phase 7 testing
// Temporary position: x=52, y=86 (matching existing jack row pattern)
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(52.0, 86.0)), module, AnalogLFO::CLK_INPUT));
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Integer sample counting for period | Float time accumulation (`dsp::Timer`) | Always best practice | Avoids quantization error at non-round BPMs |
| Simple boolean (synced/not synced) | Three-state model (FREE/ACQUIRING/LOCKED) | Standard in Eurorack firmware (Mutable Instruments Tides, Stages) | Graceful first-pulse handling, fast re-acquisition |
| Fixed timeout (e.g., 2 seconds) | Tempo-adaptive timeout with floor/ceiling | Mutable Instruments Stages design | Works correctly across full BPM range |
| Accept all measurements | Outlier rejection with state-gated activation | Common in robust clock trackers | Prevents wild frequency spikes from cable hot-swap |

**Not used (and correct not to):**
- PLL (Phase-Locked Loop): Overkill for LFO rates. Simple edge measurement + EMA is sufficient. PLLs add loop filter tuning complexity without benefit for digital trigger inputs.

## Open Questions

1. **CLK jack physical position on panel**
   - What we know: Available positions include x=42.96,y=86 (across from Rate knob) or x=52,y=86 (rightmost column) or x=52,y=96 (above OUT jack)
   - What's unclear: Final position is a Phase 10 decision (panel design)
   - Recommendation: Place at a temporary position (x=52, y=86) for Phase 7 testing. Phase 10 will finalize placement and update the SVG panel.

2. **Edge count for ACQUIRING -> LOCKED transition: 3 or 4?**
   - What we know: At alpha=0.3, 3 edges gives ~66% convergence, 4 edges gives ~76%
   - What's unclear: Whether 66% is "converged enough" for stable operation
   - Recommendation: Use 4 edges. The extra 100ms-500ms (one beat at musical tempos) is imperceptible, but the additional 10% convergence meaningfully reduces frequency wobble on the 5th and subsequent edges. This is Claude's discretion per CONTEXT.md.

3. **How Phase 7 interacts with Phase 8's frequency override**
   - What we know: Phase 7 stores `smoothedPeriod`; Phase 8 adds `freq = ratio / smoothedPeriod`
   - What's unclear: Whether Phase 7 should stub out the frequency override or leave it entirely to Phase 8
   - Recommendation: Phase 7 should NOT touch frequency computation. The CONTEXT.md says "On pulse 2, snap instantly to clock-derived frequency" but this requires the ratio table (Phase 8). Phase 7's job is to have `smoothedPeriod` ready and correct. However, per CONTEXT.md, Phase 7 should still reset phase on edges and the LFO should continue at Rate knob frequency between pulse 1 and 2. This is exactly what happens when Phase 7 resets phase but does not change freq.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Manual testing in VCV Rack 2 (no automated unit test framework established) |
| Config file | none |
| Quick run command | `make install && open -a "VCV Rack 2 Free"` |
| Full suite command | Manual testing with clock module patched to CLK input |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CLK-01 | CLK jack accepts cable, module compiles with new input | manual | `make` (build check) | N/A |
| CLK-02 | Edge detection with 0.1V/1.0V thresholds | manual | Patch clock source, verify detection via state display | N/A |
| CLK-03 | Period measurement with EMA smoothing | manual | Patch 120 BPM clock, verify `smoothedPeriod` converges (~0.5s) | N/A |
| CLK-04 | Clock-loss timeout reverts to free-running | manual | Stop clock, verify FREE state within timeout | N/A |
| CLK-05 | Outlier rejection in LOCKED state | manual | Swap clock cables (tempo jump), verify no wild frequency spike | N/A |
| CLK-06 | First pulse resets phase, does not set frequency | manual | Connect clock, verify phase resets on pulse 1, freq unchanged until pulse 2 | N/A |

### Sampling Rate
- **Per task commit:** `make` (build verification)
- **Per wave merge:** Full manual test with clock module in VCV Rack
- **Phase gate:** All 6 requirements verified manually before proceeding to Phase 8

### Wave 0 Gaps
None -- this is a C++ VCV Rack module without an established automated test framework. All testing is manual verification in the running application. The `make` build check serves as the automated smoke test.

## Sources

### Primary (HIGH confidence)
- VCV Rack 2 SDK source: `dsp/digital.hpp` -- SchmittTrigger API (`process(float, float, float)`), Timer API (`process(deltaTime)`, `getTime()`, `reset()`) -- verified line by line
- VCV Rack 2 SDK source: `dsp/filter.hpp` -- ExponentialFilter API (considered and rejected for this use case)
- Existing codebase: `src/AnalogLFO.cpp` -- current architecture, enum layout, process() structure, display atomics pattern
- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) -- trigger thresholds (0.1V/1.0V), clock timing

### Secondary (MEDIUM confidence)
- [VCV Community: Clock multiplier code](https://community.vcvrack.com/t/example-clock-multiplier-code/20570) -- period measurement patterns
- [VCV Community: TTimer usage](https://community.vcvrack.com/t/ttimer-understand-how-to-use-it/20987) -- Timer usage patterns
- [Mutable Instruments Tides Manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/tides_2018/manual/) -- adaptive timeout, state machine design reference
- Project research documents: `.planning/research/STACK.md`, `ARCHITECTURE.md`, `PITFALLS.md`, `FEATURES.md`

### Tertiary (LOW confidence)
- EMA alpha = 0.3 optimal value -- engineering judgment. CLK-03 specifies "~0.3" which aligns, but optimal value depends on real-world jitter characteristics that can only be validated empirically.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- SDK APIs verified against actual header source code
- Architecture: HIGH -- three-state model well-established in Eurorack firmware (Mutable Instruments), all decisions locked in CONTEXT.md
- Pitfalls: HIGH -- analyzed against existing source code, SDK behavior verified
- Code examples: HIGH -- compiled from verified SDK APIs and locked CONTEXT.md decisions

**Research date:** 2026-03-07
**Valid until:** Indefinitely (VCV Rack 2 SDK DSP APIs are stable; no breaking changes expected)
