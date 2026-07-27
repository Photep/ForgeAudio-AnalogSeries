# Phase 29: VCO Test Harness & LFO Non-Regression Guardrail - Context

**Gathered:** 2026-07-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Stand up the **safety net for the entire v2.0 VCO milestone** — *before any VCO DSP exists* — and lock in the shipped-LFO guardrail as a standing, always-green canary. This is the VCO-milestone twin of Phase 22.

Delivers:
1. A Rack-free `tests/VcoBlockDriver.hpp` mirroring `tests/BlockDriver.hpp`, driving a `forge::VcoCore` over sample blocks at 44.1 / 48 / 96 kHz with no `libRack` linked and non-degenerate default seeds.
2. A **bare POD seam** `src/dsp/VcoCore.hpp` — the POD `Inputs` → `step()` → output+`Telemetry` boundary only; `step()` returns silence. No DSP. (All VCO DSP is Phase 30+.)
3. The shipped LFO's `.f32` goldens replayed byte-identical in the same `make test` run as a standing non-regression canary (already exists — this phase makes it a *hardened* canary, see below).
4. Additional LFO tripwires so no later phase can **silently** regress the LFO (golden checksum lock + frozen-header hash guard + dependency-direction include audit).
5. A dedicated compile canary that genuinely runs VCO headers through both `make strict` (C++11, `-pedantic-errors`) **and** the CI MinGW compile+link-vs-`libRack` leg — proving the gate that rejected v2.0.0 bites on VCO code.
6. The full test + strict + MinGW canary green in CI on every push.

**Requirements:** TEST-01 (Rack-free VcoCore harness), TEST-04 (LFO golden replay canary), TEST-06 (strict/MinGW covers the VCO TU + headers).

**NOT in this phase (deliberate 29/30 boundary):**
- Any VCO DSP — naive pitch, `exp2_taylor5` V/oct, the 5-shape morph, no-static-state polyphony-ready core → **Phase 30** (CORE-01/CORE-03).
- Rack module registration (`addModel`, `plugin.hpp` extern, `plugin.json` `modules[]`, permanent slug) → **Phase 30** (PANEL-03).
- `MorphBlep.hpp` / anti-aliasing (CORE-02, TEST-03) → **Phase 32**.
- `< 1-cent` V/oct tracking test (TEST-02) → **Phase 31** (tested where pitch is delivered).
- New VCO goldens (TEST-05) → **Phase 36**.

</domain>

<decisions>
## Implementation Decisions

### VcoCore Seam (29/30 boundary)
- **D-01:** **Bare POD seam only.** Phase 29 creates `src/dsp/VcoCore.hpp` as the boundary contract — POD `Inputs` → `step()` → output voltage + `Telemetry` — mirroring `LfoCore.hpp`'s shape, with `step()` returning silence (0 / trivial). **No DSP.** This keeps the 29/30 line crisp: Phase 29 = harness + seam + guardrail; Phase 30 = the naive (aliased-on-purpose) DSP + Rack registration.
- **D-02:** Deliberately **NOT** following Phase 22's D-03 "pull real behavior forward" precedent. There, `DriftEngine` was pulled forward so the determinism test exercised real drift. Here the operator chose the opposite — a hollow core — because the **meaningful behavioral canary for Phase 29 is the shipped-LFO golden replay**, not the (nonexistent) VCO DSP. The VCO `VcoBlockDriver` exists in P29 only to prove the harness *plumbing* (Rack-free, 3 rates, deterministic seam), not DSP behavior.
- **D-03:** The seam **shape** is fixed (POD-in → `step()` → out + telemetry, zero `rack/` includes, siblings-only like `LfoCore.hpp`). The **field set** of the POD `Inputs` is **Claude's discretion** (see below).

### LFO Guardrail Tripwires (all hard-fail; the mission of this phase)
The existing byte-exact (drift-on macOS) / 1e-6 (drift-off, 3-OS) golden replay catches *behavioral* drift only if a change alters replayed output **and** `make test` is re-run. The milestone language is stronger — *"no later phase can **silently** threaten the live LFO."* So add three standing, **hard-fail** tripwires:
- **D-04:** **Golden-file checksum lock.** Record the SHA-256 of every LFO `.f32` golden (`tests/golden/freerun_*.f32`); a test/CI step fails if the bytes ever change. Closes the "silently regenerate the goldens to make a regression green" path — goldens can change only via a deliberate, reviewed hash update.
- **D-05:** **Frozen-header hash guard.** A checked-in SHA-256 manifest of the four frozen shared headers (`src/dsp/Waveshape.hpp`, `RackCompat.hpp`, `MathConst.hpp`, `DriftEngine.hpp`) fails CI if any is edited without bumping the manifest. Mechanically enforces "additive only." **Phase 34 additively edits `DriftEngine.hpp`** — that phase performs a deliberate one-line manifest bump. This is a *feature*: the bump forces the edit to be surfaced to the operator (matches the milestone guardrail protocol).
- **D-06:** **Shared-header include / dependency-direction audit.** A CI grep asserts no VCO-only file (`VcoCore.hpp`, future `MorphBlep.hpp`, `VcoBlockDriver.hpp`, `AnalogVCO.cpp`) is `#include`d by any LFO translation unit, and that VCO code only ever *calls* the frozen headers, never edits them. Guards the dependency direction so VCO work can't leak into the LFO build.

### Strict / MinGW VCO Coverage Proof
- **D-07:** **Dedicated, permanent compile canary.** Because nothing in `src/` includes `VcoCore.hpp` in Phase 29 (`AnalogVCO.cpp` is Phase 30) and both gates only compile headers reached via a `.cpp`, add a dedicated compile-only unit that `#include`s every VCO header (just `VcoCore.hpp` today) and force it through **both**:
  1. the `-std=c++11 -pedantic-errors` syntax gate, and
  2. the CI **MinGW compile + link-vs-`libRack`** leg (mirroring how `plugin.dll` is built) — the *only* thing that catches the in-class-`static constexpr` ODR class that rejected v2.0.0 (`make strict` alone is insufficient, per the milestone guardrail).
- **D-08:** The canary is **permanent and grows**: each later phase that adds a VCO header adds its `#include` to the canary (`MorphBlep.hpp` in P32, etc.). When `AnalogVCO.cpp` lands in P30 it joins the existing `src/*.cpp` glob automatically and is covered identically to how the real TU will be.

### Claude's Discretion
- **POD `Inputs` field set** (D-03): derive from `REQUIREMENTS.md` (PITCH/FM/MORPH/CHARACTER/DRIFT/SYNC/COARSE/FINE) and the `LfoCore` `Inputs` precedent. Operator chose "you decide." *Recommendation:* lean toward declaring the fields needed by the near-term phases (30/31) rather than speculatively modeling P33/P34, while keeping the boundary shape fixed — avoid churning the seam but don't over-speculate. Planner's call.
- **Day-one `VcoBlockDriver` invariant set:** operator chose "you decide." *Recommended default (right-sized for a stub):* (1) runs Rack-free at 44.1/48/96 kHz without `libRack`, (2) `sampleTime` injected as `1/sr` every step, (3) non-degenerate default seeds (never the `(0,0)` Xoroshiro fixed point), (4) two identical runs are bit-identical (seam determinism), (5) output finite / no NaN / no inf. **Defer all semantic DSP asserts** — `< 1-cent` pitch → P31, alias-floor → P32, ±5V-under-morph bounds → P34. Reserved/skipped test skeletons for those are optional, planner's call.
- Exact mechanism/location of the guard steps (doctest cases inside `make test` vs. standalone CI shell steps), hash-manifest file format/location, and canary-TU location (`tests/` vs. a CI-only TU) are implementation details for the researcher/planner.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### v2.0 VCO research (read first — locks approach)
- `.planning/research/ARCHITECTURE.md` — VCO architecture: POD boundary, harness design, invariant mapping, the guardrail encoding.
- `.planning/research/PITFALLS.md` — Implementation landmines (Rack-free include hygiene, `(0,0)` seed fixed point, C++11/ODR traps, portability of `std::normal_distribution`).
- `.planning/research/SUMMARY.md` — Research overview + the unanimous four-agent phase-ordering rationale.
- `.planning/research/STACK.md` — Toolchain/stack constraints.

### Requirements & roadmap
- `.planning/REQUIREMENTS.md` — TEST-01, TEST-04, TEST-06 (this phase); CORE-01/CORE-03 (Phase 30 boundary); TEST-02 (Phase 31); TEST-03/CORE-02 (Phase 32); TEST-05 (Phase 36).
- `.planning/ROADMAP.md` §"Phase 29" + the v2.0 milestone guardrail block — phase goal, 4 success criteria, and the standing-canary mandate.
- `.planning/PROJECT.md` §Constraints — the LFO non-regression guardrail (additive-only, operator-surface protocol, the four frozen shared headers named).

### Existing test/CI infrastructure to MIRROR (Phase 22 / v1.4)
- `tests/BlockDriver.hpp` — the driver to mirror as `VcoBlockDriver.hpp` (non-zero default seeds `0x1234/0x5678` + spread `0x9E3779B9/0x7F4A7C15`; `sampleTime` always overwritten to `1/sr`; `run()` + `clockedScenario()`; 44.1/48/96 kHz).
- `src/dsp/LfoCore.hpp` — the POD `Inputs` → `step()` → `float` + `Telemetry` boundary that `VcoCore.hpp` mirrors (zero `rack/` includes, `dsp/*.hpp` siblings only).
- `tests/test_golden.cpp` — the LFO golden replay to keep green + harden: drift-off portable leg (`LfoCore` direct, seed drift RNG only, `DRIFTOFF_EPSILON = 1e-6`, all OS) and drift-on bit-exact leg (`BlockDriver`, true float `==`, gated `#if defined(__APPLE__)`).
- `tests/golden/freerun_{44100,48000,96000}.f32` (drift-on, macOS) + `freerun_*_driftoff.f32` (portable) + `freerun_seeds.txt` (provenance) — the files the checksum lock (D-04) pins.
- `tools/capture_golden.cpp` — `make capture` pattern (drift-off only, `LfoCore` direct).
- `tests/main.cpp` — sole `DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN` TU; all other `tests/*.cpp` include `doctest.h` without the macro; `make test` globs `tests/*.cpp` into one binary.
- `tests/doctest.h` — vendored doctest 2.4.11.
- `Makefile` — `test` target (`TEST_CXXFLAGS = -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off`; globs `tests/*.cpp` + `src/dsp/*.hpp`); `strict` target (`-std=c++11 -pedantic-errors -fsyntax-only ... src/*.cpp`, SDK headers `-isystem`); `capture` target.
- `.github/workflows/test.yml` — job `test` (3-OS matrix; Windows uses direct `g++ ... -static -static-libgcc -static-libstdc++`) and job `toolchain-gate` (strict C++11 pedantic + the MinGW cross-compile + full link vs `-lRack` producing `plugin.dll` — the ODR-catch). Both jobs auto-pick-up new `tests/*.cpp`, `src/dsp/*.hpp`, and `src/*.cpp`.
- `src/dsp/RackCompat.hpp` — `forge::Xoroshiro128Plus` (bit-identical to Rack; the `(0,0)` fixed-point warning lives here); `src/dsp/DriftEngine.hpp` — 6-draws/step RNG contract; `src/dsp/PatchParse.hpp` — `forge::parseSeedHex`.

### Frozen shared headers the guardrail protects (D-05)
- `src/dsp/Waveshape.hpp`, `src/dsp/RackCompat.hpp`, `src/dsp/MathConst.hpp`, `src/dsp/DriftEngine.hpp` — additive-only; `DriftEngine.hpp` is the sole one Phase 34 will additively touch (with a sanctioned manifest bump).

### Prior-phase context
- `.planning/milestones/v1.4-phases/22-test-harness-foundation/22-CONTEXT.md` — the foundation this phase mirrors (D-01..D-09 there: doctest choice, Rack-free additive `make test`, bit-identical Xoroshiro, golden-output regression as the load-bearing proof).

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`tests/BlockDriver.hpp`** — direct template for `VcoBlockDriver.hpp`; copy the ctor seeding discipline (non-zero defaults, spread seeds), the `sampleTime = 1/sr` injection, and the `run()` loop.
- **`src/dsp/LfoCore.hpp`** — direct template for the `VcoCore.hpp` POD boundary (Inputs/step/Telemetry, zero Rack includes).
- **`tests/test_golden.cpp` + `tools/capture_golden.cpp`** — the golden replay + capture pattern; Phase 29 reuses/extends the LFO replay and adds the checksum lock over the same files.
- **Makefile globs** — `test` already globs `tests/*.cpp` + `src/dsp/*.hpp`; `strict` and the CI MinGW loop already glob `src/*.cpp`. New VCO files are auto-picked-up; **CI needs no structural change** to cover them (the compile canary is the one new artifact needed to force VCO *header* coverage in P29).

### Established Patterns
- Rack-free header-only DSP under `src/dsp/*.hpp` (zero `rack/` includes); `make test` is purely additive and must not perturb `make` / `make dist` / `make install` or the `../Rack-SDK` build workflow.
- Golden policy split: drift-off = portable (1e-6, all OS); drift-on = bit-exact `==`, macOS-gated. `-ffp-contract=off`, no `-ffast-math` in the test build.
- CI two-job shape: `test` (3-OS) + `toolchain-gate` (strict C++11 + MinGW link vs `libRack`).

### Integration Points
- `VcoCore.hpp` and `VcoBlockDriver.hpp` are **new files** — no edits to any LFO file. The compile canary (D-07) is the only artifact that must be threaded into the strict + MinGW CI steps.
- The three tripwires (D-04/05/06) attach to existing assets: golden files, the four frozen headers, and the source tree's include graph.

</code_context>

<specifics>
## Specific Ideas

- Hard intent: the guardrail must fail **loudly and automatically** — the tripwires are hard-fail CI gates, not advisory warnings. A canary that only warns is not a canary. The operator-surface protocol (impact + options + recommendation before proceeding) is the *human* layer on top of the automated hard-fail.
- The `make strict` C++11 gate alone is explicitly **insufficient** — the MinGW **link** leg is mandatory because the v2.0.0-rejecting bug (in-class `static constexpr` ODR) surfaces only at link. The compile canary must exercise both legs.
- The 29/30 boundary is deliberately crisp (hollow seam in 29, all DSP in 30) — a conscious departure from Phase 22's "pull real behavior forward," because here the real behavioral canary is the shipped LFO, not the not-yet-existent VCO.

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope. Everything VCO-behavioral was correctly routed to its owning phase (DSP → P30, pitch/TEST-02 → P31, anti-aliasing/TEST-03 → P32, drift/output → P34, VCO goldens/TEST-05 → P36) rather than pulled into this guardrail phase.

</deferred>

---

*Phase: 29-vco-test-harness-lfo-non-regression-guardrail*
*Context gathered: 2026-07-27*
