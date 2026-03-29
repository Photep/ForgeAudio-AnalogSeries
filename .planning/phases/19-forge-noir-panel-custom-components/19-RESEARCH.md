# Phase 19: Forge Noir Panel + Custom Components - Research

**Researched:** 2026-03-29
**Domain:** VCV Rack 2 SVG panel rendering, nanosvg constraints, custom widget C++ classes
**Confidence:** HIGH

## Summary

Phase 19 replaces the existing 12HP panel with a 14HP Forge Noir panel featuring custom SVG components. The core technical challenge is translating the HTML/CSS mockup's rich visual effects (box-shadows, conic gradients, multi-layer radials, blur filters) into nanosvg-compatible SVG that maintains the design's premium feel. The VCV Rack SDK provides clean base classes (`SvgKnob`, `SvgPort`, `SvgScrew`) that accept custom SVG files -- the pattern is to define structs that subclass these, call `setSvg()` with custom artwork, and use `createParamCentered<CustomWidget>()` in the widget constructor.

Rack 2.6.0 (Nov 2024) added 2-stop linear/radial gradient rendering, confirmed by both the changelog and the nanosvg parser source in the SDK. This is the critical enabler for metallic curvature, accent bar gradients, and knob depth effects. Gradients are defined in `<defs>` blocks (nanosvg DOES parse `<defs>` for gradients, despite not supporting `<use>`/`<defs>` for element cloning). Each gradient is limited to exactly 2 stops. Multiple overlapping shapes with different 2-stop gradients can simulate multi-stop effects.

The coordinate conversion from the forge-noir.html mockup (356px wide at 5x) to panel mm is: `mm = px * (71.12 / 356)`, factor 0.199775. All 18 controls have been mapped from mockup pixel positions to mm coordinates.

**Primary recommendation:** Create the panel SVG and 8 component SVGs (3 knob sizes, 1 trimpot, 2 jack sizes, 1 hex bolt screw, and the panel itself) using hand-authored nanosvg-compatible SVG with 2-stop gradients for depth, then wire them into custom C++ widget structs that subclass the standard VCV base classes.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** SVG-first approach with minimal NanoVG. All panel elements, components, and atmospheric effects rendered as nanosvg-compatible SVG. NanoVG used only for the waveform display (existing). No NanoVG overlays for knobs, emblem, or glows.
- **D-02:** Effects that nanosvg can't render (box-shadows, blur filters, conic gradients, CSS opacity on groups) are approximated using nanosvg-compatible elements: semi-transparent shapes for shadows, concentric rings for glow, radial gradients for depth, per-element opacity.
- **D-03:** 2-stop linear and radial gradients ARE supported (Rack 2.6.0+) and should be used extensively for metallic curvature, accent bars, and panel warmth.
- **D-04:** ALL of the following are must-haves in the SVG approximation: accent bar gradients, panel texture/warmth (radial gradient behind knobs), knob depth/metallic ring (concentric circles with gradients), section divider gradients.
- **D-05:** Knurled texture on MORPH hero knob: SKIP. Use machined concentric ring grooves instead. Knurling (conic gradient) is not achievable in nanosvg and is subtle at module scale.
- **D-06:** Custom SVG skin files for all components. Subclass SvgKnob (or equivalent VCV widget base) and point to custom SVG files. Standard VCV approach -- no NanoVG-rendered widgets.
- **D-07:** Three knob SVG skins: hero (MORPH, 82px at 5x), secondary (CHARACTER/DRIFT, 60px at 5x), utility (RATE/PHASE, 46px at 5x). Each with radial gradients for curvature, concentric circles for metallic ring, center cap indent, and machined groove texture approximation.
- **D-08:** Knob indicator line: Claude's discretion -- solid ember line or ember line with semi-transparent opacity halo, whatever looks best within SVG constraints.
- **D-09:** Two jack SVG skins: small input (36px at 5x) and large output (42px at 5x). Output jack SVG includes the ember accent ring baked in (semi-transparent circle stroke). Not a separate panel element.
- **D-10:** Trimpot SVG skin: bright metallic body (contrasting with dark knobs), directional indicator slot, scalloped edge approximated with nanosvg-compatible elements.
- **D-11:** Simplified SVG shapes for the forge emblem. Low-opacity filled paths for molten streams, small circles for ember particles, radial gradients for central glow. No blur -- soften edges using wider, lower-opacity strokes. Still atmospheric, just crisper than the HTML mockup.
- **D-12:** nanosvg doesn't support `<use>`/`<defs>` for mirroring. Emblem symmetry guaranteed by Claude's discretion -- either manually duplicated left/right elements or a build-time template script that generates flat mirrored SVG.
- **D-13:** Rune glyph rendered as simplified nanosvg-compatible SVG. Claude's discretion on detail level -- may include concentric power rings at decreasing opacity (no blur), or simplify to diamond + center dot. The rune shape is strong enough as a brand mark without the Gaussian blur glow.
- **D-14:** `forge-noir.html` is the canonical layout reference (not PNG screenshots). All component positions derived from this file's pixel coordinates, converted to mm using the 5x scale factor (71.12mm / 356px = 0.1998 mm/px).
- **D-15:** Positions in the HTML are FINAL. No further layout adjustments needed before implementation.
- **D-16:** PANEL-SPEC.md (`res/PANEL-SPEC.md`) will be completely replaced with Forge Noir specifications -- new dimensions (14HP), new color palette, new component positions, new nanosvg constraints documentation. Old 12HP spec becomes irrelevant.
- **D-17:** All text rendered as SVG `<path>` elements (nanosvg requirement). Brand text uses FoundationLogo font glyphs converted to paths, module name uses Bebas Neue, labels use Chakra Petch/JetBrains Mono -- all converted to SVG path data.

### Claude's Discretion
- Knob indicator glow approach (solid vs opacity halo) -- D-08
- Forge rune detail level (full rings vs simplified) -- D-13
- Emblem symmetry method (manual duplication vs build script) -- D-12
- SVG file organization (one panel SVG + separate component SVGs, naming conventions)
- Conversion methodology for font glyphs to SVG paths
- Machined groove texture approximation on knob bodies

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PANEL-01 | 14HP Forge Noir SVG panel with near-black background, ember accent bars, hex bolt screws | SVG structure verified: width="71.12mm" height="128.5mm", 2-stop linear gradients for accent bars, hex bolt screws as custom SvgScrew subclass |
| PANEL-02 | All controls repositioned per Forge Noir mockup (5 main knobs, 5 CV trimpots+jacks, CLK/RST/OUTPUT) | Full coordinate mapping from forge-noir.html pixel positions to mm completed -- 18 controls total (10 params, 7 inputs, 1 output) |
| PANEL-03 | Custom SVG knob components (3 sizes: hero/secondary/utility) with machined metal appearance | SvgKnob base class with bg layer confirmed; struct pattern: subclass RoundKnob, set minAngle/maxAngle, call setSvg() with custom SVG |
| PANEL-04 | Custom SVG trimpot components (bright scalloped attenuverters) | Trimpot base class confirmed: subclasses SvgKnob, has bg layer, minAngle=-0.75*PI, maxAngle=0.75*PI |
| PANEL-05 | Custom SVG jack components (2 sizes: standard/output with ember accent ring) | SvgPort base class confirmed; output ember ring baked into output jack SVG per D-09 |
| PANEL-06 | Forge emblem background element | Baked into panel SVG as low-opacity paths; symmetry via manual duplication (nanosvg has no `<use>` cloning) |
| PANEL-07 | Brand typography rendered as SVG paths (Forge Audio header, Analog LFO name) | Text-as-paths pattern already established in current AnalogLFO.svg; font glyph conversion required for FoundationLogo, Bebas Neue, Chakra Petch, JetBrains Mono |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack 2 SDK | 2.6.0+ | Module framework, panel rendering | 2-stop gradient support critical for this phase |
| nanosvg | SDK-bundled | SVG parsing for panel and components | Only SVG renderer available in Rack; constraints dictate all artwork |
| NanoVG | SDK-bundled | Display rendering (existing, not changed) | WaveformDisplay stays as-is, just repositioned |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| C++17 | GCC/Clang | Custom widget struct definitions | All custom component class definitions in AnalogLFO.cpp |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Hand-authored SVG | Inkscape/Figma export | Exported SVGs contain cruft, non-nanosvg-compatible elements; hand-authored gives full control over nanosvg compatibility |
| 2-stop gradient layering | Single solid fills | Loses all metallic depth, curvature, and premium feel |
| Custom SvgKnob subclass | NanoVG runtime rendering | D-01 explicitly prohibits NanoVG for knobs; SVG approach is simpler and cached in framebuffer |

## Architecture Patterns

### Recommended Project Structure
```
res/
  AnalogLFO.svg              # 14HP Forge Noir panel (replaces old 12HP)
  components/
    ForgeKnobHero.svg        # 82px hero knob (MORPH)
    ForgeKnobHero_bg.svg     # Hero knob background/shadow layer
    ForgeKnobSecondary.svg   # 60px secondary knob (CHARACTER, DRIFT)
    ForgeKnobSecondary_bg.svg
    ForgeKnobUtility.svg     # 46px utility knob (RATE, PHASE)
    ForgeKnobUtility_bg.svg
    ForgeTrimpot.svg          # 18px bright scalloped trimpot
    ForgeTrimpot_bg.svg
    ForgeJackInput.svg        # 36px standard input jack
    ForgeJackOutput.svg       # 42px output jack with ember ring
    ForgeHexBolt.svg          # 16px hexagonal bolt (screw replacement)
src/
  AnalogLFO.cpp               # Widget struct definitions + updated constructor
```

**Recommendation on SVG file organization:** Separate component SVGs in a `res/components/` subdirectory. The panel SVG stays at `res/AnalogLFO.svg` to maintain the existing `setPanel()` asset path.

### Pattern 1: Custom Knob Widget Struct
**What:** Subclass RoundKnob (which extends SvgKnob) to load custom SVG skins
**When to use:** All three knob sizes
**Example:**
```cpp
// Source: componentlibrary.hpp RoundKnob/RoundBigBlackKnob pattern
struct ForgeKnobHero : app::SvgKnob {
    widget::SvgWidget* bg;

    ForgeKnobHero() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;

        bg = new widget::SvgWidget;
        fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobHero.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobHero_bg.svg")));
    }
};
```

### Pattern 2: Custom Port Widget Struct
**What:** Subclass SvgPort to load custom jack SVG
**When to use:** Both input and output jack types
**Example:**
```cpp
// Source: componentlibrary.hpp PJ301MPort pattern
struct ForgeJackInput : app::SvgPort {
    ForgeJackInput() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeJackInput.svg")));
    }
};

struct ForgeJackOutput : app::SvgPort {
    ForgeJackOutput() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeJackOutput.svg")));
    }
};
```

### Pattern 3: Custom Trimpot Widget Struct
**What:** Subclass SvgKnob with narrower rotation range for attenuverter behavior
**When to use:** All 5 CV attenuator trimpots
**Example:**
```cpp
// Source: componentlibrary.hpp Trimpot pattern
struct ForgeTrimpot : app::SvgKnob {
    widget::SvgWidget* bg;

    ForgeTrimpot() {
        minAngle = -0.75 * M_PI;
        maxAngle = 0.75 * M_PI;

        bg = new widget::SvgWidget;
        fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeTrimpot.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeTrimpot_bg.svg")));
    }
};
```

### Pattern 4: Custom Screw (Hex Bolt) Widget Struct
**What:** Subclass SvgScrew with hex bolt SVG
**When to use:** All 4 panel corner positions
**Example:**
```cpp
// Source: componentlibrary.hpp ScrewSilver pattern
struct ForgeHexBolt : app::SvgScrew {
    ForgeHexBolt() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeHexBolt.svg")));
    }
};
```

### Pattern 5: Coordinate Conversion from Mockup
**What:** Convert forge-noir.html CSS pixel positions to VCV mm coordinates
**When to use:** Every addParam/addInput/addOutput call in the widget constructor
**Formula:** `mm = px * (71.12 / 356.0)` where `71.12 / 356.0 = 0.199775...`
**Example:**
```cpp
// MORPH knob at forge-noir.html position: left=178px, top=237px
// mm: (178 * 71.12/356, 237 * 71.12/356) = (35.56, 47.35)
addParam(createParamCentered<ForgeKnobHero>(mm2px(Vec(35.56f, 47.35f)),
         module, AnalogLFO::MORPH_PARAM));
```

### Pattern 6: nanosvg-Compatible SVG Component Structure
**What:** SVG file structure for knob/jack/trimpot components
**When to use:** All component SVG files
**Example:**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg"
     width="16.38mm" height="16.38mm"
     viewBox="0 0 16.38 16.38">
  <!-- 82px at 5x = 16.38mm hero knob -->
  <!-- All dimensions in mm, matching VCV coordinate system -->
  <defs>
    <radialGradient id="knob-body" cx="38%" cy="28%">
      <stop offset="0%" stop-color="#4a4a4a"/>
      <stop offset="100%" stop-color="#0a0a0a"/>
    </radialGradient>
  </defs>
  <!-- Metallic ring (bg layer renders this) -->
  <!-- Knob body with radial gradient -->
  <circle cx="8.19" cy="8.19" r="7.5" fill="url(#knob-body)"/>
  <!-- Indicator line (rotated by VCV SvgKnob) -->
  <rect x="7.69" y="1" width="1" height="5" rx="0.3"
        fill="#e85d26" opacity="0.9"/>
</svg>
```

### Anti-Patterns to Avoid
- **Using `<text>` elements:** nanosvg cannot render text. ALL text must be `<path>` elements.
- **Using CSS `<style>` blocks:** nanosvg ignores `<style>`. Use inline `fill`, `stroke`, `opacity` attributes.
- **Using `<use>` or `<defs>` for element cloning:** nanosvg does NOT support `<use>` for referencing elements. BUT `<defs>` IS supported for gradient definitions.
- **Using `<filter>` elements (blur, shadow):** Not supported. Approximate with semi-transparent shapes.
- **Using `<clipPath>`:** Not supported. Use SVG shape construction instead.
- **Using opacity on `<g>` groups:** Must set opacity on individual elements.
- **Using 3-digit hex colors:** Always use 6-digit (e.g., `#0c0c0c` not `#0c0`).
- **More than 2 gradient stops:** Rack 2.6 renders only 2-stop gradients. Layer multiple shapes for multi-stop effects.
- **Using `<image>` elements:** Not supported.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| SVG knob rotation | Custom rotation code | SvgKnob base class with TransformWidget | SvgKnob handles rotation math, framebuffer caching, minAngle/maxAngle automatically |
| Knob shadow | NanoVG shadow overlay | Separate _bg.svg with shadow circle | SvgKnob.bg layer renders behind the rotating knob body -- standard VCV pattern |
| Panel loading | Custom SVG parsing | `setPanel(createPanel(asset::plugin(...)))` | Handles SVG→panel pipeline, dark mode, dimensions |
| Component centering | Manual box.pos math | `createParamCentered<T>()` / `createInputCentered<T>()` | Automatically offsets by half the widget size |
| Font to path conversion | Manual path tracing | FontForge/Inkscape SVG export of text outlines | Complex glyph outlines need proper bezier curves; manual approximation loses quality |

**Key insight:** VCV Rack's widget system does all the hard work of SVG loading, framebuffer caching, knob rotation transforms, and event handling. The custom component work is purely SVG artwork + thin C++ struct wrappers.

## Common Pitfalls

### Pitfall 1: SVG Dimensions Mismatch
**What goes wrong:** Component SVG viewport doesn't match expected physical size, causing widgets to render at wrong scale or position offset.
**Why it happens:** SVG width/height in px vs mm confusion. VCV expects mm dimensions in the SVG.
**How to avoid:** Always declare width/height with `mm` suffix: `width="16.38mm" height="16.38mm"`. ViewBox must match: `viewBox="0 0 16.38 16.38"`. Verify: 82px at 5x scale / (356px/71.12mm) = 16.38mm.
**Warning signs:** Components appear giant/tiny, or offset from their position.

### Pitfall 2: Gradient Stop Count Exceeds 2
**What goes wrong:** Gradients with 3+ stops parse but render incorrectly (only first 2 stops used, or ignored entirely).
**Why it happens:** nanosvg parses any number of stops, but VCV Rack 2.6 renderer only handles 2-stop gradients. The parse succeeds silently.
**How to avoid:** Strictly limit every `<linearGradient>` and `<radialGradient>` to exactly 2 `<stop>` elements. Simulate multi-stop by layering multiple shapes with separate 2-stop gradients.
**Warning signs:** Gradients look flat or different from expected.

### Pitfall 3: Panel Width Change Breaks Module Position
**What goes wrong:** Changing from 12HP (60.96mm) to 14HP (71.12mm) shifts the module in existing patches, and screw positions must match new width.
**Why it happens:** Module width is determined by the panel SVG dimensions. Existing patches store module positions.
**How to avoid:** Accept the shift (documented as a known trade-off in PROJECT.md). Update all screw positions to match 14HP width. Update `box.size` if manually set anywhere.
**Warning signs:** Module looks misaligned in rack, screws float in wrong positions.

### Pitfall 4: Foreground SVG vs Background SVG Confusion
**What goes wrong:** The knob indicator appears behind the knob body, or the shadow renders on top.
**Why it happens:** SvgKnob has TWO SVG layers: the main SVG (which rotates with the knob) and the bg SVG (which stays static behind it). Shadows/rings go in bg, the rotating knob body + indicator go in the main SVG.
**How to avoid:** Main SVG = knob body + indicator line (rotates). Bg SVG = shadow circle + static outer ring (stays put). The `fb->addChildBelow(bg, tw)` call ensures bg renders below the transform widget.
**Warning signs:** Indicator doesn't rotate, or shadow rotates with the knob.

### Pitfall 5: Text Path Data Complexity
**What goes wrong:** Font glyph paths are incorrect, labels render as garbled shapes.
**Why it happens:** Manual path approximation of font glyphs is error-prone. Different fonts have wildly different glyph complexities.
**How to avoid:** Use a font-to-SVG-path tool (FontForge, Inkscape's "Object to Path"). For the current panel, the existing AnalogLFO.svg already has hand-crafted path letterforms that can be adapted. The style is geometric block letters, not high-fidelity font reproduction.
**Warning signs:** Letters look wrong, inconsistent stroke widths, broken counters (holes in B, D, O, etc.).

### Pitfall 6: Emblem Elements Not Visible
**What goes wrong:** Forge emblem (very low opacity: 0.03-0.18) is invisible or overwhelms the panel.
**Why it happens:** Opacity tuning that works in HTML rendering may look different in nanosvg rendering. Browser antialiasing differs from VCV's GPU rendering.
**How to avoid:** Start with the HTML opacity values, then test in VCV Rack and adjust. Keep emblem elements simple (paths, circles) -- complex layered transparency may render differently than expected.
**Warning signs:** Panel looks blank where emblem should be, or emblem is too bright/distracting.

### Pitfall 7: Component Layer Desync
**What goes wrong:** The hidden `<g id="components">` layer in the panel SVG has positions that don't match the C++ `mm2px(Vec(...))` calls.
**Why it happens:** Coordinates updated in one place but not the other.
**How to avoid:** Maintain the three-way synchronization: PANEL-SPEC.md table, SVG components layer, C++ widget code. Convert all positions from the single source of truth (forge-noir.html) using the same conversion factor.
**Warning signs:** Components render offset from their visual placement on the panel.

### Pitfall 8: Knob Rotation Range
**What goes wrong:** Knob indicator doesn't sweep the expected arc, or sweeps too far and clips through itself.
**Why it happens:** minAngle/maxAngle values don't match the indicator line position in the SVG. Standard RoundKnob uses -0.83*PI to 0.83*PI (~300 degrees), Trimpot uses -0.75*PI to 0.75*PI (~270 degrees).
**How to avoid:** Draw the indicator at the 12-o'clock position (pointing straight up) in the SVG. SvgKnob rotates from minAngle (CCW) to maxAngle (CW) around center. Match the rotation range to the existing knob behavior.
**Warning signs:** Indicator at wrong angle at min/max values.

## Code Examples

### Complete Widget Constructor Pattern (Target State)
```cpp
// Source: AnalogLFO.cpp AnalogLFOWidget — target after Phase 19
struct AnalogLFOWidget : ModuleWidget {
    AnalogLFOWidget(AnalogLFO* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/AnalogLFO.svg")));

        // Hex bolt screws (4 corners)
        addChild(createWidget<ForgeHexBolt>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ForgeHexBolt>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ForgeHexBolt>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ForgeHexBolt>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Waveform display (repositioned for 14HP)
        {
            WaveformDisplay* display = new WaveformDisplay();
            display->module = module;
            display->box.pos = mm2px(Vec(3.60f, 13.19f));
            display->box.size = mm2px(Vec(63.93f, 17.98f));
            addChild(display);
        }

        // Main knobs
        addParam(createParamCentered<ForgeKnobHero>(mm2px(Vec(35.56f, 47.35f)),
                 module, AnalogLFO::MORPH_PARAM));
        addParam(createParamCentered<ForgeKnobSecondary>(mm2px(Vec(21.18f, 67.32f)),
                 module, AnalogLFO::CHARACTER_PARAM));
        addParam(createParamCentered<ForgeKnobSecondary>(mm2px(Vec(49.94f, 67.32f)),
                 module, AnalogLFO::DRIFT_PARAM));
        addParam(createParamCentered<ForgeKnobUtility>(mm2px(Vec(21.18f, 83.51f)),
                 module, AnalogLFO::RATE_PARAM));
        addParam(createParamCentered<ForgeKnobUtility>(mm2px(Vec(49.94f, 83.51f)),
                 module, AnalogLFO::PHASE_OFFSET_PARAM));

        // CV trimpots
        addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(9.19f, 95.89f)),
                 module, AnalogLFO::MORPH_ATTEN_PARAM));
        addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(22.77f, 95.89f)),
                 module, AnalogLFO::CHARACTER_ATTEN_PARAM));
        addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(35.56f, 95.89f)),
                 module, AnalogLFO::DRIFT_ATTEN_PARAM));
        addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(48.75f, 95.89f)),
                 module, AnalogLFO::FM_ATTEN_PARAM));
        addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(61.93f, 95.89f)),
                 module, AnalogLFO::PHASE_OFFSET_ATTEN_PARAM));

        // CV input jacks
        addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(9.19f, 103.08f)),
                 module, AnalogLFO::MORPH_CV_INPUT));
        addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(22.77f, 103.08f)),
                 module, AnalogLFO::CHARACTER_CV_INPUT));
        addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(35.56f, 103.08f)),
                 module, AnalogLFO::DRIFT_CV_INPUT));
        addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(48.75f, 103.08f)),
                 module, AnalogLFO::FM_INPUT));
        addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(61.93f, 103.08f)),
                 module, AnalogLFO::PHASE_OFFSET_CV_INPUT));

        // Bottom I/O row
        addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(14.38f, 117.47f)),
                 module, AnalogLFO::CLK_INPUT));
        addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(35.56f, 117.47f)),
                 module, AnalogLFO::RESET_INPUT));
        addOutput(createOutputCentered<ForgeJackOutput>(mm2px(Vec(56.74f, 117.47f)),
                  module, AnalogLFO::OUTPUT));
    }
    // ... appendContextMenu unchanged
};
```

### nanosvg-Compatible Gradient SVG Pattern
```xml
<!-- Source: VCV Rack SDK nanosvg.h gradient parser (lines 2551-2619) -->
<!-- Gradients MUST be in <defs>. Max 2 stops per gradient. -->
<defs>
  <!-- Linear gradient for accent bar -->
  <linearGradient id="accent-bar" x1="0" y1="0" x2="1" y2="0"
                  gradientUnits="objectBoundingBox">
    <stop offset="0%" stop-color="#1a0800" stop-opacity="1"/>
    <stop offset="100%" stop-color="#e85d26" stop-opacity="1"/>
  </linearGradient>

  <!-- Radial gradient for knob curvature -->
  <radialGradient id="knob-curvature" cx="38%" cy="28%" r="60%">
    <stop offset="0%" stop-color="#4a4a4a"/>
    <stop offset="100%" stop-color="#0a0a0a"/>
  </radialGradient>

  <!-- Radial gradient for panel warmth -->
  <radialGradient id="panel-warmth" cx="50%" cy="35%" r="42%">
    <stop offset="0%" stop-color="#e85d26" stop-opacity="0.02"/>
    <stop offset="100%" stop-color="#e85d26" stop-opacity="0"/>
  </radialGradient>
</defs>
```

### Multi-Stop Gradient Simulation via Layering
```xml
<!-- Simulate 3-color metallic ring effect with two 2-stop gradients -->
<!-- Layer 1: bright center to mid-dark -->
<circle cx="8" cy="8" r="7" fill="none" stroke-width="1.5">
  <set attributeName="stroke" to="url(#ring-inner)"/>
</circle>
<!-- Layer 2: mid-dark to edge-dark (slightly smaller to overlap) -->
<circle cx="8" cy="8" r="5.5" fill="url(#body-gradient)"/>

<!-- Better approach: concentric circles with solid fills and opacity -->
<circle cx="8" cy="8" r="8.5" fill="#1a1a1a"/>           <!-- outer dark -->
<circle cx="8" cy="8" r="8.0" fill="#777777"/>            <!-- metallic ring bright -->
<circle cx="8" cy="8" r="7.5" fill="#555555"/>            <!-- metallic ring mid -->
<circle cx="8" cy="8" r="7.0" fill="#1a1a1a"/>            <!-- inner dark -->
<circle cx="8" cy="8" r="6.8" fill="url(#knob-curvature)"/> <!-- knob body -->
```

### Complete Component Position Map (mm)
```
Panel: 71.12mm x 128.5mm (14HP)
Conversion: mm = px * 0.199775

PARAMS:
  MORPH_PARAM              (35.56, 47.35) ForgeKnobHero      [center axis]
  CHARACTER_PARAM          (21.18, 67.32) ForgeKnobSecondary  [left column]
  DRIFT_PARAM              (49.94, 67.32) ForgeKnobSecondary  [right column]
  RATE_PARAM               (21.18, 83.51) ForgeKnobUtility    [left column]
  PHASE_OFFSET_PARAM       (49.94, 83.51) ForgeKnobUtility    [right column]
  MORPH_ATTEN_PARAM        ( 9.19, 95.89) ForgeTrimpot
  CHARACTER_ATTEN_PARAM    (22.77, 95.89) ForgeTrimpot
  DRIFT_ATTEN_PARAM        (35.56, 95.89) ForgeTrimpot
  FM_ATTEN_PARAM           (48.75, 95.89) ForgeTrimpot
  PHASE_OFFSET_ATTEN_PARAM (61.93, 95.89) ForgeTrimpot

INPUTS:
  MORPH_CV_INPUT           ( 9.19, 103.08) ForgeJackInput
  CHARACTER_CV_INPUT       (22.77, 103.08) ForgeJackInput
  DRIFT_CV_INPUT           (35.56, 103.08) ForgeJackInput
  FM_INPUT                 (48.75, 103.08) ForgeJackInput
  PHASE_OFFSET_CV_INPUT    (61.93, 103.08) ForgeJackInput
  CLK_INPUT                (14.38, 117.47) ForgeJackInput
  RESET_INPUT              (35.56, 117.47) ForgeJackInput

OUTPUTS:
  OUTPUT                   (56.74, 117.47) ForgeJackOutput

DISPLAY:
  WaveformDisplay pos      ( 3.60, 13.19) size (63.93, 17.98)

SCREWS (hex bolts):
  Top-left                 (RACK_GRID_WIDTH, 0)
  Top-right                (box.size.x - 2*RACK_GRID_WIDTH, 0)
  Bottom-left              (RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)
  Bottom-right             (box.size.x - 2*RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)
```

### Component SVG Dimension Reference
```
Hero knob SVG:       16.38mm x 16.38mm  (82px / 5.0056 px/mm)
Secondary knob SVG:  11.99mm x 11.99mm  (60px / 5.0056)
Utility knob SVG:     9.19mm x  9.19mm  (46px / 5.0056)
Trimpot SVG:          3.60mm x  3.60mm  (18px / 5.0056)
Input jack SVG:       7.19mm x  7.19mm  (36px / 5.0056)
Output jack SVG:      8.39mm x  8.39mm  (42px / 5.0056)
Hex bolt SVG:         3.20mm x  3.20mm  (16px / 5.0056)
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| No gradient support in nanosvg | 2-stop linear/radial gradients rendered | Rack 2.6.0 (Nov 2024) | Enables metallic, depth, and accent effects previously impossible |
| `SVGKnob` / `SVGPort` naming | `SvgKnob` / `SvgPort` | Rack 2.0 | Old names deprecated but still compile; use new names |
| `setSVG()` method | `setSvg()` method | Rack 2.0 | Old method deprecated; use new casing |

**Deprecated/outdated:**
- `SVGKnob`, `SVGPort`, `SVGScrew`: Use `SvgKnob`, `SvgPort`, `SvgScrew` (DEPRECATED typedef still works but prefer current names)
- `setSVG()`: Use `setSvg()` (DEPRECATED wrapper still works)

## Discretion Recommendations

### D-08: Knob Indicator Glow Approach
**Recommendation:** Solid ember line with semi-transparent halo circle. In SVG, render the indicator as two elements: (1) a narrow rect filled with linear gradient from ember to metallic, and (2) a wider semi-transparent ember circle at the indicator tip. The solid line provides readability; the halo adds the premium glow effect. This is achievable purely in SVG with `fill-opacity`.

### D-12: Emblem Symmetry Method
**Recommendation:** Manual duplication. The emblem is authored once, then manually mirror all x-coordinates about the center axis (35.56mm). For each left-side element at x_left, create a corresponding element at x_right = 71.12 - x_left. This avoids build tooling complexity for a one-time operation. The number of emblem elements (~30-40 paths/circles) is manageable.

### D-13: Forge Rune Detail Level
**Recommendation:** Full rune with concentric power rings at decreasing opacity. The HTML rune has clear geometric structure (diamonds, lines, circles) that translates directly to nanosvg. Skip only the Gaussian blur filter -- use wider strokes at lower opacity to approximate the glow aura. The power rings (simple circles) add atmospheric depth at almost zero rendering cost.

### Font Glyph to SVG Path Conversion
**Recommendation:** Use Inkscape for conversion. Workflow: (1) Create text elements using the target fonts (FoundationLogo, Bebas Neue, Chakra Petch, JetBrains Mono). (2) Apply "Object to Path" in Inkscape. (3) Extract the `d` attribute from resulting `<path>` elements. (4) Position and scale in the panel SVG. Alternative: The existing AnalogLFO.svg already has hand-crafted geometric block letter paths for "FORGE AUDIO" and "ANALOG LFO" -- these can be refined/extended rather than starting from scratch. For smaller labels (MORPH, CHARACTER, etc.) the geometric approach of the existing panel may be sufficient.

### Machined Groove Texture
**Recommendation:** Concentric circles with alternating very-low-opacity strokes. In the HTML mockup, grooves use nested `box-shadow` with alternating light/dark at 0.01-0.025 opacity. In SVG, approximate with 3-4 concentric `<circle>` elements inside the knob body at `stroke-opacity="0.02"` to `stroke-opacity="0.03"`. At module scale these provide subtle texture without overwhelming the gradient depth effect.

### SVG File Organization
**Recommendation:** `res/components/` subdirectory for all component SVGs. Panel SVG stays at `res/AnalogLFO.svg` (replacing the old one). Component naming follows `Forge{Type}{Variant}.svg` convention. Background/shadow layers use `_bg.svg` suffix per VCV convention.

## Open Questions

1. **minRackVersion in plugin.json**
   - What we know: Using 2-stop gradients requires Rack 2.6.0+. Current plugin.json has no `minRackVersion` field.
   - What's unclear: Whether omitting minRackVersion causes issues on older Rack installs (likely just fails to render gradients gracefully).
   - Recommendation: Add `"minRackVersion": "2.6.0"` to plugin.json to prevent loading on incompatible Rack versions.

2. **Exact font glyph path data**
   - What we know: FoundationLogo font file exists at project root (`FoundationLogo.ttf`). Bebas Neue and other fonts are Google Fonts. The current panel already has geometric path letters.
   - What's unclear: Whether the existing geometric letter paths match the FoundationLogo font closely enough, or if proper font-to-path conversion will produce noticeably different results.
   - Recommendation: Start with Inkscape font-to-path conversion for the brand text (FORGE, AUDIO) using FoundationLogo. If the result is too complex for nanosvg, fall back to the existing geometric block letter approach refined to match the font's character.

3. **Knob background SVG sizing**
   - What we know: The bg SVG renders statically behind the rotating knob. It typically includes the shadow and any outer ring.
   - What's unclear: Whether the bg SVG must be the same dimensions as the main knob SVG, or if it can be larger (to extend the shadow beyond the knob body).
   - Recommendation: Make bg SVG slightly larger than the knob body to accommodate shadow and outer ring effects. The SvgKnob class positions bg relative to the transform widget, so oversized bg will extend outward naturally. Test with a simple prototype first.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| VCV Rack 2 SDK | Build system, all widget headers | Checked | Present at ../Rack-SDK | -- |
| C++17 compiler | Custom widget structs | Checked | Via Makefile/plugin.mk | -- |
| make | Build (`make && make install`) | Checked | Standard macOS | -- |
| Inkscape | Font-to-path conversion (optional) | Not checked | -- | Hand-craft paths or use FontForge CLI |
| FoundationLogo.ttf | Brand text paths | Checked | Present at project root | -- |

**Missing dependencies with no fallback:**
- None -- all critical dependencies are available.

**Missing dependencies with fallback:**
- Inkscape (for font-to-path conversion): Can use FontForge CLI, or online converters, or refine existing hand-crafted paths.

## Sources

### Primary (HIGH confidence)
- VCV Rack SDK headers: `componentlibrary.hpp`, `SvgKnob.hpp`, `SvgPort.hpp`, `SvgScrew.hpp` -- Verified widget class hierarchy, setSvg() API, bg layer pattern
- VCV Rack SDK `nanosvg.h` (lines 2551-2898) -- Verified gradient parsing: `<defs>`, `<linearGradient>`, `<radialGradient>`, stop-color, stop-opacity, gradientTransform, gradientUnits all supported
- VCV Rack 2.6.0 CHANGELOG -- "Render 2-stop linear/radial gradients with any stop offsets and transformations in SVG" ([GitHub CHANGELOG](https://raw.githubusercontent.com/VCVRack/Rack/v2/CHANGELOG.md))
- Existing `res/AnalogLFO.svg` (331 lines) -- Current panel structure, path letterform patterns, nanosvg-compatible SVG conventions
- `forge-noir.html` (893 lines) -- Canonical layout reference with all pixel positions
- `DESIGN-LANGUAGE.md` (274 lines) -- Complete color palette, typography, component specs, layout rules

### Secondary (MEDIUM confidence)
- [VCV Rack Manual - Panel Guide](https://vcvrack.com/manual/Panel) -- Confirms text-to-path requirement, mm dimensions, component layer conventions
- [VCV Community - SVG transparencies and gradients](https://community.vcvrack.com/t/svg-transparencies-and-gradients/16833) -- Confirms 2-stop limit, radial gradient circular-only limitation
- [VCV Community - Custom knob SVGs](https://community.vcvrack.com/t/how-can-i-implement-custom-knob-svgs/21399) -- Confirms subclass-and-load-SVG pattern

### Tertiary (LOW confidence)
- None -- all findings verified against SDK source code.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- Verified against SDK headers and source code directly
- Architecture: HIGH -- Widget patterns confirmed in componentlibrary.hpp; coordinate conversion verified mathematically
- Pitfalls: HIGH -- nanosvg constraints verified by reading the parser source code; gradient limits confirmed by changelog + community
- Coordinate mapping: HIGH -- All 18 controls mapped with computed mm values; conversion factor mathematically derived from panel dimensions

**Research date:** 2026-03-29
**Valid until:** 2026-04-28 (stable -- VCV Rack SDK evolves slowly, nanosvg constraints unlikely to change)
