---
phase: 20-display-layout-crt-aesthetic
plan: 03
subsystem: ui
tags: [nanovg, display, scanlines, crt, breathing-glow, forge-noir, ember]

# Dependency graph
requires:
  - phase: 20-display-layout-crt-aesthetic
    plan: 01
    provides: scanlineImage and scanlineScrollPhase struct members, scanlineScrollPhase increment in step(), near-black background, ember border, breathing glow
  - phase: 20-display-layout-crt-aesthetic
    plan: 02
    provides: Three-column layout, ember-glow waveform, repositioned pills, JetBrains Mono NL font
provides:
  - CRT scanline overlay (drawScanlines) rendered last in drawLayer over all content
  - Faint ember phosphor-row lines (manual loop, 2.5px spacing) scrolling downward via scanlineScrollPhase
  - Brightened breathing border glow (0.15-0.40 opacity) with widened 4px glow/scissor bounds
affects: []

# Tech tracking
tech-stack:
  added: []
  patterns: [manual-loop-scanlines, ember-phosphor-rows]

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "Scanlines reimplemented as a manual nvgRect loop drawing faint ember lines instead of the NanoVG image-pattern dark-band approach (better visual match to Forge Noir ember palette)"
  - "Breathing glow opacity boosted 0.08-0.18 -> 0.15-0.40 and glow/scissor bounds widened 2px -> 4px for on-screen visibility"
  - "drawZeroCrossing() removed from the render path during visual tuning"

patterns-established:
  - "Manual-loop scanlines: for-loop of thin nvgRect lines at fixed spacing, scroll offset via fmodf-wrapped phase accumulator"

requirements-completed: [DISP-04]

# Metrics
duration: resumed-session
completed: 2026-06-12
---

# Phase 20 Plan 03: CRT Scanline Overlay + Visual Verification Summary

**Final CRT aesthetic layer for the Forge Noir display — faint ember scanlines scrolling over all content, brighter breathing border glow, verified in VCV Rack against all 5 DISP requirements**

## Performance

- **Tasks:** 2 (1 auto, 1 human-verify checkpoint)
- **Files modified:** 1
- **Note:** Plan executed across two sessions — Task 1 committed in a prior session, refined and verified after a resume.

## Accomplishments
- Added drawScanlines() to WaveformDisplay, called last in drawLayer so scanlines render over waveform, pills, and dot (both module and placeholder paths)
- Scanlines render as faint ember phosphor-row lines scrolling slowly downward, driven by scanlineScrollPhase
- Breathing border glow brightened for visibility and glow/scissor bounds widened to 4px
- All 5 DISP requirements visually verified in VCV Rack 2 Pro (user approved)

## Task Commits

1. **Task 1: Add CRT scanline overlay (image pattern)** - `79be789` (feat) — original plan-spec implementation
2. **Refinement: ember scanlines + brighter breathing glow** - `3ebe0d0` (feat) — visual tuning that superseded the Task 1 approach
3. **Task 2: Human visual verification** - approved by user (no code change)

## Files Created/Modified
- `src/AnalogLFO.cpp` - WaveformDisplay struct: drawScanlines() manual-loop ember-line implementation + call in drawLayer; drawBorder breathing-glow opacity/bounds boost; drawZeroCrossing call removed from render path

## Decisions Made
- Scanlines reimplemented as a manual `nvgRect` loop drawing faint ember lines (`nvgRGBAf(0.91, 0.365, 0.149, 0.10)`, 2.5px spacing) rather than the committed NanoVG image-pattern dark-band approach — better matches the Forge Noir ember palette and reads as warm phosphor rows rather than dark bands
- Breathing glow opacity raised to 0.15-0.40 and glow/scissor widened to 4px so the border pulse is actually visible on screen
- `drawZeroCrossing()` removed from the render path during tuning

## Deviations from Plan

**Significant — documented and user-approved during the human-verify checkpoint:**
1. **Scanline implementation rewritten.** The plan specified a NanoVG image pattern (`nvgCreateImageRGBA` 1x4 tile, `nvgImagePattern`, dark bands at ~0.04 alpha). The shipped implementation is a manual `nvgRect` loop drawing faint *ember* lines. The plan's `must_haves` artifacts reference `nvgCreateImageRGBA`/`nvgImagePattern`, which no longer appear in the code — the must-have *intent* (faint scrolling scanlines over the display) is satisfied, but by a different mechanism.
2. **`drawZeroCrossing()` removed.** Task 2's "what-built" description lists a dashed zero-crossing line; it is no longer rendered. Removed deliberately during visual tuning.

Both deviations were reviewed with the user before close-out and explicitly approved.

## Issues Encountered

None — build succeeds (`make install`), plugin installed to Rack2 plugins folder.

## User Setup Required

None.

## Next Phase Readiness
- Phase 20 display overhaul complete: three-column layout, ember styling, JetBrains Mono, corner brackets, breathing glow, CRT scanlines
- All 5 DISP requirements (DISP-01 through DISP-05) implemented and visually verified
- Build succeeds and runs without errors in VCV Rack 2 Pro

## Self-Check: PASSED

- FOUND: src/AnalogLFO.cpp
- FOUND: commit 79be789 (Task 1)
- FOUND: commit 3ebe0d0 (refinement)
- drawScanlines present and called in drawLayer
- make install succeeds
- Human-verify checkpoint: APPROVED by user (all 5 DISP requirements)
- NOTE: nvgCreateImageRGBA/nvgImagePattern intentionally removed (see Deviations)

---
*Phase: 20-display-layout-crt-aesthetic*
*Completed: 2026-06-12*
