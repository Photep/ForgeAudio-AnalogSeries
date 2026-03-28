# Feature Research

**Domain:** VCV Rack 2 Eurorack LFO module -- v1.3 Forge Noir milestone features
**Researched:** 2026-03-27
**Confidence:** HIGH (DSP patterns well-established, VCV Rack API documented, design language already prototyped through v19)

## Existing Foundation (v1.0 through v1.2 -- must not be disrupted)

- Three-knob analog engine (Morph, Character, Drift) with CV inputs and attenuverters
- Four-shape parametric morph (Sine->Tri->Saw->Square) with per-shape analog character modeling
- Four-layer Ornstein-Uhlenbeck drift engine with per-module RNG
- Real-time waveform display with lock-free double buffer, four-pass glow trace, phase-tracking dot
- Three-state clock tracker (FREE/ACQUIRING/LOCKED) with EMA smoothing, outlier rejection
- 15 discrete musical ratios via dual-mode Rate knob
- Division-aware phase reset with 3ms cosine crossfade
- NanoVG text overlays (SYNC badge, ratio, Hz, BPM stack, swing) with 200ms fade animations
- FM input with exponential modulation, RESET trigger, Phase Offset with CV
- Swing/shuffle via right-click menu (6 presets), phase jitter, DC wander, pitch slew, component spread, waveform bleed
- 12HP panel, bipolar +/-5V single output, ~1,374 lines of C++

---

## Feature Landscape

### Table Stakes (Users Expect These)

Features that must work correctly for the v1.3 release to feel complete. These are the "contract" of the milestone -- if any are missing, the release feels unfinished relative to the promises implied by the existing morph architecture and the Forge Noir mockup at v19.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **Morph-integrated PWM** (Sine->Tri->Saw->Square->Pulse) | The morph knob currently stops at square. Users exploring the knob expect it to go further -- a hard stop at 1.0 with square feels like the end when it should be a waypoint. Every notable morphing oscillator (Plaits, Vult Vessek, RS-MET) extends past square into pulse territory. The morph range extension was explicitly scoped for v1.3 in PROJECT.md. | MEDIUM | Requires extending morph from 4 segments (0-3) to 5 segments (0-4), adding computePulse() function, updating display buffer generation. Duty cycle sweep: 50% (square) down to 5% (narrow pulse). Standard 5%-95% limits per hardware synth convention. |
| **Forge Noir panel SVG** | The mockup is at v19 and the design language doc is comprehensive. The current panel is the old 12HP blue placeholder from v1.0. The 14HP expansion is already decided. This is the visual identity of the module. | HIGH | Full SVG redesign at 14HP. nanosvg has severe gradient limitations -- complex gradients from the HTML mockup (multi-stop radials, conic gradients, Gaussian blur) cannot render via SVG. Must use hybrid SVG+NanoVG strategy. See Technical Notes. |
| **Display three-column layout** | The Forge Noir mockup (v19) shows pills in left/right margins with waveform centered. Current code uses ad-hoc positioning that can overlap at some ratios. The separated layout is necessary for readability at the new 14HP width. | LOW | Mostly repositioning existing drawPillText/drawBpmStack calls with updated coordinates matching the three-column spec in DESIGN-LANGUAGE.md (left: x 16-55, center: x 64-256, right: x 262-302). Corner bracket accents and CRT scanline overlay are new NanoVG additions. |
| **Animated SYNC badge** (clock-pulse flash) | Current SYNC badge blinks during ACQUIRING state (2Hz sinusoidal via breathePhase). The v1.3 expectation is a brief ember flash on each clock pulse -- a "heartbeat" that shows the module is alive and locked. This is standard in hardware Eurorack modules with clock inputs (LED flash on trigger). Already scoped in PROJECT.md active items. | LOW | Already have animation infrastructure in WaveformDisplay::step(). Need a clock-edge-triggered flash envelope (fast attack ~16ms, slow decay ~200ms) modulating SYNC badge alpha. Requires one new atomic float bridge from process() to display. |

**Confidence:** HIGH -- All four features are already decided in PROJECT.md. PWM patterns are well-documented across synth literature. Panel design language is finalized at v19. Display layout is specified in DESIGN-LANGUAGE.md. SYNC animation builds on existing infrastructure.

---

### Differentiators (Competitive Advantage)

Features that go beyond what other VCV Rack LFOs offer and reinforce the Forge Audio brand identity. These elevate v1.3 from "functional" to "premium."

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **NanoVG-rendered premium knobs** | No other VCV Rack module renders knobs with the level of detail in the Forge Noir mockup -- multi-layer radial gradients, machined grooves, specular highlights, knurled outer rings. SVG knobs are flat by comparison. This makes the module look like a product photo rather than a circuit diagram. | HIGH | Custom Knob widget subclass overriding drawLayer() with NanoVG: nvgRadialGradient for body curvature, concentric stroke passes for metallic ring, repeating-conic-gradient equivalent for knurl texture (MORPH xl only). Must cache in FramebufferWidget and mark dirty only on rotation. Three size variants (xl 82px / lg 60px / md 46px). |
| **Forge emblem atmospheric background** | The subtle forge-themed pattern behind the knobs (molten streams, ember particles, chevron marks) is unique to the Forge Audio brand. No other VCV module has this level of atmospheric panel design. The define-once-mirror-via-`<use>` technique guarantees perfect symmetry, which is a hard rule. | MEDIUM | Mostly SVG with opacity-limited paths (0.03-0.18 alpha). Uses `<use href="#forge-half" transform="...scale(-1,1)">` for mirrored symmetry. Some elements (radial core glow, heat shimmer) may need NanoVG overlay if SVG gradients fail in nanosvg. |
| **Character-aware PWM** | When morph extends into pulse territory AND character is turned up, the pulse waveform exhibits analog imperfections -- duty cycle asymmetry, edge softening via tanh sigmoid, DC offset/bleed from v1.2. No other morphing LFO applies analog character modeling to its pulse waveform. | LOW | Builds directly on existing computeSquare() patterns. squareDutySpread already offsets duty cycle -- extend range for pulse. Character's tanh edge softening scales with `c` identically for pulse edges. Waveform bleed ring topology extends to include pulse as a 5th element. |
| **CRT display aesthetic** | Scanline overlay, corner bracket accents, animated border glow, dark blue-black background (#030303) -- these details make the display feel like a real oscilloscope. Most VCV Rack displays are plain rectangles with a waveform line. | LOW | Pure NanoVG additions: repeating-linear-gradient for scanlines (2px transparent / 2px dark bands), four L-shaped corner paths (8x8px, ember at opacity 0.4), nvgBoxGradient for animated glow pulsing between 0.08 and 0.18 opacity over 5 seconds using existing breathePhase. |

**Confidence:** HIGH for character-aware PWM and CRT aesthetic (straightforward NanoVG). MEDIUM for NanoVG knobs (complexity, performance risk with multiple gradient passes). HIGH for forge emblem concept (design is finalized, SVG implementation is standard).

---

### Anti-Features (Commonly Requested, Often Problematic)

Features that seem appealing but would damage the v1.3 milestone scope, performance, or design coherence.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| **Separate PWM knob** | Traditional synth design gives PWM its own control. Users familiar with hardware expect a dedicated PW knob. | Breaks the three-knob philosophy (morph, character, drift). Morph already traverses waveform space -- extending it is more coherent. 14HP panel cannot accommodate another primary knob without cramping the 1-2-2 diamond layout. | PWM integrated into morph range. Single sweep covers Sine through narrow Pulse. CV on morph modulates the full range. |
| **Full NanoVG panel** (no SVG at all) | Maximum visual quality, no gradient limitations. | Massive complexity: every text label, line, shape, jack must be positioned and drawn in code. Loses compatibility with VCV Rack's standard panel loading pipeline. SVG provides structural backbone that NanoVG enhances selectively. | Hybrid: SVG for flat elements (background, text paths, dividers, bolts) + NanoVG overlays for gradient-dependent elements (knobs, glow effects, display). |
| **Per-pulse SYNC badge color cycling** | Alternating ember/gold/white-hot colors on each clock pulse. | Distracting. SYNC flash is status feedback, not decoration. Color cycling draws attention away from the waveform display and makes the badge harder to read at a glance. | Single ember-colored flash with alpha/intensity modulation only. Consistent color maintains readability. |
| **Animated forge emblem** (drifting particles, flowing streams) | Brings the panel "alive" with motion. | GPU cost is non-trivial for continuous per-frame animation of ~50 path elements across the entire panel background. Violates "atmosphere, not foreground" -- if the emblem moves, it competes with the waveform display for visual attention. | Static emblem. The breathing glow on the display border provides the "alive" feeling. The emblem anchors the brand identity without competing. |
| **Through-zero PWM** (duty cycle crosses 0%) | Some advanced oscillators allow duty cycle to cross zero, producing phase-inverted pulse bursts. | At LFO rates, through-zero PWM is imperceptible and creates confusing discontinuities. The waveform silently disappears and reappears inverted, which reads as a bug. Adds morph mapping complexity. | Hard-limit duty cycle at 5% minimum. Standard practice across Moog, Roland, Sequential. |
| **Display showing PWM CV modulation in real-time** | Users want to see the duty cycle changing as CV modulates the morph into pulse territory. | Display buffer updates are rate-limited to ~30fps to avoid visual artifacts from fast CV. Adding CV-rate PWM animation requires audio-rate buffer updates -- the exact pattern the architecture avoids. | Display shows static shape from morph knob position. Phase dot still animates in real-time showing timing. Shape reflects "what," dot shows "motion." |
| **NanoVG trimpots and jacks** | Matching the premium knob rendering quality across all components. | Lower visual impact than knobs -- trimpots are small and jacks are standard. The effort-to-visual-reward ratio is poor. SVG versions are acceptable. | SVG trimpots and jacks for v1.3. Defer NanoVG component rendering to a future polish pass if warranted. |

**Confidence:** HIGH -- all decisions align with established project philosophy (three-knob identity, hybrid rendering, performance-first display architecture).

---

## Feature Dependencies

```
Forge Noir Panel SVG (14HP)
    |
    +-- requires --> 14HP dimensions (71.12mm x 128.5mm, already decided)
    |
    +-- enhances --> NanoVG Premium Knobs (knobs render on top of SVG panel)
    |                    |
    |                    +-- requires --> FramebufferWidget caching (perf)
    |                    +-- requires --> Custom Knob widget subclass
    |
    +-- enhances --> Forge Emblem Background (between SVG base and knobs)
    |
    +-- contains --> Display Three-Column Layout (display is panel element)
                         |
                         +-- contains --> Animated SYNC Badge (display element)
                         +-- contains --> CRT Aesthetic (display styling)

Morph-Integrated PWM
    |
    +-- extends --> Existing morph engine (computeMorphedWave, shapes[], segment count)
    |
    +-- enhances --> Character-Aware PWM (character applies to pulse shapes)
    |
    +-- independent of --> Panel/display work (DSP is separate from rendering)

Character-Aware PWM
    |
    +-- requires --> Morph-Integrated PWM (pulse waveform must exist first)
    |
    +-- reuses --> Existing squareDutySpread, tanh softening, bleed ring
```

### Dependency Notes

- **Panel SVG requires 14HP dimensions:** Already decided. One-way gate for all panel work.
- **NanoVG Knobs require FramebufferWidget:** Multi-pass gradient rendering is expensive without caching. Each knob wraps in FramebufferWidget, dirty only on rotation (parameter change). VCV API: `FramebufferWidget::setDirty()` triggers re-render on next step().
- **Display layout requires panel SVG:** Display widget position and size derive from the 14HP SVG panel. Three-column dimensions come from panel width minus margins.
- **SYNC badge is inside display:** Not a separate widget -- drawn by WaveformDisplay::drawTextOverlays(). The clock flash extends the existing syncFadeAlpha system, adding a new clockFlashEnvelope atomic.
- **PWM is independent of panel:** Morph engine changes (computePulse, segment count) can be developed and tested without the panel redesign. Zero shared code paths.
- **Character-aware PWM requires base PWM:** Cannot add analog imperfections to a pulse waveform that does not yet exist. Build computePulse first, then layer character effects.

---

## MVP Definition

### v1.3 Core (Must Ship)

These define the milestone. Without all four, v1.3 is not complete.

- [ ] **Morph-integrated PWM** -- Extends morph from 4 segments to 5 segments. Pulse region (~morph 0.75-1.0) sweeps duty cycle from 50% (square) down to 5% (narrow pulse). Character modeling applies to pulse. Display buffer renders the full extended range.
- [ ] **Forge Noir panel SVG** -- New 14HP SVG implementing the design language. Flat elements in SVG (panel background #0c0c0c, text labels as paths, decorative lines, section dividers, hex bolts, accent bars). Placeholder circles for component positions per the 1-2-2 diamond layout.
- [ ] **Display three-column layout** -- Left: ratio pill, Hz readout, swing label. Center: waveform + phase dot. Right: SYNC badge, BPM stack. Corner bracket accents. Pill positioning prevents overlap at all ratios and zoom levels.
- [ ] **Animated SYNC badge** -- Brief ember flash on clock edge, decaying over ~200ms. LOCKED state: clock-pulse flash with steady ~60% base visibility. ACQUIRING state: existing 2Hz sinusoidal blink (unchanged).

### v1.3 Polish (Should Ship, Defer Only if Time-Constrained)

- [ ] **NanoVG premium knobs** -- Three sizes (xl/lg/md). Multi-layer radial gradients, metallic ring, center cap indent, ember indicator line with glow. Knurled outer ring for MORPH xl. Cached in FramebufferWidget.
- [ ] **Forge emblem background** -- Atmospheric SVG pattern: molten streams, forge marks, ember particles. `<use>` mirroring for perfect symmetry. Low-opacity (0.03-0.18).
- [ ] **CRT display aesthetic** -- Scanline overlay, corner bracket accents (8x8px L-shapes), animated border glow using breathePhase. Display background #030303.
- [ ] **Character-aware PWM** -- Analog imperfections applied to pulse: squareDutySpread extended, tanh edge softening, bleed ring expanded to 5 shapes.

### Deferred (v2.0 VCO or Later)

- [ ] **Through-zero PWM** -- Meaningful at audio rates, not LFO rates. Defer to VCO module.
- [ ] **NanoVG trimpots and jacks** -- Lower visual impact than knobs; SVG versions acceptable for v1.3.
- [ ] **Animated forge emblem** -- Moving particles, flowing streams. GPU cost vs. design philosophy.

---

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Morph-integrated PWM | HIGH | MEDIUM | P1 |
| Forge Noir panel SVG | HIGH | HIGH | P1 |
| Display three-column layout | HIGH | LOW | P1 |
| Animated SYNC badge | MEDIUM | LOW | P1 |
| Character-aware PWM | MEDIUM | LOW | P1 (bundled with PWM) |
| NanoVG premium knobs | HIGH | HIGH | P2 |
| Forge emblem background | MEDIUM | MEDIUM | P2 |
| CRT display aesthetic | MEDIUM | LOW | P2 |

**Priority key:**
- P1: Must have for v1.3 launch. These define the milestone.
- P2: Should have. Elevates from "functional" to "premium." Defer only under time pressure.

---

## Competitor Feature Analysis

| Feature | Befaco EvenVCO | Bogaudio VCO | Vult Vessek | Surge XT VCO | Forge Analog LFO (v1.3) |
|---------|----------------|--------------|-------------|--------------|--------------------------|
| Waveform morph | None (separate outs) | None (separate outs) | Shape knob per osc | Mode selector | Continuous Sine->Tri->Saw->Square->Pulse on single knob |
| PWM | Separate PW knob, square only | Separate PW knob, square only | PW knob affects all waveforms | Per-mode controls | Integrated into morph sweep (no extra knob needed) |
| Analog character | None | None | Inherent in model | Digital (clean) | Dedicated Character knob with classic synth references |
| Drift/imperfections | None | None | Minimal | None | Drift knob: 4-layer OU, jitter, DC wander, pitch slew, component spread, bleed |
| Panel design | SVG (flat Befaco) | SVG (flat, functional) | SVG (Vult brand) | Custom NanoVG (skinnable) | Hybrid SVG+NanoVG (Forge Noir premium) |
| Display | None | None | None | Waveform in LFO | Real-time single-cycle waveform, HUD pills, phase dot, three-column layout |
| Clock sync | None (VCO) | None (VCO) | None (VCO) | Clock divider | 15 musical ratios, EMA smoothing, division-aware reset, swing |
| SYNC indicator | N/A | N/A | N/A | N/A | Animated badge with clock-pulse flash |

**Competitive position:** The Forge Analog LFO occupies a unique niche. No other VCV Rack LFO combines continuous waveform morphing (now through pulse), analog character modeling, clock sync with swing, and a premium display. The v1.3 PWM extension and Forge Noir panel further differentiate. The closest visual competitor (Surge XT) has NanoVG rendering but no analog character or morph engine. The closest functional competitor (Vult Vessek) has PW-all-waveforms but no clock sync, display, or drift.

---

## Technical Implementation Notes

### PWM as Morph Extension

**Mathematical approach:** The morph parameter (0.0-1.0) currently maps to 4 segments via `scaled = morph * 3.0`, producing segments [0,1,2] with fractional crossfade. Extending to 5 segments: `scaled = morph * 4.0`, adding segment 3 (square-to-pulse crossfade) with the pulse shape at index 4 in the shapes array.

**computePulse() function:** Parameterized by duty cycle derived from the morph fraction within the square-to-pulse segment. When the morph fraction `frac` is 0.0, duty cycle = 0.50 (identical to square). When frac = 1.0, duty cycle = 0.05 (narrow pulse). Mapping: `duty = 0.50 - frac * 0.45`.

The implementation reuses the same tanh sigmoid approach as computeSquare(), with the variable duty cycle replacing the fixed ~0.5:
```
center = duty * 0.5
halfWidth = duty * 0.5
dist = halfWidth - |phase - center| (with wrap)
pulse = tanh(sharpness * dist)
```

**Duty cycle limits:** Hard floor at 5% (0.05 duty). Below this, the pulse becomes inaudible and visually disappears -- a well-documented issue in synth design. Standard practice across Moog, Roland, Sequential is to restrict range to 5%-95%. Since a 5% duty cycle and a 95% duty cycle sound identical (acoustic symmetry), there is no reason to extend past 50% on the wide side.

**Character interaction:** At character > 0:
- squareDutySpread offsets base duty (+/-1%, existing)
- Edge softening: `edgeWidth = c * 0.08` produces tanh-smoothed transitions (existing pattern)
- Waveform bleed: wrapping ring topology extends from 4 shapes to 5 (sine-tri-saw-sqr-pulse-sine)
- Pulse neighbors: left=square, right=sine (ring wraps)

**Display buffer update:** `updateDisplayBuffer()` calls `computeMorphedWave()` which already handles crossfade. The extended range propagates automatically once computePulse is wired into the shapes array (`float shapes[5] = { sine, tri, saw, sqr, pulse }`) and segment count becomes 4 (`int segment = std::min((int)scaled, 3)`).

**CV behavior:** Morph CV already modulates `morph` in [0, 1] and clamps. Full CV sweep now covers Sine through narrow Pulse transparently. No CV path changes needed.

**Backward compatibility:** At morph=0.75 (previously: full square), the new 5-segment mapping places this at segment 3 frac 0.0, which is square-to-pulse crossfade at 0% = pure square. Users with patches at morph=1.0 will hear square (since square is now at ~0.75 in the sweep). IMPORTANT: This is a breaking change for existing patches that use morph at specific values. Morph knob position meanings shift. Mitigate by documenting in changelog and ensuring morph CV still sweeps the full useful range.

### Panel SVG + NanoVG Hybrid Strategy

**SVG handles:** Panel background (#0c0c0c flat fill), accent bars (flat color -- gradients are unreliable in nanosvg), hex bolt outlines (polygon shapes), text labels (as `<path>` elements exported from font outlines -- NOT `<text>` tags which nanosvg does not render), section divider lines, decorative slashes, module name, brand text, forge rune (simple paths only, no blur filter). Component positions marked as invisible reference circles.

**NanoVG handles:** Knob rendering (multi-layer radial gradients), forge emblem glow elements (radial gradients that nanosvg may fail on), display border glow (animated, needs per-frame update), CRT scanlines (repeating pattern), corner bracket accents. Rendered in drawLayer() on layer 1 for light/bloom effects per VCV community documentation.

**FramebufferWidget strategy:** Each NanoVG knob wraps in its own FramebufferWidget. Dirty flag set on parameter change only (rotation). The WaveformDisplay widget already redraws every frame for phase dot animation -- no FramebufferWidget needed there (and wrapping it would prevent animation).

**nanosvg limitations confirmed via VCV Community:**
- Complex multi-stop gradients: unreliable rendering
- Gaussian blur filters: not supported
- CSS styling: not supported
- `<text>` elements: not rendered (must use `<path>` outlines)
- Simple two-color linear gradients: generally work
- Opacity: works reliably
- `<use>` with transforms: works (confirmed for mirror technique)

### Animated SYNC Badge

**Current state:** `syncFadeAlpha` fades in (200ms) when clock detected, fades out when clock lost. During ACQUIRING, alpha is modulated by a 2Hz sinusoidal blink (`breathePhase * 2.5`).

**New behavior -- two-state approach:**
- **ACQUIRING:** Keep existing 2Hz blink (unchanged). Communicates "searching for clock."
- **LOCKED:** Add `clockFlashEnvelope` (new `std::atomic<float>`). Set to 1.0 on each clock edge in processClockInput(). In WaveformDisplay::step(), decay: `clockFlashEnvelope *= 0.92f` (~200ms decay at 60fps). Multiply SYNC badge alpha by `max(0.6f, clockFlashEnvelope)` to produce brief brightening on each pulse while maintaining base visibility.

**Visual effect:** SYNC badge at steady ~60% alpha when locked, with brief 100% flashes on each clock edge. Communicates "I am receiving and tracking pulses" without the instability of the ACQUIRING blink. The ember color (#e85d26) intensifies briefly on each pulse.

**Implementation cost:** One new atomic float, one line in processClockInput(), three lines in WaveformDisplay::step(). Trivial.

### Display Three-Column Layout

**Current layout:** Ad-hoc pill positions with 4px margins. Hz top-left, ratio top-left (replacing Hz), SYNC top-right, BPM stack bottom-right, swing bottom-left.

**New layout per Forge Noir mockup (v19) and DESIGN-LANGUAGE.md:**
- Left column (x: 16-55 at display scale): Ratio pill (top), Hz readout (top, exclusive with ratio), Swing pill + label (bottom)
- Center column (x: 64-256): Single-cycle waveform trace with phase dot, dashed zero-crossing reference line
- Right column (x: 262-302): SYNC badge (top), CLK BPM (bottom), LFO BPM (bottom, below CLK)

The three-column approach eliminates overlap risk. Each column has fixed x-bounds regardless of text length. The center waveform column is the widest (~60% of display width).

**New display elements:**
- Corner bracket accents: 8x8px L-shaped ember paths at each corner (1.5px stroke, opacity 0.4)
- CRT scanlines: repeating horizontal bands (2px transparent, 2px at opacity ~0.03)
- Animated border glow: nvgBoxGradient around display rect, opacity pulsing via breathePhase (0.08 to 0.18 over 5-second cycle)
- Background color change: from current blue-black (#0D0D1A) to near-black (#030303) per Forge Noir spec

---

## Backward Compatibility Considerations

| Concern | Impact | Mitigation |
|---------|--------|------------|
| Morph range remapping (4->5 segments) | Existing patches with specific morph values will produce different waveforms. Morph=1.0 was square, now maps to narrow pulse. | Document in changelog. Morph=0.75 is now approximately where square lives. This is an intentional design change, not a bug. |
| Panel size change (12HP->14HP) | Existing patches with tight module placement may need adjustment. | Standard VCV Rack behavior -- users expect panel size may change between versions. |
| Display layout change | Pill positions move. No functional impact. | Purely visual -- no user-facing behavior change. |
| New atomic (clockFlashEnvelope) | Trivial memory addition. No existing state affected. | Default to 0.0 -- no flash until first clock edge. |

---

## Sources

### HIGH Confidence (Official docs, verified APIs, direct code inspection)
- Current `src/AnalogLFO.cpp` (1,374 lines) -- all existing patterns, integration points, morph engine structure
- [VCV Rack FramebufferWidget API](https://vcvrack.com/docs-v2/structrack_1_1widget_1_1FramebufferWidget) -- dirty flag, caching, setDirty(), performance characteristics
- [VCV Rack Module Panel Guide](https://vcvrack.com/manual/Panel) -- SVG requirements, nanosvg constraints
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) -- drawLayer, TransparentWidget, widget lifecycle
- Forge Noir DESIGN-LANGUAGE.md -- complete panel specification, typography, color palette, component sizing
- Forge Noir mockup v19 (forge-noir-v19.png) -- visual reference for all panel and display layout

### MEDIUM Confidence (Multiple sources agree, community-verified)
- [Perfect Circuit: What is PWM?](https://www.perfectcircuit.com/signal/what-is-pwm) -- 5%-95% duty cycle limits, harmonic behavior, synth design conventions
- [Sound on Sound: Synthesizing Strings with PWM](https://www.soundonsound.com/techniques/synthesizing-strings-pwm-string-sounds) -- duty cycle acoustic symmetry, standard limits
- [RS-MET waveform morph discussion](https://github.com/RobinSchmidt/RS-MET/discussions/323) -- linear segment composition, morph parameter math, DC-free normalization
- [Vult Vessek documentation](https://modlfo.github.io/VultModules/vessek/) -- PW affecting all waveforms, integrated morph+PW pattern
- [VCV Community: Custom Widget Light Bloom](https://community.vcvrack.com/t/custom-widget-light-bloom/14647/3) -- ring lights via dual radial gradients, box gradients for bloom, NanoVG paint boundary semantics
- [VCV Community: SVG Gradients](https://community.vcvrack.com/t/svg-transparencies-and-gradients/16833) -- nanosvg gradient rendering limitations
- [VCV Community: drawLayer layers](https://community.vcvrack.com/t/drawlayer-has-anyone-figured-out-all-the-possible-layers/15542) -- layer 1 for lights/glow rendering
- [Mutable Instruments Plaits manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/plaits/manual/) -- waveform morph engine, narrow pulse behavior

### LOW Confidence (Single source or general knowledge)
- Exact GPU cost of multi-pass NanoVG knob rendering -- needs profiling on target hardware
- FramebufferWidget behavior with multiple gradient passes -- needs empirical testing
- nanosvg `<use>` with scale(-1,1) transform for mirror -- documented in design language but needs SVG rendering verification

---
*Feature research for: Forge Audio Analog LFO v1.3 Forge Noir*
*Researched: 2026-03-27*
