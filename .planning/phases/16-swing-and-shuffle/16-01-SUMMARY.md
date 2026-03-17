---
phase: 16-swing-and-shuffle
plan: 01
subsystem: audio-dsp
tags: [swing, shuffle, groove, clocked-lfo, context-menu, display-overlay]

# Dependency graph
requires:
  - phase: 13-fm-input
    provides: FM and phase features stable before swing layers on
  - phase: 11-display-polish
    provides: Pill-backed text overlay system (drawPillText)
provides:
  - MPC-style swing timing with 6 named presets via SWING_FRACTIONS table
  - Right-click context menu (first appendContextMenu on AnalogLFOWidget)
  - Swing-warped display buffer rendering
  - Swing text overlay in bottom-left with fade animation
  - Swing-aware phase dot positioning via inverse phase-to-time mapping
affects: [17-panel-redesign]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "deltaPhase multiplier pattern for timing warp (swing applied after drift/jitter, before phase accumulation)"
    - "Phase-to-time / time-to-phase inverse pair for display buffer and dot sync"
    - "createIndexSubmenuItem for right-click preset selection menus"
    - "Display atomic bridge (displaySwingIndex, displaySwingFraction) for audio-to-UI thread communication"

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "Swing applied as deltaPhase multiplier after drift/jitter (commutative, preserves groove feel)"
  - "No smoothing at phase 0.5 boundary -- instant rate change matches MPC/Akai swing behavior"
  - "Swing gated by isClocked -- zero effect in free-running mode (PHASE-04)"
  - "6 named presets (Straight through Max 75%) via right-click submenu, not knob"
  - "Display buffer uses time-to-phase mapping; phase dot uses inverse phase-to-time mapping"

patterns-established:
  - "appendContextMenu pattern: first right-click menu on AnalogLFOWidget, extensible for future settings"
  - "Swing overlay follows existing fade system (200ms transition, clocked+active gating)"

requirements-completed: [PHASE-03, PHASE-04]

# Metrics
duration: 15min
completed: 2026-03-17
---

# Phase 16 Plan 01: Swing and Shuffle Summary

**MPC-style swing timing with 6 presets (50%-75%) via deltaPhase multiplier, right-click context menu, phase-warped display, and swing text overlay**

## Performance

- **Duration:** ~15 min (implementation + human verification)
- **Started:** 2026-03-17T04:30:00Z
- **Completed:** 2026-03-17T07:33:00Z
- **Tasks:** 2 (1 auto + 1 human-verify)
- **Files modified:** 1

## Accomplishments
- Swing deltaPhase multiplier in process() creates long-short beat pair groove in clocked mode, gated by isClocked for free-running isolation
- 6 named presets (Straight 50%, Light 54%, Medium 58%, Triplet 66%, Heavy 71%, Max 75%) accessible via right-click context menu
- Display waveform renders swing-warped timing via time-to-phase mapping in updateDisplayBuffer()
- Phase dot position correctly tracks warped waveform via inverse phase-to-time remapping
- Swing text overlay with pill background in bottom-left corner with 200ms fade animation
- Serialization of swingIndex via dataToJson/dataFromJson with clamp validation

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement swing state, audio path, context menu, serialization, and display** - `3885a9a` (feat)
2. **Task 2: Verify swing behavior in VCV Rack** - human-verified, approved (no commit -- verification checkpoint)

**Plan metadata:** (pending -- docs commit)

## Files Created/Modified
- `src/AnalogLFO.cpp` - Added SWING_FRACTIONS/SWING_MENU_LABELS/SWING_OVERLAY_LABELS constants, swingIndex state, deltaPhase swing multiplier in process(), swing-warped updateDisplayBuffer(), swing text overlay in drawTextOverlays(), swing-aware drawPhaseDot(), appendContextMenu with Swing submenu, dataToJson/dataFromJson serialization (+110 lines, -5 lines)

## Decisions Made
- Swing applied as deltaPhase multiplier after drift/jitter, before phase accumulation -- multiplication is commutative so order is flexible, but this placement follows conceptual model (drift perturbs swing-structured timing)
- No smoothing at phase 0.5 boundary -- instant rate change is intentional, matches MPC/Akai swing behavior for authentic groove feel
- Swing gated by isClocked with threshold 0.5001f for float comparison safety -- satisfies PHASE-04 (no effect in free-running mode)
- 6 named presets via right-click submenu using createIndexSubmenuItem -- matches VCV Rack convention for module settings
- Display buffer uses time-to-phase mapping (uniform time samples with warped phase lookup); phase dot uses the mathematical inverse (phase-to-time) for correct visual tracking
- Swing overlay visible only when clocked AND swingIndex > 0 (both conditions required)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- All DSP features for v1.2 are now complete (drift, FM, phase offset, reset, bleed, swing)
- Phase 17 (Panel Redesign) is the final phase -- focuses on SVG layout for all new jacks and controls
- The appendContextMenu pattern established here can be extended in Phase 17 if additional right-click settings are needed

## Self-Check: PASSED

- FOUND: src/AnalogLFO.cpp
- FOUND: commit 3885a9a (Task 1)
- FOUND: 16-01-SUMMARY.md

---
*Phase: 16-swing-and-shuffle*
*Completed: 2026-03-17*
