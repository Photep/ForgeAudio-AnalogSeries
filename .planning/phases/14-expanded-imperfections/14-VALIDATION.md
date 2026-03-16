---
phase: 14
slug: expanded-imperfections
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-16
---

# Phase 14 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Manual verification via VCV Rack runtime |
| **Config file** | none — VCV Rack modules tested by running the module |
| **Quick run command** | `make && make install` |
| **Full suite command** | `make && make install` then verify all success criteria in VCV Rack |
| **Estimated runtime** | ~30 seconds (build) + manual verification |

---

## Sampling Rate

- **After every task commit:** Run `make && make install` — verify module loads without crash
- **After every plan wave:** Full manual verification of all success criteria in VCV Rack
- **Before `/gsd:verify-work`:** All 5 success criteria verified in VCV Rack
- **Max feedback latency:** 30 seconds (build time)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 14-01-01 | 01 | 1 | CHAR-01 | manual-only | `make && make install` + visual inspection | N/A | ⬜ pending |
| 14-01-02 | 01 | 1 | CHAR-02 | manual-only | `make && make install` + scope observation | N/A | ⬜ pending |
| 14-01-03 | 01 | 1 | CHAR-03 | manual-only | `make && make install` + knob response test | N/A | ⬜ pending |
| 14-01-04 | 01 | 1 | CHAR-04 | manual-only | `make && make install` + dual instance comparison | N/A | ⬜ pending |
| 14-01-05 | 01 | 1 | SC-5 | manual-only | `make && make install` + Drift=0 comparison | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing infrastructure covers all phase requirements. No test framework installation needed beyond the existing `make` build system.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Phase jitter visible as trace thickening at Drift>0 | CHAR-01 | Perceptual visual behavior in VCV Rack display | Set Drift above 0, observe waveform trace thickness on module display |
| DC offset wander observable on scope module | CHAR-02 | Requires patching to external scope module | Patch LFO output to scope, observe center drift over ~30s |
| Frequency change lag with Drift up | CHAR-03 | Perceptual timing behavior | Turn Rate knob rapidly with Drift up, observe settling time vs Drift=0 |
| Two identical instances produce different output | CHAR-04 | Requires two module instances side-by-side | Place two AnalogLFOs with identical settings, compare on scope |
| Drift=0 matches pre-v1.2 precision | SC-5 | Requires comparison with known-good output | Set Drift=0, verify output matches digital precision (no jitter, no DC offset, no slew lag) |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
