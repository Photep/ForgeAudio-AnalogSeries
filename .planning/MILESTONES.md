# Milestones

## v1.2 Deep Analog (Shipped: 2026-03-17)

**Phases completed:** 6 phases (1 skipped), 8 plans

**Key accomplishments:**
- Pill-backed HUD text overlays with feathered backgrounds for waveform readability (DISP-01, DISP-02)
- Independent RESET trigger with bidirectional 1ms blanking and Phase Offset knob/CV (MOD-03, MOD-04, PHASE-01, PHASE-02)
- Exponential FM input with dual-authority clocked/free mode scaling (MOD-01, MOD-02)
- Expanded analog imperfections: phase jitter, DC offset wander, pitch slew, component spread (CHAR-01 through CHAR-04)
- Waveform bleed with neighbor-shape crosstalk during morph transitions (CHAR-05)
- MPC-style swing timing with 6 named presets and context menu (PHASE-03, PHASE-04)

**Stats:**
- 1,374 lines of C++ (484 added), 44 commits
- Timeline: 5 days (2026-03-13 to 2026-03-17)
- Requirements: 15/17 complete (2 deferred: PANEL-01, PANEL-02)

### Known Gaps
- **PANEL-01**: Panel SVG update — deferred to modulation routing milestone
- **PANEL-02**: Panel control placement — deferred to modulation routing milestone
- **Phase 17** (Panel Redesign) skipped: 12HP density requires Surge-style modulation routing system

---

## v1.1 Clock Sync (Shipped: 2026-03-13)

**Phases completed:** 4 phases, 6 plans

**Key accomplishments:**
- Three-state clock tracker (FREE/ACQUIRING/LOCKED) with EMA smoothing, outlier rejection, and fast-track re-acquisition
- 15 discrete musical ratios (/16 to x16) with dual-mode Rate knob and custom tooltip
- Division-aware phase reset with anti-click 3ms cosine crossfade for beat-aligned operation
- Drift authority scaling (2% clocked vs 7.5% free) and 50ms frequency slew for smooth mode transitions
- NanoVG text overlays (SYNC badge, ratio label, BPM readout, Hz readout) with 200ms fade animations
- Panel SVG updated with CLK jack and lavender label

**Stats:**
- 890 lines of C++ (338 added), 24 files changed, 4,251 insertions
- Timeline: 6 days (2026-03-07 to 2026-03-13)
- Requirements: 18/18 complete

---

## v1.0 Analog Series LFO (Shipped: 2026-03-07)

**Phases completed:** 6 phases, 12 plans, 0 tasks

**Key accomplishments:**
- Three-knob analog engine (morph, character, drift) with CV control for all three axes
- Four-shape parametric morph: continuous Sine-Tri-Saw-Square with musically valid intermediates
- Per-shape analog character modeling: Minimoog saw, Roland square, Moog/Prophet triangle, analog sine
- Four-layer Ornstein-Uhlenbeck drift engine with per-module RNG for unique analog instability
- Real-time waveform display with lock-free double buffer, amber glow trace, and phase-tracking dot
- Polished 12HP panel with Forge Audio brand identity, two-row bottom layout, designer-ready SVG

**Stats:**
- 552 lines of C++, 72 commits, 63 files, 12,660 insertions
- Timeline: 10 days (2026-02-25 to 2026-03-07)
- Requirements: 23/23 complete

---

