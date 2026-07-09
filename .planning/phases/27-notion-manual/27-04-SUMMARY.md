---
phase: 27-notion-manual
plan: 04
subsystem: docs
tags: [annotated-panel, screenshot, playwright, panel-svg, attenuator-defaults, end-user-docs]

# Dependency graph
requires:
  - phase: 27-notion-manual (27-01)
    provides: docs/ manual hub + section scaffold that panel.md joins
provides:
  - "docs/panel.md — annotated-panel section + 19-row legend/control-reference table"
  - "docs/img/panel-annotated.png — real Rack screenshot with 19 baked numbered callouts (D-08)"
  - "docs/img/panel-raw.png — the source Rack screenshot"
  - "panel-overlay.html — reproducible callout-baking source"
affects: [phase-28-publish-submit, verify-work]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Annotated-panel baking: HTML-overlay + Playwright; transform (px = L + mm*ppm) auto-derived from the screenshot by detecting the OUTPUT ember ring (mm 83.74,119.5) + display left border (mm 5); control mm coords transcribed from src/AnalogLFO.cpp"

key-files:
  created:
    - docs/panel.md
    - docs/img/panel-raw.png
    - docs/img/panel-annotated.png
    - panel-overlay.html
  modified:
    - res/AnalogLFO.svg
    - src/AnalogLFO.cpp
    - docs/clock-sync.md
    - docs/io-reference.md
    - docs/engine-concept.md
    - docs/changelog.md

key-decisions:
  - "Callouts follow the actual wiring/legend, not the (previously mislabeled) panel print"
  - "Fixed root cause: swapped FM/PHASE trim labels in res/AnalogLFO.svg (user-directed deviation)"
  - "Changed all CV attenuators to default 0% for consistency + predictable behavior (user-directed deviation)"
  - "Rewrote the manual for end-user voice — no code/jargon/bug-fix references (user feedback)"

patterns-established:
  - "Manual voice: plain-language, user-observable behavior only; see memory docs_end_user_voice"

requirements-completed: [DOC-02]

# Metrics
duration: ~90min (interactive checkpoint)
completed: 2026-07-10
---

# Phase 27 Plan 04: Annotated Panel Summary

**Produced the D-08 annotated-panel deliverable — a real VCV Rack screenshot of the shipped ForgeAnalogLFO with 19 numbered ember-ring callouts baked in and keyed 1:1 to the panel.md legend — and, along the way, fixed a discovered panel label bug, changed the CV attenuator defaults, and rewrote the manual in an end-user voice.**

## Accomplishments
- **Task 1 (build/install):** `make install RACK_DIR=../Rack-SDK` on the main tree; built vs installed `plugin.dylib` shasums matched after a stale-flush.
- **Task 2 (panel.md legend):** authored `docs/panel.md` — image embed + a 19-row legend/control-reference table transcribed from `src/AnalogLFO.cpp`.
- **Task 3 (annotated image):** captured a real Rack screenshot (user-provided, corrected labels, uniform attenuators) and baked 19 callouts via `panel-overlay.html` + Playwright. Callouts sit outside the panel silhouette with leaders pointing in, per the UI-SPEC badge anatomy.

## Deviations (user-directed)
- **Panel label bug fix** — capturing the panel revealed the SVG printed `PHASE`/`FM` swapped over the 4th/5th CV columns vs. the wiring (confirmed against `configParam` tooltips + `res/PANEL-SPEC.md`). Swapped the two label outlines in `res/AnalogLFO.svg`, rebuilt, reinstalled.
- **Attenuator defaults → 0%** — the four additive CV attenuators defaulted to 100% (FM to 0%); changed all to 0% for consistency and predictable modulation-on-patch. Behavior-only; 50/50 tests pass.
- **End-user voice rewrite** — stripped FSM/EMA/Schmitt/Ornstein-Uhlenbeck/PWM, code identifiers, formulas, and internal bug-fix references from clock-sync/io-reference/engine-concept/changelog/panel; kept user-visible facts. Recorded as memory `docs_end_user_voice`.

## Task Commits
1. **Panel label fix** - `f2c9123` (fix)
2. **Attenuator defaults → 0%** - `6f0a8c4` (change)
3. **Panel.md legend authoring** - `b3df4ba` (feat, during Task 2)
4. **End-user voice docs rewrite** - `b0d071f` (docs)
5. **Annotated panel image + overlay** - `6e6b340` (feat)

## Verification
- Built vs installed dylib shasums matched (current build captured).
- `docs/img/panel-annotated.png` (1414×1600) + `panel-raw.png` exist; annotated PNG has 19 callouts keyed 1:1 to the legend.
- `docs/panel.md` embeds the annotated PNG by relative path; zero brand-name hits; `tests/check_docs.sh` PASS.
- Human reviewer confirmed callout placement ("approved").
