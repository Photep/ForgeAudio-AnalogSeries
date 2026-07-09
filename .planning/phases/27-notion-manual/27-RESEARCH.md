# Phase 27: Notion Manual (pivoted → GitHub Markdown) - Research

**Researched:** 2026-07-09
**Domain:** Technical documentation authoring (Markdown), VCV Rack manifest conventions, in-Rack screenshot capture + annotation
**Confidence:** HIGH (all module-content facts read directly from `src/AnalogLFO.cpp` + `src/dsp/*.hpp`; VCV manifest fields verified against official docs)

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Manual lives as **GitHub Markdown in the (now-public) repo**, not Notion. Versioned with the plugin, single source of truth. Supersedes the roadmap's Notion decision.
- **D-02:** **Per-section Markdown files under `docs/`** (one file per table-stakes section) with a `docs/index.md` hub linking them.
- **D-03:** Author in a **Pages-ready shape** (per-section files, image assets under `docs/img/` referenced by **relative links** that render both in plain GitHub view today and a Pages site later). **No Pages build step is set up in this phase — structure only.**
- **D-04:** Add a dedicated **`manualUrl`** field to `plugin.json` pointing at the repo docs. **Keep** `pluginUrl`/`authorUrl`/`sourceUrl`. (Reachable only once repo goes public in Phase 28.)
- **D-05:** **Terse reference voice** — control tables, ranges, behaviors as facts. No tutorial/hand-holding.
- **D-06:** **Generic descriptive vocabulary for the Character knob** ("vintage transistor saw", "classic PWM square", "warm analog triangle"). **No trademarked synth brand names anywhere** in the manual prose.
- **D-07:** **No patch-examples section.** Amends DOC-02.
- **D-08:** Annotated-panel section uses a **real VCV Rack screenshot of the shipped module** with **numbered callouts baked into the PNG**, paired with a **numbered legend table** that **doubles as the control-reference table**. NOT the design-mockup PNGs, NOT the SVG export.
- **D-09:** Derive changelog **from milestone history** (`.planning/milestones/*-ROADMAP.md` + ROADMAP.md milestone list). **No `CHANGELOG` file exists.** Structure by shipped milestone (v1.0 → v1.4).

### Claude's Discretion
- Exact per-section file naming/ordering under `docs/`, install-section wording, and legend-table column layout are open within the decisions above.

### Deferred Ideas (OUT OF SCOPE)
- **GitHub Pages site (MkDocs/Docusaurus):** future version. Do not set up a build step now.
- **Patch/tutorial examples:** dropped for this phase (D-07).
- **Sequencing constraint (hard):** `manualUrl` + manual are only publicly reachable once the repo flips public in **Phase 28**. Authoring/committing docs and adding `manualUrl` happens in Phase 27; the DOC-03 "shared to web" verification completes alongside/after Phase 28. Planner must note this dependency rather than assume the link is live mid-Phase-27.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description (current wording — see reconciliation) | Research Support |
|----|---------------------------------------------------|------------------|
| DOC-01 | "User manual published in Notion as a new top-level page with a subpage-per-section structure" — **superseded by D-01/D-02**: authored as GitHub Markdown under `docs/` with per-section files + `docs/index.md` hub | §Docs Structure; §Roadmap/Requirements Reconciliation gives the exact edit |
| DOC-02 | "Manual covers all table-stakes sections … patch examples …" — **amended by D-07** (drop patch examples) | §Section Set & Code-Sourced Content enumerates every remaining section with the exact facts to transcribe |
| DOC-03 | "Manual is shared to web (public) and linked from `pluginUrl`" — **amended by D-04**: linked via new `manualUrl`; public reachability completes with Phase 28 repo-public flip | §VCV Manifest Conventions (verified `manualUrl` field); §Sequencing note |
</phase_requirements>

## Summary

This is a **documentation-authoring phase**, not a code phase. Exactly one code file changes (`plugin.json`, to add `manualUrl`); everything else is new `docs/` content plus small wording edits to `ROADMAP.md` and `REQUIREMENTS.md` to reconcile the Notion→GitHub pivot. There are **no external packages to install** — so the Package Legitimacy Audit and Security Domain sections are N/A (noted below). The dominant risk is not tooling; it is **factual drift between the manual and the code**. Every control, range, ratio, and behavior in the manual must be transcribed verbatim from `src/AnalogLFO.cpp` / `src/dsp/*.hpp`, which this research has already extracted in full so the planner can write precise acceptance criteria without re-reading the source.

The module exposes **10 params, 7 inputs, 1 output**, a **15-entry ratio table**, a **6-option Swing submenu** (the only context-menu item), and a **FREE/ACQUIRING/LOCKED clock state machine**. All CV inputs use a **bipolar ±5V, 5V-per-unit** convention (`value = knob + trim × CV/5`, clamped), except FM which is **exponential** (`freq ×= 2^(CV × trim × depthScale)`), CLK/RESET which are **Schmitt-trigger gates (1.0V rising / 0.1V falling)**, and OUTPUT which is **bipolar ±5V**. These exact values are tabulated below.

The annotated-panel deliverable is low-friction here: the module is **already built, installed, and `dist/…-2.0.0-mac-arm64.vcvplugin` already exists**, and VCV Rack 2 (Free + Pro), the `../Rack-SDK`, `make`, and macOS `screencapture` are all present. The only missing tool is an image annotator (no ImageMagick) — callout baking is best done via macOS Preview markup or an HTML-overlay + Playwright approach (Playwright MCP is present).

**Primary recommendation:** Create `docs/` with `index.md` + one file per section, transcribe the tabulated code facts below verbatim, capture one real Rack screenshot and bake numbered callouts keyed to a single legend/control-reference table, add `manualUrl` to `plugin.json`, and apply the specific ROADMAP/REQUIREMENTS wording edits. Gate publication on a code-vs-docs fact check.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Manual content (prose + tables) | Static docs (`docs/*.md`) | — | Markdown files versioned in-repo (D-01/D-02) |
| Image assets (annotated panel) | Static assets (`docs/img/`) | — | Committed PNGs referenced by relative links (D-03/D-08) |
| Manifest link | Build/packaging (`plugin.json`) | — | `manualUrl` field consumed by VCV Library + Rack UI (D-04) |
| Public reachability | Repo hosting (GitHub) | Phase 28 | Links resolve only after the repo goes public (deferred) |
| Fact source of truth | Source code (`src/AnalogLFO.cpp`, `src/dsp/*.hpp`) | — | Docs transcribe; code owns the values |

## Standard Stack

There is **no library/package stack** for this phase. The "stack" is:

| Tool | Version / Location | Purpose | Notes |
|------|-------------------|---------|-------|
| GitHub-Flavored Markdown | rendered by GitHub | Manual content | Relative `.md` links + relative `img/` paths render in GitHub view today AND in MkDocs/Docusaurus later `[CITED: docs.github.com/…markdown]` |
| VCV Rack 2 manifest (`plugin.json`) | schema per vcvrack.com/manual/Manifest | `manualUrl` link | `manualUrl` is a documented field at both plugin and module level `[VERIFIED: vcvrack.com/manual/Manifest]` |
| `make` + `../Rack-SDK` | GNU Make 3.81, SDK present | Build/install module for screenshot | Per `[[vcv_build_install_workflow]]` — **no worktree isolation** (`../Rack-SDK` won't resolve) |
| VCV Rack 2 Free/Pro | installed in `/Applications` | Load module → screenshot | Module already installed |
| macOS `screencapture` (or ⌘⇧4) | `/usr/sbin/screencapture` | Capture the panel PNG | Native |
| Annotation tool | **none installed** — see options | Bake numbered callouts | Preview markup, or HTML-overlay + Playwright (both available); ImageMagick absent |

**Installation:** none required.

## Package Legitimacy Audit

**N/A — this phase installs no external packages.** The only code edit is a manual `manualUrl` string added to the existing `plugin.json`. No npm/PyPI/crates dependencies are introduced.

## Section Set & Code-Sourced Content (the deltas the planner needs)

The remaining table-stakes sections after D-07 (patch examples removed):
3-axis engine concept · annotated panel · control-reference table (= panel legend) · I/O reference · context-menu options (Swing only) · clock/sync behavior · install (Library + manual `.vcvplugin`) · changelog · license/credits.

Below is every fact to transcribe, read directly from source `[VERIFIED: src/AnalogLFO.cpp]` / `[VERIFIED: src/dsp/LfoCore.hpp]` / `[VERIFIED: src/dsp/ClockTracker.hpp]`.

### The 10 Params (`AnalogLFO::ParamId`, from `configParam` calls)

| # | Enum | Label (configParam) | Range | Default | Display |
|---|------|---------------------|-------|---------|---------|
| 1 | MORPH_PARAM | Morph | 0–1 | 0 | raw (waveform sweep: sine→tri→saw→square→pulse) |
| 2 | CHARACTER_PARAM | Character | 0–1 | 0 | raw (x² progressive curve; 0.5 knob ≈ 25% effect) |
| 3 | DRIFT_PARAM | Drift | 0–1 | 0 | raw |
| 4 | RATE_PARAM | Rate | 0.01–20 | 0.7 | " Hz" free-running; shows ratio label + " (synced)" when clocked (custom `RateParamQuantity`) |
| 5 | MORPH_ATTEN_PARAM | Morph CV | 0–1 | **1** | "%" (0–100) — attenuator |
| 6 | CHARACTER_ATTEN_PARAM | Character CV | 0–1 | **1** | "%" — attenuator |
| 7 | DRIFT_ATTEN_PARAM | Drift CV | 0–1 | **1** | "%" — attenuator |
| 8 | PHASE_OFFSET_PARAM | Phase Offset | 0–1 | 0 | " deg" (0–360°) |
| 9 | PHASE_OFFSET_ATTEN_PARAM | Phase Offset CV | 0–1 | **1** | "%" — attenuator |
| 10 | FM_ATTEN_PARAM | FM Depth | 0–1 | **0** | "%" — attenuator (note: FM depth defaults to 0) |

### The 7 Inputs (`AnalogLFO::InputId`) + CV ranges (from `process()`)

CV convention: for Morph/Character/Drift/Phase-Offset, `value = knob + trim × (CV / 5V)`, then **clamped to [0,1]**. Thus **±5V bipolar, 5V = full-scale** at trim 100%.

| # | Enum | Label | Signal type | Range / behavior |
|---|------|-------|-------------|------------------|
| 1 | MORPH_CV_INPUT | Morph CV | bipolar CV | ±5V → full morph sweep (5V/unit, attenuated by Morph CV trim) |
| 2 | CHARACTER_CV_INPUT | Character CV | bipolar CV | ±5V → full range (5V/unit; note characterSpread is folded in too) |
| 3 | DRIFT_CV_INPUT | Drift CV | bipolar CV | ±5V → full range (5V/unit) |
| 4 | CLK_INPUT | Clock | gate/trigger | Schmitt: rising edge above **1.0V**, arm below **0.1V**. Any clock/gate 0→5V or 0→10V works |
| 5 | RESET_INPUT | Reset | trigger | Schmitt 1.0V/0.1V; **1ms blanking** suppresses a reset landing on a clock-driven reset |
| 6 | PHASE_OFFSET_CV_INPUT | Phase Offset CV | bipolar CV | ±5V → 0–360° (5V/unit, attenuated by Phase Offset CV trim). Only applied when patched |
| 7 | FM_INPUT | FM | exponential CV | `freq ×= 2^(CV × FMdepth × depthScale)`, depthScale = **0.6 free-running / 0.5 clocked**. Uses `exp2_taylor5` (Rack poly), not libm |

### The 1 Output (`AnalogLFO::OutputId`)

| # | Enum | Label | Signal |
|---|------|-------|--------|
| 1 | OUTPUT | LFO | **Bipolar ±5V** (`5.f × sample`; 3ms cosine anti-click crossfade on phase reset; DC-offset drift added post-crossfade) |

### The 15-entry RATIO_TABLE (`RATIO_TABLE` + `RATIO_LABELS`)

Selected in clocked mode by the Rate knob via `round(scaledKnob × 14)` → index 0–14. **x1 is index 7.**

| Idx | Label | Multiplier | | Idx | Label | Multiplier |
|-----|-------|-----------|-|-----|-------|-----------|
| 0 | /16 | 0.0625 | | 8 | x1.5 | 1.5 |
| 1 | /8 | 0.125 | | 9 | x2 | 2.0 |
| 2 | /6 | 0.166667 | | 10 | x3 | 3.0 |
| 3 | /4 | 0.25 | | 11 | x4 | 4.0 |
| 4 | /3 | 0.333333 | | 12 | x6 | 6.0 |
| 5 | /2 | 0.5 | | 13 | x8 | 8.0 |
| 6 | /1.5 | 0.666667 | | 14 | x16 | 16.0 |
| 7 | x1 | 1.0 | | | | |

Multiplier is relative to detected clock frequency: effective LFO freq = `(1/smoothedPeriod) × RATIO_TABLE[idx]`. Display shows effective BPM (and incoming CLK BPM when ratio ≠ x1).

### The Swing submenu — the ONLY context-menu item (`appendContextMenu` → `createIndexSubmenuItem("Swing", …)`)

6 options; default index 0 (Straight); persisted as `swingIndex` in patch JSON; **active only in clocked mode** (free-running stores 0.5, no warp).

| Idx | Menu label | Fraction |
|-----|-----------|----------|
| 0 | Straight 50% | 0.50 (default) |
| 1 | Light 54% | 0.54 |
| 2 | Medium 58% | 0.58 |
| 3 | Triplet 66% | 0.66 |
| 4 | Heavy 71% | 0.71 |
| 5 | Max 75% | 0.75 |

Do **not** invent other menu options — there are none. (When swing > Straight and clocked, an overlay label appears bottom-left of the display; that is UI, not a menu item.)

### Clock/sync behavior — FREE / ACQUIRING / LOCKED state machine (`ClockTracker::step`)

- **FREE:** no clock patched (or timed out / disconnected). Rate knob = free Hz (0.01–20). Display shows Hz readout.
- **Connect + first edge:** phase resets, enters **ACQUIRING**. SYNC badge blinks at 2 Hz.
- **ACQUIRING → LOCKED:** either (a) *fast-track* — 2nd edge period within **0.8×–1.2×** of the last known period → immediate LOCK (EMA-snapped), or (b) after **≥4 consistent edges**. Period smoothed by EMA (α = 0.3).
- **LOCKED:** tracking. Ratio label + BPM shown; SYNC badge does a per-edge white-hot flash. **Outlier rejection:** edges **>3×** or **<⅓×** the smoothed period are discarded — *except* **3 consecutive outliers** (`OUTLIER_THRESHOLD`) are treated as a genuine tempo change → drop to ACQUIRING and re-learn (BUG-01 fix; prevents permanent lockout on a >3× speedup).
- **Timeout → FREE:** no edge within `max(1.0s, min(3×period, 5.0s))`.
- **Disconnect → FREE:** instant; last period cached for fast re-lock.
- **Division-aware phase reset:** phase resets on musical beat boundaries (`shouldReset(ratioIdx, beatCount)`), with the 3ms cosine anti-click crossfade. RESET jack forces phase=0 (with 1ms blanking).

### 3-axis engine concept (for the concept section)

Three independent axes (from PROJECT.md / DECISIONS — use **generic** vocabulary per D-06):
- **Morph** — waveform shape sweep: sine → triangle → saw (falling ramp) → square → narrow pulse (PWM folded into morph, even 20%-per-shape).
- **Character** — crossfade from clean/digital to analog character per shape, using **neutral terms** ("warm analog triangle", "vintage transistor saw", "classic PWM square"). **No brand names.** x² progressive curve.
- **Drift** — multi-timescale Ornstein-Uhlenbeck pitch drift + phase jitter + DC wander + thermal slew, scaled in curated proportions by one knob. Drift authority is reduced in clocked mode.

## Annotated-Panel Capture Procedure (D-08 deliverable)

Environment is ready (see Environment Availability): module already built + installed, `dist/…-2.0.0-mac-arm64.vcvplugin` present, VCV Rack 2 installed, `screencapture` present.

**Procedure (schedule as a discrete, partly human-assisted plan step):**
1. **Ensure the loaded build is current** (per `[[vcv_build_install_workflow]]`): `make install RACK_DIR=../Rack-SDK`; if the panel looks stale, `rsync -a dist/ForgeAudio-AnalogSeries/res/ "<plugins>/ForgeAudio-AnalogSeries/res/"`, copy the fresh `plugin.dylib`, then **fully quit + relaunch Rack**. Diagnose staleness by comparing `shasum plugin.dylib` (repo) vs the installed copy. **Run on the main tree — no worktree isolation** (`../Rack-SDK` won't resolve otherwise).
2. **Load** the Analog LFO in Rack, zoom in, and **capture** the panel with `screencapture -R<x,y,w,h> docs/img/panel-raw.png` or ⌘⇧4 region select. (Human-assisted: the panel must be visually correct.)
3. **Bake numbered callouts** into the PNG. Tooling options (ImageMagick is NOT installed):
   - **Preview markup** (simplest, fully manual): add numbered badges + leader lines in macOS Preview.
   - **HTML-overlay + Playwright** (reproducible, tweakable — Playwright MCP + `.playwright-mcp/` present, matches the existing `*-mockup.html` workflow): place `panel-raw.png` as a CSS background, absolutely-position numbered callout divs, screenshot the rendered page. Callout coordinates are iterated against the actual screenshot (still human-in-the-loop for placement).
   - (If desired, `brew install imagemagick` enables `magick … -annotate`, but placement coords must still be tuned to the screenshot, so it saves little over the HTML approach.)
4. **Save** the final annotated PNG to `docs/img/` (e.g. `docs/img/panel-annotated.png`) and reference it with a **relative** path from the annotated-panel section.
5. **Legend/control-reference table** keyed 1:1 to the callout numbers (D-08 says this table doubles as the control reference). Suggested callout order (top→bottom, left→right; planner may group per discretion):

   1 Display · 2 Morph · 3 Character · 4 Drift · 5 Rate · 6 Phase Offset · 7 Morph-CV trim · 8 Character-CV trim · 9 Drift-CV trim · 10 FM-Depth trim · 11 Phase-Offset-CV trim · 12 Morph-CV jack · 13 Character-CV jack · 14 Drift-CV jack · 15 FM jack · 16 Phase-Offset-CV jack · 17 CLK jack · 18 RESET jack · 19 OUTPUT jack.

   Panel geometry note for placement (mm2px on an 18HP = 91.44mm × 128.5mm panel): hero Morph knob at (45.72, 61); secondary row y=87 (Character 18, Drift 36.24, Rate 54.48, Phase Offset 72.72); trimpot row y=108.5 (Morph 7.70, Character 18.56, Drift 29.43, FM 40.29, Phase Offset 51.15); jack row y=119.5 (…, CLK 62.01, RESET 72.88, OUTPUT 83.74). **Each trimpot sits directly above the jack it attenuates.**

**Automatable vs human-assisted:** build/install/dist = automatable; screenshot framing + callout placement = human-assisted (flag as such). This is the one step in the phase that cannot be fully autonomous.

## VCV Manifest Conventions (`manualUrl`)

`[VERIFIED: vcvrack.com/manual/Manifest]`

- **`manualUrl` is a documented plugin-level field:** *"The manual website of your plugin. E.g. HTML, PDF, GitHub readme, GitHub wiki."* A GitHub-hosted `docs/index.md` URL is explicitly the intended kind of target.
- It **also exists at module level** (*"If omitted, the plugin's manual is used"*). For a single-module plugin, the plugin-level field is sufficient; a module-level `manualUrl` is optional.
- **Placement:** add `manualUrl` as a top-level key in `plugin.json` alongside the existing `authorUrl`/`pluginUrl`/`sourceUrl`. It does **not** replace them (D-04) — they coexist.
- **Value:** point at the docs landing in the public repo, e.g. `https://github.com/Photep/ForgeAudio-AnalogSeries/blob/main/docs/index.md` (or `/tree/main/docs`). The exact URL should match how GitHub renders the chosen entry file. `[ASSUMED]` — confirm the branch (`main`) and the precise blob/tree path with the user.
- **Bonus (optional, Claude's discretion):** `changelogUrl` is also a documented plugin-level field (*"Link to the changelog of the plugin"*). Since a changelog section is being authored anyway, pointing `changelogUrl` at `docs/changelog.md` is a near-free win. Not required by D-04. `[ASSUMED]`
- **Manifest validation:** Phase 26 already locked `version` = `2.0.0`, `minRackVersion` = `2.0.0`, slug, tags, and confirmed no trademarked strings. Adding `manualUrl` is additive; re-run whatever manifest lint Phase 26 used to confirm JSON validity after the edit.

## Changelog Derivation (D-09)

No `CHANGELOG` file exists; derive from the milestone list in `ROADMAP.md` + `.planning/milestones/*-ROADMAP.md`, corroborated by the per-feature milestone tags in `PROJECT.md` §"Requirements → Validated". Structure by shipped milestone:

| Milestone | Shipped | Headline content (for the changelog entry) |
|-----------|---------|--------------------------------------------|
| **v1.0 Analog Series LFO** (Phases 1-6) | 2026-03-07 | Three-knob analog engine (morph/character/drift); 4-shape morph (sine→tri→saw→square); real-time waveform display w/ phase dot; bipolar ±5V output; CV inputs for morph/character/drift; 0.01–20 Hz rate; lock-free display double-buffer |
| **v1.1 Clock Sync** (Phases 7-10) | 2026-03-13 | CLK input + period tracking; dual-mode Rate (free Hz / 15 musical ratios); division-aware phase reset; 3ms anti-click crossfade; EMA period smoothing + outlier rejection; SYNC badge / ratio / BPM display; reduced drift authority when clocked |
| **v1.2 Deep Analog** (Phases 11-17) | 2026-03-17 | FM input (exponential); RESET trigger jack + 1ms blanking; Phase Offset knob (0–360°) + CV; Swing/shuffle (clocked only); expanded imperfections (phase jitter, DC wander, thermal slew); per-instance component spread w/ serialized seed; waveform bleed |
| **v1.3 Forge Noir** (Phases 18-21) | 2026-06-13 | 18HP Forge Noir panel + custom SVG components; 5th morph shape (narrow pulse / PWM folded into morph); three-column CRT display (corner brackets, scanlines, breathing glow); per-edge animated SYNC badge flash |
| **v1.4 Tempered** (Phases 22-28) | in progress (2026-07) | Release hardening: bug fixes, automated test harness + CI, DSP core extraction, IP hardening (font purge, GPL license), VCV Library compliance/packaging, **this manual**, public source release. *(Finalize the entry text at ship.)* |

**⚠ Version-vs-milestone pitfall:** the **manifest version is `2.0.0`** (VCV requires MAJOR = Rack major = 2), while the **marketing milestones are v1.0–v1.4**. The changelog must use the milestone names ("v1.0 Analog Series LFO", …), and should state once that the plugin's manifest version is `2.0.0` for Rack 2. Do **not** relabel milestones as "2.x" or "fix" the manifest version — REQUIREMENTS explicitly forbids changing it.

## Docs Structure (GitHub-correct today AND Pages-ready later)

`docs/` does not exist yet — greenfield. Recommended shape (file names are Claude's discretion per D-02; ordering below is suggested):

```
docs/
├── index.md              # hub: overview + links to every section (the "top-level page")
├── engine-concept.md     # 3-axis engine (morph/character/drift), generic vocabulary (D-06)
├── panel.md              # annotated-panel PNG + numbered legend/control-reference table (D-08)
├── io-reference.md       # 7 inputs / 1 output with verified CV ranges
├── context-menu.md       # Swing submenu (6 options) — the only menu item
├── clock-sync.md         # FREE/ACQUIRING/LOCKED behavior + ratio table
├── install.md            # VCV Library subscribe + manual .vcvplugin
├── changelog.md          # milestone-structured (D-09)
├── license-credits.md    # GPL-3.0 + third-party notices (from LICENSE / NOTICES / OFL.txt)
└── img/
    └── panel-annotated.png
```

**Relative-link conventions (render in GitHub view AND MkDocs/Docusaurus):**
- Link between sections with **relative paths including the `.md` extension**: `[I/O Reference](io-reference.md)`. GitHub renders these; MkDocs (`use_directory_urls`) and Docusaurus both accept `.md` links. `[CITED: docs.github.com — relative links in Markdown]`
- Reference images with **relative paths**: `![Annotated panel](img/panel-annotated.png)`. Works in GitHub blob view and in a Pages build. Avoid absolute repo URLs (break locally and on Pages).
- Keep `docs/index.md` as the single hub the `manualUrl` points at.
- **License/credits source material already exists:** `LICENSE` (GPL-3.0, repo root), `NOTICES` (third-party inventory, repo root), `res/fonts/OFL.txt` (JetBrains Mono OFL). The license-credits section should summarize + link these, not duplicate them.

## Roadmap / Requirements Reconciliation (specific edits the planner must schedule)

These are documentation edits (not runtime state). Apply verbatim-style edits:

**`REQUIREMENTS.md`:**
- **DOC-01:** replace *"published in Notion as a new top-level page with a subpage-per-section structure"* → *"authored as GitHub Markdown under `docs/` with a per-section file per table-stakes section and a `docs/index.md` hub (subpage-per-section preserved)."*
- **DOC-02:** remove **"patch examples"** from the table-stakes list (D-07). Keep all other sections.
- **DOC-03:** replace *"shared to web (public) and linked from `pluginUrl`"* → *"linked from `plugin.json` `manualUrl`; public reachability completes with the Phase 28 repo-public flip."*
- **Out of Scope table:** the *"Custom Notion site / custom domain hosting"* row → retarget to *"GitHub Pages site (MkDocs/Docusaurus) — deferred to a future docs revision; `docs/` is authored Pages-ready."*

**`ROADMAP.md` §"Phase 27: Notion Manual":**
- **Title:** *"Phase 27: Notion Manual"* → *"Phase 27: User Manual (GitHub Markdown)"* (or keep the historical name with a "(pivoted → GitHub Markdown)" note — planner discretion; both `ROADMAP.md` occurrences: the milestone checklist line ~84 and the Phase Details header line ~174).
- **Goal** (line ~175): *"lives in Notion"* → *"lives as GitHub Markdown under `docs/` in the (public) repo."*
- **Success Criteria:** #1 *"new top-level Notion page … subpage-per-section"* → *"`docs/` directory with per-section `.md` files + `docs/index.md` hub"*; #2 **remove "patch examples"** from the list; #3 *"shared to web … linked from `pluginUrl`/`manualUrl`"* → *"committed `docs/`; `manualUrl` added to `plugin.json`; public reachability completes with Phase 28."*
- Update the "Notion Manual may be drafted in parallel" note (line ~201) to drop "Notion" if desired (cosmetic).

**Note the "gsd_decision_coverage_gate" memory:** the plan-phase D-NN coverage gate reads plan frontmatter `must_haves`, not body tags — ensure each D-01…D-09 decision is surfaced in plan `must_haves`, not just prose.

## Validation Architecture

> nyquist_validation is not disabled in config → section included. This is a docs phase; "validation" = the manual's facts match the code.

### Test "framework"

There is no doc test framework. The realistic validation is a **code-vs-docs fact check**, partly automatable:

| Property | Value |
|----------|-------|
| Framework | none (docs phase) — grep-based fact check + human review |
| Quick check | `grep`-assert the ratio labels, swing labels, and voltage constants appear in the docs and match `src/` |
| Full check | Human read-through of every table against `src/AnalogLFO.cpp` before publish |

### Phase requirements → verification map

| Req | Behavior | Check | Automatable? |
|-----|----------|-------|--------------|
| DOC-01 | `docs/index.md` + per-section files exist | `test -f docs/index.md` and each section file | ✅ |
| DOC-02 | All (post-D-07) sections present, no patch-examples section | grep section headings; assert no "Patch Example" heading | ✅ |
| DOC-02 | Ratio table / swing labels / CV ranges match code | grep the 15 `RATIO_LABELS`, 6 `SWING_MENU_LABELS`, `±5V`, Schmitt `1.0`/`0.1`, depthScale `0.5`/`0.6` in both docs and `src/` | ✅ (mostly) |
| DOC-03 | `plugin.json` has `manualUrl`; JSON valid | `python3 -c "import json;print(json.load(open('plugin.json'))['manualUrl'])"` | ✅ |
| DOC-08 | Annotated PNG exists + referenced | `test -f docs/img/panel-annotated.png` + grep the relative link | ✅ (existence); placement = human |
| D-06 | No trademarked synth brand names in prose | grep a denylist (Moog, Minimoog, Roland, Juno, SH-101, Prophet, Oberheim, Korg, …) across `docs/` → must be empty | ✅ |

### Wave 0 gaps
- [ ] `docs/` directory + `docs/img/` do not exist yet — created in this phase.
- [ ] Recommend a tiny `tests/check_docs.sh` (or a checklist in the phase) implementing the grep asserts above, especially the **brand-name denylist grep** (directly enforces D-06 / Pitfall 4).

## Common Pitfalls

### Pitfall 1: Trademarked synth names leak into prose (D-06 / Pitfall 4)
**What goes wrong:** The Character knob's heritage is literally "Minimoog saw / Roland square / Moog-Prophet triangle" (see PROJECT.md Key Decisions), so it is very tempting to name them.
**How to avoid:** Use only generic terms ("vintage transistor saw", "classic PWM square", "warm analog triangle"). Enforce with a denylist grep over `docs/` in the validation step. This keeps the public GPL release + VCV submission clean.
**Warning signs:** any brand token in `docs/`.

### Pitfall 2: Manual facts drift from code
**What goes wrong:** Hand-typing ranges/ratios introduces errors (e.g., writing "±10V CV" or the wrong x1 index).
**How to avoid:** Transcribe the tables in this document verbatim; run the grep fact-check before publish. CV ranges MUST come from `AnalogLFO.cpp` (per the phase note).

### Pitfall 3: Using mockups or the SVG export instead of a real Rack screenshot (D-08)
**What goes wrong:** `forge-noir-*.png`, `panel-mockup.html`, and `res/AnalogLFO.svg` do NOT match the shipped module — the SVG lacks widget-owned knob art (knobs are rendered by the widget, not baked in).
**How to avoid:** Capture the live module in Rack. Verify the loaded build isn't stale (shasum the dylib) before shooting.

### Pitfall 4: Assuming the manualUrl is live during Phase 27
**What goes wrong:** The repo is still private until Phase 28; the `manualUrl` and all `docs/` links 404 for the public until then.
**How to avoid:** Author + commit in Phase 27; treat DOC-03's "shared to web" verification as completing with/after Phase 28. Do not block Phase 27 on link reachability.

### Pitfall 5: Relabeling milestones as 2.x or "fixing" the manifest version
**What goes wrong:** The manifest version is `2.0.0` (Rack-major rule); milestones are v1.0–v1.4. Conflating them corrupts both the changelog and the (already-locked) manifest.
**How to avoid:** Changelog uses milestone names; state the `2.0.0` manifest version once as context. Never edit `version` in `plugin.json`.

### Pitfall 6: Worktree isolation breaks the screenshot build
**What goes wrong:** GSD execute-phase defaults to worktrees; `../Rack-SDK` then won't resolve and the build fails.
**How to avoid:** Run any plan step that builds/installs on the **main tree** (no worktree isolation), per `[[vcv_build_install_workflow]]`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Changelog data | A new `CHANGELOG.md` invented from scratch | Milestone table above (from `ROADMAP.md` / `milestones/`) | D-09; single source of truth already exists |
| Control facts | Paraphrased-from-memory ranges | Verbatim transcription of the code tables in this doc | Prevents drift |
| License text | Re-authored license/credits | Link/summarize existing `LICENSE`, `NOTICES`, `res/fonts/OFL.txt` | Already produced in Phase 25 |
| Docs site | MkDocs/Docusaurus build now | Plain Markdown authored Pages-ready | Explicitly deferred (D-03) |

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| `make` | Build module for screenshot | ✓ | GNU Make 3.81 | — |
| `../Rack-SDK` | Build/install | ✓ | present (`plugin.mk`) | — |
| VCV Rack 2 (Free/Pro) | Load module → screenshot | ✓ | installed in `/Applications` | — |
| Built + installed plugin | Screenshot target | ✓ | `dist/…-2.0.0-mac-arm64.vcvplugin` + installed dir present | rebuild via `make install` |
| `screencapture` | Capture panel PNG | ✓ | macOS native | ⌘⇧4 |
| ImageMagick | Bake callouts | ✗ | — | **Preview markup** or **HTML-overlay + Playwright** (both present); or `brew install imagemagick` |
| Playwright | HTML-overlay callout render | ✓ (MCP + `.playwright-mcp/`) | — | Preview markup |
| `python3` | JSON validity check for `plugin.json` | assume ✓ (macOS) | — | `node -e` / manual |

**Missing dependencies with no fallback:** none.
**Missing dependencies with fallback:** ImageMagick (annotation) — use Preview or HTML+Playwright.

## Security Domain

**N/A.** No auth, input handling, cryptography, or network surface is introduced. The only "input" is the `manualUrl` string in `plugin.json` (a static, developer-authored URL). The one adjacent security-relevant concern is **IP hygiene** (no trademarked names — D-06), covered under Pitfall 1 and the validation denylist, not ASVS.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual in Notion (public share) | GitHub Markdown in-repo `docs/` | D-01 (this phase) | Versioned with plugin; VCV reviewers expect it; no external live dependency |
| Manual linked from `pluginUrl` | Dedicated `manualUrl` field | D-04 | Matches current VCV manifest schema (both are valid fields) |

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Exact `manualUrl` value / branch (`main`) and blob-vs-tree path | VCV Manifest Conventions | Broken link; easy to fix, but confirm the public URL shape with user |
| A2 | Optionally also set `changelogUrl` → `docs/changelog.md` | VCV Manifest Conventions | None if omitted; it's an optional enhancement, not in D-04 |
| A3 | `python3` present on the machine for the JSON check | Validation Architecture | Use `node`/manual if absent |
| A4 | Section file names/ordering as drafted | Docs Structure | Explicitly Claude's discretion (D-02) — no risk, just a suggestion |

## Open Questions (RESOLVED)

1. **manualUrl exact target** — `docs/index.md` blob URL vs `tree/main/docs`? Which renders best as the "manual landing"?
   - What we know: `manualUrl` accepts a GitHub readme/URL; the repo goes public in Phase 28.
   - Recommendation: point at `…/blob/main/docs/index.md`; confirm branch name with user during planning.
   - **RESOLVED (planning):** locked to `https://github.com/Photep/ForgeAudio-AnalogSeries/blob/main/docs/index.md` in 27-03 Task 1 (owner/repo/branch confirmed from `git remote` + existing manifest URLs).
2. **Callout annotation tool** — Preview (manual) vs HTML+Playwright (reproducible)?
   - Recommendation: HTML+Playwright fits the existing mockup-HTML workflow and gives tweakable coords, but either is acceptable; it's the one human-assisted step.
   - **RESOLVED (planning):** 27-04 Task 3 selects HTML-overlay + Playwright as primary (Preview markup as fallback); ImageMagick is absent.

## Sources

### Primary (HIGH confidence)
- `src/AnalogLFO.cpp` — ParamId/InputId/OutputId enums, RATIO_TABLE + RATIO_LABELS, SWING_MENU_LABELS, `configParam`/`configInput` labels+ranges, `process()` CV math, `appendContextMenu` (Swing only), panel widget geometry.
- `src/dsp/LfoCore.hpp` — per-sample chain, FM `exp2_taylor5` + depthScale 0.5/0.6, ±5V output, anti-click crossfade.
- `src/dsp/ClockTracker.hpp` — FREE/ACQUIRING/LOCKED transitions, Schmitt 0.1/1.0V, EMA α=0.3, fast-track 0.8–1.2×, ≥4 edges, outlier 3×/⅓×, OUTLIER_THRESHOLD=3, timeout formula.
- `plugin.json` — current manifest (slug, version 2.0.0, existing URLs, no manualUrl yet).
- `.planning/ROADMAP.md`, `.planning/REQUIREMENTS.md`, `.planning/PROJECT.md` — milestone list, DOC-01/02/03, feature-by-milestone tags.
- Environment probe — build toolchain, Rack app, dist artifact, screencapture, ImageMagick absence.
- `[[vcv_build_install_workflow]]` memory — build/install/stale-flush/no-worktree conventions.

### Secondary (verified)
- `[VERIFIED: vcvrack.com/manual/Manifest]` — `manualUrl` (plugin + module level), `changelogUrl`, `authorUrl`/`pluginUrl`/`sourceUrl`/`donateUrl` field definitions.

### Tertiary (general knowledge)
- GitHub-Flavored Markdown relative-link/image behavior; MkDocs/Docusaurus `.md`-link compatibility (`[CITED]`, standard docs practice).

## Metadata

**Confidence breakdown:**
- Code-sourced content (params/inputs/output/ratios/swing/clock): **HIGH** — read verbatim from source.
- VCV manifest conventions: **HIGH** — verified against official manifest docs.
- Annotated-panel procedure: **MEDIUM-HIGH** — environment confirmed present; callout placement inherently human-assisted.
- Changelog derivation: **HIGH** — milestone data present in-repo.
- Docs structure / Pages-readiness: **MEDIUM** — standard practice; no Pages build tested this phase (deferred by D-03).

**Research date:** 2026-07-09
**Valid until:** ~2026-08-09 (stable; VCV manifest schema and repo facts are slow-moving)
