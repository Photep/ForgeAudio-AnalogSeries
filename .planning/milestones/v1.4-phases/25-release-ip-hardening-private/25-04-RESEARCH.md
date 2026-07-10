# Phase 25 Plan 04: Wordmark OFL Re-Export — Research

**Researched:** 2026-07-01
**Domain:** Font text→outline conversion, splicing baked glyph paths into a hand-authored SVG, visual-parity verification (macOS, no GUI font tooling)
**Confidence:** HIGH (pipeline, fonts, and target geometry all verified live on this machine; font *choice* is a human aesthetic decision — flagged)
**Requirement:** IP-03 (wordmark provenance provable by construction → confirmed-OFL → unblocks 25-03)

## Summary

The shipped `res/AnalogLFO.svg` bakes every piece of lettering as raw font-em-unit `<path>` geometry wrapped in `transform="translate(Tx,Ty) scale(s,-s)"` — the classic **fontTools `SVGPathPen` on a TTF** signature (quadratic `Q` curves preserved as-is, y-up flipped by the negative scale). There are **18 such baked-text runs**. The three that form the brand wordmark are located **deterministically** by their transform signature + fill:

| Run | SVG line | String | Fill | Role |
|-----|----------|--------|------|------|
| 1 | 28 | `FORGE` | `#e8612a` (ember) | header, left |
| 2 | 29 | `AUDIO` | `#e8612a` (ember) | header, right |
| 3 | 35 | `ANALOG LFO` | `#ece8e2` (off-white) | module title |

The **forge-rune** (SVG lines 30–34: concentric circles + a diamond `<polygon>` at `cx=45.72,cy=6.30`) sits between FORGE and AUDIO and is original art — **PRESERVE byte-for-byte**. The remaining 15 baked runs are functional labels (MORPH, knob/jack labels, footer) — out of scope for this plan but share the same provenance question (see Open Questions).

**Forensic finding (verified):** the existing wordmark is a **round, heavy geometric sans** — its `O` is drawn with round `Q` curves, its `F` has width/cap-height = **0.626** and thick stems (~0.23 em). This matches **neither** confirmed-OFL candidate: Bebas Neue is far too condensed (F w/cap = **0.416**) and Chakra Petch's `O` is **chamfered (14 straight segments, no curves)** with lighter strokes. The shipped geometry is therefore almost certainly drawn from the **trial FoundationLogo** face — exactly the IP-03 risk. There is no pixel-parity OFL substitute; **the wordmark's visual character will change**, and which OFL face to adopt is a human brand decision.

**Primary recommendation:** Use a throwaway Python **venv + `fontTools` 4.63.0** (verified working) to set FORGE / AUDIO / ANALOG LFO in the chosen OFL TTF with `SVGPathPen`, reproducing the *exact* existing `translate()/scale(-)` structure (already proven to render in VCV's nanosvg, since it is how the panel ships today). Generate **both** a Bebas Neue and a Chakra Petch version, render both with the Playwright MCP browser screenshot, and let the operator pick. IP-03 is satisfied by **either** confirmed-OFL choice — visual parity is a softer, human-accepted goal, not the gating requirement.

## Plan Constraints (from 25-04-PLAN.md must_haves)

- Outlines must come from confirmed-OFL Bebas Neue and/or Chakra Petch (Google Fonts / upstream OFL).
- Record source font **family + version + OFL URL** in the SUMMARY (provable by construction).
- SVG stays **path-only**: zero `<text>/<tspan>/<glyph>/<font>/@font-face/base64` and zero `<use href>`. **No font program ships** — only static outlines.
- Visual parity (best-effort) in position/size/fill; **forge-rune preserved**; non-wordmark art byte-untouched.
- `viewBox 0 0 91.44 128.5` unchanged; SVG still well-formed.
- Human visually accepts AND provenance recorded `confirmed-OFL` → IP-03 closed → 25-03 unblocked.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| IP-03 | Wordmark provenance provable as OFL-by-construction | Verified pipeline (fontTools SVGPathPen) + verified OFL sources (Bebas Neue / Chakra Petch, OFL 1.1) + exact located paths + exact mm target boxes. Choosing *any* confirmed-OFL face and re-exporting these 3 runs makes provenance provable regardless of visual outcome. |

## Recommended Approach (the one tool chain)

**Deterministic, scriptable, zero system pollution, all-verified-on-this-machine:**

```bash
# 1. Throwaway venv (do NOT --break-system-packages; do NOT commit anything here)
python3 -m venv /tmp/wordmark-venv
/tmp/wordmark-venv/bin/pip install fonttools svgpathtools     # 4.63.0 + svgpathtools, verified installable

# 2. Fetch OFL fonts (scratch only — NEVER commit the .ttf; only outlines ship)
cd /tmp && mkdir -p wm-fonts && cd wm-fonts
curl -sL -o BebasNeue-Regular.ttf   https://github.com/google/fonts/raw/main/ofl/bebasneue/BebasNeue-Regular.ttf
curl -sL -o ChakraPetch-Regular.ttf https://github.com/google/fonts/raw/main/ofl/chakrapetch/ChakraPetch-Regular.ttf
curl -sL -o ChakraPetch-Bold.ttf    https://github.com/google/fonts/raw/main/ofl/chakrapetch/ChakraPetch-Bold.ttf
curl -sL -o bebas-OFL.txt           https://github.com/google/fonts/raw/main/ofl/bebasneue/OFL.txt
curl -sL -o chakra-OFL.txt          https://github.com/google/fonts/raw/main/ofl/chakrapetch/OFL.txt
```

**Why this chain (not the alternatives):**

| Option | Verdict | Reason |
|--------|---------|--------|
| **fontTools `SVGPathPen`** ✅ PRIMARY | Use it | Pure-Python, deterministic, **emits the exact `<path d>` quadratic-`Q` form the file already uses** (verified: `M41 700H332…Q377 618…`). Reproduces the original pipeline's structure → guaranteed nanosvg-compatible. |
| `hb-view --output-format=svg` (already installed) | Fallback only | HarfBuzz 9 emits an SVG *document* with the shaped run, but you must then strip its wrapper, re-extract `<path>`, and re-fit coords — more post-processing than SVGPathPen, and its layout transform differs from the file's convention. |
| `picosvg` / `nanoemoji` | No | Not installed; aimed at color-font/emoji pipelines, overkill. |
| npm `text-to-svg` / `opentype.js` | No | Adds a Node dependency for what fontTools does natively; opentype.js cubic-izes TTF quadratics, *changing* the curve representation vs. the existing file. |
| Inkscape `--export-text-to-path` | Unavailable | Inkscape ABSENT on this machine and not trivially installable. |

**Locating / verifying paths:** `svgpathtools` (pure-Python, verified) for bbox math — **no rasterizer needed to locate the runs** (they are already identified below).
**Rendering for human acceptance:** the **Playwright MCP** (browser screenshot of `file://…/AnalogLFO.svg`) — already available in this repo (`.playwright-mcp/`), no install, faithful gradient/transform rendering. (`inkscape`, `rsvg-convert`, `resvg`, `cairosvg`, `imagemagick` are all ABSENT; `cargo` ABSENT so resvg cannot be built.)

## Wordmark Path Map (VERIFIED — exact targets)

All baked-text runs share the signature `transform="translate(Tx,Ty) scale(s,-s)"` over font-unit coords. Selector to enumerate them: `grep -nE 'scale\([0-9.]+,-[0-9.]+\)' res/AnalogLFO.svg` → 18 runs. The wordmark is exactly these three:

| String | SVG line | Fill | Baseline Ty (mm) | Current scale s | mm bbox (x0,x1 / y0,y1) | Width (mm) | Cap-H (mm) | Center-x |
|--------|----------|------|------------------|-----------------|--------------------------|-----------|-----------|----------|
| FORGE | 28 | `#e8612a` | 7.600 | 0.001758 | x[9.797, 27.196] y[4.928, 7.651] | 17.399 | 2.723 | 18.50 |
| AUDIO | 29 | `#e8612a` | 7.600 | 0.001758 | x[64.573, 81.780] y[4.928, 7.651] | 17.207 | 2.723 | 73.18 |
| ANALOG LFO | 35 | `#ece8e2` | 14.600 | 0.002148 | x[20.667, 70.584] y[11.335, 14.662] | 49.917 | 3.327 | 45.63 |

[VERIFIED: svgpathtools bbox of the live path data + transform, 2026-07-01]

**Layout invariants to preserve:**
- The wordmark is **symmetric about panel center x=45.72**: FORGE center 18.50 and AUDIO center 73.18 mirror it; ANALOG LFO is centered (45.63 ≈ 45.72).
- **Forge-rune = SVG lines 30–34** (`<circle cx=45.72 cy=6.30 …>` ×4 + `<polygon points="45.72,3.90 …">`), occupying the gap x≈[27.2, 64.6] between FORGE and AUDIO. **DO NOT TOUCH.**
- Header cap-H = 2.723 mm (FORGE/AUDIO); title cap-H = 3.327 mm (ANALOG LFO is the larger line).

**Out of scope but same provenance question** (15 runs, lines 63/76/89/102/115/119/128/137/146/155/165/166/169/172/177): MORPH knob label, secondary knob labels, jack labels, footer. The plan scopes only the 3 wordmark runs; note these to the operator (see Open Questions Q3).

## Font Sourcing (VERIFIED)

| Font | OFL source (raw URL) | Family / Author | License | UPM | Cap-H (units) | Weights available |
|------|----------------------|-----------------|---------|-----|---------------|-------------------|
| **Bebas Neue Regular** | `github.com/google/fonts/raw/main/ofl/bebasneue/BebasNeue-Regular.ttf` | Bebas Neue, Dharma Type / "The Bebas Neue Project Authors", Copyright 2019 | **SIL OFL 1.1** (`…/ofl/bebasneue/OFL.txt`) | 1000 | 700 | Regular only on GF (Fontfabric sells more) |
| **Chakra Petch** | `github.com/google/fonts/raw/main/ofl/chakrapetch/ChakraPetch-{Regular,Medium,SemiBold,Bold,…}.ttf` | Chakra Petch, Cadson Demak / "The Chakra Petch Project Authors", Copyright 2018 | **SIL OFL 1.1** (`…/ofl/chakrapetch/OFL.txt`) | 1000 | 700 | ExtraLight, Light, Regular, Medium, SemiBold, Bold (+ italics) — 12 styles |

[VERIFIED: both TTFs downloaded (61,400 B / 78,488 B), `head` of each OFL.txt reads "licensed under the SIL Open Font License, Version 1.1", UPM/cap read via fontTools, 2026-07-01]

OFL upstream repos (for the SUMMARY provenance record): Bebas Neue → `github.com/dharmatype/Bebas-Neue` (v2.000); Chakra Petch → `github.com/cadsondemak/Chakra-Petch` (orig `github.com/m4rc1e/Chakra-Petch`).

> OFL legality reminder (from 25-RESEARCH State of the Art): the OFL-FAQ permits converting glyphs to outlines and embedding them in a document; the baked outlines are **not** subject to OFL redistribution restrictions, so they ship cleanly under GPL-3.0. **No font program is added** by this plan. [CITED: openfontlicense.org/ofl-faq]

## Font Choice Analysis (the parity reality — VERIFIED data)

Scale-invariant `F` width/cap-height ratio, measured on this machine:

| Face | w/cap ratio | `O` construction | Stem weight | Verdict vs. original (0.626, round, heavy) |
|------|-------------|------------------|-------------|---------------------------------------------|
| **Original (in-SVG)** | **0.626** | round `Q` curves | thick (~0.23 em) | — (the thing we're replacing; likely FoundationLogo) |
| Bebas Neue Regular | 0.416 | round | medium | **Too condensed.** Documented brand fallback, but ~33% narrower wordmark → noticeably different footprint. |
| Chakra Petch Regular | 0.631 | **chamfered (straight)** | light | **Closest footprint** (width nearly identical) but "techy" cut-corner letterforms + lighter weight → different character. |
| Chakra Petch Bold | 0.709 | chamfered | heavy | Width a bit wide; weight closer to original; still chamfered. |

[VERIFIED: fontTools BoundsPen + RecordingPen, 2026-07-01]

**Honest conclusion:** there is **no OFL drop-in that reproduces the current look.** The choice is a brand decision:
- **Bebas Neue** = honors the documented design-language fallback; accept a more condensed, taller-feeling wordmark.
- **Chakra Petch Regular/Medium** = preserves the horizontal footprint (fits the symmetric layout and the FORGE↔rune↔AUDIO spacing with least disruption) but reads more "techy/angular."

IP-03 is satisfied by **either** — visual parity is subordinate to the human-acceptance gate. **Recommend generating both and presenting them.** Mechanizable: everything except the final aesthetic pick.

## Coordinate Mapping (font-em → mm)

The file's convention (keep it — it already renders in VCV/nanosvg):
```
x_mm = Tx + s · x_fontunit
y_mm = Ty − s · y_fontunit          # negative scale flips font y-up → SVG y-down; Ty is the baseline
```
For the **new** fonts (UPM 1000, cap 700), set the scale from the *target cap-height*, not the old scale (old scale assumed UPM~2048):

```
s_new = target_capH_mm / 700
  FORGE / AUDIO : s = 2.723 / 700 = 0.003890
  ANALOG LFO    : s = 3.327 / 700 = 0.004753
```
- Keep **Ty unchanged** (7.600 header, 14.600 title) → baseline stays put.
- Keep glyph `d` in **integer font units** + carry scale in the transform (matches existing style; no float bloat, no over-precision).
- **Horizontal:** advance widths differ per font, so re-anchor by the layout invariant:
  - FORGE → keep **left edge** at x≈9.80 (or center at 18.50).
  - AUDIO → keep **center** at 73.18 (mirror of FORGE about 45.72).
  - ANALOG LFO → keep **center** at 45.72.
  - Compute `Tx` so the *inked* bbox lands on target: render the run at `Tx=0`, measure its mm bbox-x, then `Tx = target_center − (bbox_x0+bbox_x1)/2`.

VCV note: nanosvg **does** honor per-element `transform` (the shipped panel proves it). Do **not** pre-bake coordinates into absolute mm — match the working representation exactly.

## Step-by-Step Recipe

1. **Baseline census (must be 0):** `grep -cE '<text|<tspan|<glyph|<font|@font-face|base64|<use ' res/AnalogLFO.svg` → `0`.
2. **Render reference PNG** (before): Playwright MCP screenshot of `file://…/res/AnalogLFO.svg` at the viewBox aspect. Save as `before.png`.
3. **venv + fonts**: per Recommended Approach (scratch only; never commit `.ttf`).
4. **Generate** FORGE, AUDIO, ANALOG LFO `<path d>` for the chosen face (code below). Compute `s` from cap-H, compute `Tx` to hit the target center, keep `Ty`, keep the original `fill`.
5. **Splice**: replace *only* SVG lines 28, 29, 35 with the new `<path … fill=… transform=…>`. Leave lines 30–34 (rune) and every other element untouched.
6. **Census again** (must be 0) + **well-formed check**: `python3 -c "import xml.dom.minidom,sys; xml.dom.minidom.parse('res/AnalogLFO.svg')"`.
7. **Diff non-wordmark art**: `git diff res/AnalogLFO.svg` should touch only the 3 wordmark lines.
8. **Render after PNG** (same Playwright recipe) → `after.png`; present `before.png` / `after.png` side-by-side for the human gate.
9. (Optional) rebuild plugin against `../Rack-SDK` and confirm the panel loads (nanosvg parse).
10. **Record** font family + version + OFL URL in `25-04-SUMMARY.md`; on operator acceptance set provenance `confirmed-OFL`, mirror to `STATE.md`, unblock 25-03.

## Code Examples (VERIFIED on this machine)

### Generate one wordmark run → `<path>` matching the file's convention

```python
# /tmp/wordmark-venv/bin/python  — fontTools 4.63.0 (verified)
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen
from fontTools.pens.boundsPen import BoundsPen

def run_path(ttf, text, tracking=0):
    """Return (d_string_in_font_units, advance_units, cap_units)."""
    f = TTFont(ttf); gs = f.getGlyphSet(); cmap = f.getBestCmap(); hmtx = f["hmtx"]
    cap = f["OS/2"].sCapHeight                      # 700 for both faces
    pen = SVGPathPen(gs); x = 0
    for ch in text:
        if ch == " ":                               # space = advance only
            x += hmtx[cmap[ord("0")]][0] * 0.5 + tracking; continue
        g = cmap[ord(ch)]
        gs[g].draw(TransformPen(pen, (1, 0, 0, 1, x, 0)))
        x += hmtx[g][0] + tracking
    return pen.getCommands(), x, cap               # d uses native quadratic Q (matches file)

def emit(ttf, text, fill, cap_mm, baseline_ty, center_x):
    d, adv, cap = run_path(ttf, text)
    s = cap_mm / cap                                # e.g. 2.723/700 = 0.003890
    # measure inked bbox in font units to center precisely
    f = TTFont(ttf); gs = f.getGlyphSet()
    from fontTools.svgLib.path import parse_path     # or svgpathtools
    # simplest: re-run a BoundsPen over the same layout
    bp = BoundsPen(gs); cmap = f.getBestCmap(); hmtx = f["hmtx"]; x = 0
    for ch in text:
        if ch == " ": x += hmtx[cmap[ord("0")]][0]*0.5; continue
        g = cmap[ord(ch)]; from fontTools.pens.transformPen import TransformPen
        gs[g].draw(TransformPen(bp, (1,0,0,1,x,0))); x += hmtx[g][0]
    xmin,_,xmax,_ = bp.bounds
    tx = center_x - s*(xmin+xmax)/2
    return (f'<path d="{d}" fill="{fill}" '
            f'transform="translate({tx:.3f},{baseline_ty}) scale({s:.6f},-{s:.6f})"/>')

# Header FORGE (ember), cap 2.723mm, baseline 7.6, center 18.50
print(emit("ChakraPetch-Regular.ttf", "FORGE",      "#e8612a", 2.723, 7.6,  18.50))
print(emit("ChakraPetch-Regular.ttf", "AUDIO",      "#e8612a", 2.723, 7.6,  73.18))
print(emit("ChakraPetch-Regular.ttf", "ANALOG LFO", "#ece8e2", 3.327, 14.6, 45.72))
```

Verified behaviour: `SVGPathPen.getCommands()` returns e.g. `M41 700H332V600H151V405H293V305H151V0H41Z…` with quadratic `Q` segments — the **same dialect** the shipped file uses, so nanosvg renders it identically. [VERIFIED: 2026-07-01]

### Compute a target mm bbox for any existing run (locate/verify)

```python
from svgpathtools import parse_path
p = parse_path(d_from_svg)                          # raw font-unit d
xmin,xmax,ymin,ymax = p.bbox()
X0,X1 = Tx + s*xmin, Tx + s*xmax                    # mm
Y0,Y1 = Ty - s*ymax, Ty - s*ymin                    # mm (y flip)
```

### Census + well-formed gates (automated)

```bash
grep -cE '<text|<tspan|<glyph|<font|@font-face|base64|<use ' res/AnalogLFO.svg      # == 0
python3 -c "import xml.dom.minidom; xml.dom.minidom.parse('res/AnalogLFO.svg'); print('well-formed')"
grep -c '<path' res/AnalogLFO.svg                                                    # >0, ~115
```

## Visual-Parity Verification Recipe

1. **Before/after PNGs via Playwright MCP** (available; no install): open `file://…/res/AnalogLFO.svg`, set viewport to the panel aspect (e.g. 366×514 px = ×4 of mm), screenshot full element → `before.png` / `after.png`.
2. **Optional automated diff metric** (pure-Python, installable): `/tmp/wordmark-venv/bin/pip install pillow numpy` then compare per-pixel:
   ```python
   from PIL import Image, ImageChops; import numpy as np
   a=Image.open("before.png").convert("RGB"); b=Image.open("after.png").convert("RGB")
   d=np.asarray(ImageChops.difference(a,b)); print("mean Δ:", d.mean(), "max Δ:", d.max())
   ```
   Expect change **only** in the wordmark bands (y≈5–15 mm region); knobs/jacks/display Δ≈0.
3. **The real gate is human** (Task 4): the operator accepts the new wordmark look (it *will* differ — see Font Choice Analysis), not a numeric threshold.
4. (Belt-and-suspenders) rebuild against `../Rack-SDK` per `vcv_build_install_workflow.md` and eyeball the panel in Rack (nanosvg is the true renderer; browser is an approximation).

## Pitfalls

| # | Pitfall | Why it bites | Mitigation |
|---|---------|--------------|------------|
| 1 | Expecting pixel parity | Original is a round heavy geometric (FoundationLogo-like); no OFL face matches | Set expectation up front: this is a *re-cut*, human-accepted. Generate both Bebas & Chakra for choice. |
| 2 | Touching the forge-rune | Lines 30–34 are original art between FORGE/AUDIO; easy to clobber when editing the header region | Edit *only* lines 28, 29, 35. `git diff` must show 3 changed lines. |
| 3 | Introducing a font node | A stray `<text>`, `<use>`, `font-family` attr, or `<defs>` glyph ref reintroduces a font program | Use SVGPathPen (emits bare `<path>` only). Re-run the census (==0) after splice. |
| 4 | Wrong scale (UPM mismatch) | New fonts are UPM 1000 / cap 700; old scale 0.001758 assumed UPM~2048 → reusing it shrinks text ~½ | Recompute `s = cap_mm/700` (header 0.003890, title 0.004753). Never reuse old `s`. |
| 5 | Breaking symmetry | Different advance widths shift FORGE/AUDIO off the 45.72 mirror axis | Re-anchor by center (FORGE 18.50, AUDIO 73.18, ANALOG LFO 45.72) via the bbox-center `Tx` formula. |
| 6 | Tracking/kerning drift | Display lockups often have custom letter-spacing; default advances may look loose/tight | Expose a `tracking` knob (code supports it); tune to fit the target width, operator confirms. |
| 7 | Cubic-izing the curves | opentype.js / some tools convert TTF quadratics to cubics, bloating + changing representation | Stick to fontTools SVGPathPen (preserves native `Q`). |
| 8 | Over-precision bloat | Emitting mm-space floats with many decimals balloons the file | Keep `d` in integer font units; carry scale in transform (matches existing file). |
| 9 | Committing the .ttf | The font binary must NOT enter the repo/history (the whole point of the phase) | Fonts live in `/tmp` scratch; only outlines are spliced. `git status` must show no `.ttf`. |
| 10 | Stale NOTICES/OFL credit | `res/fonts/OFL.txt` currently credits only JetBrains Mono | If a new face is adopted, add a Bebas Neue / Chakra Petch attribution line to `NOTICES` (good practice; outlines don't legally require shipping their OFL, but record the source). |

## Package Legitimacy Audit

slopcheck unavailable in this session (`pip` PEP-668 managed env); packages verified by **direct download + provenance + registry age** instead. All are mature, widely-used, install into a throwaway venv (not the system), and ship **no postinstall scripts**.

| Package / Asset | Registry | Age | Provenance | Verdict |
|-----------------|----------|-----|------------|---------|
| `fonttools` 4.63.0 | PyPI | since 2008 (fonttools/fonttools, maintained by the fontTools authors / Google) | Industry-standard font toolkit; used by Google Fonts build pipeline | **Approved** — verified installs + runs (read UPM, SVGPathPen) |
| `svgpathtools` | PyPI | since 2015 (mathandy/svgpathtools) | Established pure-Python SVG path lib | **Approved** — verified bbox computation |
| `pillow` (optional, diff) | PyPI | since 2010 (python-pillow) | Ubiquitous imaging lib | Approved (optional) |
| BebasNeue-Regular.ttf | google/fonts `ofl/bebasneue` | 2019 | SIL OFL 1.1, Dharma Type; 61,400 B downloaded & header-verified | **Approved (OFL 1.1)** — outline source only, not shipped |
| ChakraPetch-*.ttf | google/fonts `ofl/chakrapetch` | 2018 | SIL OFL 1.1, Cadson Demak; 78 KB downloaded & header-verified | **Approved (OFL 1.1)** — outline source only, not shipped |

**[SLOP] removed:** none. **[SUS] flagged:** none. No npm dependency is introduced (fontTools chain is Python-only; opentype.js/text-to-svg explicitly rejected).

## Validation Architecture

> `nyquist_validation` absent → enabled. This plan touches no code (`make test` stays green by construction); its "tests" are deterministic SVG-shape gates plus a human render-acceptance gate.

### Test Framework
| Property | Value |
|----------|-------|
| Framework | shell asserts + `xml.dom.minidom` well-formed check + Playwright MCP render (human gate). doctest suite (`make test`) unaffected. |
| Quick run | `grep -cE '<text\|<tspan\|<glyph\|<font\|@font-face\|base64\|<use ' res/AnalogLFO.svg` |
| Full | census (above) + `git diff --stat res/AnalogLFO.svg` + xml well-formed + before/after render |

### Phase Requirements → Test Map
| Req | Behavior | Type | Automated command | Exists? |
|-----|----------|------|-------------------|---------|
| IP-03 | Outlines from confirmed-OFL face | provenance (manual record) | source family+version+OFL URL recorded in SUMMARY | ✅ (record) |
| IP-03 | Path-only, no font program | smoke | `grep -cE '<text\|<tspan\|<glyph\|<font\|@font-face\|base64\|<use ' res/AnalogLFO.svg` → 0 | ✅ shell |
| IP-03 | Well-formed, viewBox intact | smoke | `python3 -c "import xml.dom.minidom;xml.dom.minidom.parse('res/AnalogLFO.svg')"` + `grep -c 'viewBox="0 0 91.44 128.5"'` | ✅ shell |
| IP-03 | Only 3 wordmark lines changed; rune intact | regression | `git diff res/AnalogLFO.svg` touches lines 28/29/35 only | ✅ git |
| IP-03 | Visual parity / acceptance | manual-only | Playwright before/after render → human accepts | ❌ human gate |
| (regression) | Code untouched | unit | `make test` (47/47) | ✅ existing |

### Sampling Rate
- **Per edit:** census == 0 + xml well-formed.
- **Pre-gate:** `git diff` scope check (3 lines) + before/after render.
- **Phase gate:** operator `confirmed-OFL` → IP-03 closed → 25-03's precondition satisfied.

### Wave 0 Gaps
- None for code. Optional: a `scripts/svg-font-census.sh` wrapping the census grep across `res/*.svg` for repeatable gating (also reused by 25-03's clean-room check).

## Security Domain

> `security_enforcement` absent → enabled. Asset/IP plan, no application attack surface. The only relevant control is **supply-chain provenance** — which is the entire point.

| ASVS | Applies | Control |
|------|---------|---------|
| V10/V14 Supply chain | yes | Fonts fetched only from `github.com/google/fonts` (authoritative OFL mirror); SIL OFL 1.1 header verified on each; outline source recorded by URL+version. fontTools/svgpathtools from PyPI into a throwaway venv. |
| V5 Input validation | n/a | static asset |
| others | no | no auth/session/crypto |

STRIDE (plan-relevant): **Tampering/wrong-source** → only Bebas Neue / Chakra Petch OFL cuts used, provenance recorded (mitigates T-25-14). **Information disclosure (font program leak)** → census==0 after splice (T-25-15). **Tampering with non-wordmark art** → 3-line `git diff` bound (T-25-16).

## Open Questions (HUMAN judgment vs. mechanizable)

| # | Question | Type | Recommendation |
|---|----------|------|----------------|
| Q1 | **Which OFL face for the wordmark?** Bebas Neue (documented fallback, condensed look) vs. Chakra Petch (closest footprint, techy look). | **HUMAN (brand)** | Generate both; present renders; operator picks. IP-03 is satisfied either way. |
| Q2 | **Acceptable to change the wordmark's visual character?** No OFL face reproduces the round heavy geometric original. | **HUMAN (aesthetic)** | Set expectation early; the human-acceptance gate (Task 4) owns this. |
| Q3 | **Do the 15 non-wordmark labels** (MORPH, knob/jack labels, footer — same suspect provenance) also need re-export? Plan scopes only the 3 wordmark runs. | **HUMAN (scope)** | Confirm whether IP-03 closure requires *all* baked text be OFL-sourced, or only the brand wordmark. If all, extend the same recipe to the other 15 runs (mechanizable). |
| Q4 | **Tracking/letter-spacing** of the re-cut wordmark. | mostly mechanizable | Tune `tracking` to hit target widths (FORGE 17.40 / AUDIO 17.21 / ANALOG LFO 49.92 mm), operator confirms. |
| Q5 | **NOTICES attribution** for the adopted face. | mechanizable | Add a credit line to `NOTICES` / `res/fonts` note (Pitfall 10). |
| Everything else | path location, conversion, coord mapping, census, render-diff | **MECHANIZABLE** | Fully specified + verified above. |

## Sources

### Primary (HIGH)
- Direct inspection of `res/AnalogLFO.svg` — 18 baked-text runs enumerated; 3 wordmark runs decoded glyph-by-glyph (FORGE/AUDIO/ANALOG LFO); rune at lines 30–34. 2026-07-01.
- Live pipeline verification: fontTools 4.63.0 venv install; `SVGPathPen` emits quadratic-`Q` `<path>`; UPM/cap/F-bbox/O-construction read for Bebas Neue & Chakra Petch; svgpathtools bbox → exact mm target boxes. 2026-07-01.
- google/fonts OFL fonts downloaded + OFL 1.1 headers verified: `ofl/bebasneue/BebasNeue-Regular.ttf`, `ofl/chakrapetch/ChakraPetch-*.ttf` + `OFL.txt`. 2026-07-01.
- 25-RESEARCH.md (OFL outline legality, State of the Art), 25-02-SUMMARY.md (needs-regeneration decision), project_design_language.md (FoundationLogo brand face, forge-rune, ember #e85d26).

### Secondary (MEDIUM)
- github.com/google/fonts METADATA (Bebas Neue v2.000, 2019; Chakra Petch 2018, Cadson Demak, 12 styles) — WebSearch.
- fonts.google.com/specimen/Bebas+Neue, /Chakra+Petch (OFL 1.1, weights).

### Tertiary
- None relied on for decisions.

## Metadata

**Confidence breakdown:**
- Located wordmark paths + mm target boxes: **HIGH** — computed from live geometry.
- Conversion pipeline (fontTools SVGPathPen): **HIGH** — executed end-to-end on this machine.
- Font sourcing / OFL 1.1: **HIGH** — files downloaded + license headers verified.
- Which face best matches: **HIGH on the data** (Bebas 0.416 vs Chakra 0.631 vs original 0.626; Chakra O is chamfered) — but the *decision* is **HUMAN/aesthetic** (no OFL parity exists).
- VCV/nanosvg compatibility of the transform pattern: **HIGH** — the panel already ships this exact representation.

**Research date:** 2026-07-01
**Valid until:** 2026-08-01 (stable; re-verify the raw google/fonts URLs resolve before fetching).
