# Stack Research: v1.3 Forge Noir

**Domain:** VCV Rack 2 module development -- adding PWM morph extension, Forge Noir SVG panel, display layout changes, and animated sync badge to existing C++ LFO module
**Researched:** 2026-03-27
**Confidence:** HIGH (validated against existing codebase, VCV Rack SDK docs, nanosvg/NanoVG references)

---

## What Already Exists (DO NOT ADD)

The v1.2 codebase already has everything needed for core DSP and rendering. No new libraries, frameworks, or dependencies are required for v1.3. The existing stack is:

| Technology | Purpose | Status |
|------------|---------|--------|
| VCV Rack 2 SDK (~2.6) | Framework, build system, component library | In place |
| C++17 | Module DSP + widget code | In place |
| NanoVG (bundled in SDK) | Real-time display rendering | In place |
| nanosvg (bundled in SDK) | Panel SVG parsing/rasterization | In place |
| Standard VCV Makefile + plugin.mk | Build system | In place |

**No new dependencies. No new libraries. No new build system changes.** The v1.3 milestone is purely implementation work within the existing stack.

---

## Stack Requirements Per Feature

### 1. PWM as Morph Extension

**What's needed:** Pure C++ math. No new dependencies.

**Implementation stack:**

| Component | What | Why |
|-----------|------|-----|
| `computePulse()` function | New waveform generator using phase comparison against variable duty cycle | Extends the morph sweep from 4 shapes (sine/tri/saw/square) to 5 (+ narrow pulse) |
| Morph scaling change | `morph * 4.f` instead of `morph * 3.f`, 4 segments instead of 3 | Adds a fifth position at morph=1.0 while preserving existing waveform positions at 0.0, 0.25, 0.5, 0.75 |
| `tanh()` edge softening | Same sigmoid technique already used in `computeSquare()` | Consistent analog character modeling; reuse existing pattern |
| Bleed ring expansion | `shapes[5]` wrapping ring instead of `shapes[4]` | Waveform bleed (CHAR-05) needs to include pulse as a neighbor of square and wrap back to sine |

**Key technical detail:** The square-to-pulse morph is a continuous duty cycle narrowing. At morph=0.75 (square), duty is 50%. At morph=1.0 (narrow pulse), duty narrows to ~10-15%. The `character` knob should apply the same type of analog deformation (edge softening, duty asymmetry with component spread) as it does for square.

**What NOT to add:**
- No separate PWM knob or CV input. The morph knob IS the PWM control in the square-to-pulse region.
- No PWM LFO. The morph CV input provides external PWM modulation.
- No anti-aliasing (polyBLEP). This is an LFO at sub-audio rates. Anti-aliasing is a VCO concern (deferred to v2.0).

**Confidence:** HIGH. This is straightforward DSP math using the exact same patterns already validated in `computeSquare()`.

---

### 2. Forge Noir Panel SVG (nanosvg constraints)

**What's needed:** A new 14HP SVG file respecting nanosvg's limited feature set, plus custom SVG component widgets (hex bolt screws, Forge knobs, Forge jacks).

**nanosvg Supported Features (use these):**

| SVG Feature | Support Level | Notes |
|-------------|---------------|-------|
| `<rect>`, `<circle>`, `<ellipse>`, `<line>`, `<polyline>`, `<polygon>` | Full | All basic shapes work |
| `<path>` with M/L/C/S/Q/T/A/Z commands | Full | All path commands including arcs |
| `<g>` groups with `transform` | Full | translate, scale, rotate, skewX, skewY, matrix |
| `fill`, `stroke`, `stroke-width` | Full | Inline attributes only (not CSS) |
| `opacity`, `fill-opacity`, `stroke-opacity` | Full | Per-element opacity works |
| `<linearGradient>` (2-color) | Full | Simple two-stop linear gradients |
| `<radialGradient>` (2-color, circular) | Partial | Circular only (cx/cy/r). No fx/fy. No elliptical. Improved in VCV 2.6 |
| `fill-rule` (evenodd, nonzero) | Full | Already used in current panel for letter cutouts |
| `stroke-dasharray`, `stroke-linecap`, `stroke-linejoin` | Full | Dashed lines, round/butt/square caps |
| Units: mm, px, pt, cm, in | Full | Panel must use mm (71.12mm x 128.5mm for 14HP) |

**nanosvg NOT Supported (avoid these):**

| SVG Feature | Status | Workaround |
|-------------|--------|------------|
| `<text>` / `<tspan>` | Not supported | Convert all text to `<path>` geometry (already done in current panel) |
| `<use>` / `<defs>` references | Not supported | Inline all geometry. No symbol reuse. |
| CSS `<style>` blocks | Not supported | Use inline `fill=` / `stroke=` attributes only |
| `<filter>` (blur, drop-shadow) | Not supported | Approximate glows with semi-transparent overlapping shapes |
| `<clipPath>` / `<mask>` | Not supported | Use path intersection or opacity layering |
| Multi-stop gradients (3+ stops) | Not supported | Chain multiple 2-stop gradient rectangles to simulate |
| Gradient transforms (`gradientTransform`) | Buggy | Avoid. Position gradient coordinates in absolute space |
| `<image>` | Not supported | Everything must be vector |
| CSS animations | Not supported | No animated SVG; animation lives in NanoVG code |

**Custom SVG Components needed:**

| Component | Type | Approach |
|-----------|------|----------|
| Hex bolt screws | `app::SvgScrew` subclass | Create `ForgeHexBolt.svg` (hex with socket detail), load via `setSvg(Svg::load(asset::plugin(...)))` |
| Hero knob (MORPH) | `app::SvgKnob` subclass | Create `ForgeKnobXL.svg` (background) + `ForgeKnobXL-bg.svg` if needed. Knurled ring as concentric path rings in SVG |
| Secondary knobs (CHAR/DRIFT) | `app::SvgKnob` subclass | `ForgeKnobLG.svg` -- machined metal look via radial gradient (2-color only) |
| Utility knobs (RATE/PHASE) | `app::SvgKnob` subclass | `ForgeKnobMD.svg` -- same visual language, smaller |
| Trimpots | `app::SvgKnob` subclass | `ForgeTrim.svg` -- scalloped trimpot with directional indicator |
| Input jacks | `app::SvgPort` subclass | `ForgeJackSmall.svg` -- PJ301M-style metallic rings, concentric gradient |
| Output jack | `app::SvgPort` subclass | `ForgeJackLarge.svg` -- same + ember accent ring as additional path with `fill-opacity` |

**Forge Noir SVG implementation strategy:**

The DESIGN-LANGUAGE.md describes effects (multi-layer box-shadows, conic-gradients, repeating gradients, blur filters) that are CSS/HTML-only and cannot exist in an SVG parsed by nanosvg. The SVG panel must be a **faithful translation** that achieves the same visual impression within nanosvg constraints:

| Design Language Element | SVG Translation |
|------------------------|-----------------|
| Panel texture (radial gradient) | Single 2-color radial gradient rect, centered at ~35% height |
| Accent bars (multi-stop gradient) | 3-5 adjacent thin rects with solid fills simulating gradient steps |
| Forge emblem (atmospheric background) | Paths with low fill-opacity (0.03-0.18) in ember/gold colors |
| Section divider (gradient line) | Thin rect with linearGradient (2-color, from transparent to ember) |
| Decorative header slashes | Simple rotated `<line>` elements with stroke-opacity |
| Module name "cut" through divider line | Panel-colored rect behind text paths to mask the line |
| Knob body (multi-layer gradient) | Rendered as SVG component, not panel SVG. Use 2-color radial gradient for main curvature |
| CRT scanlines on display | Cannot do in SVG; render in NanoVG `drawBackground()` if desired |

**Panel SVG file structure:**

```
res/
  AnalogLFO-ForgeNoir.svg      (14HP panel background)
  ForgeHexBolt.svg              (custom screw)
  ForgeKnobXL.svg               (hero knob body)
  ForgeKnobLG.svg               (secondary knob body)
  ForgeKnobMD.svg               (utility knob body)
  ForgeTrim.svg                 (trimpot body)
  ForgeJackSmall.svg            (input jack)
  ForgeJackLarge.svg            (output jack)
```

**Confidence:** HIGH for nanosvg constraints (verified against nanosvg source, VCV community reports, and existing panel patterns). MEDIUM for exact visual fidelity of the Forge Noir design -- some CSS effects (knurled texture via conic-gradient, multi-layer box-shadows) will need creative SVG approximation.

---

### 3. NanoVG Display Layout Changes

**What's needed:** Restructure the existing `WaveformDisplay::drawLayer()` to use the Forge Noir three-column layout. No new dependencies.

**Current NanoVG APIs already in use (no changes needed):**

| Function | Current Use | v1.3 Use |
|----------|-------------|----------|
| `nvgBeginPath` / `nvgRoundedRect` / `nvgFill` | Display background | Same + corner bracket accents |
| `nvgStrokeColor` / `nvgStroke` | Waveform trace, inset frame | Same |
| `nvgRadialGradient` / `nvgFillPaint` | Phase dot halo | Same |
| `nvgBoxGradient` | Pill backgrounds | Same |
| `nvgFontFaceId` / `nvgFontSize` / `nvgText` | Text overlays (SYNC, Hz, BPM, ratios) | Same, repositioned to columns |
| `nvgFontBlur` | Text glow effect | Same |
| `nvgTextBounds` / `nvgTextMetrics` | Pill sizing | Same |
| `nvgScissor` / `nvgResetScissor` | Zero-crossing reference line clipping | Same |

**Custom Font for Forge Noir Display:**

The current display uses `ShareTechMono-Regular.ttf` (system font). The Forge Noir design language specifies JetBrains Mono for display data. To use it:

| Font | File | Purpose | Loading Pattern |
|------|------|---------|-----------------|
| JetBrains Mono | `res/fonts/JetBrainsMono-Regular.ttf` | Display data (Hz, BPM, ratios, SYNC) | `APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/JetBrainsMono-Regular.ttf"))` |

**Important:** NanoVG font handles must be loaded every frame in `drawLayer()`, not cached across frames. The existing pattern (`APP->window->loadFont()`) is correct -- it returns from an internal cache. JetBrains Mono is SIL Open Font License, safe for plugin distribution.

The Forge Noir design language also specifies Bebas Neue and Chakra Petch, but those are for the SVG panel text (rendered as `<path>` elements), not for NanoVG display text. Only JetBrains Mono is needed in the NanoVG runtime.

**Layout change specifics:**

| Column | X Range (approx) | Content |
|--------|-------------------|---------|
| Left | 4px - ~35% width | Ratio pill, Hz readout, Swing label |
| Center | ~20% - ~80% width | Waveform trace + phase dot (existing, repositioned) |
| Right | ~65% width - margin | SYNC badge, CLK/LFO BPM stack |

The three-column layout prevents pill-waveform overlap. This is a coordinate reorganization of existing draw calls, not new rendering technology.

**Confidence:** HIGH. All NanoVG APIs needed are already in use in the v1.2 display code.

---

### 4. Animated Sync Badge (Clock-Pulse Flash)

**What's needed:** An animation state variable bridged from the audio thread, and a brightness modulation in the existing SYNC badge draw code. No new dependencies.

**Implementation stack:**

| Component | What | Why |
|-----------|------|-----|
| `std::atomic<bool> displayClockPulse` | New atomic bridge from audio thread | Signals when a clock edge is detected; set to `true` in `processClockInput()` when trigger fires |
| `float syncFlashAlpha` in WaveformDisplay | Flash envelope state (decays from 1.0 to 0.0) | Visual feedback that clock is being received |
| Exponential decay in `step()` | `syncFlashAlpha *= 0.92f` (or similar) per frame | ~60fps gives roughly 100-150ms visible flash, natural exponential decay |
| Brightness boost in `drawTextOverlays()` | Modulate SYNC badge ember color brightness by `syncFlashAlpha` | Brighter on clock pulse, dims between pulses |

**Animation architecture (matching existing pattern):**

The existing codebase already has the correct architecture for this:
1. Audio thread sets atomic flag on clock edge (like `displayClockState`)
2. Widget `step()` reads atomic and updates animation envelope (like `syncFadeAlpha` updates)
3. `drawLayer()` uses animation state to modulate rendering (like the ACQUIRING blink effect)

The clock-pulse flash adds one new atomic and one new animation variable. The `WaveformDisplay` already redraws every frame (it is a `TransparentWidget`, not cached in a `FramebufferWidget`), so no dirty-flagging is needed.

**Alternative considered and rejected:**
- Using `dsp::PulseGenerator` in the widget for flash timing. Rejected because widgets don't have access to sample rate, and the ~60fps frame rate is sufficient for visual effects. A simple multiplicative decay per frame is simpler and correct.

**Confidence:** HIGH. Direct extension of existing animation patterns already working in v1.2.

---

## Alternatives Considered

| Recommended | Alternative | Why Not |
|-------------|-------------|---------|
| Text as `<path>` in panel SVG | Embedding TTF fonts in SVG | nanosvg does not support `<text>` elements at all |
| 2-color gradients for knob metallic look | Multi-stop gradients | nanosvg only reliably supports 2-stop gradients |
| NanoVG corner bracket accents on display | SVG-rendered display frame | Display is a NanoVG widget overlaid on the SVG panel; decorative frame belongs in NanoVG where it can animate |
| Multiplicative alpha decay for flash | Timer-based PulseGenerator in widget | Widgets run at frame rate (~60fps), not sample rate; decay per frame is simpler and frame-rate-independent with dt scaling |
| JetBrains Mono for display | Keep ShareTechMono | ShareTechMono is a system font with good readability but JetBrains Mono aligns with the Forge Noir design language and offers better glyph quality at small sizes |
| Inline all SVG elements | `<use>` / `<defs>` for symmetry | nanosvg does not support `<use>` or `<defs>` references; the DESIGN-LANGUAGE.md's `<use>` mirroring advice applies to the HTML mockup, not the production SVG |
| Separate SVG files per component | Single monolithic SVG | VCV Rack's component library architecture expects individual SVG files per widget type; this enables hot-swapping and theming |

---

## What NOT to Add

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| External SVG preprocessing tool (svgo, etc.) | Adds build complexity; nanosvg handles simple SVGs fine | Hand-author SVGs or use Inkscape with Object-to-Path |
| OpenGL shaders for knob rendering | Overkill for static knob graphics; VCV Rack's SvgKnob handles rotation | SVG-based knob components with radial gradients |
| Custom NanoVG build or patches | Modifying SDK headers breaks compatibility with future VCV Rack updates | Work within bundled NanoVG's capabilities |
| Additional C++ libraries (Eigen, etc.) | PWM is trivial math; no linear algebra or DSP library needed | Standard `<cmath>` (already included) |
| Separate display widget class hierarchy | Current `WaveformDisplay` struct handles all display rendering | Extend existing struct with new layout coordinates |
| Animation framework / tween library | One exponential decay variable does not need a framework | `syncFlashAlpha *= decayFactor` in `step()` |
| Web fonts or @font-face in SVG | Not supported by nanosvg | Convert all panel text to `<path>` elements |
| SVG filters for glow/shadow | Not supported by nanosvg | Use overlapping semi-transparent shapes for glow approximation |
| FramebufferWidget for WaveformDisplay | The display updates every frame (phase dot moves continuously) | Keep as TransparentWidget; framebuffer caching would require marking dirty every frame anyway |

---

## Version Compatibility

| Component | Version | Compatibility Notes |
|-----------|---------|---------------------|
| VCV Rack SDK | 2.x (tested ~2.6) | Stable API. `app::SvgScrew`, `app::SvgKnob`, `app::SvgPort` classes unchanged since v2.0 |
| nanosvg (bundled) | SDK-pinned | Gradient support improved in 2.6; target 2-color gradients for compatibility with older Rack versions |
| NanoVG (bundled) | SDK-pinned | `nvgBoxGradient`, `nvgRadialGradient`, font functions all stable since v2.0 |
| JetBrains Mono | Latest (SIL OFL) | No version dependency; TTF file bundled with plugin. ~150KB per weight |
| C++17 | As per SDK | `std::atomic`, `std::array`, structured bindings all available |

---

## File Additions Summary

```
res/
  AnalogLFO-ForgeNoir.svg      NEW  (14HP Forge Noir panel)
  ForgeHexBolt.svg              NEW  (custom hex bolt screw)
  ForgeKnobXL.svg               NEW  (hero knob body)
  ForgeKnobLG.svg               NEW  (secondary knob body)
  ForgeKnobMD.svg               NEW  (utility knob body)
  ForgeTrim.svg                 NEW  (trimpot body)
  ForgeJackSmall.svg            NEW  (input jack)
  ForgeJackLarge.svg            NEW  (output jack)
  fonts/
    JetBrainsMono-Regular.ttf   NEW  (display font)

src/
  AnalogLFO.cpp                 MODIFY (PWM, layout, flash, custom components)
```

No Makefile changes needed. The existing `SOURCES += $(wildcard src/*.cpp)` and `DISTRIBUTABLES += res` already cover all additions.

---

## Sources

- [VCV Rack Module Panel Guide](https://vcvrack.com/manual/Panel) -- Official SVG requirements, nanosvg limitations (HIGH confidence)
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) -- Font loading, NanoVG rendering, FramebufferWidget, custom widgets (HIGH confidence)
- [VCV Rack FramebufferWidget API](https://vcvrack.com/docs-v2/structrack_1_1widget_1_1FramebufferWidget) -- Dirty flag, setDirty(), performance model (HIGH confidence)
- [nanosvg GitHub (memononen/nanosvg)](https://github.com/memononen/nanosvg) -- Supported SVG elements, paint types, gradient types (HIGH confidence)
- [nanosvg DeepWiki](https://deepwiki.com/memononen/nanosvg) -- Feature overview, paint type enums, path commands (HIGH confidence)
- [VCV Community: SVG transparencies and gradients](https://community.vcvrack.com/t/svg-transparencies-and-gradients/16833) -- Gradient support details, 2-color limitation, VCV 2.6 improvements (MEDIUM confidence)
- [VCV Community: Notes for theme-able SVGs with nanoSvg](https://community.vcvrack.com/t/notes-for-theme-able-svgs-with-nanosvg/20060) -- Theming patterns, SVG traversal (MEDIUM confidence)
- [VCV Community: Custom knob SVGs](https://community.vcvrack.com/t/how-can-i-implement-custom-knob-svgs/21399) -- SvgKnob subclass pattern (HIGH confidence)
- [VCV Rack componentlibrary.hpp (v2)](https://github.com/VCVRack/Rack/blob/v2/include/componentlibrary.hpp) -- ScrewSilver/ScrewBlack/SvgKnob class patterns (HIGH confidence)
- [stellare-modular/vcv-rack-sdk nanovg.h](https://github.com/stellare-modular/vcv-rack-sdk/blob/master/dep/include/nanovg.h) -- NanoVG font and gradient API signatures (HIGH confidence)
- [Perfect Circuit: What is PWM?](https://www.perfectcircuit.com/signal/what-is-pwm) -- PWM fundamentals, duty cycle narrowing (HIGH confidence)
- [Yamaha Synth: All Squares are Pulse](https://yamahasynth.com/learn/synth-programming/synth-basics-all-squares-are-pulse/) -- Pulse wave as square with variable duty (HIGH confidence)
- Existing codebase: `src/AnalogLFO.cpp` -- computeSquare (lines 276-297), computeMorphedWave (lines 299-343), WaveformDisplay (lines 802-1293) (HIGH confidence)

---
*Stack research for: Forge Audio Analog Series v1.3 Forge Noir*
*Researched: 2026-03-27*
