---
phase: 12-reset-and-phase-offset
plan: 02
subsystem: dsp
tags: [phase-offset, cv-modulation, quadrature, waveform, vcv-rack]

# Dependency graph
requires:
  - phase: 12-reset-and-phase-offset
    plan: 01
    provides: "RESET_INPUT, resetBlanking, crossfade mechanism"
provides:
  - "PHASE_OFFSET_PARAM and PHASE_OFFSET_ATTEN_PARAM enum members"
  - "PHASE_OFFSET_CV_INPUT enum member and jack"
  - "Offset-at-readout waveform generation (offset applied before computeMorphedWave, not in accumulator)"
  - "Offset-inclusive displayPhase (dot tracks output position)"
  - "Phase Offset knob, attenuator trimpot, and CV jack on panel (temporary positions)"
affects: [13, 17]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Offset-at-readout: phase offset added to readout variable, not to accumulator, preserving drift/clock/reset behavior"
    - "Display dot tracks offset-inclusive phase while waveform buffer renders raw 0-1 sweep"

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "Offset applied at readout (float p = phase + offset) not in accumulator -- preserves all existing timing behavior"
  - "Display buffer renders without offset; display dot (displayPhase) includes offset -- knob visually slides dot along waveform"
  - "Phase Offset controls at temporary panel positions -- Phase 17 finalizes layout"

patterns-established:
  - "Offset-at-readout: add offset to readout phase before waveform lookup, wrap with if(p>=1) p-=1"
  - "Display separation: buffer shows raw waveform shape, dot shows offset-inclusive read position"

requirements-completed: [PHASE-01, PHASE-02]

# Metrics
duration: 15min
completed: 2026-03-15
---

# Phase 12 Plan 02: Phase Offset Summary

**Phase Offset knob with CV input implementing offset-at-readout waveform generation, enabling quadrature patches and real-time phase animation**

## Performance

- **Duration:** ~15 min (continuation from checkpoint)
- **Started:** 2026-03-14T09:00:00Z
- **Completed:** 2026-03-15
- **Tasks:** 2 (1 auto + 1 human-verify)
- **Files modified:** 1

## Accomplishments
- Phase Offset knob (0-360 degrees) with attenuator and CV input for real-time modulation
- Offset-at-readout architecture: offset applied before computeMorphedWave, not in phase accumulator, preserving clock sync, RESET, and drift behavior
- Display dot tracks offset-inclusive position while waveform buffer remains un-offset (turning knob slides dot along waveform)
- Smooth, click-free offset changes verified in VCV Rack including quadrature (90-degree pair) operation

## Task Commits

Each task was committed atomically:

1. **Task 1: Add Phase Offset param, CV input, and offset-at-readout logic** - `3dc709a` (feat)
2. **Task 2: Verify Phase Offset behavior in VCV Rack** - human-verify checkpoint (approved)

**Plan metadata:** [pending] (docs: complete plan)

## Files Created/Modified
- `src/AnalogLFO.cpp` - Added PHASE_OFFSET_PARAM, PHASE_OFFSET_ATTEN_PARAM, PHASE_OFFSET_CV_INPUT enums; offset-at-readout waveform generation; offset-inclusive displayPhase; Phase Offset knob, trimpot, and CV jack widgets at temporary positions

## Decisions Made
- Offset applied at readout (`float p = (float)phase + phaseOffset`) rather than modifying the accumulator -- this preserves all existing timing behavior (clock sync, RESET, drift) without any interaction issues
- Display buffer renders the raw 0-1 waveform shape without offset; the display dot (via displayPhase) includes offset -- this provides visual feedback of "which part of the cycle is being output" by sliding the dot along the fixed waveform shape
- Phase Offset controls placed at temporary panel positions (knob at 30.48,86; trimpot at 52,96; CV jack at 52,118) -- Phase 17 panel redesign will finalize all control positions

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 12 complete: RESET trigger and Phase Offset both verified and functional
- Both features use temporary panel positions documented for Phase 17 layout redesign
- Phase Offset does not interfere with CLK sync or RESET -- all three features coexist cleanly
- Ready for Phase 13 (FM input) development

## Self-Check: PASSED

- FOUND: src/AnalogLFO.cpp
- FOUND: 12-02-SUMMARY.md
- FOUND: 3dc709a (Task 1 commit)

---
*Phase: 12-reset-and-phase-offset*
*Completed: 2026-03-15*
