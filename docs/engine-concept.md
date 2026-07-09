# Engine Concept

ForgeAnalogLFO is built around three independent axes. Each knob controls one axis, and all three are visible in real time on the display.

## Morph

Continuous waveform-shape sweep across five shapes, even 20% per shape:

| Position | Shape |
|----------|-------|
| 0% | Sine |
| 20% | Triangle |
| 40% | Saw (falling ramp) |
| 60% | Square |
| 80–100% | Narrow pulse |

Pulse-width modulation is folded into the morph sweep past square — no separate PWM control. Shapes crossfade continuously; neighbor bleed adds crosstalk during the transition.

## Character

Crossfade from clean/digital shapes to analog character, applied per shape before the morph crossfade. Descriptors are generic:

| Shape | Character target |
|-------|------------------|
| Triangle | Warm analog triangle |
| Saw | Vintage transistor saw |
| Square | Classic PWM square |

The knob follows an x² progressive curve: the 0.5 knob position yields roughly 25% of the full effect, so low settings stay subtle and high settings turn aggressive.

## Drift

One knob scales a bundle of analog imperfections in curated proportions:

| Component | Behavior |
|-----------|----------|
| Pitch drift | Multi-timescale Ornstein–Uhlenbeck wander |
| Phase jitter | Per-cycle timing variance |
| DC wander | Slow output-offset drift |
| Thermal slew | Frequency lag on change |

Drift authority is reduced in clocked mode to prevent phase-error accumulation while preserving analog feel.
