# Phase 16: Swing and Shuffle - Research

**Researched:** 2026-03-17
**Domain:** LFO phase distortion (swing/shuffle timing), VCV Rack context menus, display rendering
**Confidence:** HIGH

## Summary

Phase 16 adds MPC-style swing timing to the clocked LFO by modifying the deltaPhase accumulation rate based on phase position within each cycle. The first half of each cycle (phase 0-0.5) accumulates slower and the second half (phase 0.5-1.0) accumulates faster, creating the characteristic long-short beat pair pattern. The implementation requires changes in four areas: (1) a deltaPhase multiplier in the audio `process()` path gated by `isClocked`, (2) a right-click context menu using VCV Rack's `appendContextMenu` pattern with `createIndexSubmenuItem`, (3) serialization of the swing index via the existing `dataToJson`/`dataFromJson` pattern, and (4) display modifications for swing text overlay and phase-warped waveform rendering.

The swing math is straightforward and well-understood. All integration points exist in the current codebase with clear precedent patterns. The right-click menu is the module's first `appendContextMenu` override, but VCV Rack's helper functions make this a simple addition.

**Primary recommendation:** Implement swing as a deltaPhase multiplier applied after existing drift/jitter modifications but before `phase += deltaPhase`, using the formula `multiplier = 0.5 / S` for even beats and `0.5 / (1-S)` for odd beats, where S is the swing fraction (0.5-0.75).

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- MPC-style beat pairs: alternate beats get different durations (even = long, odd = short)
- Swing modifies deltaPhase (phase accumulation rate), not readout
- Even beat accumulates phase slower (takes more time), odd beat accumulates faster (takes less time)
- Within each LFO cycle: first half = even beat (long), second half = odd beat (short)
- Beat parity determined by phase position within cycle (0->0.5 = even, 0.5->1.0 = odd)
- Range: 50% (straight) through 75% (extreme dotted-note swing)
- At 50%: both halves equal duration -- identical to v1.1 behavior
- At 66%: classic triplet swing -- even beat takes 2/3 of cycle time, odd beat takes 1/3
- Swing always warps the individual LFO cycle regardless of ratio
- At x2: each of the 2 LFO cycles per clock period is independently swung
- At x/2: the single LFO cycle spanning 2 clock beats is swung within itself
- Consistent model: swing = "first half of each cycle is long, second half is short"
- No special cases for different ratios -- swing is a per-cycle phase distortion
- Swing and Phase Offset are independent layers (Swing modifies deltaPhase, Phase Offset applied at readout)
- First context menu on this module (appendContextMenu override on AnalogLFOWidget)
- 6 named presets: Straight (50%), Light (54%), Medium (58%), Triplet (66%), Heavy (71%), Max (75%)
- Checkmark next to active preset (standard VCV menu pattern)
- Swing setting serialized via dataToJson/dataFromJson
- Default: Straight (50%) -- no swing on fresh instances
- Show swing preset name + percentage in display when swing > 50%
- Uses Phase 11 pill-background pattern for readability
- Positioned bottom-left of display area
- Hidden at 50% (straight) -- no clutter when swing is off
- Only visible in clocked mode
- Follows existing 200ms fade animation pattern for show/hide
- Display shows asymmetric cycle reflecting swing timing
- 256 display samples distributed non-uniformly
- Phase dot position reflects swing-warped timing
- At 50% swing: uniform distribution -- display identical to v1.1

### Claude's Discretion
- Exact deltaPhase multiplier formula for even/odd beats within the 50-75% range
- Whether swing phase boundary (0.5) needs smoothing to avoid discontinuity at the even/odd transition
- Exact display sample redistribution algorithm for phase-warped waveform
- Swing text overlay font size and exact pill dimensions (match existing overlay style)
- Whether swing overlay should dim like CLK line (60% alpha) or full brightness
- Implementation of appendContextMenu (VCV Rack MenuItem/createSubmenu patterns)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| PHASE-03 | Swing/shuffle control warps phase progression within beat pairs in clocked mode | DeltaPhase multiplier formula, swing preset table, appendContextMenu pattern, serialization, display overlay |
| PHASE-04 | Swing inactive in free-running mode | `isClocked` gate on deltaPhase multiplier, swing overlay only shown in clocked mode |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack 2 SDK | v2.x | Module framework, context menu API | Project platform |
| NanoVG | (bundled) | Display rendering for swing overlay | Already used for all display rendering |

### Supporting
No additional libraries needed. All swing functionality implemented with existing SDK facilities.

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `createIndexSubmenuItem` | Manual MenuItem subclass with checkmarks | Helper does the same with less code; use the helper |
| deltaPhase multiplier | Phase readout remapping | CONTEXT.md locks deltaPhase approach; readout remapping would not interact correctly with drift/jitter |

**Installation:**
No new dependencies required.

## Architecture Patterns

### Integration Point Map

```
process() [AnalogLFO.cpp:560]
  |
  +-- isClocked determination [line 568]
  |
  +-- deltaPhase calculation [line 625]
  |     double deltaPhase = (double)freq * (double)args.sampleTime;
  |
  +-- drift modifies deltaPhase [lines 628-662]
  |     deltaPhase *= (1.0 + drift...)
  |     deltaPhase *= (1.0 + jitter...)
  |
  +-- >>> NEW: swing modifies deltaPhase (AFTER drift, BEFORE phase accumulation)
  |     if (isClocked && swingFraction > 0.5f) {
  |         double multiplier = (phase < 0.5) ? (0.5 / S) : (0.5 / (1.0 - S));
  |         deltaPhase *= multiplier;
  |     }
  |
  +-- phase += deltaPhase [line 664]
  +-- phase wrap [line 665]
```

### Pattern 1: Swing DeltaPhase Multiplier Formula

**What:** Modify the phase accumulation rate based on which half of the cycle the phase is currently in.

**When to use:** Every sample in clocked mode when swing > 50%.

**Math derivation:**

Let S = swing fraction (0.5 to 0.75). S represents the fraction of total cycle TIME allocated to the first half (phase 0 to 0.5).

- First half (even beat): phase must traverse 0.5 in S fraction of the time
  - Rate multiplier = `0.5 / S` (slower than 1.0 when S > 0.5)
- Second half (odd beat): phase must traverse 0.5 in (1-S) fraction of the time
  - Rate multiplier = `0.5 / (1.0 - S)` (faster than 1.0 when S > 0.5)

**Verification:**

| Swing | S | Even multiplier | Odd multiplier | Even duration | Odd duration | Total |
|-------|---|-----------------|----------------|---------------|--------------|-------|
| 50% | 0.50 | 1.000 | 1.000 | 0.500 | 0.500 | 1.000 |
| 54% | 0.54 | 0.926 | 1.087 | 0.540 | 0.460 | 1.000 |
| 58% | 0.58 | 0.862 | 1.190 | 0.580 | 0.420 | 1.000 |
| 66% | 0.66 | 0.758 | 1.471 | 0.660 | 0.340 | 1.000 |
| 71% | 0.71 | 0.704 | 1.724 | 0.710 | 0.290 | 1.000 |
| 75% | 0.75 | 0.667 | 2.000 | 0.750 | 0.250 | 1.000 |

Total always sums to 1.0 -- cycle period is perfectly preserved at all swing settings.

At S=0.5: both multipliers are exactly 1.0 -- zero-cost passthrough (no floating point drift).

**Code:**
```cpp
// Swing: modify deltaPhase for clocked mode (PHASE-03)
// swingFraction: 0.5 (straight) to 0.75 (max swing)
if (isClocked && swingFraction > 0.5001f) {
    // Even beat (first half): slower accumulation
    // Odd beat (second half): faster accumulation
    double swingMul = (phase < 0.5)
        ? (0.5 / (double)swingFraction)
        : (0.5 / (1.0 - (double)swingFraction));
    deltaPhase *= swingMul;
}
```

**Confidence:** HIGH -- pure arithmetic identity, verified at all preset values.

### Pattern 2: Phase Boundary Transition (0.5 crossing)

**What:** When phase crosses 0.5, the deltaPhase multiplier changes discontinuously.

**Analysis:** This is NOT a problem requiring smoothing.

- The multiplier changes the RATE of phase advancement, not the output value
- The waveform output is continuous (computed from phase, which is continuous)
- MPC/Akai swing works identically -- instant rate change at beat boundary
- Smoothing would blur the swing feel and defeat the purpose
- At S=0.5 there is no discontinuity at all

**Recommendation:** No smoothing needed at the 0.5 boundary. Apply the multiplier based on a simple `phase < 0.5` test.

**Confidence:** HIGH -- matches how all classic drum machines implement swing.

### Pattern 3: VCV Rack appendContextMenu with createIndexSubmenuItem

**What:** Add a right-click context menu to AnalogLFOWidget with 6 swing presets.

**VCV Rack provides `createIndexSubmenuItem`** which:
- Takes a label string, vector of option labels, a getter lambda, and a setter lambda
- Automatically shows a checkmark next to the currently selected item
- Returns a `MenuItem*` to add to the menu

**Code:**
```cpp
// In AnalogLFOWidget struct:
void appendContextMenu(Menu* menu) override {
    AnalogLFO* module = getModule<AnalogLFO>();

    menu->addChild(new MenuSeparator);
    menu->addChild(createMenuLabel("Swing"));

    menu->addChild(createIndexSubmenuItem("Swing",
        {"Straight 50%", "Light 54%", "Medium 58%", "Triplet 66%", "Heavy 71%", "Max 75%"},
        [=]() { return (size_t)module->swingIndex; },
        [=](size_t idx) { module->swingIndex = (int)idx; }
    ));
}
```

**Confidence:** HIGH -- based on official VCV Rack Plugin API Guide documentation.

### Pattern 4: Swing Serialization

**What:** Persist swing setting across patch save/load using existing dataToJson/dataFromJson.

**Code:**
```cpp
// In dataToJson():
json_object_set_new(rootJ, "swingIndex", json_integer(swingIndex));

// In dataFromJson():
json_t* swingJ = json_object_get(rootJ, "swingIndex");
if (swingJ)
    swingIndex = json_integer_value(swingJ);
```

**Note:** Unlike spread seed (which needed hex string encoding for uint64_t), swingIndex is a small int (0-5) and json_integer is perfectly safe.

**Confidence:** HIGH -- follows exact pattern from Phase 14; int values have no sign issues.

### Pattern 5: Display Phase-Warped Waveform

**What:** Modify `updateDisplayBuffer()` to show the swing-warped waveform timing.

**Current code** (line 313-320):
```cpp
void updateDisplayBuffer(float morph, float character) {
    int writeIdx = 1 - displayReadIdx.load(std::memory_order_relaxed);
    for (int i = 0; i < DISPLAY_SAMPLES; i++) {
        float p = (float)i / (float)DISPLAY_SAMPLES;
        displayBuffers[writeIdx][i] = computeMorphedWave(p, morph, character);
    }
    displayReadIdx.store(writeIdx, std::memory_order_release);
}
```

**Swing-warped version:** The x-axis represents TIME (uniform), and we map time -> phase using the inverse swing function.

Given time fraction t in [0,1] and swing fraction S:
- If t < S: phase = t * 0.5 / S
- If t >= S: phase = 0.5 + (t - S) * 0.5 / (1.0 - S)

At S=0.5 this simplifies to phase = t (identity). The display x-axis stays uniform (256 evenly spaced time samples), but each sample's phase lookup is warped.

**Code:**
```cpp
void updateDisplayBuffer(float morph, float character, float swingFrac) {
    int writeIdx = 1 - displayReadIdx.load(std::memory_order_relaxed);
    for (int i = 0; i < DISPLAY_SAMPLES; i++) {
        float t = (float)i / (float)DISPLAY_SAMPLES;  // uniform time
        float p;
        if (swingFrac <= 0.5001f) {
            p = t;  // no swing: phase = time (fast path)
        } else if (t < swingFrac) {
            p = t * 0.5f / swingFrac;                          // even: [0,S) -> [0,0.5)
        } else {
            p = 0.5f + (t - swingFrac) * 0.5f / (1.f - swingFrac); // odd: [S,1) -> [0.5,1)
        }
        displayBuffers[writeIdx][i] = computeMorphedWave(p, morph, character);
    }
    displayReadIdx.store(writeIdx, std::memory_order_release);
}
```

**Visual effect:** First half of display is stretched (more detail), second half compressed. At 66% triplet swing, the first 2/3 of display width shows phase 0-0.5, last 1/3 shows phase 0.5-1.0.

**Confidence:** HIGH -- inverse of the audio-path swing function; verified at boundary values.

### Pattern 6: Phase Dot Swing Mapping

**What:** The display phase dot position must also reflect swing timing.

Currently `displayPhase` is set to `(float)phase + phaseOffset` in process(). The display reads this and uses `phaseToX()` to convert to pixel position. For swing, we need to convert phase -> display time so the dot moves at the correct visual rate.

The forward mapping (phase -> time for display position) is the inverse of the display buffer mapping:
- If phase < 0.5: displayTime = phase * S / 0.5 = phase * 2.0 * S
- If phase >= 0.5: displayTime = S + (phase - 0.5) * (1.0 - S) / 0.5 = S + (phase - 0.5) * 2.0 * (1.0 - S)

At S=0.5: displayTime = phase (identity). Correct.

**Implementation options:**
1. Apply this mapping in the display widget's `drawPhaseDot()` before calling `phaseToX()`
2. Store an additional `displaySwingFraction` atomic so the display widget has the swing info

**Recommendation:** Store `displaySwingFraction` as a relaxed atomic (same pattern as `displayDrift`, `displaySmoothedPeriod`, etc.) and apply the phase-to-time mapping in the display widget.

**Code (in display widget):**
```cpp
// In drawPhaseDot(), after reading phase from atomic:
float swingFrac = module->displaySwingFraction.load(std::memory_order_relaxed);
if (swingFrac > 0.5001f) {
    if (phase < 0.5f)
        phase = phase * 2.f * swingFrac;
    else
        phase = swingFrac + (phase - 0.5f) * 2.f * (1.f - swingFrac);
}
```

**Confidence:** HIGH -- mathematically inverse of display buffer mapping.

### Pattern 7: Swing Text Overlay

**What:** Show swing preset name and percentage in bottom-left of display, with pill background, when swing > 50% and in clocked mode.

**Existing pattern reference:** `drawTextOverlays()` at line 1107 renders Hz (top-left), ratio (top-left), SYNC (top-right), and BPM stack (bottom-right). The swing overlay goes in the bottom-left -- the one remaining corner.

**Code (added to drawTextOverlays):**
```cpp
// Swing overlay (clocked mode, bottom-left, only when swing > 50%)
if (swingFadeAlpha > 0.001f) {
    int swingIdx = module->displaySwingIndex.load(std::memory_order_relaxed);
    if (swingIdx > 0) {  // 0 = Straight (hidden)
        const char* swingLabels[] = {"", "LIGHT 54%", "MEDIUM 58%", "TRIPLET 66%", "HEAVY 71%", "MAX 75%"};
        drawPillText(vg, font->handle, margin, box.size.y - margin,
                     swingLabels[swingIdx], fontSize,
                     NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM, swingFadeAlpha);
    }
}
```

**Font size:** 10px, matching all other overlays (Hz, ratio, BPM).

**Alpha:** Full brightness (same as Hz readout and BPM line), not dimmed like the supplemental CLK line. The swing overlay is primary information, not supplemental context.

**Fade animation:** Add `swingFadeAlpha` to the existing fade system in `step()`:
- Show when: `isClocked && swingIndex > 0`
- Uses same 200ms fade speed as other overlays

**Confidence:** HIGH -- follows established pill/overlay patterns from Phase 11.

### Anti-Patterns to Avoid
- **Modifying phase readout instead of deltaPhase:** CONTEXT.md explicitly locks deltaPhase modification. Readout modification would interact badly with Phase Offset (which is also readout-based) and would not affect drift/jitter behavior correctly.
- **Resetting phase at swing boundary:** Never reset or discontinuously jump phase at the 0.5 boundary. Phase must remain continuous; only the accumulation RATE changes.
- **Per-ratio swing special cases:** No ratio-specific logic. Swing always warps the individual LFO cycle regardless of whether the ratio is x1, x2, /4, etc. This was a resolved blocker in STATE.md.
- **Smoothing the swing boundary transition:** The rate change at 0.5 is intentional and matches real MPC behavior. Smoothing would dilute the groove feel.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Context menu with checkmarks | Custom MenuItem subclass | `createIndexSubmenuItem()` | VCV SDK helper handles checkmarks, submenu display, and selection automatically |
| Fade animation | Custom timer system | Existing fadeAlpha pattern in `step()` | Already implemented for syncFadeAlpha, ratioFadeAlpha, etc. |
| Text with pill background | Custom rect + text rendering | Existing `drawPillText()` | Already handles measurement, padding, feathered gradient, 2-pass glow |
| JSON serialization | Custom format | `json_integer()` / `json_integer_value()` | Standard jansson API, same as Rack uses internally |

## Common Pitfalls

### Pitfall 1: Swing Multiplier at Exactly S=0.5
**What goes wrong:** Checking `swingFraction > 0.5f` with floating point may result in applying a near-1.0 multiplier unnecessarily, wasting cycles.
**Why it happens:** Float comparison edge cases.
**How to avoid:** Use a threshold: `swingFraction > 0.5001f` or check `swingIndex > 0` instead of comparing floats.
**Warning signs:** Unnecessary computation in non-swing mode.

### Pitfall 2: Division by Zero at S=1.0 or S=0.0
**What goes wrong:** `0.5 / (1.0 - S)` would divide by zero if S reaches 1.0.
**Why it happens:** If clamping/validation is missing.
**How to avoid:** Swing presets are fixed at 50-75% (never 100%). Clamp S to [0.5, 0.75] as a safety measure. The preset index approach (0-5 mapping to fixed values) makes this impossible by construction.
**Warning signs:** NaN or Inf in deltaPhase.

### Pitfall 3: Display Buffer Not Updated When Swing Changes
**What goes wrong:** Waveform display remains straight after changing swing.
**Why it happens:** `updateDisplayBuffer()` is only called on phase wrap or param change; swing change via menu doesn't trigger it.
**How to avoid:** After swing index changes (in the setter lambda or via a flag), force a display buffer refresh. Could set a flag that process() checks, or track `prevSwingIndex` like `prevDisplayMorph`.
**Warning signs:** Visual mismatch between display and audio after changing swing setting.

### Pitfall 4: Swing Overlay Showing in Free-Running Mode
**What goes wrong:** Swing text appears even though swing has no effect.
**Why it happens:** Fade target checks only swing index, not clocked state.
**How to avoid:** Swing overlay fade target: `bool showSwing = isClocked && swingIndex > 0`. Both conditions required.
**Warning signs:** Misleading UI suggesting swing is active when it has no audio effect.

### Pitfall 5: Phase Dot Desyncing from Waveform at Swing
**What goes wrong:** Phase dot position doesn't match the swing-warped waveform trace.
**Why it happens:** Display buffer uses time->phase mapping but dot still uses raw phase for x-position.
**How to avoid:** Apply the inverse mapping (phase->time) to the display phase before computing dot x-position.
**Warning signs:** Dot appears to slide along waveform at wrong speed in swung mode.

### Pitfall 6: Swing Multiplier Applied in Wrong Order
**What goes wrong:** Swing multiplier interacts incorrectly with drift/jitter.
**Why it happens:** Applying swing before drift means drift modifies the swing ratio; applying after means drift and swing are independent.
**How to avoid:** Apply swing AFTER drift/jitter modifications. Both are multipliers on deltaPhase, so order doesn't mathematically matter for the audio result (multiplication is commutative), but conceptually swing is a timing structure that drift perturbs. Apply swing last before `phase += deltaPhase`.
**Warning signs:** None (mathematically equivalent), but placing after drift follows the conceptual model better.

## Code Examples

### Complete Swing Data Members
```cpp
// Source: derived from existing module patterns in AnalogLFO.cpp

// Swing presets: index into SWING_FRACTIONS table
static constexpr float SWING_FRACTIONS[6] = {
    0.50f,   // Straight
    0.54f,   // Light
    0.58f,   // Medium
    0.66f,   // Triplet (2/3)
    0.71f,   // Heavy
    0.75f    // Max (3/4)
};

static constexpr const char* SWING_LABELS[6] = {
    "Straight 50%", "Light 54%", "Medium 58%",
    "Triplet 66%", "Heavy 71%", "Max 75%"
};

// Overlay labels (shorter, for display)
static constexpr const char* SWING_OVERLAY_LABELS[6] = {
    "",             // hidden at Straight
    "LIGHT 54%",
    "MEDIUM 58%",
    "TRIPLET 66%",
    "HEAVY 71%",
    "MAX 75%"
};

int swingIndex = 0;  // 0=Straight (default), persisted

// Display bridge atomics
std::atomic<int> displaySwingIndex{0};
std::atomic<float> displaySwingFraction{0.5f};
```

### Complete Swing in process()
```cpp
// Source: derived from existing deltaPhase modification pattern (lines 625-662)

// After drift/jitter modify deltaPhase, before phase accumulation:

// Swing: warp phase timing for clocked mode (PHASE-03)
float swingFrac = SWING_FRACTIONS[swingIndex];
if (isClocked && swingFrac > 0.5001f) {
    double swingMul = (phase < 0.5)
        ? (0.5 / (double)swingFrac)
        : (0.5 / (1.0 - (double)swingFrac));
    deltaPhase *= swingMul;
}

// Update display atomics
displaySwingIndex.store(swingIndex, std::memory_order_relaxed);
displaySwingFraction.store(swingFrac, std::memory_order_relaxed);

phase += deltaPhase;
if (phase >= 1.0) phase -= 1.0;
```

### Complete appendContextMenu
```cpp
// Source: VCV Rack Plugin API Guide (https://vcvrack.com/manual/PluginGuide)

struct AnalogLFOWidget : ModuleWidget {
    // ... existing constructor ...

    void appendContextMenu(Menu* menu) override {
        AnalogLFO* module = getModule<AnalogLFO>();

        menu->addChild(new MenuSeparator);

        menu->addChild(createIndexSubmenuItem("Swing",
            {"Straight 50%", "Light 54%", "Medium 58%",
             "Triplet 66%", "Heavy 71%", "Max 75%"},
            [=]() { return (size_t)module->swingIndex; },
            [=](size_t idx) { module->swingIndex = (int)idx; }
        ));
    }
};
```

### Complete Serialization Addition
```cpp
// Source: existing dataToJson/dataFromJson pattern (lines 539-558)

json_t* dataToJson() override {
    json_t* rootJ = json_object();
    // Existing: spread seed
    char buf[32];
    snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)spreadSeed[0]);
    json_object_set_new(rootJ, "spreadSeed0", json_string(buf));
    snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)spreadSeed[1]);
    json_object_set_new(rootJ, "spreadSeed1", json_string(buf));
    // NEW: swing index
    json_object_set_new(rootJ, "swingIndex", json_integer(swingIndex));
    return rootJ;
}

void dataFromJson(json_t* rootJ) override {
    // Existing: spread seed
    json_t* s0J = json_object_get(rootJ, "spreadSeed0");
    json_t* s1J = json_object_get(rootJ, "spreadSeed1");
    if (s0J && s1J) {
        spreadSeed[0] = std::stoull(json_string_value(s0J), nullptr, 16);
        spreadSeed[1] = std::stoull(json_string_value(s1J), nullptr, 16);
        initComponentSpread();
    }
    // NEW: swing index
    json_t* swingJ = json_object_get(rootJ, "swingIndex");
    if (swingJ)
        swingIndex = rack::math::clamp((int)json_integer_value(swingJ), 0, 5);
}
```

### Complete Display Time-to-Phase Mapping
```cpp
// Source: inverse of audio-path swing formula

// In updateDisplayBuffer():
void updateDisplayBuffer(float morph, float character, float swingFrac) {
    int writeIdx = 1 - displayReadIdx.load(std::memory_order_relaxed);
    for (int i = 0; i < DISPLAY_SAMPLES; i++) {
        float t = (float)i / (float)DISPLAY_SAMPLES;
        float p;
        if (swingFrac <= 0.5001f) {
            p = t;  // fast path: no swing
        } else if (t < swingFrac) {
            p = t * 0.5f / swingFrac;
        } else {
            p = 0.5f + (t - swingFrac) * 0.5f / (1.f - swingFrac);
        }
        displayBuffers[writeIdx][i] = computeMorphedWave(p, morph, character);
    }
    displayReadIdx.store(writeIdx, std::memory_order_release);
}
```

### Complete Phase-to-Time Mapping for Display Dot
```cpp
// Source: inverse of display buffer mapping

// In WaveformDisplay::drawPhaseDot(), remap phase to display time:
float swingFrac = module->displaySwingFraction.load(std::memory_order_relaxed);
if (swingFrac > 0.5001f) {
    // Map accumulator phase -> visual time for correct dot position
    if (phase < 0.5f)
        phase = phase * 2.f * swingFrac;            // [0,0.5) -> [0,S)
    else
        phase = swingFrac + (phase - 0.5f) * 2.f * (1.f - swingFrac);  // [0.5,1) -> [S,1)
}
// Then use phaseToX(phase) as normal
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Phase readout remapping for swing | DeltaPhase modification | Locked by CONTEXT.md | Swing interacts correctly with drift, jitter, and Phase Offset |
| Continuous swing parameter | Discrete named presets via right-click | Locked by CONTEXT.md | Musically meaningful values, no panel space required |
| Per-ratio swing logic | Per-cycle swing regardless of ratio | Resolved STATE.md blocker | Consistent behavior, no special cases |

## Open Questions

1. **Display buffer update trigger on swing change**
   - What we know: Currently `updateDisplayBuffer()` triggers on phase wrap or morph/character change. Swing changes via right-click menu need to trigger a refresh too.
   - What's unclear: Whether to track `prevSwingIndex` in process() or use a different mechanism.
   - Recommendation: Track `prevSwingIndex` alongside `prevDisplayMorph` / `prevDisplayCharacter`. When swing index changes, trigger display buffer update. This is the cheapest and most consistent approach.

2. **Swing overlay alpha: full or dimmed?**
   - What we know: CLK line uses 60% alpha (supplemental). Hz/BPM/ratio use full alpha (primary).
   - What's unclear: CONTEXT.md doesn't specify.
   - Recommendation: Full brightness (1.0 alpha). Swing is a primary setting the user chose intentionally, not supplemental metadata. It occupies the bottom-left corner with no competition. If it looks too bright in practice, can easily be tuned down.

3. **Thread safety of swingIndex**
   - What we know: `swingIndex` is written from the UI thread (menu callback) and read from the audio thread (process()). This is a single int.
   - What's unclear: Whether a plain int is sufficient or if it needs an atomic.
   - Recommendation: Use `std::atomic<int>` for `swingIndex`. The store from the menu lambda and the load from process() need at least relaxed ordering. The cost is negligible and it follows the project's established pattern of using atomics for any cross-thread data (like `displayClockState`). Alternatively, a plain int with relaxed atomic load/store is fine since it's a single word-sized value -- but using atomic<int> makes the intent explicit.

## Sources

### Primary (HIGH confidence)
- VCV Rack Plugin API Guide (https://vcvrack.com/manual/PluginGuide) -- appendContextMenu, createIndexSubmenuItem, createCheckMenuItem patterns
- VCV Rack v2 API reference (https://vcvrack.com/docs-v2/namespacerack) -- function signatures for menu helpers
- AnalogLFO.cpp source code (local) -- all integration points, existing patterns, line numbers

### Secondary (MEDIUM confidence)
- VCV Rack MenuItem.hpp source (https://vcvrack.com/docs-v2/MenuItem_8hpp_source) -- MenuItem struct definition
- VCV Community forums (https://community.vcvrack.com/t/adding-context-menu-modes-to-your-module/5865) -- community examples of appendContextMenu

### Tertiary (LOW confidence)
None -- all findings verified against primary sources.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- no new dependencies, all VCV Rack SDK built-in
- Architecture (swing math): HIGH -- pure arithmetic, verified at all preset values, total cycle time preserved
- Architecture (context menu): HIGH -- official VCV Rack Plugin API Guide documents `createIndexSubmenuItem`
- Architecture (display): HIGH -- inverse of audio-path formula, verified at boundary values
- Pitfalls: HIGH -- based on actual code inspection and established project patterns

**Research date:** 2026-03-17
**Valid until:** Indefinite -- swing math is timeless; VCV Rack v2 API is stable
