---
created: 2026-03-15T01:53:38.895Z
title: Add missing panel labels and finalize control positions
area: ui
files:
  - res/AnalogLFO.svg
  - src/AnalogLFO.cpp:1094-1097
---

## Problem

Several controls added in Phases 12-01 and 12-02 are missing SVG panel labels, and the Phase Offset controls are in temporary positions:

1. **PHASE label** — needs SVG path-based label matching the style of MORPH/CHARACTER/DRIFT/RATE (hand-drawn paths in `#c0c0d0`, not programmatic text)
2. **RESET jack** — no label at all (added in Phase 12-01 at x=52, y=86)
3. **Phase Offset CV jack** — no label, positioned at y=118 below the "FORGE AUDIO" footer text, visually disconnected from the other Phase Offset controls
4. **Phase Offset attenuverter** — no label (at x=52, y=96)

## Solution

Address in Phase 17 (final layout pass):
- Add SVG path labels for PHASE, RESET, and PHASE CV to `AnalogLFO.svg`
- Reposition Phase Offset knob, attenuverter, and CV jack into a coherent column or row
- Ensure all controls have consistent label sizing and spacing
