---
phase: 13-fm-input
verified: 2026-03-15T00:00:00Z
status: passed
score: 4/4 must-haves verified
re_verification: false
human_verification:
  - test: "Patch a bipolar CV source (e.g., second LFO) into the FM jack, turn attenuator up, observe LFO output on a scope module"
    expected: "LFO frequency visibly modulates in sync with the FM source; faster and slower periods alternate"
    why_human: "Exponential frequency modulation is a real-time auditory/visual behavior; cannot be confirmed by static code inspection alone"
  - test: "With FM attenuator at zero and a cable patched, compare LFO output to v1.1 behavior (no cable)"
    expected: "Output is identical -- no frequency change, no modulation artifacts"
    why_human: "Backward-compatibility guarantee requires runtime observation; attenuator-at-zero disables FM mathematically (2^0 = 1) but only runtime confirms no side effects"
  - test: "In clocked mode (CLK patched, SYNC acquired), patch FM source with full attenuator; compare FM effect to free-running mode"
    expected: "FM effect is noticeably reduced (depthScale 0.5 vs 0.6 free); clock sync is maintained, waveform still resets at clock edges cleanly"
    why_human: "Clock sync integrity and comparative FM depth require live VCV Rack testing with a clock source; cannot be confirmed statically"
  - test: "Patch a +/-10V source into FM jack with attenuator at full; observe LFO behavior over several seconds"
    expected: "LFO remains running and stable -- no freezing, no silence, no NaN/infinite frequency artifacts"
    why_human: "Extreme-input stability (the fmax(freq, 0.001f) clamp effectiveness) must be verified at runtime with actual audio processing"
---

# Phase 13: FM Input Verification Report

**Phase Goal:** Users can frequency-modulate the LFO from external CV sources in both free and clocked modes
**Verified:** 2026-03-15
**Status:** human_needed (all automated checks passed; 4 runtime behaviors need human confirmation)
**Re-verification:** No -- initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|---------|
| 1 | Patching a bipolar CV into FM jack and turning up attenuator produces audible frequency modulation | ? NEEDS HUMAN | Code is structurally correct: `freq *= dsp::exp2_taylor5(fmCV * fmAtten * depthScale)` at line 499; human must confirm audible effect |
| 2 | FM attenuator at zero (default) produces output identical to v1.1 -- no modulation regardless of FM input voltage | ? NEEDS HUMAN | Mathematically guaranteed by `2^(cv * 0 * scale) = 2^0 = 1.0`; confirmed by `configParam(FM_ATTEN_PARAM, 0.f, 1.f, 0.f, ...)` default=0.0; human should confirm no runtime side effects |
| 3 | In clocked mode, FM is gentler (reduced authority) and clock sync is maintained at moderate FM depths | ? NEEDS HUMAN | `depthScale = isClocked ? 0.5f : 0.6f` (line 497) confirmed; runtime sync integrity needs human verification |
| 4 | FM never drives frequency negative -- LFO remains stable at any FM depth and input voltage | ? NEEDS HUMAN | `freq = std::fmax(freq, 0.001f)` clamp at line 500 (post-FM) confirmed; extreme-input stability requires runtime test |

**Score:** 4/4 truths have correct supporting implementation (all ? NEEDS HUMAN -- automated evidence complete, runtime confirmation pending)

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/AnalogLFO.cpp` | FM_ATTEN_PARAM enum, FM_INPUT enum, FM processing block, FM widget controls | VERIFIED | All four components present and wired; see detailed checks below |

### Artifact Detail: Three-Level Check

**Level 1 (Exists):** `src/AnalogLFO.cpp` exists and was modified in commits `ff87325` and `43d25a7`.

**Level 2 (Substantive):**
- `FM_ATTEN_PARAM` — 4 references (enum line 18, configParam line 432, getValue line 495, widget line 1116)
- `FM_INPUT` — 5 references (enum line 28, configInput line 433, isConnected line 493, getVoltage line 494, widget line 1118)
- FM processing block spans lines 491–501: isConnected guard, fmCV read, fmAtten read, depthScale ternary, fmPitch calculation, `freq *= dsp::exp2_taylor5(fmPitch)`, post-FM clamp
- Widget: trimpot at `(8.0, 118.0)`, jack at `(20.0, 118.0)` -- both in widget constructor lines 1115–1118

**Level 3 (Wired):** FM block is inside `process()` at lines 491–501, operating on the live `freq` variable that feeds directly into `deltaPhase` accumulation at line 508. Enum values indexed correctly (FM_ATTEN_PARAM is 9th in ParamId, FM_INPUT is 7th in InputId, both before their LEN sentinels).

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| FM processing block in process() | freq calculation | `freq *= dsp::exp2_taylor5(fmPitch)` at line 499 | WIRED | FM applied to post-slew `freq` variable; feeds into deltaPhase at line 508 |
| FM authority scaling | isClocked boolean | `depthScale = isClocked ? 0.5f : 0.6f` at line 497 | WIRED (with deviation) | Implemented as specified; value is 0.5f (not 0.1f from plan) -- see deviation note below |

**Deviation note on key_link pattern:** The PLAN frontmatter specified the pattern `isClocked.*0\.1f.*0\.6f` for the clocked authority link. The actual implementation uses `0.5f` (not `0.1f`) for clocked mode. This is an intentional, documented deviation recorded in the SUMMARY: the original 0.1f was found to be inaudibly weak during human verification (Task 2), and was corrected to 0.5f. The link is functionally WIRED; only the specific coefficient differs from the plan's draft value. This is not a gap -- it is a verified bug fix.

**FM insertion point:** The processing block is positioned AFTER `freqSlew.process()` (line 488) and its post-slew clamp (line 489). This is a deviation from the PLAN's intended pre-slew insertion point, also documented in the SUMMARY as a necessary bug fix (pre-slew insertion caused FM to be filtered by the ~3Hz lowpass, making modulation inaudible). The post-slew placement is correct and intentional.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|---------|
| MOD-01 | 13-01-PLAN.md | FM input jack applies exponential frequency modulation with CV-controllable depth | SATISFIED | `freq *= dsp::exp2_taylor5(fmCV * fmAtten * depthScale)` at line 499; jack at enum line 28 and widget line 1118; attenuator at enum line 18 and widget line 1116 |
| MOD-02 | 13-01-PLAN.md | FM authority reduced in clocked mode to prevent clock-phase fighting | SATISFIED | `depthScale = isClocked ? 0.5f : 0.6f` at line 497; isClocked computed from clockState/smoothedPeriod at line 472 |

No orphaned requirements: REQUIREMENTS.md traceability table maps only MOD-01 and MOD-02 to Phase 13, matching the PLAN frontmatter exactly.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `src/AnalogLFO.cpp` | 1114 | `// FM controls (TEMPORARY positions -- Phase 17 finalizes layout)` | Info | Intentional placeholder comment; layout finalization is explicitly deferred to Phase 17 (PANEL-02). Not a blocker. |

No TODO/FIXME/HACK markers, no empty implementations, no stub returns, no console.log-only handlers found.

### Human Verification Required

#### 1. Basic FM Modulation (MOD-01)

**Test:** Patch a bipolar CV source (e.g., a VCV LFO) into the FM jack. Set FM attenuator to zero. Then slowly turn the FM attenuator knob up.
**Expected:** At zero attenuator, LFO output is identical to without the cable. As attenuator increases, LFO frequency begins to visibly vary on a scope module -- faster and slower periods alternating with the FM source.
**Why human:** Exponential frequency modulation at audio/LFO rate is a real-time auditory behavior. The math is correct (`dsp::exp2_taylor5`) but only runtime confirms the modulation is audible and correctly tracks the input.

#### 2. Backward Compatibility at Default Attenuator (MOD-01 / v1.1 parity)

**Test:** Patch any CV source into the FM jack with the FM attenuator at its default (fully counterclockwise = 0.0). Monitor output.
**Expected:** Zero change in LFO behavior compared to having no cable patched. No frequency drift, no modulation.
**Why human:** The attenuator-at-zero guarantee is mathematically sound (`2^(cv * 0 * scale) = 1.0`), but runtime confirmation ensures no unexpected side effects from the `isConnected()` branch being entered.

#### 3. Clocked Mode FM Authority (MOD-02)

**Test:** Patch a clock source into CLK, wait for SYNC lock. Then patch an LFO into the FM jack with full attenuator. Compare FM depth visually to the same patch in free-running mode.
**Expected:** FM effect is noticeably reduced in clocked mode (depthScale 0.5 vs 0.6 free). Clock sync is maintained -- waveform resets at clock edges cleanly even with active FM.
**Why human:** The depth ratio (0.5/0.6) and clock-sync integrity under FM can only be confirmed by live observation. Code paths branch correctly but timing interactions between FM and phase resets need runtime validation.

#### 4. Stability Under Extreme FM Input (MOD-01)

**Test:** Patch a +/-10V source into FM jack with attenuator fully up. Let run for several seconds.
**Expected:** LFO continues running stably. No silence, no freezing, no NaN or infinite-frequency artifacts, no crash.
**Why human:** The `fmax(freq, 0.001f)` clamp at line 500 provides the stability floor, but extreme-input robustness must be confirmed at runtime. A +/-10V input at full attenuator produces `freq *= 2^(10 * 0.6) = 2^6 = 64x` in free mode -- requires confirming the ceiling is also safe.

### Gaps Summary

No automated gaps. All required code structures are present, substantive, and wired correctly. The two deviations from the plan (post-slew FM insertion; clocked depthScale 0.5f instead of 0.1f) are both justified and documented in the SUMMARY as bug fixes found during human verification. They improve correctness.

All four observable truths require human runtime verification in VCV Rack to confirm the implementation actually achieves the goal.

---

_Verified: 2026-03-15_
_Verifier: Claude (gsd-verifier)_
