---
phase: 08-frequency-override-and-ratio-table
plan: 01
subsystem: dsp
tags: [clock-sync, ratio-table, frequency-override, param-quantity, vcv-rack]

# Dependency graph
requires:
  - phase: 07-clock-input-and-period-tracking
    provides: "ClockState enum, smoothedPeriod, displayClockState atomic, processClockInput()"
provides:
  - "RATIO_TABLE[15] static constexpr frequency multipliers (/16 to x16)"
  - "RATIO_LABELS[15] static constexpr display strings"
  - "displayRatioIndex atomic for lock-free GUI state transfer"
  - "RateParamQuantity nested struct with dual-mode tooltip"
  - "Dual-mode frequency override in process() (Hz when free, ratio*clockFreq when clocked)"
affects: [09-phase-reset-and-drift, 10-clock-display]

# Tech tracking
tech-stack:
  added: []
  patterns: ["dual-mode frequency override gated on clockState+smoothedPeriod", "custom ParamQuantity for context-aware tooltip", "getScaledValue() for normalized knob position independent of param range"]

key-files:
  created: []
  modified: ["src/AnalogLFO.cpp"]

key-decisions:
  - "Used paramQuantities[RATE_PARAM]->getScaledValue() for 0-1 normalized knob position, avoiding manual rescaling of the 0.01-20Hz range"
  - "Ratio index derived at runtime from knob position -- not serialized, not stored as state"
  - "No hysteresis, no slew, no crossfade -- pure nearest-snap with instant frequency jump (Phase 9 layers smoothing on top)"

patterns-established:
  - "Custom ParamQuantity subclass via configParam<T>() template for context-dependent tooltips"
  - "Dual-mode conditional: if (clockState != FREE && smoothedPeriod > 0) for clocked behavior"
  - "displayRatioIndex atomic follows same lock-free pattern as displayClockState, displayPhase, displayDrift"

requirements-completed: [RATE-01, RATE-02, RATE-03, RATE-06]

# Metrics
duration: 7min
completed: 2026-03-10
---

# Phase 8 Plan 1: Frequency Override and Ratio Table Summary

**15-ratio clock divider/multiplier table with dual-mode Rate knob: continuous Hz when free-running, snapped musical ratios (/16 to x16) when clock-synced, with custom ParamQuantity tooltip**

## Performance

- **Duration:** 7 min
- **Started:** 2026-03-09T21:51:26Z
- **Completed:** 2026-03-09T21:58:41Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments
- RATIO_TABLE[15] with musical multipliers from /16 (0.0625) to x16 (16.0) covering divisions, triplets, dotted notes, and multiplications
- Dual-mode frequency calculation in process(): free-running path identical to v1.0, clocked path derives freq from smoothedPeriod and selected ratio
- RateParamQuantity nested struct overrides getDisplayValueString() and getUnit() for context-aware tooltip (Hz vs ratio label with "(synced)" suffix)
- All four RATE requirements verified through hands-on VCV Rack testing

## Task Commits

Each task was committed atomically:

1. **Task 1: Add ratio table, frequency override, and custom RateParamQuantity** - `2221691` (feat)
2. **Task 2: Verify dual-mode Rate knob in VCV Rack** - checkpoint:human-verify (approved, no code changes)

## Files Created/Modified
- `src/AnalogLFO.cpp` - Added RATIO_TABLE, RATIO_LABELS, displayRatioIndex atomic, RateParamQuantity nested struct, dual-mode freq override in process(), configParam<RateParamQuantity> template

## Decisions Made
- Used `paramQuantities[RATE_PARAM]->getScaledValue()` for normalized 0-1 knob position rather than manual rescaling -- cleaner and automatically tracks any future param range changes
- Ratio index is purely runtime-derived from knob position + clock state; not serialized or persisted
- Kept Phase 8 deliberately simple per CONTEXT.md: no hysteresis, no slew, no crossfade, no Nth-edge reset logic (all deferred to Phase 9)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- RATIO_TABLE and frequency override ready for Phase 9 to layer anti-click crossfade on ratio changes
- displayRatioIndex atomic ready for Phase 10 to show ratio badge on display
- Every-edge phase reset (from Phase 7) is expected behavior; Phase 9 will implement Nth-edge reset logic for divided ratios
- No slew or smoothing on frequency jumps -- Phase 9 adds this

## Self-Check: PASSED

- FOUND: src/AnalogLFO.cpp
- FOUND: 08-01-SUMMARY.md
- FOUND: commit 2221691 (Task 1)

---
*Phase: 08-frequency-override-and-ratio-table*
*Completed: 2026-03-10*
