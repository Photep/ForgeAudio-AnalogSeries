---
phase: 4
slug: analog-character
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-06
---

# Phase 4 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | VCV Rack 2 live testing (visual + auditory) + `make` compile check |
| **Config file** | Makefile (standard VCV Rack build) |
| **Quick run command** | `make` |
| **Full suite command** | `make && make install` + visual verification in VCV Rack |
| **Estimated runtime** | ~15 seconds (compile) + manual verification |

---

## Sampling Rate

- **After every task commit:** Run `make`
- **After every plan wave:** Run `make && make install` + visual verification
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 15 seconds (compile)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 04-01-01 | 01 | 1 | CHAR-01 | compile | `make` | N/A | ⬜ pending |
| 04-01-02 | 01 | 1 | CHAR-01 | compile | `make` | N/A | ⬜ pending |
| 04-01-03 | 01 | 1 | CHAR-02 | compile | `make` | N/A | ⬜ pending |
| 04-01-04 | 01 | 1 | CHAR-03 | compile | `make` | N/A | ⬜ pending |
| 04-01-05 | 01 | 1 | CHAR-04 | compile | `make` | N/A | ⬜ pending |
| 04-01-06 | 01 | 1 | CHAR-05 | compile | `make` | N/A | ⬜ pending |
| 04-02-01 | 02 | 1 | CHAR-07 | compile | `make` | N/A | ⬜ pending |
| 04-02-02 | 02 | 1 | CHAR-07 | compile | `make` | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. No test framework needed beyond `make` + VCV Rack visual verification.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Character knob at zero = bit-identical digital output | CHAR-01 | Requires visual comparison of display waveforms | Set character to 0, compare display output with Phase 2 baseline for each waveform shape |
| Saw exponential ramp curvature + soft reset | CHAR-02 | Requires visual inspection of waveform shape | Set morph to saw (0.5-0.67), sweep character 0-100%, verify smooth curvature and soft reset on display |
| Square sigmoid edges + duty asymmetry | CHAR-03 | Requires visual/auditory assessment | Set morph to square (0.75-1.0), sweep character, verify soft edges and slight asymmetry on display |
| Triangle rounded peaks + slope asymmetry | CHAR-04 | Requires visual inspection | Set morph to triangle (0.25-0.33), sweep character, verify rounded peaks and asymmetric slopes |
| Sine residual THD (subtle thickening) | CHAR-05 | Requires visual/auditory assessment | Set morph to sine (0.0), sweep character, observe slight thickening on display |
| Character CV modulation response | CHAR-07 | Requires live patching | Patch LFO to Character CV, verify display responds to CV changes |
| Progressive knob feel | CHAR-01 | Requires perceptual assessment | Sweep character slowly; first half should be subtle, second half stronger |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 15s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending