---
phase: 02-waveform-engine
plan: 02
subsystem: dsp
tags: [vcv-rack, lfo, waveform-morphing, phase-accumulator, dsp, bipolar-output]

# Dependency graph
requires:
  - phase: 02-waveform-engine
    plan: 01
    provides: "Linear rate param, MORPH_ATTEN_PARAM, double phase member, cmath include"
provides:
  - "computeMorphedWave() function with 4-shape morph interpolation"
  - "process() callback with double-precision phase accumulator"
  - "Morph CV processing with attenuator and hard clamp"
  - "Bipolar +/-5V waveform output"
  - "Working LFO module producing real waveform output"
affects: [03-waveform-display, 05-analog-character, 06-drift-engine]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Falling saw (ramp down) for morph-compatible crossfade alignment"
    - "Equal quarter morph distribution: sine(0-25%) tri(25-50%) saw(50-75%) sqr(75-100%)"
    - "Double-precision phase accumulator for sub-Hz stability"

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "Flipped saw from rising to falling ramp to eliminate amplitude dip in saw-to-square morph crossfade"
  - "Accepted tri-to-saw peak asymmetry as inherent to linear crossfading (not a defect)"

patterns-established:
  - "Inline DSP in module struct (extract only when second consumer exists)"
  - "computeMorphedWave(phase, morph) as single waveform generation entry point"
  - "CV processing pattern: knob + attenuator * voltage / 10V with hard clamp"

requirements-completed: [WAVE-01, WAVE-02, OUT-01]

# Metrics
duration: ~15min
completed: 2026-02-25
---

# Phase 02 Plan 02: Waveform Engine DSP Summary

**Four-shape morph oscillator with double-precision phase accumulator, falling-saw convention for crossfade alignment, Morph CV with attenuator, and bipolar +/-5V output**

## Performance

- **Duration:** ~15 min (includes checkpoint verification in VCV Rack)
- **Started:** 2026-02-25T07:52:00Z
- **Completed:** 2026-02-25T09:30:16Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments
- Implemented complete DSP engine: computeMorphedWave() generates sine, triangle, falling saw, and square with continuous morph interpolation across equal quarter segments
- Double-precision phase accumulator prevents stall at minimum rate (0.01Hz verified stable over 10+ seconds)
- Morph CV input with attenuator Trimpot provides external morph modulation (10V = full range, attenuator at 0 blocks CV)
- Verified all waveform shapes, morph sweep continuity, rate range, CV modulation, output voltage, and low-rate stability in VCV Rack

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement the morph waveform engine and process() callback** - `b651a09` (feat)
2. **Post-checkpoint fix: Flip saw to falling ramp** - `e0486b3` (fix)

## Files Created/Modified
- `src/AnalogLFO.cpp` - computeMorphedWave() function with 4-shape morph interpolation, process() callback with phase accumulator, rate control, CV processing, and bipolar output

## Decisions Made
- **Flipped saw from rising to falling ramp:** Rising saw and square were phase-opposed during linear crossfade at morph ~0.58, causing ~50% amplitude loss. Falling saw aligns energy with both triangle (before) and square (after), matching classic analog LFO convention (Minimoog, SH-101, Juno).
- **Accepted tri-to-saw peak asymmetry:** Linear crossfade between triangle and saw produces peak asymmetry at intermediate morph positions. This is inherent to the crossfade method and was accepted as non-defective behavior.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Saw waveform direction flipped from rising to falling**
- **Found during:** Task 2 (human verification in VCV Rack)
- **Issue:** Rising saw was phase-opposed to square wave, causing ~50% amplitude dip during saw-to-square morph crossfade at ~0.58 position
- **Fix:** Changed saw formula from `2.f * phase - 1.f` (rising) to `1.f - 2.f * phase` (falling) to align energy with adjacent shapes
- **Files modified:** src/AnalogLFO.cpp
- **Verification:** VCV Rack scope confirmed smooth amplitude across full morph sweep
- **Committed in:** `e0486b3`

---

**Total deviations:** 1 auto-fixed (1 bug fix)
**Impact on plan:** Essential fix for correct morph behavior. No scope creep. Matches analog LFO convention.

## Issues Encountered
- Tri-to-saw crossfade shows peak asymmetry at intermediate morph positions -- investigated and determined to be inherent to linear crossfading between shapes with different peak positions. Accepted as expected behavior.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Core DSP engine complete and verified -- module produces real waveform output
- computeMorphedWave() ready for Phase 3 display (can be called with any phase/morph to render waveform trace)
- Phase 5 (Analog Character) will modify the four base waveforms inside computeMorphedWave()
- Phase 6 (Drift) will modulate the phase accumulator and rate
- All waveform shapes verified working in VCV Rack with scope

## Self-Check: PASSED

All files exist, all commits verified.

---
*Phase: 02-waveform-engine*
*Completed: 2026-02-25*
