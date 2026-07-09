# Phase 26: VCV Library Compliance + Packaging - Pattern Map

**Mapped:** 2026-07-09
**Files analyzed:** 12 (3 new, 9 modified/restructured)
**Analogs found:** 11 / 12 (fixture generator has no in-repo tool analog; composed from two existing patterns)

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/dsp/MathConst.hpp` (NEW) | config/constant header | transform (pure constexpr) | `src/dsp/Swing.hpp` | exact (rack-free constexpr-only header) |
| `src/dsp/Waveshape.hpp` (MOD) | utility (DSP math) | transform | self (M_PI→kPi in place) | exact |
| `src/dsp/DriftEngine.hpp` (MOD) | service (stochastic engine) | event-driven/streaming | self (M_PI→kPi in place) | exact |
| `src/dsp/LfoCore.hpp` (MOD) | service (orchestrator) | streaming | self (M_PI→kPi in place) | exact |
| `src/AnalogLFO.cpp` (MOD) | module/controller (Rack shell) | streaming request-response | self (M_PI→kPi in place) | exact |
| `tests/test_extraction.cpp` (MOD) | test | transform | self (M_PI→kPi in place) | exact |
| `tests/test_golden.cpp` (RESTRUCTURE) | test (golden replay) | file-I/O + transform | self (existing CANONICAL_OS split) | exact |
| `tests/golden/freerun_*_driftoff.f32` (NEW) | test fixture (binary data) | file-I/O | `tests/golden/freerun_*.f32` | exact |
| fixture generator TU (NEW, `tools/`) | tool (one-shot capture) | batch file-I/O | `tests/BlockDriver.hpp` + `loadF32` (inverse) | role-match (no existing tool) |
| Makefile generator target (NEW) | build config | — | `Makefile` `test:` target (L40-46) | exact |
| `plugin.json` (MOD) | config/manifest | static | self | exact |
| `tests/golden/freerun_seeds.txt` (MOD) | doc/fixture provenance | static | self | exact |
| `.github/workflows/test.yml` | config/CI | — | self (likely no edit; kPi needs no flag) | exact |

## Pattern Assignments

### `src/dsp/MathConst.hpp` (NEW — config/constant header, transform)

**Analog:** `src/dsp/Swing.hpp` — the closest existing rack-free, constexpr-only,
zero-include-dependency `src/dsp/*.hpp` header. Copy its exact header skeleton.

**Header skeleton to copy** (`src/dsp/Swing.hpp` lines 1-15, 42):
```cpp
#pragma once
// src/dsp/Swing.hpp
//
// <purpose comment block>
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes.

namespace forge {

static constexpr float SWING_FRACTIONS[6] = { ... };   // <- constexpr member pattern

} // namespace forge
```

**Concrete content** (from RESEARCH.md Code Examples, D-06) — NOTE: this header is itself scanned by
plan 26-02 Task 2's `grep -rn M_PI src/ tests/` zero-hit gate, so the comment MUST NOT contain the literal
cmath pi macro token. Refer to the value only via the hex identity `0x400921FB54442D18` and the phrase
"cmath's pi macro". Copy this reworded wording verbatim:
```cpp
#pragma once
// src/dsp/MathConst.hpp
//
// Rack-free shared math constants (D-06). Replaces the non-standard cmath pi
// macro so the direct-g++ Windows CI test leg compiles without -D_USE_MATH_DEFINES.
//
// Include hygiene: ZERO Rack-SDK includes (matches every other src/dsp/*.hpp).

namespace forge {
// Bit-for-bit identical IEEE-754 double to cmath's pi macro (hex identity 0x400921FB54442D18),
// so (float)kPi and (double)kPi equal cmath's pi value exactly. Golden fixtures unperturbed.
inline constexpr double kPi = 3.14159265358979323846;
} // namespace forge
```

**Convention notes:**
- Every `src/dsp/*.hpp` opens `#pragma once` (NOT include guards) then a `// src/dsp/<Name>.hpp` banner. Confirmed on Swing.hpp:1-2, Waveshape.hpp:1-2, DriftEngine.hpp:1-2, LfoCore.hpp:1-2.
- All content lives in `namespace forge { ... }`.
- The "ZERO Rack-SDK includes" hygiene comment appears in every DSP header — include it here too.
- Use `inline constexpr` (C++17) so the header can be included in multiple TUs without ODR violation.
- The comment must NOT write the literal cmath pi macro token — the plan 26-02 zero-hit `M_PI` grep gate scans this header, so an M_PI mention in the comment would false-fail the gate. Reference the value via the hex identity + "cmath's pi macro" only.

---

### M_PI → `forge::kPi` replacement (5 files, 22 occurrences)

All five files apply the SAME mechanical substitution: `(float)M_PI` → `(float)forge::kPi`,
`M_PI` (bare double context) → `forge::kPi`. The `(float)` / `2.f *` / `0.75 *` scaffolding
around each site is UNCHANGED — only the token `M_PI` changes. Each file must `#include "dsp/MathConst.hpp"`.

**`src/dsp/Waveshape.hpp`** (utility, transform) — sites at 40, 72, 75, 92.
Add `#include "dsp/MathConst.hpp"` in the include block (currently `#include <cmath>` / `<algorithm>` at L17-18).
```cpp
// L40 before:  float sine = std::sin(2.f * (float)M_PI * phase);
// L40 after:   float sine = std::sin(2.f * (float)forge::kPi * phase);
```

**`src/dsp/DriftEngine.hpp`** (service, event-driven) — sites at 66, 70, 74, 78, 83.
Add `#include "dsp/MathConst.hpp"` to the include block (L25-27: `<cmath>`/`<cstdint>`/`<random>` + `"dsp/RackCompat.hpp"`).
```cpp
// L66 before:  ouLayers[0].theta = 2.f * (float)M_PI * 0.05f;   // 0.314
// L66 after:   ouLayers[0].theta = 2.f * (float)forge::kPi * 0.05f;   // 0.314
```
NOTE: this file is ALSO the D-07 spread-path landmine source (`std::normal_distribution` at L46 + L98) — do NOT touch that; the kPi edit is orthogonal.

**`src/dsp/LfoCore.hpp`** (service, streaming) — site at 229.
Add `#include "dsp/MathConst.hpp"` to the include block (L22-24: `<cmath>`/`<cstdint>`/`<algorithm>`).
```cpp
// L229 before:  float mix = 0.5f - 0.5f * std::cos((float)M_PI * crossfadeProgress);
// L229 after:   float mix = 0.5f - 0.5f * std::cos((float)forge::kPi * crossfadeProgress);
```

**`src/AnalogLFO.cpp`** (module/controller, streaming) — sites at 421, 422 (×3), 425, 426 (×2), 981, 1043, 1044, 1059, 1060, 1074, 1075.
Already includes `"dsp/LfoCore.hpp"` (L2) which will transitively pull MathConst.hpp once LfoCore includes it — but add an explicit `#include "dsp/MathConst.hpp"` for clarity (include block at L1-10).
Two site shapes appear:
```cpp
// float-cast scaling (L421-426, 981):
//   2.f * (float)M_PI  ->  2.f * (float)forge::kPi
// bare-double knob angle (L1043-1075):
//   minAngle = -0.75 * M_PI;  ->  minAngle = -0.75 * forge::kPi;
```

**`tests/test_extraction.cpp`** (test, transform) — site at 164.
Add `#include "dsp/MathConst.hpp"` (test TU already resolves `dsp/` via `-Isrc`; include block starts with `"doctest.h"` at L24).
```cpp
// L164 before:  float mix = 0.5f - 0.5f * std::cos((float)M_PI * crossfadeProgress);
// L164 after:   float mix = 0.5f - 0.5f * std::cos((float)forge::kPi * crossfadeProgress);
```

**Verification excerpt** (from CONTEXT D-06 numeric-safety claim): after replacement,
`grep -rn M_PI src/ tests/` must return zero hits — this includes the new MathConst.hpp, whose comment is
reworded to avoid the literal token so the zero-hit invariant is absolute — and `make test` on macOS must
remain bit-exact against the existing `freerun_*.f32` (kPi rounds to the same double as the cmath pi macro).

---

### `tests/test_golden.cpp` (RESTRUCTURE — test golden replay, file-I/O + transform)

**Analog:** itself — the existing file ALREADY implements the exact pattern the restructure
generalizes: a `#if defined(__APPLE__)` canonical-OS bit-exact vs. epsilon split.

**Existing split to preserve** (lines 68-74):
```cpp
#if defined(__APPLE__)
constexpr bool   CANONICAL_OS  = true;
constexpr double GOLDEN_EPSILON = 0.0;       // bit-exact on the canonical capture OS
#else
constexpr bool   CANONICAL_OS  = false;
constexpr double GOLDEN_EPSILON = 1e-5;      // drift-on tolerance off the canonical OS
#endif
```

**Existing comparator dispatch to reuse** (lines 85-97):
```cpp
if (CANONICAL_OS) {
    for (size_t i = 0; i < ref.size(); ++i) CHECK(got[i] == ref[i]);      // bit-exact
} else {
    for (size_t i = 0; i < ref.size(); ++i)
        CHECK(std::fabs(got[i] - ref[i]) <= GOLDEN_EPSILON);              // abs epsilon
}
```

**Existing binary reader to reuse verbatim** (lines 39-45):
```cpp
std::vector<float> loadF32(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::vector<float> v; float x;
    while (f.read(reinterpret_cast<char*>(&x), sizeof x)) v.push_back(x);
    return v;
}
```

**Restructure per D-07 (what changes, NOT how to compare):**
1. Add a `replayGoldenDriftOff(sr, path)` that constructs a driver with drift OFF AND
   the spread path neutralized (do NOT call `setSpreadSeed` — see Shared Pattern "Spread
   neutralization"), then compares against `freerun_<sr>_driftoff.f32` with a small libm
   epsilon (~1e-6, exact value Claude's discretion per CONTEXT L100) on ALL OSes.
2. Gate the existing drift-ON replay (`replayGolden`, lines 76-98 + TEST_CASEs 102-112)
   to `#if defined(__APPLE__) ... #endif` so it runs macOS-only (canonical). Off-canonical,
   the drift-off replay is the cross-platform regression guard.
3. Keep `goldenBase()` (lines 55-64) for the drift-on macOS case; add a drift-off variant
   with `in.drift = 0.0f`.

**Do NOT:** widen the drift-on epsilon to cover `normal_distribution` divergence
(CONTEXT D-07 / RESEARCH Anti-Patterns) — the drift-on path becomes macOS-gated instead.

---

### `tests/golden/freerun_{44100,48000,96000}_driftoff.f32` (NEW — test fixture, file-I/O)

**Analog:** existing `tests/golden/freerun_{44100,48000,96000}.f32` (32768 bytes each).

**Format contract** (from `freerun_seeds.txt` L10-11 + `loadF32`):
raw little-endian float32, one file per sample rate, `GOLDEN_SAMPLES = 8192` samples =
32768 bytes, git-tracked (NOT gitignored). New drift-off fixtures MUST match this format
exactly so the reused `loadF32` reader works unchanged.

**Generation scenario** (CONTEXT D-07 + RESEARCH Open Q1 recommendation): identical params
to the drift-on golden EXCEPT `drift = 0.0f` and NO `setSpreadSeed` call. Captured on macOS
(canonical OS), then committed.

---

### Fixture generator TU (NEW — tool, batch file-I/O) — house in `tools/`

**No existing in-repo tool analog** — `freerun_seeds.txt:39` references a
`build-test/capture` one-shot generator that is NOT committed (RESEARCH Pitfall 5). Compose
the generator from two existing patterns:

**Pattern A — drive loop from `tests/BlockDriver.hpp` (lines 45-55):**
```cpp
std::vector<float> run(int nSamples, const std::function<forge::Inputs(int)>& inputAt) {
    std::vector<float> out; out.reserve(nSamples);
    const float dt = (float)(1.0 / sampleRate);
    for (int i = 0; i < nSamples; ++i) {
        forge::Inputs in = inputAt(i);
        in.sampleTime = dt;
        out.push_back(core.step(in));
    }
    return out;
}
```
CRITICAL divergence from BlockDriver: its constructor (lines 35-41) ALWAYS calls
`core.setSpreadSeed(sp0, sp1)` (line 40). The generator must NOT — see Shared Pattern
"Spread neutralization". Either add a no-spread BlockDriver construction mode or construct
`forge::LfoCore` directly and call only `core.seed(s0, s1)`.

**Pattern B — binary f32 writer = inverse of `loadF32` (test_golden.cpp:39-45):**
```cpp
// Write little-endian float32 (mirror of loadF32's f.read):
std::ofstream f(path, std::ios::binary);
for (float x : samples) f.write(reinterpret_cast<const char*>(&x), sizeof x);
```

**Capture params to reuse** (test_golden.cpp:47-64): `DRIFT_S0/S1`, `GOLDEN_SAMPLES=8192`,
`goldenBase()` — but set `in.drift = 0.0f` and skip spread seeding.

---

### Makefile generator target (NEW — build config)

**Analog:** the existing `test:` target (`Makefile` lines 40-46) — the model for a
Rack-free, TEST_-namespaced auxiliary target that compiles with `$(CXX)` and needs nothing
from `plugin.mk`.

**Existing target to mirror** (lines 40-46):
```make
.PHONY: test
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p build-test
	$(CXX) $(TEST_CXXFLAGS) $(TEST_SOURCES) -o $@
```

**Reuse `TEST_CXXFLAGS`** (line 38) — the generator MUST compile with the same
`-ffp-contract=off -std=c++17 -Isrc -Itests` flags so captured fixtures are bit-identical
to what `make test` will replay:
```make
TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -I$(TEST_DIR) -Wall -Wextra -ffp-contract=off
```

**Guard note** (lines 22-24): the new target, like `test`, must NOT require `plugin.mk`.
Add `capture` (or similar) to the `ifeq ($(filter test,...))` skip filter so
`make capture` works without `../Rack-SDK`.

---

### `plugin.json` (MOD — config/manifest, static)

**Analog:** itself. Field-level edits only (PKG-02/03):

**Current state** (lines 5, 9-11):
```json
"minRackVersion": "2.6.0",   <- D-02: lower to "2.0.0"
"authorUrl": "",             <- D-01: "https://github.com/Photep/ForgeAudio-AnalogSeries"
"pluginUrl": "",             <- D-01: same URL
"sourceUrl": "",             <- D-01: same URL
```
Keep unchanged: `slug` (D-04 final), `version` `2.0.0` (D-03), both `tags` (whitelist-valid).
Verify (do NOT edit) Makefile VERSION — it is `jq`-derived in `plugin.mk:6`, no Makefile
VERSION line exists (RESEARCH finding).

---

### `tests/golden/freerun_seeds.txt` (MOD — provenance doc)

**Analog:** itself. Append a drift-off scenario block documenting: `drift=0.0`, "no
`setSpreadSeed` — spread coefficients stay at 0.f defaults", the new filenames
`freerun_<sr>_driftoff.f32`, and the cross-platform epsilon rationale. Mirror the existing
`scenario = ... / drift = ...` block style (lines 13-29).

---

### `.github/workflows/test.yml` (likely NO edit — config/CI)

**Analog:** itself. Per CONTEXT D-06/RESEARCH, the `forge::kPi` constexpr needs NO compiler
flag, so the Windows direct-g++ line (lines 33-35) does not change. Windows goes green because
`M_PI` is gone; Linux goes green because the drift-off fixture replaces the non-portable
drift-on comparison on non-canonical legs. The matrix (lines 11-15) and both run steps stay
as-is. Only touch this file if a step needs the new fixtures on the path (they already resolve
from the checkout root — line 27 comment).

## Shared Patterns

### Rack-free header hygiene
**Source:** `src/dsp/Swing.hpp:13`, `Waveshape.hpp:15-16`, `DriftEngine.hpp:23`, `LfoCore.hpp:21`
**Apply to:** `src/dsp/MathConst.hpp` (new) and all DSP-header edits.
```cpp
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes.
```
Every `src/dsp/*.hpp` opens `#pragma once` + `// src/dsp/<Name>.hpp` banner, wraps content in
`namespace forge { ... }`, and includes NOTHING from `$(RACK_DIR)/include`. MathConst.hpp must
follow this so the Rack-free `make test` / Windows-g++ legs compile it.

### Little-endian float32 fixture I/O
**Source:** reader `tests/test_golden.cpp:39-45`; format spec `freerun_seeds.txt:10-11`
**Apply to:** new drift-off fixtures + the generator's writer (inverse of `loadF32`).
Raw LE float32, 8192 samples, 32768 bytes/file, git-tracked. Writer must be the exact byte
inverse of `loadF32` so `sizeof(float)` framing matches.

### Spread neutralization (D-07 landmine — CRITICAL)
**Source of the trap:** `tests/BlockDriver.hpp:40` (`core.setSpreadSeed(sp0, sp1)` in ctor)
→ `src/dsp/LfoCore.hpp:101-109` → `src/dsp/DriftEngine.hpp:95-98`
(`std::normal_distribution<float> d{0.f,1.f}`).
**Apply to:** the fixture generator AND the drift-off replay in test_golden.cpp.
Turning `drift=0` is NOT sufficient — the spread coefficients (`sawCurvatureSpread`,
`characterSpread`, etc.) are drawn from `std::normal_distribution` (non-portable) and perturb
the waveform even with drift off. The drift-off path MUST skip `setSpreadSeed` entirely so all
`*Spread` fields stay at their `0.f` defaults (DriftEngine.hpp:55-61). Only then is the output
portable (Xoroshiro uniform + libm sin/cos → bit-exact-to-~1e-6 cross-platform).

### Canonical-OS bit-exact / epsilon split
**Source:** `tests/test_golden.cpp:68-74` (macro guard) + 85-97 (comparator dispatch)
**Apply to:** the restructured golden. Reuse the exact `#if defined(__APPLE__)` guard and the
`got[i] == ref[i]` (bit-exact) vs `std::fabs(got[i]-ref[i]) <= EPS` (absolute) dispatch. The
restructure changes WHICH scenario runs on each leg, not the comparator machinery.

### TEST_CXXFLAGS bit-stability flags
**Source:** `Makefile:38` — `-std=c++17 -O2 -g -Isrc -I$(TEST_DIR) -Wall -Wextra -ffp-contract=off`
**Apply to:** the generator's compile target AND (already matched) the Windows CI g++ line
(`test.yml:34`). `-ffp-contract=off` and no `-ffast-math` are load-bearing for cross-build bit
stability — the generator must capture with the same flags the replay uses.

## No Analog Found

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| fixture generator TU | tool | batch file-I/O | No one-shot capture tool is committed (the `build-test/capture` referenced in `freerun_seeds.txt:39` is absent). Not a true "no analog" — compose from `BlockDriver.hpp` run loop (drive) + inverse of `test_golden.cpp` `loadF32` (write) + `Makefile:40-46` target shape. Planner should author it fresh from those three excerpts. |

## Metadata

**Analog search scope:** `src/dsp/`, `src/AnalogLFO.cpp`, `tests/`, `tests/golden/`, `Makefile`, `plugin.json`, `.github/workflows/`
**Files scanned:** 13 read (Swing/DriftEngine/LfoCore/Waveshape headers, AnalogLFO.cpp includes, BlockDriver, test_golden, test_extraction, main.cpp, Makefile, plugin.json, test.yml, freerun_seeds.txt) + full `grep M_PI` (22 sites across 5 files)
**Pattern extraction date:** 2026-07-09
