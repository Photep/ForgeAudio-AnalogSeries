---
phase: 08
slug: frequency-override-and-ratio-table
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-03-07
---

# Phase 08 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Manual testing in VCV Rack 2 (no automated unit test framework) |
| **Config file** | none |
| **Quick run command** | `make` |
| **Full suite command** | `make install && open -a "VCV Rack 2 Free"` |
| **Estimated runtime** | ~15 seconds (build) |

---

## Sampling Rate

- **After every task commit:** Run `make`
- **After every plan wave:** Run `make install` + manual VCV Rack test
- **Before `/gsd:verify-work`:** Full manual suite must pass
- **Max feedback latency:** 15 seconds (build time)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 08-01-01 | 01 | 1 | RATE-01, RATE-02, RATE-03 | build | `make` | N/A | pending |
| 08-01-02 | 01 | 1 | RATE-06 | build | `make` | N/A | pending |
| 08-02-01 | 02 | 2 | RATE-01, RATE-02, RATE-03, RATE-06 | manual | `make install` + VCV Rack | N/A | pending |

*Status: pending · green · red · flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. No new test framework needed. `make` serves as the automated smoke test for each task.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Rate knob unchanged when CLK disconnected | RATE-01 | No unit test framework; requires VCV Rack GUI | Launch VCV Rack, verify Rate knob tooltip shows Hz, verify LFO frequency matches knob value |
| Rate knob snaps to 15 ratios when clocked | RATE-02, RATE-03 | Requires clock source + live module | Patch clock to CLK, sweep Rate knob fully, verify 15 discrete frequency steps |
| Tooltip shows ratio label when clocked | RATE-06 | Requires VCV Rack tooltip rendering | Hover Rate knob while clocked, verify "x4 (synced)" format |
| Tooltip reverts to Hz when clock disconnects | RATE-01, RATE-06 | Requires cable disconnect interaction | Disconnect CLK cable, hover Rate knob, verify "Rate: X.XX Hz" |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 15s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
