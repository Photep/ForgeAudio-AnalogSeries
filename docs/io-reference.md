# I/O Reference

## Inputs

CV convention for Morph, Character, Drift, and Phase Offset: `value = knob + trim × (CV / 5V)`, clamped to `[0, 1]`. At 100% trim this is bipolar ±5V, 5V per full-scale unit.

| # | Input | Signal type | Range / behavior |
|---|-------|-------------|------------------|
| 1 | Morph CV | Bipolar CV | ±5V → full morph sweep at 5V/unit; attenuated by the Morph CV trim |
| 2 | Character CV | Bipolar CV | ±5V → full range at 5V/unit; per-instance characterSpread folded in |
| 3 | Drift CV | Bipolar CV | ±5V → full range at 5V/unit; attenuated by the Drift CV trim |
| 4 | Clock | Gate/trigger | Schmitt trigger: fires rising above 1.0V, arms below 0.1V. Any 0→5V or 0→10V clock works |
| 5 | Reset | Trigger | Schmitt trigger 1.0V/0.1V; forces phase to 0 with 1ms blanking (suppresses a reset landing on a clock-driven reset) |
| 6 | Phase Offset CV | Bipolar CV | ±5V → 0–360° at 5V/unit; attenuated by the Phase Offset CV trim. Applied only when patched |
| 7 | FM | Exponential CV | `freq ×= 2^(CV × FM Depth × depthScale)`, depthScale = 0.6 free-running / 0.5 clocked |

## Output

| # | Output | Signal |
|---|--------|--------|
| 1 | LFO | Bipolar ±5V (`5.f × sample`); 3ms cosine anti-click crossfade on phase reset; DC-offset drift added after the crossfade |
