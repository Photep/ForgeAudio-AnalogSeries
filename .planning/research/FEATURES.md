# Feature Research — VCV Rack Module User Manual (Notion)

**Domain:** Public-facing user manual / product documentation for a finished VCV Rack 2 module (Forge Audio "Analog Series" Analog LFO), authored in Notion.
**Researched:** 2026-06-14
**Confidence:** HIGH (official VCV manual + multiple leading-author manuals + official Notion docs corroborate every structural claim)

> Scope note: This is documentation research, not product-feature research. "Features" below = manual sections and documentation conventions. The module itself is feature-frozen — the deliverable documents what already ships. The template's MVP/dependency framing is reinterpreted as "table-stakes manual sections vs nice-to-have" and "section authoring order."
>
> This file supersedes the v1.3 product-feature research previously at this path (researched 2026-03-27).

---

## How Leading VCV Authors Document Modules (Evidence)

Surveyed five reference manuals plus the official VCV manual. Each is a distinct convention point; the recommended structure synthesizes the best of all.

| Author / Manual | Format | Structure observed | What to steal |
|-----------------|--------|--------------------|---------------|
| **VCV (official free modules manual)** | Web (illustrated) | Core Concepts page (cross-cutting: knob behavior, modulation, polyphony, tempo-sync) THEN per-module pages; large annotated screenshots; bulleted feature lists | The "Core Concepts first, then modules" split; annotated panel screenshot as the anchor of each page |
| **Surge XT Rack manual** | Web (GitHub Pages) | Horizontal nav, intro/overview, prose per-module with annotated screenshots, links back to main Surge manual; **no install section** (assumes Rack present) | Annotated screenshot + prose; concept cross-links. Note: relies on prose over tables — we will improve on this with a control table |
| **Stoermelder (PackOne `/docs/*.md`)** | Per-module markdown | Introduction → feature/workflow sections (often versioned, e.g. "14-bit CC (v1.9.0+)") → **Changelog** at the bottom (v1.1.0 → v1.10.0). Animated GIFs for workflows, PNGs for context menus | Per-feature workflow sections; **changelog appended to the module page**; GIFs for time-based behavior (perfect for our drift/sync animation) |
| **Bogaudio** | Single README, category-grouped | Per-module: short description + bulleted capabilities, controls woven into prose, **standardized footer** ("Note on Polyphony", "Note on Bypassing", "When bypassed") | Standardized footer blocks for cross-cutting behaviors (polyphony, bypass) — readers learn to expect them |
| **Befaco / Mutable-style** | Mutable-inspired authors lean on the original hardware manuals' "one page per concept, signal-flow diagram up top" tradition | Signal-flow diagram, then control-by-control | A signal-flow diagram near the top is a strong convention for analog-modeled modules |

**Cross-author consensus (table-stakes conventions):**
1. An **annotated panel screenshot** is the visual anchor of the main page.
2. **Controls are explained by name**, grouped logically (main controls, then I/O, then menu).
3. **Context-menu / right-click options get their own section** — universal in VCV docs because they're invisible on the panel.
4. **A changelog lives in the docs** (Stoermelder appends per module; VCV keeps a global one).
5. **Polyphony and bypass behavior** are explicitly stated (Bogaudio standardizes this) — even "monophonic by design" is worth one line.
6. Prose-heavy is the norm, but **none of these use a clean control-reference table** — that is our opportunity to be clearer than the field.

---

## Feature Landscape (Manual Sections)

### Table Stakes (Users / VCV Community Expect These)

Missing any of these makes the manual feel incomplete by VCV-community standards.

| Section | Why Expected | Effort | Notes |
|---------|--------------|--------|-------|
| Overview / Concept ("What is it") | First thing every reference manual opens with; sells the three-knob engine | LOW | 2-4 sentences + hero panel image. Lead with the Morph/Character/Drift value prop |
| Annotated panel screenshot | The visual anchor of every VCV module page (VCV official, Surge, Stoermelder) | LOW | Numbered callouts mapping to the control table. Single most important image |
| Signal-flow / "How it works" diagram | Analog-modeled-module convention (Mutable/Befaco tradition); demystifies a 3-axis engine | MED | CLK→tracker→rate; knobs→engine→single ±5V out. Mermaid or a simple PNG |
| **Control reference table** (knobs/buttons) | Users scan for "what does this knob do"; clearer than the prose norm | LOW | One row per control. Template below. This is the spine of the manual |
| Inputs / Outputs reference (jacks) | Every patch question is "what goes where"; CV ranges are non-obvious | LOW | Voltage ranges, polarity, what each CV modulates. Note single bipolar ±5V output |
| Context-menu (right-click) options | Invisible on panel; **universal dedicated section** in VCV docs | LOW | Swing presets and any other menu items. Screenshot of the open menu |
| Rate-knob dual-mode explanation | Behavior changes when CLK is patched — genuinely confusing without docs | MED | The 15 snapped ratios, free Hz mode, the switch trigger. Needs its own subsection |
| Clock sync explanation | 3-state tracker (FREE/ACQUIRING/LOCKED), phase reset, anti-click | MED | Explain the SYNC badge states and what the display pills mean |
| Installation — VCV Library | The expected install path for a Library module | LOW | Subscribe on library.vcvrack.com + sync in Rack. Steps below |
| Installation — Manual (.vcvplugin) | Needed pre-Library and for power users; VCV documents both | LOW | Open user folder, drop .vcvplugin, restart. Steps below |
| Changelog / version history | Stoermelder + VCV both ship one; users check "what changed" | LOW | Derive directly from MILESTONES.md (v1.0→v1.4). Append at bottom |
| Credits / License | GPL-3.0 is a v1.4 requirement; attribution expected for open source | LOW | License, author, links to source repo + VCV Library page |

### Differentiators (Make This Manual Better Than Average)

Not strictly expected, but elevate the manual and directly serve this module's unusual 3-axis engine.

| Section | Value Proposition | Effort | Notes |
|---------|-------------------|--------|-------|
| "Understanding the Analog Engine" conceptual page | The Morph/Character/Drift model is the core value (PROJECT.md) — a dedicated mental-model page makes it click | MED | The 3-axis explainer below. The single highest-leverage page for this product |
| Patch / usage examples (2-4) | VCV users learn by patching; examples drive adoption | MED | LFO-appropriate examples below. Ideally with a screenshot or .vcv download |
| Animated GIFs of drift + SYNC flash | Drift and the per-edge SYNC flash are time-based — static images can't show them (Stoermelder uses GIFs exactly here) | MED | One GIF of the ember waveform drifting; one of the SYNC badge locking |
| Troubleshooting / FAQ | Pre-empts support questions ("why won't it sync?", "ratios feel off") | LOW | Short Q&A. Address known v1.4 fixes (tempo jumps, x1.5 alignment) as resolved |
| "Tips & tricks" / sweet spots | Curated knob recipes invite exploration (Character x² curve rewards it) | LOW | e.g. "Drift ~30% + Character ~60% on a triangle = vintage Juno wobble" |
| Quick-start / "first 60 seconds" callout | Lowers the barrier; a single callout box at the top | LOW | "Patch the output to a filter cutoff, sweep Morph, turn up Character." Notion callout block |

### Anti-Features (Documentation Pitfalls to Avoid)

| Pitfall | Why Tempting | Why Problematic | Instead |
|---------|--------------|-----------------|---------|
| Documenting DSP internals (OU layers, EMA alpha, crossfade ms) in the user manual | The engineering is impressive | Users don't care; obscures usage; ties docs to implementation | Keep internals in code/README; manual describes *behavior and feel* |
| One giant single page | Easy to write | Hard to scan/link; Notion ToC gets unwieldy; bad on mobile | Subpage-per-section tree (below). Each section deep-linkable |
| Tables-only OR prose-only | Consistency | Tables lack the "why"; prose lacks scannability | Hybrid: control **table** for reference + short prose for concepts (beats every surveyed manual) |
| Hardware-style electrical specs (impedance, etc.) | Looks authoritative | Meaningless in software; not a VCV convention | Voltage ranges and signal polarity only |
| Inventing/aspirational features | Filling the page | Documents things that don't ship; erodes trust | Document only shipped behavior; defer VCO to a future "Series" page |
| Re-explaining how VCV Rack works | Helpful to novices | Bloat; VCV's own manual owns this | Link to vcvrack.com/manual for general Rack concepts |
| Trademark-y "sounds like a Minimoog" claims | Marketing punch | Legal/trademark risk (already an Out-of-Scope concern in PROJECT.md) | "Classic-synth-inspired character references"; describe the *behavior* |

---

## Recommended Notion Page Tree (Concrete)

Top-level published page + subpages. Each subpage has its own `/Table of contents` block and is independently deep-linkable. This mirrors VCV's "Core Concepts then modules" and Stoermelder's per-feature split, adapted to a single module.

```
📕 Forge Audio — Analog LFO — User Manual        ← top-level, "Share to web" ON
│   (Hero panel image; one-paragraph overview; quick-start callout;
│    /Table of contents block linking to every subpage)
│
├── 1. Overview & Concept
│       What it is, who it's for, the three-knob value prop, hero shot
│
├── 2. Understanding the Analog Engine   ★ differentiator, highest leverage
│       The Morph / Character / Drift mental model (3-axis explainer)
│       One subsection per axis, each with its own waveform image/GIF
│
├── 3. Signal Flow
│       Diagram (Mermaid or PNG): inputs → engine → single ±5V output;
│       CLK → tracker → rate-mode switch
│
├── 4. Controls Reference
│       Annotated panel screenshot (numbered callouts)
│       Control reference TABLE (template below)
│       4a. Rate knob — dual mode (free Hz / 15 musical ratios)
│       4b. Phase Offset (knob + CV)
│
├── 5. Inputs & Outputs
│       Jack reference table (CV ranges, polarity, FM, RESET, CLK, output)
│
├── 6. Clock Sync
│       3-state tracker + SYNC badge states; phase reset; swing interaction;
│       reading the display pills (ratio / BPM / incoming BPM)
│
├── 7. The Display
│       Three-column CRT layout: SYNC badge, ratio/BPM pills, ember waveform,
│       phase dot. Animated GIF of drift + SYNC flash
│
├── 8. Right-Click Menu Options
│       Swing presets and any other menu items; screenshot of open menu
│
├── 9. Patch Examples            ★ differentiator
│       2-4 LFO-appropriate recipes (below), each with screenshot
│
├── 10. Installation
│       10a. From the VCV Library (recommended)
│       10b. Manual install (.vcvplugin)
│
├── 11. Troubleshooting / FAQ
│
└── 12. Version History (Changelog) · Credits · License
        From MILESTONES.md; GPL-3.0; source repo + Library links
```

**Authoring order (the template's "MVP" reframed):**
- **Write first (table-stakes spine):** 1, 4, 5, 8, 10, 12. A manual with just these is *complete and shippable*.
- **Write second (the value-add that sells the module):** 2, 6, 7, 9.
- **Write last (polish):** 3 (diagram), 11, tips/quick-start callout.

---

## Control-Reference Table Template (the spine of the manual)

Use one table for panel **controls** and a separate table for **jacks**. Improves on every surveyed manual (none use a clean table).

**Controls (knobs / buttons):**

| # | Control | Type | Range / Default | What it does | CV input? |
|---|---------|------|-----------------|--------------|-----------|
| 1 | Morph | Knob | Sine→Tri→Saw→Square→Pulse, continuous | Selects waveform shape along one continuous axis | Yes (Morph CV) |
| 2 | Character | Knob | 0–100%, default 0 | Digital-clean → classic-synth analog modeling, per shape (x² curve — subtle early) | Yes (Character CV) |
| 3 | Drift | Knob | 0–100% | Multi-timescale analog pitch instability (jitter, wander, slew, spread) | Yes (Drift CV) |
| 4 | Rate | Knob | Free: ~0.01–20 Hz · Clocked: 15 ratios (/16…×16) | LFO speed; **switches to musical ratios when CLK is patched** | Yes (Rate CV) |
| 5 | Phase Offset | Knob | 0–360° | Shifts output phase at the readout | Yes (Phase CV) |

(The `#` column maps to the numbered callouts on the annotated panel screenshot.)

**Inputs & Outputs (jacks):**

| Jack | Dir | Signal / Range | Notes |
|------|-----|----------------|-------|
| Morph CV | In | ±5 V (bipolar) | Adds to Morph knob |
| Character CV | In | 0–10 V (or ±5 V — confirm in code) | Adds to Character |
| Drift CV | In | ±5 V | Scales drift amount; display responds to CV |
| Rate CV | In | ±5 V | Free: V/oct-like; Clocked: selects ratio |
| Phase CV | In | ±5 V | Adds to Phase Offset |
| FM | In | ±5 V | Exponential FM; reduced authority when clocked |
| CLK | In | Trigger/gate | Engages clock sync; switches Rate to ratio mode |
| RESET | In | Trigger | Resets phase with anti-click crossfade; 1 ms blanking |
| OUT | Out | ±5 V bipolar | Single morphed output (no inverted/discrete outs by design) |

> Confirm exact CV voltage ranges/polarity against `AnalogLFO.cpp` before publishing — the manual must match shipped behavior, not assumptions. Add a "Polyphony: monophonic by design" footer line (Bogaudio convention).

---

## Presenting the 3-Axis Analog Engine to New Users

The hardest documentation problem here. Recommended framing (used in section 2):

**Anchor metaphor — "three independent dials on a vintage oscillator":**
- **Morph = WHAT shape** — "Pick your waveform by turning one knob instead of flipping a switch. It sweeps smoothly: Sine → Triangle → Saw → Square → Pulse." (Show the five shapes as a labeled strip image, with the in-between morph positions called out.)
- **Character = HOW analog** — "From a clean digital waveform to the warmth and imperfection of classic analog synths. Each shape has its own character target. The effect is gentle at first and grows as you turn it up." (x² curve — say "rewards turning it past halfway".)
- **Drift = HOW alive** — "Real analog circuits never sit perfectly still. Drift adds slow, organic instability — the higher you go, the more the pitch wanders and breathes. Watch the waveform on the display move on its own." (This is where the **animated GIF** earns its place.)

**Presentation tactics:**
1. **One subsection per axis**, each with its own image, in Morph→Character→Drift order (matches the knob layout and PROJECT.md's "natural progression").
2. **A small 3-cell comparison callout**: same patch at (clean), (warm), (wild) — three screenshots showing the display.
3. **"They're independent"** stated explicitly — the key insight is that the three axes don't fight each other.
4. **Lead with feel, not DSP.** Never mention Ornstein-Uhlenbeck, EMA, or layer frequencies in the user manual.

---

## Patch / Usage Examples (LFO-appropriate, 2–4)

| # | Name | What it teaches | Patch |
|---|------|-----------------|-------|
| 1 | **Slow filter sweep** (first patch) | Core use as a modulation source; Morph + Character feel | OUT → filter cutoff CV. Set Rate slow (~0.2 Hz), sweep Morph, raise Character. Hear the shape and warmth change |
| 2 | **Tempo-synced wobble** | Dual-mode Rate + clock sync + SYNC badge | Clock module → CLK. Rate now snaps to ratios; pick ÷2. OUT → VCA or filter for a beat-locked LFO. Watch the BPM/ratio pills |
| 3 | **Living analog vibrato / instability** | Drift as the headline differentiator | OUT (small attenuation) → VCO 1V/oct or FM. Turn Drift up to hear organic, never-repeating pitch wander |
| 4 | **Self-FM / evolving texture** (advanced) | FM input + Phase Offset + swing | Patch OUT → FM (or a second LFO → FM). Add swing via right-click for groove; use Phase Offset to align with other modulators |

Each example: short setup steps + a screenshot of the patch. Example 1 doubles as the quick-start.

---

## Installation Instructions (Both Paths — Verified Against vcvrack.com/manual/Installing)

**A. From the VCV Library (recommended, once accepted):**
1. Open **library.vcvrack.com**, log in with your VCV account.
2. Find **Forge Audio — Analog LFO** and click **Add to my library** (subscribe).
3. In VCV Rack, choose **Library → Update all** (or Sync) and restart Rack if prompted.
4. The module appears in the browser under **Forge Audio**.

**B. Manual install via `.vcvplugin` (pre-Library / power users):**
1. In Rack: **Help → Open user folder** (opens the `Rack2` folder).
2. Open `plugins-<OS>-<CPU>/` (e.g. `plugins-mac-arm64`, `plugins-win-x64`, `plugins-lin-x64`).
3. Copy the downloaded `ForgeAudio-AnalogLFO-x.x.x.vcvplugin` into that folder.
4. Restart VCV Rack — Rack extracts and loads the plugin on launch.
5. Troubleshooting note: if it doesn't appear, check `log.txt` in the user folder, and verify the **plugin major version matches your Rack major version (2.x)**.

> Include a security note (VCV's own warning): only install plugins from trusted sources. Link the source GitHub repo and the Library page.

---

## Notion-Specific Structuring Best Practices (Verified Against Notion Help Docs)

| Practice | Recommendation | Source-backed |
|----------|----------------|---------------|
| Page hierarchy | Use the **Wiki** feature or a parent page with subpages (one per manual section). Nested subpages = clean left-sidebar nav | Notion Wiki feature / help docs |
| Table of contents | Add a **`/Table of contents` block** at the top of the top-level page and each long subpage — auto-populates from H1/H2/H3 headings | Notion content-blocks help |
| Callouts | Use **callout blocks** for quick-start, tips, and warnings (e.g. "⚠️ Rate switches modes when CLK is patched"). Callouts now support custom titles, hidden emoji, toggles | Notion content-blocks help |
| Control reference | Use Notion **simple tables** (not databases) for the control/jack reference — render cleanly when published, no DB overhead | Notion blocks |
| Images / diagrams | Drag-drop PNGs; the **annotated panel screenshot** is the page anchor. Images can be hyperlinked. Embed GIFs natively for drift/SYNC animation | Notion embeds/images help |
| Signal-flow diagram | Notion supports `/code` with **Mermaid** for a maintainable signal-flow diagram, or a PNG export | Notion blocks |
| Publishing | **Share → Share to web** ON. Optionally set a custom domain/site. Test with a small group before announcing | Notion "Publish a Notion Site" help |
| Publish settings | For a public manual, keep **"Allow duplicate as template" off** unless desired; **search-engine indexing on** so users can find it; comments/editing off for the public link | Notion sharing & permissions help |
| Cross-linking | Link section subpages with `@`-mentions / page links so the diagram, control table, and clock-sync page reference each other | Notion |

---

## Section Dependencies (Authoring Order)

```
Annotated panel screenshot
    └──feeds──> Controls Reference table (# callouts map to rows)
    └──feeds──> Inputs & Outputs table

Understanding the Analog Engine (concept)
    └──prereq for──> Patch Examples (examples assume the mental model)

Clock Sync section
    └──prereq for──> Rate dual-mode subsection (ratios only make sense with sync)
    └──prereq for──> Display section (pills show ratio/BPM)

MILESTONES.md ──source──> Changelog
AnalogLFO.cpp ──source-of-truth──> Control + I/O tables (verify ranges)
```

---

## Table-Stakes vs Nice-to-Have (Final Split)

**Table-stakes (a VCV manual is incomplete without these):**
Overview/Concept · Annotated panel screenshot · Controls reference table · Inputs/Outputs reference · Right-click menu section · Rate dual-mode explanation · Clock-sync explanation · Installation (Library + manual) · Changelog · Credits/License.

**Nice-to-have (elevate the manual; serve this module's unusual engine):**
"Understanding the Analog Engine" page · Signal-flow diagram · Patch examples · Animated GIFs · Troubleshooting/FAQ · Tips & sweet spots · Quick-start callout · The Display deep-dive.

**Cut / avoid:** DSP internals · single-giant-page · electrical specs · trademark claims · re-teaching VCV Rack basics · documenting unreleased VCO features.

---

## Sources

- [VCV Rack — Installing & Running](https://vcvrack.com/manual/Installing) (HIGH — official; both install paths)
- [VCV Rack — Plugin Manifest](https://vcvrack.com/manual/Manifest) (HIGH — plugin.json / manual URL field)
- [VCV Rack — official Manual](https://vcvrack.com/manual/) (HIGH)
- [Surge XT VCV Rack Modules Manual](https://surge-synthesizer.github.io/rack_xt_manual/) (HIGH — structure reference)
- [Stoermelder PackOne — MIDI-CAT docs](https://github.com/stoermelder/vcvrack-packone/blob/v1/docs/MidiCat.md) (HIGH — per-feature sections + appended changelog + GIFs)
- [Stoermelder PackOne — docs index](https://github.com/stoermelder/vcvrack-packone) (HIGH)
- [Bogaudio Modules — README](https://github.com/bogaudio/BogaudioModules) (HIGH — standardized polyphony/bypass footers)
- [CDM — VCV Rack's new docs for free modules](https://cdm.link/vcv-rack-free-docs/) (MEDIUM — illustrated-docs philosophy)
- [Myk Eff — Adding Plugins to VCV Rack](https://soundand.design/adding-plugins-to-vcv-rack-f3d9ce5bba0e) (MEDIUM — community install walkthrough)
- [MetaModule — Manual VCV plugin install](https://metamodule.info/docs/rack_manual_install.html) (MEDIUM — .vcvplugin folder steps)
- [Notion — Publish a Notion Site](https://www.notion.com/help/public-pages-and-web-publishing) (HIGH — publish/share-to-web)
- [Notion — Types of content blocks](https://www.notion.com/help/guides/types-of-content-blocks) (HIGH — ToC, callout, table blocks)
- [Notion — Sharing & permissions](https://www.notion.com/help/sharing-and-permissions) (HIGH — public-page settings)
- [Notion — Embeds & images](https://www.notion.com/help/embed-and-connect-other-apps) (HIGH)
- [Notion VIP — Wiki feature](https://www.notion.vip/insights/notions-wiki-feature-overlooked-superpower) (MEDIUM — hierarchy patterns)
- Internal: `.planning/PROJECT.md`, `.planning/MILESTONES.md` (HIGH — module behavior + changelog source)

---
*Feature research for: VCV Rack module user manual authored in Notion*
*Researched: 2026-06-14*
