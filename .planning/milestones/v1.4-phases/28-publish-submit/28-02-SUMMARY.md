---
phase: 28-publish-submit
plan: 02
status: complete
completed: 2026-07-10
requirements: [PUB-01]
---

# 28-02 Summary — Public flip (gated on CLEAN purge re-verify)

## What was done

Flipped the repository from PRIVATE to PUBLIC — but only after a fresh-clone HARD-STOP
re-verification proved the trial-font purge still holds on the exact remote state being
exposed (including the new `v2.0.0` tag). Then confirmed genuine anonymous reachability.

- **Task 1 (HARD-STOP purge re-verify):** `git clone --mirror` of the remote into a
  throwaway dir. The mirror carried every ref — `refs/heads/main`,
  `refs/heads/chore/finalise-18hp-docs-and-cleanup`, `refs/pull/1/head`, and all tags
  `v1.0`–`v1.3` + `v2.0.0`. Primary check `rev-list --all --objects | grep -iE 'Barell|FoundationLogo'`
  returned **empty (exit 1) = CLEAN**. Belt-and-braces: purged blob OIDs `031e8db` and
  `3533f3e` both **ABSENT** (`git cat-file -e` non-zero). Verdict emitted: **CLEAN — safe to flip**.
- **Task 2 (flip — IRREVERSIBLE, human-gated):** Operator confirmed the CLEAN verdict and
  accepted the one-way-door consequence, then approved. Ran
  `gh repo edit Photep/ForgeAudio-AnalogSeries --visibility public --accept-visibility-change-consequences`.
  Confirmed `gh repo view --json visibility --jq .visibility` == **PUBLIC**.
- **Task 3 (anonymous reachability):** Verified with *unauthenticated* clients (an authed
  clone would succeed even while private — false positive):
  - `curl -sI` repo page → **200**
  - `curl -sI` raw manualUrl (`raw.githubusercontent.com/.../main/docs/index.md`) → **200**
  - Token-free `git clone` (`GIT_TERMINAL_PROMPT=0`, empty credential helper) → succeeded.
  Also closes DOC-03 (manual publicly reachable).

## Key result

- Repo visibility: **PUBLIC** (flipped only after CLEAN fresh-mirror re-verify).
- `sourceUrl` and `manualUrl` return 200 to an unauthenticated caller.
- PUB-01 satisfied: history verified clean at the exact moment of exposure.

## Notes

- One-way door acknowledged: content served while public may already be cloned/forked/cached.
- The public flip was gated strictly on Task 1's CLEAN verdict; a DIRTY result would have
  been a HARD STOP before any `gh repo edit`.
