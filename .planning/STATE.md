---
gsd_state_version: 1.0
milestone: v1.3
milestone_name: Forge Noir
status: ready_to_plan
stopped_at: Phase 20.1 complete (5/5) — ready to discuss Phase 21
last_updated: 2026-06-12T10:35:33.763Z
last_activity: 2026-06-12
progress:
  total_phases: 5
  completed_phases: 3
  total_plans: 13
  completed_plans: 13
  percent: 60
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-03-28)

**Core value:** The three-knob analog engine (morph, character, drift) that lets users dial in anywhere from pristine digital to authentic vintage analog character, with immediate visual feedback.
**Current focus:** Phase 21 — animated sync badge

## Current Position

Phase: 21
Plan: Not started
Status: Ready to plan
Last activity: 2026-06-12

Progress: [█████████░] 92%

## Performance Metrics

**Velocity (cumulative):**

- v1.0: 12 plans in 58 min (4.8 min avg)
- v1.1: 6 plans in 6 days (includes human verification sessions)
- v1.2: 8 plans in ~97 min

## Accumulated Context

### Roadmap Evolution

- Phase 20.1 inserted after Phase 20: Panel Redesign 18HP Fresh Layout — swap to res/AnalogLFO-fresh.svg, remap widgets, before Animated SYNC Badge (URGENT)

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
- [Phase 20.1-02]: Stripped all metal knob/trimpot bodies from 18HP fresh.svg (widget owns the knob); kept soft shadows + scallop ticks as recessed-socket read (D-01)
- [Phase 20.1-02]: Promoted fresh.svg to production res/AnalogLFO.svg; Rack auto-derives 18HP from viewBox 91.44mm, no plugin.json width field (D-03, D-05)
- [Phase ?]: [Phase 20.1-04]: Re-tuned only hardcoded px in WaveformDisplay (margin 6->8, brackets 3/5->4/6, pills bumped ~12%, bpm 3.5->4.0, clk 2.9->3.3, topY 6->7); proportional box.size math left to re-flow; pillValueSize [[maybe_unused]] clears prior warning (D-06)

### Pending Todos

1 pending todo (see `.planning/todos/pending/`)

- Separate display pills from waveform visualiser (addressed by Phase 20 three-column layout)

### Blockers/Concerns

None.

## Session Continuity

Last session: 2026-06-12T01:29:26.298Z
Stopped at: Phase 20.1 UI-SPEC approved
Resume file: None
