---
phase: 11-display-polish
plan: 01
subsystem: ui
tags: [nanovg, pill-background, bpm-display, hud, waveform-display]

# Dependency graph
requires:
  - phase: 10-display-and-panel
    provides: "WaveformDisplay with drawGlowText, drawTextOverlays, fade alpha system"
provides:
  - "drawPillText() helper with feathered nvgBoxGradient pill backgrounds"
  - "drawBpmStack() dual-line CLK/BPM display with x1 collapsing"
  - "All HUD text overlays readable over waveform trace"
affects: [12-reset-phase-offset, 17-panel-redesign]

# Tech tracking
tech-stack:
  added: []
  patterns: [nvgBoxGradient feathered pill rendering, dual-line text stack with shared pill]

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "Each text overlay gets its own individual pill background (not one shared pill)"
  - "CLK line rendered at 60% alpha relative to BPM line for visual hierarchy"
  - "Pill feather distance of 3px with matching oversized path to prevent clipping"

patterns-established:
  - "drawPillText: nvgBoxGradient pill + 2-pass glow text pattern for readable overlays"
  - "Shared pill for multi-line stacks: union bounding rects before drawing single pill"

requirements-completed: [DISP-01, DISP-02]

# Metrics
duration: ~15min
completed: 2026-03-13
---

# Phase 11 Plan 01: Display Polish Summary

**Feathered dark pill backgrounds behind all HUD text overlays via nvgBoxGradient, plus dual-line CLK/BPM stack with x1 collapsing**

## Performance

- **Duration:** ~15 min (across two sessions with visual verification checkpoint)
- **Started:** 2026-03-13
- **Completed:** 2026-03-13
- **Tasks:** 3 (2 auto + 1 checkpoint)
- **Files modified:** 1

## Accomplishments
- All four HUD text overlays (Hz, ratio, SYNC, BPM) now render with soft-edged feathered dark pill backgrounds, keeping text readable when the waveform trace passes through
- New drawPillText() helper wraps nvgBoxGradient pill rendering with 2-pass glow text, replacing drawGlowText() calls in overlay rendering
- New drawBpmStack() shows dual-line display: dim CLK line (raw clock BPM) above bright BPM line (ratio-adjusted) at non-x1 ratios, collapsing to single BPM line at x1
- Both CLK and BPM lines share a single pill background for visual cohesion

## Task Commits

Each task was committed atomically:

1. **Task 1: Add drawPillText helper and apply pill backgrounds to all existing overlays** - `d4e9e83` (feat)
2. **Task 2: Add dual-line BPM stack with CLK/BPM display and x1 collapsing** - `9dbccd9` (feat)
3. **Task 3: Visual verification of pill overlays and BPM stack** - checkpoint approved (no commit)

## Files Created/Modified
- `src/AnalogLFO.cpp` - Added drawPillText() with nvgBoxGradient pill rendering, drawBpmStack() with dual-line CLK/BPM display, updated drawTextOverlays() to use pill-backed text

## Decisions Made
- Each text overlay gets its own individual pill background rather than a single shared background covering the entire HUD area
- CLK line rendered at 60% alpha relative to BPM line to establish clear visual hierarchy (supplemental context vs primary info)
- Pill uses 3px feather distance with oversized nvgRoundedRect path to prevent gradient edge clipping
- Kept drawGlowText() intact for potential future use

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Display foundation complete with readable overlays at all waveform positions
- Phase 12 (RESET and Phase Offset) can proceed -- no display blockers remain
- drawPillText() pattern established for any future HUD text additions

## Self-Check: PASSED

- FOUND: src/AnalogLFO.cpp
- FOUND: commit d4e9e83 (Task 1)
- FOUND: commit 9dbccd9 (Task 2)
- FOUND: 11-01-SUMMARY.md

---
*Phase: 11-display-polish*
*Completed: 2026-03-13*
