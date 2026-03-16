---
phase: 14-expanded-imperfections
plan: 01
subsystem: dsp
tags: [drift, pitch-slew, phase-jitter, analog-imperfections, exponential-filter]

# Dependency graph
requires:
  - phase: 13-fm-input
    provides: "FM processing block and frequency signal chain in process()"
provides:
  - "driftSlew TExponentialFilter for thermal frequency lag (CHAR-03)"
  - "Per-sample phase jitter via white noise on deltaPhase (CHAR-01)"
  - "Drift parameter reads moved earlier in process() for pitch slew gating"
affects: [14-02, 15-waveform-bleed, 16-swing-shuffle]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Layered exponential filters: freqSlew (mode transitions) -> driftSlew (thermal lag) -> FM"
    - "Drift-gated imperfection with progressive curve and clocked authority scaling"

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "driftSlew lambda 500 for bypass (effectively instant at Drift=0) rather than conditional branch"
  - "Phase jitter magnitude 0.3% max deviation -- conservative for subtle trace thickening"
  - "Drift parameter reads moved ~20 lines earlier in process() to gate pitch slew (safe: pure parameter read with no dependencies)"

patterns-established:
  - "Separate TExponentialFilter instances for independent slew concerns (mode transition vs thermal lag)"
  - "Phase jitter via independent white noise multiplicative on deltaPhase (not correlated with OU layers)"

requirements-completed: [CHAR-03, CHAR-01]

# Metrics
duration: 3min
completed: 2026-03-16
---

# Phase 14 Plan 01: Pitch Slew and Phase Jitter Summary

**Drift-gated thermal frequency lag via driftSlew filter (2ms-300ms tau) and per-sample phase jitter (0.3% max) for analog oscillator instability**

## Performance

- **Duration:** 3 min
- **Started:** 2026-03-16T10:25:17Z
- **Completed:** 2026-03-16T10:27:53Z
- **Tasks:** 1
- **Files modified:** 1

## Accomplishments
- Added driftSlew TExponentialFilter as a separate thermal frequency lag filter layered after existing freqSlew
- Added per-sample phase jitter using independent white noise from existing normalDist(rng)
- Both features are drift-gated (inactive at Drift=0) with progressive curve scaling and reduced clocked authority (2% vs 7.5%)
- Signal chain order preserved: freqSlew -> driftSlew -> FM -> deltaPhase -> OU drift -> phase jitter -> phase accumulation

## Task Commits

Each task was committed atomically:

1. **Task 1: Add pitch slew filter and phase jitter to process()** - `5e49e9d` (feat)

**Plan metadata:** pending (docs: complete plan)

## Files Created/Modified
- `src/AnalogLFO.cpp` - Added driftSlew member, constructor init, pitch slew block between freqSlew and FM, phase jitter in OU drift block

## Decisions Made
- driftSlew bypass uses lambda=500 (not conditional branch) for branchless hot path -- at lambda=500 the filter converges in <2ms
- Phase jitter magnitude set to 0.003f (0.3% max) -- conservative to produce subtle trace thickening, not visible wobble
- Drift parameter reads (driftKnob, driftAtten, driftCV, drift clamp) moved ~20 lines earlier in process() to gate the pitch slew block -- safe because these are pure parameter reads with no dependencies on phase or waveform state

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Pitch slew and phase jitter complete; ready for Plan 14-02 (DC offset wander and component spread)
- The drift parameter reads are now available earlier in process(), which Plan 14-02's DC offset wander can also leverage
- Component spread (CHAR-04) will need dataToJson/dataFromJson serialization (new to this module)

## Self-Check: PASSED

- [x] src/AnalogLFO.cpp exists
- [x] 14-01-SUMMARY.md exists
- [x] Commit 5e49e9d exists

---
*Phase: 14-expanded-imperfections*
*Completed: 2026-03-16*
