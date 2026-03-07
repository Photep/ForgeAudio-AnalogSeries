# Phase 6: Polish & Cleanup - Research

**Researched:** 2026-03-07
**Domain:** VCV Rack panel layout, NanoVG display animation, SVG editing, documentation maintenance
**Confidence:** HIGH

## Summary

Phase 6 closes four tech debt items identified in the v1.0 milestone audit. All items are cosmetic or documentation-level -- no DSP changes, no new features, no new enum values. The work spans three files: `src/AnalogLFO.cpp` (drift visual + widget layout), `res/AnalogLFO.svg` (panel graphics + labels + connection lines), and `.planning/ROADMAP.md` (stale text fix).

The primary technical risk is the bottom row panel restructure: moving from a single-row 7-component layout at y=104mm to a two-row layout (trimpots at ~96mm, jacks at ~108mm) with connection lines. This requires synchronized changes across three sources of truth (C++ widget, SVG panel, PANEL-SPEC.md). The drift visual changes are straightforward multiplier increases plus changing the drift level source from `params[DRIFT_PARAM].getValue()` to a CV-modulated value.

**Primary recommendation:** Split into two plans -- (1) code changes (drift visual + widget layout in C++) and SVG panel restructure, (2) visual verification in VCV Rack. The documentation fix is trivial and can be included in either plan.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Drift dot instability intensity: 4-5x current values. Trail jitter AND halo pulsing both amplified equally. Dot center stays on waveform trace (no center wobble).
- Drift instability CV response: Must read combined drift value (knob + attenuator * CV / 10V, clamped 0-1), not just `params[DRIFT_PARAM].getValue()`.
- Bottom row layout restructure: Standard Eurorack two-row convention. Upper row: 3 attenuator trimpots vertically above their jacks. Lower row: 3 CV input jacks + 1 output jack. Connection lines: thin vertical lines from each trimpot down to its associated jack.
- Rate knob stays at current position (y=86mm, own row).
- Output jack sits in bottom jack row alongside 3 CV inputs, visually distinct.
- OUT-02 documentation fix: verify REQUIREMENTS.md, fix stale ROADMAP.md success criteria.

### Claude's Discretion
- Connection line style (solid vs amber accent, weight) -- pick what looks best in the Forge Audio brand palette
- Drift visual animation source (OU-driven vs independent breathePhase) -- pick whichever is cleanest and looks best
- Drift visual intensity curve mapping (x^2 vs linear vs other) -- pick the mapping that feels best
- Exact vertical spacing for the two-row bottom section (trimpots ~96mm, jacks ~108mm or similar)
- Output jack visual distinction treatment (amber ring, different label color, etc.)

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope.
</user_constraints>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| VCV Rack SDK | 2.x | Module framework | Only framework for VCV Rack modules |
| NanoVG | (bundled) | Vector graphics for display | Bundled with Rack, used by `drawLayer()` |
| nanosvg | (bundled) | SVG panel rendering | Bundled with Rack, parses panel SVGs |

### Supporting
No additional libraries needed. All changes use existing VCV Rack SDK types (`Trimpot`, `PJ301MPort`, `RoundBlackKnob`) and NanoVG rendering calls already present in the codebase.

## Architecture Patterns

### Current File Structure (unchanged by this phase)
```
src/
  AnalogLFO.cpp      # Module + WaveformDisplay + Widget (single-file pattern)
  plugin.cpp          # Plugin registration
  plugin.hpp          # Plugin header
res/
  AnalogLFO.svg       # Panel SVG (nanosvg-compatible)
  PANEL-SPEC.md       # Designer handoff specification
```

### Pattern 1: Three Sources of Truth Synchronization
**What:** Component positions must match across C++ widget code, SVG components layer, and PANEL-SPEC.md.
**When to use:** Every time a component position changes.
**Existing pattern in codebase:**
```cpp
// C++ widget (mm2px coordinates)
addParam(createParamCentered<Trimpot>(mm2px(Vec(7.0, 104.0)), module, AnalogLFO::MORPH_ATTEN_PARAM));
```
```xml
<!-- SVG components layer (same coordinates) -->
<circle cx="7.0" cy="104.0" r="3.03" fill="#ff0000"/>   <!-- Morph CV Atten: Trimpot -->
```
```markdown
<!-- PANEL-SPEC.md (same coordinates) -->
| Morph CV Atten | Trimpot | 7.0 | 104.0 | 6.05 | Param |
```

### Pattern 2: CV-Modulated Parameter Reading
**What:** Combining knob value + attenuator * CV / 10V with clamping.
**Already used in `process()` for all three parameters:**
```cpp
float drift = rack::math::clamp(driftKnob + driftAtten * driftCV / 10.f, 0.f, 1.f);
```
**Need to replicate this in display code.** The challenge is that `drawPhaseDot()` runs on the GUI thread, not the audio thread. The module's `process()` already computes the combined drift value but does not expose it. Options:
1. Add an `std::atomic<float>` member to transfer the combined drift value from audio to GUI (matches existing `displayPhase` pattern).
2. Compute the same formula in the display code (reading params and inputs from GUI thread -- safe for display, VCV Rack allows this).

**Recommendation:** Use option 1 (atomic transfer) -- it is the established pattern in this codebase (`displayPhase`, `displayReadIdx`), avoids duplicating CV math, and guarantees the visual matches the audio's actual drift value exactly.

### Pattern 3: SVG Connection Lines
**What:** Thin vertical lines in the SVG panel connecting related components.
**Existing pattern:** The section divider at y=94mm.
```xml
<line x1="8" y1="94" x2="53" y2="94"
      stroke="#e8a838" stroke-width="0.15" stroke-opacity="0.5"/>
```
Connection lines from trimpots to jacks will use the same SVG `<line>` element with vertical orientation, amber color, and subtle opacity.

### Anti-Patterns to Avoid
- **Reading audio-thread state directly from GUI:** Do NOT access OU state, RNG, or other non-atomic members from `drawPhaseDot()`. Use atomics for any audio-to-GUI data transfer.
- **Changing enum values or adding params:** This phase must NOT add new params, inputs, or outputs. The enum is frozen.
- **Breaking existing waveform display behavior:** Drift visual tuning must not affect the waveform trace rendering, dot position tracking, or breathe animation.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Audio-to-GUI data transfer | Custom mutex/lock | `std::atomic<float>` | Lock-free, matches existing pattern, no audio thread blocking |
| Panel component layout | Manual pixel calculations | `mm2px(Vec(x, y))` with millimeter coordinates | VCV Rack standard, matches SVG viewBox coordinates directly |
| SVG text rendering | `<text>` elements | `<path>` letterforms | nanosvg does not support `<text>` -- all existing labels use geometric path letters |

## Common Pitfalls

### Pitfall 1: Three-Source Desynchronization
**What goes wrong:** Changing component positions in C++ but forgetting SVG or PANEL-SPEC.md (or vice versa).
**Why it happens:** Three files store the same coordinates independently.
**How to avoid:** Change all three in the same task. Verify with a grep for the old coordinates to ensure none remain.
**Warning signs:** Module loads but components appear offset from their labels/backgrounds.

### Pitfall 2: SVG Label Position Math Errors
**What goes wrong:** SVG path-based labels appear at wrong positions due to incorrect `translate()` or `scale()` calculations.
**Why it happens:** Each label requires manual calculation: `startX = centerX - (totalUnits * scale) / 2`. Easy to miscalculate unit widths or spacing.
**How to avoid:** Follow the existing label pattern exactly. Standard letter widths: most letters = 3 units, M = 4.5, N = 3.5, I = 0.9, W = 4.5. Standard spacing = 0.4-0.6 units between letters. Cap height = 5 units. Scale determines rendered size.
**Warning signs:** Labels visually off-center or overlapping components.

### Pitfall 3: Drift Visual Too Aggressive
**What goes wrong:** 4-5x multiplier makes the trail jitter or halo so large that visual elements leave the display bounds or look glitchy.
**Why it happens:** The display area is only 27mm tall. Trail jitter in Y coordinates could push trail segments outside the display if too large.
**How to avoid:** The display has `nvgScissor()` clipping, so out-of-bounds rendering is safe but visually undesirable. Current jitter: `driftLevel * 0.3f` pixels. At 5x: `driftLevel * 1.5f` pixels. With `driftLevel` max = 1.0 and display height ~80px (27mm * 2.95 px/mm), 1.5px of jitter is still well within bounds. This should be fine.
**Warning signs:** Trail segments visibly jumping to display edges.

### Pitfall 4: Drift Level Atomic Race
**What goes wrong:** Display reads a stale drift level because the atomic write in `process()` and the read in `drawPhaseDot()` use relaxed ordering.
**Why it happens:** `std::memory_order_relaxed` provides no synchronization guarantees on timing.
**How to avoid:** This is acceptable. The display runs at 60fps and the audio thread updates at 48kHz+. A frame or two of staleness is invisible. Use `memory_order_relaxed` for the new `displayDrift` atomic, same as `displayPhase`.
**Warning signs:** None -- this is standard VCV Rack practice.

### Pitfall 5: Bottom Row Component Overlap
**What goes wrong:** Jacks (8.03mm diameter) or trimpots (6.05mm diameter) overlap when positioned too close together.
**Why it happens:** 12HP panel is only 60.96mm wide. Four jacks need at least ~10mm center-to-center spacing.
**How to avoid:** With 4 jacks, use ~14mm center-to-center spacing. Example positions: x = 10, 24, 38, 52mm. This gives 14mm spacing with 4mm clearance from panel edges, comfortable for 8mm-diameter jacks.

## Code Examples

### Example 1: Adding displayDrift Atomic (audio-to-GUI transfer)

In the `AnalogLFO` struct, alongside existing atomics:
```cpp
std::atomic<float> displayDrift{0.f};   // Combined CV-modulated drift level for display
```

In `process()`, after computing the combined drift value:
```cpp
float drift = rack::math::clamp(driftKnob + driftAtten * driftCV / 10.f, 0.f, 1.f);
displayDrift.store(drift, std::memory_order_relaxed);
```

In `drawPhaseDot()`, replace the current line 378:
```cpp
// OLD: float driftLevel = module ? module->params[AnalogLFO::DRIFT_PARAM].getValue() : 0.f;
// NEW:
float driftLevel = module ? module->displayDrift.load(std::memory_order_relaxed) : 0.f;
```

### Example 2: Amplified Drift Visuals (4-5x increase)

Current values and their 5x amplified replacements:
```cpp
// Trail jitter: was 0.3f, now 1.5f (5x)
float jitterAmount = driftLevel * 1.5f;
float trailJitter = jitterAmount * std::sin(breathePhase * 3.7f + (float)i * 1.3f);

// Halo variation: was 0.15f, now 0.75f (5x)
float haloJitter = 1.f + driftLevel * 0.75f * std::sin(breathePhase * 2.3f);
```

### Example 3: Two-Row Widget Layout (C++)

```cpp
// Bottom section: Upper row (trimpots at ~96mm)
addParam(createParamCentered<Trimpot>(mm2px(Vec(10.0, 96.0)), module, AnalogLFO::MORPH_ATTEN_PARAM));
addParam(createParamCentered<Trimpot>(mm2px(Vec(24.0, 96.0)), module, AnalogLFO::CHARACTER_ATTEN_PARAM));
addParam(createParamCentered<Trimpot>(mm2px(Vec(38.0, 96.0)), module, AnalogLFO::DRIFT_ATTEN_PARAM));

// Bottom section: Lower row (jacks at ~108mm)
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.0, 108.0)), module, AnalogLFO::MORPH_CV_INPUT));
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.0, 108.0)), module, AnalogLFO::CHARACTER_CV_INPUT));
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.0, 108.0)), module, AnalogLFO::DRIFT_CV_INPUT));
addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(52.0, 108.0)), module, AnalogLFO::OUTPUT));
```

Note: exact X positions are Claude's discretion. These values give ~14mm center-to-center spacing which is comfortable for PJ301MPort (8.03mm diameter). The trimpots (6.05mm diameter) sit directly above their jacks at the same X coordinate.

### Example 4: SVG Connection Lines (Trimpot to Jack)

```xml
<!-- Connection lines: trimpot to jack -->
<line x1="10.0" y1="99.0" x2="10.0" y2="105.0"
      stroke="#e8a838" stroke-width="0.2" stroke-opacity="0.4"/>
<line x1="24.0" y1="99.0" x2="24.0" y2="105.0"
      stroke="#e8a838" stroke-width="0.2" stroke-opacity="0.4"/>
<line x1="38.0" y1="99.0" x2="38.0" y2="105.0"
      stroke="#e8a838" stroke-width="0.2" stroke-opacity="0.4"/>
```

Y1 = bottom of trimpot (trimpotY + radius), Y2 = top of jack (jackY - radius). These are approximate and depend on final Y positions chosen.

### Example 5: SVG Label Repositioning

Labels need to move above the jack row. For each label (MCV, CCV, DCV, OUT), recalculate the `translate()` Y to sit just above the jacks (e.g., y = jackY - 5mm for label baseline). X centers need to match the new component X positions.

Existing label pattern (MCV at x=14):
```xml
<!-- "MCV" label centered at x=14, y=96.2 -->
<g transform="translate(12.418, 96.2) scale(0.28)">
```

New label position would be at the new jack X with Y adjusted for the two-row layout.

### Example 6: Output Jack Visual Distinction

To distinguish the output jack from input jacks, add an amber accent ring in the SVG:
```xml
<!-- Output jack amber accent ring -->
<circle cx="52.0" cy="108.0" r="5.0" fill="none"
        stroke="#e8a838" stroke-width="0.3" stroke-opacity="0.6"/>
```

Or use a different label color (amber `#e8a838` instead of lavender `#8888aa`) for the "OUT" label.

## Detailed Change Map

### File: src/AnalogLFO.cpp

| Location | Change | Lines Affected |
|----------|--------|----------------|
| AnalogLFO struct | Add `std::atomic<float> displayDrift{0.f}` | Near line 52 |
| process() | Add `displayDrift.store(drift, ...)` after drift computation | After line 227 |
| drawPhaseDot() line 378 | Change drift source from `params[DRIFT_PARAM].getValue()` to `displayDrift.load(...)` | Line 378 |
| drawPhaseDot() line 398 | Change trail jitter from `0.3f` to `1.5f` (5x) | Line 398 |
| drawPhaseDot() line 415 | Change halo variation from `0.15f` to `0.75f` (5x) | Line 415 |
| Widget constructor lines 509-515 | Restructure bottom row from single row to two rows | Lines 509-515 |

### File: res/AnalogLFO.svg

| Location | Change | SVG Lines Affected |
|----------|--------|-------------------|
| Section divider | Move or adjust y=94 divider for new layout | Line 175-176 |
| Jack labels (MCV, CCV, DCV, OUT) | Reposition translate() to match new X/Y positions | Lines 182-227 |
| New content | Add 3 vertical connection lines (trimpot-to-jack) | New elements |
| New content | Add output jack accent ring (visual distinction) | New element |
| Components layer | Update all component circle cx/cy values to match new positions | Lines 259-273 |
| Labels | May need to add trimpot labels or keep unlabeled (Eurorack convention: trimpots often unlabeled) |

### File: res/PANEL-SPEC.md

| Section | Change |
|---------|--------|
| Section 3 (Layout Zones) | Update Jack labels zone and Jack row zone descriptions for two-row layout |
| Section 4 (Component Positions) | Update all bottom-row component X/Y values |
| Bottom row layout paragraph | Rewrite to describe two-row trimpot/jack layout with connection lines |

### File: .planning/ROADMAP.md

| Location | Change |
|----------|--------|
| Phase 2 success criteria #4 | Change "Module produces bipolar +/-5V output and inverted output simultaneously from the output jacks" to match the actual OUT-02 design decision |

### File: .planning/REQUIREMENTS.md

| Location | Change |
|----------|--------|
| OUT-02 | Already correct: "Inverted output removed by design decision (single output simplifies panel and display)". No change needed. |

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Single bottom row, 7 components | Two-row (trimpots above jacks) | This phase | Standard Eurorack layout convention |
| Drift visual reads knob only | Drift visual reads CV-modulated value | This phase | Visual matches audio behavior |
| Subtle drift visual (0.3/0.15 multipliers) | Perceptible drift visual (1.5/0.75 multipliers) | This phase | "Sells the alive feeling" |

## Open Questions

1. **Exact X positions for four-column bottom section**
   - What we know: Need 4 columns (Morph CV, Character CV, Drift CV, OUT). Panel is 60.96mm wide. Need ~10mm+ center-to-center for 8mm jacks.
   - What's unclear: Whether to distribute evenly across full width or cluster toward center.
   - Recommendation: Even distribution with ~14mm spacing (e.g., x = 10, 24, 38, 52). This matches the visual weight of the panel and provides clearance from screws. Fine-tune during visual verification.

2. **Section divider position**
   - What we know: Current divider at y=94mm. With trimpots moving to ~96mm, the divider may need to move up or be removed.
   - What's unclear: Whether to keep divider at all, or let connection lines serve as the visual separator.
   - Recommendation: Keep the divider, move it to y=92mm (above the trimpot row), maintaining the visual break between main controls and CV/IO section.

3. **Drift visual intensity -- 4x or 5x?**
   - What we know: User said "4-5x current intensity". Current trail = 0.3f, halo = 0.15f.
   - What's unclear: Exact feel can only be judged in VCV Rack.
   - Recommendation: Start at 5x (trail = 1.5f, halo = 0.75f). Adjust during visual verification if needed.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | None (VCV Rack plugin -- compilation + manual runtime verification) |
| Config file | Makefile (compilation verification only) |
| Quick run command | `make` |
| Full suite command | `make && make install` (then launch VCV Rack) |

### Phase Requirements -> Test Map

This phase has no formal requirement IDs (tech debt closure). Validation is against the 4 success criteria:

| Criterion | Behavior | Test Type | Automated Command | File Exists? |
|-----------|----------|-----------|-------------------|-------------|
| SC-1 | OUT-02 in REQUIREMENTS.md accurate | manual-only | `grep "OUT-02" .planning/REQUIREMENTS.md` | N/A |
| SC-2 | Drift dot instability perceptible | manual-only | Build + load in VCV Rack | N/A |
| SC-3 | Dot instability responds to CV | manual-only | Patch CV to drift input, observe | N/A |
| SC-4 | Bottom row uses consistent design | manual-only | Visual inspection in VCV Rack | N/A |

**Justification for manual-only:** VCV Rack plugins have no headless test mode. All visual/layout verification requires running the module in VCV Rack and observing the rendered result. Compilation verification (`make`) confirms no syntax/linking errors.

### Sampling Rate
- **Per task commit:** `make` (compilation check)
- **Per wave merge:** `make && make install` + VCV Rack visual inspection
- **Phase gate:** All 4 success criteria verified visually in VCV Rack

### Wave 0 Gaps
None -- no automated test infrastructure applies to this project (VCV Rack plugin architecture).

## Sources

### Primary (HIGH confidence)
- **Codebase inspection** (src/AnalogLFO.cpp, res/AnalogLFO.svg, res/PANEL-SPEC.md) -- all current code patterns, coordinates, and conventions
- **VCV Rack SDK** -- `mm2px()`, widget classes (`Trimpot`, `PJ301MPort`), `std::atomic` pattern for audio-to-GUI transfer
- **v1.0 Milestone Audit** (.planning/v1.0-MILESTONE-AUDIT.md) -- exact tech debt items to address

### Secondary (MEDIUM confidence)
- **VCV Rack Fundamental LFO** (referenced in CONTEXT.md) -- design language for two-row trimpot/jack layout convention
- **nanosvg limitations** (documented in PANEL-SPEC.md Section 6) -- SVG feature constraints

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - no new dependencies, all existing VCV Rack SDK patterns
- Architecture: HIGH - all changes follow established codebase patterns (atomics, mm2px, SVG path labels)
- Pitfalls: HIGH - pitfalls identified from direct code inspection and prior phase experience
- Layout positions: MEDIUM - exact positions are discretionary and need visual verification

**Research date:** 2026-03-07
**Valid until:** 2026-04-07 (stable -- VCV Rack SDK and codebase are not changing)
