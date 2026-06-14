# Phase 22: Test Harness Foundation - Research

**Researched:** 2026-06-14
**Domain:** Standalone C++17 test harness + Rack-independent header-only DSP core extraction for a VCV Rack 2 LFO module
**Confidence:** HIGH — architecture is locked by milestone research; all extraction targets, Rack helper types, and the exact Xoroshiro128+ source were read line-by-line from the actual codebase and the local Rack SDK this session.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Use **doctest** (single drop-in `doctest.h`, zero deps, fastest compile, auto discovery). Catch2 and hand-rolled asserts rejected.
- **D-02:** Extract a driveable `LfoCore` orchestrator plus dependencies into `src/dsp/*.hpp`: phase accumulator, `Waveshape` (sine/tri/saw/square/pulse/morph/bleed), `ClockTracker`, `RatioTable`, `Swing`, **and `DriftEngine`**. Core takes a POD `forge::Inputs`, returns an output voltage.
- **D-03:** **DriftEngine is pulled FORWARD from Phase 24 into Phase 22.** The fixed-seed determinism invariant must exercise REAL stochastic drift, not a placeholder.
- **D-04 (CROSS-PHASE CONSEQUENCE):** Because the full DSP core extraction now lands in Phase 22, **Phase 24 must be re-scoped** to display/thread cleanups (CLEAN-01..05), `process()` shell-thinning to a ~20-line pass-through, and the `ouLayers[0].state` → explicit `bleedLfo` lift. TEST-02 is effectively satisfied here; confirm and update the requirement-ownership table.
- **D-05:** The `computeMorphedWave` bleed dependency on `ouLayers[0].state` must be lifted to an explicit `float bleedLfo` parameter during extraction.
- **D-06:** Re-implement the small Rack DSP types as `forge::` equivalents in `src/dsp/RackCompat.hpp`: `SchmittTrigger`, `Timer`, one-pole filter / pulse generator as needed; `std::clamp` for `rack::math::clamp`.
- **D-07:** **Re-implement Xoroshiro128+** (~15 lines) as `forge::Xoroshiro128Plus`, used by BOTH module and tests. Drift output stays **bit-identical to the shipped plugin**. Keep `std::normal_distribution`. Module may seed from `random_device`; core MUST accept an explicit seed. Templating on RNG + injecting `mt19937_64` was rejected.
- **D-08:** Prove preserved behavior via an **automated golden-output regression**: capture reference output blocks from the CURRENT inline DSP **before** extraction, then assert the extracted core reproduces them within epsilon. Permanent test. No manual in-Rack audition required for this gate.
- **D-09:** **Wire GitHub Actions CI now** — ubuntu/macos/windows matrix running `make test`. Rack-free (no `RACK_DIR`, no display/`xvfb`, no SDK fetch). Runs on the private repo.

### Claude's Discretion
- Invariant tolerances (frequency-accuracy %, phase-continuity click threshold, drift epsilon), golden-data storage format/location under `tests/`, exact `src/dsp/` file split, and test file organization. **All recommendations below in these areas are RECOMMENDATIONS, not locked** — flagged inline.

### Deferred Ideas (OUT OF SCOPE)
- Functional bug fixes (#1 clock lockout, #2 x1.5/÷1.5 alignment, #3 free-run swing desync, #4 patch-load guard) → Phase 23. `RatioTable.hpp` / `ClockTracker.hpp` / `Swing.hpp` should be *shaped* to receive these (e.g. room for `BEATS_PER_ALIGN[]`) but the fixes and their regression tests land in Phase 23.
- x1.5 / ÷1.5 in-Rack listening audition → Phase 23.
- Display/thread cleanups, `process()` shell-thinning, dead-code removal → Phase 24.
- TEST-03 unit tests (waveshape ranges, ratio/alignment table, outlier recovery, swing math) → Phase 23.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| TEST-01 | A `make test` target builds and runs a standalone doctest binary without disturbing the existing plugin build | "make test Recipe" section — purely additive target, header-only compile, no `plugin.mk`, no `libRack`. Verified the existing 14-line Makefile and confirmed the additive pattern does not perturb `make`/`make dist`/`make install`. |
| TEST-02 | Pure DSP logic extracted into a Rack-independent header-only core (`src/dsp/*.hpp`) consumed by the plugin shell | "src/dsp/ File Split" + "RackCompat.hpp Contents" sections. NOTE: REQUIREMENTS.md currently lists TEST-02 owner as Phase 24; per D-04 the full extraction lands HERE — flag for ownership-table update. |
| TEST-04 | A headless block-driver harness runs the core over sample blocks and asserts invariants (frequency accuracy, phase continuity at reset, ±5V bounds, fixed-seed determinism) | "BlockDriver Design" + "Invariant → Assertion Mapping" + "Validation Architecture" sections. |
</phase_requirements>

## Summary

This phase converts a locked design into concrete, plan-ready implementation guidance. The DSP in `src/AnalogLFO.cpp` is **already near-pure**: every waveshaping function is `float→float` math that reads only spread members plus `ouLayers[0].state`; the clock/ratio/swing/drift logic is algorithmically pure but currently reads Rack containers (`inputs[CLK_INPUT]`, `params[RATE_PARAM]`). The extraction is mechanical, not a rewrite. The four Rack helper types the DSP uses (`SchmittTrigger`, `Timer`, `PulseGenerator`, `TExponentialFilter`) plus `Xoroshiro128Plus` and `exp2_taylor5` were read directly from the SDK this session and are all small enough to re-implement bit-identically in `src/dsp/RackCompat.hpp`.

The harness is a `BlockDriver` over the extracted `LfoCore` — no Rack boot, no `rack::Module`, no `APP->` globals, no `libRack` link. Because the core is header-only and includes nothing under `rack/`, `make test` compiles only `tests/*.cpp` + `src/dsp/*.hpp` and runs cross-platform out of the box, making the GitHub Actions matrix trivial (no SDK fetch, no `xvfb`).

The load-bearing safety mechanism is the **golden-output regression (D-08)**: capture reference blocks from the CURRENT inline DSP *before* extraction, then assert the extracted core reproduces them. Because Xoroshiro128+ is re-implemented bit-identically (D-07), the drift stream matches exactly — so the golden match is expected to be **bit-exact, not merely within epsilon**, as long as the float math path is preserved verbatim (this is the single biggest fidelity risk; see Pitfall 2 on `exp2_taylor5` and Pitfall 5 on `TExponentialFilter` snap semantics).

**Primary recommendation:** Re-implement the 6 Rack primitives in `RackCompat.hpp` verbatim from the SDK source quoted below; split the core into 7 headers under `src/dsp/`; capture golden blocks from the inline DSP at 44.1/48/96 kHz BEFORE deleting any inline copy; assert bit-exact replay (epsilon only as a documented fallback if a platform float divergence is proven). Wire CI as a 3-line `make test` matrix.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Waveshaping (sine/tri/saw/sqr/pulse/morph/bleed) | DSP Core (`Waveshape.hpp`) | — | Already pure `float→float`; only touches spread coeffs + bleed param |
| Clock tracking (EMA + outlier + FSM) | DSP Core (`ClockTracker.hpp`) | Rack Shell (reads `inputs[CLK_INPUT]`) | Algorithm pure; voltage injected as `float` |
| Ratio / beat-alignment | DSP Core (`RatioTable.hpp`) | Rack Shell (reads `RATE_PARAM` scaled value) | Pure integer math; ratio index injected |
| Swing phase warp | DSP Core (`Swing.hpp`) | — | Pure once `swingFrac` + `isClocked` are inputs |
| Drift / OU / jitter / DC | DSP Core (`DriftEngine.hpp`) | — | Pure given injected RNG + `sampleTime` |
| Orchestration (mirrors `process()`) | DSP Core (`LfoCore.hpp`) | Rack Shell (POD packing) | Core takes `forge::Inputs`, returns voltage |
| Param/input/output array I/O | Rack Shell (`AnalogLFO.cpp`) | — | `params[]`/`inputs[]`/`outputs[]`/`ProcessArgs` stay in the shell |
| Display double-buffer + atomics | Rack Shell | — | GUI-thread bridge; out of DSP-test scope |
| JSON persistence | Rack Shell | — | `json_t*` (jansson) is a Rack dep; stays in shell |
| Test invariant assertions | Test Harness (`tests/`) | — | Drives `LfoCore` via `BlockDriver`; links nothing |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| doctest | 2.4.11 | Single-header test framework + auto discovery | Locked (D-01). Single `doctest.h`, zero deps, fastest compile, matches project's no-external-deps lean. `[ASSUMED]` version — vendor the header and pin its version comment; verify latest tag at release time. |
| C++ stdlib `<random>` | C++17 | `std::normal_distribution<float>{0,1}` for OU noise | Locked (D-07) — keep the standard distribution; only the engine (Xoroshiro) is re-implemented. |
| Apple clang / g++ | clang 16 (local) / gcc on CI | Compile the test binary | Verified locally: `/usr/bin/clang++` Apple clang 16.0.0, `/usr/bin/g++`, `/usr/bin/make` all present. |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| (none) | — | The core links nothing | The whole point of extract-the-core: zero runtime deps for tests. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| doctest | Catch2 v3 | Rejected (D-01) — adds submodule/vcpkg; Surge XT uses it but the project favors single-header. |
| doctest | Hand-rolled asserts (Squinky Labs style) | Rejected (D-01) — no discovery/reporting. |
| Re-implement Xoroshiro128+ | Template `DriftEngine<Rng>` + inject `mt19937_64` | Rejected (D-07) — tested drift stream would diverge from shipped; determinism test would not guard real behavior. |

**Installation:**
```bash
# No package install. Vendor the single header:
#   curl -L https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h -o tests/doctest.h
# Then commit tests/doctest.h into the repo (no submodule).
```

**Version verification:** doctest is header-only and vendored, so there is no registry install to slopcheck. The header should be downloaded from the official `doctest/doctest` GitHub release tag and committed. Pin the version in a comment at the top of `tests/doctest.h`. `[ASSUMED: doctest 2.4.11 is current]` — confirm the latest release tag at implementation time; any 2.4.x is fine.

## Package Legitimacy Audit

> This phase installs **no** registry packages. The only third-party artifact is a single vendored header (`doctest.h`) committed directly into the repo from the official GitHub release. There is no npm/PyPI/crates install surface, so slopcheck/registry verification does not apply.

| Package | Registry | Disposition |
|---------|----------|-------------|
| doctest.h | none (vendored from github.com/doctest/doctest release tag) | Approved — download from official release, commit, pin version in header comment. Verify SHA against the release asset if paranoid. |

**Packages removed due to slopcheck [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none
**Action for planner:** Add a one-line provenance note where `doctest.h` is committed (source URL + version). No `checkpoint:human-verify` needed — it is a single inspectable header from a well-known project, not an opaque registry dependency.

## Architecture Patterns

### System Architecture Diagram

```
                         ┌─────────────────────────────────────────────┐
   Clock cable ─┐        │  RACK SHELL  (src/AnalogLFO.cpp : Module)    │
   Reset cable ─┤        │  process(args):                              │
   CV inputs   ─┼──────► │   1. read params[] / inputs[] / ProcessArgs  │
   Rate knob   ─┘        │   2. pack into POD forge::Inputs             │
                         │   3. v = core.step(in)   ◄──── delegates     │
                         │   4. write outputs[], display atomics, JSON  │
                         └───────────────┬─────────────────────────────┘
                                         │ forge::Inputs (POD)
                                         ▼
   ┌──────────────────────────────────────────────────────────────────────┐
   │  DSP CORE  src/dsp/*.hpp  — header-only, ZERO rack/ includes          │
   │                                                                        │
   │   forge::Inputs ──► LfoCore::step() ───────────────────► float V      │
   │                        │                                               │
   │       ┌────────────────┼───────────────┬──────────────┬────────────┐  │
   │       ▼                ▼               ▼              ▼            ▼  │
   │  ClockTracker     RatioTable        Swing        DriftEngine  Waveshape│
   │  (clkV,dt)→edge   (idx,beat)→reset  (phase,frac) (dt,drift,   (phase,  │
   │   +smoothedPeriod  +shouldReset      →swingMul    rng)→        morph,  │
   │                                                  {dPhaseMul,  char,    │
   │                                                   dcOffset,   bleedLfo)│
   │                                                   bleedLfo}   →[-1,1]  │
   │       └──────── uses ────► RackCompat.hpp (forge::SchmittTrigger,      │
   │                            Timer, PulseGenerator, OnePole,             │
   │                            Xoroshiro128Plus, exp2_taylor5, clamp)      │
   └──────────────────────────────────────────────────────────────────────┘
                                         ▲
                                         │ same headers, second consumer
   ┌─────────────────────────────────────┴────────────────────────────────┐
   │  TEST HARNESS  tests/*.cpp + tests/doctest.h  — links NOTHING          │
   │   BlockDriver{ LfoCore core; double sampleRate; }                      │
   │     .run(nSamples, inputAt) ──► std::vector<float> out                 │
   │   golden capture/replay │ invariants: bounds/freq/phase/determinism    │
   └────────────────────────────────────────────────────────────────────────┘
```

Trace the primary use case: a clock cable voltage enters the shell → packed into `forge::Inputs.clkVoltage` → `LfoCore::step` feeds `ClockTracker` → produces `smoothedPeriod` → `RatioTable` decides phase reset → `DriftEngine` perturbs `deltaPhase` and produces `bleedLfo` → `Swing` warps `deltaPhase` → phase accumulates → `Waveshape::morphedWave(phase, morph, character, bleedLfo)` → ×5V + crossfade + DC → output voltage. The test harness feeds the same `forge::Inputs` directly with no Rack.

### Recommended Project Structure
```
src/
├── AnalogLFO.cpp        # Rack shell — stays; #includes dsp/LfoCore.hpp this phase (delegation begins)
├── plugin.{hpp,cpp}     # unchanged
└── dsp/                 # NEW — header-only core, zero rack/ includes
    ├── RackCompat.hpp   # forge::{SchmittTrigger,Timer,PulseGenerator,OnePole,Xoroshiro128Plus}, clamp, exp2_taylor5
    ├── Waveshape.hpp    # sine/tri/saw/square/pulse/morph + bleed (bleedLfo param), progressiveCurve, spread struct
    ├── RatioTable.hpp   # RATIO_TABLE[15], RATIO_LABELS, shaped for BEATS_PER_ALIGN[15] (P23), shouldReset()
    ├── ClockTracker.hpp # EMA + outlier rejection + 3-state FSM; step(clkV,dt,connected)→{state,period,edge}
    ├── Swing.hpp        # SWING_FRACTIONS[6], swingPhaseMultiplier(phase,frac,isClocked)
    ├── DriftEngine.hpp  # OULayer, 4 OU layers + DC OU + jitter; step(dt,drift,isClocked,rng,sqrtDt)
    └── LfoCore.hpp      # forge::Inputs POD + LfoCore orchestrator: step(Inputs)→float
tests/
├── doctest.h           # vendored single header (committed)
├── main.cpp            # #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN — ONE TU only
├── BlockDriver.hpp     # drives LfoCore over sample blocks
├── test_waveshape.cpp  # bounds + shape identity (scaffold; full ranges → P23)
├── test_invariants.cpp # bounds/freq/phase-continuity/determinism (TEST-04)
├── test_golden.cpp     # golden capture/replay (D-08)
└── golden/             # reference data, one file per (rate × scenario)
    ├── freerun_44100.f32
    ├── freerun_48000.f32
    └── freerun_96000.f32
.github/workflows/
└── test.yml            # ubuntu/macos/windows matrix → make test
```

### Pattern 1: POD Inputs at the core boundary
**What:** The shell translates Rack arrays into one plain struct; the core never sees Rack indices.
**When to use:** Always — decouples test authoring from `params[]`/`inputs[]` ordering and makes `BlockDriver` trivial.
**Example:**
```cpp
// src/dsp/LfoCore.hpp  — Source: derived from process() in src/AnalogLFO.cpp:659-852
namespace forge {
struct Inputs {
    float rate = 0.7f;        // free-run Hz (params[RATE_PARAM].getValue())
    float ratioScaled = 0.f;  // paramQuantities[RATE_PARAM]->getScaledValue() (0..1) — for ratio idx
    float morph = 0.f;        // post-CV, post-clamp [0,1]
    float character = 0.f;    // post-CV, post-clamp [0,1] (includes characterSpread)
    float drift = 0.f;        // post-CV, post-clamp [0,1]
    float phaseOffset = 0.f;  // [0,1]
    float fmCV = 0.f;         // volts; 0 if unpatched
    float fmAtten = 0.f;
    bool  fmConnected = false;
    float clkVoltage = 0.f;
    bool  clkConnected = false;
    float resetVoltage = 0.f;
    bool  resetConnected = false;
    int   swingIndex = 0;     // 0..5
    float sampleTime = 1.f/44100.f;  // INJECTED, never read from a global
};
}
```

### Pattern 2: Inject sampleTime and RNG — never read globals
**What:** `sampleTime` is a field of `Inputs`; the RNG is a member of the core seeded via an explicit `seed(s0,s1)` method, mirroring the module.
**When to use:** Always — this is what makes block-replay deterministic and the determinism invariant meaningful.

### Pattern 3: Header-only, one TU owns doctest's main
**What:** Exactly ONE `.cpp` (`tests/main.cpp`) defines `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`; all other test `.cpp`s `#include "doctest.h"` without that macro.
**Why:** Defining the implementation macro in more than one TU produces duplicate-symbol link errors (the classic doctest setup pitfall).

### Anti-Patterns to Avoid
- **Linking `libRack` into the test binary** — fragile, unsupported, platform-bound, drags in GLFW/GL/`APP->` globals; buys nothing the core harness lacks. (Confirmed: `../Rack-SDK/libRack.dylib` exports engine symbols, but `APP->`/`Window` globals make bare `process()` unsafe without booting the engine.)
- **Copy-pasting DSP into tests** — tested code drifts from shipped code. One header, two consumers.
- **Defining `computeMorphedWave` in both the shell and the core during transition** — ODR/double-definition trap and silent divergence. Delete the inline copy the moment the shell delegates (see Pitfall 4).
- **Hidden global / `random_device` RNG in testable code** — kills determinism. The module seeds from `random_device`; the core must accept an explicit seed.
- **Defining the doctest impl macro in >1 TU** — duplicate `main`/symbol link error.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| PRNG for drift | A "good enough" custom RNG | Re-implement the EXACT Rack Xoroshiro128+ (15 lines, source quoted below) | Bit-identity to shipped drift is the whole point of D-07; any deviation breaks the golden gate |
| Normal-distributed noise | Box-Muller by hand | `std::normal_distribution<float>{0,1}` | Locked (D-07); standard, already used by the module |
| Test framework | assert macros + a runner | doctest (vendored) | Locked (D-01); discovery + readable failures for one header |
| Schmitt/Timer/Pulse/one-pole | New behavior | Re-implement SDK structs verbatim (sources quoted) | Behavior must match shipped exactly; SDK code is tiny |
| Float exp2 in FM path | A naive `std::exp2` | Replicate `exp2_taylor5` (bit tricks, quoted) | The FM path uses Rack's approximation; `std::exp2` differs in the low bits → golden mismatch when FM is active |

**Key insight:** Every "small helper" the core needs already exists in the SDK as a tiny, readable struct. Re-implementing them verbatim (not "equivalently") is what makes the golden-output gate a *bit-exact* check rather than an epsilon hand-wave.

## Runtime State Inventory

> This phase is an **extraction/refactor**, so the inventory applies. The canonical question: after the core is extracted and the shell delegates, what runtime systems still carry the old behavior?

| Category | Items Found | Action Required |
|----------|-------------|------------------|
| Stored data | **None** — the module persists only `spreadSeed0/1` (hex) and `swingIndex` via `dataToJson`/`dataFromJson` (src/AnalogLFO.cpp:632-657). No collection names, user_ids, or stored strings reference any extracted symbol. Patch compatibility is unaffected: JSON keys are unchanged and stay in the shell. | None — verified by reading dataToJson/dataFromJson. |
| Live service config | **None** — no external services, no n8n/Datadog/Tailscale. Pure local C++ plugin. | None. |
| OS-registered state | **None** — no scheduled tasks, no daemons. | None. |
| Secrets/env vars | **None** — no secrets; `RACK_DIR` is the only env var and is irrelevant to `make test`. | None. |
| Build artifacts | The plugin `build/` and the installed `.dylib`/`.vst` are produced by `make`/`make install` and are NOT affected by extraction (same source files). `build-test/` is NEW and ephemeral. Add `build-test/` to `.gitignore`. | Add `build-test/` (and optionally `tests/golden/*.f32` decision — keep golden files TRACKED) to `.gitignore`. |

**Behavior-preservation note (the real risk for this refactor):** the only "runtime state" that can silently break is **floating-point output identity** between the inline DSP and the extracted core. That is exactly what the golden-output regression (D-08) guards. There is no datastore/service/OS state to migrate — the migration here is purely code, and its correctness gate is the golden test.

## Common Pitfalls

### Pitfall 1: `make test` accidentally depends on Rack
**What goes wrong:** A core header transitively `#include`s something under `rack/` (e.g. via `plugin.hpp`, or by leaving a `rack::dsp::SchmittTrigger` member), so `make test` fails to compile without the SDK, or worse, pulls SDK headers and couples CI to `RACK_DIR`.
**Why it happens:** The inline DSP lives inside `struct AnalogLFO : Module` in a file that opens with `#include "plugin.hpp"` (→ `#include <rack.hpp>`). Mechanical copy can carry a stray `rack::`/`dsp::`/`APP->` reference.
**How to avoid:** Every `src/dsp/*.hpp` must include ONLY `<cmath>`, `<cstdint>`, `<array>`, `<random>`, `<algorithm>` — never `rack.hpp`, `plugin.hpp`, or `rack/*`. Add a guard test: `make test` itself is the guard (it compiles with `-Isrc -Itests` and NO `-I$(RACK_DIR)/include`). Optionally add a grep CI step: fail if `grep -rE 'rack\.hpp|#include <rack|APP->' src/dsp/` returns anything.
**Warning signs:** test build needs `RACK_DIR`; any `rack::` token under `src/dsp/`.

### Pitfall 2: `exp2_taylor5` in the FM path breaks bit-identity
**What goes wrong:** `process()` does `freq *= dsp::exp2_taylor5(fmPitch)` (src/AnalogLFO.cpp:715). If the core uses `std::exp2` instead, FM-active golden blocks diverge in the low mantissa bits → golden test fails (or is silently "fixed" with a loose epsilon, defeating D-08).
**Why it happens:** `exp2_taylor5` is a Rack-specific polynomial approximation using float bit tricks (`exp2Floor` sets the exponent via `int32` reinterpret + `polyHorner`), not the libm `exp2`.
**How to avoid:** Replicate `exp2_taylor5` + `exp2Floor` + `polyHorner` verbatim in `RackCompat.hpp` (the float specialization only — the core never needs SIMD). Source quoted in Code Examples below. ALTERNATIVELY, scope the golden capture to FM-disconnected scenarios (FM defaults off) and exercise FM only in epsilon-tolerant invariant tests — RECOMMENDATION: replicate it, so the FM path is also bit-guarded.
**Warning signs:** golden test passes with FM off but fails with FM patched; a `1e-6` epsilon "needed" only when FM is active.

### Pitfall 3: The `ouLayers[0].state` bleed lift (D-05) changes bleed behavior
**What goes wrong:** `computeMorphedWave` reads `ouLayers[0].state` directly (src/AnalogLFO.cpp:369: `bleedIntensity *= (1.f + ouLayers[0].state * 0.2f)`). Lifting it to a `float bleedLfo` parameter must pass the SAME value at the SAME point in the per-sample sequence, or the bleed modulation shifts by a sample or scales differently.
**Why it happens:** In the inline code, the OU layers are stepped INSIDE the drift block (L733-738) and `ouLayers[0].state` is read LATER inside `computeMorphedWave` at output time (L825). The extracted `LfoCore::step` must capture `ouLayers[0].state` AFTER the drift update for that sample and feed it as `bleedLfo`, exactly as today.
**How to avoid:** In `LfoCore::step`, sequence: (1) `DriftEngine::step` updates OU layers and returns `bleedLfo = ouLayers[0].state` (the post-update value); (2) pass that `bleedLfo` into `Waveshape::morphedWave`. When drift < 0.001 the OU layers are NOT stepped in the inline code (the drift block is skipped) — so `ouLayers[0].state` retains its last value; the core must preserve this (do not zero `bleedLfo` when drift is low — pass the layer's current state). The golden test at drift>0 AND drift=0 catches a mistake here.
**Warning signs:** golden block matches at drift=0 but diverges at drift>0 (or vice versa); bleed "looks the same" but a few samples differ.

### Pitfall 4: ODR / double-definition during the transition
**What goes wrong:** Both the shell's inline `computeMorphedWave` and the core's version exist simultaneously; the shell calls one, the golden test calls the other, and they drift. Or two header-only definitions without `inline`/`static` cause duplicate-symbol link errors in the plugin.
**Why it happens:** Header-only free functions defined in `.hpp` without `inline` violate ODR when included in multiple TUs.
**How to avoid:** All free functions in `src/dsp/*.hpp` must be `inline` (or members of a struct, which are implicitly inline). During transition, capture golden data from the INLINE DSP first (D-08 ordering), then have the shell `#include "dsp/LfoCore.hpp"` and DELETE the inline copy in the same change so there is exactly one definition. Never ship a state where both exist and are both reachable.
**Warning signs:** duplicate-symbol link error in the plugin build; golden passes but the running plugin behaves differently (two live copies).

### Pitfall 5: `TExponentialFilter` snap-to-input semantics
**What goes wrong:** The freq slew and drift slew use `dsp::TExponentialFilter` whose `process` snaps `out` to `in` when `out == y` (granularity guard): `out = simd::ifelse(out == y, in, y)` (SDK filter.hpp). A naive one-pole `out += (in-out)*lambda*dt` omits the snap and diverges in the low bits at steady state.
**Why it happens:** Easy to write the textbook one-pole and miss the equality-snap branch.
**How to avoid:** Replicate the exact `process`: compute `y = out + (in-out)*lambda*dt; out = (out == y) ? in : y; return out;`. Source quoted below. NOTE: the module also lazily reconfigures `lambda` per-sample for the drift slew (L699-703) — the core's `OnePole`/slew must expose `setLambda` and the shell-equivalent re-config must be reproduced inside `LfoCore::step`.
**Warning signs:** golden diverges only after many samples at a constant frequency (steady-state snap missing).

### Pitfall 6: Cross-platform float determinism in CI
**What goes wrong:** The same source produces different low bits on ubuntu vs macos vs windows (x87 vs SSE, `-ffast-math`, FMA contraction, different libm `sin`/`exp`/`log`), so a bit-exact golden test that passes locally fails on a CI runner.
**Why it happens:** Bit-exact IEEE-754 across compilers/platforms is NOT guaranteed for transcendentals (`std::sin`, `std::log`, `std::sqrt` are mostly correctly-rounded but `std::exp`/`std::sin` are not bit-portable across libms) or under fast-math/FMA.
**How to avoid (layered, RECOMMENDED):**
  1. Compile the test target WITHOUT `-ffast-math` and WITH `-ffp-contract=off` to disable FMA contraction (a common silent divergence source). On x86 ensure SSE math (default on all 64-bit targets; no x87 issue on modern toolchains).
  2. Run the **bit-exact golden test only on a single canonical runner** (e.g. `ubuntu-latest`) and run the cross-platform matrix with an **epsilon-tolerant** variant of the same assertions (the invariant tests already tolerate epsilon). This is the pragmatic split most cross-platform DSP suites use.
  3. Determinism INVARIANT (same-seed → same output WITHIN one process/platform) is bit-exact and portable — assert that everywhere. The cross-PLATFORM bit-identity of the golden reference is the part that needs the single-runner treatment.
**Verdict / flag for planner:** Treat **same-platform same-seed determinism as bit-exact (portable)**; treat the **golden reference as bit-exact on one canonical OS + epsilon-tolerant elsewhere**. Pick the canonical OS to match where the golden `.f32` files were captured. This is a RECOMMENDATION (Claude's-Discretion area); the alternative (epsilon everywhere) is simpler but weaker. Epsilon recommendation if used: `1e-5` absolute on a ±5 V signal (≈ 0.2 ppm of full scale).
**Warning signs:** golden green on macOS dev box, red on Windows CI with tiny diffs.

## Code Examples

### Re-implemented Xoroshiro128+ (bit-identical to shipped)
```cpp
// src/dsp/RackCompat.hpp
// Source: VERBATIM from ../Rack-SDK/include/random.hpp:26-70 (read this session).
// Bit-identical to rack::random::Xoroshiro128Plus — required by D-07.
namespace forge {
struct Xoroshiro128Plus {
    using result_type = uint64_t;
    uint64_t state[2] = {};
    Xoroshiro128Plus() {}
    explicit Xoroshiro128Plus(uint64_t s0, uint64_t s1 = 0) { seed(s0, s1); }
    void seed(uint64_t s0, uint64_t s1 = 0) {
        state[0] = s0; state[1] = s1;
        operator()();                 // shift a bad seed, exactly as Rack does
    }
    static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
    uint64_t operator()() {
        uint64_t s0 = state[0], s1 = state[1];
        uint64_t result = s0 + s1;
        s1 ^= s0;
        state[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14);
        state[1] = rotl(s1, 36);
        return result;
    }
    static constexpr uint64_t min() { return 0; }
    static constexpr uint64_t max() { return UINT64_MAX; }  // required for std::normal_distribution
};
}
```

### Re-implemented Rack DSP primitives (verbatim behavior)
```cpp
// src/dsp/RackCompat.hpp  — Source: ../Rack-SDK/include/dsp/digital.hpp & filter.hpp (read this session)
namespace forge {

// SchmittTrigger — float specialization (digital.hpp:82-161). UNINITIALIZED handling matters.
struct SchmittTrigger {
    enum State : uint8_t { LOW, HIGH, UNINITIALIZED };
    State s = UNINITIALIZED;
    void reset() { s = UNINITIALIZED; }
    bool process(float in, float lowThreshold = 0.f, float highThreshold = 1.f) {
        if (s == LOW && in >= highThreshold)        { s = HIGH; return true; }
        else if (s == HIGH && in <= lowThreshold)   { s = LOW; }
        else if (s == UNINITIALIZED && in >= highThreshold) { s = HIGH; }
        else if (s == UNINITIALIZED && in <= lowThreshold)  { s = LOW; }
        return false;
    }
    bool isHigh() { return s == HIGH; }
};

// Timer (digital.hpp:199-218)
struct Timer {
    float time = 0.f;
    void reset() { time = 0.f; }
    float process(float dt) { time += dt; return time; }
    float getTime() { return time; }
};

// PulseGenerator (digital.hpp:167-195) — note: keeps the longer of existing/new on trigger()
struct PulseGenerator {
    float remaining = 0.f;
    void reset() { remaining = 0.f; }
    bool process(float dt) { if (remaining > 0.f) { remaining -= dt; return true; } return false; }
    bool isHigh() { return remaining > 0.f; }
    void trigger(float duration = 1e-3f) { if (duration > remaining) remaining = duration; }
};

// OnePole (== rack::dsp::TExponentialFilter<float>, filter.hpp:59) — snap-to-input is load-bearing
struct OnePole {
    float out = 0.f, lambda = 0.f;
    void reset() { out = 0.f; }
    void setLambda(float l) { lambda = l; }
    void setTau(float tau) { lambda = 1.f / tau; }
    float process(float dt, float in) {
        float y = out + (in - out) * lambda * dt;
        out = (out == y) ? in : y;   // granularity snap — Pitfall 5
        return out;
    }
};

inline float clamp(float x, float lo, float hi) { return std::clamp(x, lo, hi); } // C++17

// exp2_taylor5 — Source: ../Rack-SDK/include/dsp/approx.hpp (float path only).
// Needed bit-identical for the FM path (Pitfall 2).
inline float exp2Floor(float x, float* xf) {
    x += 127.f;
    int32_t xi = (int32_t)x;
    if (xf) *xf = x - (float)xi;
    union { float yi; int32_t yii; };
    yii = xi << 23;
    return yi;
}
inline float exp2_taylor5(float x) {
    float xf;
    float yi = exp2Floor(x, &xf);
    // polyHorner over a[6], evaluated high-to-low (matches approx.hpp:polyHorner)
    const float a[6] = {1.0f, 0.69315169353961f, 0.2401595990753f,
                        0.055817908652f, 0.008991698010f, 0.001879100722f};
    float yf = a[5];
    for (int i = 4; i >= 0; --i) yf = yf * xf + a[i];  // Horner, same order as polyHorner
    return yi * yf;
}
} // namespace forge
```
> [VERIFIED this session] `../Rack-SDK/include/dsp/approx.hpp:38-48` `polyHorner` evaluates highest-coefficient-first: `y = a[N-1]; for n=1..N-1: y = a[N-1-n] + y*x`. The loop in the example above (`yf = a[5]; for i=4..0: yf = yf*xf + a[i]`) is bit-identical to it. No further verification needed — the `exp2_taylor5` re-implementation is confirmed bit-exact against the SDK.

### make test recipe (additive, never touches the plugin target)
```makefile
# Makefile — ADD below the existing `include $(RACK_DIR)/plugin.mk` line.
# Purely additive: `make`, `make dist`, `make install` behavior is unchanged;
# RACK_DIR is irrelevant to this target (no rack/ include path, no libRack link).

TEST_DIR      := tests
TEST_BIN      := build-test/test
TEST_SOURCES  := $(wildcard $(TEST_DIR)/*.cpp)
TEST_HEADERS  := $(wildcard src/dsp/*.hpp) $(wildcard $(TEST_DIR)/*.hpp)
# -Isrc lets tests #include "dsp/LfoCore.hpp"; -Itests finds doctest.h.
# NO -I$(RACK_DIR)/include. No -ffast-math; -ffp-contract=off for cross-platform bit stability.
TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -I$(TEST_DIR) -Wall -Wextra -ffp-contract=off

.PHONY: test
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p build-test
	$(CXX) $(TEST_CXXFLAGS) $(TEST_SOURCES) -o $@
```
> The existing Makefile is only 14 lines (verified) and ends with `include $(RACK_DIR)/plugin.mk`. Appending the block above is non-perturbing because `plugin.mk` does not define a `test` target and the new variables are namespaced with `TEST_`. `$(CXX)` is provided by the environment / plugin.mk's toolchain defaults.

### BlockDriver
```cpp
// tests/BlockDriver.hpp  — Source: ARCHITECTURE.md sketch, fleshed out against LfoCore boundary
#pragma once
#include "dsp/LfoCore.hpp"
#include <vector>
#include <functional>

struct BlockDriver {
    forge::LfoCore core;
    double sampleRate = 44100.0;
    explicit BlockDriver(double sr = 44100.0, uint64_t s0 = 0x1234, uint64_t s1 = 0x5678)
        : sampleRate(sr) { core.seed(s0, s1); }

    std::vector<float> run(int nSamples, const std::function<forge::Inputs(int)>& inputAt) {
        std::vector<float> out; out.reserve(nSamples);
        for (int i = 0; i < nSamples; ++i) {
            forge::Inputs in = inputAt(i);
            in.sampleTime = (float)(1.0 / sampleRate);
            out.push_back(core.step(in));
        }
        return out;
    }

    // Helper: square-wave clock at given BPM (rising edge to +10V) into clkVoltage
    static std::function<forge::Inputs(int)> clockedScenario(double sr, double bpm,
                                                             forge::Inputs base) {
        double period = 60.0 / bpm;                 // seconds per beat
        double half   = period * 0.5;
        return [=](int i) {
            forge::Inputs in = base;
            double t = i / sr;
            double ph = std::fmod(t, period);
            in.clkVoltage = (ph < half) ? 10.f : 0.f;
            in.clkConnected = true;
            return in;
        };
    }
};
```

### doctest bootstrap (single impl TU)
```cpp
// tests/main.cpp  — the ONLY file that defines the implementation. Provides main().
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
// all other test_*.cpp:  #include "doctest.h"   (no macro) + #include test/core headers
```

### Golden capture/replay (D-08)
```cpp
// tests/test_golden.cpp  (replay side; capture side is a one-shot generator — see below)
// Storage format RECOMMENDATION: raw little-endian float32, one file per (scenario × rate),
// under tests/golden/. Plain .f32 keeps the test reader 3 lines and diffs are inspectable as
// binary; a sidecar .txt with seed + scenario params documents provenance. Tracked in git.
static std::vector<float> loadF32(const char* path) {
    std::ifstream f(path, std::ios::binary);
    std::vector<float> v; float x;
    while (f.read(reinterpret_cast<char*>(&x), sizeof x)) v.push_back(x);
    return v;
}
TEST_CASE("golden: freerun matches reference @ 48k") {
    auto ref = loadF32("tests/golden/freerun_48000.f32");
    BlockDriver d(48000.0, /*s0*/0xC0FFEE, /*s1*/0xBADF00D);   // SAME seed used at capture
    forge::Inputs base; base.rate = 2.0f; base.morph = 0.4f; base.character = 0.6f; base.drift = 0.5f;
    auto got = d.run((int)ref.size(), [&](int){ return base; });
    REQUIRE(got.size() == ref.size());
    for (size_t i = 0; i < ref.size(); ++i)
        CHECK(got[i] == doctest::Approx(ref[i]).epsilon(0));  // epsilon(0) ⇒ bit-exact on canonical OS
}
```
> **Capture procedure (D-08 ordering — do this BEFORE deleting any inline DSP):** add a temporary generator (a throwaway `main` or a `tests/_capture` target) that includes the EXTRACTED core, drives the same scenarios/seeds at 44.1/48/96 kHz, and writes the `.f32` files — BUT validate that the extracted core's first capture matches a one-time hand-comparison against the still-present inline DSP. The robust sequence: (1) extract core but keep inline copy; (2) in a scratch test, run BOTH on identical inputs and assert equality (this is the real "did extraction preserve behavior" check); (3) once equal, write the core's output as the golden reference and DELETE the inline copy; (4) `test_golden.cpp` thereafter pins the core against those files forever. Step 2 is the extraction-correctness gate; steps 3-4 are the permanent regression.

### GitHub Actions workflow (D-09)
```yaml
# .github/workflows/test.yml
name: test
on: [push, pull_request]
jobs:
  test:
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
      # No RACK_DIR, no SDK fetch, no xvfb, no display. Core is plain C++17.
      - name: make test (unix)
        if: runner.os != 'Windows'
        run: make test
      - name: make test (windows)
        if: runner.os == 'Windows'
        # Windows runners lack GNU make by default; use the MSYS2/mingw make or
        # invoke the compiler directly. RECOMMENDATION: add a mingw make via
        # `choco install make` OR run the equivalent g++ command directly:
        run: |
          g++ -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off tests/*.cpp -o test.exe
          ./test.exe
```
> **Windows caveat (flag for planner):** `make` is not preinstalled on `windows-latest`, and the default compiler is MSVC (`cl.exe`), not g++. Two clean options: (a) `choco install make` + ensure a g++/clang is on PATH (the runner ships MinGW/LLVM), or (b) skip `make` on Windows and invoke the compiler directly as shown. MSVC is also viable but its float codegen differs — for bit-exact goldens prefer the same compiler family as the canonical capture OS (see Pitfall 6). **Caching:** none needed — there are no dependencies to cache; the build is seconds. Add `actions/cache` only if doctest grows, which it won't.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Test VCV DSP by booting Rack / instantiating `Module` | Extract Rack-independent core, drive headless | Established VCV practice (Squinky Labs, Surge XT) | No GL/window/`APP->`; trivial CI |
| `git filter-branch` for history | `git-filter-repo` | git's own docs deprecate filter-branch | (Relevant to later release phase, not this one) |

**Deprecated/outdated:** none relevant to this phase. doctest, the SDK primitives, and the Xoroshiro algorithm are all stable.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | doctest 2.4.11 is the current release | Standard Stack | Low — any 2.4.x works; pin whatever the latest tag is at impl time |
| A2 | ~~`polyHorner` evaluates highest-coefficient-first~~ **RESOLVED** — verified against SDK approx.hpp:38-48 this session; example matches bit-for-bit | Code Examples (exp2_taylor5) | None — resolved |
| A3 | Cross-platform bit-identity of transcendental-heavy output is NOT guaranteed | Pitfall 6 / Validation | Low — well-established; mitigation (canonical OS + epsilon matrix) is conservative either way |
| A4 | `windows-latest` lacks preinstalled GNU make | CI section | Low — verifiable; fallback (direct g++) provided regardless |
| A5 | Re-implemented primitives reproduce SDK behavior bit-for-bit | Code Examples | Medium — sources were read verbatim this session; the extraction-correctness gate (golden capture step 2) catches any miss |
| A6 | `std::normal_distribution<float>` produces identical sequences across libstdc++/libc++/MSVC for the same Xoroshiro stream | Determinism | **HIGH RISK** — `std::normal_distribution` is NOT specified to be portable across standard library implementations. See Open Question 1. |

## Open Questions

1. **Is `std::normal_distribution<float>` output portable across platforms?**
   - What we know: D-07 locks `std::normal_distribution`. Xoroshiro is now bit-identical, so the RNG *stream* matches across platforms.
   - What's unclear: The C++ standard does NOT require `std::normal_distribution` to produce the same values across libstdc++ (Linux), libc++ (macOS), and MSVC STL (Windows) even given an identical underlying RNG stream — the Box-Muller vs Marsaglia-polar choice and internal caching differ. This means the **drift-on golden reference may be bit-exact only on the libstd implementation it was captured with.**
   - Recommendation: This REINFORCES Pitfall 6's verdict — capture the drift-on golden on ONE canonical OS and run drift-on golden bit-exact ONLY there; on the other two matrix legs, run the drift-on path with an epsilon tolerance (or run only drift-OFF goldens bit-exact cross-platform, where no normal_distribution is involved). The same-platform same-seed determinism invariant remains bit-exact everywhere. Planner should make this split explicit in the test design. (If perfect cross-platform bit-identity of drift were required, the only fix would be re-implementing the normal transform too — out of scope for this phase and not requested.)

2. **Golden capture seed source.** The module seeds from `random_device` (non-deterministic) and derives `spreadSeed` from it. For golden capture the core must be seeded with a FIXED `(s0,s1)` AND a fixed `spreadSeed` so component-spread coefficients are reproducible.
   - Recommendation: `LfoCore` exposes both `seed(s0,s1)` (drift RNG) and an explicit spread-seed setter (or computes spread from the same fixed seed). Document the exact seeds used for each golden file in the sidecar `.txt`. The `initComponentSpread()` logic (src/AnalogLFO.cpp:198-216) uses a SEPARATE `Xoroshiro128Plus spreadRng` seeded from `spreadSeed[0/1]` — the core must reproduce this exactly, so spread coefficients are part of the extracted surface.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| clang++ | `make test` (local) | ✓ | Apple clang 16.0.0 | g++ |
| g++ | `make test` / CI | ✓ (local) | present at /usr/bin/g++ | clang++ |
| make | `make test` | ✓ (local) | present | direct compiler invocation (Windows CI) |
| doctest.h | test framework | ✗ (not yet vendored) | 2.4.x | none — must vendor (single header) |
| Rack SDK / libRack | NOT used by tests | n/a | — | by design, never linked |

**Missing dependencies with no fallback:** doctest.h must be vendored into `tests/` (single header, no install).
**Missing dependencies with fallback:** GNU make on Windows CI → direct g++ invocation (provided in the workflow).

## Validation Architecture

> Nyquist validation is ENABLED (config.json has no `workflow.nyquist_validation` key → treated as enabled). This section is the source for the derived VALIDATION.md.

### Test Framework
| Property | Value |
|----------|-------|
| Framework | doctest 2.4.x (vendored single header `tests/doctest.h`) |
| Config file | none — `tests/main.cpp` defines `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` |
| Quick run command | `make test` (compiles + runs the whole suite; build is seconds) |
| Full suite command | `make test` (same — single binary) |

### Phase Requirements → Test Map
| Req / Success Criterion | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TEST-01 | `make test` builds + runs a standalone doctest binary, plugin build untouched | smoke/build | `make test && make` (both succeed) | ❌ Wave 0 (Makefile block + tests/main.cpp) |
| TEST-02 | Core under `src/dsp/*.hpp` has zero `rack/` includes and is consumed by the shell | static + build | `make test` compiles with NO `-I$(RACK_DIR)`; optional `! grep -rE 'rack\.hpp|<rack|APP->' src/dsp/` | ❌ Wave 0 (src/dsp/*.hpp) |
| TEST-04 / SC: ±5V bounds | every output sample ∈ [−5,+5] V (+ tiny DC epsilon when drift>0) | invariant | `make test` → `test_invariants.cpp::bounds` | ❌ Wave 0 |
| TEST-04 / SC: frequency accuracy | free-run period within tolerance of knob Hz | invariant | `test_invariants.cpp::frequency` | ❌ Wave 0 |
| TEST-04 / SC: phase continuity at reset | `|out[n]-out[n-1]|` under click threshold across the 3 ms crossfade at clock reset | invariant | `test_invariants.cpp::phase_continuity` | ❌ Wave 0 |
| TEST-04 / SC: fixed-seed determinism | two same-seed cores → bit-identical block (drift on); different seeds diverge | invariant | `test_invariants.cpp::determinism` | ❌ Wave 0 |
| TEST-04 / SC: 44.1/48/96 kHz | invariants hold at all three rates | invariant (parametrized) | `test_invariants.cpp` loops over {44100,48000,96000} | ❌ Wave 0 |
| D-08: extraction preserved behavior | extracted core reproduces inline-DSP golden blocks | golden regression | `test_golden.cpp` (bit-exact canonical OS; epsilon elsewhere for drift-on) | ❌ Wave 0 (+ golden/*.f32) |
| Waveshape sanity (scaffold) | morph∈[0,1]×character∈[0,1] sweep stays in [−1,1] pre-scale; shape identity at segment boundaries | unit | `test_waveshape.cpp` | ❌ Wave 0 (full ranges → P23) |

### Sampling Rate
- **Per task commit:** `make test` (whole suite, seconds).
- **Per wave merge:** `make test` + `make` (ensure the plugin still builds — extraction must not break the shell).
- **Phase gate:** `make test` green on all three CI legs (with the drift-on golden split per Open Question 1) before `/gsd:verify-work`.

### Candidate Tolerances (Claude's-Discretion — RECOMMENDATIONS, NOT locked)
| Invariant | Recommended tolerance | Rationale |
|-----------|----------------------|-----------|
| ±5 V output bounds | `[-5.0 - 0.12, +5.0 + 0.12]` V | DC wander adds up to ~50–100 mV at full drift in free-run (src comment L758); 120 mV gives headroom. Without drift: strict ±5.0 V (allow `1e-4` FP slop). |
| Frequency accuracy (free-run) | within **±1.0 %** of knob Hz over ≥ 2 s | Slew + double-precision phase makes free-run very accurate; 1% is comfortable and catches gross errors. Tighten to 0.5% if stable. |
| Phase continuity at reset (click) | `|Δout|` ≤ **0.5 V** per sample across the 3 ms crossfade | Crossfade is cosine over 3 ms (L834-842); a click would be a multi-volt step. 0.5 V flags a broken crossfade without false positives on legit slope. |
| Determinism epsilon (same platform) | **0** (bit-exact) | Same seed, same platform, same code path ⇒ identical. |
| Golden epsilon (canonical OS) | **0** (bit-exact) | Bit-identical Xoroshiro + verbatim primitives ⇒ exact, drift-OFF everywhere; drift-ON exact on capture OS. |
| Golden epsilon (other OS, drift-on) | **`1e-5` absolute** | normal_distribution non-portability (Open Q1) + transcendental libm differences (Pitfall 6). ≈ 0.2 ppm of ±5 V FS. |

### Wave 0 Gaps
- [ ] `Makefile` — append the `make test` block (no perturbation to plugin targets)
- [ ] `tests/doctest.h` — vendor from the official release tag
- [ ] `tests/main.cpp` — doctest impl TU
- [ ] `tests/BlockDriver.hpp` — the harness
- [ ] `src/dsp/RackCompat.hpp` — Xoroshiro + SchmittTrigger + Timer + PulseGenerator + OnePole + exp2_taylor5 + clamp
- [ ] `src/dsp/{Waveshape,RatioTable,ClockTracker,Swing,DriftEngine,LfoCore}.hpp` — the extracted core
- [ ] `tests/{test_invariants,test_golden,test_waveshape}.cpp`
- [ ] `tests/golden/*.f32` (+ sidecar provenance `.txt`) captured from inline DSP per D-08 ordering
- [ ] `.github/workflows/test.yml` — 3-OS matrix
- [ ] `.gitignore` — add `build-test/`

## Sources

### Primary (HIGH confidence)
- `../Rack-SDK/include/random.hpp` — exact `Xoroshiro128Plus` source (read verbatim this session)
- `../Rack-SDK/include/dsp/digital.hpp` — `SchmittTrigger`/`Timer`/`PulseGenerator` source (read verbatim)
- `../Rack-SDK/include/dsp/filter.hpp` — `TExponentialFilter` source incl. snap-to-input (read verbatim)
- `../Rack-SDK/include/dsp/approx.hpp` — `exp2_taylor5`/`exp2Floor`/`polyHorner` (read; Horner order to re-confirm — A2)
- `src/AnalogLFO.cpp` — all DSP read line-by-line (waveshaping L194-388, clock L407-544, reset L546-565, process L659-852, JSON L632-657, spread L198-216)
- `Makefile` — confirmed 14-line additive surface
- `.planning/research/ARCHITECTURE.md` — LOCKED design (extract-the-core verdict, BlockDriver sketch, assertion table, CI sketch)
- `.planning/research/PITFALLS.md` — Pitfall 6 (#11/#12 refactor regression → extract-pure-function-first discipline) and #4 (JSON guard, Phase 23)
- `.planning/REQUIREMENTS.md` — TEST-01/02/03/04 rows + libRack anti-pattern + TEST-02 ownership note (P24 → flag for P22 per D-04)

### Secondary (MEDIUM confidence)
- doctest project (single-header framework; vendor from official release tag) — version to confirm at impl time
- Community VCV testing consensus (Squinky Labs, Surge XT) — corroborates extract-the-core (cited in ARCHITECTURE.md)

### Tertiary (LOW confidence)
- `std::normal_distribution` cross-platform portability — asserted NON-portable per the C++ standard's lack of a portability guarantee; surfaced as Open Question 1 (not verified empirically this session).

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — doctest is locked; no other deps; compilers verified present
- Architecture / file split: HIGH — locked by ARCHITECTURE.md; concrete split derived from actual code structure
- Rack-primitive re-implementation: HIGH — sources read verbatim; one Horner-order detail flagged (A2) to re-confirm
- Golden/determinism strategy: MEDIUM — bit-exact same-platform is solid; cross-platform drift-on bit-identity is the genuine unknown (Open Q1, Pitfall 6) and is handled by the canonical-OS + epsilon split
- Pitfalls: HIGH — each is anchored to a specific line of the actual DSP

**Research date:** 2026-06-14
**Valid until:** ~2026-07-14 (stable domain; only doctest version and the SDK could shift, both minor)
