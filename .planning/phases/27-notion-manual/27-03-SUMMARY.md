---
phase: 27-notion-manual
plan: 03
subsystem: docs
tags: [plugin.json, manualUrl, vcv-manifest, roadmap, requirements]

# Dependency graph
requires:
  - phase: 27-notion-manual (27-01/27-02)
    provides: docs/ manual hub (index.md) + changelog.md that manualUrl/changelogUrl point at
provides:
  - "plugin.json manualUrl -> docs/index.md blob URL (D-04)"
  - "plugin.json changelogUrl -> docs/changelog.md (optional A2)"
  - "ROADMAP Phase 27 reconciled Notion -> GitHub Markdown (D-01), patch-examples dropped (D-07)"
  - "REQUIREMENTS DOC-01/02/03 + Out-of-Scope reconciled to the locked decisions"
affects: [phase-28-publish-submit, verify-work]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "VCV manifest links: manualUrl/changelogUrl coexist with authorUrl/pluginUrl/sourceUrl (do not replace)"

key-files:
  created: []
  modified:
    - plugin.json
    - .planning/ROADMAP.md
    - .planning/REQUIREMENTS.md

key-decisions:
  - "manualUrl locked to https://github.com/Photep/ForgeAudio-AnalogSeries/blob/main/docs/index.md (owner/repo/branch from existing manifest URLs)"
  - "Also set changelogUrl -> docs/changelog.md (A2 optional win; changelog.md already authored in 27-02)"
  - "DOC status checkboxes left untouched — verify-work owns phase completion"

patterns-established:
  - "Manifest link fields are additive: new URL fields coexist with the existing repo URLs"

requirements-completed: [DOC-01, DOC-02, DOC-03]

# Metrics
duration: 2min
completed: 2026-07-09
---

# Phase 27 Plan 03: manualUrl + ROADMAP/REQUIREMENTS Reconciliation Summary

**Added `manualUrl` (+ `changelogUrl`) to a valid `plugin.json` (version untouched at 2.0.0) and reconciled ROADMAP §Phase 27 and REQUIREMENTS DOC-01/02/03 from the Notion plan to the GitHub-Markdown `docs/` pivot with patch-examples dropped.**

## Performance

- **Duration:** ~2 min
- **Started:** 2026-07-09T07:50:07Z
- **Completed:** 2026-07-09T07:52:12Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- `plugin.json` gained a top-level `manualUrl` pointing at `docs/index.md` (D-04), with `authorUrl`/`pluginUrl`/`sourceUrl` all retained and `version` unchanged at `2.0.0` (Pitfall 5).
- Optional `changelogUrl` -> `docs/changelog.md` added (A2) — both target files already exist from 27-01/27-02.
- ROADMAP §Phase 27 retitled "Notion Manual" -> "User Manual (GitHub Markdown)" (milestone checklist line, Phase Details header, and progress-table row); Goal + Success Criteria rewritten to `docs/` + `docs/index.md` hub + `manualUrl` with Phase 28 reachability note; patch-examples removed (D-07); parallel-drafting note de-Notioned.
- REQUIREMENTS DOC-01 (GitHub Markdown under `docs/` + hub), DOC-02 (patch-examples dropped), DOC-03 (`manualUrl` + Phase 28 reachability) reworded; Out-of-Scope row retargeted from "Custom Notion site" to deferred GitHub Pages site.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add manualUrl to plugin.json** - `26b2502` (feat)
2. **Task 2: Reconcile ROADMAP §Phase 27** - `50c0b61` (docs)
3. **Task 3: Reconcile REQUIREMENTS DOC-01/02/03 + Out of Scope** - `993fad8` (docs)

## Files Created/Modified
- `plugin.json` - Added `manualUrl` + `changelogUrl` top-level fields; all prior URLs and version 2.0.0 retained.
- `.planning/ROADMAP.md` - Phase 27 title/goal/success-criteria + progress-table row + parallel note reconciled to the GitHub-Markdown pivot.
- `.planning/REQUIREMENTS.md` - DOC-01/02/03 wording + Out-of-Scope row reconciled.

## Decisions Made
- Kept `manualUrl` on the `blob/main/docs/index.md` form (renders as the manual landing on GitHub) rather than `tree/main/docs` — matches the locked planning decision.
- Added `changelogUrl` (A2, optional) since `docs/changelog.md` was already authored in 27-02 — near-free manifest enhancement.
- Left DOC-01/02/03 status checkboxes as-is (DOC-01/02 `[x]`, DOC-03 `[ ]`) per the plan — verify-work owns completion.

## Deviations from Plan

None - plan executed exactly as written. (The only optional element, `changelogUrl`, was explicitly sanctioned by the plan action / A2 and its target file exists.)

## Issues Encountered
- The first ROADMAP Phase-Details Edit failed because the `old_string` omitted the "Listed as a discrete phase." sentence from the Depends-on line; re-issued with the exact text. No content impact.

## Threat Flags

None. The only security-relevant surface is the static developer-authored `manualUrl`/`changelogUrl` strings; owner/repo/branch match the existing verified `pluginUrl`/`sourceUrl` (Photep/ForgeAudio-AnalogSeries, main) — T-27-06 mitigated. JSON validity asserted post-edit (T-27-05). No new endpoints, auth paths, or file access introduced.

## Known Stubs

None. `manualUrl`/`changelogUrl` resolve to real committed files (`docs/index.md`, `docs/changelog.md`); public reachability of the URLs completes with the Phase 28 repo-public flip by design (T-27-07 accepted — not a stub).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- 27-03 done. Remaining Phase 27 work: 27-04 (annotated-panel: build/screenshot + numbered-callout PNG + legend/control-reference table — human-assisted).
- `manualUrl` is committed but resolves publicly only after the Phase 28 repo-public flip (sequencing constraint, by design — no live URL check attempted this phase).

---
*Phase: 27-notion-manual*
*Completed: 2026-07-09*
