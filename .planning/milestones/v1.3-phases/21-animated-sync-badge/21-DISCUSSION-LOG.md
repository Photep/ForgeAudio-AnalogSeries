# Phase 21: Animated SYNC Badge - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-13
**Phase:** 21-animated-sync-badge
**Areas discussed:** Flash style, When it fires, Intensity

---

## Flash Style

| Option | Description | Selected |
|--------|-------------|----------|
| Color shift to white-hot | Text/glow shifts from ember toward hot white-yellow at peak, decays back | |
| Glow bloom | Keep ember color, boost glow pass (blur radius + alpha) at peak | |
| Both — color shift + bloom | White-hot color shift AND glow bloom together at peak | ✓ (Claude's call) |
| You decide | Builder picks best fit for Forge Noir aesthetic | ✓ (user delegated) |

**User's choice:** "You decide" → Claude locked **Both — color shift + glow bloom**.
**Notes:** Steady state already at full ember alpha, so brightness must come from color/glow beyond alpha. Spark-on-hot-metal metaphor fits the "Forge" identity.

---

## When It Fires

| Option | Description | Selected |
|--------|-------------|----------|
| LOCKED only | Flash on edges only once locked; ACQUIRING keeps its 2 Hz blink | ✓ (Claude's call) |
| Both states | Flash on every detected edge in ACQUIRING and LOCKED | |
| You decide | Builder chooses cleanest read | ✓ (user delegated) |

**User's choice:** "You decide" → Claude locked **LOCKED only**.
**Notes:** ACQUIRING already owns the 2 Hz blink (semantics = searching). Avoids two competing animations. Clean split: blink = acquiring, flash = locked & responding.

---

## Intensity

| Option | Description | Selected |
|--------|-------------|----------|
| Punchy & obvious | Strong, clearly visible flash | ✓ (Claude's call) |
| Subtle & refined | Gentle, understated brightening | |
| You decide | Tune to look right against panel | ✓ (user delegated) |

**User's choice:** "You decide" → Claude locked **Clearly visible / punchy, tunable at acceptance gate**.
**Notes:** Success criterion #1 requires the flash be distinct from steady-state at a glance. Exponential decay keeps it from being harsh. Exact peak magnitude is a single-constant tune.

---

## Claude's Discretion

All three areas were delegated by the user ("you decide"). Decisions D-01/D-02/D-03
reflect the builder's recommendation and are now locked. Exact numeric constants
(peak white-point, glow blur radius, brightness multiplier) and the DSP→widget
edge-signal mechanism are left to implementation and the acceptance gate.

## Deferred Ideas

Three pending todos keyword-matched the phase but were reviewed and NOT folded
(unrelated to badge animation): "Separate display pills from waveform visualiser"
(display refactor), "Surge-style modulation routing system" (large new feature),
and "Pulse width modulation" (already delivered by Phase 18). All remain in the backlog.
