# Forge Audio Analog LFO - Panel Specification (Forge Noir)

Designer handoff document for the 14HP Forge Noir panel SVG.

This document captures every coordinate, size, color, and constraint needed to recreate or refine the panel without reading C++ code. The component positions in this spec, the SVG components layer, and the C++ widget code MUST stay synchronized. If a designer moves a component, the C++ code must be updated to match.

---

## 1. Panel Dimensions

| Property | Value |
|----------|-------|
| Width | 71.12mm (14HP x 5.08mm/HP) |
| Height | 128.5mm (standard 3U Eurorack) |
| Panel center X | 35.56mm |
| HP count | 14 |
| Screw clear zone | ~5mm diameter at each corner |

The SVG must declare `width="71.12mm"`, `height="128.5mm"`, and `viewBox="0 0 71.12 128.5"`.

---

## 2. Color Palette

### Panel Palette (60/30/10 Split)

| Role | Hex | Usage |
|------|-----|-------|
| Dominant (60%) | `#0c0c0c` | Panel background -- near-black matte |
| Secondary (30%) | `#1a1a1a` | Panel border, knob metallic ring dark band, hex bolt outer, section divider endpoints |
| Accent (10%) | `#e85d26` (Forge Ember) | Brand text, knob indicator lines, output jack accent ring, accent bars, CV connecting lines, header slashes, output label, decorative line |

### Extended Palette

| Name | Hex | Usage |
|------|-----|-------|
| Panel Black | `#0c0c0c` | Panel background fill |
| Panel Border | `#1a1a1a` | Subtle panel edge, dark metallic bands |
| Forge Ember | `#e85d26` | Primary accent -- brand, indicators, accents, glows |
| Molten Gold | `#daa520` | Secondary accent -- rune power ring outermost, display readout warm details |
| Hot Gold | `#f0a030` | Tertiary -- rune center mid-layer, highlight moments |
| White Hot | `#ffe0a0` | Rune core peak brightness |
| Warm White | `#e8e4e0` | Module name |
| Hero Label | `#f0ece8` | MORPH label (warmer than warm white) |
| Label Light | `#c0bbb5` | Primary control labels (CHARACTER, DRIFT) |
| Label Mid | `#888888` | Secondary control labels (RATE, PHASE) |
| Label Dim | `#777777` | CV labels |
| Label I/O | `#999999` | CLK, RST labels |
| Knob Highlight | `#4a4a4a` | Knob body gradient bright stop |
| Knob Dark | `#0a0a0a` | Knob body gradient dark stop |
| Metallic Ring Bright | `#777777` | Knob outer metallic ring peak |
| Metallic Ring Mid | `#555555` | Knob outer metallic ring transition |
| Trimpot Bright | `#6a6a6a` | Trimpot body bright gradient stop |
| Jack Inner Ring | `#cccccc` | Jack concentric ring bright peak |
| Display BG | `#030303` | Display background (reserved for Phase 20) |

### Accent Reserved-For List

Forge Ember `#e85d26` is used ONLY for:
1. Brand text ("FORGE", "AUDIO")
2. Knob indicator lines (all 3 sizes)
3. Output jack accent ring (semi-transparent stroke)
4. Top and bottom accent bar gradients
5. CV connecting lines (gradient, fading downward)
6. Header slash marks (gradient, fading downward)
7. Output label text
8. Decorative line behind module name
9. Emblem glow elements (low opacity: 0.02-0.18)
10. Rune glyph outer diamond and power rings (low opacity)
11. Morph arc (dashed decorative ring around MORPH knob)
12. Footer "FORGE" text (opacity 0.25)

---

## 3. Layout Zones

| Zone | Y Range (mm) | Height (mm) | Content |
|------|-------------|-------------|---------|
| Header + accent bar | 0 - 8 | 8 | Ember accent bar (0.60mm), brand text (FORGE / rune / AUDIO), header slashes |
| Module name | 8 - 12 | 4 | "ANALOG LFO" title, decorative line behind |
| Waveform display | 13 - 31 | 18 | Dark display area for waveform visualization |
| MORPH zone | 32 - 55 | 23 | "MORPH" label, hero knob at (35.56, 47.35), morph arc |
| CHARACTER/DRIFT zone | 55 - 75 | 20 | Primary labels, secondary knobs at (21.18/49.94, 67.32) |
| RATE/PHASE zone | 75 - 90 | 15 | Secondary labels, utility knobs at (21.18/49.94, 83.51) |
| CV trimpot row | 90 - 98 | 8 | 5 trimpots at y=95.89mm, CV labels above |
| CV jack row | 98 - 107 | 9 | 5 CV input jacks at y=103.08mm, CV connecting lines |
| Bottom I/O | 107 - 123 | 16 | CLK, RST inputs + OUTPUT jack at y=117.47mm, I/O labels |
| Footer + accent bar | 123 - 128.5 | 5.5 | Footer "FORGE" text, ember accent bar (0.60mm) |

---

## 4. Component Position Map

All coordinates are center positions in millimeters. Widget classes are custom Forge Noir types.

### Params (Knobs + Trimpots)

| Control | Widget Class | Center X (mm) | Center Y (mm) | Size (mm) | Type |
|---------|-------------|---------------|---------------|-----------|------|
| MORPH_PARAM | ForgeKnobHero | 35.56 | 47.35 | 16.38 | Param |
| CHARACTER_PARAM | ForgeKnobSecondary | 21.18 | 67.32 | 11.99 | Param |
| DRIFT_PARAM | ForgeKnobSecondary | 49.94 | 67.32 | 11.99 | Param |
| RATE_PARAM | ForgeKnobUtility | 21.18 | 83.51 | 9.19 | Param |
| PHASE_OFFSET_PARAM | ForgeKnobUtility | 49.94 | 83.51 | 9.19 | Param |
| MORPH_ATTEN_PARAM | ForgeTrimpot | 9.19 | 95.89 | 3.60 | Param |
| CHARACTER_ATTEN_PARAM | ForgeTrimpot | 22.77 | 95.89 | 3.60 | Param |
| DRIFT_ATTEN_PARAM | ForgeTrimpot | 35.56 | 95.89 | 3.60 | Param |
| FM_ATTEN_PARAM | ForgeTrimpot | 48.75 | 95.89 | 3.60 | Param |
| PHASE_OFFSET_ATTEN_PARAM | ForgeTrimpot | 61.93 | 95.89 | 3.60 | Param |

### Inputs

| Control | Widget Class | Center X (mm) | Center Y (mm) | Size (mm) | Type |
|---------|-------------|---------------|---------------|-----------|------|
| MORPH_CV_INPUT | ForgeJackInput | 9.19 | 103.08 | 7.19 | Input |
| CHARACTER_CV_INPUT | ForgeJackInput | 22.77 | 103.08 | 7.19 | Input |
| DRIFT_CV_INPUT | ForgeJackInput | 35.56 | 103.08 | 7.19 | Input |
| FM_INPUT | ForgeJackInput | 48.75 | 103.08 | 7.19 | Input |
| PHASE_OFFSET_CV_INPUT | ForgeJackInput | 61.93 | 103.08 | 7.19 | Input |
| CLK_INPUT | ForgeJackInput | 14.38 | 117.47 | 7.19 | Input |
| RESET_INPUT | ForgeJackInput | 35.56 | 117.47 | 7.19 | Input |

### Outputs

| Control | Widget Class | Center X (mm) | Center Y (mm) | Size (mm) | Type |
|---------|-------------|---------------|---------------|-----------|------|
| OUTPUT | ForgeJackOutput | 56.74 | 117.47 | 8.39 | Output |

### Waveform Display

| Element | Position (mm) | Size (mm) |
|---------|--------------|-----------|
| WaveformDisplay | pos (3.60, 13.19) | size (63.93, 17.98) |

### Screws (Hex Bolts)

| Position | Coordinate |
|----------|------------|
| Top-left | (RACK_GRID_WIDTH, 0) |
| Top-right | (box.size.x - 2*RACK_GRID_WIDTH, 0) |
| Bottom-left | (RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH) |
| Bottom-right | (box.size.x - 2*RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH) |

**Widget size hierarchy:**
- ForgeKnobHero: 16.38mm diameter (primary control -- MORPH)
- ForgeKnobSecondary: 11.99mm diameter (secondary controls -- CHARACTER, DRIFT)
- ForgeKnobUtility: 9.19mm diameter (utility controls -- RATE, PHASE)
- ForgeTrimpot: 3.60mm diameter (CV attenuverters)
- ForgeJackInput: 7.19mm diameter (standard input)
- ForgeJackOutput: 8.39mm diameter (output with ember accent ring)
- ForgeHexBolt: 3.20mm (hexagonal screw)

**CV section layout (two-row convention):** 5 trimpots at y=95.89mm (upper row), 5 jacks at y=103.08mm (lower row). Five columns at X = 9.19, 22.77, 35.56, 48.75, 61.93mm give ~13.58mm center-to-center spacing. Each trimpot sits directly above its associated CV jack at the same X coordinate. Ember connecting lines (0.20mm, gradient opacity) visually link each trimpot to its jack.

**Bottom I/O layout:** CLK at x=14.38mm, RST at x=35.56mm (center axis), OUTPUT at x=56.74mm. Symmetrical distribution with center emphasis.

---

## 5. nanosvg Compatibility Constraints

VCV Rack uses nanosvg to render panel SVGs. This imposes strict limitations on what SVG features can be used.

**NOT supported (will be ignored or cause errors):**
- `<text>` elements -- convert all text to `<path>` elements
- CSS `<style>` blocks -- use inline attributes (`fill`, `stroke`, etc.)
- `<use>` and `<defs>` references (except `<defs>` for gradient definitions)
- `<clipPath>` clipping
- `<filter>` effects (blur, shadow, etc.)
- `<image>` embedded images
- `opacity` on `<g>` groups -- set opacity on individual elements instead
- 3-digit hex colors -- always use 6-digit hex (e.g., `#0c0c0c`, not `#0c0`)

**Supported elements:**
- `<rect>`, `<circle>`, `<ellipse>`, `<line>`, `<polyline>`, `<polygon>`, `<path>`, `<g>`

**Supported attributes:**
- `fill`, `stroke`, `stroke-width`, `opacity`, `fill-opacity`, `stroke-opacity`
- `fill-rule` (evenodd, nonzero)
- `transform` (translate, scale, rotate)
- `rx`, `ry` on `<rect>` for rounded corners
- 2-stop linear and radial gradients (since Rack 2.6.0)

**Gradient constraints:**
- Maximum 2 color stops per gradient definition
- Both `<linearGradient>` and `<radialGradient>` supported
- Defined inside `<defs>` block
- Referenced via `url(#gradientId)` in fill or stroke attributes

---

## 6. SVG File Inventory

### Panel SVG

| File | Path | Dimensions | Description |
|------|------|-----------|-------------|
| Panel | `res/AnalogLFO.svg` | 71.12mm x 128.5mm | 14HP Forge Noir panel with all decorative elements, labels, emblem, rune, and component placement layer |

### Component SVGs (res/components/)

| File | Dimensions (mm) | Description |
|------|-----------------|-------------|
| ForgeKnobHero.svg | 16.38 x 16.38 | MORPH knob body + indicator (rotates) |
| ForgeKnobHero_bg.svg | ~18.00 x 18.00 | Shadow + outer metallic ring (static) |
| ForgeKnobSecondary.svg | 11.99 x 11.99 | CHARACTER/DRIFT knob body + indicator |
| ForgeKnobSecondary_bg.svg | ~13.50 x 13.50 | Shadow + metallic ring |
| ForgeKnobUtility.svg | 9.19 x 9.19 | RATE/PHASE knob body + indicator |
| ForgeKnobUtility_bg.svg | ~10.50 x 10.50 | Shadow + metallic ring |
| ForgeTrimpot.svg | 3.60 x 3.60 | CV attenuverter body + indicator (rotates) |
| ForgeTrimpot_bg.svg | ~4.20 x 4.20 | Shadow ring |
| ForgeJackInput.svg | 7.19 x 7.19 | Standard input jack |
| ForgeJackOutput.svg | 8.39 x 8.39 | Output jack with ember accent ring |
| ForgeHexBolt.svg | 3.20 x 3.20 | Hexagonal bolt screw replacement |

---

## 7. SVG Component Layer Convention

The panel SVG contains a hidden group `<g id="components" style="display:none">` that marks component placement positions with color-coded circles. This layer is invisible at runtime but serves as a reference for designers and tools.

**Color coding:**
- **Red circles** (`#ff0000`) = Params (knobs + trimpots), radius = half the component diameter
- **Green circles** (`#00ff00`) = Inputs (jacks), radius = half the component diameter
- **Blue circles** (`#0000ff`) = Outputs (jacks), radius = half the component diameter
- **Magenta rect** (`#ff00ff`) = Display area, matching display box dimensions

**Circle center** = widget placement center, which must match the `mm2px(Vec(x, y))` coordinates in the C++ widget code (`src/AnalogLFO.cpp`).

---

## 8. Coordinate System Notes

- **Origin** (0,0) is the top-left corner of the panel
- **X** increases rightward, **Y** increases downward
- All coordinates in this document are in **millimeters**
- Conversion to VCV Rack pixels: `pixels = mm * (75.0 / 25.4)` = `mm * 2.95276`
- The C++ code uses `mm2px(Vec(x, y))` which applies this conversion automatically

**Synchronization requirement:** The three sources of truth for component positions must always agree:
1. This document (`res/PANEL-SPEC.md`) -- Section 4 tables
2. The SVG components layer (`res/AnalogLFO.svg`) -- `<g id="components">` circle positions
3. The C++ widget code (`src/AnalogLFO.cpp`) -- `mm2px(Vec(x, y))` calls

If any one of these changes, update the other two to match.

---

## 9. Export Checklist

When exporting the panel SVG from a design tool (Illustrator, Inkscape, Figma, etc.), follow these steps:

1. [ ] Set document units to millimeters
2. [ ] Set width to 71.12mm, height to 128.5mm
3. [ ] Convert all text to curves/paths (no `<text>` elements)
4. [ ] Remove any CSS `<style>` blocks from the SVG
5. [ ] Verify no `<text>` elements remain (search the exported SVG source)
6. [ ] Ensure viewBox matches: `viewBox="0 0 71.12 128.5"`
7. [ ] Verify `width="71.12mm"` and `height="128.5mm"` have the `mm` suffix
8. [ ] Keep the hidden components layer intact (`<g id="components" style="display:none">`)
9. [ ] Do not change component circle positions (they must match C++ widget code)
10. [ ] Verify all colors use 6-digit hex format
11. [ ] Verify all gradients have maximum 2 stops
12. [ ] `<defs>` used only for gradient definitions
13. [ ] No `<use>`, `<clipPath>`, `<filter>`, or `<image>` elements
14. [ ] Test by building and loading in VCV Rack: `make && make install`
