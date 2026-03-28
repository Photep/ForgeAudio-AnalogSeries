# Domain Pitfalls: v1.3 Forge Noir

**Domain:** Adding morph-integrated PWM, Forge Noir panel redesign (12HP to 14HP), display layout restructuring, and animated sync badge to an existing 1,374-line VCV Rack 2 LFO module
**Researched:** 2026-03-27
**Confidence:** HIGH (based on direct source code analysis of AnalogLFO.cpp, VCV Rack 2 SDK documentation, VCV Rack GitHub source for patch serialization, nanosvg limitations documentation, FramebufferWidget API reference, and community-documented PWM click artifacts)

---

## Critical Pitfalls

Mistakes that cause rewrites, broken patches, audible artifacts, or architectural dead-ends. These MUST be addressed in the phase that introduces the relevant feature.

---

### Pitfall 1: Morph Continuity Break at the Square-to-Pulse Boundary

**What goes wrong:** The current morph sweep is `Sine -> Tri -> Saw -> Square` mapped to [0, 1] via `float scaled = morph * 3.f` (line 308), producing 3 crossfade segments of equal width (0.333 each). Extending to `Sine -> Tri -> Saw -> Square -> Narrow Pulse` adds a 4th segment, changing the math to `morph * 4.f` with 4 equal segments (0.25 each). This means every existing morph CV mapping and knob position shifts: what was "pure square" at morph=1.0 is now somewhere at morph=0.75, and the top 25% of the knob is the new pulse territory. Every existing patch that stores a morph parameter value breaks its sound -- a user who carefully dialed in a saw/square blend at morph=0.85 now gets something between square and pulse.

**Why it happens:** The morph param is stored as a normalized 0-1 float in patch JSON. VCV Rack restores parameter values directly on patch load. If the mapping of morph values to waveform regions changes, the same stored value produces a different waveform.

**Consequences:** Every existing patch with a non-zero morph setting sounds different after the update. Users who have dialed in specific morph positions for modulation targets lose their sounds. CV sequences mapped to morph shift their entire tonal palette.

**Prevention:**
1. Keep the existing [0, 1] range for `Sine -> Tri -> Saw -> Square` completely intact. The square waveform must remain at morph=1.0.
2. Extend the morph range beyond 1.0 for the pulse region. The new morph range becomes [0, 1.333] or similar, where [1.0, 1.333] maps square (50% duty) to narrow pulse. The param config changes from `configParam(MORPH_PARAM, 0.f, 1.f, ...)` to `configParam(MORPH_PARAM, 0.f, 1.333f, ...)`.
3. Alternatively (and better for knob feel): remap the internal morph value so that the 4-segment mapping uses unequal segment widths. The first three segments each get 25% of knob travel (unchanged positions within those segments), and the last 25% of knob travel covers square-to-pulse. The math: for `morph <= 0.75`, use `scaled = morph * (3.0/0.75) = morph * 4.0` which maps [0, 0.75] to [0, 3.0] -- same 3 crossfade segments. For `morph > 0.75`, the fourth segment maps [0.75, 1.0] to the square-to-pulse range. This preserves morph=0.0 as sine, morph=0.25 as tri, morph=0.5 as saw, morph=0.75 as square -- and adds morph > 0.75 as pulse territory.
4. **Critical check:** Verify that the existing `computeMorphedWave` `shapes[4]` array and segment indexing via `int segment = std::min((int)scaled, 2)` adapts correctly to 5 shapes and 4 segments. The clamp becomes `std::min((int)scaled, 3)`.

**Detection:** A/B test: save a patch at morph=0.5 (pure saw) before and after the change. If the output waveform differs, the mapping is broken.

**Phase to address:** The very first phase that modifies the morph range or adds the pulse waveform.

**Confidence:** HIGH (direct analysis of morph mapping at AnalogLFO.cpp lines 299-343; patch serialization confirmed to store raw param values)

---

### Pitfall 2: Pulse Wave Amplitude Collapse at Extreme Duty Cycles

**What goes wrong:** As duty cycle narrows from 50% toward very low values (5-10%), the pulse wave becomes a series of narrow spikes. The RMS energy drops proportional to the square root of the duty cycle. At 10% duty, the wave is at +1 for 10% of the cycle and -1 for 90% -- still bipolar, still peak +/-1. But at 5% duty, the positive spike is so narrow that it is perceived as much quieter than a 50% square wave, even though peak amplitude is the same. Users sweeping morph from square into pulse will hear the output "disappear." Worse, when this feeds downstream modules expecting consistent amplitude, the modulation depth appears to collapse.

**Why it happens:** A mathematically correct pulse wave with constant +/-1 amplitude and narrowing duty cycle has decreasing RMS. The DC offset also shifts: at 50% duty, DC=0. At 10% duty, DC = 0.1*(+1) + 0.9*(-1) = -0.8V normalized. The output biases heavily negative.

**Consequences:** Non-uniform loudness across the morph sweep. DC offset shift at narrow duty cycles. Downstream VCAs and filters behave inconsistently.

**Prevention:**
1. Limit minimum duty cycle to 10-15% (never approach 0%). At morph=1.0 the pulse is narrow but still energetic. The existing `squareDutySpread` component spread (line 282) adds up to +/-1% variation, so the effective minimum is safe at 10%.
2. Consider DC-compensated pulse generation: shift the waveform so the DC component stays near zero regardless of duty cycle. For duty cycle `d`, the high level = `(1-d)` and low level = `-d`, which maintains zero mean. This changes the spectral character slightly but eliminates DC shift.
3. Alternatively, accept the DC shift as authentic (real analog pulse waves have DC offset) but document it, and ensure the Drift knob's DC offset wander (line 794) doesn't compound with the pulse's inherent DC offset to exceed the +/-5V output range.
4. The 5V output scaling (line 778 `float outputVoltage = 5.f * sample`) will produce +5V/-5V peaks regardless of duty cycle. Verify that `sample` stays in [-1, +1] even with bleed (line 316-340) and DC offset (line 794) active simultaneously during pulse operation.

**Detection:** Sweep morph slowly from 0.75 (square) to 1.0 (narrow pulse) while monitoring output voltage on a scope module. Watch for: amplitude perceived drop, DC offset shift, output exceeding +/-5V.

**Phase to address:** Pulse waveform implementation phase.

**Confidence:** HIGH (standard DSP engineering; duty cycle / RMS relationship is well-established)

---

### Pitfall 3: Panel Width Change Causes Module Overlap on Existing Patch Load

**What goes wrong:** The VCV Rack patch JSON stores module position as grid coordinates `[x, y]` via `pos = pos.div(RACK_GRID_SIZE).round()` (confirmed from RackWidget.cpp source). Module width is NOT stored in the patch -- it is derived from the SVG panel at load time. When the module changes from 12HP (60.96mm) to 14HP (71.12mm), loading an old patch places the module at its saved grid position, but the module is now 2HP wider. Any module to the right that was placed flush against the old 12HP boundary now overlaps. VCV Rack calls `setModulePosForce()` during patch load, which pushes overlapping modules to the right. This cascading push can shift an entire rack row, breaking carefully laid-out patches.

**Why it happens:** VCV Rack's patch format deliberately does not store module width -- it trusts that the module's code/SVG defines the width. This is normally fine because module widths do not change. The 12HP to 14HP change is unusual. The `setModulePosForce()` function handles collisions by displacement, not by error.

**Consequences:** Patches load successfully but with shifted module positions. Users who arranged modules tightly (a common Eurorack practice) find their layout disrupted. Cables remain connected (they are stored by module/port ID, not position), so functionality is preserved, but the visual layout is damaged. Users may not notice the shift immediately if it only affects modules far to the right.

**Prevention:**
1. **Accept and document the shift.** This is a one-time migration cost. The module loads, cables work, parameters are preserved. Only the visual layout shifts. Document in release notes: "Analog LFO is now 14HP (was 12HP). Existing patches will load correctly but modules to the right may shift by 2HP."
2. **Do NOT change the module slug** (`ForgeAnalogLFO` in plugin.json line 14). Changing the slug would make the module unrecognizable to existing patches, which is far worse than a 2HP layout shift. The slug MUST remain identical.
3. **Do NOT change param/input/output enum values or ordering.** The current enums (lines 8-37) define param, input, and output IDs by order. Adding new params (e.g., a PWM-related param) must append to the end of the enum, not insert into the middle. Inserting shifts all subsequent IDs and breaks stored parameter values in patches.
4. **Test with an existing patch file.** Before release, save a patch with the 12HP module, then load it with the 14HP build. Verify: (a) module loads at correct position, (b) all params restore correctly, (c) cables reconnect, (d) right-neighbor modules push rather than overlap.

**Detection:** Load a saved 12HP patch with the 14HP build. Check if the module overlaps its right neighbor. Check if parameter values are preserved (especially morph, character, drift).

**Phase to address:** The panel redesign phase, immediately on first SVG swap.

**Confidence:** HIGH (confirmed from VCV Rack source: position stored as grid coords, width derived from SVG, `setModulePosForce` handles collisions)

---

### Pitfall 4: nanosvg Rendering Limitations Silently Breaking the Forge Noir Panel

**What goes wrong:** The Forge Noir design language (DESIGN-LANGUAGE.md) specifies features that nanosvg cannot render: CSS classes, gradients with more than 2 stops, SVG `<use>` elements for symmetry mirroring, `<text>` elements, SVG filters (Gaussian blur for the forge rune glow), linear-gradient syntax from CSS, `<defs>` blocks, and opacity via CSS properties. Designing the panel in HTML/CSS (as the mockup demonstrates) and then exporting to SVG will produce an SVG file that looks correct in a browser or Inkscape but renders as a black rectangle with scattered colored shapes in VCV Rack.

**Why it happens:** VCV Rack uses nanosvg (a minimal SVG parser) for panel rendering, not a full SVG renderer. nanosvg supports: basic shapes (rect, circle, ellipse, line, polyline, polygon, path), fill/stroke colors, opacity via `fill-opacity`/`stroke-opacity` attributes, and simple 2-stop linear/radial gradients. It does NOT support: `<text>`, `<use>`, `<defs>`, `<filter>`, `<clipPath>`, CSS stylesheets, `<style>` blocks, multi-stop gradients, elliptical radial gradients (only circular), or `transform` on gradient definitions.

**Consequences:** The panel renders incorrectly or as a blank rectangle. Elements that depend on unsupported features silently disappear. The module becomes unusable because the panel shows no labels or component positions. Since the SVG also defines component placeholder positions (colored circles/rects at specific coordinates), incorrect rendering can misplace knobs and jacks.

**Prevention:**
1. **Convert ALL text to paths** in Inkscape (Path > Object to Path) before saving. The current panel SVG (res/AnalogLFO.svg) already does this correctly -- every letter is a `<path>` element.
2. **Replace `<use>` mirror transforms with duplicated geometry.** The DESIGN-LANGUAGE.md specifies `<use href="#forge-half" transform="translate(panelWidth, 0) scale(-1, 1)">` for perfect symmetry. nanosvg does not support `<use>`. Instead, duplicate the left-half paths and manually mirror their coordinates. This is verbose but guaranteed to render.
3. **Replace multi-stop gradients with layered 2-stop gradients** or solid colors with opacity. The forge ember accent bar (`linear-gradient(90deg, #1a0800, #e85d26 25%, #daa520 50%, #e85d26 75%, #1a0800)`) has 5 stops. Replace with 2-3 overlapping rectangles with 2-stop gradients, or use a single solid color with opacity that approximates the effect.
4. **Remove all SVG filter elements.** The forge rune's Gaussian blur glow (`stdDeviation 1.8`) will not render. Approximate the glow effect by layering multiple paths with decreasing opacity and increasing stroke width (the same technique already used for the waveform trace glow in the display, lines 896-917).
5. **Remove all `<defs>` blocks.** Inline gradient definitions directly on elements using `fill="url(#gradientId)"` syntax where the gradient is defined as a sibling element, not inside `<defs>`.
6. **Test the SVG in VCV Rack early and often.** Do not design the entire panel in Inkscape and test at the end. After each major element (brand text, knob positions, display area, CV section), load the SVG in VCV Rack to verify rendering.
7. **Validate radial gradients are circular, not elliptical.** nanosvg has a hardcoded radius value (160px) for radial gradients that can cause incorrect proportions with elliptical gradients. Use only `r` (not `rx`/`ry`) for radial gradient radius. As of VCV Rack 2.6, gradient handling has improved, but circular is still safest.

**Detection:** Load the SVG in VCV Rack. If any element is missing, miscolored, or positioned wrong compared to Inkscape preview, the element uses an unsupported feature. Compare VCV Rack rendering against Inkscape rendering side-by-side.

**Phase to address:** Panel SVG creation phase -- the FIRST time the new SVG is authored.

**Confidence:** HIGH (confirmed from VCV Rack Panel Guide and community documentation of nanosvg limitations; verified against existing AnalogLFO.svg which correctly uses paths-only approach)

---

### Pitfall 5: FramebufferWidget Dirty Flagging Causing Stale or CPU-Draining Animation

**What goes wrong:** The current WaveformDisplay (line 802) is a `TransparentWidget` that redraws every frame via `drawLayer` (line 1264). This works because NanoVG rendering is lightweight for the current display. The Forge Noir design adds an animated sync badge (clock-pulse flash) and potentially the CRT scanline overlay, corner bracket accents, and three-column pill layout from the design language. If the display is placed inside a `FramebufferWidget` for performance (caching static elements), the animated sync badge will only update when the framebuffer is marked dirty. Two failure modes:

  **(a) Never marking dirty:** The sync badge appears frozen. The pulse flash animation never plays. The phase dot stops moving. The waveform trace becomes static.

  **(b) Always marking dirty:** Calling `setDirty()` every frame defeats the purpose of the FramebufferWidget. The display re-renders from scratch every frame, using MORE GPU than a plain TransparentWidget (because FramebufferWidget adds framebuffer management overhead on top of the NanoVG draw calls).

**Why it happens:** The FramebufferWidget is designed for widgets that change rarely (knob positions, static labels). Animated widgets that change every frame should NOT be inside a FramebufferWidget. The temptation is to wrap the entire display in one for "performance" without understanding when it helps versus hurts.

**Consequences:** Mode (a): Visual bugs where animations appear frozen. Mode (b): Higher CPU/GPU usage than before, possibly causing audio dropouts. Community developers have documented that reducing render frame rates from 60fps to 30fps or 15fps significantly reduces audio dropouts, confirming that rendering performance directly impacts audio stability.

**Prevention:**
1. **Keep the WaveformDisplay as a TransparentWidget** (current approach). The display is inherently animated -- the phase dot moves, overlays fade, the breathe animation runs continuously. It should redraw every frame. The current implementation is correct for this use case.
2. **If performance becomes an issue, optimize the NanoVG draw calls themselves** rather than adding a FramebufferWidget. Specific optimizations: (a) Skip pill text rendering when alpha < 0.001f (already done at lines 1200, 1209, 1216, 1230, 1235), (b) Reduce waveform path complexity from 256 points to 128 when the display is small (zoomed out), (c) Skip the comet trail (5 extra circles per frame) when the module is not focused/visible.
3. **For the animated sync badge specifically:** The current SYNC badge already blinks during ACQUIRING state (lines 1218-1221) via the breathe phase. A clock-pulse flash animation can be implemented the same way: store the timestamp of the last clock edge, compute flash intensity as `exp(-decay * timeSinceEdge)`, and render with that alpha. This is a single float computation per frame, negligible cost.
4. **The `step()` method (line 816) runs at widget frame rate (~60fps) regardless of FramebufferWidget.** Animation state updates in `step()` are cheap. The expensive part is NanoVG path rendering in `drawLayer()`. If performance is an issue, the right approach is to reduce draw complexity, not to cache an inherently dynamic widget.

**Detection:** Profile with VCV Rack's built-in performance meters. If the module's GUI thread time is disproportionate (>5% of frame time), investigate which NanoVG calls are expensive. Font loading via `APP->window->loadFont()` (line 1189) should be cached by VCV Rack internally, but verify it is not reloading from disk every frame.

**Phase to address:** Display layout restructuring phase -- when the display widget is modified for the three-column Forge Noir layout.

**Confidence:** HIGH (confirmed from VCV Rack FramebufferWidget API docs; current TransparentWidget approach validated against animated display requirements)

---

### Pitfall 6: Enum Reordering Breaking Patch Parameter Restoration

**What goes wrong:** VCV Rack serializes parameter values by their integer ID (the position in the ParamId enum). The current enum (lines 8-19) defines 10 params: MORPH_PARAM=0 through FM_ATTEN_PARAM=9. If a new parameter for PWM (e.g., a pulse width knob or internal state) is inserted anywhere except at the end, every subsequent param ID shifts. A patch saved with the old ordering where RATE_PARAM=3 stored value 0.7 Hz will now assign that 0.7 to whatever param is at ID=3 in the new enum, which could be something completely different.

**Why it happens:** C++ enums assign sequential integer values. Inserting a new entry before the terminal `PARAMS_LEN` shifts `PARAMS_LEN` up by 1 but does not shift existing entries -- unless the insertion is before them. The temptation when extending the morph range is to add a `PWM_PARAM` alongside `MORPH_PARAM` at the top of the enum for code readability.

**Consequences:** All parameter values in existing patches load into wrong parameters. Morph gets drift's value, character gets rate's value, etc. The module sounds completely wrong and the user cannot figure out why.

**Prevention:**
1. **ALWAYS append new params/inputs/outputs at the end of their respective enums**, immediately before `PARAMS_LEN` / `INPUTS_LEN` / `OUTPUTS_LEN`. Never insert in the middle.
2. The morph-integrated PWM design (from PROJECT.md) explicitly avoids needing a new PWM param -- PWM is part of the morph sweep, not a separate control. This is a design advantage: no new param enum entry needed for PWM itself.
3. If future needs require a new param (unlikely for v1.3 but possible), add it as: `NEW_PARAM, PARAMS_LEN` -- replacing the old `PARAMS_LEN` position with the new param and putting `PARAMS_LEN` after it.
4. Similarly for InputId: if any new input jack is added, it goes at the end before `INPUTS_LEN`. The Forge Noir redesign does not add new I/O jacks (per the active requirements in PROJECT.md), so this should not be an issue for v1.3.

**Detection:** Save a patch with the old build. Load it with the new build. Check that every knob position matches what was saved. Specifically verify: Morph at a non-default value, Rate at a non-default Hz, Phase Offset at a non-zero angle.

**Phase to address:** ANY phase that modifies the Module struct's enums.

**Confidence:** HIGH (standard VCV Rack development practice; enum ordering is the #1 backward compatibility pitfall documented by the community)

---

## Moderate Pitfalls

---

### Pitfall 7: Waveform Bleed Ring Topology Breaks with 5 Shapes

**What goes wrong:** The current waveform bleed implementation (lines 316-340) uses a 4-element wrapping ring: `shapes[4] = { sine, tri, saw, sqr }` with neighbor access via `(segment - 1 + 4) % 4` and `(segment + 2) % 4`. Adding a 5th shape (pulse) changes the ring size to 5. The modular arithmetic must change from `% 4` to `% 5`, the `shapes` array grows to 5 elements, and the neighbor identification logic changes because `segment + 2` no longer always points to the shape "right of segment end" -- with 5 shapes and 4 segments, the boundary conditions are different.

**What goes wrong specifically:** With 4 shapes and 3 segments, `segment` ranges [0, 2]. The "right neighbor" at `(segment + 2) % 4` correctly gives shape index 2, 3, or 0 (wrapping). With 5 shapes and 4 segments, `segment` ranges [0, 3]. The "right neighbor" at `(segment + 2) % 5` gives 2, 3, 4, or 0. This is actually correct for a 5-element ring -- but the `leftIdx = (segment - 1 + 5) % 5` needs verification. At segment=0 (sine-to-tri crossfade), leftIdx would be 4 (pulse). Is pulse bleeding into a sine/tri crossfade the desired behavior? In the 4-shape ring, square bleeds into sine, which is musically reasonable (adjacent in a circular topology). In the 5-shape ring, pulse (a narrow spike) bleeding into the sine region produces a sharp artifact.

**Prevention:**
1. Decide whether the ring should wrap (pulse neighbors sine) or be open-ended (sine has no left neighbor, pulse has no right neighbor). For a morph sweep that extends linearly from sine to pulse, an open-ended topology makes more sense: no bleed at the extremes, maximum bleed in the middle.
2. If using a ring topology, reduce the bleed intensity for the pulse shape because its extreme waveform character (narrow spikes) will produce disproportionate bleed artifacts compared to the smoother sine/tri/saw shapes.
3. Update the `shapes` array size, ring modulus, and neighbor index math atomically -- do not partially update.

**Phase to address:** Pulse waveform implementation phase.

**Confidence:** HIGH (direct analysis of bleed implementation at lines 316-340)

---

### Pitfall 8: Display Buffer Not Updated for Pulse Waveform Region

**What goes wrong:** The display buffer update function `updateDisplayBuffer` (line 345) calls `computeMorphedWave(p, morph, character)` for 256 phase samples. When the morph range extends to include pulse, `computeMorphedWave` must correctly compute the pulse waveform for morph values in the new pulse region. If `computeMorphedWave` is updated but `updateDisplayBuffer` still passes morph values clamped to [0, 1], the display never shows the pulse waveform -- it caps at square.

**Prevention:**
1. Ensure `updateDisplayBuffer` passes the full morph range (including pulse territory) to `computeMorphedWave`.
2. Verify that the display phase dot and comet trail correctly interpolate the buffer at pulse waveform positions.
3. The display update trigger logic (lines 742-757) uses `std::fabs(morph - prevDisplayMorph) > 0.002f` for change detection. This works regardless of morph range as long as the morph value is passed correctly.
4. If using the unequal-segment approach (Pitfall 1, option 3), the internal morph value passed to `computeMorphedWave` may differ from the knob value. Ensure the display uses the same internal morph value as the audio path.

**Phase to address:** Same phase as pulse waveform implementation.

**Confidence:** HIGH (direct analysis of display buffer path at line 345-360)

---

### Pitfall 9: SVG Component Placeholders Misaligned After 12HP-to-14HP Conversion

**What goes wrong:** VCV Rack reads colored circles/rects in the SVG to determine component (knob, jack, port) positions when using `createPanel()` and position-from-SVG helpers. The current panel uses hardcoded `mm2px(Vec(...))` positions in the widget constructor (lines 1296-1341), NOT SVG placeholders. However, if the Forge Noir panel SVG includes position placeholders (as is best practice per VCV Panel Guide), and those positions do not exactly match the `mm2px` calls in the C++ code, knobs and jacks will appear offset from their SVG labels.

**Prevention:**
1. Decide on ONE source of truth for component positions: either the C++ code with `mm2px(Vec(...))` or the SVG placeholders. The current code uses C++ positions exclusively. For the Forge Noir redesign, switching to SVG-driven positions would be cleaner but requires converting all `addParam`/`addInput`/`addOutput` calls to use `createParamCentered<>(mm2px(Vec(x, y)), ...)` where x, y match the SVG.
2. When switching from 12HP (60.96mm width) to 14HP (71.12mm width), ALL x-coordinate positions must be recalculated. The current positions (e.g., morph knob at x=30.48, which is center of 60.96mm) must shift to x=35.56 (center of 71.12mm) or to whatever the Forge Noir layout specifies.
3. The DESIGN-LANGUAGE.md specifies exact positions at 5x scale: panel center at x=178px (which is 35.56mm at 1x, confirming 14HP center). Secondary columns at x=106, 250 (21.2mm, 50mm). CV columns at x=46, 114, 178, 244, 310 (9.2mm, 22.8mm, 35.56mm, 48.8mm, 62mm). These must be transcribed precisely to mm2px calls.
4. **Screws:** The current screw positions (lines 1301-1304) use `box.size.x` which auto-adjusts to panel width. These are safe.

**Phase to address:** Panel SVG creation and widget constructor update.

**Confidence:** HIGH (direct analysis of widget constructor at lines 1296-1341)

---

### Pitfall 10: Font Loading in drawLayer Causing Per-Frame Disk Access

**What goes wrong:** The current `drawTextOverlays` method (line 1189) calls `APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"))` every time it is invoked. While VCV Rack caches fonts internally, this call still involves a hash lookup and shared_ptr reference counting every frame. The Forge Noir design language specifies THREE different fonts: Bebas Neue (brand/hero), Chakra Petch (labels), JetBrains Mono (data). If each font is loaded per-frame via `loadFont()`, the overhead triples.

**Prevention:**
1. Store font handles as member variables of the display widget, loaded once in the constructor or on first draw.
2. Note: VCV Rack's font loading requires a valid NVGcontext, which is only available during draw calls, not in the constructor. The standard pattern is to load on first draw and cache:
   ```cpp
   int jetbrainsFont = -1;
   void drawLayer(const DrawArgs& args, int layer) override {
       if (jetbrainsFont < 0) {
           auto font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/JetBrainsMono.ttf"));
           if (font) jetbrainsFont = font->handle;
       }
       // ... use jetbrainsFont
   }
   ```
3. The Forge Noir display design uses JetBrains Mono for pills (Hz, BPM, SYNC, ratio labels) and the current code uses ShareTechMono. This font change must be reflected in the actual font file bundled in `res/fonts/`.
4. Ensure font files are bundled in the plugin's `res/fonts/` directory, not loaded from system fonts. Use `asset::plugin()` not `asset::system()` for custom fonts. System fonts may not exist on all platforms.

**Phase to address:** Display layout restructuring phase.

**Confidence:** MEDIUM (VCV Rack's internal font caching may make this a non-issue in practice, but the 3-font overhead is worth optimizing proactively)

---

### Pitfall 11: Crossfade Artifacts When PWM Duty Cycle Is Modulated by Morph CV

**What goes wrong:** The existing anti-click crossfade (lines 780-789) fires on clock-edge phase resets with a 3ms cosine crossfade. When morph CV rapidly sweeps through the square-to-pulse transition, the duty cycle changes dramatically within a single LFO cycle. If the duty cycle changes mid-cycle such that the waveform's transition edge (where it flips from +1 to -1) moves past the current phase position, the waveform value can jump discontinuously within a single cycle -- no phase reset involved, so the crossfade never fires.

**Why it happens:** The square/pulse waveform is computed per-sample based on the current phase and current duty cycle. If duty = 0.5 at sample N and duty = 0.3 at sample N+1, and the phase is at 0.4, the output jumps from +1 (phase < 0.5) to -1 (phase > 0.3). This is a 2-unit (10V) discontinuity with no crossfade protection.

**Prevention:**
1. This is primarily an audio-rate concern (VCOs), not an LFO concern. At LFO rates (0.01-20Hz), the morph CV would need to change the duty cycle faster than the LFO frequency for this to be audible. At 20Hz, a duty cycle change within 50ms would matter. At 0.5Hz, only a change within 2 seconds matters, which is unlikely to be abrupt.
2. However, with FM input modulating the frequency up to much higher values (exponential FM can push the effective frequency above 100Hz), this becomes relevant.
3. The existing `computeSquare` (line 276) uses `tanh` sigmoid softening (line 294) when character > 0. At character=0 (pure digital), the edge is a hard step function, and the discontinuity from duty cycle change is maximally sharp. At higher character values, the `tanh` smoothing provides implicit anti-aliasing of the duty cycle transition.
4. **Simplest prevention:** Ensure the pulse waveform computation always applies minimum edge softening, even at character=0, when computing the pulse region. A subtle `tanh` with sharpness=50 (producing ~0.5% edge width) is inaudible as smoothing but prevents clicks from duty cycle modulation.

**Phase to address:** Pulse waveform implementation phase.

**Confidence:** MEDIUM (edge case that depends on morph CV rate and LFO frequency; the tanh character smoothing provides natural mitigation at non-zero character values)

---

### Pitfall 12: Three-Column Display Layout Breaking at Small Zoom Levels

**What goes wrong:** The Forge Noir design specifies a three-column display layout: left column (ratio pill, Hz readout, swing label), center column (waveform + phase dot), right column (SYNC badge, CLK/BPM stack). At 5x scale (356px panel width, display ~320px wide), the columns have generous spacing. But VCV Rack renders at multiple zoom levels, and at 50% zoom the display is only ~32px wide. Three columns of text pills at 5px font size become illegible at low zoom, and text elements overlap each other.

**Prevention:**
1. The current pill text implementation (lines 1018-1065) uses `nvgTextBounds` for measurement and positions pills at fixed margin offsets from display edges. This approach scales correctly with zoom because NanoVG handles coordinate transforms. Text at 10px font size is 10px regardless of zoom -- zoom is a viewport transform, not a font size change.
2. However, verify that pill backgrounds (the feathered box gradient at lines 1041-1053) do not overlap the waveform trace at narrow display widths. The `nvgScissor` call (line 1268) clips to display bounds, preventing overdraw, but overlapping pills look messy.
3. For the three-column layout, test at 50%, 75%, 100%, 150%, and 200% zoom. If pills overlap at low zoom, add a zoom-aware threshold that hides secondary information (swing label, CLK BPM) when the display is below a certain pixel width.
4. The design language specifies specific x-coordinates for columns (left: 16-55, center: 64-256, right: 262-302 at 5x scale). These translate to mm coordinates that work at any zoom. Verify these do not overlap at 1x scale.

**Phase to address:** Display layout restructuring phase.

**Confidence:** MEDIUM (NanoVG scaling handles most cases; the risk is primarily visual overlap at extreme zoom levels)

---

### Pitfall 13: Animated Sync Badge Consuming CPU When Not Visible

**What goes wrong:** Adding a clock-pulse flash animation to the SYNC badge means computing the flash decay curve every frame, even when: (a) the module is not clocked (badge not visible), (b) the module is off-screen (not in viewport), (c) the module is bypassed. If the animation updates an atomic variable from the audio thread and a timer in the GUI thread, both threads do unnecessary work.

**Prevention:**
1. Gate the animation computation on visibility. The current code already skips SYNC rendering when `syncFadeAlpha <= 0.001f` (line 1216). The flash animation should be gated the same way.
2. For the audio thread: store the last clock edge timestamp as an `std::atomic<float>` only when the clock state is ACQUIRING or LOCKED. The GUI thread reads this atomic and computes `exp(-decay * (currentTime - edgeTime))` only when drawing the badge.
3. The `step()` method (line 816) already checks `if (module)` before updating fade timers. The flash animation fits naturally into this pattern.
4. When the module is bypassed, `dimFactor` is set to 0.25f (line 1279). The flash animation should respect this dim factor to avoid a bright flash on a dimmed display.

**Phase to address:** Animated sync badge implementation phase.

**Confidence:** HIGH (standard optimization pattern; gating is already demonstrated in the existing code)

---

## Minor Pitfalls

---

### Pitfall 14: Custom Font Files Not Bundled in Plugin Distribution

**What goes wrong:** The Forge Noir design specifies Bebas Neue, Chakra Petch, and JetBrains Mono fonts. If these font files are not included in the plugin's `res/fonts/` directory, the module will crash or render with fallback fonts on machines that do not have these fonts installed system-wide. The current code uses `asset::system("res/fonts/ShareTechMono-Regular.ttf")` which loads from VCV Rack's built-in fonts. Custom fonts must use `asset::plugin(pluginInstance, "res/fonts/FontName.ttf")`.

**Prevention:**
1. Bundle all three font files (.ttf or .otf) in `res/fonts/`.
2. Check font licenses: Bebas Neue (SIL OFL), Chakra Petch (SIL OFL), JetBrains Mono (SIL OFL / Apache 2.0) -- all permissively licensed for bundling.
3. Use `asset::plugin()` not `asset::system()` for all custom font loads.
4. Note: These fonts are for the NanoVG display only. The panel SVG uses text-as-paths, so panel rendering does not depend on font files.

**Phase to address:** Display layout restructuring phase, when fonts are first loaded.

**Confidence:** HIGH (standard VCV Rack plugin pattern; straightforward to get right if remembered)

---

### Pitfall 15: Component Spread Seed Incompatibility After Adding Pulse Shape

**What goes wrong:** The `initComponentSpread()` function (lines 196-212) generates deterministic random offsets from the stored seed. It calls `d(spreadRng)` in a specific order to produce `ouWeightSpread[0..3]`, `characterSpread`, `sawCurvatureSpread`, `squareDutySpread`, `triAsymmetrySpread`, and `bleedSpread`. If a new spread parameter is added for the pulse waveform (e.g., `pulseMinDutySpread`) and the call is inserted before `bleedSpread`, then `bleedSpread` (and any subsequent values) will receive a different random value from the same seed. Existing patches with stored seeds will have their component spread characteristics change subtly.

**Prevention:**
1. Append any new spread parameter calls at the END of `initComponentSpread()`, after `bleedSpread`. This preserves all existing spread values from the same seed.
2. Since PWM is an extension of the morph range, the existing `squareDutySpread` may already cover the pulse region's behavior. If so, no new spread parameter is needed.
3. If a new parameter IS needed (e.g., minimum pulse width variation), add it after `bleedSpread = d(spreadRng) * 0.02f;` (line 211).

**Phase to address:** Pulse waveform implementation phase, if spread is added.

**Confidence:** HIGH (deterministic RNG ordering is a known concern; documented in v1.2 pitfalls as well)

---

### Pitfall 16: Panel SVG Cache Interference When Testing Multiple Versions

**What goes wrong:** VCV Rack caches loaded SVGs by filename. During development, if you save a modified `AnalogLFO.svg` and reload VCV Rack, the cache may serve the old version. More insidiously, if you have both the old 12HP and new 14HP SVG with the same filename in different build directories, the cache might serve the wrong one.

**Prevention:**
1. After modifying the SVG, fully quit and restart VCV Rack to clear the cache.
2. During development, use the VCV Rack `-d` (developer mode) flag if available, which may disable caching.
3. If testing both 12HP and 14HP versions, use different filenames temporarily during development, then rename to `AnalogLFO.svg` for final build.

**Phase to address:** Panel SVG creation phase.

**Confidence:** MEDIUM (cache behavior documented by community; exact invalidation behavior may vary by VCV Rack version)

---

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|-------------|---------------|------------|
| PWM via morph extension | Pitfall 1 (morph continuity), Pitfall 2 (amplitude collapse), Pitfall 6 (enum ordering) | Preserve existing morph mapping, limit min duty cycle, append new enums |
| Forge Noir panel SVG | Pitfall 3 (width change overlap), Pitfall 4 (nanosvg limitations), Pitfall 9 (placeholder alignment) | Document 2HP shift, text-as-paths only, test SVG in Rack early |
| Display layout restructuring | Pitfall 5 (FramebufferWidget misuse), Pitfall 10 (font loading), Pitfall 12 (zoom levels), Pitfall 14 (font bundling) | Keep TransparentWidget, cache fonts, test at multiple zoom levels |
| Animated sync badge | Pitfall 5 (dirty flagging), Pitfall 13 (CPU when hidden) | Gate animation on visibility, use decay curve from atomic timestamp |
| Waveform bleed update | Pitfall 7 (ring topology), Pitfall 8 (display buffer) | Update ring modulus and neighbor logic atomically, pass full morph range |
| Backward compatibility | Pitfall 1 (morph mapping), Pitfall 3 (panel width), Pitfall 6 (enum ordering), Pitfall 15 (spread seed) | Preserve all existing numeric mappings, append-only changes |

---

## Integration Pitfalls

These are not about any single feature but about how the features interact when combined.

### Integration Pitfall A: PWM + Character + Bleed Triple Interaction

The pulse waveform with character-modeled edge softening (tanh shaping) AND waveform bleed from adjacent shapes creates a complex interaction. At high character, the pulse edges are soft (tanh smoothing), the bleed adds saw/square crosstalk, and the DC offset from the drift engine shifts the baseline. Test the combined effect at: morph=0.9 (square/pulse crossfade), character=1.0 (max analog), drift=1.0 (max imperfection). Verify output stays within +/-5.5V and no NaN/inf occurs.

### Integration Pitfall B: Panel Resize + Display Resize Coordinate Mismatch

The display widget position is set in mm2px coordinates in the widget constructor (line 1312: `display->box.pos = mm2px(Vec(2.f, 15.f))`). When the panel changes from 12HP to 14HP, the display position and size must be updated to match the Forge Noir layout. If the display size changes but the internal NanoVG drawing coordinates (used for phaseToX, valueToY at lines 848-856) are based on `box.size`, they will auto-scale. However, pill text positions that use absolute offsets (e.g., `float margin = 4.f` at line 1193) may need adjustment if the display aspect ratio changes significantly.

### Integration Pitfall C: dataFromJson Migration for New Features

The `dataFromJson` method (lines 592-604) currently restores `spreadSeed` and `swingIndex`. If v1.3 adds new serialized state (e.g., animation preferences, display mode), the `dataFromJson` must gracefully handle loading patches from v1.2 that lack these fields. The current pattern of checking `if (swingJ)` before accessing the value is correct -- apply the same null-check pattern for any new fields.

---

## Sources

- [VCV Rack FramebufferWidget API](https://vcvrack.com/docs-v2/structrack_1_1widget_1_1FramebufferWidget) - dirty flag behavior, oversample, setDirty()
- [VCV Rack Module Panel Guide](https://vcvrack.com/manual/Panel) - SVG requirements, nanosvg limitations, HP sizing
- [VCV Rack Plugin API Guide](https://vcvrack.com/manual/PluginGuide) - module slugs, parameter serialization
- [FramebufferWidget community discussion](https://community.vcvrack.com/t/framebufferwidget-question/3041) - usage patterns, dirty flag timing
- [NanoVG optimization discussion](https://community.vcvrack.com/t/trying-to-optimize-nanovg/16364) - rendering performance, shadow impact
- [SVG gradient limitations](https://community.vcvrack.com/t/svg-transparencies-and-gradients/16833) - 2-stop limit, radial gradient issues
- [VCO PWM click artifacts (Fundamental #140)](https://github.com/VCVRack/Fundamental/issues/140) - MinBLEP double-discontinuity bug
- [PWM community discussion](https://community.vcvrack.com/t/clicks-and-pops-from-pwm/15824) - click artifacts from duty cycle modulation
- [nanosvg theme-able SVG notes](https://community.vcvrack.com/t/notes-for-theme-able-svgs-with-nanosvg/20060) - SVG cache behavior
- [VCV Rack RackWidget.cpp source](https://github.com/VCVRack/Rack/blob/v2/src/app/RackWidget.cpp) - module position stored as grid coords, width from SVG
- [VCV Rack ModuleWidget.cpp source](https://github.com/VCVRack/Rack/blob/v2/src/app/ModuleWidget.cpp) - panel width derived from SVG: `box.size.x = std::round(panel->box.size.x / RACK_GRID_WIDTH) * RACK_GRID_WIDTH`
- Direct source code analysis: `/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/src/AnalogLFO.cpp` (1,360 lines)
- Direct source code analysis: `/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/res/AnalogLFO.svg` (existing 12HP panel)
- Direct source code analysis: `/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/DESIGN-LANGUAGE.md` (Forge Noir specification)
