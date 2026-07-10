---
phase: 22-test-harness-foundation
verified: 2026-06-14T11:00:00Z
status: passed
score: 10/10
overrides_applied: 0
re_verification: null
---

# Phase 22: Test Harness Foundation — Verification Report

**Phase Goal:** A standalone C++ test target builds and runs against a Rack-independent DSP core, with a headless block-driver harness — the safety net every later phase depends on.
**Verified:** 2026-06-14
**Status:** PASSED
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | `make test` builds and runs a standalone doctest binary green, WITHOUT touching or breaking the existing plugin build, and the test target links no libRack and needs no RACK_DIR | VERIFIED | `make test` exits 0: 35 cases / 1,647,397 assertions / SUCCESS. `make` builds plugin.dylib. Makefile guards `include $(RACK_DIR)/plugin.mk` with `ifeq ($(filter test,...))` so CI runners without Rack-SDK pass — confirmed via `make test RACK_DIR=/nonexistent` (green). TEST_CXXFLAGS contains no `-I$(RACK_DIR)/include`. |
| 2 | A Rack-independent `src/dsp/` core (header-only, `namespace forge`, zero `rack/` includes) exists and is consumed by the plugin shell — the plugin still builds and loads unchanged, and the inline DSP is deleted | VERIFIED | `grep -L 'rack/' src/dsp/*.hpp` lists all 7 headers (RackCompat, Waveshape, RatioTable, Swing, ClockTracker, DriftEngine, LfoCore). `src/AnalogLFO.cpp` contains `#include "dsp/LfoCore.hpp"` and `core.step(` at line 306. `grep -c 'float computeMorphedWave'` returns 0; `grep -c 'void processClockInput'` returns 0. Plugin builds `plugin.dylib` clean. |
| 3 | `tests/BlockDriver.hpp` drives `forge::LfoCore` over sample blocks, injecting `sampleTime = 1/sampleRate` per sample, and seeding deterministically | VERIFIED | `tests/BlockDriver.hpp` defines `struct BlockDriver` with `run(nSamples, inputAt)` that sets `in.sampleTime = dt` (where `dt = 1/sampleRate`) per sample. Constructor seeds drift + spread with non-zero defaults. `clockedScenario` static helper confirmed. |
| 4 | Invariant tests assert ±5V output bounds, frequency accuracy, phase continuity at reset, and fixed-seed determinism, parametrized over 44.1/48/96 kHz | VERIFIED | `tests/test_invariants.cpp` defines 6 TEST_CASEs covering all 4 invariants over `SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0}`. All pass under `make test`. Bounds: [-5.12, +5.12] drift-on, ±5.0+1e-4 drift-off. Freq: ±1% over ≥4 s. Continuity: `|dOut| ≤ 0.5V` on sine+drift-off. Determinism: same seed → bit-exact, different seed → diverges. |
| 5 | Invariant tests pass at 44.1 / 48 / 96 kHz | VERIFIED | `make test` exits 0 with all 35 test cases passing, including all invariant cases at every rate. |
| 6 | D-08 golden-output regression exists and is bit-exact on canonical OS | VERIFIED | Three `.f32` golden files exist at `tests/golden/freerun_{44100,48000,96000}.f32`, each exactly 32768 bytes (8192 samples × 4 bytes). `git ls-files tests/golden/` confirms all four golden files are tracked (not gitignored). `tests/test_golden.cpp` loads each file and replays with canonical seeds; uses direct `==` comparison (not Approx) for bit-exact on macOS, `1e-5` abs tolerance off-canonical. Golden replay passes under `make test`. |
| 7 | D-09 GitHub Actions CI workflow `.github/workflows/test.yml` is well-formed with 3-OS matrix running `make test` | VERIFIED | `.github/workflows/test.yml` exists. Contains 3-OS matrix: `[ubuntu-latest, macos-latest, windows-latest]`. Triggered on `[push, pull_request]`. Unix legs run `make test`. Windows leg uses direct g++ with flags matching Makefile `TEST_CXXFLAGS` including `-ffp-contract=off`. `grep -c 'RACK_DIR\|xvfb'` returns 0. `actions/checkout@v4` is the only action (version-pinned). |
| 8 | RackCompat.hpp re-implements the 6 Rack primitives bit-identically (Xoroshiro128Plus, SchmittTrigger, Timer, PulseGenerator, OnePole, exp2_taylor5) plus clamp shim, in `namespace forge`, zero rack/ includes | VERIFIED | `src/dsp/RackCompat.hpp` exists, contains `namespace forge`, all 6 primitives + clamp. `OnePole::process` contains `(out == y) ? in : y` snap (Pitfall 5 preserved). No `rack/` includes. RackCompat unit tests pass in `make test`. |
| 9 | D-05 bleed-LFO coupling lifted: `computeMorphedWave` no longer reads `ouLayers[0].state` — uses explicit `bleedLfo` parameter | VERIFIED | `grep -c 'ouLayers' src/dsp/Waveshape.hpp` = 0. `morphedWave(float phase, float morph, float character, float bleedLfo)` signature confirmed. DriftEngine returns `bleedLfo = ouLayers[0].state` (RETAINED, not zeroed, when drift < 0.001). |
| 10 | TEST-01, TEST-02 (initial core scaffold satisfied in Phase 22), and TEST-04 requirements are satisfied per REQUIREMENTS.md | VERIFIED | REQUIREMENTS.md marks TEST-01, TEST-02, and TEST-04 as Complete. All three are substantively verified by codebase evidence above. Cross-phase D-04 advisory noted separately. |

**Score:** 10/10 truths verified

---

## Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `tests/doctest.h` | Vendored single-header doctest 2.4.11 with provenance comment | VERIFIED | File exists (321920 bytes). First 3 lines contain provenance comment with version + URL. |
| `tests/main.cpp` | Sole TU defining `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` | VERIFIED | Contains the define. No other test TU defines it (confirmed via grep across all test/*.cpp). |
| `tests/test_smoke.cpp` | Trivial passing smoke test | VERIFIED | Contains `TEST_CASE` and `CHECK(1 + 1 == 2)`. |
| `Makefile` | Additive `make test` recipe, guarded `plugin.mk` include (CR-01) | VERIFIED | `TEST_BIN`, `TEST_CXXFLAGS`, `.PHONY: test` present. `ifeq ($(filter test,...))` guard for `include $(RACK_DIR)/plugin.mk`. `FLAGS += -Isrc` for plugin to resolve `dsp/*.hpp` headers. |
| `src/dsp/RackCompat.hpp` | `namespace forge` with 6 primitives + clamp, zero rack/ includes | VERIFIED | Exists, substantive, all verified above. |
| `src/dsp/Waveshape.hpp` | Wave math with explicit `bleedLfo` param, zero rack/ / ouLayers | VERIFIED | Exists. `morphedWave(float,float,float,float)` signature. 0 ouLayers refs. |
| `src/dsp/RatioTable.hpp` | `RATIO_TABLE[15]` + `RATIO_LABELS[15]` + `shouldReset`, BEATS_PER_ALIGN slot | VERIFIED | Exists. Length-15 arrays. `shouldReset(int, int)` present. Future `BEATS_PER_ALIGN[15]` slot comment present. |
| `src/dsp/Swing.hpp` | `SWING_FRACTIONS[6]` + `swingPhaseMultiplier` with isClocked + straight gates | VERIFIED | Exists. Length-6 array. Gates return 1.0 in `!isClocked` and `<= 0.5001f` branches. |
| `src/dsp/ClockTracker.hpp` | EMA + FSM; `step(clkV, dt, connected, ratioIdx)`, zero rack/ includes | VERIFIED | Exists. `smoothedPeriod` member. `step(float, float, bool, int)` signature with injected params. No `inputs[` references. |
| `src/dsp/DriftEngine.hpp` | 4 OU layers + jitter + DC OU; `seed`/`setSpreadSeed`; bleedLfo retained at low drift | VERIFIED | Exists. `OULayer` struct. `bleedLfo = ouLayers[0].state` after update. Drift < 0.001 retained (not zeroed). Explicit seed interface. |
| `src/dsp/LfoCore.hpp` | `struct Inputs` POD + `LfoCore::step(Inputs)->float` + `seed`/`setSpreadSeed`; `std::exp2` = 0 | VERIFIED | Exists. `struct Inputs` at line 38. `float step(const Inputs&)` at line 116. 4 references to `exp2_taylor5`; `std::exp2` = 0. |
| `src/AnalogLFO.cpp` | Shell includes `dsp/LfoCore.hpp`, delegates to `core.step()`; inline DSP deleted | VERIFIED | `#include "dsp/LfoCore.hpp"` at line 2. `core.step(in)` at line 306. 0 occurrences of `computeMorphedWave`, 0 of `processClockInput`. |
| `tests/test_dsp_units.cpp` | Unit tests for 4 leaf headers under Rack-free target | VERIFIED | Exists (9266 bytes). Includes all 4 headers. 12+ TEST_CASEs covering RackCompat, Waveshape, RatioTable, Swing. |
| `tests/test_extraction.cpp` | D-08 extraction-correctness gate: core == inline bit-exact at 44.1/48/96 kHz | VERIFIED | Exists (7934 bytes). 6 cases (3 rates × {drift-on, drift=0}). 49152 bit-exact assertions. All pass. |
| `tests/test_dsp_stateful.cpp` | Behavioral tests for ClockTracker + DriftEngine | VERIFIED | Exists (9597 bytes). 8 cases including lock/outlier/re-acquire and DriftEngine determinism/draw-order/bleed-retention. |
| `tests/BlockDriver.hpp` | Headless block-driver: `struct BlockDriver`, `run()`, `clockedScenario()` | VERIFIED | Exists (2845 bytes). All three items present. Injects `sampleTime = 1/sampleRate` per sample. |
| `tests/test_invariants.cpp` | TEST-04 invariants parametrized over 3 sample rates | VERIFIED | Exists (7691 bytes). 6 TEST_CASEs. `SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0}`. All 4 invariants covered. |
| `tests/test_golden.cpp` | D-08 permanent golden replay | VERIFIED | Exists (4233 bytes). Loads `tests/golden/freerun_*.f32`. Canonical-OS bit-exact path uses direct `==`. Off-canonical drift-on uses `1e-5` abs tolerance. |
| `tests/golden/freerun_44100.f32` | Reference block at 44.1k (raw LE float32, 8192 samples) | VERIFIED | Exists, 32768 bytes. Tracked in git. |
| `tests/golden/freerun_48000.f32` | Reference block at 48k | VERIFIED | Exists, 32768 bytes. Tracked in git. |
| `tests/golden/freerun_96000.f32` | Reference block at 96k | VERIFIED | Exists, 32768 bytes. Tracked in git. |
| `tests/golden/freerun_seeds.txt` | Provenance sidecar: seeds, scenario, canonical OS, sample count | VERIFIED | Exists (2343 bytes). Documents seeds, scenario params, canonical OS (macOS / Apple clang / libc++). |
| `.github/workflows/test.yml` | 3-OS CI matrix running `make test` | VERIFIED | Exists (1460 bytes). 3-OS matrix. Unix: `make test`. Windows: direct g++ fallback with `-ffp-contract=off`. No RACK_DIR, no xvfb. |

---

## Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `Makefile test target` | `tests/*.cpp + src/dsp/*.hpp` | `$(CXX) -std=c++17 -Isrc -Itests -ffp-contract=off`, NO `-I$(RACK_DIR)/include` | WIRED | TEST_CXXFLAGS confirmed; `make test RACK_DIR=/nonexistent` green (CR-01 guard). |
| `src/AnalogLFO.cpp process()` | `forge::LfoCore::step` | `core.step(in)` at line 306 | WIRED | Line 2: `#include "dsp/LfoCore.hpp"`. Line 306: `float outputVoltage = core.step(in)`. |
| `src/dsp/LfoCore.hpp` | `ClockTracker, RatioTable, Swing, DriftEngine, Waveshape` | Orchestration via headers | WIRED | LfoCore includes all five sub-headers; calls `clock.step()`, `drift.step()`, `wave.morphedWave()`, etc. |
| `src/dsp/Waveshape.hpp morphedWave` | `src/dsp/DriftEngine.hpp bleedLfo output` | `bleedLfo = ouLayers[0].state` passed as `bleedLfo` arg | WIRED | DriftEngine `step()` returns struct with `bleedLfo`; LfoCore passes it to `morphedWave`. |
| `tests/test_invariants.cpp` | `tests/BlockDriver.hpp` | `BlockDriver.run(nSamples, inputAt)` over `{44100, 48000, 96000}` | WIRED | File includes BlockDriver.hpp; uses `forge::BlockDriver d(sr)` and `d.run(...)` in every test case. |
| `tests/test_golden.cpp` | `tests/golden/*.f32` | `loadF32(path)` + replay with capture seeds | WIRED | Loads `"tests/golden/freerun_44100.f32"` etc. Seeds from freerun_seeds.txt documented in test. |
| `.github/workflows/test.yml` | `make test` | Matrix job, unix make / windows direct g++ | WIRED | `run: make test` on non-Windows; direct `g++ ... tests/*.cpp -o test.exe && ./test.exe` on Windows. |

---

## Data-Flow Trace (Level 4)

The DSP core headers are not web-rendering components; they process audio data and return `float` samples. The extraction-correctness gate (`tests/test_extraction.cpp`) provides the equivalent of a data-flow trace: it drives the core on 8192 samples at 3 rates and asserts bit-exact output, confirming real data flows through the chain. Golden replay does the same against frozen reference data. No hollow props or empty state vars — all data paths exercise the full OU/phase/waveshape chain.

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|--------------------|--------|
| `LfoCore::step` | `outputVoltage` | DriftEngine + Waveshape chain | Yes — full per-sample OU/jitter/phase/waveshape sequence | FLOWING |
| `DriftEngine::step` | `bleedLfo` | `ouLayers[0].state` (post-update, retained at drift < 0.001) | Yes — stochastic OU updated each sample | FLOWING |
| `tests/test_golden.cpp` | `got` (BlockDriver output) | Replay via BlockDriver + LfoCore + seeds from freerun_seeds.txt | Yes — bit-exact against committed reference blocks | FLOWING |

---

## Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| `make test` exits 0 | `make test` | 35 cases / 1,647,397 assertions / SUCCESS | PASS |
| `make test` is Rack-free (no RACK_DIR) | `make test RACK_DIR=/nonexistent` | 35 / 1,647,397 / SUCCESS | PASS |
| `make` (plugin build) unchanged | `make` | `plugin.dylib` built, exit 0 | PASS |
| Golden files are tracked (not gitignored) | `git ls-files tests/golden/` | All 4 files listed | PASS |
| Inline DSP deleted from AnalogLFO.cpp | `grep -c 'float computeMorphedWave' src/AnalogLFO.cpp` | 0 | PASS |
| DSP headers have zero rack/ includes | `grep -L 'rack/' src/dsp/*.hpp` | All 7 headers listed | PASS |
| LfoCore uses forge::exp2_taylor5, not std::exp2 | `grep -c 'std::exp2' src/dsp/LfoCore.hpp` | 0 | PASS |
| Waveshape has no ouLayers coupling | `grep -c 'ouLayers' src/dsp/Waveshape.hpp` | 0 | PASS |
| build-test/ gitignored; tests/golden not | `.gitignore` check | `build-test/` present; no `tests/golden` rule | PASS |

---

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|----------|
| TEST-01 | Plan 22-01 | `make test` target builds and runs standalone doctest binary without disturbing plugin build | SATISFIED | `make test` exits 0 (35 cases). `make` builds plugin.dylib unchanged. Test target has no `-I$(RACK_DIR)/include`. |
| TEST-02 | Plans 22-02 + 22-03 | Pure DSP logic extracted into Rack-independent header-only core (`src/dsp/*.hpp`) consumed by plugin shell | SATISFIED | All 7 `src/dsp/*.hpp` headers exist with zero rack/ includes. Plugin shell delegates `process()` to `core.step()`. Inline DSP deleted. Extraction-correctness gate proven bit-exact at 44.1/48/96 kHz. |
| TEST-04 | Plan 22-04 | Headless block-driver harness runs DSP core over sample blocks, asserts output invariants at 44.1/48/96 kHz | SATISFIED | `tests/BlockDriver.hpp` drives LfoCore. `tests/test_invariants.cpp` covers all 4 invariants (±5V, freq ±1%, continuity ≤0.5V, bit-exact determinism) at all 3 rates. All pass. |

### D-04 Advisory: TEST-02 Ownership Label (Advisory Only — Not a Gap)

REQUIREMENTS.md line 97 and line 130 state that TEST-02's "primary owning phase" is Phase 24. The actual full core extraction (all 7 `src/dsp/*.hpp` headers + shell delegation + inline DSP deletion) landed in Phase 22 per locked decisions D-03/D-04 documented in the Phase 22-03 SUMMARY.

REQUIREMENTS.md already marks TEST-02 as "Complete" in the traceability table and notes "Phase 22 (scaffold) + Phase 24 (full extraction)". The bottom-of-file prose note ("Primary owning phase: Phase 24") is now stale.

**Recommended operator action:** Update REQUIREMENTS.md line 130 to reflect that full extraction landed in Phase 22, and that Phase 24's remaining scope is shell-thinning (CLEAN-01..05 + ~20-line pass-through). This is the human-flagged advisory from D-04 — not a phase defect.

---

## Probe Execution

No `scripts/*/tests/probe-*.sh` files are present. No probes declared in PLAN files. The verification environment's authoritative probe is `make test`, which was run directly above.

| Check | Command | Result | Status |
|-------|---------|--------|--------|
| `make test` (main probe) | `make test` | 35/35 cases, 1,647,397 assertions, SUCCESS | PASS |
| `make test` Rack-free | `make test RACK_DIR=/nonexistent` | 35/35 cases, SUCCESS | PASS |

---

## Anti-Patterns Found

No anti-patterns detected:
- No `TBD`, `FIXME`, or `XXX` markers found in any phase-modified file.
- No `TODO` or `PLACEHOLDER` markers found.
- No empty/stub implementations (all DSP headers carry verbatim-lifted logic; all test files contain substantive assertions).
- The `BEATS_PER_ALIGN[15]` comment in `src/dsp/RatioTable.hpp` is a documented shape-for-P23 marker (D-04), not an unreferenced stub — current `round(1/RATIO_TABLE[idx])` divisor math is live and behaviorally complete.
- The `PLACEHOLDER` word does not appear in any inspected file.
- No empty state variables initialized to `[]`, `{}`, or `null` that feed rendering without population.

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| (none) | — | — | — | — |

---

## Human Verification Required

No items require human verification.

All behavioral assertions are programmatically verified: build correctness confirmed via `make test` and `make`, invariant correctness via the full test suite (1,647,397 assertions passing), data-flow traced via extraction-correctness gate (bit-exact at 44.1/48/96 kHz), golden regression confirmed bit-exact on the canonical OS. The GitHub Actions CI matrix (.github/workflows/test.yml) requires a push to the private repo to observe three-platform green — this is advisory, not a blocker, and the YAML is well-formed and structurally correct.

---

## Gaps Summary

No gaps. All must-haves are satisfied.

---

## Phase 22 Summary

All four phase success criteria are met:

1. `make test` builds and runs 35 doctest cases / 1,647,397 assertions green without touching the plugin build. The Makefile guards `include $(RACK_DIR)/plugin.mk` so the test target is truly Rack-free (CR-01, confirmed via `make test RACK_DIR=/nonexistent`).

2. Seven Rack-independent `src/dsp/*.hpp` headers exist, all with zero `rack/` includes, all in `namespace forge`. The plugin shell includes `dsp/LfoCore.hpp` and delegates `process()` to `core.step()`. The inline DSP is deleted.

3. `tests/BlockDriver.hpp` drives `forge::LfoCore` over sample blocks with `sampleTime = 1/sampleRate` per sample. `tests/test_invariants.cpp` asserts ±5V output bounds, ±1% frequency accuracy, ≤0.5V phase continuity at reset, and bit-exact fixed-seed determinism.

4. All invariant tests pass at 44.1 / 48 / 96 kHz.

The D-08 golden-output regression is bit-exact (32768-byte files, direct `==` comparison on canonical macOS). The D-09 GitHub Actions CI matrix (3-OS, Rack-free) is well-formed. No blockers, no debt markers, no stubs.

---

_Verified: 2026-06-14T11:00:00Z_
_Verifier: Claude (gsd-verifier)_
