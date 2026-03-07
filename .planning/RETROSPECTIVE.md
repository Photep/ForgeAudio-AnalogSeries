# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v1.0 — Analog Series LFO

**Shipped:** 2026-03-07
**Phases:** 6 | **Plans:** 12 | **Total execution:** 58 min

### What Was Built
- Complete three-knob analog engine (morph, character, drift) with CV control
- Four-shape parametric morph oscillator (Sine-Tri-Saw-Square)
- Per-shape analog character modeling targeting real synths (Minimoog, Roland, Moog/Prophet)
- Four-layer Ornstein-Uhlenbeck drift engine with per-module RNG
- Real-time waveform display with lock-free double buffer and NanoVG rendering
- 12HP branded panel with designer-ready SVG

### What Worked
- Strict dependency chain (scaffold → engine → display → character → drift) kept each phase focused
- Visual verification plans (even-numbered plans) caught real issues: display occluding knobs, triangle phase inversion, square shape misalignment
- Research phases before planning produced well-targeted DSP implementations
- Falling saw convention decision (Phase 2) elegantly solved the morph crossfade amplitude dip
- Characterize-then-morph ordering (Phase 4) gave coherent transitions without special cases

### What Was Inefficient
- Character deformation amplitudes from research (2-3%) were too subtle at LFO rates — had to increase to 8-50% for perceptibility. Research should account for sub-audio vs audio-rate perception differences.
- Drift dot instability parameters needed two rounds of tuning (initial was too subtle, then 5x amplification in Phase 6). Could have been more aggressive in initial implementation.
- Bottom row layout went through three iterations (Phase 2, 5, 6) as component count changed. Earlier component inventory would have avoided rework.
- Phase 4 (Pitch Controls with octave/semitone) was planned then removed — not meaningful for sub-audio LFO. Better domain analysis during requirements would have caught this.

### Patterns Established
- Lock-free atomic transfers for audio-to-GUI data (displayBuffers, displayPhase, displayDrift)
- Even-numbered plans as visual verification in VCV Rack (human-in-the-loop)
- Progressive x^2 knob curves for subtle-to-aggressive response
- Two-row bottom layout convention (trimpots above jacks)
- Per-module RNG seeding for unique behavior per instance

### Key Lessons
1. Sub-audio LFO rates need larger deformation amounts than audio-rate research suggests — users see the waveform, not just hear it
2. Visual verification catches layout and interaction issues that code review cannot — worth the extra plan
3. Milestone audit before completion catches stale docs and subtle issues (displayDrift CV gap) that accumulate during fast execution
4. Panel layout should be designed for final component count, not incrementally expanded

### Cost Observations
- Model mix: quality profile throughout
- Sessions: ~6 planning + execution sessions
- Notable: 58 min total execution across 12 plans (4.8 min avg) — very efficient for a complete DSP module

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Execution Time | Phases | Key Change |
|-----------|---------------|--------|------------|
| v1.0 | 58 min | 6 | Initial milestone — established visual verification pattern |

### Top Lessons (Verified Across Milestones)

1. Visual verification plans are essential for UI-heavy modules — catches issues code review misses
2. Research phase perception context matters — sub-audio vs audio-rate needs different parameter ranges
