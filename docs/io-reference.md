# I/O Reference

## Inputs

Each of the Morph, Character, Drift, and Phase-Offset CV inputs adds to its knob. The trim above each input sets how much of the CV comes through: fully down (0%) passes none, fully up (100%) passes the full ±5V range — where 5V equals one full sweep of that parameter.

| # | Input | Signal type | Behavior |
|---|-------|-------------|----------|
| 1 | Morph CV | Bipolar CV | ±5V sweeps the full morph range; amount set by the Morph CV trim |
| 2 | Character CV | Bipolar CV | ±5V covers the full range; amount set by the Character CV trim |
| 3 | Drift CV | Bipolar CV | ±5V covers the full range; amount set by the Drift CV trim |
| 4 | Clock | Gate/trigger | Registers above 1.0V, re-arms below 0.1V. Any 5V or 10V clock works |
| 5 | Reset | Trigger | Restarts the waveform at the beginning of its cycle (click-free) |
| 6 | Phase Offset CV | Bipolar CV | ±5V spans 0–360°; amount set by the Phase-Offset CV trim. Active only when patched |
| 7 | FM | Exponential CV | Exponential frequency modulation. The FM Depth trim sets the amount; at 0% (its default) the FM input has no effect |

## Output

| # | Output | Signal |
|---|--------|--------|
| 1 | LFO | Bipolar ±5V. Click-free on reset |
