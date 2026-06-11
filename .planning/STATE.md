---
gsd_state_version: 1.0
milestone: v1.3
milestone_name: Forge Noir
status: executing
stopped_at: Completed 20-03-PLAN.md — Phase 20 complete
last_updated: "2026-06-12T00:00:00Z"
last_activity: 2026-06-12
progress:
  total_phases: 4
  completed_phases: 3
  total_plans: 8
  completed_plans: 8
  percent: 100
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-03-28)

**Core value:** The three-knob analog engine (morph, character, drift) that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback.
**Current focus:** Phase 20 complete — next: Phase 21 (Animated SYNC Badge)

## Current Position

Phase: 20 complete
Plan: 03 complete (all 3 plans done)
Status: Phase 20 complete — 21 next
Last activity: 2026-06-12

Progress: [██████████] 100%

## Performance Metrics

**Velocity (cumulative):**

- v1.0: 12 plans in 58 min (4.8 min avg)
- v1.1: 6 plans in 6 days (includes human verification sessions)
- v1.2: 8 plans in ~97 min

## Accumulated Context

### Decisions

All decisions logged in PROJECT.md Key Decisions table.
v1.0-v1.2 decisions archived in milestones/ ROADMAP files.

Key pending decisions for v1.3:

- PWM as morph extension [0.75, 1.0] preserving backward compat
- Forge Noir design language (near-black, ember orange, gold accents)
- Panel expansion 12HP to 14HP (shifts modules in existing patches)
- Bleed ring topology for 5 shapes (open-ended vs ring -- decide in Phase 18)
- [Phase 18]: Pulse duty derived from morph position externally, not inside computePulse (D-08)
- [Phase 18]: Backward compatibility dropped: morph*4.f replaces morph*3.f for 5-shape sweep (D-02)
- [Phase 18]: Edge width clamped via min(duty,1-duty)*0.8 to prevent amplitude collapse at narrow duty
- [Phase 19-01]: Trimpot scalloped edge as 8 small circles (nanosvg-simple approach)
- [Phase 19-01]: Output jack dual-layer ember rings (behind + on top of flange) for depth
- [Phase 19-01]: Machined groove texture via alternating white/black strokes at 0.02-0.03 opacity
- [Phase 19-02]: Emblem mirroring via manual coordinate duplication (nanosvg has no use/transform support)
- [Phase 19-02]: Letterform scale factors: 0.8=4.0mm, 0.64=3.20mm, 0.36=1.80mm, 0.28=1.40mm
- [Phase 19-03]: All knob widgets use -0.83*PI rotation range; trimpots at -0.75*PI per UI-SPEC
- [Phase 19-03]: CircularShadow disabled on all custom widgets (SVG includes own shadows in _bg.svg)
- [Phase 19-03]: minRackVersion 2.6.0 enforces gradient rendering support
- [Phase 20-01]: breathePhase slowed to 0.2Hz (5s cycle) for border glow; also affects phase dot idle breathe
- [Phase 20-01]: Separate blinkPhase accumulator at 2Hz for SYNC ACQUIRING blink (independent of border glow)
- [Phase 20-01]: Border glow uses NVG_HOLE inner-cutout fill to stay within scissor bounds
- [Phase 20-02]: phaseToX uses proportional 0.20/0.60 fractions for zoom-compatible center-column constraint
- [Phase 20-02]: Hz readout format: 1 decimal <10Hz, integer otherwise (UI-SPEC copywriting contract)
- [Phase 20-02]: Font blur reduced 3.0 to 2.0 for all glow text at sub-5px font sizes

### Pending Todos

1 pending todo (see `.planning/todos/pending/`)

- Separate display pills from waveform visualiser (addressed by Phase 20 three-column layout)

### Blockers/Concerns

None.

## Session Continuity

Last session: 2026-04-02T04:21:22Z
Stopped at: Completed 20-02-PLAN.md
Resume file: .planning/phases/20-display-layout-crt-aesthetic/20-03-PLAN.md
