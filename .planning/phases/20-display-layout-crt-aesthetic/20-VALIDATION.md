---
phase: 20
slug: display-layout-crt-aesthetic
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-01
---

# Phase 20 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Visual inspection (NanoVG rendering, no unit test framework) |
| **Config file** | None -- VCV Rack plugin, visual verification only |
| **Quick run command** | `make install && open -a "VCV Rack 2 Pro"` |
| **Full suite command** | Same as quick run -- visual inspection in VCV Rack |
| **Estimated runtime** | ~30 seconds (build + launch) |

---

## Sampling Rate

- **After every task commit:** `make install` + visual verification in VCV Rack
- **After every plan wave:** Full visual review of all 5 DISP requirements
- **Before `/gsd:verify-work`:** All 5 success criteria verified visually, screenshot documentation
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 20-01-01 | 01 | 1 | DISP-01 | manual-visual | Load patch, verify pill positions in left/right columns | N/A | ⬜ pending |
| 20-01-02 | 01 | 1 | DISP-02 | manual-visual | Load patch, verify waveform in center 60% | N/A | ⬜ pending |
| 20-02-01 | 02 | 1 | DISP-03 | manual-visual | Visual check for L-shaped brackets at all 4 corners | N/A | ⬜ pending |
| 20-02-02 | 02 | 1 | DISP-04 | manual-visual | Look for faint scanline bands, verify slow scroll | N/A | ⬜ pending |
| 20-02-03 | 02 | 1 | DISP-05 | manual-visual | Watch idle module 10+ seconds, verify border glow pulse | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `res/fonts/JetBrainsMonoNL-Regular.ttf` -- font file must be downloaded and placed before any text rendering changes
- No automated test infrastructure -- all verification is visual. This is standard for VCV Rack NanoVG rendering.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Three-column layout | DISP-01 | NanoVG rendering, no DOM/pixel testing | Load patch, verify pills appear in left/right margins, never overlapping waveform center |
| Waveform confinement | DISP-02 | Visual geometry check | Load patch with various morph shapes, verify trace does not extend into pill columns |
| Corner brackets | DISP-03 | Visual element presence | Check at 100% and 200% zoom for L-shaped ember brackets at all 4 corners |
| CRT scanlines | DISP-04 | Subtle visual effect | Look for faint horizontal bands, verify slow downward scroll animation |
| Breathing border glow | DISP-05 | Animation timing check | Watch idle module for 10+ seconds, verify border opacity pulses smoothly |

---

## Validation Sign-Off

- [ ] All tasks have manual-visual verification instructions
- [ ] Sampling continuity: visual check after every task commit
- [ ] Wave 0 covers font dependency
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
