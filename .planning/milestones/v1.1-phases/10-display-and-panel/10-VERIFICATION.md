---
phase: 10-display-and-panel
verified: 2026-03-13T00:00:00Z
status: human_needed
score: 6/7 must-haves verified
human_verification:
  - test: "Free-running Hz readout — turn Rate knob"
    expected: "Hz value updates in top-left of waveform display; no SYNC badge, no ratio, no BPM visible"
    why_human: "Visual rendering requires VCV Rack runtime; programmatic checks confirm implementation exists and is wired but cannot confirm pixel output"
  - test: "Clock connect — ACQUIRING blink"
    expected: "Patch a clock to CLK jack; Hz fades out (~200ms), SYNC badge appears top-right blinking at ~2Hz, ratio label appears top-left, BPM appears bottom-right"
    why_human: "Real-time blink behavior and fade transitions require running application"
  - test: "Clock lock — LOCKED solid badge"
    expected: "After a few clock edges the SYNC badge stops blinking and becomes solid amber"
    why_human: "State machine transition (ACQUIRING -> LOCKED) and resulting visual change require running application"
  - test: "Clock disconnect — reverse fade"
    expected: "Unplug clock cable: SYNC/ratio/BPM fade out, Hz readout fades back in, each over ~200ms"
    why_human: "Fade timing and visual smoothness require running application"
  - test: "CLK label on panel SVG"
    expected: "Lavender CLK label visible above the CLK jack, positioned correctly and matching jack label convention"
    why_human: "SVG rendering and visual placement require VCV Rack runtime"
  - test: "BPM accuracy at known ratio"
    expected: "At 120 BPM clock with /4 ratio: display shows 30 BPM; at x2 ratio: display shows 240 BPM"
    why_human: "BPM calculation correctness at specific ratios requires running application with measured clock source"
---

# Phase 10: Display and Panel Verification Report

**Phase Goal:** The user can see at a glance whether the LFO is clock-synced and at what ratio
**Verified:** 2026-03-13
**Status:** human_needed
**Re-verification:** No -- initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | SYNC badge appears in top-right corner when clock is connected and tracking | ? HUMAN | `drawTextOverlays()` draws "SYNC" at `box.size.x - margin` (top-right) guarded by `syncFadeAlpha`; alpha driven by `displayClockState != FREE`; needs runtime to confirm render |
| 2 | SYNC badge blinks at ~2Hz during ACQUIRING state and is solid during LOCKED state | ? HUMAN | Blink logic confirmed: `clockState == AnalogLFO::ACQUIRING` path multiplies `effectiveAlpha` by `0.5 + 0.5 * sin(breathePhase * 2.5f)`; breathePhase advances at 0.8Hz so 0.8*2.5=2Hz; needs runtime to confirm |
| 3 | Ratio label appears in top-left corner showing current ratio (e.g., /4, x2) when clocked | ? HUMAN | `ratioFadeAlpha` driven to 1 when `clockState != FREE`; draws `RATIO_LABELS[ratioIdx]` at top-left; ratioIdx read from `displayRatioIndex` atomic; needs runtime |
| 4 | Hz readout appears in top-left corner showing frequency when free-running | ? HUMAN | `hzFadeAlpha` initialised to 1 (starts visible); driven to 0 when clocked; reads `RATE_PARAM` value which is configured 0.01-20Hz; displays as "%.2f Hz"; needs runtime |
| 5 | BPM readout appears in bottom-right corner showing effective BPM when clocked | ? HUMAN | `bpmFadeAlpha` driven by clocked state; `effectiveBPM = 60/period * RATIO_TABLE[ratioIdx]`; `displaySmoothedPeriod` loaded from atomic bridge (audio->GUI); needs runtime |
| 6 | All text overlays fade in/out over ~200ms during mode transitions | ? HUMAN | `fadeSpeed = 1/(0.2*60) = 0.083`; per-overlay clamped ramp in `step()` called ~60fps; math is correct but visual smoothness requires runtime |
| 7 | CLK label is visible on panel SVG above the CLK jack | ? HUMAN | SVG group at `translate(41.21, 78.6) scale(0.35)` confirmed at line 172-184 of `res/AnalogLFO.svg` with three `fill="#8888aa"` paths (C, L, K); nanosvg-compatible geometry paths; needs runtime to confirm render position |

**Score:** 6/7 truths have fully verified implementations (automated checks all pass); all 7 require human runtime confirmation for visual output

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/AnalogLFO.cpp` | displaySmoothedPeriod atomic, drawGlowText helper, drawTextOverlays method, fade animation state | VERIFIED | All four elements confirmed at lines 100, 724, 741, 549-573 respectively |
| `res/AnalogLFO.svg` | CLK label path group in lavender | VERIFIED | Group at lines 172-184, three paths with fill="#8888aa", translate(41.21, 78.6) scale(0.35) |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `AnalogLFO::process()` | `displaySmoothedPeriod` | atomic store of smoothedPeriod | WIRED | Line 453: `displaySmoothedPeriod.store(smoothedPeriod, std::memory_order_relaxed)` |
| `WaveformDisplay::drawTextOverlays()` | `displaySmoothedPeriod` | atomic load for BPM calculation | WIRED | Line 783: `module->displaySmoothedPeriod.load(std::memory_order_relaxed)` used in BPM formula |
| `WaveformDisplay::drawLayer()` | `drawTextOverlays()` | call after drawPhaseDot() | WIRED | Line 838: `drawTextOverlays(vg)` called inside `if (module)` block in layer 1, after `drawPhaseDot` |
| `WaveformDisplay::step()` | fade alphas | fade timer updates at ~60fps | WIRED | Lines 562-573: fadeSpeed calculated, all four alphas updated via `clamp`-based ramp on each step() call |

All four key links from PLAN frontmatter verified as wired in actual code.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| DISP-01 | 10-01-PLAN, 10-02-PLAN | Panel SVG updated with CLK jack and label | SATISFIED (human-confirmed per 10-02-SUMMARY) | CLK label at lines 172-184 of `res/AnalogLFO.svg`; CLK input circle in components layer at line 323; user visually confirmed 2026-03-13 |
| DISP-02 | 10-01-PLAN, 10-02-PLAN | Waveform display shows "SYNC" badge when clocked | SATISFIED (human-confirmed per 10-02-SUMMARY) | SYNC badge in `drawTextOverlays()` at line 769-780; blink logic for ACQUIRING; user visually confirmed 2026-03-13 |
| DISP-03 | 10-01-PLAN, 10-02-PLAN | Display shows current division ratio label | SATISFIED (human-confirmed per 10-02-SUMMARY) | Ratio label drawn from `RATIO_LABELS[ratioIdx]` at line 762-766; ratioIdx from `displayRatioIndex` atomic updated in `process()`; user visually confirmed 2026-03-13 |
| DISP-06 | 10-01-PLAN, 10-02-PLAN | Display shows BPM calculated from clock source and selected rate divider | SATISFIED (human-confirmed per 10-02-SUMMARY) | BPM formula `60/period * RATIO_TABLE[ratioIdx]` at line 784-793; `displaySmoothedPeriod` bridged from audio thread; user visually confirmed 2026-03-13 |

**Orphaned requirements check:** DISP-04 and DISP-05 are mapped to Phase 9 in REQUIREMENTS.md — not Phase 10. No orphaned requirements for Phase 10.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/AnalogLFO.cpp` | 800, 840 | `drawPlaceholder()` | Info | Structural fallback for module browser (no module pointer); not a stub — it is the correct VCV Rack pattern for null-module display. Not a concern. |

No blockers or warnings found. No TODO/FIXME/HACK comments. No console output. No empty return stubs in production paths. The `nvgFontBlur` is correctly reset to 0.0f after the glow pass (line 737), avoiding blur bleed.

### Human Verification Required

The 10-02 plan was a human-gate checkpoint. Per 10-02-SUMMARY.md, the user ran VCV Rack and confirmed all four DISP requirements visually on 2026-03-13. That human verification is already recorded.

The items listed below are what a fresh human verifier would need to confirm if they wished to re-run the tests:

#### 1. Free-Running Hz Readout

**Test:** Load the plugin in VCV Rack with no CLK cable; turn the Rate knob
**Expected:** Hz value updates in top-left of waveform display; no SYNC badge, no ratio label, no BPM
**Why human:** Visual rendering requires VCV Rack runtime

#### 2. ACQUIRING Blink Behaviour

**Test:** Patch a clock source to the CLK jack
**Expected:** Hz fades out over ~200ms; SYNC badge fades in at top-right blinking at ~2Hz; ratio label fades in at top-left; BPM fades in at bottom-right
**Why human:** Real-time blink rate and fade smoothness require running application

#### 3. LOCKED State — Solid Badge

**Test:** Allow the LFO to receive several clock edges
**Expected:** SYNC badge stops blinking and becomes a solid amber glow
**Why human:** ACQUIRING -> LOCKED state transition requires real clock signal

#### 4. Clock Disconnect — Reverse Fade

**Test:** Unplug the clock cable while in LOCKED mode
**Expected:** SYNC badge, ratio label, and BPM all fade out over ~200ms; Hz readout fades back in
**Why human:** Fade timing and visual smoothness require running application

#### 5. CLK Label Visual Position

**Test:** Inspect panel in VCV Rack
**Expected:** Lavender "CLK" text appears above the CLK jack on the right side, at the same vertical row as RATE
**Why human:** SVG rendering and pixel placement require VCV Rack runtime

#### 6. BPM Accuracy

**Test:** Clock at a known BPM (e.g., 120 BPM) and step through ratio positions
**Expected:** At x1: 120 BPM; at /4: 30 BPM; at x2: 240 BPM
**Why human:** End-to-end BPM calculation correctness requires running application with measurable clock

### Gaps Summary

No gaps. All automated checks pass:

- Both commits (75169e7, ecf0f16) exist in the repository
- `displaySmoothedPeriod` atomic is declared (line 100), stored in `process()` (line 453), and loaded in `drawTextOverlays()` (line 783) — full audio-to-GUI bridge wired
- `drawGlowText()` is a substantive two-pass implementation (blur pass + sharp pass) at lines 724-739
- `drawTextOverlays()` renders all four overlays with correct guard conditions and alpha values at lines 741-797
- `drawTextOverlays(vg)` is called in `drawLayer()` at line 838, inside the layer 1 guard, inside the `if (module)` block, after `drawPhaseDot`
- Fade state (four alpha floats + fadeSpeed) is declared at lines 549-552 and updated in `step()` at lines 562-573
- CLK label is a three-path SVG group at lines 172-184 with `fill="#8888aa"` (lavender), using nanosvg-compatible geometry only (no text/defs/style/filter)
- CLK input circle is in the components layer at line 323
- Requirements DISP-01, DISP-02, DISP-03, DISP-06 are all covered; DISP-04 and DISP-05 are correctly attributed to Phase 9

Human verification was completed by the user during plan 10-02 on 2026-03-13, with all four DISP requirements confirmed and the RATE label color decision recorded (keep #c0c0d0).

---

_Verified: 2026-03-13_
_Verifier: Claude (gsd-verifier)_
