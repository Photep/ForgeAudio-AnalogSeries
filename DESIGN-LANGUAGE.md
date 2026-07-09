# Forge Audio — Design Language & Panel Design Prompt

## Overview

This document captures the complete design language developed for the Forge Audio Analog Series modules, established during the design of the Analog LFO "Forge Noir" panel. Use this as a reference prompt when designing future modules to maintain visual consistency across the product line.

---

## Brand Identity

### Name Treatment
- **"FORGE"** and **"AUDIO"** are rendered as two separate elements
- FORGE is left-aligned to a margin ~36px from the left panel edge (at 5x scale)
- AUDIO is right-aligned to ~18px from the right panel edge
- Font: **FoundationLogo** (fallback: Bebas Neue)
- Size: 20px at 5x scale
- Letter-spacing: 5px
- Color: `#e85d26` (forge ember)
- Text-shadow: `0 0 12px rgba(232,93,38,0.25)`

### Forge Rune Glyph
A custom SVG rune centered between FORGE and AUDIO, vertically centered with the text. Elements:
- **Outer diamond** — angular outline, stroke `#e85d26`, opacity 0.6
- **Inner diamond** — hot fill `#e85d26`, opacity 0.18
- **Vertical axis** — thin line through center with forked arrow tips at top and bottom
- **Horizontal extensions** — short lines from diamond midpoints
- **Diagonal notch marks** — on diamond edges, like forge brand impressions
- **Three-layer glowing center** — ember outer (`#e85d26` 0.15), molten gold mid (`#f0a030` 0.85), white-hot core (`#ffe0a0` 0.9)
- **Radiating power rings** — 4 concentric circles behind the rune, fading outward:
  - r=16, stroke-width 3, opacity 0.14
  - r=20, stroke-width 2, opacity 0.09
  - r=25, stroke-width 1.5, opacity 0.06
  - r=31, stroke-width 1 (gold `#d4a017`), opacity 0.035
- Entire rune group has Gaussian blur glow filter (stdDeviation 1.8)

### Module Name
- "ANALOG LFO" (or module name) sits below the brand, centered
- Font: **Bebas Neue**, 20px, letter-spacing 7px
- Color: `#e8e4e0` (warm white)
- Has `background: #0c0c0c` with horizontal padding to "cut" through the decorative line behind it, creating a symmetrical break

### Decorative Line
- A thin horizontal ember gradient line runs behind the module name at its vertical center
- `linear-gradient(90deg, transparent, rgba(232,93,38,0.25) 30%, rgba(232,93,38,0.25) 70%, transparent)`
- The module name text's background creates the symmetrical cut

### Header Slashes
- 3 small diagonal slash marks on each side of the header, flanking the brand text
- Left side: angle outward to the left (`rotate(-25deg)`)
- Right side: angle outward to the right (`rotate(25deg)`)
- Symmetrically positioned about the panel center
- `linear-gradient(180deg, rgba(232,93,38,0.5), transparent)`

---

## Color Palette

| Name | Hex | Usage |
|------|-----|-------|
| Panel Black | `#0e0e11` → `#070708` | Panel background (subtle top→bottom gradient, not flat) |
| Panel Border | `#202024` | Panel edge, subtle borders |
| Forge Ember | `#e85d26` | Primary accent — brand, indicators, accents, glows (single value across panel, components, and code) |
| Molten Gold | `#d4a017` | Secondary accent — accent-rail center, display readouts, warm details |
| Hot Gold | `#f0a030` | Tertiary — phase dots, rune center, highlights |
| White Hot | `#ffe0a0` | Rune core, peak brightness moments |
| Warm White | `#e8e4e0` | Module name, hero labels (MORPH) |
| Label Light | `#c0bbb5` | Primary control labels (CHARACTER, DRIFT) |
| Label Mid | `#888` | Secondary control labels (RATE, PHASE), CV labels |
| Label Dim | `#777` / `#666` | Tertiary labels, BPM readouts |
| Display BG | `#030303` | Display area background |

---

## Typography Hierarchy

| Level | Font | Size | Weight | Letter-spacing | Color | Usage |
|-------|------|------|--------|---------------|-------|-------|
| Brand | FoundationLogo | 20px | — | 5px | `#e85d26` | "FORGE" / "AUDIO" |
| Module Name | Bebas Neue | 20px | — | 7px | `#e8e4e0` | "ANALOG LFO" |
| Hero Label | Bebas Neue | 16px | — | 5px | `#f0ece8` + subtle ember shadow | MORPH |
| Primary Label | Chakra Petch | 9px | 600 | 2.5px | `#c0bbb5` | CHARACTER, DRIFT |
| Secondary Label | Chakra Petch | 8px | 500 | 2px | `#888` | RATE, PHASE |
| CV Label | JetBrains Mono | 7px | 500 | 1.5px | `#777` | MORPH, CHAR, DRIFT, FM, PHASE |
| I/O Label | Chakra Petch | 8px | 600 | 2px | `#999` | CLK, RST |
| Output Label | Bebas Neue | 14px | — | 4px | `#e85d26` + ember shadow | OUTPUT |
| Display Pills | JetBrains Mono | 5-8px | 400-600 | — | varies | SYNC, Hz, BPM, Swing |

> **Shipping reality:** the fonts above describe the *design intent* / mockup letterforms. In the shipping panel, all static text (brand, module name, labels) is pre-converted to SVG `<path>` outlines, so no font file is needed at runtime. The only bundled font is **JetBrains Mono** (`res/fonts/`, OFL-licensed), used for the live display readouts. FoundationLogo and the other mockup fonts are **not** redistributed.

### Label Spacing Rule
- **10px gap** between label bottom and component top edge (consistent across all knob sizes)
- **10px gap** between component bottom and next label top (zone-to-zone)
- **14px gap** between display bottom and first knob label

---

## Panel Structure

### Dimensions
- **18HP** (91.44mm wide × 128.5mm tall)
- Panel center axis: x = 45.72mm
- Authoritative mm coordinates for every component live in [res/PANEL-SPEC.md](res/PANEL-SPEC.md); this document covers the visual language, that one covers exact placement.

> Note: the px values elsewhere in this document date from the original 5x HTML mockup (a 14HP-width proportioned canvas) and are kept as *relative* rendering recipes. Treat the mm figures here and in PANEL-SPEC.md as the shipping truth.

### Accent Bars
- Thin ember/gold rails at top and bottom edges
- `linear-gradient(90deg, #1a0800, #e85d26 22%, #d4a017 50%, #e85d26 78%, #1a0800)`

### Hex Bolts
- 16×16px hexagonal bolts at all four corners
- Outer hex: `linear-gradient(160deg, #333, #1a1a1a)` with polygon clip-path
- Inner hex socket: `#0a0a0a`, 7×7px

### Section Dividers
- `linear-gradient(90deg, transparent, rgba(232,93,38,0.3) 20%, rgba(218,165,32,0.2) 50%, rgba(232,93,38,0.3) 80%, transparent)`
- Inset 28px from each side

### Panel Texture
- Subtle radial gradient centered at ~35% height: `rgba(232,93,38,0.02)` fading to transparent
- Very faint repeating conic gradient for noise: `rgba(255,255,255,0.003)` at 6px scale

---

## Component Design

### Knobs — Premium Multi-Layer Rendering

**Body gradient (3 layers):**
1. Specular highlight: `radial-gradient(ellipse at 30% 22%, rgba(255,255,255,0.14), transparent 45%)`
2. Edge vignette: `radial-gradient(circle, transparent 48%, rgba(0,0,0,0.35) 100%)`
3. Main curvature: `radial-gradient(ellipse at 38% 28%, #4a4a4a, #2e2e2e 25%, #1a1a1a 50%, #101010 75%, #0a0a0a 100%)`

**Box-shadow (depth + metallic ring):**
- Drop shadow: `0 6px 20px rgba(0,0,0,0.75)`, `0 2px 4px rgba(0,0,0,0.5)`
- Double-band metallic ring: `0 0 0 1.5px #151515`, `0 0 0 3px #4a4a4a`, `0 0 0 4px #333`, `0 0 0 5px #1a1a1a`
- Inner highlights: `inset 0 2px 4px rgba(255,255,255,0.07)`, `inset 0 -2px 6px rgba(0,0,0,0.5)`

**Center cap:** Machined indent — `radial-gradient(circle at 44% 38%, #282828, #080808)` with subtle ring shadow

**Surface texture:** Concentric machined grooves via nested `box-shadow` on `::after`

**Indicator line:**
- Anchored at knob center: `bottom: 50%; transform-origin: bottom center`
- Gradient: `linear-gradient(180deg, #f08040, #e85d26 60%, #c04020)`
- Glow: `0 0 8px rgba(232,93,38,0.6), 0 0 3px rgba(232,93,38,0.9)`

**Size hierarchy:**
| Role | Class | Diameter | Indicator height | Special |
|------|-------|----------|-----------------|---------|
| Hero (MORPH) | ForgeKnobHero | 16.38mm | 32% | Knurled outer ring, largest metallic ring |
| Control (CHAR, DRIFT, RATE, PHASE) | ForgeKnobSecondary | 11.99mm | 28% | Standard metallic ring |

> The four control knobs share one class and size — there is no separate "utility" knob. RATE and PHASE are the same `ForgeKnobSecondary` as CHARACTER and DRIFT.

### Jacks — PJ301M-Style Metallic

**Concentric ring system (radial-gradient):**
- Center void: `#020202` at 15%
- Inner ring: peaks at `#ccc` around 29%
- Outer ring: peaks at `#ccc` around 47%
- Flange taper: `#555` → `#1a1a1a` from 60% to 100%

**Specular highlight:** `radial-gradient(ellipse at 28% 20%, rgba(255,255,255,0.18), transparent 35%)` on `::before`

**Center hole:** 28% diameter, `radial-gradient(circle at 40% 35%, #0a0a0a, #000)` with deep `inset` shadow

**Sizes:** Small (36px) for inputs, Large (42px) for output

**Output accent:** Ember ring — `0 0 0 3px rgba(232,93,38,0.5), 0 0 18px rgba(232,93,38,0.2)`

### Trimpots

- 24px diameter
- Body: specular highlight + radial gradient (`#6a6a6a` → `#1a1a1a`)
- Metallic outer ring via triple box-shadow bands
- **Directional indicator** (not through-slot): `bottom: 50%; transform-origin: bottom center` — same system as knobs
- Indicator: `linear-gradient(180deg, #ccc, #888)`, 2px wide, 30% height

### CV Connecting Lines
- 1px wide, ember gradient fading downward
- `linear-gradient(180deg, rgba(232,93,38,0.25), rgba(232,93,38,0.08))`
- Connect each trimpot to its CV jack below

---

## Display Design

### Container
- Background: `#030303`
- Border: `1.5px solid rgba(232,93,38,0.3)` with border-radius 4px
- Animated glow: `box-shadow` pulses between 0.08 and 0.18 opacity over 5 seconds
- CRT scanline overlay: `repeating-linear-gradient` with 2px transparent / 2px dark bands
- Corner bracket accents: 8×8px L-shaped ember borders at each corner (1.5px, opacity 0.4)

### Three-Column Layout
- **Left column (x: 16-55):** Ratio pill, Hz readout, Swing pill + label
- **Center column (x: 64-256):** Single-cycle waveform with phase dot
- **Right column (x: 262-302):** SYNC badge, CLK/LFO BPM stack

### Waveform Rendering
- Three-layer glow: wide diffuse (stroke-width 7, opacity 0.06), medium (3.5, 0.15), sharp (2, 0.85)
- Dashed zero-crossing reference line
- Phase dot: three concentric circles (glow 0.08, mid 0.22, core `#f0a030` 0.95)
- Comet trail: thin curved path behind the dot (opacity 0.2)

### Pill Styling
- Background: `rgba(232,93,38,0.1-0.12)` with ember stroke
- Border-radius: 2-2.5px
- Font: JetBrains Mono, 5-8px
- Inset from display edges to clear corner bracket accents

---

## Forge Emblem (Background Pattern)

A symmetrical atmospheric SVG behind the main knob section, suggesting smoldering embers and molten metal.

### Construction Method
- Define left half in `<g id="forge-half">`, render once, then mirror via `<use href="#forge-half" transform="translate(panelWidth, 0) scale(-1, 1)">` for **guaranteed perfect symmetry**

### Elements (per side)
1. **Molten metal streams** — 3 flowing curves from center outward/downward at varying widths (16px/7px/2.5px) and opacities (0.055/0.045/0.1)
2. **Forge marks flanking MORPH** — 2 chevron pairs pointing inward, 3 curved heat arc lines, 2 horizontal dash marks, 1 diamond accent
3. **Ember particles** — ~11 small circles (r: 0.7-2px) at varying opacities (0.08-0.18) in ember/gold/hot-gold colors

### Centered Elements (inherently symmetric)
1. **Core forge glow** — radial gradient ellipse behind MORPH (0.2 center → 0.08 mid → 0 edge)
2. **Secondary warmth** — larger gold radial gradient
3. **Central heat column** — vertical line, 28px wide, opacity 0.03
4. **Heat shimmer waves** — 3 horizontal sinusoidal paths at different depths

---

## Layout Rules

> Coordinates below are the shipping 18HP layout in mm (see [res/PANEL-SPEC.md](res/PANEL-SPEC.md) §4 for the full table). Panel center axis: **x = 45.72mm**.

### Knob Arrangement: hero + 4-across control row
```
            [MORPH]                     ← Hero, centered (45.72, 61.00)
  [CHAR]  [DRIFT]  [RATE]  [PHASE]      ← Control row, single line at y = 87.00
```
- MORPH hero centered on the panel axis, below the display
- Four equal control knobs in one row at y=87.00, x = 18.00 / 36.24 / 54.48 / 72.72 (even 18.24mm spacing)

### Bottom Grid: trimpot row over jack row
- **Trimpot row** (y=108.50): 5 CV attenuverters at x = 7.70 / 18.56 / 29.43 / 40.29 / 51.15
- **Jack row** (y=119.50): a single 8-across row on the same ~10.86mm rhythm
  - Columns 1–5 are the CV inputs directly below their trimpots (same x), linked by ember connecting lines
  - Columns 6–8 continue the rhythm: CLK (62.01), RST (72.88), OUTPUT (83.74)
- OUTPUT sits far right and retains its ember accent ring

### Symmetry Rule
The **decorative and upper elements are mirrored about the center axis (x = 45.72mm)** — accent rails, header/brand, forge emblem, display well, and the MORPH hero. Use SVG mirroring where possible to guarantee this. The **functional lower grid** (trimpots + jacks + CLK/RST/OUTPUT) is intentionally *not* mirrored: it reads as a uniform left-to-right rhythm anchored at the left margin, with OUTPUT deliberately at the right end.

---

## Design Process Notes

### What Worked
- Starting with 3 wildly different directions (Industrial/Noir/Minimal) to find the aesthetic
- Iterating on spacing with a consistent grid rather than ad-hoc pixel tweaking
- Using SVG `<use>` mirroring for guaranteed symmetry
- The `bottom: 50%; transform-origin: bottom center` technique for knob indicators
- Three-column display layout preventing pill-waveform overlap
- Building the forge emblem as atmospheric background, not foreground decoration
- Splitting brand into two elements for flexible left/right alignment

### Design Principles
1. **Symmetry where it decorates** — decorative and upper elements (rails, brand, emblem, display, hero) mirror perfectly about center; the functional lower grid follows a uniform left-to-right rhythm instead
2. **Consistent spacing** — uniform gaps between labels and components throughout
3. **Component hierarchy** — size communicates importance (MORPH hero > the four control knobs > trimpots)
4. **Ember as identity** — `#e85d26` is the signature color, used for all accents, never for backgrounds
5. **Subtlety in layers** — atmospheric effects (forge emblem, panel texture) should enhance without competing
6. **Typography does work** — clear font hierarchy makes the panel readable at a glance
