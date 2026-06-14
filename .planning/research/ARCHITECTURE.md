# Architecture Research: v1.4 Tempered — Automated Test Harness

**Domain:** Automated testing for a VCV Rack 2 DSP module (unit + headless integration)
**Researched:** 2026-06-14
**Confidence:** HIGH — codebase fully analyzed; local Rack SDK contents inspected directly; community testing patterns corroborated by multiple VCV developers (Squinky Labs, Surge XT, the VCV unit-testing thread)

---

## Bottom line up front (the decisive recommendation)

**Extract the pure DSP into Rack-independent header-only code (`src/dsp/*.hpp`) and drive it from a standalone test binary. Do NOT instantiate `rack::Module` / link `libRack` in the test harness.**

The "headless integration harness" requirement is satisfied by a **block driver over the extracted DSP core**, not by booting Rack. This is exactly what every serious VCV plugin with a test suite does (Squinky Labs: "the interesting stuff is at the composite level or lower, and that stuff is not allowed to call any rack code at all"; Surge XT: Catch2 tests over the Rack-independent Surge engine). It is lower-friction, fully cross-platform, CI-trivial, and avoids the entire Rack runtime (GLFW, window, OpenGL, autosave).

Why not link `libRack`? I inspected the local SDK at `../Rack-SDK`. It ships `libRack.dylib` (12 MB) which **does** export real engine symbols (`rack::engine::Engine::stepBlock`, `rack::window::Window::*`). So linking is *technically possible* — but it drags in the window/GL/threading runtime, is an unsupported use of the SDK, is platform-fragile (you'd fight `APP->` globals and the `Window` singleton), and gives you nothing the extracted-core harness doesn't. The juice is not worth the squeeze for a single feature-frozen LFO.

---

## Current Architecture Snapshot (what we're testing)

`src/AnalogLFO.cpp` — 1,641 lines, three structs. Everything DSP lives in `struct AnalogLFO : Module`. The coupling profile:

| Logic | Rack-coupled? | Coupling surface | Testability today |
|-------|---------------|------------------|-------------------|
| `computeSine/Triangle/Saw/Square/Pulse`, `computeMorphedWave`, `progressiveCurve` | **No** — pure `float→float` math, only reads members (`*Spread`, `ouLayers[0].state`) | Member access only | Near-pure; trivial to extract |
| Ratio table + division/beat-alignment reset (#2) | **Yes** | `paramQuantities[RATE_PARAM]->getScaledValue()`, `RATIO_TABLE`, `clockBeatCount` | Logic is integer math; reads one param |
| Clock tracker EMA + outlier rejection + state machine (#1) | **Yes** | `inputs[CLK_INPUT]`, `dsp::SchmittTrigger`, `dsp::Timer`, atomics | Algorithm is pure; I/O via Rack types |
| Swing phase warp (#3) | **Partial** | `SWING_FRACTIONS`, `phase`, `isClocked` gate | Pure math once `swingFrac`/`isClocked` are inputs |
| Drift/OU/jitter/DC engine | **Partial** | `rack::random::Xoroshiro128Plus`, `args.sampleTime` | Pure given an injected RNG + sampleTime |
| `process()` orchestration | **Yes** | `params[]`, `inputs[]`, `outputs[]`, `ProcessArgs` | The glue; tested via integration driver |
| `WaveformDisplay::step()` frame timing (#11) | **Yes** | hard-coded `/60.f`; fix wants `APP->window->getLastFrameDuration()` | GUI thread; out of DSP test scope |

**Key insight:** the waveshaping functions are *already* effectively pure (they only touch a handful of spread members + `ouLayers[0].state` for bleed modulation). The clock/ratio/swing/drift logic is *algorithmically* pure but reads Rack containers. The decoupling job is small and mechanical, not a rewrite.

---

## Recommended Target Architecture

### Layering

```
┌──────────────────────────────────────────────────────────────┐
│  RACK SHELL  (src/AnalogLFO.cpp — struct AnalogLFO : Module)  │
│  process(): read params/inputs → call core → write outputs    │
│  Owns: ProcessArgs, atomics for display, ParamQuantity, JSON  │
│  THIN. No DSP math lives here after extraction.               │
├──────────────────────────────────────────────────────────────┤
│  DSP CORE  (src/dsp/*.hpp — header-only, ZERO rack includes)  │
│  ┌────────────────┐ ┌────────────────┐ ┌──────────────────┐  │
│  │ Waveshape.hpp  │ │ ClockTracker.hpp│ │ RatioTable.hpp   │  │
│  │ sine/tri/saw/  │ │ EMA+outlier+    │ │ 15 ratios +      │  │
│  │ square/pulse/  │ │ 3-state FSM     │ │ beats-per-align  │  │
│  │ morph/bleed    │ │ (#1)            │ │ table (#2)       │  │
│  └────────────────┘ └────────────────┘ └──────────────────┘  │
│  ┌────────────────┐ ┌────────────────┐ ┌──────────────────┐  │
│  │ Swing.hpp (#3) │ │ DriftEngine.hpp │ │ LfoCore.hpp      │  │
│  │ phase warp     │ │ OU + jitter +DC │ │ orchestrator:    │  │
│  │                │ │ (injected RNG)  │ │ step(inputs)→V   │  │
│  └────────────────┘ └────────────────┘ └──────────────────┘  │
├──────────────────────────────────────────────────────────────┤
│  TEST HARNESS  (tests/ — standalone binary, links NOTHING)    │
│  unit: assert on each core header in isolation                │
│  integration: BlockDriver runs LfoCore over N samples,        │
│                asserts frequency/phase/bounds/determinism      │
└──────────────────────────────────────────────────────────────┘
```

### What to extract, concretely

The core must not include `plugin.hpp` or anything under `rack/`. Three Rack types currently used by the DSP need cheap replacements:

1. **`dsp::SchmittTrigger`** — re-implement as a ~10-line `forge::SchmittTrigger` in the core (it's just hysteresis on a threshold). Used by `clockTrigger`/`resetTrigger`.
2. **`dsp::Timer`** — trivial accumulator (`time += dt; reset()`). Re-implement as `forge::Timer`.
3. **`dsp::TExponentialFilter` / `dsp::PulseGenerator`** — one-pole / countdown; ~10 lines each.
4. **`rack::random::Xoroshiro128Plus` + `std::normal_distribution`** — keep `std::normal_distribution` (standard) and re-implement Xoroshiro128+ in the core (a published ~15-line algorithm), OR template the drift engine on an RNG type and inject `std::mt19937_64` in tests. **Inject the RNG** so tests get determinism with a fixed seed.
5. **`rack::math::clamp`** — replace with `std::clamp` (C++17, already standard).

These shims are small enough to live in `src/dsp/RackCompat.hpp`. Simplest, most robust path: **the core owns its own tiny copies, the module delegates to the core.** Bit-identical behavior is verified once by audition; thereafter the core is the single source of truth and the module is a pass-through.

### Decoupling strategy (decisive)

- **Waveshaping** → move verbatim into `Waveshape.hpp` as free functions / a small struct holding the spread coefficients. The `ouLayers[0].state` bleed-modulation dependency becomes an explicit `float bleedLfo` parameter passed in.
- **Clock tracker** → `ClockTracker` struct: `step(float clkVoltage, float dt) → {state, smoothedPeriod, edgeFired}`. All `inputs[CLK_INPUT]` reads happen in the shell and are passed in as a voltage. This is the home for the **consecutive-outlier counter fix (#1)**.
- **Ratio/alignment** → `RatioTable.hpp`: the existing `RATIO_TABLE[15]` plus the new `BEATS_PER_ALIGN[15] = {16,8,6,4,3,2,3,1,2,1,1,1,1,1,1}` (the #2 fix) and a `shouldReset(ratioIdx, beatCount)` free function. Pure integer math — the easiest, highest-value unit-test target.
- **Swing** → `Swing.hpp`: `swingPhaseMultiplier(phase, swingFrac, isClocked)` and `swingDisplayWarp(t, swingFrac)`. The #3 fix (store *effective* swing) is in the shell, but the warp math is unit-tested here.
- **Drift** → `DriftEngine<Rng>`: holds OU layers, `step(dt, driftAmount, isClocked, rng) → {deltaPhaseMul, dcOffset}`. RNG injected for deterministic tests.
- **`LfoCore`** → the orchestrator mirroring `process()`: takes a plain `Inputs` POD (rate, morph, character, drift, clkVoltage, resetVoltage, fmCV, phaseOffset, swingIndex, sampleTime) and returns output voltage. The Rack `process()` becomes ~20 lines: unpack `params/inputs`, call `core.step(...)`, write `outputs`/atomics.

---

## The headless integration harness

A `BlockDriver` (in `tests/`) owns an `LfoCore` and a fixed sample rate. It synthesizes input blocks and steps the core sample-by-sample, recording output:

```cpp
// tests/BlockDriver.hpp  (sketch)
struct BlockDriver {
    forge::LfoCore core;
    double sampleRate = 44100.0;
    std::vector<float> run(int nSamples, std::function<forge::Inputs(int)> inputAt) {
        std::vector<float> out; out.reserve(nSamples);
        forge::Inputs in;
        for (int i = 0; i < nSamples; ++i) {
            in = inputAt(i);
            in.sampleTime = 1.0f / (float)sampleRate;
            out.push_back(core.step(in));
        }
        return out;
    }
    // helper: emit a square-wave clock at given BPM into in.clkVoltage
};
```

This **is** the integration test: it exercises clock acquisition → lock → ratio alignment → phase reset → crossfade → output, over real sample blocks, with zero Rack. Determinism comes from the injected fixed-seed RNG. Frame-rate / `APP->window` concerns (#11) never enter, because display animation is GUI-thread and out of DSP-test scope.

### Can you instead instantiate `rack::Module` and call `process()`?

Technically yes against this SDK (`libRack.dylib` exports the engine), but **not recommended**:
- You must construct/teardown the `Engine`, `Window`, and `APP` globals; `WaveformDisplay::step()` and `RateParamQuantity` reach for `APP->`/`paramQuantities`, so a bare `process()` call without a configured `Engine` will dereference uninitialized globals.
- It binds tests to a specific SDK build and platform; CI must ship/locate `libRack.dylib`.
- It is an unsupported usage; the SDK lib is a plugin-link stub + runtime, not a documented embedding API.
- It buys nothing: every invariant we care about (below) is observable at the core boundary.

**Verdict: extract-the-core wins on every axis.** Confidence HIGH.

---

## Build wiring (`make test` alongside `plugin.mk`)

Keep the plugin build **untouched**. Add an independent target that compiles only the test sources + headers — never `plugin.mk`, never `libRack`.

```makefile
# Makefile (additions — existing plugin.mk include stays as-is)

# --- Test target: pure DSP core, no Rack ---
TEST_DIR      := tests
TEST_BIN      := build-test/test
TEST_SOURCES  := $(wildcard $(TEST_DIR)/*.cpp)
# Core is header-only (src/dsp/*.hpp); tests #include it directly.
TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra

.PHONY: test
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SOURCES) $(wildcard src/dsp/*.hpp)
	@mkdir -p build-test
	$(CXX) $(TEST_CXXFLAGS) $(TEST_SOURCES) -o $@
```

- **Builds only the core**, not the Rack SDK — because the core is header-only and includes nothing from `rack/`. This is the "compile only the extracted core" requirement, satisfied for free.
- `make` / `make dist` / `make install` behavior is unchanged; `make test` is purely additive and `RACK_DIR` is irrelevant to it.
- The plugin's `process()` `#include "dsp/LfoCore.hpp"` — same headers, two consumers. One source of truth, no drift between tested and shipped code.

### Framework choice

Use **doctest** (single header, drop into `tests/`, zero deps, fastest compile) or **Catch2 v3** (also widely used in VCV land — Surge XT uses Catch2). Recommendation: **doctest** for this small suite — it is a single `doctest.h`, no submodule/vcpkg, matching the project's "no external deps / single-header" lean. Squinky Labs' approach (hand-rolled `assert` + custom runner) also works and adds nothing to the tree, but doctest gives readable failure output and test discovery for marginal cost. Either keeps the no-runtime-deps constraint.

---

## Meaningful assertions for an LFO over sample blocks

Mapped to specific findings and LFO invariants:

| Test | Asserts | Ties to |
|------|---------|---------|
| **Output bounds** | Every sample ∈ [−5V, +5V] (allow tiny DC-wander epsilon, since `dcOffsetV` adds post-clamp). Without drift, strictly ±5V. The bleed normalization (`result /= (1+bleedIntensity)`) must keep ±1 pre-scale. | Invariant; guards `computeMorphedWave` |
| **Frequency accuracy (free-run)** | Count zero-crossings / measure period over N seconds at a known Rate; assert within tolerance of the knob Hz. | Invariant |
| **Phase continuity at reset** | At each clock-driven phase reset, |output[n] − output[n−1]| stays under a threshold across the 3 ms cosine crossfade (no step discontinuity / click). | RATE-05 crossfade |
| **Ratio beat-alignment (#2)** | Drive a steady clock; for each of the 15 ratios assert the LFO completes exactly the expected whole cycles between resets: **/1.5 resets every 3 beats, x1.5 every 2 beats**, integer divisions every `divisor` beats, multipliers every-beat-or-benign. No mid-cycle truncation (phase ≈ 0 at reset). | **Fix #2 regression test** |
| **Consecutive-outlier recovery (#1)** | Lock at 120 BPM; switch clock to 4× rate; assert that after 2–3 edges the tracker re-acquires the new tempo (does NOT stay LOCKED at stale period forever). Also test the slow-down trap with a fast base clock. | **Fix #1 regression test** |
| **Swing math (#3)** | `swingPhaseMultiplier` returns 1.0 (straight) for swingFrac=0.5; for 0.66 even half stretched / odd half compressed by the expected ratio; **isClocked=false ⇒ multiplier=1.0** (free-run never warps). Unit test that display-warp/dot-remap uses the *same* effective value. | **Fix #3 regression test** |
| **Morph/character output ranges** | Sweep morph∈[0,1]×character∈[0,1] over a full phase cycle; assert outputs stay in [−1,1] pre-scale at every grid point (catches a character coefficient pushing a shape out of range). Spot-check shape identity at morph segment boundaries (0.0=sine, 0.25=tri, 0.5=saw, 0.75=square, 1.0=narrow pulse). | Waveshape invariants |
| **Determinism (fixed seed)** | Two `LfoCore` instances seeded identically produce bit-identical output over a block (drift on). Different seeds diverge. | Drift engine; reproducible CI |
| **Ratio table sanity** | `RATIO_TABLE` and `BEATS_PER_ALIGN` are length-15 and the p/q lowest-terms `q` matches the alignment table entry. | Static guard for #2 |
| **Patch-load guard (#4)** | (Shell-level) feed malformed/non-string JSON to `dataFromJson`; assert no crash and seed unchanged. Testable cleanly only if the parse predicate is factored into a free function over a `const char*` — otherwise validate by inspection. | Fix #4 |

Note on #4: `dataFromJson` uses `json_t*` (jansson, a Rack dep). To unit-test the real path you'd need jansson linked, reintroducing a dependency. **Recommendation:** test the *parsing predicate* (is-string check + try/catch fallback) as a free function over a `const char*`, and cover the real `json_t*` path by manual/inspection — keep the harness Rack-free.

---

## Patterns to follow

- **Inject sample rate & RNG; never read globals in the core.** `sampleTime` and the RNG are constructor/step parameters. This is what makes block-replay deterministic.
- **POD input struct** (`forge::Inputs`) at the core boundary — the shell translates `params[]`/`inputs[]`/`ProcessArgs` into it. Decouples test authoring from Rack array indices.
- **Header-only core** — one include, two consumers (module + tests), zero build coupling, no link step for tests.
- **Regression-test-per-bug** — each of #1/#2/#3 lands with the failing-then-passing test described above, satisfying the milestone's "bug fixes land as regression tests" goal.

## Anti-patterns to avoid

- **Linking `libRack` into the test binary** — fragile, unsupported, platform-bound, buys nothing. (Confirmed: the SDK lib exists and exports engine symbols, but the `APP->`/`Window` globals make bare `process()` calls unsafe without booting the engine.)
- **Testing through the GUI** (`WaveformDisplay`) — display is GUI-thread, frame-rate-coupled (#11), and not where correctness lives.
- **Copy-pasting DSP into tests** — leads to tested code drifting from shipped code. Single header, shared.
- **Hidden global RNG / `std::random_device` in the hot path of testable code** — kills determinism. The module may seed from `random_device`, but the core must accept a seed.

---

## CI feasibility (GitHub Actions)

Trivial, because the test binary needs no Rack SDK, no display, no GL:

```yaml
# .github/workflows/test.yml (sketch)
on: [push, pull_request]
jobs:
  test:
    strategy: { matrix: { os: [ubuntu-latest, macos-latest, windows-latest] } }
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4
      - run: make test    # compiles src/dsp + tests, runs the suite
```

No `RACK_DIR`, no headless display, no `xvfb`. Cross-platform out of the box (the core is plain C++17). This is the decisive practical advantage of extract-the-core over link-against-Rack: a Rack-linked harness would need the SDK fetched per-OS and a virtual display, which the community thread flags as the hard, partially-unsolved part of CI testing.

---

## Example repos / sources cited

- **Squinky Labs (SquinkyVCV)** — large VCV plugin; ~2/3 of the codebase is tests; DSP lives in "composites" forbidden from calling Rack code; separate test build. The canonical "extract the core" exemplar. (VCV unit-testing thread.)
- **Surge XT for Rack (surge-synthesizer)** — Catch2 test suite with CI over the Rack-independent Surge engine; the Rack module is a thin shell.
- **VCV Community "Unit Testing" thread** — multiple devs converge on: isolate DSP from Rack, build a separate test binary, Catch2 (or custom asserts), "the interesting stuff is not allowed to call any rack code." https://community.vcvrack.com/t/unit-testing/7477
- **VCV Community "Rack SDK with Catch2 via vcpkg and CMake with CI/CD"** — confirms CI runtime testing without a display is the unsolved-friction part when you *do* link Rack; reinforces the headers-only/extract path. https://community.vcvrack.com/t/tip-rack-sdk-with-catch2-via-vcpkg-and-cmake-with-ci-cd/23244
- **Local SDK inspection** — `../Rack-SDK/libRack.dylib` (12 MB, exports `rack::engine::Engine::stepBlock`, `rack::window::Window::*`); no static `.a`. Confirms linking is possible but pulls the full runtime. (Primary evidence, HIGH confidence.)
- **VCV Manual — DSP** https://vcvrack.com/manual/DSP and **rack::engine::Module** https://vcvrack.com/docs-v2/structrack_1_1engine_1_1Module — `process()` semantics, `ProcessArgs::sampleTime`/`sampleRate`.

---

## Confidence assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Extract-core vs link-Rack verdict | HIGH | Corroborated by 2 major plugins + community consensus + direct SDK inspection |
| Decoupling is small/mechanical | HIGH | Waveshaping already pure; clock/ratio/swing algorithmically pure (code read line-by-line) |
| `make test` wiring | HIGH | Header-only core needs no link; additive target proven pattern |
| Headless harness = block driver | HIGH | Standard practice; satisfies "process()-driver over sample blocks" literally |
| Test cases map to findings | HIGH | Each of #1/#2/#3/#4 has a concrete assertion derived from the finding text |
| CI feasibility | HIGH | No Rack/display dep ⇒ plain matrix build; the only friction (display) is avoided by design |
| #4 JSON test depth | MEDIUM | Full `json_t*` path needs jansson; recommend testing the predicate as a free function |

## Gaps / phase-plan flags

- **Bit-identical extraction must be auditioned once** (per the milestone's audition-gate on #2): after moving waveshaping/clock/swing into the core, confirm the running module sounds/behaves identical before deleting the in-module copies. One-time manual gate, then the core is authoritative.
- **Decide RNG strategy** at phase start: re-implement Xoroshiro128+ in the core (keeps drift bit-identical to shipped) vs. template on RNG and inject `mt19937_64` in tests (simpler, but drift output won't bit-match the shipped Xoroshiro stream — still fine for invariant tests, only matters for cross-version determinism). Recommend re-implementing Xoroshiro128+ in the core for fidelity.
- **`computeMorphedWave` bleed dependency on `ouLayers[0].state`** must be lifted to an explicit parameter during extraction — easy to miss; flag in the extraction checklist.
