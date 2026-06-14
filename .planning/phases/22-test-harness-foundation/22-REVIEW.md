---
phase: 22-test-harness-foundation
reviewed: 2026-06-14T00:00:00Z
depth: standard
files_reviewed: 18
files_reviewed_list:
  - src/AnalogLFO.cpp
  - src/dsp/RackCompat.hpp
  - src/dsp/Waveshape.hpp
  - src/dsp/RatioTable.hpp
  - src/dsp/Swing.hpp
  - src/dsp/ClockTracker.hpp
  - src/dsp/DriftEngine.hpp
  - src/dsp/LfoCore.hpp
  - tests/BlockDriver.hpp
  - tests/main.cpp
  - tests/test_smoke.cpp
  - tests/test_dsp_units.cpp
  - tests/test_dsp_stateful.cpp
  - tests/test_extraction.cpp
  - tests/test_golden.cpp
  - tests/test_invariants.cpp
  - Makefile
  - .github/workflows/test.yml
findings:
  critical: 3
  warning: 4
  info: 3
  total: 10
status: issues_found
---

# Phase 22: Code Review Report

**Reviewed:** 2026-06-14
**Depth:** standard
**Files Reviewed:** 18
**Status:** issues_found

## Summary

Phase 22 extracts the DSP core from the 1641-line `AnalogLFO.cpp` into a header-only `forge::` library under `src/dsp/`, wires the plugin shell to delegate to `LfoCore::step()`, and erects a Rack-free doctest harness with invariant, extraction-gate, and golden-regression tests, plus GitHub Actions CI. The extraction is disciplined: bit-identity preservation is explicit, seeding hazards are called out, and the D-05 bleed-wiring (post-drift ouLayers[0].state) is correctly threaded through DriftEngine::Result. The test suite is well-structured and covers the main correctness axes.

Three genuine blockers were found. The most significant is that the CI unix `make test` step will always fail on GitHub Actions runners because `include $(RACK_DIR)/plugin.mk` at line 17 of the Makefile requires `../Rack-SDK/plugin.mk` to be present, which it is not in a clean CI checkout. A `dataFromJson` null-pointer crash exists when the saved `.vcv` patch contains the spread seed as a non-string JSON node (e.g., hand-edited). And `ClockTracker.hpp` duplicates the `shouldReset()` division logic verbatim instead of calling `forge::shouldReset()`, creating a guaranteed silent divergence when Phase 23 applies the BEATS_PER_ALIGN table fix.

## Critical Issues

### CR-01: CI unix `make test` unconditionally fails without Rack SDK

**File:** `Makefile:17` and `.github/workflows/test.yml:22`

**Issue:** The Makefile uses `include $(RACK_DIR)/plugin.mk` (not `-include`). GNU Make treats bare `include` as a hard error when the file is absent. On the `ubuntu-latest` and `macos-latest` GitHub Actions runners, `RACK_DIR` defaults to `../Rack-SDK` (line 1), which does not exist in a plain repo checkout. The `make test (unix)` step therefore fails immediately at Makefile parsing — before reaching the `test:` target — with "No such file or directory". The CI comment says "No Rack toolchain needed" but the Makefile itself contradicts that for unix runners. Only the Windows path (direct `g++` invocation, which bypasses the Makefile entirely) is currently functional in CI.

**Fix:** Either switch to `-include` for `plugin.mk`, or install the Rack SDK in CI before running `make test`, or extract the test build recipe into a standalone `Makefile.test` that has no dependency on plugin infrastructure:

```make
# Option A: tolerate absent plugin.mk for the test target only
-include $(RACK_DIR)/plugin.mk

# Option B: standalone test makefile (tests/Makefile) invoked as:
#   cd tests && make
# Or guard the include so the test target works without it:
ifneq ($(MAKECMDGOALS),test)
include $(RACK_DIR)/plugin.mk
endif

TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off
test: $(TEST_SOURCES) $(TEST_HEADERS)
    @mkdir -p build-test
    $(CXX) $(TEST_CXXFLAGS) $(TEST_SOURCES) -o build-test/test
    ./build-test/test
```

---

### CR-02: `dataFromJson` crashes on malformed patch: `json_string_value` returns NULL for non-string nodes

**File:** `src/AnalogLFO.cpp:242-243`

**Issue:** The guard on line 241 only checks that the JSON nodes `s0J` / `s1J` are non-null (i.e., the keys exist), not that they are of type `JSON_STRING`. When a `.vcv` patch file is hand-edited or corrupted such that `spreadSeed0` is stored as a JSON integer or boolean, `json_object_get` returns a non-null node and the guard passes, but `json_string_value()` returns `NULL` for non-string nodes per the Jansson API contract. `std::stoull(NULL, nullptr, 16)` is undefined behavior that crashes in practice (null pointer dereference inside the string-to-integer conversion).

```cpp
// VULNERABLE:
json_t* s0J = json_object_get(rootJ, "spreadSeed0");
json_t* s1J = json_object_get(rootJ, "spreadSeed1");
if (s0J && s1J) {
    spreadSeed[0] = std::stoull(json_string_value(s0J), nullptr, 16);  // json_string_value can return NULL
```

**Fix:**

```cpp
json_t* s0J = json_object_get(rootJ, "spreadSeed0");
json_t* s1J = json_object_get(rootJ, "spreadSeed1");
if (json_is_string(s0J) && json_is_string(s1J)) {
    spreadSeed[0] = std::stoull(json_string_value(s0J), nullptr, 16);
    spreadSeed[1] = std::stoull(json_string_value(s1J), nullptr, 16);
    initComponentSpread();
}
```

---

### CR-03: `ClockTracker::step()` duplicates `shouldReset()` logic instead of calling it, guaranteeing silent divergence at Phase 23

**File:** `src/dsp/ClockTracker.hpp:168-173`

**Issue:** `ClockTracker.hpp` already includes `RatioTable.hpp` (line 28) for `forge::shouldReset`, but the actual reset decision at lines 168-173 re-implements that logic inline instead of calling the function:

```cpp
// ClockTracker.hpp:168-173 (duplicate):
bool reset = true;
if (currentRatioIdx >= 0 && RATIO_TABLE[currentRatioIdx] < 1.f) {
    int divisor = (int)std::round(1.f / RATIO_TABLE[currentRatioIdx]);
    reset = (clockBeatCount >= divisor);
}
```

This is byte-for-byte the body of `forge::shouldReset()`. The Phase 23 roadmap explicitly documents that `shouldReset()` is the designated hook for replacing the divisor calculation with a `BEATS_PER_ALIGN[15]` table. If Phase 23 patches `shouldReset()` without simultaneously patching this inline copy in `ClockTracker`, the beat-aligned reset will fix the x1.5/div1.5 alignment for the `LfoCore` telemetry path but leave the actual clock FSM reset firing on the old (wrong) cadence — a silent behavioral split between `ClockTracker` and `shouldReset`.

**Fix:** Replace the inline logic with a call to the existing function:

```cpp
// Replace lines 168-173 with:
bool reset = forge::shouldReset(currentRatioIdx, clockBeatCount);
```

The call is valid because `currentRatioIdx` is -1 when not clocked-ready, and `shouldReset(-1, ...)` returns `true` per its implementation, which matches the `bool reset = true` default that the old code initialized before the guard.

---

## Warnings

### WR-01: `union`-based type punning in `exp2Floor` is undefined behavior per the C++ standard

**File:** `src/dsp/RackCompat.hpp:108-110`

**Issue:** `exp2Floor` uses a C-style anonymous union to reinterpret `int32_t` bits as `float`:

```cpp
union { float yi; int32_t yii; };
yii = xi << 23;
return yi;     // reads from member that was not last written to
```

Reading from a union member that was not the most recently written member is undefined behavior per the C++ standard (N4860 §9.5, "In a union, at most one of the non-static data members can be active at any time"). GCC and Clang both define this as a supported extension (type-punning via unions is preserved under their ABI guarantees), and the code is copied verbatim from the Rack SDK which is compiled the same way, so it works in every practiced context. However, it is flagged here because:
1. The `make test` target explicitly uses `-Wall -Wextra` — under some GCC versions `-Wstrict-aliasing` will warn.
2. The standard-conforming alternative (`std::memcpy`) compiles to identical code on all relevant targets with optimization enabled.

**Fix:**

```cpp
inline float exp2Floor(float x, float* xf) {
    x += 127.f;
    int32_t xi = (int32_t)x;
    if (xf) *xf = x - (float)xi;
    int32_t bits = xi << 23;
    float yi;
    std::memcpy(&yi, &bits, sizeof(yi));  // defined behavior, same codegen
    return yi;
}
```

---

### WR-02: `actions/checkout@v4` floating tag is a supply-chain risk

**File:** `.github/workflows/test.yml:17`

**Issue:** The CI workflow pins `actions/checkout@v4`, a floating major-version tag. GitHub's official Actions organization could theoretically have a tag mutable event (or the v4 tag could be moved in a fork/mirror scenario). For a private audio-plugin repository, this risk is low but real: a compromised or updated `v4` could exfiltrate secrets or inject build artifacts. The recommended practice for security-sensitive CI is to pin to a specific commit SHA.

```yaml
# Current (mutable):
- uses: actions/checkout@v4

# Hardened (immutable):
- uses: actions/checkout@11bd71901bbe5b1630ceea73d27597364c9af683  # v4.2.2
```

---

### WR-03: `loadF32` in `test_golden.cpp` silently returns an empty vector on file-open failure, producing a misleading `REQUIRE` failure message

**File:** `tests/test_golden.cpp:39-44`

**Issue:** `loadF32` opens the golden `.f32` file with `std::ifstream` but does not check whether `open()` succeeded before reading:

```cpp
std::vector<float> loadF32(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::vector<float> v;
    float x;
    while (f.read(reinterpret_cast<char*>(&x), sizeof x)) v.push_back(x);
    return v;
}
```

If the file does not exist (e.g., the golden `.f32` files are missing from the checkout, or the test binary is run from a directory other than the repo root), `f.read()` silently returns nothing, the returned vector has size 0, and `REQUIRE(ref.size() == (size_t)GOLDEN_SAMPLES)` fails with a size-mismatch message that does not mention the missing file or its path. This makes CI failure diagnosis unnecessarily difficult.

**Fix:**

```cpp
std::vector<float> loadF32(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        throw std::runtime_error("loadF32: cannot open golden file: " + path);
    }
    std::vector<float> v;
    float x;
    while (f.read(reinterpret_cast<char*>(&x), sizeof x)) v.push_back(x);
    return v;
}
```

---

### WR-04: `interpolateBuffer` negative-index guard (`if (i0 < 0) i0 = 0`) is dead code — a negative `i0` is structurally impossible

**File:** `src/AnalogLFO.cpp:441`

**Issue:**

```cpp
float interpolateBuffer(const std::array<float, 256>& buffer, float phase) const {
    float idx = phase * 256.f;
    int i0 = (int)idx;
    int i1 = (i0 + 1);
    if (i0 >= 256) i0 = 255;
    if (i1 >= 256) i1 = 0;
    if (i0 < 0) i0 = 0;   // dead: phase is always in [0,1) at all call sites
    ...
}
```

`phase` at every call site (`drawPhaseDot`) is either `module->displayPhase` (which comes from `tel.displayPhase = p` where `p` is wrapped to `[0, 1)` in LfoCore) or `trailPhase` (which explicitly adds 1.f when negative). So `idx = phase * 256.f` is always in `[0, 256)`, and `i0 = (int)idx` is always in `[0, 255]`. The `< 0` guard is therefore unreachable dead code, and its presence implies the caller may pass out-of-range phases — which could mislead future maintainers. The guard should either be removed (with a comment explaining the invariant) or upgraded to an assertion.

**Fix:**

```cpp
// phase guaranteed in [0, 1) from LfoCore telemetry and explicit +1 wrap in comet trail
float idx = phase * 256.f;
int i0 = (int)idx;                 // always in [0, 255]
int i1 = (i0 + 1) % 256;          // wraps 255 -> 0 for boundary interpolation
float frac = idx - (float)i0;
return buffer[i0] + frac * (buffer[i1] - buffer[i0]);
```

---

## Info

### IN-01: `static constexpr` namespace-scope arrays in `RatioTable.hpp` and `Swing.hpp` carry a redundant `static` keyword in C++17

**File:** `src/dsp/RatioTable.hpp:20,39` and `src/dsp/Swing.hpp:18`

**Issue:** Namespace-scope `constexpr` variables have internal linkage in C++17 (implicitly `static`). The explicit `static` is therefore redundant but harmless — no ODR violation, no additional copies. However, the test target compiles as C++17 while the plugin itself may compile under an earlier standard. In C++11/14, `static` on a `constexpr` namespace-scope variable is actually load-bearing (it guarantees internal linkage when the header is included in multiple translation units). Since the project straddles both standards (plugin = older Rack toolchain; tests = C++17), the `static` is correctly defensive and should be kept. This is flagged as informational only.

---

### IN-02: `sqrtSampleTime` lazy initialization in `LfoCore` has no reset path

**File:** `src/dsp/LfoCore.hpp:192`

**Issue:**

```cpp
if (sqrtSampleTime == 0.f) sqrtSampleTime = std::sqrt(sampleTime);  // lazy init
```

`sqrtSampleTime` is initialized once on the first call to `step()` and never re-computed. This mirrors the inline code exactly, so it is not a regression. However, there is no `reset()` method on `LfoCore`, so if a consumer changes the sample rate mid-run (e.g., by constructing a `BlockDriver` at one rate and then passing `Inputs` with a different `sampleTime`), `sqrtSampleTime` will be stale, causing all subsequent OU noise terms to be scaled by the wrong square root. In the `BlockDriver` this cannot happen (it overwrites `sampleTime` from `1/sampleRate` every call), but it is a latent trap for any future consumer of `LfoCore` that changes `sampleTime` dynamically.

**Fix:** Document the constraint clearly, or compute `sqrtSampleTime` per-step (the cost is one `sqrtf` per sample — negligible):

```cpp
// Per-step: remove lazy init; always compute
float sqrtDt = std::sqrt(sampleTime);
DriftEngine::Result dr = drift.step(sampleTime, drift_, driftAmount, isClocked, sqrtDt);
```

---

### IN-03: `test_dsp_stateful.cpp` DriftEngine RNG-draw-order test uses a hand-rolled mirror that shares spread seed but seeds the mirror's `ouWeightSpread` from it — the mirror is not truly independent of the engine under test

**File:** `tests/test_dsp_stateful.cpp:163-196`

**Issue:** The "RNG draw order" test creates both an `eng` and a `mirror` DriftEngine, seeds both with the same spread seed (`0x9E3779B9ULL, 0x7F4A7C15ULL`), and then manually re-implements one step of the `eng` math in the `mirror` to compare against `eng.step()`. The intent is to verify that `eng.step()` consumes exactly 6 RNG draws in the documented order (4 OU, 1 jitter, 1 DC). However, the test does not verify that the `refRng` advances exactly 6 draws per `eng.step()` call — it manually consumes 6 draws from `refRng` and checks that the final state matches. This correctly tests draw count but does not catch a scenario where draws are consumed in an incorrect order that still produces the right final output value by coincidence. The test is useful as a regression guard but would be more precise if it interleaved the comparison per-draw rather than checking only aggregate results.

This is informational: the test provides meaningful assurance and does not contain a correctness error. A stronger formulation (comparing each draw's contribution individually) would be more explicit.

---

_Reviewed: 2026-06-14_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
