---
phase: 27-notion-manual
plan: 02
subsystem: docs
tags: [documentation, install, changelog, license, validation-gate]
requires:
  - "docs/ hub + wave-1 section files (27-01)"
  - "LICENSE, NOTICES, res/fonts/OFL.txt (Phase 25)"
  - "milestone history (ROADMAP.md, milestones/*, PROJECT.md)"
provides:
  - "docs/install.md — VCV Library + manual .vcvplugin install paths"
  - "docs/changelog.md — milestone-structured changelog v1.0–v1.4"
  - "docs/license-credits.md — GPL-3.0 summary + third-party notices links"
  - "tests/check_docs.sh — brand-name denylist + docs fact-check gate (D-06)"
affects:
  - "27-04 (panel.md added later; gate intentionally does not require it yet)"
  - "Phase 28 public flip (docs become publicly readable)"
tech-stack:
  added: []
  patterns:
    - "GitHub-Flavored Markdown, relative links to repo-root legal files"
    - "bash grep-based denylist + fact-check as the single automatable doc gate"
key-files:
  created:
    - docs/install.md
    - docs/license-credits.md
    - docs/changelog.md
    - tests/check_docs.sh
  modified: []
decisions:
  - "License/credits summarizes + links LICENSE/NOTICES/OFL.txt; no re-authored license body (Don't Hand-Roll / T-27-03)"
  - "Changelog uses milestone names v1.0–v1.4, states manifest 2.0.0 once as context, no 2.x relabeling (D-09 / Pitfall 5)"
  - "check_docs.sh requires the 7 wave-1 section files only (not panel.md), so it PASSES at wave 1"
metrics:
  duration: ~7 min
  completed: 2026-07-09
---

# Phase 27 Plan 02: Install, Changelog, License/Credits + Docs Gate Summary

Authored the install, changelog, and license/credits reference sections and stood up `tests/check_docs.sh` — the brand-name denylist + code-fact gate that closes the Wave 0 validation gap and enforces D-06/Pitfall 4 automatically.

## What Was Built

**Task 1 — install + license/credits** (`4d0ac74`)
- `docs/install.md`: `##` VCV Library primary path (subscribe + sync) and `##` Manual install secondary path (download `.vcvplugin`, place in Rack user plugins folder, relaunch), plus the "Module not appearing? Fully quit and relaunch Rack…" troubleshooting line. Terse reference voice (D-05).
- `docs/license-credits.md`: states GPL-3.0-or-later, relative-links `../LICENSE`, `../NOTICES`, `../res/fonts/OFL.txt`; a Third-party notices table summarizing JetBrains Mono (bundled OFL font) and the Bebas Neue / Chakra Petch panel wordmark outlines (confirmed-OFL per Phase 25). Summarize + link only — no re-authored license text (T-27-03).

**Task 2 — changelog** (`bf98945`)
- `docs/changelog.md`: one `##` entry per shipped milestone using milestone names — "v1.0 Analog Series LFO", "v1.1 Clock Sync", "v1.2 Deep Analog", "v1.3 Forge Noir", "v1.4 Tempered" — each with headline feature bullets from RESEARCH §Changelog Derivation. Manifest version `2.0.0` stated exactly once as Rack-2 context; v1.4 marked in-progress (release hardening + manual + public source release). No milestone relabeled 2.x (Pitfall 5).

**Task 3 — docs gate** (`7846568`)
- `tests/check_docs.sh` (executable, `set -euo pipefail`): (1) case-insensitive denylist grep over `docs/` for Minimoog/Moog/Roland/Juno/SH-101/Prophet/Oberheim/Korg — non-zero exit on any hit (D-06); (2) existence asserts for `docs/index.md` + the 7 wave-1 section files, deliberately excluding `docs/panel.md` (authored in 27-04); (3) fact asserts for the 6 Swing labels, ratio labels `/16`, `/1.5`, `x1.5`, `x16`, and the `5V` CV token. Prints per-check OK/FAIL and a PASS/FAIL summary line. Resolves paths relative to the script so it runs from anywhere.

## Verification

- Per-task automated `<verify>` blocks all passed.
- `bash tests/check_docs.sh` → **PASS** (EXIT=0): zero denylist hits, all 8 files present, all 11 fact tokens found.
- Changelog: all of v1.0–v1.4 present, `2.0.0` present, `! grep -qE '"?2\.[1-4]\b'` holds (no 2.x relabel).

## Deviations from Plan

None — plan executed exactly as written.

## Threat Model Coverage

- **T-27-02 (Tampering / IP)** — mitigated: `check_docs.sh` denylist fails on any trademarked synth brand across `docs/`; currently zero hits.
- **T-27-03 (Repudiation / attribution)** — mitigated: license-credits summarizes + links the authoritative LICENSE/NOTICES/OFL.txt, single-sourced.
- **T-27-04 (Information disclosure)** — mitigated: install/changelog carry public feature/install facts only; no internal paths, secrets, or PII.

## Known Stubs

None — all four files are complete, wired to real source facts and existing legal files. `panel.md` is intentionally out of scope for this plan (owned by 27-04, wave 2) and the gate is designed to pass without it.

## Self-Check: PASSED

All 5 files exist; all 3 task commits present in history.
