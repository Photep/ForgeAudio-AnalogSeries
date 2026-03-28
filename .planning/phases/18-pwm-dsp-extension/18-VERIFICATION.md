---
phase: 18-pwm-dsp-extension
verified: 2026-03-28T06:00:00Z
status: human_needed
score: 4/5 must-haves fully verified (WAVE-05 is a documentation contradiction, not a code defect)
re_verification: false
human_verification:
  - test: "Sweep morph 0 to 1 in VCV Rack and observe display"
    expected: "Five visually distinct shapes in order: sine (smooth sine), triangle (symmetric V), saw (ramp), square (50% pulse), then progressively narrowing pulse reaching approximately 5% duty at full clockwise"
    why_human: "Display rendering is a GUI behavior; computeMorphedWave() is wired to updateDisplayBuffer() but shape correctness at the visual level cannot be confirmed without running VCV Rack"
  - test: "Set morph at maximum (full clockwise), character at minimum. Observe display and/or scope output"
    expected: "Narrow pulse approximately 5% duty cycle width, crisp digital edges"
    why_human: "Exact duty cycle percentage cannot be measured programmatically without a running module or unit test framework"
  - test: "Set morph at approximately 80% (square-to-pulse boundary), slowly sweep to 100% with character at 50%"
    expected: "Smooth continuous narrowing of pulse width with no audible click or pop at the square-pulse transition"
    why_human: "Continuity at boundary (Pitfall 2 from RESEARCH.md) requires auditory verification; the linear crossfade in computeMorphedWave() should handle it but this is a subjective auditory assessment"
  - test: "Set morph near 1.0, character above 0.5. Listen for bleed from neighboring shape"
    expected: "Faint sine-like softening bleeds in at high character; the narrow pulse does not sound harsh or metallic; bleed is subtle (+/-4% base)"
    why_human: "WAVE-04 bleed ring correctness (pulse neighbors sine) is in code at % 5 arithmetic, but the subjective quality of the sine-pulse bleed is an auditory judgment"
  - test: "Confirm WAVE-05 requirements disposition with project owner"
    expected: "Project owner confirms D-02 (backward compatibility dropped) supersedes WAVE-05 as written, and REQUIREMENTS.md should be updated to reflect the dropped requirement rather than marking it complete"
    why_human: "WAVE-05 as written says backward compatibility is preserved. The implementation intentionally breaks it per D-02. REQUIREMENTS.md marks WAVE-05 [x] complete, which is technically incorrect. A human decision is needed: either rewrite WAVE-05 to describe what was actually implemented, or accept the contradiction as a documentation inconsistency."
---

# Phase 18: PWM DSP Extension Verification Report

**Phase Goal:** Users can sweep the morph knob through a fifth waveform shape -- variable-width pulse -- with full analog character (backward compatibility dropped per D-02)
**Verified:** 2026-03-28
**Status:** human_needed (automated checks pass; 5 items require human verification in VCV Rack)
**Re-verification:** No -- initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Sweeping morph 0 to 1 produces five distinct shapes: sine, triangle, saw, square, pulse | ? HUMAN NEEDED | `computeMorphedWave()` uses `morph * 4.f`, `shapes[5] = { sine, tri, saw, sqr, pls }`, correct segment clamping to 3 — code structure verified; visual shape correctness needs VCV Rack |
| 2 | Pulse duty narrows from 50% at the square-pulse boundary to 5% at morph=1.0 | ✓ VERIFIED | `pulseDuty = 0.50f - 0.45f * std::fmin(pulseFrac, 1.f)` at line 343; at pulseFrac=0 duty=0.50, at pulseFrac=1 duty=0.05 — math is correct |
| 3 | Character knob softens pulse edges via tanh rounding -- crisp at 0, rounded at 1 | ✓ VERIFIED | `computePulse()` at lines 302-328: character guard at 0.001f returns digital pulse; above guard, `progressiveCurve(character)` scales tanh sharpness; `std::tanh(sharpness * dist)` confirmed at line 324 |
| 4 | Bleed ring wraps through 5 shapes with pulse neighboring sine | ✓ VERIFIED | `leftIdx = (segment - 1 + 5) % 5` at line 365, `rightIdx = (segment + 2) % 5` at line 366 — both occurrences use `% 5`; no `% 4` remains in morph function |
| 5 | Module compiles without errors or warnings via make -j4 | ✓ VERIFIED | Full rebuild confirms: `c++ ... -c -o build/src/AnalogLFO.cpp.o src/AnalogLFO.cpp` then link succeeded; zero errors, zero warnings |

**Score:** 4/5 truths verified programmatically; truth 1 confirmed structurally but requires human visual confirmation

---

## Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/AnalogLFO.cpp` | computePulse(), 5-shape computeMorphedWave(), pulseEdgeSpread | ✓ VERIFIED | File exists; all three components present and substantive (not stubs) |

---

## Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `computePulse()` | `computeMorphedWave()` | shapes[5] array index 4 | ✓ WIRED | `float shapes[5] = { sine, tri, saw, sqr, pls }` at line 347; `pls = computePulse(phase, character, pulseDuty)` at line 344 |
| `pulseEdgeSpread` | `computePulse()` | sharpness multiplier | ✓ WIRED | `sharpness *= (1.f + pulseEdgeSpread)` at line 315 — spread directly modulates edge softening intensity |
| `initComponentSpread()` | `pulseEdgeSpread` | normal distribution draw appended after bleedSpread | ✓ WIRED | `pulseEdgeSpread = d(spreadRng) * 0.02f` at line 214 — appears AFTER `bleedSpread = d(spreadRng) * 0.02f` at line 212; preserves existing spread ordering (Pitfall 4 mitigation confirmed) |
| `computeMorphedWave()` | `updateDisplayBuffer()` | called per display sample | ✓ WIRED | `displayBuffers[writeIdx][i] = computeMorphedWave(p, morph, character)` at line 394; display renders pulse automatically |
| `computeMorphedWave()` | `process()` | called per audio sample | ✓ WIRED | `float sample = computeMorphedWave(p, morph, character)` at line 809 |

---

## Data-Flow Trace (Level 4)

Not applicable for this phase. This is a pure DSP computation phase (no database, no API, no async data). `computeMorphedWave()` is a synchronous pure function called directly from `process()` and `updateDisplayBuffer()`. Input parameters flow directly to output samples — no data disconnection possible.

---

## Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Module compiles clean | `touch src/AnalogLFO.cpp && make -j4` | Zero errors, zero warnings — full rebuild succeeded | ✓ PASS |
| computePulse() function exists with three parameters | `grep -n "float computePulse"` | Line 302: `float computePulse(float phase, float character, float duty)` | ✓ PASS |
| 5-shape scaling constant | `grep -n "morph \* 4"` | Line 337: `float scaled = morph * 4.f;` | ✓ PASS |
| shapes[5] array | `grep -n "shapes\[5\]"` | Line 347: `float shapes[5] = { sine, tri, saw, sqr, pls }` | ✓ PASS |
| Duty range formula | `grep -n "pulseDuty"` | Line 343: `0.50f - 0.45f * std::fmin(pulseFrac, 1.f)` — gives 50% to 5% | ✓ PASS |
| Pitfall 1 mitigation | `grep -n "maxEdge"` | Line 310: `float maxEdge = std::fmin(duty, 1.f - duty) * 0.8f` | ✓ PASS |
| Old 4-shape constants absent | `grep -n "morph \* 3\.f\|shapes\[4\]\|% 4"` in morph function | No matches — old constants fully removed | ✓ PASS |
| Task commits exist | `git log --oneline -5` | `65166a9 feat(18-01): add computePulse()...` and `1688f29 feat(18-01): update computeMorphedWave()...` both present | ✓ PASS |

---

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|---------|
| WAVE-01 | 18-01-PLAN.md | Morph sweep extends past square into variable-width pulse (Sine -> Tri -> Saw -> Square -> Pulse) | ✓ SATISFIED | `shapes[5] = { sine, tri, saw, sqr, pls }`, `morph * 4.f`, segment clamped to 3 — five shapes, correct order |
| WAVE-02 | 18-01-PLAN.md | Pulse duty cycle ranges from 50% (square) to 5% (narrow pulse) | ✓ SATISFIED | `pulseDuty = 0.50f - 0.45f * std::fmin(pulseFrac, 1.f)`: at pulseFrac=0 duty=0.50, at pulseFrac=1 duty=0.05 |
| WAVE-03 | 18-01-PLAN.md | Character knob applies analog deformation to pulse (tanh edge softening, component spread) | ✓ SATISFIED | `computePulse()` uses `progressiveCurve(character)`, `std::tanh(sharpness * dist)`, and `pulseEdgeSpread` component spread |
| WAVE-04 | 18-01-PLAN.md | Waveform bleed ring wraps to 5 shapes (pulse neighbors sine) | ✓ SATISFIED | `% 5` in both bleed ring lines (365, 366); ring order sine-tri-saw-sqr-pulse-sine confirmed by comment at line 364 |
| WAVE-05 | 18-01-PLAN.md | Existing morph positions preserved for backward compatibility | ! DOCUMENTATION CONTRADICTION | See note below |

### WAVE-05 Documentation Contradiction

REQUIREMENTS.md defines WAVE-05 as "Existing morph positions preserved for backward compatibility [0, 0.75] = original 4 shapes" and marks it `[x]` complete. The traceability table also marks it Complete.

The implementation does the **opposite**: `morph * 3.f` is replaced with `morph * 4.f`. This is a deliberate and documented breaking change (D-02 in CONTEXT.md, acknowledged in RESEARCH.md, and stated in the PLAN: "Backward compatibility dropped: morph*4.f replaces morph*3.f").

The 18-01-PLAN.md frontmatter lists WAVE-05 in `requirements:` and SUMMARY lists it in `requirements-completed`. This means WAVE-05 is claimed as completed by the phase, but the implementation intentionally violates its stated definition.

**Root cause:** WAVE-05 was written before D-02 dropped backward compatibility. The requirement was never updated to reflect the decision.

**Impact on goal:** None. The phase goal explicitly states "backward compatibility dropped per D-02." The DSP implementation is correct per the decisions. This is purely a requirements documentation inconsistency.

**Human action needed:** Update REQUIREMENTS.md to either (a) rewrite WAVE-05 to describe what was actually shipped — "Morph positions across [0, 1.0] remap to 5 even segments; existing patches will produce different shapes (breaking change accepted, D-02)" — or (b) mark WAVE-05 as dropped/superseded rather than complete.

---

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/AnalogLFO.cpp` | 213 | Comment line begins `/ Pulse edge softening spread:` (missing second `/` for `//`) | ℹ️ Info | Not a code defect — appears to be a display artifact in the grep output. Actual source reads correctly; compile succeeded with zero warnings, confirming no syntax issue at this location. |

No TODO/FIXME/placeholder comments found in the modified file sections. No empty implementations. No hardcoded empty returns. No orphaned artifacts.

---

## Human Verification Required

### 1. Five-Shape Visual Sweep

**Test:** Load the module in VCV Rack. Connect a scope or display. Slowly sweep the morph knob from 0 to full clockwise.
**Expected:** Display shows five visually distinct shapes in sequence: smooth sine, symmetric triangle, rising sawtooth, 50% square pulse, then progressively narrowing pulse. Each shape occupies approximately 20% of the knob range.
**Why human:** Shape rendering is a GUI output. The code structure is verified but correctness of shape visual appearance requires the running module.

### 2. Duty Cycle Extremes

**Test:** Set morph to maximum (fully clockwise), character to minimum (fully counter-clockwise). Observe the waveform display or scope output.
**Expected:** Narrow pulse with approximately 5% duty cycle — positive phase occupies roughly 1/20th of the cycle period. Edges should be crisp (digital) at character=0.
**Why human:** The formula `pulseDuty = 0.50f - 0.45f * 1.0f = 0.05f` is verified, but whether the display visually renders a clear narrow pulse at this extreme requires human inspection.

### 3. Square-to-Pulse Boundary Continuity

**Test:** Set morph to approximately 80% (near the square-pulse boundary), character at 50%. Slowly sweep morph from 80% to 100% while listening and watching the display.
**Expected:** Smooth, continuous narrowing of pulse width with no audible click, pop, or timbral discontinuity at the boundary.
**Why human:** Pitfall 2 from RESEARCH.md: `computeSquare()` has a character-dependent duty shift (`0.04f + squareDutySpread`) that creates a slight mismatch with `computePulse()` at frac=0. The linear crossfade should absorb this, but auditory confirmation is required.

### 4. Bleed Ring Pulse-Sine Interaction

**Test:** Set morph near 1.0 (fully in pulse region), character at 70% or above. Listen to the output.
**Expected:** Subtle sine-like softening is audible in the narrow pulse output. The overall character should be warm or complex but not harsh, metallic, or distorted beyond the expected analog character range.
**Why human:** The `% 5` arithmetic correctly routes bleed from sine as the right neighbor of pulse. Whether the sine-pulse crosstalk sounds musically acceptable is a subjective auditory judgment (Pitfall 3 from RESEARCH.md).

### 5. WAVE-05 Requirements Disposition

**Test:** Review REQUIREMENTS.md WAVE-05 entry against the actual implementation and D-02 decision.
**Expected:** Project owner confirms the correct disposition — either rewrite WAVE-05 to describe the "breaking change accepted" outcome, or mark it as dropped rather than complete.
**Why human:** This is a documentation correctness decision that only the project owner can make. The code is correct per D-02; only the requirements document is inconsistent.

---

## Gaps Summary

No code gaps. All five DSP must-haves are implemented correctly:

- `computePulse(float phase, float character, float duty)` exists at line 302 with correct tanh edge softening, Pitfall 1 mitigation (`maxEdge = fmin(duty, 1-duty) * 0.8f`), and component spread wiring
- `computeMorphedWave()` updated to `morph * 4.f`, `shapes[5]`, `segment` clamped to 3, pulseDuty derived from morph position, and bleed ring updated to `% 5`
- `pulseEdgeSpread` member variable at line 154, RNG draw at line 214 (appended after `bleedSpread` per Pitfall 4)
- All old 4-shape constants (`morph * 3.f`, `shapes[4]`, `% 4`) confirmed absent
- Module builds cleanly with zero errors and zero warnings

The only outstanding items are human verification of runtime behavior in VCV Rack and a documentation decision on WAVE-05's recorded status in REQUIREMENTS.md.

---

_Verified: 2026-03-28_
_Verifier: Claude (gsd-verifier)_
