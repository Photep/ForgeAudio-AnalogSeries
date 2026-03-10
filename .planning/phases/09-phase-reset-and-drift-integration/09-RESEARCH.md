# Phase 9: Phase Reset and Drift Integration - Research

**Researched:** 2026-03-10
**Domain:** Division-aware phase reset, anti-click crossfade, drift authority scaling, frequency slew transitions (VCV Rack 2 SDK)
**Confidence:** HIGH

## Summary

Phase 9 addresses the four remaining requirements that make clock sync feel polished and professional: division-aware phase reset (RATE-04), anti-click crossfade (RATE-05), drift authority reduction in clocked mode (DISP-04), and smooth frequency transitions when connecting/disconnecting CLK (DISP-05). These are all modifications to the existing `process()` flow in `src/AnalogLFO.cpp` -- no new files, no new dependencies, no Makefile changes.

The current implementation (after Phases 7+8) does a hard `phase = 0.0` on every clock edge regardless of ratio, which causes two problems: (1) at division ratios like /4, the LFO never completes a full cycle because it resets every beat instead of every 4th beat, and (2) the hard phase snap creates an output voltage discontinuity that produces audible clicks. The existing drift engine applies 7.5% maximum frequency deviation in all modes, which accumulates excessive phase error between clock resets when clocked.

The solution breaks into four distinct pieces that can be implemented incrementally: (1) a clock beat counter that only resets phase on every Nth edge for /N ratios (and on every edge for x1 and xN ratios), (2) a short output-side crossfade (2-5ms cosine window) that blends the pre-reset output value with the post-reset waveform to eliminate clicks, (3) a conditional that scales `driftScale` from 0.075f down to 0.02f when clocked, and (4) an `ExponentialFilter` on the frequency value that smooths transitions between clocked and free-running frequencies over ~50-100ms.

**Primary recommendation:** Implement all four pieces as inline modifications to `processClockInput()` and `process()`. Use the SDK's `dsp::TExponentialFilter` for frequency slew. Use a custom cosine crossfade counter (not an SDK primitive) for the anti-click window because the crossfade needs to track both pre-reset and post-reset output values, which no SDK filter handles.

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| RATE-04 | Phase resets to 0 on clock edge, with division-aware counting for /N ratios | Beat counter tracks edges within one LFO cycle. For ratios < 1.0 (divisions), compute `divisor = round(1.0 / ratio)` and only reset phase when `beatCount >= divisor`. For ratios >= 1.0 (unity/multiplications), reset on every edge. See Architecture Pattern 1. |
| RATE-05 | Anti-click crossfade (2-5ms cosine) on output after phase reset | On phase reset, capture the current output value as `crossfadeFrom`. Over the next 2-5ms, blend from `crossfadeFrom` to the live waveform output using a cosine curve: `mix = 0.5 - 0.5 * cos(pi * t / fadeTime)`. Apply after `computeMorphedWave()`, before voltage output. See Architecture Pattern 2. |
| DISP-04 | Drift authority reduced in clocked mode (~2% vs 7.5% free-running) | Change `driftScale = driftAmount * 0.075f` to `driftScale = driftAmount * (isClocked ? 0.02f : 0.075f)`. The 2% maximum deviation at full drift accumulates at most ~2% phase error per cycle, which the crossfade absorbs transparently. See Architecture Pattern 3. |
| DISP-05 | Smooth frequency slew during clock-to-free and free-to-clock transitions | Apply `dsp::TExponentialFilter` to the final `freq` value. Set lambda for ~50ms time constant (`lambda = 1.0 / 0.05 = 20`). When the mode changes (free->clocked or clocked->free), the filter smooths the frequency transition. See Architecture Pattern 4. |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `dsp::TExponentialFilter<float>` | VCV Rack 2 SDK (stable since 2.0) | Smooth frequency transitions (DISP-05) | SDK built-in exponential smoother. `process(deltaTime, in)` with configurable lambda. Verified in SDK at `include/dsp/filter.hpp` lines 58-88. |
| `std::cos()` / `M_PI` | C++ standard | Cosine crossfade curve (RATE-05) | Standard math. Cosine crossfade `0.5 - 0.5 * cos(pi * t)` has zero derivative at start and end, eliminating secondary clicks from the crossfade itself. |
| `std::round()` | C++ standard | Compute integer divisor from ratio (`round(1.0 / ratio)`) for RATE-04 | Standard math. Needed to convert fractional ratios like /1.5 (0.666667) to a beat count of 2 (since `round(1/0.666667) = round(1.5) = 2`). |

### Not Needed
| Library | Why NOT Needed |
|---------|----------------|
| `dsp::TSlewLimiter` | Its linear slew (clamped rate of change) produces a different curve than exponential smoothing. For frequency transitions, exponential smoothing sounds more natural -- it approaches the target asymptotically rather than arriving abruptly. |
| `dsp::TExponentialSlewLimiter` | Useful when rise and fall need different rates. Our frequency transitions are symmetric (free->clocked and clocked->free should feel the same). Plain `ExponentialFilter` with one lambda is simpler. |
| `dsp::TRCFilter` | RC filter is for per-sample continuous signals. We need event-driven smoothing (frequency changes discontinuously when mode switches). `ExponentialFilter` is the right tool. |
| Any external library | All four requirements are ~40 lines of C++ total using SDK primitives and standard math. |

**Installation:** No changes. No new dependencies. No Makefile changes. No new includes needed (existing includes cover `<cmath>`, `<atomic>`; SDK DSP headers already accessible).

## Architecture Patterns

### Pattern 1: Division-Aware Phase Reset (RATE-04)

**What:** Count clock edges and only reset phase on the Nth edge for /N division ratios.

**Current behavior (broken for divisions):**
```cpp
// In processClockInput(), line 287:
phase = 0.0;  // Resets on EVERY clock edge -- wrong for /4, /8, etc.
```

**Problem:** At /4, the LFO should complete one full cycle per 4 clock beats. Currently it resets every beat, meaning the waveform never gets past the first 25% of its cycle.

**Solution:**

```cpp
// New member variable
int clockBeatCount = 0;  // Counts clock edges within one LFO cycle

// In processClockInput(), replace the hard phase reset:
if (clockTrigger.process(clkVoltage, 0.1f, 1.0f)) {
    // ... period measurement code unchanged ...
    clockBeatCount++;

    // Determine whether this edge should reset phase
    bool shouldReset = true;
    if (ratioIdx >= 0 && RATIO_TABLE[ratioIdx] < 1.f) {
        // Division ratio: only reset every N beats
        int divisor = (int)std::round(1.f / RATIO_TABLE[ratioIdx]);
        shouldReset = (clockBeatCount >= divisor);
    }

    if (shouldReset) {
        phase = 0.0;
        clockBeatCount = 0;
        // Trigger crossfade (see Pattern 2)
    }
    // Period measurement and EMA smoothing happen on every edge regardless
}
```

**Key details:**
- `clockBeatCount` must be reset to 0 when entering FREE state (cable disconnect, timeout)
- `clockBeatCount` must be reset to 0 when the ratio changes (knob turn changes divisor)
- For ratios >= 1.0 (x1, x2, x3, etc.), `shouldReset` is always true -- reset every edge
- For /1.5 (ratio = 0.666667), `round(1/0.666667) = round(1.5) = 2` -- reset every 2nd edge, which is correct because the LFO needs 1.5 beats per cycle, and resetting every 2nd beat gives a clean alignment point
- For /6 (ratio = 0.166667), `round(1/0.166667) = round(6) = 6` -- reset every 6th edge

**Beat count for each ratio:**

| Ratio | Multiplier | `round(1/ratio)` | Reset every Nth edge |
|-------|-----------|-------------------|---------------------|
| /16 | 0.0625 | 16 | 16th |
| /8 | 0.125 | 8 | 8th |
| /6 | 0.1667 | 6 | 6th |
| /4 | 0.25 | 4 | 4th |
| /3 | 0.3333 | 3 | 3rd |
| /2 | 0.5 | 2 | 2nd |
| /1.5 | 0.6667 | 2 | 2nd |
| x1 | 1.0 | N/A | every |
| x1.5+ | >1.0 | N/A | every |

**Note on /1.5:** At /1.5, the LFO needs 1.5 clock beats per cycle. Resetting every 2nd beat means the phase is at `2 * 0.6667 = 1.333` cycles worth of phase -- it has wrapped once and is at 0.333 into the second cycle. The reset snaps it back to 0.0, which is a small correction. This is acceptable because the crossfade (Pattern 2) smooths it. The alternative (resetting every 1.5 beats) is impossible since we can only count integer edges.

**Where `ratioIdx` comes from:** The ratio index is already computed in `process()` after `processClockInput()` returns. We need to either: (a) compute the ratio index inside `processClockInput()` using the same `getScaledValue()` logic, or (b) pass the previous frame's ratio index into `processClockInput()`. Option (a) is cleaner -- compute it locally in `processClockInput()` for the division check, and let `process()` compute it independently for frequency calculation. Both will produce the same result because the knob position does not change within a single sample.

### Pattern 2: Anti-Click Output Crossfade (RATE-05)

**What:** When a phase reset occurs, blend the output from the pre-reset value to the post-reset waveform value over 2-5ms using a cosine curve.

**Key design decision: output-side crossfade, not phase-side.**

The crossfade operates on the final output voltage, AFTER `computeMorphedWave()` and AFTER character modeling. This is critical because:
1. The internal `phase` variable must snap to 0.0 immediately for timing accuracy (downstream modules that read phase need it exact)
2. Character modeling (saw's soft reset, square's duty cycle) should compute from the new phase position naturally
3. The crossfade only hides the output discontinuity -- it does not affect internal state

**Implementation:**

```cpp
// New member variables
float crossfadeFrom = 0.f;      // Output value captured at moment of phase reset
float crossfadeProgress = 1.f;   // 0.0 = just reset, 1.0 = crossfade complete
float crossfadeDuration = 0.f;   // Duration in seconds (2-5ms, set on reset)

// In processClockInput(), when shouldReset is true:
if (shouldReset) {
    // Capture current output BEFORE resetting phase
    float p = (float)phase;
    crossfadeFrom = computeMorphedWave(p, currentMorph, currentCharacter) * 5.f;
    crossfadeProgress = 0.f;
    crossfadeDuration = 0.003f;  // 3ms default (144 samples at 48kHz)
    phase = 0.0;
    clockBeatCount = 0;
}

// In process(), after computing the output:
float sample = computeMorphedWave(p, morph, character);
float outputVoltage = 5.f * sample;

// Apply crossfade if active
if (crossfadeProgress < 1.f) {
    crossfadeProgress += args.sampleTime / crossfadeDuration;
    if (crossfadeProgress >= 1.f) {
        crossfadeProgress = 1.f;
    } else {
        // Cosine crossfade: smooth start and end (zero derivative at boundaries)
        float mix = 0.5f - 0.5f * std::cos((float)M_PI * crossfadeProgress);
        outputVoltage = crossfadeFrom + mix * (outputVoltage - crossfadeFrom);
    }
}

outputs[OUTPUT].setVoltage(outputVoltage);
```

**Why cosine crossfade:**
- **Linear crossfade** has a corner at the start and end, which can produce its own subtle click (the derivative of the output is discontinuous)
- **Cosine crossfade** `0.5 - 0.5 * cos(pi * t)` has zero derivative at t=0 and t=1, ensuring perfectly smooth entry and exit
- At t=0: mix = 0 (100% from pre-reset value)
- At t=0.5: mix = 0.5 (50/50 blend)
- At t=1: mix = 1 (100% from live output)

**Duration choice: 3ms (tunable 2-5ms):**
- 2ms = 96 samples at 48kHz. Very short, might not fully mask large discontinuities
- 3ms = 144 samples at 48kHz. Good default -- inaudible timing-wise, sufficient for smooth blending
- 5ms = 240 samples at 48kHz. Very safe but starts to be audible as a "softening" of the attack

**Empirical tuning note:** The exact duration should be tuned by ear in VCV Rack with the LFO modulating a resonant filter. Start at 3ms and adjust if clicks are audible (increase) or if the modulation feels sluggish (decrease). This aligns with STATE.md's blocker note: "Anti-click crossfade duration and drift authority percentage need empirical tuning."

**Edge case: crossfade still active when next reset arrives.** If the clock is very fast and resets arrive within 3ms of each other (>333 Hz), the crossfade could be interrupted. At LFO rates this is irrelevant -- even at x16 with 300 BPM clock (the fastest musical scenario), the period between resets is `1/(300/60 * 16) = 12.5ms`, well above 3ms. But defensively: if a new reset arrives during crossfade, restart the crossfade from the current blended output value.

### Pattern 3: Drift Authority Reduction in Clocked Mode (DISP-04)

**What:** Scale the drift engine's maximum frequency deviation from 7.5% to ~2% when clocked.

**Current code (line 424):**
```cpp
float driftScale = driftAmount * 0.075f;  // 7.5% max frequency deviation
```

**Modified code:**
```cpp
bool isClocked = (clockState == ACQUIRING || clockState == LOCKED) && smoothedPeriod > 0.f;
float maxDrift = isClocked ? 0.02f : 0.075f;
float driftScale = driftAmount * maxDrift;
```

**Why 2% in clocked mode:**
- At 2% maximum deviation, the phase error accumulated between clock resets is at most ~2% of a cycle (e.g., at x1, the phase at reset time could be off by 0.02 from 1.0, meaning the output voltage discontinuity is at most ~0.1V on a 5V peak output)
- A 0.1V jump is well within the range that a 3ms cosine crossfade makes inaudible
- At 7.5% (free-running value), the phase error would be ~0.375V, which is on the edge of audibility even with crossfade
- The 2% drift still provides audible analog character -- the LFO wobbles subtly rather than staying rigidly locked, which is the desired "analog clocked LFO" feel
- The requirement says "~2%" which gives us latitude to tune between 1.5-3%

**Phase error accumulation analysis:**

| Drift Level | Max Deviation | Phase Error at Reset (x1) | Output Discontinuity (5V peak) |
|-------------|--------------|--------------------------|-------------------------------|
| 0% | 0% | 0.000 | 0.00V |
| 50% | 1% | 0.010 | ~0.05V |
| 100% | 2% | 0.020 | ~0.10V |
| 100% (old) | 7.5% | 0.075 | ~0.38V |

The 0.10V worst case with the new 2% limit is comfortably below the threshold of audibility when smoothed by a 3ms cosine crossfade.

**No smooth transition needed between drift scales:** The drift amount changes instantaneously when clock state changes. Since the OU process is continuous and the scale factor only affects the *multiplier* on the already-smooth OU output, there is no discontinuity. The drift simply becomes "quieter" instantly, which is inaudible because the OU layers are already low-frequency (0.05-2Hz).

### Pattern 4: Frequency Slew for Mode Transitions (DISP-05)

**What:** Smooth the frequency when transitioning between clocked and free-running modes (and vice versa).

**Problem:** When a CLK cable is connected and the module enters ACQUIRING, the LFO might jump from 0.7 Hz (Rate knob) to 2 Hz (clock-derived). When disconnected, it jumps back. These frequency jumps cause an audible pitch change in the LFO output.

**Solution: `dsp::TExponentialFilter` on the frequency value.**

```cpp
// New member variable
dsp::TExponentialFilter<float> freqSlew;

// In constructor:
freqSlew.setLambda(20.f);  // lambda = 1/tau = 1/0.05 = 20 -> 50ms time constant
freqSlew.out = 0.7f;       // Initialize to default Rate knob value

// In process(), after computing freq but before using it:
float targetFreq = freq;  // The "ideal" frequency (either from knob or clock)
freq = freqSlew.process(args.sampleTime, targetFreq);
freq = std::fmax(freq, 0.001f);

// ... rest of process() uses smoothed freq ...
```

**Why 50ms time constant:**
- 50ms is ~2400 samples at 48kHz. This is fast enough to be imperceptible as a "delay" in clock sync response, but slow enough to eliminate the frequency jump
- The exponential filter reaches 95% of target in ~3 time constants = 150ms. At 120 BPM (0.5s period), the frequency settles well within one beat
- Compare: Phase 7 research settled on "instant snap" for the initial implementation, with the note that "Phase 9 adds smooth slew." This is that slew

**Edge case: first clock lock.** When the first clock frequency is computed (edge 2), the slew filter smooths from the Rate knob Hz to the clock-derived Hz. This is the correct behavior -- the user should hear the LFO smoothly transition to the clock tempo, not jump.

**Edge case: ratio changes while clocked.** When the user turns the Rate knob in clocked mode (changing from x1 to x4, for example), the frequency jumps by a factor of 4. The slew filter will smooth this. At 50ms time constant, a 4x frequency jump settles in about 150ms, which may feel slightly sluggish for fast knob turns. This is acceptable because:
1. Ratio changes are discrete jumps (15 positions), not continuous sweeps
2. The Phase 8 CONTEXT.md said "no queuing to next clock edge -- instant response to knob movement" -- the slew is fast enough (50ms) that it *feels* instant while preventing clicks
3. If empirical testing shows the slew is too noticeable on ratio changes, the lambda can be increased (faster slew) or the slew can be bypassed for ratio-only changes

**Important: initialize freqSlew.out correctly.**
- On module creation, set `freqSlew.out = params[RATE_PARAM].getValue()` (but params may not be initialized yet in the constructor)
- Safest approach: set `freqSlew.out = 0.7f` (the default Rate param value) in the constructor, which matches the default knob position
- Alternatively, detect uninitialized state and snap on first process() call

### Anti-Patterns to Avoid

- **Do NOT crossfade on the phase variable.** Smoothing `phase` would break the internal timing. The crossfade must be on the output voltage only. `phase = 0.0` must be instantaneous.
- **Do NOT apply the crossfade BEFORE character modeling.** The character modeling (saw soft reset, square duty cycle) has its own behavior at phase 0.0. The crossfade must happen AFTER `computeMorphedWave()` to avoid double-smoothing artifacts.
- **Do NOT use a separate timer for crossfade duration.** Use the `crossfadeProgress` counter driven by `args.sampleTime`. No need for `dsp::Timer` here.
- **Do NOT reduce drift to zero in clocked mode.** The whole point of this module is analog character. Drift=1 in clocked mode should still produce audible wobble -- just reduced enough that the crossfade absorbs the phase error at reset.
- **Do NOT apply frequency slew to the phase reset itself.** The phase reset is an instantaneous event (correct phase alignment). The frequency slew only affects the frequency *value* used for phase accumulation between resets.
- **Do NOT apply drift authority reduction with a slew.** The change from 0.075 to 0.02 drift scale can happen instantly because the OU process output is already smooth and low-frequency.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Frequency smoothing on mode transitions | Manual slew with timer and rate math | `dsp::TExponentialFilter<float>` | SDK built-in. Handles deltaTime correctly. Single `process()` call per sample. Already verified in SDK headers (filter.hpp lines 58-88). Asymptotically approaches target -- no overshoot. |
| Integer division from fractional ratio | Manual if/else chain for each ratio | `std::round(1.f / RATIO_TABLE[ratioIdx])` | One expression covers all ratios. Produces correct integer divisors even for fractional ratios (/1.5 -> 2). |
| Crossfade curve | Linear interpolation | Cosine curve `0.5 - 0.5 * cos(pi * t)` | Zero derivative at start/end eliminates secondary clicks. Linear crossfade has corners that can be audible through resonant filters. |

**Key insight:** Three of the four requirements are pure arithmetic (~5-10 lines each). Only the crossfade has any complexity, and that is because it needs to capture the pre-reset output value at the right moment. No new infrastructure is needed.

## Common Pitfalls

### Pitfall 1: Crossfade Computing Pre-Reset Value AFTER Phase Reset
**What goes wrong:** If `phase = 0.0` is executed before capturing the pre-reset output, the crossfade starts from the value at phase=0 instead of the value where the waveform actually was. The crossfade then blends from phase-0 to phase-0, achieving nothing -- the click remains.
**Why it happens:** Natural code ordering puts `phase = 0.0` first, then "oh, I should crossfade."
**How to avoid:** Capture `crossfadeFrom = computeMorphedWave((float)phase, morph, character) * 5.f` BEFORE setting `phase = 0.0`. This requires morph and character values to be available in `processClockInput()`, which they are not currently. Two solutions: (a) compute them early in `process()` before calling `processClockInput()`, or (b) use the *previous frame's* output value (stored in a member variable). Option (b) is simpler and produces a nearly identical result since the phase changes by less than 0.001 between frames at LFO rates.
**Warning signs:** Click persists despite crossfade being "active." Debug by logging `crossfadeFrom` -- if it equals the post-reset value, the capture is too late.

### Pitfall 2: Beat Counter Not Reset When Ratio Changes
**What goes wrong:** User is at /4 (reset every 4th edge), `clockBeatCount` is at 2. User turns knob to /2 (reset every 2nd edge). The beat counter is still at 2, so the next edge (count=3) is not a reset point for /2, causing the LFO to run for one extra beat without resetting.
**Why it happens:** `clockBeatCount` was designed for one ratio and was not reset when the ratio changed.
**How to avoid:** Track `prevRatioIdx`. When `ratioIdx != prevRatioIdx`, set `clockBeatCount = 0`. This ensures the new ratio starts fresh. The minor side effect (one premature reset) is inaudible with the crossfade.
**Warning signs:** LFO phase alignment shifts by one beat after changing division ratio.

### Pitfall 3: Frequency Slew Delaying Initial Clock Lock
**What goes wrong:** On the very first clock lock (edge 2), the frequency slew filter smooths the transition from 0.7 Hz to the clock frequency over 50ms. This is correct behavior, but if the user expects "instant" lock, they may perceive a brief pitch wobble on the first beat.
**Why it happens:** The ExponentialFilter starts at 0.7 Hz (Rate knob default) and needs ~150ms to reach 95% of the target.
**How to avoid:** Consider snapping the filter on the very first clock lock by setting `freqSlew.out = targetFreq` directly when transitioning from FREE to ACQUIRING/LOCKED for the first time (i.e., when `smoothedPeriod` transitions from 0 to a valid value). Subsequent mode changes use the normal slew. Alternatively, keep the slew on the first lock too -- it only lasts one beat and sounds like a natural "sync" effect.
**Warning signs:** Brief pitch slide on first clock connection. May actually sound good -- empirical tuning needed.

### Pitfall 4: Crossfade Interacting with Display Phase Tracking
**What goes wrong:** The phase dot on the display tracks `displayPhase`, which is the raw `phase` value. During a crossfade, the audio output is blending old and new, but the display shows the new phase position immediately. The display dot "jumps" to the reset position while the audio is still crossfading.
**Why it happens:** The crossfade only affects the audio output, not the internal phase state.
**How to avoid:** Accept this. The display update rate (~60fps) means the crossfade completes within a single display frame (3ms crossfade vs 16.7ms frame time). The user will never see the intermediate state. The dot teleport on reset is expected and correct -- Phase 10's display will add a sync badge that contextualizes this behavior.
**Warning signs:** This is NOT a bug. Do not try to smooth the display phase.

### Pitfall 5: Drift Authority Change Creating Audible Artifact on Cable Connect/Disconnect
**What goes wrong:** When CLK cable is connected, `isClocked` flips from false to true, and `driftScale` jumps from `driftAmount * 0.075` to `driftAmount * 0.02`. The OU process output is multiplied by this scale, so the drift contribution to `deltaPhase` drops by 73% in one sample.
**Why it happens:** Instantaneous scale change on a continuously-varying signal.
**How to avoid:** This is actually fine because: (1) the OU process output is already low-frequency (highest layer is ~2Hz), (2) the drift contribution to `deltaPhase` is typically very small (a few percent), and (3) the 73% drop in a few-percent contribution is imperceptibly small. A 0.075 -> 0.02 change on a deltaPhase of 0.001 changes it by 0.0000055 -- well below any audible threshold. No smoothing needed.
**Warning signs:** N/A. This will not produce any audible artifact.

## Code Examples

### Complete Modified processClockInput() (Phase Reset Logic)

```cpp
// Source: Synthesized from current codebase + RATE-04/RATE-05 requirements
// New member variables needed:
int clockBeatCount = 0;
int prevRatioIdx = -1;
float crossfadeFrom = 0.f;
float crossfadeProgress = 1.f;
float crossfadeDuration = 0.003f;  // 3ms
float lastOutputVoltage = 0.f;     // Previous frame's output for crossfade capture

void processClockInput(float sampleTime) {
    bool clkConnected = inputs[CLK_INPUT].isConnected();

    // Instant revert on cable disconnect
    if (!clkConnected) {
        if (prevClkConnected) {
            if (smoothedPeriod > 0.f) {
                lastSmoothedPeriod = smoothedPeriod;
            }
            clockState = FREE;
            clockEdgeCount = 0;
            clockBeatCount = 0;        // NEW: reset beat counter
            clockTimer.reset();
            smoothedPeriod = 0.f;
            displayClockState.store(FREE, std::memory_order_relaxed);
        }
        prevClkConnected = false;
        return;
    }
    prevClkConnected = true;

    // Accumulate time
    clockTimer.process(sampleTime);

    // Timeout check
    if (clockState != FREE && smoothedPeriod > 0.f) {
        float timeout = std::fmax(1.0f, std::fmin(3.0f * smoothedPeriod, 5.0f));
        if (clockTimer.getTime() > timeout) {
            lastSmoothedPeriod = smoothedPeriod;
            clockState = FREE;
            clockEdgeCount = 0;
            clockBeatCount = 0;        // NEW: reset beat counter
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

        if (clockEdgeCount == 1) {
            // First edge: enter ACQUIRING
            clockState = ACQUIRING;
            clockBeatCount = 0;
            // Phase reset on first edge (always)
            crossfadeFrom = lastOutputVoltage;
            crossfadeProgress = 0.f;
            phase = 0.0;
            displayClockState.store(ACQUIRING, std::memory_order_relaxed);
        }
        else if (rawPeriod > 0.001f) {
            // Outlier rejection (LOCKED only)
            if (clockState == LOCKED && smoothedPeriod > 0.f) {
                bool isOutlier = (rawPeriod > 3.0f * smoothedPeriod) ||
                                 (rawPeriod < smoothedPeriod / 3.0f);
                if (isOutlier) return;
            }

            // Fast-track re-acquisition
            if (clockState == ACQUIRING && clockEdgeCount == 2 && lastSmoothedPeriod > 0.f) {
                float ratio = rawPeriod / lastSmoothedPeriod;
                if (ratio > 0.8f && ratio < 1.2f) {
                    smoothedPeriod = lastSmoothedPeriod;
                    smoothedPeriod += 0.3f * (rawPeriod - smoothedPeriod);
                    clockState = LOCKED;
                    displayClockState.store(LOCKED, std::memory_order_relaxed);
                }
            }

            // EMA smoothing
            if (smoothedPeriod <= 0.f) {
                smoothedPeriod = rawPeriod;
            } else {
                smoothedPeriod += 0.3f * (rawPeriod - smoothedPeriod);
            }

            // ACQUIRING -> LOCKED transition
            if (clockState == ACQUIRING && clockEdgeCount >= 4) {
                clockState = LOCKED;
                displayClockState.store(LOCKED, std::memory_order_relaxed);
            } else if (clockState == ACQUIRING) {
                displayClockState.store(ACQUIRING, std::memory_order_relaxed);
            }

            // NEW: Division-aware phase reset (RATE-04)
            clockBeatCount++;

            // Determine current ratio for division check
            int currentRatioIdx = -1;
            if ((clockState == ACQUIRING || clockState == LOCKED) && smoothedPeriod > 0.f) {
                float knobNormalized = paramQuantities[RATE_PARAM]->getScaledValue();
                currentRatioIdx = (int)std::round(knobNormalized * 14.f);
                currentRatioIdx = rack::math::clamp(currentRatioIdx, 0, 14);
            }

            // Reset beat counter if ratio changed
            if (currentRatioIdx != prevRatioIdx && prevRatioIdx >= 0) {
                clockBeatCount = 1;  // This edge counts as beat 1 of new ratio
            }
            prevRatioIdx = currentRatioIdx;

            bool shouldReset = true;
            if (currentRatioIdx >= 0 && RATIO_TABLE[currentRatioIdx] < 1.f) {
                int divisor = (int)std::round(1.f / RATIO_TABLE[currentRatioIdx]);
                shouldReset = (clockBeatCount >= divisor);
            }

            if (shouldReset) {
                // Capture pre-reset output for crossfade (RATE-05)
                crossfadeFrom = lastOutputVoltage;
                crossfadeProgress = 0.f;
                phase = 0.0;
                clockBeatCount = 0;
            }
        }
    }
}
```

### Complete Modified process() (Crossfade, Drift, Slew)

```cpp
// Source: Current process() with RATE-05, DISP-04, DISP-05 additions

void process(const ProcessArgs& args) override {
    // Clock detection (Phase 7)
    processClockInput(args.sampleTime);

    // Rate: dual-mode frequency calculation (Phase 8)
    float targetFreq;
    int ratioIdx = -1;
    bool isClocked = (clockState == ACQUIRING || clockState == LOCKED) && smoothedPeriod > 0.f;

    if (isClocked) {
        float knobNormalized = paramQuantities[RATE_PARAM]->getScaledValue();
        ratioIdx = (int)std::round(knobNormalized * 14.f);
        ratioIdx = rack::math::clamp(ratioIdx, 0, 14);
        float clockFreq = 1.f / smoothedPeriod;
        targetFreq = clockFreq * RATIO_TABLE[ratioIdx];
    } else {
        targetFreq = params[RATE_PARAM].getValue();
    }
    targetFreq = std::fmax(targetFreq, 0.001f);

    // NEW: Frequency slew for smooth transitions (DISP-05)
    float freq = freqSlew.process(args.sampleTime, targetFreq);
    freq = std::fmax(freq, 0.001f);

    displayRatioIndex.store(ratioIdx, std::memory_order_relaxed);

    // Phase accumulation
    double deltaPhase = (double)freq * (double)args.sampleTime;

    // Drift processing with clocked mode authority reduction (DISP-04)
    float driftKnob = params[DRIFT_PARAM].getValue();
    float driftAtten = params[DRIFT_ATTEN_PARAM].getValue();
    float driftCV = inputs[DRIFT_CV_INPUT].getVoltage();
    float drift = rack::math::clamp(driftKnob + driftAtten * driftCV / 5.f, 0.f, 1.f);
    displayDrift.store(drift, std::memory_order_relaxed);

    if (drift >= 0.001f) {
        if (sqrtSampleTime == 0.f) sqrtSampleTime = std::sqrt(args.sampleTime);
        float driftAmount = progressiveCurve(drift);
        float combinedOU = 0.f;
        for (int i = 0; i < NUM_OU_LAYERS; i++) {
            float noise = normalDist(rng);
            ouLayers[i].state += ouLayers[i].theta * (0.f - ouLayers[i].state) * args.sampleTime
                               + ouLayers[i].sigma * sqrtSampleTime * noise;
            combinedOU += ouLayers[i].state * ouLayers[i].weight;
        }
        // NEW: Reduced drift authority in clocked mode (DISP-04)
        float maxDrift = isClocked ? 0.02f : 0.075f;
        float driftScale = driftAmount * maxDrift;
        deltaPhase *= (1.0 + (double)(driftScale * combinedOU));
    }

    phase += deltaPhase;
    if (phase >= 1.0) phase -= 1.0;

    // ... morph/character unchanged ...

    float sample = computeMorphedWave(p, morph, character);
    float outputVoltage = 5.f * sample;

    // NEW: Anti-click crossfade (RATE-05)
    if (crossfadeProgress < 1.f) {
        crossfadeProgress += args.sampleTime / crossfadeDuration;
        if (crossfadeProgress >= 1.f) {
            crossfadeProgress = 1.f;
        } else {
            float mix = 0.5f - 0.5f * std::cos((float)M_PI * crossfadeProgress);
            outputVoltage = crossfadeFrom + mix * (outputVoltage - crossfadeFrom);
        }
    }

    // Store for next frame's crossfade capture
    lastOutputVoltage = outputVoltage;

    outputs[OUTPUT].setVoltage(outputVoltage);
}
```

### ExponentialFilter Initialization

```cpp
// In constructor, after configParam calls:
freqSlew.setLambda(20.f);   // 50ms time constant
freqSlew.out = 0.7f;        // Default Rate param value
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Hard phase reset on every edge | Division-aware Nth-edge reset | Standard in clocked LFO firmware (Mutable Instruments Tides, etc.) | Allows division ratios to complete full cycles |
| No click mitigation | Output-side cosine crossfade (2-5ms) | Standard DSP practice for discontinuity smoothing | Eliminates audible clicks at phase reset |
| Same drift in all modes | Reduced drift authority when clocked | Common in clocked analog-modeled LFOs | Prevents excessive phase error while preserving analog character |
| Instant frequency jump on mode change | ExponentialFilter-based frequency slew | Standard UX practice | Eliminates jarring frequency transitions |

**Why output-side crossfade (not phase smoothing or RC filter on output):**
- Phase smoothing would break timing accuracy
- RC filter on the output would permanently alter the waveform shape (attenuating fast transitions in square/saw waves)
- Output crossfade is a short event (3ms) that only activates on reset, leaving the normal waveform completely unaffected

## Open Questions

1. **Exact crossfade duration**
   - What we know: 2-5ms range, 3ms is a good starting point. 144 samples at 48kHz.
   - What's unclear: Whether 3ms is sufficient for large phase discontinuities (e.g., saw wave at phase 0.9 jumping to phase 0.0 = ~9V discontinuity on +/-5V output).
   - Recommendation: Start at 3ms. Test with LFO modulating a resonant filter cutoff (worst case for click audibility). Adjust by ear. The constant is a single float, trivial to tune.

2. **Frequency slew on ratio changes**
   - What we know: 50ms time constant smooths mode transitions. But it also smooths ratio changes (e.g., x1 to x4).
   - What's unclear: Whether the 50ms slew is perceptible/desirable when quickly sweeping through ratios.
   - Recommendation: Keep the slew for now. If ratio changes feel sluggish during testing, increase lambda to 40-50 (25-20ms time constant) or add a bypass for ratio-only changes.

3. **Initial clock lock behavior**
   - What we know: The freq slew filter smooths the transition from Rate knob Hz to clock Hz on first lock.
   - What's unclear: Whether snapping immediately on first lock (bypassing slew) gives a better UX.
   - Recommendation: Try with slew first. If it feels "slow to lock," add a snap on first `smoothedPeriod > 0` transition.

4. **Handling /1.5 division edge count**
   - What we know: `round(1/0.6667) = 2`, so /1.5 resets every 2nd edge. The LFO completes 1.333 cycles in 2 beats, so it wraps once naturally and is 0.333 into the second cycle when the reset arrives.
   - What's unclear: Whether the 0.333-cycle phase error at reset produces a noticeable click even with crossfade.
   - Recommendation: Test specifically. /1.5 and x1.5 are the edge cases. If /1.5 clicks, consider resetting every 3rd edge instead (which would give 2 full cycles in 3 beats -- exact alignment, but longer between resets).

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Manual testing in VCV Rack 2 (no automated unit test framework established) |
| Config file | none |
| Quick run command | `make install && open -a "VCV Rack 2 Free"` |
| Full suite command | Manual testing with clock module patched to CLK input, LFO output to filter/VCA |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| RATE-04 | Phase resets on correct edges for /N ratios | manual | `make` (build check) + visual: patch clock at 60 BPM, set /4, verify LFO completes full cycle over 4 beats | N/A |
| RATE-05 | No audible click on phase reset | manual | `make` (build check) + auditory: route LFO to resonant filter cutoff, listen for clicks at clock edges | N/A |
| DISP-04 | Drift reduced in clocked mode | manual | Set Drift=100%, compare LFO wobble amount between free-running and clocked modes | N/A |
| DISP-05 | Smooth frequency transition on cable connect/disconnect | manual | Slowly connect/disconnect CLK cable, listen for frequency jump vs smooth glide | N/A |

### Sampling Rate
- **Per task commit:** `make` (build verification)
- **Per wave merge:** Full manual test: clock at various tempos, test all division ratios, test with LFO modulating filter for click detection, test cable connect/disconnect
- **Phase gate:** All 4 requirements verified manually before proceeding to Phase 10

### Wave 0 Gaps
None -- this is a C++ VCV Rack module without an established automated test framework. All testing is manual verification in the running application. The `make` build check serves as the automated smoke test.

## Sources

### Primary (HIGH confidence)
- VCV Rack 2 SDK source: `include/dsp/filter.hpp` -- `TExponentialFilter<float>` API: `setLambda()`, `process(deltaTime, in)`, `out` member. Verified line by line at `/Users/mrcbrown/Claude/Software/Forge Audio/Rack-SDK/include/dsp/filter.hpp` lines 58-88.
- VCV Rack 2 SDK source: `include/dsp/filter.hpp` -- `TSlewLimiter` and `TExponentialSlewLimiter` APIs reviewed and rejected (linear slew / asymmetric rates not needed). Lines 131-189.
- Existing codebase: `src/AnalogLFO.cpp` -- Phase 7+8 complete. Phase reset at line 287 (`phase = 0.0`), drift engine at lines 405-426 (`driftScale = driftAmount * 0.075f`), frequency override at lines 383-397, output at line 467 (`outputs[OUTPUT].setVoltage(5.f * sample)`).
- Phase 7 RESEARCH.md: Clock state machine, processClockInput() architecture, edge detection patterns.
- Phase 8 RESEARCH.md + SUMMARY.md: Ratio table, frequency override, RateParamQuantity.
- PITFALLS.md: Pitfalls 1 (click on reset), 2 (drift fighting sync), 8 (cable disconnect), 11 (character interaction with reset).

### Secondary (MEDIUM confidence)
- [VCV Community: Anti-click crossfade implementation](https://community.vcvrack.com/t/implementing-an-anti-click-crossfade-over-very-short-sample-times/16350) -- Confirmed 5ms as accepted crossfade duration in VCV Rack modules. `dsp::SlewLimiter` validated as effective for switch de-clicking. Our crossfade approach (cosine window) is more targeted.
- [Mutable Instruments Tides Manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/tides_2018/manual/) -- Confirmed clock division/multiplication as standard feature. Phase locking at LFO rates. Different algorithm at audio rates.
- [KVR Audio: Synth click prevention](https://www.kvraudio.com/forum/viewtopic.php?t=200894) -- Community consensus: RC filter ~3-4ms for post-discontinuity smoothing. Our cosine crossfade is the refined version of this technique.
- [JUCE Forum: LFO clicks problem](https://forum.juce.com/t/lfo-clicks-problem/41475) -- Confirmed that phase reset clicks are a universal LFO design challenge. Discontinuity at phase=0 when syncing with BPM.

### Tertiary (LOW confidence)
- Crossfade duration (3ms default): Engineering judgment informed by VCV community (5ms suggested), DSP literature (2-5ms standard for anti-click). Exact value requires empirical tuning per STATE.md blocker note.
- Drift authority (2%): Engineering judgment balancing analog feel vs phase error. The requirement says "~2%" which supports this value, but final tuning is empirical.
- Frequency slew time constant (50ms): Engineering judgment. No community consensus found for this specific parameter. 50ms is based on the principle that mode transitions should feel "smooth but fast."

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- SDK APIs verified against actual header source code in local Rack-SDK
- Architecture (division-aware reset): HIGH -- Mathematical analysis of all 15 ratios produces correct integer divisors. Pattern well-established in clocked Eurorack firmware.
- Architecture (crossfade): HIGH -- Cosine crossfade is standard DSP technique. Output-side application avoids interfering with phase accuracy or waveform character.
- Architecture (drift reduction): HIGH -- Simple conditional on a single float constant. No complex interactions.
- Architecture (frequency slew): HIGH -- `ExponentialFilter` API verified in SDK. Time constant selection is engineering judgment (MEDIUM for the 50ms value specifically).
- Pitfalls: HIGH -- Analyzed against current source code. Crossfade capture ordering pitfall is the most critical.
- Code examples: HIGH -- Built directly from verified current source code with minimal additions.

**Research date:** 2026-03-10
**Valid until:** Indefinitely (VCV Rack 2 SDK DSP filter APIs are stable; the techniques used are fundamental DSP patterns)
