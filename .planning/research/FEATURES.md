# Feature Landscape: Clock Sync LFO (v1.1)

**Domain:** VCV Rack / Eurorack clock-synchronized LFO
**Researched:** 2026-03-07
**Overall Confidence:** HIGH
**Scope:** NEW features only -- clock sync additions to existing Analog Series LFO

---

## Existing Foundation (v1.0 -- already built, must not be disrupted)

- Three-knob analog engine (Morph, Character, Drift)
- Four-shape parametric morph (Sine-Tri-Saw-Square)
- Per-shape analog character modeling
- Four-layer OU drift engine
- Real-time waveform display with phase-tracking dot, comet trail, glow
- CV inputs for all three knobs with trimpot attenuators
- Rate knob (0.01-20Hz linear, at x=18, y=86mm)
- Bipolar +/-5V single output
- 12HP panel with Forge Audio branding
- Lock-free double buffer for audio-to-display transfer
- Per-module Xoroshiro128Plus RNG

---

## Table Stakes

Features users expect from ANY clock-synced LFO. Missing any of these means the module feels broken or incomplete. Every reference module (VCV LFO-1, Batumi, Tides, Mutable Instruments Stages) implements all of these.

| Feature | Why Expected | Complexity | Dependencies on Existing | Notes |
|---------|--------------|------------|-------------------------|-------|
| **CLK trigger input jack** | Every synced Eurorack LFO has one. No jack = no sync. This is the physical entry point for the entire feature set. | Low | New `InputId` enum entry, panel SVG update, widget code for jack placement | Standard PJ301MPort jack. Must fit on existing 12HP panel without moving existing components. |
| **Edge detection via Schmitt trigger** | VCV Rack standard: low threshold 0.1V, high threshold 1.0V (per official Voltage Standards). Without proper hysteresis, noise causes false triggers. | Low | None -- new state variable | Use `dsp::SchmittTrigger` with `process(x, 0.1f, 1.f)`. Well-documented VCV SDK pattern. Single line of code per sample. |
| **Clock period measurement** | Core mechanism: measure elapsed time between consecutive rising edges to derive tempo. Every other feature depends on this measurement. | Low | None -- new state variables (`float clockPeriod`, `int samplesSinceEdge`) | Increment sample counter each `process()` call. On trigger, compute `period = counter * sampleTime`. Store for frequency derivation. |
| **Clock period smoothing** | Single-edge measurement is jittery (clock generators have timing variance). Users expect stable, non-wobbling tempo tracking. Batumi, Tides, and Stages all smooth over multiple edges. | Medium | Requires period measurement | Exponential moving average (EMA) is the right approach: `smoothed = alpha * new + (1-alpha) * smoothed` with alpha ~0.3-0.5. Responds to tempo changes within 2-3 edges. Simpler than ring buffer, no allocation. |
| **Dual-mode Rate knob** | When CLK connected: Rate selects division/multiplication ratio. When CLK disconnected: Rate works exactly as v1.0 (free Hz). This is exactly how VCV LFO-1 works -- FREQ knob becomes multiplier when CLK patched, with 1x at center. The canonical reference behavior. | Medium | Modifies existing `RATE_PARAM` interpretation in `process()` | Must be fully backward compatible: unpatched CLK = identical to v1.0. The knob does not physically change -- only its interpretation changes based on `inputs[CLK_INPUT].isConnected()`. |
| **Phase reset on clock edge** | LFO restarts from phase=0.0 on each relevant clock tick. Beat alignment is the entire point of clock sync. Batumi's SYNC mode does this. Without phase reset, the LFO drifts out of alignment with the beat. | Medium | Modifies existing `phase` accumulator (set `phase = 0.0` on trigger) | For x1 ratio: reset on every clock edge. For divisions (/2, /4): reset every Nth edge via counter. For multiplications (x2, x4): reset on clock edge, subdivide between edges. |
| **Clock division ratios** | Slower than clock: /1, /2, /4, /8 minimum. These map to standard musical note values (whole, half, quarter, eighth relative to clock). Every clocked module from Doepfer A-160-1 to Pamela's includes power-of-2 divisions. | Medium | Requires period measurement, modifies phase accumulation rate | Count incoming clock edges with a division counter. Phase frequency = `1.0 / (clockPeriod * divisionRatio)`. |
| **Clock multiplication ratios** | Faster than clock: x1, x2, x4, x8 minimum. Subdivide the measured period. Tides does 1-16x. Pamela's does up to 192x. Users need faster-than-clock modulation for tremolo-like effects synced to beat. | Medium | Requires smoothed period measurement (jitter more noticeable when multiplied) | Set LFO frequency to `multiplier / smoothedPeriod`. Phase accumulates at the multiplied rate. Jitter in period measurement is amplified by multiplier, so smoothing is critical. |
| **Visual sync indicator** | Users need to know at a glance whether the LFO is free-running or clock-locked. Ohmer KlokSpid uses green SYNC LED. Surge XT uses orange switch. Every clock module shows sync state. | Low | Existing NanoVG display infrastructure, new `std::atomic<bool>` for sync state | Small "SYNC" text badge or icon on the waveform display when CLK connected and receiving valid edges. Use existing forge amber (#e8a838). |
| **Division/multiplication label** | When synced, the Rate knob maps to a ratio, not Hz. Users need to see "/4" or "x2" on screen. Without this, the Rate knob is opaque in synced mode. | Low | Existing NanoVG display, requires ratio calculation | Text overlay on display showing current ratio string. Updates when Rate knob moves. Small, lower corner, unobtrusive. |

**Confidence:** HIGH -- these features are universally present across VCV LFO-1 (official), Batumi (industry standard hardware LFO), Tides (Mutable Instruments reference), Pamela's New/Pro Workout, and Mutable Instruments Stages. Verified through official documentation and multiple community sources.

---

## Differentiators

Features that would set this clock sync apart from typical implementations. Not expected, but would be noticed and valued.

| Feature | Value Proposition | Complexity | Dependencies on Existing | Notes |
|---------|-------------------|------------|-------------------------|-------|
| **Triplet and dotted ratios (/3, /6, x3, x6, /1.5, x1.5)** | Most basic clock-synced LFOs only offer power-of-2 ratios. Triplet divisions create polyrhythmic modulation that musicians actively seek. Part of why Pamela's Pro Workout is so popular. | Low (incremental over basic ratios) | Same mechanism as even divisions | Adds /3, /6, x3, x6 for triplet feel. /1.5 and x1.5 for dotted note relationships. Musical value per added complexity is very high. |
| **Drift interaction with clock sync** | Unique to Forge Audio: the existing OU drift engine could apply reduced modulation to the synced frequency, creating an LFO that follows the clock but with analog-style imperfection. No other VCV clock-synced LFO does this because no other has a drift engine. | Low | Existing drift engine -- just apply a scaled fraction to synced frequency | When synced, apply `driftScale * 0.25` (reduced from free-running 0.075) to the clock-derived frequency. Creates "analog clock follower" feel. Must be subtle enough not to break perceived sync. The drift knob at 0 = perfect digital sync; drift knob up = slightly wobbly vintage clock follower. |
| **Smooth clock-loss transition** | When clock stops, most modules either freeze abruptly or snap to free-running. A gradual transition to free-running at the last-known tempo feels more musical and less jarring. Mutable Instruments Stages implements adaptive timeout. | Medium | Requires timeout detection logic, modifies mode-switching code | Track time since last clock edge. Timeout threshold = 3x last smoothed period (adaptive, per Stages approach). After timeout, continue oscillating at last known frequency in free-running mode. On next valid clock edge, immediately re-engage sync. |
| **Animated sync badge** | Instead of static "SYNC" text, the badge pulses with the clock -- brief brightness increase on each clock edge, fading over ~100ms. Fits the existing "alive" display aesthetic with breathing dots and comet trails. | Low | Existing NanoVG display animation infrastructure | Store `lastClockTime` as atomic float. In display draw, compute fade = `1.0 - min(1.0, timeSinceEdge / 0.1)`. Modulate badge alpha. Complements existing display animation language. |
| **Extended ratio range (/16 to x16)** | Tides does 1/16 to 16x. Pamela's does /16384 to x192. Offering a wide range gives more creative options for both glacially slow modulation and fast rhythmic effects. | Low (just more entries in ratio table) | Same mechanism as basic ratios | 15 ratio steps across the knob range: /16, /8, /6, /4, /3, /2, /1.5, x1, x1.5, x2, x3, x4, x6, x8, x16. Comfortable spacing of ~6.7% per step. |

**Confidence:** MEDIUM -- these are design differentiators based on analysis of what existing modules lack, not explicitly community-requested features. The drift interaction is novel and untested in the wild.

---

## Anti-Features

Features to explicitly NOT build for clock sync. Either out of scope, over-engineered, or undermining the module's identity.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| **Separate RESET input jack** | Panel space on 12HP is precious -- we have room for exactly one new jack. A dedicated RESET jack separate from CLK adds panel complexity for a use case mostly covered by clock-edge phase reset. Batumi combines sync and reset behavior. | Phase reset happens automatically on CLK edge. If users need independent reset timing, they can use an external trigger combiner or sequential switch upstream. |
| **Phase offset knob or CV** | Would require a 4th panel knob or CV jack, breaking the three-knob engine identity that IS the brand. Phase offset is a niche feature for multi-LFO phase relationships. | Defer to a hypothetical future "quad LFO" module. The three-knob engine must remain three knobs. |
| **Swing/shuffle** | Adds rhythmic variation to clock divisions. Musically interesting but belongs in a clock generator (Pamela's) or clock modifier, not an LFO. Adding swing makes this module try to be a clock processor. | Users who want swing should swing the incoming clock before it reaches CLK input. Modular philosophy: each module does one thing well. |
| **Internal BPM/tempo mode** | Turning the LFO into its own clock generator with BPM display and tap tempo. This makes it a clock module, not an LFO. Scope creep. | The Rate knob in free-running mode already sets frequency in Hz. BPM display is a clock module feature (Impromptu Clocked, Pamela's). |
| **Tap tempo button on panel** | A physical panel button for manually tapping tempo. Requires panel real estate and is completely redundant when CLK input exists -- CLK input IS the tap tempo mechanism (measures period from triggers). | CLK input handles this. Users can use any external button/trigger module to tap. |
| **PLL (Phase-Locked Loop) sync** | A PLL continuously adjusts frequency to maintain phase coherence without hard reset. Massively over-engineered for sub-audio LFO rates. Introduces tracking delay, potential oscillation/hunting artifacts, and complex tuning. PLLs are for audio-rate VCO hard sync. | Simple period measurement + phase reset is sufficient and predictable at LFO rates. Users understand "measure tempo, reset phase" behavior intuitively. |
| **CV control of division ratio** | A CV input to modulate the clock division ratio externally. Adds another jack to an already-tight 12HP panel. Complex edge cases (what happens when ratio changes mid-cycle? Glitches, clicks, phase jumps). | The Rate knob handles ratio selection manually. CV-controlled clock division is a separate utility module's job (like Pamela's per-channel CV). |
| **Clock output / clock-through jack** | Re-emitting the measured clock or a derived clock output. This is clock distribution, not LFO territory. | Users who need clock distribution use dedicated clock modules (Impromptu Clocked, etc.). |
| **Separate Reset vs. Sync mode toggle** | Batumi has a back-panel jumper choosing between "Reset" mode (hard restart on each pulse) and "Sync" mode (adapt period from pulse timing). Adding a mode toggle increases complexity and user confusion for a subtle behavioral difference. | Implement ONE well-designed behavior that combines the best of both: measure period (Sync-style tempo tracking) AND reset phase on clock edge (Reset-style alignment). This is what users actually want -- tempo following with beat alignment. |
| **Continuous (non-snapped) ratio interpolation** | Allowing the Rate knob to smoothly interpolate between, say, /4 and /2. Sounds like it offers precision, but creates non-musical frequencies that defeat the purpose of clock sync. A ratio of 3.7x is meaningless musically. | Snap to nearest discrete ratio. The whole point of clock division is musical relationships. |

**Confidence:** HIGH -- these decisions align with the established "three-knob engine" philosophy, the 12HP panel constraint, and modular design principles verified across the Eurorack ecosystem.

---

## Feature Dependencies

```
CLK Input Jack (panel SVG + InputId enum + widget placement)
    |
    v
Edge Detection (dsp::SchmittTrigger, process per sample)
    |
    v
Clock Period Measurement (sample counter between edges)
    |
    +-------> Clock Period Smoothing (EMA, alpha ~0.3-0.5)
    |              |
    |              v
    |         Synced Frequency Derivation (ratio / smoothedPeriod)
    |              |
    |              v
    |         Rate Knob Remapping (free Hz -> snapped ratio table)
    |              |
    |              v
    |         Phase Accumulation Override (synced deltaPhase in process())
    |              |
    |              +---> Drift Interaction [differentiator]
    |                    (reduced OU scaling applied to synced frequency)
    |
    +-------> Phase Reset on Clock Edge (phase = 0.0)
    |              |
    |              v
    |         Division Counter (for /2, /4 etc. -- reset every N edges)
    |
    +-------> Sync State Detection (isConnected + validEdgeCount >= 2)
    |              |
    |              +---> Display: Sync Badge (atomic<bool> syncActive)
    |              |
    |              +---> Display: Ratio Label (atomic<int> currentRatioIndex)
    |              |
    |              +---> Display: Clock Pulse Flash [differentiator]
    |
    +-------> Clock-Loss Timeout (3x smoothedPeriod elapsed with no edge)
                   |
                   v
               Fallback to Free-Running at last known frequency
```

**Critical path:** CLK Jack -> Edge Detection -> Period Measurement -> Period Smoothing -> Ratio Mapping -> Phase Accumulation Override. Everything else branches off this spine.

**Existing code touch points:**
- `process()`: Add CLK detection before phase accumulation, conditionally override `deltaPhase`
- `AnalogLFO()` constructor: Add `configInput(CLK_INPUT, "Clock")`, new param config for rate tooltip
- `AnalogLFOWidget()`: Add CLK jack placement, potentially adjust layout
- `WaveformDisplay::drawLayer()`: Add sync badge and ratio label rendering
- Enums: Add `CLK_INPUT` to `InputId`

---

## Rate Knob Dual-Mode Specification

This is the most design-critical feature -- how the existing Rate knob changes behavior when CLK is connected.

### Free-Running Mode (no CLK patched) -- UNCHANGED from v1.0

- Range: 0.01 Hz to 20 Hz
- Linear mapping
- Direct frequency control
- Tooltip: "Rate: X.XX Hz"
- Code: `float freq = params[RATE_PARAM].getValue();`

### Synced Mode (CLK patched and receiving valid clock)

The Rate knob selects from discrete, snapped musical ratios:

| Knob Range | Ratio | Meaning | Display Label | Phase Reset Behavior |
|------------|-------|---------|---------------|---------------------|
| 0.00-0.03 | /16 | 16x slower than clock | "/16" | Reset every 16th clock edge |
| 0.04-0.10 | /8 | 8x slower | "/8" | Reset every 8th clock edge |
| 0.11-0.17 | /6 | 6x slower (dotted triplet) | "/6" | Reset every 6th clock edge |
| 0.18-0.24 | /4 | 4x slower | "/4" | Reset every 4th clock edge |
| 0.25-0.31 | /3 | 3x slower (triplet) | "/3" | Reset every 3rd clock edge |
| 0.32-0.38 | /2 | 2x slower | "/2" | Reset every 2nd clock edge |
| 0.39-0.45 | /1.5 | Dotted note | "/1.5" | Reset every 1.5 clocks (alternate 1,2 pattern) |
| 0.46-0.53 | x1 | Matches clock exactly | "x1" | Reset every clock edge |
| 0.54-0.60 | x1.5 | Dotted subdivision | "x1.5" | Reset on clock, extra cycle at 1.5x |
| 0.61-0.67 | x2 | 2x faster | "x2" | Reset on clock edge, 2 cycles between |
| 0.68-0.74 | x3 | 3x faster (triplet) | "x3" | Reset on clock edge, 3 cycles between |
| 0.75-0.81 | x4 | 4x faster | "x4" | Reset on clock edge, 4 cycles between |
| 0.82-0.88 | x6 | 6x faster | "x6" | Reset on clock edge, 6 cycles between |
| 0.89-0.95 | x8 | 8x faster | "x8" | Reset on clock edge, 8 cycles between |
| 0.96-1.00 | x16 | 16x faster | "x16" | Reset on clock edge, 16 cycles between |

**Rationale for ratio set:**
- Power-of-2 (/16, /8, /4, /2, x1, x2, x4, x8, x16) = standard in every clock module
- Triplets (/6, /3, x3, x6) = polyrhythmic musical value, what makes Pamela's popular
- Dotted (/1.5, x1.5) = common in DAW LFO sync, translates well to modular
- 15 positions across knob range = comfortable spacing
- Center position = x1 (1:1) matches VCV LFO-1 convention

**Snap behavior:** Knob snaps to nearest ratio. No interpolation between ratios -- continuous interpolation creates non-musical frequencies that defeat clock sync.

**Tooltip in synced mode:** "Rate: x2 (synced)" or "Rate: /4 (synced)"

---

## Clock Period Measurement Specification

### Algorithm

```
On each process() call:
    samplesSinceClock++

    if (clockTrigger.process(inputs[CLK_INPUT].getVoltage(), 0.1f, 1.f)):
        // Rising edge detected
        float newPeriod = samplesSinceClock * args.sampleTime
        samplesSinceClock = 0

        if (newPeriod >= 0.01f AND newPeriod <= 30.f):  // 100Hz to 0.033Hz guard
            if (smoothedPeriod <= 0):
                smoothedPeriod = newPeriod  // First edge: no smoothing
            else if (newPeriod / smoothedPeriod > 3.0 OR smoothedPeriod / newPeriod > 3.0):
                smoothedPeriod = newPeriod  // Tempo jump: reset smoothing
            else:
                smoothedPeriod = 0.4 * newPeriod + 0.6 * smoothedPeriod  // EMA

        clockEdgeCount++
        // Phase reset logic here
```

### Guard Rails

| Guard | Value | Reason |
|-------|-------|--------|
| Minimum period | 10ms (100Hz) | Faster than any reasonable LFO clock |
| Maximum period | 30s (0.033Hz) | Longer gaps likely mean clock stopped, not slow tempo |
| Outlier rejection | 3x ratio threshold | Tempo changes > 3x are treated as new tempo, not smoothed |
| Minimum edges for sync | 2 | Need at least 2 edges to establish a period |

### Clock-Loss Timeout

- Timeout threshold: `3.0 * smoothedPeriod`
- After timeout: set `syncActive = false`, continue oscillating at `1.0 / smoothedPeriod` (last known frequency) in free-running mode
- On next valid edge: immediately re-engage sync, reset phase
- Adaptive: timeout scales with tempo. Fast clock (100ms period) = 300ms timeout. Slow clock (5s period) = 15s timeout. This prevents false timeouts on slow clocks (a problem Mutable Instruments Stages specifically solved).

---

## Display Integration Specification

The existing waveform display needs three additions, all rendered through the existing NanoVG/FramebufferWidget pipeline.

### 1. Sync Badge

- **Content:** Text "SYNC" in upper-right corner of display area
- **Style:** Forge amber (#e8a838) at 50-70% opacity, small font (~6-8px)
- **Visibility:** Appears when `syncActive == true` (CLK connected AND >= 2 valid edges received AND not timed out)
- **Thread safety:** Read `std::atomic<bool> displaySyncActive` from audio thread

### 2. Ratio Label

- **Content:** Current ratio string (e.g., "x4", "/2", "x1") in lower-right corner
- **Style:** Same amber, slightly larger than sync badge (~8-10px)
- **Visibility:** Only when synced (same condition as sync badge)
- **Update:** Changes when Rate knob position crosses a ratio boundary
- **Thread safety:** Read `std::atomic<int> displayRatioIndex` from audio thread, map to string in GUI thread

### 3. Clock Pulse Flash (differentiator -- can be cut if performance issue)

- **Effect:** Brief brightness boost on the waveform trace on each clock edge
- **Implementation:** Store `std::atomic<float> lastClockTimestamp`. In draw, compute `flash = max(0, 1 - (currentTime - lastClockTimestamp) / 0.08)`. Multiply trace alpha by `1.0 + 0.15 * flash`.
- **Duration:** ~80ms fade, 15% peak brightness increase
- **Risk:** If this causes excessive dirty-flagging of the FramebufferWidget, cut it. Display performance is more important than this polish detail.

### Performance Considerations

- All new display data transferred via `std::atomic` variables (matching existing `displayPhase`, `displayDrift` pattern)
- No new allocations in audio thread
- Ratio label string lookup happens in GUI thread (map index to `const char*`)
- Sync badge and ratio label are static text -- only need redraw when state changes, not every frame

---

## Panel Layout Specification

The CLK jack must fit on the existing 12HP panel. Current bottom section:

```
Rate knob:        [RATE]
                   x=18, y=86

Trimpots (y=96):  [MorphCV]  [CharCV]  [DriftCV]
                   x=10       x=24       x=38

Jacks (y=108):    [MorphCV]  [CharCV]  [DriftCV]    [OUT]
                   x=10       x=24       x=38        x=52
```

### Recommended: CLK jack at x=52, y=96 (above OUT jack)

```
Trimpots (y=96):  [MorphCV]  [CharCV]  [DriftCV]    [CLK]
                   x=10       x=24       x=38        x=52

Jacks (y=108):    [MorphCV]  [CharCV]  [DriftCV]    [OUT]
                   x=10       x=24       x=38        x=52
```

**Rationale:**
- The spot at x=52, y=96 is currently empty (no component there in v1.0)
- CLK is visually grouped with the output section (right side) rather than the CV section (left/center)
- Functional grouping: CLK (timing input) near OUT (signal output) is logical
- Zero disruption to existing component positions
- CLK label goes between the jack positions, standard Eurorack labeling

---

## MVP Recommendation

### Must Have (minimum viable clock sync):
1. CLK input jack with `dsp::SchmittTrigger` edge detection
2. Clock period measurement with EMA smoothing
3. Dual-mode Rate knob (free Hz / snapped ratios)
4. Phase reset on clock edge (with division counter for /N ratios)
5. Power-of-2 ratios: /8, /4, /2, x1, x2, x4, x8
6. Sync badge on display ("SYNC" text)
7. Ratio label on display (current ratio string)
8. Panel SVG updated with CLK jack at x=52, y=96
9. Clock-loss timeout with free-running fallback

### Should Have (high-value additions):
10. Triplet ratios: /3, /6, x3, x6
11. Extended range: /16, x16
12. Dotted ratios: /1.5, x1.5
13. Guard rails on period measurement (min/max/outlier rejection)

### Nice to Have (defer if timeline tight):
14. Drift interaction with synced frequency
15. Animated sync badge (pulse with clock)
16. Clock pulse flash on display trace

### Defer to Later:
- Phase offset -- breaks three-knob identity
- Swing/shuffle -- clock generator territory
- CV control of division ratio -- panel space constraint
- Separate RESET jack -- panel space constraint

---

## Backward Compatibility Requirements (Non-Negotiable)

| Requirement | How to Verify |
|-------------|--------------|
| No CLK patched = identical behavior to v1.0 | Rate knob range, phase accumulation, drift, output voltage all unchanged. A/B test: v1.0 and v1.1 with same knob positions must produce identical output. |
| Existing patches load correctly | New params/inputs must have safe defaults. `CLK_INPUT` unconnected = free mode. No new params that alter existing behavior. |
| No CPU regression in free-running mode | Clock detection code early-exits when `!inputs[CLK_INPUT].isConnected()`. The `isConnected()` check is essentially free. |
| Display unchanged when not synced | No sync badge, no ratio label, no visual artifacts from sync code when CLK is unpatched. |
| Serialization backward compatible | v1.0 presets load in v1.1 without errors. New inputs/params use safe defaults. VCV Rack SDK handles missing inputs gracefully. |
| Output voltage range unchanged | Still bipolar +/-5V. Clock sync changes frequency/phase, not amplitude. |

---

## Sources

### HIGH Confidence (Official documentation, SDK API references)
- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) -- trigger thresholds (0.1V low, 1V high), trigger duration (1ms at 10V), clock signal conventions
- [VCV Rack API: dsp::TSchmittTrigger](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TSchmittTrigger) -- edge detection API, `process(value, lowThreshold, highThreshold)`
- [VCV Rack API: dsp::TTimer](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TTimer) -- timing utility for period measurement
- [VCV Rack API: dsp::ClockDivider](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1ClockDivider) -- clock division utility
- [VCV Rack API: dsp::PulseGenerator](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1PulseGenerator) -- pulse generation utility
- [VCV Free Modules](https://vcvrack.com/Free) -- VCV LFO-1: CLK input syncs frequency (not phase), FREQ knob becomes multiplier with 1x at center

### MEDIUM Confidence (Multiple manufacturer/community sources agree)
- [Xaoc Devices Batumi](https://xaocdevices.com/main/batumi/) -- Reset vs Sync modes: Reset shortens cycle, Sync adapts period (tap tempo). Setting affects all 4 LFOs via back-panel jumper.
- [Mutable Instruments Tides Manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/tides_2018/manual/) -- Clock mode: frequency knob becomes ratio control (1/16 to 16x). External clock as reference.
- [Pamela's Pro Workout](https://busycircuits.com/pages/alm034) -- Division/multiplication x192 to /16384 including non-integer. Color display with real-time visualization.
- [Pamela's NEW Workout](https://busycircuits.com/pages/alm017) -- Division /512 to x48 including non-integer factors. OLED display.
- [VCV Community: Clock multiplier implementation](https://community.vcvrack.com/t/example-clock-multiplier-code/20570) -- Developer discussion of clock multiplication in VCV Rack
- [VCV Community: LFO Clock discussion](https://community.vcvrack.com/t/vcv-lfo-clock/19755) -- User expectations for CLK input behavior
- [ModWiggler: Syncable/Clockable LFOs](https://modwiggler.com/forum/viewtopic.php?t=165407) -- User expectations for synced LFOs across hardware ecosystem
- [ModWiggler: LFO with sync and reset](https://www.modwiggler.com/forum/viewtopic.php?t=222452) -- Community discussion of reset vs sync behavior preferences
- [Mutable Instruments Stages firmware](https://github.com/qiemem/eurorack/tree/v1.1.0/stages) -- Adaptive clock-loss timeout (adapts to cycle duration, logic from Marbles/Tides 2)
- [Keith McMillen: Clock Division](https://www.keithmcmillen.com/blog/simple-synthesis-part-13-clock-division/) -- Standard division ratios (/2, /4, /8, /16, /32, /64)

### LOW Confidence (Single source, training data only)
- Specific EMA alpha value (0.3-0.5) -- based on DSP best practice knowledge, not verified in VCV context
- Outlier rejection threshold (3x) -- engineering judgment, not empirically validated for this use case
