---
phase: 9
slug: phase-reset-and-drift-integration
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-10
---

# Phase 9 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Manual testing in VCV Rack 2 (no automated unit test framework) |
| **Config file** | none |
| **Quick run command** | `make install && open -a "VCV Rack 2 Free"` |
| **Full suite command** | Manual testing with clock module patched to CLK input, LFO output to filter/VCA |
| **Estimated runtime** | ~30 seconds (build) + manual verification |

---

## Sampling Rate

- **After every task commit:** Run `make install && open -a "VCV Rack 2 Free"`
- **After every plan wave:** Full manual test: clock at various tempos, test all division ratios, test with LFO modulating filter for click detection, test cable connect/disconnect
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds (build time)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 09-01-01 | 01 | 1 | RATE-04 | manual | `make` (build check) + visual: patch clock at 60 BPM, set /4, verify LFO completes full cycle over 4 beats | N/A | ⬜ pending |
| 09-01-02 | 01 | 1 | RATE-05 | manual | `make` (build check) + auditory: route LFO to resonant filter cutoff, listen for clicks at clock edges | N/A | ⬜ pending |
| 09-02-01 | 02 | 1 | DISP-04 | manual | Set Drift=100%, compare LFO wobble amount between free-running and clocked modes | N/A | ⬜ pending |
| 09-02-02 | 02 | 1 | DISP-05 | manual | Slowly connect/disconnect CLK cable, listen for frequency jump vs smooth glide | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. The `make` build check serves as the automated smoke test. All behavioral verification is manual in VCV Rack 2.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Phase resets on correct edges for /N ratios | RATE-04 | Requires visual inspection of LFO waveform in running VCV Rack | Patch clock at 60 BPM, set /4, verify LFO completes full cycle over 4 beats |
| No audible click on phase reset | RATE-05 | Auditory perception test | Route LFO to resonant filter cutoff, listen for clicks at clock edges |
| Drift reduced in clocked mode | DISP-04 | Requires visual comparison of wobble amount | Set Drift=100%, compare LFO wobble between free-running and clocked modes |
| Smooth frequency transition on cable connect/disconnect | DISP-05 | Auditory perception test | Slowly connect/disconnect CLK cable, listen for frequency jump vs smooth glide |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
