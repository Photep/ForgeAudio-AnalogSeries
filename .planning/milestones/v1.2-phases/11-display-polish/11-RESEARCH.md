# Phase 11: Display Polish - Research

**Researched:** 2026-03-13
**Domain:** NanoVG text overlay rendering, pill backgrounds, dual BPM display
**Confidence:** HIGH

## Summary

Phase 11 adds soft-edged pill backgrounds behind all HUD text overlays (SYNC badge, ratio label, BPM, Hz) so they remain readable when the waveform trace passes through them, and adds a dual-line BPM display showing raw incoming clock BPM alongside the ratio-adjusted effective BPM.

The existing `drawTextOverlays()` function (AnalogLFO.cpp:741) is the single modification point. All work happens in NanoVG rendering code on the display widget. The NanoVG `nvgBoxGradient` API provides exactly the feathered-edge pill background effect requested. Text bounds are measured with `nvgTextBounds` to size pills dynamically. No new audio-thread changes are needed -- `displaySmoothedPeriod` already provides the raw clock period, and `displayRatioIndex` provides ratio selection.

**Primary recommendation:** Refactor `drawGlowText()` into a `drawPillText()` helper that measures text bounds, renders a box-gradient pill background, then draws the 2-pass glow text on top. Add a new `drawBpmStack()` function for the dual-line CLK/BPM readout with conditional x1-ratio collapsing.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Dark navy pill backgrounds (#1a1a2e at ~80% opacity) behind each text overlay
- Feathered gradient edges (3-4px fade from full opacity to transparent) -- no hard boundary, text emerges from a dark cloud
- Each text element gets its own individual pill (SYNC, ratio, Hz, BPM stack each have separate pills)
- Comfortable padding (4-5px) around text within pills
- Raw incoming clock BPM and effective BPM stacked vertically in the bottom-right corner
- Raw CLK BPM on top (labeled "CLK"), effective BPM below (labeled "BPM")
- Both lines share one pill background (grouped as related info)
- Raw CLK line: smaller font (~8px) and dimmer (~60% alpha) -- supplemental context
- Effective BPM line: standard font (10px) and full alpha -- primary readout
- At x1 ratio: only one line shown ("120 BPM") -- no redundant CLK line
- At non-x1 ratios: both lines shown (e.g., "120 CLK" / "30 BPM")
- All text stays amber (0.91, 0.66, 0.22) -- same color family as waveform trace
- Pills provide the contrast; text color stays cohesive with display identity
- Keep existing 2-pass glow rendering (blur pass + sharp pass) on all text
- Raw CLK line gets proportionally dimmed glow (60% alpha on glow pass too)
- Free-running mode: no changes -- single Hz readout in top-left with pill background added
- Clocked mode: ratio (top-left), SYNC (top-right), BPM stack (bottom-right) -- no Hz shown
- No additional info elements added beyond what DISP-01/DISP-02 require
- BPM stack appears immediately during ACQUIRING state (alongside blinking SYNC), not waiting for LOCKED

### Claude's Discretion
- Exact gradient implementation (NVG box gradient vs radial gradient for pill edges)
- Precise pill corner radius
- Fine-tuning of feather distance if 3-4px doesn't look right at runtime
- Exact vertical spacing between CLK and BPM lines in the stacked pair

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| DISP-01 | Display text overlays (SYNC badge, ratio, BPM, Hz) readable over waveform in all brightness/color scenarios via soft-edged pill backgrounds | nvgBoxGradient API provides feathered rounded rect fills; nvgTextBounds measures text for dynamic pill sizing; draw pills before text in render order |
| DISP-02 | Incoming clock BPM displayed alongside effective (ratio-adjusted) BPM when clocked | displaySmoothedPeriod atomic already bridges clock period; raw BPM = 60/period; conditional dual-line layout with x1 collapsing via ratioIdx == 7 check |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| NanoVG | bundled with VCV Rack 2 SDK | Vector rendering for pill backgrounds and text | Already used for all display rendering; provides nvgBoxGradient for feathered fills |

### Supporting
| API Function | Signature | Purpose | When to Use |
|--------------|-----------|---------|-------------|
| `nvgBoxGradient` | `(ctx, x, y, w, h, r, f, icol, ocol)` | Create feathered rounded-rect paint | For every pill background -- `f` (feather) controls edge softness |
| `nvgTextBounds` | `(ctx, x, y, string, end, bounds[4])` | Measure text bounding box | Before drawing pill, to calculate pill dimensions |
| `nvgTextMetrics` | `(ctx, &ascender, &descender, &lineh)` | Get font metrics (line height) | For BPM stack vertical spacing calculations |
| `nvgRoundedRect` | `(ctx, x, y, w, h, r)` | Create rounded rect path | Pill shape path, filled with box gradient paint |
| `nvgFillPaint` | `(ctx, paint)` | Set fill to gradient paint | Apply box gradient to pill path |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| nvgBoxGradient | nvgRadialGradient | Radial creates circular feather, not rectangular; box gradient matches pill shape naturally |
| nvgBoxGradient | Solid nvgRoundedRect | No feathered edges; hard boundary looks harsh against display background |

**No installation needed.** All APIs are part of the VCV Rack 2 SDK already included via `#include "plugin.hpp"` which pulls in nanovg.h.

## Architecture Patterns

### Recommended Approach: Modified drawTextOverlays()

All changes happen within the existing `WaveformDisplay` struct. The render order in `drawLayer()` already has text overlays drawn last (after waveform trace and phase dot), which is correct -- pills render as part of the text overlay pass, behind text but above the waveform.

### Pattern 1: Pill-Backed Text Helper

**What:** Replace bare `drawGlowText()` calls with a new `drawPillText()` that measures text, draws a feathered pill background, then draws glow text on top.

**When to use:** Every text element that needs a pill (Hz, ratio, SYNC, BPM stack).

**Example:**
```cpp
// Source: NanoVG official API (nanovg.h lines 407-413, 539-545, 611)
void drawPillText(NVGcontext* vg, int fontHandle, float x, float y,
                  const char* text, float fontSize, int align, float alpha) {
    // Set font state for measurement (must match drawing state)
    nvgFontFaceId(vg, fontHandle);
    nvgFontSize(vg, fontSize);
    nvgTextAlign(vg, align);

    // Measure text bounds
    float bounds[4]; // [xmin, ymin, xmax, ymax]
    nvgTextBounds(vg, x, y, text, NULL, bounds);

    // Pill dimensions with padding
    float pad = 4.f;
    float feather = 3.f;
    float cornerRadius = 3.f;
    float px = bounds[0] - pad;
    float py = bounds[1] - pad;
    float pw = (bounds[2] - bounds[0]) + 2.f * pad;
    float ph = (bounds[3] - bounds[1]) + 2.f * pad;

    // Draw feathered pill background
    NVGpaint pillPaint = nvgBoxGradient(vg,
        px, py, pw, ph,
        cornerRadius, feather,
        nvgRGBAf(0.102f, 0.102f, 0.180f, 0.80f * alpha), // #1a1a2e @ 80% opacity
        nvgRGBAf(0.102f, 0.102f, 0.180f, 0.f));            // fade to transparent
    nvgBeginPath(vg);
    nvgRoundedRect(vg, px - feather, py - feather,
                   pw + 2.f * feather, ph + 2.f * feather, cornerRadius + feather);
    nvgFillPaint(vg, pillPaint);
    nvgFill(vg);

    // Draw glow text on top (existing 2-pass technique)
    nvgFontBlur(vg, 3.0f);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha * 0.4f));
    nvgText(vg, x, y, text, NULL);

    nvgFontBlur(vg, 0.0f);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha));
    nvgText(vg, x, y, text, NULL);
}
```

### Pattern 2: Dual-Line BPM Stack with Shared Pill

**What:** A dedicated function that calculates raw and effective BPM, determines whether to show one or two lines, measures combined text bounds for a shared pill, and renders.

**When to use:** Bottom-right BPM display in clocked mode.

**Example:**
```cpp
// Source: CONTEXT.md decisions + NanoVG API
void drawBpmStack(NVGcontext* vg, int fontHandle, float alpha) {
    int ratioIdx = module->displayRatioIndex.load(std::memory_order_relaxed);
    float period = module->displaySmoothedPeriod.load(std::memory_order_relaxed);
    if (ratioIdx < 0 || period <= 0.f) return;

    float margin = 4.f;
    float pad = 4.f;
    float feather = 3.f;

    // Calculate BPM values
    float rawBPM = 60.f / period;
    float effectiveBPM = rawBPM * AnalogLFO::RATIO_TABLE[ratioIdx];
    bool isX1 = (ratioIdx == 7); // RATIO_TABLE[7] = 1.0

    // Format text strings
    std::string bpmText;
    if (effectiveBPM < 1.f)
        bpmText = rack::string::f("%.1f BPM", effectiveBPM);
    else
        bpmText = rack::string::f("%d BPM", (int)std::round(effectiveBPM));

    // Effective BPM line (always shown) -- standard 10px
    float bpmFontSize = 10.f;
    float bpmX = box.size.x - margin;
    float bpmY = box.size.y - margin;

    if (isX1) {
        // Single line: just effective BPM with pill
        drawPillText(vg, fontHandle, bpmX, bpmY, bpmText.c_str(),
                     bpmFontSize, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM, alpha);
    } else {
        // Dual line: CLK on top, BPM below, shared pill
        std::string clkText;
        if (rawBPM < 1.f)
            clkText = rack::string::f("%.1f CLK", rawBPM);
        else
            clkText = rack::string::f("%d CLK", (int)std::round(rawBPM));

        float clkFontSize = 8.f;
        float lineSpacing = 2.f; // gap between CLK and BPM lines
        float clkAlpha = alpha * 0.6f;

        // Measure both lines for shared pill
        // ... measure bounds, union them, draw single pill, then both text lines
    }
}
```

### Pattern 3: Render Order (Critical)

**What:** Pill backgrounds must render after waveform but before text. Since pills and text are in the same function, the order within `drawPillText()` is: measure -> pill fill -> glow pass -> sharp pass.

**When to use:** Always. Getting render order wrong defeats the purpose.

```
drawLayer(layer=1):
  1. drawBackground()        -- deep navy fill
  2. drawInsetFrame()         -- border
  3. drawWaveformTrace()      -- amber waveform (renders OVER background)
  4. drawPhaseDot()           -- animated dot
  5. drawTextOverlays()       -- pills + text (pills occlude waveform, text on top)
```

### Anti-Patterns to Avoid
- **Drawing pills globally first, text later:** Don't separate pill and text passes. Each `drawPillText()` call must draw its own pill immediately before its text, so font state (size, alignment) stays consistent between measurement and drawing.
- **Hard-coded pill positions without text measurement:** Font metrics can vary slightly. Always measure with `nvgTextBounds()` to size pills dynamically.
- **Using nvgFillColor for pills instead of nvgFillPaint:** Solid fills have hard edges. Must use `nvgFillPaint` with `nvgBoxGradient` for the feathered effect.
- **Forgetting to set font state before nvgTextBounds:** `nvgTextBounds` uses the current font face, size, and alignment. Must call `nvgFontFaceId`, `nvgFontSize`, `nvgTextAlign` before measuring.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Feathered edge pills | Manual multi-rect alpha blending | `nvgBoxGradient` with feather parameter | GPU-accelerated, sub-pixel accurate, single draw call |
| Text bounding boxes | Character-width estimation | `nvgTextBounds()` | Font-aware, handles kerning, respects alignment |
| Line height / spacing | Hard-coded pixel values | `nvgTextMetrics()` for ascender/descender | Correct across font sizes, DPI-independent |

**Key insight:** NanoVG was designed specifically for this kind of HUD rendering. Its box gradient is literally described as "useful for rendering drop shadows or highlights for boxes" in the official header. Fighting the API (manual alpha shells, etc.) produces worse results with more code.

## Common Pitfalls

### Pitfall 1: Box Gradient Sizing Mismatch
**What goes wrong:** The pill shape path is the same size as the box gradient parameters, causing feathered edges to be clipped.
**Why it happens:** `nvgBoxGradient` defines the *solid core* of the gradient. The feather extends outward from these bounds. If the rounded rect path matches the gradient bounds exactly, the feathered edges are outside the fill path and get clipped.
**How to avoid:** Make the `nvgRoundedRect` path larger than the box gradient by the feather distance on all sides:
```cpp
// Box gradient defines the solid core
NVGpaint paint = nvgBoxGradient(vg, px, py, pw, ph, radius, feather, inner, outer);
// Path must be large enough to contain feathered edges
nvgRoundedRect(vg, px - feather, py - feather,
               pw + 2*feather, ph + 2*feather, radius + feather);
```
**Warning signs:** Pills appear with hard edges despite feather parameter being set.

### Pitfall 2: nvgTextBounds Requires Font State
**What goes wrong:** Text bounds return incorrect values (often zero or previous text's bounds).
**Why it happens:** `nvgTextBounds` uses the *current* NanoVG font state (face, size, alignment). If you measure before setting these, you get garbage.
**How to avoid:** Always call `nvgFontFaceId`, `nvgFontSize`, `nvgTextAlign` before `nvgTextBounds`. The `drawPillText` helper encapsulates this correctly.
**Warning signs:** Pills are wrong size or positioned incorrectly relative to text.

### Pitfall 3: Alpha Compositing of Pill Over Waveform Glow
**What goes wrong:** The pill background is visible but doesn't fully occlude the waveform glow passes underneath.
**Why it happens:** The waveform trace has 4-pass glow rendering with the outermost pass at 6px width. If pill opacity is too low, the bright glow halo of the waveform still shows through.
**How to avoid:** The 80% opacity on the pill core (#1a1a2e @ 0.80 alpha) should provide sufficient occlusion. The feathered edge transitions to transparent, so near the pill boundary the waveform will gently show through -- this is the desired "text emerges from a dark cloud" aesthetic. If waveform glow still bleeds through at the pill center, increase core opacity.
**Warning signs:** Text still hard to read when waveform trace passes directly through the center of a pill.

### Pitfall 4: BPM Stack Pill Sizing for Variable Content
**What goes wrong:** The shared pill for CLK/BPM lines is sized for one configuration but looks wrong when switching between x1 (single line) and non-x1 (dual line).
**Why it happens:** Pill dimensions change when content changes (one line vs two). If measured only once, the pill size is stale.
**How to avoid:** Always remeasure text bounds every frame. NanoVG text measurement is cheap (no GPU cost, just font metrics math). The pill naturally resizes as content changes, and the 200ms fade animation on `bpmFadeAlpha` provides smooth visual transitions.
**Warning signs:** Pill too large for single-line mode, or too small clipping dual-line mode.

### Pitfall 5: BPM Display During ACQUIRING State
**What goes wrong:** BPM shows "inf" or garbage values during early ACQUIRING state when smoothedPeriod is still unstable.
**Why it happens:** During ACQUIRING, smoothedPeriod transitions from 0 to the first measurement. If period is very small or zero, 60/period produces huge or infinite values.
**How to avoid:** Guard with `period > 0.f` check (already present in current code). Additionally, the BPM format (`%d` for >= 1 BPM, `%.1f` for < 1 BPM) naturally handles reasonable ranges. Extreme outliers during ACQUIRING are transient and blink-masked by the SYNC badge animation.
**Warning signs:** Flash of "999999 BPM" text for one frame during clock acquisition.

## Code Examples

### Example 1: nvgBoxGradient Pill (verified from nanovg.h)

```cpp
// Source: VCV Rack SDK, dep/include/nanovg.h lines 407-413
// Create a feathered pill background
float feather = 3.f;
float radius = 3.f;
NVGpaint pill = nvgBoxGradient(vg,
    pillX, pillY, pillW, pillH,  // solid core bounds
    radius,                        // corner radius
    feather,                       // feather distance (3-4px per CONTEXT.md)
    nvgRGBAf(0.102f, 0.102f, 0.180f, 0.80f),  // inner: #1a1a2e @ 80%
    nvgRGBAf(0.102f, 0.102f, 0.180f, 0.00f)   // outer: fade to transparent
);
nvgBeginPath(vg);
// Path extends beyond gradient bounds to contain feather
nvgRoundedRect(vg,
    pillX - feather, pillY - feather,
    pillW + 2*feather, pillH + 2*feather,
    radius + feather);
nvgFillPaint(vg, pill);
nvgFill(vg);
```

### Example 2: nvgTextBounds for Dynamic Pill Sizing (verified from nanovg.h)

```cpp
// Source: VCV Rack SDK, dep/include/nanovg.h lines 539-545, 608-611
// Must set font state before measuring
nvgFontFaceId(vg, fontHandle);
nvgFontSize(vg, 10.f);
nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);

float bounds[4]; // [xmin, ymin, xmax, ymax]
nvgTextBounds(vg, x, y, "SYNC", NULL, bounds);

float textW = bounds[2] - bounds[0];
float textH = bounds[3] - bounds[1];
// Pill = text bounds + padding on all sides
float pillX = bounds[0] - 4.f;
float pillY = bounds[1] - 4.f;
float pillW = textW + 8.f;
float pillH = textH + 8.f;
```

### Example 3: Dual-Line BPM Stack Measurement

```cpp
// Source: NanoVG nvgTextMetrics (nanovg.h line 624)
// Get line height for stacking
nvgFontFaceId(vg, fontHandle);
nvgFontSize(vg, 10.f);
float ascender, descender, lineh;
nvgTextMetrics(vg, &ascender, &descender, &lineh);

// BPM line at bottom, CLK line above it
float bpmY = box.size.y - margin;
float clkY = bpmY - lineh - 2.f; // 2px gap between lines
```

### Example 4: x1 Ratio Detection

```cpp
// Source: AnalogLFO.cpp line 47 -- RATIO_TABLE[7] = 1.0 (x1)
int ratioIdx = module->displayRatioIndex.load(std::memory_order_relaxed);
bool isX1 = (ratioIdx == 7);
if (isX1) {
    // Single line: "120 BPM"
} else {
    // Dual line: "120 CLK" over "30 BPM"
}
```

## State of the Art

| Old Approach (current code) | New Approach (this phase) | Impact |
|------------------------------|---------------------------|--------|
| Bare glow text over waveform | Pill-backed glow text | Text always readable regardless of waveform position |
| Single effective BPM readout | Dual CLK + BPM stack | Users see both raw clock and ratio-adjusted tempo |
| Same font size for all text | 8px CLK (supplemental) + 10px BPM (primary) | Visual hierarchy distinguishes primary from supplemental |

**Nothing deprecated:** The existing `drawGlowText()` function can be kept as-is for any future text that doesn't need a pill, or replaced entirely by `drawPillText()`.

## Open Questions

1. **Exact pill opacity tuning**
   - What we know: 80% opacity on #1a1a2e core was decided in CONTEXT.md
   - What's unclear: Whether 80% is sufficient to occlude the brightest waveform glow pass (4th pass at 85% alpha, 1.5px width). This depends on whether trace passes through pill center vs. edges.
   - Recommendation: Start with 80%, visually test with a square wave at full character (maximum brightness), adjust if needed. This is Claude's discretion per CONTEXT.md.

2. **BPM stack line spacing**
   - What we know: CLK above BPM, within shared pill
   - What's unclear: Exact pixel gap between lines for best readability at 8px and 10px font sizes
   - Recommendation: Use `nvgTextMetrics()` line height plus a 1-2px gap. Fine-tune visually. This is Claude's discretion per CONTEXT.md.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Manual visual inspection (display rendering) |
| Config file | none -- visual UI cannot be unit tested |
| Quick run command | `cd /Users/mrcbrown/Claude/Software/Forge\ Audio/Analog\ Series && make -j4 && cp -r plugin.dylib ~/.Rack2/plugins/ForgeAudio-AnalogSeries/` |
| Full suite command | Manual: load patch, verify overlays in free-running and clocked modes |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| DISP-01 | Pill backgrounds render behind text overlays, waveform trace does not occlude text | manual-only | Visual inspection: connect clock, observe SYNC/ratio/BPM overlays while waveform passes through text areas | N/A -- visual |
| DISP-02 | Dual BPM display shows CLK and BPM at non-x1 ratios, single BPM at x1 | manual-only | Visual inspection: set ratio to /2, verify "120 CLK" / "60 BPM" shown; set to x1, verify single "120 BPM" | N/A -- visual |

### Sampling Rate
- **Per task commit:** Build compiles without errors (`make -j4`)
- **Per wave merge:** Visual inspection in VCV Rack with test patch
- **Phase gate:** Full visual verification across all clock states and ratio settings

### Wave 0 Gaps
None -- this phase modifies existing NanoVG rendering code only. No test infrastructure is applicable to visual display rendering. Compile-time verification (successful build) is the automated gate.

## Sources

### Primary (HIGH confidence)
- VCV Rack 2 SDK `dep/include/nanovg.h` (local file) -- nvgBoxGradient, nvgTextBounds, nvgTextMetrics, nvgRoundedRect API signatures and documentation comments
- Project source `src/AnalogLFO.cpp` (local file) -- existing drawGlowText(), drawTextOverlays(), WaveformDisplay struct, display atomics, RATIO_TABLE

### Secondary (MEDIUM confidence)
- [NanoVG official header](https://github.com/memononen/nanovg/blob/master/src/nanovg.h) -- API documentation matches local SDK copy
- [NanoVG box gradient issue discussion](https://github.com/memononen/nanovg/issues/515) -- confirms feather parameter behavior and path sizing requirements

### Tertiary (LOW confidence)
- None -- all findings verified against local SDK source

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- NanoVG APIs verified against local SDK header
- Architecture: HIGH -- single function to modify, existing patterns well understood, all integration points identified
- Pitfalls: HIGH -- box gradient sizing issue confirmed by NanoVG issue tracker; font state requirement documented in official header

**Research date:** 2026-03-13
**Valid until:** Indefinite -- NanoVG API is stable, VCV Rack 2 SDK is pinned
