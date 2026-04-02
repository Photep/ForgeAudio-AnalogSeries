---
phase: 20-display-layout-crt-aesthetic
plan: 02
subsystem: ui
tags: [nanovg, display, waveform, pills, three-column-layout, forge-noir, ember, jetbrains-mono]

# Dependency graph
requires:
  - phase: 20-display-layout-crt-aesthetic
    plan: 01
    provides: Near-black background, ember border, corner brackets, zero-crossing line, JetBrains Mono NL font asset, blinkPhase accumulator
provides:
  - Three-column display layout (left pills, center waveform, right pills)
  - Waveform trace constrained to center 60% of display width via phaseToX()
  - 3-layer ember glow waveform rendering (widths 7/3.5/2, opacities 0.06/0.15/0.85)
  - 3-circle concentric phase dot (ember outer/mid, hot gold core)
  - Ember-tinted pill backgrounds with stroke border (replaces navy feathered boxes)
  - JetBrains Mono NL font used for all display text
  - Column-based text positioning (left 11%, right 89%)
  - SYNC ACQUIRING 2Hz blink via blinkPhase (independent of border glow)
affects: [20-03 scanline-overlay]

# Tech tracking
tech-stack:
  added: []
  patterns: [ember-pill-fill-stroke, three-column-layout, concentric-circle-phase-dot, 3-layer-ember-glow]

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "phaseToX uses proportional 0.20/0.60 fractions (not hardcoded pixels) for zoom compatibility"
  - "Hz readout shows 1 decimal for <10Hz, integer otherwise (per UI-SPEC copywriting contract)"
  - "All pill text uses NVG_ALIGN_CENTER instead of LEFT/RIGHT for column-centered positioning"
  - "Font blur reduced from 3.0 to 2.0 for small font sizes (2.9-4.1px) to preserve readability"

patterns-established:
  - "Ember pill pattern: rounded rect fill at 0.10*alpha + stroke at 0.25*alpha + 2-pass glow text"
  - "Column positioning: left at 11% width, right at 89% width, top at 6px, bottom at height-4px"
  - "Widget-scale font sizes: 4.1/3.5/3.2/2.9px for label/value/small/micro roles"

requirements-completed: [DISP-01, DISP-02]

# Metrics
duration: 6min
completed: 2026-04-02
---

# Phase 20 Plan 02: Three-Column Layout + Forge Noir Styling Summary

**Three-column display layout with ember-glow waveform trace, concentric gold-core phase dot, and JetBrains Mono NL pill text in left/right margin columns**

## Performance

- **Duration:** 6 min
- **Started:** 2026-04-02T04:15:31Z
- **Completed:** 2026-04-02T04:21:22Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments
- Waveform trace constrained to center 60% of display (20%-80% via phaseToX) with 3-layer ember glow replacing 4-pass amber
- Phase dot rendered as 3 concentric circles (ember outer glow, ember mid ring, hot gold core) replacing radial gradient halo
- All data pills repositioned into left column (Hz, ratio, swing) and right column (SYNC, BPM stack)
- Pills restyled with ember-tinted fill+stroke backgrounds replacing navy feathered boxes (no nvgBoxGradient calls remain)
- Font switched from ShareTechMono (system) to JetBrains Mono NL (bundled) with widget-scale sizes
- SYNC ACQUIRING blink correctly uses blinkPhase (2Hz) instead of breathePhase (0.2Hz)

## Task Commits

Each task was committed atomically:

1. **Task 1: Update coordinate helpers and waveform/dot rendering** - `1109028` (feat)
2. **Task 2: Restyle pills and reposition all text overlays** - `567ec86` (feat)

## Files Created/Modified
- `src/AnalogLFO.cpp` - WaveformDisplay struct: phaseToX center-column constraint, drawWaveformTrace 3-layer ember glow, drawPhaseDot 3-circle concentric, drawPillText ember fill+stroke, drawGlowText ember color, drawTextOverlays column-based layout with JetBrains Mono NL, drawBpmStack right-column positioning with ember styling, drawPlaceholder ember color

## Decisions Made
- phaseToX uses proportional fractions (0.20/0.60 of box.size.x) rather than hardcoded pixel values, ensuring layout scales correctly across VCV Rack zoom levels
- Hz readout format changed from "%.2f Hz" to "%.1f Hz" for <10Hz and "%d Hz" otherwise, per UI-SPEC copywriting contract
- All pill text alignment changed to NVG_ALIGN_CENTER for consistent column-centered positioning
- Font blur reduced from 3.0 to 2.0 across all glow text passes to prevent unreadable blobs at sub-5px font sizes

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Three-column layout complete, all pills in margin columns, waveform in center
- Display ready for Plan 03 CRT scanline overlay (scanlineImage and scanlineScrollPhase already initialized by Plan 01)
- All amber colors replaced with ember throughout display rendering
- Build succeeds with all changes

## Self-Check: PASSED

- FOUND: src/AnalogLFO.cpp
- FOUND: commit 1109028 (Task 1)
- FOUND: commit 567ec86 (Task 2)
- ShareTechMono count: 0 (completely removed)
- nvgBoxGradient count: 0 (completely removed)
- nvgRadialGradient count: 0 (completely removed)
- JetBrainsMonoNL-Regular.ttf referenced in drawTextOverlays

---
*Phase: 20-display-layout-crt-aesthetic*
*Completed: 2026-04-02*
