# Phase 1: Plugin Scaffold and Panel - Research

**Researched:** 2026-02-25
**Domain:** VCV Rack 2 plugin scaffolding, SVG panel design, module registration
**Confidence:** HIGH

## Summary

Phase 1 delivers a buildable VCV Rack 2 plugin with a branded 12HP SVG panel that establishes the spatial layout for all future phases. This is well-charted territory: the VCV Rack 2 SDK (version 2.6.6, confirmed installed locally at `/Users/mrcbrown/Claude/Software/Forge Audio/Rack-SDK/`) provides a stable Makefile-based build system, a standard plugin registration pattern, and SVG panel rendering via nanosvg. The existing POC at `/Users/mrcbrown/Claude/Software/Forge Audio/LFO/` has already validated the build pipeline, module registration, and SVG panel creation on this exact machine.

The primary technical work is (1) creating the plugin scaffold files (Makefile, plugin.json, plugin.hpp, plugin.cpp, module .cpp), (2) designing a 12HP SVG panel with the Forge Audio brand identity and the specific control layout decided by the user (diamond hierarchy: display, morph, character/drift, rate/pitch, 4 jacks), and (3) structuring the SVG with documented coordinates and nanosvg-compatible elements for designer handoff. The SVG must use millimeter units, convert all text to paths, and avoid CSS, filters, clipPaths, and complex gradients.

Component sizing for the decided layout on a 12HP (60.96mm wide) panel is the key design challenge: fitting a generous waveform display (~20mm tall), a hero morph knob (~15mm diameter, RoundBigBlackKnob), flanking character/drift knobs (~12mm diameter, RoundLargeBlackKnob), utility rate/pitch controls (~10mm, RoundBlackKnob), and 4 jacks (each ~8mm, PJ301MPort) in a single bottom row. All these dimensions have been verified against the actual SVG files in the installed VCV Rack 2.6.6 application. The 60.96mm width provides approximately 53mm of usable space between screw clear zones -- adequate but requiring careful spacing.

**Primary recommendation:** Adapt the proven POC scaffold pattern (Makefile, plugin.json, plugin.hpp, plugin.cpp, module .cpp) for the Analog Series plugin. Design the SVG panel at 60.96mm x 128.5mm with all text as paths, a hidden components layer for widget placement coordinates, and documented spacing for each control zone. Use the exact same build-install-test cycle validated in the POC.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **Panel dimensions:** 12 HP width (~60mm) -- spacious premium feel. Enough room for generous display and comfortable knob spacing.
- **Control layout (diamond / 2-1-2 hierarchy):**
  - Top third: Waveform display -- generous, approximately one-third of panel height. This is the showcase feature.
  - Below display: Morph knob -- large, hero position, centered. The primary interaction point.
  - Below morph: Character and Drift knobs -- medium-sized, flanking pair left and right.
  - Below character/drift: Rate knob and Octave pitch control -- smaller, utility row.
  - Bottom edge: Single row of 4 jacks -- morph CV in, drift CV in, output, inverted output. All in one line.
- **Visual hierarchy:**
  - Display dominates the upper panel -- it's what draws the eye
  - Morph knob is the largest control, signaling it's the primary knob
  - Character and drift are secondary, equal in size to each other
  - Rate and pitch are tertiary utility controls
  - Jacks are minimal, tucked at the very bottom

### Claude's Discretion
- Brand treatment and aesthetics (logo, typography, visual style within the navy/amber/lavender palette)
- Module browser metadata (name, tags, description)
- Exact knob sizes and spacing within the hierarchy
- SVG structure and coordinate system
- How placeholder positions for future controls are indicated

### Deferred Ideas (OUT OF SCOPE)
None -- discussion stayed within phase scope
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| INFR-01 | VCV Rack 2 plugin with proper plugin.json, Makefile, module registration, and cross-platform build | POC validates exact pattern; SDK 2.6.6 installed locally; Makefile, plugin.json, plugin.hpp/cpp, module .cpp patterns fully documented below |
| PANL-01 | 12HP SVG panel with Forge Audio brand identity (deep navy #1a1a2e, amber #e8a838, lavender labels, white-gray text) | 12HP = 60.96mm x 128.5mm; color palette established in POC; nanosvg compatibility rules documented; all text must be paths |
| PANL-02 | Panel SVG structured for designer handoff (template files, documented coordinates, nanosvg-compatible) | Component layer specification with color-coded circles documented; coordinate system uses mm with mm2px conversion; nanosvg restrictions catalogued; PANEL-SPEC.md pattern from POC provides handoff template |
</phase_requirements>

## Standard Stack

### Core

| Component | Version | Purpose | Why Standard | Confidence |
|-----------|---------|---------|--------------|------------|
| VCV Rack 2 SDK | 2.6.6 (2025-11-04) | Build headers, libraries, plugin.mk | Required to compile VCV Rack plugins. Installed locally at `../Rack-SDK`. | HIGH (verified locally) |
| VCV Rack 2 Free | 2.6.x | Runtime testing environment | Required to load and test plugin. | HIGH |
| C++17 via clang | System (Xcode CLT) | Implementation language | SDK's plugin.mk sets `-std=c++17`. macOS default compiler. | HIGH |
| GNU Make + plugin.mk | SDK bundled | Build system | SDK ships plugin.mk which handles compilation, linking, install, dist. Do not use CMake. | HIGH |
| nanosvg | SDK bundled | SVG panel rendering | VCV Rack's internal SVG renderer. Limited subset of SVG features. | HIGH |

### Supporting

| Component | Purpose | When to Use |
|-----------|---------|-------------|
| Inkscape or Affinity Designer | Create/edit SVG panels | Panel design; must export with mm units and text as paths |
| Python 3 (system) | SDK's helper.py (optional) | Can generate scaffold files, but hand-writing is recommended |
| `jq` (system) | plugin.mk reads version from plugin.json | Required by build system |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Hand-written scaffold | helper.py generated | helper.py works but hand-writing gives more control and understanding |
| GNU Make (plugin.mk) | CMake | Non-standard, breaks ecosystem conventions, creates maintenance burden |
| Single SVG panel | ThemedSvgPanel (light/dark) | SDK 2.4+ supports dark mode panels via `createPanel(lightPath, darkPath)` but adds complexity for Phase 1; defer dark mode support |

## Architecture Patterns

### Recommended Project Structure

```
ForgeAudio-AnalogSeries/           # Plugin root
  Makefile                          # Minimal: RACK_DIR, SOURCES, include plugin.mk
  plugin.json                       # Plugin + module metadata
  .gitignore                        # Exclude build/, dep/, dist/
  src/
    plugin.hpp                      # Shared: extern Plugin*, extern Model*
    plugin.cpp                      # init(): registers all models
    AnalogLFO.cpp                   # Module + Widget (placeholder process() for Phase 1)
  res/
    AnalogLFO.svg                   # Branded 12HP panel SVG
    PANEL-SPEC.md                   # Designer handoff documentation
```

### Pattern 1: Plugin Registration Chain (3-File Pattern)

**What:** Every VCV Rack 2 plugin follows a strict registration chain: plugin.hpp declares externs, plugin.cpp registers models in init(), module file defines Module + Widget + Model.
**When to use:** Always -- this is the only way VCV Rack discovers modules.
**Source:** Verified against POC and VCV Fundamental plugin.

```cpp
// src/plugin.hpp
#pragma once
#include <rack.hpp>
using namespace rack;
extern Plugin* pluginInstance;
extern Model* modelAnalogLFO;
```

```cpp
// src/plugin.cpp
#include "plugin.hpp"
Plugin* pluginInstance;
void init(Plugin* p) {
    pluginInstance = p;
    p->addModel(modelAnalogLFO);
}
```

```cpp
// src/AnalogLFO.cpp
#include "plugin.hpp"

struct AnalogLFO : Module {
    enum ParamId { /* ... */ PARAMS_LEN };
    enum InputId { /* ... */ INPUTS_LEN };
    enum OutputId { /* ... */ OUTPUTS_LEN };
    enum LightId { LIGHTS_LEN };

    AnalogLFO() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        // configParam/configInput/configOutput calls
    }

    void process(const ProcessArgs& args) override {
        // Empty for Phase 1 -- DSP added in Phase 2
    }
};

struct AnalogLFOWidget : ModuleWidget {
    AnalogLFOWidget(AnalogLFO* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/AnalogLFO.svg")));
        // addChild screws, addParam knobs, addInput/addOutput jacks
    }
};

Model* modelAnalogLFO = createModel<AnalogLFO, AnalogLFOWidget>("ForgeAnalogLFO");
```

### Pattern 2: SVG Panel Structure (nanosvg-Compatible)

**What:** SVG panel with millimeter units, viewBox matching width/height, all text as paths, optional hidden components layer for widget placement.
**When to use:** Every VCV Rack module panel.
**Source:** VCV Rack Panel Guide, verified against POC panel.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg"
     width="60.96mm" height="128.5mm"
     viewBox="0 0 60.96 128.5">

  <!-- Background fill -->
  <rect x="0" y="0" width="60.96" height="128.5" fill="#1a1a2e"/>

  <!-- All visible elements: paths only (no <text>) -->
  <!-- Brand name, module name, labels, decorative elements -->

  <!-- Hidden components layer for helper.py / documentation -->
  <g id="components" style="display:none">
    <!-- Params (red circles at center positions) -->
    <!-- Inputs (green circles) -->
    <!-- Outputs (blue circles) -->
  </g>
</svg>
```

### Pattern 3: Widget Placement with mm2px

**What:** All widget positions specified in millimeters, converted to pixels via `mm2px()`.
**When to use:** Every `addParam`, `addInput`, `addOutput` call.
**Source:** VCV Rack Plugin Development Tutorial, verified in POC.

```cpp
// mm2px converts millimeters to VCV Rack's internal pixel coordinate system
// Factor: 75.0 / 25.4 = 2.95276 pixels per mm
// Positions specified as center points
addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(30.48, 55.0)), module, AnalogLFO::MORPH_PARAM));
addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.0, 118.0)), module, AnalogLFO::MORPH_CV_INPUT));
```

### Pattern 4: Makefile (Minimal)

**What:** Minimal Makefile that sets RACK_DIR and includes plugin.mk.
**Source:** SDK template, verified against POC.

```makefile
RACK_DIR ?= ../Rack-SDK

FLAGS +=
CFLAGS +=
CXXFLAGS +=
LDFLAGS +=

SOURCES += $(wildcard src/*.cpp)

DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

include $(RACK_DIR)/plugin.mk
```

### Anti-Patterns to Avoid

- **Setting slug/version in Makefile or code:** These are read from plugin.json automatically in Rack 2. Setting them in init() is the obsolete v0.6/v1 pattern.
- **Using `RACK_DIR ?= ../..` without override:** Only works if plugin is inside Rack source tree. Set `RACK_DIR` as environment variable pointing to the SDK.
- **Text elements in SVG:** nanosvg silently ignores `<text>` -- they simply do not render. All text must be converted to `<path>` elements.
- **CSS `<style>` blocks in SVG:** nanosvg does not process CSS. Use inline `fill`, `stroke`, etc. attributes.
- **SVG dimensions in px:** SVG `width` and `height` must have `mm` suffix (e.g., `width="60.96mm"`). viewBox values should match in mm.
- **`<use>`, `<clipPath>`, `<filter>`, `<image>` in SVG:** All unsupported by nanosvg. Will silently fail.
- **Setting opacity on `<g>` groups:** Unreliable in nanosvg. Set opacity on individual elements.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Build system | Custom Makefile rules | SDK's `plugin.mk` via `include $(RACK_DIR)/plugin.mk` | Handles linking, packaging, cross-platform, install targets |
| Plugin loading | Custom dynamic library loading | SDK `init()` function + `addModel()` | Rack discovers and calls init() automatically |
| SVG panel rendering | Custom rendering | `createPanel(asset::plugin(...))` | Handles SVG parsing, scaling, nanosvg rendering |
| Asset path resolution | Hardcoded paths | `asset::plugin(pluginInstance, "res/file.svg")` | Resolves correct path regardless of install location |
| Module browser integration | Custom registration | `createModel<Module, Widget>("slug")` | Handles factory creation, JSON serialization, model registry |
| Widget positioning | Manual pixel math | `mm2px()` helper + `Vec()` | Correct unit conversion from mm to screen pixels |
| Geometric text in SVG | Custom font rendering | Path data for each letter (as in POC panel) | nanosvg cannot render `<text>`; path data renders pixel-perfectly |

**Key insight:** For Phase 1, the only creative work is the SVG panel design. Everything else follows established patterns from the POC.

## Common Pitfalls

### Pitfall 1: RACK_DIR Not Set or Wrong Path
**What goes wrong:** `make` fails with "plugin.mk: No such file or directory"
**Why it happens:** The Makefile `include $(RACK_DIR)/plugin.mk` cannot find the SDK.
**How to avoid:** Set `export RACK_DIR="/Users/mrcbrown/Claude/Software/Forge Audio/Rack-SDK"` or use the Makefile default `RACK_DIR ?= ../Rack-SDK` (which works since the SDK is at the sibling directory).
**Warning signs:** Any make error referencing plugin.mk or missing include files.

### Pitfall 2: Model Slug Mismatch
**What goes wrong:** Plugin compiles but module does not appear in the browser.
**Why it happens:** The string in `createModel<>("ForgeAnalogLFO")` must EXACTLY match `modules[].slug` in plugin.json. Case-sensitive.
**How to avoid:** Define slug once, use the exact same string in both places. Grep both files for the slug.
**Warning signs:** Plugin brand appears in browser but module is missing.

### Pitfall 3: SVG Panel Dimensions Wrong
**What goes wrong:** Panel appears at wrong size, components misaligned, or panel is blank.
**Why it happens:** SVG must use mm units. Height must be exactly 128.5mm. Width must be exact multiple of 5.08mm. For 12HP: width must be 60.96mm.
**How to avoid:** Use explicit `width="60.96mm" height="128.5mm" viewBox="0 0 60.96 128.5"` in the `<svg>` tag. Verify in a text editor after export.
**Warning signs:** Module wrong width in rack, components not aligned with panel artwork.

### Pitfall 4: Text Elements Invisible in Panel
**What goes wrong:** Labels and brand text appear in design tool but are invisible in VCV Rack.
**Why it happens:** nanosvg cannot render `<text>` elements. They are silently ignored.
**How to avoid:** Convert ALL text to curves/paths before export. Search the exported SVG for `<text` -- should find zero matches. The POC demonstrates geometric path letterforms that render perfectly.
**Warning signs:** Panel loads but text is missing. Labels appear in Inkscape/Affinity but not in Rack.

### Pitfall 5: Plugin Version Mismatch
**What goes wrong:** Plugin rejected or modules not working.
**Why it happens:** VCV Rack 2 expects plugin version starting with "2." (e.g., "2.0.0").
**How to avoid:** Use `"version": "2.0.0"` in plugin.json.
**Warning signs:** Check `~/Library/Application Support/Rack2/log.txt` for errors.

### Pitfall 6: Forgetting make install
**What goes wrong:** Old plugin loads, changes not visible.
**Why it happens:** `make` compiles to `build/`. Must run `make install` to copy to Rack's plugins folder.
**How to avoid:** Always `make && make install`. Then quit and restart VCV Rack (no hot-reload).
**Warning signs:** Changes not taking effect after Rack restart.

### Pitfall 7: Component Overlap on 12HP Panel
**What goes wrong:** Knobs or jacks overlap each other or extend past panel edges.
**Why it happens:** 12HP (60.96mm) is spacious but not unlimited. A RoundBigBlackKnob is ~15mm diameter; two medium knobs (12mm each) side by side need at least 24mm plus margins.
**How to avoid:** Use the verified component sizes below. Ensure minimum 2mm clearance between component edges. Plan the layout on paper/grid first.
**Warning signs:** Visual overlap when module loads in Rack.

## Code Examples

### Complete plugin.json for Analog Series

```json
{
  "slug": "ForgeAudio-AnalogSeries",
  "name": "Forge Audio - Analog Series",
  "version": "2.0.0",
  "license": "GPL-3.0-or-later",
  "brand": "Forge Audio",
  "author": "Forge Audio",
  "authorUrl": "",
  "pluginUrl": "",
  "sourceUrl": "",
  "description": "Analog-modeled oscillator modules with waveform morphing, classic synth character, and drift",
  "modules": [
    {
      "slug": "ForgeAnalogLFO",
      "name": "Analog LFO",
      "description": "Sub-audio oscillator with waveform morphing, analog character, and drift modeling",
      "tags": [
        "Low-frequency oscillator",
        "Waveshaper"
      ]
    }
  ]
}
```

**Tag choices:** "Low-frequency oscillator" (primary function) and "Waveshaper" (waveform morphing aspect). Both are valid tags from the official VCV Rack manifest specification.

### Complete .gitignore

```
# VCV Rack build artifacts
build/
dep/
dist/
plugin.so
plugin.dylib
plugin.dll
*.d
*.o

# OS files
.DS_Store
Thumbs.db
```

### Build and Test Cycle

```bash
# Navigate to project root
cd "/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series"

# Build (RACK_DIR resolved via Makefile default ../Rack-SDK)
make

# Install to VCV Rack plugins folder
make install
# Installs to: ~/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio-AnalogSeries/

# Quit VCV Rack if running, then relaunch
open "/Applications/VCV Rack 2 Free.app"

# Check for errors
tail -50 ~/Library/Application\ Support/Rack2/log.txt
```

### SVG Panel Skeleton (12HP)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg"
     width="60.96mm" height="128.5mm"
     viewBox="0 0 60.96 128.5">

  <!-- Background: Deep Navy -->
  <rect x="0" y="0" width="60.96" height="128.5" fill="#1a1a2e"/>

  <!-- Panel border -->
  <rect x="0.15" y="0.15" width="60.66" height="128.2"
        fill="none" stroke="#2d2d4a" stroke-width="0.3"/>

  <!-- Top amber accent stripe -->
  <rect x="0" y="0" width="60.96" height="1" fill="#e8a838"/>

  <!-- Bottom amber accent stripe -->
  <rect x="0" y="127.5" width="60.96" height="1" fill="#e8a838"/>

  <!-- Brand text, module name, labels: ALL as <path> elements -->
  <!-- ... path data for "FORGE AUDIO", "ANALOG LFO", etc. ... -->

  <!-- Section dividers, decorative elements -->
  <!-- ... -->

  <!-- Hidden components layer (for documentation / helper.py) -->
  <g id="components" style="display:none">
    <!-- Params (red) -->
    <circle id="Morph" cx="30.48" cy="55.0" r="5" fill="#ff0000"/>
    <circle id="Character" cx="18.0" cy="72.0" r="4" fill="#ff0000"/>
    <circle id="Drift" cx="42.96" cy="72.0" r="4" fill="#ff0000"/>
    <circle id="Rate" cx="18.0" cy="88.0" r="3" fill="#ff0000"/>
    <circle id="Octave" cx="42.96" cy="88.0" r="3" fill="#ff0000"/>
    <!-- Inputs (green) -->
    <circle id="MorphCV" cx="9.5" cy="118.0" r="2.5" fill="#00ff00"/>
    <circle id="DriftCV" cx="23.82" cy="118.0" r="2.5" fill="#00ff00"/>
    <!-- Outputs (blue) -->
    <circle id="Out" cx="37.14" cy="118.0" r="2.5" fill="#0000ff"/>
    <circle id="InvOut" cx="51.46" cy="118.0" r="2.5" fill="#0000ff"/>
  </g>
</svg>
```

**Note:** The coordinates above are starting-point recommendations. Exact positions will be refined during implementation to ensure proper clearance between all components. See the Layout Calculation section below.

## Component Size Reference (Verified)

Measured directly from the SVG files in the installed VCV Rack 2.6.6 application at `/Applications/VCV Rack 2 Free.app/Contents/Resources/res/ComponentLibrary/`.

| Component | Widget Class | Pixel Size | MM Size | Recommended Use |
|-----------|-------------|-----------|---------|-----------------|
| Small knob | `RoundSmallBlackKnob` | 22.68 x 22.68 px | ~7.68mm dia | Attenuators, fine controls |
| Standard knob | `RoundBlackKnob` | 28.35 x 28.35 px | ~9.60mm dia | Utility controls (Rate, Pitch) |
| Large knob | `RoundLargeBlackKnob` | 36.00 x 36.00 px | ~12.19mm dia | Secondary controls (Character, Drift) |
| Big knob | `RoundBigBlackKnob` | 45.00 x 45.00 px | ~15.24mm dia | Primary control (Morph) |
| Huge knob | `RoundHugeBlackKnob` | 53.86 x 53.86 px | ~18.24mm dia | Hero control (available if Morph needs to be larger) |
| Jack port | `PJ301MPort` | 23.70 x 23.70 px | ~8.03mm dia | All input/output jacks |
| Screw | `ScrewSilver` | 15.00 x 15.00 px | ~5.08mm dia | Corner screws (1HP square) |

**Conversion factor:** `mm2px(mm) = mm * (75.0 / 25.4) = mm * 2.95276`

## 12HP Panel Layout Calculation

### Available Space

| Dimension | Value | Notes |
|-----------|-------|-------|
| Panel width | 60.96mm | 12HP x 5.08mm |
| Panel height | 128.5mm | Standard Eurorack 3U |
| Panel center X | 30.48mm | Horizontal center |
| Screw clear zone | ~5mm dia at each corner | Keep 2.5mm radius clear |
| Usable width | ~50.8mm | Between screw zones (5.08mm to 55.88mm) |
| Usable height | ~120.4mm | Between screw zones (4.0mm to 124.5mm) |

### Knob Selection for Visual Hierarchy

| Control | Widget | Diameter | Rationale |
|---------|--------|----------|-----------|
| **Morph** (hero) | `RoundBigBlackKnob` | ~15.24mm | Largest control = primary interaction. Unmistakable visual dominance. |
| **Character** (secondary) | `RoundLargeBlackKnob` | ~12.19mm | Medium size, equal to Drift. Clearly smaller than Morph. |
| **Drift** (secondary) | `RoundLargeBlackKnob` | ~12.19mm | Medium size, equal to Character. Flanking pair. |
| **Rate** (tertiary) | `RoundBlackKnob` | ~9.60mm | Smaller utility control. |
| **Octave** (tertiary) | `RoundBlackKnob` | ~9.60mm | Smaller utility control. |

### Vertical Zone Allocation (Diamond Layout)

Panel height 128.5mm divided into zones matching the user's diamond hierarchy:

| Zone | Y Range | Height | Content |
|------|---------|--------|---------|
| Top stripes + brand | 0 - 8mm | 8mm | Amber accent, "FORGE AUDIO" brand |
| Module name | 8 - 14mm | 6mm | "ANALOG LFO" title |
| **Waveform display** | 16 - 42mm | ~26mm | Generous display area (~one-third of usable height, as requested) |
| Morph label + knob | 43 - 60mm | ~17mm | "MORPH" label + RoundBigBlackKnob centered |
| Character/Drift labels + knobs | 62 - 78mm | ~16mm | Flanking pair with labels |
| Rate/Octave labels + knobs | 80 - 94mm | ~14mm | Utility row with labels |
| Section divider | 95mm | ~1mm | Thin divider line |
| Jack labels | 97 - 100mm | ~3mm | "MCV", "DCV", "OUT", "INV" labels |
| **Jack row** | 100 - 108mm | ~8mm | 4 jacks in a single row |
| Bottom brand + stripe | 109 - 128.5mm | ~19mm | "FORGE AUDIO" branding, amber stripe |

### Horizontal Jack Spacing (4 Jacks in Single Row)

Usable width between margins: ~52mm (from ~4.5mm to ~56.5mm).
4 jacks at ~8mm diameter each need 32mm just for the jacks. With ~20mm to distribute as spacing:

| Jack | Label | Center X | Notes |
|------|-------|----------|-------|
| Morph CV | MCV | ~11.0mm | Left side input |
| Drift CV | DCV | ~24.5mm | Center-left input |
| Output | OUT | ~38.0mm | Center-right output |
| Inv. Output | INV | ~51.5mm | Right side output |

Spacing between jack centers: ~13.5mm (well above the ~8mm diameter minimum).

### Horizontal Knob Spacing (Flanking Pairs)

Character (left) and Drift (right) at ~12.19mm diameter:
- Character center X: ~20mm (from panel left edge)
- Drift center X: ~41mm (from panel left edge)
- Gap between knob edges: 41 - 20 - 12.19 = ~8.8mm (comfortable clearance)

Rate (left) and Octave (right) at ~9.60mm diameter:
- Same X positions as Character/Drift for visual alignment
- Smaller knobs, so even more clearance

### Waveform Display Dimensions

Per the user's decision, the display should occupy approximately one-third of panel height and feel like the centrepiece.

| Property | Value | Notes |
|----------|-------|-------|
| Width | ~52mm | Nearly full panel width minus small margins |
| Height | ~26mm | Generous, approximately 1/5 of total panel -- see note |
| Top-left X | ~4.5mm | Small margin from panel edge |
| Top-left Y | ~16mm | Below module name |
| Center | 30.48mm, 29mm | Center of display area |

**Note on "one-third":** The user specified "approximately one-third of panel height" for the display. On a 128.5mm panel, one-third is ~43mm. However, the usable height after brand areas, screws, and the 4 control rows is approximately 100mm. ~26mm is roughly one-quarter of the full panel but approximately one-third of the control area (display through jacks spans ~92mm; display at 26mm is ~28% of that span). The display needs to look generous relative to the controls, not necessarily be exactly 42.8mm tall. If this feels too small during implementation, it can be expanded to ~30-35mm by compressing the spacing between control rows.

## SVG Design Recommendations (Claude's Discretion)

### Brand Treatment

Recommendation: Follow the POC panel pattern closely for brand consistency:
- **Top:** "FORGE AUDIO" brand text in muted lavender (#8888aa), small all-caps, centered
- **Below brand:** "ANALOG LFO" module name in bright white-gray (#e0e0e0), large bold, centered
- **Amber accent stripe:** Thin (1mm) amber (#e8a838) stripe at top and bottom panel edges
- **Decorative line:** Amber line under module name, as in POC

### Module Browser Metadata

Recommendation for plugin.json:
- **Module name:** "Analog LFO" -- clear, descriptive, distinguishes from future VCO
- **Description:** "Sub-audio oscillator with waveform morphing, analog character, and drift modeling"
- **Tags:** `["Low-frequency oscillator", "Waveshaper"]`
  - "Low-frequency oscillator" -- primary module function (official VCV tag)
  - "Waveshaper" -- captures the morphing/character aspect (official VCV tag)

### Knob Size Selection

Recommendation (documented above): Use the 3-tier hierarchy that directly maps to the user's visual hierarchy:
- Hero: `RoundBigBlackKnob` (~15mm) for Morph
- Secondary: `RoundLargeBlackKnob` (~12mm) for Character and Drift
- Tertiary: `RoundBlackKnob` (~10mm) for Rate and Octave

Alternative considered: `RoundHugeBlackKnob` (~18mm) for Morph. Rejected because at 18mm diameter on a 61mm wide panel, it would consume ~30% of the width and leave insufficient room for the flanking knobs below. The 15mm Big knob provides clear visual dominance while leaving comfortable margins.

### SVG Structure

Recommendation: Organize the SVG with clear sections matching the POC pattern:
1. Background (`<rect>` fill)
2. Border and accent stripes
3. Brand text group (paths)
4. Module name group (paths)
5. Display area placeholder (rectangle showing reserved area)
6. Section labels and dividers
7. Control labels (paths)
8. Hidden components layer (`id="components"`, `style="display:none"`)

### Placeholder Positions for Future Controls

Recommendation: In the SVG's components layer, include placeholder circles for ALL controls that will exist in the final module (even those added in future phases like the waveform display overlay). Mark Phase 1 controls as primary and future-phase controls with a comment. This gives the designer the complete spatial picture.

Additionally, for the waveform display area, include a `<rect>` element (in the visible portion of the SVG, not just the components layer) with a subtle fill (#0d0d1a, slightly darker than the panel background) to visually reserve the display space. This makes the display area visible even before the NanoVG display widget is implemented in Phase 3.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Slug/version in Makefile | Only in plugin.json | Rack v1 to v2 | Remove from Makefile |
| `p->slug = "..."` in init() | Auto-read from plugin.json | Rack v1 to v2 | init() only needs addModel() |
| 3-digit hex colors | 6-digit hex required | SDK 2.6.5 | helper.py fixed; always use 6-digit |
| Manual factory functions | `createModel<M, W>("slug")` template | Rack v2 | Cleaner registration |
| `APP->window->loadSvg()` | `createPanel(asset::plugin(...))` | Rack v2 | Simpler, supports dark mode |
| No dark mode support | `ThemedSvgPanel` + `createPanel(light, dark)` | Rack 2.4.0 | Optional; defer for Phase 1 |
| Limited gradient support | 2-stop linear AND radial gradients | Rack 2.6.0 | SVG can now use simple radial gradients |

**Deprecated/outdated:**
- `helper.py` SVG-to-code generation: Works but community recommends hand-placing widgets
- Setting slug/version in code: Will cause warnings in Rack 2.x
- Using px units in SVG: Will produce wrong dimensions

## Open Questions

1. **Plugin slug format: hyphenated or not?**
   - What we know: Slugs must be alphanumeric with hyphens and underscores. The POC uses "ForgeAudio" (no hyphen). The Analog Series could use "ForgeAudio-AnalogSeries" or "ForgeAudioAnalogSeries".
   - What's unclear: Whether the slug should include "AnalogSeries" (since this is a new, separate plugin from the POC) or remain "ForgeAudio" (single plugin, multiple modules).
   - Recommendation: Use `"ForgeAudio-AnalogSeries"` as the plugin slug -- clearly distinguishes from the POC plugin, uses standard hyphen separator, and allows future expansion with more Analog Series modules. The module slug would be `"ForgeAnalogLFO"`.

2. **Dark mode panel (ThemedSvgPanel)?**
   - What we know: SDK 2.4+ supports `ThemedSvgPanel` with separate light/dark SVG files. The Forge Audio aesthetic (deep navy background) already looks good in both light and dark Rack themes.
   - What's unclear: Whether the navy background is sufficient for dark mode or if a separate dark SVG is needed.
   - Recommendation: Skip ThemedSvgPanel for Phase 1. The deep navy (#1a1a2e) background already works well against dark backgrounds. Can be added later as a minor enhancement.

3. **Exact display area dimensions**
   - What we know: User wants "approximately one-third of panel height" for the display as a generous centrepiece.
   - What's unclear: Whether this means one-third of total panel (128.5/3 = ~43mm tall, which is very large) or one-third of the control area.
   - Recommendation: Start with ~26-30mm tall display. This provides a visually dominant display area while leaving room for the 5 knobs and jack row below. Adjust during implementation if it feels too small -- the layout has ~4mm of flexible spacing between zones that can be compressed.

## Sources

### Primary (HIGH confidence)
- [VCV Rack Plugin Development Tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial) -- Scaffold pattern, build commands, module registration
- [VCV Rack Plugin Manifest](https://vcvrack.com/manual/Manifest) -- plugin.json spec, complete tag list, version rules
- [VCV Rack Panel Guide](https://vcvrack.com/manual/Panel) -- SVG dimensions, units, component layer colors, nanosvg restrictions
- [VCV Rack Downloads](https://vcvrack.com/downloads/) -- SDK 2.6.6 confirmed available (2025-11-04)
- [VCV Rack Changelog](https://github.com/VCVRack/Rack/blob/v2/CHANGELOG.md) -- Version history, SVG gradient support in 2.6.0
- Local Rack SDK at `/Users/mrcbrown/Claude/Software/Forge Audio/Rack-SDK/` -- Verified 2.6.6, component library headers
- Local VCV Rack app at `/Applications/VCV Rack 2 Free.app/` -- Component SVG files measured directly
- POC at `/Users/mrcbrown/Claude/Software/Forge Audio/LFO/` -- Validated build pipeline, SVG panel pattern, registration chain

### Secondary (MEDIUM confidence)
- [VCV Community: nanosvg theming notes](https://community.vcvrack.com/t/notes-for-theme-able-svgs-with-nanosvg/20060) -- SVG caching behavior, theming approach
- [VCV Community: SVG panel dimensions](https://community.vcvrack.com/t/svg-panel-dimensions-and-eurorack/9874) -- Community confirmation of dimension conventions
- POC PANEL-SPEC.md -- Designer handoff pattern, export checklist, nanosvg compatibility summary

### Tertiary (LOW confidence)
- Component sizes extrapolated from SVG pixel dimensions -- pixel-to-mm conversion assumes 75 DPI consistently; small rounding differences possible

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- SDK 2.6.6 installed and verified locally; POC validates entire build pipeline on this machine
- Architecture: HIGH -- Registration chain, SVG panel pattern, build system all verified against POC and official docs
- Pitfalls: HIGH -- Multiple sources confirm common issues; POC experience validates workarounds
- Layout: MEDIUM-HIGH -- Component sizes verified from actual SVGs; layout spacing calculated but not yet physically tested in VCV Rack

**Research date:** 2026-02-25
**Valid until:** 2026-03-25 (VCV Rack SDK is stable; major version changes are infrequent)
