---
phase: 22-test-harness-foundation
plan: 03
subsystem: dsp-core
tags: [dsp-extraction, header-only, rack-free, golden-regression, bit-identity, shell-delegation]

# Dependency graph
requires:
  - "22-01: Rack-free `make test` doctest harness (tests/doctest.h, tests/main.cpp, additive Makefile target)"
  - "22-02: leaf DSP headers (RackCompat, Waveshape, RatioTable, Swing) with the D-05 bleedLfo lift"
provides:
  - "src/dsp/ClockTracker.hpp — EMA + outlier rejection + FREE/ACQUIRING/LOCKED FSM + fast-track re-acquire; step(clkV,dt,connected,ratioIdx)->{state,smoothedPeriod,edgeFired,resetWanted}, zero rack/ coupling"
  - "src/dsp/DriftEngine.hpp — 4 OU layers + jitter + DC OU with exact 4xOU->jitter->DC RNG draw order; explicit seed/setSpreadSeed (no OS entropy in core); step() returns deltaPhaseMul + dcOffsetV + bleedLfo (retained, not zeroed, at drift<0.001)"
  - "src/dsp/LfoCore.hpp — forge::Inputs POD + LfoCore::step(Inputs)->float orchestrator mirroring process() L659-852 exactly; seed/setSpreadSeed; Telemetry block for the shell's display atomics"
  - "tests/test_extraction.cpp — D-08 extraction-correctness gate: core == independent inline-process() reference bit-exact at 44.1/48/96 kHz (drift on + drift=0)"
  - "tests/golden/freerun_{44100,48000,96000}.f32 + freerun_seeds.txt — frozen golden baseline from the validated core (raw LE float32, 8192 samples, tracked in git)"
  - "src/AnalogLFO.cpp — thinned shell that #includes dsp/LfoCore.hpp and delegates process() to core.step(); inline DSP DELETED"
affects: [22-04, "Phase 23 (bug fixes shaped into these headers)", "Phase 24 (re-scoped per D-04 — see Cross-Phase Flag)"]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Full driveable core extraction: LfoCore orchestrates ClockTracker+RatioTable+Swing+DriftEngine+Waveshape with the exact inline per-sample order (D-02)"
    - "D-08 capture ordering: extract -> prove core==inline bit-exact (gate) -> freeze goldens from the validated core -> delegate shell + delete inline in the SAME change (Pitfall 4)"
    - "Shell-side telemetry: LfoCore::step populates a Telemetry struct the shell reads to publish display atomics, keeping step()->float and the audio path clean"
    - "Header-only core consumed by TWO toolchains: forge::clamp/clampi written without std::clamp so the same headers compile under the test target's C++17 AND the plugin's C++11"

key-files:
  created:
    - src/dsp/ClockTracker.hpp
    - src/dsp/DriftEngine.hpp
    - src/dsp/LfoCore.hpp
    - tests/test_extraction.cpp
    - tests/test_dsp_stateful.cpp
    - tests/golden/freerun_44100.f32
    - tests/golden/freerun_48000.f32
    - tests/golden/freerun_96000.f32
    - tests/golden/freerun_seeds.txt
  modified:
    - src/AnalogLFO.cpp
    - src/dsp/RackCompat.hpp
    - Makefile

key-decisions:
  - "Extraction-correctness gate implemented as an INDEPENDENT hand-transcribed inline-process() reference (the inline struct can't compile Rack-free); each block cites its AnalogLFO.cpp source lines, asserts bit-exact (epsilon 0) over 8192 samples at all 3 rates, drift-on AND drift=0"
  - "forge::clamp rewritten without std::clamp + new clampi for ints — the plugin's C++11 toolchain lacks std::clamp once it includes the core; finite-input bit-identical, goldens unchanged"
  - "Makefile FLAGS += -Isrc so the plugin resolves the headers' #include \"dsp/*.hpp\"; non-perturbing (test target already passed -Isrc); make/dist/install otherwise unchanged"
  - "Display preview (updateDisplayBuffer) delegates to core.wave.morphedWave with core.drift.ouLayers[0].state as bleedLfo, matching the inline preview which read the live OU-0 state"
  - "Module-side seeding: a shell-local forge::Xoroshiro128Plus derives spreadSeed from random_device and forwards seeds via core.seed()/setSpreadSeed(); spread-seed persistence (dataToJson/dataFromJson) unchanged -> patch compatibility preserved"

requirements-completed: [TEST-02]

# Metrics
duration: 41min
completed: 2026-06-14
---

# Phase 22 Plan 03: Stateful DSP Extraction + Shell Delegation + Golden Baseline Summary

**The full driveable `forge::LfoCore` (ClockTracker + DriftEngine + the orchestrator) is extracted into `src/dsp/*.hpp`, proven bit-for-bit identical to the still-live inline DSP via an independent extraction-correctness gate at 44.1/48/96 kHz, frozen into committed golden `.f32` baselines, and the plugin shell now delegates `process()` to `core.step()` with the inline DSP deleted in the same change — one source of truth, no ODR hazard.**

## CROSS-PHASE FLAG (D-04) — HUMAN CONFIRMATION REQUESTED

**TEST-02 full core extraction effectively LANDED HERE (Phase 22), not Phase 24.**

`REQUIREMENTS.md` currently records (line 97 + line 130):
> `| TEST-02 | Phase 22 (scaffold) + Phase 24 (full extraction) | Complete |`
> "Primary owning phase: Phase 24."

That is now stale. After this plan, the **complete** Rack-independent DSP core (`src/dsp/{RackCompat,Waveshape,RatioTable,Swing,ClockTracker,DriftEngine,LfoCore}.hpp`) exists, has zero `rack/` includes, and the plugin shell delegates `process()` to it. **TEST-02 is fully satisfied in Phase 22.**

**Requested ownership-table update (for human confirmation):**
- Change TEST-02 primary owner from "Phase 24 (full extraction)" to "Phase 22 (full extraction)".
- **Phase 24's remaining scope becomes:** CLEAN-01..05 (display/thread cleanups) + `process()` shell-thinning to a ~20-line pass-through. NOTE: the `ouLayers[0].state` -> explicit `bleedLfo` lift (originally listed for Phase 24) was already done in Plan 02 (D-05), and the core extraction is done here — so Phase 24 is now **shell-thinning + cleanups only, not DSP extraction.**

**Materiality check (per the plan's instruction to flag if scope shifts beyond the ownership note):** Phase 24's planned work does NOT expand — it shrinks. The shell after this plan still does param/input packing, display-atomic publishing, the display preview buffer, JSON, and the I/O arrays (larger than 20 lines, as the plan expected). Phase 24 thins that pass-through and does the CLEAN-* cleanups. No new work is created for Phase 24; the only change is the requirement-ownership label. **No guessing was required — surfacing for the human to update REQUIREMENTS.md line 97/130.**

## Performance

- **Duration:** ~41 min
- **Tasks:** 3
- **Files modified:** 12 (9 created, 3 modified)

## Accomplishments

- **ClockTracker.hpp** — `processClockInput` FSM lifted verbatim (AnalogLFO.cpp:407-544): disconnect revert, timeout, Schmitt edge detection, EMA smoothing, LOCKED-only outlier rejection, fast-track re-acquisition, ACQUIRING->LOCKED after 4 edges, division-aware reset. The two `inputs[CLK_INPUT]` reads and the `paramQuantities[RATE_PARAM]` scaled-value read are injected (`clkV`/`connected`/`ratioIdx`); phase-reset/crossfade/blanking become a `resetWanted` signal for LfoCore; display atomics stay in the shell. Zero `rack/` coupling.
- **DriftEngine.hpp** — `OULayer` + 4-layer/DC-OU theta/sigma/weight init + `initComponentSpread` + the per-sample OU/jitter/DC step lifted verbatim (AnalogLFO.cpp:94-99/198-216/606-629/727-761). **Exact RNG draw order preserved** (4x OU -> 1x jitter -> 1x DC). Explicit `seed`/`setSpreadSeed` (separate spreadRng, Open Q2); no OS-entropy source inside the core. `step()` returns `{deltaPhaseMul, dcOffsetV, bleedLfo}`; **bleedLfo = post-update `ouLayers[0].state`, RETAINED (not zeroed) when drift < 0.001** (Pitfall 3).
- **LfoCore.hpp** — `forge::Inputs` POD (verbatim from RESEARCH L186-204) + `step(Inputs)->float` mirroring process() L659-852 step-for-step: clock+reset, dual-mode freq, freqSlew, drift clamp, **driftSlew per-sample lazy `setLambda`** (Pitfall 5), **`exp2_taylor5` FM** (Pitfall 2), double-precision `deltaPhase`, DriftEngine modulation, swing warp, accumulate+wrap, morph/character, phase offset, `morphedWave(p,m,c,bleedLfo)`, 5x + 3ms cosine crossfade + dcOffsetV. A `Telemetry` block surfaces clock state / ratio / period / drift / swing / display-phase / locked-edge for the shell's display atomics without touching the audio path.
- **test_extraction.cpp (D-08 gate)** — drives LfoCore AND an independent, hand-transcribed inline-`process()` reference (each block citing its AnalogLFO.cpp lines) on identical seeds/inputs; asserts **bit-exact (epsilon 0)** over 8192 samples at 44.1/48/96 kHz for **drift-on AND drift=0** (49152 assertions). This is the load-bearing "extraction changed nothing" proof, run while the inline DSP was still live.
- **Goldens frozen + shell wired + inline deleted (one change)** — captured `freerun_{44100,48000,96000}.f32` (raw LE float32, 8192 samples) + `freerun_seeds.txt` provenance from the validated core; then `#include "dsp/LfoCore.hpp"`, delegated `process()` to `core.step()`, fed display atomics from `core.tel`, and **DELETED** the inline `computeMorphedWave`/`compute*`/`processClockInput`/`processResetInput`/OU step/spread members in the SAME commit (Pitfall 4). `make` builds `plugin.dylib` clean from scratch (no duplicate symbols; exports `_init` + `_modelAnalogLFO`).
- **test_dsp_stateful.cpp** — 8 behavior cases (ClockTracker lock/reject-outlier/re-acquire/injection; DriftEngine determinism/divergence/draw-order/bleed-retention) added under the Rack-free target.

## Task Commits

1. **Task 1: extract ClockTracker + DriftEngine stateful DSP headers** — `3887021` (feat)
2. **Task 2: build LfoCore orchestrator + extraction-correctness gate** — `ae67525` (feat)
3. **Task 3: capture goldens, delegate shell to LfoCore, delete inline DSP** — `fdb63c3` (feat)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking build] Plugin C++11 toolchain lacks `std::clamp`**
- **Found during:** Task 3 (`make` after wiring the shell to include LfoCore.hpp)
- **Issue:** The headers used `std::clamp` (C++17). The Rack-free test target compiles `-std=c++17`, but the plugin compiles `-std=c++11` (Rack SDK default). Once the plugin included the core, `std::clamp` failed to compile.
- **Fix:** Rewrote `forge::clamp` as a branch ternary and added `forge::clampi` for ints; replaced the integer `std::clamp` calls in ClockTracker/LfoCore. Finite-input bit-identical — re-captured goldens are byte-for-byte unchanged, proving behavior was preserved. (Did NOT bump the plugin's `-std` to avoid perturbing float codegen.)
- **Files modified:** src/dsp/RackCompat.hpp, src/dsp/ClockTracker.hpp, src/dsp/LfoCore.hpp
- **Commit:** fdb63c3

**2. [Rule 3 - Blocking build] Plugin include path missing `-Isrc`**
- **Found during:** Task 3 (`make`)
- **Issue:** The headers use `#include "dsp/*.hpp"`; the plugin build had no `-Isrc`, so the nested includes resolved relative to `src/dsp/` (looked for `src/dsp/dsp/RackCompat.hpp`).
- **Fix:** Added `FLAGS += -Isrc` to the Makefile (the test target already passed `-Isrc`). Non-perturbing to `make`/`dist`/`install` otherwise.
- **Files modified:** Makefile
- **Commit:** fdb63c3

**3. [Rule 1 - Test design] Outlier ClockTracker test produced a non-outlier edge**
- **Found during:** Task 1
- **Issue:** My first outlier-rejection test left `clockTimer` mid-cycle after `driveClock`, so the manufactured "early" edge measured a near-normal period and was EMA-smoothed instead of rejected (0.515 vs 0.500).
- **Fix:** Replaced with a `clockInterval` helper that holds LOW for a precise interval then fires exactly one edge — locking via clean 0.5s intervals, then injecting one 0.05s interval (< smoothedPeriod/3) which is correctly rejected, then proving a subsequent in-range edge is accepted.
- **Files modified:** tests/test_dsp_stateful.cpp
- **Commit:** 3887021

**4. [Rule 1 - Determinism landmine] Spread seed (0,0) hangs `normal_distribution`**
- **Found during:** Task 1 (full-suite run hung at 30s+ in the draw-order test)
- **Issue:** `forge::Xoroshiro128Plus` seeded `(0,0)` is a fixed point that emits an all-zero stream; `std::normal_distribution`'s rejection sampling then loops forever. My draw-order test used `setSpreadSeed(0,0)`.
- **Fix:** Use a non-zero spread seed in tests; documented the landmine in the test, the DriftEngine header, and `freerun_seeds.txt`. The module never seeds (0,0) (it seeds from random_device), so production is unaffected — but the seeds sidecar now warns capture/replay must use non-zero spread seeds.
- **Files modified:** tests/test_dsp_stateful.cpp (+ doc notes)
- **Commit:** 3887021

## Plugin-vs-Golden Toolchain Note (not a deviation — design fact)

The goldens are bit-exact against the core **under the test toolchain** (`-std=c++17 -ffp-contract=off`, no fast-math). The plugin compiles with `-funsafe-math-optimizations` (Rack default), so the shipped `.dylib`'s drift output may differ in the low mantissa from the goldens — this is expected and consistent with RESEARCH Pitfall 6 (goldens canonical under the test toolchain). The extraction gate and golden replay both run under `make test`; the plugin's behavior is preserved relative to the prior inline DSP because both compiled under identical plugin flags. The same-platform same-seed determinism invariant remains bit-exact everywhere.

## Known Stubs

None. All three new headers carry real, verbatim-lifted DSP; the gate and goldens are live. The `BEATS_PER_ALIGN[15]` comment in RatioTable.hpp (from Plan 02) remains a documented shape-for-P23 marker, not a stub.

## Verification Evidence

- `make test`: 26 cases / 117991 assertions / SUCCESS (incl. the 6 extraction-gate cases at 3 rates x {drift-on, drift=0}, 49152 bit-exact assertions, and 8 stateful behavior cases).
- `make`: builds `plugin.dylib` clean from a forced full recompile, exit 0, no duplicate-symbol error; `nm` shows `_init` + `_modelAnalogLFO`; `otool` confirms a well-formed Mach-O.
- `grep -L 'rack...'` strict include check: all dsp headers have zero `rack.hpp`/`<rack`/`rack/`/`rack::`/`APP->`.
- `grep -c 'std::exp2' src/dsp/LfoCore.hpp` = 0; FM uses `forge::exp2_taylor5` (4 refs).
- `grep -c 'float computeMorphedWave' src/AnalogLFO.cpp` = 0; `grep -c 'void processClockInput'` = 0 (inline DSP deleted).
- `ls tests/golden/*.f32 | wc -l` = 3; `git check-ignore` confirms the `.f32` files are TRACKED.
- Re-capture after every header change produced byte-identical goldens (clamp ternary + telemetry are audio-neutral).

## User Setup Required

None — build-only, offline, no external service configuration.

## Next Phase Readiness

- Plan 04 can now wire the permanent golden-replay test (`tests/test_golden.cpp`) and the CI matrix against the frozen `.f32` baselines + `freerun_seeds.txt` (bit-exact on the canonical OS, epsilon 1e-5 on other legs for drift-on per Pitfall 6 / Open Q1).
- The full Rack-independent core is in place; Phase 24 is shell-thinning + CLEAN-* only (pending human confirmation of the D-04 ownership note above).

## Self-Check: PASSED

- Files FOUND: src/dsp/ClockTracker.hpp, src/dsp/DriftEngine.hpp, src/dsp/LfoCore.hpp, tests/test_extraction.cpp, tests/test_dsp_stateful.cpp, tests/golden/freerun_{44100,48000,96000}.f32, tests/golden/freerun_seeds.txt.
- Commits FOUND: 3887021, ae67525, fdb63c3.

---
*Phase: 22-test-harness-foundation*
*Completed: 2026-06-14*
