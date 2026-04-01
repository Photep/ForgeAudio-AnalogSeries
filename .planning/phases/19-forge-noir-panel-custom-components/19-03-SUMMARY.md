---
phase: 19-forge-noir-panel-custom-components
plan: 03
subsystem: ui
tags: [cpp, vcv-rack, widget-structs, svg-knob, svg-port, svg-screw, forge-noir, nanosvg]

# Dependency graph
requires:
  - phase: 19-forge-noir-panel-custom-components
    provides: "11 SVG component files (Plan 01) and 14HP panel SVG (Plan 02)"
provides:
  - "7 custom C++ widget structs: ForgeKnobHero, ForgeKnobSecondary, ForgeKnobUtility, ForgeTrimpot, ForgeJackInput, ForgeJackOutput, ForgeHexBolt"
  - "Rewritten AnalogLFOWidget constructor with all 18 controls at Forge Noir positions"
  - "minRackVersion 2.6.0 in plugin.json for gradient support"
  - "Complete 14HP PANEL-SPEC.md replacing old 12HP specification"
  - "Verified build (make exits 0)"
affects: [19-04-integration, 20-display-layout]

# Tech tracking
tech-stack:
  added: []
  patterns: ["SvgKnob dual-layer pattern: shadow->opacity=0.0 + bg via fb->addChildBelow(bg, tw)", "SvgPort shadow disable pattern for custom jack widgets", "SvgScrew custom replacement via setSvg in constructor"]

key-files:
  created: []
  modified:
    - src/AnalogLFO.cpp
    - plugin.json
    - res/PANEL-SPEC.md

key-decisions:
  - "All knob widgets use -0.83*PI rotation range (~300 degrees) except trimpots at -0.75*PI (~270 degrees) per UI-SPEC interaction contract"
  - "CircularShadow disabled on all 6 custom widget types (knobs + jacks) since SVG components include their own shadow layers"
  - "minRackVersion set to 2.6.0 to enforce gradient rendering support required by component SVGs"

patterns-established:
  - "Custom widget struct pattern: inherit from app::SvgKnob/SvgPort/SvgScrew, disable shadow, load custom SVG in constructor"
  - "Background widget pattern: create widget::SvgWidget*, addChildBelow(bg, tw) on framebuffer, load _bg.svg"
  - "mm2px coordinate system: all positions specified as mm floats, converted via mm2px(Vec(x, y))"

requirements-completed: [PANEL-02, PANEL-03, PANEL-04, PANEL-05]

# Metrics
duration: 4min
completed: 2026-03-30
---

# Phase 19 Plan 03: Custom Widget Structs and C++ Integration Summary

**7 custom Forge Noir widget structs wired into AnalogLFOWidget with all 18 controls at 14HP coordinates, CircularShadow disabled, plugin.json enforcing Rack 2.6.0, and build verified**

## Performance

- **Duration:** ~4 min
- **Started:** 2026-03-30T10:04:45Z
- **Completed:** 2026-03-30T10:09:00Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- Defined 7 custom widget structs (ForgeKnobHero, ForgeKnobSecondary, ForgeKnobUtility, ForgeTrimpot, ForgeJackInput, ForgeJackOutput, ForgeHexBolt) with proper VCV Rack base class inheritance
- Rewrote AnalogLFOWidget constructor: all 18 controls (10 params, 7 inputs, 1 output) positioned at exact Forge Noir coordinates from UI-SPEC
- Eliminated all stock VCV widget types (RoundBigBlackKnob, RoundLargeBlackKnob, RoundBlackKnob, Trimpot, PJ301MPort, ScrewSilver)
- Disabled CircularShadow on all 6 shadow-capable widgets (shadow->opacity = 0.0)
- Added minRackVersion 2.6.0 to plugin.json for 2-stop gradient support
- Replaced PANEL-SPEC.md with complete 14HP Forge Noir specification documenting all positions, colors, nanosvg constraints, and SVG inventory
- Build verified: `make` exits 0 with all custom widget structs and SVG asset paths

## Task Commits

Each task was committed atomically:

1. **Task 1: Define custom widget structs and rewrite AnalogLFOWidget constructor** - `4a756bb` (feat)
2. **Task 2: Update plugin.json, replace PANEL-SPEC.md, and verify build** - `a83dcb0` (chore)

## Files Created/Modified
- `src/AnalogLFO.cpp` - 7 custom widget structs inserted before AnalogLFOWidget; constructor rewritten with custom types and Forge Noir coordinates
- `plugin.json` - Added minRackVersion 2.6.0 for gradient support requirement
- `res/PANEL-SPEC.md` - Complete rewrite from 12HP to 14HP Forge Noir specification with all custom widget types, positions, colors, and nanosvg constraints

## Decisions Made
- All knob widgets use -0.83*PI rotation range (~300 degrees) per UI-SPEC; trimpots use -0.75*PI (~270 degrees) for smaller sweep
- CircularShadow disabled on all custom widgets since the SVG component files include their own shadow layers in the _bg.svg files
- minRackVersion set to 2.6.0 to ensure gradient rendering works (components use 2-stop gradients)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Known Stubs
None - all widget structs are fully wired to SVG assets, all positions are final Forge Noir coordinates, no placeholder values.

## Next Phase Readiness
- All custom widgets wired and building successfully -- ready for visual testing in VCV Rack
- Panel SVG (Plan 02) + component SVGs (Plan 01) + widget structs (this plan) form complete rendering pipeline
- Waveform display repositioned at (3.60, 13.19) size (63.93, 17.98) -- ready for Phase 20 display layout work

## Self-Check: PASSED

- [x] src/AnalogLFO.cpp modified with 7 structs and rewritten constructor
- [x] plugin.json contains minRackVersion 2.6.0
- [x] res/PANEL-SPEC.md contains 14HP Forge Noir spec (no 12HP/60.96mm references)
- [x] Commit 4a756bb exists (Task 1)
- [x] Commit a83dcb0 exists (Task 2)
- [x] Build passes (make exits 0)

---
*Phase: 19-forge-noir-panel-custom-components*
*Completed: 2026-03-30*
