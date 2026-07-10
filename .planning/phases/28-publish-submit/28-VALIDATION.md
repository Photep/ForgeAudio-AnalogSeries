---
phase: 28
slug: publish-submit
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-07-10
---

# Phase 28 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
>
> **Nature of this phase:** Publish + Submit is an *operational* phase — it runs
> git/gh/curl commands and performs two irreversible, outward-facing actions
> (flip repo public, open the VCV Library submission issue). There is no product
> code and therefore no unit-test surface. "Validation" here means **command-output
> assertions** (grep returns empty, HTTP 200, tag resolves to a commit) plus
> **human-gated confirmation** on the irreversible steps. This is the correct
> Nyquist shape for a release phase, not a coverage gap.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | none — assertions are shell command exit codes / captured output (git, gh, curl) |
| **Config file** | none |
| **Quick run command** | `git rev-list --all --objects \| grep -iE 'Barell\|FoundationLogo'` (must be empty) |
| **Full suite command** | Fresh mirror-clone purge re-verify + `gh repo view --json visibility` + `curl -sI <manualUrl>` |
| **Estimated runtime** | ~30–60 seconds (network-bound: clone + curl) |

---

## Sampling Rate

- **After every task commit:** Re-run that task's command-output assertion (e.g. purge grep empty, tag resolves).
- **After every plan wave:** Re-confirm the gate that wave depends on still holds (purge-clean before flip; public + reachable before submit).
- **Before `/gsd:verify-work`:** Purge grep empty on a fresh clone, repo visibility == public, manualUrl returns 200, submission issue URL recorded.
- **Max feedback latency:** ~60 seconds.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 28-xx | pre-flip | 0 | PUB-01 | IP-02 re-verify | Purged trial fonts absent from ALL history on a fresh remote mirror | command-output | `d=$(mktemp -d); git clone --mirror <remote> "$d/m"; git -C "$d/m" rev-list --all --objects \| grep -iE 'Barell\|FoundationLogo'` → **empty (grep exit 1)** | N/A | ⬜ pending |
| 28-xx | tag | 0 | PUB-02 | — | Release commit tagged `v2.0.0`; exact 40-char hash captured | command-output | `git rev-parse v2.0.0^{commit}` resolves; `git rev-parse HEAD` recorded | N/A | ⬜ pending |
| 28-xx | tag | 0 | PUB-01 | — | Release commit is on `main` (manualUrl `/blob/main/...` resolves) | command-output | `git merge-base --is-ancestor <release-commit> main` exits 0 | N/A | ⬜ pending |
| 28-xx | flip | 1 | PUB-01 | — | Repo visibility is public **only after** purge re-verify passed | command-output | `gh repo view --json visibility --jq .visibility` == `PUBLIC` | N/A | ⬜ pending |
| 28-xx | flip | 1 | PUB-01 | — | Public sourceUrl + manualUrl reachable anonymously | command-output | `curl -sI <manualUrl>` → `200`; anonymous `git ls-remote <sourceUrl>` succeeds | N/A | ⬜ pending |
| 28-xx | submit | 2 | PUB-02 | — | Exactly one submission issue, slug-titled, carries sourceUrl + license + manualUrl + full commit hash | command-output | `gh issue view <url> --repo VCVRack/library` shows title == `ForgeAudio-AnalogSeries` and body contains the 40-char hash | N/A | ⬜ pending |
| 28-xx | submit | 2 | PUB-02 | — | Submission issue URL recorded for future update comments | source | STATE.md / SUMMARY contains the issue URL | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*
*Task IDs finalized by the planner; the rows above map every PUB requirement to a checkable command output.*

---

## Wave 0 Requirements

- [ ] No test framework to install — assertions are inline shell commands.
- [ ] Confirm `git`, `gh` (authed), `git-filter-repo`/`git`, and `curl` present (verified in RESEARCH.md environment audit).

*Existing tooling covers all phase assertions.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Flip repo to **public** | PUB-01 | Irreversible one-way exposure; must be operator-confirmed AFTER the purge re-verify command returns empty | Operator reviews the empty-grep output, then runs `gh repo edit --visibility public --accept-visibility-change-consequences` and confirms in the transcript |
| Open the **one** VCV Library submission issue | PUB-02 | Permanent public channel; the single issue becomes the update thread forever — cannot be re-done | Operator confirms the drafted issue body (slug title, sourceUrl, license, manualUrl, full commit hash) before it is filed against `VCVRack/library` |

---

## Validation Sign-Off

- [ ] Every PUB requirement maps to a command-output assertion or an operator-confirmed manual step
- [ ] Sampling continuity: each irreversible action is gated by the prior wave's assertion (purge-clean → flip; public+reachable → submit)
- [ ] Wave 0 covers the purge re-verify and tag/main-ancestry assertions
- [ ] No watch-mode flags
- [ ] Feedback latency < 60s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
