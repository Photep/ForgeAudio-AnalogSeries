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

Pulse width narrows as you sweep past square — there's no separate width control. Shapes crossfade continuously, with a touch of blend between neighbors during each transition.

## Character

Crossfade from clean/digital shapes to analog character, applied per shape before the morph crossfade. Descriptors are generic:

| Shape | Character target |
|-------|------------------|
| Triangle | Warm analog triangle |
| Saw | Vintage transistor saw |
| Square | Classic square |

The knob follows a progressive curve: the halfway position gives roughly a quarter of the full effect, so low settings stay subtle and high settings turn aggressive.

## Drift

One knob scales a bundle of analog imperfections in curated proportions:

| Component | Behavior |
|-----------|----------|
| Pitch drift | Slow, natural pitch wander |
| Phase jitter | Small cycle-to-cycle timing variance |
| Output-offset drift | Slow wander in the output's center |
| Thermal lag | Frequency eases toward changes rather than snapping |

Drift is eased back in clocked mode so it stays tight to the clock while keeping its analog feel.
