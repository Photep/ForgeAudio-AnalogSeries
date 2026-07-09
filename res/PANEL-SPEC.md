# Forge Audio Analog LFO - Panel Specification (Forge Noir)

Designer handoff document for the 18HP Forge Noir panel SVG.

This document captures every coordinate, size, color, and constraint needed to recreate or refine the panel without reading C++ code. Component positions here, the panel art in `res/AnalogLFO.svg`, and the widget placement code in `src/AnalogLFO.cpp` MUST stay synchronized. If a designer moves a component in the art, the C++ `mm2px(Vec(x, y))` call must be updated to match (and vice versa).

> **Source of truth:** the shipping values below are transcribed from `src/AnalogLFO.cpp` (component placement) and `res/AnalogLFO.svg` (dimensions, palette, artwork). Where this document and the code ever disagree, the code wins — update this document.

---

## 1. Panel Dimensions

| Property | Value |
|----------|-------|
| Width | 91.44mm (18HP x 5.08mm/HP) |
| Height | 128.5mm (standard 3U Eurorack) |
| Panel center X | 45.72mm |
| HP count | 18 |
| Bolt-hole inset | 2.60mm from each edge |

The SVG declares `width="91.44mm"`, `height="128.5mm"`, and `viewBox="0 0 91.44 128.5"`.

---

## 2. Color Palette

### Structural (near-black)

The panel background is a subtle vertical gradient, not a flat fill.

| Role | Value | Usage |
|------|-------|-------|
| Background gradient | `#0e0e11` → `#0a0a0c` → `#070708` (top→bottom) | Panel body fill (`url(#bg)`) |
| Panel border | `#202024` | 0.35mm inset edge stroke |
| Bolt fill / dark bands | `#161618`, `#2c2c30` | Hex-bolt bodies, metallic dark bands |

### Accent (Forge Ember + gold)

| Name | Hex | Usage |
|------|-----|-------|
| Forge Ember | `#e85d26` | Primary accent — brand text, indicators, accent rails, output ring, CV lines, emblem glow, rune |
| Ember Highlight | `#ff8a4c` | Brightest ember moments (indicator tips, hottest glow) |
| Molten Gold | `#d4a017` | Secondary accent — rail center, display readout warm details, rune power ring |
| Rail dark stop | `#1a0800` | Fade-out ends of the top/bottom accent rails |

The top and bottom accent rails share one gradient: `#1a0800` → `#e85d26` (0.22) → `#d4a017` (0.5) → `#e85d26` (0.78) → `#1a0800`.

> **Ember is a single value:** `#e85d26` is used consistently across the panel art, the component SVGs (`res/components/`), and the runtime NanoVG code (`src/AnalogLFO.cpp`). Do not introduce a near-duplicate (an earlier panel revision used `#e8612a`; it was normalized).

### Text / label greys (luminance hierarchy)

Static text is a ramp of warm greys — brighter = higher in the visual hierarchy.

| Hex | Tier | Typical usage |
|------|------|---------------|
| `#ece8e2` | Brightest | Module name ("ANALOG LFO"), MORPH hero label |
| `#c8c4be` | Light | Primary control labels (CHARACTER, DRIFT) |
| `#a39e96` | Mid | Secondary control labels (RATE, PHASE) |
| `#8f8a82` | Dim | CV labels and small I/O text (the bulk of small glyphs) |

### Display (waveform screen)

| Role | Value | Usage |
|------|-------|-------|
| Screen fill | `#0b0d0c` → `#040504` (radial) | Display background (`url(#screen)`) |
| Screen stroke | ember/gold gradient, low opacity | Bezel edge (`url(#dispStroke)`) |

> The live display (waveform, pills, BPM, phase dot) is drawn at runtime by the `WaveformDisplay` NanoVG widget, not baked into the SVG. The SVG only supplies the screen well and bezel.

---

## 3. Layout Zones (top → bottom)

| Zone | Y (mm) | Content |
|------|--------|---------|
| Top accent rail | 0 – 0.7 | Ember/gold rail gradient |
| Header | ~1 – 14 | Brand (FORGE / rune / AUDIO), header slashes, "ANALOG LFO" name + decorative line |
| Waveform display | 19.00 – 45.00 | Screen well (see §4) |
| MORPH zone | ~53 – 69 | Hero knob at y=61.00, forge emblem glow behind |
| Control-knob row | ~81 – 93 | CHARACTER, DRIFT, RATE, PHASE at y=87.00 |
| Trimpot row | ~106 – 111 | 5 CV attenuverters at y=108.50 |
| Bottom I/O row | ~116 – 123 | 5 CV inputs + CLK, RST, OUTPUT at y=119.50 |
| Bottom accent rail | 127.80 – 128.5 | Ember/gold rail gradient |

---

## 4. Component Position Map

All coordinates are **center** positions in millimeters, taken directly from `src/AnalogLFO.cpp`. Widget classes are custom Forge Noir types.

### Params — Knobs

| Control | Widget Class | Center X | Center Y | Diameter |
|---------|-------------|----------|----------|----------|
| MORPH_PARAM | ForgeKnobHero | 45.72 | 61.00 | 16.38 |
| CHARACTER_PARAM | ForgeKnobSecondary | 18.00 | 87.00 | 11.99 |
| DRIFT_PARAM | ForgeKnobSecondary | 36.24 | 87.00 | 11.99 |
| RATE_PARAM | ForgeKnobSecondary | 54.48 | 87.00 | 11.99 |
| PHASE_OFFSET_PARAM | ForgeKnobSecondary | 72.72 | 87.00 | 11.99 |

The four control knobs form a single row at y=87.00 with even 18.24mm center-to-center spacing.

### Params — Trimpots (CV attenuverters)

| Control | Widget Class | Center X | Center Y | Diameter |
|---------|-------------|----------|----------|----------|
| MORPH_ATTEN_PARAM | ForgeTrimpot | 7.70 | 108.50 | 5.40 |
| CHARACTER_ATTEN_PARAM | ForgeTrimpot | 18.56 | 108.50 | 5.40 |
| DRIFT_ATTEN_PARAM | ForgeTrimpot | 29.43 | 108.50 | 5.40 |
| FM_ATTEN_PARAM | ForgeTrimpot | 40.29 | 108.50 | 5.40 |
| PHASE_OFFSET_ATTEN_PARAM | ForgeTrimpot | 51.15 | 108.50 | 5.40 |

### Inputs

| Control | Widget Class | Center X | Center Y | Diameter |
|---------|-------------|----------|----------|----------|
| MORPH_CV_INPUT | ForgeJackInput | 7.70 | 119.50 | 7.19 |
| CHARACTER_CV_INPUT | ForgeJackInput | 18.56 | 119.50 | 7.19 |
| DRIFT_CV_INPUT | ForgeJackInput | 29.43 | 119.50 | 7.19 |
| FM_INPUT | ForgeJackInput | 40.29 | 119.50 | 7.19 |
| PHASE_OFFSET_CV_INPUT | ForgeJackInput | 51.15 | 119.50 | 7.19 |
| CLK_INPUT | ForgeJackInput | 62.01 | 119.50 | 7.19 |
| RESET_INPUT | ForgeJackInput | 72.88 | 119.50 | 7.19 |

### Outputs

| Control | Widget Class | Center X | Center Y | Diameter |
|---------|-------------|----------|----------|----------|
| OUTPUT | ForgeJackOutput | 83.74 | 119.50 | 8.39 |

### Waveform Display

| Element | Pos (mm) | Size (mm) | Span |
|---------|----------|-----------|------|
| WaveformDisplay | (5.00, 19.00) | (81.44, 26.00) | x 5.00–86.44, y 19.00–45.00 (horizontally centered) |

### Screws (Hex Bolts)

| Position | Coordinate (mm) |
|----------|-----------------|
| Top-left | (2.60, 2.60) |
| Top-right | (88.84, 2.60) |
| Bottom-left | (2.60, 125.90) |
| Bottom-right | (88.84, 125.90) |

The bottom-right bolt is pushed into the corner to clear the OUTPUT jack's ember accent rings.

### Widget size hierarchy

- ForgeKnobHero — 16.38mm (primary control: MORPH)
- ForgeKnobSecondary — 11.99mm (all four control knobs: CHARACTER, DRIFT, RATE, PHASE)
- ForgeTrimpot — 5.40mm (CV attenuverters)
- ForgeJackInput — 7.19mm (standard input)
- ForgeJackOutput — 8.39mm (output, with ember accent ring)
- ForgeHexBolt — 3.20mm (hexagonal screw)

> There is no separate "utility" knob class. RATE and PHASE use the same `ForgeKnobSecondary` as CHARACTER and DRIFT.

**Bottom-row layout:** the bottom I/O row is a single line of eight jacks at y=119.50 with even ~10.86mm spacing. Columns 1–5 (x = 7.70, 18.56, 29.43, 40.29, 51.15) each carry a trimpot directly above at y=108.50; ember connecting lines visually link each trimpot to its jack. Columns 6–8 continue the same rhythm: CLK (62.01), RST (72.88), OUTPUT (83.74).

---

## 5. nanosvg Compatibility Constraints

VCV Rack renders panel SVGs with nanosvg, which imposes strict limits.

**NOT supported (ignored or broken):**
- `<text>` elements — convert all text to `<path>` outlines (the shipping panel contains zero `<text>` elements)
- CSS `<style>` blocks — use inline attributes (`fill`, `stroke`, etc.)
- `<use>` references and `<defs>` other than gradient definitions
- `<clipPath>` clipping
- `<filter>` effects (blur, shadow, etc.)
- `<image>` embedded rasters
- `opacity` on `<g>` groups — set opacity on individual elements
- 3-digit hex colors — always use 6-digit hex

**Supported elements:** `<rect>`, `<circle>`, `<ellipse>`, `<line>`, `<polyline>`, `<polygon>`, `<path>`, `<g>`

**Supported attributes:** `fill`, `stroke`, `stroke-width`, `opacity`, `fill-opacity`, `stroke-opacity`, `fill-rule`, `transform` (translate/scale/rotate), `rx`/`ry` on `<rect>`

**Gradient constraints:** max 2 color stops per gradient in older Rack; the panel uses multi-stop gradients that require Rack ≥ 2.6.0. Both `<linearGradient>` and `<radialGradient>` are supported, defined in `<defs>`, referenced via `url(#id)`.

---

## 6. SVG File Inventory

### Panel SVG

| File | Path | Dimensions | Description |
|------|------|-----------|-------------|
| Panel | `res/AnalogLFO.svg` | 91.44mm x 128.5mm | 18HP Forge Noir panel: background, accent rails, emblem, brand text, labels, display well/bezel, and footer. All text pre-converted to `<path>` outlines. |

### Component SVGs (`res/components/`)

| File | Dimensions (mm) | Description |
|------|-----------------|-------------|
| ForgeKnobHero.svg | 16.38 x 16.38 | MORPH knob body + indicator (rotates) |
| ForgeKnobHero_bg.svg | 18.00 x 18.00 | Shadow + outer metallic ring (static) |
| ForgeKnobSecondary.svg | 11.99 x 11.99 | Control knob body + indicator (rotates) |
| ForgeKnobSecondary_bg.svg | 13.50 x 13.50 | Shadow + metallic ring (static) |
| ForgeTrimpot.svg | 5.40 x 5.40 | CV attenuverter body + indicator (rotates) |
| ForgeTrimpot_bg.svg | 6.30 x 6.30 | Shadow ring (static) |
| ForgeJackInput.svg | 7.19 x 7.19 | Standard input jack |
| ForgeJackOutput.svg | 8.39 x 8.39 | Output jack with ember accent ring |
| ForgeHexBolt.svg | 3.20 x 3.20 | Hexagonal bolt screw replacement |

> Each knob/trimpot is a two-file pair: a static `_bg` (shadow + metal ring) drawn behind a rotating foreground (body + indicator). The `_bg` is auto-centered under its foreground by the widget code.

---

## 7. Placement Synchronization

Unlike earlier revisions, the 18HP SVG does **not** contain a hidden `<g id="components">` placement layer. Component positions live in exactly two places and must agree:

1. This document (`res/PANEL-SPEC.md`) — Section 4 tables
2. The C++ widget code (`src/AnalogLFO.cpp`) — `mm2px(Vec(x, y))` calls

The panel artwork (`res/AnalogLFO.svg`) draws the labels, wells, and decorative rings that sit *around* each component; it must be visually aligned to the same coordinates, but there is no machine-readable placement layer to diff against. When a position changes, update both the code and this document, and re-check the art visually in Rack.

---

## 8. Coordinate System Notes

- **Origin** (0,0) is the top-left corner of the panel
- **X** increases rightward, **Y** increases downward; all coordinates are in **millimeters**
- Conversion to VCV Rack pixels: `pixels = mm * (75.0 / 25.4)` = `mm * 2.95276`
- `mm2px(Vec(x, y))` applies this automatically; `createParamCentered` / `createInputCentered` / `createOutputCentered` place the widget **center** at the given point

---

## 9. Export Checklist

When exporting the panel SVG from a design tool (Illustrator, Inkscape, Figma, etc.):

1. [ ] Set document units to millimeters
2. [ ] Set width to 91.44mm, height to 128.5mm
3. [ ] Convert all text to curves/paths (no `<text>` elements)
4. [ ] Remove any CSS `<style>` blocks
5. [ ] Verify no `<text>` elements remain (search the exported source)
6. [ ] Ensure `viewBox="0 0 91.44 128.5"`
7. [ ] Verify `width="91.44mm"` and `height="128.5mm"` retain the `mm` suffix
8. [ ] Keep component art (labels, wells, rings) aligned to the Section 4 coordinates
9. [ ] Verify all colors use 6-digit hex; ember is exactly `#e85d26`
10. [ ] Use only `<defs>` gradient definitions — no `<use>`, `<clipPath>`, `<filter>`, or `<image>`
11. [ ] Build and load in VCV Rack: `make && make install`
