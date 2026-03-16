---
phase: 14-expanded-imperfections
verified: 2026-03-16T00:00:00Z
status: passed
score: 12/12 must-haves verified
re_verification: false
gaps: []
human_verification:
  - test: "Load a patch, reload VCV Rack, confirm the module produces the same character as before load"
    expected: "Waveform character (spread offsets) is identical across save/load cycles"
    why_human: "Requires actual patch save, reload, and A/B listening comparison — cannot verify determinism from static code inspection alone"
  - test: "Place two instances with identical knob positions, compare output character over 60+ seconds"
    expected: "The two instances produce audibly different drift patterns and waveform shape due to different component spread seeds"
    why_human: "Requires runtime comparison of two live instances — behavioral divergence cannot be verified by reading source"
  - test: "Patch a scope to the output with Drift at maximum, observe over 60+ seconds"
    expected: "Output center point slowly wanders away from 0V by roughly 50-100mV then drifts back — visible as slow scope center shift"
    why_human: "DC offset wander is a slow process (~33s cycle) requiring live observation"
  - test: "Set Rate to a fixed value, move Rate knob rapidly, observe frequency response at high Drift vs Drift=0"
    expected: "At high Drift, frequency changes lag noticeably (sluggish); at Drift=0, changes are immediate"
    why_human: "Pitch slew time constants (2ms-300ms) require live audio comparison to perceive"
---

# Phase 14: Expanded Imperfections Verification Report

**Phase Goal:** Expand drift imperfections with pitch slew, phase jitter, DC offset wander, and component spread
**Verified:** 2026-03-16
**Status:** PASSED
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Rapid frequency changes show settling lag when Drift is above zero | VERIFIED | `driftSlew.process()` at line 575 applies TExponentialFilter with tau 2ms-300ms scaled by drift |
| 2 | At Drift=0, frequency response is identical to v1.1 (no lag) | VERIFIED | `else { driftSlew.setLambda(500.f); }` at line 573 — lambda=500 = effectively instantaneous bypass |
| 3 | Waveform timing shows subtle per-sample jitter when Drift is above zero | VERIFIED | `deltaPhase *= (1.0 + (double)(jitterScale * jitterNoise));` at line 620, inside `if (drift >= 0.001f)` |
| 4 | Phase jitter is completely inactive at Drift=0 | VERIFIED | Jitter code is inside the `if (drift >= 0.001f)` block (lines 598-632); never entered at zero |
| 5 | Phase jitter authority is reduced in clocked mode (2% vs 7.5%) | VERIFIED | `float jitterAuthority = isClocked ? 0.02f : 0.075f;` at line 618 |
| 6 | DC offset wander does NOT reset on clock phase resets | VERIFIED | `dcOffsetOU.state` update is in process() drift block (line 625), not in `processClockInput()` — clock resets only affect `phase`, not dcOffsetOU |
| 7 | DC offset is completely inactive at Drift=0 | VERIFIED | `else { dcOffsetV = 0.f; }` at line 631 |
| 8 | Two instances produce different output character due to per-instance component spread | VERIFIED (code path) | Constructor generates `spreadSeed[0] = rng(); spreadSeed[1] = rng();` then calls `initComponentSpread()` — each instance gets a unique RNG seed at construction |
| 9 | Component spread is always active regardless of Drift setting | VERIFIED | Spread offsets (`triAsymmetrySpread`, `sawCurvatureSpread`, `squareDutySpread`, `ouWeightSpread`, `characterSpread`) are applied unconditionally outside any drift gate |
| 10 | Component spread seed persists across patch saves/loads | VERIFIED | `dataToJson()` writes both seed words as hex strings; `dataFromJson()` reads them back and calls `initComponentSpread()` |
| 11 | At Drift=0, all drift-gated imperfections are completely inactive | VERIFIED | Pitch slew lambda=500 (bypass), jitter and DC offset inside `if (drift >= 0.001f)` with else-zeroing of dcOffsetV |
| 12 | DC offset applied AFTER crossfade capture, avoiding click on phase reset | VERIFIED | Line 697: `lastOutputVoltage = outputVoltage;` then line 700: `outputVoltage += dcOffsetV;` then line 703: `outputs[OUTPUT].setVoltage(outputVoltage);` — exact required ordering |

**Score:** 12/12 truths verified

---

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/AnalogLFO.cpp` | driftSlew filter member and phase jitter processing | VERIFIED | Line 116: member; lines 471-472: constructor init; lines 566-575: pitch slew block; lines 617-620: phase jitter |
| `src/AnalogLFO.cpp` | DC offset OU layer and component spread system | VERIFIED | Lines 119-128: members; lines 503-506: dcOffsetOU constructor; lines 163-177: initComponentSpread() |
| `src/AnalogLFO.cpp` | Component spread seed serialization | VERIFIED | Lines 509-528: dataToJson() and dataFromJson() with hex-string encoding |
| `src/AnalogLFO.cpp` | Component spread offsets applied to compute functions | VERIFIED | Line 201 (tri), 229 (saw), 247 (square), 608 (OU weights), 647 (character) |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| driftSlew filter | freq variable in process() | driftSlew.process() between freqSlew and FM block | WIRED | freqSlew at line 554, driftSlew at line 575, FM block at line 580 — correct ordering confirmed |
| jitterNoise | deltaPhase | multiplicative application after OU drift block | WIRED | OU drift at line 613, jitter at line 620, phase accumulation at line 634 |
| dcOffsetOU.state | outputVoltage | addition AFTER crossfade processing | WIRED | lastOutputVoltage capture at 697, dcOffsetV applied at 700, output write at 703 |
| spreadSeed | dataToJson/dataFromJson | hex string serialization | WIRED | spreadSeed0/spreadSeed1 keys in both methods; stoull() parsing in dataFromJson |
| component spread offsets | computeTriangle/computeSaw/computeSquare | member variable offsets added to shape parameters | WIRED | triAsymmetrySpread at line 201, sawCurvatureSpread at line 229, squareDutySpread at line 247 |
| ouWeightSpread | OU drift calculation | offset added to ouLayers[i].weight in drift block | WIRED | `(ouLayers[i].weight + ouWeightSpread[i])` at line 608 |

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| CHAR-01 | 14-01 | Phase jitter adds per-sample random phase deviation scaled by Drift knob | SATISFIED | jitterNoise/jitterScale/jitterAuthority at lines 617-620; drift-gated and clocked-authority scaled |
| CHAR-02 | 14-02 | DC offset drift adds slow-wandering output bias scaled by Drift knob | SATISFIED | dcOffsetOU OU layer at lines 624-628; applied to outputVoltage after crossfade at line 700 |
| CHAR-03 | 14-01 | Pitch slew adds frequency lag (component thermal response) scaled by Drift knob | SATISFIED | driftSlew filter at lines 566-575; tau range 2ms-300ms via slewTau formula |
| CHAR-04 | 14-02 | Component spread applies per-instance random parameter offsets (serialized via RNG seed) | SATISFIED | initComponentSpread(), dataToJson(), dataFromJson() — 5 spread offsets applied throughout |

All four phase 14 requirements satisfied. No orphaned requirements found — REQUIREMENTS.md traceability table maps exactly CHAR-01 through CHAR-04 to Phase 14, and both plans claim them in their `requirements` frontmatter fields.

---

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| None found | — | — | — | — |

No TODOs, FIXMEs, placeholders, empty return stubs, or incomplete handlers detected in the modified file.

---

### Human Verification Required

#### 1. Spread Seed Persistence Across Save/Load

**Test:** Save a patch with two AnalogLFO instances, close Rack, reopen, compare output character
**Expected:** Each instance reproduces the exact same drift and waveform character it had before save
**Why human:** Deterministic RNG reproduction requires live runtime comparison — static analysis confirms the code path exists but not that json round-trips correctly in practice

#### 2. Per-Instance Spread Divergence

**Test:** Place two AnalogLFO instances with all knobs at identical positions; listen for 60+ seconds at moderate Drift
**Expected:** The two instances produce audibly different drift patterns and subtle waveform shape differences
**Why human:** Behavioral divergence requires runtime A/B comparison of live audio output

#### 3. DC Offset Wander Visibility

**Test:** Patch a scope module to the LFO output, set Drift to maximum, observe for 60+ seconds
**Expected:** Output center slowly wanders away from 0V by ~50-100mV then drifts back over a ~33s cycle
**Why human:** Slow OU process requires extended live observation; magnitude depends on sample rate and runtime RNG state

#### 4. Pitch Slew Perceptibility

**Test:** Set Rate to a mid-range value, rapidly sweep the Rate knob from low to high at full Drift, then repeat at Drift=0
**Expected:** At high Drift, frequency changes are noticeably sluggish (2ms-300ms lag); at Drift=0, changes are immediate
**Why human:** Perceptual comparison of time constants requires live listening

---

### Gaps Summary

No gaps. All twelve observable truths verified. All key links confirmed wired at the correct signal chain positions. Build is clean with zero compiler errors or warnings. All four CHAR requirements are implemented with the exact patterns specified in the plan frontmatter.

The component spread magnitude values (2% OU weights, 1.5% character/tri-asymmetry, 2% saw curvature, 1% square duty) are noted in SUMMARY.md as conservative starting points pending empirical tuning — this is an acknowledged design decision, not a gap.

---

_Verified: 2026-03-16_
_Verifier: Claude (gsd-verifier)_
