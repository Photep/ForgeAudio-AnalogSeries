# Phase 12: RESET and Phase Offset - Research

**Researched:** 2026-03-14
**Domain:** VCV Rack trigger input handling, phase offset DSP, anti-click crossfade, concurrent trigger arbitration
**Confidence:** HIGH

## Summary

Phase 12 adds two independent features to the LFO: (1) a RESET trigger jack that snaps the phase accumulator to zero with a click-free crossfade, independent of clock state, and (2) a Phase Offset knob+CV that shifts the waveform output by 0-360 degrees. Both features build directly on existing infrastructure -- the crossfade mechanism from Phase 9 (v1.1) and the waveform generation pipeline already in place.

The RESET trigger reuses the same `dsp::SchmittTrigger` class already used for `clockTrigger`, and the same cosine crossfade mechanism (`crossfadeFrom`, `crossfadeProgress`, `crossfadeDuration`). The key new challenge is arbitration between simultaneous CLK and RESET triggers within 1ms -- solved by a blanking window using `dsp::PulseGenerator`. Phase offset is applied at readout (added to phase before waveform lookup), not at the accumulator, following the VCV Fundamental LFO convention and the explicit requirement in PHASE-01 ("applied at waveform readout (not accumulator)").

**Primary recommendation:** Add RESET input with a second SchmittTrigger and a 1ms blanking PulseGenerator for CLK/RESET arbitration. Add Phase Offset param+CV using the standard knob+attenuator+CV pattern already established for Morph/Character/Drift. Apply offset as `float p = fmod((float)phase + offset, 1.f)` at the point where waveform is computed. No new libraries or dependencies needed.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| MOD-03 | Separate RESET trigger jack resets phase independently from CLK, with 1ms blanking to prevent double-resets | Second `dsp::SchmittTrigger` for RESET input; `dsp::PulseGenerator` with 1ms duration for blanking window; RESET triggers crossfade + phase=0 regardless of clockState |
| MOD-04 | RESET uses existing anti-click cosine crossfade | Reuses `crossfadeFrom`, `crossfadeProgress`, `crossfadeDuration` members; same cosine crossfade code path at lines 523-531 |
| PHASE-01 | Phase offset knob shifts waveform phase 0-360 degrees, applied at waveform readout (not accumulator) | Offset computed as normalized [0,1] from knob+CV, added to phase before `computeMorphedWave()` call; display dot and display buffer both use offset-inclusive phase |
| PHASE-02 | Phase offset CV input for external modulation | Standard attenuverter+CV pattern (bipolar CV, unipolar knob, additive); enables quadrature patches with constant 2.5V into CV (90/360 * 5V = 1.25V... or with attenuator scaling) |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack 2 SDK | bundled | All DSP, trigger detection, filtering | Already the project's sole dependency |
| `dsp::SchmittTrigger` | SDK | Rising-edge detection for RESET input | Same class already used for `clockTrigger` at line 91; proven thresholds (0.1V low, 1.0V high) |
| `dsp::PulseGenerator` | SDK | 1ms blanking window for CLK/RESET arbitration | Lightweight timer; `trigger(0.001f)` starts window, `process(sampleTime)` returns true while active |

### Supporting
| API | Purpose | When to Use |
|-----|---------|-------------|
| `dsp::TExponentialFilter` | Smooth phase offset changes to prevent clicks | Only if needed -- offset at readout is inherently smooth for continuous waveforms; may not be necessary |
| `rack::math::clamp` | Bound phase offset knob+CV sum to [0,1] | Standard pattern for all knob+CV combinations in codebase |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| PulseGenerator blanking | Sample counter (int) | PulseGenerator is sample-rate-independent; counter requires sampleRate lookup |
| SchmittTrigger (0.1V/1.0V) | BooleanTrigger | SchmittTrigger handles analog voltage with hysteresis; BooleanTrigger is for booleans only |
| Phase offset at readout | Phase offset at accumulator | Accumulator offset would shift the reset point, break clock alignment, and cause phase discontinuities -- requirement explicitly says "at readout" |

**No installation needed.** All APIs available through existing `#include "plugin.hpp"`.

## Architecture Patterns

### Pattern 1: RESET Trigger Processing

**What:** A second SchmittTrigger detects rising edges on the RESET input, triggering phase reset with crossfade, independent of clock state.

**When to use:** Every sample in `process()`, after `processClockInput()` but before phase accumulation.

**Example:**
```cpp
// New members in AnalogLFO struct:
dsp::SchmittTrigger resetTrigger;
dsp::PulseGenerator resetBlanking;  // 1ms blanking window

// In process(), after processClockInput():
void processResetInput(float sampleTime) {
    if (!inputs[RESET_INPUT].isConnected()) return;

    float resetVoltage = inputs[RESET_INPUT].getVoltage();
    if (resetTrigger.process(resetVoltage, 0.1f, 1.0f)) {
        // Check blanking window -- skip if CLK just fired
        if (!resetBlanking.process(sampleTime)) {
            // Trigger crossfade and reset phase
            crossfadeFrom = lastOutputVoltage;
            crossfadeProgress = 0.f;
            phase = 0.0;
        }
    } else {
        // Still need to advance blanking timer even when not triggering
        resetBlanking.process(sampleTime);
    }
}
```

**Critical detail:** The blanking window must be set by CLK edges too. When `processClockInput()` performs a phase reset, it should call `resetBlanking.trigger(0.001f)` to start the 1ms blanking window. This prevents a RESET trigger arriving within 1ms of a CLK edge from causing a double-reset.

### Pattern 2: Bidirectional Blanking

**What:** Both CLK and RESET set the same blanking timer, so whichever fires first suppresses the other within the 1ms window.

**When to use:** Whenever CLK or RESET triggers a phase reset.

**Example:**
```cpp
// In processClockInput(), when shouldReset is true:
if (shouldReset) {
    crossfadeFrom = lastOutputVoltage;
    crossfadeProgress = 0.f;
    phase = 0.0;
    clockBeatCount = 0;
    resetBlanking.trigger(0.001f);  // Start blanking window
}

// In processResetInput():
if (resetTrigger.process(resetVoltage, 0.1f, 1.0f)) {
    if (!resetBlanking.isHigh()) {
        crossfadeFrom = lastOutputVoltage;
        crossfadeProgress = 0.f;
        phase = 0.0;
        resetBlanking.trigger(0.001f);  // Start blanking window
        // NOTE: Do NOT reset clockBeatCount or clockTimer here
        // RESET is independent of clock state
    }
}
```

**Critical detail:** RESET must NOT interfere with clock period tracking. It should not reset `clockTimer`, `clockBeatCount`, or `smoothedPeriod`. RESET only resets `phase` and triggers the crossfade. The clock tracker continues running independently.

### Pattern 3: Phase Offset at Readout

**What:** Phase offset is added to the phase value at the point of waveform computation, not to the accumulator. The accumulator always runs 0-1 unmodified.

**When to use:** At the waveform generation step (currently line 518-519 in AnalogLFO.cpp).

**Example:**
```cpp
// Phase offset computation (in process(), before waveform generation):
float offsetKnob = params[PHASE_OFFSET_PARAM].getValue();  // 0..1 representing 0..360 degrees
float offsetCV = 0.f;
if (inputs[PHASE_OFFSET_CV_INPUT].isConnected()) {
    float offsetAtten = params[PHASE_OFFSET_ATTEN_PARAM].getValue();
    offsetCV = offsetAtten * inputs[PHASE_OFFSET_CV_INPUT].getVoltage() / 5.f;
}
float phaseOffset = rack::math::clamp(offsetKnob + offsetCV, 0.f, 1.f);

// Apply offset at readout
float p = (float)phase + phaseOffset;
if (p >= 1.f) p -= 1.f;
float sample = computeMorphedWave(p, morph, character);
```

**Why at readout, not accumulator:**
1. PHASE-01 explicitly requires "applied at waveform readout (not accumulator)"
2. Accumulator offset would shift where clock resets land (phase 0 + offset instead of true phase 0)
3. Changing offset at the accumulator creates discontinuities; at readout, continuous waveforms produce smooth output changes naturally
4. VCV Fundamental LFO uses the same approach (offset subtracted from phase before waveform computation)

### Pattern 4: Display Phase with Offset

**What:** The display dot and display buffer must reflect the offset-inclusive phase so the visual matches the audio output.

**When to use:** When updating `displayPhase` and when computing the display buffer.

**Example:**
```cpp
// Display phase includes offset (so dot position matches output)
float displayP = (float)phase + phaseOffset;
if (displayP >= 1.f) displayP -= 1.f;
displayPhase.store(displayP, std::memory_order_relaxed);

// Display buffer: does NOT include offset (buffer shows waveform shape)
// The dot position (from displayPhase) tracks across the fixed waveform
// This way, changing offset moves the dot along the waveform, showing
// which part of the cycle is currently being output
```

**Design decision:** The display buffer shows the waveform shape (no offset), while the dot position includes offset. This means changing offset slides the dot along the waveform, giving visual feedback of what phase point is being output. The alternative (offset in the buffer too) would show the same dot position but a shifted waveform, which is less intuitive.

### Pattern 5: Phase Offset Attenuverter for Quadrature Patches

**What:** The CV input for phase offset should support bipolar modulation to enable quadrature (90-degree) patches.

**When to use:** For PHASE-02 requirement -- external modulation of phase offset.

**Example quadrature patch:** With two LFOs at the same rate, patch a constant +2.5V (from a DC offset module) into the Phase Offset CV of the second LFO with attenuator at full. This adds 0.5V/5V = 0.1 normalized... No -- the knob range is 0-1 (360 degrees), so for 90 degrees the user sets the second LFO's Phase Offset knob to 0.25 (90/360). The CV input enables modulating this in real time.

**Attenuverter vs attenuator:** The existing codebase uses unipolar attenuators (0-1 range, `Trimpot` widget) for Morph/Character/Drift CV. For Phase Offset CV, the same pattern works. The knob provides the base offset (0-360 degrees), and CV modulates around that point. A standard unipolar attenuator is sufficient because the knob itself covers the full range.

### Anti-Patterns to Avoid

- **Resetting clockTimer or clockBeatCount from RESET:** RESET is independent of clock. Resetting clock state would corrupt period tracking and cause frequency jumps.
- **Applying offset to the phase accumulator:** Creates discontinuities when offset changes, breaks clock alignment, contradicts PHASE-01 requirement.
- **Using separate crossfade state for RESET:** The same crossfade mechanism (`crossfadeFrom`, `crossfadeProgress`) handles both CLK and RESET resets. Using separate state would require merging two simultaneous crossfades -- unnecessary complexity since the blanking window prevents both from firing within 1ms.
- **Smoothing/slewing the offset value before applying:** For continuous waveforms (sine, triangle), changing the phase readout point produces smooth output changes inherently. Slewing the offset would add latency and prevent instant quadrature patches. Only waveforms with discontinuities (square, near-square) could produce a click, but the cosine crossfade on reset already handles the hard-reset case, and gradual knob turns produce gradual output changes even for square waves.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Rising-edge trigger detection | Threshold comparison with state tracking | `dsp::SchmittTrigger` | Handles hysteresis, initialization state, proven thresholds for Eurorack voltage standards |
| 1ms blanking timer | Sample counter with sampleRate math | `dsp::PulseGenerator` | Sample-rate-independent, clean API (`trigger()`, `process()`, `isHigh()`) |
| Smooth param changes | Manual exponential smoothing | `dsp::TExponentialFilter` | Already used for `freqSlew`; but likely not needed for offset since readout-based offset is inherently smooth |

**Key insight:** The existing crossfade infrastructure from Phase 9 handles the entire anti-click requirement for RESET. No new crossfade mechanism is needed -- RESET triggers the exact same crossfade code path that CLK reset does.

## Common Pitfalls

### Pitfall 1: RESET Corrupts Clock Period Tracking
**What goes wrong:** RESET resets `clockTimer` or `clockBeatCount`, causing the next CLK edge to measure an incorrect period, which feeds into EMA smoothing and corrupts the frequency for several cycles.
**Why it happens:** Tempting to "reset everything" when RESET fires, by analogy with the full reset in `processClockInput()`.
**How to avoid:** RESET must ONLY set `phase = 0.0`, capture `crossfadeFrom`, and set `crossfadeProgress = 0.f`. It must not touch any clock tracking state (`clockTimer`, `clockBeatCount`, `smoothedPeriod`, `clockEdgeCount`, `clockState`).
**Warning signs:** LFO frequency wobbles or jumps after a RESET trigger while clocked.

### Pitfall 2: Double-Reset from Simultaneous CLK + RESET
**What goes wrong:** Both CLK and RESET fire on the same sample (or within microseconds), each triggering its own phase reset. The second reset captures `crossfadeFrom` as the post-first-reset value (near-zero), creating an audible glitch or a crossfade from silence.
**Why it happens:** In hardware Eurorack systems, modules driven by the same clock source often have CLK and RESET arriving within microseconds of each other.
**How to avoid:** Bidirectional blanking with `dsp::PulseGenerator`. Whichever fires first (CLK or RESET) starts a 1ms blanking window. The other is suppressed if it arrives within that window. 1ms = 48 samples at 48kHz, well within the "simultaneous" perception threshold.
**Warning signs:** Occasional clicks or weird crossfade artifacts when CLK and RESET are driven by related signals (e.g., same clock divider).

### Pitfall 3: Phase Offset Wrapping Creates Display Discontinuity
**What goes wrong:** When `phase + offset >= 1.0`, the dot position jumps from right edge to left edge of the display, appearing to teleport.
**Why it happens:** The display dot tracks offset-inclusive phase, which wraps around 1.0.
**How to avoid:** This is actually correct behavior -- the dot should wrap. The waveform display shows one full cycle, and the dot's position represents where in that cycle the output currently is. Wrapping is expected and matches how a real oscilloscope trace would behave. The key is that the dot should wrap smoothly (it moves continuously, just past the right edge and back from the left). The existing dot-drawing code already handles phase wrapping for the comet trail (line 673: `if (trailPhase < 0.f) trailPhase += 1.f`).
**Warning signs:** None -- if the dot wraps, that's correct. Only a problem if it stays stuck.

### Pitfall 4: Phase Offset CV Produces Out-of-Range Values
**What goes wrong:** Extreme CV voltages push `offsetKnob + offsetCV` outside [0,1], causing `fmod` to produce negative values or values > 1.
**Why it happens:** CV can be +/-10V, attenuator only limits to 0-1 scaling, knob is 0-1.
**How to avoid:** Clamp the final offset to [0,1] before applying. Since offset represents a circular quantity (0 and 1 are the same point), an alternative is `fmod` with proper handling of negatives, but clamping is simpler and matches the existing pattern for all other knob+CV combinations in the codebase.
**Warning signs:** Waveform output jumps or produces unexpected shapes at extreme CV.

### Pitfall 5: Blanking Window Must Be Advanced Every Sample
**What goes wrong:** `PulseGenerator::process()` is only called when a RESET trigger is detected, so the blanking timer never decrements, and subsequent CLK resets are permanently suppressed.
**Why it happens:** The blanking timer is placed inside an `if (triggered)` block that only executes on trigger edges.
**How to avoid:** Call `resetBlanking.process(sampleTime)` every sample, unconditionally, regardless of whether a trigger was detected. Check `resetBlanking.isHigh()` to determine if blanking is active.
**Warning signs:** After the first RESET trigger, CLK resets stop working entirely.

### Pitfall 6: Display Buffer Should NOT Include Phase Offset
**What goes wrong:** If phase offset is applied when computing the display buffer, the waveform appears to shift horizontally on screen when the offset knob is turned. This is confusing because the waveform shape hasn't changed -- only the output point within the cycle.
**Why it happens:** Display buffer is computed by iterating p=0..1 and calling `computeMorphedWave()`. Adding offset here shifts the entire rendered waveform.
**How to avoid:** Display buffer renders the waveform WITHOUT offset (p = i/256 as it does now). The display DOT position includes offset (via `displayPhase`). This way, turning the offset knob moves the dot along the waveform, giving clear visual feedback of "you're reading a different point in the cycle."
**Warning signs:** Waveform appears to scroll horizontally when offset knob is turned; user confusion about what the knob does.

## Code Examples

### Example 1: New Enum Entries

```cpp
// Source: Follows existing pattern in AnalogLFO.cpp lines 8-31
enum ParamId {
    // ... existing params ...
    PHASE_OFFSET_PARAM,        // 0-1 representing 0-360 degrees
    PHASE_OFFSET_ATTEN_PARAM,  // 0-1 attenuator for CV
    PARAMS_LEN
};
enum InputId {
    // ... existing inputs ...
    RESET_INPUT,
    PHASE_OFFSET_CV_INPUT,
    INPUTS_LEN
};
```

### Example 2: Constructor Configuration

```cpp
// Source: Follows existing configParam pattern at lines 384-395
configParam(PHASE_OFFSET_PARAM, 0.f, 1.f, 0.f, "Phase Offset", " deg", 0.f, 360.f);
configParam(PHASE_OFFSET_ATTEN_PARAM, 0.f, 1.f, 1.f, "Phase Offset CV", "%", 0.f, 100.f);
configInput(RESET_INPUT, "Reset");
configInput(PHASE_OFFSET_CV_INPUT, "Phase Offset CV");
```

Note: `configParam` with display offset `0.f` and display multiplier `360.f` means the tooltip shows "Phase Offset: 90.0 deg" when knob is at 0.25.

### Example 3: PulseGenerator Blanking API

```cpp
// Source: VCV Rack SDK dsp/digital.hpp lines 167-195
// PulseGenerator API:
//   trigger(float duration)  - starts the pulse (1ms = 0.001f)
//   process(float deltaTime) - advances timer, returns true if HIGH
//   isHigh()                 - returns true if pulse is still active
//   reset()                  - immediately ends pulse

dsp::PulseGenerator resetBlanking;

// When CLK fires a reset:
resetBlanking.trigger(0.001f);  // 1ms blanking

// When RESET fires:
if (resetTrigger.process(voltage, 0.1f, 1.0f)) {
    if (!resetBlanking.isHigh()) {
        // Safe to reset -- no recent CLK reset
        crossfadeFrom = lastOutputVoltage;
        crossfadeProgress = 0.f;
        phase = 0.0;
        resetBlanking.trigger(0.001f);  // Also blank future CLK
    }
}

// MUST advance every sample:
resetBlanking.process(args.sampleTime);
```

### Example 4: Phase Offset at Readout (Full Integration)

```cpp
// In process(), replacing lines 517-519:

// Compute phase offset
float offsetKnob = params[PHASE_OFFSET_PARAM].getValue();
float offsetCV = 0.f;
if (inputs[PHASE_OFFSET_CV_INPUT].isConnected()) {
    float offsetAtten = params[PHASE_OFFSET_ATTEN_PARAM].getValue();
    offsetCV = offsetAtten * inputs[PHASE_OFFSET_CV_INPUT].getVoltage() / 5.f;
}
float phaseOffset = rack::math::clamp(offsetKnob + offsetCV, 0.f, 1.f);

// Apply offset at readout
float p = (float)phase + phaseOffset;
if (p >= 1.f) p -= 1.f;
float sample = computeMorphedWave(p, morph, character);

// Display phase includes offset
float displayP = (float)phase + phaseOffset;
if (displayP >= 1.f) displayP -= 1.f;
displayPhase.store(displayP, std::memory_order_relaxed);
```

### Example 5: Widget Placement (Temporary Until Phase 17)

```cpp
// Source: Follows existing widget pattern at lines 1008-1045
// Temporary positions -- Phase 17 (Panel Redesign) will finalize layout
// For now, place RESET jack near CLK jack, offset knob in available space

// RESET jack: near CLK at x=42.96, use y=96 (currently trimpot row area)
// This is a TEMPORARY position. Phase 17 will reorganize the full panel.
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(52.0, 96.0)), module, AnalogLFO::RESET_INPUT));

// Phase Offset: needs knob + attenuator + CV jack
// Temporary placement TBD during planning -- limited panel space
```

**Important:** Panel layout is deferred to Phase 17. Phase 12 must add functional controls with temporary positions that compile and are usable for testing, but the final layout is not decided here.

## State of the Art

| Old Approach (current code) | New Approach (this phase) | Impact |
|------------------------------|---------------------------|--------|
| Only CLK triggers phase reset | Both CLK and RESET trigger phase reset independently | Users can reset LFO independently of clock sync |
| No phase offset | Knob+CV phase offset at readout | Quadrature patches, phase-shifted modulation |
| Single trigger source, no arbitration needed | Bidirectional 1ms blanking window | Prevents double-resets from simultaneous triggers |
| Display dot tracks raw phase | Display dot tracks offset-inclusive phase | Visual feedback matches audio output |

**Nothing deprecated:** All existing CLK sync behavior remains unchanged. RESET is an additive feature. Phase Offset at zero produces identical output to v1.1.

## Open Questions

1. **Phase Offset display interaction with waveform buffer**
   - What we know: Dot should track offset-inclusive phase; buffer should show un-offset waveform
   - What's unclear: Whether the display update trigger logic (line 502-514) needs to account for offset changes causing apparent phase wraps
   - Recommendation: Keep existing display update triggers. Offset changes don't affect the buffer contents (only dot position). The `phaseWrapped` detection on line 502 uses the raw accumulator `phase`, not the offset-inclusive value, so offset changes won't trigger spurious buffer updates. This is correct.

2. **Blanking direction: should RESET also suppress CLK resets?**
   - What we know: Success criteria says "simultaneous triggers on CLK and RESET within 1ms do not cause double-resets"
   - What's unclear: Should the blanking be bidirectional (RESET suppresses subsequent CLK AND CLK suppresses subsequent RESET)?
   - Recommendation: Yes, bidirectional. The success criteria says "do not cause double-resets" regardless of which arrives first. Both directions must be handled. A single `PulseGenerator` shared between both code paths achieves this naturally.

3. **Temporary panel positions for new controls**
   - What we know: Phase 17 handles final panel layout
   - What's unclear: Where to temporarily place RESET jack, Phase Offset knob, Phase Offset attenuator, and Phase Offset CV jack without overlapping existing controls
   - Recommendation: The planner should identify temporary positions that don't collide with existing components (see Panel Spec section 4 for current occupancy). The CLK jack area (x=42.96, y=86) has open space nearby. Alternatively, some controls could be right-click menu items temporarily, but physical jacks must be on the panel.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Manual verification in VCV Rack (DSP behavior) + compile-time check |
| Config file | none -- no automated test framework for VCV Rack modules |
| Quick run command | `cd "/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series" && make -j4` |
| Full suite command | Manual: build, install, load in VCV Rack, test with clock and trigger sources |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| MOD-03 | RESET trigger resets phase independently with 1ms blanking | manual-only | Patch: clock into CLK, same clock into RESET via mult. Verify no double-reset clicks. Then patch independent trigger into RESET. Verify phase resets on trigger. | N/A |
| MOD-04 | RESET uses existing cosine crossfade | manual-only | Patch fast trigger (~10Hz) into RESET with square wave morph at full character. Listen for clicks. Should be click-free. | N/A |
| PHASE-01 | Phase offset knob 0-360 degrees, display dot tracks offset | manual-only | Turn offset knob from 0 to full. Observe dot moving along waveform. Output should shift smoothly. At 0.25 (90 deg), sine output should be cosine-like. | N/A |
| PHASE-02 | Phase offset CV input for quadrature patches | manual-only | Patch constant 1.25V (= 0.25 * 5V) into Phase Offset CV. Verify output is 90 degrees shifted from identical un-offset LFO. Modulate with slow LFO -- verify smooth phase animation. | N/A |

### Sampling Rate
- **Per task commit:** Build compiles without errors (`make -j4`)
- **Per wave merge:** Manual test in VCV Rack with test patch
- **Phase gate:** Full verification of all 5 success criteria in VCV Rack

### Wave 0 Gaps
None -- this phase modifies existing C++ source only. No test framework applies to real-time audio module behavior beyond compile verification. All behavioral verification is manual in VCV Rack.

## Sources

### Primary (HIGH confidence)
- Project source `src/AnalogLFO.cpp` (local, 1048 lines) -- complete current implementation including crossfade mechanism, clock tracking, phase accumulation, display system
- VCV Rack SDK `include/dsp/digital.hpp` (local) -- SchmittTrigger, PulseGenerator, Timer API signatures verified
- VCV Rack SDK `include/dsp/filter.hpp` (local) -- TExponentialFilter API for potential offset smoothing
- VCV Fundamental LFO source (`github.com/VCVRack/Fundamental/blob/v2/src/LFO.cpp`) -- offset at readout pattern, reset trigger handling

### Secondary (MEDIUM confidence)
- `.planning/REQUIREMENTS.md` -- MOD-03, MOD-04, PHASE-01, PHASE-02 requirement definitions
- `.planning/ROADMAP.md` -- Phase 12 success criteria (5 criteria verified)
- `res/PANEL-SPEC.md` -- current panel layout for temporary control placement
- `.planning/phases/11-display-polish/11-RESEARCH.md` -- research format and display system patterns

### Tertiary (LOW confidence)
- VCV community forum discussions on phase offset and reset patterns -- general patterns confirmed but no specific implementation details

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all APIs verified against local SDK headers; no new dependencies
- Architecture: HIGH -- builds on proven crossfade and trigger detection already in codebase; offset-at-readout pattern confirmed by VCV Fundamental LFO and by explicit requirement text
- Pitfalls: HIGH -- double-reset scenario well understood from real Eurorack systems; blanking window is standard approach; display offset interaction analyzed against actual display code

**Research date:** 2026-03-14
**Valid until:** Indefinite -- VCV Rack 2 SDK is stable, project codebase patterns are established
