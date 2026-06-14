---
phase: 18
slug: pwm-dsp-extension
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-28
---

# Phase 18 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Browser-based manual + automated script verification |
| **Config file** | none — single HTML file application |
| **Quick run command** | `grep -c "computePulse" forge-noir.html` |
| **Full suite command** | `node -e "const fs=require('fs');const h=fs.readFileSync('forge-noir.html','utf8');const checks=['computePulse','pulseEdgeSpread','NUM_SHAPES = 5','% 5'];checks.forEach(c=>{const f=h.includes(c);console.log(f?'✅':'❌',c)});if(!checks.every(c=>h.includes(c)))process.exit(1)"` |
| **Estimated runtime** | ~1 second |

---

## Sampling Rate

- **After every task commit:** Run `grep -c "computePulse" forge-noir.html`
- **After every plan wave:** Run full suite command
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 1 second

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 18-01-01 | 01 | 1 | WAVE-01 | grep | `grep "NUM_SHAPES = 5" forge-noir.html` | ❌ W0 | ⬜ pending |
| 18-01-02 | 01 | 1 | WAVE-01 | grep | `grep "computePulse" forge-noir.html` | ❌ W0 | ⬜ pending |
| 18-01-03 | 01 | 1 | WAVE-02 | grep | `grep "pulseEdgeSpread" forge-noir.html` | ❌ W0 | ⬜ pending |
| 18-01-04 | 01 | 1 | WAVE-03 | grep | `grep "% 5" forge-noir.html` | ❌ W0 | ⬜ pending |
| 18-01-05 | 01 | 1 | WAVE-04 | manual | Browser morph sweep test | N/A | ⬜ pending |
| 18-01-06 | 01 | 1 | WAVE-05 | manual | Browser character knob test in pulse region | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- Existing infrastructure covers all phase requirements — single HTML file with inline JS, verified via grep and browser testing.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Morph sweep produces 5 shapes audibly | WAVE-01 | Audio output requires human ear | Sweep morph 0→1, verify sine→tri→saw→square→pulse progression |
| Backward-compatible patch loading | WAVE-03 | Requires comparing audio output to v1.2 | Load existing patch with morph<0.75, verify identical sound |
| Character knob softens pulse edges | WAVE-04 | Visual + audio verification | In pulse region, turn Character knob, verify visible edge softening |
| Smooth square-to-pulse transition | WAVE-05 | Click/discontinuity detection requires listening | Slowly sweep morph through square→pulse boundary, listen for clicks |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 1s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
