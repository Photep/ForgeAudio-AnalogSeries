---
phase: 14-expanded-imperfections
plan: 02
subsystem: dsp
tags: [drift, dc-offset, component-spread, serialization, ornstein-uhlenbeck, analog-imperfections]

# Dependency graph
requires:
  - phase: 14-expanded-imperfections
    plan: 01
    provides: "Drift parameter reads, phase jitter, pitch slew, OU drift block structure"
provides:
  - "DC offset wander via dedicated slow OU layer (CHAR-02)"
  - "Component spread with deterministic RNG seed and hex-string serialization (CHAR-04)"
  - "dataToJson/dataFromJson serialization methods for module state persistence"
affects: [15-waveform-bleed, 16-swing-shuffle]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Separate OU layer for DC offset (independent from pitch drift OU layers)"
    - "Hex-string serialization for uint64_t seeds (avoids Pitfall 6 sign issues)"
    - "Component spread always active via deterministic offsets from persisted RNG seed"
    - "DC offset applied AFTER crossfade capture (avoids Pitfall 3 click on reset)"

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp

key-decisions:
  - "DC offset OU layer at 0.03Hz (theta=0.188, sigma=0.614) for ~33s wander cycle"
  - "DC offset authority scaled same as pitch drift (2% clocked, 7.5% free) with 0.1x final scale"
  - "Component spread seed stored as hex strings via json_string (not json_integer) to avoid uint64_t sign issues"
  - "Component spread magnitudes: 2% OU weights, 1.5% character/tri-asymmetry, 2% saw curvature, 1% square duty"
  - "DC offset applied AFTER lastOutputVoltage capture to prevent crossfade clicks (Pitfall 3)"

patterns-established:
  - "dataToJson/dataFromJson for module state persistence (first use in this module)"
  - "initComponentSpread() deterministic regeneration from seed (constructor + deserialization)"

requirements-completed: [CHAR-02, CHAR-04]

# Metrics
duration: 3min
completed: 2026-03-16
---

# Phase 14 Plan 02: DC Offset Wander and Component Spread Summary

**DC offset wander via dedicated 0.03Hz OU layer (~50-100mV at full drift) and per-instance component spread with hex-serialized RNG seed for persistent module personality**

## Performance

- **Duration:** 3 min
- **Started:** 2026-03-16T10:31:29Z
- **Completed:** 2026-03-16T10:35:14Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments
- Added DC offset wander OU layer with very slow timescale (0.03Hz, ~33s cycle) providing ~50-100mV output center drift at full Drift setting
- DC offset applied AFTER crossfade capture, preventing clicks on phase reset (Pitfall 3)
- Added component spread system with deterministic parameter offsets from a serialized RNG seed
- Spread offsets applied to triangle asymmetry, saw curvature, square duty cycle, OU layer weights, and character curve response
- Component spread always active (even at Drift=0) -- each module instance has unique personality
- Seed serialization uses hex strings to avoid uint64_t sign issues (Pitfall 6)
- All four CHAR requirements now implemented: pitch slew (01), phase jitter (01), DC offset wander (02), component spread (02)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add DC offset wander OU layer** - `8ba3544` (feat)
2. **Task 2: Add component spread with serialization** - `59c0b5b` (feat)

**Plan metadata:** pending (docs: complete plan)

## Files Created/Modified
- `src/AnalogLFO.cpp` - Added dcOffsetOU member + constructor init, DC offset processing in drift block, DC offset application after crossfade, component spread members + initComponentSpread() method, dataToJson/dataFromJson serialization, spread offsets in computeTriangle/computeSaw/computeSquare/OU loop/character clamp

## Decisions Made
- DC offset OU layer uses 0.03Hz center frequency (theta = 2*pi*0.03 = 0.188) with sigma = 0.614 for stationary std ~1.0 -- produces ~33s wander cycle
- DC offset authority uses same scaling as pitch drift (2% clocked, 7.5% free) with additional 0.1x scale factor for ~50-100mV range
- DC offset applied AFTER `lastOutputVoltage = outputVoltage` capture but BEFORE `outputs[OUTPUT].setVoltage()` -- critical ordering per Pitfall 3 to prevent crossfade clicks when DC offset is nonzero
- Component spread seed stored as hex strings via `json_string()` / `std::stoull()` rather than `json_integer()` to avoid uint64_t -> int64_t sign truncation (Pitfall 6)
- Component spread magnitudes set conservatively: 2% OU weights, 1.5% character response, 1.5% triangle asymmetry, 2% saw curvature, 1% square duty cycle
- No `onReset()` override -- spread seed represents permanent factory tolerance, not resettable state

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- All four CHAR requirements complete -- expanded imperfections phase is done
- Module compiles cleanly with zero errors
- Component spread magnitudes flagged for empirical tuning (per STATE.md blocker) -- current values are conservative starting points
- dataToJson/dataFromJson now exists for this module, available for future state persistence needs

## Self-Check: PASSED

- [x] src/AnalogLFO.cpp exists
- [x] 14-02-SUMMARY.md exists
- [x] Commit 8ba3544 exists
- [x] Commit 59c0b5b exists

---
*Phase: 14-expanded-imperfections*
*Completed: 2026-03-16*
