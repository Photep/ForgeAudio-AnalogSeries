---
phase: 7
slug: clock-input-and-period-tracking
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-07
---

# Phase 7 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Manual testing in VCV Rack 2 (no automated unit test framework) |
| **Config file** | none |
| **Quick run command** | `make install` |
| **Full suite command** | Manual testing with clock module patched to CLK input |
| **Estimated runtime** | ~30 seconds (build) + manual verification |

---

## Sampling Rate

- **After every task commit:** Run `make install`
- **After every plan wave:** Full manual test with clock module in VCV Rack
- **Before `/gsd:verify-work`:** All 6 requirements verified manually
- **Max feedback latency:** 30 seconds (build time)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 07-01-01 | 01 | 1 | CLK-01 | manual | `make` (build check) | N/A | ⬜ pending |
| 07-01-02 | 01 | 1 | CLK-02 | manual | `make` + patch clock source | N/A | ⬜ pending |
| 07-01-03 | 01 | 1 | CLK-03 | manual | `make` + verify smoothedPeriod convergence | N/A | ⬜ pending |
| 07-01-04 | 01 | 1 | CLK-04 | manual | `make` + stop clock, verify FREE state | N/A | ⬜ pending |
| 07-01-05 | 01 | 1 | CLK-05 | manual | `make` + swap clock cables | N/A | ⬜ pending |
| 07-01-06 | 01 | 1 | CLK-06 | manual | `make` + verify first pulse behavior | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

*Existing infrastructure covers all phase requirements.* No automated test framework needed — this is a C++ VCV Rack module. The `make` build check serves as the automated smoke test.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| CLK jack accepts cable, enters clocked mode | CLK-01 | Requires VCV Rack runtime | Patch clock source to CLK jack, verify state transitions |
| Edge detection with correct thresholds | CLK-02 | Requires real signal processing | Patch clock, verify detection via debug display |
| Period measurement converges | CLK-03 | Requires real-time clock signal | Patch 120 BPM clock, verify smoothedPeriod ~0.5s |
| Clock-loss timeout works | CLK-04 | Requires stopping external clock | Stop clock, verify FREE state within timeout |
| Outlier rejection filters tempo jumps | CLK-05 | Requires cable hot-swap | Swap clock cables, verify no wild spikes |
| First pulse resets phase only | CLK-06 | Requires observing first-pulse behavior | Connect clock, verify phase reset without freq change |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
