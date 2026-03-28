# Phase 18: PWM DSP Extension - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md -- this log preserves the alternatives considered.

**Date:** 2026-03-28
**Phase:** 18-pwm-dsp-extension
**Areas discussed:** Morph mapping, Bleed ring topology, Pulse character, Duty cycle range

---

## Morph Mapping

### Q1: How should the morph knob map to 5 shapes?

| Option | Description | Selected |
|--------|-------------|----------|
| Append at [0.75, 1.0] | Keep morph * 3.0 for [0, 0.75]. Add pulse as 4th segment in [0.75, 1.0]. Old patches bit-identical. | |
| Rescale all 5 evenly | Each shape gets 20% of knob (morph * 4.0). Breaks backward compat. | ✓ |
| Nonlinear mapping | Original 4 shapes compressed slightly. Near-identical but not bit-exact. | |

**User's choice:** Rescale all 5 evenly
**Notes:** None

### Q2: WAVE-05 backward compat conflict

| Option | Description | Selected |
|--------|-------------|----------|
| Drop WAVE-05 | Accept breaking change. Even spacing worth it. | ✓ |
| Keep WAVE-05, use append | Stick with [0, 0.75] = original 4, [0.75, 1.0] = pulse. | |
| Compromise | Slight rescale, keep original positions close enough. | |

**User's choice:** Drop WAVE-05
**Notes:** User explicitly chose breaking change over backward compatibility.

### Q3: Morph order

| Option | Description | Selected |
|--------|-------------|----------|
| Sine->Tri->Saw->Sqr->Pulse | Natural harmonic progression. Pulse is endpoint of increasing edge content. | ✓ |
| Sine->Tri->Saw->Pulse->Sqr | Pulse before square. Two most similar shapes at opposite ends. | |

**User's choice:** Sine->Tri->Saw->Sqr->Pulse

---

## Bleed Ring Topology

### Q1: Ring wrap or open-ended?

| Option | Description | Selected |
|--------|-------------|----------|
| Full ring | sine->tri->saw->sqr->pulse->sine (% 5). Consistent with current modular arithmetic. | ✓ |
| Open-ended | No wrap from pulse to sine. Pulse only bleeds from square. | |
| Weighted ring | Full ring but pulse->sine bleed attenuated to 50%. | |

**User's choice:** Full ring
**Notes:** Consistent with existing 4-shape ring pattern.

---

## Pulse Character

### Q1: How Character knob affects pulse

| Option | Description | Selected |
|--------|-------------|----------|
| Tanh edge softening | Same as computeSquare. Character=0 crisp, character=1 rounded. Consistent. | ✓ |
| Tanh + duty wobble | Edge softening plus character modulates duty per instance via component spread. | |
| You decide | Claude's discretion. | |

**User's choice:** Tanh edge softening

### Q2: Component spread effect

| Option | Description | Selected |
|--------|-------------|----------|
| Edges only | Component spread affects edge softening intensity. Duty stays where morph puts it. | ✓ |
| Edges + duty spread | Each instance gets slight duty offset from component spread. | |
| You decide | Claude's discretion. | |

**User's choice:** Edges only

---

## Duty Cycle Range

### Q1: How narrow at morph = 1.0?

| Option | Description | Selected |
|--------|-------------|----------|
| 5% (very narrow) | 50% down to 5%. Classic PWM territory. Matches WAVE-02 spec. | ✓ |
| 10% (moderate) | 50% down to 10%. Avoids extreme thin pulses. | |
| 2% (extreme) | Nearly a click. Maximum range but risks artifacts. | |

**User's choice:** 5% (very narrow)

### Q2: Duty mapping curve

| Option | Description | Selected |
|--------|-------------|----------|
| Linear | duty = 50% - 45% * frac. Even knob response. Simple and predictable. | ✓ |
| Progressive curve | Narrows slowly at first, faster at extreme. More resolution in musical range. | |
| You decide | Claude's discretion. | |

**User's choice:** Linear

---

## Claude's Discretion

- Pulse generation implementation details
- `computePulse()` function structure
- Anti-aliasing or edge-case handling for extreme narrow widths

## Deferred Ideas

None -- discussion stayed within phase scope.

## Folded Todos

- Pulse width modulation (2026-03-17) -- directly addressed
- Separate display pills from waveform visualiser (2026-03-17) -- primarily Phase 20
