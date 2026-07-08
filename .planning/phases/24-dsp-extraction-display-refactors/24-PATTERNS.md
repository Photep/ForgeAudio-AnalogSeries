# Phase 24: DSP Extraction + Display Refactors - Pattern Map

**Mapped:** 2026-06-26
**Files analyzed:** 5 (4 new, 1 modified)
**Analogs found:** 5 / 5 (every new file has an in-repo template; the modified shell has per-site analogs)

> Read this with `24-RESEARCH.md`. RESEARCH supplies the *target* code (seqlock, dt math, fill body); this file supplies the *existing conventions* each new/modified file must mirror (header shape, namespace, ODR rules, doctest TU shape, atomic-publish idiom). Where RESEARCH already wrote the body, the planner's job is to make it match the analog's house style below.

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/dsp/DisplayFill.hpp` (NEW) | utility (pure DSP helper) | transform | `src/dsp/Waveshape.hpp` (+ `Swing.hpp` for shape) | exact (same dir, header-only, `forge::`, pure) |
| `src/dsp/Anim.hpp` (NEW) | utility (pure math helper) | transform | `src/dsp/Swing.hpp` | exact (tiny header, inline free fns + constexpr) |
| `tests/test_display.cpp` (NEW) | test | unit (request-response) | `tests/test_golden.cpp` | exact (doctest TU consuming `src/dsp/*.hpp`) |
| `tests/test_anim.cpp` (NEW) | test | unit (request-response) | `tests/test_smoke.cpp` (+ `test_golden.cpp`) | exact (tiny pure-helper doctest TU) |
| `src/AnalogLFO.cpp` (MODIFIED) | module shell + GUI widget | event-driven / request-response | per-site (atomic publish `:316-330`, double-buffer `:167-187`) | role-match (no seqlock analog exists; reuse its idioms) |

---

## Pattern Assignments

### `src/dsp/DisplayFill.hpp` (NEW — utility, transform)

**Analog:** `src/dsp/Waveshape.hpp` (header shape, ODR rules, comment header) + `src/dsp/Swing.hpp` (the `> 0.5001f` fast-path gate this fill reproduces).

**Header preamble + include hygiene** — every `src/dsp/*.hpp` opens with `#pragma once`, a `// src/dsp/<Name>.hpp` banner, a provenance note citing the `AnalogLFO.cpp` line range lifted, and the ODR/no-Rack rule. Mirror it (`Waveshape.hpp:1-23`):
```cpp
#pragma once
// src/dsp/Waveshape.hpp
// ...
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes. Every free
// function is `inline` or a struct member (ODR, Pitfall 4).

#include <cmath>
#include <algorithm>

#include "dsp/RackCompat.hpp"   // forge::clamp (where the inline code used rack::math::clamp)

namespace forge {
```
- `DisplayFill.hpp` includes `<array>` + `"dsp/Waveshape.hpp"` (for `Waveshape::morphedWave`). NO `<atomic>`, NO Rack includes.
- Free function MUST be `inline`; `DISPLAY_SAMPLES` MUST be `constexpr` (ODR / Pitfall 6 — this header is included by the shell AND `tests/test_display.cpp`).

**Core transform** — lift the loop body **verbatim** from `AnalogLFO::updateDisplayBuffer` (`AnalogLFO.cpp:169-185`), with `bleedLfo` promoted from the live `core.drift.ouLayers[0].state` read (`:184`) to an explicit parameter (mirrors the D-05 lift already done in `Waveshape.hpp:156`). The exact body to preserve (currently at `AnalogLFO.cpp:167-187`):
```cpp
void updateDisplayBuffer(float morph, float character, float swingFrac = 0.5f) {
    int writeIdx = 1 - displayReadIdx.load(std::memory_order_relaxed);
    for (int i = 0; i < DISPLAY_SAMPLES; i++) {
        float t = (float)i / (float)DISPLAY_SAMPLES;  // uniform time
        float p;
        if (swingFrac <= 0.5001f) {
            p = t;  // fast path: no swing
        } else if (t < swingFrac) {
            p = t * 0.5f / swingFrac;                              // even: [0,S) -> [0,0.5)
        } else {
            p = 0.5f + (t - swingFrac) * 0.5f / (1.f - swingFrac); // odd: [S,1) -> [0.5,1)
        }
        displayBuffers[writeIdx][i] =
            core.wave.morphedWave(p, morph, character, core.drift.ouLayers[0].state);
    }
    displayReadIdx.store(writeIdx, std::memory_order_release);
}
```
The pure fn keeps lines 170-178 + the `morphedWave` call exactly; it drops the `writeIdx`/`displayReadIdx` book-keeping (that stays shell-side in `fillFromSnapshot`, see RESEARCH Pattern 2). `bleedLfo` replaces the inline `core.drift.ouLayers[0].state`. Target signature/body is in `24-RESEARCH.md` Pattern 2 (lines 213-235) — make it match the `Waveshape.hpp` house style above.

**Namespace + single-source constant:** declare `constexpr int DISPLAY_SAMPLES = 256;` in `forge::`, and have the shell reference `forge::DISPLAY_SAMPLES` (or alias its existing `static constexpr int DISPLAY_SAMPLES = 256;` at `AnalogLFO.cpp:100` to it) so the `256` lives in one place (Pitfall 6).

---

### `src/dsp/Anim.hpp` (NEW — utility, transform)

**Analog:** `src/dsp/Swing.hpp` — the closest existing header in size and shape: `#pragma once`, banner + provenance comment, `namespace forge`, a `constexpr` table, and a single `inline` free function with the behavioral gate documented inline. Zero Rack includes.

**Shape to mirror** (`Swing.hpp:1-42`, whole file):
```cpp
#pragma once
// src/dsp/Swing.hpp
// ... provenance + "Behavioral gates preserved exactly" + "ZERO Rack-SDK includes."
namespace forge {

static constexpr float SWING_FRACTIONS[6] = { /* ... */ };

inline double swingPhaseMultiplier(double phase, float swingFrac, bool isClocked) {
    if (isClocked && swingFrac > 0.5001f) { /* ... */ }
    return 1.0;
}

} // namespace forge
```
- `Anim.hpp` includes only `<cmath>` (for `std::pow`). `constexpr float kMaxFrameDt`; `inline float clampFrameDt(float)`, `inline float flashDecay(float, float)`.
- Provenance comment MUST cite the `AnalogLFO.cpp` sites being converted (`:388-429`) and note the ANIM-02 `0.92` factor is preserved by mathematical equivalence (D-03), never re-tuned.
- The NAN guard MUST be the explicit `!(raw==raw)` form, NOT `rack::math::clamp` (Pitfall 1 — and the header has zero Rack includes anyway). Target body in `24-RESEARCH.md` Pattern 3 (lines 261-273).

---

### `tests/test_display.cpp` (NEW — test, unit)

**Analog:** `tests/test_golden.cpp` (a non-impl doctest TU that includes a `src/dsp` consumer and asserts numeric output) + `tests/test_smoke.cpp` (minimal non-impl TU shape).

**Mandatory TU rules** (every test TU except `main.cpp`):
- Does **NOT** define `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` — `tests/main.cpp` is the only TU that defines the impl macro (`main.cpp:1-6`; defining it twice = duplicate-symbol link error). State this in the file's top comment, matching `test_golden.cpp:24` and `test_smoke.cpp:2-5`.
- Auto-discovered: `TEST_SOURCES := $(wildcard tests/*.cpp)` (Makefile `:33`) — **no Makefile edit needed** to add this file.

**Include + TEST_CASE shape** (from `test_golden.cpp:26-34` and `test_smoke.cpp:6-10`):
```cpp
#include "doctest.h"
#include "dsp/DisplayFill.hpp"   // -Isrc resolves "dsp/..."
#include "dsp/LfoCore.hpp"
#include <array>

TEST_CASE("display: fill is a pure, thread-independent function of the snapshot") {
    ...
    CHECK(a[i] == b[i]);   // bit-exact within this binary
}
```
- TEST_CASE naming convention is `"<domain>: <behavior>"` — `test_golden.cpp` uses `"golden: ..."`, so use `"display: ..."`. Target cases are in `24-RESEARCH.md` lines 370-394.
- Seed the core with the canonical golden spread seeds (`0x9E3779B9ULL, 0x7F4A7C15ULL`) exactly as `test_golden.cpp:50-51` / `BlockDriver.hpp:37` do, so any spread-dependent shape matches the frozen baseline.
- **Bit-exact-within-binary only** (Pitfall 7): use direct `==` for same-binary determinism (as `test_golden.cpp:89-91` does on the canonical OS). Do NOT bit-compare against the plugin binary. Use `doctest::Approx` for any cross-build/feel check.

---

### `tests/test_anim.cpp` (NEW — test, unit)

**Analog:** `tests/test_smoke.cpp` (tiny pure-function TU) for the skeleton; `test_golden.cpp` for `doctest::Approx` usage.

```cpp
#include "doctest.h"
#include "dsp/Anim.hpp"
#include <cmath>

TEST_CASE("anim: pathological dt clamps to one cap step") {
    CHECK(forge::clampFrameDt(10.0f) == doctest::Approx(1.f/30.f));
    CHECK(forge::clampFrameDt(std::nanf("")) == 0.f);
    ...
}
TEST_CASE("anim: decay is feel-identical at 60fps") {
    CHECK(forge::flashDecay(1.f, 1.f/60.f) == doctest::Approx(0.92f));  // never ==, Pitfall 5
}
```
Same non-impl rule, same auto-glob, `"anim: ..."` naming. Use `doctest::Approx` for the decay equivalence (NEVER `==` — `pow(0.92f,1.f)` differs from `0.92f` by ≤1 ULP, Pitfall 5). Target cases in `24-RESEARCH.md` lines 397-415.

---

### `src/AnalogLFO.cpp` (MODIFIED — module shell + GUI widget)

No existing seqlock in the repo, so the **new** snapshot-publish has no direct analog — copy its body from `24-RESEARCH.md` Pattern 1 (lines 168-207). But every surrounding idiom it must blend into already exists here; mirror these:

**Atomic-publish idiom (audio writes relaxed, GUI reads relaxed/acquire)** — the seqlock + snapshot struct join this existing surface. Source `AnalogLFO.cpp:316-330`:
```cpp
const forge::LfoCore::Telemetry& t = core.tel;
displayClockState.store(t.clockState, std::memory_order_relaxed);
displayRatioIndex.store(t.ratioIdx, std::memory_order_relaxed);
...
displaySwingFraction.store(t.isClocked ? t.swingFrac : 0.5f, std::memory_order_relaxed);
if (t.lockedEdge) displayClockEdge.fetch_add(1, std::memory_order_relaxed);
```
Member-declaration style for the new `displaySnapshotSeq` mirrors the block at `:102-132` (`std::atomic<int> displayClockState{0};` with a `// ANIM-01: ...` purpose comment). `<atomic>`/`<array>` already included (`:5-6`); add `<cstdint>` if needed for `uint32_t`.

**Double-buffer publish (becomes `fillFromSnapshot` body)** — the writer side moves to the GUI thread but the publish stays byte-identical. Source `AnalogLFO.cpp:168/183-186`: `int writeIdx = 1 - displayReadIdx.load(relaxed); ...; displayReadIdx.store(writeIdx, release);`. Keep that exact `release` store (`:186`); the consumer at `:970` reads `displayReadIdx.load(std::memory_order_acquire)` — do not touch it.

**Trigger gate to keep, fill call to replace** — `AnalogLFO.cpp:332-351`. Keep the `phaseWrapped || ((morphChanged||characterChanged||swingChanged) && paramReady)` detection and the `displaySwing = t.isClocked ? t.swingFrac : 0.5f` gate (`:344`); **replace only** the `updateDisplayBuffer(...)` call (`:345`) with the seqlock snapshot publish (capturing `morph`, `character`, `displaySwing`, and `core.drift.ouLayers[0].state` AT this instant — D-02).

**Widget `step()` dt conversion** — `AnalogLFO.cpp:386-431`. Add `float dt = forge::clampFrameDt((float)APP->window->getLastFrameDuration());` at the top, then convert each accumulator per the table in `24-RESEARCH.md` lines 250-256: `breathePhase` (`:388`), `blinkPhase` (`:392`), `scanlineScrollPhase` (`:396`), `fadeSpeed` (`:402`), and `flashIntensity *= 0.92f` (`:429`) → `forge::flashDecay(...)`. The wrap/clamp guards (`:389,393,397`, the `rack::math::clamp` ramps `:410-419`) are amplitude-preserving — leave them.

**Pill fade-out cache (CLEAN-03)** — add `cachedRatioIdx`/`cachedPeriod` widget members near the fade-alpha members (`:366-370`); refresh them in `step()` only while genuinely clocked. Then replace the two early-returns with alpha gates:
- `drawBpmStack` gate `AnalogLFO.cpp:742`: `if (ratioIdx < 0 || period <= 0.f) return;` → drive from cached values + `bpmFadeAlpha` gate.
- Ratio pill gate `AnalogLFO.cpp:891`: `if (ratioFadeAlpha > 0.001f && ratioIdx >= 0)` → `... && cachedRatioIdx >= 0`.
- **No atomic/DSP change** — the audio thread keeps publishing `-1` on disconnect (D-05; "hold the atomics" rejected). Target in `24-RESEARCH.md` Pattern 4 (lines 281-297).

**Dead-code / isStill (CLEAN-01/02)** — mechanical deletes (D-07):
- `drawZeroCrossing` (`AnalogLFO.cpp:529`, definition only, no caller) → delete.
- `scanlineImage` member (`:373`, declared, unused) → delete.
- `isStill` (`:974`): `rate <= 0.001f` is unreachable (`RATE_PARAM` min is `0.01f`, `:194`). Resolve by deleting it → `float dimFactor = module->isBypassed() ? 0.25f : 1.f;` (`:975`).

---

## Shared Patterns

### Header-only `src/dsp/` convention
**Source:** `src/dsp/Swing.hpp`, `src/dsp/Waveshape.hpp`
**Apply to:** `DisplayFill.hpp`, `Anim.hpp`
- `#pragma once` (no include-guard macros) — every `src/dsp/*.hpp` uses it.
- `// src/dsp/<Name>.hpp` banner + provenance comment citing the lifted `AnalogLFO.cpp` line range + a "ZERO Rack-SDK includes" / ODR note.
- Everything inside `namespace forge { ... } // namespace forge`.
- Every free function `inline`; every constant `constexpr` / `static constexpr` (ODR — headers included by the shell AND multiple test TUs; Pitfall 6).
- No `<atomic>`, no `rack/` includes; use `"dsp/RackCompat.hpp"` (`forge::clamp`) only if clamp is needed.

### doctest translation-unit convention
**Source:** `tests/main.cpp` (impl), `tests/test_golden.cpp` + `tests/test_smoke.cpp` (consumers)
**Apply to:** `tests/test_display.cpp`, `tests/test_anim.cpp`
- Non-impl TUs `#include "doctest.h"` only — **never** define `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` (owned solely by `main.cpp:4`).
- `TEST_CASE("<domain>: <behavior>")` naming (`"golden: ..."` → `"display: ..."`, `"anim: ..."`).
- `CHECK` for soft asserts, `REQUIRE` for preconditions; `doctest::Approx(x)` for any float comparison that is not same-binary bit-exact (Pitfall 5/7). Direct `==` only for same-binary determinism.
- `#include "dsp/..."` resolves via `-Isrc`; `"doctest.h"`/`"BlockDriver.hpp"` via `-I tests`.

### Makefile auto-globbing (no edit required)
**Source:** `Makefile:11,33-34,43-45`
**Apply to:** all four new files
- Plugin build: `SOURCES += $(wildcard src/*.cpp)` — globs `.cpp` only, so the two new **headers** need no source-list change.
- Test build: `TEST_SOURCES := $(wildcard tests/*.cpp)` picks up the new `.cpp` TUs; `TEST_HEADERS := $(wildcard src/dsp/*.hpp) ...` picks up the new headers as deps.
- Test target is Rack-free, `-ffp-contract=off`, `-Isrc -I tests` (`:37`). Confirm both `make` and `make test` still build after adding files (headers are include-only).

### Capture-at-trigger / no-live-read discipline (D-02)
**Source:** the D-05 lift already in `src/dsp/Waveshape.hpp:156` (`bleedLfo` is a parameter, not a live read)
**Apply to:** `DisplayFill.hpp` (`bleedLfo` MUST stay a parameter) + the `process()` snapshot publish
- Making `bleedLfo`/`swingFrac` parameters structurally prevents the GUI fill from reading `core.drift.ouLayers[0].state` or recomputing the swing gate at paint time (Pitfalls 3 + the swing anti-pattern, `24-RESEARCH.md:300-301`).

---

## No Analog Found

| Concern | Role | Data Flow | Reason | Mitigation |
|---------|------|-----------|--------|------------|
| Seqlock snapshot publish (`displaySnapshotSeq` + `DisplaySnapshot`) | concurrency primitive | event-driven (SPSC audio→GUI) | No seqlock exists in the repo today; the existing audio→GUI transfer is the 2-slot `displayBuffers`/`displayReadIdx` double-buffer (`AnalogLFO.cpp:101-102`), which is a *related* but not identical pattern | Copy the body from `24-RESEARCH.md` Pattern 1 (lines 168-207); blend it into the existing relaxed-atomic publish idiom (`:316-330`) and member-decl style (`:102-132`) documented above. Memory-ordering rationale is in RESEARCH §Pattern 1. |

---

## Metadata

**Analog search scope:** `src/dsp/*.hpp` (8 headers), `tests/*.cpp` + `tests/*.hpp` (8 files), `src/AnalogLFO.cpp` (1,144 lines — targeted reads at the canonical_refs sites), `Makefile`.
**Files scanned:** ~18
**No CLAUDE.md / `.claude/skills/` present** in the working tree — conventions inferred directly from the source headers above.
**Pattern extraction date:** 2026-06-26
</content>
</invoke>
