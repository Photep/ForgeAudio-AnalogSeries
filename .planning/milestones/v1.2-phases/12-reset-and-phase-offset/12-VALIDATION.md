---
phase: 12
slug: reset-and-phase-offset
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-14
---

# Phase 12 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Manual verification in VCV Rack (DSP behavior) + compile-time check |
| **Config file** | none — no automated test framework for VCV Rack modules |
| **Quick run command** | `cd "/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series" && make -j4` |
| **Full suite command** | Manual: build, install, load in VCV Rack, test with clock and trigger sources |
| **Estimated runtime** | ~15 seconds (build) + manual testing |

---

## Sampling Rate

- **After every task commit:** Run `make -j4`
- **After every plan wave:** Manual test in VCV Rack with test patch
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 15 seconds (compile)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 12-01-01 | 01 | 1 | MOD-03 | manual-only | `make -j4` (compile check) | N/A | ⬜ pending |
| 12-01-02 | 01 | 1 | MOD-04 | manual-only | `make -j4` (compile check) | N/A | ⬜ pending |
| 12-01-03 | 01 | 1 | PHASE-01 | manual-only | `make -j4` (compile check) | N/A | ⬜ pending |
| 12-01-04 | 01 | 1 | PHASE-02 | manual-only | `make -j4` (compile check) | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. No test framework applies to real-time audio module behavior beyond compile verification.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| RESET trigger resets phase with click-free crossfade | MOD-03, MOD-04 | Real-time audio behavior requires VCV Rack runtime | Patch clock into CLK, same clock into RESET via mult. Verify no double-reset clicks. Patch independent trigger into RESET. Verify phase resets on trigger edge. |
| CLK/RESET simultaneous triggers no double-reset | MOD-03 | Timing-sensitive behavior requires live patch | Send simultaneous triggers to CLK and RESET within 1ms. Verify no corruption of clock period tracking. |
| Phase offset knob 0-360 degrees, dot tracks offset | PHASE-01 | Visual and audio verification in VCV Rack | Turn offset knob from 0 to full. Observe dot moving along waveform. At 0.25 (90 deg), sine output should be cosine-like. |
| Phase offset CV for quadrature patches | PHASE-02 | Requires patching constant voltage and comparing outputs | Patch constant 1.25V into Phase Offset CV. Verify output is 90 degrees shifted from identical un-offset LFO. Modulate with slow LFO — verify smooth phase animation. |
| Changing offset while running produces no clicks | PHASE-01 | Audio click detection requires listening | Rapidly sweep offset knob while LFO runs at audible rate. Listen for clicks or discontinuities. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 15s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
