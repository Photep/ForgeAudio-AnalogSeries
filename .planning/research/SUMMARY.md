# Project Research Summary

**Project:** Forge Audio - Analog Series v1.1 (Clock Sync)
**Domain:** Clock-synchronized LFO in VCV Rack 2
**Researched:** 2026-03-07
**Confidence:** HIGH

## Executive Summary

The v1.1 milestone adds clock synchronization to the existing Analog Series LFO -- a well-understood feature in the Eurorack ecosystem with clear reference implementations (VCV Fundamental LFO-1, Mutable Instruments Tides, Xaoc Batumi). The approach is straightforward: a single CLK input jack feeds a Schmitt trigger for edge detection, a float-accumulating timer measures the period between edges, an EMA filter smooths the measurement, and the Rate knob reinterprets its range as discrete musical division/multiplication ratios when a clock cable is connected. All required components are VCV Rack SDK built-ins already validated in v1.0. No new dependencies, source files, or build changes are needed. The entire feature adds approximately 50-80 lines to the existing 538-line `AnalogLFO.cpp`.

The recommended approach is to treat this as a targeted modification of the existing `process()` function: insert clock tracking before frequency computation, conditionally override the frequency source (clock-derived ratio vs. Rate knob Hz), and add phase reset on clock edges. The existing drift, morph, character, and display systems remain untouched in free-running mode. The critical design decision is how drift interacts with clock sync -- the OU drift engine was designed for free-running operation and actively fights clock alignment. Drift authority must be reduced in clocked mode (from 7.5% to ~2% frequency deviation) to prevent accumulating phase errors that cause clicks on reset.

The primary risks are: phase reset discontinuity producing audible clicks (mitigate with 2-5ms output crossfade), drift engine fighting clock sync (mitigate by scaling drift down in clocked mode), and clock period measurement instability with irregular or changing-tempo sources (mitigate with EMA smoothing and outlier rejection). All three risks have well-documented prevention strategies, and the module's backward compatibility is guaranteed by design -- `isConnected()` gates all clock behavior, so no CLK cable means identical v1.0 behavior.

## Key Findings

### Recommended Stack

No new dependencies. Every component needed for clock sync is already available in the VCV Rack 2 SDK and was already listed (though not all used) in the v1.0 stack. The addition is purely about using existing SDK primitives for a new purpose.

**Core technologies (new usage):**
- **`dsp::SchmittTrigger`:** Edge detection on CLK input -- SDK built-in with standard 0.1V/1.0V thresholds matching VCV voltage standards
- **`dsp::Timer` (TTimer):** Float-precision time accumulation between clock edges -- avoids integer sample-counting quantization error
- **`dsp::ExponentialFilter`:** Period smoothing via EMA -- already used for parameter smoothing in v1.0, same utility applied to clock tracking
- **NanoVG (bundled):** Sync badge and division label rendering in existing display -- `nvgText()` with system font for dynamic ratio labels

**Explicitly rejected:** `dsp::ClockDivider` (sample-counting utility, NOT musical division), PLL (overkill for LFO rates), external libraries (nonexistent in VCV ecosystem), `std::mutex` (violates real-time audio constraints), serialization of clock state (transient by design).

**Build changes:** None. No new files, no new includes, no Makefile modifications.

### Expected Features

**Must have (table stakes):**
- CLK trigger input jack at x=52, y=96mm (empty space on existing 12HP panel)
- Edge detection via `dsp::SchmittTrigger` with VCV standard thresholds
- Clock period measurement with EMA smoothing (alpha ~0.3)
- Dual-mode Rate knob (free Hz when unpatched, snapped ratios when clocked)
- Phase reset on clock edge with division-aware counting
- Power-of-2 division/multiplication ratios: /8, /4, /2, x1, x2, x4, x8
- Sync badge ("SYNC" text) on waveform display
- Division ratio label on display
- Clock-loss timeout with automatic free-running fallback

**Should have (differentiators):**
- Triplet ratios (/3, /6, x3, x6) and dotted ratios (/1.5, x1.5) -- 15 total positions
- Extended range (/16 to x16) for creative extremes
- Drift interaction with clock sync -- reduced OU authority for "analog clock follower" feel
- Guard rails on period measurement (min 10ms, max 30s, 3x outlier rejection)

**Defer (v2+):**
- Phase offset knob/CV -- breaks three-knob identity
- Swing/shuffle -- clock generator territory, not LFO
- CV control of division ratio -- panel space constraint
- Separate RESET jack -- panel space constraint
- Animated sync badge (clock-pulse flash) -- cut if display performance issue
- Tap tempo -- CLK input IS tap tempo

### Architecture Approach

The clock sync integrates directly into the existing monolithic `AnalogLFO` struct as a set of member variables and one helper method (`processClockInput()`). No new classes, no separate files. The architecture follows a simple insertion pattern: clock tracking runs at the top of `process()`, conditionally overrides the frequency source, and everything downstream (drift, morph, character, display) is unaware of whether the frequency came from the Rate knob or the clock. This is architecturally clean because the clock sync's output is just a float frequency value, exactly what the existing code expects.

**Major components (modified):**
1. **Clock Tracker (inline)** -- `SchmittTrigger` + float timer + EMA smoother + validity flag; ~30 lines of state and one method
2. **Ratio Table (static)** -- 13-15 discrete musical ratios (/8 through x8 or /16 through x16); Rate knob maps to nearest index
3. **Phase Reset Logic** -- Division-aware edge counter; resets phase only on the Nth clock edge for /N ratios
4. **Display Overlay** -- Two new `std::atomic` variables (`displayClockValid`, `displayRatioIndex`) following existing lock-free pattern; NanoVG text rendering for badge and label
5. **Panel SVG** -- One new jack hole at x=52, y=86mm (or y=96mm) with "CLK" label

### Critical Pitfalls

1. **Phase reset discontinuity (clicks)** -- Hard-resetting phase to 0.0 on clock edges produces voltage jumps up to 6.2V on a sine wave. Prevent with a 2-5ms cosine crossfade on the output side. Apply crossfade after character modeling to avoid interaction with saw soft-reset. Test with LFO modulating a resonant filter.

2. **Drift engine fighting clock sync** -- OU drift modifies `deltaPhase` by up to 7.5%, accumulating phase error between clock edges that amplifies reset clicks. Prevent by reducing drift authority to ~2% in clocked mode. The drift knob at 0 = perfect digital sync; drift up = subtle analog wobble that stays within acceptable phase error.

3. **Clock period measurement instability** -- Single-edge measurement is jittery; first pulse has no reference; tempo changes cause frequency overshoot. Prevent with EMA smoothing (alpha 0.3), first-edge snap (no smoothing), outlier rejection (3x threshold for tempo jumps), and adaptive timeout (3x smoothed period).

4. **Wrong Schmitt trigger thresholds** -- Non-standard thresholds break interoperability with certain clock sources. Prevent by using exactly `dsp::SchmittTrigger::process(x, 0.1f, 1.f)` per VCV Voltage Standards. Test with multiple clock modules.

5. **Division phase reset on every edge** -- Resetting phase on every clock edge at /4 ratio means the waveform never completes a full cycle. Prevent with a beat counter that only resets phase every Nth edge for /N divisions.

## Implications for Roadmap

Based on research, the v1.1 clock sync milestone divides into 4-5 phases along a strict dependency chain. The critical path is: CLK input -> edge detection -> period measurement -> ratio mapping -> phase accumulation override. Everything else branches off this spine.

### Phase 1: Clock Input and Period Tracking
**Rationale:** Everything depends on detecting clock edges and measuring the period. This is the foundation -- if edges are not detected correctly or the period is unstable, nothing else can work. Get this right first and validate in isolation.
**Delivers:** CLK_INPUT enum entry, `configInput()`, `processClockInput()` method with SchmittTrigger edge detection, float timer period measurement, EMA smoothing, clock validity tracking, cable disconnect handling, clock-loss timeout.
**Addresses:** CLK jack, edge detection, period measurement, period smoothing, clock-loss fallback (table stakes)
**Avoids:** Pitfall 4 (wrong thresholds -- use SDK SchmittTrigger), Pitfall 9 (integer counting -- use float timer), Pitfall 3 (instability -- EMA smoothing), Pitfall 8 (stuck in clocked mode -- check isConnected every process call), Pitfall 13 (first pulse -- phase reset but keep free-run frequency until second edge)

### Phase 2: Frequency Override and Ratio Table
**Rationale:** With period tracking working, the next step is using it to drive the LFO frequency. The dual-mode Rate knob and ratio table are the core functional change -- this is where the module actually "syncs" to the clock.
**Delivers:** Static ratio table (13-15 musical divisions/multiplications), `getRatioIndex()` mapping from Rate knob position to table index, conditional frequency computation in `process()` (clock-derived vs. Rate knob Hz), snap quantization to discrete ratios.
**Addresses:** Dual-mode Rate knob, division ratios, multiplication ratios, triplet/dotted ratios (table stakes + differentiators)
**Avoids:** Pitfall 5 (wrong ratios -- discrete table, no continuous interpolation), Pitfall 6 (tempo overshoot -- frequency derived from already-smoothed period)

### Phase 3: Phase Reset and Drift Integration
**Rationale:** Phase reset is what makes clock sync feel "locked." It depends on both period tracking (Phase 1) and division ratios (Phase 2) because division-aware counting requires knowing the current ratio. Drift interaction must be designed alongside phase reset because drift directly affects the magnitude of phase error at reset time.
**Delivers:** Phase reset on clock edge, division-aware beat counter, anti-click crossfade (2-5ms cosine on output), drift authority reduction in clocked mode, smooth clock-to-free transition.
**Addresses:** Phase reset, drift interaction with sync (differentiator), smooth clock-loss transition (differentiator)
**Avoids:** Pitfall 1 (clicks -- crossfade), Pitfall 2 (drift fighting sync -- reduced authority), Pitfall 11 (character interaction -- crossfade on final output after character)

### Phase 4: Display and Panel
**Rationale:** Pure visual layer with no functional dependency except reading state from Phases 1-3. Can be started in parallel with Phase 3 but is listed after because it is lower priority. The panel SVG update is trivial and independent.
**Delivers:** "SYNC" badge on waveform display, division ratio label, `displayClockValid` and `displayRatioIndex` atomics, NanoVG font rendering setup, panel SVG update with CLK jack hole and label, `addInput()` widget call.
**Addresses:** Sync badge, ratio label, CLK jack panel placement (table stakes)
**Avoids:** Pitfall 7 (display glitch -- clock reset flag, rate-limit rebuilds at high multiplication ratios)

### Phase 5: Polish and Edge Cases
**Rationale:** Refinement that depends on all prior phases being functionally complete. Rate knob tooltip, stress testing, edge case handling.
**Delivers:** Custom `ParamQuantity` for Rate knob tooltip in clocked mode (shows "x4 (synced)" instead of "3.50 Hz"), animated sync badge (if performance allows), stress testing for rapid connect/disconnect, extreme ratios, very slow/fast clocks.
**Addresses:** Rate knob tooltip (minor pitfall), animated clock pulse flash (differentiator, cuttable)
**Avoids:** Pitfall 12 (tooltip wrong units), Pitfall 10 (audio-rate LFO at high multiplication -- display warning or cap)

### Phase Ordering Rationale

- **Strict dependency chain:** Period tracking -> ratio mapping -> phase reset -> display. Each layer requires the previous to be testable.
- **Audio before visual:** Get the clock sync sounding correct (Phases 1-3) before making it look correct (Phase 4). Clicks and drift artifacts matter more than missing display badges.
- **Drift integrated with phase reset:** These two concerns are tightly coupled -- drift magnitude determines phase error at reset, which determines click severity. Designing them together in Phase 3 prevents retrofitting.
- **Panel SVG is trivial:** The jack position (x=52, y=86mm or y=96mm) is in empty space. No existing components move. This can happen any time.

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 3 (Phase Reset + Drift):** The anti-click crossfade interacts with the existing character modeling (saw soft-reset region) in ways that need empirical testing. The drift authority reduction factor (2% vs 1% vs 3%) needs perceptual tuning. This is the phase most likely to require iteration.

Phases with standard patterns (skip research-phase):
- **Phase 1 (Clock Input):** Entirely standard VCV Rack SDK patterns. SchmittTrigger + Timer + EMA is the canonical approach, well-documented with community examples.
- **Phase 2 (Ratio Table):** Pure math -- a static array and an index calculation. No unknowns.
- **Phase 4 (Display):** Follows the exact same atomic pattern established in v1.0. NanoVG font rendering is standard.
- **Phase 5 (Polish):** Custom ParamQuantity is a documented VCV Rack pattern.

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | All components are VCV Rack SDK built-ins validated in v1.0. Zero new dependencies. API stable across 2.0-2.5. |
| Features | HIGH | Table stakes verified across 6+ reference modules (VCV LFO-1, Batumi, Tides, Pamela's, Stages, Doepfer A-160-1). Feature set is unambiguous. |
| Architecture | HIGH | Monolithic inline approach matches existing codebase style. Integration points are minimal (one new enum, one helper method, two conditionals in process()). Direct source code analysis confirms feasibility. |
| Pitfalls | MEDIUM-HIGH | Critical pitfalls (clicks, drift conflict, period instability) are well-grounded in DSP fundamentals and confirmed by community reports. Moderate pitfalls are based on sound engineering judgment. Specific EMA alpha values and crossfade durations need empirical tuning. |

**Overall confidence:** HIGH

### Gaps to Address

- **Anti-click crossfade duration:** The 2-5ms range is a starting point. Too short and clicks remain audible; too long and the waveform transition sounds "smeared." Needs A/B testing with the LFO modulating filter cutoff and VCA gain at various clock rates.
- **EMA alpha tuning:** 0.3 is the recommended default but the optimal value depends on real-world clock jitter characteristics in VCV Rack. Test with multiple clock sources (Fundamental CLKD, Impromptu CLOCKED, MIDI clock) and both steady and changing tempos.
- **Drift authority in clocked mode:** The 2% figure is an engineering estimate. Too high and clicks persist; too low and the "analog clock follower" character is lost. Needs perceptual testing with Drift knob across its full range while clocked.
- **CLK jack position:** Two positions proposed -- x=52,y=86mm (across from Rate knob, per ARCHITECTURE.md) and x=52,y=96mm (above OUT jack, per FEATURES.md). Both are in empty space. Final decision during panel layout phase.
- **Ratio table size:** 13 ratios (ARCHITECTURE.md) vs 15 ratios (FEATURES.md). The 15-ratio set includes /16, x16, /1.5, x1.5 which add musical value at negligible complexity. Recommend the 15-ratio set.

## Sources

### Primary (HIGH confidence)
- [VCV Rack Voltage Standards](https://vcvrack.com/manual/VoltageStandards) -- trigger thresholds (0.1V/1.0V), clock timing conventions
- [VCV Rack API: dsp::TSchmittTrigger](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TSchmittTrigger) -- edge detection API
- [VCV Rack API: dsp::TTimer](https://vcvrack.com/docs-v2/structrack_1_1dsp_1_1TTimer) -- time measurement API
- [VCV Rack API: dsp namespace](https://vcvrack.com/docs-v2/namespacerack_1_1dsp) -- full DSP utilities
- [VCV Rack DSP Manual](https://vcvrack.com/manual/DSP) -- general DSP guidance
- Existing codebase: `src/AnalogLFO.cpp` (538 lines, directly analyzed)

### Secondary (MEDIUM confidence)
- [VCV Fundamental LFO](https://library.vcvrack.com/Fundamental/LFO) -- CLK input syncs frequency, FREQ knob becomes multiplier
- [Xaoc Devices Batumi](https://xaocdevices.com/main/batumi/) -- Reset vs Sync modes
- [Mutable Instruments Tides Manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/tides_2018/manual/) -- Clock mode with 1/16 to 16x range
- [Pamela's Pro Workout](https://busycircuits.com/pages/alm034) -- Division/multiplication reference
- [VCV Community: Clock multiplier code](https://community.vcvrack.com/t/example-clock-multiplier-code/20570) -- Implementation patterns
- [VCV Community: TTimer usage](https://community.vcvrack.com/t/ttimer-understand-how-to-use-it/20987) -- Practical Timer patterns

### Tertiary (LOW confidence)
- Specific EMA alpha value (0.3) -- engineering judgment, needs empirical validation
- Anti-click crossfade duration (2-5ms) -- standard DSP practice, needs tuning for this specific module
- Drift authority scaling (2%) -- estimated from source code analysis, needs perceptual validation

---
*Research completed: 2026-03-07*
*Ready for roadmap: yes*
