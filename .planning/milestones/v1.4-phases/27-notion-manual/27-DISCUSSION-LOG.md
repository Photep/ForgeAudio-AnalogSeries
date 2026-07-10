# Phase 27: Notion Manual - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-07-09
**Phase:** 27-Notion-Manual
**Areas discussed:** Platform choice, Annotated panel, Voice, Trademark-safe language, Patch examples, Manifest link

---

## Platform Choice (user-initiated — "Is Notion the best platform?")

| Option | Description | Selected |
|--------|-------------|----------|
| GitHub Markdown | Per-section `docs/` files, versioned, single source of truth, no external dependency, Pages-ready | ✓ |
| Notion (as roadmapped) | Rich editor via MCP, best default polish, external forever-live dependency | |
| GitHub Pages site | Static site (MkDocs/Docusaurus), best presentation + versioned, build step to maintain | |

**User's choice:** GitHub Markdown, Pages-ready structure (per-section files + `docs/img/` relative links + `docs/index.md` hub).
**Notes:** User asked whether Notion was the right platform and whether they could migrate to GitHub Pages later. Confirmed migration is near-free from Markdown; chose GitHub MD now with a Pages-ready shape. This supersedes the roadmap's Notion decision.

---

## Annotated Panel

| Option | Description | Selected |
|--------|-------------|----------|
| Real Rack screenshot + baked numbers | Launch module in Rack, screenshot, overlay numbered callouts + legend table | ✓ |
| Reuse forge-noir PNG render | Existing design mockups with overlays — may not match shipped module | |
| SVG panel export + numbers | Clean vector but lacks widget-owned knob art — misleading | |

**User's choice:** Real Rack screenshot with baked-in numbered callouts.
**Notes:** Numbered legend doubles as the control-reference table. Adds a "capture module running in Rack" dependency.

---

## Voice

| Option | Description | Selected |
|--------|-------------|----------|
| Reference + light tutorial | Concise reference + short how-to framing per section | |
| Terse reference only | Just the facts — tables, ranges, behaviors | ✓ |
| Full tutorial / hand-holding | Walkthrough-heavy, teaches modular from scratch | |

**User's choice:** Terse reference only.

---

## Trademark-safe Language (Character knob)

| Option | Description | Selected |
|--------|-------------|----------|
| Generic descriptive only | Neutral vocabulary, no brand names anywhere | ✓ |
| Descriptive + 'inspired by' asides | Occasional "reminiscent of Moog-style…" framing | |
| Name them as sonic references | Explicit brand names as references — highest exposure | |

**User's choice:** Generic descriptive only. Satisfies Pitfall 4; safest for public GPL + VCV submission.

---

## Patch Examples

| Option | Description | Selected |
|--------|-------------|----------|
| Curated 4 | Filter sweep, clocked wobble, drifting drone, swing groove | |
| Broad 6 | Curated 4 + FM-shaped LFO + phase-offset dual-mod | |
| Minimal 2 | One free-run + one clocked | |
| **0 (user free-text)** | No patch-examples section at all | ✓ |

**User's choice:** 0 patch examples.
**Notes:** Consistent with terse-reference direction. Contradicts DOC-02 (lists patch examples as table-stakes); user confirmed the amendment to drop the section and the DOC-02/roadmap update.

---

## Manifest Link

| Option | Description | Selected |
|--------|-------------|----------|
| Add manualUrl → docs, keep pluginUrl on repo | Dedicated field, matches VCV conventions | ✓ |
| Repoint pluginUrl → docs | Fewer fields, conflates plugin page with manual | |

**User's choice:** Add `manualUrl` → docs; keep `pluginUrl`/`authorUrl`/`sourceUrl` on the repo.

---

## Claude's Discretion

- Exact `docs/` file naming and section ordering.
- Install-section wording (covers both Library + manual `.vcvplugin`).
- Legend/control-reference table column layout.
- Changelog granularity (structured by shipped milestone v1.0 → v1.4).

## Deferred Ideas

- GitHub Pages site (MkDocs/Docusaurus) — future-version upgrade, structure made near-free by the Pages-ready `docs/` layout.
- Patch/tutorial examples — dropped now, could return in a future docs revision.
- Public reachability of `manualUrl` depends on the Phase 28 repo-public flip (sequencing constraint, not a deferral).
