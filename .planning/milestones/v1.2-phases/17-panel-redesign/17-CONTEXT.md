# Phase 17: Panel Redesign - Context

**Gathered:** 2026-03-17
**Status:** Ready for planning

<domain>
## Phase Boundary

Update the 12HP panel SVG to accommodate all v1.2 components (FM jack, RESET jack, Phase Offset knob/CV, FM attenuator, Phase Offset attenuator) in a clean, cohesive layout. All new components currently sit at temporary positions from Phases 12-13 and need final placement. Swing is right-click menu only (decided Phase 16) — no panel control needed. Panel must stay 12HP per out-of-scope constraint. Requirements: PANEL-01, PANEL-02.

</domain>

<decisions>
## Implementation Decisions

### Layout zones
- Keep existing 3-zone stacking: knobs (y~54-69), mid-row controls (y~86), bottom section (trimpots y~96, jacks y~108)
- Redistribute components within existing zones to balance density — no new zone needed
- 4-column bottom section: Morph (x~10), Character (x~24), Drift (x~38), Output (x~52) as anchors
- FM trimpot+jack and Phase Offset trimpot+jack integrated into the 4-column structure
- All jack/control spacing must meet 7mm minimum center-to-center

### Mid-row organization
- RESET jack adjacent to CLK jack on the right side — timing inputs grouped as a pair
- Rate knob and Phase Offset knob as a knob pair on the left/center
- Visual separation between the knob pair and timing input pair (spacing, not divider)

### Control grouping
- Trimpot-above-jack convention for ALL CV pairs (existing Morph/Character/Drift + new FM and Phase Offset CV)
- Amber section divider line stays at y=92 separating knob zone from CV/jack zone
- Amber accent ring on Output jack only — no accent rings on other jacks
- No category colors or additional visual distinction for input types

### Labels
- Phase Offset knob: "PHASE" — white-gray (#c0c0d0), knob label convention
- RESET jack: "RST" — muted lavender (#8888aa), jack label convention, matches CLK's 3-letter style
- FM pair: "FM" — muted lavender (#8888aa), jack/input label convention
- Phase Offset CV pair: "PHASE CV" — muted lavender (#8888aa), follows '[param] CV' pattern
- All existing labels unchanged (MORPH, CHARACTER, DRIFT, RATE, CLK, MORPH CV, CHAR CV, DRIFT CV, OUT)
- All labels rendered as geometric path letterforms (no <text> elements) — matches existing panel

### SVG structure
- Elements grouped by panel zone in named `<g>` groups with id attributes: brand, display, knobs, mid-row, bottom-section, footer
- Hidden components layer updated with all 19 components using RGB color coding: red=params (knobs/trimpots), green=inputs (jacks), blue=outputs
- Connection lines (thin amber) for ALL trimpot-jack pairs including new FM and Phase Offset CV pairs
- nanosvg constraints documented in SVG header comment block: no `<text>`, no `<use>`/`<defs>`, no `<style>`, no filters, no complex gradients
- Component positions in hidden layer MUST match mm2px coordinates in AnalogLFO.cpp widget code

### Claude's Discretion
- Exact x/y positions for all 19 components within the 3-zone, 4-column structure (7mm minimum spacing constraint)
- Whether FM and Phase Offset CV need a third bottom row at y~118 or fit within existing rows
- Label scale factors and exact positioning to fit without overlap
- Connection line routing and label gap dimensions
- Whether RATE label needs repositioning given new PHASE label nearby
- Exact grouping boundaries for SVG `<g>` elements

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements
- `.planning/REQUIREMENTS.md` — PANEL-01 (panel SVG with all jacks/controls), PANEL-02 (FM jack, RESET jack, phase offset knob/CV placement)

### Architecture context
- `.planning/PROJECT.md` — Brand identity colors, 12HP constraint, two-row bottom layout decision, SVG constraints (nanosvg compatible)
- `res/AnalogLFO.svg` — Current panel SVG to be redesigned (source of truth for existing layout)
- `src/AnalogLFO.cpp` lines 1301-1340 — Widget code with mm2px positions that SVG must match

### Prior phase decisions
- `.planning/STATE.md` — Phase 12 temporary positions (RESET, Phase Offset), Phase 13 temporary positions (FM), Phase 16 swing as right-click menu

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `res/AnalogLFO.svg`: Complete 12HP panel with geometric path letterforms for all text — extend with new labels using same path-based approach
- Widget code (AnalogLFO.cpp:1301-1340): All 19 component positions defined via `mm2px(Vec(x, y))` — SVG positions must match these exactly
- Existing label path data (SVG): Full alphabet of geometric letterforms at various scales — reuse for new labels (PHASE, RST, FM, PHASE CV)
- Hidden components layer: Documentation overlay with RGB-coded circles per component

### Established Patterns
- Knob labels: scale ~0.35-0.4, #c0c0d0, centered above knob
- Jack labels: scale ~0.28-0.35, #8888aa, centered above/below jack depending on zone
- Output label: #e8a838 amber, matching accent ring
- Connection lines: #e8a838 at 0.2 stroke-width, 0.4 opacity, broken around labels
- Section divider: #e8a838 at 0.15 stroke-width, 0.5 opacity, spanning ~x=8 to x=53

### Integration Points
- Widget constructor (AnalogLFO.cpp:1298): `setPanel()` loads SVG — panel filename unchanged
- mm2px coordinate system: SVG units in mm, widget uses mm2px() for pixel conversion — 1:1 correspondence
- Component types: RoundBigBlackKnob (Morph), RoundLargeBlackKnob (Char/Drift), RoundBlackKnob (Rate/Phase), Trimpot (attenuators), PJ301MPort (all jacks)
- appendContextMenu (AnalogLFO.cpp:1348): Swing menu already added — no panel widget changes needed for swing

</code_context>

<specifics>
## Specific Ideas

No specific requirements — open to standard approaches within the constraints above.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 17-panel-redesign*
*Context gathered: 2026-03-17*
