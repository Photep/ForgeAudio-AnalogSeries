# Phase 29: VCO Test Harness & LFO Non-Regression Guardrail - Pattern Map

**Mapped:** 2026-07-28
**Files analyzed:** 10 (8 NEW, 2 MODIFIED)
**Analogs found:** 9 / 10

> **GUARDRAIL (read first):** The Analog LFO is shipped and live in the VCV Library.
> Every pattern below is **copy-from**, never **edit-in-place**. The following files are
> **UNCHANGED — hard requirement**: `tests/BlockDriver.hpp`, `tests/test_golden.cpp`,
> all 11 `src/dsp/*.hpp`, `src/AnalogLFO.cpp`, `src/plugin.{cpp,hpp}`, `plugin.json`,
> `tests/golden/*.f32`, and `Makefile`'s `TEST_CXXFLAGS`. Where a pattern would normally
> imply refactoring a shared file, the additive alternative is called out inline.

---

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|---|---|---|---|---|
| `src/dsp/VcoCore.hpp` | model (POD seam / DSP core) | transform (POD-in → float-out) | `src/dsp/LfoCore.hpp` | **exact** |
| `tests/VcoBlockDriver.hpp` | test harness (driver) | batch / block streaming | `tests/BlockDriver.hpp` | **exact** |
| `tests/test_vco_harness.cpp` | test | batch | `tests/test_invariants.cpp` | **exact** |
| `tests/test_lfo_guardrail.cpp` | test | file-I/O + transform (hash) | `tests/test_golden.cpp` (structure only — do NOT edit it) | role-match |
| `tests/Sha256.hpp` | utility (test-scope) | transform (bytes → digest) | `src/dsp/RackCompat.hpp` (vendored-algorithm header shape) | partial |
| `tests/golden/SHA256SUMS`, `src/dsp/FROZEN.sha256` | config (data manifest) | file-I/O | `tests/golden/freerun_seeds.txt` (provenance-file precedent) | partial |
| `tests/check_includes.sh` | middleware (static gate) | batch / static analysis | `tests/check_docs.sh` | **exact** |
| canary TU (`src/vco_compile_canary.cpp`) | config (compile/link gate TU) | — (no runtime data flow) | *none in repo* | **no analog** |
| `.github/workflows/test.yml` | config (CI) | event-driven | itself (existing `toolchain-gate` steps) | **exact** |
| `Makefile` | config (build) | — | itself (`test` / `capture` / `strict` targets) | **exact** |

**Project-wide conventions observed in every analog (apply to all new files):**
- **Indentation is TAB** in `.cpp` / `.hpp` / `.sh`. Every file read uses hard tabs.
- Every header opens `#pragma once` then a `// <relative/path>` comment line, then a banner
  block explaining purpose, the source it was lifted from, and preserved landmines.
- Every `src/dsp/*.hpp` banner ends with the literal line
  `// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes.`
- Every test TU banner ends with
  `// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).`
- Namespace is `forge`, closed with `} // namespace forge`.

---

## Pattern Assignments

### `src/dsp/VcoCore.hpp` (model / POD seam, transform)

**Analog:** `src/dsp/LfoCore.hpp` — **exact**. Copy the shape; write no DSP (D-01).

**Header banner + include pattern** (`src/dsp/LfoCore.hpp:1-36`) — note the hygiene line and the
comment-annotated sibling includes; **only `dsp/*.hpp` siblings + standard headers**:
```cpp
#pragma once
// src/dsp/LfoCore.hpp
//
// Driveable LFO orchestrator (D-02): takes a POD forge::Inputs, returns the
// output voltage, ...
//
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes.

#include <cmath>
#include <cstdint>
#include <algorithm>

#include "dsp/MathConst.hpp"   // forge::kPi (D-06, rack-free pi constant)
#include "dsp/RackCompat.hpp"    // forge::OnePole, forge::exp2_taylor5, forge::clamp

namespace forge {
```

**POD `Inputs` pattern** (`src/dsp/LfoCore.hpp:38-56`) — plain `struct`, NSDMI on every field, no
constructor, per-field trailing comment naming the shell source, `sampleTime` last with the
injection comment:
```cpp
// POD core boundary (RESEARCH.md L186-204). Each field maps to its params[]/
// inputs[]/ProcessArgs source in the shell; the core never sees Rack indices.
struct Inputs {
	float rate = 0.7f;          // free-run Hz (params[RATE_PARAM].getValue())
	float morph = 0.f;          // post-CV, post-clamp [0,1]
	float character = 0.f;      // post-CV, post-clamp [0,1] (caller adds characterSpread)
	float drift = 0.f;          // post-CV, post-clamp [0,1]
	float fmCV = 0.f;           // volts; 0 if unpatched
	float fmAtten = 0.f;
	bool  fmConnected = false;
	float sampleTime = 1.f / 44100.f;  // INJECTED, never read from a global
};
```
> **Name-collision rule (R-9):** `forge::Inputs` is TAKEN by the LFO. The VCO POD **must** be
> `forge::VcoInputs`. A second `forge::Inputs` in another header is a cross-TU ODR violation
> that compiles silently in TUs that include only one.

**Core struct + Telemetry + seeding pattern** (`src/dsp/LfoCore.hpp:58-118`) — nested `Telemetry`
struct with NSDMIs then a `Telemetry tel;` member; `seed`/`setSpreadSeed` with `s1 = 0` defaults;
`float step(const Inputs& in)`:
```cpp
struct LfoCore {
	// --- orchestration state ... ---
	double phase = 0.0;
	DriftEngine drift;

	// --- Last-step telemetry (shell reads these to feed display atomics; NOT part
	//     of the audio path). Populated by step() each sample. ---
	struct Telemetry {
		int   clockState = 0;
		float displayPhase = 0.f;    // phase + offset, wrapped (dot position)
	};
	Telemetry tel;

	LfoCore() {
		freqSlew.setLambda(20.f);
		freqSlew.out = 0.7f;
	}

	void seed(uint64_t s0, uint64_t s1 = 0) { drift.seed(s0, s1); }
	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) {
		drift.setSpreadSeed(s0, s1);
		...
	}

	float step(const Inputs& in) {
		float sampleTime = in.sampleTime;
		...
	}
};
} // namespace forge
```
> **P29 delta:** `step()` returns silence. Still wire `seed()`/`setSpreadSeed()` now so
> `VcoBlockDriver`'s ctor discipline is real from day one (RESEARCH §Xoroshiro).

**C++11-safety pattern for any constant table** (`src/dsp/Swing.hpp:17-25`) — namespace-scope
`static constexpr`, **never** in-class `static constexpr` that is runtime-indexed (that is the
exact ODR class that got v2.0.0 rejected):
```cpp
namespace forge {
// AnalogLFO.cpp:68-75 — swing presets (PHASE-03).
static constexpr float SWING_FRACTIONS[6] = {
	0.50f,   // Straight
	...
};
```
And the "no C++17 inline variable" rationale, verbatim from `src/dsp/MathConst.hpp:12-14`:
```cpp
// Plain constexpr (internal linkage per TU), NOT `inline constexpr`: inline variables are
// C++17 and the Rack toolchain builds with -std=c++11.
constexpr double kPi = 3.14159265358979323846;
```

---

### `tests/VcoBlockDriver.hpp` (test harness / driver, batch)

**Analog:** `tests/BlockDriver.hpp` — **exact**. **COPY THE FILE, DO NOT TEMPLATE IT.**

> **R-2 / P-4 — the highest-probability accidental LFO regression in this phase.** Refactoring
> `BlockDriver` and `VcoBlockDriver` into a shared template, or touching `BlockDriver`'s ctor
> defaults or `run()` loop, changes what the macOS bit-exact drift-ON leg feeds `LfoCore` and
> moves `freerun_*.f32`. **Additive alternative:** accept ~40 duplicated lines in an independent
> file, and add `tests/BlockDriver.hpp` to the D-05 hash manifest so any future edit to it is
> mechanically surfaced.

**Full pattern to mirror** (`tests/BlockDriver.hpp:1-55`) — banner with the seeding landmine,
single project include, struct-not-class, ctor with 5 defaulted non-zero seeds, `run()` that
**unconditionally overwrites** `sampleTime`:
```cpp
#pragma once
// tests/BlockDriver.hpp
//
// Headless block-driver harness over forge::LfoCore (TEST-04). ...
// Links NOTHING outside the Rack-free core (zero rack/ includes).
//
// Seeding note (Pitfall 4 / freerun_seeds.txt landmine): LfoCore::seed(s0,s1)
// seeds ONLY the drift RNG; component-spread coefficients stay zero until
// setSpreadSeed(sp0,sp1) is called. Both default to non-zero values here because
// forge::Xoroshiro128Plus seeded (0,0) is a degenerate fixed point that emits an
// all-zero stream, which makes std::normal_distribution loop forever. ...

#include "dsp/LfoCore.hpp"

#include <vector>
#include <functional>
#include <cmath>
#include <cstdint>

namespace forge {

struct BlockDriver {
	forge::LfoCore core;
	double sampleRate = 44100.0;

	// Default seeds are non-zero (never the degenerate (0,0) Xoroshiro fixed point).
	explicit BlockDriver(double sr = 44100.0,
	                     uint64_t s0 = 0x1234ULL, uint64_t s1 = 0x5678ULL,
	                     uint64_t sp0 = 0x9E3779B9ULL, uint64_t sp1 = 0x7F4A7C15ULL)
		: sampleRate(sr) {
		core.seed(s0, s1);
		core.setSpreadSeed(sp0, sp1);
	}

	// Drive nSamples through the core. inputAt(i) supplies the per-sample Inputs;
	// sampleTime is always overwritten to 1/sampleRate (the harness owns timing).
	std::vector<float> run(int nSamples, const std::function<forge::Inputs(int)>& inputAt) {
		std::vector<float> out;
		out.reserve(nSamples);
		const float dt = (float)(1.0 / sampleRate);
		for (int i = 0; i < nSamples; ++i) {
			forge::Inputs in = inputAt(i);
			in.sampleTime = dt;
			out.push_back(core.step(in));
		}
		return out;
	}
};

} // namespace forge
```
**Retarget deltas only:** `LfoCore`→`VcoCore`, `forge::Inputs`→`forge::VcoInputs`, add
`in.sampleRate = (float)sampleRate;` next to the `sampleTime` injection, drop
`clockedScenario()` (LFO-specific; `<cmath>` becomes optional). Keep the seed values
`0x1234/0x5678` + `0x9E3779B9/0x7F4A7C15` **verbatim** — proven non-degenerate.

---

### `tests/test_vco_harness.cpp` (test, batch)

**Analog:** `tests/test_invariants.cpp` — **exact**. This is the parametrized-over-3-rates suite.

**TU banner + include + anonymous-namespace fixture pattern** (`tests/test_invariants.cpp:1-40`):
```cpp
// tests/test_invariants.cpp
//
// TEST-04 behavioral invariant suite over the extracted forge::LfoCore, driven
// headless through tests/BlockDriver.hpp. Each invariant is parametrized over the
// three production sample rates {44100, 48000, 96000} Hz, ...
//
// Invariants (RESEARCH.md L580-590 test map; tolerances L597-606):
//   1. ±5V output bounds      — ...
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "BlockDriver.hpp"

#include <vector>
#include <cmath>
#include <cstdint>

namespace {

// The three production sample rates every invariant is parametrized over.
constexpr double SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0};

// Canonical free-run scenario (matches the golden seeds.txt, drift ON).
forge::Inputs freeRunBase() {
	forge::Inputs in;
	in.rate        = 2.0f;
	in.morph       = 0.4f;
	...
	return in;
}

} // namespace
```
> Note `#include "doctest.h"` and `#include "BlockDriver.hpp"` are **quoted, bare filenames**
> (resolved by `-Itests`); project headers use the `dsp/` prefix (resolved by `-Isrc`).
> Note the fixture builder uses `forge::Inputs in;` + field assignment — **never** brace-init
> (`VcoInputs in{...}` is a C++11 error because NSDMIs disqualify aggregate status; P-8).

**Determinism test-case pattern** (`tests/test_invariants.cpp:156-172`) — the `CAPTURE(sr)` +
`REQUIRE` size + manual bit-exact loop shape to mirror for the seam-determinism invariant:
```cpp
TEST_CASE("invariant: fixed-seed determinism (same seed -> bit-identical)") {
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const int n = (int)std::lround(sr * 2.0);
		forge::Inputs base = freeRunBase();

		forge::BlockDriver a(sr, 0xC0FFEEULL, 0xBADF00DULL);
		forge::BlockDriver b(sr, 0xC0FFEEULL, 0xBADF00DULL);
		auto oa = a.run(n, [&](int) { return base; });
		auto ob = b.run(n, [&](int) { return base; });
		REQUIRE(oa.size() == ob.size());
		bool identical = true;
		for (size_t i = 0; i < oa.size(); ++i) {
			if (oa[i] != ob[i]) { identical = false; break; }  // bit-exact
		}
		CHECK(identical);
	}
}
```
> **Anti-vacuity (P-7):** drive with a *varying* `inputAt(i)` sweep (the lambda above returns a
> constant `base`; the `clockedScenario` at `tests/BlockDriver.hpp:59-71` is the in-repo pattern
> for a varying functor) so these assertions become meaningful the instant P30 DSP lands.

---

### `tests/test_lfo_guardrail.cpp` (test, file-I/O + hash transform)

**Analog:** `tests/test_golden.cpp` — **structure only**. This new TU exists precisely so
`test_golden.cpp` stays **byte-unchanged** (R-3) and can itself be hash-pinned.

**Binary fixture reader to copy** (`tests/test_golden.cpp:41-49`) — the hash lock reads the same
files; read raw bytes rather than floats, but mirror the `std::ios::binary` + relative-path shape:
```cpp
// Raw little-endian float32 reader (RESEARCH.md L477-482). One file per rate,
// 8192 samples (32768 bytes); format documented in freerun_seeds.txt.
std::vector<float> loadF32(const std::string& path) {
	std::ifstream f(path, std::ios::binary);
	std::vector<float> v;
	float x;
	while (f.read(reinterpret_cast<char*>(&x), sizeof x)) v.push_back(x);
	return v;
}
```
> **Relative-path contract:** fixture paths are literal `"tests/golden/freerun_44100.f32"` —
> the binary MUST run from the repo root. `make test` does (`./$(TEST_BIN)`), and the Windows
> CI leg runs `./test.exe` from the checkout root.

**Constant block + TEST_CASE naming pattern** (`tests/test_golden.cpp:51-56, 137-147`) — pinned
values as `constexpr` in an anonymous namespace; one `TEST_CASE` per fixture with a
`"<prefix>: <description>"` name so `-tc="golden*"` filters work:
```cpp
// Exact capture parameters from tests/golden/freerun_seeds.txt.
constexpr uint64_t DRIFT_S0  = 0x0000000000C0FFEEULL;
constexpr int      GOLDEN_SAMPLES = 8192;

TEST_CASE("golden: drift-off freerun replay matches reference @ 44.1k") {
	replayGoldenDriftOff(44100.0, "tests/golden/freerun_44100_driftoff.f32");
}
```
Use prefix `"lfo guardrail: ..."` so `-tc="lfo guardrail*"` selects the new tripwires.

**Bit-exact comparator rationale to reuse verbatim** (`tests/test_golden.cpp:125-130`) — the
in-source justification for plain `==` over `doctest::Approx`:
```cpp
	// Bit-exact replay on the canonical OS. Use a direct float == (NOT
	// doctest::Approx, whose epsilon(0) still applies a relative-scaling margin
	// and is not a true bit-exact comparator).
	for (size_t i = 0; i < ref.size(); ++i) {
		CHECK(got[i] == ref[i]);
	}
```

**Platform-gating pattern** if any guard must be OS-specific (`tests/test_golden.cpp:112, 132`):
```cpp
#if defined(__APPLE__)
...
#endif // __APPLE__
```
> D-04 expected digests go in as **string literals in this TU** (not a data file), so changing a
> golden requires a code diff. Emit `tests/golden/SHA256SUMS` alongside as a human-runnable
> convenience only.

---

### `tests/Sha256.hpp` (utility, test-scope)

**Analog:** no exact match. Closest shape is `src/dsp/RackCompat.hpp` — the repo's precedent for a
vendored algorithm in a header, whose banner names its upstream source verbatim:
```
// Source: VERBATIM from ../Rack-SDK/include/random.hpp:26-70.
// Bit-identical to rack::random::Xoroshiro128Plus.
```
Mirror that provenance-banner discipline. **Placement rule:** `tests/` only, never `src/` — a
hash used as an integrity tripwire must not enter the shipped C++11 build graph. Write it
C++11-clean anyway (no `std::clamp`, no `inline constexpr`) so a future move is cheap.
**Negative control:** unit-test against the NIST vector `SHA-256("abc") = ba7816bf...f20015ad`.

---

### `tests/check_includes.sh` (middleware / static gate, batch)

**Analog:** `tests/check_docs.sh` — **exact**. Copy the whole scaffold.

**Full script skeleton** (`tests/check_docs.sh:1-38, 90-101`) — shebang, purpose-banner listing the
enforced rules, `set -euo pipefail`, script-relative `ROOT` resolution, `fail`/`note_fail`
accumulator, numbered `[n/N]` sections, ruled summary block, explicit `exit 0` / `exit 1`:
```bash
#!/usr/bin/env bash
#
# check_docs.sh — the single automatable documentation gate for the manual.
#
# Enforces:
#   (1) D-06 / Pitfall 4 — no trademarked synth brand names anywhere in docs/
#   ...
# Returns 0 (PASS) only when all three groups pass. Any denylist hit, missing
# section file, or missing fact token fails the gate with a non-zero exit.

set -euo pipefail

# Resolve repo root relative to this script so it runs from anywhere.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DOCS="${ROOT}/docs"

fail=0
note_fail() { echo "  FAIL: $1"; fail=1; }

# ---------------------------------------------------------------------------
# (1) Brand-name denylist — trademarked synth names must never appear in docs/.
# ---------------------------------------------------------------------------
echo "[1/3] Brand-name denylist (D-06)..."
DENYLIST='Minimoog|Moog|Roland|Juno|SH-101|Prophet|Oberheim|Korg'
if hits=$(grep -rniE "${DENYLIST}" "${DOCS}" 2>/dev/null); then
	note_fail "trademarked brand name(s) found in docs/:"
	echo "${hits}" | sed 's/^/    /'
else
	echo "  OK: zero denylist hits"
fi
...
# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo "--------------------------------------------------"
if [[ "${fail}" -eq 0 ]]; then
	echo "PASS: docs gate clean (denylist + section files + code facts)."
	exit 0
else
	echo "FAIL: docs gate found problems (see above)."
	exit 1
fi
```
Also copy the **array + loop assertion** idiom (`tests/check_docs.sh:44-60`) for enumerating the
LFO-closure files and the required canary includes:
```bash
REQUIRED_FILES=(
	"index.md"
	"engine-concept.md"
)
for f in "${REQUIRED_FILES[@]}"; do
	if [[ -f "${DOCS}/${f}" ]]; then
		echo "  OK: docs/${f}"
	else
		note_fail "missing docs/${f}"
	fi
done
```
> **P-5 — the anti-pattern this analog also demonstrates.** `tests/check_docs.sh` is a complete,
> well-written gate that **nothing invokes** — not the Makefile, not CI (verified by grep). It has
> been inert since Phase 27. Copy its shape; do **not** copy its wiring status. The same task that
> creates `check_includes.sh` must add its `.github/workflows/test.yml` step.

---

### canary TU — `src/vco_compile_canary.cpp` (config / compile+link gate)

**Analog:** **none — no analog found.** No compile-only TU exists in this repo. The closest
*rationale* precedent is the out-of-line-definition block at `src/AnalogLFO.cpp:384-395` (the
shipped fix for the ODR class this canary is meant to catch) — reuse its comment voice.

Construct from RESEARCH §"Compile Canary Design" instead. The load-bearing property (P-1): a bare
`#include` canary **emits no code**, is ODR-used by nothing, and is permanently silently green.
It must call into the headers from an **external-linkage** function with a **runtime-dependent**
argument. Follow `src/dsp/LfoCore.hpp`'s banner convention and state the D-08 growth rule
("EVERY new VCO header must be added to the include list below") in the banner so a future reader
does not "clean it up."

**Placement consequence (from the real build globs, verified):** in `src/` it is picked up free by
all four gates — `SOURCES += $(wildcard src/*.cpp)` (`Makefile:11`), `make strict`'s
`$(wildcard src/*.cpp)` (`Makefile:76`), CI strict `src/*.cpp` (`test.yml:72`), and CI MinGW
`for f in src/*.cpp` (`test.yml:80`). Anywhere else costs 3 wiring edits that can rot exactly the
way `check_docs.sh` did. Cost of `src/`: one unused namespaced symbol ships in the release binary
(R-10 — operator-facing decision).

---

### `.github/workflows/test.yml` (config / CI, event-driven) — **MODIFIED, additive only**

**Analog:** itself. **R-5: ADD steps; do not modify existing ones.** New guard steps go in the
`toolchain-gate` job (ubuntu, LF checkout — dodges the Windows CRLF hazard P-3).

**Existing step shape to match** (`.github/workflows/test.yml:68-88`) — a comment-free `- name:`
with a `run: |` block; note both the strict command and the MinGW loop are **literal duplications**
of the Makefile, and the link line already globs `build-ci/*.o`:
```yaml
      - name: Strict C++11 pedantic gate (our code only)
        run: |
          g++ -std=c++11 -pedantic-errors -fsyntax-only -Wall -Wextra -Wno-unused-parameter \
            -Isrc -isystem /tmp/lin/Rack-SDK/include -isystem /tmp/lin/Rack-SDK/dep/include \
            src/*.cpp

      - name: Install MinGW cross-compiler
        run: sudo apt-get update && sudo apt-get install -y --no-install-recommends g++-mingw-w64-x86-64

      - name: win-x64 leg reproduction (compile + full link vs libRack)
        run: |
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
New steps to append (same shape): `- name: Frozen-header hash guard (D-05)` running
`sha256sum -c src/dsp/FROZEN.sha256`, and `- name: Include / dependency-direction audit (D-06)`
running `bash tests/check_includes.sh`. Both end with an `echo "...: PASS"` line per the house
style above.

**Job/leg preamble comment style** (`.github/workflows/test.yml:41-51`) — long `#` block before a
job explaining *why the gate exists and what masks the failure locally*; mirror this if adding a
job-level comment.

> **`TEST_CXXFLAGS` are duplicated verbatim** at `test.yml:38`
> (`g++ -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off -static ...`).
> R-4: never change these — float results (and both golden legs) move. If the canary or guards
> need flags, introduce separate `CANARY_*` / `GUARD_*` variables.

---

### `Makefile` (config / build) — **MODIFIED only if required**

**Analog:** itself. `test`, `capture`, `strict` are the three additive-target precedents.

**Additive-target pattern** (`Makefile:26-46`) — a ruled banner asserting the target is purely
additive, `TEST_`-namespaced variables, `.PHONY`, and an explicit "links NO libRack" note:
```make
# ---------------------------------------------------------------------------
# Standalone test harness (TEST-01) — purely additive.
# `make`, `make dist`, `make install` are unchanged: plugin.mk defines no `test`
# target and all variables below are TEST_-namespaced. This target links NO
# libRack and uses NO -I$(RACK_DIR)/include (RACK_DIR is irrelevant to `make test`).
# ---------------------------------------------------------------------------
TEST_DIR      := tests
TEST_BIN      := build-test/test
TEST_SOURCES  := $(wildcard $(TEST_DIR)/*.cpp)
TEST_HEADERS  := $(wildcard src/dsp/*.hpp) $(wildcard $(TEST_DIR)/*.hpp)
TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -I$(TEST_DIR) -Wall -Wextra -ffp-contract=off

.PHONY: test
test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SOURCES) $(TEST_HEADERS)
	@mkdir -p build-test
	$(CXX) $(TEST_CXXFLAGS) $(TEST_SOURCES) -o $@
```

**The skip-filter that any new Rack-free target MUST join** (`Makefile:18-24`) — P-6/R-11:
```make
# `make test` is Rack-free (TEST-01 / D-09). A bare `include` hard-fails when
# ../Rack-SDK is absent (e.g. GitHub Actions ubuntu/macos runners), so skip it
# when `test` is the goal ...
ifeq ($(filter test capture,$(MAKECMDGOALS)),)
include $(RACK_DIR)/plugin.mk
endif
```
A `make guards` target that is not added to `$(filter test capture guards,...)` hard-fails on any
runner without `../Rack-SDK`.
> Environment: local `make` is **GNU Make 3.81** — avoid `$(file ...)`, `::=`, `.ONESHELL`.
> **If the canary lands in `src/`, this file needs NO edit at all** (the `$(wildcard src/*.cpp)`
> at lines 11 and 76 already cover it). That is the lowest-risk outcome.

---

## Shared Patterns

### Rack-free include hygiene
**Source:** every `src/dsp/*.hpp` (e.g. `LfoCore.hpp:21`, `Swing.hpp:13`, `MathConst.hpp:7`)
**Apply to:** `src/dsp/VcoCore.hpp`, and asserted by `tests/check_includes.sh`
```cpp
// Include hygiene (Pitfall 1 / TEST-02): ZERO Rack-SDK includes.
```
Enforced by construction: the `test` target passes no `-I$(RACK_DIR)/include` and links no
`-lRack`, so a stray `rack/` include is a hard compile failure.

### Non-degenerate seeding
**Source:** `tests/BlockDriver.hpp:12-17` (banner) + `:35-41` (ctor)
**Apply to:** `tests/VcoBlockDriver.hpp`, the canary TU, every new test fixture
Always pass **both** `s0` and `s1`, both non-zero. `DriftEngine::seed(uint64_t s0, uint64_t s1 = 0)`
defaults `s1` to 0, so `seed(0)` recreates the `(0,0)` Xoroshiro fixed point → all-zero stream →
`std::normal_distribution` infinite loop → Rack hang (P-9). Canonical safe pairs:
`0x1234/0x5678` (drift), `0x9E3779B9/0x7F4A7C15` (spread).

### doctest registration
**Source:** `tests/main.cpp:1-5`
**Apply to:** `tests/test_vco_harness.cpp`, `tests/test_lfo_guardrail.cpp`
```cpp
// tests/main.cpp — the ONLY translation unit that defines the doctest implementation.
// Defining DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN here provides main() and the doctest runtime.
// No other test TU may define this macro (doing so produces duplicate-symbol link errors).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
```
Every other test TU: `#include "doctest.h"` **without** the macro, and closes its banner with
`// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).`
Test-case names are `"<prefix>: <sentence>"` so `-tc="prefix*"` filters (verified:
`./build-test/test -tc="golden*"` → 6 passed / 44 skipped).

### C++11 hard rules for anything reachable from `src/*.cpp`
**Source:** `src/dsp/MathConst.hpp:12-14`, `src/dsp/Swing.hpp:18`, `src/AnalogLFO.cpp:384-395`,
`src/dsp/RackCompat.hpp:97`
**Apply to:** `src/dsp/VcoCore.hpp` and the canary TU (**not** `tests/`, which builds at C++17)
Banned: `std::clamp` (use `forge::clamp`/`forge::clampi`), `inline constexpr` variables,
`if constexpr`, `[[maybe_unused]]`, structured bindings, nested-namespace `a::b {}`, `auto`
return deduction, generic lambdas, `M_PI` (use `forge::kPi`), brace-init of an NSDMI struct.
Tables: namespace-scope `static constexpr`, never runtime-indexed in-class `static constexpr`.

### Guard-must-be-wired
**Source:** the counter-example `tests/check_docs.sh` (referenced by nothing since Phase 27)
**Apply to:** `tests/check_includes.sh`, the D-05 manifest check, the canary
Every guard created in this phase gets an explicit `.github/workflows/test.yml` step **in the same
task that creates it**, plus a verification step that greps the workflow for the guard's filename.

---

## No Analog Found

| File | Role | Data Flow | Reason |
|---|---|---|---|
| canary TU (`src/vco_compile_canary.cpp`) | config (compile/link gate) | — | No compile-only / ODR-probe TU exists in the repo. Build from RESEARCH §"Compile Canary Design"; borrow only the banner voice from `src/AnalogLFO.cpp:384-395` and the header conventions from `src/dsp/LfoCore.hpp`. |
| `tests/Sha256.hpp` (partial) | utility | transform | No hashing code exists anywhere in the repo. Only the *vendored-algorithm header* shape (provenance banner) from `src/dsp/RackCompat.hpp` transfers. |

---

## Metadata

**Analog search scope:** `src/`, `src/dsp/`, `tests/`, `tools/`, `.github/workflows/`, `Makefile`
**Files read this session:** `tests/BlockDriver.hpp`, `tests/check_docs.sh`,
`.github/workflows/test.yml`, `src/dsp/LfoCore.hpp` (1-130), `tests/test_golden.cpp`,
`tests/main.cpp`, `Makefile`, `src/dsp/MathConst.hpp`, `tests/test_invariants.cpp` (1-45, 140-200),
`src/dsp/Swing.hpp` (1-25)
**Source edits made:** none (read-only)
**Pattern extraction date:** 2026-07-28
