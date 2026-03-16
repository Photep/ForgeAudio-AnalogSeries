---
phase: 15
slug: waveform-bleed
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-17
---

# Phase 15 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Manual verification via VCV Rack runtime |
| **Config file** | none -- VCV Rack modules tested by running the module |
| **Quick run command** | `make && make install` |
| **Full suite command** | `make && make install` then verify all success criteria in VCV Rack |
| **Estimated runtime** | ~30 seconds (build + install + visual inspection) |

---

## Sampling Rate

- **After every task commit:** Run `make && make install` -- verify module loads without crash
- **After every plan wave:** Full manual verification of all success criteria in VCV Rack
- **Before `/gsd:verify-work`:** All success criteria verified in VCV Rack
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 15-01-01 | 01 | 1 | CHAR-05 SC1 | manual-only | Visual: Character=1, Morph=0.5, observe display for neighbor bleed | N/A | ⬜ pending |
| 15-01-02 | 01 | 1 | CHAR-05 SC2 | manual-only | Set Character=0, sweep Morph: verify clean crossfade, no bleed | N/A | ⬜ pending |
| 15-01-03 | 01 | 1 | CHAR-05 SC3 | manual-only | Patch output to scope, sweep all params, verify no spikes above +/-5V | N/A | ⬜ pending |
| 15-01-04 | 01 | 1 | CHAR-05 spread | manual-only | Two AnalogLFOs side-by-side, compare crossfade character on scope | N/A | ⬜ pending |
| 15-01-05 | 01 | 1 | CHAR-05 modulation | manual-only | Observe display at high Character over 30+ seconds for slow fluctuation | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. No test framework needed beyond existing build system (`make && make install`).

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Neighbor shape bleed visible on display | CHAR-05 SC1 | Perceptual/visual -- requires VCV Rack display rendering | Set Character=1, Morph=0.5, observe waveform trace for adjacent shape influence |
| Clean crossfade at Character=0 | CHAR-05 SC2 | Perceptual/visual -- must compare to v1.1 behavior | Set Character=0, sweep Morph end-to-end, verify crisp transitions |
| Output within +/-5V bounds | CHAR-05 SC3 | Requires VCV Rack scope module | Patch output to scope, sweep Morph/Character/bleed combinations, check for spikes |
| Component spread variation | CHAR-05 spread | Requires two module instances running | Place two AnalogLFOs, same settings, compare crossfade on scope |
| Slow bleed modulation | CHAR-05 modulation | Requires real-time observation over 30+ seconds | Watch display at high Character for subtle fluctuation in bleed amount |

**Manual-only justification:** VCV Rack DSP modules are tested by running in the host application. All behaviors are perceptual (visual/auditory) and require the module running in context.

---

## Validation Sign-Off

- [ ] All tasks have manual verification instructions
- [ ] Sampling continuity: build verification after every commit
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
