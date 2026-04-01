---
phase: 20
slug: display-layout-crt-aesthetic
status: draft
nyquist_compliant: true
wave_0_complete: true
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
| 20-01-01 | 01 | 1 | (D-07 font asset) | automated | `test -f res/fonts/JetBrainsMonoNL-Regular.ttf && test -s res/fonts/JetBrainsMonoNL-Regular.ttf` | N/A | ⬜ pending |
| 20-01-02 | 01 | 1 | DISP-03, DISP-05 | manual-visual + grep | Load patch, verify corner brackets and breathing border glow; `grep -c drawBorder src/AnalogLFO.cpp` returns 2 | N/A | ⬜ pending |
| 20-02-01 | 02 | 2 | DISP-02 | manual-visual + grep | Load patch, verify waveform in center 60%; `grep "box.size.x \* 0.20f" src/AnalogLFO.cpp` | N/A | ⬜ pending |
| 20-02-02 | 02 | 2 | DISP-01 | manual-visual + grep | Load patch, verify pill positions in left/right columns; `grep -c nvgBoxGradient src/AnalogLFO.cpp` returns 0 | N/A | ⬜ pending |
| 20-03-01 | 03 | 3 | DISP-04 | manual-visual + grep | Load patch, verify faint scanline bands with slow scroll; `grep -c drawScanlines src/AnalogLFO.cpp` returns 3+; `awk '/drawTextOverlays/,/drawScanlines/' src/AnalogLFO.cpp \| grep -c drawScanlines` returns 1 | N/A | ⬜ pending |
| 20-03-02 | 03 | 3 | DISP-01..05 | checkpoint:human-verify | Full visual verification of all 5 DISP requirements in VCV Rack | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `res/fonts/JetBrainsMonoNL-Regular.ttf` -- font file must be downloaded and placed before any text rendering changes (handled by Plan 01, Task 1)
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

- [x] All tasks have manual-visual verification instructions
- [x] Sampling continuity: visual check after every task commit
- [x] Wave 0 covers font dependency
- [x] No watch-mode flags
- [x] Feedback latency < 30s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
