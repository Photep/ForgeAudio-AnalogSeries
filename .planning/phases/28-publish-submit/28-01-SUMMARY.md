---
phase: 28-publish-submit
plan: 01
status: complete
completed: 2026-07-10
requirements: [PUB-01, PUB-02]
---

# 28-01 Summary — Finalize release tree (still PRIVATE)

## What was done

Brought the release + docs work onto `main`, tagged `v2.0.0`, and pushed both to the
still-PRIVATE remote. No public exposure occurred in this plan.

- **Task 1 (decision):** Operator selected **ff-only** merge strategy. A clean
  fast-forward was verified available (`git merge-base --is-ancestor main HEAD` exit 0),
  so the tag lands on the exact reviewed release commit rather than a synthetic merge.
- **Task 2 (merge + push):** `git checkout main` → `git merge --ff-only chore/finalise-18hp-docs-and-cleanup`
  fast-forwarded `main` `4b27436 → 4d7b0a8`, then `git push origin main`. Verified both
  `docs/index.md` and `docs/changelog.md` are present on `main` (manualUrl + changelogUrl
  targets) and that remote `main` == local `main`. Visibility confirmed still **PRIVATE**.
- **Task 3 (tag + capture):** Created annotated tag `v2.0.0`
  (`Forge Audio Analog Series v2.0.0 — first VCV Library release`) on the pushed `main`
  commit, pushed it (`git push origin v2.0.0`), and dereferenced it to the release commit.

## Key result

- **v2.0.0 release commit hash (40-char):** `4d7b0a81f7aabed83626a11951956fff173b6ad7`
  - Captured via `git rev-parse v2.0.0^{commit}` (tag object SHA is `375de13…`; always dereference).
  - Confirmed on `main` (`git merge-base --is-ancestor <hash> main` exit 0).
  - This is the exact build ref the VCV Library submission (28-03) must carry — never a branch/tag name.
- Tag present on remote: `git ls-remote --tags origin v2.0.0` ✓
- `plugin.json` version confirmed `2.0.0` (tag MUST be v2.0.0 = Rack major, not the internal v1.4 milestone label).
- No GitHub Release created, no `.vcvplugin` attached (VCV rebuilds all platforms from source).

## Verification

- `gh repo view --json visibility --jq .visibility` == `PRIVATE` — no exposure this plan.
- `git rev-parse v2.0.0^{commit}` → 40-char hash, recorded in STATE.md.
- `git ls-tree --name-only main docs/index.md docs/changelog.md` lists both files (count 2).

## Notes

- Merge FF also carried the Phase 28 planning docs and Phase 27 doc/cleanup work onto
  `main` — all within `.planning/`, `docs/`, and repo hygiene; no source regressions.
- The begin-phase STATE.md edit was stashed during the merge/tag so the tagged commit is
  exactly the reviewed `4d7b0a8`, then restored for the hash-recording commit.
