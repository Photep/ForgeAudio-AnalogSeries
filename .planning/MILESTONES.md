# Milestones

## v1.3 Forge Noir (Shipped: 2026-06-13)

**Phases completed:** 5 phases (18, 19, 20, 20.1 inserted, 21), 14 plans, 20 tasks

**Key accomplishments:**

- Five-shape morph sweep with PWM — variable-width pulse as the 5th shape (Sine→Tri→Saw→Square→Pulse), tanh edge softening, 50%→5% duty cycle, and 5-shape bleed ring wrap (Phase 18, WAVE-01..05)
- Forge Noir custom components — 11 nanosvg component skins (3 knob sizes, scalloped trimpots, ember-ringed output jack, hex bolts) wired as 7 custom widget structs (Phase 19, PANEL-01..07)
- Forge Noir panel artwork — near-black panel with 110+ path letterforms, forge emblem, rune glyph, and gradient component placement (Phase 19)
- Three-column CRT display — pills in dedicated left/right margins, ember-glow waveform constrained to center, corner brackets, scrolling scanlines, and breathing border glow (Phase 20, DISP-01..05)
- 18HP fresh panel redesign — promoted to production `res/AnalogLFO.svg` with widget-owned knobs, single-row equal secondary knobs, grouped clock section, all anchors aligned at 100%/200% (Phase 20.1, PANEL-08..12)
- Animated SYNC badge — per-edge white-hot flash while LOCKED via lock-free atomic counter and 0.92×/frame exponential decay envelope (Phase 21, ANIM-01..02)

**Stats:**
- 1,641 lines of C++ (`AnalogLFO.cpp`), 106 commits, 96 files changed, +16,160 / −2,455
- Timeline: 2026-03-28 → 2026-06-13
- Requirements: 24/24 complete (WAVE 5, PANEL 12, DISP 5, ANIM 2)
- Milestone audit: passed (tech-debt resolved inline — WAVE-05 rescoped per D-02, verification bookkeeping flipped, dead code removed)

**Pre-close cleanup:** 3 stale todos resolved (PWM→Phase 18, display-pill separation→Phase 20, Surge-routing→out of scope); Phase 18 PWM UAT owner-approved in Rack; Phase 19 14HP panel UAT superseded by Phase 20.1 18HP redesign.

---

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
