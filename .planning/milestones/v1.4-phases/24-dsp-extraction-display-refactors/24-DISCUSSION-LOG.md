# Phase 24: DSP Extraction + Display Refactors - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-26
**Phase:** 24-dsp-extraction-display-refactors
**Areas discussed:** Buffer thread move (CLEAN-05), Animation dt & locked decay (CLEAN-04), Behavior-preservation gate, Pill fade symmetry (CLEAN-03)

---

## Buffer thread move (CLEAN-05)

### How far to move regeneration off the audio thread

| Option | Description | Selected |
|--------|-------------|----------|
| Hybrid dirty-flag | Audio thread keeps cheap trigger detection but only sets an atomic 'dirty' flag; GUI step() reads it and runs the fill. Minimal change to proven logic, lowest risk. | ✓ |
| Full move to GUI | GUI step() owns both trigger detection and fill; re-derives wrap logic at ~60fps. Cleanest separation but risks changing when regen fires. | |
| You decide | Planner picks. | |

**User's choice:** Hybrid dirty-flag

### How the GUI fill gets its inputs

| Option | Description | Selected |
|--------|-------------|----------|
| Snapshot at trigger | Audio thread snapshots morph/character/swingFrac/bleedLfo into a lock-free struct at flag-set; GUI consumes it. Bit-identical preview shape. | ✓ |
| GUI reads live values | GUI reads current knobs + latest atomics at paint time. Simpler, but microscopic shape drift vs. today. | |
| You decide | Planner picks, preview must stay visually identical. | |

**User's choice:** Snapshot at trigger
**Notes:** Preserves the live drift state (ouLayers[0].state) captured at the trigger instant.

---

## Animation dt & locked decay (CLEAN-04)

### Conversion scope vs. the ANIM-02-locked 0.92 badge decay

| Option | Description | Selected |
|--------|-------------|----------|
| Convert all, preserve feel | Convert everything to clamped dt including badge decay → pow(0.92, dt*60), identical at 60fps yet frame-rate-independent. | ✓ |
| Convert all except badge | Convert breathe/blink/scanline/fades; leave 0.92 badge decay frame-paced. Partial CLEAN-04. | |
| You decide | Planner reconciles. | |

**User's choice:** Convert all, preserve feel

### dt source and clamp ceiling

| Option | Description | Selected |
|--------|-------------|----------|
| ~2 frames / 33ms | Clamp dt to ~1/30s; smooth recovery after a stall; clean pathological-dt test. | ✓ |
| Larger ceiling / ~100ms | Faster catch-up but bigger visual jump after a freeze. | |
| You decide | Planner fixes the exact ceiling. | |

**User's choice:** ~2 frames / 33ms
**Notes:** dt source = APP->window->getLastFrameDuration().

---

## Behavior-preservation gate

| Option | Description | Selected |
|--------|-------------|----------|
| Headless buffer test + UAT | Display-buffer determinism test (identical buffer regardless of fill thread) + manual in-Rack UAT for feel. | ✓ |
| Automated tests only | dt test + buffer test, no manual UAT gate. | |
| Manual UAT only | dt test + manual visual check, skip the headless buffer test. | |

**User's choice:** Headless buffer test + UAT
**Notes:** The CLEAN-04 pathological-dt test is mandated regardless; this is the additional gate.

---

## Pill fade symmetry (CLEAN-03)

| Option | Description | Selected |
|--------|-------------|----------|
| Widget-side cache | Cache last ratioIdx + period in the widget, draw while alpha > threshold, drop the ratioIdx>=0 early-return. Pure draw-layer fix. | ✓ |
| Hold the atomics | Don't write -1/sentinel on disconnect until fade completes. Risks consumers reading -1 as 'free'. | |
| You decide | Planner picks. | |

**User's choice:** Widget-side cache
**Notes:** Root cause is CODE-REVIEW #10 — pills early-return on the disconnect sentinel and pop off while the badge fades 200ms. Hz/Swing overlays already correct.

---

## Claude's Discretion

- Exact dirty-flag/snapshot struct layout and memory ordering.
- Precise clamp constant expression.
- Whether the headless buffer-determinism test lives in the existing doctest/BlockDriver harness or a new file; test naming/organization.
- How `isStill` (CLEAN-02) is resolved (remove vs. wire to a reachable condition).
- CLEAN-01 dead-code removal (`drawZeroCrossing`, `scanlineImage`) — mechanical.

## Deferred Ideas

- RNG strategy and full DSP extraction/thinning/bleedLfo lift — already resolved in Phase 22 (D-02/D-04/D-05/D-07); ROADMAP's "open decision: RNG strategy" for Phase 24 is stale. Not re-asked.
- IP/font git-history purge + LICENSE/NOTICES → Phase 25.
- VCV manifest validation + `.vcvplugin` packaging → Phase 26.
- `swingIndex` GUI→audio non-atomic write — pre-existing v1.3 tech debt, out of scope.
