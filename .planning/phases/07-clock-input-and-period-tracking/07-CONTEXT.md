# Phase 7: Clock Input and Period Tracking - Context

**Gathered:** 2026-03-07
**Status:** Ready for planning

<domain>
## Phase Boundary

Detect clock edges, measure period, handle loss/outliers. The LFO reliably tracks incoming clock tempo from any VCV Rack clock source. This is pure DSP infrastructure -- no display changes, no ratio table, no anti-click crossfade (those are Phases 8-10).

</domain>

<decisions>
## Implementation Decisions

### First-pulse behavior
- First clock pulse resets phase to exactly 0.0 (hard reset, no crossfade -- Phase 9 adds anti-click later)
- LFO continues at free-running Rate knob frequency between pulse 1 and pulse 2
- On pulse 2, snap instantly to clock-derived frequency (no slew)
- On clock loss timeout, snap back to Rate knob frequency instantly (Phase 9 adds smooth slew)

### Acquisition state model
- Three states: FREE -> ACQUIRING -> LOCKED
- ACQUIRING starts on pulse 1 (phase reset, but frequency stays free-running)
- Use measured period immediately from pulse 2 onward, even during ACQUIRING
- Transition to LOCKED after 3-4 consistent edges (EMA convergence)
- Outlier filter only active in LOCKED state -- ACQUIRING accepts all measurements
- Clock state exposed as std::atomic<int> for lock-free display access (same pattern as displayPhase/displayDrift)

### Fast-track re-acquisition
- On clock loss, remember last smoothed period
- If new clock arrives within ~20% of last known period, skip ACQUIRING and fast-track to LOCKED
- If new clock is outside 20% tolerance, full re-acquisition from ACQUIRING

### Outlier rejection
- Symmetric 3x threshold (rejects both speedups and slowdowns exceeding 3x current period)
- Outliers silently ignored -- discard measurement, keep current smoothed period
- Let timeout handle intentional tempo changes (reject outliers -> timeout -> FREE -> re-acquire new tempo)

### Timeout scaling
- Base timeout: 3x smoothed period
- Floor: 1 second (prevents false timeout at fast tempos like 300 BPM)
- Ceiling: 5 seconds (prevents excessive wait at slow tempos like 20 BPM)
- Formula: clamp(3.0 * smoothedPeriod, 1.0, 5.0)

### Cable disconnect
- Instant revert to FREE when CLK jack is physically disconnected (using VCV isConnected())
- Timeout only applies when cable is connected but pulses stop

### Claude's Discretion
- Exact EMA alpha value (requirements say ~0.3, Claude can tune)
- Exact edge count for ACQUIRING -> LOCKED transition (3 or 4, based on convergence math)
- Internal struct organization for clock state
- Timer implementation details

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `rack::dsp::SchmittTrigger`: VCV SDK built-in, standard 0.1V/1.0V thresholds -- use directly for CLK edge detection
- `progressiveCurve()`: x^2 curve for character/drift, not needed here but establishes pattern
- Xoroshiro128Plus RNG and OU layers: drift engine pattern, not directly relevant but shows struct organization style

### Established Patterns
- Phase accumulation: `double phase` with `double deltaPhase` for precision at low frequencies -- clock period tracking should maintain same precision
- Atomic display transfer: `displayPhase`, `displayDrift` as `std::atomic<float>`, `displayReadIdx` as `std::atomic<int>` -- clock state should follow this pattern
- CV processing: additive offset + attenuator + clamp pattern for all CV inputs
- No serialization of transient state: OU drift state not saved (fresh on patch load) -- clock state should follow same philosophy

### Integration Points
- `InputId` enum: add `CLK_INPUT` after `CHARACTER_CV_INPUT`
- `process()` function: clock detection logic goes at the top, before rate/phase accumulation
- `AnalogLFOWidget` constructor: CLK jack placement in bottom section (y=108mm row with other jacks, or separate position TBD in Phase 10)
- Rate calculation: currently `float freq = params[RATE_PARAM].getValue()` -- Phase 8 will override this with clock-derived frequency when LOCKED, but Phase 7 just needs to store the measured period

</code_context>

<specifics>
## Specific Ideas

- Keep Phase 7 deliberately simple: hard resets, instant transitions, no slew. Phases 8-9 layer smoothness on top.
- The three-state model (FREE/ACQUIRING/LOCKED) maps naturally to display behavior in Phase 10 (no badge / blinking badge / solid badge).
- Timeout clamping (1s-5s) ensures responsive behavior across the full BPM range VCV users encounter.

</specifics>

<deferred>
## Deferred Ideas

None -- discussion stayed within phase scope

</deferred>

---

*Phase: 07-clock-input-and-period-tracking*
*Context gathered: 2026-03-07*
