# Clock & Sync

## State Machine

The clock tracker runs a three-state FSM: **FREE → ACQUIRING → LOCKED**.

| State | Behavior |
|-------|----------|
| FREE | No clock patched (or timed out / disconnected). Rate knob = free Hz (0.01–20). Display shows the Hz readout |
| ACQUIRING | Entered on connect + first edge. Phase resets. SYNC badge blinks at 2 Hz. Period smoothed by EMA (α = 0.3) |
| LOCKED | Ratio label + BPM shown. SYNC badge does a per-edge white-hot flash |

### Transitions

| Transition | Condition |
|------------|-----------|
| First edge → ACQUIRING | Connect + first detected edge; phase resets |
| ACQUIRING → LOCKED (fast-track) | 2nd edge period within 0.8×–1.2× of the last known period → immediate lock (EMA-snapped) |
| ACQUIRING → LOCKED (consistency) | After ≥4 consistent edges |
| Timeout → FREE | No edge within `max(1.0s, min(3×period, 5.0s))` |
| Disconnect → FREE | Instant; last period cached for fast re-lock |

### Outlier rejection (LOCKED)

Edges more than 3× or less than ⅓× the smoothed period are discarded as glitches — **except** 3 consecutive outliers (`OUTLIER_THRESHOLD`) are treated as a genuine tempo change, dropping the tracker back to ACQUIRING to re-learn (BUG-01 fix; prevents a permanent lockout on a >3× speedup). A lone outlier edge is always rejected.

Clock and Reset use a Schmitt trigger: fire rising above 1.0V, arm below 0.1V.

## Ratio Table

In clocked mode the Rate knob selects one of 15 ratios via `round(scaledKnob × 14)` → index 0–14. **x1 is index 7.** Effective LFO frequency = `(1 / smoothedPeriod) × RATIO_TABLE[idx]`.

| Ratio | Index | Multiplier |
|-------|-------|-----------|
| /16 | 0 | 0.0625 |
| /8 | 1 | 0.125 |
| /6 | 2 | 0.166667 |
| /4 | 3 | 0.25 |
| /3 | 4 | 0.333333 |
| /2 | 5 | 0.5 |
| /1.5 | 6 | 0.666667 |
| x1 | 7 | 1.0 |
| x1.5 | 8 | 1.5 |
| x2 | 9 | 2.0 |
| x3 | 10 | 3.0 |
| x4 | 11 | 4.0 |
| x6 | 12 | 6.0 |
| x8 | 13 | 8.0 |
| x16 | 14 | 16.0 |

Phase reset is division-aware via `shouldReset(ratioIdx, beatCount)`, with the 3ms cosine anti-click crossfade on each reset. The RESET jack forces phase to 0 with 1ms blanking.
