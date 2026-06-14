---
phase: 22-test-harness-foundation
plan: 04
subsystem: test-harness
tags: [test-harness, block-driver, invariants, golden-regression, ci, cross-platform, rack-free]

# Dependency graph
requires:
  - "22-03: extracted forge::LfoCore (step(Inputs)->float, seed/setSpreadSeed) + frozen golden .f32 baselines + freerun_seeds.txt"
  - "22-01: Rack-free `make test` doctest harness (tests/doctest.h, tests/main.cpp, additive Makefile target)"
provides:
  - "tests/BlockDriver.hpp — headless block-driver over LfoCore: run(nSamples, inputAt) injects sampleTime=1/sr per sample, seeds drift+spread (non-zero defaults), clockedScenario(sr,bpm,base) helper"
  - "tests/test_invariants.cpp — TEST-04 invariants (±5V bounds, free-run freq ±1%, phase continuity at reset ≤0.5V, fixed-seed determinism bit-exact) parametrized over 44.1/48/96 kHz"
  - "tests/test_golden.cpp — D-08 permanent golden replay: bit-exact (direct ==) on canonical macOS, 1e-5 abs tolerance for drift-on off-canonical"
  - ".github/workflows/test.yml — 3-OS CI matrix (ubuntu/macos/windows) running make test (unix) / direct g++ (windows), Rack-free, checkout pinned @v4"
affects: ["Phase 23 (each bug fix lands as a regression against these invariants)", "Phase 24 (refactors must keep the goldens + invariants green)"]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Headless block-driver: BlockDriver owns timing (sampleTime=1/sr injected per sample), seeds deterministically, exposes a functor-based inputAt + clockedScenario"
    - "Cross-platform golden split (Pitfall 6 / Open Q1): drift-on golden bit-exact ONLY on the canonical capture OS (compile-time __APPLE__ gate), 1e-5 abs elsewhere; same-platform determinism bit-exact everywhere"
    - "Bit-exact float comparison via direct == (NOT doctest::Approx.epsilon(0), which still applies a relative margin)"
    - "Windows CI g++ fallback mirrors Makefile TEST_CXXFLAGS (-ffp-contract=off) so flags stay consistent across the make and direct-compile paths"

key-files:
  created:
    - tests/BlockDriver.hpp
    - tests/test_invariants.cpp
    - tests/test_golden.cpp
    - .github/workflows/test.yml
  modified: []

key-decisions:
  - "Phase-continuity-at-reset invariant uses morph=0 (SINE) drift=0: saw/square/pulse contain legitimate near-vertical waveform edges (a saw drops +1→-1 in one tick) that are NOT reset clicks; the continuous sine isolates the 3 ms crossfade so a broken reset is the only way |dOut| exceeds 0.5 V"
  - "Golden bit-exact comparison switched from doctest::Approx(ref).epsilon(0) to direct float == — Approx.epsilon(0) still applies a relative-scaling margin and rejected values that are in fact bit-identical (matches the extraction gate's REQUIRE(a==b))"
  - "BlockDriver default spread seeds = the canonical golden spread seeds (0x9E3779B9/0x7F4A7C15) and default drift seeds are non-zero — Xoroshiro (0,0) is a degenerate all-zero fixed point that hangs std::normal_distribution"
  - "Canonical-OS gate is compile-time (__APPLE__) per freerun_seeds.txt L38 (macOS / Apple clang / libc++); off-canonical legs use 1e-5 absolute tolerance for the drift-on path"

requirements-completed: [TEST-04]

# Metrics
duration: 9min
completed: 2026-06-14
---

# Phase 22 Plan 04: BlockDriver + Invariants + Golden Replay + CI Summary

**The headless `tests/BlockDriver.hpp` drives the extracted `forge::LfoCore` over sample blocks, the full TEST-04 invariant suite (±5V bounds, ±1% free-run frequency, ≤0.5V reset continuity, bit-exact fixed-seed determinism) passes at 44.1/48/96 kHz, the permanent D-08 golden replay pins the core bit-exact to the frozen `.f32` baselines on the canonical OS, and a Rack-free 3-OS GitHub Actions matrix runs `make test` on every push — completing the v1.4 safety net.**

## Performance

- **Duration:** ~9 min
- **Tasks:** 3
- **Files created:** 4

## Accomplishments

- **tests/BlockDriver.hpp** — `struct BlockDriver { forge::LfoCore core; double sampleRate; }` with a constructor that seeds drift + component-spread deterministically (non-zero defaults to dodge the `(0,0)` Xoroshiro fixed point that hangs `std::normal_distribution`), a `run(nSamples, inputAt)` that injects `sampleTime = 1/sampleRate` per sample and collects `core.step(in)`, and a static `clockedScenario(sr, bpm, base)` emitting a 50%-duty square-wave clock into `clkVoltage`. Zero rack/ includes; links only the Rack-free core.
- **tests/test_invariants.cpp (TEST-04)** — all four invariants parametrized over `{44100, 48000, 96000}`: (1) ±5V bounds — `[-5.12,+5.12]` with drift, strict `±5.0±1e-4` without; (2) free-run frequency accuracy — sub-sample-interpolated rising zero-crossings over 4 s, within ±1.0% of the knob Hz; (3) phase continuity at reset — clocked scenario at 120 BPM on the SINE shape (drift off), `|out[n]-out[n-1]| ≤ 0.5 V` everywhere; (4) fixed-seed determinism — same seed ⇒ bit-identical block (drift on), different drift seed ⇒ divergent stream.
- **tests/test_golden.cpp (D-08 permanent regression)** — `loadF32` reader + three TEST_CASEs replaying `freerun_{44100,48000,96000}.f32` through a BlockDriver seeded with the EXACT seeds from `freerun_seeds.txt` (drift `0xC0FFEE`/`0xBADF00D`, spread `0x9E3779B9`/`0x7F4A7C15`). Canonical-OS split (compile-time `__APPLE__`): bit-exact direct `==` on macOS (the capture OS), `1e-5` absolute tolerance for the drift-on path off-canonical (std::normal_distribution non-portability, Open Q1). Green bit-exact on the dev machine (8192 samples × 3 rates = 24576 exact assertions).
- **.github/workflows/test.yml (D-09)** — `name: test`, `on: [push, pull_request]`, `test` job with `strategy: { fail-fast: false, matrix: { os: [ubuntu-latest, macos-latest, windows-latest] } }`, `actions/checkout@v4` (version-pinned, T-22-04-T). Unix legs run `make test`; the Windows leg uses the direct g++ fallback (no preinstalled GNU make, default MSVC) mirroring the Makefile `TEST_CXXFLAGS` incl. `-ffp-contract=off`, via `shell: bash`. No Rack toolchain, no SDK fetch, no display server (`grep -c 'RACK_DIR\|xvfb'` = 0).

## Task Commits

1. **Task 1: BlockDriver.hpp + TEST-04 invariant suite** — `f559dea` (feat)
2. **Task 2: permanent golden replay regression (D-08)** — `2b68337` (feat)
3. **Task 3: GitHub Actions CI matrix (D-09)** — `96335a6` (feat)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test design] Phase-continuity invariant tripped on legitimate waveform edges**
- **Found during:** Task 1
- **Issue:** The plan specified the phase-continuity test on the canonical free-run scenario (morph=0.4). At morph=0.4 the morphed wave sits in the Tri→Saw region; the saw's reset edge drops +1→-1 in a single phase tick, producing a legitimate ~5.95 V adjacent-sample step that is part of the waveshape, NOT a reset click — so `|dOut| ≤ 0.5 V` failed at all three rates.
- **Fix:** Run the phase-continuity invariant on `morph=0` (SINE, continuous) with `drift=0`. The sine has no intrinsic vertical edge, so the only way `|dOut|` exceeds 0.5 V is a broken 3 ms reset crossfade — which is exactly what the invariant is meant to catch. The other three invariants stay on the drift-on free-run scenario.
- **Files modified:** tests/test_invariants.cpp
- **Commit:** f559dea

**2. [Rule 1 - Bug] `doctest::Approx(ref).epsilon(0)` is not a true bit-exact comparator**
- **Found during:** Task 2
- **Issue:** The RESEARCH template used `CHECK(got[i] == doctest::Approx(ref[i]).epsilon(0))` for the canonical-OS bit-exact path. `Approx` with `epsilon(0)` still applies a relative-scaling margin internally and rejected values that print identically and are in fact bit-identical (e.g. `3.76795 == Approx(3.76795)` reported NOT correct) — 24576 spurious failures.
- **Fix:** Use a direct float `==` for the canonical bit-exact path (matching the extraction gate's `REQUIRE(a == b)` in test_extraction.cpp). The off-canonical drift-on path keeps a `std::fabs(diff) <= 1e-5` absolute check. Replay then matches the frozen goldens bit-for-bit.
- **Files modified:** tests/test_golden.cpp
- **Commit:** 2b68337

## Known Stubs

None. BlockDriver drives the live core, the invariants assert real measured behavior, and the golden replay pins the frozen `.f32` baselines bit-exact.

## Verification Evidence

- `make test`: **35 cases / 1,647,397 assertions / SUCCESS** (was 26 cases pre-plan; +9 new cases — 6 invariant cases [4 invariants, two split into on/off + same/diff sub-cases] + 3 golden replay cases, each looping the 3 rates internally).
- Golden replay is **bit-exact** (direct `==`) against `freerun_{44100,48000,96000}.f32` on the canonical macOS dev machine — 8192 samples × 3 rates.
- `tests/BlockDriver.hpp` defines `struct BlockDriver` with `run(...)` injecting `sampleTime` + `clockedScenario` helper; includes `dsp/LfoCore.hpp`, links nothing rack/.
- `grep -E '44100|48000|96000' tests/test_invariants.cpp` matches all three rates.
- `.github/workflows/test.yml`: 3-OS matrix (ubuntu/macos/windows-latest), push+pull_request, unix `make test` / windows direct g++ with `-ffp-contract=off`; `grep -c 'RACK_DIR\|xvfb'` = 0; checkout pinned `@v4`; YAML well-formed (parsed: matrix=ubuntu-latest,macos-latest,windows-latest; 3 steps).
- GitHub Actions workflow cannot be executed locally; validated YAML structure + confirmed the commands match the local `make test` and the Makefile flags.

## User Setup Required

None for the harness/tests (build-only, offline). The CI matrix will run automatically on the next push to the private repo; verify the three legs go green in the Actions tab (the drift-on golden uses the epsilon path on the ubuntu/windows legs per the documented Pitfall 6 split).

## Next Phase Readiness

- TEST-04 satisfied: the behavioral baseline (invariants + golden) is in place and CI-gated across three platforms — Phase 23 bug fixes can each land as a regression test against these invariants, and Phase 24 refactors must keep them green.
- The full v1.4 test-harness safety net (TEST-01/02/04 + D-08 golden + D-09 CI) is complete.

## Self-Check: PASSED

- Files FOUND: tests/BlockDriver.hpp, tests/test_invariants.cpp, tests/test_golden.cpp, .github/workflows/test.yml.
- Commits FOUND: f559dea, 2b68337, 96335a6.

---
*Phase: 22-test-harness-foundation*
*Completed: 2026-06-14*
