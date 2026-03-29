# Phase 19: Forge Noir Panel + Custom Components - Research

**Researched:** 2026-03-29
**Domain:** VCV Rack 2 SVG panel rendering, nanosvg constraints, custom widget C++ classes
**Confidence:** HIGH

## Summary

Phase 19 replaces the existing 12HP panel with a 14HP Forge Noir panel featuring custom SVG components. The core technical challenge is translating the HTML/CSS mockup's rich visual effects (box-shadows, conic gradients, multi-layer radials, blur filters) into nanosvg-compatible SVG that maintains the design's premium feel. The VCV Rack SDK provides clean base classes (`SvgKnob`, `SvgPort`, `SvgScrew`) that accept custom SVG files -- the pattern is to define structs that subclass these, call `setSvg()` with custom artwork, and use `createParamCentered<CustomWidget>()` in the widget constructor.

Rack 2.6.0 (Nov 2024) added 2-stop linear/radial gradient rendering, confirmed by both the changelog and the nanosvg parser source in the SDK. This is the critical enabler for metallic curvature, accent bar gradients, and knob depth effects. Gradients are defined in `<defs>` blocks (nanosvg DOES parse `<defs>` for gradients, despite not supporting `<use>`/`<defs>` for element cloning). Each gradient is limited to exactly 2 stops. Multiple overlapping shapes with different 2-stop gradients can simulate multi-stop effects.

The coordinate conversion from the forge-noir.html mockup (356px wide at 5x) to panel mm is: `mm = px * (71.12 / 356)`, factor 0.199775. All 18 controls have been mapped from mockup pixel positions to mm coordinates. A complete UI-SPEC (19-UI-SPEC.md) has been generated and checker-approved, documenting the rendering contracts, component inventory, and position map in detail.

**Primary recommendation:** Create the panel SVG and 12 component SVGs (3 knob sizes with bg layers, 1 trimpot with bg layer, 2 jack sizes, 1 hex bolt screw, and the panel itself) using hand-authored nanosvg-compatible SVG with 2-stop gradients for depth, then wire them into custom C++ widget structs that subclass the standard VCV base classes. Disable the built-in CircularShadow (`shadow->opacity = 0.0;`) when providing custom shadow artwork in the bg SVG.

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
| PANEL-03 | Custom SVG knob components (3 sizes: hero/secondary/utility) with machined metal appearance | SvgKnob base class with bg layer confirmed; struct pattern: subclass SvgKnob, set minAngle/maxAngle, add bg SvgWidget, call setSvg() with custom SVG, disable built-in CircularShadow |
| PANEL-04 | Custom SVG trimpot components (bright scalloped attenuverters) | Trimpot base class confirmed: subclasses SvgKnob, has bg layer, minAngle=-0.75*PI, maxAngle=0.75*PI |
| PANEL-05 | Custom SVG jack components (2 sizes: standard/output with ember accent ring) | SvgPort base class confirmed; output ember ring baked into output jack SVG per D-09; disable built-in CircularShadow if providing custom shadow |
| PANEL-06 | Forge emblem background element | Baked into panel SVG as low-opacity paths; symmetry via manual duplication (nanosvg has no `<use>` cloning); forge-noir.html emblem source has 3 gradient stops in radial -- must simplify to 2-stop for nanosvg |
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
    ForgeKnobHero.svg        # 82px hero knob (MORPH) -- rotating layer
    ForgeKnobHero_bg.svg     # Hero knob background/shadow/metallic ring -- static layer
    ForgeKnobSecondary.svg   # 60px secondary knob (CHARACTER, DRIFT)
    ForgeKnobSecondary_bg.svg
    ForgeKnobUtility.svg     # 46px utility knob (RATE, PHASE)
    ForgeKnobUtility_bg.svg
    ForgeTrimpot.svg          # 18px bright scalloped trimpot -- rotating layer
    ForgeTrimpot_bg.svg       # Trimpot shadow ring -- static layer
    ForgeJackInput.svg        # 36px standard input jack
    ForgeJackOutput.svg       # 42px output jack with ember ring baked in
    ForgeHexBolt.svg          # 16px hexagonal bolt (screw replacement)
  PANEL-SPEC.md               # Updated for 14HP Forge Noir (replaces old 12HP spec)
src/
  AnalogLFO.cpp               # Widget struct definitions + updated constructor
```

**Recommendation on SVG file organization:** Separate component SVGs in a `res/components/` subdirectory. The panel SVG stays at `res/AnalogLFO.svg` to maintain the existing `setPanel()` asset path.

### Pattern 1: Custom Knob Widget Struct
**What:** Subclass SvgKnob (via a RoundKnob-like intermediate) to load custom SVG skins with background layer
**When to use:** All three knob sizes
**Example:**
```cpp
// Source: componentlibrary.hpp RoundKnob/RoundBigBlackKnob pattern
struct ForgeKnobHero : app::SvgKnob {
    widget::SvgWidget* bg;

    ForgeKnobHero() {
        minAngle = -0.83 * M_PI;
        maxAngle = 0.83 * M_PI;

        // CRITICAL: Disable built-in CircularShadow -- we provide custom shadow in bg SVG
        shadow->opacity = 0.0;

        bg = new widget::SvgWidget;
        fb->addChildBelow(bg, tw);

        setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobHero.svg")));
        bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobHero_bg.svg")));
    }
};
```

**Why shadow->opacity = 0.0:** SvgKnob creates a built-in `CircularShadow* shadow` member rendered via NanoVG (a simple dark circle). When providing a custom shadow via the bg SVG layer, the built-in shadow must be disabled to prevent visual doubling. This pattern is used by NKK, CKSS, and other SDK components (componentlibrary.hpp:801,810,818).

### Pattern 2: Custom Port Widget Struct
**What:** Subclass SvgPort to load custom jack SVG
**When to use:** Both input and output jack types
**Example:**
```cpp
// Source: componentlibrary.hpp PJ301MPort pattern
struct ForgeJackInput : app::SvgPort {
    ForgeJackInput() {
        // Disable built-in shadow -- jack SVG includes its own depth effects
        shadow->opacity = 0.0;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeJackInput.svg")));
    }
};

struct ForgeJackOutput : app::SvgPort {
    ForgeJackOutput() {
        shadow->opacity = 0.0;
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeJackOutput.svg")));
    }
};
```

**Note:** SvgPort also has a `CircularShadow* shadow` member (SvgPort.hpp:16). The stock PJ301MPort does NOT disable it, so it gets the default NanoVG shadow. For Forge components with custom depth rendering in the SVG, disable it.

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

        shadow->opacity = 0.0;

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

**Note:** SvgScrew does NOT have a CircularShadow member (SvgScrew.hpp) -- no shadow to disable.

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

**Important:** The HTML uses `transform: translate(-50%, -50%)` on knobs/jacks, meaning the CSS `left` and `top` values ARE center coordinates. No additional offset needed.

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
  <!-- Knob body with radial gradient -->
  <circle cx="8.19" cy="8.19" r="7.5" fill="url(#knob-body)"/>
  <!-- Center cap indent -->
  <circle cx="8.19" cy="8.19" r="1.8" fill="#080808"/>
  <!-- Machined groove texture (very subtle) -->
  <circle cx="8.19" cy="8.19" r="6.0" fill="none" stroke="#ffffff" stroke-width="0.1" stroke-opacity="0.02"/>
  <circle cx="8.19" cy="8.19" r="5.0" fill="none" stroke="#000000" stroke-width="0.1" stroke-opacity="0.03"/>
  <circle cx="8.19" cy="8.19" r="4.0" fill="none" stroke="#ffffff" stroke-width="0.1" stroke-opacity="0.02"/>
  <!-- Indicator line at 12-o'clock (SvgKnob rotates this) -->
  <rect x="7.69" y="1" width="1" height="5" rx="0.3"
        fill="#e85d26" opacity="0.9"/>
  <!-- Indicator glow halo at tip -->
  <circle cx="8.19" cy="1.5" r="1.2" fill="#e85d26" opacity="0.25"/>
</svg>
```

### Pattern 7: Panel SVG Declaration
**What:** SVG document structure for the 14HP panel
**When to use:** res/AnalogLFO.svg
**Example:**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg"
     width="71.12mm" height="128.5mm"
     viewBox="0 0 71.12 128.5">
  <!-- Forge Noir 14HP panel -->
  <defs>
    <!-- Gradients for accent bars, panel warmth, etc. -->
    <linearGradient id="accent-left" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#1a0800"/>
      <stop offset="100%" stop-color="#e85d26"/>
    </linearGradient>
    <linearGradient id="accent-right" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0%" stop-color="#e85d26"/>
      <stop offset="100%" stop-color="#1a0800"/>
    </linearGradient>
    <radialGradient id="panel-warmth" cx="50%" cy="35%" r="42%">
      <stop offset="0%" stop-color="#e85d26" stop-opacity="0.02"/>
      <stop offset="100%" stop-color="#e85d26" stop-opacity="0"/>
    </radialGradient>
  </defs>

  <!-- Panel background -->
  <rect x="0" y="0" width="71.12" height="128.5" fill="#0c0c0c"/>

  <!-- Panel border -->
  <rect x="0.15" y="0.15" width="70.82" height="128.2"
        fill="none" stroke="#1a1a1a" stroke-width="0.3"/>

  <!-- Accent bars (two rects per bar for 2-stop gradient approximation) -->
  <rect x="0" y="0" width="35.56" height="0.6" fill="url(#accent-left)"/>
  <rect x="35.56" y="0" width="35.56" height="0.6" fill="url(#accent-right)"/>

  <!-- ... emblem, decorations, labels, component layer ... -->

  <!-- Hidden component placement layer -->
  <g id="components" style="display:none">
    <circle cx="35.56" cy="47.35" r="8.19" fill="#ff0000"/>  <!-- MORPH -->
    <!-- ... all control positions ... -->
  </g>
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
- **Leaving CircularShadow enabled with custom bg shadows:** Creates doubled shadow artifacts. Always set `shadow->opacity = 0.0;` when providing custom shadow artwork.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| SVG knob rotation | Custom rotation code | SvgKnob base class with TransformWidget | SvgKnob handles rotation math, framebuffer caching, minAngle/maxAngle automatically |
| Knob shadow | NanoVG shadow overlay | Separate _bg.svg with shadow circle (and `shadow->opacity = 0.0;`) | SvgKnob.bg layer renders behind the rotating knob body -- standard VCV pattern |
| Panel loading | Custom SVG parsing | `setPanel(createPanel(asset::plugin(...)))` | Handles SVG-to-panel pipeline, dark mode, dimensions |
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
**How to avoid:** Main SVG = knob body + indicator line (rotates). Bg SVG = shadow circle + static outer metallic ring (stays put). The `fb->addChildBelow(bg, tw)` call ensures bg renders below the transform widget.
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

### Pitfall 9: CircularShadow Doubling
**What goes wrong:** Components appear to have an extra blurry dark circle behind them, creating visual artifacts or muddying the carefully designed bg SVG.
**Why it happens:** SvgKnob and SvgPort both create a `CircularShadow* shadow` member (NanoVG-rendered) by default. If you also provide a shadow in the bg SVG, both render -- doubling the shadow effect.
**How to avoid:** Set `shadow->opacity = 0.0;` in every custom widget struct constructor. This is standard practice in the SDK for components with custom shadow artwork (see NKK, CKSS structs in componentlibrary.hpp).
**Warning signs:** Components look like they float higher than expected, or have a dark halo around them.

### Pitfall 10: Emblem 3-Stop Gradient in Source HTML
**What goes wrong:** The forge-noir.html emblem uses a 3-stop radial gradient (`forge-core` has stops at 0%, 45%, 100%). Direct copy into panel SVG breaks because nanosvg only supports 2 stops.
**Why it happens:** The HTML reference is designed for browser rendering with no gradient stop limits.
**How to avoid:** Simplify the `forge-core` gradient to 2 stops (e.g., ember at center opacity 0.2 to transparent at edge). For a closer approximation, layer two ellipses with separate 2-stop gradients -- one for the inner-to-mid fade, one for the mid-to-outer fade.
**Warning signs:** Emblem glow is either a hard-edged spot or completely invisible.

## Code Examples

### Complete Widget Constructor Pattern (Target State)
```cpp
// Source: AnalogLFO.cpp AnalogLFOWidget -- target after Phase 19
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
<!-- Source: VCV Rack SDK nanosvg.h gradient parser -->
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
<!-- Simulate multi-band metallic ring with concentric solid circles -->
<!-- Better than 2-stop gradient layering for this use case -->
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
Hero knob bg SVG:    ~18.00mm x 18.00mm (90px -- slightly larger for shadow + ring)
Secondary knob SVG:  11.99mm x 11.99mm  (60px / 5.0056)
Secondary knob bg:   ~13.50mm x 13.50mm (68px)
Utility knob SVG:     9.19mm x  9.19mm  (46px / 5.0056)
Utility knob bg:     ~10.50mm x 10.50mm (53px)
Trimpot SVG:          3.60mm x  3.60mm  (18px / 5.0056)
Trimpot bg SVG:      ~4.20mm x  4.20mm  (21px)
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
**Recommendation:** Solid ember line with semi-transparent halo circle. In SVG, render the indicator as two elements: (1) a narrow rect filled with linear gradient from `#f08040` (outer/top) to `#c04020` (inner/bottom), and (2) a semi-transparent ember circle at the indicator tip (r=1.2mm hero, 0.9mm secondary, 0.7mm utility, fill `#e85d26`, opacity 0.25). The solid line provides readability; the halo adds the premium glow effect. This is achievable purely in SVG with `fill-opacity` and already documented in the UI-SPEC.

### D-12: Emblem Symmetry Method
**Recommendation:** Manual duplication. The emblem is authored once, then manually mirror all x-coordinates about the center axis (35.56mm). For each left-side element at x_left, create a corresponding element at x_right = 71.12 - x_left. For paths, also invert horizontal control point directions. This avoids build tooling complexity for a one-time operation. The number of emblem elements (~30-40 paths/circles) is manageable.

### D-13: Forge Rune Detail Level
**Recommendation:** Full rune with concentric power rings at decreasing opacity. The HTML rune has clear geometric structure (diamonds, lines, circles) that translates directly to nanosvg. Skip only the Gaussian blur filter and the `<use>` element -- use wider strokes at lower opacity to approximate the glow aura. The power rings (simple circles with stroke) add atmospheric depth at almost zero rendering cost.

### Font Glyph to SVG Path Conversion
**Recommendation:** Use Inkscape for conversion. Workflow: (1) Create text elements using the target fonts (FoundationLogo, Bebas Neue, Chakra Petch, JetBrains Mono). (2) Apply "Object to Path" in Inkscape. (3) Extract the `d` attribute from resulting `<path>` elements. (4) Position and scale in the panel SVG. Alternative: The existing AnalogLFO.svg already has hand-crafted geometric block letter paths for "FORGE AUDIO" and "ANALOG LFO" -- these can be refined/extended rather than starting from scratch. For smaller labels (MORPH, CHARACTER, etc.) the geometric approach of the existing panel may be sufficient since these appear at tiny sizes.

### Machined Groove Texture
**Recommendation:** Concentric circles with alternating very-low-opacity strokes. In the HTML mockup, grooves use nested `box-shadow` with alternating light/dark at 0.01-0.025 opacity. In SVG, approximate with 3-4 concentric `<circle>` elements inside the knob body at `stroke-opacity="0.02"` to `stroke-opacity="0.03"`, alternating `#ffffff` and `#000000` strokes. At module scale these provide subtle texture without overwhelming the gradient depth effect.

### SVG File Organization
**Recommendation:** `res/components/` subdirectory for all component SVGs. Panel SVG stays at `res/AnalogLFO.svg` (replacing the old one). Component naming follows `Forge{Type}{Variant}.svg` convention. Background/shadow layers use `_bg.svg` suffix per VCV convention. Total: 12 SVG files (1 panel + 11 component files).

## Open Questions

1. **minRackVersion in plugin.json**
   - What we know: Using 2-stop gradients requires Rack 2.6.0+. Current plugin.json has no `minRackVersion` field (verified).
   - What's unclear: Whether omitting minRackVersion causes issues on older Rack installs (likely just fails to render gradients gracefully).
   - Recommendation: Add `"minRackVersion": "2.6.0"` to plugin.json to prevent loading on incompatible Rack versions.

2. **Exact font glyph path data**
   - What we know: FoundationLogo font file exists at project root (`FoundationLogo.ttf`). Bebas Neue and other fonts are Google Fonts. The current panel already has geometric path letters.
   - What's unclear: Whether the existing geometric letter paths match the FoundationLogo font closely enough, or if proper font-to-path conversion will produce noticeably different results.
   - Recommendation: Start with Inkscape font-to-path conversion for the brand text (FORGE, AUDIO) using FoundationLogo. If the result is too complex for nanosvg, fall back to the existing geometric block letter approach refined to match the font's character. For all other labels, adapt and extend the existing geometric block letter approach.

3. **Knob background SVG sizing**
   - What we know: The bg SVG renders statically behind the rotating knob. It includes the custom shadow and outer metallic ring. The UI-SPEC specifies bg SVG dimensions as slightly larger than the main SVG.
   - What's unclear: The exact positioning behavior when bg SVG and main SVG have different dimensions.
   - Recommendation: Make bg SVG slightly larger than the knob body per the UI-SPEC dimensions. The SvgWidget is positioned relative to the framebuffer, and `fb->addChildBelow(bg, tw)` places it behind the transform widget. Test with a simple prototype. If centering is off, adjust bg SVG viewport or add offset in the constructor.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| VCV Rack 2 SDK | Build system, all widget headers | Present | ../Rack-SDK | -- |
| C++17 compiler | Custom widget structs | Present | Via Makefile/plugin.mk | -- |
| make | Build (`make && make install`) | Present | Standard macOS | -- |
| Inkscape | Font-to-path conversion (optional) | Not checked | -- | Hand-craft paths or use FontForge CLI |
| FoundationLogo.ttf | Brand text paths | Present | Project root | -- |

**Missing dependencies with no fallback:**
- None -- all critical dependencies are available.

**Missing dependencies with fallback:**
- Inkscape (for font-to-path conversion): Can use FontForge CLI, online converters, or refine existing hand-crafted paths.

## Validation Architecture

### Build Validation
| Test | Command | Pass Criteria |
|------|---------|---------------|
| Build succeeds | `make` | Exit code 0, no errors |
| Plugin loads | `make install` + open VCV Rack | Module appears in module browser |

### File Existence Checks
| Assertion | Command | Covers |
|-----------|---------|--------|
| Panel SVG exists with 14HP dimensions | `grep -q 'width="71.12mm"' res/AnalogLFO.svg && grep -q 'height="128.5mm"' res/AnalogLFO.svg` | PANEL-01 |
| Panel background color is near-black | `grep -q '#0c0c0c' res/AnalogLFO.svg` | PANEL-01 |
| Accent bar gradient present | `grep -q 'linearGradient' res/AnalogLFO.svg && grep -q '#e85d26' res/AnalogLFO.svg` | PANEL-01 |
| All component SVG files exist | `ls res/components/ForgeKnobHero.svg res/components/ForgeKnobHero_bg.svg res/components/ForgeKnobSecondary.svg res/components/ForgeKnobSecondary_bg.svg res/components/ForgeKnobUtility.svg res/components/ForgeKnobUtility_bg.svg res/components/ForgeTrimpot.svg res/components/ForgeTrimpot_bg.svg res/components/ForgeJackInput.svg res/components/ForgeJackOutput.svg res/components/ForgeHexBolt.svg` | PANEL-03, PANEL-04, PANEL-05 |
| No `<text>` elements in any SVG | `grep -rL '<text' res/AnalogLFO.svg res/components/*.svg` should find nothing | PANEL-07 |
| No `<style>` blocks in any SVG | `! grep -rl '<style' res/AnalogLFO.svg res/components/*.svg` | nanosvg compliance |
| No `<use>` element cloning | `! grep -rl '<use ' res/AnalogLFO.svg res/components/*.svg` | nanosvg compliance |
| No 3+ stop gradients | Each gradient block has exactly 2 `<stop>` elements | nanosvg compliance |
| minRackVersion set to 2.6.0 | `grep -q '"minRackVersion".*"2.6.0"' plugin.json` | PANEL-01 |

### C++ Code Assertions
| Assertion | Grep Pattern | Covers |
|-----------|-------------|--------|
| All 10 params registered | Count `addParam(` calls = 10 (5 knobs + 5 trimpots) | PANEL-02 |
| All 7 inputs registered | Count `addInput(` calls = 7 | PANEL-02 |
| 1 output registered | Count `addOutput(` calls = 1 | PANEL-02 |
| ForgeKnobHero used for MORPH | `grep 'ForgeKnobHero.*MORPH_PARAM'` | PANEL-03 |
| ForgeKnobSecondary used for CHAR/DRIFT | `grep 'ForgeKnobSecondary.*CHARACTER_PARAM\|DRIFT_PARAM'` | PANEL-03 |
| ForgeKnobUtility used for RATE/PHASE | `grep 'ForgeKnobUtility.*RATE_PARAM\|PHASE_OFFSET_PARAM'` | PANEL-03 |
| ForgeTrimpot used for all 5 atten | Count `ForgeTrimpot` in addParam calls = 5 | PANEL-04 |
| ForgeJackInput for inputs | `grep 'ForgeJackInput.*INPUT'` | PANEL-05 |
| ForgeJackOutput for output | `grep 'ForgeJackOutput.*OUTPUT'` | PANEL-05 |
| ForgeHexBolt for all 4 screws | Count `ForgeHexBolt` = 4 | PANEL-01 |
| shadow->opacity = 0.0 in all custom widgets | `grep 'shadow->opacity' src/AnalogLFO.cpp` | Pitfall 9 |

### SVG Content Validation
| Assertion | What to Check | Covers |
|-----------|--------------|--------|
| 3 knob sizes are distinct | Hero SVG viewBox width > Secondary > Utility | PANEL-03 |
| Output jack has ember accent ring | `grep -q '#e85d26' res/components/ForgeJackOutput.svg` | PANEL-05 |
| Input jack does NOT have ember ring | Output-specific stroke not present in ForgeJackInput.svg | PANEL-05 |
| Trimpot is bright (contrast with knobs) | Trimpot SVG has brighter gradient stops (#6a6a6a) vs knob (#4a4a4a) | PANEL-04 |
| Emblem elements present in panel SVG | `grep -c 'opacity="0.0[0-9]' res/AnalogLFO.svg` > 10 (many low-opacity elements) | PANEL-06 |
| Brand text paths present | `grep -c '<path' res/AnalogLFO.svg` > 20 (many path elements for text) | PANEL-07 |
| Component layer present | `grep -q 'id="components"' res/AnalogLFO.svg` | PANEL-02 |

### Visual Validation (Manual -- Build + Load in Rack)
| Assertion | What to Look For | Covers |
|-----------|-----------------|--------|
| No black rectangles | Module renders without missing elements | PANEL-01 |
| Near-black background visible | Panel is dark but not pure black | PANEL-01 |
| Ember accent bars at top/bottom | Orange gradient bars at panel edges | PANEL-01 |
| Hex bolts in 4 corners | Hexagonal bolt heads, not round screws | PANEL-01 |
| MORPH knob largest | Visual size hierarchy: MORPH > CHAR/DRIFT > RATE/PHASE | PANEL-03 |
| Knobs rotate with drag | Indicator moves with mouse drag | PANEL-03 |
| Trimpots brighter than knobs | Visual contrast between trimpot and knob bodies | PANEL-04 |
| Output jack has orange ring | Only the output jack has the ember accent | PANEL-05 |
| Output jack larger than inputs | Visible size difference | PANEL-05 |
| Forge emblem subtle behind knobs | Faint atmospheric pattern, not distracting | PANEL-06 |
| Brand text readable | "FORGE" and "AUDIO" in ember, "ANALOG LFO" in warm white | PANEL-07 |
| All controls interactive | Every knob, trimpot, and jack responds to interaction | PANEL-02 |
| Morph arc visible around MORPH | Dashed decorative circle around hero knob | PANEL-03 |
| Rune glyph between FORGE/AUDIO | Diamond shape with center glow | PANEL-07 |

### Interaction Validation
| Assertion | How to Test | Covers |
|-----------|-----------|--------|
| Knob rotation range correct | Drag MORPH from min to max: ~300 degrees sweep | PANEL-03 |
| Trimpot rotation range correct | Drag trimpot from min to max: ~270 degrees | PANEL-04 |
| Cables connect to jacks | Drag cable from any jack: cable attaches | PANEL-05 |
| Module width is 14HP | Module occupies 14HP width in rack | PANEL-01 |
| Context menu still works | Right-click module: Swing submenu appears | PANEL-02 (no regression) |

### Requirement Coverage Summary

| Req ID | Build Checks | Code Checks | Visual Checks | Status |
|--------|-------------|-------------|---------------|--------|
| PANEL-01 | SVG dimensions, bg color, gradients, minRackVersion | ForgeHexBolt count = 4 | No black rects, accent bars, hex bolts | Fully covered |
| PANEL-02 | -- | param/input/output counts, component layer | All controls interactive | Fully covered |
| PANEL-03 | Component SVGs exist | Widget type per param | 3 distinct sizes, rotation works, morph arc | Fully covered |
| PANEL-04 | Trimpot SVGs exist | ForgeTrimpot count = 5 | Bright metallic body, rotation range | Fully covered |
| PANEL-05 | Jack SVGs exist | Widget types for I/O | Ember ring on output only, size difference | Fully covered |
| PANEL-06 | Low-opacity element count in panel SVG | -- | Subtle atmospheric emblem | Fully covered |
| PANEL-07 | No `<text>` elements, path count | -- | Brand text readable | Fully covered |

## Sources

### Primary (HIGH confidence)
- VCV Rack SDK headers: `componentlibrary.hpp` (RoundKnob line 280, Trimpot line 637, PJ301MPort line 757, ScrewSilver line 965, shadow->opacity pattern lines 801+), `SvgKnob.hpp` (CircularShadow member line 17), `SvgPort.hpp` (CircularShadow member line 16), `SvgScrew.hpp` (no shadow member), `Knob.hpp` (minAngle/maxAngle lines 33-34), `CircularShadow.hpp` -- Verified widget class hierarchy, setSvg() API, bg layer pattern, shadow management
- VCV Rack SDK `nanosvg.h` -- Verified gradient parsing: `<defs>`, `<linearGradient>`, `<radialGradient>`, stop-color, stop-opacity, gradientTransform, gradientUnits all supported; 2-stop limit
- VCV Rack 2.6.0 CHANGELOG -- "Render 2-stop linear/radial gradients with any stop offsets and transformations in SVG"
- Existing `res/AnalogLFO.svg` -- Current 12HP panel structure, path letterform patterns, nanosvg-compatible SVG conventions
- `forge-noir.html` (893 lines) -- Canonical layout reference with all pixel positions, verified coordinate mappings
- `DESIGN-LANGUAGE.md` (274 lines) -- Complete color palette, typography, component specs, layout rules
- `19-UI-SPEC.md` (473 lines) -- Checker-approved visual contract with component rendering details, position map, validation criteria
- `plugin.json` -- Verified no minRackVersion field currently set; must add "2.6.0"

### Secondary (MEDIUM confidence)
- [VCV Rack Manual - Panel Guide](https://vcvrack.com/manual/Panel) -- Confirms text-to-path requirement, mm dimensions, component layer conventions
- [VCV Community - SVG transparencies and gradients](https://community.vcvrack.com/t/svg-transparencies-and-gradients/16833) -- Confirms 2-stop limit, radial gradient circular-only limitation
- [VCV Community - Custom knob SVGs](https://community.vcvrack.com/t/how-can-i-implement-custom-knob-svgs/21399) -- Confirms subclass-and-load-SVG pattern

### Tertiary (LOW confidence)
- None -- all findings verified against SDK source code.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- Verified against SDK headers and source code directly
- Architecture: HIGH -- Widget patterns confirmed in componentlibrary.hpp; CircularShadow handling verified; coordinate conversion verified mathematically
- Pitfalls: HIGH -- nanosvg constraints verified by reading the parser source code; gradient limits confirmed by changelog + community; shadow doubling verified by reading SvgKnob.hpp and componentlibrary.hpp
- Coordinate mapping: HIGH -- All 18 controls mapped with computed mm values; conversion factor mathematically derived from panel dimensions; verified against UI-SPEC

**Research date:** 2026-03-29
**Valid until:** 2026-04-28 (stable -- VCV Rack SDK evolves slowly, nanosvg constraints unlikely to change)
