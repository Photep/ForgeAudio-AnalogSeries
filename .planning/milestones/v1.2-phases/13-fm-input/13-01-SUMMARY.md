---
phase: 13-fm-input
plan: 01
subsystem: dsp
tags: [fm, exponential-fm, frequency-modulation, exp2_taylor5, attenuator]

# Dependency graph
requires:
  - phase: 12-reset-phase-offset
    provides: "Phase offset readout path, frequency slew filter, isClocked boolean"
provides:
  - "FM_ATTEN_PARAM and FM_INPUT enum entries"
  - "Exponential FM processing block in process() (post-slew)"
  - "Dual-scale FM authority (clocked vs free mode)"
  - "FM trimpot and jack widgets at temporary panel positions"
affects: [14-expanded-imperfections, 16-swing-shuffle, 17-panel-redesign]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Post-slew FM insertion: FM applied after freqSlew.process() to avoid lowpass filtering"
    - "Dual authority scaling: isClocked ? 0.5f : 0.6f for clock-compatible FM"

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "FM processing moved after frequency slew filter (not before as originally planned) to prevent 3Hz lowpass from filtering out modulation"
  - "Clocked depth scale set to 0.5f (not 0.1f as planned) because clock phase resets already enforce sync, 0.1f was inaudibly weak"
  - "FM attenuator default 0.0 (not 1.0) so patching a cable does not immediately modulate -- opt-in design"

patterns-established:
  - "Post-slew modulation: frequency modulation applied after slew filtering to preserve modulation bandwidth"
  - "Dual-authority pattern: isClocked ternary for scaling modulation inputs"

requirements-completed: [MOD-01, MOD-02]

# Metrics
duration: ~15min
completed: 2026-03-15
---

# Phase 13 Plan 01: FM Input Summary

**Exponential FM input with attenuator and dual-authority clocked scaling via post-slew exp2_taylor5 multiplication**

## Performance

- **Duration:** ~15 min (across two sessions with human verification)
- **Tasks:** 2 (1 auto + 1 human-verify checkpoint)
- **Files modified:** 1

## Accomplishments
- Added FM input jack and attenuator trimpot with exponential frequency modulation
- FM authority automatically reduced in clocked mode (0.5x vs 0.6x depth scale) to preserve clock sync
- FM at default attenuator (0.0) produces output identical to v1.1 -- zero modulation
- LFO stable under extreme FM inputs (+/-10V at full attenuator)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add FM input jack, attenuator, and exponential FM processing** - `ff87325` (feat)
2. **Task 2: Verify FM behavior in VCV Rack** - `43d25a7` (fix -- post-verification corrections)

The fix commit (43d25a7) addressed two issues found during human verification:
- FM processing moved AFTER frequency slew filter (was being filtered out by ~3Hz lowpass)
- Clocked depth scale changed from 0.1f to 0.5f (original 0.1f was too conservative)

## Files Created/Modified
- `src/AnalogLFO.cpp` - FM_ATTEN_PARAM and FM_INPUT enums, configParam/configInput calls, exponential FM processing block in process(), FM trimpot and jack widgets at temporary panel positions

## Decisions Made
- **Post-slew FM insertion:** Originally planned to insert FM before the frequency slew filter. During verification, FM modulation was inaudible because the ~3Hz slew lowpass was filtering it out. Moved FM multiplication to after `freqSlew.process()` so modulation bandwidth is preserved.
- **Clocked depth 0.5f (not 0.1f):** Plan specified 0.1f for conservative clock compatibility. Testing showed this was inaudibly weak. Since clock phase resets already enforce sync at clock edges, 0.5f provides usable modulation while maintaining sync.
- **FM attenuator defaults to 0.0:** Ensures backward compatibility -- patching a cable into the FM jack does nothing until the user explicitly turns up the attenuator.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] FM processing moved after frequency slew filter**
- **Found during:** Task 2 (human verification)
- **Issue:** FM modulation applied before freqSlew.process() was being filtered out by the ~3Hz lowpass slew filter, making FM inaudible
- **Fix:** Moved FM multiplication block to after `freq = freqSlew.process(args.sampleTime, targetFreq)` and `freq = std::fmax(freq, 0.001f)`
- **Files modified:** src/AnalogLFO.cpp
- **Verification:** FM modulation visible and audible in VCV Rack after fix
- **Committed in:** 43d25a7

**2. [Rule 1 - Bug] Clocked depth scale too conservative**
- **Found during:** Task 2 (human verification)
- **Issue:** Clocked depth scale of 0.1f produced inaudibly weak FM modulation; clock phase resets already enforce sync so heavy attenuation was unnecessary
- **Fix:** Changed depthScale from 0.1f to 0.5f for clocked mode
- **Files modified:** src/AnalogLFO.cpp
- **Verification:** FM noticeable in clocked mode at 0.5f, clock sync maintained via phase resets
- **Committed in:** 43d25a7

---

**Total deviations:** 2 auto-fixed (2 bugs found during verification)
**Impact on plan:** Both fixes were necessary for correct FM behavior. The insertion point change was a design oversight in the plan (did not account for slew filter interaction). The depth scale was an empirical tuning correction. No scope creep.

## Issues Encountered
None beyond the deviations documented above.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- FM input fully functional, ready for Phase 14 (Expanded Imperfections)
- FM controls at temporary panel positions (8.0, 118.0 and 20.0, 118.0) -- Phase 17 will finalize layout
- Blocker "FM clocked-mode authority design decision" from STATE.md is now resolved (0.5f depth scale)

## Self-Check: PASSED

- [x] src/AnalogLFO.cpp exists with FM_ATTEN_PARAM (4 refs) and FM_INPUT (5 refs)
- [x] Commit ff87325 exists (feat: add FM input)
- [x] Commit 43d25a7 exists (fix: post-verification corrections)
- [x] 13-01-SUMMARY.md created

---
*Phase: 13-fm-input*
*Completed: 2026-03-15*
