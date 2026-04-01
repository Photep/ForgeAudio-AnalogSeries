# Phase 20: Display Layout + CRT Aesthetic - Research

**Researched:** 2026-04-01
**Domain:** NanoVG display rendering, three-column layout, CRT visual effects, font bundling
**Confidence:** HIGH

## Summary

Phase 20 transforms the existing `WaveformDisplay` widget from a full-width waveform with corner-positioned pills into a structured three-column layout with CRT-inspired visual treatment. The existing code is well-structured with clear sub-draw methods (`drawBackground`, `drawInsetFrame`, `drawWaveformTrace`, `drawPhaseDot`, `drawPillText`, `drawBpmStack`, `drawTextOverlays`) that can be modified in-place. The key architectural changes are: (1) updating `phaseToX()` to constrain the waveform to the center 60% of display width, (2) repositioning all pill text from corner-based to column-based layout, (3) replacing the navy pill styling with Forge Noir ember-tinted pills, (4) adding scanline overlay via NanoVG image pattern, (5) adding corner bracket decorations, and (6) converting the border from inset shadow to ember border with breathing glow.

The display widget measures 188.8 x 53.1 pixels at VCV Rack's native resolution. The design language mockup (forge-noir.html) uses a 320x90 SVG viewBox at 5x scale, giving a scale factor of 0.59 from mockup coordinates to widget pixels. All mockup coordinates must be scaled by this factor. Font sizes in the mockup (5-8px) become very small at widget scale (2.9-4.7px), which is typical for VCV Rack module displays but requires JetBrains Mono's superior small-size legibility.

NanoVG lacks native dashed line support, so the zero-crossing reference line must be drawn as a series of short filled rectangles or individual line segments. Scanlines are best implemented via `nvgCreateImageRGBA` with `NVG_IMAGE_REPEATY` and `nvgImagePattern`, creating a repeating 4-pixel-tall stripe pattern (2px transparent + 2px dark) that scrolls by adjusting the pattern origin each frame.

**Primary recommendation:** Modify existing WaveformDisplay sub-draw methods in sequence -- background/border first, then column layout + phaseToX(), then pill restyling, then CRT effects. Each change is independently testable by visual inspection. Bundle JetBrains Mono NL (no ligatures) Regular TTF in `res/fonts/`.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Display uses three-column layout per DESIGN-LANGUAGE.md: left column (ratio, Hz, swing pills), center column (waveform trace + phase dot), right column (SYNC badge, CLK/BPM stack)
- **D-02:** `phaseToX()` coordinate helper updated to map waveform trace to center ~60% of display width, with consistent margins for pill columns on both sides
- **D-03:** Pills positioned in their dedicated margin columns -- they never overlap the waveform trace
- **D-04:** Subtle atmosphere scanlines -- barely visible (~0.03-0.05 opacity dark bands), enhancing the analog screen feel without competing with the waveform or pills
- **D-05:** Slow scroll animation -- very slow downward drift (~1px/sec) simulating CRT refresh. Adds subtle life to the display.
- **D-06:** Full Forge Noir pill styling -- ember-tinted backgrounds (rgba(232,93,38,0.1-0.12)) with ember stroke, replacing current navy feathered boxes
- **D-07:** Font switch from ShareTechMono to JetBrains Mono -- bundle the .ttf in res/fonts/
- **D-08:** Pill border-radius matches design language: 2-2.5px with ember stroke border. Crisper, more defined pills.
- **D-09:** 3-layer glow per design language: wide diffuse (stroke-width 7, opacity 0.06), medium (3.5, 0.15), sharp core (2, 0.85). Replaces current 4-pass approach.
- **D-10:** Add dashed zero-crossing reference line at vertical center -- subtle, low opacity, helps users read waveform symmetry/offset
- **D-11:** Waveform trace color shifts to ember (#e85d26) family. Phase dot core stays molten gold (#f0a030). Trace = wire, dot = hot spot.
- **D-12:** Display background adopts #030303 (near-black) from design language, replacing current navy-ish #0d0d1a
- **D-13:** Full design language border: 1.5px ember border (rgba(232,93,38,0.3)) with breathing glow animation pulsing 0.08-0.18 opacity over 5 seconds. Replace current inset shadow/highlight frame.
- **D-14:** Corner bracket decorations drawn on top of border: 8x8px L-shaped ember borders at each corner (1.5px, opacity 0.4) per design language

### Claude's Discretion
- Phase dot rendering approach: 3 concentric circles (design language) vs radial gradient halo (current) -- pick whichever looks better with #030303 background
- Comet trail and drift jitter behavior preserved regardless of dot approach
- Scanline scroll implementation details (phase accumulator, line spacing at actual widget scale)
- `breathePhase` rate adjustment to match 5-second border glow cycle vs current 0.8Hz
- JetBrains Mono weight/size tuning for readability at small display size

### Deferred Ideas (OUT OF SCOPE)
- Pulse width modulation (2026-03-17) -- already completed in Phase 18. False keyword match.
- Surge-style modulation routing system (2026-03-17) -- out of scope per PROJECT.md. False keyword match.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| DISP-01 | Three-column display layout (left pills, center waveform, right pills) | Column boundaries calculated from mockup (left: 9.4-32.4px, center: 37.8-151.0px, right: 154.6-178.2px). phaseToX() update constrains trace. drawTextOverlays() repositioned to column coordinates. |
| DISP-02 | Waveform rendering constrained to center ~60% of display width | phaseToX() change: margin = center_start, range = center_width. Center column is exactly 60% of display width (113.3px of 188.8px). |
| DISP-03 | Corner bracket decorations on display border | Four NanoVG L-shaped paths at corners: 2.9px inset, 4.7x4.7px size, 1.5px stroke, ember rgba(232,93,38,0.4). Drawn after border, before scanlines. |
| DISP-04 | CRT scanline aesthetic overlay | nvgCreateImageRGBA with NVG_IMAGE_REPEATY: 1x4px RGBA tile (2px transparent + 2px at 0.03-0.05 opacity). nvgImagePattern covers display, scrolls via offset. |
| DISP-05 | Breathing display border glow animation | breathePhase rate adjusted to 0.2Hz (5s cycle). Border stroke drawn with pulsing glow: ember rgba with alpha oscillating 0.08-0.18 via sin(breathePhase). |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| NanoVG | VCV Rack 2 bundled | All display rendering (paths, text, fills, images) | Only option for VCV Rack widget rendering |
| JetBrains Mono NL | 2.304 (OFL-1.1) | Display pill text, data readouts | Design language specifies it; excellent small-size legibility; no-ligature variant prevents data display artifacts |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| VCV Rack SDK | 2.6.0+ | Widget framework, asset loading, font caching | TransparentWidget, drawLayer, APP->window->loadFont() |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| JetBrains Mono NL | JetBrains Mono (with ligatures) | Ligature variant could render unexpected glyphs in data text (e.g., "ff" ligature). NL variant is safer for numeric/label display. |
| nvgCreateImageRGBA scanlines | Individual nvgRect per scanline | Rect loop is simpler conceptually but costs ~13 draw calls vs 1 image pattern fill. Pattern approach is both faster and supports scrolling trivially. |

**Font acquisition:**
```bash
# Download JetBrains Mono NL Regular TTF
# From: https://github.com/JetBrains/JetBrainsMono/releases
# File needed: JetBrainsMonoNL-Regular.ttf
# Place at: res/fonts/JetBrainsMonoNL-Regular.ttf
# License: OFL-1.1 (free for commercial use, no attribution required)
```

## Architecture Patterns

### Display Rendering Order (drawLayer sequence)
```
drawLayer(layer 1):
  1. drawBackground()        -- #030303 fill
  2. drawBorder()            -- 1.5px ember stroke + breathing glow  [replaces drawInsetFrame]
  3. drawCornerBrackets()    -- 4 L-shaped ember brackets            [NEW]
  4. drawZeroCrossing()      -- dashed center line in center column  [NEW]
  5. drawWaveformTrace()     -- 3-layer glow, center column only     [MODIFIED]
  6. drawPhaseDot()          -- dot + comet trail, center column     [MODIFIED colors]
  7. drawTextOverlays()      -- pills in left/right columns          [MODIFIED positions+style]
  8. drawScanlines()         -- subtle scrolling overlay on top      [NEW]
```

### Pattern 1: Column-Constrained Coordinate Helpers
**What:** Update `phaseToX()` to map [0,1] phase to center column only, not full display width.
**When to use:** Every waveform trace point and phase dot position.
**Example:**
```cpp
// Current (full-width):
float phaseToX(float phase) const {
    float margin = 4.f;
    return margin + phase * (box.size.x - 2.f * margin);
}

// Updated (center-column):
float phaseToX(float phase) const {
    // Center column: 20% to 80% of display width
    float centerStart = box.size.x * 0.20f;
    float centerWidth = box.size.x * 0.60f;
    return centerStart + phase * centerWidth;
}
```

### Pattern 2: Scanline Image Pattern with Scroll
**What:** Create a small repeating RGBA texture and use `nvgImagePattern` with shifting origin for slow scroll.
**When to use:** CRT scanline overlay drawn last in render order.
**Example:**
```cpp
// In struct member initialization (once, lazily):
int scanlineImage = -1;

// In drawScanlines():
if (scanlineImage < 0) {
    // 1px wide x 4px tall tile: 2px transparent, 2px dark
    // At widget scale, 4px ≈ 2 scanline pairs per ~4 widget pixels
    unsigned char scanlineTile[1 * 4 * 4]; // 1x4 RGBA
    memset(scanlineTile, 0, sizeof(scanlineTile));
    // Rows 0-1: transparent (alpha=0, already zeroed)
    // Rows 2-3: dark band
    scanlineTile[2*4+3] = 10;  // row 2 alpha (~0.04 opacity = 10/255)
    scanlineTile[3*4+3] = 10;  // row 3 alpha
    scanlineImage = nvgCreateImageRGBA(vg, 1, 4,
        NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY | NVG_IMAGE_NEAREST,
        scanlineTile);
}

// Scroll offset: ~1px/sec at 60fps = 1/60 px per frame
float scrollOffset = fmodf(scanlineScrollPhase, 4.f);
NVGpaint scanPaint = nvgImagePattern(vg, 0, scrollOffset,
    1.f, 4.f, 0.f, scanlineImage, 1.f);
nvgBeginPath(vg);
nvgRect(vg, 0, 0, box.size.x, box.size.y);
nvgFillPaint(vg, scanPaint);
nvgFill(vg);
```

### Pattern 3: Ember Pill Styling (replaces navy feathered boxes)
**What:** Pill backgrounds switch from navy `nvgBoxGradient` to ember-tinted fill with stroke border.
**When to use:** All pill rendering (ratio, Hz, SYNC, BPM, swing).
**Example:**
```cpp
void drawPillText(NVGcontext* vg, int fontHandle, float x, float y,
                  const char* text, float fontSize, int align, float alpha) {
    nvgFontFaceId(vg, fontHandle);
    nvgFontSize(vg, fontSize);
    nvgTextAlign(vg, align);

    float bounds[4];
    nvgTextBounds(vg, x, y, text, NULL, bounds);

    float pad = 3.f;
    float cornerRadius = 2.f;
    float px = bounds[0] - pad;
    float py = bounds[1] - pad;
    float pw = (bounds[2] - bounds[0]) + 2.f * pad;
    float ph = (bounds[3] - bounds[1]) + 2.f * pad;

    // Ember-tinted fill
    nvgBeginPath(vg);
    nvgRoundedRect(vg, px, py, pw, ph, cornerRadius);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.10f * alpha));
    nvgFill(vg);

    // Ember stroke border
    nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.25f * alpha));
    nvgStrokeWidth(vg, 0.5f);
    nvgStroke(vg);

    // Glow text pass
    nvgFontBlur(vg, 2.0f);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, alpha * 0.3f));
    nvgText(vg, x, y, text, NULL);

    // Sharp text pass
    nvgFontBlur(vg, 0.0f);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, alpha));
    nvgText(vg, x, y, text, NULL);
}
```

### Pattern 4: Corner Brackets
**What:** Four L-shaped ember decorations at display corners.
**When to use:** Drawn once per frame after border, before waveform content.
**Example:**
```cpp
void drawCornerBrackets(NVGcontext* vg) {
    float inset = 3.f;   // ~5px mockup * 0.59 scale
    float size = 5.f;    // ~8px mockup * 0.59 scale
    float w = box.size.x;
    float h = box.size.y;

    nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.4f));
    nvgStrokeWidth(vg, 1.0f);  // ~1.5px mockup scaled
    nvgLineCap(vg, NVG_BUTT);

    // Top-left
    nvgBeginPath(vg);
    nvgMoveTo(vg, inset, inset + size);
    nvgLineTo(vg, inset, inset);
    nvgLineTo(vg, inset + size, inset);
    nvgStroke(vg);

    // Top-right
    nvgBeginPath(vg);
    nvgMoveTo(vg, w - inset - size, inset);
    nvgLineTo(vg, w - inset, inset);
    nvgLineTo(vg, w - inset, inset + size);
    nvgStroke(vg);

    // Bottom-left
    nvgBeginPath(vg);
    nvgMoveTo(vg, inset, h - inset - size);
    nvgLineTo(vg, inset, h - inset);
    nvgLineTo(vg, inset + size, h - inset);
    nvgStroke(vg);

    // Bottom-right
    nvgBeginPath(vg);
    nvgMoveTo(vg, w - inset - size, h - inset);
    nvgLineTo(vg, w - inset, h - inset);
    nvgLineTo(vg, w - inset, h - inset - size);
    nvgStroke(vg);
}
```

### Pattern 5: Breathing Border Glow
**What:** Border glow opacity pulses between 0.08 and 0.18 over 5 seconds using the existing `breathePhase` accumulator (rate adjusted to 0.2Hz).
**When to use:** Replaces `drawInsetFrame()`.
**Example:**
```cpp
void drawBorder(NVGcontext* vg) {
    float w = box.size.x;
    float h = box.size.y;

    // Breathing glow: 0.08 to 0.18 opacity, 5s cycle
    // breathePhase now runs at 0.2Hz (adjusted in step())
    float glowAlpha = 0.08f + 0.10f * (0.5f + 0.5f * std::sin(breathePhase));
    // Note: sin range [-1,1], mapped to [0,1], then scaled to [0.08, 0.18]

    // Outer glow
    nvgBeginPath(vg);
    nvgRoundedRect(vg, -2.f, -2.f, w + 4.f, h + 4.f, 6.f);
    nvgRoundedRect(vg, 0, 0, w, h, 4.f);  // inner cutout
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, glowAlpha));
    nvgFill(vg);

    // Solid border
    nvgBeginPath(vg);
    nvgRoundedRect(vg, 0, 0, w, h, 4.f);
    nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.3f));
    nvgStrokeWidth(vg, 1.5f);
    nvgStroke(vg);
}
```

### Anti-Patterns to Avoid
- **Drawing scanlines as individual rects in a loop:** Each rect is a separate NanoVG draw call. Use image pattern instead -- single fill covers entire display.
- **Caching Font objects across frames:** VCV Rack's Window can be destroyed and recreated. Always call `APP->window->loadFont()` within drawLayer.
- **Allocating in drawLayer:** No heap allocation in render path. Scanline image created lazily once, reused thereafter.
- **Using nvgFontBlur with very small text:** At widget scale, fonts are 2.9-4.7px. Excessive blur (>2px) will obliterate text. Keep blur radius proportional to font size.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Dashed lines | Stroke dasharray (not available) | Loop of short `nvgRect` fills or `nvgMoveTo/LineTo` segments | NanoVG has no native dashed stroke support. Manual segments work fine for horizontal zero-crossing line. |
| Repeating scanline pattern | Loop of `nvgRect` for each scanline | `nvgCreateImageRGBA` + `nvgImagePattern` with `NVG_IMAGE_REPEATY` | One draw call vs ~13. Pattern origin offset handles scroll animation for free. |
| Font loading | Custom font cache | `APP->window->loadFont(asset::plugin(...))` | VCV Rack manages font lifecycle tied to GPU context. |
| Breathing animation | Separate timer | Existing `breathePhase` accumulator in `step()` | Already incremented every frame. Just adjust rate constant. |

**Key insight:** NanoVG's image pattern API is the single most important tool for this phase. It solves scanlines efficiently and the zero-crossing line (individual segments) is the only place where manual path construction is needed.

## Common Pitfalls

### Pitfall 1: Scanline Image Not Created (GPU context)
**What goes wrong:** `nvgCreateImageRGBA` returns -1 if called before NanoVG context is ready.
**Why it happens:** Calling in constructor before GPU init.
**How to avoid:** Lazy-create in first drawLayer call. Check `scanlineImage < 0` and create on first use.
**Warning signs:** Black display, no scanlines visible.

### Pitfall 2: breathePhase Rate Mismatch
**What goes wrong:** Border glow and phase dot breathe at different rates after changing breathePhase.
**Why it happens:** breathePhase currently drives both border glow and phase dot breathing. Changing its rate to 0.2Hz (5s border cycle) also slows the dot breathing.
**How to avoid:** Either (a) use a second accumulator `borderGlowPhase` for the border glow, or (b) accept 5s breathing on both (the dot breathe is only active when LFO is paused, so slower breathing is acceptable). Recommendation: single accumulator at 0.2Hz is fine -- the dot breathe is a subtle idle effect.
**Warning signs:** Phase dot pulsing too slowly or too fast.

### Pitfall 3: Font Size Too Small at Widget Scale
**What goes wrong:** Text is unreadable blobs at native VCV Rack zoom.
**Why it happens:** Mockup font sizes (5-8px) scale to 2.9-4.7px at widget resolution.
**How to avoid:** Use JetBrains Mono which is optimized for small sizes. Test at 100% zoom in VCV Rack. Consider bumping font sizes slightly above strict mockup scaling if readability suffers. The display is already small -- readability trumps exact mockup fidelity at sub-4px sizes.
**Warning signs:** Pills look like colored blobs instead of readable text.

### Pitfall 4: Scanline Scroll Jitter from Float Precision
**What goes wrong:** Scanline scroll visibly jumps or stutters after running for a long time.
**Why it happens:** `scanlineScrollPhase` accumulates without wrapping, eventually losing float precision.
**How to avoid:** Wrap `scanlineScrollPhase` with `fmodf(phase, 4.0f)` (tile height) every frame. The tile repeats every 4px so wrapping at 4.0 is seamless.
**Warning signs:** Scanline scroll looks choppy after module runs for hours.

### Pitfall 5: Pill Positioning Hardcoded to Pixel Values
**What goes wrong:** Pills misaligned on displays with different DPI or VCV Rack zoom levels.
**Why it happens:** Using absolute pixel coordinates instead of proportional placement.
**How to avoid:** Express pill positions as fractions of `box.size.x` and `box.size.y`, not hardcoded pixel values. The column boundaries should be percentages (0-20% left, 20-80% center, 80-100% right).
**Warning signs:** Layout breaks at non-standard VCV Rack zoom levels.

### Pitfall 6: NanoVG nvgCreateImageRGBA Leak
**What goes wrong:** GPU memory leak if scanline image is recreated every frame.
**Why it happens:** Calling nvgCreateImageRGBA without tracking the handle.
**How to avoid:** Store image handle as struct member. Create once (lazy init). NanoVG image is tied to the context and cleaned up when context is destroyed. No manual delete needed for VCV Rack's managed NVGcontext.
**Warning signs:** GPU memory usage climbing over time.

### Pitfall 7: Glow Rendering Outside Scissor Rect
**What goes wrong:** Border glow effect is clipped by the scissor region set at display bounds.
**Why it happens:** `nvgScissor(vg, 0, 0, box.size.x, box.size.y)` clips exactly at widget bounds, but glow extends beyond.
**How to avoid:** Draw border glow BEFORE setting scissor, or expand scissor by glow radius. Alternatively, draw glow as an inner effect (already clipped naturally). The code example above uses an inner cutout approach with `NVG_HOLE` which stays within bounds.
**Warning signs:** Glow appears as hard-edged rectangle instead of soft fade.

## Code Examples

### Existing Code: Current drawTextOverlays Layout (to be modified)
```cpp
// Source: src/AnalogLFO.cpp lines 1232-1287
// Current positions use corner-based placement:
//   Hz/Ratio: top-left (margin, margin + fontSize)
//   SYNC: top-right (box.size.x - margin, margin + fontSize)
//   BPM: bottom-right (box.size.x - margin, box.size.y - margin)
//   Swing: bottom-left (margin, box.size.y - margin)
//
// Phase 20 changes these to column-based positions:
//   Left column: ratio at top, Hz below, swing at bottom
//   Right column: SYNC at top, CLK/BPM stack at bottom
```

### Column Layout Constants
```cpp
// Derived from DESIGN-LANGUAGE.md mockup (320x90 viewBox → 188.8x53.1 widget)
// Column boundaries as fractions of display width:
static constexpr float LEFT_COL_START = 0.05f;    // ~9.4px at 188.8px width
static constexpr float LEFT_COL_END   = 0.17f;    // ~32px
static constexpr float CENTER_START   = 0.20f;    // ~37.8px
static constexpr float CENTER_END     = 0.80f;    // ~151px
static constexpr float RIGHT_COL_START = 0.82f;   // ~155px
static constexpr float RIGHT_COL_END   = 0.95f;   // ~179px
```

### Dashed Zero-Crossing Line (no native NanoVG dashes)
```cpp
void drawZeroCrossing(NVGcontext* vg) {
    float centerY = box.size.y / 2.f;
    float xStart = box.size.x * 0.20f;  // center column start
    float xEnd = box.size.x * 0.80f;    // center column end
    float dashLen = 2.f;
    float gapLen = 3.f;
    float x = xStart;

    nvgBeginPath(vg);
    while (x < xEnd) {
        float end = std::min(x + dashLen, xEnd);
        nvgMoveTo(vg, x, centerY);
        nvgLineTo(vg, end, centerY);
        x = end + gapLen;
    }
    nvgStrokeColor(vg, nvgRGBAf(1.f, 1.f, 1.f, 0.06f));
    nvgStrokeWidth(vg, 0.5f);
    nvgStroke(vg);
}
```

### Font Loading for JetBrains Mono
```cpp
// In drawTextOverlays():
std::shared_ptr<Font> font = APP->window->loadFont(
    asset::plugin(pluginInstance, "res/fonts/JetBrainsMonoNL-Regular.ttf"));
if (!font) return;
// Use font->handle with nvgFontFaceId()
```

### Phase Dot: Concentric Circles Approach (recommended for #030303 background)
```cpp
// Three concentric circles per design language:
// Outer glow: r*3, ember, opacity 0.08
// Mid ring: r*2, ember, opacity 0.22
// Core: r*1, hot gold #f0a030, opacity 0.95
void drawPhaseDotCircles(NVGcontext* vg, float x, float y,
                         float dotRadius, float dimFactor, float breatheFactor) {
    // Outer glow
    nvgBeginPath(vg);
    nvgCircle(vg, x, y, dotRadius * 3.f);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.08f * dimFactor * breatheFactor));
    nvgFill(vg);

    // Mid ring
    nvgBeginPath(vg);
    nvgCircle(vg, x, y, dotRadius * 2.f);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.22f * dimFactor * breatheFactor));
    nvgFill(vg);

    // Core
    nvgBeginPath(vg);
    nvgCircle(vg, x, y, dotRadius);
    nvgFillColor(vg, nvgRGBAf(0.941f, 0.627f, 0.188f, 0.95f * dimFactor * breatheFactor));
    nvgFill(vg);
}
```

## Dimension Reference

### Display Widget
| Property | Value |
|----------|-------|
| Position | mm2px(Vec(3.60f, 13.19f)) |
| Size (mm) | 63.93mm x 17.98mm |
| Size (px) | 188.8 x 53.1 |
| Mockup viewBox | 320 x 90 |
| Mockup scale | 0.59x |

### Column Boundaries (widget pixels)
| Column | Start | End | Width |
|--------|-------|-----|-------|
| Left | 9.4px | 32.4px | 23.0px |
| Center | 37.8px | 151.0px | 113.3px |
| Right | 154.6px | 178.2px | 23.6px |

### Font Sizes (mockup -> widget)
| Mockup | Widget | Usage |
|--------|--------|-------|
| 7px | 4.1px | Ratio, SYNC labels |
| 6px | 3.5px | BPM values |
| 5.5px | 3.2px | Swing text |
| 5px | 2.9px | Hz label, SWING label, CLK/LFO labels |

### Color Reference (NanoVG float values)
| Name | Hex | nvgRGBAf |
|------|-----|----------|
| Display BG | #030303 | nvgRGBAf(0.012f, 0.012f, 0.012f, 1.f) |
| Ember | #e85d26 | nvgRGBAf(0.91f, 0.365f, 0.149f, ...) |
| Hot Gold | #f0a030 | nvgRGBAf(0.941f, 0.627f, 0.188f, ...) |
| Molten Gold | #daa520 | nvgRGBAf(0.855f, 0.647f, 0.125f, ...) |
| Label Dim | #666 | nvgRGBAf(0.4f, 0.4f, 0.4f, ...) |

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Full-width waveform | Three-column layout | Phase 20 | Fixes pill/waveform overlap, clean data readout margins |
| Navy feathered pill backgrounds | Ember-tinted solid pills with stroke | Phase 20 | Matches Forge Noir palette, crisper appearance |
| ShareTechMono (system font) | JetBrains Mono NL (bundled) | Phase 20 | Better small-size legibility, consistent across platforms |
| 4-pass waveform glow | 3-layer glow (design language spec) | Phase 20 | Fewer draw calls, matches design language |
| Inset shadow border | Ember border with breathing glow | Phase 20 | Animated, matches Forge Noir aesthetic |
| Static display | CRT scanline overlay with slow scroll | Phase 20 | Adds analog monitor feel |

## Open Questions

1. **Scanline Tile Size at Widget Scale**
   - What we know: Mockup uses 2px transparent + 2px dark bands (at 5x scale). At widget scale, this would be ~1.2px per band. NanoVG with `NVG_IMAGE_NEAREST` may not render sub-pixel patterns cleanly.
   - What's unclear: Whether a 4px tile at widget scale looks better than a scaled 4px tile at mockup scale. May need to test both.
   - Recommendation: Start with 4px tile (2+2) at widget scale with `NVG_IMAGE_NEAREST` flag. If bands are too coarse, try 2px tile (1+1). The very low opacity (0.03-0.05) makes the exact spacing forgiving.

2. **Phase Dot: Concentric Circles vs Radial Gradient**
   - What we know: Design language specifies 3 concentric circles. Current code uses radial gradient halo. On #030303 background, concentric circles may look more defined.
   - What's unclear: Visual quality difference at 53px display height with ~1.6px dot radius.
   - Recommendation: Use concentric circles (design language approach). The 3-circle approach is simpler code, matches the design spec, and the opacity layering creates a natural gradient effect. The radial gradient approach requires maintaining the NVGpaint gradient which is more code for similar visual result.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Visual inspection (NanoVG rendering, no unit test framework) |
| Config file | None -- VCV Rack plugin, visual verification only |
| Quick run command | `make -C "/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series" install && open -a "VCV Rack 2 Pro"` |
| Full suite command | Same as quick run -- visual inspection in VCV Rack |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| DISP-01 | Three-column layout: pills in margins, waveform in center | manual-visual | Load patch, verify pill positions in left/right columns, waveform in center | N/A |
| DISP-02 | Waveform constrained to center 60% | manual-visual | Load patch with various morph shapes, verify trace does not extend into margins | N/A |
| DISP-03 | Corner bracket decorations visible | manual-visual | Visual check at 100% and 200% zoom for L-shaped brackets at all 4 corners | N/A |
| DISP-04 | CRT scanline overlay visible | manual-visual | Look for faint horizontal bands across display, verify slow downward scroll | N/A |
| DISP-05 | Breathing border glow animation | manual-visual | Watch idle module for 10+ seconds, verify border glow pulses | N/A |

### Sampling Rate
- **Per task commit:** `make install` + visual verification in VCV Rack
- **Per wave merge:** Full visual review of all 5 DISP requirements
- **Phase gate:** All 5 success criteria verified visually, screenshot documentation

### Wave 0 Gaps
- [ ] `res/fonts/JetBrainsMonoNL-Regular.ttf` -- font file must be downloaded and placed before any text rendering changes
- [ ] No automated test infrastructure -- all verification is visual. This is standard for VCV Rack NanoVG rendering.

## Sources

### Primary (HIGH confidence)
- VCV Rack SDK nanovg.h (`/Users/mrcbrown/Claude/Software/Forge Audio/Rack-SDK/dep/include/nanovg.h`) -- NanoVG API reference for image patterns, gradients, compositing, image flags
- DESIGN-LANGUAGE.md -- Complete Forge Noir design language with display column layout, pill styling, color palette
- forge-noir.html -- Canonical layout mockup with exact pixel coordinates (SVG viewBox 320x90)
- src/AnalogLFO.cpp lines 846-1337 -- Current WaveformDisplay implementation

### Secondary (MEDIUM confidence)
- [VCV Community: Dashed or dotted strokes with NanoVG](https://community.vcvrack.com/t/dashed-or-dotted-strokes-with-nanovg/6415) -- Confirms no native dash support, manual segments required
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) -- Font loading via asset::plugin, loadFont lifecycle
- [JetBrains Mono GitHub](https://github.com/JetBrains/JetBrainsMono) -- OFL-1.1 license, NL variant availability

### Tertiary (LOW confidence)
- None -- all findings verified against primary sources.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- NanoVG is the only option, APIs verified in SDK header
- Architecture: HIGH -- existing code structure is clear, column layout is straightforward coordinate math
- Pitfalls: HIGH -- NanoVG limitations (no dashes, image lifecycle) verified against SDK and community docs
- Dimension calculations: HIGH -- derived from actual mm2px conversion and mockup viewBox

**Research date:** 2026-04-01
**Valid until:** 2026-05-01 (stable -- NanoVG API and VCV Rack 2 SDK are mature)
