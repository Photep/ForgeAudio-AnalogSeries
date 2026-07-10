---
phase: 27-notion-manual
plan: 01
subsystem: docs
tags: [markdown, github-docs, vcv-rack, manual, reference]

# Dependency graph
requires:
  - phase: 27-notion-manual (research/context/ui-spec)
    provides: D-01/D-02/D-05/D-06/D-07 doc decisions + verbatim code-fact tables
provides:
  - docs/ manual scaffold with docs/index.md hub
  - engine-concept, io-reference, context-menu, clock-sync reference sections
  - relative .md cross-link convention for the manual
affects: [27-02, 27-03, 27-04, panel-annotation, install, changelog, license-credits, manualUrl]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Per-section GitHub Markdown under docs/ with a docs/index.md hub (D-01/D-02)"
    - "Cross-links use relative paths WITH the .md extension (Pages-ready, D-03)"
    - "Every control/IO/ratio/swing/FSM fact lives in a pipe table, terse voice (D-05)"
    - "Generic Character vocabulary; zero trademarked synth brand names (D-06)"

key-files:
  created:
    - docs/index.md
    - docs/engine-concept.md
    - docs/io-reference.md
    - docs/context-menu.md
    - docs/clock-sync.md
  modified: []

key-decisions:
  - "Manual authored as GitHub Markdown under docs/ (not Notion) per D-01/D-02"
  - "CV ranges, ratio table, swing labels, and clock FSM transcribed verbatim from src/AnalogLFO.cpp + src/dsp/ClockTracker.hpp (Pitfall 2 drift guard)"
  - "index.md links all 8 sections incl. sections authored by later plans (panel/install/changelog/license-credits); no patch-examples link (D-07)"

patterns-established:
  - "docs/ hub-and-spoke: index.md is the manualUrl landing linking every section by relative .md path"
  - "Reference tables sourced 1:1 from code enums/constants, not paraphrased"

requirements-completed: []  # DOC-01/DOC-02 span all 4 phase plans; not complete after plan 01 (panel/install/changelog/license + manualUrl land in 27-02..27-04). Left unmarked to keep state accurate.

# Metrics
duration: ~8min
completed: 2026-07-09
---

# Phase 27 Plan 01: Manual Scaffold + Code-Fact Sections Summary

**docs/ GitHub-Markdown manual scaffold with a hub and four verbatim-from-source reference sections (engine concept, I/O with CV ranges, Swing menu, clock/sync FSM + 15-ratio table).**

## Performance

- **Duration:** ~8 min
- **Completed:** 2026-07-09
- **Tasks:** 3
- **Files created:** 5

## Accomplishments
- Created the greenfield `docs/` manual scaffold with `docs/index.md` as the hub linking all 8 sections via relative `.md` paths (no patch-examples link, D-07).
- Authored `docs/engine-concept.md` documenting all three axes (Morph/Character/Drift) in generic vocabulary — brand-name denylist grep returns zero hits (D-06).
- Authored `docs/io-reference.md` — 7 inputs + 1 output with CV convention `value = knob + trim × (CV/5V)`, ±5V/5V-per-unit, Schmitt 1.0V/0.1V, FM `depthScale` 0.6 free / 0.5 clocked, ±5V output + 3ms cosine crossfade — all transcribed verbatim from `src/AnalogLFO.cpp`/`LfoCore.hpp`.
- Authored `docs/context-menu.md` — the Swing submenu with exactly the 6 code options and no invented items.
- Authored `docs/clock-sync.md` — FREE/ACQUIRING/LOCKED FSM (fast-track 0.8×–1.2×, ≥4-edge lock, outlier >3×/<⅓× + 3-consecutive re-acquire, timeout formula) and the 15-entry ratio table (x1 = index 7), transcribed verbatim from `src/dsp/ClockTracker.hpp` + `RATIO_TABLE`.

## Task Commits

Each task was committed atomically:

1. **Task 1: docs hub + engine-concept** - `49bbfb1` (docs)
2. **Task 2: I/O reference + context-menu** - `145703c` (docs)
3. **Task 3: clock/sync FSM + ratio table** - `8c4eb7d` (docs)

**Plan metadata:** committed with this SUMMARY (docs: complete plan)

## Files Created/Modified
- `docs/index.md` - Manual hub: H1 title + intro + relative `.md` link list to all 8 sections
- `docs/engine-concept.md` - 3-axis engine concept (Morph/Character/Drift), generic vocabulary
- `docs/io-reference.md` - 7 inputs / 1 output with verified CV ranges from source
- `docs/context-menu.md` - Swing submenu (6 options), the only context-menu item
- `docs/clock-sync.md` - FREE/ACQUIRING/LOCKED FSM + 15-entry ratio table

## Decisions Made
- **Requirements DOC-01/DOC-02 left unmarked:** these are phase-spanning (require the panel, install, changelog, license-credits sections + `manualUrl` from plans 27-02..27-04). Marking them complete after plan 01 would misrepresent state; later plans / verify-work own final completion.
- Sections authored by later plans (panel.md, install.md, changelog.md, license-credits.md) are intentionally pre-linked from the hub now so the hub is stable; those targets land in 27-02..27-04.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None. `gsd-sdk` state handlers use `--named` args (record-metric `--phase/--plan/--duration/--tasks/--files`, add-decision `--summary`); adjusted invocation accordingly.

## Known Stubs
The hub links four sections not yet authored (panel.md, install.md, changelog.md, license-credits.md). These are not stubs in the data-flow sense — they are scheduled deliverables of plans 27-02..27-04 (D-08 panel, install/changelog/license, `manualUrl`). No empty-value/placeholder data rendering is involved.

## User Setup Required
None - no external service configuration required. Public reachability of the docs completes with the Phase 28 repo-public flip (not this phase).

## Next Phase Readiness
- Hub + four code-fact sections are stable and cross-linked; 27-02..27-04 can author the remaining sections (annotated panel, install, changelog, license-credits) and add `manualUrl` to `plugin.json`.
- No blockers.

## Self-Check: PASSED

All 5 docs files + SUMMARY.md exist on disk; all task commits (`49bbfb1`, `145703c`, `8c4eb7d`) and the metadata commit (`baf21e7`) present in git history.

---
*Phase: 27-notion-manual*
*Completed: 2026-07-09*
