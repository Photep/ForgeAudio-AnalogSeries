---
phase: 27
slug: notion-manual
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-07-09
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

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 27-01-01 | 01 | 1 | DOC-01 | — | N/A | file-exists | `test -f docs/index.md && ls docs/*.md` | ❌ W0 | ⬜ pending |
| 27-01-02 | 01 | 1 | DOC-02 | — | N/A | grep | verify I/O CV ranges in docs match `src/AnalogLFO.cpp` (±5V, 5V/unit) | ❌ W0 | ⬜ pending |
| 27-01-03 | 01 | 1 | DOC-02 | — | N/A | grep | Swing submenu 6 options + 15-entry RATIO_TABLE present and match code | ❌ W0 | ⬜ pending |
| 27-02-01 | 02 | 2 | DOC-03 | — | N/A | grep | `plugin.json` contains `manualUrl` field | ✅ | ⬜ pending |
| 27-02-02 | 02 | 2 | DOC-02 | — | N/A | grep | brand-name denylist grep across `docs/` returns zero hits (Pitfall 4 / D-06) | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

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

- [ ] All tasks have automated grep/existence verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references (denylist + code-fact oracle)
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
