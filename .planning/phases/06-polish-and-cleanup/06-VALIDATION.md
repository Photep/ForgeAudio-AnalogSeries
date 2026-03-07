---
phase: 6
slug: polish-and-cleanup
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-07
---

# Phase 6 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None (VCV Rack plugin — compilation + manual runtime verification) |
| **Config file** | Makefile (compilation verification only) |
| **Quick run command** | `make` |
| **Full suite command** | `make && make install` |
| **Estimated runtime** | ~10 seconds |

---

## Sampling Rate

- **After every task commit:** Run `make`
- **After every plan wave:** Run `make && make install` + VCV Rack visual inspection
- **Before `/gsd:verify-work`:** Full suite must be green + all 4 success criteria verified visually
- **Max feedback latency:** 10 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 06-01-01 | 01 | 1 | SC-1 (OUT-02 docs) | manual-only | `grep "OUT-02" .planning/REQUIREMENTS.md` | N/A | ⬜ pending |
| 06-01-02 | 01 | 1 | SC-2 (drift visual) | manual-only | `make` | N/A | ⬜ pending |
| 06-01-03 | 01 | 1 | SC-3 (drift CV response) | manual-only | `make` | N/A | ⬜ pending |
| 06-01-04 | 01 | 1 | SC-4 (bottom row layout) | manual-only | `make` | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. No automated test framework applies to VCV Rack plugin architecture.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| OUT-02 description accuracy | SC-1 | Documentation text verification | Grep REQUIREMENTS.md for OUT-02, confirm it reflects INV output removal |
| Drift dot instability visible | SC-2 | Visual animation quality | Load module in VCV Rack, set drift to max, observe trail jitter and halo variation |
| Drift visual responds to CV | SC-3 | CV-modulated visual behavior | Patch LFO to drift CV input, observe dot instability changes with CV signal |
| Bottom row design consistency | SC-4 | Visual layout judgment | Load module in VCV Rack, inspect trimpot/jack alignment, connection lines, output distinction |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
