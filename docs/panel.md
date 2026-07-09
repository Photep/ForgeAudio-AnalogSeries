# Annotated Panel

![Annotated panel](img/panel-annotated.png)

Numbered callouts are baked into the panel image above. Each number keys 1:1 to a row in the legend below. This legend doubles as the control-reference table — ranges and behaviors are transcribed from the module source.

| # | Control | Type | Range / Behavior |
|---|---------|------|------------------|
| 1 | Display | Readout | Waveform preview + phase dot; shows Hz readout free-running, or ratio label + BPM + SYNC badge (and swing overlay) when clocked |
| 2 | Morph | Knob | 0–1, default 0. Waveform shape sweep: sine → triangle → saw → square → narrow pulse (PWM folded into morph, even ~20% per shape) |
| 3 | Character | Knob | 0–1, default 0. Crossfade from clean/digital to analog character per shape (warm analog triangle, vintage transistor saw, classic PWM square); x² progressive curve, so 0.5 knob ≈ 25% effect |
| 4 | Drift | Knob | 0–1, default 0. Multi-timescale drift: Ornstein-Uhlenbeck pitch drift + phase jitter + DC wander + thermal slew. Drift authority reduced in clocked mode |
| 5 | Rate | Knob | 0.01–20 Hz free-running (default 0.7); selects one of 15 musical ratios when clocked, showing the ratio label + " (synced)" |
| 6 | Phase Offset | Knob | 0–360°, default 0 (param 0–1 scaled to degrees) |
| 7 | Morph-CV trim | Trimpot (attenuator) | 0–100%, default 100%. Scales the Morph CV input |
| 8 | Character-CV trim | Trimpot (attenuator) | 0–100%, default 100%. Scales the Character CV input |
| 9 | Drift-CV trim | Trimpot (attenuator) | 0–100%, default 100%. Scales the Drift CV input |
| 10 | FM-Depth trim | Trimpot (attenuator) | 0–100%, default 0%. Scales the FM input depth |
| 11 | Phase-Offset-CV trim | Trimpot (attenuator) | 0–100%, default 100%. Scales the Phase Offset CV input |
| 12 | Morph-CV jack | Input | Bipolar ±5V, 5V/unit; `value = knob + trim × (CV / 5V)`, clamped to [0, 1] |
| 13 | Character-CV jack | Input | Bipolar ±5V, 5V/unit; per-instance characterSpread folded in |
| 14 | Drift-CV jack | Input | Bipolar ±5V, 5V/unit; clamped to [0, 1] |
| 15 | FM jack | Input | Exponential CV; `freq ×= 2^(CV × FM Depth × depthScale)`, depthScale = 0.6 free-running / 0.5 clocked |
| 16 | Phase-Offset-CV jack | Input | Bipolar ±5V → 0–360° at 5V/unit; applied only when patched |
| 17 | CLK jack | Input | Gate/trigger; Schmitt trigger fires rising above 1.0V, arms below 0.1V. Any 0→5V or 0→10V clock works |
| 18 | RESET jack | Input | Trigger; Schmitt trigger 1.0V/0.1V; forces phase to 0 with 1ms blanking |
| 19 | OUTPUT jack | Output | Bipolar ±5V (`5.f × sample`); 3ms cosine anti-click crossfade on phase reset; DC-offset drift added after the crossfade |
