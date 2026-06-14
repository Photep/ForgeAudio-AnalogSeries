# Phase 20: Display Layout + CRT Aesthetic - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-01
**Phase:** 20-display-layout-crt-aesthetic
**Areas discussed:** CRT scanline feel, Pill visual refresh, Waveform trace update, Display color migration

---

## CRT Scanline Feel

| Option | Description | Selected |
|--------|-------------|----------|
| Subtle atmosphere | Barely visible (~0.03-0.05 opacity dark bands), enhances analog screen feel without screaming retro | ✓ |
| Clearly retro | Distinctly visible scanline banding (~0.08-0.12 opacity), unmistakably CRT-inspired | |
| You decide | Claude picks the right intensity | |

**User's choice:** Subtle atmosphere
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Static | Fixed horizontal bands, simpler, no GPU cost | |
| Slow scroll | Very slow downward drift (~1px/sec) simulating CRT refresh, subtle but alive | ✓ |
| You decide | Claude picks based on perf impact | |

**User's choice:** Slow scroll
**Notes:** None

---

## Pill Visual Refresh

| Option | Description | Selected |
|--------|-------------|----------|
| Full Forge Noir | Ember-tinted backgrounds with ember stroke, per DESIGN-LANGUAGE.md | ✓ |
| Keep current navy pills | Current feathered navy boxes are subtle and readable | |
| Hybrid | Keep feathered approach but shift colors toward ember/dark | |

**User's choice:** Full Forge Noir
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Switch to JetBrains Mono | Matches DESIGN-LANGUAGE.md spec, need to bundle .ttf | ✓ |
| Keep ShareTechMono | Already loaded from VCV system fonts, zero-cost | |
| You decide | Claude picks based on visual result and font availability | |

**User's choice:** Switch to JetBrains Mono
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Match design language | 2-2.5px border-radius, ember stroke border, crisper pills | ✓ |
| You decide | Claude tunes pill geometry for best NanoVG result | |

**User's choice:** Match design language
**Notes:** None

---

## Waveform Trace Update

| Option | Description | Selected |
|--------|-------------|----------|
| Align to 3-layer | Match design language: wide diffuse (7px, 0.06), medium (3.5, 0.15), sharp core (2, 0.85) | ✓ |
| Keep 4-pass | Current 4-pass is already tuned, extra pass gives smoother falloff | |
| You decide | Claude picks based on visual result with #030303 background | |

**User's choice:** Align to 3-layer
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Yes, add it | Subtle dashed horizontal line at vertical center, helps read waveform symmetry | ✓ |
| Skip it | Current display is clean without it, adding lines adds clutter | |
| You decide | Claude decides based on how it reads at actual display size | |

**User's choice:** Yes, add zero-crossing reference line
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Align to design language | 3 concentric circles (glow 0.08, mid 0.22, core #f0a030 0.95) | |
| Keep current approach | Radial gradient halo + bright center dot, just recolor | |
| You decide | Claude picks whichever looks better with the new display background | ✓ |

**User's choice:** You decide (Claude's discretion)
**Notes:** Keep comet trail and drift jitter behavior regardless of dot approach

---

## Display Color Migration

| Option | Description | Selected |
|--------|-------------|----------|
| Adopt #030303 | Matches Forge Noir design language, near-black makes ember elements pop | ✓ |
| Keep navy tint | Current navy gives CRT phosphor feel, differentiates from panel background | |
| Split the difference | ~#060610, very dark with faint blue undertone for depth | |

**User's choice:** Adopt #030303
**Notes:** None

| Option | Description | Selected |
|--------|-------------|----------|
| Ember trace, gold dot | Trace in ember (#e85d26) family, dot core stays gold (#f0a030), creates visual hierarchy | ✓ |
| Keep current gold-amber | Closest match is Molten Gold #daa520, current warm gold reads well | |
| You decide | Claude picks based on contrast with #030303 | |

**User's choice:** Ember trace, gold dot
**Notes:** Trace = wire, dot = hot spot visual hierarchy

| Option | Description | Selected |
|--------|-------------|----------|
| Full design language | 1.5px ember border + breathing glow (0.08-0.18 opacity, 5s), replace current frame | ✓ |
| Keep current + add glow | Keep inset shadow/highlight frame, add breathing glow and corner brackets on top | |
| You decide | Claude picks approach that looks best with corner brackets | |

**User's choice:** Full design language
**Notes:** None

---

## Claude's Discretion

- Phase dot rendering approach (3 concentric circles vs radial gradient halo)
- Scanline scroll implementation details
- breathePhase rate adjustment for border glow
- JetBrains Mono weight/size tuning

## Deferred Ideas

None — discussion stayed within phase scope.
