# Project Research Summary

**Project:** Forge Audio Analog Series -- v1.3 Forge Noir
**Domain:** VCV Rack 2 module development (LFO: PWM extension, panel redesign, display layout, animation)
**Researched:** 2026-03-27
**Confidence:** HIGH

## Executive Summary

The v1.3 Forge Noir milestone extends the existing ~1,374-line single-file VCV Rack 2 LFO module in four well-bounded directions: adding a fifth morph shape (pulse/PWM), replacing the 12HP panel with a 14HP Forge Noir design, restructuring the display into a three-column layout, and adding a clock-pulse flash animation to the SYNC badge. Critically, all four features fit entirely within the existing stack -- no new libraries, no new build system changes, no new dependencies. This is implementation work, not infrastructure work. The one deliberate addition is JetBrains Mono (a bundled TTF file for the NanoVG display), which follows the established font loading pattern and requires no code changes to the build system.

The recommended build order is A (PWM DSP) -> B (SVG panel + widget positions) -> C (display layout) -> D (sync animation), driven by hard dependencies: the panel must exist before display positions can be finalized, and the SYNC badge must be in its final position before adding flash animation. PWM is fully independent of all rendering work and should ship first to validate the DSP extension before visual complexity accumulates. The panel SVG is the most labor-intensive deliverable -- the HTML/CSS mockup (v19) uses features that nanosvg cannot render, requiring systematic element-by-element translation to nanosvg-compatible geometry, tested in VCV Rack iteratively rather than at the end.

The dominant risk class is backward compatibility: the morph range remapping (4->5 shapes) changes what existing patches sound like unless handled with unequal segment widths; the panel width change (12HP->14HP) shifts neighboring modules in saved layouts; and enum ordering mistakes would silently corrupt saved parameter values. All three risks have clear, established mitigations and are not reasons to change the design -- they are reasons to test carefully and document in the changelog.

---

## Key Findings

### Recommended Stack

No new dependencies are needed for v1.3. The entire milestone is implementation work within the existing stack: VCV Rack 2 SDK (~2.6), C++17, NanoVG (bundled), nanosvg (bundled), and the standard VCV Makefile build system.

The one deliberate file addition is JetBrains Mono Regular (`res/fonts/JetBrainsMono-Regular.ttf`) to align display typography with the Forge Noir design language. It is SIL OFL licensed (~150KB) and loaded via the established `APP->window->loadFont(asset::plugin(...))` pattern. No Makefile changes are needed -- the existing `DISTRIBUTABLES += res` covers new files automatically.

Full details: `.planning/research/STACK.md`

**Core technologies:**
- VCV Rack 2 SDK: framework, component library, panel rendering -- in place, API stable since v2.0
- C++17: all DSP and widget code -- in place, `std::atomic`, `std::array`, structured bindings all available
- NanoVG (SDK-bundled): real-time display rendering -- in place, all APIs needed for v1.3 are already in active use
- nanosvg (SDK-bundled): panel SVG parsing -- in place, constraints are well-documented and already respected in the v1.2 panel
- JetBrains Mono TTF: display font for Forge Noir -- new file addition, zero code dependency changes required

**Critical version note:** Target 2-stop gradients only for SVG compatibility across older Rack versions. Radial gradient support improved in VCV 2.6 but circular-only (`r`, not `rx/ry`) is still the safest constraint.

**File additions summary:**
```
res/
  AnalogLFO-ForgeNoir.svg      NEW  (14HP Forge Noir panel)
  ForgeHexBolt.svg             NEW  (custom hex bolt screw)
  ForgeKnobXL.svg              NEW  (hero knob body)
  ForgeKnobLG.svg              NEW  (secondary knob body)
  ForgeKnobMD.svg              NEW  (utility knob body)
  ForgeTrim.svg                NEW  (trimpot body)
  ForgeJackSmall.svg           NEW  (input jack)
  ForgeJackLarge.svg           NEW  (output jack)
  fonts/
    JetBrainsMono-Regular.ttf  NEW  (display font)

src/
  AnalogLFO.cpp                MODIFY (PWM, layout, flash, custom components)
```

### Expected Features

The v1.3 scope is locked -- all four core features are explicitly decided in PROJECT.md. Research validates feasibility and de-risks implementation rather than redefining scope.

Full details: `.planning/research/FEATURES.md`

**Must have (table stakes -- define the milestone):**
- Morph-integrated PWM -- sine -> tri -> saw -> square -> narrow pulse as a single continuous sweep; no separate knob; backward compatibility requires the unequal-segment-widths approach (see pitfalls)
- Forge Noir panel SVG (14HP) -- flat elements in SVG (background, text-as-paths, labels, decorative geometry, hex bolts); gradient and glow elements deferred to NanoVG overlays
- Display three-column layout -- left pills (ratio, Hz, swing), center waveform, right pills (SYNC, BPM); eliminates current overlap-at-some-ratios problem
- Animated SYNC badge -- clock-pulse flash on each clock edge (fast attack, ~200ms exponential decay); existing 2Hz sinusoidal blink during ACQUIRING state unchanged

**Should have (elevates functional to premium -- defer only under time pressure):**
- NanoVG premium knobs (xl/lg/md variants with multi-layer radial gradients, metallic rings, knurl texture for MORPH xl) -- highest complexity P2 item, with unvalidated GPU cost
- Forge emblem atmospheric background (molten streams, ember particles, chevron marks at 0.03-0.18 opacity) -- SVG implementation
- CRT display aesthetic (scanline overlay, corner bracket accents, animated border glow via breathePhase)
- Character-aware PWM (squareDutySpread extended to pulse region, tanh edge softening, bleed ring topology extended to 5 shapes) -- small additive work, bundles well with Phase A

**Defer (v2.0 or later):**
- Through-zero PWM -- meaningful at audio rates only; adds morph mapping complexity for no LFO benefit
- NanoVG trimpots and jacks -- poor effort-to-visual-reward ratio; SVG versions are acceptable for v1.3
- Animated forge emblem (drifting particles, flowing streams) -- GPU cost non-trivial; violates "atmosphere not foreground" design principle

**Competitive position:** No other VCV Rack LFO combines continuous waveform morphing through pulse, analog character modeling on the pulse shape, clock sync with swing, and a premium display. The v1.3 Forge Noir panel and NanoVG knobs further differentiate on presentation. The closest visual competitor (Surge XT) has NanoVG rendering but no analog character or morph engine. The closest functional competitor (Vult Vessek) has PW-all-waveforms but no clock sync, display, or drift.

### Architecture Approach

The module is a single-file, three-struct architecture: `AnalogLFO` (Module/DSP), `WaveformDisplay` (TransparentWidget/NanoVG), `AnalogLFOWidget` (ModuleWidget/panel). All audio-to-GUI communication uses relaxed atomics via a lock-free double buffer for waveform data. This architecture is not changing for v1.3 -- all four features integrate as localized extensions to existing structs via established patterns.

Full details: `.planning/research/ARCHITECTURE.md`

**Major components and v1.3 changes:**
1. `AnalogLFO` (Module) -- NEW: `computePulse()`, `pulseWidthSpread`, `clockPulseCount` atomic; MODIFIED: `computeMorphedWave()` (4->5 shapes, unequal segments), `initComponentSpread()`
2. `WaveformDisplay` (TransparentWidget) -- NEW: `syncFlashAlpha`, `lastClockPulseCount`, three-column coordinate system, corner brackets; MODIFIED: pill positions, colors, font to JetBrains Mono
3. `AnalogLFOWidget` (ModuleWidget) -- MODIFIED: all `mm2px(Vec(...))` positions for 14HP layout, new SVG panel path, new custom widget subclasses
4. `res/AnalogLFO.svg` (Panel) -- REPLACED: 12HP -> 14HP, complete Forge Noir redesign

**Key patterns applied:**
- Monotonic counter (`clockPulseCount`) for audio-to-GUI edge events -- avoids the lost-event race condition of a bool flag; GUI detects multiple edges per frame if needed
- Proportional coordinate system for display column boundaries -- `leftColRight()`, `centerLeft()`, etc. defined as fractions of `box.size.x`; survives any display box resize
- Shape array extension (4->5 elements) -- morph engine was generic: N shapes with (N-1) segments; adding a shape is additive, not a refactor
- Hybrid SVG+NanoVG rendering -- SVG for static structure, NanoVG for gradients and animation; this is the correct split for nanosvg's constraints
- Unequal morph segment widths -- morph [0, 0.75] maps to existing 3 crossfade segments; morph [0.75, 1.0] maps to the new square-to-pulse segment; preserves all existing patch sounds exactly

### Critical Pitfalls

Sixteen pitfalls were identified in research. The top five by consequence severity:

1. **Morph continuity break at the square-to-pulse boundary** -- The naive 5-equal-segment approach (`morph * 4.f`) shifts every existing patch: square moves from morph=1.0 to morph=0.75. Prevent with unequal segment widths: morph [0, 0.75] maps to the 3 existing crossfade segments; morph [0.75, 1.0] maps to square-to-pulse. Detection: A/B test at morph=0.5 (pure saw) before and after -- output must be identical.

2. **nanosvg rendering failures silently breaking the panel** -- The HTML/CSS mockup uses features nanosvg cannot render: `<text>`, `<use>`, `<defs>`, `<filter>`, CSS classes, multi-stop gradients, clip-paths. Designing the full panel in Inkscape before testing in VCV Rack produces a black rectangle in production. Prevent by testing in VCV Rack after each major SVG element, not at the end.

3. **Enum reordering corrupting saved parameter values** -- Inserting a new param anywhere except the end of the enum shifts all subsequent IDs, causing patches to load with wrong values. Prevent by always appending to enums before `PARAMS_LEN`. The morph-integrated PWM design avoids needing any new param (PWM is the morph sweep, not a separate control) -- this risk is low for v1.3 but must be verified if any new param is added.

4. **Pulse wave amplitude and DC collapse at narrow duty cycles** -- At 10% duty, perceived amplitude drops and DC bias shifts toward -0.8V normalized. Prevent with a hard minimum duty cycle floor (10-15%) and verification that drift DC offset does not compound past +/-5V. Accept the inherent DC characteristic as authentic analog behavior.

5. **Panel width change disrupting saved patch layouts** -- 12HP to 14HP forces a 2HP right-shift of all neighboring modules on patch load. Cables remain connected, parameters restore correctly. Prevent by NOT changing the module slug, documenting in the changelog, and testing load behavior with a real v1.2 patch file before release.

Additional pitfalls covered in detail in `.planning/research/PITFALLS.md`: bleed ring topology (Pitfall 7), display buffer for pulse region (Pitfall 8), component placeholder alignment (Pitfall 9), font loading per-frame overhead (Pitfall 10), crossfade artifacts from rapid morph CV (Pitfall 11), three-column layout at small zoom (Pitfall 12), animation CPU when hidden (Pitfall 13), font bundling (Pitfall 14), component spread seed ordering (Pitfall 15), SVG cache during development (Pitfall 16).

---

## Implications for Roadmap

Based on combined research, the phase structure is determined by the dependency graph. There is no ambiguity about ordering.

### Phase A: PWM DSP Extension

**Rationale:** Smallest, most self-contained change. Pure C++ math modifying only `AnalogLFO` struct. Zero panel dependencies -- the existing display automatically renders the new waveform range because `updateDisplayBuffer()` calls `computeMorphedWave()`. Validates the most important feature before visual complexity accumulates. Character-aware PWM bundles here because it modifies the same code (bleed ring, squareDutySpread, tanh softening on pulse edges).

**Delivers:** Full five-shape morph sweep (sine->tri->saw->square->narrow pulse) using unequal segment widths to preserve all existing patch sounds. computePulse() with tanh edge softening. Bleed ring extended to 5 shapes with open-ended vs. ring topology decision made. pulseWidthSpread added as last entry in initComponentSpread().

**Features addressed:** Morph-integrated PWM (P1 must-have), Character-aware PWM (P1 bundled)

**Critical pitfalls to avoid:** Pitfall 1 (morph continuity -- unequal segment widths), Pitfall 2 (amplitude/DC collapse -- hard floor at 10-15% duty), Pitfall 6 (enum ordering -- no new params for v1.3 PWM), Pitfall 7 (bleed ring topology -- update modulus atomically), Pitfall 8 (display buffer -- passes full morph range), Pitfall 15 (spread seed ordering -- append pulseWidthSpread last)

**Research flag:** No deeper research needed. All patterns are direct extensions of existing computeSquare() code. LOW complexity, HIGH confidence.

---

### Phase B: Forge Noir SVG Panel + Widget Positions

**Rationale:** The panel SVG is a standalone file with no code dependencies beyond the widget constructor positions. It is the most labor-intensive deliverable and is the structural blocker for Phase C (display box position and size depend on the 14HP layout). Creating the panel before touching display code avoids working in two coordinate systems simultaneously.

**Delivers:** Full 14HP Forge Noir panel SVG (panel background #0c0c0c, text-as-paths for all labels, decorative lines, hex bolt geometry, forge emblem as simplified low-opacity paths, display placeholder rect). Updated widget constructor with all `mm2px(Vec(...))` positions recalculated for 14HP per DESIGN-LANGUAGE.md coordinates. Custom SVG component files for knobs, jacks, trimpots, hex bolts.

**Features addressed:** Forge Noir panel SVG (P1 must-have), Forge emblem atmospheric background (P2, simplified SVG version acceptable)

**Critical pitfalls to avoid:** Pitfall 3 (2HP width change -- document, do not change slug), Pitfall 4 (nanosvg limitations -- test in VCV Rack after each major SVG element), Pitfall 9 (component placeholder alignment -- single source of truth in C++ positions), Pitfall 16 (SVG cache -- restart VCV Rack fully after SVG changes during development)

**Research flag:** No deeper research needed. nanosvg constraints are fully documented. Translation from HTML/CSS mockup to nanosvg-compatible SVG is labor-intensive but not technically uncertain. MEDIUM complexity overall; HIGH confidence on the constraints, MEDIUM confidence on exact visual fidelity of the design language translation.

---

### Phase C: Display Three-Column Layout

**Rationale:** Depends on Phase B (display box position and size are finalized). The three-column restructuring is a coordinate reorganization of existing NanoVG draw calls -- no new rendering technology, no new atomics. Column boundaries defined as proportions of `box.size.x` auto-adapt to the new 14HP display dimensions. CRT aesthetic elements (corner brackets, scanlines, border glow) bundle here because they modify the same drawLayer() function with minimal additional code.

**Delivers:** Left/center/right column layout for all pills, waveform confined to center column, Forge Noir display colors (#030303 background, ember accent border), JetBrains Mono font loaded and cached for data readouts, corner bracket accents (8x8px L-shapes), CRT scanline overlay, animated border glow via breathePhase.

**Features addressed:** Display three-column layout (P1 must-have), CRT display aesthetic (P2)

**Critical pitfalls to avoid:** Pitfall 5 (FramebufferWidget misuse -- keep WaveformDisplay as TransparentWidget, never wrap), Pitfall 10 (font loading -- cache handle in member variable on first draw, not every frame), Pitfall 12 (zoom levels -- test at 50%/75%/100%/150%/200%), Pitfall 14 (font bundling -- use `asset::plugin()` not `asset::system()` for JetBrains Mono)

**Research flag:** No deeper research needed. All NanoVG APIs are already in active use in the v1.2 display code. Coordinate math is proportional arithmetic. LOW complexity, HIGH confidence.

---

### Phase D: Animated SYNC Badge

**Rationale:** Depends on Phase C (SYNC badge must be in its final position). The smallest GUI change in the milestone: one new atomic (`clockPulseCount`), one new float (`syncFlashAlpha`), a few lines in `step()`, and a minor alpha boost in `drawTextOverlays()`. Appropriately scoped as the final phase because it builds on both the display layout work (Phase C) and the existing breathePhase animation infrastructure.

**Delivers:** Brief ember flash on each clock edge while LOCKED (fast attack, exponential decay ~200ms, base visibility ~60%). Flash uses monotonic counter bridge from audio thread to prevent lost-event races between audio and GUI frames. ACQUIRING state 2Hz sinusoidal blink is unchanged.

**Features addressed:** Animated SYNC badge (P1 must-have)

**Critical pitfalls to avoid:** Pitfall 5 (no FramebufferWidget involved here), Pitfall 13 (gate animation on `syncFadeAlpha > 0.001f`, matching existing pattern)

**Research flag:** No deeper research needed. Pattern is a direct extension of the existing breathePhase and syncFadeAlpha animation system. LOW complexity, HIGH confidence.

---

### NanoVG Premium Knobs (P2 -- Phase E or Deferred)

**Rationale:** The only P2 feature without a natural anchor in the above phases. NanoVG knobs require a custom widget subclass overriding `drawLayer()` with multi-pass radial gradients and FramebufferWidget caching (dirty only on rotation). This is architecturally separate from all other v1.3 work and carries the highest implementation risk: GPU cost of multi-pass gradient rendering is unvalidated, and FramebufferWidget interaction with multiple gradient passes has not been profiled.

**Recommendation:** Defer to a clearly-scoped Phase E after the core milestone ships. Profile GPU cost immediately before committing to the full three-size system (xl/lg/md). Do not let knob rendering block the milestone.

**Research flag:** NEEDS empirical performance validation before implementation begins. Multi-pass NanoVG knob GPU cost is explicitly listed at LOW confidence in FEATURES.md. If pursuing Phase E, profile with VCV Rack's performance meters on the first prototype knob before building all three sizes.

---

### Phase Ordering Rationale

- Phase A first: PWM DSP is zero-dependency on rendering work; validates the most important feature change with maximum isolation; any bugs are easier to isolate before panel and display complexity accumulates
- Phase B second: the panel is the structural blocker for all coordinate work; must be locked before display positions can be finalized; it is the highest-effort single deliverable and benefits from being done while the implementation is still fresh
- Phase C third: depends on Phase B display box position and size; all NanoVG APIs are already in use so iteration is fast once the panel coordinate system is established
- Phase D last: depends on Phase C SYNC badge final position; it is the smallest change and the appropriate integration capstone
- NanoVG knobs deferred or Phase E: architecturally orthogonal to all other work; carries the only unvalidated performance risk in the milestone

---

### Research Flags

Phases needing deeper research or empirical validation:

- **NanoVG Premium Knobs (Phase E):** GPU performance of multi-pass gradient rendering at three knob sizes is untested. Needs profiling on target hardware before committing to the full implementation. Listed at LOW confidence in source research.

Phases with standard, established patterns (skip additional research):

- **Phase A (PWM DSP):** Direct extension of computeSquare(). All math is proven in existing code.
- **Phase B (SVG Panel):** nanosvg constraints are fully documented; translation from mockup is labor, not research. Iterative testing in VCV Rack is the validation mechanism.
- **Phase C (Display Layout):** All NanoVG APIs already in active use. Coordinate math is proportional arithmetic.
- **Phase D (Sync Animation):** Monotonic counter and exponential decay pattern directly mirrors existing animation infrastructure.

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | No new dependencies. All APIs verified against SDK docs and existing codebase at specific line numbers. JetBrains Mono follows an established loading pattern. |
| Features | HIGH | All four core features explicitly scoped in PROJECT.md. PWM math, display layout, and animation patterns are well-documented. Design language finalized at mockup v19. NanoVG knob GPU cost is the one LOW-confidence item. |
| Architecture | HIGH | Existing codebase fully analyzed (1,359 lines). All integration points identified with line numbers. Build order validated against dependency graph. Three key architectural patterns (monotonic counter, proportional coordinates, shape array extension) are established. |
| Pitfalls | HIGH | 16 pitfalls identified, 12 at HIGH confidence from direct source analysis. Two at MEDIUM confidence (crossfade artifacts from rapid morph CV, font loading overhead) are edge cases with natural mitigations. Two at MEDIUM confidence (SVG cache behavior, visual fidelity of nanosvg gradient translation) are development process concerns, not architectural risks. |

**Overall confidence:** HIGH

### Gaps to Address

- **Exact mm coordinates for 14HP component positions:** DESIGN-LANGUAGE.md specifies positions at 5x pixel scale. These need dividing by 5 to get mm, then converting via `mm2px()` in the widget constructor. The arithmetic is straightforward but must be done precisely. Verify against the mockup coordinate grid before authoring the SVG. Key positions: panel center at x=35.56mm, secondary columns at x=21.2mm and x=49.9mm, CV jack columns at x=9.2mm, 22.8mm, 35.56mm, 48.8mm, 62mm.

- **Minimum pulse duty cycle floor:** Research recommends 10-15%. The exact value should be validated by ear at morph=1.0 with character=0 -- the pulse should be narrow and distinctive, not perceived as silence or near-silence. The computePulse() ARCHITECTURE.md prototype uses 12.5% as the base duty at character=0, which is a natural starting point. Confirm by listening and observing the display.

- **Bleed ring topology for 5 shapes:** Research identifies two options: ring topology (pulse bleeds into sine, matching the circular structure of the existing 4-shape ring) vs. open-ended (no bleed at the extremes of sine and pulse). An open-ended topology is argued to be more musically appropriate for a linear morph sweep, but this is a design decision. Make it explicitly before implementing Phase A.

- **NanoVG knob GPU cost:** The only genuinely unknown quantity in the milestone. Multi-pass radial gradient rendering for three knob sizes has not been profiled. If pursuing NanoVG knobs (Phase E), profile the first prototype knob at 60fps before building the full system.

- **nanosvg `<use>` with scale(-1,1) for forge emblem symmetry:** DESIGN-LANGUAGE.md specifies `<use>` mirroring for the forge emblem. ARCHITECTURE.md documents that `<use>` is not supported in nanosvg. The resolution is to duplicate and manually mirror geometry. Confirm by testing a simple mirrored element in VCV Rack before committing to the emblem geometry.

---

## Sources

### Primary (HIGH confidence)

- `src/AnalogLFO.cpp` (1,374 lines) -- direct source analysis for all integration points, current patterns, enum ordering, line-number-specific pitfall identification
- [VCV Rack Module Panel Guide](https://vcvrack.com/manual/Panel) -- SVG requirements, nanosvg limitations, HP sizing
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) -- drawLayer, font loading, widget lifecycle, parameter serialization
- [VCV Rack FramebufferWidget API](https://vcvrack.com/docs-v2/structrack_1_1widget_1_1FramebufferWidget) -- dirty flag, caching model, when to use vs. avoid
- [nanosvg GitHub (memononen/nanosvg)](https://github.com/memononen/nanosvg) -- supported elements, paint types, gradient types
- [VCV Rack RackWidget.cpp source](https://github.com/VCVRack/Rack/blob/v2/src/app/RackWidget.cpp) -- module position stored as grid coords, width derived from SVG
- [VCV Rack ModuleWidget.cpp source](https://github.com/VCVRack/Rack/blob/v2/src/app/ModuleWidget.cpp) -- panel width derivation from SVG
- `DESIGN-LANGUAGE.md` -- complete Forge Noir specification, color palette, typography, component sizing, layout coordinates at 5x scale
- [nanosvg DeepWiki](https://deepwiki.com/memononen/nanosvg) -- feature overview, paint type enums, path commands

### Secondary (MEDIUM confidence)

- [VCV Community: SVG transparencies and gradients](https://community.vcvrack.com/t/svg-transparencies-and-gradients/16833) -- gradient support details, 2-color limitation, VCV 2.6 improvements
- [VCV Community: Notes for theme-able SVGs with nanoSvg](https://community.vcvrack.com/t/notes-for-theme-able-svgs-with-nanosvg/20060) -- theming patterns, SVG cache behavior
- [VCV Community: Custom knob SVGs](https://community.vcvrack.com/t/how-can-i-implement-custom-knob-svgs/21399) -- SvgKnob subclass pattern
- [VCV Community: Custom Widget Light Bloom](https://community.vcvrack.com/t/custom-widget-light-bloom/14647/3) -- ring lights via dual radial gradients, box gradients for bloom
- [VCV Community: NanoVG optimization](https://community.vcvrack.com/t/trying-to-optimize-nanovg/16364) -- rendering performance, shadow impact
- [Perfect Circuit: What is PWM?](https://www.perfectcircuit.com/signal/what-is-pwm) -- 5%-95% duty cycle limits, synth design conventions
- [Sound on Sound: Synthesizing Strings with PWM](https://www.soundonsound.com/techniques/synthesizing-strings-pwm-string-sounds) -- duty cycle acoustic symmetry, standard limits
- [Mutable Instruments Plaits manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/plaits/manual/) -- waveform morph engine, narrow pulse behavior
- [VCO PWM click artifacts (Fundamental #140)](https://github.com/VCVRack/Fundamental/issues/140) -- discontinuity artifacts from duty cycle modulation

### Tertiary (LOW confidence -- empirical validation needed)

- GPU cost of multi-pass NanoVG knob rendering -- no community benchmarks found; must profile on target hardware before committing to Phase E
- FramebufferWidget with multiple gradient passes -- community anecdotes suggest benefit for static knobs but not quantified for the Forge Noir rendering approach
- nanosvg `<use>` with scale(-1,1) for forge emblem mirroring -- design language specifies this technique, architecture research flags it as unsupported; needs empirical confirmation in VCV Rack

---

*Research completed: 2026-03-27*
*Ready for roadmap: yes*
