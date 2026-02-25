# Forge Audio Analog LFO - Panel Specification

Designer handoff document for the Analog LFO module panel SVG.

This document captures every coordinate, size, color, and constraint needed to recreate or refine the panel without reading C++ code. The component positions in this spec, the SVG components layer, and the C++ widget code MUST stay synchronized. If a designer moves a component, the C++ code must be updated to match.

---

## 1. Panel Dimensions

| Property | Value |
|----------|-------|
| Width | 60.96mm (12HP x 5.08mm/HP) |
| Height | 128.5mm (standard 3U Eurorack) |
| Panel center X | 30.48mm |
| HP count | 12 |
| Screw clear zone | ~5mm diameter at each corner |

The SVG must declare `width="60.96mm"`, `height="128.5mm"`, and `viewBox="0 0 60.96 128.5"`.

---

## 2. Color Palette

| Name | Hex | Usage |
|------|-----|-------|
| Deep Navy | `#1a1a2e` | Panel background |
| Panel Border | `#2d2d4a` | Inset border stroke (0.3mm width, 0.15mm inset) |
| Forge Amber | `#e8a838` | Accent stripes, decorative lines, display border |
| Display Dark | `#0d0d1a` | Waveform display background |
| Muted Lavender | `#8888aa` | Brand text ("FORGE AUDIO"), jack labels |
| Light Lavender-Gray | `#c0c0d0` | Control labels (MORPH, CHARACTER, DRIFT, RATE, OCTAVE) |
| Bright White-Gray | `#e0e0e0` | Module name text ("ANALOG LFO") |

---

## 3. Layout Zones (Diamond Hierarchy)

The panel follows a diamond hierarchy: the most important control (Morph) is largest and centered, flanked by medium controls (Character, Drift), with utility controls (Rate, Octave) smallest at the periphery.

| Zone | Y Range (mm) | Height (mm) | Content |
|------|-------------|-------------|---------|
| Top accent + brand | 0 - 8 | 8 | Amber stripe (1mm), "FORGE AUDIO" brand text |
| Module name | 8 - 14 | 6 | "ANALOG LFO" title, amber decorative line at 14.5mm |
| Waveform display | 17 - 42 | 25 | Dark display area for Phase 3 waveform visualization |
| Morph knob zone | 43 - 62 | 19 | "MORPH" label at y=43.5, primary Morph knob (largest) centered at y=54.0 |
| Character/Drift zone | 60 - 76 | 16 | Labels at y=60.5, medium knobs flanking center at y=69.0 |
| Rate/Octave zone | 79 - 92 | 13 | Smaller utility knobs, labels above |
| Section divider | 94 | -- | Amber decorative line (0.15mm, 50% opacity) |
| Jack labels | 97 | -- | MCV, DCV, OUT labels |
| Jack row | 100 - 108 | 8 | Morph CV attenuator (Trimpot) + 3 jacks in bottom row |
| Bottom brand + accent | 109 - 128.5 | 19.5 | "FORGE AUDIO" brand text, amber stripe (1mm) |

---

## 4. Component Positions

All coordinates are center positions in millimeters. Diameter is the physical widget diameter used for spacing calculations.

| Component | Widget Class | Center X (mm) | Center Y (mm) | Diameter (mm) | Type |
|-----------|-------------|---------------|---------------|---------------|------|
| Morph | RoundBigBlackKnob | 30.48 | 54.0 | 15.24 | Param |
| Character | RoundLargeBlackKnob | 18.0 | 69.0 | 12.19 | Param |
| Drift | RoundLargeBlackKnob | 42.96 | 69.0 | 12.19 | Param |
| Rate | RoundBlackKnob | 18.0 | 86.0 | 9.60 | Param |
| Octave | RoundBlackKnob | 42.96 | 86.0 | 9.60 | Param |
| Morph CV Atten | Trimpot | 9.0 | 104.0 | 6.05 | Param |
| Morph CV | PJ301MPort | 21.0 | 104.0 | 8.03 | Input |
| Drift CV | PJ301MPort | 35.0 | 104.0 | 8.03 | Input |
| Output | PJ301MPort | 51.0 | 104.0 | 8.03 | Output |

**Knob size hierarchy:**
- RoundBigBlackKnob: 15.24mm diameter (primary control)
- RoundLargeBlackKnob: 12.19mm diameter (secondary controls)
- RoundBlackKnob: 9.60mm diameter (utility controls)
- Trimpot: 6.05mm diameter (attenuator)

**Bottom row layout:** All at Y=104.0mm. Morph CV attenuator Trimpot at x=9.0 is visually grouped with Morph CV jack at x=21.0 (~12mm gap). Drift CV jack at x=35.0 (~14mm from MCV). Output jack at x=51.0 (~16mm from DCV). The Trimpot (6.05mm) and jacks (8.03mm) fit comfortably within the 12HP panel width.

---

## 5. SVG Component Layer Convention

The SVG contains a hidden group `<g id="components" style="display:none">` that marks component placement positions with color-coded circles. This layer is invisible at runtime but serves as a reference for designers and tools.

**Color coding:**
- **Red circles** (`#ff0000`) = Params (knobs), radius = half the component diameter
- **Green circles** (`#00ff00`) = Inputs (jacks), radius = 4.0mm
- **Blue circles** (`#0000ff`) = Outputs (jacks), radius = 4.0mm

**Circle center** = widget placement center, which must match the `mm2px(Vec(x, y))` coordinates in the C++ widget code (`src/AnalogLFO.cpp`).

Example from the SVG:
```xml
<g id="components" style="display:none">
  <!-- Params (red) -->
  <circle cx="30.48" cy="54.0" r="7.62" fill="#ff0000"/>   <!-- Morph -->
  <circle cx="18.0" cy="69.0" r="6.1" fill="#ff0000"/>     <!-- Character -->
  <circle cx="9.0" cy="104.0" r="3.03" fill="#ff0000"/>    <!-- Morph CV Atten -->
  <!-- Inputs (green) -->
  <circle cx="21.0" cy="104.0" r="4.0" fill="#00ff00"/>    <!-- Morph CV -->
  <!-- Outputs (blue) -->
  <circle cx="51.0" cy="104.0" r="4.0" fill="#0000ff"/>    <!-- Out -->
</g>
```

---

## 6. nanosvg Compatibility Constraints

VCV Rack uses nanosvg to render panel SVGs. This imposes strict limitations on what SVG features can be used.

**NOT supported (will be ignored or cause errors):**
- `<text>` elements -- convert all text to `<path>` elements
- CSS `<style>` blocks -- use inline attributes (`fill`, `stroke`, etc.)
- `<use>` and `<defs>` references
- `<clipPath>` clipping
- `<filter>` effects (blur, shadow, etc.)
- `<image>` embedded images
- `opacity` on `<g>` groups -- set opacity on individual elements instead
- 3-digit hex colors -- always use 6-digit hex (e.g., `#1a1a2e`, not `#1a2`)

**Supported elements:**
- `<rect>`, `<circle>`, `<ellipse>`, `<line>`, `<polyline>`, `<polygon>`, `<path>`, `<g>`

**Supported attributes:**
- `fill`, `stroke`, `stroke-width`, `opacity`, `fill-opacity`, `stroke-opacity`
- `fill-rule` (evenodd, nonzero)
- `transform` (translate, scale, rotate)
- `rx`, `ry` on `<rect>` for rounded corners
- 2-stop linear and radial gradients (since Rack 2.6.0)

---

## 7. Export Checklist

When exporting the panel SVG from a design tool (Illustrator, Inkscape, Figma, etc.), follow these steps:

1. [ ] Set document units to millimeters
2. [ ] Set width to 60.96mm, height to 128.5mm
3. [ ] Convert all text to curves/paths (no `<text>` elements)
4. [ ] Remove any CSS `<style>` blocks from the SVG
5. [ ] Verify no `<text>` elements remain (search the exported SVG source)
6. [ ] Ensure viewBox matches: `viewBox="0 0 60.96 128.5"`
7. [ ] Verify `width="60.96mm"` and `height="128.5mm"` have the `mm` suffix
8. [ ] Keep the hidden components layer intact (`<g id="components" style="display:none">`)
9. [ ] Do not change component circle positions (they must match C++ widget code)
10. [ ] Verify all colors use 6-digit hex format
11. [ ] Remove any `<defs>`, `<use>`, `<clipPath>`, `<filter>`, or `<image>` elements
12. [ ] Test by building and loading in VCV Rack: `make && make install`

---

## 8. Coordinate System Notes

- **Origin** (0,0) is the top-left corner of the panel
- **X** increases rightward, **Y** increases downward
- All coordinates in this document are in **millimeters**
- Conversion to VCV Rack pixels: `pixels = mm * (75.0 / 25.4)` = `mm * 2.95276`
- The C++ code uses `mm2px(Vec(x, y))` which applies this conversion automatically

**Synchronization requirement:** The three sources of truth for component positions must always agree:
1. This document (`res/PANEL-SPEC.md`) -- Section 4 table
2. The SVG components layer (`res/AnalogLFO.svg`) -- `<g id="components">` circle positions
3. The C++ widget code (`src/AnalogLFO.cpp`) -- `mm2px(Vec(x, y))` calls

If any one of these changes, update the other two to match.
