---
phase: 5
slug: drift-engine
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-07
---

# Phase 5 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Compilation + static code analysis (no automated test framework) |
| **Config file** | None — no test infrastructure in project |
| **Quick run command** | `make -j4` |
| **Full suite command** | `make clean && make -j4` |
| **Estimated runtime** | ~5 seconds |

---

## Sampling Rate

- **After every task commit:** Run `make -j4`
- **After every plan wave:** Run `make clean && make -j4`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 5 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 05-01-01 | 01 | 1 | DRFT-01 | static analysis | `make -j4` | N/A | ⬜ pending |
| 05-01-02 | 01 | 1 | DRFT-01 | static analysis | `make -j4` | N/A | ⬜ pending |
| 05-01-03 | 01 | 1 | DRFT-02 | static analysis | `make -j4` | N/A | ⬜ pending |
| 05-02-01 | 02 | 1 | DRFT-01 | static analysis | `make -j4` | N/A | ⬜ pending |
| 05-02-02 | 02 | 1 | DRFT-01 | static analysis | `make -j4` | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

*Existing infrastructure covers all phase requirements.*

No test framework infrastructure needed — this project uses compilation + static analysis + manual runtime verification (consistent with phases 1-4). All verification is code-review-based due to VCV Rack plugin architecture (no headless test runner).

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Drift at zero = stable output | DRFT-01 | Requires audio monitoring in VCV Rack | Load module, set drift to 0, verify pitch is stable |
| Multi-timescale drift audible | DRFT-01 | Subjective audio perception | Set drift to full, listen for slow wander + fast jitter |
| Two modules drift independently | DRFT-01 | Requires two module instances in VCV Rack | Create two identical LFOs, observe different drift patterns |
| Drift CV modulates amount | DRFT-02 | Requires patching in VCV Rack | Patch external CV to drift input, verify drift varies with voltage |
| Three-knob engine integration | DRFT-01/02 | Requires runtime with all knobs active | Set morph, character, and drift to non-zero, verify all affect output |
| Display dot instability visible | DRFT-01 | Visual perception in VCV Rack | Increase drift, observe subtle dot speed variation |
| Panel layout visual balance | DRFT-02 | Visual aesthetics judgment | Check 7 bottom-row components look balanced |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
