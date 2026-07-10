# Phase 27: Notion Manual - Context

**Gathered:** 2026-07-09
**Status:** Ready for planning

<domain>
## Phase Boundary

Author and publish the Analog LFO user manual as a **public, sectioned, linked** reference document, then wire its URL into the plugin manifest. The manual covers the module's controls, I/O, engine concept, clock/sync behavior, context-menu options, install steps, changelog, and license/credits — verified against the actual code before publishing.

**Platform pivot (supersedes ROADMAP):** The manual ships as **GitHub Markdown in the repo**, NOT Notion. The phase name "Notion Manual" and the "Notion" wording in ROADMAP §Phase 27 and REQUIREMENTS DOC-01/02/03 are superseded by this decision (see D-01 and the deviation note below). The underlying requirements — a public, subpage/section-structured, manifest-linked manual verified against `AnalogLFO.cpp` — are unchanged; only the hosting platform changed.

</domain>

<decisions>
## Implementation Decisions

### Platform & Structure
- **D-01:** Manual lives as **GitHub Markdown in the (now-public) repo**, not Notion. Rationale: versioned with the plugin, single source of truth, no forever-live external dependency right as the repo goes public, and it's what VCV reviewers expect. Supersedes the roadmap's Notion decision.
- **D-02:** **Per-section Markdown files under `docs/`** (one file per table-stakes section) with a `docs/index.md` hub linking them — satisfies DOC-01's "subpage-per-section" intent and maps 1:1 to a future GitHub Pages nav.
- **D-03:** Author in a **Pages-ready shape** so a future MkDocs/Docusaurus upgrade is near-free: per-section files, committed image assets under `docs/img/` referenced by **relative links** (render correctly both in plain GitHub view today and in a Pages site later). No Pages build step is set up in this phase — structure only.

### Manifest Link
- **D-04:** Add a dedicated **`manualUrl`** field to `plugin.json` pointing at the repo docs (e.g. `docs/index.md` / the docs landing). **Keep** `pluginUrl`/`authorUrl`/`sourceUrl` on the GitHub repo. Matches VCV conventions; satisfies DOC-03. (Note: manual URL is only reachable once the repo is public in Phase 28 — see deferred/sequencing note.)

### Voice & Content Craft
- **D-05:** **Terse reference voice** — control tables, ranges, and behaviors stated as facts. No tutorial/hand-holding. Lean and scannable for the VCV audience.
- **D-06:** **Generic descriptive vocabulary for the Character knob** — describe sonic character with neutral terms ("vintage transistor saw", "classic PWM square", "warm analog triangle"). **No trademarked synth brand names anywhere** in the manual prose. Directly satisfies Pitfall 4 and keeps the public GPL release + VCV submission clean.
- **D-07:** **No patch-examples section.** User chose 0 examples, consistent with the terse-reference direction. **This amends DOC-02** (which lists "patch examples" as table-stakes) — see deviation note. Remaining table-stakes sections still all included.

### Annotated Panel
- **D-08:** The annotated-panel section uses a **real VCV Rack screenshot of the shipped module** with **numbered callouts baked into the PNG** (Markdown can't overlay callouts live), paired with a **numbered legend table**. The legend table **doubles as the control-reference table** (number → control → range/behavior). NOT the design-mockup PNGs (may not match shipped module) and NOT the SVG export (lacks widget-owned knob art).
  - **Dependency:** requires capturing the module running in Rack — build/install per [[vcv_build_install_workflow]], load in Rack, screenshot, annotate. This is a concrete deliverable step the plan must schedule (may be human-assisted for the capture).

### Changelog
- **D-09:** Derive changelog content **from milestone history** (`.planning/milestones/*-ROADMAP.md` + ROADMAP.md milestone list) — **no `CHANGELOG` file exists** in the repo. Structure by shipped milestone (v1.0 → v1.4).

### Claude's Discretion
- Exact per-section file naming/ordering under `docs/` (planner may finalize), install-section wording, and legend-table column layout are open within the decisions above.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements & Roadmap (contain the superseded "Notion" wording to update)
- `.planning/REQUIREMENTS.md` §DOC-01/DOC-02/DOC-03 — the three manual requirements; DOC-01/02/03 platform wording ("Notion") is superseded by D-01, and DOC-02's "patch examples" table-stakes item is amended by D-07.
- `.planning/ROADMAP.md` §"Phase 27: Notion Manual" — phase goal + success criteria; success-criteria wording referencing Notion is superseded by D-01/D-04.

### Source of truth for manual content (verify against code before publishing)
- `src/AnalogLFO.cpp` — authoritative for: param/input/output enums (10 params, 7 inputs, 1 output), the 15-entry `RATIO_TABLE`, the FREE/ACQUIRING/LOCKED clock state machine, and the single **Swing** context-menu submenu (6 options: Straight 50% / Light 54% / Medium 58% / Triplet 66% / Heavy 71% / Max 75%). **CV voltage ranges MUST be read from here** for the I/O reference (per phase note).
- `plugin.json` — current manifest: slug `ForgeAudio-AnalogSeries`, module slug `ForgeAnalogLFO`, version `2.0.0`, URLs all → GitHub repo, **no `manualUrl` yet** (added by D-04).
- `.planning/PROJECT.md` §"Context"/"Constraints" — brand identity (Forge Noir colors/fonts), panel size 18HP, feature summary for the engine-concept section.

### Brand/design reference for tone & naming
- `.planning/milestones/v1.0-ROADMAP.md` … `v1.4-ROADMAP.md` — milestone-by-milestone history for the changelog (D-09).

No other external specs — requirements fully captured in decisions above.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`src/AnalogLFO.cpp` enums + RATIO_TABLE:** direct source for the control-reference and I/O tables — no interpretation needed, transcribe verbatim (with CV ranges confirmed).
- **`res/AnalogLFO.svg` + `forge-noir-*.png` renders (repo root):** brand/visual reference; NOT the panel screenshot source (D-08 mandates a real Rack screenshot).
- **`.planning/milestones/*-ROADMAP.md`:** changelog source (D-09).

### Established Patterns
- **Widget-owned knob art (D-01, Phase 19/20.1):** why the SVG export can't be used for the annotated panel — knobs are rendered by the widget, not baked into the SVG. Confirms D-08's Rack-screenshot choice.
- **Context menu is minimal:** only the Swing submenu exists — the "context-menu options" section is short by design; do not invent options.

### Integration Points
- **`plugin.json`:** the one code file this phase edits — add `manualUrl` (D-04). Everything else is new `docs/` content.
- **Build/install to capture the screenshot:** follow [[vcv_build_install_workflow]] (relative `../Rack-SDK`, stale-install flush) to get the shipped module into Rack for the annotated-panel capture.

</code_context>

<specifics>
## Specific Ideas

- Manual is a **lean factual reference**, Forge Noir premium-but-practical, not a beginner modular tutorial.
- Section set (table-stakes, patch-examples removed per D-07): 3-axis engine concept · annotated panel · control-reference table (= panel legend) · I/O reference (with verified CV ranges) · context-menu options (Swing only) · clock/sync behavior · install (Library + manual `.vcvplugin`) · changelog · license/credits.
- Install section covers **both** paths: VCV Library install and manual `.vcvplugin` install.

</specifics>

<deferred>
## Deferred Ideas

- **GitHub Pages site (MkDocs/Docusaurus):** explicitly a *future version* upgrade, not this phase. The Pages-ready `docs/` structure (D-02/D-03) makes it near-free later. Do not set up a build step now.
- **Patch/tutorial examples:** dropped for this phase (D-07); could return in a future docs revision if desired.
- **Sequencing note (not a deferral, a hard constraint):** the `manualUrl` and manual are only publicly reachable once the repo is flipped public in **Phase 28**. Authoring/committing the docs and adding `manualUrl` happens in Phase 27; public reachability + the DOC-03 "shared to web" verification effectively completes alongside/after Phase 28's repo-public flip. Planner should note this dependency rather than assume the link is live mid-Phase-27.

**Roadmap/requirements deviations to reconcile (flag for planner + a roadmap update):**
1. **Platform: Notion → GitHub Markdown** (D-01). Update ROADMAP §Phase 27 title + success criteria and REQUIREMENTS DOC-01/02/03 wording.
2. **DOC-02 amended: drop "patch examples"** from the table-stakes list (D-07).
3. **Add `manualUrl`** as the manifest link field (D-04) — DOC-03 currently says "linked from `pluginUrl`".

</deferred>

---

*Phase: 27-Notion-Manual*
*Context gathered: 2026-07-09*
