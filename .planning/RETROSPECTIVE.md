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

## Milestone: v1.1 — Clock Sync

**Shipped:** 2026-03-13
**Phases:** 4 | **Plans:** 6 | **Timeline:** 6 days

### What Was Built
- Three-state clock tracker (FREE/ACQUIRING/LOCKED) with EMA smoothing and outlier rejection
- 15 discrete musical ratios (/16 to x16) with dual-mode Rate knob
- Division-aware phase reset with 3ms cosine crossfade for click-free beat alignment
- Drift authority scaling and frequency slew for smooth mode transitions
- NanoVG text overlays (SYNC badge, ratio label, BPM, Hz) with 200ms fade animations
- Panel SVG CLK jack with lavender label

### What Worked
- Layered phase approach: clock detection → ratio mapping → phase reset → display kept each phase tightly scoped
- Human verification plans (07-02, 10-02) caught zero issues — implementations were correct on first pass
- Atomic display bridge pattern from v1.0 (displayPhase, displayDrift) extended cleanly for three new atomics (displayClockState, displayRatioIndex, displaySmoothedPeriod)
- Research before Phase 7 produced the right algorithm (EMA vs PLL vs Kalman decision) — no rework
- Phase 9 crossfade approach (capture lastOutputVoltage before reset) avoided coupling processClockInput() to morph/character state

### What Was Inefficient
- Phase 8 and 9 ROADMAP.md checkboxes were not updated after completion — caught during audit. Roadmap status tracking should be automated or checked at plan completion.
- SUMMARY.md files lack `one_liner` field — gsd-tools summary-extract returns null. Either populate one_liner during plan completion or skip extraction.

### Patterns Established
- Relaxed atomics for all audio-to-GUI bridges (consistent with v1.0, now 5 atomics total)
- Division-aware clock counting pattern for /N ratios using integer beat counter
- SVG text-as-paths for nanosvg compatibility (CLK label as three separate path elements)
- NanoVG two-pass glow text rendering (blur pass + sharp pass)
- Fade animation state machine for smooth text overlay transitions

### Key Lessons
1. First-pass implementations that follow research closely tend to pass verification without changes — invest in research quality
2. The atomic bridge pattern scales well — adding new display data requires only declaring the atomic and wiring store/load
3. Clock-synced DSP is simpler than expected when built on a clean state machine foundation — the layered approach (detect → divide → reset → display) kept complexity manageable

### Cost Observations
- Model mix: quality profile (opus planning, sonnet execution/verification)
- Notable: All 6 plans executed cleanly; zero rework needed post-verification

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Timeline | Phases | Plans | Key Change |
|-----------|----------|--------|-------|------------|
| v1.0 | 10 days | 6 | 12 | Established visual verification pattern |
| v1.1 | 6 days | 4 | 6 | Layered DSP approach; zero rework post-verification |

### Cumulative Stats

| Milestone | LOC (total) | LOC Added | Requirements |
|-----------|-------------|-----------|-------------|
| v1.0 | 552 | 552 | 23/23 |
| v1.1 | 890 | 338 | 18/18 |

### Top Lessons (Verified Across Milestones)

1. Visual verification plans are essential for UI-heavy modules — catches issues code review misses (v1.0 confirmed, v1.1 reconfirmed)
2. Research before planning produces first-pass implementations that pass verification without rework (v1.0 character amplitudes exception, v1.1 fully validated)
3. Atomic bridge pattern for audio-to-GUI data scales cleanly across milestones — declare, store, load
