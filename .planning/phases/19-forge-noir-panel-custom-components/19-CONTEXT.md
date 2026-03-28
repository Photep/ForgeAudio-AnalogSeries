# Phase 19: Forge Noir Panel + Custom Components - Context

**Gathered:** 2026-03-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Replace the 12HP panel with a 14HP Forge Noir SVG panel featuring custom machined-metal knobs (3 sizes), scalloped trimpots, accent-ring jacks (2 sizes), forge emblem background, forge rune glyph, and path-rendered brand typography. All controls repositioned per the Forge Noir mockup layout. This phase covers the static panel and component SVGs plus C++ widget code updates -- display internals (three-column layout, CRT scanlines, border glow) are Phase 20, and animated SYNC badge is Phase 21.

</domain>

<decisions>
## Implementation Decisions

### Rendering Strategy
- **D-01:** SVG-first approach with minimal NanoVG. All panel elements, components, and atmospheric effects rendered as nanosvg-compatible SVG. NanoVG used only for the waveform display (existing). No NanoVG overlays for knobs, emblem, or glows.
- **D-02:** Effects that nanosvg can't render (box-shadows, blur filters, conic gradients, CSS opacity on groups) are approximated using nanosvg-compatible elements: semi-transparent shapes for shadows, concentric rings for glow, radial gradients for depth, per-element opacity.
- **D-03:** 2-stop linear and radial gradients ARE supported (Rack 2.6.0+) and should be used extensively for metallic curvature, accent bars, and panel warmth.

### Visual Fidelity Priorities
- **D-04:** ALL of the following are must-haves in the SVG approximation: accent bar gradients, panel texture/warmth (radial gradient behind knobs), knob depth/metallic ring (concentric circles with gradients), section divider gradients.
- **D-05:** Knurled texture on MORPH hero knob: SKIP. Use machined concentric ring grooves instead. Knurling (conic gradient) is not achievable in nanosvg and is subtle at module scale.

### Custom Components
- **D-06:** Custom SVG skin files for all components. Subclass SvgKnob (or equivalent VCV widget base) and point to custom SVG files. Standard VCV approach -- no NanoVG-rendered widgets.
- **D-07:** Three knob SVG skins: hero (MORPH, 82px at 5x), secondary (CHARACTER/DRIFT, 60px at 5x), utility (RATE/PHASE, 46px at 5x). Each with radial gradients for curvature, concentric circles for metallic ring, center cap indent, and machined groove texture approximation.
- **D-08:** Knob indicator line: Claude's discretion -- solid ember line or ember line with semi-transparent opacity halo, whatever looks best within SVG constraints.
- **D-09:** Two jack SVG skins: small input (36px at 5x) and large output (42px at 5x). Output jack SVG includes the ember accent ring baked in (semi-transparent circle stroke). Not a separate panel element.
- **D-10:** Trimpot SVG skin: bright metallic body (contrasting with dark knobs), directional indicator slot, scalloped edge approximated with nanosvg-compatible elements.

### Forge Emblem
- **D-11:** Simplified SVG shapes for the forge emblem. Low-opacity filled paths for molten streams, small circles for ember particles, radial gradients for central glow. No blur -- soften edges using wider, lower-opacity strokes. Still atmospheric, just crisper than the HTML mockup.
- **D-12:** nanosvg doesn't support `<use>`/`<defs>` for mirroring. Emblem symmetry guaranteed by Claude's discretion -- either manually duplicated left/right elements or a build-time template script that generates flat mirrored SVG.

### Forge Rune Glyph
- **D-13:** Rune glyph rendered as simplified nanosvg-compatible SVG. Claude's discretion on detail level -- may include concentric power rings at decreasing opacity (no blur), or simplify to diamond + center dot. The rune shape is strong enough as a brand mark without the Gaussian blur glow.

### Canonical Layout Reference
- **D-14:** `forge-noir.html` is the canonical layout reference (not PNG screenshots). All component positions derived from this file's pixel coordinates, converted to mm using the 5x scale factor (71.12mm / 356px = 0.1998 mm/px).
- **D-15:** Positions in the HTML are FINAL. No further layout adjustments needed before implementation.

### Panel Spec
- **D-16:** PANEL-SPEC.md (`res/PANEL-SPEC.md`) will be completely replaced with Forge Noir specifications -- new dimensions (14HP), new color palette, new component positions, new nanosvg constraints documentation. Old 12HP spec becomes irrelevant.

### Typography
- **D-17:** All text rendered as SVG `<path>` elements (nanosvg requirement). Brand text uses FoundationLogo font glyphs converted to paths, module name uses Bebas Neue, labels use Chakra Petch/JetBrains Mono -- all converted to SVG path data.

### Claude's Discretion
- Knob indicator glow approach (solid vs opacity halo) -- D-08
- Forge rune detail level (full rings vs simplified) -- D-13
- Emblem symmetry method (manual duplication vs build script) -- D-12
- SVG file organization (one panel SVG + separate component SVGs, naming conventions)
- Conversion methodology for font glyphs to SVG paths
- Machined groove texture approximation on knob bodies

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Design Language
- `DESIGN-LANGUAGE.md` -- Complete Forge Noir design language: colors, typography, component rendering specs, emblem construction, layout rules, symmetry constraints
- `forge-noir.html` -- Canonical layout reference with exact pixel positions for all components. FINAL positions, no adjustments needed. Convert to mm using 71.12/356 factor.

### Panel Constraints
- `res/PANEL-SPEC.md` -- Current 12HP panel spec (will be REPLACED by Phase 19). Read for nanosvg compatibility constraints in Section 6 and SVG component layer convention in Section 5 -- these patterns carry forward.

### Requirements
- `.planning/REQUIREMENTS.md` -- PANEL-01 through PANEL-07 define the panel requirements
- `.planning/PROJECT.md` -- Key Decisions table, brand identity section, nanosvg constraints

### Prior Phase Context
- `.planning/phases/18-pwm-dsp-extension/18-CONTEXT.md` -- Phase 18 decisions (PWM DSP). Morph sweep now has 5 shapes -- relevant for any morph arc markings on the panel.

### VCV Rack Widget System
- `src/AnalogLFO.cpp` lines 1339-1403 -- Current AnalogLFOWidget class with stock VCV components. This is the code that gets rewritten with custom widget classes and new positions.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `AnalogLFOWidget` class (src/AnalogLFO.cpp:1339): Current widget setup using `setPanel()`, `createParamCentered<>`, `createInputCentered<>`, `createOutputCentered<>` -- same pattern used with custom widget classes
- `WaveformDisplay` widget (src/AnalogLFO.cpp:846): NanoVG-rendered display widget -- stays as-is, just repositioned to new panel coordinates
- `res/AnalogLFO.svg`: Current 12HP panel SVG -- will be completely replaced with 14HP Forge Noir SVG

### Established Patterns
- VCV Rack uses `SvgKnob`, `SvgPort`, `SvgScrew` base classes that load SVG skins
- Component positions use `mm2px(Vec(x, y))` conversion in C++ code
- Panel loaded via `setPanel(createPanel(asset::plugin(pluginInstance, "res/AnalogLFO.svg")))`
- Hidden `<g id="components">` layer in SVG for designer handoff (color-coded circles)

### Integration Points
- `plugin.json`: Must update `"minRackVersion"` if using gradient features from Rack 2.6.0
- `AnalogLFOWidget` constructor: All `addParam`/`addInput`/`addOutput` calls need new coordinates
- `res/` directory: New SVG files for panel and each custom component
- Module width: `box.size` changes from 12HP to 14HP -- affects screw positions and any width-dependent layout

</code_context>

<specifics>
## Specific Ideas

- The HTML mockup includes a dashed morph arc (104px diameter at 5x) around the MORPH knob -- decorative element showing the morph sweep range
- Header slashes: 3 diagonal slash marks flanking each side of the brand text, angled outward
- Bottom brand "FORGE" rendered at reduced opacity (0.25) as a subtle footer element
- CV connecting lines: thin ember gradient lines visually linking each trimpot to its CV jack below
- The forge emblem SVG in the HTML defines a left half and mirrors it -- the flat SVG output must have both halves explicitly drawn

</specifics>

<deferred>
## Deferred Ideas

None -- discussion stayed within phase scope.

</deferred>

---

*Phase: 19-forge-noir-panel-custom-components*
*Context gathered: 2026-03-28*
