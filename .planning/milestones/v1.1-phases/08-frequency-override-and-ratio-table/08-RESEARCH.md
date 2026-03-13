# Phase 8: Frequency Override and Ratio Table - Research

**Researched:** 2026-03-07
**Domain:** Clock-synced frequency override, ratio table mapping, custom ParamQuantity tooltip (VCV Rack 2 SDK)
**Confidence:** HIGH

## Summary

Phase 8 transforms the Rate knob into a dual-mode control: continuous Hz when free-running (identical to v1.0), snapped musical ratios when clock-synced. The implementation requires three distinct pieces: (1) a static ratio table of 15 musical divisions/multiplications, (2) a frequency override in `process()` that replaces the Rate knob Hz value with `(1/smoothedPeriod) * ratio`, and (3) a custom `ParamQuantity` subclass that changes the tooltip between "Rate: X.XX Hz" and "Rate: x4 (synced)".

All three pieces are straightforward C++ additions to the existing `AnalogLFO` struct in `src/AnalogLFO.cpp`. The custom `ParamQuantity` pattern is well-established in the VCV Rack 2 SDK -- `configParam<TParamQuantity>()` accepts a template argument, and the SDK automatically sets the `module` pointer so the subclass can read runtime state (clock state, ratio index) from the audio thread via atomics. No new files, no new dependencies, no Makefile changes.

The main architectural insight is that the knob's internal parameter range stays 0.01-20 Hz unchanged. In clocked mode, the raw parameter value is ignored for frequency computation -- instead, the normalized knob position (0..1) selects a ratio index via `round(normalized * 14)`. This means the knob physically works the same way (smooth rotation) but its meaning changes contextually. When clock disconnects, the parameter value is read directly for Hz again, with no reconfiguration needed.

**Primary recommendation:** Add the ratio table as a `static constexpr` array, override frequency at the existing `freq = params[RATE_PARAM].getValue()` line, and use `configParam<RateParamQuantity>()` for the custom tooltip. Expose `displayRatioIndex` as `std::atomic<int>` following the established lock-free pattern.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Equal spacing: each of 15 ratios gets an equal slice of the knob's 0-1 normalized range
- Ratio table: /16, /8, /6, /4, /3, /2, /1.5, x1, x1.5, x2, x3, x4, x6, x8, x16
- Stored as a static const float array of 15 ratio multiplier values
- Index derived from knob position: `round(knobNormalized * 14)`
- Knob's internal parameter range stays 0.01-20Hz (no reconfiguration on clock connect/disconnect)
- When clock first connects, snap to nearest ratio based on current knob position
- When clock disconnects, instant restore to Hz meaning (consistent with Phase 7's instant-revert-to-FREE decision)
- Custom ParamQuantity for RATE_PARAM
- Free-running mode: shows standard "Rate: X.XX Hz" (identical to v1.0)
- Clocked mode: shows ratio label with synced indicator, e.g., "x4 (synced)"
- Division format: "/4" for divisions, "x4" for multiplications
- Fractional ratios: "/1.5" and "x1.5" (decimal, not unicode fractions)
- Unison: "x1 (synced)" -- same format as all other ratios
- No hysteresis: pure nearest-snap to closest ratio from knob position
- Immediate frequency jump when ratio changes (new ratio applied on next sample)
- No queuing to next clock edge -- instant response to knob movement
- Current ratio index exposed as std::atomic<int> for lock-free display access (same pattern as displayClockState, displayPhase, displayDrift)

### Claude's Discretion
- Exact ParamQuantity subclass structure
- Whether ratio multiplier values are stored as float or double
- Internal variable naming conventions
- How normalized knob position is extracted from the 0.01-20Hz param range

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| RATE-01 | Rate knob sets frequency directly when CLK is disconnected (identical to v1.0) | The frequency override conditional checks `clockState`. When FREE, `freq = params[RATE_PARAM].getValue()` -- unchanged from current code. No modification to free-running path. |
| RATE-02 | Rate knob selects clock division/multiplication ratio when CLK is connected | When ACQUIRING (with smoothedPeriod > 0) or LOCKED, normalize knob position via `getScaledValue()` (SDK method, maps 0.01-20 to 0-1), compute `ratioIndex = round(normalized * 14)`, override `freq = (1.f / smoothedPeriod) * RATIO_TABLE[ratioIndex]`. |
| RATE-03 | 15 discrete snapped ratios: /16, /8, /6, /4, /3, /2, /1.5, x1, x1.5, x2, x3, x4, x6, x8, x16 | Static constexpr array of 15 float multipliers. Division ratios stored as their reciprocal frequency multiplier: /16 = 1/16 = 0.0625f, x16 = 16.0f. See Standard Stack section for exact values. |
| RATE-06 | Custom ParamQuantity tooltip shows ratio label (e.g., "x4 (synced)") in clocked mode | Subclass `ParamQuantity` as `RateParamQuantity`. Override `getDisplayValueString()` to return ratio label when clocked, standard Hz value when free. Override `getUnit()` to return " Hz" when free, " (synced)" when clocked. Use `configParam<RateParamQuantity>()` template syntax. Module pointer access verified in SDK source. |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `ParamQuantity` (subclass) | VCV Rack 2 SDK (stable since 2.0) | Custom tooltip for dual-mode Rate knob | SDK built-in pattern. `configParam<T>()` template auto-sets `module` pointer. `SwitchQuantity` is an existing subclass example. |
| `Quantity::getScaledValue()` | VCV Rack 2 SDK (stable since 2.0) | Normalize knob position to 0-1 range | SDK built-in. Formula: `(value - minValue) / (maxValue - minValue)`. Avoids manually computing normalization. |
| `std::atomic<int>` | C++17 standard | Expose ratio index to GUI thread | Established pattern in this codebase: `displayClockState`, `displayPhase`, `displayDrift` all use atomics. |
| `std::round()` | C++ standard | Map normalized knob position to ratio index | Standard math. `round(0-1 * 14)` gives integer 0-14 with symmetric rounding. |

### Not Needed
| Library | Why NOT Needed |
|---------|----------------|
| `SwitchQuantity` | Cannot be used because it requires `snapEnabled = true` which would make the knob physically snap (detent feel). We want smooth knob rotation with logical snapping only. Custom `ParamQuantity` subclass is correct. |
| `dsp::ClockDivider` | This is a sample-counting utility for reducing callback frequency. NOT a musical clock divider. |
| Any new header includes | Everything needed is already included. |

### Ratio Table Values

The 15 ratios as frequency multipliers (how to derive LFO frequency from clock frequency):

```
Index  Ratio   Multiplier   Meaning
  0    /16     0.0625f      LFO completes 1 cycle per 16 clock beats
  1    /8      0.125f       LFO completes 1 cycle per 8 clock beats
  2    /6      0.166667f    LFO completes 1 cycle per 6 clock beats
  3    /4      0.25f        LFO completes 1 cycle per 4 clock beats
  4    /3      0.333333f    LFO completes 1 cycle per 3 clock beats
  5    /2      0.5f         LFO completes 1 cycle per 2 clock beats
  6    /1.5    0.666667f    Dotted-note feel (2/3 speed)
  7    x1      1.0f         Unison: LFO period equals clock period
  8    x1.5    1.5f         Triplet feel (3/2 speed)
  9    x2      2.0f         LFO completes 2 cycles per clock beat
 10    x3      3.0f         LFO completes 3 cycles per clock beat
 11    x4      4.0f         LFO completes 4 cycles per clock beat
 12    x6      6.0f         LFO completes 6 cycles per clock beat
 13    x8      8.0f         LFO completes 8 cycles per clock beat
 14    x16     16.0f        LFO completes 16 cycles per clock beat
```

**Stored as float.** Double precision is unnecessary -- the multipliers are exact simple fractions (or close enough for float: 1/6 = 0.16666667f is precise to 8 significant digits, more than sufficient for audio LFO rates).

**Installation:** No changes. No new dependencies. No Makefile changes.

## Architecture Patterns

### Pattern 1: Frequency Override Conditional

**What:** Replace the Rate knob Hz value with clock-derived frequency when synced.

**Where:** Line 332 of current `src/AnalogLFO.cpp`, immediately after `processClockInput()`.

**Current code:**
```cpp
float freq = params[RATE_PARAM].getValue();
freq = std::fmax(freq, 0.001f);
```

**New code pattern:**
```cpp
// Source: CONTEXT.md locked decisions + SDK ParamQuantity API
float freq;
int ratioIdx = -1;

if ((clockState == ACQUIRING || clockState == LOCKED) && smoothedPeriod > 0.f) {
    // Clocked mode: derive frequency from clock period and ratio
    float knobNormalized = paramQuantities[RATE_PARAM]->getScaledValue();
    ratioIdx = (int)std::round(knobNormalized * 14.f);
    ratioIdx = rack::math::clamp(ratioIdx, 0, 14);
    float clockFreq = 1.f / smoothedPeriod;
    freq = clockFreq * RATIO_TABLE[ratioIdx];
} else {
    // Free-running mode: direct Hz from knob (identical to v1.0)
    freq = params[RATE_PARAM].getValue();
}
freq = std::fmax(freq, 0.001f);

// Update display atomic for tooltip/display use
displayRatioIndex.store(ratioIdx, std::memory_order_relaxed);
```

**Key details:**
- `paramQuantities[RATE_PARAM]->getScaledValue()` returns the normalized 0-1 position. This is a Quantity method that computes `(value - minValue) / (maxValue - minValue)`. For RATE_PARAM (0.01 to 20), a value of 0.01 returns ~0.0, and 20.0 returns 1.0.
- `ratioIdx = -1` signals "not in ratio mode" to the display system.
- The override applies during both ACQUIRING (once smoothedPeriod > 0, i.e., from the second clock edge onward) and LOCKED. This matches CONTEXT.md: "On pulse 2, snap instantly to clock-derived frequency."
- `rack::math::clamp` is the SDK's clamp utility (already available, no new include needed).

### Pattern 2: Custom ParamQuantity Subclass

**What:** Override tooltip text based on clock state.

**Structure:**
```cpp
// Source: VCV Rack 2 SDK ParamQuantity.hpp (verified in local SDK)
struct RateParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        AnalogLFO* lfo = static_cast<AnalogLFO*>(module);
        if (!lfo) return ParamQuantity::getDisplayValueString();

        int ratioIdx = lfo->displayRatioIndex.load(std::memory_order_relaxed);
        if (ratioIdx < 0) {
            // Free-running: standard Hz display
            return ParamQuantity::getDisplayValueString();
        }

        // Clocked: show ratio label
        return AnalogLFO::RATIO_LABELS[ratioIdx];
    }

    std::string getUnit() override {
        AnalogLFO* lfo = static_cast<AnalogLFO*>(module);
        if (!lfo) return ParamQuantity::getUnit();

        int ratioIdx = lfo->displayRatioIndex.load(std::memory_order_relaxed);
        if (ratioIdx < 0) {
            return " Hz";  // Free-running: standard unit
        }
        return " (synced)";  // Clocked: synced indicator
    }
};
```

**Registration in constructor:**
```cpp
// Replace: configParam(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz");
// With:
configParam<RateParamQuantity>(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz");
```

**How it works:**
- VCV Rack calls `getString()` on the `ParamQuantity` to generate tooltip text.
- `getString()` returns `getLabel() + ": " + getDisplayValueString() + getUnit()`.
- In free-running mode: "Rate: 0.70 Hz" (identical to v1.0).
- In clocked mode: "Rate: x4 (synced)".
- The `module` pointer is set automatically by `configParam<T>()` (verified in SDK source: `q->ParamQuantity::module = this;`).
- The `static_cast<AnalogLFO*>(module)` is safe because the ParamQuantity is only ever created for this specific module type.
- Reading `displayRatioIndex` via atomic is lock-free and safe from the GUI thread.

### Pattern 3: Ratio Label Table

**What:** Static array of display strings for each ratio.

```cpp
static constexpr const char* RATIO_LABELS[15] = {
    "/16", "/8", "/6", "/4", "/3", "/2", "/1.5",
    "x1",
    "x1.5", "x2", "x3", "x4", "x6", "x8", "x16"
};
```

**Why static constexpr:** Zero runtime allocation. The strings are compile-time constants. Using `const char*` avoids `std::string` construction overhead on every tooltip query.

### Pattern 4: Forward Declaration for Circular Reference

**What:** The `RateParamQuantity` struct needs to reference `AnalogLFO` to cast the module pointer, but it must be defined before `AnalogLFO` (because `configParam<RateParamQuantity>()` is called in the constructor). This creates a circular dependency.

**Solution:** Forward-declare `AnalogLFO`, define `RateParamQuantity` with inline method declarations, then define the method bodies after `AnalogLFO` is fully defined. Alternatively (simpler): define `RateParamQuantity` inside the `AnalogLFO` struct as a nested struct -- this avoids the circular reference entirely since the nested struct has access to the enclosing struct's definition.

**Recommended approach: nested struct.**

```cpp
struct AnalogLFO : Module {
    // ... existing enums, members ...

    static constexpr float RATIO_TABLE[15] = { /* ... */ };
    static constexpr const char* RATIO_LABELS[15] = { /* ... */ };

    struct RateParamQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            AnalogLFO* lfo = static_cast<AnalogLFO*>(module);
            // ... implementation ...
        }
        std::string getUnit() override {
            AnalogLFO* lfo = static_cast<AnalogLFO*>(module);
            // ... implementation ...
        }
    };

    // ... rest of AnalogLFO ...
};
```

Then in the constructor: `configParam<RateParamQuantity>(RATE_PARAM, ...);`

**Why nested struct works:** At the point where `RateParamQuantity`'s methods are compiled, `AnalogLFO` is complete (all members defined before the nested struct's method bodies are instantiated). The `module` pointer from `ParamQuantity` is typed as `Module*`, so `static_cast<AnalogLFO*>` is valid.

### Anti-Patterns to Avoid

- **Do NOT reconfigure the parameter range when clock connects/disconnects.** CONTEXT.md explicitly says "Knob's internal parameter range stays 0.01-20Hz." The knob value continues to have Hz meaning -- it is simply not used for frequency computation when clocked.
- **Do NOT use `snapEnabled` on the Rate param.** Snap would make the knob physically detent, which is undesirable. The snapping is logical (in the frequency computation), not physical.
- **Do NOT add hysteresis.** CONTEXT.md says "No hysteresis: pure nearest-snap." The ratio changes the moment the nearest ratio changes.
- **Do NOT queue ratio changes to the next clock edge.** CONTEXT.md says "No queuing to next clock edge -- instant response to knob movement."
- **Do NOT add crossfade or slew on frequency jumps.** That is Phase 9's responsibility.
- **Do NOT serialize the ratio index.** It is derived from knob position + clock state at runtime.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Knob normalization (0-1 from Hz range) | Manual `(value - 0.01) / (20 - 0.01)` | `paramQuantities[RATE_PARAM]->getScaledValue()` | SDK method handles edge cases, matches how VCV Rack internally normalizes. Available because `paramQuantities` is a public member of `Module`. |
| Tooltip string formatting | Manual `std::to_string()` + concatenation | Override `getDisplayValueString()` and `getUnit()` on custom `ParamQuantity` | VCV Rack calls `getString()` which assembles label + value + unit. Overriding the pieces lets the framework handle formatting. |
| Clamping ratio index | Manual `if` statements | `rack::math::clamp(ratioIdx, 0, 14)` | SDK utility, already available, no new includes. |

**Key insight:** The VCV Rack 2 SDK's `ParamQuantity` system is designed for exactly this use case -- parameters whose display meaning changes at runtime. The template-based `configParam<T>()` pattern makes subclassing clean and zero-cost.

## Common Pitfalls

### Pitfall 1: Circular Reference Between RateParamQuantity and AnalogLFO
**What goes wrong:** `RateParamQuantity` needs to `static_cast<AnalogLFO*>(module)` but must be defined before `AnalogLFO` (for `configParam<T>` to work), creating an incomplete type error.
**Why it happens:** C++ requires types to be complete when casting or accessing members.
**How to avoid:** Define `RateParamQuantity` as a nested struct inside `AnalogLFO`. This guarantees `AnalogLFO` is complete when the nested struct's methods are compiled.
**Warning signs:** Compiler error: "incomplete type 'AnalogLFO' used in nested name specifier."

### Pitfall 2: getScaledValue() Returns Stale or Zero During Module Init
**What goes wrong:** Calling `paramQuantities[RATE_PARAM]->getScaledValue()` before the module is fully initialized could return 0 or garbage.
**Why it happens:** During the first few `process()` calls, the parameter system may not be fully settled.
**How to avoid:** The frequency override is gated by `smoothedPeriod > 0.f`, which is only true after the second clock edge. By that point, the module is long past initialization. This is a non-issue in practice but worth noting.
**Warning signs:** N/A -- the clock state gate prevents this naturally.

### Pitfall 3: Tooltip Flicker During Clock State Transitions
**What goes wrong:** The tooltip rapidly switches between Hz and ratio display as the clock state transitions through ACQUIRING.
**Why it happens:** The GUI thread reads `displayRatioIndex` which changes as the clock state machine transitions.
**How to avoid:** Set `displayRatioIndex` to -1 (free-running) when in FREE state, and only set it to a valid ratio index when `smoothedPeriod > 0` AND the clock state is ACQUIRING or LOCKED. The override conditional already does this correctly.
**Warning signs:** Tooltip text flickering between "0.70 Hz" and "x1 (synced)" when first connecting a clock.

### Pitfall 4: Frequency Discontinuity When Clock Disconnects
**What goes wrong:** When clock is disconnected, frequency jumps from clock-derived value (e.g., 2 Hz at x1 with 120 BPM clock) to knob value (e.g., 0.7 Hz default).
**Why it happens:** The knob value was never changed during clocked operation -- it was just ignored.
**How to avoid:** This is expected and correct per CONTEXT.md: "When clock disconnects, instant restore to Hz meaning." Phase 9 will add smooth slew for this transition. In Phase 8, the jump is intentional.
**Warning signs:** This is NOT a bug. Do not add slew to "fix" it.

### Pitfall 5: Division-by-Zero with smoothedPeriod
**What goes wrong:** `1.f / smoothedPeriod` is undefined when `smoothedPeriod` is 0.
**Why it happens:** If the gate `smoothedPeriod > 0.f` is not checked before computing `1.f / smoothedPeriod`.
**How to avoid:** The conditional `(clockState == ACQUIRING || clockState == LOCKED) && smoothedPeriod > 0.f` ensures we never divide by zero. This gate is mandatory.
**Warning signs:** NaN or Inf in the frequency computation.

## Code Examples

### Complete Ratio Table and Labels

```cpp
// Source: CONTEXT.md locked decisions
// 15 musical ratios as frequency multipliers relative to clock frequency
static constexpr float RATIO_TABLE[15] = {
    1.f/16.f,   // /16  = 0.0625
    1.f/8.f,    // /8   = 0.125
    1.f/6.f,    // /6   = 0.166667
    1.f/4.f,    // /4   = 0.25
    1.f/3.f,    // /3   = 0.333333
    1.f/2.f,    // /2   = 0.5
    2.f/3.f,    // /1.5 = 0.666667
    1.f,        // x1   = 1.0
    3.f/2.f,    // x1.5 = 1.5
    2.f,        // x2   = 2.0
    3.f,        // x3   = 3.0
    4.f,        // x4   = 4.0
    6.f,        // x6   = 6.0
    8.f,        // x8   = 8.0
    16.f        // x16  = 16.0
};

static constexpr const char* RATIO_LABELS[15] = {
    "/16", "/8", "/6", "/4", "/3", "/2", "/1.5",
    "x1",
    "x1.5", "x2", "x3", "x4", "x6", "x8", "x16"
};
```

**Note:** Using `1.f/6.f` and `2.f/3.f` expressions for clarity. The compiler evaluates these at compile time.

### Normalized Knob Position Extraction

```cpp
// Source: VCV Rack 2 SDK Quantity.hpp (verified in local SDK)
// Quantity::getScaledValue() returns (value - minValue) / (maxValue - minValue)
// For RATE_PARAM: minValue=0.01, maxValue=20.0
// At value=0.01: returns ~0.0
// At value=10.005: returns ~0.5
// At value=20.0: returns 1.0

float knobNormalized = paramQuantities[RATE_PARAM]->getScaledValue();
int ratioIdx = (int)std::round(knobNormalized * 14.f);
ratioIdx = rack::math::clamp(ratioIdx, 0, 14);
```

### Complete RateParamQuantity Nested Struct

```cpp
// Source: VCV Rack 2 SDK ParamQuantity.hpp pattern (verified)
struct RateParamQuantity : ParamQuantity {
    std::string getDisplayValueString() override {
        AnalogLFO* lfo = static_cast<AnalogLFO*>(module);
        if (!lfo) return ParamQuantity::getDisplayValueString();

        int ratioIdx = lfo->displayRatioIndex.load(std::memory_order_relaxed);
        if (ratioIdx < 0) {
            // Free-running: use default Hz formatting
            return ParamQuantity::getDisplayValueString();
        }

        // Clocked: return ratio label
        return std::string(RATIO_LABELS[ratioIdx]);
    }

    std::string getUnit() override {
        AnalogLFO* lfo = static_cast<AnalogLFO*>(module);
        if (!lfo) return ParamQuantity::getUnit();

        int ratioIdx = lfo->displayRatioIndex.load(std::memory_order_relaxed);
        if (ratioIdx < 0) {
            return " Hz";
        }
        return " (synced)";
    }
};
```

**Tooltip output examples:**
- Free-running, knob at 0.7: `"Rate: 0.70000 Hz"` (VCV default formatting with displayPrecision=5)
- Clocked, ratio x4: `"Rate: x4 (synced)"`
- Clocked, ratio /1.5: `"Rate: /1.5 (synced)"`
- Clocked, ratio x1: `"Rate: x1 (synced)"`

### Constructor Change

```cpp
// Replace this line:
// configParam(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz");
// With:
configParam<RateParamQuantity>(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz");
```

The template argument tells `configParam` to create a `RateParamQuantity` instead of a plain `ParamQuantity`. The SDK sets `q->ParamQuantity::module = this` automatically (verified in SDK source at Module.hpp line 131).

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Separate sync mode parameter | Dual-mode knob with contextual meaning | Standard in modern Eurorack (Mutable Instruments, etc.) | Fewer controls, more intuitive UX |
| Continuous (non-snapped) sync ratios | Discrete snapped musical ratios | Industry consensus | Non-snapped produces non-musical results |
| Fixed tooltip text | Dynamic `ParamQuantity` subclass | VCV Rack 2.0 SDK (2022) | Clean separation of display from DSP |
| Manual string formatting for tooltips | Override `getDisplayValueString()` + `getUnit()` | VCV Rack 2.0 SDK pattern | Framework handles `getString()` assembly |

## Open Questions

1. **Display precision in free-running mode**
   - What we know: VCV Rack default `displayPrecision` is 5, which gives "0.70000 Hz". The original `configParam` used `" Hz"` as unit.
   - What's unclear: Whether 5 decimal places looks good or if we should reduce to 2-3 for cleaner display.
   - Recommendation: Leave at default (5) for now. This matches v1.0 behavior exactly. If the display looks cluttered, reduce precision in Phase 10 (display polish). Not a Phase 8 concern.

2. **Ratio selection during ACQUIRING with no smoothedPeriod**
   - What we know: Between clock edge 1 and edge 2, `smoothedPeriod` is 0 and `clockState` is ACQUIRING.
   - What's unclear: Whether the tooltip should show a ratio label during this brief window.
   - Recommendation: No. The gate `smoothedPeriod > 0.f` means no ratio override occurs until edge 2. The tooltip shows Hz during this window. This is correct -- we don't know the clock tempo yet, so showing a ratio would be misleading.

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
| RATE-01 | Rate knob behaves identically to v1.0 when CLK disconnected | manual | `make` (build check) | N/A |
| RATE-02 | Rate knob selects ratio when CLK connected | manual | Patch clock, turn Rate knob, verify LFO frequency changes in discrete steps | N/A |
| RATE-03 | 15 discrete snapped ratios from /16 to x16 | manual | Sweep Rate knob fully while clocked, count distinct frequency steps | N/A |
| RATE-06 | Custom tooltip shows ratio label when clocked | manual | Hover over Rate knob while clocked, verify tooltip shows e.g. "x4 (synced)" | N/A |

### Sampling Rate
- **Per task commit:** `make` (build verification)
- **Per wave merge:** Full manual test: patch clock, sweep Rate knob, verify tooltip, disconnect clock, verify Hz mode
- **Phase gate:** All 4 requirements verified manually before proceeding to Phase 9

### Wave 0 Gaps
None -- this is a C++ VCV Rack module without an established automated test framework. All testing is manual verification in the running application. The `make` build check serves as the automated smoke test.

## Sources

### Primary (HIGH confidence)
- VCV Rack 2 SDK source: `include/engine/ParamQuantity.hpp` -- ParamQuantity struct definition, `getDisplayValueString()` and `getUnit()` virtual methods, SwitchQuantity subclass example (verified in local SDK at `/Users/mrcbrown/Claude/Software/Forge Audio/Rack-SDK/include/engine/ParamQuantity.hpp`)
- VCV Rack 2 SDK source: `include/engine/Module.hpp` -- `configParam<TParamQuantity>()` template signature, automatic `module` pointer assignment at line 131 (verified in local SDK)
- VCV Rack 2 SDK source: `include/Quantity.hpp` -- `getScaledValue()`, `toScaled()`, `getString()` methods (verified in local SDK)
- Existing codebase: `src/AnalogLFO.cpp` -- Phase 7 clock state machine, atomic display pattern, process() structure, configParam call at line 293

### Secondary (MEDIUM confidence)
- [VCV Rack API: ParamQuantity](https://vcvrack.com/docs-v2/structrack_1_1engine_1_1ParamQuantity) -- online API reference (cross-verified with local SDK headers)
- [VCV Rack API: Module](https://vcvrack.com/docs-v2/structrack_1_1engine_1_1Module) -- configParam template documentation

### Tertiary (LOW confidence)
- None. All findings verified against local SDK source code.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- SDK APIs verified against actual header source code in local Rack-SDK
- Architecture: HIGH -- All decisions locked in CONTEXT.md; patterns verified against SDK; nested struct approach validated against C++ language rules
- Pitfalls: HIGH -- Analyzed against existing source code and SDK behavior; circular reference pitfall verified by examining type requirements
- Code examples: HIGH -- Compiled from verified SDK APIs, locked CONTEXT.md decisions, and existing codebase patterns

**Research date:** 2026-03-07
**Valid until:** Indefinitely (VCV Rack 2 SDK ParamQuantity API is stable; no breaking changes expected)
