---
phase: 15-waveform-bleed
plan: 01
subsystem: dsp
tags: [waveform, morph, bleed, crosstalk, analog-character, component-spread]

# Dependency graph
requires:
  - phase: 14-expanded-imperfections
    provides: "computeMorphedWave() pipeline, initComponentSpread(), OU layers, component spread members"
provides:
  - "Waveform bleed (neighbor-shape crosstalk) in computeMorphedWave() gated by Character knob"
  - "bleedSpread member variable for per-instance bleed magnitude variation"
  - "OU layer 0 modulation of bleed intensity for slow temporal variation"
affects: [17-panel-redesign]

# Tech tracking
tech-stack:
  added: []
  patterns: [neighbor-ring-indexing, proximity-weighted-bleed, normalization-for-amplitude-safety]

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "Array indexing replaces switch/case in computeMorphedWave() for indexed neighbor access while producing identical primary crossfade results"
  - "Wrapping ring topology (sine-tri-saw-sqr-sine) for neighbor identification via modular arithmetic"
  - "Normalization divisor (1 + bleedIntensity) guarantees output stays in [-1,1] regardless of bleed magnitude"
  - "bleedSpread appended as last RNG call in initComponentSpread() to preserve existing spread values"

patterns-established:
  - "Neighbor bleed via proximity-weighted ring indexing: leftWeight = 1-frac, rightWeight = frac"
  - "Amplitude-safe additive mixing via normalization divisor pattern"

requirements-completed: [CHAR-05]

# Metrics
duration: ~5min
completed: 2026-03-17
---

# Phase 15 Plan 01: Waveform Bleed Summary

**Adjacent-shape crosstalk in computeMorphedWave() via proximity-weighted neighbor bleed, gated by Character knob with per-instance component spread and OU-layer temporal modulation**

## Performance

- **Duration:** ~5 min (implementation) + human verification session
- **Started:** 2026-03-17T07:20:00Z (approx)
- **Completed:** 2026-03-17T07:30:00Z (approx)
- **Tasks:** 2 (1 auto + 1 human-verify)
- **Files modified:** 1

## Accomplishments
- Waveform bleed adds capacitive-coupling-style crosstalk from neighboring shapes during morph transitions when Character > 0
- At Character=0, morph remains a crisp crossfade identical to v1.1 (no behavioral regression)
- Output amplitude guaranteed within +/-5V via normalization divisor
- Per-instance variation via bleedSpread member (component spread pattern)
- Slow temporal variation via existing OU layer 0 modulation (+/-20% fluctuation)

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement waveform bleed in computeMorphedWave()** - `50fd1b6` (feat)
2. **Task 2: Verify waveform bleed in VCV Rack** - user-approved checkpoint (no code commit)

**Plan metadata:** (see final docs commit)

## Files Created/Modified
- `src/AnalogLFO.cpp` - Added bleedSpread member, bleedSpread generation in initComponentSpread(), replaced switch/case with array-indexed neighbor bleed in computeMorphedWave()

## Decisions Made
- Array indexing replaces switch/case in computeMorphedWave() -- enables indexed neighbor access for bleed while producing identical primary crossfade results
- Wrapping ring topology (sine-tri-saw-sqr-sine) via modular arithmetic for neighbor identification
- Normalization divisor `result /= (1 + bleedIntensity)` guarantees output stays in [-1,1] regardless of bleed magnitude
- bleedSpread appended as the last RNG call in initComponentSpread() to preserve all existing spread values (no behavioral regression for prior component spread)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 15 complete, CHAR-05 requirement satisfied
- Ready for Phase 16 (Swing and Shuffle) -- no dependencies on waveform bleed
- Phase 17 (Panel Redesign) can proceed after Phase 16; no new jacks or controls were added in this phase

## Self-Check: PASSED

- FOUND: src/AnalogLFO.cpp
- FOUND: commit 50fd1b6

---
*Phase: 15-waveform-bleed*
*Completed: 2026-03-17*
