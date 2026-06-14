---
phase: 20-display-layout-crt-aesthetic
plan: 01
subsystem: ui
tags: [nanovg, display, crt, animation, font, waveform-display]

# Dependency graph
requires:
  - phase: 19-forge-noir-panel-custom-components
    provides: Forge Noir panel aesthetic and custom widget components
provides:
  - Near-black (#030303) display background replacing navy
  - Ember breathing border with 5-second glow cycle (drawBorder)
  - Corner bracket decorations at all four display corners (drawCornerBrackets)
  - Dashed zero-crossing reference line in center column (drawZeroCrossing)
  - JetBrains Mono NL Regular font asset for display text rendering
  - blinkPhase accumulator (2Hz) for SYNC ACQUIRING blink
  - scanlineImage and scanlineScrollPhase struct members for CRT overlay
affects: [20-02 pill-text-overlays, 20-03 scanline-waveform-trace]

# Tech tracking
tech-stack:
  added: [JetBrains Mono NL v2.304 (OFL-1.1)]
  patterns: [ember-breathing-border, corner-bracket-decoration, dashed-line-via-segments, expanded-scissor-for-glow]

key-files:
  created:
    - res/fonts/JetBrainsMonoNL-Regular.ttf
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "breathePhase slowed to 0.2Hz (5s cycle) — affects both border glow and phase dot idle breathe; acceptable per RESEARCH.md Pitfall 2"
  - "Separate blinkPhase accumulator at 2Hz for SYNC ACQUIRING indicator, independent of border glow rate"
  - "Corner bracket dimensions: 3px inset, 5px arm (scaled from 5px/8px mockup at 0.59x)"
  - "Border glow uses NVG_HOLE inner-cutout approach to stay within scissor bounds (Pitfall 7)"

patterns-established:
  - "Expanded scissor (-2,-2,w+4,h+4) for border glow, then tight scissor (0,0,w,h) for content"
  - "NanoVG manual dash segments for dashed lines (no native dash support)"
  - "Multiple phase accumulators in step() for independent animation rates"

requirements-completed: [DISP-03, DISP-05]

# Metrics
duration: 5min
completed: 2026-04-02
---

# Phase 20 Plan 01: Display Container + Font Summary

**Near-black display background with ember breathing border, corner brackets, zero-crossing line, and JetBrains Mono NL font asset for Forge Noir display aesthetic**

## Performance

- **Duration:** 5 min
- **Started:** 2026-04-01T22:41:39Z
- **Completed:** 2026-04-02T02:24:33Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- Downloaded and placed JetBrains Mono NL Regular v2.304 font (208KB, OFL-1.1) at res/fonts/
- Replaced navy display background with near-black #030303 per design language D-12
- Replaced drawInsetFrame with drawBorder: 1.5px ember stroke with breathing glow (0.08-0.18 alpha, 5s cycle) per D-13
- Added four L-shaped corner bracket decorations per D-14
- Added dashed zero-crossing reference line spanning center column (20%-80% width) per D-10
- Added blinkPhase (2Hz) and scanline struct members ready for Plan 02 and Plan 03

## Task Commits

Each task was committed atomically:

1. **Task 1: Download JetBrains Mono NL font** - `9eab29f` (chore)
2. **Task 2: Overhaul display container** - `d0c8f3d` (feat)

## Files Created/Modified
- `res/fonts/JetBrainsMonoNL-Regular.ttf` - JetBrains Mono NL Regular v2.304 (no ligatures variant, 208KB)
- `src/AnalogLFO.cpp` - WaveformDisplay struct: new drawBorder, drawCornerBrackets, drawZeroCrossing methods; updated drawBackground color/radius; updated step() rates; new struct members; updated drawLayer rendering order

## Decisions Made
- Used single breathePhase at 0.2Hz for both border glow and phase dot idle breathing (per RESEARCH.md Pitfall 2 recommendation: slower dot breathing is acceptable as a subtle idle effect)
- Added separate blinkPhase accumulator at 2Hz specifically for SYNC ACQUIRING indicator blink, ensuring it is independent of the border glow rate
- Corner bracket dimensions scaled from mockup (5px inset, 8px arm at 5x scale) to widget pixels (3px inset, 5px arm at 0.59x scale)
- Border glow uses NVG_HOLE inner-cutout fill approach rather than separate outer rect, preventing scissor clipping issues

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- JetBrains Mono NL font available for Plan 02 pill text rendering
- blinkPhase accumulator running at 2Hz, ready for Plan 02 SYNC ACQUIRING blink
- scanlineImage and scanlineScrollPhase initialized, ready for Plan 03 CRT overlay
- drawLayer rendering order updated with slots for drawZeroCrossing before waveform trace
- Build succeeds with all changes

## Self-Check: PASSED

- FOUND: res/fonts/JetBrainsMonoNL-Regular.ttf
- FOUND: src/AnalogLFO.cpp
- FOUND: 20-01-SUMMARY.md
- FOUND: commit 9eab29f (Task 1)
- FOUND: commit d0c8f3d (Task 2)

---
*Phase: 20-display-layout-crt-aesthetic*
*Completed: 2026-04-02*
