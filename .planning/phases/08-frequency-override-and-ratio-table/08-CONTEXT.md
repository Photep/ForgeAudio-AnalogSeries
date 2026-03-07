# Phase 8: Frequency Override and Ratio Table - Context

**Gathered:** 2026-03-07
**Status:** Ready for planning

<domain>
## Phase Boundary

The Rate knob seamlessly switches between free-running Hz and musical clock divisions/multiplications. When CLK is disconnected, the knob behaves identically to v1.0 (continuous Hz). When CLK is connected and tracking, the knob snaps to 15 discrete musical ratios. This phase implements the ratio table, knob mapping, frequency override, and custom tooltip -- no phase reset, no crossfade, no display changes (those are Phases 9-10).

</domain>

<decisions>
## Implementation Decisions

### Ratio zone mapping
- Equal spacing: each of 15 ratios gets an equal slice of the knob's 0-1 normalized range
- Ratio table: /16, /8, /6, /4, /3, /2, /1.5, x1, x1.5, x2, x3, x4, x6, x8, x16
- Stored as a static const float array of 15 ratio multiplier values
- Index derived from knob position: `round(knobNormalized * 14)`
- Knob's internal parameter range stays 0.01-20Hz (no reconfiguration on clock connect/disconnect)
- When clock first connects, snap to nearest ratio based on current knob position
- When clock disconnects, instant restore to Hz meaning (consistent with Phase 7's instant-revert-to-FREE decision)

### Tooltip and display labels
- Custom ParamQuantity for RATE_PARAM
- Free-running mode: shows standard "Rate: X.XX Hz" (identical to v1.0)
- Clocked mode: shows ratio label with synced indicator, e.g., "x4 (synced)"
- Division format: "/4" for divisions, "x4" for multiplications
- Fractional ratios: "/1.5" and "x1.5" (decimal, not unicode fractions)
- Unison: "x1 (synced)" -- same format as all other ratios

### Ratio transition feel
- No hysteresis: pure nearest-snap to closest ratio from knob position
- Immediate frequency jump when ratio changes (new ratio applied on next sample)
- No queuing to next clock edge -- instant response to knob movement
- Current ratio index exposed as std::atomic<int> for lock-free display access (same pattern as displayClockState, displayPhase, displayDrift)

### Claude's Discretion
- Exact ParamQuantity subclass structure
- Whether ratio multiplier values are stored as float or double
- Internal variable naming conventions
- How normalized knob position is extracted from the 0.01-20Hz param range

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `clockState` and `smoothedPeriod` from Phase 7: clock tracking infrastructure already in place at line 68-71
- `displayClockState` atomic pattern: established lock-free display transfer, ratio index follows same pattern
- `processClockInput()`: called at top of process(), ratio override goes right after it

### Established Patterns
- Phase accumulation: `double deltaPhase = (double)freq * (double)args.sampleTime` at line 336 -- freq override replaces the Rate param value when clocked
- Atomic display transfer: `std::atomic<int>` for state, `std::atomic<float>` for continuous values
- No serialization of transient state: ratio selection derived from knob position + clock state, not saved
- configParam with unit suffix: `configParam(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz")` -- ParamQuantity replaces this

### Integration Points
- Line 332: `float freq = params[RATE_PARAM].getValue()` -- this is THE override point. When LOCKED or ACQUIRING with smoothedPeriod, `freq = (1.0f / smoothedPeriod) * ratioTable[ratioIndex]`
- Line 293: `configParam(RATE_PARAM, ...)` -- replace with custom ParamQuantity
- Line 73: `std::atomic<int> displayClockState` -- add `std::atomic<int> displayRatioIndex` nearby

</code_context>

<specifics>
## Specific Ideas

- The ratio table is musically standard: /16 through x16 covers the full range from very slow sweeps (16 clock cycles per LFO cycle) to rapid trills (16 LFO cycles per clock cycle)
- The /1.5 and x1.5 ratios correspond to dotted-note and triplet-feel divisions -- important for musical use
- Keep this phase deliberately simple like Phase 7: immediate jumps, no smoothing. Phase 9 layers crossfade and slew on top.

</specifics>

<deferred>
## Deferred Ideas

None -- discussion stayed within phase scope

</deferred>

---

*Phase: 08-frequency-override-and-ratio-table*
*Context gathered: 2026-03-07*
