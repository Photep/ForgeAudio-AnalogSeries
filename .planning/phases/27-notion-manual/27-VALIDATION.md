---
phase: 27
slug: notion-manual
status: approved
nyquist_compliant: true
wave_0_complete: true
created: 2026-07-09
finalized: 2026-07-09
---

# Phase 27 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> This is a **documentation-authoring** phase — validation is primarily doc-fact-vs-code
> assertions and grep-based content checks rather than a code test suite.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | shell assertions / grep (no unit-test framework for docs) |
| **Config file** | none — validation via grep + file-existence checks |
| **Quick run command** | `grep`-based fact/denylist checks (see Per-Task map) |
| **Full suite command** | doc-fact audit: CV ranges + Swing options + RATIO_TABLE vs `src/AnalogLFO.cpp`, brand-name denylist grep across `docs/` |
| **Estimated runtime** | ~5 seconds |

---

## Sampling Rate

- **After every task commit:** Run the task's grep/existence check
- **After every plan wave:** Run the doc-fact audit (docs facts match code)
- **Before `/gsd:verify-work`:** All facts verified against `src/AnalogLFO.cpp`; brand-name denylist grep returns zero hits
- **Max feedback latency:** 5 seconds

---

## Per-Task Verification Map

Task IDs are `27-{plan}-{task-index}` (tasks are positional in each PLAN.md). Automated verify for content facts is the single doc gate `tests/check_docs.sh` (authored in 27-02 Task 3); manifest and reconciliation plans use targeted greps; the annotated panel is human-verified.

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 27-01-01 | 01 | 1 | DOC-01 | — | N/A | file-exists | `test -f docs/index.md && ls docs/*.md` | ❌ W0 | ⬜ pending |
| 27-01-02 | 01 | 1 | DOC-02 | — | N/A | grep | I/O CV ranges + RATIO_TABLE (15) in docs match `src/AnalogLFO.cpp` (±5V, 5V/unit) via `tests/check_docs.sh` | ❌ W0 | ⬜ pending |
| 27-01-03 | 01 | 1 | DOC-02 | — | N/A | grep | Swing 6 options + clock FSM terms present and match code via `tests/check_docs.sh` | ❌ W0 | ⬜ pending |
| 27-02-01 | 02 | 1 | DOC-02 | T-27-04 | no secrets/PII in prose | file-exists | `test -f docs/install.md docs/changelog.md docs/license-credits.md` | ❌ W0 | ⬜ pending |
| 27-02-02 | 02 | 1 | DOC-02 | T-27-03 | attribution correctness | grep | changelog uses milestone names v1.0–v1.4; states `2.0.0` once (no v2.x relabel) | ❌ W0 | ⬜ pending |
| 27-02-03 | 02 | 1 | DOC-02 | T-27-02 | brand-name denylist | grep | `tests/check_docs.sh` executable + `bash -n` clean; denylist grep over `docs/` returns zero hits (Pitfall 4 / D-06) | ❌ W0 | ⬜ pending |
| 27-03-01 | 03 | 1 | DOC-03 | — | JSON integrity | grep | `plugin.json` contains `manualUrl` = `…/blob/main/docs/index.md` | ✅ | ⬜ pending |
| 27-03-02 | 03 | 1 | DOC-01 | — | N/A | grep | ROADMAP §Phase 27 reconciled (Notion→GitHub; success criteria updated) | ✅ | ⬜ pending |
| 27-03-03 | 03 | 1 | DOC-02 | — | N/A | grep | REQUIREMENTS DOC-01/02/03 wording reconciled (platform, patch-examples dropped, manualUrl) | ✅ | ⬜ pending |
| 27-04-01 | 04 | 2 | DOC-02 | — | N/A | file-exists | build/install per vcv workflow (main tree, no worktree); `docs/img/panel-raw.png` captured | ❌ W0 | ⬜ pending |
| 27-04-02 | 04 | 2 | DOC-02 | — | N/A | file-exists | `docs/panel.md` + `docs/img/panel-annotated.png` (19 callouts) exist; legend doubles as control-reference | ❌ W0 | ⬜ pending |
| 27-04-03 | 04 | 2 | DOC-02 | — | N/A | manual (checkpoint) | human-verify: numbered callouts align to real widgets and match the legend table (D-08) | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky · "W0" = created by phase tasks, not pre-existing*

---

## Wave 0 Requirements

- [ ] Brand-name denylist (the set of trademarked synth names to grep for) — defined before authoring so D-06/Pitfall 4 is checkable
- [ ] Extracted code-fact reference (CV ranges, RATIO_TABLE, Swing options) captured from `src/AnalogLFO.cpp` in RESEARCH.md — used as the assertion oracle

*Existing repo has no doc-test infrastructure; these are lightweight grep/assertion checks, not a framework install.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Annotated-panel screenshot matches shipped module (numbered callouts align to real widgets) | DOC-02 | Requires visual inspection of a rendered Rack screenshot; callout placement is human-assisted (D-08) | Build+install per vcv_build_install_workflow, load ForgeAnalogLFO in Rack, confirm each numbered callout points at the correct control and matches the legend table |
| Manual is shared to web / publicly reachable via `manualUrl` | DOC-03 | Public reachability depends on the repo going public in Phase 28 (sequencing constraint) | After Phase 28 repo-public flip, open `manualUrl` unauthenticated and confirm it resolves to the docs |

---

## Validation Sign-Off

- [x] All tasks have automated grep/existence verify or Wave 0 dependencies (checkpoint 27-04-03 uses a file-existence automated verify + human-verify gate)
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references (denylist + code-fact oracle embedded in `tests/check_docs.sh` + RESEARCH oracle)
- [x] No watch-mode flags
- [x] Feedback latency < 5s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-07-09 (Dimension 8 re-derived against final 4 plans by gsd-plan-checker — all checks pass)
