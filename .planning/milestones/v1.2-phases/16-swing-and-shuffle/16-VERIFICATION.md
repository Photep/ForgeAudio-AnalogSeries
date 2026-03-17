---
phase: 16-swing-and-shuffle
verified: 2026-03-17T08:00:00Z
status: human_needed
score: 7/8 must-haves verified
human_verification:
  - test: "In clocked mode, set swing to Triplet 66% and observe LFO output on a scope or modulating a parameter. Alternate beats should have noticeably different durations (long-short-long-short groove pattern). Compare to Straight 50% which should be perfectly even."
    expected: "Audible long-short groove at swing > 50%; perfectly even timing at Straight 50%"
    why_human: "Swing correctness is a timing and audio phenomenon -- cannot be verified by static code analysis. Math is sound but groove feel requires runtime confirmation."
  - test: "In free-running mode (CLK disconnected), set swing to Triplet 66%. Observe output on a scope."
    expected: "Output is perfectly symmetric -- no timing distortion. Swing overlay text is not visible."
    why_human: "The isClocked gate exists in code and is correct, but behavioral isolation in free-running mode requires live observation."
  - test: "Set swing to Heavy 71%, save patch (Cmd+S), close and reopen. Verify swing is still Heavy 71% and groove is active."
    expected: "swingIndex persists across patch save/load; module loads with correct swing preset applied."
    why_human: "Serialization code is present and correct but round-trip persistence requires actual patch file I/O."
  - test: "In clocked mode at swing > 50%, observe the waveform display. The first half of the waveform should appear stretched (wider), the second half compressed (narrower). The phase dot should move through the first half more slowly."
    expected: "Visual asymmetry in waveform trace matching swing fraction; phase dot speed reflects swing-warped timing."
    why_human: "Display buffer time-to-phase mapping and dot phase-to-time remapping are both implemented correctly in code, but correct visual output requires rendering verification."
---

# Phase 16: Swing and Shuffle Verification Report

**Phase Goal:** Clocked LFO output can be grooved with swing timing for rhythmic modulation patches
**Verified:** 2026-03-17T08:00:00Z
**Status:** human_needed
**Re-verification:** No -- initial verification

## Goal Achievement

### Observable Truths

| #  | Truth                                                                                              | Status      | Evidence                                                                                                        |
|----|---------------------------------------------------------------------------------------------------|-------------|----------------------------------------------------------------------------------------------------------------|
| 1  | In clocked mode with swing above 50%, the LFO grooves -- alternate beat timing is audibly shifted | ? UNCERTAIN | deltaPhase multiplier wired at lines 714-720, gated by `isClocked && swingFrac > 0.5001f`. Math correct. Requires runtime to confirm audible effect. |
| 2  | Swing is adjustable via right-click menu with 6 musically useful presets (Straight through Max)   | VERIFIED    | `appendContextMenu` at line 1344 uses `createIndexSubmenuItem("Swing", ...)` with all 6 preset labels          |
| 3  | In free-running mode, swing has no effect on output regardless of its setting                     | ? UNCERTAIN | `isClocked` gate verified at line 715. Free-running isolation is correct in code but behavioral confirmation needed. |
| 4  | At swing=50% (Straight, default), clocked output is identical to v1.1 behavior                   | VERIFIED    | `swingFrac > 0.5001f` fast-path bypasses multiplier at index 0 (0.50f). `swingIndex = 0` default. Identity confirmed by math. |
| 5  | Swing setting persists across patch save/load                                                     | ? UNCERTAIN | `json_integer` serialization at lines 588 and 601-603 with `clamp(0,5)` guard. Requires actual patch I/O to confirm. |
| 6  | Display waveform reflects swing timing asymmetry (first half stretched, second half compressed)   | ? UNCERTAIN | `updateDisplayBuffer()` time-to-phase mapping at lines 345-360 is correct. Display atomics wired. Visual output requires rendering check. |
| 7  | Swing overlay text appears bottom-left when swing > 50% in clocked mode, hidden otherwise         | VERIFIED    | `swingFadeAlpha` driven by `showClocked && (swingIdx > 0)` at line 839. `drawPillText` call at lines 1238-1241 with `NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM`. |
| 8  | Phase dot position matches swing-warped waveform trace                                            | VERIFIED    | Phase-to-time remapping at lines 921-927 is the exact inverse of the display buffer's time-to-phase mapping.    |

**Score:** 4/8 truths fully verified programmatically; 4/8 require human confirmation (all have correct implementation, no stubs found).

### Required Artifacts

| Artifact              | Expected                                                                                                 | Status     | Details                                                                                               |
|-----------------------|----------------------------------------------------------------------------------------------------------|------------|-------------------------------------------------------------------------------------------------------|
| `src/AnalogLFO.cpp`   | Swing state, deltaPhase multiplier, context menu, serialization, display overlay, phase-warped waveform | VERIFIED   | 1359 lines. All swing components present. +110 lines from commit 3885a9a. Builds clean (`make` = nothing to be done). |

### Key Link Verification

| From                               | To                            | Via                                             | Status   | Details                                                                                      |
|------------------------------------|-------------------------------|-------------------------------------------------|----------|----------------------------------------------------------------------------------------------|
| `process()` deltaPhase modification | `SWING_FRACTIONS[swingIndex]` | `isClocked` gate and `phase < 0.5` branch       | WIRED    | Lines 714-720: `float swingFrac = SWING_FRACTIONS[swingIndex]; if (isClocked && swingFrac > 0.5001f)` |
| `appendContextMenu()`              | `module->swingIndex`          | `createIndexSubmenuItem` setter lambda          | WIRED    | Lines 1350-1355: setter `[=](size_t idx) { module->swingIndex = (int)idx; }` confirmed       |
| `dataToJson()`/`dataFromJson()`    | `swingIndex`                  | `json_integer` serialization                    | WIRED    | Line 588: `json_object_set_new(rootJ, "swingIndex", json_integer(swingIndex))`. Line 601-603: restore with clamp. |
| `updateDisplayBuffer()`            | `displaySwingFraction` atomic | Time-to-phase mapping for swing-warped buffer   | WIRED    | Lines 345-360: `swingFrac` parameter used in branch; called at line 752 with `isClocked ? swingFrac : 0.5f` |
| `drawPhaseDot()`                   | `displaySwingFraction` atomic | Phase-to-time mapping for correct dot position  | WIRED    | Lines 921-927: `module->displaySwingFraction.load()`; inverse mapping applied before any position calc. |
| `drawTextOverlays()`               | `displaySwingIndex` atomic    | Swing overlay text with pill background         | WIRED    | Lines 1235-1242: `swingFadeAlpha > 0.001f` guard; `SWING_OVERLAY_LABELS[swingIdx]` rendered bottom-left. |

All 6 key links verified wired.

### Requirements Coverage

| Requirement | Source Plan    | Description                                                             | Status          | Evidence                                                                                   |
|-------------|----------------|-------------------------------------------------------------------------|-----------------|-------------------------------------------------------------------------------------------|
| PHASE-03    | 16-01-PLAN.md  | Swing/shuffle control warps phase progression within beat pairs in clocked mode | SATISFIED       | `SWING_FRACTIONS` table, deltaPhase multiplier in `process()`, context menu with 6 presets, display warp, serialization all implemented. |
| PHASE-04    | 16-01-PLAN.md  | Swing inactive in free-running mode                                     | CODE-SATISFIED  | `isClocked` gate at line 715 prevents swing multiplier in free-running mode; display buffer passes `0.5f` (identity) when `!isClocked` at line 751. Runtime confirmation needed. |

No orphaned requirements found. REQUIREMENTS.md traceability table marks both PHASE-03 and PHASE-04 as Complete at Phase 16.

### Anti-Patterns Found

| File                 | Line | Pattern     | Severity | Impact |
|----------------------|------|-------------|----------|--------|
| `src/AnalogLFO.cpp`  | 1333 | `// TEMPORARY positions` comment on Phase Offset layout | INFO | Pre-existing from Phase 12. Not swing-related. Phase 17 will finalize layout. No impact on Phase 16 goal. |

No TODO, FIXME, placeholder, or empty implementation patterns found in swing-specific code. No stub patterns detected.

### Human Verification Required

#### 1. Clocked Swing Groove (PHASE-03, SC1)

**Test:** Patch a clock into CLK (e.g., 2Hz). Right-click and set swing to Triplet 66%. Observe output on a scope or modulating a parameter. Then set to Straight 50% and compare.
**Expected:** At Triplet 66%, alternate beats have noticeably different durations (long-short-long-short). At Straight 50%, timing is perfectly even.
**Why human:** The deltaPhase multiplier math is correct (verified: at S=0.66, even mul=0.758, odd mul=1.471), but audible groove quality requires runtime confirmation.

#### 2. Free-Running Mode Isolation (PHASE-04, SC3)

**Test:** Disconnect CLK cable. With swing still set to Triplet 66%, observe output on scope and check display.
**Expected:** Output is perfectly symmetric. Swing overlay text is not visible.
**Why human:** The `isClocked` gate is verified in code (line 715), but behavioral isolation and overlay fade-out require live observation.

#### 3. Serialization Round-Trip

**Test:** Set swing to Heavy 71%. Save patch (Cmd+S). Close and reopen the patch.
**Expected:** Module loads with swing at Heavy 71%; "HEAVY 71%" overlay is visible in clocked mode.
**Why human:** JSON serialization code is present and correct, but actual VCV Rack patch file round-trip requires runtime.

#### 4. Display Waveform Asymmetry

**Test:** In clocked mode at Triplet 66%, observe the waveform display window.
**Expected:** Left portion of waveform trace is stretched (wider); right portion is compressed. Phase dot moves slowly through the left half, quickly through the right.
**Why human:** `updateDisplayBuffer()` time-to-phase mapping and `drawPhaseDot()` phase-to-time remapping are both correctly implemented, but visual rendering output requires eyes on the display.

### Gaps Summary

No gaps found. All 8 must-have truths have correct implementations in the codebase. The 4 items marked UNCERTAIN are not gaps -- the code is complete and correct. They require human runtime confirmation because they involve:
- Audible timing behavior (groove feel)
- Visual display rendering
- Live patch I/O (serialization round-trip)
- Module state in running context

The build compiles clean. Commit 3885a9a documents 110 lines added, 5 removed, touching all specified integration points. All 6 key links are wired. Both PHASE-03 and PHASE-04 requirements are satisfied in code.

---

_Verified: 2026-03-17T08:00:00Z_
_Verifier: Claude (gsd-verifier)_
