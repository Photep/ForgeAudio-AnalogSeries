# Phase 19: Forge Noir Panel + Custom Components - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md -- this log preserves the alternatives considered.

**Date:** 2026-03-28
**Phase:** 19-forge-noir-panel-custom-components
**Areas discussed:** Visual fidelity trade-offs, Custom component strategy, Forge emblem approach, Canonical layout reference

---

## Visual Fidelity Trade-offs

### Rendering Strategy

| Option | Description | Selected |
|--------|-------------|----------|
| SVG-first, minimal NanoVG | Approximate effects using nanosvg-compatible elements. NanoVG only for display. | :heavy_check_mark: |
| Hybrid SVG + NanoVG overlays | SVG for flat elements, NanoVG for atmospheric effects. More code but closer to mockup. | |
| Maximum NanoVG rendering | SVG as minimal scaffolding, most richness drawn by NanoVG at runtime. | |

**User's choice:** SVG-first, minimal NanoVG (Recommended)
**Notes:** User accepted the recommended approach -- keep rendering simple and nanosvg-compatible.

### Must-Have Visual Effects

| Option | Description | Selected |
|--------|-------------|----------|
| Accent bar gradients | Ember-to-gold gradient bars at top and bottom | :heavy_check_mark: |
| Panel texture / warmth | Subtle radial gradient giving panel warm center glow | :heavy_check_mark: |
| Knob depth/metallic ring | Multi-band metallic ring and drop shadow approximation | :heavy_check_mark: |
| Section divider gradients | Ember gradient divider lines between panel zones | :heavy_check_mark: |

**User's choice:** All four selected as must-haves
**Notes:** Full atmospheric quality matters -- no effects should be dropped.

### MORPH Knob Knurling

| Option | Description | Selected |
|--------|-------------|----------|
| Skip knurling, machined rings only | Concentric circle grooves to suggest machined metal | :heavy_check_mark: |
| Approximate with radial lines | Thin radial line segments around knob edge | |
| Dotted ring pattern | Small circles arranged in a ring | |

**User's choice:** Skip knurling, machined rings only
**Notes:** Knurling is subtle at module scale and not achievable in nanosvg.

---

## Custom Component Strategy

### Knob Implementation

| Option | Description | Selected |
|--------|-------------|----------|
| Custom SVG skins | SVG files per knob size, subclass SvgKnob. Standard VCV approach. | :heavy_check_mark: |
| NanoVG-rendered knobs | Draw knobs in C++ using NanoVG at runtime. Non-standard. | |
| SVG base + NanoVG indicator overlay | SVG body, NanoVG draws ember indicator with glow. | |

**User's choice:** Custom SVG skins (Recommended)
**Notes:** Standard VCV approach preferred.

### Indicator Line Glow

| Option | Description | Selected |
|--------|-------------|----------|
| Solid ember line, no glow | Flat #e85d26 line | |
| Ember line with opacity halo | Line plus wider semi-transparent shape behind it | |
| You decide | Claude's discretion | :heavy_check_mark: |

**User's choice:** Claude's discretion
**Notes:** Whatever looks best within SVG constraints.

### Jack Implementation

| Option | Description | Selected |
|--------|-------------|----------|
| All in SVG | Ember accent ring baked into output jack SVG. One SVG per type. | :heavy_check_mark: |
| Ember ring on panel SVG | Ring on panel at output jack position. Couples to layout. | |
| You decide | Claude's discretion | |

**User's choice:** All in SVG (Recommended)
**Notes:** Keep components self-contained.

---

## Forge Emblem Approach

### Emblem Rendering

| Option | Description | Selected |
|--------|-------------|----------|
| Simplified SVG shapes | Low-opacity paths, circles, radial gradients. No blur. | :heavy_check_mark: |
| Drop emblem entirely | Clean panel without atmospheric background. | |
| Emblem as NanoVG overlay | Runtime rendering with blur support. Exception to SVG-first. | |

**User's choice:** Simplified SVG shapes (Recommended)
**Notes:** Atmospheric quality preserved, just crisper without blur.

### Forge Rune Detail Level

| Option | Description | Selected |
|--------|-------------|----------|
| Simplified SVG rune | Diamond outline, inner fill, center dot. Skip blur rings. | |
| Keep concentric glow rings | Approximate 4 rings as circle strokes at decreasing opacity. | |
| You decide | Claude's discretion | :heavy_check_mark: |

**User's choice:** Claude's discretion
**Notes:** None.

### Symmetry Method

| Option | Description | Selected |
|--------|-------------|----------|
| Duplicate elements manually | Write both halves explicitly. More verbose but nanosvg-compatible. | |
| Generate from template | Build script to output flat mirrored SVG. | |
| You decide | Claude's discretion | :heavy_check_mark: |

**User's choice:** Claude's discretion
**Notes:** None.

---

## Canonical Layout Reference

### Mockup Reference

| Option | Description | Selected |
|--------|-------------|----------|
| v19 (latest full panel) | Full panel view, most recent iteration. | |
| Closeup version | Different vertical spacing and proportions. | |
| forge-noir.html | The HTML file is the latest version (user clarification) | :heavy_check_mark: |

**User's choice:** forge-noir.html is the latest version
**Notes:** User clarified that the HTML file, not the PNG screenshots, is the canonical reference.

### Position Finality

| Option | Description | Selected |
|--------|-------------|----------|
| HTML positions are final | Convert pixel positions to mm directly. No changes. | :heavy_check_mark: |
| Mostly final, minor tweaks | Some adjustments needed. | |
| Need to review first | Open in browser to verify. | |

**User's choice:** HTML positions are final
**Notes:** No layout adjustments needed before implementation.

### Panel Spec Handling

| Option | Description | Selected |
|--------|-------------|----------|
| Replace entirely | Rewrite PANEL-SPEC.md with Forge Noir specifications. | :heavy_check_mark: |
| Update in-place | Modify existing sections. | |
| You decide | Claude's discretion | |

**User's choice:** Replace entirely (Recommended)
**Notes:** Old 12HP spec becomes irrelevant with new design.

---

## Claude's Discretion

- Knob indicator glow approach (D-08)
- Forge rune detail level (D-13)
- Emblem symmetry method (D-12)
- SVG file organization and naming
- Font-to-path conversion methodology
- Machined groove texture approximation

## Deferred Ideas

None -- discussion stayed within phase scope.
