---
phase: 12-reset-and-phase-offset
verified: 2026-03-15T00:00:00Z
status: human_needed
score: 9/9 must-haves verified
human_verification:
  - test: "RESET click-free crossfade in VCV Rack"
    expected: "Rising-edge trigger at RESET jack resets LFO phase with no audible click, waveform restarts from 0"
    why_human: "Click-free behavior requires auditory verification; cannot detect crossfade quality from source code"
  - test: "RESET does not corrupt clock period tracking"
    expected: "While CLK is running and BPM display is stable, sending rapid triggers to RESET does not disturb BPM readout"
    why_human: "Clock state independence requires runtime behavioral observation across two concurrent input streams"
  - test: "Simultaneous CLK + RESET within 1ms produce exactly one reset"
    expected: "Patching the same clock into both CLK and RESET produces no double-reset artifacts (no frequency wobble, no click pair)"
    why_human: "1ms blanking window outcome requires runtime timing observation; cannot verify edge ordering statically"
  - test: "No-cable RESET is transparent (identical to pre-Phase-12)"
    expected: "With no cable in RESET jack, LFO behavior is indistinguishable from v1.1"
    why_human: "Behavioral equivalence requires A/B comparison in the running module"
  - test: "Phase Offset knob shifts output 0-360 degrees smoothly"
    expected: "Turning knob from 0 to full moves output phase continuously; at 0.25 a sine looks like a cosine; no clicks during sweep"
    why_human: "Smooth phase shift and click-free sweeping require auditory/visual verification on a scope or display"
  - test: "Display dot tracks offset-inclusive position"
    expected: "As Phase Offset knob turns, the display dot slides along the waveform curve; waveform shape itself does not rotate"
    why_human: "Display dot behavior requires visual observation in VCV Rack GUI"
  - test: "Phase Offset CV modulates offset in real time"
    expected: "Patching a slow LFO into Phase Offset CV causes the dot to move and output phase to shift continuously"
    why_human: "CV modulation behavior is observable only at runtime"
  - test: "Quadrature operation (two instances, 90-degree offset)"
    expected: "Two Analog LFO instances at identical rates with second at Phase Offset 0.25 show 90-degree phase relationship on scope"
    why_human: "Quadrature accuracy requires scope observation of two simultaneous outputs"
  - test: "Phase Offset zero with no CV is identical to pre-Phase-12"
    expected: "At default knob position with no CV, output waveform matches a v1.1 instance"
    why_human: "Behavioral equivalence at zero-offset requires runtime A/B comparison"
---

# Phase 12: RESET and Phase Offset Verification Report

**Phase Goal:** Add independent RESET trigger input and Phase Offset knob with CV for phase-shifting the waveform output. RESET provides hard sync and re-trigger capability. Phase Offset enables quadrature patches and real-time phase animation.
**Verified:** 2026-03-15
**Status:** human_needed — all automated checks passed; runtime behavioral verification remains
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Rising-edge trigger at RESET jack snaps LFO to start with click-free crossfade | ? HUMAN | Code: crossfadeFrom/crossfadeProgress mechanism in processResetInput (line 402-404) identical to CLK reset path; click-free quality requires auditory check |
| 2 | RESET operates independently of CLK — does not touch clock state | ✓ VERIFIED | processResetInput (lines 392-411) contains zero references to clockTimer, clockBeatCount, smoothedPeriod, clockEdgeCount, or clockState; explicit code comment on lines 407-408 |
| 3 | Simultaneous CLK + RESET within 1ms cause only one reset | ? HUMAN | Code: resetBlanking.trigger(0.001f) called in both CLK first-edge (line 314), shouldReset (line 386), and RESET (line 406) paths; blanking checked before RESET fires (line 400); 1ms window verification needs runtime timing |
| 4 | No RESET cable connected — behavior identical to v1.1 | ? HUMAN | Code: early return `if (!inputs[RESET_INPUT].isConnected()) return;` (line 396) guarantees no-cable no-op; equivalence to v1.1 requires A/B runtime confirmation |
| 5 | Phase Offset knob shifts waveform output 0-360 degrees | ? HUMAN | Code: phaseOffset applied at readout (line 560-561: `float p = (float)phase + phaseOffset; if (p >= 1.f) p -= 1.f;`); range 0-1 mapped to 360deg via configParam multiplier; auditory/scope verification needed |
| 6 | Display dot tracks offset-inclusive position | ? HUMAN | Code: displayPhase stores displayP (line 567: `displayP = (float)phase + phaseOffset`); waveform buffer loop (lines 254-257) uses raw `p` only, no phaseOffset; visual check needed in GUI |
| 7 | Phase Offset CV modulates offset in real time | ? HUMAN | Code: CV branch (lines 553-556) reads PHASE_OFFSET_CV_INPUT with attenuator, clamped to [0,1]; runtime modulation quality needs verification |
| 8 | Changing Phase Offset produces smooth changes with no clicks | ? HUMAN | Code: offset applied to readout only (not accumulator), so no discontinuity in phase accumulator; crossfade not triggered; smoothness requires auditory confirmation |
| 9 | Phase Offset zero with no CV — output identical to v1.1 | ? HUMAN | Code: at offsetKnob=0, offsetCV=0 → phaseOffset=0 → p = (float)phase + 0 = phase; path is mathematically identical to pre-Phase-12; runtime A/B check needed |

**Automated score:** 1/9 truths statically verified (Truth 2); remaining 8 require runtime verification.
**Code-completeness score:** 9/9 — all truths have correct, substantive implementation present in source.

---

## Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/AnalogLFO.cpp` (Plan 01) | RESET_INPUT enum, resetTrigger, resetBlanking, processResetInput, bidirectional blanking in processClockInput | ✓ VERIFIED | All six elements present and substantive |
| `src/AnalogLFO.cpp` (Plan 02) | PHASE_OFFSET_PARAM, PHASE_OFFSET_ATTEN_PARAM, PHASE_OFFSET_CV_INPUT, offset-at-readout, offset-inclusive displayPhase | ✓ VERIFIED | All five elements present and substantive |

### Artifact Detail — Plan 01 Elements

| Element | Line(s) | Status |
|---------|---------|--------|
| `RESET_INPUT` in `InputId` enum | 25 | ✓ Present |
| `dsp::SchmittTrigger resetTrigger` | 116 | ✓ Present |
| `dsp::PulseGenerator resetBlanking` | 117 | ✓ Present |
| `processResetInput()` method | 392-411 | ✓ Substantive (rising-edge detection, blanking check, crossfade reset) |
| `resetBlanking.trigger()` in CLK first-edge path | 314 | ✓ Present |
| `resetBlanking.trigger()` in CLK shouldReset path | 386 | ✓ Present |
| `processResetInput(args.sampleTime)` called in `process()` | 463 | ✓ Wired (called immediately after processClockInput) |
| `configInput(RESET_INPUT, "Reset")` in constructor | 426 | ✓ Present |
| RESET jack widget at (52.0, 86.0) | 1084 | ✓ Present |

### Artifact Detail — Plan 02 Elements

| Element | Line(s) | Status |
|---------|---------|--------|
| `PHASE_OFFSET_PARAM` in `ParamId` enum | 16 | ✓ Present |
| `PHASE_OFFSET_ATTEN_PARAM` in `ParamId` enum | 17 | ✓ Present |
| `PHASE_OFFSET_CV_INPUT` in `InputId` enum | 26 | ✓ Present |
| `configParam(PHASE_OFFSET_PARAM, 0,1,0,"Phase Offset"," deg",0,360)` | 427 | ✓ Present, multiplier 360 confirmed |
| `configParam(PHASE_OFFSET_ATTEN_PARAM, ...)` | 428 | ✓ Present |
| `configInput(PHASE_OFFSET_CV_INPUT, "Phase Offset CV")` | 429 | ✓ Present |
| Phase offset computation (knob + atten * CV / 5V, clamped) | 551-557 | ✓ Substantive |
| Offset applied at readout: `float p = (float)phase + phaseOffset` | 560-561 | ✓ Present, wraps correctly |
| `displayPhase.store(displayP, ...)` where displayP includes offset | 565-567 | ✓ Present, exactly one store site |
| `updateDisplayBuffer()` contains no phaseOffset reference | 252-259 | ✓ Confirmed — raw `p` sweep only |
| Phase Offset knob widget at (30.48, 86.0) | 1095 | ✓ Present |
| Phase Offset attenuator Trimpot at (52.0, 96.0) | 1096 | ✓ Present |
| Phase Offset CV jack at (52.0, 118.0) | 1097 | ✓ Present |
| Build compiles without errors | — | ✓ `make`: "Nothing to be done for all" (current) |

---

## Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `processResetInput()` | `crossfadeFrom, crossfadeProgress, phase` | Same crossfade mechanism as processClockInput | ✓ WIRED | Lines 402-404: `crossfadeFrom = lastOutputVoltage; crossfadeProgress = 0.f; phase = 0.0;` — exact same three-line pattern as CLK reset |
| `processClockInput()` first-edge | `resetBlanking` | Bidirectional blanking trigger | ✓ WIRED | Line 314: `resetBlanking.trigger(0.001f);` in `clockEdgeCount == 1` block |
| `processClockInput()` shouldReset | `resetBlanking` | Bidirectional blanking trigger | ✓ WIRED | Line 386: `resetBlanking.trigger(0.001f);` in `shouldReset` block |
| Phase offset computation | `computeMorphedWave()` | Offset added to phase before waveform lookup | ✓ WIRED | Line 560: `float p = (float)phase + phaseOffset;` fed directly to `computeMorphedWave(p, morph, character)` on line 562 |
| Phase offset computation | `displayPhase` | Offset-inclusive phase stored for display dot | ✓ WIRED | Lines 565-567: `float displayP = (float)phase + phaseOffset; if (displayP >= 1.f) displayP -= 1.f; displayPhase.store(displayP, ...)` |
| `updateDisplayBuffer()` | `computeMorphedWave()` | Buffer renders WITHOUT offset (raw 0-1 phase sweep) | ✓ WIRED | Lines 254-257: loop uses `float p = (float)i / (float)DISPLAY_SAMPLES` — no phaseOffset anywhere in updateDisplayBuffer |

All 6 key links verified.

---

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| MOD-03 | 12-01 | Separate RESET trigger jack resets phase independently from CLK, with 1ms blanking to prevent double-resets | ✓ SATISFIED | RESET_INPUT jack + processResetInput with resetBlanking.process() guard; blanking fired bidirectionally from CLK paths |
| MOD-04 | 12-01 | RESET uses existing anti-click cosine crossfade | ✓ SATISFIED | processResetInput lines 402-404 use crossfadeFrom/crossfadeProgress — exact same mechanism as CLK reset; crossfade applied via shared code path at lines 571-578 |
| PHASE-01 | 12-02 | Phase offset knob shifts waveform phase 0-360 degrees, applied at waveform readout (not accumulator) | ✓ SATISFIED | Lines 560-562: offset added to readout variable `p`, not to `phase` accumulator; phase accumulator advances unmodified |
| PHASE-02 | 12-02 | Phase offset CV input for external modulation | ✓ SATISFIED | Lines 553-556: PHASE_OFFSET_CV_INPUT read with isConnected() guard, attenuated, added to knob value before clamp |

No orphaned requirements. All four IDs (MOD-03, MOD-04, PHASE-01, PHASE-02) claimed by phase 12 plans are implemented. REQUIREMENTS.md traceability table marks all four as "Complete" for Phase 12.

---

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/AnalogLFO.cpp` | 1006, 1046, 1067 | `drawPlaceholder` | ℹ️ Info | Standard VCV widget thumbnail code — not a stub; used only when `module == nullptr` (browser preview) |

No blocker or warning-level anti-patterns found. No TODO/FIXME/HACK comments in phase-12-modified code. No empty handlers. No stub return values.

---

## Human Verification Required

### 1. RESET Click-Free Crossfade

**Test:** Set LFO to ~1Hz sine, patch a slow trigger (~0.5Hz) into RESET. Listen and watch scope.
**Expected:** Waveform resets cleanly on each trigger with no audible click or discontinuity.
**Why human:** Crossfade mechanism is present and correct in code, but perceptual click-free quality requires auditory confirmation.

### 2. RESET Does Not Corrupt Clock Tracking

**Test:** Patch clock into CLK (BPM display stable), then send rapid triggers into RESET from an independent source.
**Expected:** BPM display remains stable; clock period tracking is unaffected.
**Why human:** Clock state independence is verified statically (processResetInput touches zero clock members), but the absence of unintended interactions requires runtime observation.

### 3. Simultaneous CLK + RESET Blanking

**Test:** Patch same clock source (via mult) into both CLK and RESET simultaneously.
**Expected:** Exactly one reset per clock edge — no double-reset clicks, no frequency wobble.
**Why human:** 1ms blanking window correctness depends on sample-accurate timing that cannot be verified statically.

### 4. No-Cable RESET Transparency

**Test:** Remove all cables from RESET jack. Run LFO normally.
**Expected:** Behavior indistinguishable from a v1.1 module.
**Why human:** Static analysis confirms early return on disconnected input, but full behavioral equivalence needs runtime A/B comparison.

### 5. Phase Offset Knob Sweep

**Test:** Set LFO to ~1Hz sine. Slowly turn Phase Offset knob from 0 to full while monitoring output on a scope.
**Expected:** Phase shifts continuously 0-360 degrees; at 0.25 output resembles cosine; at 0.5 output is inverted; no clicks during sweep.
**Why human:** Smooth shift and perceptual click-free quality require auditory/scope observation.

### 6. Display Dot Tracks Offset

**Test:** With LFO running, turn Phase Offset knob.
**Expected:** Display dot slides along the waveform curve (waveform shape stays fixed; dot moves).
**Why human:** Display dot behavior is visual and requires observation in VCV Rack GUI.

### 7. Phase Offset CV Modulation

**Test:** Patch a slow LFO (~0.2Hz) into Phase Offset CV with attenuator at mid position.
**Expected:** Dot position modulates smoothly; output phase shifts in real time.
**Why human:** CV modulation quality and smoothness require runtime observation.

### 8. Quadrature Operation

**Test:** Two Analog LFO instances at identical rates; set second instance Phase Offset to 0.25. Patch both to scope.
**Expected:** 90-degree phase relationship visible on scope.
**Why human:** Quadrature accuracy requires scope observation of two simultaneous outputs.

### 9. Zero-Offset Equivalence

**Test:** Set Phase Offset to 0, disconnect Phase Offset CV. Compare output to a second instance with no Phase Offset controls.
**Expected:** Outputs are identical.
**Why human:** Mathematical equivalence is confirmed in code, but runtime A/B confirmation validates no unexpected interaction.

---

## Summary

Phase 12 implementation is complete and correct at the code level. All nine must-have truths have substantive implementations in `src/AnalogLFO.cpp`. All six key links are wired. All four requirement IDs (MOD-03, MOD-04, PHASE-01, PHASE-02) are satisfied. The build is current (no recompilation needed).

The only outstanding items are runtime behavioral verifications — the two human-approved checkpoints in Plans 01 and 02 covered most of these during development (per SUMMARY.md: "Human-verified: click-free resets, no double-resets, clock tracking unaffected" and "Smooth, click-free offset changes verified in VCV Rack including quadrature (90-degree pair) operation"). If those approvals are accepted as sufficient, the phase goal is achieved. If independent re-verification is required, the nine human checks above provide the test protocol.

---

_Verified: 2026-03-15_
_Verifier: Claude (gsd-verifier)_
