---
phase: 25
slug: release-ip-hardening-private
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-30
---

# Phase 25 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> NOTE: This is an IP/release-hardening phase. Verification is by **shell
> assertion** (git, grep, make, file existence) and **clean-room re-clone**,
> not by a unit-test framework. There is no application code under test.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | none — shell assertions (git / grep / make / test -f) |
| **Config file** | none |
| **Quick run command** | `git rev-list --all --objects \| grep -iE 'Barell\|FoundationLogo'` (MUST be empty) |
| **Full suite command** | clean-room re-clone + `make dist` sanity (see Manual-Only) |
| **Estimated runtime** | ~30–120 seconds (re-clone dominates) |

---

## Sampling Rate

- **After every task commit:** Run the relevant shell assertion for that task (file exists / gitignored / grep empty).
- **After every plan wave:** Re-run the working-tree + history grep checks.
- **Before `/gsd:verify-work`:** Clean-room re-clone verification MUST return empty for both trial fonts.
- **Max feedback latency:** ~120 seconds.

---

## Per-Task Verification Map

> Task IDs are assigned by the planner. The verifications below are the
> known acceptance probes; the planner maps them onto its task breakdown.

| Verification | Requirement | Test Type | Automated Command | Status |
|---|---|---|---|---|
| Trial fonts removed from working tree | IP-01 | shell | `! test -f BCBarellTEST-Regular.otf && ! test -f FoundationLogo.ttf` | ⬜ pending |
| Trial fonts gitignored | IP-01 | shell | `git check-ignore BCBarellTEST-Regular.otf FoundationLogo.ttf` exits 0 | ⬜ pending |
| Fonts purged from ALL local history | IP-02 | shell | `git rev-list --all --objects \| grep -iE 'Barell\|FoundationLogo'` empty | ⬜ pending |
| Fonts purged on remote (clean-room) | IP-02 | shell | fresh `git clone` of remote → same grep returns empty | ⬜ pending |
| LICENSE present + shipped | PKG-01 | shell | `test -f LICENSE` AND `make dist` includes LICENSE in the zip | ⬜ pending |
| NOTICES inventories third-party assets | PKG-04 | shell+manual | `test -f NOTICES` (or `res/OFL.txt`) AND `make dist` ships it | ⬜ pending |
| SVG font-outline provenance confirmed | IP-03 | manual | human confirms FORGE/AUDIO outlines are OFL Bebas, not trial FoundationLogo (Assumption A1) | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

*No test framework to install. Existing shell tooling (git 2.39.5, grep, make) plus `git-filter-repo` (installed via `brew install git-filter-repo`) covers all phase verifications.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| SVG font-outline provenance (Assumption A1) | IP-03 | Cannot be determined from path geometry — requires human judgment on whether the FORGE/AUDIO wordmark outlines were drawn from the trial FoundationLogo font or the OFL Bebas Neue cut | Inspect `res/AnalogLFO.svg` (and sibling panel SVGs) against known OFL Bebas/Chakra letterforms; if unconfirmed, regenerate the wordmark paths from confirmed OFL fonts before release |
| Clean-room purge verification | IP-02 | The hard gate for Phase 28 — must be proven against the actual remote, not the local rewrite | Fresh `git clone` of the private remote into a temp dir, run the history grep, confirm empty across `--all --tags` |
| Irreversible force-push to private remote | IP-02 | Destructive; must occur push-first (local 75 commits ahead) and while remote is still PRIVATE | Confirm remote is private (`gh repo view`), confirm local→remote pushed first, then filter-repo on fresh clone, then `git push --force --all --tags` |

---

## Validation Sign-Off

- [ ] All tasks have a shell assertion or a documented manual verification
- [ ] Sampling continuity: every task has an automated or manual probe
- [ ] Clean-room re-clone verification defined for IP-02 (hard gate for Phase 28)
- [ ] No watch-mode flags
- [ ] Feedback latency < 120s
- [ ] `nyquist_compliant: true` set in frontmatter once planner maps task IDs

**Approval:** pending
