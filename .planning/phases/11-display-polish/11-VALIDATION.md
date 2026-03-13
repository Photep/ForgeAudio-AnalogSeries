---
phase: 11
slug: display-polish
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-13
---

# Phase 11 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Manual visual inspection (display rendering) |
| **Config file** | none — visual UI cannot be unit tested |
| **Quick run command** | `cd "/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series" && make -j4 && cp -r plugin.dylib ~/.Rack2/plugins/ForgeAudio-AnalogSeries/` |
| **Full suite command** | Manual: load patch, verify overlays in free-running and clocked modes |
| **Estimated runtime** | ~15 seconds (build) + manual inspection |

---

## Sampling Rate

- **After every task commit:** Run `make -j4` (compile check)
- **After every plan wave:** Visual inspection in VCV Rack with test patch
- **Before `/gsd:verify-work`:** Full visual verification across all clock states and ratio settings
- **Max feedback latency:** 15 seconds (build time)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 11-01-01 | 01 | 1 | DISP-01 | manual-only | `make -j4` (compile gate) | N/A — visual | ⬜ pending |
| 11-01-02 | 01 | 1 | DISP-02 | manual-only | `make -j4` (compile gate) | N/A — visual | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. This phase modifies existing NanoVG rendering code only. No test infrastructure is applicable to visual display rendering. Compile-time verification (successful build) is the automated gate.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Pill backgrounds render behind text overlays, waveform trace does not occlude text | DISP-01 | Visual rendering output — no programmatic assertion possible | Connect clock, observe SYNC/ratio/BPM/Hz overlays while waveform passes through text areas at various morph/character/drift settings |
| Dual BPM display shows CLK and BPM at non-x1 ratios | DISP-02 | Visual layout verification | Set ratio to /2, verify "CLK" and "BPM" lines both shown with correct values |
| Single BPM at x1 ratio (no redundant CLK line) | DISP-02 | Visual layout verification | Set ratio to x1, verify only single "BPM" line displayed |
| Feathered gradient edges on pills (no hard boundary) | DISP-01 | Visual aesthetic quality | Inspect pill edges — should fade smoothly from dark center to transparent edge |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 15s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
