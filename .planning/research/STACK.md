# Stack Research: Clock Sync Additions

**Domain:** VCV Rack LFO clock synchronization (v1.1 milestone)
**Researched:** 2026-03-07
**Confidence:** HIGH (all components are VCV Rack SDK built-ins already validated in v1.0; no external dependencies)

## Scope

This document covers ONLY the stack additions and changes needed for clock sync features. The core stack (VCV Rack 2 SDK, C++17, NanoVG, nanosvg, GNU Make/plugin.mk, lock-free double buffer, OU drift engine) is validated and unchanged from v1.0. See v1.0 STACK.md for foundation details.

## New SDK Components Needed

### Clock Edge Detection

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `rack::dsp::SchmittTrigger` | SDK built-in | Detect rising edges on CLK input | Already in the SDK and already listed in v1.0 stack (though not yet used in AnalogLFO.cpp). Standard VCV approach for trigger inputs. Uses hysteresis with configurable thresholds -- call `process(voltage, lowThreshold, highThreshold)` which returns `true` on rising edge. Default thresholds (0.f low, 1.f high) match VCV voltage standards. No alternative needed. |

### Clock Period Measurement

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `rack::dsp::Timer` (TTimer) | SDK built-in | Measure elapsed time between clock edges | Accumulates time via `process(deltaTime)`, read with `getTime()`, clear with `reset()`. Use to measure the period between consecutive CLK rising edges. The SDK provides this specifically for timing measurements. No external timing library needed. |

### Period Smoothing

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| `rack::dsp::ExponentialFilter` | SDK built-in | Smooth measured clock periods for stability | One-pole exponential moving average. Apply to raw period measurements to reject jitter and produce stable tempo tracking. Already listed in v1.0 stack for parameter smoothing -- same utility, new application. Set lambda to balance responsiveness vs. stability (0.1-0.3 range for clock tracking). |

### Display Sync Indicator

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| NanoVG (already bundled) | SDK bundled | Render sync badge and division label in display | Already used for the waveform display. Add sync indicator drawing to the existing `WaveformDisplay::drawLayer()`. NanoVG text rendering (`nvgText`) for division labels, simple geometry for a sync badge. No new dependency. |

### Panel Update

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| SVG panel (nanosvg, already used) | SDK bundled | Add CLK jack hole to the panel SVG | Same approach as v1.0. Edit `res/AnalogLFO.svg` to add the CLK input jack position. No new technology. |

## Components NOT Needed

These were considered and explicitly rejected:

| Technology | Why NOT Needed |
|------------|----------------|
| `rack::dsp::ClockDivider` | This is a sample-counting utility for reducing process callback frequency (e.g., update lights every 256 samples). It does NOT implement musical clock division. Our clock division is ratio-based frequency math: `lfoFreq = clockFreq * ratio`. Do not confuse these. |
| PLL (phase-locked loop) | Overkill for LFO clock sync. PLLs are for continuous phase tracking with proportional-integral correction. Our use case is simpler: measure clock period, derive frequency, reset phase on edge. A PLL adds complexity (loop filter tuning, lock detection, acquisition time) without benefit for a module that receives clean digital triggers. |
| External clock/tempo libraries | No such thing exists in the VCV Rack ecosystem. Clock tracking is always hand-rolled from SchmittTrigger + Timer. The algorithm is ~20 lines of code. |
| `rack::dsp::SlewLimiter` for period smoothing | ExponentialFilter is more appropriate. SlewLimiter has asymmetric rate limiting which can cause the tracked period to lag behind tempo changes. ExponentialFilter provides symmetric exponential smoothing that converges correctly. |
| `dsp::PulseGenerator` | Would be needed if we were generating clock output triggers. We are only receiving clock input. Not needed for this milestone. |
| JSON serialization (`dataToJson`/`dataFromJson`) | Clock state is transient -- period measurement, timer accumulator, and sync status should NOT be serialized. On patch load, the module starts in free-running mode and re-acquires clock automatically when edges arrive. This matches the existing design decision of not serializing OU drift state ("fresh randomness on patch load"). |
| `std::thread` or async | All clock tracking runs in the `process()` callback. Period measurement and frequency calculation are trivial arithmetic -- no async needed. |
| Additional C++ standard library features | Everything needed is already in `<cmath>`, `<atomic>`, `<array>`. No new includes required beyond what v1.0 already uses. |

## Integration Points with Existing Code

### New Enum Members

Add to existing enums in `AnalogLFO`:

```cpp
// In InputId enum -- add CLK input
enum InputId {
    MORPH_CV_INPUT,
    DRIFT_CV_INPUT,
    CHARACTER_CV_INPUT,
    CLK_INPUT,          // NEW
    INPUTS_LEN
};
```

### New Member Variables

Add to `AnalogLFO` struct (no new headers needed):

```cpp
// Clock sync state
dsp::SchmittTrigger clockTrigger;    // Edge detection on CLK input
dsp::Timer clockTimer;               // Period measurement
float clockPeriod = 0.f;             // Smoothed period (seconds)
float rawClockPeriod = 0.f;          // Last measured raw period
bool clockSynced = false;            // Whether we have valid clock data
int clockEdgeCount = 0;              // Edges received (for acquisition)
std::atomic<bool> displaySynced{false};  // Lock-free sync status for display
std::atomic<float> displayDivision{1.f}; // Lock-free division ratio for display
```

### Rate Knob Dual-Mode

The existing Rate knob (`RATE_PARAM`, range 0.01-20 Hz) must switch behavior:

- **Free mode** (no CLK cable): Current behavior, direct frequency in Hz
- **Sync mode** (CLK connected): Rate knob becomes a division/multiplication selector

The knob range does NOT need to change. The interpretation changes based on `inputs[CLK_INPUT].isConnected()`. This is a standard VCV pattern used by the Fundamental LFO.

### Phase Reset Integration

Phase reset on clock edge integrates directly with the existing `phase` accumulator (already `double` precision). On a clock edge in sync mode, set `phase = 0.0` to align the LFO to the beat. This is a one-line operation in the process callback.

### Display Integration

The existing `WaveformDisplay` reads module state via atomics. Add two new atomics (`displaySynced`, `displayDivision`) following the same lock-free pattern used for `displayPhase` and `displayDrift`. The display draws a small sync badge and division text when `displaySynced` is true.

## Clock Division/Multiplication Ratios

Standard musical ratios to support via the Rate knob in sync mode:

| Ratio | Meaning | LFO Cycles per Clock |
|-------|---------|---------------------|
| /8 | 8x slower than clock | 0.125 |
| /4 | 4x slower | 0.25 |
| /3 | 3x slower | 0.333 |
| /2 | 2x slower | 0.5 |
| x1 | Match clock | 1.0 |
| x2 | 2x faster | 2.0 |
| x3 | 3x faster | 3.0 |
| x4 | 4x faster | 4.0 |
| x8 | 8x faster | 8.0 |

Map the Rate knob's continuous range to snap or smoothly select between these ratios. The knob's center position should map to x1 (match clock).

## Algorithm Summary: Clock Period Tracking

The core algorithm for period measurement is straightforward using SDK primitives:

```
On each process() call:
  1. clockTimer.process(args.sampleTime)     // accumulate time
  2. if clockTrigger.process(clkVoltage):    // rising edge detected
       rawPeriod = clockTimer.getTime()
       clockTimer.reset()
       if rawPeriod is reasonable (0.01s to 30s):
         smooth clockPeriod toward rawPeriod (ExponentialFilter)
         clockSynced = true
         clockEdgeCount++
         reset phase to 0.0                  // beat alignment
  3. if clockTimer.getTime() > timeout:       // clock lost
       clockSynced = false
       fall back to free-running mode
```

This is ~20 lines in the process callback. No libraries, no complexity.

## Period Smoothing Strategy

Use `dsp::ExponentialFilter` with a lambda around 0.1-0.2 for clock period smoothing:

- **Lambda 0.1**: Heavy smoothing. Stable but slow to respond to tempo changes. Takes ~10 edges to converge. Good for live performance where tempo is mostly stable.
- **Lambda 0.2**: Moderate smoothing. Responds within ~5 edges. Good default.
- **Lambda 0.5**: Light smoothing. Responds in 2-3 edges but more jittery. Good for rapid tempo changes.

Recommendation: Start at 0.15, tune during development. The first 2-3 edges after CLK connection should use the raw period directly (no smoothing) to minimize acquisition time.

## Clock Timeout

If no clock edge arrives within 2x the last measured period (or a fixed maximum like 4 seconds if no period established), consider the clock lost and revert to free-running mode. This handles:
- CLK cable disconnected
- Upstream clock module stopped
- Upstream clock module deleted

## Version Compatibility

No version concerns. All components (`SchmittTrigger`, `Timer`, `ExponentialFilter`) have been stable in the VCV Rack 2 SDK since 2.0.0. The current project targets SDK 2.5.x. No breaking changes in these APIs between 2.0 and 2.5.

## Build Changes

None. No new source files, no new dependencies, no Makefile changes. All new code goes into the existing `src/AnalogLFO.cpp`. The only file-system change is updating `res/AnalogLFO.svg` to add the CLK jack position.

## Sources

- [VCV Rack API: TSchmittTrigger](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TSchmittTrigger) -- trigger detection API
- [VCV Rack API: TTimer](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TTimer) -- time measurement API
- [VCV Rack API: ClockDivider](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1ClockDivider) -- sample-counting utility (NOT musical clock division)
- [VCV Rack API: PulseGenerator](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1PulseGenerator) -- trigger output generation (not needed)
- [VCV Rack API: dsp namespace](https://vcvrack.com/docs-v2/namespacerack_1_1dsp) -- full DSP utilities listing
- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) -- trigger levels (10V, 1ms), gate thresholds, cable delay (1-sample)
- [VCV Rack DSP Manual](https://vcvrack.com/manual/DSP) -- general DSP guidance
- [VCV Community: Clock Multiplier Code](https://community.vcvrack.com/t/example-clock-multiplier-code/20570) -- community discussion on clock multiplication approaches
- [VCV Community: TTimer Usage](https://community.vcvrack.com/t/ttimer-understand-how-to-use-it/20987) -- practical Timer usage patterns
- [VCV Fundamental LFO](https://library.vcvrack.com/Fundamental/LFO) -- reference: CLK input synchronizes frequency, FREQ knob becomes multiplier
- Existing codebase: `src/AnalogLFO.cpp` (directly verified, 537 lines)

---
*Stack research for: Forge Audio Analog Series v1.1 Clock Sync*
*Researched: 2026-03-07*
