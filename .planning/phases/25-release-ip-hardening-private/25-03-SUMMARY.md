---
phase: 25-release-ip-hardening-private
plan: 03
subsystem: infra
tags: [git-filter-repo, history-rewrite, ip-02, force-push, clean-room-verify]

requires:
  - phase: 25-release-ip-hardening-private
    provides: 25-01 clean tree + 25-04 OFL panel (what the purge preserves); 25-02/25-04 IP-03 confirmed-OFL gate
provides:
  - Trial-font blobs permanently purged from ALL git history on the private remote (IP-02)
  - Clean-room re-clone verification EMPTY — the hard gate Phase 28's public flip depends on
affects: [28-public-flip]

tech-stack:
  added: [git-filter-repo 2.47.0]
  patterns: [push-first-before-history-rewrite, verify-on-second-independent-clone]

key-files:
  created: [.planning/phases/25-release-ip-hardening-private/25-03-SUMMARY.md]
  modified: []

key-decisions:
  - "Push-first invariant: pushed local main + all Phase 22-25 work to origin BEFORE the rewrite so the throwaway clone contained everything (no commits lost)"
  - "filter-repo run on a throwaway clone, never the working repo; verified clean IN that clone before any push"
  - "Operator aborted the first go/no-go attempt (2026-07-01); re-authorized and completed on 2026-07-08"
  - "Repo stayed PRIVATE throughout; verification done on a SECOND brand-new clone (never the filter-repo clone) per stale-cache pitfall"

patterns-established:
  - "Irreversible remote rewrites go behind an explicit human go/no-go gate + independent clean-room re-verification"

requirements-completed: [IP-02]

duration: ~2 sessions (2026-07-01 prep+abort, 2026-07-08 completion)
completed: 2026-07-08
---

# Phase 25 Plan 03: Trial-Font History Purge Summary

**Both trial-font blobs are permanently purged from ALL git history on the still-private remote (all branches + tag v1.3), and an independent clean-room re-clone verifies EMPTY — the IP-02 hard gate Phase 28's public flip depends on. Push-first preserved every Phase 22-25 commit; the repo stayed private throughout.**

## Performance

- **Duration:** two sessions — reversible prep + operator abort at the go/no-go gate (2026-07-01), re-run + completion (2026-07-08)
- **Tasks:** 3 (reversible prep → blocking go/no-go gate → force-push + clean-room verify + resync)

## Live measurements (recorded per plan output requirement)

- **Ahead-count at push-first:** local was 91 commits ahead of the parked `origin/main` (2cf7b59 → 8a24538, the first push on 2026-07-01), then +1 (the abort-record commit, 8a24538 → 5eff231 on 2026-07-08). Push-first satisfied both times (`origin/main..HEAD == 0`, remote == local).
- **Rewrite:** `git filter-repo --invert-paths --path BCBarellTEST-Regular.otf --path FoundationLogo.ttf` on a throwaway fresh clone.
  - `main`: `5eff231` → **`afc1ae2`**
  - tag `v1.3`: `edd3606` → **`1f7441e`** (rewritten, font-free)
  - 381 commits preserved (none dropped).

## Clean-room verification (IP-02 hard gate — SECOND independent fresh clone)

| Check | Result |
|-------|--------|
| `git rev-list --all --objects \| grep -iE 'Barell\|FoundationLogo'` | **EMPTY** ✓ |
| `git cat-file -e 031e8db9…` (BCBarell blob) | **MISSING** ✓ |
| `git cat-file -e 3533f3e4…` (FoundationLogo blob) | **MISSING** ✓ |
| `git ls-tree -r v1.3 \| grep -iE 'Barell\|FoundationLogo'` | **EMPTY** (v1.3 = 1f7441e) ✓ |
| Phase 22-25 work present (381 commits; LICENSE + NOTICES + OFL.txt + regenerated Chakra panel in HEAD tree) | ✓ no work lost |
| `gh repo view … isPrivate` (before AND after) | **true** ✓ repo never left private |

## Local resync

`git fetch origin` → `git reset --hard origin/main` (HEAD → afc1ae2) → `git fetch --tags --force origin` (local v1.3 stale tag → 1f7441e) → `git reflog expire --expire=now --all` → `git gc --prune=now`. Result: local history fully clean (0 trial-path objects, both blob OIDs unreachable), in sync with the rewritten `origin/main`; untracked `.planning`/mockup work preserved.

## Self-Check: PASSED

IP-02 satisfied: trial fonts purged from all branches + tag v1.3 on the private remote, verified EMPTY on an independent clean-room clone. No Phase 22-25 work lost. Repo remained PRIVATE. This is the hard gate Phase 28's public flip is now cleared against.

## Downstream

- **Phase 28 public-flip gate is now clearable** — a final fresh-clone grep at flip time should re-confirm EMPTY (PUB-01).
- The remote and local are both at `afc1ae2`; the two throwaway scratch clones under `scratchpad/25-03-*` are disposable.
