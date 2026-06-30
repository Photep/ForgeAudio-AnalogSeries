---
phase: 25-release-ip-hardening-private
plan: 01
subsystem: infra
tags: [licensing, gpl-3.0, ofl, packaging, makefile, gitignore]

requires:
  - phase: 24-dsp-extraction-display-refactors
    provides: green 47/47 Rack-free test suite the IP changes must not regress
provides:
  - Trial fonts removed from the working tree and gitignored (IP-01)
  - Verbatim GPL-3.0 LICENSE at repo root, shipped in dist (PKG-01)
  - NOTICES third-party inventory + verbatim SIL OFL 1.1 res/fonts/OFL.txt, both shipped (PKG-04)
  - Makefile DISTRIBUTABLES wired to ship NOTICES
affects: [25-03 history-purge, 28-public-flip, release-packaging]

tech-stack:
  added: []
  patterns: [verbatim-license-text-fetched-from-authoritative-source]

key-files:
  created: [LICENSE, NOTICES, res/fonts/OFL.txt]
  modified: [.gitignore, Makefile]

key-decisions:
  - "LICENSE fetched verbatim from gnu.org/licenses/gpl-3.0.txt (674 lines); editing voids the grant"
  - "OFL.txt is the exact OFL.txt that travels with JetBrains Mono (its copyright header + SIL OFL 1.1 body)"
  - "OFL.txt ships via the existing 'DISTRIBUTABLES += res' glob (lives under res/fonts/); only NOTICES needed a new glob"
  - "plugin.json left untouched — license/version already correct; PKG-02/03 are Phase 26"

patterns-established:
  - "Reversible IP cleanup lands + is committed before the irreversible 25-03 history purge"

requirements-completed: [IP-01, PKG-01, PKG-04]

duration: ~10min
completed: 2026-07-01
---

# Phase 25 Plan 01: Legal Clean Tree Summary

**The working tree is now legally clean and complete for a public GPL-3.0 release: trial fonts gone + gitignored, verbatim GPL-3.0 LICENSE + NOTICES + OFL.txt present and verified shipping in the dist artifact, with tests still 47/47 green.**

## Performance

- **Duration:** ~10 min
- **Tasks:** 3 completed
- **Files modified:** 5 (3 created, 2 modified) + 2 deleted

## Accomplishments

### Task 1 — Remove trial fonts + gitignore (IP-01)
- `git rm BCBarellTEST-Regular.otf FoundationLogo.ttf` (both mockup-only, referenced by no build/source/SVG)
- Appended both names to `.gitignore` under a "purged in Phase 25" comment
- `res/fonts/JetBrainsMonoNL-Regular.ttf` (legitimate OFL runtime font) left untouched
- Commit: `2e28b1f`

### Task 2 — LICENSE + NOTICES + OFL.txt + Makefile (PKG-01, PKG-04)
- `LICENSE`: verbatim FSF GPL-3.0 (674 lines, gnu.org)
- `res/fonts/OFL.txt`: verbatim SIL OFL 1.1 (the JetBrains Mono OFL.txt)
- `NOTICES`: inventories plugin GPL-3.0, JetBrains Mono (OFL → OFL.txt), and Bebas Neue + Chakra Petch wordmark outline provenance (OFL)
- `Makefile`: added `DISTRIBUTABLES += $(wildcard NOTICES*)` after the LICENSE glob
- `plugin.json` unchanged (verified no diff)
- Commit: `be4684f`

### Task 3 — Build/dist sanity gate (regression + PKG-01/PKG-04)
- `make test`: **47/47 passed** (2,590,445 assertions), exit 0 — no regression
- `make dist` against `../Rack-SDK`: succeeded, produced `ForgeAudio-AnalogSeries-2.0.0-mac-arm64.vcvplugin`
- Dist artifact verified (zstd tar):
  - Contains `LICENSE`, `NOTICES`, `res/fonts/OFL.txt` (OFL.txt sits beside `JetBrainsMonoNL-Regular.ttf`) ✓
  - Contains **no** trial fonts (`grep -iE 'Barell|FoundationLogo'` empty) ✓
- Dist assertion was run live (Rack-SDK present) — NOT operator-deferred.

## Self-Check: PASSED

All five must_have truths satisfied; all artifacts present with required markers; all key_links (NOTICES + res globs) verified in the actual dist artifact.

## Notes for downstream

- This corrected tree is what 25-03's push-first invariant must preserve: 25-01's commits (`2e28b1f`, `be4684f`) must be on local `main` and pushed to origin BEFORE the history purge.
- Live divergence at execution time: `git rev-list --count origin/main..HEAD` ≈ 79+ (re-measure live in 25-03; do not assume).
