---
phase: 15-waveform-bleed
verified: 2026-03-17T00:00:00Z
status: human_needed
score: 4/5 must-haves verified
re_verification: null
gaps: []
human_verification:
  - test: "With Character above zero and Morph at an intermediate position (~50%), the waveform display shows subtle influence from neighboring shapes — the trace looks slightly 'impure' compared to Character=0"
    expected: "Waveform display reflects neighbor bleed at Character > 0; sweeping Morph 0-100% shows bleed present at all intermediate positions"
    why_human: "Visual comparison of waveform shape requires runtime display observation in VCV Rack"
  - test: "At Character=0, sweep Morph from 0% to 100% and observe display"
    expected: "Crisp, clean linear crossfade with zero neighbor influence — identical to v1.1 behavior"
    why_human: "Regression check against prior behavior requires visual confirmation"
  - test: "Patch LFO output to a scope module with Character=100% and Morph=50%; sweep all parameter combinations"
    expected: "Output stays within +/-5V at all morph/character/bleed combinations"
    why_human: "Amplitude envelope under all parameter combinations requires real-time signal observation"
  - test: "Place two Analog LFO instances with identical settings (Character=100%, Morph=50%, Rate=2Hz); compare waveform displays"
    expected: "Both instances show slightly different bleed character due to component spread (bleedSpread differs per seed)"
    why_human: "Per-instance variation requires visual side-by-side display comparison"
  - test: "With Character=100% and Morph=50%, observe the display for 30+ seconds"
    expected: "Bleed magnitude fluctuates slowly — the crossfade 'impurity' drifts visibly over the observation window"
    why_human: "Temporal OU-layer modulation of bleed intensity requires sustained real-time observation"
---

# Phase 15: Waveform Bleed Verification Report

**Phase Goal:** Add waveform bleed — adjacent-shape crosstalk during morph transitions controlled by Character knob (CHAR-05)
**Verified:** 2026-03-17
**Status:** human_needed — all automated checks pass; five behavioral truths require VCV Rack observation
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | With Character above zero and intermediate Morph, waveform trace shows adjacent-shape influence | ? NEEDS HUMAN | Code path confirmed: bleed logic runs when character >= 0.001f; display calls computeMorphedWave() directly. Behavior requires VCV Rack visual confirmation. |
| 2 | At Character=0, morph is a crisp crossfade with no bleed — identical to v1.1 | ? NEEDS HUMAN | `character >= 0.001f` gate verified in code; at Character=0 the function returns the pre-bleed result unchanged. Runtime regression check requires VCV Rack observation. |
| 3 | Output stays within +/-5V at all morph/character/bleed combinations | ✓ VERIFIED | Normalization `result /= (1.f + bleedIntensity)` at line 307 mathematically guarantees result stays in [-1, 1]; scaling to +/-5V is therefore bounded. bleedIntensity is clamped non-negative at line 293 ensuring divisor >= 1. |
| 4 | Two instances with identical knob settings show slightly different bleed magnitude due to component spread | ? NEEDS HUMAN | bleedSpread is initialized from the per-instance spreadSeed RNG (line 179) — different seeds produce different values. Confirmed by code; visual side-by-side comparison still needed for human sign-off. |
| 5 | Bleed magnitude fluctuates slowly over time via OU layer 0 modulation | ? NEEDS HUMAN | `bleedIntensity *= (1.f + ouLayers[0].state * 0.2f)` at line 292 confirmed. ouLayers[0] runs at 0.05 Hz (~20s cycle). Slow temporal variation requires sustained observation to confirm. |

**Score:** 1/5 truths fully verified by static analysis; 4/5 require human runtime confirmation. Automated infrastructure for all 5 truths is verified correct.

---

## Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/AnalogLFO.cpp` | Waveform bleed in computeMorphedWave(), bleedSpread member, initComponentSpread() addition | ✓ VERIFIED | File exists (1254 lines). All three required additions present and substantive. |

### Artifact Detail: src/AnalogLFO.cpp

**Level 1 — Exists:** Yes (1254 lines, full implementation file)

**Level 2 — Substantive:** All acceptance criteria confirmed:

| Criterion | Line | Status |
|-----------|------|--------|
| `float bleedSpread = 0.f;` member variable | 129 | ✓ |
| `bleedSpread = d(spreadRng) * 0.02f;` as LAST call in initComponentSpread() | 179 | ✓ |
| `float shapes[4] = { sine, tri, saw, sqr };` | 274 | ✓ |
| `if (character >= 0.001f)` gate | 284 | ✓ |
| `float effectiveBleed = std::fmax(0.f, 0.04f + bleedSpread);` | 288 | ✓ |
| `bleedIntensity *= (1.f + ouLayers[0].state * 0.2f);` | 292 | ✓ |
| `int leftIdx  = (segment - 1 + 4) % 4;` | 296 | ✓ |
| `int rightIdx = (segment + 2) % 4;` | 297 | ✓ |
| `result /= (1.f + bleedIntensity);` (normalization) | 307 | ✓ |
| Old switch/case removed from computeMorphedWave() | — | ✓ (grep confirms absent) |

**Level 3 — Wired:**

- computeMorphedWave() called from display buffer path at line 317 (`displayBuffers[writeIdx][i] = computeMorphedWave(p, morph, character)`)
- computeMorphedWave() called from audio output path at line 707 (`float sample = computeMorphedWave(p, morph, character)`)
- bleedSpread initialized in initComponentSpread() (line 179) which is called during module initialization
- bleedSpread consumed directly in computeMorphedWave() at line 288 via member access

Status: ✓ WIRED at all three levels

---

## Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `computeMorphedWave()` | `bleedSpread` member | direct member access | ✓ WIRED | Line 288: `std::fmax(0.f, 0.04f + bleedSpread)` |
| `computeMorphedWave()` | `ouLayers[0].state` | direct member access | ✓ WIRED | Line 292: `bleedIntensity *= (1.f + ouLayers[0].state * 0.2f)` |
| `computeMorphedWave()` | `progressiveCurve(character)` | Character-gated bleed intensity | ✓ WIRED | Line 285: `float c = progressiveCurve(character)` inside the character gate |
| `initComponentSpread()` | `bleedSpread` | normal distribution from spreadRng | ✓ WIRED | Line 179: `bleedSpread = d(spreadRng) * 0.02f` — confirmed as last RNG call |

All four key links from PLAN frontmatter: WIRED.

---

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| CHAR-05 | 15-01-PLAN.md | Waveform bleed introduces adjacent-shape crosstalk during morph transitions | ✓ SATISFIED | computeMorphedWave() implements neighbor bleed gated by Character; compile clean; all code-verifiable acceptance criteria pass |

**Orphaned requirements check:** REQUIREMENTS.md maps CHAR-05 to Phase 15 only. No additional requirement IDs assigned to Phase 15. No orphaned requirements.

---

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| src/AnalogLFO.cpp | 1215 | Comment mentions "placeholder rect" | ℹ️ Info | Refers to the SVG display widget layout, not a code placeholder — no impact on bleed implementation |

No blocker or warning anti-patterns found in bleed-related code.

---

## Compile Verification

`make` (forced recompile): **CLEAN** — zero errors, zero warnings under `-Wall -Wextra`.

Commit `50fd1b6` exists in git history: `feat(15-01): add waveform bleed in computeMorphedWave()`

---

## Human Verification Required

All five observable truths require VCV Rack observation. The code paths are correct but behavior cannot be confirmed without running the module.

### 1. Bleed Visible on Display (SC1)

**Test:** Set Character to 100%, Morph to ~50% (tri-to-saw segment), Rate to ~2Hz. Observe the waveform display. Then sweep Morph slowly from 0% to 100%.
**Expected:** Display trace shows subtle influence from neighboring shapes — the crossfade looks slightly "impure" at intermediate Morph positions. Bleed present across all intermediate positions.
**Why human:** Visual comparison of waveform shape requires runtime display observation in VCV Rack.

### 2. Clean Crossfade at Character=0 (SC2)

**Test:** Set Character to 0%. Sweep Morph from 0% to 100%.
**Expected:** Crisp, clean linear crossfade — identical to v1.1. No neighbor influence visible at any Morph position.
**Why human:** Behavioral regression against prior behavior requires visual confirmation.

### 3. Amplitude Safety (SC3)

**Test:** Patch LFO output to a scope module (e.g., Fundamental Scope). Set Character=100%, Morph=50%. Sweep all parameter combinations.
**Expected:** Output stays within +/-5V at all morph/character/bleed combinations.
**Why human:** Real-time signal level measurement on scope required. (Note: mathematically guaranteed by normalization at line 307, but scope confirmation is the acceptance criterion.)

### 4. Component Spread — Per-Instance Variation

**Test:** Add a second Analog LFO instance. Set both to identical settings: Character=100%, Morph=50%, Rate=2Hz. Compare waveform displays side by side.
**Expected:** Both instances show slightly different bleed character — different crossfade "impurity" profile — due to differing bleedSpread values from distinct spreadSeed RNGs.
**Why human:** Per-instance visual difference requires side-by-side display comparison in VCV Rack.

### 5. Temporal Bleed Modulation

**Test:** Set Character=100%, Morph=50%. Watch the display for 30+ seconds.
**Expected:** Bleed magnitude fluctuates slowly — the crossfade "impurity" drifts over the observation window, driven by OU layer 0 at ~20s cycle.
**Why human:** Slow temporal modulation (20s cycle) requires sustained real-time observation to confirm.

---

## Automated Verification Summary

All code-verifiable items pass:

- bleedSpread member variable: present at line 129
- bleedSpread initialization in initComponentSpread() as last RNG call: confirmed at line 179
- computeMorphedWave() full bleed implementation: all nine acceptance-criteria patterns confirmed
- Old switch/case removed: confirmed absent
- All four key links wired: confirmed
- CHAR-05 requirement: satisfied by implementation
- Compile: clean (zero errors, zero warnings)
- Commit 50fd1b6: exists in git history

The five observable truths requiring VCV Rack observation are structurally supported by the code. Human sign-off closes this phase.

---

_Verified: 2026-03-17_
_Verifier: Claude (gsd-verifier)_
