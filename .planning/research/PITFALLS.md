# Domain Pitfalls: Clock Sync for Analog Series LFO

**Domain:** Adding clock synchronization to an existing analog-modeled VCV Rack LFO module
**Researched:** 2026-03-07
**Overall Confidence:** MEDIUM-HIGH (VCV Rack voltage standards verified via official docs; DSP patterns from established engineering practice; drift/clock interaction analysis based on direct source code review of existing AnalogLFO.cpp)

---

## Critical Pitfalls

Mistakes that cause rewrites, audible artifacts, or architectural dead-ends.

---

### Pitfall 1: Phase Reset Discontinuity (Clicks and Pops)

**What goes wrong:** When a clock edge arrives and the LFO phase is hard-reset to 0.0, the output voltage jumps instantaneously from wherever the waveform currently is to the waveform value at phase 0.0. For a sine wave at phase 0.7, that is a jump from approximately -3.1V to 0V. For a falling saw at phase 0.3, that is a jump from +2V to +5V. This produces a broadband impulse -- an audible click or pop -- identical to a needle drop on vinyl.

**Why it happens:** The existing `AnalogLFO::process()` uses a continuous phase accumulator (`double phase`) that wraps smoothly at 1.0. Clock sync introduces a forced discontinuity: the phase must snap to 0.0 on each clock edge to maintain beat alignment. The larger the phase error at the moment of reset, the larger the output voltage discontinuity.

**Consequences:** Audible clicks on every clock pulse, especially noticeable when the LFO modulates a filter cutoff or VCA gain. At slow clock rates (e.g., 1 beat per 2 seconds), the click stands alone and is very obvious. Users will perceive the module as broken. If the LFO is modulating pitch, each click is a transient tuning spike.

**Prevention:**
1. **Short crossfade on reset:** When a clock edge triggers a phase reset, do not snap `phase = 0.0` immediately. Instead, capture the current output value and crossfade from it to the new waveform value over 2-5ms (roughly 100-220 samples at 44.1kHz). A cosine crossfade (`0.5 - 0.5 * cos(pi * t / fadeTime)`) avoids corners that produce their own clicks.
2. **Reset at natural zero-crossing:** If the waveform is within a few percent of phase 0.0 or 1.0 when the clock arrives, snap directly (the discontinuity is small enough to be inaudible). Only engage the crossfade when the phase error exceeds a threshold (e.g., >5% of cycle).
3. **Separate the phase reset from the output reset:** Update `phase = 0.0` immediately for timing accuracy, but apply a short slew on the output side. This keeps the internal phase accumulator sample-accurate for downstream timing while hiding the artifact.
4. **Test with the LFO modulating a resonant filter:** Clicks are most audible through a resonant filter because the impulse excites the filter's resonance, producing a "ping" on every clock pulse.

**Detection (warning signs):**
- Audible tick/click synchronized with clock input
- Waveform display shows visible vertical jump at clock edge
- Spectral analysis shows broadband energy spikes at clock rate

**Phase mapping:** Address in the first implementation phase of clock sync. The phase reset mechanism is the foundation -- if this clicks, nothing else matters.

**Confidence:** HIGH (fundamental DSP principle; verified by KVR Audio forum discussions on discontinuity artifacts and JUCE forum LFO click threads)

---

### Pitfall 2: Drift Engine Fighting Clock Sync

**What goes wrong:** The existing OU (Ornstein-Uhlenbeck) drift engine modifies `deltaPhase` before phase accumulation (line 244 of AnalogLFO.cpp: `deltaPhase *= (1.0 + (double)(driftScale * combinedOU))`). When clock sync is active, the LFO frequency is derived from the measured clock period, not from the Rate knob. If drift continues to modulate `deltaPhase`, it pulls the LFO frequency away from the clock-derived frequency, causing the phase to accumulate error between clock edges. On the next clock edge, the phase error is larger, requiring a bigger reset correction, which produces a bigger click. At high drift levels, the LFO could be as much as 7.5% off-frequency (the current `driftScale` maximum), meaning the phase at clock arrival could be off by 7.5% of a full cycle per period.

**Why it happens:** The drift engine was designed for free-running operation where cumulative phase error is musically desirable (it IS the drift). But in clocked mode, phase error is a defect, not a feature. The two systems have contradictory goals: drift wants to wander, clock sync wants to lock.

**Consequences:** Phase reset clicks proportional to drift amount. At Drift=1.0 with slow clock rates, the click becomes a pronounced "thump" as the phase can be 7.5% off (0.375V discontinuity on a +/-5V output). Users who want "clocked LFO with analog character" -- a very reasonable use case -- get punished with artifacts. They must choose between clock sync and drift, which defeats the module's value proposition.

**Prevention:**
1. **Reduce drift authority in clocked mode:** When clocked, scale drift contribution down significantly. A 1-2% frequency deviation is enough to add subtle analog feel without accumulating large phase errors between clock edges. Use `driftScale = driftAmount * 0.02f` in clocked mode vs the current `0.075f` in free mode.
2. **Apply drift to waveform shape, not frequency:** In clocked mode, redirect drift energy to subtle waveform deformation (character perturbation, tiny duty cycle wobble) rather than frequency deviation. The user gets analog feel without timing drift.
3. **Phase correction with slew:** On each clock edge, measure the accumulated phase error, then apply a gradual correction over the next ~10% of the cycle rather than an instant snap. This distributes the correction energy over time, hiding it as a subtle speed change rather than a click.
4. **Document the interaction:** Make it clear in tooltips or documentation that Drift in clocked mode affects waveform character rather than timing. Users coming from hardware analog synths will understand -- real clocked LFOs also have less drift than free-running ones.

**Detection:**
- Increase Drift to maximum, enable clock, listen for periodic clicks
- Watch the waveform display: visible "jerk" at phase reset means drift accumulated too much error
- Compare the display at Drift=0 clocked vs Drift=1 clocked

**Phase mapping:** Must be designed alongside the clock sync engine, not as an afterthought. The drift behavior in clocked mode is an architectural decision.

**Confidence:** HIGH (direct analysis of existing source code at `/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/src/AnalogLFO.cpp` lines 230-245)

---

### Pitfall 3: Clock Period Measurement Instability

**What goes wrong:** Measuring the time between clock edges by counting samples seems simple but has multiple failure modes: (1) the first clock pulse has no previous edge, so no period can be computed; (2) tempo changes (the user adjusts BPM upstream) cause a sudden period change that, if applied instantly, produces a frequency jump; (3) irregular clock sources (swing, shuffle, humanized clocks) produce alternating long/short periods; (4) very slow clocks (e.g., 1 beat per 8 seconds) mean the LFO runs at a guessed frequency for a very long time before getting corrected.

**Why it happens:** A single period measurement between two edges is inherently noisy. Real-world clock sources in VCV Rack include hand-turned knobs, CV-controlled BPM, swung/shuffled clocks, and MIDI clocks with jitter. Taking a single inter-edge interval as truth produces jittery LFO frequency.

**Consequences:** LFO frequency wobbles visibly on the display. At slow tempos, the LFO may run at wildly wrong speeds for seconds. With swung clocks, the LFO alternates between two different speeds every beat, producing an unpleasant "breathing" artifact rather than the intended smooth modulation.

**Prevention:**
1. **Exponential moving average (EMA) of period:** Smooth the measured period using `smoothedPeriod = alpha * newPeriod + (1 - alpha) * smoothedPeriod`. An alpha of 0.3-0.5 balances responsiveness with stability. This requires at least 2-3 clock pulses before the estimate is usable.
2. **Outlier rejection:** If a new period measurement differs from the smoothed period by more than 50%, treat it as a tempo change rather than noise. Apply it immediately but flag the system to expect further changes (tempo is ramping).
3. **First-pulse handling:** On the first clock edge, do not attempt to set the LFO frequency. Simply record the timestamp. On the second edge, compute the first period but treat it as preliminary. On the third edge, you have two measurements and can begin smoothing. Before the second edge arrives, keep the LFO running at its current Rate knob frequency.
4. **Timeout detection:** If no clock edge arrives for more than 2x the last measured period, consider the clock stopped. Either revert to free-running mode or hold the last known frequency. Do not let the timer overflow or accumulate indefinitely.
5. **Display the measured BPM/period:** Show the user what the module thinks the tempo is. This makes debugging easy and builds trust.

**Detection:**
- Patch a slightly irregular clock (e.g., a clock with swing) and observe LFO frequency stability
- Change upstream BPM abruptly and watch for frequency overshoot
- Remove the clock cable and verify the module recovers gracefully

**Phase mapping:** Core clock sync implementation phase. Period measurement is the foundation that everything else (division, multiplication, display) depends on.

**Confidence:** HIGH (established digital clock recovery principle; community reports of jitter issues with VCV Rack clocks)

---

### Pitfall 4: Wrong Schmitt Trigger Thresholds Breaking Interoperability

**What goes wrong:** Using non-standard trigger thresholds means the module fails to detect clock pulses from certain sources, or falsely triggers on noise. VCV Rack has an official voltage standard: Schmitt trigger with low threshold ~0.1V and high threshold ~1-2V. Modules that deviate from this (e.g., using 2.5V threshold for "cleaner" detection) will miss triggers from modules outputting weak pulses, or modules using 5V gates instead of 10V triggers.

**Why it happens:** Developers assume all triggers are clean 0-10V pulses and use a simple comparator instead of a Schmitt trigger. Or they implement hysteresis but pick thresholds that don't match the ecosystem. The VCV Rack Voltage Standards page explicitly warns about the Gibbs phenomenon: bandlimited trigger sources (e.g., a VCO square wave used as a clock) produce ringing around transitions that causes false retriggering without proper hysteresis.

**Consequences:** Module works in testing with one clock source but fails with others. Users report "clock sync doesn't work" with specific module combinations. Extremely frustrating to debug because the developer's test setup works fine.

**Prevention:**
1. **Use `dsp::SchmittTrigger` from the VCV Rack SDK.** Call `schmittTrigger.process(input, 0.1f, 1.f)` exactly as documented in the Voltage Standards. Do not roll your own.
2. **Test with multiple clock sources:** VCV Fundamental CLKD, Impromptu CLOCKED, Bogaudio CLKD, a raw square wave from a VCO, and an attenuated trigger (3V amplitude). If any of these fail, the thresholds are wrong.
3. **Handle the clock-after-reset timing rule:** VCV Rack standard says modules with CLOCK and RESET inputs should ignore CLOCK triggers up to 1ms after a RESET trigger. This prevents the 1-sample cable delay from causing clock and reset to be processed in the wrong order. Use `dsp::Timer` to track the post-reset blanking window.

**Detection:**
- Clock sync works with some modules but not others
- Occasional double-triggers (two resets per clock pulse) from bandlimited clock sources
- Module triggers on noise when no clock is connected but the input jack has crosstalk

**Phase mapping:** First thing to implement in clock input handling. Get this wrong and nothing else can be tested reliably.

**Confidence:** HIGH (verified against official VCV Rack Voltage Standards documentation)

---

## Moderate Pitfalls

---

### Pitfall 5: Clock Division/Multiplication Producing Wrong Ratios

**What goes wrong:** The Rate knob in clocked mode should select division/multiplication ratios (x1, x2, x4, /2, /4, etc.). Common implementation mistakes: (1) allowing continuous ratios that produce non-musical subdivisions; (2) implementing multiplication by outputting phase-advanced clocks instead of adjusting the LFO frequency; (3) getting the direction backwards (x2 should mean the LFO completes 2 cycles per clock period, not half a cycle).

**Why it happens:** In free-running mode, the Rate knob is continuous (0.01-20Hz). Reusing this continuous range in clocked mode produces values like "x1.37" which has no musical meaning. The knob needs to snap to discrete ratios in clocked mode.

**Consequences:** LFO never quite lines up with the beat. Divisions feel "off" to musicians. Non-standard ratios confuse users who expect x1, x2, x4, /2, /3, /4 etc.

**Prevention:**
1. **Define a fixed ratio table:** Common useful ratios: /16, /8, /4, /3, /2, x1, x2, x3, x4, x8, x16. Map the 0-1 knob range to indices into this table with equal-width zones.
2. **Snap the knob position to detent-like zones:** Apply quantization to the knob value before looking up the ratio. Display the current ratio (e.g., "x4" or "/2") on the display or in the tooltip.
3. **Dual-mode Rate knob:** The existing `configParam(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz")` needs different range, labels, and tooltip formatting in clocked mode. Use `paramQuantity->displayMultiplier` or override the display string dynamically.
4. **Test triplet ratios:** /3 and x3 are musically important but easy to accidentally omit from the ratio table.

**Detection:**
- LFO completes cycle at unexpected times relative to clock
- Display shows non-integer ratio
- Users report confusion about what the Rate knob does in clocked mode

**Phase mapping:** Part of the Rate knob dual-mode implementation phase.

**Confidence:** MEDIUM-HIGH (based on study of existing clocked LFO behaviors in Mutable Instruments Tides and VCV Fundamental LFO documentation)

---

### Pitfall 6: Tempo Change Causing Frequency Overshoot or Oscillation

**What goes wrong:** When upstream tempo changes (e.g., user rotates the BPM knob on a clock module), the measured period changes. If the LFO frequency tracks the measured period with too little smoothing, it jerks to the new tempo. If it smooths too much, it takes many beats to catch up. The worst case: the EMA smoothing constant is tuned for one tempo range but oscillates at another. For example, alpha=0.5 is stable at 120 BPM but at 30 BPM (2-second periods), each measurement has massive weight and the frequency ping-pongs.

**Why it happens:** Exponential smoothing with a fixed alpha treats fast and slow tempos identically in terms of samples-between-updates, but the musical expectation differs. At 120 BPM, you get 2 period measurements per second -- plenty of data. At 30 BPM, you get 0.5 per second -- each measurement dominates.

**Consequences:** At slow tempos, the LFO visibly lurches when tempo changes. At fast tempos, the smoothing makes tempo tracking feel sluggish. Users perceive the module as "laggy" or "twitchy" depending on the tempo.

**Prevention:**
1. **Use period-adaptive smoothing:** Scale alpha based on the number of measurements received recently. Start with alpha=0.7 for the first few measurements at a new tempo, then reduce to alpha=0.3 for steady-state tracking.
2. **Detect large tempo changes explicitly:** If the new period differs from the smoothed period by more than 25%, treat it as an intentional tempo change. Apply the new period with high weight (alpha=0.8) rather than the steady-state alpha.
3. **Rate-limit frequency changes:** Apply a slew to the LFO frequency itself (not just the period measurement). A 50ms slew on the frequency prevents audible jumps while tracking tempo changes within a few beats.
4. **Test with BPM automation:** Patch a CV source into the clock module's BPM input and sweep from 60 to 240 BPM. The LFO should track smoothly without overshoot.

**Detection:**
- Visible frequency jump in display when tempo changes
- LFO takes more than 3 beats to lock to new tempo
- Oscillating frequency visible in output when tempo is changing

**Phase mapping:** Refinement phase after basic clock sync works. Get the basic period measurement working first, then tune the smoothing behavior.

**Confidence:** MEDIUM (EMA smoothing is standard, but optimal alpha values require empirical tuning specific to this module's use cases)

---

### Pitfall 7: Display Not Updating Correctly in Clocked Mode

**What goes wrong:** The existing display update logic relies on phase wrapping (`phase < prevPhaseForDisplay`) to trigger waveform buffer redraws (line 266). In clocked mode, phase wrapping behavior changes: phase resets to 0.0 on clock edges (which triggers the wrap detection) but the LFO may also wrap naturally between clock edges if the division ratio is >1. Additionally, the phase-tracking dot and comet trail animations assume smooth continuous phase progression. A hard phase reset makes the dot teleport to the start, breaking the smooth animation.

**Why it happens:** The display code was designed for free-running operation with continuous, monotonic phase progression. Clock sync introduces discontinuities that violate the `phase < prevPhaseForDisplay` assumption -- any phase reset looks like a wrap, and multiple wraps per clock period (for multiplied ratios) produce excessive buffer rebuilds.

**Consequences:** Display flickers or redraws too frequently at high multiplication ratios. Dot animation glitches visibly on phase resets. Users see a "jump" in the animation that makes the module feel janky, even if the audio output is clean (thanks to crossfading). The display undermines confidence in the clock sync feature.

**Prevention:**
1. **Add a "clock reset" flag:** When a clock edge triggers a phase reset, set a flag that the display code can check. The display can then animate the dot smoothly back to the start (or skip the trail for that frame) instead of showing a teleport.
2. **Rate-limit display rebuilds in multiplied mode:** At x8 multiplication, the phase wraps 8 times per clock period. The display does not need to rebuild the waveform buffer 8 times per beat. Add a minimum interval between wrap-triggered rebuilds (e.g., 100ms).
3. **Show a clock sync indicator:** Add a small visual badge (e.g., a "CLK" label or a dot that blinks on each clock edge) so users know the module is tracking. This sets the expectation that behavior is different from free-running mode.
4. **Display the division ratio:** Show "x4" or "/2" on the display or in the Rate knob tooltip. This gives immediate feedback about what the Rate knob is doing in clocked mode.

**Detection:**
- Visual glitch when clock edge arrives (dot jumps, trail breaks)
- Display appears to flicker at high multiplication ratios
- Waveform buffer rebuilds visible as brief rendering stutter

**Phase mapping:** Display updates come after core clock sync is working. But plan the display changes during the design phase to avoid retrofitting.

**Confidence:** HIGH (direct analysis of existing display code in AnalogLFO.cpp lines 265-279 and 376-447)

---

### Pitfall 8: Cable Disconnection Not Reverting to Free-Running Mode

**What goes wrong:** When the user unplugs the CLK cable, the module should seamlessly revert to free-running mode using the Rate knob value. Common mistakes: (1) the module keeps running at the last clocked frequency indefinitely; (2) the module stops outputting because it is waiting for a clock edge that will never come; (3) the transition from clocked to free-running produces a frequency jump because the Rate knob was in "division ratio" mode and the raw value no longer maps to a sensible frequency.

**Why it happens:** The module checks `inputs[CLK_INPUT].isConnected()` but only on clock edges, not every sample. Or the module stores a "clocked mode" state that is not cleared when the cable is removed. Or the Rate knob parameter range was remapped for division mode and the unmapped value is now wrong.

**Consequences:** Module appears to "freeze" or "break" when CLK cable is removed. Users must restart the module or re-patch to recover. This is especially frustrating during a performance.

**Prevention:**
1. **Check `isConnected()` every process() call:** It is cheap (a pointer check) and authoritative. When the CLK input transitions from connected to disconnected, immediately enter free-running mode.
2. **Keep the Rate knob parameter range unchanged:** Do not modify `paramQuantity->minValue/maxValue` when switching modes. Instead, interpret the same 0.01-20Hz range differently: in free mode it is a direct frequency; in clocked mode it maps to the ratio table. This means the knob position is always valid in both modes.
3. **Smooth the frequency transition:** When switching from clocked to free-running (or vice versa), apply a short frequency slew (50-100ms) to avoid an abrupt frequency change. The LFO might be running at 2Hz (clocked x2 at 60 BPM) and the Rate knob says 0.7Hz -- a direct switch would be audible.
4. **Clear clock state on disconnect:** Reset `smoothedPeriod`, `clockEdgeCount`, and any EMA state when the cable is removed. Stale clock data should not influence behavior when the cable is reconnected later.
5. **Serialize the mode state?** Probably not. When a patch loads and the CLK cable is connected, the module should detect the first two clock edges and lock on. Saving clock period to JSON is fragile because the upstream clock may have changed. The OU drift state is already not serialized (by design, line 109 of PROJECT.md) -- treat clock state the same way.

**Detection:**
- Unplug CLK cable and verify LFO immediately returns to Rate knob frequency
- Replug CLK cable and verify clock sync re-engages within 2-3 beats
- Save/load patch with CLK connected and verify module re-syncs on load

**Phase mapping:** Must be handled in the core clock sync implementation. Backward compatibility (no CLK = unchanged behavior) is a hard requirement per PROJECT.md.

**Confidence:** HIGH (standard VCV Rack module design pattern; `isConnected()` behavior well-documented)

---

### Pitfall 9: Accumulating Timing Error from Integer Sample Counting

**What goes wrong:** Counting samples between clock edges using an integer counter introduces quantization error. At 44100Hz sample rate and 120 BPM (0.5s period), the period is exactly 22050 samples -- no error. But at 130 BPM (0.4615s period), the true period is 20353.846... samples. Rounding to 20354 or 20353 introduces a 0.005% error per beat. Over a long session (hundreds of beats), this accumulates as phase drift between the LFO and the clock.

**Why it happens:** Sample counting is inherently integer-quantized. The error is small per beat but cumulative. At musical tempos, the error is typically sub-millisecond per beat, but over 1000 beats (about 8 minutes at 120 BPM) it can reach several milliseconds -- enough to be noticeable as a slight "drift" in the sync.

**Consequences:** LFO slowly drifts out of phase with the clock over long sessions. Phase reset on each clock edge corrects this, but the correction becomes a periodic click (pitfall 1) if the accumulated error is large enough.

**Prevention:**
1. **Use floating-point time accumulation:** Instead of counting integer samples, accumulate elapsed time using `float timer += args.sampleTime`. This gives sub-sample precision and avoids integer quantization. VCV Rack's `dsp::Timer` does exactly this.
2. **Reset the timer on each clock edge:** The timer accumulates time since the last edge. When a new edge arrives, the accumulated time IS the measured period (in seconds, not samples). Reset the timer and start accumulating again.
3. **Use double precision for phase accumulation:** The existing code already uses `double phase` (line 32) -- this is good. A `double` has ~15 significant digits, meaning phase error from floating-point accumulation is negligible for any practical session length.
4. **Correct accumulated phase on each clock edge:** Even with perfect frequency tracking, tiny errors accumulate. On each clock edge, the phase should be exactly at 0.0 (or whatever the reset target is). The correction is the anti-click crossfade from pitfall 1.

**Detection:**
- Run the LFO clocked for 10+ minutes and compare output phase to clock phase
- Measure the phase at each clock edge -- it should not drift in one direction

**Phase mapping:** Use `dsp::Timer` from the start. This is a one-line architectural decision that prevents the problem entirely.

**Confidence:** HIGH (fundamental digital timing principle)

---

## Minor Pitfalls

---

### Pitfall 10: Clock Multiplication at High Ratios Producing Inaudible LFO

**What goes wrong:** At x16 multiplication with a 120 BPM clock (2Hz base), the LFO runs at 32Hz -- into the audio range. The waveform is no longer a useful modulation source; it is a low-frequency audio tone. The display becomes a blur. The user may not realize what happened.

**Prevention:** Consider capping the maximum ratio so the resulting frequency stays below 20Hz (the current Rate param maximum), or display a warning when the effective frequency exceeds the sub-audio range. Alternatively, allow it but label it clearly -- some users want audio-rate LFO from clock sync.

**Phase mapping:** Ratio table design phase.

**Confidence:** MEDIUM (design decision, not a technical bug)

---

### Pitfall 11: Phase Reset Interfering with Waveform Character Modeling

**What goes wrong:** The saw waveform has a "soft capacitor reset" (AnalogLFO.cpp line 117-123) that smooths the transition at phase 0.0. If the clock sync phase reset forces the phase to exactly 0.0, the soft reset region is entered abruptly, and the character modeling may interact unexpectedly with the crossfade anti-click logic (pitfall 1). Two different smoothing mechanisms fighting over the same phase region can produce unexpected waveform shapes.

**Prevention:** Ensure the anti-click crossfade operates on the final output (after character modeling), not on the raw phase. The character modeling should operate on the new phase position as if it were a normal cycle start. Test all four waveform shapes (sine, triangle, saw, square) with phase reset at various phase positions.

**Phase mapping:** During anti-click crossfade implementation. Requires testing with character knob at various settings.

**Confidence:** HIGH (direct analysis of character modeling code in AnalogLFO.cpp)

---

### Pitfall 12: Rate Knob Tooltip Showing Wrong Units in Clocked Mode

**What goes wrong:** The Rate knob is configured with `configParam(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz")`. When clocked, the knob represents a division/multiplication ratio, not a frequency. If the tooltip still shows "Rate: 3.50 Hz", users are confused because the actual LFO frequency depends on the clock.

**Prevention:** Override `ParamQuantity::getDisplayValueString()` to show the ratio (e.g., "x4" or "/2") when clocked, and the frequency in Hz when free-running. Update the `configParam` unit string dynamically, or use a custom `ParamQuantity` subclass.

**Phase mapping:** UI/UX polish phase after core clock sync works.

**Confidence:** HIGH (straightforward VCV Rack API usage)

---

### Pitfall 13: Not Handling the First Clock Pulse Gracefully

**What goes wrong:** On the first clock edge after cable connection (or patch load), there is no previous edge to measure a period against. The module does not know the clock tempo. If it resets the phase to 0.0 on this first edge, it has correct phase alignment but runs at the free-running Rate knob frequency until the second edge provides a period measurement. This produces one LFO cycle at the "wrong" speed -- noticeable if the Rate knob frequency and clock tempo differ significantly.

**Prevention:** Accept the one-cycle latency gracefully. On the first clock edge: reset the phase (for alignment) but keep the current frequency. On the second edge: compute the period and switch to clocked frequency. On the third edge: begin EMA smoothing. Document this: "clock sync locks within 2-3 beats" is an acceptable and expected behavior for any clocked module. Do not try to "guess" the tempo from a single pulse.

**Phase mapping:** Clock sync initialization logic.

**Confidence:** HIGH (matches behavior of Mutable Instruments Tides and other clocked Eurorack modules)

---

### Pitfall 14: Autosave Serialization Overhead for Clock State

**What goes wrong:** VCV Rack autosaves every 15 seconds, calling `dataToJson()`. If clock state variables are serialized unnecessarily (e.g., the raw timer value, smoothed period history, edge count), this adds overhead and the saved state is meaningless on reload because the clock may have changed.

**Prevention:** Do NOT serialize clock timing state (period measurements, EMA state, timer values). Only serialize the boolean "was clocked" state if needed for UI display purposes on reload. The module should re-acquire clock sync from scratch on patch load, just as it does for drift (OU layers are not serialized per the existing design decision).

**Phase mapping:** Serialization implementation, late in the milestone.

**Confidence:** HIGH (consistent with existing design decision for OU state)

---

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation | Priority |
|---|---|---|---|
| Clock input detection | Pitfall 4 (wrong thresholds), Pitfall 13 (first pulse) | Use `dsp::SchmittTrigger` with `process(x, 0.1f, 1.f)`, handle first-pulse gracefully | CRITICAL |
| Period measurement | Pitfall 3 (instability), Pitfall 9 (integer counting) | Use `dsp::Timer` (float), EMA smoothing with alpha 0.3-0.5 | CRITICAL |
| Phase reset | Pitfall 1 (clicks), Pitfall 11 (character interaction) | Crossfade 2-5ms on output, test all waveform shapes | CRITICAL |
| Drift interaction | Pitfall 2 (drift fighting sync) | Reduce drift authority to 1-2% in clocked mode, or redirect to waveform shape | CRITICAL |
| Division/multiplication | Pitfall 5 (wrong ratios), Pitfall 10 (audio-rate) | Fixed ratio table, snap quantization | MODERATE |
| Tempo tracking | Pitfall 6 (overshoot) | Period-adaptive EMA, large-change detection | MODERATE |
| Cable disconnect | Pitfall 8 (stuck in clocked mode) | Check `isConnected()` every process(), clear clock state | MODERATE |
| Display | Pitfall 7 (animation glitch) | Clock reset flag, rate-limit rebuilds, show sync indicator | MODERATE |
| Rate knob UX | Pitfall 5 (ratios), Pitfall 12 (tooltip) | Dual-mode interpretation, dynamic tooltip | LOW |
| Serialization | Pitfall 14 (overhead) | Do not serialize clock timing state | LOW |

---

## Interaction Matrix: Clock Sync vs. Existing Features

This module's unique challenge is that clock sync must interact cleanly with three existing systems that were designed for free-running operation:

| Existing System | Interaction with Clock Sync | Risk | Mitigation |
|---|---|---|---|
| **OU Drift Engine** | Drift modulates frequency, conflicting with clock-derived frequency | HIGH | Reduce drift scope in clocked mode (Pitfall 2) |
| **Waveform Character** | Character modeling at phase=0.0 interacts with reset crossfade | MEDIUM | Apply crossfade on final output, after character (Pitfall 11) |
| **Display System** | Phase resets trigger unwanted display rebuilds and dot teleport | MEDIUM | Clock reset flag, animation smoothing (Pitfall 7) |
| **Phase Accumulator** | Double-precision phase works fine; clock just resets it | LOW | Existing architecture is compatible |
| **CV Modulation** | CV inputs for morph/character/drift unaffected by clock | LOW | No change needed |
| **Lock-free Display Buffer** | Double-buffer pattern works fine with clocked updates | LOW | No change needed |

---

## Sources

### Official Documentation (HIGH confidence)
- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) - Schmitt trigger thresholds, clock/reset timing rules, trigger output standards
- [VCV Rack DSP Manual](https://vcvrack.com/manual/DSP) - Signal processing guidance
- [VCV Rack API: dsp::TSchmittTrigger](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TSchmittTrigger) - Trigger detection API
- [VCV Rack API: dsp::TTimer](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TTimer) - Timer accumulation API
- [VCV Rack API: dsp::PulseGenerator](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1PulseGenerator) - Trigger output API
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) - Module serialization, `dataToJson`/`dataFromJson`

### Community and Technical Discussions (MEDIUM confidence)
- [VCV Community: Example clock multiplier code](https://community.vcvrack.com/t/example-clock-multiplier-code/20570) - Period measurement patterns
- [VCV Community: Clock generators BPM accuracy](https://community.vcvrack.com/t/clock-generators-bpm-accuracy/20030) - Timing precision issues
- [VCV Community: Schmitt trigger help](https://community.vcvrack.com/t/help-schmitt-trigger/14774) - Threshold selection
- [VCV Community: VCV LFO Clock](https://community.vcvrack.com/t/vcv-lfo-clock/19755) - Rate knob behavior in clocked mode
- [KVR Audio: Why discontinuities pop](https://www.kvraudio.com/forum/viewtopic.php?t=182555) - Discontinuity artifact theory
- [JUCE Forum: LFO Clicks problem](https://forum.juce.com/t/lfo-clicks-problem/41475) - Phase reset click solutions

### Module References (MEDIUM confidence)
- [Mutable Instruments Tides Manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/tides_2018/manual/) - PLL clock tracking, division/multiplication behavior
- [VCV Fundamental LFO](https://library.vcvrack.com/Fundamental/LFO) - CLK input behavior reference
- [ModWiggler: LFO with sync and reset](https://www.modwiggler.com/forum/viewtopic.php?t=222452) - Reset discontinuity discussion
- [ModWiggler: Modules with clock/reset issues](https://www.modwiggler.com/forum/viewtopic.php?t=222946) - Real-world clock sync problems
- [Learning Modular: Master Clocks and Reset](https://learningmodular.com/master-clocks-reset/) - Clock/reset timing in Eurorack

### Direct Source Analysis (HIGH confidence)
- `/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/src/AnalogLFO.cpp` - Existing module implementation, drift engine, display system, phase accumulator
