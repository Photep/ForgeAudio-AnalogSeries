---
phase: 19-forge-noir-panel-custom-components
plan: 01
subsystem: ui
tags: [svg, nanosvg, vcv-rack, knob, jack, trimpot, panel-components]

# Dependency graph
requires:
  - phase: 19-forge-noir-panel-custom-components
    provides: "UI spec with component rendering contracts and nanosvg constraints"
provides:
  - "11 nanosvg-compatible SVG component files for Forge Noir panel controls"
  - "3 knob sizes (hero/secondary/utility) with rotating + background layers"
  - "Trimpot with bright metallic body and scalloped edge detail"
  - "Input/output jack pair with ember accent ring on output only"
  - "Hex bolt screw replacement"
affects: [19-03-widget-structs, 19-02-panel-svg, 19-04-integration]

# Tech tracking
tech-stack:
  added: []
  patterns: ["SVG component dual-layer pattern (rotating + static background)", "nanosvg-compatible gradient limitation (max 2 stops)", "mm-based SVG dimensions matching VCV Rack coordinate system"]

key-files:
  created:
    - res/components/ForgeKnobHero.svg
    - res/components/ForgeKnobHero_bg.svg
    - res/components/ForgeKnobSecondary.svg
    - res/components/ForgeKnobSecondary_bg.svg
    - res/components/ForgeKnobUtility.svg
    - res/components/ForgeKnobUtility_bg.svg
    - res/components/ForgeTrimpot.svg
    - res/components/ForgeTrimpot_bg.svg
    - res/components/ForgeJackInput.svg
    - res/components/ForgeJackOutput.svg
    - res/components/ForgeHexBolt.svg
  modified: []

key-decisions:
  - "Scalloped edge on trimpot uses 8 small circles at even angular intervals rather than path-based notches for nanosvg simplicity"
  - "Output jack ember rings placed both behind and on top of flange for visual depth"
  - "Knob machined grooves use alternating white/black strokes at very low opacity (0.02-0.03) for subtle lathe texture"

patterns-established:
  - "Dual-layer SVG component: rotating layer (*_name.svg) + static background (*_name_bg.svg) for VCV Rack SvgKnob/SvgPort"
  - "All gradients limited to 2 stops per gradient definition for nanosvg compatibility"
  - "No text/style/use/filter/clipPath/image elements in any SVG for nanosvg compatibility"
  - "Indicator line at 12 o'clock position with linearGradient fill (SvgKnob handles rotation)"

requirements-completed: [PANEL-03, PANEL-04, PANEL-05]

# Metrics
duration: 8min
completed: 2026-03-30
---

# Phase 19 Plan 01: SVG Component Files Summary

**11 nanosvg-compatible SVG component skins for Forge Noir panel: 3 knob sizes with dark metallic gradients, bright trimpot, ember-accented output jack, and hex bolt screws**

## Performance

- **Duration:** 8 min
- **Started:** 2026-03-30T07:43:00Z
- **Completed:** 2026-03-30T07:51:00Z
- **Tasks:** 2
- **Files created:** 11

## Accomplishments
- Created 3 knob sizes (hero 16.38mm, secondary 11.99mm, utility 9.19mm) each with rotating body and static background layers
- Created trimpot with bright metallic gradient (#6a6a6a) visually distinct from dark knob gradients (#4a4a4a)
- Created output jack with ember accent ring (#e85d26) while input jack has no ember -- clear visual hierarchy
- All 11 SVG files are fully nanosvg-compatible: max 2-stop gradients, no forbidden elements, mm-based dimensions

## Task Commits

Each task was committed atomically:

1. **Task 1: Create knob SVG files (3 sizes x 2 layers)** - `ac49213` (feat)
2. **Task 2: Create trimpot, jack, and hex bolt SVG files** - `ada911c` (feat)

## Files Created/Modified
- `res/components/ForgeKnobHero.svg` - 16.38mm hero knob rotating layer with body gradient, center cap, grooves, ember indicator
- `res/components/ForgeKnobHero_bg.svg` - 18.00mm hero knob static background with drop shadow and 4-ring metallic surround
- `res/components/ForgeKnobSecondary.svg` - 11.99mm secondary knob rotating layer (CHARACTER/DRIFT)
- `res/components/ForgeKnobSecondary_bg.svg` - 13.50mm secondary knob static background
- `res/components/ForgeKnobUtility.svg` - 9.19mm utility knob rotating layer (RATE/PHASE)
- `res/components/ForgeKnobUtility_bg.svg` - 10.50mm utility knob static background
- `res/components/ForgeTrimpot.svg` - 3.60mm bright metallic trimpot with scalloped edge and indicator slot
- `res/components/ForgeTrimpot_bg.svg` - 4.20mm trimpot shadow ring
- `res/components/ForgeJackInput.svg` - 7.19mm input jack with metallic rings and specular highlight
- `res/components/ForgeJackOutput.svg` - 8.39mm output jack with ember accent ring and outer glow
- `res/components/ForgeHexBolt.svg` - 3.20mm hexagonal bolt with gradient face and inner socket

## Decisions Made
- Scalloped edge on trimpot implemented as 8 small circles at 45-degree intervals around the perimeter (simpler and more nanosvg-reliable than path-based notches)
- Output jack ember accent rings rendered both behind the flange (for glow spillover) and on top (for visible ring) to create depth
- Knob machined groove texture uses alternating white/black concentric circle strokes at very low opacity (0.02-0.03) for subtle lathe effect

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Known Stubs
None - all SVG files are complete with final visual content.

## Next Phase Readiness
- All 11 component SVG files ready for C++ widget struct loading in Plan 03
- Panel SVG (Plan 02) can reference these component dimensions for layout accuracy
- Dimensions match UI-SPEC component inventory exactly

## Self-Check: PASSED

- All 11 SVG files exist in res/components/
- Commit ac49213 (Task 1) verified
- Commit ada911c (Task 2) verified
- 19-01-SUMMARY.md exists

---
*Phase: 19-forge-noir-panel-custom-components*
*Completed: 2026-03-30*
