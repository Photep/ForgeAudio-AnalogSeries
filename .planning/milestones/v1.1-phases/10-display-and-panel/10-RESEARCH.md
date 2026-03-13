# Phase 10: Display and Panel - Research

**Researched:** 2026-03-12
**Domain:** NanoVG text rendering, VCV Rack display overlays, SVG panel editing
**Confidence:** HIGH

## Summary

Phase 10 adds text overlays to the existing WaveformDisplay widget (SYNC badge, ratio label, BPM readout, Hz readout) and updates the panel SVG with a CLK jack label. All work is display/panel -- no DSP changes. The primary technologies are NanoVG text rendering (already used for waveform trace drawing) and SVG path editing (already established pattern for panel labels).

The existing codebase provides strong foundations: the `WaveformDisplay` class already has a `drawLayer(layer==1)` pipeline with four-pass glow rendering, a `breathePhase` animation timer, and atomic display state variables (`displayClockState`, `displayRatioIndex`) that already transfer clock state from the audio thread. The main gaps are: (1) no font is currently loaded -- NanoVG text requires a font face, (2) `smoothedPeriod` is not yet exposed atomically for BPM calculation, and (3) fade animation state needs to be added for the 200ms transitions.

**Primary recommendation:** Use VCV Rack's built-in `ShareTechMono-Regular.ttf` system font (loaded via `asset::system("res/fonts/ShareTechMono-Regular.ttf")`), implement text glow via NanoVG's `nvgFontBlur()` (two-pass: blurred then sharp), and drive SYNC blink from the existing `breathePhase` timer rescaled to ~2Hz.

<user_constraints>

## User Constraints (from CONTEXT.md)

### Locked Decisions
- SYNC badge: top-right corner, amber (#e8a838) with subtle glow, no background pill, NanoVG font rendering
- SYNC states: FREE=hidden, ACQUIRING=blinks ~2Hz, LOCKED=solid amber
- Ratio label: top-left corner, shows RATIO_LABELS[] value, only visible when clocked, amber with glow
- BPM readout: bottom-right corner, shows effective BPM (clock BPM x ratio), only visible when clocked, amber with glow
- Hz readout: top-left corner (same position as ratio label), shows frequency in free-running mode, amber with glow
- Display transitions: ~200ms fade for all mode changes, immediate text swap for ratio knob changes
- Panel SVG: "CLK" label above CLK jack at (42.96, 86.0) in lavender (#8888aa) geometric path letterforms
- Panel SVG: "RATE" label above Rate knob at (18.0, 86.0) -- same lavender style (NOTE: RATE label already exists in white-gray; see research notes)

### Claude's Discretion
- Font choice for NanoVG text overlays (bundled font or VCV built-in)
- Exact font sizes for SYNC badge, ratio, BPM, Hz text
- Glow implementation details (multi-pass or blur)
- Exact fade easing curve for 200ms transitions
- BPM number formatting (integer vs one decimal place)
- SYNC blink easing (sine wave, linear, etc.)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope

</user_constraints>

<phase_requirements>

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| DISP-01 | Panel SVG updated with CLK jack and label | SVG path letterform pattern fully established; CLK jack already wired in C++ at (42.96, 86.0); only label path group missing from SVG |
| DISP-02 | Waveform display shows "SYNC" badge when clocked | `displayClockState` atomic already transfers FREE/ACQUIRING/LOCKED; NanoVG `nvgText` + `nvgFontBlur` provides text glow; `breathePhase` drives blink |
| DISP-03 | Display shows current division ratio label | `displayRatioIndex` atomic already transfers ratio index; `RATIO_LABELS[]` already defined; same NanoVG text pattern as SYNC badge |
| DISP-06 | Display shows BPM calculated from clock source and rate divider | Requires new `std::atomic<float> displaySmoothedPeriod`; BPM = 60.0 / (smoothedPeriod / ratioMultiplier); same NanoVG text pattern |

</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| NanoVG | Bundled with VCV Rack 2 SDK | 2D vector text rendering in display widget | Already used for waveform trace; only API available for in-widget text |
| ShareTechMono-Regular.ttf | System font in VCV Rack | Monospace display font for overlays | Ships with Rack (no bundling needed); clean, technical aesthetic matching Eurorack displays |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `rack::string::f()` | VCV Rack 2 SDK | printf-style string formatting | For BPM ("120 BPM") and Hz ("0.70 Hz") text |
| `std::atomic<float>` | C++17 stdlib | Lock-free audio-to-display data transfer | For exposing smoothedPeriod to display thread |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| ShareTechMono (system font) | Bundled custom font in res/fonts/ | Extra file to distribute; ShareTechMono already available, monospace, matches Eurorack display aesthetic |
| nvgFontBlur (2-pass glow) | Multi-pass text like waveform 4-pass glow | 4-pass text glow is overkill; nvgFontBlur is NanoVG's built-in text blur and achieves the effect with 2 draws instead of 8 |
| DejaVuSans (system font) | Proportional, harder to read at small sizes; ShareTechMono is designed for displays |

**Font loading:**
```cpp
// In drawLayer(), load from cache each frame (VCV Rack pattern)
std::shared_ptr<Font> font = APP->window->loadFont(
    asset::system("res/fonts/ShareTechMono-Regular.ttf"));
```

## Architecture Patterns

### Recommended Code Structure

All changes confined to two locations:

```
src/AnalogLFO.cpp
├── AnalogLFO struct
│   └── + std::atomic<float> displaySmoothedPeriod  (new)
│   └── + store displaySmoothedPeriod in process()   (new)
├── WaveformDisplay struct
│   └── + float fadeAlpha fields (syncFade, ratioFade, bpmFade, hzFade)
│   └── + int prevDisplayClockState (for detecting transitions)
│   └── step()   (+ update fade timers)
│   └── drawLayer()
│       └── + drawTextOverlays(vg)  (new method, called after drawPhaseDot)
└── AnalogLFOWidget (unchanged)

res/AnalogLFO.svg
└── + CLK label path group  (new, after RATE label)
```

### Pattern 1: NanoVG Text with Glow (Two-Pass nvgFontBlur)
**What:** Draw text twice -- first blurred for glow, then sharp on top
**When to use:** All text overlays (SYNC, ratio, BPM, Hz)
**Example:**
```cpp
// Source: NanoVG demo.c pattern, verified against nanovg.h API
void drawGlowText(NVGcontext* vg, float x, float y, const char* text,
                   float fontSize, int align, float alpha) {
    std::shared_ptr<Font> font = APP->window->loadFont(
        asset::system("res/fonts/ShareTechMono-Regular.ttf"));
    if (!font) return;

    nvgFontFaceId(vg, font->handle);
    nvgFontSize(vg, fontSize);
    nvgTextAlign(vg, align);

    // Pass 1: Glow (blurred, lower alpha)
    nvgFontBlur(vg, 3.0f);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha * 0.4f));
    nvgText(vg, x, y, text, NULL);

    // Pass 2: Sharp text on top
    nvgFontBlur(vg, 0.0f);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha));
    nvgText(vg, x, y, text, NULL);
}
```

### Pattern 2: Fade Animation via step() Timer
**What:** Track per-element fade alpha, update in step() (~60Hz), apply in drawLayer()
**When to use:** Mode transitions (clock connect/disconnect, state changes)
**Example:**
```cpp
// In WaveformDisplay:
float syncFadeAlpha = 0.f;   // 0=hidden, 1=fully visible
float ratioFadeAlpha = 0.f;
float bpmFadeAlpha = 0.f;
float hzFadeAlpha = 1.f;     // starts visible in free-running mode
int prevClockStateForFade = 0; // detect transitions

void step() override {
    // Advance breathe animation
    breathePhase += 2.f * (float)M_PI * 0.8f / 60.f;
    if (breathePhase > 2.f * (float)M_PI) breathePhase -= 2.f * (float)M_PI;

    if (!module) { TransparentWidget::step(); return; }

    int clockState = module->displayClockState.load(std::memory_order_relaxed);
    float fadeSpeed = 1.f / (0.2f * 60.f);  // 200ms at ~60fps

    // Target alphas based on clock state
    bool showClocked = (clockState != 0);  // ACQUIRING or LOCKED
    float syncTarget = showClocked ? 1.f : 0.f;
    float ratioTarget = showClocked ? 1.f : 0.f;
    float bpmTarget = showClocked ? 1.f : 0.f;
    float hzTarget = showClocked ? 0.f : 1.f;

    // Approach targets
    syncFadeAlpha += rack::math::clamp(syncTarget - syncFadeAlpha, -fadeSpeed, fadeSpeed);
    ratioFadeAlpha += rack::math::clamp(ratioTarget - ratioFadeAlpha, -fadeSpeed, fadeSpeed);
    bpmFadeAlpha += rack::math::clamp(bpmTarget - bpmFadeAlpha, -fadeSpeed, fadeSpeed);
    hzFadeAlpha += rack::math::clamp(hzTarget - hzFadeAlpha, -fadeSpeed, fadeSpeed);

    TransparentWidget::step();
}
```

### Pattern 3: SYNC Badge Blink (ACQUIRING State)
**What:** Modulate SYNC badge alpha with sine wave at ~2Hz when ACQUIRING
**When to use:** ACQUIRING clock state only
**Example:**
```cpp
// In drawTextOverlays:
int clockState = module->displayClockState.load(std::memory_order_relaxed);
if (syncFadeAlpha > 0.001f) {
    float effectiveAlpha = syncFadeAlpha;
    if (clockState == AnalogLFO::ACQUIRING) {
        // Blink at ~2Hz using a dedicated blink phase
        // breathePhase runs at 0.8Hz, scale up: 2Hz/0.8Hz = 2.5x
        float blink = 0.5f + 0.5f * std::sin(breathePhase * 2.5f);
        effectiveAlpha *= blink;
    }
    drawGlowText(vg, syncX, syncY, "SYNC", fontSize,
                 NVG_ALIGN_RIGHT | NVG_ALIGN_TOP, effectiveAlpha);
}
```

### Pattern 4: BPM Calculation from Atomic Period
**What:** Display thread reads smoothed period, computes effective BPM with ratio
**When to use:** BPM readout in clocked mode
**Example:**
```cpp
// In AnalogLFO::process(), after smoothedPeriod is updated:
displaySmoothedPeriod.store(smoothedPeriod, std::memory_order_relaxed);

// In WaveformDisplay::drawTextOverlays():
float period = module->displaySmoothedPeriod.load(std::memory_order_relaxed);
int ratioIdx = module->displayRatioIndex.load(std::memory_order_relaxed);
if (period > 0.f && ratioIdx >= 0) {
    float clockBPM = 60.f / period;
    float effectiveBPM = clockBPM * AnalogLFO::RATIO_TABLE[ratioIdx];
    std::string bpmText = rack::string::f("%d BPM", (int)std::round(effectiveBPM));
    drawGlowText(vg, bpmX, bpmY, bpmText.c_str(), fontSize,
                 NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM, bpmFadeAlpha);
}
```

### Pattern 5: SVG Geometric Path Letterforms
**What:** Add text as geometric `<path>` elements with fill color, no `<text>` elements
**When to use:** Panel SVG labels (CLK label)
**Example from existing SVG:**
```xml
<!-- "CLK" label centered at x=42.96mm, y=78.6mm (above CLK jack) -->
<!-- C=3 L=3 K=3, spacing 0.5 => total 10 units -->
<!-- scale=0.35, Scaled: 10*0.35 = 3.5, startX = 42.96 - 3.5/2 = 41.21 -->
<g transform="translate(41.21, 78.6) scale(0.35)">
  <!-- C -->
  <path fill="#8888aa" d="M0.6,0 L3,0 L3,0.9 L0.9,0.9 L0.9,4.1 L3,4.1 L3,5 L0.6,5 L0,4.4 L0,0.6 Z"/>
  <!-- L at x=3.5 -->
  <path fill="#8888aa" d="M3.5,0 L4.4,0 L4.4,4.1 L6.5,4.1 L6.5,5 L3.5,5 Z"/>
  <!-- K at x=7.0 -->
  <path fill="#8888aa" d="M7,0 L7.9,0 L7.9,2 L9.4,0 L10.5,0 L8.6,2.3 L10.5,5 L9.4,5 L7.9,3 L7.9,5 L7,5 Z"/>
</g>
```
**Note on RATE label:** The SVG already has a "RATE" label at the correct position in white-gray (#c0c0d0), matching knob label style. The CONTEXT.md mentions adding RATE in lavender (#8888aa). Since all existing knob labels (MORPH, CHARACTER, DRIFT, RATE) use #c0c0d0 and all jack labels (MORPH CV, CHAR CV, DRIFT CV) use #8888aa, the CLK label should be #8888aa (jack label) but the RATE label should remain #c0c0d0 (knob label) for consistency.

### Anti-Patterns to Avoid
- **Loading font in constructor:** Font must be loaded each frame from cache in `drawLayer()` -- the NVGcontext is only valid during draw calls. Storing the font handle between frames causes crashes.
- **Reading non-atomic smoothedPeriod from display thread:** Float reads across threads are not guaranteed atomic on all platforms. Must use `std::atomic<float>` with relaxed ordering.
- **Rendering text on layer 0:** Text overlays are self-illuminating and must be drawn on layer 1 (inside the existing `if (layer == 1)` block) to render correctly with VCV Rack's light simulation.
- **Using `<text>` elements in SVG:** nanosvg does not support `<text>`, `<defs>`, `<style>`, or `<filter>` elements. All text must be geometric `<path>` letterforms.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Text glow effect | Custom multi-pass text rendering (4 passes at different sizes) | `nvgFontBlur()` two-pass pattern | NanoVG has built-in font blur; two draws (blurred + sharp) is simpler, faster, and looks correct |
| String formatting | Manual char buffers / snprintf | `rack::string::f()` | VCV Rack's printf-style formatter, handles memory allocation |
| Font caching | Manual font handle storage | `APP->window->loadFont()` | VCV Rack caches internally; returns same shared_ptr for same path |
| Fade easing | Custom easing functions | Linear clamp approach in step() | 200ms is fast enough that linear fade looks smooth; cosine easing adds no perceptible benefit at this speed |

**Key insight:** NanoVG already provides text blur as a first-class feature -- no need to replicate the four-pass waveform glow pattern for text. The `nvgFontBlur()` approach is specifically designed for text glow/shadow effects.

## Common Pitfalls

### Pitfall 1: Font Loading Returns nullptr
**What goes wrong:** `APP->window->loadFont()` returns nullptr if the font path is wrong or the font file doesn't exist. Drawing with a null font handle crashes.
**Why it happens:** Typo in font path, or using `asset::plugin()` when the font is a system font (or vice versa).
**How to avoid:** Always null-check the font before calling `nvgFontFaceId()`. Use `asset::system()` for built-in Rack fonts, `asset::plugin()` for bundled fonts.
**Warning signs:** Crash in `nvgFontFaceId` or missing text with no error.

### Pitfall 2: Forgetting nvgFontBlur(0) After Glow Pass
**What goes wrong:** All subsequent text draws are blurred because NanoVG font state is persistent within a frame.
**Why it happens:** NanoVG text state (size, blur, align, font face) persists until explicitly changed.
**How to avoid:** Always reset `nvgFontBlur(vg, 0.0f)` after the glow pass. Or better: set all text state before each draw call.
**Warning signs:** All text looks blurry/fuzzy.

### Pitfall 3: Text Position in Pixel vs mm Coordinates
**What goes wrong:** Text appears at wrong position because the widget's local coordinate system is in pixels (after mm2px conversion), not mm.
**Why it happens:** The WaveformDisplay's `box.size` is already in pixels. Text coordinates within `drawLayer()` are pixel-relative to the widget origin.
**How to avoid:** Use `box.size.x` and `box.size.y` for relative positioning. The display is approximately 215px x 102px.
**Warning signs:** Text outside the display bounds, overlapping the waveform.

### Pitfall 4: SYNC Blink Frequency Mismatch
**What goes wrong:** SYNC badge blinks too fast or too slow because the breathe phase runs at 0.8Hz, not 2Hz.
**Why it happens:** Reusing `breathePhase` directly without frequency scaling.
**How to avoid:** Either multiply `breathePhase` by 2.5 (to get 2Hz from 0.8Hz), or add a separate blink timer. Multiplying is simpler since the existing timer already advances in `step()`.
**Warning signs:** Blink rate visually "off" compared to clock rate.

### Pitfall 5: Display Thread Reading Audio-Thread-Only Variables
**What goes wrong:** Data race on `smoothedPeriod` (plain float, not atomic).
**Why it happens:** `smoothedPeriod` was added in Phase 7 as an internal audio-thread variable. Phase 10 needs it for BPM display.
**How to avoid:** Add `std::atomic<float> displaySmoothedPeriod` and store it in `process()` alongside the existing atomic stores.
**Warning signs:** Occasional garbage BPM values, rare crashes on some platforms.

### Pitfall 6: SVG Path Letterform Alignment
**What goes wrong:** CLK label appears misaligned or at wrong position.
**Why it happens:** Geometric path coordinates are in a local coordinate system that gets scaled and translated. The startX calculation must account for total text width and centering.
**How to avoid:** Follow the exact pattern used for existing labels: calculate total unit width, multiply by scale, compute startX = centerX - scaledWidth/2.
**Warning signs:** Label visually off-center or overlapping the jack.

## Code Examples

### Complete drawTextOverlays Method Structure
```cpp
// Source: Synthesized from VCV Rack Plugin API Guide patterns and existing WaveformDisplay code
void drawTextOverlays(NVGcontext* vg) {
    std::shared_ptr<Font> font = APP->window->loadFont(
        asset::system("res/fonts/ShareTechMono-Regular.ttf"));
    if (!font) return;

    float margin = 4.f;
    float fontSize = 10.f;  // Tune as needed; ~10px is readable at display scale

    int clockState = module->displayClockState.load(std::memory_order_relaxed);
    int ratioIdx = module->displayRatioIndex.load(std::memory_order_relaxed);

    // Hz readout (free-running mode, top-left)
    if (hzFadeAlpha > 0.001f && ratioIdx < 0) {
        float rate = module->params[AnalogLFO::RATE_PARAM].getValue();
        std::string hzText = rack::string::f("%.2f Hz", rate);
        drawGlowText(vg, font->handle, margin, margin + fontSize,
                     hzText.c_str(), fontSize,
                     NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM, hzFadeAlpha);
    }

    // Ratio label (clocked mode, top-left)
    if (ratioFadeAlpha > 0.001f && ratioIdx >= 0) {
        drawGlowText(vg, font->handle, margin, margin + fontSize,
                     AnalogLFO::RATIO_LABELS[ratioIdx], fontSize,
                     NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM, ratioFadeAlpha);
    }

    // SYNC badge (clocked mode, top-right)
    if (syncFadeAlpha > 0.001f) {
        float effectiveAlpha = syncFadeAlpha;
        if (clockState == AnalogLFO::ACQUIRING) {
            float blink = 0.5f + 0.5f * std::sin(breathePhase * 2.5f);
            effectiveAlpha *= blink;
        }
        drawGlowText(vg, font->handle, box.size.x - margin, margin + fontSize,
                     "SYNC", fontSize,
                     NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM, effectiveAlpha);
    }

    // BPM readout (clocked mode, bottom-right)
    if (bpmFadeAlpha > 0.001f && ratioIdx >= 0) {
        float period = module->displaySmoothedPeriod.load(std::memory_order_relaxed);
        if (period > 0.f) {
            float effectiveBPM = 60.f / period * AnalogLFO::RATIO_TABLE[ratioIdx];
            std::string bpmText = rack::string::f("%d BPM", (int)std::round(effectiveBPM));
            drawGlowText(vg, font->handle, box.size.x - margin,
                         box.size.y - margin,
                         bpmText.c_str(), fontSize,
                         NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM, bpmFadeAlpha);
        }
    }
}
```

### Helper: drawGlowText
```cpp
// Source: NanoVG demo.c nvgFontBlur pattern + VCV Rack Plugin API Guide
void drawGlowText(NVGcontext* vg, int fontHandle, float x, float y,
                   const char* text, float fontSize, int align, float alpha) {
    nvgFontFaceId(vg, fontHandle);
    nvgFontSize(vg, fontSize);
    nvgTextAlign(vg, align);

    // Pass 1: Glow
    nvgFontBlur(vg, 3.0f);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha * 0.4f));
    nvgText(vg, x, y, text, NULL);

    // Pass 2: Sharp
    nvgFontBlur(vg, 0.0f);
    nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha));
    nvgText(vg, x, y, text, NULL);
}
```

### Integration Point in drawLayer()
```cpp
// In WaveformDisplay::drawLayer(), after drawPhaseDot():
void drawLayer(const DrawArgs& args, int layer) override {
    if (layer == 1) {
        NVGcontext* vg = args.vg;
        nvgSave(vg);
        nvgScissor(vg, 0, 0, box.size.x, box.size.y);

        drawBackground(vg);
        drawInsetFrame(vg);

        if (module) {
            // ... existing buffer/phase/trace/dot code ...
            drawWaveformTrace(vg, buffer, dimFactor);
            drawPhaseDot(vg, buffer, phase, dimFactor);
            drawTextOverlays(vg);   // <-- NEW: text overlays on top
        } else {
            drawPlaceholder(vg);
        }

        nvgResetScissor(vg);
        nvgRestore(vg);
    }
    TransparentWidget::drawLayer(args, layer);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `Font::load()` static method | `APP->window->loadFont()` | VCV Rack 2.0 | Old method deprecated; new method handles caching and context properly |
| Custom font bundling | System fonts via `asset::system()` | VCV Rack 2.0 | 9 fonts bundled with Rack; no need to distribute common fonts |
| Storing font handle in constructor | Loading from cache each draw frame | VCV Rack 2.0 | NVGcontext invalidation between frames means font must be re-resolved |

**Deprecated/outdated:**
- `Font::load()` static method: Deprecated in VCV Rack 2. Use `APP->window->loadFont()`.
- `rack0.hpp` compatibility: Old `stringf` macro replaced by `string::f` in rack namespace.

## Open Questions

1. **Exact font size for display overlays**
   - What we know: The display is ~215px x ~102px in widget coordinates. Text needs to be readable but not dominate the waveform.
   - What's unclear: Exact pixel size that looks good at different zoom levels.
   - Recommendation: Start with 10px for all overlays. The monospace ShareTechMono at 10px should be ~4-5 characters wide for "SYNC" and readable. Tune during implementation by visual testing in VCV Rack.

2. **RATE label color discrepancy**
   - What we know: The CONTEXT.md says to add "RATE" label in lavender (#8888aa). The SVG already has a "RATE" label in white-gray (#c0c0d0), consistent with other knob labels (MORPH, CHARACTER, DRIFT).
   - What's unclear: Whether the user intended to change the existing RATE label color or was unaware it already existed.
   - Recommendation: Keep existing RATE label as-is (#c0c0d0, matching knob label convention). Add only the CLK label in #8888aa (matching jack label convention: MORPH CV, CHAR CV, DRIFT CV). Flag this for user review during verification.

3. **BPM formatting: integer vs decimal**
   - What we know: Common in music software to show integer BPM for typical tempos (60-200), but decimal for slow/fast.
   - Recommendation: Use integer formatting (`%d BPM`) since effective BPM after ratio application will usually be a clean-ish number, and decimal precision adds visual noise. If BPM < 1, show one decimal ("0.5 BPM").

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Manual VCV Rack testing (no automated GUI test framework) |
| Config file | none |
| Quick run command | `make install && open /Applications/Rack2Free.app` |
| Full suite command | `make install && open /Applications/Rack2Free.app` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| DISP-01 | CLK label visible on panel SVG | manual-only | Visual inspection in VCV Rack module browser | N/A |
| DISP-02 | SYNC badge shows/hides/blinks based on clock state | manual-only | Patch clock source to CLK jack, observe display | N/A |
| DISP-03 | Ratio label updates when Rate knob turned in clocked mode | manual-only | Turn Rate knob while clock connected, observe display | N/A |
| DISP-06 | BPM readout shows correct effective BPM | manual-only | Connect known-BPM clock, verify calculation | N/A |

**Justification for manual-only:** All requirements are visual display behaviors. VCV Rack does not provide automated GUI testing infrastructure. Verification requires visual observation of NanoVG rendering in the running application.

### Sampling Rate
- **Per task commit:** `make` (compile check)
- **Per wave merge:** `make install` + manual VCV Rack testing
- **Phase gate:** All four requirements visually verified in VCV Rack

### Wave 0 Gaps
None -- no test infrastructure applicable. Manual test protocol covers all requirements.

## Sources

### Primary (HIGH confidence)
- VCV Rack SDK `nanovg.h` at `/Users/mrcbrown/Claude/Software/Forge Audio/Rack-SDK/dep/include/nanovg.h` -- full NanoVG text API (nvgText, nvgFontBlur, nvgFontSize, nvgTextAlign, nvgFontFaceId, nvgGlobalAlpha)
- VCV Rack Plugin API Guide (https://vcvrack.com/manual/PluginGuide) -- font loading, drawLayer patterns, NanoVG text example code
- VCV Rack Font struct API (https://vcvrack.com/docs-v2/structrack_1_1window_1_1Font) -- `APP->window->loadFont()` usage
- Existing `AnalogLFO.cpp` source code -- WaveformDisplay class, atomic display variables, four-pass glow pattern, breathePhase timer

### Secondary (MEDIUM confidence)
- VCV Community font discussion (https://community.vcvrack.com/t/what-are-best-practices-for-distributing-fonts/17271) -- confirmed system fonts: ShareTechMono-Regular.ttf, DejaVuSans.ttf, etc. loaded via `asset::system("res/fonts/...")`
- VCV Community NanoVG label discussion (https://community.vcvrack.com/t/advanced-nanovg-custom-label/6769) -- nvgFontFaceId/nvgText usage patterns confirmed
- NanoVG demo.c (https://github.com/memononen/nanovg/blob/master/example/demo.c) -- nvgFontBlur glow/shadow pattern

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- NanoVG text API verified directly from SDK header; font paths verified from community discussion with multiple confirmations
- Architecture: HIGH -- all patterns derive from existing codebase (atomic transfer, drawLayer, step timer) with minimal new concepts
- Pitfalls: HIGH -- font loading, thread safety, and SVG constraints all well-established from 9 prior phases of development
- Code examples: MEDIUM -- synthesized from verified API patterns but not compiled/tested; font size and positions will need tuning

**Research date:** 2026-03-12
**Valid until:** 2026-06-12 (stable domain -- NanoVG and VCV Rack 2 APIs frozen)
