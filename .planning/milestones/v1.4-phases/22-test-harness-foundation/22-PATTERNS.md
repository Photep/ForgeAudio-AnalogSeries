# Phase 22: Test Harness Foundation - Pattern Map

**Mapped:** 2026-06-14
**Files analyzed:** 18 (16 new, 2 modified)
**Analogs found:** 9 inline-DSP analogs + 6 SDK-primitive analogs (quoted verbatim in RESEARCH.md) / 18 total

> **This is an EXTRACTION phase, not greenfield.** For most new `src/dsp/*.hpp` headers the "closest existing analog" is the corresponding inline code already living in `src/AnalogLFO.cpp` (1,641 lines) — the new header lifts that code almost verbatim. For `RackCompat.hpp` the analogs are Rack SDK primitives the inline code consumes; RESEARCH.md already quotes the re-implementations verbatim (lines 293-396) — copy from there, do not re-derive. For brand-new files with no analog (vendored `doctest.h`, `BlockDriver.hpp`, CI workflow, golden fixtures) there is no codebase analog — use the RESEARCH.md sketch as the template (cited per file).

---

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/dsp/Waveshape.hpp` | DSP core (pure fns) | transform | `AnalogLFO.cpp` L194-388 (`progressiveCurve`, `computeSine/Triangle/Saw/Square/Pulse`, `computeMorphedWave`) | exact (lift verbatim) |
| `src/dsp/RatioTable.hpp` | DSP core (data + logic) | transform | `AnalogLFO.cpp` L42-65 (`RATIO_TABLE`/`RATIO_LABELS`) + L503-533 (beat-align/`shouldReset`) | exact (lift verbatim) |
| `src/dsp/ClockTracker.hpp` | DSP core (FSM) | event-driven | `AnalogLFO.cpp` L407-544 (`processClockInput`) | exact (lift; inject `clkVoltage`) |
| `src/dsp/Swing.hpp` | DSP core (pure fn) | transform | `AnalogLFO.cpp` L67-75 (`SWING_FRACTIONS`) + L763-773 (swing warp) | exact (lift verbatim) |
| `src/dsp/DriftEngine.hpp` | DSP core (stochastic) | event-driven (per-sample RNG) | `AnalogLFO.cpp` L94-105 (`OULayer`) + L198-216 (`initComponentSpread`) + L606-629 (OU init) + L726-761 (OU/jitter/DC step) | exact (lift verbatim) |
| `src/dsp/LfoCore.hpp` | DSP core (orchestrator) | request-response (`Inputs`→`float`) | `AnalogLFO.cpp` L659-852 (`process()`) | exact (orchestration mirror) |
| `src/dsp/RackCompat.hpp` | utility (SDK shims) | n/a | Rack SDK primitives — quoted verbatim in RESEARCH.md L293-396 | exact (copy from RESEARCH) |
| `tests/doctest.h` | test framework (vendored) | n/a | **no analog** — vendor single header, RESEARCH.md L87-95 | new |
| `tests/main.cpp` | test bootstrap (1 TU) | n/a | **no analog** — RESEARCH.md L463-469 | new |
| `tests/BlockDriver.hpp` | test harness | request-response (block loop) | **no analog** — RESEARCH.md L422-461 | new |
| `tests/test_invariants.cpp` | test (invariants) | batch assert | **no analog** — RESEARCH.md Validation table L580-590 | new |
| `tests/test_golden.cpp` | test (regression) | batch assert | **no analog** — RESEARCH.md L471-493 | new |
| `tests/test_waveshape.cpp` | test (scaffold) | batch assert | **no analog** — RESEARCH.md L590 (full ranges → P23) | new |
| `tests/golden/*.f32` (+ `.txt`) | fixtures | file-I/O | **no analog** — captured from inline DSP per D-08, RESEARCH.md L471-493 | new |
| `.github/workflows/test.yml` | config (CI) | n/a | **no analog** — RESEARCH.md L495-522 | new |
| `Makefile` (MODIFIED) | config (build) | n/a | `Makefile` L1-14 (existing) + RESEARCH.md L398-420 (additive block) | extend (append below `include`) |
| `.gitignore` (MODIFIED) | config | n/a | existing `.gitignore` (build artifacts section) | extend (add `build-test/`) |
| `src/AnalogLFO.cpp` (MODIFIED) | Rack shell | request-response | itself — delete inline DSP, `#include "dsp/LfoCore.hpp"`, delegate | extract-in-place |

---

## Pattern Assignments

### `src/dsp/Waveshape.hpp` (DSP core, transform)

**Analog:** `src/AnalogLFO.cpp` L194-388 — already pure `float→float`; the easiest, highest-confidence lift.

**Lift verbatim:**
- `progressiveCurve` (L194-196), `computeSine` (L222-232), `computeTriangle` (L234-260), `computeSaw` (L262-278), `computeSquare` (L280-301), `computePulse` (L303-329), `computeMorphedWave` (L331-388).

**Spread-member dependency** — these functions read instance members `triAsymmetrySpread`, `sawCurvatureSpread`, `squareDutySpread`, `pulseEdgeSpread`, `bleedSpread` (L240, L268, L286, L316, L365). Carry these into a `Waveshape` struct (or a passed `Spread` POD) — they are produced by `DriftEngine`/spread init, not the wave math.

**THE bleed lift (D-05 / Pitfall 3) — load-bearing.** `computeMorphedWave` at L369 reads `ouLayers[0].state` directly:
```cpp
// AnalogLFO.cpp:369 — the ONE Rack-state coupling in the otherwise-pure wave math
bleedIntensity *= (1.f + ouLayers[0].state * 0.2f);
```
This MUST become an explicit parameter. New signature:
```cpp
inline float morphedWave(float phase, float morph, float character, float bleedLfo /*= ouLayers[0].state*/);
// and line 369 becomes:  bleedIntensity *= (1.f + bleedLfo * 0.2f);
```
Per Pitfall 3 (RESEARCH.md L262-266): `LfoCore::step` must pass the **post-drift-update** `ouLayers[0].state` for that sample. When `drift < 0.001f` the inline drift block is skipped and `ouLayers[0].state` retains its last value — the core must pass that retained value (do NOT zero `bleedLfo` at low drift). Golden tests at drift=0 AND drift>0 catch a mistake.

**ODR rule (Pitfall 4):** every free function here must be `inline` (or a struct member, implicitly inline).

---

### `src/dsp/RatioTable.hpp` (DSP core, transform)

**Analog:** `src/AnalogLFO.cpp` L42-65 (tables) + L503-533 (beat-alignment / reset decision).

**Lift verbatim — the tables:**
```cpp
// AnalogLFO.cpp:43-59 — pure integer/float constants, the easiest unit-test target
static constexpr float RATIO_TABLE[15] = { 1.f/16.f, 1.f/8.f, /* ... */ 8.f, 16.f };
static constexpr const char* RATIO_LABELS[15] = { "/16", /* ... */ "x16" };
```

**Lift — the reset decision** (L520-524):
```cpp
// AnalogLFO.cpp:520-524 — division-aware reset; becomes RatioTable::shouldReset(ratioIdx, beatCount)
bool shouldReset = true;
if (currentRatioIdx >= 0 && RATIO_TABLE[currentRatioIdx] < 1.f) {
    int divisor = (int)std::round(1.f / RATIO_TABLE[currentRatioIdx]);
    shouldReset = (clockBeatCount >= divisor);
}
```

**Shape for P23 (do NOT implement the fix here).** Per CONTEXT D-04/deferred: leave room for `BEATS_PER_ALIGN[15]` (the #2 x1.5/÷1.5 alignment fix lands in Phase 23). Structure `shouldReset` so a per-ratio beat-count table can replace the `1.f/RATIO_TABLE[...]` divisor math without an API change.

---

### `src/dsp/ClockTracker.hpp` (DSP core, event-driven FSM)

**Analog:** `src/AnalogLFO.cpp` L407-544 (`processClockInput`) — algorithmically pure; today reads `inputs[CLK_INPUT]`.

**Inject the voltage** — replace the two Rack reads with injected params:
```cpp
// AnalogLFO.cpp:408  →  bool clkConnected (param)
bool clkConnected = inputs[CLK_INPUT].isConnected();
// AnalogLFO.cpp:446  →  float clkV (param)
float clkVoltage = inputs[CLK_INPUT].getVoltage();
```
New entry: `ClockTracker::step(float clkV, float dt, bool connected) -> {state, smoothedPeriod, edge}`.

**Lift verbatim — the FSM body** (carries `ClockState{FREE,ACQUIRING,LOCKED}` from L40):
- Disconnect revert (L411-425), timeout (L432-443), edge detection via `clockTrigger.process(clkV, 0.1f, 1.0f)` (L447), EMA smoothing `smoothedPeriod += 0.3f*(raw - smoothedPeriod)` (L488-493), outlier rejection (L466-472), ACQUIRING→LOCKED after 4 edges (L496-501), fast-track re-acquisition (L477-486).

**Shell-owned reads that must become inputs/outputs, not stay in the tracker:**
- L509-512 reads `paramQuantities[RATE_PARAM]->getScaledValue()` for the ratio index → inject `ratioIdx` (computed by the shell / `RatioTable`).
- `crossfadeFrom`/`crossfadeProgress`/`phase` writes (L456-458, L528-532) and `resetBlanking.trigger` (L460, L532) → these belong to `LfoCore` orchestration; `ClockTracker` should signal "reset wanted this edge", `LfoCore` performs the phase reset + crossfade capture.
- `displayClockState`/`displayClockEdge` atomics (L421, L441, L484, L541) → **stay in the shell** (display bridge, out of DSP-test scope).

**Uses from `RackCompat.hpp`:** `forge::SchmittTrigger` (replaces `dsp::SchmittTrigger clockTrigger`, L121), `forge::Timer` (replaces `dsp::Timer clockTimer`, L122).

---

### `src/dsp/Swing.hpp` (DSP core, transform)

**Analog:** `src/AnalogLFO.cpp` L67-75 (`SWING_FRACTIONS[6]`) + L763-773 (warp).

**Lift verbatim:**
```cpp
// AnalogLFO.cpp:68-75
static constexpr float SWING_FRACTIONS[6] = {0.50f,0.54f,0.58f,0.66f,0.71f,0.75f};

// AnalogLFO.cpp:767-773 — becomes swingPhaseMultiplier(phase, swingFrac, isClocked)
float swingFrac = SWING_FRACTIONS[swingIndex];
if (isClocked && swingFrac > 0.5001f) {
    double swingMul = (phase < 0.5) ? (0.5 / (double)swingFrac)
                                    : (0.5 / (1.0 - (double)swingFrac));
    deltaPhase *= swingMul;
}
```
The `isClocked` gate (PHASE-04: swing inactive in free-run) and the `> 0.5001f` straight-mode fast path are behavioral — preserve both. (P23 owns the #3 free-run swing-desync fix; shape the API to receive it but do not change behavior here.)

---

### `src/dsp/DriftEngine.hpp` (DSP core, stochastic / per-sample RNG)

**Analog:** `src/AnalogLFO.cpp` L94-105 (`OULayer` + member decls) + L198-216 (`initComponentSpread`) + L606-629 (OU layer init) + L726-761 (per-sample OU/jitter/DC step).

**Lift verbatim — the OU struct + layer constants:**
```cpp
// AnalogLFO.cpp:94-99
struct OULayer { float state = 0.f; float theta; float sigma; float weight; };
// + the 4-layer + dcOffsetOU theta/sigma/weight init at L606-629 (carry these exact constants)
```

**Lift verbatim — the per-sample OU/jitter/DC step** (L731-761) — this is the determinism-critical block:
```cpp
// AnalogLFO.cpp:733-738 — 4-layer OU; RNG and sqrtDt INJECTED, never global
for (int i = 0; i < NUM_OU_LAYERS; i++) {
    float noise = normalDist(rng);
    ouLayers[i].state += ouLayers[i].theta * (0.f - ouLayers[i].state) * dt
                       + ouLayers[i].sigma * sqrtSampleTime * noise;
    combinedOU += ouLayers[i].state * (ouLayers[i].weight + ouWeightSpread[i]);
}
// jitter (L744-749) + DC-offset OU (L751-757) follow — preserve the exact RNG draw ORDER
// (4× OU draws, then 1× jitter, then 1× DC) — order is part of the bit-identical stream.
```
**Output `bleedLfo`:** after the loop, expose `ouLayers[0].state` (the post-update value) as the `bleedLfo` return — this is what `Waveshape::morphedWave` consumes (Pitfall 3).

**Drift-low skip semantics (Pitfall 3):** when `drift < 0.001f` the inline code skips the whole block (L759-761 sets `dcOffsetV = 0` only; OU `state`s are untouched). The core must replicate: do not step OU layers, leave `ouLayers[0].state` retained, return it as `bleedLfo`.

**Lift verbatim — `initComponentSpread`** (L198-216) and **`spreadRng`** seeding (L199-200). Per Open Question 2 (RESEARCH.md L551-552): spread uses a **separate** `Xoroshiro128Plus spreadRng` seeded from `spreadSeed[0/1]` — the core must expose a spread-seed setter so golden capture reproduces spread coefficients exactly.

**RNG (D-07):** uses `forge::Xoroshiro128Plus` from `RackCompat.hpp` (replaces `rack::random::Xoroshiro128Plus`, L101) + `std::normal_distribution<float>{0,1}` (kept standard, L102). The module seeds from `random_device` (L598-599); the core MUST accept an explicit `seed(s0,s1)`.

---

### `src/dsp/LfoCore.hpp` (DSP core, orchestrator — `Inputs`→`float`)

**Analog:** `src/AnalogLFO.cpp` L659-852 (`process()`) — the orchestration mirror.

**`forge::Inputs` POD** — derived from the shell's param/input reads scattered through `process()`. Use the RESEARCH.md L186-204 struct verbatim (it already maps each field to its `params[]`/`inputs[]`/`ProcessArgs` source).

**Orchestration sequence to mirror** (the exact per-sample order is load-bearing for goldens):
1. Clock + reset (L661-662) → `ClockTracker::step` + reset handling.
2. Dual-mode freq: clocked `clockFreq * RATIO_TABLE[ratioIdx]` vs free-run knob Hz (L665-680).
3. `freqSlew.process` (L683) → `forge::OnePole` (mode-transition slew).
4. Drift read + clamp (L687-690).
5. Pitch slew `driftSlew` with **per-sample lazy `setLambda`** (L695-705) — Pitfall 5: the `setLambda(1.f/slewTau)` reconfig must be reproduced inside `LfoCore::step`.
6. FM: `freq *= dsp::exp2_taylor5(fmPitch)` (L715) → `forge::exp2_taylor5` (Pitfall 2 — bit-identity).
7. `deltaPhase = freq * sampleTime` (L724, double precision).
8. `DriftEngine::step` modifies `deltaPhase`, produces `dcOffsetV` + `bleedLfo` (L727-761).
9. `Swing` warp (L768-773).
10. `phase += deltaPhase; wrap` (L779-780).
11. Morph/character CV+clamp (L782-792) — note `characterSpread` added at L792.
12. Phase offset at readout (L813-824).
13. `computeMorphedWave(p, morph, character, bleedLfo)` (L825) → `Waveshape::morphedWave`.
14. `5.f * sample` + **anti-click crossfade** (L831-842) + `dcOffsetV` after crossfade (L848).

**Stays in the SHELL `AnalogLFO.cpp`** (do NOT pull into core): `paramQuantities[...]->getScaledValue()` (L509, L671), all `displayX.store(...)` atomics, `updateDisplayBuffer` (L805), `dataToJson`/`dataFromJson` (L632-657, jansson dep), and the `params[]`/`inputs[]`/`outputs[]` array I/O.

---

### `src/dsp/RackCompat.hpp` (utility — SDK primitive shims)

**Analog:** Rack SDK primitives the inline DSP consumes. **RESEARCH.md already quotes the verbatim re-implementations — copy from there, do not re-derive from the SDK.**

| `forge::` type | RESEARCH.md lines | Replaces (in `AnalogLFO.cpp`) |
|----------------|-------------------|-------------------------------|
| `Xoroshiro128Plus` | L293-319 | `rack::random::Xoroshiro128Plus` (L101, L199) |
| `SchmittTrigger` | L327-340 | `dsp::SchmittTrigger` (L121, L166) — UNINITIALIZED handling matters |
| `Timer` | L342-348 | `dsp::Timer` (L122) |
| `PulseGenerator` | L350-357 | `dsp::PulseGenerator resetBlanking` (L167) — keeps longer of existing/new on `trigger()` |
| `OnePole` | L359-370 | `dsp::TExponentialFilter<float> freqSlew/driftSlew` (L140-141) — snap-to-input is load-bearing (Pitfall 5) |
| `clamp` | L372 | `rack::math::clamp` (L511, L673, L690, L786, L792, L820, L656) → `std::clamp` |
| `exp2_taylor5` + `exp2Floor` | L374-393 | `dsp::exp2_taylor5` (L715) — bit-identity for FM (Pitfall 2) |

**Include hygiene (Pitfall 1):** this header (and every `src/dsp/*.hpp`) includes ONLY `<cmath> <cstdint> <array> <random> <algorithm>` — never `rack.hpp`, `plugin.hpp`, or `rack/*`.

---

### `tests/BlockDriver.hpp` (test harness — NO ANALOG)

**No codebase analog — new file.** Template: RESEARCH.md L422-461. Drives `forge::LfoCore` over `nSamples`, injects `sampleTime = 1.0/sampleRate` per sample, seeds via `core.seed(s0,s1)`. Includes a `clockedScenario` helper (square-wave clock at BPM into `clkVoltage`). Links NOTHING.

### `tests/main.cpp` (doctest bootstrap — NO ANALOG)

**No analog.** Template: RESEARCH.md L463-469. The **ONLY** TU that defines `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` (Pattern 3 / anti-pattern: defining it in >1 TU → duplicate-symbol link error).

### `tests/doctest.h` (vendored framework — NO ANALOG)

**No analog.** Vendor the single header from the official `doctest/doctest` release tag (RESEARCH.md L87-106). Pin the version in a top-of-file comment + add a provenance note (source URL + version) where committed. doctest 2.4.x.

### `tests/test_invariants.cpp` (invariants — NO ANALOG)

**No analog.** Template: RESEARCH.md Validation table L580-590 + candidate tolerances L597-606. Asserts: ±5V bounds (`[-5.12, +5.12]` with drift, strict ±5 without), frequency accuracy (±1%), phase continuity at reset (`|Δout| ≤ 0.5V` across the 3ms crossfade), fixed-seed determinism (bit-exact same-platform), parametrized over {44100, 48000, 96000}.

### `tests/test_golden.cpp` (regression — NO ANALOG)

**No analog.** Template: RESEARCH.md L471-493. Loads `tests/golden/*.f32`, replays `BlockDriver` with the SAME seed used at capture, asserts bit-exact (`epsilon(0)`) on the canonical OS / `1e-5` elsewhere for drift-on (Pitfall 6 + Open Q1). **D-08 capture ordering is critical** (RESEARCH.md L493): extract core → keep inline copy → assert core==inline on identical inputs (extraction-correctness gate) → write core output as golden → DELETE inline copy. Never ship both definitions live (Pitfall 4).

### `tests/test_waveshape.cpp` (scaffold — NO ANALOG)

**No analog.** Scaffold only (full ranges → P23). RESEARCH.md L590: morph×character sweep stays in [−1,1] pre-scale; shape identity at segment boundaries.

### `tests/golden/*.f32` + sidecar `.txt` (fixtures — NO ANALOG)

**No analog.** Captured from the inline DSP per D-08 (RESEARCH.md L471-493). Raw little-endian float32, one file per (scenario × rate). Tracked in git; sidecar `.txt` documents seed + scenario params.

### `.github/workflows/test.yml` (CI — NO ANALOG)

**No analog.** Template: RESEARCH.md L495-522. ubuntu/macos/windows matrix → `make test` (unix) / direct `g++` (windows lacks GNU make; MSVC float codegen differs — prefer the same compiler family as the canonical golden-capture OS, RESEARCH.md L522). No `RACK_DIR`, no SDK fetch, no xvfb.

---

### `Makefile` (MODIFIED — extend, do not rewrite)

**Analog:** the existing 14-line `Makefile` (read this session) + the additive block in RESEARCH.md L398-420.

**Append BELOW line 14** (`include $(RACK_DIR)/plugin.mk`). The block is non-perturbing: `plugin.mk` defines no `test` target; new vars are `TEST_`-namespaced. Key flags: `-std=c++17 -O2 -g -Isrc -I$(TEST_DIR) -ffp-contract=off`, **NO** `-I$(RACK_DIR)/include`, **NO** `-ffast-math` (cross-platform bit stability, Pitfall 6). `make`/`make dist`/`make install` behavior unchanged; `RACK_DIR` irrelevant to `test`.

### `.gitignore` (MODIFIED — extend)

**Analog:** existing `.gitignore` (has a `build/ dep/ dist/ *.o *.d` artifacts section). Add `build-test/` (new ephemeral test build dir). Keep `tests/golden/*.f32` TRACKED (do not ignore — they are the regression baseline, RESEARCH.md L244).

### `src/AnalogLFO.cpp` (MODIFIED — extract-in-place)

**Analog:** itself. After capturing goldens, `#include "dsp/LfoCore.hpp"`, delegate `process()` to `core.step(in)`, and **delete the inline DSP copies in the same change** (Pitfall 4: never ship a state where both inline and core definitions are live). This phase begins delegation; the ~20-line pass-through shell-thinning is finished in Phase 24 (D-04).

---

## Shared Patterns

### Inject, never read globals (Pattern 2 — applies to ALL `src/dsp/*.hpp`)
**Source:** RESEARCH.md L207-213; contrast `AnalogLFO.cpp` L598-599 (`random_device` seed) and L724 (`args.sampleTime`).
**Apply to:** every core file. `sampleTime` is a field of `forge::Inputs`; the RNG is seeded via explicit `seed(s0,s1)`. No `inputs[...]`, no `params[...]`, no `APP->`, no `random_device` inside `src/dsp/`.

### POD `forge::Inputs` at the core boundary (Pattern 1)
**Source:** RESEARCH.md L180-204.
**Apply to:** `LfoCore.hpp` (defines it), `BlockDriver.hpp` + all `tests/*.cpp` (construct it), `AnalogLFO.cpp` shell (packs it from Rack arrays). The core never sees Rack indices.

### `inline` everything (ODR — Pitfall 4)
**Source:** RESEARCH.md L268-272.
**Apply to:** every free function in `src/dsp/*.hpp` must be `inline` or a struct member. Header-only free functions without `inline` → duplicate-symbol link errors when included in both the plugin and the test TUs.

### Bit-identity discipline (D-07 / D-08 — the load-bearing safety net)
**Source:** RESEARCH.md L222-232 ("Don't Hand-Roll"), L290-396 (verbatim primitive sources).
**Apply to:** `RackCompat.hpp` (re-implement primitives **verbatim, not equivalently**) and `DriftEngine.hpp` (preserve exact RNG draw order). The golden gate is a *bit-exact* check, which only works if every helper reproduces the SDK bit-for-bit. The three landmines: `exp2_taylor5` (Pitfall 2), `OnePole` snap-to-input (Pitfall 5), and the `ouLayers[0].state` bleed lift (Pitfall 3).

### Include hygiene — zero `rack/` (Pitfall 1 / TEST-02)
**Source:** RESEARCH.md L250-254.
**Apply to:** every `src/dsp/*.hpp`. Allowed includes: `<cmath> <cstdint> <array> <random> <algorithm>`. Optional CI guard: `! grep -rE 'rack\.hpp|<rack|APP->' src/dsp/`.

---

## No Analog Found

Files with no codebase analog — planner uses the cited RESEARCH.md sketch as the template:

| File | Role | Data Flow | Template Source |
|------|------|-----------|-----------------|
| `tests/doctest.h` | framework (vendored) | n/a | RESEARCH.md L87-106 (vendor from official release) |
| `tests/main.cpp` | bootstrap | n/a | RESEARCH.md L463-469 |
| `tests/BlockDriver.hpp` | harness | request-response | RESEARCH.md L422-461 |
| `tests/test_invariants.cpp` | test | batch | RESEARCH.md L580-606 |
| `tests/test_golden.cpp` | test | batch | RESEARCH.md L471-493 |
| `tests/test_waveshape.cpp` | test (scaffold) | batch | RESEARCH.md L590 |
| `tests/golden/*.f32` (+`.txt`) | fixtures | file-I/O | RESEARCH.md L471-493 (capture from inline DSP, D-08) |
| `.github/workflows/test.yml` | CI config | n/a | RESEARCH.md L495-522 |

`RackCompat.hpp` is NOT in this table — its analogs (the SDK primitives) are quoted verbatim in RESEARCH.md L293-396 and re-implemented bit-identically; it is a "copy from RESEARCH" file, not a "no analog" file.

---

## Metadata

**Analog search scope:** `src/AnalogLFO.cpp` (1,641 lines, read L1-852 covering every extraction target), `Makefile` (14 lines), `.gitignore`; confirmed `src/dsp/` and `tests/` are absent (all new).
**Files scanned:** 3 source/config (read) + RESEARCH.md/CONTEXT.md (upstream).
**Primary analog:** `src/AnalogLFO.cpp` — 9 inline-DSP extraction targets, line-anchored above.
**Pattern extraction date:** 2026-06-14
