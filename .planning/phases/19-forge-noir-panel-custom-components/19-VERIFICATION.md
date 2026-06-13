---
phase: 19-forge-noir-panel-custom-components
verified: 2026-04-01T08:34:46Z
status: passed
score: 5/5 must-haves verified (in-Rack visual gate performed in plan 19-04 — see 19-04-SUMMARY.md; status flipped at 2026-06-13 milestone audit)
human_verification:
  - test: "Confirm label overlap with knobs is cosmetically acceptable or flag for Phase 20 refinement"
    expected: "MORPH, CHARACTER, DRIFT, RATE, PHASE labels are legible without obscuring control interaction"
    why_human: "User already observed overlap in VCV Rack; verification of acceptability vs. blocking is a judgment call"
  - test: "Confirm display box positioning relative to MORPH knob area"
    expected: "Display does not visually intrude on MORPH section in a way that degrades usability"
    why_human: "Visual spatial judgment — cannot be determined from code coordinates alone"
  - test: "Confirm vertical section spacing feels correct at 14HP panel scale"
    expected: "CHARACTER/DRIFT, RATE/PHASE, CV/jack rows have legible breathing room"
    why_human: "Proportional spacing perception requires viewing the rendered module"
---

# Phase 19: Forge Noir Panel + Custom Components Verification Report

**Phase Goal:** The module appears in VCV Rack with the 14HP Forge Noir visual identity -- near-black panel, custom machined-metal knobs, scalloped trimpots, accent-ring jacks, forge emblem, and path-rendered brand typography
**Verified:** 2026-04-01T08:34:46Z
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths (from ROADMAP.md Success Criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Module loads in VCV Rack as 14HP panel with Forge Noir color scheme (near-black background, ember orange accent bars) — no black rectangles or missing elements | ✓ VERIFIED | `res/AnalogLFO.svg` declares `width="71.12mm"`, `fill="#0c0c0c"` background, ember `#e85d26` accent gradients; `make install` confirmed clean in 19-04-SUMMARY |
| 2 | All controls (5 main knobs, 5 CV trimpots, 5 CV jacks, CLK, RST, output jack) are interactive and positioned per Forge Noir mockup | ✓ VERIFIED | All 18 `addParam`/`addInput`/`addOutput` calls present at exact spec mm coordinates; user confirmed interactivity in VCV Rack |
| 3 | Three distinct knob sizes visible (hero MORPH, secondary CHARACTER/DRIFT, utility RATE/PHASE) with machined-metal appearance; trimpots are visually distinct scalloped attenuverters | ✓ VERIFIED | Three SVG width values confirmed: `16.38mm` (hero), `11.99mm` (secondary), `9.19mm` (utility); `ForgeTrimpot.svg` has bright metallic radialGradient + 8 scallop notches |
| 4 | Jacks show two distinct sizes with ember accent rings on output jack only | ✓ VERIFIED | `ForgeJackInput.svg` = `7.19mm`, `ForgeJackOutput.svg` = `8.39mm`; output contains `#e85d26` stroke circles; input does not |
| 5 | Forge emblem visible as subtle background element; brand text renders correctly as SVG paths | ✓ VERIFIED | Emblem present via `radialGradient id="emblem-core/emblem-gold"` ellipses in panel SVG; 110 `<path>` elements, zero `<text>` elements confirmed |

**Score:** 5/5 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `res/components/ForgeKnobHero.svg` | 16.38mm hero knob rotating layer | ✓ VERIFIED | `width="16.38mm"` confirmed |
| `res/components/ForgeKnobHero_bg.svg` | Hero knob static background with radialGradient | ✓ VERIFIED | `radialGradient id="hero-bg-shadow"` confirmed |
| `res/components/ForgeKnobSecondary.svg` | 11.99mm secondary knob | ✓ VERIFIED | `width="11.99mm"` confirmed |
| `res/components/ForgeKnobSecondary_bg.svg` | Secondary knob background | ✓ VERIFIED | `radialGradient id="secondary-bg-shadow"` confirmed |
| `res/components/ForgeKnobUtility.svg` | 9.19mm utility knob | ✓ VERIFIED | `width="9.19mm"` confirmed |
| `res/components/ForgeKnobUtility_bg.svg` | Utility knob background | ✓ VERIFIED | `radialGradient id="utility-bg-shadow"` confirmed |
| `res/components/ForgeTrimpot.svg` | 3.60mm bright trimpot with scalloped edge | ✓ VERIFIED | `width="3.60mm"`, bright metallic gradient + 8 scallop notches at 45-degree intervals |
| `res/components/ForgeTrimpot_bg.svg` | Trimpot shadow ring | ✓ VERIFIED | `#000000` shadow circle confirmed |
| `res/components/ForgeJackInput.svg` | 7.19mm input jack — no accent ring | ✓ VERIFIED | `width="7.19mm"`, no `#e85d26` present |
| `res/components/ForgeJackOutput.svg` | 8.39mm output jack with ember accent ring | ✓ VERIFIED | `width="8.39mm"`, three `#e85d26` stroke circles confirmed |
| `res/components/ForgeHexBolt.svg` | Hex bolt screw | ✓ VERIFIED | Two `<polygon>` elements confirmed |
| `res/AnalogLFO.svg` | 14HP Forge Noir panel — all decorative elements, emblem, typography paths | ✓ VERIFIED | 537 lines, `width="71.12mm"`, 110 path elements, 0 text elements, emblem ellipses, ember accent gradients |
| `src/AnalogLFO.cpp` | Custom widget struct definitions + updated AnalogLFOWidget constructor | ✓ VERIFIED | All 7 structs defined (lines 1343–1412), all 18 controls wired with custom types, shadow disabled on all |
| `plugin.json` | minRackVersion 2.6.0 | ✓ VERIFIED | `"minRackVersion": "2.6.0"` at line 5 |
| `res/PANEL-SPEC.md` | Updated 14HP Forge Noir panel spec | ✓ VERIFIED | `71.12mm` present at line 13 |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `src/AnalogLFO.cpp` | `res/AnalogLFO.svg` | `setPanel(createPanel(asset::plugin(..., "res/AnalogLFO.svg")))` | ✓ WIRED | Line 1418 confirmed |
| `src/AnalogLFO.cpp` | `res/components/ForgeKnobHero.svg` | `setSvg(Svg::load(asset::plugin(..., "res/components/ForgeKnobHero.svg")))` | ✓ WIRED | Line 1351 confirmed |
| `src/AnalogLFO.cpp` | `res/components/ForgeKnobHero_bg.svg` | `bg->setSvg(Svg::load(...))` | ✓ WIRED | Line 1352 confirmed |
| `src/AnalogLFO.cpp` | `res/components/ForgeKnobSecondary.svg` | `setSvg` in struct | ✓ WIRED | Line 1364 confirmed |
| `src/AnalogLFO.cpp` | `res/components/ForgeKnobUtility.svg` | `setSvg` in struct | ✓ WIRED | Line 1377 confirmed |
| `src/AnalogLFO.cpp` | `res/components/ForgeTrimpot.svg` | `setSvg` in struct | ✓ WIRED | Line 1390 confirmed |
| `src/AnalogLFO.cpp` | `res/components/ForgeJackInput.svg` | `setSvg` in struct | ✓ WIRED | Line 1398 confirmed |
| `src/AnalogLFO.cpp` | `res/components/ForgeJackOutput.svg` | `setSvg` in struct | ✓ WIRED | Line 1405 confirmed |
| `src/AnalogLFO.cpp` | `res/components/ForgeHexBolt.svg` | `setSvg` in `ForgeHexBolt` struct, 4 `createWidget<ForgeHexBolt>` calls | ✓ WIRED | Lines 1411, 1421–1424 confirmed |
| `ForgeKnobHero` widget | `AnalogLFOWidget` constructor | `addParam(createParamCentered<ForgeKnobHero>(...))` | ✓ WIRED | Line 1436 confirmed |
| `ForgeJackOutput` widget | `AnalogLFOWidget` constructor | `addOutput(createOutputCentered<ForgeJackOutput>(...))` | ✓ WIRED | Line 1476 confirmed |

### Data-Flow Trace (Level 4)

Not applicable — this phase produces SVG artwork and C++ widget wiring. There are no React/dynamic data rendering patterns. The WaveformDisplay widget at `mm2px(Vec(3.60f, 13.19f))` was repositioned from the old 12HP layout; its data flow (`module` pointer assignment at line 1429) was not introduced in this phase and was already verified in prior phases.

### Behavioral Spot-Checks

Step 7b: SKIPPED — behavioral verification was performed by the user directly in VCV Rack (the target runtime environment). The module cannot be exercised without launching VCV Rack. The 19-04-SUMMARY confirms `make install` exited 0 and the user approved the checkpoint.

### Requirements Coverage

| Requirement | Source Plan(s) | Description | Status | Evidence |
|-------------|---------------|-------------|--------|----------|
| PANEL-01 | 19-02, 19-04 | 14HP Forge Noir SVG panel with near-black background, ember accent bars, hex bolt screws | ✓ SATISFIED | `res/AnalogLFO.svg` at 71.12mm with `#0c0c0c` background, ember accent gradients, 4 `ForgeHexBolt` screws |
| PANEL-02 | 19-03, 19-04 | All controls repositioned per Forge Noir mockup (5 main knobs, 5 CV trimpots+jacks, CLK/RST/OUTPUT) | ✓ SATISFIED | All 18 controls at exact spec mm coordinates in `src/AnalogLFO.cpp`; REQUIREMENTS.md checkbox is stale (not yet ticked) but implementation is complete and user-verified |
| PANEL-03 | 19-01, 19-03, 19-04 | Custom SVG knob components (3 sizes: hero/secondary/utility) with machined metal appearance | ✓ SATISFIED | 6 knob SVG files confirmed (3 rotating + 3 bg); all 3 sizes used in constructor |
| PANEL-04 | 19-01, 19-03, 19-04 | Custom SVG trimpot components (bright scalloped attenuverters) | ✓ SATISFIED | `ForgeTrimpot.svg` + `ForgeTrimpot_bg.svg` confirmed; bright metallic gradient, 8 scallop notches |
| PANEL-05 | 19-01, 19-03, 19-04 | Custom SVG jack components (2 sizes: standard/output with ember accent ring) | ✓ SATISFIED | `ForgeJackInput.svg` (7.19mm, no accent) and `ForgeJackOutput.svg` (8.39mm, `#e85d26` ring) confirmed |
| PANEL-06 | 19-02, 19-04 | Forge emblem background element | ✓ SATISFIED | `emblem-core` and `emblem-gold` radialGradient ellipses present in panel SVG layer 17 |
| PANEL-07 | 19-02, 19-04 | Brand typography rendered as SVG paths (Forge Audio header, Analog LFO name) | ✓ SATISFIED | 110 `<path>` elements in panel SVG, 0 `<text>` elements |

**Note on PANEL-02 checkbox:** REQUIREMENTS.md shows `[ ] PANEL-02` (unchecked) and `Pending` in the status table. The implementation is demonstrably complete — all 18 controls are at Forge Noir spec coordinates in `src/AnalogLFO.cpp` and the user verified them in VCV Rack. The checkbox appears to have been left stale during the documentation merge. This should be updated to checked/Complete.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `res/AnalogLFO.svg` | 298 | Comment reads `LAYER 7: DISPLAY PLACEHOLDER` | ℹ️ Info | Not a stub — the comment labels a decorative dark background rect (`fill="#030303"`) that the `WaveformDisplay` C++ widget draws over. The WaveformDisplay is wired with real module data. No impact. |

No blocking stubs found. No `TODO`/`FIXME` markers in SVG or C++ files. No stock VCV widgets remaining (`RoundBigBlackKnob`, `Trimpot`, `PJ301MPort`, `ScrewSilver` — all replaced). `shadow->opacity = 0.0` confirmed on all 6 interactive widget structs (knobs, trimpots, jacks).

### Human Verification Required

#### 1. Label Overlap with Knobs

**Test:** In VCV Rack, visually assess whether MORPH, CHARACTER, DRIFT, RATE, and PHASE label text is sufficiently legible alongside the knobs
**Expected:** Labels are readable; their partial overlap with knob boundaries does not confuse the control's identity or prevent interaction
**Why human:** User observed the overlap in the initial checkpoint. The question is whether this is a Phase 20 refinement item or an acceptability blocker. Code positions are locked by spec coordinates — only visual judgment can resolve this.

#### 2. Display Box vs. MORPH Area Encroachment

**Test:** In VCV Rack, confirm the waveform display rectangle (at y=13.19mm, height=17.98mm, so bottom edge at y=31.17mm) does not visually intrude on the MORPH knob region (centered at y=47.35mm, radius ~8.19mm, so top edge ~39.16mm)
**Expected:** Approximately 8mm of clear panel between display bottom and MORPH knob top — acceptable spacing
**Why human:** The coordinates show ~8mm gap, which should be adequate, but the user observed the display "encroaching" on the MORPH area. Whether this is a rendering perception issue vs. an actual layout problem requires viewing in context.

#### 3. Vertical Section Spacing

**Test:** Evaluate the visual rhythm between the CHARACTER/DRIFT section (y=67.32mm), RATE/PHASE section (y=83.51mm), and CV/jack rows (y=95.89mm, 103.08mm)
**Expected:** Clear visual separation between sections; sections do not feel compressed
**Why human:** 16.19mm gap between CHARACTER/DRIFT and RATE/PHASE rows; 12.38mm between RATE/PHASE and CV trimpots. Whether this reads as comfortable spacing is a judgment call requiring the rendered module.

### Gaps Summary

No functional gaps. All 5 observable truths are verified. All 15 required artifacts exist, are substantive, and are wired. All 7 requirement IDs are satisfied by code evidence.

The three human verification items above are cosmetic refinement concerns that the user already flagged in the VCV Rack checkpoint. They are recorded here so Phase 20 planning can incorporate label alignment, display positioning, and section spacing adjustments if the user decides to refine them.

The REQUIREMENTS.md PANEL-02 checkbox is a documentation staleness issue only — the implementation is complete.

---

_Verified: 2026-04-01T08:34:46Z_
_Verifier: Claude (gsd-verifier)_
