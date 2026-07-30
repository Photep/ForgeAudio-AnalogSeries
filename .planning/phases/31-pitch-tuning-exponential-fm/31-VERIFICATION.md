---
phase: 31-pitch-tuning-exponential-fm
verified: 2026-07-30T05:07:19Z
status: passed
score: 15/15 must-haves verified
behavior_unverified: 0
overrides_applied: 0
---

# Phase 31: Pitch, Tuning & Exponential FM Verification Report

**Phase Goal:** Musical, accurate pitch — 1V/octave tracking with coarse/fine tune and audio-rate exponential FM, all summed in the volt domain before a single exponential, proven correct to within a cent.
**Verified:** 2026-07-30T05:07:19Z
**Status:** passed
**Re-verification:** No — initial verification

## Method

This verification did not trust SUMMARY.md claims. Every load-bearing claim below was
independently re-derived from source (`src/dsp/VcoCore.hpp`, `src/AnalogVCO.cpp`,
`tests/test_vco_pitch.cpp`, `tests/test_vco_core.cpp`, `tests/check_includes.sh`) and, where
possible, re-run from a clean build on this machine rather than read out of a summary:

- `make test`, `make guards`, `make strict` were each run fresh and independently reproduced
  the SUMMARY's exact figures (81 cases / 2,618,053 assertions / 0 failed; guard suite PASS;
  strict PASS).
- A full `make clean && RACK_DIR=../Rack-SDK make` was run to reproduce the plugin link from
  scratch; the resulting `plugin.dylib` is exactly 169,072 bytes, matching 31-08-SUMMARY's
  recorded figure.
- All six phase-gate doctest selectors (`-tc="*v/oct tracking*"`, `*COARSE*`, `*FINE*`,
  `*exponential FM*`, `*Nyquist*`, `*hostile pitch*`) were re-run individually; matched case
  and assertion counts reproduced exactly: 2/603, 1/170, 1/156, 2/494, 1/93, 1/395 — all 0
  failed.
- The doctest selector false-green trap was independently reproduced: `-tc='*this case name
  does not exist anywhere*'` reports `0 | 0 passed | 0 failed | 81 skipped` and still exits
  `Status: SUCCESS!`, confirming the phase gate's own point that exit status alone is not
  evidence and matched-case-count assertion is required.
- `-tc='*golden*'` (9/49188/0) and `-tc='*guardrail*'` (10/60/0) were re-run to independently
  confirm the six shipped-LFO `.f32` goldens replay byte-identical.
- `git diff e881174..HEAD` (pre-phase tip → HEAD) was inspected directly: the only `src/`
  files touched anywhere in the whole phase are `src/dsp/VcoCore.hpp` and `src/AnalogVCO.cpp`.
  No frozen header, `src/AnalogLFO.cpp`, `res/AnalogLFO.svg`, or `plugin.json` appears in the
  diff. `HOSTILE_RATES[]`/`HOSTILE_TIMES[]` in `tests/test_vco_core.cpp` are untouched.

## Goal Achievement

### Observable Truths — the four ROADMAP success criteria

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | V/OCT tracks 1V/oct from `C4=0V→261.6256Hz`, reusing `forge::exp2_taylor5` verbatim, error < 1 cent (TEST-02 phase gate) | ✓ VERIFIED | Exactly one non-comment `exp2_taylor5(...)` call in `src/dsp/VcoCore.hpp` (line 366; confirmed by comment-stripped grep count = 1). `kVcoFreqC4 = 261.6256f`. Primary tier (`tests/test_vco_pitch.cpp` invariant 2) measures sub-sample-interpolated zero crossings on the driver's *returned samples*, ground-truthed against `261.6256 * std::exp2(volts)` in double (libm, never `exp2_taylor5` against itself). Fixed tolerance 0.05 cents, independently re-run: `-tc="*v/oct tracking*"` → 2 cases / 603 assertions / 0 failed. Own measured worst-case figures recorded (0.00967639 c @ 44.1 kHz), 20× inside the 1-cent requirement. |
| 2 | COARSE sweeps ±5 octaves continuous; FINE trims ±1 semitone (±100 cents) | ✓ VERIFIED | `src/AnalogVCO.cpp:111` `configParam(COARSE_PARAM, -5.f, 5.f, 0.f, ...)`, no snap/step flag. `:129` `configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine Tune", " cents", 0.f, 100.f)`. `.planning/REQUIREMENTS.md:18` and `.planning/ROADMAP.md` Phase 31 SC2 both correctly read **±1 semitone (±100 cents)**, confirming D-00's pre-planning correction from the stale ±2 landed. Invariant 4/5 in `tests/test_vco_pitch.cpp` measure COARSE across the full −5..+5 range including non-integer values, and FINE's ±1 semitone against exactly ±100 cents (not ±200). Re-run: `-tc="*COARSE*"` → 1/170/0; `-tc="*FINE*"` → 1/156/0. |
| 3 | Exponential FM (input × bipolar attenuverter × depth) sums into the pitch volt domain **before** the single `exp2_taylor5` call | ✓ VERIFIED | `VcoCore.hpp:339-340`: `float pitchVolts = in.pitchCV + in.coarse + in.fine * (1.f/12.f); if (in.fmConnected) pitchVolts += in.fmVolts * in.fmAtten;` — summed in volts, then one exponential at line 366. FM-03 identity (invariant 6) proves `fmVolts=V @ fmAtten=1` is bit-exact to `pitchCV += V` with FM unpatched, using a grid deliberately built from FRACTIONAL volt pairs (verified by reading `FM_IDENTITY_GRID[]`), with blind (whole-number) rows explicitly marked and excluded from the evidentiary claim. The negative control (`DeliberatelyMultiplicativeFmCore`, confined to an anonymous namespace inside the test TU) is REQUIRED to fail the identity on the fractional rows (`REQUIRE(fractionalMismatchTotal > 0)`, invariant 7). Re-run: `-tc="*exponential FM*"` → 2 cases / 494 assertions / 0 failed (invariant 6 + invariant 7 together). |
| 4 | Frequency clamped just below Nyquist; phase accumulates in double precision | ✓ VERIFIED | `kVcoNyquistGuardFrac = 0.495f` (D-11), `PROVISIONAL` comment removed (grep confirms zero hits). Invariant 8 proves the clamp FIRES exactly at the derived ceiling (`telFreq == expectedMaxFreq`, recomputed symbolically) while the oscillator keeps oscillating (`nUp >= 8` precondition, non-constant block, peak > 1 V audible), and proves it does NOT fire one volt below (strict inequality + libm-referenced tracking). Re-run: `-tc="*Nyquist*"` → 1/93/0. `phase` is `double` (`VcoCore.hpp:248`), pinned by `static_assert(std::is_same<decltype(forge::VcoCore::phase), double>::value, ...)` in invariant 9's compile-time check, plus a runtime 100,000-step non-regression pin. Re-run: `-tc="*hostile pitch*"` → 1/395/0. |

**Score:** 4/4 ROADMAP success criteria verified, all with independently reproduced test evidence (not merely read from SUMMARY).

### The Six Non-Vacuity Requirements (31-VALIDATION.md) — independently re-checked against source

| # | Requirement | Status | Evidence |
|---|---|---|---|
| a | Primary tier measures the OUTPUT, not `tel.freqHz` | ✓ VERIFIED | `grep -n "tel\.freqHz" tests/test_vco_pitch.cpp` returns 6 hits, all inside invariant 3 (secondary tier, line 1027), invariant 8 (lines 2217/2279), and invariant 9 (lines 2579/2588/2589) — none inside invariant 2 (primary tier, lines 839-928), which calls `estimateFreqRising(out, ...)` on the driver's returned `std::vector<float>`. Note: the 31-05-SUMMARY claim ("exactly once, line 686") was accurate only as of plan 31-05, before invariants 8/9 (31-07) legitimately added their own, separately-justified `tel.freqHz` reads for the clamp-fires and hostile-plateau checks. This is not a regression of TRAP 1 — the TEST-02 primary gate itself still never reads telemetry. |
| b | Ground truth from libm (`std::exp2` in double), never `exp2_taylor5` against itself | ✓ VERIFIED | `expectedFreqHz()` (line 329) is `261.6256 * std::exp2(volts)`. `grep` for `std::exp2\|std::pow` in `src/` finds only the pre-existing, out-of-phase-diff `src/dsp/Anim.hpp:40`. |
| c | Expectations one octave apart across the grid | ✓ VERIFIED | `kGridStepVolts = 0.5` (half a volt = half an octave); invariant 2 asserts `CHECK(expectedRatio > 1.4)` between consecutive grid expectations before any tolerance check. |
| d | `REQUIRE(nUp >= 8)` precedes any tolerance check | ✓ VERIFIED | 5 occurrences in the file (lines 921, 1230, 1376, 1775, 2252), each preceding its tolerance/equality assertion in source order. |
| e | FM negative control is test-TU-only, anonymous namespace, observed FAILING the identity | ✓ VERIFIED | `DeliberatelyMultiplicativeFmCore` (line 646) sits inside the file's single anonymous namespace (opens line 153, closes line 719); the banner states containment is asserted by a recursive grep for the type's name under `src/`, none found. Invariant 7 requires `fractionalMismatchTotal > 0` (`REQUIRE`, line 2030) — independently re-run and green. |
| f | Clamp-boundary case proving the clamp FIRES (`tel.freqHz` equals `kVcoNyquistGuardFrac * sampleRate` exactly, oscillator keeps oscillating) | ✓ VERIFIED | Invariant 8, lines 2217-2257: `telFreq == expectedMaxFreq` (exact float equality, recomputed symbolically), `REQUIRE(nUp >= 8)` before any CHECK, `blockIsNotConstant`, `peakAudible` (`peakAbs > 1.0`). |

**Score:** 6/6 non-vacuity requirements verified.

### The Three Named Traps — independently re-verified

| Trap | Claim | Independent finding |
|---|---|---|
| TRAP 5 — FM identity blind to multiplicative implementation on whole-number terms | Grid uses fractional volt pairs, blind rows marked, negative control fails on sighted rows | Confirmed. `FM_IDENTITY_GRID[]` (lines 560-569) has 4 `blindRow: true` rows (whole-number terms, including the V/OCT-at-0-default row) and 4 `blindRow: false` fractional rows. Invariant 7 accumulates mismatches only over `!blindRow` rows and `REQUIRE`s a non-zero total — independently re-run, green. |
| Doctest selector false-green | A selector matching zero cases exits 0 and prints `Status: SUCCESS!`; the gate asserts matched case COUNTS, not exit status | Confirmed by direct re-run: `./build-test/test -tc='*this case name does not exist anywhere*'` → `0 \| 0 passed \| 0 failed \| 81 skipped`, `Status: SUCCESS!`, exit 0. 31-08-SUMMARY records per-selector matched counts (2,1,1,2,1,1) as the gate; independently reproduced identically above. |
| Install-freshness near-miss | Stale artefact satisfied 2 of 3 freshness facts at the same byte size; freshness proof rests on whole-tree byte equality | Confirmed by reading `31-09-SUMMARY.md`: both dylibs measured at 175,056 bytes with identical export addresses; freshness was escalated to `diff -r dist/ForgeAudio-AnalogSeries <installed>` (whole-tree byte equality, run twice) plus an 18-file aggregate SHA-256 digest, not to the original 3-fact sampled check. |

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `src/dsp/VcoCore.hpp` | Four-term volt-domain pitch sum, one `exp2_taylor5` call, D-14 bound, D-11 clamp policy | ✓ VERIFIED | Read in full; matches every D-01/D-05/D-06/D-09/D-10/D-11/D-14 claim. |
| `src/AnalogVCO.cpp` | Four new controls declared, zero arithmetic in the shell | ✓ VERIFIED | Read in full; `process()` contains only bare field assignments; banner's "THIS FILE DOES NO DSP" holds. |
| `tests/test_vco_pitch.cpp` | New file, 9 invariants, TEST-02 gate | ✓ VERIFIED | 2713 lines, 9 `TEST_CASE`s confirmed by grep, matching the claimed structure exactly. |
| `tests/check_includes.sh` | `VCO_SIDE_ALLOW` entry for the new test file | ✓ VERIFIED | `"tests/test_vco_pitch.cpp"` present in the array (line 330); `make guards` independently re-run, PASS. |
| `res/AnalogVCO.svg` | Four new marker rects | ✓ VERIFIED (via `make guards`/panel load) | Not separately opened as an image but structurally corroborated by the widget-to-rect pairing comment in `AnalogVCO.cpp` and a successful plugin link. |
| `.planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md` | Register of out-of-scope discoveries | ✓ VERIFIED | 15 items, none of which cover an in-scope requirement (PITCH-01..05, FM-01..03, TEST-02) — all are legitimately deferred (shipped-LFO UB, half-closed `forge::clamp`, hostile-timing-grid extension, seed entropy, panel affordance, etc.), each pointed at a specific later phase or explicitly unowned by decision. |

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `in.fmConnected` | `pitchVolts` | `if (in.fmConnected) pitchVolts += in.fmVolts * in.fmAtten;` | ✓ WIRED | D-09's gate lives in the core, confirmed structurally and behaviorally (D-09 SUBCASE: any hostile FM voltage/atten is a bit-exact no-op when unpatched). |
| `pitchVolts` (summed) | `exp2_taylor5(pitchVolts)` | the D-14 bound pair immediately precedes the single call | ✓ WIRED | Lines 363-366. |
| `forge::kVcoNyquistGuardFrac` | `tests/test_vco_pitch.cpp`'s sweep ceilings | `clampCeilingVolts()`/`estimatorCeilingVolts()` read the constant symbolically | ✓ WIRED | D-21 honored; no hardcoded Hz/volt literal found in the gate. |
| `AnalogVCO.cpp::process()` | `forge::VcoCore::step()` | 7 of 8 `VcoInputs` DSP fields forwarded raw | ✓ WIRED | Confirmed by direct read; `drift` is the one deliberately-unfed field, tracked as deferred item 9. |

### Requirements Coverage

| Requirement | Source Plan(s) | Status | Evidence |
|---|---|---|---|
| PITCH-01 | 31-03, 31-05, 31-08 | ✓ SATISFIED | TEST-02 primary/secondary tiers, single exp2_taylor5. |
| PITCH-02 | 31-04, 31-06, 31-08, 31-09 | ✓ SATISFIED | COARSE −5..+5 continuous, invariant 4, operator UAT. |
| PITCH-03 | 31-04, 31-06, 31-08, 31-09 | ✓ SATISFIED | FINE ±1 semitone/±100 cents, invariant 5, operator UAT. |
| PITCH-04 | 31-02, 31-07, 31-08, 31-09 | ✓ SATISFIED | kVcoNyquistGuardFrac=0.495f settled, invariant 8, operator UAT (deep-FM flattening observed and disclosed). |
| PITCH-05 | 31-03, 31-07, 31-08 | ✓ SATISFIED | double phase, compile-time + runtime pins, unchanged since Phase 30. |
| FM-01 | 31-04, 31-06, 31-08, 31-09 | ✓ SATISFIED | 1 V/full-atten measured as exactly 1 octave; audio-rate square modulator case; operator UAT. |
| FM-02 | 31-04, 31-06, 31-08, 31-09 | ✓ SATISFIED | Bipolar −1..+1, inversion + sign-difference + zero-no-op all bit-exact; operator UAT (affordance-only observation routed to Phase 35, behavior not reopened). |
| FM-03 | 31-03, 31-06, 31-08 | ✓ SATISFIED | Summation identity + non-vacuous negative control (TRAP 5). |
| TEST-02 | 31-01, 31-05, 31-08 | ✓ SATISFIED | Phase gate: hard gate on matched case counts, independently re-run. |

No orphaned requirements: `.planning/REQUIREMENTS.md` maps exactly these nine IDs to Phase 31, all marked Complete, all accounted for above.

### Milestone Guardrail (v2.0 LFO non-regression)

| Check | Status | Evidence |
|---|---|---|
| No edit to `src/dsp/LfoCore.hpp`, `Waveshape.hpp`, `RackCompat.hpp`, `src/AnalogLFO.cpp`, `res/AnalogLFO.svg`, `src/dsp/FROZEN.sha256`, `plugin.json` | ✓ VERIFIED | `git diff e881174..HEAD --stat` shows only `src/AnalogVCO.cpp` and `src/dsp/VcoCore.hpp` touched under `src/`, plus `tests/`, `res/AnalogVCO.svg`, and `.planning/`. |
| Six LFO `.f32` goldens replay byte-identical | ✓ VERIFIED | Independently re-run: `-tc="*golden*"` → 9/49188/0; `-tc="*guardrail*"` → 10/60/0. |
| `must_haves.prohibitions` hold | ✓ VERIFIED | No frozen header in diff; `HOSTILE_RATES[]`/`HOSTILE_TIMES[]` byte-identical (zero diff hits); `kVcoMaxDeltaPhase` unchanged at `0.5` (D-12); no in-class `static constexpr` table added; no `std::exp2`/`std::pow` introduced in `src/`; no permanent `-fsanitize=undefined` wiring in `Makefile` or `.github/workflows/` (both greps empty). |

### Toolchain / C++11 Cleanliness

| Check | Status | Evidence |
|---|---|---|
| No `inline constexpr` in `src/` | ✓ VERIFIED | Zero non-comment hits. |
| No in-class `static constexpr` table added by this phase | ✓ VERIFIED | `src/dsp/VcoCore.hpp` and `src/AnalogVCO.cpp` contain none; pre-existing scalar in-class `static constexpr` in unrelated frozen headers (`RackCompat.hpp`, `DriftEngine.hpp`, `ClockTracker.hpp`) are untouched by this phase's diff. |
| No `std::exp2`/`std::pow` in `src/` except the pre-existing `Anim.hpp:40` | ✓ VERIFIED | Confirmed by grep; `Anim.hpp` is unrelated shipped-LFO code, outside the phase diff. |
| `make strict` (C++11 `-pedantic-errors`) | ✓ VERIFIED | Independently re-run, PASS. |
| Real plugin link (`RACK_DIR=../Rack-SDK make`) | ✓ VERIFIED | Independently rebuilt from clean; `plugin.dylib` = 169,072 bytes, matching the recorded figure. |

### D-18 Self-Measurement

✓ VERIFIED. `tests/test_vco_pitch.cpp`'s own comment blocks (lines 115-129, 174-203) record this phase's own measured worst-case cents figures per tier per rate. Neither of the two prior-milestone research figures (~1e-6 / ~1e-4 relative error) is cited anywhere in the delivered test file or `VcoCore.hpp` (grep for both figures returns zero hits in either file).

### Anti-Patterns Found

None. `grep` for `TBD|FIXME|XXX` and `TODO|HACK|PLACEHOLDER|placeholder|coming soon|not yet implemented` across `src/AnalogVCO.cpp`, `src/dsp/VcoCore.hpp`, and `tests/test_vco_pitch.cpp` returns no debt markers. The one "placeholder" hit is `VcoCore.hpp:20`, which reads "...is final rather than a placeholder" — describing the opposite of a stub.

### Human Verification Required

None outstanding. The phase's own Manual-Only Verification item (musical feel of COARSE/FINE/FM in a real patch) was already discharged inside this phase by plan 31-09's operator in-Rack UAT: **approved, with one observation** (the FM DEPTH knob's bipolar behavior was unexpected from its widget). That observation was correctly split into a closed BEHAVIOR (FM-02's bipolarity — locked, verified bit-exact, not reopened) and an open AFFORDANCE (routed to Phase 35 as deferred item 14). The shipped Analog LFO was confirmed visually and audibly unchanged in the same session.

### Gaps Summary

No gaps found. All four ROADMAP success criteria, all nine requirement IDs, and all six 31-VALIDATION.md non-vacuity requirements are independently verified against source and against fresh local test/build runs — not merely read from SUMMARY.md. The three traps this phase's own executors flagged as the highest-risk spots (the FM identity's blind rows, the doctest selector false-green, and the install-freshness near-miss) were each independently re-derived and confirmed correctly handled. The milestone guardrail (shipped LFO non-regression) holds across the whole phase diff.

---

_Verified: 2026-07-30T05:07:19Z_
_Verifier: Claude (gsd-verifier)_
