---
phase: 19
slug: forge-noir-panel-custom-components
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-03-29
---

# Phase 19 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | make (C++ build) + grep-based SVG/code assertions |
| **Config file** | Makefile |
| **Quick run command** | `make` |
| **Full suite command** | `make && make install` |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Run `make`
- **After every plan wave:** Run `make && make install`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 19-01-01 | 01 | 1 | PANEL-01 | file check | `grep -q 'width="71.12mm"' res/AnalogLFO.svg && grep -q 'height="128.5mm"' res/AnalogLFO.svg` | ❌ W0 | ⬜ pending |
| 19-01-02 | 01 | 1 | PANEL-01 | file check | `grep -q '#0c0c0c' res/AnalogLFO.svg` | ❌ W0 | ⬜ pending |
| 19-01-03 | 01 | 1 | PANEL-01 | file check | `grep -q 'linearGradient' res/AnalogLFO.svg && grep -q '#e85d26' res/AnalogLFO.svg` | ❌ W0 | ⬜ pending |
| 19-01-04 | 01 | 1 | PANEL-07 | file check | `! grep -rl '<text' res/AnalogLFO.svg res/components/*.svg` | ❌ W0 | ⬜ pending |
| 19-01-05 | 01 | 1 | nanosvg | file check | `! grep -rl '<style' res/AnalogLFO.svg res/components/*.svg` | ❌ W0 | ⬜ pending |
| 19-01-06 | 01 | 1 | nanosvg | file check | `! grep -rl '<use ' res/AnalogLFO.svg res/components/*.svg` | ❌ W0 | ⬜ pending |
| 19-02-01 | 02 | 1 | PANEL-03 | file check | `ls res/components/ForgeKnobHero.svg res/components/ForgeKnobHero_bg.svg res/components/ForgeKnobSecondary.svg res/components/ForgeKnobSecondary_bg.svg res/components/ForgeKnobUtility.svg res/components/ForgeKnobUtility_bg.svg` | ❌ W0 | ⬜ pending |
| 19-02-02 | 02 | 1 | PANEL-04 | file check | `ls res/components/ForgeTrimpot.svg res/components/ForgeTrimpot_bg.svg` | ❌ W0 | ⬜ pending |
| 19-02-03 | 02 | 1 | PANEL-05 | file check | `ls res/components/ForgeJackInput.svg res/components/ForgeJackOutput.svg` | ❌ W0 | ⬜ pending |
| 19-02-04 | 02 | 1 | PANEL-01 | file check | `ls res/components/ForgeHexBolt.svg` | ❌ W0 | ⬜ pending |
| 19-03-01 | 03 | 2 | PANEL-02 | code grep | `grep -c 'addParam(' src/AnalogLFO.cpp` = 10 | ❌ W0 | ⬜ pending |
| 19-03-02 | 03 | 2 | PANEL-02 | code grep | `grep -c 'addInput(' src/AnalogLFO.cpp` = 7 | ❌ W0 | ⬜ pending |
| 19-03-03 | 03 | 2 | PANEL-02 | code grep | `grep -c 'addOutput(' src/AnalogLFO.cpp` = 1 | ❌ W0 | ⬜ pending |
| 19-03-04 | 03 | 2 | PANEL-03 | code grep | `grep 'ForgeKnobHero' src/AnalogLFO.cpp` | ❌ W0 | ⬜ pending |
| 19-03-05 | 03 | 2 | PANEL-03 | code grep | `grep 'ForgeKnobSecondary' src/AnalogLFO.cpp` | ❌ W0 | ⬜ pending |
| 19-03-06 | 03 | 2 | PANEL-03 | code grep | `grep 'ForgeKnobUtility' src/AnalogLFO.cpp` | ❌ W0 | ⬜ pending |
| 19-03-07 | 03 | 2 | PANEL-04 | code grep | `grep -c 'ForgeTrimpot' src/AnalogLFO.cpp` >= 5 | ❌ W0 | ⬜ pending |
| 19-03-08 | 03 | 2 | PANEL-05 | code grep | `grep 'ForgeJackInput' src/AnalogLFO.cpp` | ❌ W0 | ⬜ pending |
| 19-03-09 | 03 | 2 | PANEL-05 | code grep | `grep 'ForgeJackOutput' src/AnalogLFO.cpp` | ❌ W0 | ⬜ pending |
| 19-03-10 | 03 | 2 | PANEL-01 | code grep | `grep -c 'ForgeHexBolt' src/AnalogLFO.cpp` = 4 | ❌ W0 | ⬜ pending |
| 19-03-11 | 03 | 2 | PANEL-03 | code grep | `grep 'shadow->opacity' src/AnalogLFO.cpp` | ❌ W0 | ⬜ pending |
| 19-04-01 | 04 | 2 | PANEL-01 | config | `grep -q '"minRackVersion".*"2.6.0"' plugin.json` | ❌ W0 | ⬜ pending |
| 19-04-02 | 04 | 2 | all | build | `make` exits 0 | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `res/components/` directory exists
- [ ] All SVG files created per component inventory
- [ ] No external test framework needed — validation uses grep, ls, and make

*Existing infrastructure (Makefile, src/) covers build validation.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| No black rectangles on render | PANEL-01 | Requires visual inspection in VCV Rack | Load module, check no missing SVG elements |
| Ember accent bars visible | PANEL-01 | Visual color verification | Top/bottom gradient bars render orange |
| 3 knob sizes visually distinct | PANEL-03 | Size comparison requires visual | MORPH largest, CHAR/DRIFT medium, RATE/PHASE smallest |
| Knobs rotate with drag | PANEL-03 | Interaction test | Drag each knob, verify indicator moves |
| Trimpots visually brighter than knobs | PANEL-04 | Color contrast comparison | Side-by-side brightness difference |
| Output jack has ember accent ring | PANEL-05 | Visual color check | Only output jack has orange ring |
| Output jack larger than inputs | PANEL-05 | Size comparison | Visual difference in jack diameter |
| Forge emblem subtle behind knobs | PANEL-06 | Opacity/atmosphere check | Faint pattern, not distracting |
| Brand text readable | PANEL-07 | Typography verification | "FORGE" and "AUDIO" in ember, "ANALOG LFO" in warm white |
| Cables connect to jacks | PANEL-05 | Interaction test | Drag cable to each jack |
| Module is 14HP wide | PANEL-01 | Width verification | Module occupies 14HP in rack |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
