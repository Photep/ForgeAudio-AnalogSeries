---
phase: 10
slug: display-and-panel
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-12
---

# Phase 10 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Manual VCV Rack testing (no automated GUI test framework) |
| **Config file** | none |
| **Quick run command** | `make` |
| **Full suite command** | `make install && open /Applications/Rack2Free.app` |
| **Estimated runtime** | ~30 seconds (compile + launch) |

---

## Sampling Rate

- **After every task commit:** Run `make` (compile check)
- **After every plan wave:** Run `make install` + manual VCV Rack testing
- **Before `/gsd:verify-work`:** All four DISP requirements visually verified in VCV Rack
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 10-01-01 | 01 | 1 | DISP-01 | manual-only | Visual inspection in VCV Rack | N/A | ⬜ pending |
| 10-02-01 | 02 | 1 | DISP-02 | manual-only | Patch clock, observe SYNC badge | N/A | ⬜ pending |
| 10-02-02 | 02 | 1 | DISP-03 | manual-only | Turn Rate knob while clocked, observe ratio label | N/A | ⬜ pending |
| 10-02-03 | 02 | 1 | DISP-06 | manual-only | Connect known-BPM clock, verify BPM display | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. No test framework applicable — all requirements are visual display behaviors verified manually in VCV Rack.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| CLK label visible on panel | DISP-01 | Visual SVG rendering | Open module in VCV Rack, verify CLK label above jack |
| SYNC badge shows/hides/blinks | DISP-02 | NanoVG display animation | Patch clock → observe blinking (ACQUIRING) → solid (LOCKED) → disconnect → badge disappears |
| Ratio label updates in real time | DISP-03 | NanoVG display + knob interaction | Connect clock, turn Rate knob, verify label changes (/4, x2, etc.) |
| BPM readout correct | DISP-06 | Calculated display value | Connect known-BPM clock (e.g., 120 BPM), verify effective BPM updates with ratio changes |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: compile check after every task
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
