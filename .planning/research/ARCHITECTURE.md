# Architecture Research: v1.3 Forge Noir Integration

**Domain:** Feature integration into existing VCV Rack 2 LFO module
**Researched:** 2026-03-27
**Confidence:** HIGH (existing codebase fully analyzed, all integration points identified in code, VCV SDK constraints verified against official docs)

## Current Architecture Snapshot (Post v1.2)

The AnalogLFO is a 1,359-line single-file module (`src/AnalogLFO.cpp`) with three structs:

1. **`AnalogLFO` (Module)** -- DSP: phase accumulator, waveform engine (4 shapes + morph + bleed), character engine, drift engine (4 OU layers + DC offset), clock tracker (3-state FSM), swing, FM, phase offset, reset, display buffer management. All DSP in `process()`.
2. **`WaveformDisplay` (TransparentWidget)** -- NanoVG rendering in `drawLayer()` layer 1: waveform trace (4-pass glow), phase dot (comet trail + breathe + drift jitter), text overlays (SYNC, ratio, BPM, Hz, swing) with individual pill backgrounds and 200ms fade animations.
3. **`AnalogLFOWidget` (ModuleWidget)** -- Panel layout: SVG panel via `createPanel()`, knob/jack/trimpot placement, context menu (swing presets).

### Current Data Flow (Per Sample)

```
process():
  1. processClockInput(sampleTime)          // edge detection, EMA period, phase reset
  2. processResetInput(sampleTime)          // reset trigger + 1ms blanking
  3. Compute targetFreq (clocked or free)
  4. Apply frequency slew (freqSlew)
  5. Read drift params + CV -> drift amount
  6. Apply pitch slew (driftSlew, thermal lag)
  7. Apply FM (exponential, authority-scaled)
  8. deltaPhase = freq * sampleTime         // double precision
  9. If drift > 0: run 4 OU layers + jitter + DC offset
  10. Apply swing warp to deltaPhase (clocked only)
  11. phase += deltaPhase; wrap [0, 1)
  12. Read morph/character params + CV
  13. Update display buffer on wrap or param change (~30fps rate limit)
  14. Apply phase offset at readout
  15. computeMorphedWave(p, morph, character) -> sample [-1, +1]
  16. Apply crossfade if recent phase reset
  17. Apply DC offset wander (after crossfade)
  18. Output: sample * 5V
```

### Current Waveform Engine

```
computeMorphedWave(phase, morph, character):
  1. Compute 4 shapes: sine, tri, saw, sqr      // each with character deformation
  2. shapes[4] = { sine, tri, saw, sqr }
  3. scaled = morph * 3.0                         // morph [0,1] -> [0,3]
  4. segment = min(floor(scaled), 2)              // 0=sine-tri, 1=tri-saw, 2=saw-sqr
  5. frac = scaled - segment
  6. result = shapes[segment] + frac * (shapes[segment+1] - shapes[segment])
  7. If character > 0: apply waveform bleed (neighbor crosstalk, ring topology)
  8. return result
```

### Current Display Architecture

```
Audio Thread (process())                GUI Thread (step()/drawLayer())
  |                                       |
  |-- displayBuffers[2] (double buf) ---->|  displayReadIdx (atomic, acquire/release)
  |-- displayPhase (atomic, relaxed) ---->|  phase dot position
  |-- displayDrift (atomic, relaxed) ---->|  drift jitter intensity
  |-- displayClockState (atomic) -------->|  SYNC badge visibility
  |-- displayRatioIndex (atomic) -------->|  ratio label text
  |-- displaySmoothedPeriod (atomic) ---->|  BPM calculation
  |-- displaySwingIndex (atomic) -------->|  swing overlay text
  |-- displaySwingFraction (atomic) ----->|  swing phase-to-time remap for dot
```

### Current Panel (12HP, 60.96mm)

SVG panel at `res/AnalogLFO.svg`: nanosvg-compatible (no `<text>`, no `<use>`, no `<defs>`, no `<style>`, no filters). All text rendered as `<path>` letterforms. FM and Phase Offset controls at temporary positions noted in code comments.

---

## v1.3 Integration Analysis

### Feature 1: PWM Extension of computeMorphedWave()

**What changes:** Extend morph range from [sine, tri, saw, sqr] to [sine, tri, saw, sqr, pulse]. The 5th waveform is a narrow pulse, making PWM an organic extension of the morph knob sweep rather than a separate control.

**Integration point:** `computeMorphedWave()` at line 299.

**Current state (modified):**
```cpp
float computeMorphedWave(float phase, float morph, float character) {
    float shapes[4] = { sine, tri, saw, sqr };
    float scaled = morph * 3.f;           // 4 shapes = 3 segments
    int segment = std::min((int)scaled, 2);
    // ...crossfade + bleed...
}
```

**Target state:**
```cpp
float computePulse(float phase, float character) {
    // Narrow pulse: 12.5% duty cycle base, character narrows further
    // At character=0: clean digital pulse at 12.5% duty
    // At character=1: narrower (~5%) with soft edges like computeSquare's tanh approach
    float c = progressiveCurve(character);
    float baseDuty = 0.125f - c * 0.075f;  // 12.5% -> 5% at full character
    float duty = baseDuty + pulseWidthSpread;  // component spread (new member)
    // Reuse computeSquare's sigmoid edge softening pattern
    float edgeWidth = c * 0.06f;
    float sharpness = 1.f / std::fmax(edgeWidth, 0.001f);
    // ... tanh-based soft pulse (same pattern as computeSquare)
}

float computeMorphedWave(float phase, float morph, float character) {
    float pulse = computePulse(phase, character);
    float shapes[5] = { sine, tri, saw, sqr, pulse };  // 5 shapes
    float scaled = morph * 4.f;           // 5 shapes = 4 segments
    int segment = std::min((int)scaled, 3);
    // ...crossfade unchanged...
    // Bleed: ring topology now wraps 5 shapes (sine-tri-saw-sqr-pulse-sine)
    int leftIdx  = (segment - 1 + 5) % 5;
    int rightIdx = (segment + 2) % 5;
    // ...rest unchanged...
}
```

**What is new vs modified:**

| Item | Status | Details |
|------|--------|---------|
| `computePulse()` | NEW | New function, follows existing `computeSquare()` pattern |
| `pulseWidthSpread` member | NEW | Added to component spread, like `squareDutySpread` |
| `initComponentSpread()` | MODIFIED | Add one more `d(spreadRng) * 0.01f` call for pulseWidthSpread |
| `computeMorphedWave()` | MODIFIED | shapes[4] -> shapes[5], morph * 3.f -> morph * 4.f, segment clamp 2 -> 3 |
| Bleed ring modulus | MODIFIED | `% 4` -> `% 5` in neighbor calculation |
| `updateDisplayBuffer()` | UNCHANGED | Already calls `computeMorphedWave()` -- automatically picks up the new range |
| Display buffer | UNCHANGED | 256 samples still sufficient for narrow pulse visibility |

**Serialization impact:** `pulseWidthSpread` is derived from `spreadSeed` (deterministic), so no new JSON fields needed. Existing patches with `morph=1.0` (full square) will still produce square because the new segment (sqr->pulse) occupies morph [0.75, 1.0] in the 5-shape mapping, so morph=1.0 now produces full pulse. **This is a behavioral change for saved patches** -- morph values above 0.75 will shift. Users with morph CV patched to full range will hear different shapes at the top of the range.

**Mitigation:** This is acceptable because (a) it is the explicit design intent per PROJECT.md, (b) LFO morph positions are rarely saved as precise values, and (c) the change is musical (more shapes = richer sweep).

**Risk:** LOW. The pattern exactly mirrors existing `computeSquare()`. The only new DSP math is duty cycle + tanh sigmoid, both proven in the existing codebase.

---

### Feature 2: New 14HP SVG Panel (Forge Noir)

**What changes:** Replace `res/AnalogLFO.svg` (12HP, `#1a1a2e` theme) with a new 14HP SVG in the Forge Noir design language (`#0c0c0c` near-black, ember accents, forged metal aesthetic).

**Integration point:** `AnalogLFOWidget` constructor at line 1296.

**nanosvg constraints (verified via official VCV docs and community):**

| SVG Feature | Supported? | Workaround |
|-------------|------------|------------|
| Basic shapes (rect, circle, path, line, polygon) | YES | -- |
| Fill colors (hex, rgb) | YES | -- |
| Stroke colors and widths | YES | -- |
| Opacity attribute | YES | -- |
| Simple linear gradients (2-color, inline) | PARTIALLY | Define gradient before reference; avoid xlink:href chaining |
| Radial gradients | PARTIALLY | Same caveats as linear |
| `<text>` / fonts | NO | Convert all text to `<path>` outlines |
| `<use>` / `<defs>` references | NO | Inline all geometry; no reusable symbols |
| `<style>` / CSS | NO | Use inline attributes only |
| `<filter>` (blur, drop-shadow) | NO | Omit all filter effects |
| `<clipPath>` | NO | Use path geometry directly |
| `<mask>` | NO | Use opacity on overlapping shapes |
| `transform` on groups | YES | translate, scale, rotate supported |
| fill-rule="evenodd" | YES | Used in existing panel |
| Nested `<g>` groups | YES | For organizational clarity |

**Panel design strategy:**

The Forge Noir mockup (`forge-noir.html`) uses CSS effects that are impossible in nanosvg: box-shadow, repeating-conic-gradient, blur filters, clip-path polygons, animated glows. The SVG panel must approximate these with layered geometry:

1. **Panel background:** Single `<rect>` fill `#0c0c0c`
2. **Accent bars (top/bottom):** `<rect>` with a simple gradient (2-stop: `#e85d26` center, dark edges) or solid `#e85d26` since complex multi-stop gradients are unreliable in nanosvg
3. **Hex bolts:** `<polygon>` hexagons (6-vertex) with fill colors approximating the gradient -- outer hex `#2a2a2a`, inner hex `#0a0a0a`
4. **Brand text ("FORGE" / "AUDIO"):** Outlines as `<path>` in FoundationLogo letterforms, fill `#e85d26`
5. **Forge rune glyph:** Pure geometry -- diamonds as `<polygon>`, lines as `<line>`, circles as `<circle>`. No blur glow (omit the filter). The rune reads fine without blur at module scale.
6. **Module name ("ANALOG LFO"):** Bebas Neue outlines as `<path>`, fill `#e8e4e0`
7. **Decorative line:** `<line>` or thin `<rect>` in `#e85d26` at low opacity (0.25)
8. **Knob labels:** All `<path>` letterforms in Chakra Petch / JetBrains Mono outlines
9. **Section dividers:** Thin `<rect>` with opacity
10. **Forge emblem:** Simplified -- molten streams as `<path>` curves, ember particles as `<circle>`, chevrons as `<path>`. Omit conic gradients and blur. Use low opacity fills.
11. **Display placeholder:** `<rect>` at display position, fill `#030303`, stroke `rgba(232,93,38,0.3)` -- the actual display is NanoVG-rendered on top
12. **Component indicators:** Component outlines for knob/jack/trimpot positions (optional, VCV standard widgets render on top)

**What is new vs modified:**

| Item | Status | Details |
|------|--------|---------|
| `res/AnalogLFO.svg` | REPLACED | New 14HP SVG (71.12mm x 128.5mm). Keep old in git history. |
| `AnalogLFOWidget` constructor | MODIFIED | All `mm2px(Vec(...))` positions must be updated for 14HP layout |
| Component positions | MODIFIED | 1-2-2 diamond knob layout per DESIGN-LANGUAGE.md |
| Screw positions | MODIFIED | `box.size.x` changes from 12HP to 14HP |
| Display position/size | MODIFIED | New position within 14HP layout |
| FM/Phase Offset positions | MODIFIED | Moved from "TEMPORARY" to final Forge Noir positions |

**14HP Layout (from DESIGN-LANGUAGE.md):**

```
Panel: 71.12mm wide x 128.5mm tall
Center axis: 35.56mm (= 71.12/2)

Brand + Name zone:     y ~6-12mm
Display:               y ~13-31mm (18mm tall, ~2mm side margins)
MORPH (hero):          centered, y ~37mm
CHARACTER / DRIFT:     flanking at x~21.2mm, x~49.9mm, y ~45mm
RATE / PHASE:          same columns, y ~55mm
CLK / RST / OUTPUT:    y ~67mm, evenly spaced
CV section:            5 columns, trimpots y ~74mm, jacks y ~84mm
```

*Note: exact mm values need validation against mockup coordinates. The mockup uses 5x pixel scale (356px = 71.12mm), so divide px by 5 to get mm.*

**Risk:** MEDIUM. The SVG translation from CSS mockup to nanosvg-compatible geometry is the most labor-intensive task. Gradient compatibility is the main uncertainty -- if multi-stop gradients fail, fall back to solid fills at averaged colors. The knob/jack widget positions are straightforward arithmetic.

---

### Feature 3: Display Layout Restructuring (Three-Column)

**What changes:** Reorganize the WaveformDisplay's NanoVG-drawn overlay layout from "pills anywhere over waveform" to "left column / center waveform / right column" per the Forge Noir design language.

**Integration point:** `WaveformDisplay::drawTextOverlays()` at line 1188 and coordinate helpers at lines 848-856.

**Current layout (all overlays on top of waveform):**

```
+-----------------------------------------+
| [Hz/Ratio] top-left     [SYNC] top-right|
|                                         |
|          ~ waveform trace ~             |
|          ~ with phase dot ~             |
|                                         |
| [SWING] bot-left      [BPM] bot-right  |
+-----------------------------------------+
```

**Target layout (three-column, per mockup):**

```
+------+-------------------------+-------+
| Ratio|                         | SYNC  |
| Hz   |   ~ waveform trace ~   |       |
| readout|  ~ with phase dot ~  | CLK   |
| Swing|                         | BPM   |
+------+-------------------------+-------+
 ~55px    center column ~192px    ~55px
```

At display SVG coordinates (320x90 viewBox), left column occupies x: 16-55, center x: 64-256, right column x: 262-302. In NanoVG mm coordinates at the actual display widget size, these need proportional mapping.

**What is new vs modified:**

| Item | Status | Details |
|------|--------|---------|
| `phaseToX()` | MODIFIED | Waveform trace confined to center column (add left/right inset for pill columns) |
| `drawTextOverlays()` | MODIFIED | Rearrange all pill positions to left/right margin columns |
| `drawBpmStack()` | MODIFIED | Reposition to right column |
| `drawPillText()` | POTENTIALLY MODIFIED | Pill styling may change from feathered boxGradient to Forge Noir ember-tinted pill (outlined rect with low-opacity ember fill) |
| Display background color | MODIFIED | `#0d0d1a` -> `#030303` (Forge Noir display BG) |
| Inset frame | MODIFIED | Amber border color to match `rgba(232,93,38,0.3)` |
| Corner bracket accents | NEW | Four L-shaped ember corner marks inside display (NanoVG `nvgLineTo` paths) |
| Scanline overlay | NEW (optional) | Faint horizontal line pattern for CRT effect (performance-cheap: few `nvgRect` calls) |
| `valueToY()` | UNCHANGED | Vertical mapping stays the same |
| Waveform trace rendering | UNCHANGED | Same 4-pass glow, same stroke widths |
| Phase dot rendering | UNCHANGED | Same comet trail + breathe + jitter |
| Fade animation system | UNCHANGED | Same 200ms fade in/out with alpha interpolation |

**Key constraint:** The display is a `TransparentWidget` drawing in `drawLayer()` layer 1. This means it draws on top of the SVG panel. The display dimensions are set in `AnalogLFOWidget` constructor via `display->box.pos` and `display->box.size` -- these change with the 14HP layout.

**Coordinate mapping approach:** Define constants for the three-column boundaries relative to `box.size.x`:

```cpp
// Column boundaries (proportional to display width)
float leftColRight() const { return box.size.x * 0.18f; }   // ~18% for left pills
float centerLeft() const { return box.size.x * 0.20f; }     // center waveform start
float centerRight() const { return box.size.x * 0.80f; }    // center waveform end
float rightColLeft() const { return box.size.x * 0.82f; }   // right pills start
```

Then `phaseToX()` maps phase [0,1] to [centerLeft(), centerRight()] instead of the current full-width mapping.

**Risk:** LOW. This is purely cosmetic NanoVG repositioning. No DSP changes. No new atomics. The coordinate math is trivial. The main risk is aesthetic -- getting pill sizes, fonts, and spacing to match the mockup requires iteration, but that is a polish concern, not an architectural one.

---

### Feature 4: Animated SYNC Badge (Clock-Pulse Flash)

**What changes:** When the module receives a clock edge, the SYNC badge briefly flashes brighter (a "pulse" animation), providing visual feedback that clock pulses are being received.

**Integration point:** `WaveformDisplay::step()` at line 816 and `drawTextOverlays()` SYNC section at line 1216.

**Current SYNC animation:**
- LOCKED: steady full alpha
- ACQUIRING: ~2Hz blink using `breathePhase * 2.5` sinusoidal
- FREE: faded out (0 alpha via 200ms fade)

**Target SYNC animation (additive):**
- All existing behavior preserved
- NEW: On each clock edge, a brief brightness flash (100ms decay) overlaid on the badge

**Implementation approach -- clock pulse bridge:**

The audio thread already detects clock edges in `processClockInput()`. We need a lightweight signal from audio thread to GUI thread indicating "a clock edge just occurred." The existing pattern uses `std::atomic` relaxed loads.

```cpp
// In AnalogLFO module:
std::atomic<uint32_t> clockPulseCount{0};  // NEW: monotonic edge counter

// In processClockInput(), at each detected edge:
clockPulseCount.fetch_add(1, std::memory_order_relaxed);
```

```cpp
// In WaveformDisplay:
uint32_t lastClockPulseCount = 0;  // NEW: last seen count
float syncFlashAlpha = 0.f;        // NEW: flash brightness (decays toward 0)

void step() override {
    // ... existing fade logic ...

    // Clock pulse flash detection
    if (module) {
        uint32_t currentCount = module->clockPulseCount.load(std::memory_order_relaxed);
        if (currentCount != lastClockPulseCount) {
            syncFlashAlpha = 1.f;  // fire flash
            lastClockPulseCount = currentCount;
        }
        // Decay: ~100ms at 60fps
        float flashDecay = 1.f / (0.1f * 60.f);  // 100ms
        syncFlashAlpha = std::fmax(0.f, syncFlashAlpha - flashDecay);
    }
}
```

Then in `drawTextOverlays()`, the SYNC badge alpha becomes:

```cpp
float flashBoost = syncFlashAlpha * 0.5f;  // additive brightness on pulse
effectiveAlpha = std::fmin(1.f, effectiveAlpha + flashBoost);
// Optionally: draw a slightly larger glow rect behind SYNC on flash
```

**What is new vs modified:**

| Item | Status | Details |
|------|--------|---------|
| `AnalogLFO::clockPulseCount` | NEW | `std::atomic<uint32_t>`, incremented on each clock edge |
| `processClockInput()` | MODIFIED | Add `clockPulseCount.fetch_add(1)` at edge detection (2 locations: first edge and subsequent edges with shouldReset) |
| `WaveformDisplay::lastClockPulseCount` | NEW | Tracks last-seen count for edge detection on GUI side |
| `WaveformDisplay::syncFlashAlpha` | NEW | Decaying brightness for flash effect |
| `WaveformDisplay::step()` | MODIFIED | Add flash detection + decay logic |
| `drawTextOverlays()` SYNC section | MODIFIED | Add flash boost to alpha; optionally add glow pass |
| Existing SYNC fade/blink | UNCHANGED | Flash is additive on top of existing behavior |

**Alternative considered: atomic bool pulse flag.** An atomic bool that the audio thread sets and the GUI thread clears is simpler but has a race condition: if two clock edges arrive between GUI frames, the second pulse is lost. The monotonic counter avoids this entirely and the GUI can even detect multiple pulses per frame if needed for more aggressive flash.

**Risk:** LOW. One new atomic (negligible performance cost), one new float in display widget, a few lines of step() logic, and a minor alpha boost in draw. Pattern matches existing `breathePhase` animation approach.

---

## Component Boundaries

| Component | Responsibility | v1.3 Changes |
|-----------|---------------|--------------|
| `AnalogLFO` (Module) | DSP, state, audio-to-GUI bridges | NEW: `computePulse()`, `pulseWidthSpread`, `clockPulseCount`. MODIFIED: `computeMorphedWave()` (5 shapes), `initComponentSpread()` |
| `WaveformDisplay` (TransparentWidget) | NanoVG rendering, animation state | NEW: `syncFlashAlpha`, `lastClockPulseCount`, corner brackets, three-column layout. MODIFIED: coordinate mapping, pill positions, colors |
| `AnalogLFOWidget` (ModuleWidget) | Panel + component placement | MODIFIED: all positions for 14HP, new SVG panel path |
| `res/AnalogLFO.svg` (Panel) | Static visual panel | REPLACED: 12HP -> 14HP, entire redesign in Forge Noir language |

## Internal Boundaries

| Boundary | Communication | v1.3 Impact |
|----------|---------------|-------------|
| Module <-> Display | Relaxed atomics (lock-free) | Add `clockPulseCount` atomic |
| Module <-> Panel SVG | None (panel is static) | Panel replaced, positions updated |
| Display <-> Panel SVG | Layered (display draws on layer 1 above panel) | Display repositioned, colors updated |
| Waveform engine <-> Display buffer | Double buffer + `computeMorphedWave()` | Automatically picks up 5th shape |

---

## Recommended Build Order

Build order considers dependencies between features and testability at each step.

### Phase A: PWM Extension (1 day)

**Why first:** Smallest, most self-contained change. Modifies only DSP code. Immediately testable by listening and observing the display (existing display automatically shows the new waveform range). No panel or layout dependencies.

1. Add `computePulse()` function (follow `computeSquare()` pattern)
2. Add `pulseWidthSpread` member + `initComponentSpread()` update
3. Modify `computeMorphedWave()`: shapes[5], morph * 4.f, segment clamp 3, bleed ring % 5
4. Test: sweep morph fully, verify sine->tri->saw->sqr->pulse progression
5. Test: character deforms pulse (edge softening, duty narrowing)
6. Test: bleed wraps correctly (pulse bleeds into sine at morph=1.0)

### Phase B: SVG Panel (2-3 days)

**Why second:** The panel is a standalone SVG file with no code dependencies beyond the widget constructor positions. It can be designed and iterated independently. Creating it before the display layout change avoids doing both coordinate systems simultaneously.

1. Create new `res/AnalogLFO.svg` at 14HP (71.12mm x 128.5mm)
2. Translate mockup geometry into nanosvg-compatible SVG:
   - Panel background, accent bars, hex bolts
   - Brand text as path outlines (FORGE / AUDIO in FoundationLogo)
   - Forge rune glyph as pure geometry (no filters)
   - Module name as path outlines (ANALOG LFO in Bebas Neue)
   - All labels as path outlines
   - Decorative elements (lines, slashes, forge emblem simplified)
   - Display placeholder rect
3. Update `AnalogLFOWidget` constructor: all `mm2px(Vec(...))` positions for 14HP layout
4. Test: module loads, all knobs/jacks in correct positions, panel renders without artifacts
5. Verify gradient compatibility -- if any gradient fails, replace with solid color

### Phase C: Display Layout (1 day)

**Why third:** Depends on the panel being correct (display position/size from Phase B). The three-column layout is a coordinate reorganization of existing NanoVG code.

1. Define column boundary constants (proportional to display width)
2. Modify `phaseToX()` to map waveform to center column only
3. Reposition all pills in `drawTextOverlays()` to left/right columns
4. Reposition `drawBpmStack()` to right column
5. Update display background/frame colors to Forge Noir palette
6. Add corner bracket accents (four L-shaped ember lines)
7. Update pill styling: current feathered `nvgBoxGradient` pills -> Forge Noir ember-outlined pills (if changing style)
8. Test: verify pills never overlap waveform, all text readable, fade animations preserved

### Phase D: Animated SYNC Badge (0.5 day)

**Why last:** Depends on Phase C (SYNC badge must be in its final position). Smallest GUI change.

1. Add `clockPulseCount` atomic to `AnalogLFO`
2. Add `fetch_add(1)` at clock edge detection points in `processClockInput()`
3. Add `lastClockPulseCount` and `syncFlashAlpha` to `WaveformDisplay`
4. Add flash detection + decay in `step()`
5. Add alpha boost in SYNC badge drawing
6. Test: connect clock source, verify flash on each pulse, verify flash decays smoothly, verify no flash in FREE mode

---

## Anti-Patterns to Avoid

### Anti-Pattern 1: Separate PWM Control

**What people do:** Add a dedicated PW knob or parameter for pulse width, separate from morph.
**Why it is wrong for this project:** The design philosophy is "PWM integrated into morph" (PROJECT.md). A separate knob adds panel complexity, breaks the three-knob analog engine concept, and fragments the morph sweep.
**Do this instead:** The 5th morph segment (sqr->pulse) IS the pulse width control. Character adjusts the pulse shape (edge softening, duty narrowing).

### Anti-Pattern 2: Rendering Forge Noir Effects in SVG

**What people do:** Try to replicate CSS box-shadows, conic gradients, and blur filters in the SVG panel.
**Why it is wrong:** nanosvg ignores filters, complex gradients, and clip-paths. The panel renders as a flat mess.
**Do this instead:** Approximate effects with layered flat geometry. A hex bolt is two hexagonal polygons, not a gradient-filled clip-path. A metallic ring is two concentric circles with different fills, not a radial gradient. Accept that the SVG panel provides the static backdrop while NanoVG provides the glow/animation.

### Anti-Pattern 3: FramebufferWidget for Animated Display

**What people do:** Wrap the WaveformDisplay in a FramebufferWidget for performance, then mark it dirty every frame for animation.
**Why it is wrong:** If dirty every frame, you pay the framebuffer overhead without caching benefit. The current TransparentWidget approach is correct -- it redraws every frame in `drawLayer()` layer 1, which is expected for animated content. FramebufferWidget is for **static** or **rarely-changing** content.
**Do this instead:** Keep WaveformDisplay as TransparentWidget. The SVG panel is already cached by VCV's panel rendering. Only the display area (small fraction of module) redraws per frame.

### Anti-Pattern 4: Blocking Audio Thread for GUI State

**What people do:** Use mutexes or condition variables to synchronize audio and GUI threads.
**Why it is wrong:** Any blocking in `process()` causes audio glitches. The existing architecture correctly uses relaxed atomics for all display bridges.
**Do this instead:** For the clock pulse flash, use `std::atomic<uint32_t>` with `fetch_add` (audio) and `load` (GUI). No locks, no blocking, no allocation.

---

## Architectural Patterns Applied

### Pattern 1: Monotonic Counter for Edge Events

**What:** Audio thread increments an atomic counter on each event (clock edge). GUI thread detects the change by comparing with its last-seen value.
**Why better than bool flag:** No lost events between GUI frames. GUI can detect multiple events per frame if needed.
**Where used:** `clockPulseCount` for sync flash animation.

### Pattern 2: Proportional Coordinate System

**What:** Display layout boundaries defined as proportions of widget dimensions, not absolute pixel values.
**Why:** Survives display resize (if the display box changes size in the panel). Proportional constants are easy to tune.
**Where used:** Three-column layout boundaries (`leftColRight()`, `centerLeft()`, etc.).

### Pattern 3: Shape Array Extension

**What:** Adding a new waveform shape by extending the shapes[] array and adjusting the morph scaling factor, without changing the crossfade or bleed algorithms.
**Why:** The existing architecture was designed for extensibility. The morph engine is generic: N shapes with (N-1) crossfade segments.
**Where used:** `computeMorphedWave()` going from 4 to 5 shapes.

---

## Sources

- [VCV Rack Module Panel Guide](https://vcvrack.com/manual/Panel) -- SVG requirements, dimensions, nanosvg limitations
- [VCV Rack FramebufferWidget API](https://vcvrack.com/docs-v2/structrack_1_1widget_1_1FramebufferWidget) -- dirty flag, step/draw relationship
- [VCV Rack Widget API](https://vcvrack.com/docs-v2/structrack_1_1widget_1_1Widget) -- drawLayer(), step() lifecycle
- [VCV Community: SVG Transparencies and Gradients](https://community.vcvrack.com/t/svg-transparencies-and-gradients/16833) -- gradient support details
- [VCV Community: Notes for Themeable SVGs with nanoSvg](https://community.vcvrack.com/t/notes-for-theme-able-svgs-with-nanosvg/20060) -- nanosvg compatibility
- [VCV Community: Graphic Filters and Custom Components](https://community.vcvrack.com/t/graphic-filters-and-custom-components/16365) -- filter limitations
- [nanosvg Issue #87: Text Support](https://github.com/memononen/nanosvg/issues/87) -- text must be paths
- [nanosvg Issue #34: Gradient Support](https://github.com/memononen/nanosvg/issues/34) -- gradient limitations
- Existing codebase: `src/AnalogLFO.cpp` (1,359 lines, fully analyzed)
- Design reference: `forge-noir.html` (mockup at 5x scale), `DESIGN-LANGUAGE.md` (complete spec)

---
*Architecture research for: v1.3 Forge Noir integration into Analog Series LFO*
*Researched: 2026-03-27*
