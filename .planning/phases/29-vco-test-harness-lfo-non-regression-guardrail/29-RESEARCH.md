# Phase 29: VCO Test Harness & LFO Non-Regression Guardrail - Research

**Researched:** 2026-07-28
**Domain:** C++11/C++17 test-harness plumbing, byte-exact golden regression, build/CI gate engineering for a shipped VCV Rack 2 plugin
**Confidence:** HIGH (every structural claim below was read directly from this repo's source, Makefile, and CI workflow; the C++11/ODR and C++17-ism claims were empirically re-verified on this machine during research)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**VcoCore Seam (29/30 boundary)**
- **D-01:** **Bare POD seam only.** Phase 29 creates `src/dsp/VcoCore.hpp` as the boundary contract — POD `Inputs` → `step()` → output voltage + `Telemetry` — mirroring `LfoCore.hpp`'s shape, with `step()` returning silence (0 / trivial). **No DSP.** This keeps the 29/30 line crisp: Phase 29 = harness + seam + guardrail; Phase 30 = the naive (aliased-on-purpose) DSP + Rack registration.
- **D-02:** Deliberately **NOT** following Phase 22's D-03 "pull real behavior forward" precedent. There, `DriftEngine` was pulled forward so the determinism test exercised real drift. Here the operator chose the opposite — a hollow core — because the **meaningful behavioral canary for Phase 29 is the shipped-LFO golden replay**, not the (nonexistent) VCO DSP. The VCO `VcoBlockDriver` exists in P29 only to prove the harness *plumbing* (Rack-free, 3 rates, deterministic seam), not DSP behavior.
- **D-03:** The seam **shape** is fixed (POD-in → `step()` → out + telemetry, zero `rack/` includes, siblings-only like `LfoCore.hpp`). The **field set** of the POD `Inputs` is **Claude's discretion** (see below).

**LFO Guardrail Tripwires (all hard-fail; the mission of this phase)**
- **D-04:** **Golden-file checksum lock.** Record the SHA-256 of every LFO `.f32` golden (`tests/golden/freerun_*.f32`); a test/CI step fails if the bytes ever change. Closes the "silently regenerate the goldens to make a regression green" path — goldens can change only via a deliberate, reviewed hash update.
- **D-05:** **Frozen-header hash guard.** A checked-in SHA-256 manifest of the four frozen shared headers (`src/dsp/Waveshape.hpp`, `RackCompat.hpp`, `MathConst.hpp`, `DriftEngine.hpp`) fails CI if any is edited without bumping the manifest. Mechanically enforces "additive only." **Phase 34 additively edits `DriftEngine.hpp`** — that phase performs a deliberate one-line manifest bump. This is a *feature*: the bump forces the edit to be surfaced to the operator (matches the milestone guardrail protocol).
- **D-06:** **Shared-header include / dependency-direction audit.** A CI grep asserts no VCO-only file (`VcoCore.hpp`, future `MorphBlep.hpp`, `VcoBlockDriver.hpp`, `AnalogVCO.cpp`) is `#include`d by any LFO translation unit, and that VCO code only ever *calls* the frozen headers, never edits them. Guards the dependency direction so VCO work can't leak into the LFO build.

**Strict / MinGW VCO Coverage Proof**
- **D-07:** **Dedicated, permanent compile canary.** Because nothing in `src/` includes `VcoCore.hpp` in Phase 29 (`AnalogVCO.cpp` is Phase 30) and both gates only compile headers reached via a `.cpp`, add a dedicated compile-only unit that `#include`s every VCO header (just `VcoCore.hpp` today) and force it through **both**:
  1. the `-std=c++11 -pedantic-errors` syntax gate, and
  2. the CI **MinGW compile + link-vs-`libRack`** leg (mirroring how `plugin.dll` is built) — the *only* thing that catches the in-class-`static constexpr` ODR class that rejected v2.0.0 (`make strict` alone is insufficient, per the milestone guardrail).
- **D-08:** The canary is **permanent and grows**: each later phase that adds a VCO header adds its `#include` to the canary (`MorphBlep.hpp` in P32, etc.). When `AnalogVCO.cpp` lands in P30 it joins the existing `src/*.cpp` glob automatically and is covered identically to how the real TU will be.

### Claude's Discretion
- **POD `Inputs` field set** (D-03): derive from `REQUIREMENTS.md` (PITCH/FM/MORPH/CHARACTER/DRIFT/SYNC/COARSE/FINE) and the `LfoCore` `Inputs` precedent. Operator chose "you decide." *Recommendation:* lean toward declaring the fields needed by the near-term phases (30/31) rather than speculatively modeling P33/P34, while keeping the boundary shape fixed — avoid churning the seam but don't over-speculate. Planner's call.
- **Day-one `VcoBlockDriver` invariant set:** operator chose "you decide." *Recommended default (right-sized for a stub):* (1) runs Rack-free at 44.1/48/96 kHz without `libRack`, (2) `sampleTime` injected as `1/sr` every step, (3) non-degenerate default seeds (never the `(0,0)` Xoroshiro fixed point), (4) two identical runs are bit-identical (seam determinism), (5) output finite / no NaN / no inf. **Defer all semantic DSP asserts** — `< 1-cent` pitch → P31, alias-floor → P32, ±5V-under-morph bounds → P34. Reserved/skipped test skeletons for those are optional, planner's call.
- Exact mechanism/location of the guard steps (doctest cases inside `make test` vs. standalone CI shell steps), hash-manifest file format/location, and canary-TU location (`tests/` vs. a CI-only TU) are implementation details for the researcher/planner.

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope. Everything VCO-behavioral was correctly routed to its owning phase (DSP → P30, pitch/TEST-02 → P31, anti-aliasing/TEST-03 → P32, drift/output → P34, VCO goldens/TEST-05 → P36) rather than pulled into this guardrail phase.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| **TEST-01** | A Rack-free test harness drives `VcoCore` over sample blocks (mirrors `BlockDriver`), runnable via `make test` with no libRack | §"Existing Test Harness (verbatim)" gives the exact `forge::BlockDriver` contract to mirror; §"Makefile / Build Targets" proves `make test` links no libRack and auto-globs `tests/*.cpp` + `tests/*.hpp`, so `VcoBlockDriver.hpp` + a new `test_vco_*.cpp` need **zero** Makefile edits |
| **TEST-04** | The shipped LFO's `.f32` goldens are replayed byte-identical in the same `make test` run as a standing non-regression canary | §"Golden `.f32` Replay (verbatim)" documents the existing 6 replay cases, the byte-exact `==` macOS leg + 1e-6 portable leg, and the exact seeds; §"Tripwire Design (D-04/05/06)" gives the hardening mechanism + the current SHA-256 values |
| **TEST-06** | The strict C++11 gate (`make strict`) and the CI MinGW link leg cover the new `AnalogVCO` translation unit (ODR / C++17-ism protection) | §"The C++17-ism / ODR Hazard" — empirically verified which classes each leg catches; §"Compile Canary Design (D-07)" gives two placement options with a recommendation and the exact glob/wiring consequences of each |

**Boundary reminder (not this phase):** CORE-01/CORE-03/PANEL-03 → P30; TEST-02 → P31; TEST-03/CORE-02 → P32; TEST-05 → P36.
</phase_requirements>

---

## Summary

Phase 29 is a **plumbing and gate-engineering phase, not a DSP phase**. Every load-bearing asset it must mirror already exists, is small, and was read directly during this research: `tests/BlockDriver.hpp` (74 lines), `src/dsp/LfoCore.hpp` (252 lines), `tests/test_golden.cpp` (162 lines), a 78-line `Makefile`, and an 89-line `.github/workflows/test.yml`. The build system is unusually friendly to this phase: `make test` globs `tests/*.cpp` + `tests/*.hpp` + `src/dsp/*.hpp`, and both CI jobs glob `src/*.cpp` — so the *harness* half (TEST-01) and the LFO *golden* half (TEST-04) need **no build-file changes at all**. The only artifact that genuinely requires new wiring is the D-07 compile canary, and its placement is the single most consequential design decision in the phase.

The critical technical finding, verified empirically on this machine during research: **the ODR failure class that rejected v2.0.0 cannot be reproduced locally on macOS at any optimization level.** Apple clang 16 materializes an in-class `static constexpr` array as a per-TU *local* symbol (`nm` shows `l__ZN5forge8VcoTable3TBLE.const`) and links cleanly at `-O0` and `-O3` under `-std=c++11`. Meanwhile `-std=c++11 -pedantic-errors -fsyntax-only` **does** hard-error on `inline constexpr` variables, `if constexpr`, `[[maybe_unused]]`, and `std::clamp`. Conclusion: `make strict` is a genuine, locally-runnable C++17-ism gate, and the CI MinGW **link** leg is the *only* gate for the ODR class — with **no local substitute**. Any plan task phrased as "verify locally that the ODR gate bites" is impossible to satisfy; that proof must be a CI-observed negative control.

The second critical finding is a **gap between D-05's literal wording and its intent**. D-05 names four frozen headers. The LFO's actual behavioral include closure is **eleven** headers (`LfoCore`, `ClockTracker`, `RatioTable`, `Swing`, `Waveshape`, `DriftEngine`, `RackCompat`, `MathConst`, `PatchParse`, `DisplayFill`, `Anim`). Three of the seven unlisted ones — `PatchParse.hpp`, `DisplayFill.hpp`, `Anim.hpp` — are precisely the headers `.planning/research/ARCHITECTURE.md` flags as *"REUSED by the VCO shell (candidates)"*, and none of them is exercised by any golden replay. That is an unguarded surface where a later VCO phase could silently regress the shipped LFO. Options and a recommendation are in §"Risk to the Shipped LFO".

**Primary recommendation:** Copy `BlockDriver.hpp` → `VcoBlockDriver.hpp` (do **not** refactor them into a shared template); add the D-04 golden hash lock and D-05/D-06 guards in a **new** `tests/test_lfo_guardrail.cpp` so `tests/test_golden.cpp` stays byte-unchanged and can itself be hash-pinned; place the D-07 canary at `src/vco_compile_canary.cpp` so all four gates cover it with zero wiring edits; and make the canary **ODR-use** the VCO headers (a runtime-parameterised, external-linkage probe function) — a bare `#include` canary emits no code and would silently fail to exercise the link leg at all.

---

## Architectural Responsibility Map

This phase has no web tiers; the meaningful boundaries are build/link domains.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| `VcoInputs` POD + `VcoCore::step()` seam (silent) | **Pure DSP core** (`src/dsp/`, Rack-free, must compile at BOTH c++11 and c++17) | — | Mirrors `LfoCore.hpp`; consumed by the Rack shell (P30) *and* the Rack-free test target, so it is the one artifact that must satisfy both standards |
| `VcoBlockDriver` block drive, sampleTime injection, seeding | **Test harness** (`tests/`, c++17-only, never shipped) | — | `tests/BlockDriver.hpp` precedent: the driver owns timing and seeding so the core stays free of globals. Test-only ⇒ may use `std::function`/c++17 freely |
| LFO golden replay (behavioral canary) | **Test harness** (`tests/test_golden.cpp`, unchanged) | CI `test` job (3-OS) | Already exists and is green; P29 must not edit it |
| Golden-byte hash lock (D-04) | **Test harness** (new TU inside `make test`) | CI `test` job | Binary files, no line-ending hazard; belongs inside `make test` to satisfy roadmap criterion 1 ("a single `make test` run") |
| Frozen-header hash guard (D-05) | **CI** (`toolchain-gate`, ubuntu) | `make guards` (optional local) | Header files are *text*; a cross-OS in-test hash hits the Windows CRLF hazard. Ubuntu-only checkout is LF-clean |
| Include / dependency-direction audit (D-06) | **CI** (ubuntu shell script) | `make guards` (optional local) | Pure static grep over the source tree; `tests/check_docs.sh` is the in-repo precedent for this shape |
| C++11 syntax gate over VCO headers | **Build** (`make strict`) | CI `toolchain-gate` step 1 | Catches C++17-isms; verified locally |
| ODR/link gate over VCO headers | **CI only** (`toolchain-gate` MinGW compile + link vs `libRack`) | — | Verified: no macOS/clang equivalent exists at any `-O` level |

---

## Existing Test Harness (verbatim, read from source)

### `tests/BlockDriver.hpp` — the template to mirror

- **Path:** `tests/BlockDriver.hpp` (74 lines)
- **Namespace:** `forge` — `struct forge::BlockDriver` (a `struct`, not a `class`; all members public)
- **Member state:**
  ```cpp
  forge::LfoCore core;
  double sampleRate = 44100.0;
  ```
- **Constructor signature (exact):**
  ```cpp
  explicit BlockDriver(double sr = 44100.0,
                       uint64_t s0 = 0x1234ULL, uint64_t s1 = 0x5678ULL,
                       uint64_t sp0 = 0x9E3779B9ULL, uint64_t sp1 = 0x7F4A7C15ULL)
      : sampleRate(sr) {
      core.seed(s0, s1);
      core.setSpreadSeed(sp0, sp1);
  }
  ```
- **Per-block API (exact):**
  ```cpp
  std::vector<float> run(int nSamples, const std::function<forge::Inputs(int)>& inputAt);
  static std::function<forge::Inputs(int)> clockedScenario(double sr, double bpm, forge::Inputs base);
  ```
- **How it advances time / supplies sample rate:** the harness *owns* timing. `run()` computes `const float dt = (float)(1.0 / sampleRate);` once, then **unconditionally overwrites** the caller's field every sample: `in.sampleTime = dt;`. The core never reads a global or a Rack `ProcessArgs`.
- **How it avoids linking libRack:** it includes exactly one project header, `#include "dsp/LfoCore.hpp"`, plus `<vector> <functional> <cmath> <cstdint>`. `LfoCore.hpp` and its entire closure contain **zero** `rack/` includes (verified by grep across `src/dsp/*.hpp`). The `test` target passes no `-I$(RACK_DIR)/include` and links no `-lRack` (see §Makefile), so a stray Rack include is a hard compile failure — the hygiene is enforced by construction, not by an assertion.
- **Seeding discipline (the landmine, quoted from the header banner):**
  > `LfoCore::seed(s0,s1)` seeds ONLY the drift RNG; component-spread coefficients stay zero until `setSpreadSeed(sp0,sp1)` is called. Both default to non-zero values here because `forge::Xoroshiro128Plus` seeded `(0,0)` is a degenerate fixed point that emits an all-zero stream, which makes `std::normal_distribution` loop forever.

**`VcoBlockDriver` mirror contract (recommended):**

```cpp
#pragma once
// tests/VcoBlockDriver.hpp
#include "dsp/VcoCore.hpp"
#include <vector>
#include <functional>
#include <cstdint>

namespace forge {
struct VcoBlockDriver {
    forge::VcoCore core;
    double sampleRate = 44100.0;

    explicit VcoBlockDriver(double sr = 44100.0,
                            uint64_t s0 = 0x1234ULL, uint64_t s1 = 0x5678ULL,
                            uint64_t sp0 = 0x9E3779B9ULL, uint64_t sp1 = 0x7F4A7C15ULL)
        : sampleRate(sr) { core.seed(s0, s1); core.setSpreadSeed(sp0, sp1); }

    std::vector<float> run(int nSamples, const std::function<forge::VcoInputs(int)>& inputAt) {
        std::vector<float> out;
        out.reserve(nSamples);
        const float dt = (float)(1.0 / sampleRate);
        for (int i = 0; i < nSamples; ++i) {
            forge::VcoInputs in = inputAt(i);
            in.sampleTime = dt;
            in.sampleRate = (float)sampleRate;   // VCO addition: Nyquist clamp needs it (PITCH-04, P31)
            out.push_back(core.step(in));
        }
        return out;
    }
};
} // namespace forge
```

> ⚠ **Do NOT template/DRY the two drivers.** See §"Risk to the Shipped LFO" R-2 — this is the single most likely accidental LFO regression in this phase.

### `src/dsp/LfoCore.hpp` — the POD boundary to mirror

- `struct forge::Inputs` — **15 fields**, all with non-static data member initializers (NSDMIs), no constructor. The timing field is documented as `float sampleTime = 1.f / 44100.f;  // INJECTED, never read from a global`.
- `struct forge::LfoCore` — public state members; `float step(const Inputs& in)`; `void seed(uint64_t s0, uint64_t s1 = 0)`; `void setSpreadSeed(uint64_t s0, uint64_t s1 = 0)`; a nested `struct Telemetry { ... }; Telemetry tel;`.
- Includes only `<cmath> <cstdint> <algorithm>` + six `"dsp/*.hpp"` siblings. Zero `rack/`.
- **Name collision hazard:** `forge::Inputs` is already taken by the LFO. The VCO POD **must** be `forge::VcoInputs` (the name `.planning/research/ARCHITECTURE.md` already specifies). `Telemetry` is safe because `LfoCore` nests it.

---

## Golden `.f32` Replay (verbatim, read from source)

### Files

| Path | Size | Content |
|------|------|---------|
| `tests/golden/freerun_44100.f32` | 32768 B | drift-**ON**, macOS-gated, bit-exact |
| `tests/golden/freerun_48000.f32` | 32768 B | " |
| `tests/golden/freerun_96000.f32` | 32768 B | " |
| `tests/golden/freerun_44100_driftoff.f32` | 32768 B | drift-**OFF**, portable, 1e-6 |
| `tests/golden/freerun_48000_driftoff.f32` | 32768 B | " |
| `tests/golden/freerun_96000_driftoff.f32` | 32768 B | " |
| `tests/golden/freerun_seeds.txt` | 4796 B | provenance: scenario, seeds, epsilon rationale, canonical OS |

**Binary format:** raw little-endian `float32`, no header, 8192 samples = 32768 bytes per file. Reader (`tests/test_golden.cpp:43-49`):

```cpp
std::vector<float> loadF32(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::vector<float> v; float x;
    while (f.read(reinterpret_cast<char*>(&x), sizeof x)) v.push_back(x);
    return v;
}
```

Writer is the exact byte-inverse in `tools/capture_golden.cpp:72`.

### Comparison logic (two legs, deliberately different)

1. **Portable drift-OFF leg — runs on ALL OSes.** `replayGoldenDriftOff(sr, path)` constructs `forge::LfoCore` **directly** (NOT via `BlockDriver`, whose ctor would seed the spread path and pull in the non-portable `std::normal_distribution`), calls only `core.seed(DRIFT_S0, DRIFT_S1)`, leaves every `*Spread` at its `0.f` default, and asserts:
   ```cpp
   constexpr double DRIFTOFF_EPSILON = 1e-6;
   CHECK(std::fabs((double)got[i] - (double)ref[i]) <= DRIFTOFF_EPSILON);
   ```
2. **Bit-exact drift-ON leg — `#if defined(__APPLE__)` only.** `replayGolden(sr, path)` uses `forge::BlockDriver d(sr, DRIFT_S0, DRIFT_S1, SPREAD_S0, SPREAD_S1);` and asserts a **true float equality**, with an explicit in-source rationale against `doctest::Approx`:
   ```cpp
   // Use a direct float == (NOT doctest::Approx, whose epsilon(0) still applies a
   // relative-scaling margin and is not a true bit-exact comparator).
   CHECK(got[i] == ref[i]);
   ```

### Seeds (exact, `tests/test_golden.cpp:52-55`)

```cpp
constexpr uint64_t DRIFT_S0  = 0x0000000000C0FFEEULL;
constexpr uint64_t DRIFT_S1  = 0x000000000BADF00DULL;
constexpr uint64_t SPREAD_S0 = 0x000000009E3779B9ULL;
constexpr uint64_t SPREAD_S1 = 0x000000007F4A7C15ULL;
constexpr int      GOLDEN_SAMPLES = 8192;
```

Scenario (`goldenBase()`): `rate=2.0f, morph=0.4f, character=0.6f, drift=0.5f, phaseOffset=0, swingIndex=0`, all connections false. Drift-off variant is identical except `drift=0.0f`.

### Registration & run

- Six `TEST_CASE("golden: ...")` blocks; three portable, three `__APPLE__`-gated.
- `tests/main.cpp` is the **sole** TU defining `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`; every other `tests/*.cpp` includes `"doctest.h"` without the macro. Any second definition = duplicate-symbol link error.
- Relative fixture paths (`"tests/golden/freerun_44100_driftoff.f32"`) mean **the binary must run from the repo root**. `make test` does (`./$(TEST_BIN)`), and the Windows CI leg explicitly notes it too.
- **Verified baseline this session:** `make test` from clean → `50 test cases | 50 passed | 0 failed`, `2,615,027 assertions`, **10.5 s** wall for compile + link + run. `./build-test/test -tc="golden*"` → `6 passed, 44 skipped`. doctest **2.4.11** vendored at `tests/doctest.h`. `make strict` → `strict C++11 gate: PASS`.

### Current SHA-256 values (computed this session — use these to seed the D-04/D-05 manifests)

Goldens:
```
86f110db82efafc140d6ebc4e13a3015c30afcc3ba761d596d3a3855a01f16c7  tests/golden/freerun_44100.f32
cf947ae18b32c4a52c1dfbb48e7a26466ac43bcc245319d999a124ecc2f3b1a5  tests/golden/freerun_44100_driftoff.f32
51e274fe2c2477da0ba71a1acdd97eca2bd9dd7ff421237a03530e1f9e0e77c8  tests/golden/freerun_48000.f32
e3ed634ef50352fd6b81288bb548bb73079521009168deaf1aa5ac4164118be5  tests/golden/freerun_48000_driftoff.f32
a450d0963e5eda8fcba15084978f49e3bc22d9d6001104d81432b4f181229b74  tests/golden/freerun_96000.f32
b935779570067988a23282c60d2e6a33b4ea691f4f31b36a4d36ecdf07be3af2  tests/golden/freerun_96000_driftoff.f32
```

Frozen headers (D-05's named four):
```
e8ae07004e15e136cf29c703d9a22928bc72cdf85b61427b15ab441e08230d76  src/dsp/Waveshape.hpp
405a878d2293365e555adaa9c226eb5e13f3983f347878a66bf4a518bcc603f8  src/dsp/RackCompat.hpp
091eba706906841211a72482208e0cb5a55d2adba196ddfd9250d7b68b24f91b  src/dsp/MathConst.hpp
698146fdb18c4bef496e74fb135da462a9dc72ba79a471b484b55aedaa7ac7c4  src/dsp/DriftEngine.hpp
```

> These are LF-normalized working-tree hashes computed on macOS with `shasum -a 256`. If a plan task regenerates them, it must do so on a LF checkout (see the CRLF hazard in §Pitfalls).

---

## Makefile / Build Targets (verbatim, read from source)

`Makefile` is 78 lines. `RACK_DIR ?= ../Rack-SDK` (line 1) — the **relative-path convention**; `../Rack-SDK` exists on this machine and `make strict` resolved against it successfully. Do not introduce worktrees or absolute SDK paths.

### Targets

| Target | Command | Notes |
|--------|---------|-------|
| *(default)* | from `$(RACK_DIR)/plugin.mk` | Builds `plugin.dylib`/`.so`/`.dll` from `SOURCES += $(wildcard src/*.cpp)` |
| `test` | `$(CXX) $(TEST_CXXFLAGS) $(TEST_SOURCES) -o build-test/test` then `./build-test/test` | Rack-free |
| `capture` | `$(CXX) $(TEST_CXXFLAGS) tools/capture_golden.cpp -o build-test/capture` then run | One-shot golden generator, **not** part of `test` |
| `strict` | see below | `-fsyntax-only`, needs `$(RACK_DIR)` |

### Exact variables

```make
TEST_DIR      := tests
TEST_BIN      := build-test/test
TEST_SOURCES  := $(wildcard $(TEST_DIR)/*.cpp)
TEST_HEADERS  := $(wildcard src/dsp/*.hpp) $(wildcard $(TEST_DIR)/*.hpp)
TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -I$(TEST_DIR) -Wall -Wextra -ffp-contract=off
```

```make
strict:
	$(CXX) -std=c++11 -pedantic-errors -fsyntax-only -Wall -Wextra -Wno-unused-parameter \
		-Isrc -isystem $(RACK_DIR)/include -isystem $(RACK_DIR)/dep/include $(wildcard src/*.cpp)
	@echo "strict C++11 gate: PASS"
```

Observed expansion this session: `... src/AnalogLFO.cpp src/plugin.cpp`.

### The plugin.mk skip guard (why `make test` works with no SDK)

```make
ifeq ($(filter test capture,$(MAKECMDGOALS)),)
include $(RACK_DIR)/plugin.mk
endif
```
A bare `include` hard-fails on CI runners with no `../Rack-SDK`, so `test`/`capture` are filtered out. **Consequence for this phase:** if the planner adds a new Rack-free target (e.g. `make guards`), it **must** be added to that filter list or it will break on CI runners without the SDK. This is a concrete, easy-to-miss edit.

### TU enumeration — what is automatic vs. what needs wiring

| New artifact | Picked up automatically by | Needs manual wiring in |
|---|---|---|
| `tests/VcoBlockDriver.hpp` | `TEST_HEADERS` glob (prerequisite only) | — |
| `tests/test_vco_harness.cpp` | `TEST_SOURCES` glob → `make test`; CI Windows leg's literal `tests/*.cpp` | — |
| `tests/test_lfo_guardrail.cpp` | same | — |
| `src/dsp/VcoCore.hpp` | `TEST_HEADERS` glob (prerequisite only); compiled only when a `.cpp` includes it | — |
| **Canary TU in `src/`** | `SOURCES` (plugin build), `make strict` wildcard, CI strict step `src/*.cpp`, CI MinGW loop `for f in src/*.cpp`, CI MinGW link `build-ci/*.o` | **nothing** |
| **Canary TU in `tests/` or `tools/`** | `make test` (if in `tests/`) — at c++17, which proves nothing about c++11 | `Makefile` strict target **and** CI strict step **and** CI MinGW loop (3 edits) |
| `src/AnalogVCO.cpp` (P30) | all five globs above | — |

### Toolchain flags in the real plugin build (`../Rack-SDK/compile.mk`)

```
FLAGS   += -MMD -MP -g -O3 -funsafe-math-optimizations -fno-omit-frame-pointer
FLAGS   += -Wall -Wextra -Wno-unused-parameter -Wno-vla-extension
CXXFLAGS += -std=c++11
ARCH_X64 -> -march=nehalem ; ARCH_LIN/WIN -> -Wsuggest-override ; ARCH_WIN -> -D_USE_MATH_DEFINES -municode
plugin.mk: FLAGS += -fPIC -I$(RACK_DIR)/include ... ; LDFLAGS += -shared -L$(RACK_DIR) -lRack
```
No `-fvisibility=hidden`, no `-flto` ⇒ **an external-linkage function in a `src/` TU is emitted and linked even if nothing calls it.** That is what makes the canary-in-`src/` design work against the link leg.

**Environment note:** local `make` is **GNU Make 3.81** (Apple's stock). Avoid Make ≥ 4.0 features (`$(file ...)`, `::=`, `.ONESHELL` reliance) in any new target.

---

## CI (`.github/workflows/test.yml`, verbatim)

**Trigger:** `on: [push, pull_request]` — yes, every push. Two jobs.

### Job `test` — 3-OS matrix `[ubuntu-latest, macos-latest, windows-latest]`

- unix: `run: make test`
- windows: **does not use make.** It re-states the flags literally:
  ```bash
  g++ -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off \
      -static -static-libgcc -static-libstdc++ tests/*.cpp -o test.exe
  ./test.exe
  ```
  ⇒ new `tests/*.cpp` are auto-covered, but **`TEST_CXXFLAGS` is duplicated here**; any flag change must be mirrored or the two builds silently diverge.
- ⇒ **the golden legs already run in CI on all three OSes.**

### Job `toolchain-gate` — `ubuntu-latest`, `RACK_SDK_VERSION: 2.6.6`

1. Fetch both SDKs: `Rack-SDK-2.6.6-lin-x64.zip` → `/tmp/lin`, `Rack-SDK-2.6.6-win-x64.zip` → `/tmp/win`.
2. **Strict C++11 pedantic gate** — *does not call `make strict`*; it re-states the command:
   ```bash
   g++ -std=c++11 -pedantic-errors -fsyntax-only -Wall -Wextra -Wno-unused-parameter \
     -Isrc -isystem /tmp/lin/Rack-SDK/include -isystem /tmp/lin/Rack-SDK/dep/include \
     src/*.cpp
   ```
   ⇒ **second duplication.** The Makefile `strict` target and this step can drift.
3. `sudo apt-get install -y --no-install-recommends g++-mingw-w64-x86-64`
4. **win-x64 leg reproduction (compile + full link vs libRack)** — the ODR gate:
   ```bash
   mkdir -p build-ci
   for f in src/*.cpp; do
     x86_64-w64-mingw32-g++ -std=c++11 -O3 -funsafe-math-optimizations -fno-omit-frame-pointer \
       -Wall -Wextra -Wno-unused-parameter -march=nehalem -D_USE_MATH_DEFINES -municode \
       -Isrc -I/tmp/win/Rack-SDK/include -I/tmp/win/Rack-SDK/dep/include \
       -c -o "build-ci/$(basename "$f").o" "$f"
   done
   x86_64-w64-mingw32-g++ -municode -o build-ci/plugin.dll build-ci/*.o \
     -shared -L/tmp/win/Rack-SDK -lRack -static-libstdc++
   echo "win-x64 link gate: PASS"
   ```

**What a new TU needs, per leg:** if it lives in `src/`, nothing. If it lives elsewhere, three separate edits: the Makefile `strict` wildcard, the CI strict step's file list, and the CI MinGW `for f in src/*.cpp` list (the link line already globs `build-ci/*.o`, so it needs no change).

**Missing from the current gates (cheap hardening, optional):** `.planning/RETROSPECTIVE.md:240` recommends `-Werror=c++14-extensions -Werror=c++17-extensions` alongside `-pedantic-errors`. Neither the Makefile nor CI adopted them. Under Apple clang these are real flag names (verified: the C++17-ism errors this session reported `[-Werror,-Wc++17-extensions]` / `[-Werror,-Wc++17-attribute-extensions]`). GCC also documents `-Wc++17-extensions`, but that was **not** verified here — treat as `[ASSUMED]` and validate in CI before relying on it.

---

## Xoroshiro / Seeds

- **Type:** `struct forge::Xoroshiro128Plus`, `src/dsp/RackCompat.hpp:22-42`. Banner: *"Source: VERBATIM from `../Rack-SDK/include/random.hpp:26-70`. Bit-identical to `rack::random::Xoroshiro128Plus`."*
- **State/API:**
  ```cpp
  using result_type = uint64_t;
  uint64_t state[2] = {};
  Xoroshiro128Plus() {}
  explicit Xoroshiro128Plus(uint64_t s0, uint64_t s1 = 0) { seed(s0, s1); }
  void seed(uint64_t s0, uint64_t s1 = 0) { state[0] = s0; state[1] = s1; operator()(); }
  uint64_t operator()();
  static constexpr uint64_t min() { return 0; }
  static constexpr uint64_t max() { return UINT64_MAX; }  // required for std::normal_distribution
  ```
- **The `(0,0)` degenerate fixed point.** With `state = {0,0}`: `result = 0+0 = 0`; `s1 ^= s0 → 0`; `state[0] = rotl(0,55) ^ 0 ^ 0 = 0`; `state[1] = rotl(0,36) = 0`. The state is a fixed point emitting an all-zero stream forever. The seed-shift call inside `seed()` does not rescue it. `std::normal_distribution`'s Box–Muller rejection loop never terminates on an all-zero uniform stream ⇒ **infinite loop / Rack hang on patch load**. This is documented in three places: `RackCompat.hpp` (implicitly, via the Rack-verbatim note), `tests/BlockDriver.hpp:16-17`, and `tests/golden/freerun_seeds.txt:24-26`.
- **Where defaults are set today:**
  | Location | Values |
  |---|---|
  | `tests/BlockDriver.hpp:36-37` (ctor defaults) | drift `0x1234 / 0x5678`; spread `0x9E3779B9 / 0x7F4A7C15` |
  | `tests/test_golden.cpp:52-55` / `tools/capture_golden.cpp:34-35` | drift `0xC0FFEE / 0xBADF00D`; spread `0x9E3779B9 / 0x7F4A7C15` |
  | `tests/test_extraction.cpp:179-180` | `0xC0FFEE / 0xBADF00D`, `0x9E3779B9 / 0x7F4A7C15`, with the comment *"non-zero (Xoroshiro (0,0) is degenerate)"* |
  | `src/dsp/DriftEngine.hpp:91,96-98` | `seed(s0, s1=0)` / `setSpreadSeed(s0, s1=0)` — **`s1` defaults to 0**, so a caller passing only `s0=0` reproduces the hazard |
  | `src/dsp/PatchParse.hpp` | `forge::parseSeedHex` — non-throwing patch-seed parse (BUG-04 fix) |
- **Safe non-degenerate default for `VcoBlockDriver`:** reuse `0x1234 / 0x5678` (drift) and `0x9E3779B9 / 0x7F4A7C15` (spread) verbatim from `BlockDriver`. Rationale: identical provenance, already documented, already proven non-degenerate, and it keeps the two drivers trivially comparable. *(`0x9E3779B9` and `0x7F4A7C15` are the well-known Weyl/32-bit-mix constants — arbitrary but non-zero, which is all that matters.)*
- **Note for the hollow P29 seam:** even though `VcoCore::step()` returns silence, wire `seed()`/`setSpreadSeed()` into the seam now so `VcoBlockDriver`'s ctor discipline is real from day one and P30/P34 do not have to change the driver.

---

## `VcoCore` / `AnalogVCO` Naming — Current File Inventory

**Nothing VCO-related exists yet.** Verified by directory listing and grep.

### `src/` today
```
src/AnalogLFO.cpp      49486 B   the shipped Rack shell (SHIPPED — do not touch)
src/plugin.cpp           124 B   init(): p->addModel(modelAnalogLFO);
src/plugin.hpp           118 B   extern Model* modelAnalogLFO;
src/dsp/Anim.hpp          2031 B
src/dsp/ClockTracker.hpp  8562 B
src/dsp/DisplayFill.hpp   2427 B
src/dsp/DriftEngine.hpp   7087 B   ← frozen (D-05)
src/dsp/LfoCore.hpp      10141 B
src/dsp/MathConst.hpp      711 B   ← frozen (D-05)
src/dsp/PatchParse.hpp    1233 B
src/dsp/RackCompat.hpp    4753 B   ← frozen (D-05)
src/dsp/RatioTable.hpp    2601 B
src/dsp/Swing.hpp         1558 B
src/dsp/Waveshape.hpp     9180 B   ← frozen (D-05)
```
`plugin.json` has exactly one `modules[]` entry (`slug: "ForgeAnalogLFO"`), `version: "2.0.1"`.

### `tests/` today
`BlockDriver.hpp`, `doctest.h` (2.4.11), `main.cpp`, `check_docs.sh`, and 9 `test_*.cpp` (`anim, display, dsp_stateful, dsp_units, extraction, golden, invariants, regression, smoke`) + `golden/`.

### NEW vs MODIFIED for Phase 29

| File | Status | Notes |
|---|---|---|
| `src/dsp/VcoCore.hpp` | **NEW** | `forge::VcoInputs` POD + `forge::VcoCore` (silent `step()`), `Telemetry`, `seed`/`setSpreadSeed`. Zero `rack/`. Must compile at **both** c++11 and c++17 |
| `tests/VcoBlockDriver.hpp` | **NEW** | copy of `BlockDriver.hpp`, retargeted |
| `tests/test_vco_harness.cpp` | **NEW** | TEST-01 plumbing invariants at 44.1/48/96 kHz |
| `tests/test_lfo_guardrail.cpp` | **NEW** | D-04 golden hash lock (+ optional D-05 in-test variant) |
| `tests/Sha256.hpp` | **NEW** *(if in-test hashing chosen)* | vendored SHA-256, tests-only, never in `src/` |
| canary TU (`src/vco_compile_canary.cpp` **or** `tools/vco_canary.cpp`) | **NEW** | D-07; placement decision below |
| `tests/check_includes.sh` | **NEW** | D-06 dependency-direction grep, mirrors `tests/check_docs.sh` shape |
| `tests/golden/SHA256SUMS` / `src/dsp/FROZEN.sha256` | **NEW** (data) | manifest(s) for D-04/D-05 — format/location is planner's discretion |
| `.github/workflows/test.yml` | **MODIFIED** (additive steps) | wire D-05/D-06 guards; wire canary if it is not in `src/` |
| `Makefile` | **MODIFIED** *(only if canary is outside `src/`, or if a `make guards` target is added)* | if a new Rack-free target is added it **must** join the `ifeq ($(filter test capture,...))` skip list |
| `tests/test_golden.cpp`, `tests/BlockDriver.hpp`, `src/dsp/*.hpp` (all 11), `src/AnalogLFO.cpp`, `plugin.json`, `plugin.cpp`, `plugin.hpp`, `tests/golden/*.f32` | **UNCHANGED — hard requirement** | see §"Risk to the Shipped LFO" |

### Recommended `VcoInputs` field set (Claude's discretion, D-03)

Take `.planning/research/ARCHITECTURE.md` Pattern 1 verbatim **minus** the two sync fields (P33 owns those; adding POD fields later is a non-breaking additive change because the VCO has no goldens until P36):

```cpp
namespace forge {
struct VcoInputs {
    // --- pitch (volt/octave domain; summed before ONE exp2) --- P30/P31
    float pitchCV     = 0.f;
    float coarse      = 0.f;
    float fine        = 0.f;
    // --- exponential FM (added into pitch volts BEFORE exp2) --- P31
    float fmVolts     = 0.f;
    float fmAtten     = 0.f;
    bool  fmConnected = false;
    // --- analog engine (post-CV, post-clamp [0,1]) --- P34; names match forge::Inputs
    float morph       = 0.f;
    float character   = 0.f;
    float drift       = 0.f;
    // --- injected timing (harness/shell owns this) ---
    float sampleTime  = 1.f / 44100.f;
    float sampleRate  = 44100.f;   // PITCH-04 Nyquist clamp
};
} // namespace forge
```
Keep `morph/character/drift` **identically named** to `forge::Inputs` so the shared `Waveshape`/`DriftEngine` wiring is copy-paste in P34. *(Sync fields deferred: `syncVoltage`, `syncConnected` — P33.)*

Recommended `VcoCore::Telemetry` (from ARCHITECTURE step 10): `float freqHz = 0.f; float displayPhase = 0.f; bool syncFired = false;`.

---

## The C++17-ism / ODR Hazard — Empirically Re-Verified

### The historical failure (from `git show 8615945` and `.planning/RETROSPECTIVE.md:226-235`)

> MinGW `ld` failed with `undefined reference` to three in-class `static constexpr` arrays (`RATIO_TABLE`, `RATIO_LABELS`, `SWING_OVERLAY_LABELS`). Root cause: the VCV toolchain builds **all** platforms with `-std=c++11`, where in-class `static constexpr` members are *declarations only* — ODR-use (runtime indexing takes the array's address) requires an out-of-line definition. **Local mac clang folds the references at `-O3`, masking the bug entirely.**

The shipped fix, `src/AnalogLFO.cpp:384-395`:
```cpp
// Out-of-line definitions for the in-class static constexpr arrays. Required under
// C++11/14 ... ODR-use (runtime indexing takes the array's address) needs a definition
// or MinGW's linker fails with "undefined reference". Redundant but harmless from C++17 onward.
constexpr float AnalogLFO::RATIO_TABLE[15];
constexpr const char* AnalogLFO::RATIO_LABELS[15];
constexpr float AnalogLFO::SWING_FRACTIONS[6];
constexpr const char* AnalogLFO::SWING_MENU_LABELS[6];
constexpr const char* AnalogLFO::SWING_OVERLAY_LABELS[6];
```

### What each gate actually catches — measured this session

Test A: the ODR class. A scratch header with `struct VcoTable { static constexpr float TBL[4] = {...}; static float pick(int i){ return TBL[i & 3]; } };`, ODR-used from an external-linkage function.

| Command (Apple clang 16, this machine) | Result |
|---|---|
| `clang++ -std=c++11 -pedantic-errors -fsyntax-only -Wall -Wextra` | **PASS** — strict does not and cannot catch it |
| `clang++ -std=c++11 -O0 … -o bin` (link) | **PASS** — links clean |
| `clang++ -std=c++11 -O3 … -o bin` (link) | **PASS** — links clean |
| `nm -C odr.o` | `l__ZN5forge8VcoTable3TBLE.const` — a **local** (per-TU) symbol; clang materialises its own copy |

> **[VERIFIED: local experiment]** The ODR class does **not reproduce on macOS/clang at any optimization level**. There is no local substitute for the CI GCC/MinGW link leg. A plan task that says "reproduce the ODR failure locally" is unsatisfiable; the negative control must run in CI.

Test B: C++17-isms under `-std=c++11 -pedantic-errors -fsyntax-only`:

| Construct | Result |
|---|---|
| `inline constexpr double kX = 1.0;` | `error: inline variables are a C++17 extension [-Werror,-Wc++17-extensions]` |
| `if constexpr (…)` | `error: constexpr if is a C++17 extension [-Werror,-Wc++17-extensions]` |
| `[[maybe_unused]] int x;` | `error: use of the 'maybe_unused' attribute is a C++17 extension [-Werror,-Wc++17-attribute-extensions]` |
| `std::clamp(2,0,1)` | `error: no member named 'clamp' in namespace 'std'` |

> **[VERIFIED: local experiment]** `make strict` is a real, locally-runnable, hard-failing C++17-ism gate — for all four of the classes that historically bit this project. It is *only* the ODR/link class that requires CI.

### Rules the new VCO headers must obey

| Rule | Why | In-repo precedent |
|---|---|---|
| Namespace-scope `static constexpr` (or plain `constexpr`) for tables/constants — **never** in-class `static constexpr` that gets runtime-indexed | Sidesteps ODR entirely (internal linkage ⇒ each TU has a definition) | `src/dsp/RatioTable.hpp:17` `static constexpr float RATIO_TABLE[15]`; `src/dsp/Swing.hpp:18`; `src/dsp/MathConst.hpp:14` `constexpr double kPi` (explicitly *not* `inline constexpr`) |
| No `inline constexpr` variables | C++17 | `MathConst.hpp` banner |
| No `if constexpr`, structured bindings, nested-namespace `a::b {}`, `[[maybe_unused]]` | C++17 | commit `8615945` |
| No `std::clamp` | C++17 | `RackCompat.hpp:97` ships `forge::clamp` for exactly this reason |
| No `auto` return-type deduction, no generic lambdas | C++14 | — |
| **No brace-init of a struct with NSDMIs** (`VcoInputs in{1.f, …}`) | In C++11 a class with NSDMIs is *not* an aggregate (relaxed only in C++14). `VcoInputs in;` and `VcoInputs in{};` are fine; `VcoInputs in{…values…}` is a C++11 error | `forge::Inputs` is always used as `forge::Inputs in;` throughout `tests/` |
| Zero `rack/` includes; only `"dsp/*.hpp"` siblings + standard headers | Rack-free `make test` | every `src/dsp/*.hpp` |

> Note: `tests/*.cpp` build at **c++17** and freely use `std::clamp` (e.g. `tests/test_extraction.cpp:78`). Test TUs and `tests/VcoBlockDriver.hpp` are therefore **not** c++11-clean and **must not** be included by the canary. The canary covers `src/dsp/Vco*.hpp` only.

---

## Compile Canary Design (D-07) — the key decision

### The trap that makes a naive canary useless

A TU that only does `#include "dsp/VcoCore.hpp"` and nothing else **emits no code**. Header-defined inline/member functions are only instantiated when used, so no `static constexpr` is ever ODR-used and the MinGW link leg has nothing to resolve. Such a canary would be permanently, silently green and would satisfy neither D-07 nor success criterion 3.

**The canary must ODR-use the headers.** Sketch:

```cpp
// vco_compile_canary.cpp
// PERMANENT C++11 / ODR COMPILE CANARY (Phase 29 D-07/D-08).
// Purpose: force every VCO DSP header through (1) the -std=c++11 -pedantic-errors
// syntax gate and (2) the CI MinGW compile+link-vs-libRack leg — the only gate that
// catches the in-class `static constexpr` ODR class that rejected v2.0.0.
// EVERY new VCO header must be added to the include list below (D-08).
// This TU deliberately contains no DSP and is never called at runtime.
#include "dsp/VcoCore.hpp"
// P32: #include "dsp/MorphBlep.hpp"

namespace forge {
// External linkage + a runtime-dependent argument: prevents the optimizer from
// folding the header code away, so inline bodies are emitted and any ODR-used
// static constexpr table produces a real relocation the linker must resolve.
float vcoCompileCanaryProbe(int i);
float vcoCompileCanaryProbe(int i) {
    VcoCore core;
    core.seed(0x1234ULL, 0x5678ULL);
    core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);
    VcoInputs in;
    in.sampleTime = 1.f / 44100.f;
    in.sampleRate = 44100.f;
    float acc = 0.f;
    for (int k = 0; k < (i & 3) + 1; ++k) acc += core.step(in);   // runtime trip count
    return acc + core.tel.displayPhase;
}
} // namespace forge
```

The `(i & 3)` runtime trip count and the external-linkage signature are load-bearing: they defeat constant folding and dead-symbol elimination. `compile.mk` sets no `-fvisibility=hidden` and no `-flto`, so the symbol survives into `plugin.dll`.

### Placement options

| | **Option A — `src/vco_compile_canary.cpp`** *(recommended)* | **Option B — `tools/vco_canary.cpp`** (or `tests/`) |
|---|---|---|
| `make strict` coverage | automatic (`$(wildcard src/*.cpp)`) | needs a Makefile edit |
| CI strict step coverage | automatic (`src/*.cpp`) | needs a workflow edit |
| CI MinGW compile+link coverage | automatic (`for f in src/*.cpp`, then `build-ci/*.o`) | needs a workflow edit |
| Local `make` (plugin build, c++11) coverage | automatic — a **4th** free gate | none |
| Fidelity to how `AnalogVCO.cpp` will be gated in P30 | identical by construction (D-08's stated intent) | approximate |
| Wiring that can silently rot | none | 3 separate places, and the Makefile/CI strict duplication already proves drift happens here |
| Cost | ships one extra tiny TU (one unused `forge::vcoCompileCanaryProbe` symbol, no static init, no runtime effect) inside the released `plugin.dylib`/`.so`/`.dll` and the VCV Library build | zero release-artifact impact |

**Recommendation: Option A.** The operator's stated posture ("a canary that only warns is not a canary"; chose all three tripwires) prioritises a guard that cannot silently stop covering something. Option A is self-maintaining across all four gates with zero build-file edits, and it is the highest-fidelity stand-in for the real `AnalogVCO.cpp`. The cost is a single unused namespaced function in the shipped binary — no static initialiser, no LFO code path touched, no symbol-collision surface (`forge::vcoCompileCanaryProbe` is unique). Document that cost in the file banner so a future reader does not "clean it up."

**If the operator objects to any dead code in the shipped artifact**, take Option B and make the three edits in one plan task, plus add a D-06 grep asserting the canary file is referenced by the Makefile strict target and by both CI steps — otherwise the wiring can rot exactly the way `tests/check_docs.sh` did (see §Pitfalls P-5).

### Optional extra gate (cheap, additive) — native-GCC standalone link

Because the canary includes only Rack-free headers, it has **no** Rack symbol dependencies and can be link-checked on native Linux GCC without any SDK:

```bash
g++ -std=c++11 -O3 -Isrc -shared -Wl,--no-undefined -o /dev/null src/vco_compile_canary.cpp
```

This would surface the ODR class in ~1 s, independent of the MinGW toolchain install. **[ASSUMED]** — the mechanism is the same one that failed in v2.0.0, but this exact invocation was not run (no GCC and no Linux available locally). It is *additive*, not a replacement: D-07 mandates the MinGW leg, which must stay.

---

## Tripwire Design (D-04 / D-05 / D-06)

### D-04 — golden-byte checksum lock

| Option | Mechanism | Assessment |
|---|---|---|
| **A (recommended)** | Vendored SHA-256 in `tests/Sha256.hpp` (tests-only, ~120 lines, C++11-clean, no deps) + a doctest case in **new** `tests/test_lfo_guardrail.cpp` that hashes all six `.f32` files and compares against expected digests **hard-coded as string literals in the test source** | Runs in the *same `make test` run* on all three OSes and locally (matches roadmap criterion 1). Zero external-tool dependency — important, because `sha256sum` **does not exist on macOS** (verified: only `shasum` and `openssl` are present). `.f32` files contain NUL bytes so git treats them as binary ⇒ **no CRLF hazard**. Hard-coding the digests in the TU (rather than a data file) means changing a golden requires a *code* diff, which is the whole point of D-04 |
| B | CI shell step `sha256sum -c tests/golden/SHA256SUMS` | Simplest, but ubuntu-only in practice (`sha256sum` absent on macOS; Git-Bash availability on windows-latest is **[ASSUMED]**), and it does not run under `make test` |
| C | Cheap non-cryptographic checksum (FNV-1a) | Contradicts D-04's explicit "SHA-256" |

Also emit a machine-readable `tests/golden/SHA256SUMS` alongside (Option A + a data file) so a human can run `shasum -a 256 -c tests/golden/SHA256SUMS` — belt and suspenders, one file.

**Meta-guard worth adding:** hash `tests/test_golden.cpp` itself. Otherwise the canary's own source can be weakened (`==` → `Approx`, widened `DRIFTOFF_EPSILON`, a case commented out) without any tripwire firing. This is the highest-leverage single addition in the phase and is fully consistent with D-04's intent.

### D-05 — frozen-header hash guard

**The CRLF hazard.** There is **no `.gitattributes` in this repo** (verified). GitHub's `windows-latest` runners ship git with `core.autocrlf=true`, so `.hpp` files check out with CRLF and their SHA-256 differs from the LF hashes above. **[ASSUMED — high likelihood, must be handled]**

| Option | Mechanism | Assessment |
|---|---|---|
| **A (recommended)** | Ubuntu-only step in `toolchain-gate`: `sha256sum -c src/dsp/FROZEN.sha256` | Dodges CRLF entirely (ubuntu checkout is LF), zero new C++ code, hard-fails, and D-05's purpose (block a *silent* edit) is fully served by one leg |
| B | In-test doctest case using the same `Sha256.hpp`, hashing with `\r` stripped before digesting | Runs everywhere incl. locally; requires documenting "hash is over LF-normalized bytes" so the manifest can't be regenerated by a raw `shasum` |
| C | Add `.gitattributes` (`* text=auto eol=lf`) then hash raw | Repo-wide checkout-behavior change affecting Windows contributors — surface to the operator before choosing |

**Scope gap in D-05 (must be surfaced).** D-05 names four headers. The LFO's actual behavioral closure is eleven:

```
src/AnalogLFO.cpp → LfoCore.hpp, MathConst.hpp, PatchParse.hpp, DisplayFill.hpp, Anim.hpp
LfoCore.hpp       → MathConst, RackCompat, Waveshape, RatioTable, Swing, ClockTracker, DriftEngine
ClockTracker.hpp  → RackCompat, RatioTable
DriftEngine.hpp   → RackCompat, MathConst
DisplayFill.hpp   → Waveshape
Waveshape.hpp     → MathConst, RackCompat
```

| Header | In D-05's four? | Covered by a golden replay? |
|---|---|---|
| `Waveshape.hpp`, `RackCompat.hpp`, `MathConst.hpp`, `DriftEngine.hpp` | ✅ | ✅ |
| `LfoCore.hpp` | ❌ | ✅ (fully — it *is* the replayed core) |
| `RatioTable.hpp`, `Swing.hpp`, `ClockTracker.hpp` | ❌ | ⚠ partially — the golden scenario is free-run, `clkConnected=false`, `swingIndex=0` |
| **`PatchParse.hpp`, `DisplayFill.hpp`, `Anim.hpp`** | ❌ | ❌ **not at all** |

`PatchParse.hpp` / `DisplayFill.hpp` / `Anim.hpp` are exactly the three that `.planning/research/ARCHITECTURE.md` lists as *"REUSED by the VCO shell (candidates)"* — i.e. the headers a later VCO phase is most likely to edit, and the ones with **zero** behavioral coverage. A silent LFO regression there would be caught by nothing.

**Options (planner to surface to operator):**
1. **Literal D-05** — hash the four named. Lowest change, leaves the gap above.
2. **Recommended — D-05 + closure extension:** hash all eleven LFO-closure headers plus `src/AnalogLFO.cpp` and `tests/test_golden.cpp`. Strictly additive to the locked decision (a superset), zero behavioral risk, and it closes the only unguarded silent-regression surface in the milestone. A later phase that legitimately needs to touch one of them performs the same sanctioned one-line bump D-05 already anticipates for `DriftEngine.hpp` in P34.
3. Four now, extend when the VCO shell first reaches for a shell header (P35) — defers the risk into the phase least able to notice it. Not recommended.

### D-06 — include / dependency-direction audit

Precedent to mirror: `tests/check_docs.sh` — `#!/usr/bin/env bash`, `set -euo pipefail`, `SCRIPT_DIR`/`ROOT` resolution so it runs from anywhere, a `fail=0` + `note_fail()` accumulator, numbered `[n/N]` sections, non-zero exit on any hit.

Suggested assertions for `tests/check_includes.sh`:

1. **No LFO TU includes any VCO file.** Grep the enumerated LFO closure (`src/AnalogLFO.cpp` + the 11 headers) for `#include.*\(Vco\|MorphBlep\)` → must be empty.
2. **No LFO test TU includes a VCO header.** Same grep over `tests/BlockDriver.hpp`, `tests/test_golden.cpp`, `tests/test_invariants.cpp`, `tests/test_extraction.cpp`.
3. **VCO headers are Rack-free.** `src/dsp/Vco*.hpp` must contain no `#include <rack` / `#include "rack`. *(Also enforced implicitly: `make test` compiles them with no Rack include path.)*
4. **VCO headers include only `dsp/*.hpp` siblings + standard headers.** Enumerate and reject anything else.
5. **Canary completeness (D-08 enforcement).** Every `src/dsp/Vco*.hpp` and `src/dsp/MorphBlep.hpp` must appear in the canary TU's include list. This makes D-08's "each later phase adds its `#include`" mechanically enforced rather than a convention — high value, cheap.
6. *(Option B only)* the canary path appears in the Makefile strict target and in both CI steps.

**Wire it.** `tests/check_docs.sh` exists and is referenced by **nothing** — not the Makefile, not CI (verified by grep). That is the in-repo proof that an unwired guard rots. Every guard this phase creates must be invoked from `.github/workflows/test.yml`; a `make guards` convenience target is optional (and if added, must join the `ifeq ($(filter test capture,...))` skip list).

---

## Risk to the Shipped LFO — every touchpoint + the safe alternative

| # | Touchpoint | Risk | Safe additive alternative |
|---|---|---|---|
| **R-1** | `src/dsp/` frozen four (and the other 7 closure headers) | Any edit changes LFO output / shipped sound | P29 touches **none**. All new code in `src/dsp/VcoCore.hpp`. D-05 makes this mechanical |
| **R-2** | `tests/BlockDriver.hpp` — the "DRY the two drivers" temptation | **Highest-probability accidental regression in this phase.** Templating it, or changing its ctor defaults / `run()` loop, changes what the macOS bit-exact drift-ON leg feeds `LfoCore` ⇒ `freerun_*.f32` fails (best case) or shifts subtly (worst case) | **Copy, do not refactor.** `VcoBlockDriver.hpp` is an independent file. Accept ~40 duplicated lines; add `tests/BlockDriver.hpp` to the D-05 manifest so an edit cannot be silent |
| **R-3** | `tests/test_golden.cpp` — hardening the canary means editing the canary | An edit could weaken the comparison (`==`→`Approx`, widened `DRIFTOFF_EPSILON`, an `#if` that skips a case) and the file is its own only witness | Put every new guard in a **new** `tests/test_lfo_guardrail.cpp`; leave `test_golden.cpp` **byte-unchanged**, and hash-pin it (D-04 meta-guard) |
| **R-4** | `Makefile` `TEST_CXXFLAGS` | Changing `-O2`, dropping `-ffp-contract=off`, or adding `-ffast-math` shifts float results ⇒ the 1e-6 drift-off replay and the bit-exact macOS replay can both move | Never edit `TEST_CXXFLAGS`. If the canary or guards need flags, use separate `CANARY_*` / `GUARD_*` variables. Note the Windows CI leg **duplicates** these flags literally — any change needs two edits |
| **R-5** | `.github/workflows/test.yml` | Editing an existing step could weaken a live gate; the strict command is duplicated between Makefile and CI and can drift | **Add** steps; do not modify existing ones. Prefer Option A canary placement so no existing step needs editing at all |
| **R-6** | `tests/golden/*.f32` + `freerun_seeds.txt` | Running `make capture` regenerates fixtures and would mask a regression | **Do not run `make capture` in this phase.** D-04 pins the bytes; the plan should state this explicitly as a prohibition |
| **R-7** | `plugin.json` / `plugin.cpp` / `plugin.hpp` | Registration changes affect the live plugin manifest | P29 touches **none** — registration is P30 (PANEL-03) |
| **R-8** | `src/AnalogLFO.cpp` | The shipped shell | Untouched. Recommend adding it to the D-05 manifest |
| **R-9** | **`forge::` namespace collision** | `forge::Inputs` already exists. If `VcoCore.hpp` declared a *different* `forge::Inputs`, any TU including both fails loudly — but a TU including only one would compile, giving a genuine cross-TU **ODR violation** with undefined behavior | Name it `forge::VcoInputs` (ARCHITECTURE's name). Also prefix any new free functions/constants (`forge::vco*`). D-06 can grep for a second `struct Inputs` definition |
| **R-10** | Canary TU in `src/` (Option A) | The **only** P29 change that alters the shipped binary — one extra unused symbol in `plugin.dylib`/`.so`/`.dll` | Namespaced, external-linkage, no static initialiser, no LFO code path referenced. Documented in the banner. Alternative is Option B (three wiring edits). Surface the trade-off to the operator |
| **R-11** | A new Rack-free `make` target (e.g. `make guards`) | If it is not added to `ifeq ($(filter test capture,$(MAKECMDGOALS)),)`, `make guards` hard-fails on any CI runner without `../Rack-SDK` | Add the new goal name to that filter in the same edit |
| **R-12** | Adding `.gitattributes` (D-05 Option C) | Repo-wide checkout line-ending change on Windows | Prefer D-05 Option A (ubuntu-only) or B (CR-normalizing hasher); if C is chosen, surface it explicitly |

**Nothing in Phase 29 requires a behavioral or breaking change to the shipped LFO.** Every deliverable is a new file or an additive CI step. The only judgment calls that reach the shipped artifact are R-10 (canary placement) and, if chosen, R-12.

---

## Don't Hand-Roll

| Problem | Don't build | Use instead | Why |
|---|---|---|---|
| Test runner, filters, reporting | a bespoke `main()` / assertion macros | vendored **doctest 2.4.11** (`tests/doctest.h`), `tests/main.cpp` owns `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` | Already the project standard; a second impl macro = duplicate-symbol link error |
| Block-driving a core over samples | a new harness abstraction | copy `tests/BlockDriver.hpp` | Its ctor seeding + `sampleTime` overwrite discipline is the accumulated fix for two real landmines |
| Bit-exact float comparison | `doctest::Approx(x).epsilon(0)` | plain `==` | `test_golden.cpp:125-127` documents that `Approx` with `epsilon(0)` still applies relative scaling and is not a true bit-exact comparator |
| Cross-platform hashing shell-out | `sha256sum` | vendored `Sha256.hpp` in tests, or an ubuntu-only CI step | `sha256sum` **does not exist on macOS** (verified: `shasum`/`openssl` only) |
| pi | `M_PI` / `<cmath>` macro | `forge::kPi` (`MathConst.hpp`) | Exists precisely so the direct-g++ Windows leg compiles without `-D_USE_MATH_DEFINES`; IEEE-identical so goldens are unperturbed |
| Clamping | `std::clamp` | `forge::clamp` / `forge::clampi` (`RackCompat.hpp:97-98`) | `std::clamp` is C++17; hard-errors under the c++11 gate |
| PRNG | anything new | `forge::Xoroshiro128Plus` | Bit-identical to Rack; the whole golden discipline rests on it |
| Guard-script scaffolding | a new pattern | mirror `tests/check_docs.sh` | Established shape (`set -euo pipefail`, `ROOT` resolution, `note_fail`, numbered sections, non-zero exit) |

**Key insight:** every hand-rolled alternative in this domain re-opens a bug this project has already paid for once — the `(0,0)` seed hang, the `Approx` false-green, the `M_PI` Windows break, the `std::clamp` C++11 rejection, the spread-path portability divergence.

---

## Common Pitfalls (specific to this phase)

### P-1: A bare-`#include` compile canary is permanently, silently green
**What goes wrong:** the TU emits no code; nothing is ODR-used; the MinGW link leg has nothing to resolve. The gate reports PASS forever while covering nothing.
**How to avoid:** the canary must instantiate and call the headers from an **external-linkage** function with a **runtime-dependent** argument (see §Compile Canary Design).
**Warning sign:** `nm build-ci/vco_compile_canary.cpp.o` shows no `forge::` symbols and no undefined references.

### P-2: Believing `make strict` covers the ODR class
**What goes wrong:** `-fsyntax-only` never links. Green `make strict` + green `make test` was exactly the state in which v2.0.0 was tagged and rejected.
**How to avoid:** treat the CI MinGW **link** leg as the sole ODR gate. `.planning/PITFALLS.md` records "Tag/submit on green `make strict` alone" as a **never**-acceptable shortcut.
**Verified corollary:** you also cannot reproduce it locally on macOS at `-O0` or `-O3` — do not write a plan task that asks for it.

### P-3: Windows CRLF silently breaks a text-file hash guard
**What goes wrong:** no `.gitattributes` exists; `windows-latest` checks out `.hpp` with CRLF, so a cross-OS in-test SHA-256 over headers fails on Windows only.
**How to avoid:** hash headers on the ubuntu leg only (D-05 Option A), or CR-normalize before digesting (Option B). Binary `.f32` files are unaffected.

### P-4: Refactoring `BlockDriver` into a shared template
See R-2. Any change to its ctor defaults or `run()` loop is a direct LFO-golden risk. Duplicate ~40 lines instead.

### P-5: Writing a guard and not wiring it
**What goes wrong:** `tests/check_docs.sh` is a complete, well-written gate that **nothing** invokes — not the Makefile, not CI. It has been inert since Phase 27.
**How to avoid:** every guard this phase creates gets an explicit `.github/workflows/test.yml` step in the *same task* that creates it. Add a plan verification step that greps the workflow for each guard's filename.

### P-6: Adding a Rack-free `make` target without updating the plugin.mk skip filter
`ifeq ($(filter test capture,$(MAKECMDGOALS)),)` — a new goal not in that list hard-fails on runners lacking `../Rack-SDK`.

### P-7: Vacuous invariants on a silent core
**What goes wrong:** "no NaN", "output finite", "same seed ⇒ bit-identical" are trivially satisfied by a `step()` that returns `0.f`. The suite is green and proves nothing, and may stay vacuous if P30 does not revisit it.
**How to avoid:** (a) drive the core with *varying* inputs across the block (`inputAt(i)` returning a sweep) so the assertions become meaningful the instant DSP lands; (b) consider one explicit "silent by construction" assertion (`CHECK(out[i] == 0.f)`) as a deliberate tombstone P30 must remove — this forces P30 to acknowledge the seam changed. *(Tombstone is optional; it does create one line of intentional churn. Planner's call.)*

### P-8: C++11 aggregate-initialization of the POD
`VcoInputs in{…}` is a C++11 error because NSDMIs disqualify aggregate status until C++14. `VcoInputs in;` / `VcoInputs in{};` are fine. Easy to write in a c++17 test, then break the canary.

### P-9: `seed(s0)` with `s1` defaulting to 0
`DriftEngine::seed(uint64_t s0, uint64_t s1 = 0)` — a caller passing `seed(0)` recreates the `(0,0)` hang. Always pass both, always non-zero.

---

## Code Examples (verified patterns from this repo)

### Rack-free POD core boundary (`src/dsp/LfoCore.hpp:40-56, 94-118`)
```cpp
namespace forge {
struct Inputs {
    float rate = 0.7f;
    /* … */
    float sampleTime = 1.f / 44100.f;  // INJECTED, never read from a global
};

struct LfoCore {
    /* state … */
    struct Telemetry { int clockState = 0; /* … */ };
    Telemetry tel;

    LfoCore() { freqSlew.setLambda(20.f); freqSlew.out = 0.7f; /* … */ }
    void seed(uint64_t s0, uint64_t s1 = 0) { drift.seed(s0, s1); }
    void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) { /* … */ }
    float step(const Inputs& in) { /* … */ }
};
} // namespace forge
```

### Namespace-scope constexpr table — the ODR-safe pattern (`src/dsp/Swing.hpp:18`)
```cpp
namespace forge {
static constexpr float SWING_FRACTIONS[6] = { /* … */ };
} // namespace forge
```
`static` at namespace scope ⇒ internal linkage ⇒ each TU has a *definition*, so runtime indexing never produces an undefined reference. Contrast with the in-class form that rejected v2.0.0.

### Bit-exact golden replay (`tests/test_golden.cpp:116-131`)
```cpp
void replayGolden(double sr, const std::string& path) {
    auto ref = loadF32(path);
    REQUIRE(ref.size() == (size_t)GOLDEN_SAMPLES);
    forge::BlockDriver d(sr, DRIFT_S0, DRIFT_S1, SPREAD_S0, SPREAD_S1);
    forge::Inputs base = goldenBase();
    auto got = d.run((int)ref.size(), [&](int) { return base; });
    REQUIRE(got.size() == ref.size());
    for (size_t i = 0; i < ref.size(); ++i) CHECK(got[i] == ref[i]);   // true bit-exact
}
```

### Portable replay that neutralizes the spread path (`tests/test_golden.cpp:88-109`)
```cpp
forge::LfoCore core;
core.seed(DRIFT_S0, DRIFT_S1);   // drift RNG only; NEVER setSpreadSeed here
const float dt = (float)(1.0 / sr);
for (size_t i = 0; i < ref.size(); ++i) {
    forge::Inputs in = base; in.sampleTime = dt;
    got.push_back(core.step(in));
}
```

### Guard-script shape to mirror (`tests/check_docs.sh:20-30`)
```bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
fail=0
note_fail() { echo "  FAIL: $1"; fail=1; }
# … numbered [n/N] sections …
exit $fail
```

---

## Validation Architecture

*(`.planning/config.json` does not set `workflow.nyquist_validation` — treated as enabled.)*

### Test Framework

| Property | Value |
|---|---|
| Framework | **doctest 2.4.11**, vendored at `tests/doctest.h` |
| Config file | none — configuration is `tests/main.cpp` (`DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`) + `Makefile` `TEST_CXXFLAGS` |
| Quick run command | `make test` — **10.5 s** clean, **0.5 s** when up to date (measured this session) |
| Filtered run | `./build-test/test -tc="golden*"` (verified: 6 passed / 44 skipped); `-ltc` lists all cases |
| Full suite command | `make test && make strict` locally; `git push` for the CI legs (3-OS `test` + `toolchain-gate`) |
| Current baseline | 50 cases / 50 passed / 2,615,027 assertions; `make strict` PASS |

### Phase Requirements → Test Map

| Req | Behavior to validate | Type | Automated command | Exists? |
|---|---|---|---|---|
| TEST-01 | `VcoBlockDriver` drives `VcoCore` at 44.1 / 48 / 96 kHz | unit | `./build-test/test -tc="vco harness*"` | ❌ Wave 0 (`tests/test_vco_harness.cpp`) |
| TEST-01 | Harness links no libRack | **structural** | `make test` succeeds with no `-I$(RACK_DIR)/include` and no `-lRack` — a Rack include is a compile error by construction | ✅ (property of the target) |
| TEST-01 | `sampleTime == 1/sr` injected every step | unit | same TU — assert the core observed `1/sr` (expose via `Telemetry` or a probe input) | ❌ Wave 0 |
| TEST-01 | Default seeds are non-degenerate | unit | assert `VcoBlockDriver{}` ctor defaults ≠ `(0,0)`; assert the drift RNG emits a non-zero draw | ❌ Wave 0 |
| TEST-01 | Seam determinism: two identical runs bit-identical; different seed diverges | unit | mirrors `tests/test_invariants.cpp:156,175` | ❌ Wave 0 — **⚠ vacuous while `step()` is silent (P-7)** |
| TEST-01 | Output finite (no NaN/Inf) | unit | `CHECK(std::isfinite(out[i]))` | ❌ Wave 0 — ⚠ vacuous while silent |
| TEST-04 | 6 LFO golden replays still green | regression | `./build-test/test -tc="golden*"` | ✅ exists, must stay **byte-unchanged** |
| TEST-04 | Golden bytes unchanged (D-04) | property/hash | new case in `tests/test_lfo_guardrail.cpp` | ❌ Wave 0 |
| TEST-04 | Frozen headers unchanged (D-05) | property/hash | ubuntu CI step `sha256sum -c src/dsp/FROZEN.sha256` | ❌ Wave 0 |
| TEST-04 | Dependency direction (D-06) | static analysis | `tests/check_includes.sh` in CI | ❌ Wave 0 |
| TEST-06 | VCO headers compile at `-std=c++11 -pedantic-errors` | compile gate | `make strict` | ✅ target exists; ❌ canary TU missing |
| TEST-06 | VCO headers survive MinGW compile **+ link vs libRack** | **CI-only** link gate | `toolchain-gate` job on push | ✅ job exists; ❌ canary TU missing |

### What is machine-checkable vs. what needs a held-out / negative control

**Machine-checkable, fully automated, no judgment:**
- All doctest cases (`make test`), including the golden replays and the D-04 hash lock.
- `make strict` — verified this session to hard-error on all four historically-relevant C++17-isms.
- D-05 hash manifest comparison; D-06 grep assertions.
- The CI MinGW compile+link exit status.

**Requires a held-out / negative control (a green run proves nothing here):**

A guardrail is only validated by observing it go **red**. Four of this phase's deliverables are guards, so each needs a failure sample:

| Guard | Negative control | Where it can run |
|---|---|---|
| D-04 golden hash lock | Unit-test the hasher against a NIST vector (`SHA-256("abc") = ba7816bf 8f01cfea 414140de 5dae2223 b00361a3 96177a9c b410ff61 f20015ad`) **and** assert that hashing a one-byte-perturbed in-memory copy of a golden yields a different digest. Fully automatable, permanent, non-destructive | local + all CI |
| D-05 frozen-header guard | One-off: append a blank line to a temp copy of `Waveshape.hpp`, run the guard, confirm non-zero exit, discard. Can be scripted as a self-test the guard runs on a synthetic fixture rather than the real header | local + ubuntu CI |
| D-06 include audit | One-off / scripted: run the grep against a synthetic fixture directory containing a deliberate violation, assert non-zero exit | local + CI |
| **D-07 ODR link gate** | **CI-only.** Add a temporary in-class `static constexpr` array with runtime indexing (no out-of-line definition) to `VcoCore.hpp`, push, observe the `toolchain-gate` MinGW link step fail with `undefined reference`, then revert. **[VERIFIED: local experiment]** this cannot be done locally — Apple clang links it clean at `-O0` and `-O3` | **push to CI only** |
| D-07 C++17-ism gate | Locally reproducible: `inline constexpr` / `if constexpr` / `[[maybe_unused]]` / `std::clamp` each hard-error under `-std=c++11 -pedantic-errors -fsyntax-only` (all four verified this session) | local |

**Recommended:** make the D-04 hasher self-test and the D-06 fixture negative control **permanent automated cases** (they cost nothing and re-prove the guards on every run). Make the D-07 ODR negative control a **one-time `checkpoint:human-verify` task in this phase's plan** — the operator pushes a deliberately-broken canary and confirms CI goes red. Without it, criterion 3 ("failing on any ODR / C++17-ism") is asserted, not demonstrated. Optionally make it permanent via a CI step that compiles a checked-in known-bad fixture and **asserts a non-zero exit** — turning the negative control into a standing gate. **[ASSUMED — design proposal, not an in-repo pattern.]**

**Not machine-checkable in this phase:** nothing behavioral. D-01 makes the VCO core silent by design, so there is no audio/UAT surface. No in-Rack UAT is required for Phase 29.

### Sampling Rate

- **Per task commit:** `make test` (10.5 s clean / 0.5 s incremental) — well inside the < 30 s budget. Add `make strict` (~2 s) on any commit that touches `src/`.
- **Per wave merge:** `make test && make strict` + the D-05/D-06 guard scripts locally (if a `make guards` target is added).
- **Phase gate:** push → both CI jobs green (3-OS `test` including all six golden replays + `toolchain-gate` strict + MinGW link) **and** the one-time ODR negative control observed red-then-green.
- **Standing canary cadence (the phase's actual deliverable):** every subsequent phase (30–36) ends with the same command set. Because `on: [push, pull_request]`, the sampling rate is *every commit* — appropriate for a guardrail whose failure mode is silent.

### Wave 0 Gaps

- [ ] `src/dsp/VcoCore.hpp` — the seam TEST-01 drives
- [ ] `tests/VcoBlockDriver.hpp` — covers TEST-01
- [ ] `tests/test_vco_harness.cpp` — covers TEST-01
- [ ] `tests/test_lfo_guardrail.cpp` — covers TEST-04 (D-04) + hasher self-test
- [ ] `tests/Sha256.hpp` — if in-test hashing is chosen (D-04 Option A)
- [ ] `tests/golden/SHA256SUMS` and/or `src/dsp/FROZEN.sha256` — D-04/D-05 manifests (seed values in §"Current SHA-256 values")
- [ ] `tests/check_includes.sh` — covers TEST-04 (D-06)
- [ ] canary TU — covers TEST-06
- [ ] `.github/workflows/test.yml` additive steps — wires D-05/D-06 (and the canary, if Option B)
- Framework install: **none needed** (doctest vendored, compiler present, `../Rack-SDK` present)

---

## State of the Art (project-local)

| Old approach | Current approach | When changed | Impact |
|---|---|---|---|
| Trust local mac clang `-O3` green | `-std=c++11 -pedantic-errors` strict gate + MinGW compile-and-**link** CI leg | v2.0.0 rejection → `8615945`, `8f50bfa` (2026-07-13) | The only reliable pre-tag signal; `make strict` alone is documented as insufficient |
| `inline constexpr` in headers | plain / `static constexpr` at namespace scope | `8615945` | C++17 inline variables rejected by the c++11 toolchain |
| In-class `static constexpr` arrays, runtime-indexed | out-of-line definitions after the struct | `8615945` | MinGW `undefined reference` at link |
| `M_PI` from `<cmath>` | `forge::kPi` (`MathConst.hpp`) | Phase 26 (`261d4f2`) | Windows direct-g++ leg compiles without `-D_USE_MATH_DEFINES` |
| Drift-ON goldens as the only regression pin | split policy — drift-OFF portable (1e-6, 3-OS) + drift-ON bit-exact (macOS-gated) | Phase 26 | `std::normal_distribution` is not portable across libc++/libstdc++/MinGW |
| `doctest::Approx(x).epsilon(0)` | plain `float ==` | Phase 22/26 | `Approx` still applies relative scaling; not bit-exact |

**Deprecated / do not reintroduce:** `std::clamp`, `inline constexpr` variables, `[[maybe_unused]]`, `if constexpr`, structured bindings, nested-namespace definitions, `M_PI`, and `doctest::Approx` for bit-exact comparison — anywhere in the c++11 build graph.

---

## Package Legitimacy Audit

**Not applicable — this phase installs no external packages.** The only third-party artifacts are already vendored and unchanged: `tests/doctest.h` (doctest 2.4.11) and the Rack SDK consumed via the relative `../Rack-SDK` path (CI fetches `Rack-SDK-2.6.6-{lin,win}-x64.zip` from `vcvrack.com`, unchanged by this phase). No npm/PyPI/crates dependency is added or recommended.

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

---

## Environment Availability

| Dependency | Required by | Available | Version | Fallback |
|---|---|---|---|---|
| C++ compiler (`c++`/`clang++`) | `make test`, `make strict` | ✅ | Apple clang 16.0.0 (arm64-apple-darwin23.6.0) | — |
| GNU Make | all targets | ✅ | **3.81** (stock macOS) | avoid Make ≥ 4.0 syntax |
| `../Rack-SDK` | `make`, `make strict` | ✅ | present (relative path convention) | `make test`/`capture` skip `plugin.mk` |
| doctest | `make test` | ✅ | 2.4.11 vendored (`tests/doctest.h`) | — |
| `shasum` | manifest generation | ✅ | `/usr/bin/shasum` | — |
| `openssl` | alt hashing | ✅ | `/usr/bin/openssl` (+ homebrew) | — |
| **`sha256sum`** | shell-based hash guard | ❌ | — | use `shasum -a 256` locally; `sha256sum` on ubuntu CI; or in-test C++ SHA-256 |
| **`x86_64-w64-mingw32-g++`** | reproducing the ODR link gate locally | ❌ | — | **none** — the gate is CI-only (and clang cannot reproduce the failure anyway) |
| **`docker`** | simulating the linux/MinGW legs locally | ❌ | — | **none** — push to CI |
| GitHub Actions runners | all CI legs | ✅ | ubuntu/macos/windows-latest, `on: [push, pull_request]` | — |

**Missing dependencies with no fallback:**
- MinGW cross-compiler and Docker are both absent locally ⇒ **the D-07 link gate and its negative control can only be exercised by pushing to CI.** The plan must budget for a push-and-observe cycle rather than a local verification step.

**Missing dependencies with fallback:**
- `sha256sum` — use the in-test C++ hasher (D-04 Option A) and/or an ubuntu-only CI step (D-05 Option A).

---

## Security Domain

Offline audio-plugin domain; no network, no auth, no multi-user surface. The only adversarial input is persisted patch state, and Phase 29 introduces none.

| ASVS Category | Applies | Standard control |
|---|---|---|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| **V5 Input Validation** | **partially — deferred** | Deserialized RNG seeds must be domain-validated (`{0,0}` rejected) — the LFO already does this via `forge::parseSeedHex` (`src/dsp/PatchParse.hpp`, BUG-04). The VCO acquires a serialization surface only in **P30/P35**; Phase 29 has no JSON path |
| V6 Cryptography | no (non-security use) | SHA-256 here is an integrity/tripwire mechanism, not a security control. Use a vendored, test-scope implementation; never hand-roll a *security* primitive in `src/` |
| V14 Configuration | yes | CI workflow edits are additive; pin nothing new; `RACK_SDK_VERSION: 2.6.6` stays as-is |

| Threat pattern | STRIDE | Mitigation |
|---|---|---|
| All-zero seed from a hand-edited patch → `std::normal_distribution` infinite loop → Rack hang (DoS-by-patch) | Denial of Service | Validate seeds on load; never default `s1` to 0 in a real seeding call. **Already fixed for the LFO; must not be re-introduced by VCO code (P30+).** Phase 29 contributes by making non-degenerate seeds the harness default |
| Silent regression of a live, published module | Tampering (integrity) | This entire phase — D-04/D-05/D-06 tripwires + strict/MinGW canary |

---

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|---|---|---|
| A1 | GitHub `windows-latest` checks out text files with CRLF (`core.autocrlf=true`), breaking a cross-OS SHA-256 over `.hpp` files | Tripwire D-05 / Pitfall P-3 | If wrong: the recommended ubuntu-only placement is merely conservative (no harm). If right and ignored: Windows CI goes red on a guard that has nothing to do with the LFO |
| A2 | Git-for-Windows bash on `windows-latest` provides `sha256sum` | Tripwire D-04 Option B | Only affects the *rejected* option; the recommendation avoids the dependency entirely |
| A3 | GCC (CI ubuntu / MinGW) accepts `-Wc++14-extensions` / `-Wc++17-extensions` as `-Werror=` targets | CI hardening suggestion | An unrecognized flag fails the CI step outright — verify in a throwaway CI run before adopting; the hardening is optional |
| A4 | `g++ -std=c++11 -O3 -shared -Wl,--no-undefined` on native Linux reproduces the ODR `undefined reference` without libRack | Compile Canary "optional extra gate" | Purely additive; the mandated MinGW leg is unaffected if this does not work |
| A5 | Replacing the CI strict step with `make strict RACK_DIR=/tmp/lin/Rack-SDK` would work (removing the Makefile↔CI duplication) | CI section | `plugin.mk` is included for non-`test`/`capture` goals; it worked locally against `../Rack-SDK`, but the CI SDK layout was not exercised. Low value / non-zero risk — **recommend leaving the duplication alone this phase** and instead choosing canary Option A so no CI step needs editing |
| A6 | A permanent "compile a known-bad fixture, assert non-zero exit" CI step is a sound standing negative control | Validation Architecture | Design proposal, not an in-repo pattern. If it proves flaky, fall back to the one-time `checkpoint:human-verify` |
| A7 | Adding one unused external-linkage function to `src/` has no practical effect on the shipped plugin | Canary Option A / R-10 | Based on reading `compile.mk`/`plugin.mk` (no `-fvisibility=hidden`, no `-flto`) — the *linkage* claim is verified; the "no practical effect" judgment is mine and should be confirmed by the operator since it touches the released artifact |

---

## Open Questions

1. **Should D-05's manifest cover four headers or the full eleven-header LFO closure?**
   - Known: D-05 locks the four. The closure is eleven; `PatchParse.hpp` / `DisplayFill.hpp` / `Anim.hpp` have **zero** golden coverage and are the three ARCHITECTURE flags for VCO-shell reuse.
   - Unclear: whether the operator considers the extension in-spirit (it is a strict superset of the locked decision, not a contradiction).
   - Recommendation: plan for the extension, and surface it to the operator as *"D-05 as written leaves three shell headers unguarded with no behavioral backstop; extending the manifest is additive and costs one line each."*

2. **Canary placement — `src/` (ships a dead TU) vs. `tools/` (three wiring edits that can rot).**
   - Recommendation: `src/vco_compile_canary.cpp`, surfaced to the operator as a release-artifact decision (R-10). Both options fully satisfy D-07/D-08.

3. **Is a "silent by construction" tombstone assertion (`CHECK(out == 0.f)`) worth one line of deliberate P30 churn?**
   - Recommendation: yes — it is the only thing preventing P29's plumbing suite from being silently vacuous, and it forces P30 to acknowledge the seam changed. Planner's call.

4. **Should `tests/check_docs.sh` be wired into CI while this phase is editing the workflow?**
   - Strictly out of scope (Phase 27 artifact), but it is one line and the phase is already establishing "guards must be wired." Flag as a candidate `.planning/todos/` item rather than absorbing it.

---

## Project Constraints (from CLAUDE.md)

**No `./CLAUDE.md` or `./.claude/CLAUDE.md` exists in this repository** (verified). `.claude/` contains only `settings.local.json` and is gitignored. **No project skills directory** (`.claude/skills/`, `.agents/skills/`) exists.

Binding constraints therefore come from planning documents, and are reproduced here for the planner:

- **`.planning/REQUIREMENTS.md` (milestone guardrail):** *"The shipped Analog LFO — live in the VCV Library, pinned by bit-exact `.f32` goldens — must not get breaking or behavioral changes. All VCO work is additive (new files); shared `src/dsp/` headers stay frozen. Any change risking LFO behavior is surfaced to the operator with impact + remediation options + a recommendation before proceeding."*
- **`.planning/ROADMAP.md` line 99:** the four shared headers stay frozen; `DriftEngine.hpp` is the sole sanctioned additive touch, in Phase 34.
- **`.planning/PITFALLS.md`:** "Tag/submit on green `make strict` alone" is a **never**-acceptable shortcut; the MinGW link leg is required.
- **`.planning/config.json`:** `commit_docs: true`, `granularity: fine`, `workflow.plan_check: true`, `workflow.verifier: true`, `parallelization: true`, `use_worktrees: false`.

---

## Sources

### Primary (HIGH confidence — read directly from this repo this session)
- `Makefile` (78 lines, full read) — targets, `TEST_CXXFLAGS`, strict command, `plugin.mk` skip filter, `RACK_DIR ?= ../Rack-SDK`
- `.github/workflows/test.yml` (89 lines, full read) — both jobs, triggers, MinGW compile+link commands
- `tests/BlockDriver.hpp`, `tests/main.cpp`, `tests/test_golden.cpp`, `tests/test_extraction.cpp`, `tests/test_invariants.cpp` (head), `tests/check_docs.sh` (head), `tools/capture_golden.cpp` — full or targeted reads
- `src/dsp/LfoCore.hpp`, `RackCompat.hpp`, `DriftEngine.hpp`, `MathConst.hpp`; `src/plugin.cpp`, `src/plugin.hpp`, `plugin.json`; include-graph grep over all of `src/dsp/*.hpp` and `src/AnalogLFO.cpp`
- `tests/golden/freerun_seeds.txt` — format, seeds, epsilon rationale, canonical-OS policy
- `../Rack-SDK/compile.mk`, `../Rack-SDK/plugin.mk` — real toolchain flags (`-std=c++11 -O3 -funsafe-math-optimizations`, `-fPIC`, `-shared -lRack`, no `-fvisibility`/`-flto`)
- `git show 8615945`, `git log` — the ODR fix commit and its message
- `.planning/RETROSPECTIVE.md:226-283`, `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`, `.planning/config.json`
- `.planning/research/{ARCHITECTURE,PITFALLS,STACK}.md` — milestone-level research, built on rather than duplicated
- **Live command output:** `make test` (clean: 50/50 pass, 2.6M assertions, 10.5 s), `./build-test/test -tc="golden*"` (6 pass / 44 skip), `./build-test/test -ltc`, `make strict` (PASS), `shasum -a 256` over goldens + frozen headers, environment probes (`sha256sum` absent, no MinGW, no Docker, GNU Make 3.81, Apple clang 16)
- **Live experiments:** ODR `static constexpr` reproduction attempt at `-fsyntax-only` / `-O0` / `-O3` + `nm` symbol inspection; four C++17-ism rejection tests under `-std=c++11 -pedantic-errors`

### Secondary (MEDIUM confidence)
- `.planning/research/PITFALLS.md` claims about GCC/MinGW behavior (consistent with the v2.0.0 rejection record, but GCC was not runnable locally)

### Tertiary (LOW confidence)
- GitHub Actions `windows-latest` `core.autocrlf` default (A1); Git-Bash `sha256sum` availability (A2); GCC `-Wc++NN-extensions` flag names (A3); native-GCC `--no-undefined` standalone link reproduction (A4)

---

## Metadata

**Confidence breakdown:**
- Existing harness / golden / Makefile / CI facts: **HIGH** — read line-by-line from source, with commands executed to confirm behavior
- C++17-ism and ODR gate behavior: **HIGH** — empirically re-verified on this machine, including the negative result that macOS cannot reproduce the ODR class
- LFO-regression touchpoint enumeration: **HIGH** — derived from the actual include graph and the actual build globs
- Tripwire mechanism recommendations (D-04/05/06): **MEDIUM-HIGH** — the constraints are verified; the specific mechanism choices are engineering judgment, with options presented
- CRLF / Git-Bash / GCC-flag specifics: **LOW-MEDIUM** — see Assumptions Log A1–A3

**Research date:** 2026-07-28
**Valid until:** 2026-08-27 (30 days — the toolchain, SDK 2.6.6, and CI shape are stable; re-verify only if `.github/workflows/test.yml`, the `Makefile`, or `tests/golden/` changes)
