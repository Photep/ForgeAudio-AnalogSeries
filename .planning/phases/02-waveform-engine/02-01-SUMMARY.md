---
phase: 02-waveform-engine
plan: 01
subsystem: dsp
tags: [vcv-rack, lfo, panel-layout, trimpot, attenuator, linear-rate]

# Dependency graph
requires:
  - phase: 01-plugin-scaffold-and-panel
    provides: "Module scaffold with enums, SVG panel, PANEL-SPEC.md"
provides:
  - "MORPH_ATTEN_PARAM enum and configParam for morph CV attenuation"
  - "Linear rate mapping (0.01-20Hz) replacing exponential"
  - "Single output (INV removed) with redesigned bottom row layout"
  - "double phase member variable ready for DSP"
  - "cmath include ready for DSP"
affects: [02-02-PLAN, waveform-engine-dsp]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Trimpot for CV attenuator grouped visually near its input jack"
    - "Linear Hz rate mapping for LFO (raw value = frequency)"

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp
    - res/AnalogLFO.svg
    - res/PANEL-SPEC.md

key-decisions:
  - "Linear rate 0.01-20Hz with default 0.7Hz - raw param value IS frequency in Hz"
  - "Morph CV attenuator defaults to 0 (no CV effect until user turns up)"
  - "Bottom row: Trimpot at x=9, MCV jack at x=21, DCV jack at x=35, OUT jack at x=51"

patterns-established:
  - "Three sources of truth sync: C++ widget, SVG components layer, PANEL-SPEC.md table"

requirements-completed: [OUT-02, PTCH-01, WAVE-03]

# Metrics
duration: 3min
completed: 2026-02-25
---

# Phase 02 Plan 01: Module Scaffold Restructure Summary

**Restructured LFO scaffold: removed INV output, added Morph CV attenuator Trimpot, switched rate to linear 0.01-20Hz, redesigned bottom row for 3 jacks + 1 trimpot**

## Performance

- **Duration:** 3 min
- **Started:** 2026-02-25T07:46:22Z
- **Completed:** 2026-02-25T07:48:52Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Removed INV_OUTPUT from enum, configOutput, widget, SVG label, and SVG component circle
- Added MORPH_ATTEN_PARAM with Trimpot widget (6.05mm) at (9.0, 104.0) near Morph CV jack
- Changed Rate param from exponential (base-2, -8 to +4) to linear Hz (0.01 to 20.0, default 0.7) -- tooltip now shows "Rate: 0.70 Hz"
- Redesigned bottom row with 4 elements evenly spaced across panel width
- Added cmath include and double phase member for upcoming DSP engine (Plan 02)
- All three sources of truth (C++, SVG, PANEL-SPEC.md) verified synchronized with 9 components

## Task Commits

Each task was committed atomically:

1. **Task 1: Restructure enums, configParams, and widget layout in AnalogLFO.cpp** - `76f9d3e` (feat)
2. **Task 2: Update SVG panel and PANEL-SPEC.md to match new layout** - `5a93327` (feat)

## Files Created/Modified
- `src/AnalogLFO.cpp` - Added MORPH_ATTEN_PARAM, removed INV_OUTPUT, linear rate, Trimpot widget, cmath include, phase member
- `res/AnalogLFO.svg` - Removed INV label, repositioned MCV/DCV/OUT labels, added Trimpot component circle, updated jack circles
- `res/PANEL-SPEC.md` - Updated component table (9 components), layout zones, spacing docs, SVG example

## Decisions Made
- Linear rate mapping (0.01-20Hz, default 0.7Hz) chosen over exponential -- raw param value IS the frequency, making DSP computation simpler and tooltip display cleaner
- Morph CV attenuator defaults to 0 (no CV effect) rather than 1 (full CV) -- safer default, user must intentionally enable CV modulation
- Bottom row spacing: 12mm between Trimpot and MCV (visual pairing), 14mm to DCV, 16mm to OUT -- progressive spacing gives breathing room at the output end

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Scaffold is ready for Plan 02 (DSP engine implementation)
- MORPH_ATTEN_PARAM available for CV attenuation math
- Linear rate value can be used directly as Hz frequency
- double phase member ready for oscillator accumulation
- cmath available for sin/cos/abs DSP functions

## Self-Check: PASSED

All files exist, all commits verified.

---
*Phase: 02-waveform-engine*
*Completed: 2026-02-25*
