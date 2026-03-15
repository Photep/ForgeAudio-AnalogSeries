---
phase: 13
slug: fm-input
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-15
---

# Phase 13 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Manual verification in VCV Rack (DSP behavior) + compile-time check |
| **Config file** | none — no automated test framework for VCV Rack modules |
| **Quick run command** | `cd "/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series" && make -j4` |
| **Full suite command** | Manual: build, install, load in VCV Rack, test with FM source and clock |
| **Estimated runtime** | ~10 seconds (build) + manual testing |

---

## Sampling Rate

- **After every task commit:** Run `make -j4`
- **After every plan wave:** Manual test in VCV Rack with FM + clock test patch
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 10 seconds (build time)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 13-01-01 | 01 | 1 | MOD-01 | manual-only | `make -j4` (compile check) | N/A | ⬜ pending |
| 13-01-02 | 01 | 1 | MOD-02 | manual-only | `make -j4` (compile check) | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

*Existing infrastructure covers all phase requirements.*

No test framework applies to real-time audio module behavior beyond compile verification. All behavioral verification is manual in VCV Rack.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| FM input applies exponential frequency modulation with attenuator depth control | MOD-01 | Real-time audio DSP behavior requires live VCV Rack testing | Patch: LFO -> FM input of module. Turn FM attenuator from 0 to full. Observe frequency modulation on scope. Verify modulation depth increases with attenuator. At attenuator=0, verify no modulation. |
| FM authority reduced in clocked mode to prevent clock-phase fighting | MOD-02 | Clock sync behavior requires live clock source and visual waveform inspection | Patch: clock source (e.g. 120 BPM) into CLK. Patch LFO into FM with full attenuator. Compare modulation depth in free mode vs. clocked mode. Clocked should show significantly less FM effect. Verify clock sync is maintained (waveform still resets at clock edges). |
| FM attenuator at zero produces identical output to v1.1 | MOD-01 | Behavioral equivalence requires A/B comparison in running module | With FM cable patched but attenuator at 0, compare output waveform to module without FM code. Should be identical. |
| FM never drives frequency negative | MOD-01 | Stability under extreme inputs requires manual stress testing | Patch +/-10V signal into FM with full attenuator. Verify LFO remains stable, no freezing or erratic behavior. |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
