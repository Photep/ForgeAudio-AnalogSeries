# Phase 20: Display Layout + CRT Aesthetic - Context

**Gathered:** 2026-04-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Restructure the waveform display into a three-column layout with data readout pills in dedicated left and right margins, waveform trace confined to the center ~60%, and CRT-inspired visual treatment (scanlines, corner brackets, breathing border glow). Align all display rendering to the Forge Noir color palette and design language.

</domain>

<decisions>
## Implementation Decisions

### Three-Column Layout
- **D-01:** Display uses three-column layout per DESIGN-LANGUAGE.md: left column (ratio, Hz, swing pills), center column (waveform trace + phase dot), right column (SYNC badge, CLK/BPM stack)
- **D-02:** `phaseToX()` coordinate helper updated to map waveform trace to center ~60% of display width, with consistent margins for pill columns on both sides
- **D-03:** Pills positioned in their dedicated margin columns — they never overlap the waveform trace

### CRT Scanline Effect
- **D-04:** Subtle atmosphere scanlines — barely visible (~0.03-0.05 opacity dark bands), enhancing the analog screen feel without competing with the waveform or pills
- **D-05:** Slow scroll animation — very slow downward drift (~1px/sec) simulating CRT refresh. Adds subtle life to the display.

### Pill Visual Refresh
- **D-06:** Full Forge Noir pill styling — ember-tinted backgrounds (rgba(232,93,38,0.1-0.12)) with ember stroke, replacing current navy feathered boxes
- **D-07:** Font switch from ShareTechMono to JetBrains Mono — bundle the .ttf in res/fonts/
- **D-08:** Pill border-radius matches design language: 2-2.5px with ember stroke border. Crisper, more defined pills.

### Waveform Trace Update
- **D-09:** 3-layer glow per design language: wide diffuse (stroke-width 7, opacity 0.06), medium (3.5, 0.15), sharp core (2, 0.85). Replaces current 4-pass approach.
- **D-10:** Add dashed zero-crossing reference line at vertical center — subtle, low opacity, helps users read waveform symmetry/offset
- **D-11:** Waveform trace color shifts to ember (#e85d26) family. Phase dot core stays molten gold (#f0a030). Trace = wire, dot = hot spot.

### Display Color Migration
- **D-12:** Display background adopts #030303 (near-black) from design language, replacing current navy-ish #0d0d1a
- **D-13:** Full design language border: 1.5px ember border (rgba(232,93,38,0.3)) with breathing glow animation pulsing 0.08-0.18 opacity over 5 seconds. Replace current inset shadow/highlight frame.
- **D-14:** Corner bracket decorations drawn on top of border: 8x8px L-shaped ember borders at each corner (1.5px, opacity 0.4) per design language

### Claude's Discretion
- Phase dot rendering approach: 3 concentric circles (design language) vs radial gradient halo (current) — pick whichever looks better with #030303 background
- Comet trail and drift jitter behavior preserved regardless of dot approach
- Scanline scroll implementation details (phase accumulator, line spacing at actual widget scale)
- `breathePhase` rate adjustment to match 5-second border glow cycle vs current 0.8Hz
- JetBrains Mono weight/size tuning for readability at small display size

### Folded Todos
- **Separate display pills from waveform visualiser** (from 2026-03-17, Phase 17 discussion) — directly addressed by three-column layout (D-01/D-02/D-03). Waveform and data readouts no longer compete for the same space.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Design Language
- `DESIGN-LANGUAGE.md` — Complete Forge Noir design language. Sections: Display Design (container, three-column layout, waveform rendering, pill styling), color palette, typography stack.
- `forge-noir.html` — Canonical layout reference with pixel positions. Display dimensions and column boundaries at 5x scale.

### Requirements
- `.planning/REQUIREMENTS.md` — DISP-01 through DISP-05 define the display requirements for this phase
- `.planning/PROJECT.md` — Key Decisions table, brand identity section, NanoVG constraints

### Existing Implementation
- `src/AnalogLFO.cpp` lines 846-1337 — `WaveformDisplay` struct: all NanoVG rendering code including `drawBackground()`, `drawInsetFrame()`, `drawWaveformTrace()`, `drawPhaseDot()`, `drawGlowText()`, `drawPillText()`, `drawBpmStack()`, `drawTextOverlays()`, `drawPlaceholder()`, `drawLayer()`
- `src/AnalogLFO.cpp` lines 891-895 — `phaseToX()` and `valueToY()` coordinate helpers (must be updated for center-column constraint)

### Prior Phase Context
- `.planning/phases/19-forge-noir-panel-custom-components/19-CONTEXT.md` — Phase 19 decisions (panel SVG, widget components, Forge Noir design language adoption)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `drawPillText()` (line 1062): feathered box-gradient pill renderer — needs color/style update but structure is reusable
- `drawGlowText()` (line 1045): 2-pass glow text (blur + sharp) — reusable for pills and labels
- `drawBpmStack()` (line 1111): dual-line CLK/BPM with shared pill — needs style update but layout logic reusable
- `breathePhase` animation (line 850): already drives phase dot breathing — can drive border glow (adjust rate from 0.8Hz to 0.2Hz for 5s cycle)
- Fade animation infrastructure (syncFadeAlpha, ratioFadeAlpha, etc.) — all pill fade logic preserved, just repositioned

### Established Patterns
- NanoVG rendering in `drawLayer()` layer 1 with save/scissor/restore
- Atomic display bridge variables for thread-safe audio→display data transfer
- `drawArgs` pattern for batching NanoVG draw calls
- Font loading via `APP->window->loadFont()` — will need JetBrains Mono path

### Integration Points
- `WaveformDisplay::drawLayer()` — main entry point, calls all sub-draw methods in sequence
- `phaseToX()` / `valueToY()` — every trace and dot render passes through these; updating them constrains the waveform to center column
- `drawTextOverlays()` — currently positions pills by corner; needs column-based positioning
- No changes to DSP code or atomic bridges — purely display-side refactor

</code_context>

<specifics>
## Specific Ideas

- Scanline slow scroll gives the display a living, analog-monitor quality — not a static overlay but a subtle continuous motion
- Ember trace with gold dot creates visual hierarchy: the waveform is the "circuit path" and the phase dot is the "hot solder point"
- Three-column layout directly resolves the long-standing pill/waveform overlap issue from Phase 17 discussion

</specifics>

<deferred>
## Deferred Ideas

### Reviewed Todos (not folded)
- **Pulse width modulation** (2026-03-17) — already completed in Phase 18. False keyword match.
- **Surge-style modulation routing system** (2026-03-17) — out of scope per PROJECT.md. False keyword match.

</deferred>

---

*Phase: 20-display-layout-crt-aesthetic*
*Context gathered: 2026-04-01*
