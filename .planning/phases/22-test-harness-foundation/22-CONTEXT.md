# Phase 22: Test Harness Foundation - Context

**Gathered:** 2026-06-14
**Status:** Ready for planning

<domain>
## Phase Boundary

Stand up the **safety net** that every later v1.4 phase depends on:

1. A standalone `make test` doctest binary that builds and runs **without touching or breaking the existing plugin build**.
2. A Rack-independent header-only DSP core under `src/dsp/*.hpp` (zero `rack/` includes) consumed by the plugin shell — the plugin still builds and loads unchanged.
3. A headless `tests/BlockDriver.hpp` harness that runs the core over sample blocks and asserts invariants: ±5V output bounds, frequency accuracy, phase continuity at reset, fixed-seed determinism.
4. Invariant tests pass at 44.1 / 48 / 96 kHz, establishing the behavioral baseline for later refactors.

**Architecture is LOCKED by research** (`.planning/research/ARCHITECTURE.md`, HIGH confidence) and is NOT a gray area:
- Extract-the-core, header-only `src/dsp/*.hpp`, zero `rack/` includes — **do NOT link `libRack`** into the test binary.
- `BlockDriver` over the core **is** the "integration harness" — do NOT boot Rack / instantiate `rack::Module` / touch `APP->` globals.
- `make test` is purely additive; `make` / `make dist` / `make install` behavior unchanged; `RACK_DIR` irrelevant to the test target.
- POD `forge::Inputs` struct at the core boundary; `sampleTime` and RNG are injected (constructor/step params), never read from globals.

**NOT in this phase:** the functional bug fixes (#1–#4 → Phase 23), the display/thread cleanups and shell-thinning (→ Phase 24), VCV packaging (→ Phase 26).

</domain>

<decisions>
## Implementation Decisions

### Test Framework
- **D-01:** Use **doctest** (single drop-in `doctest.h` header, zero deps, fastest compile, readable failure output + auto test discovery). Matches the project's "no external deps / single-header" lean. Catch2 and hand-rolled asserts were rejected — Catch2 adds a submodule/vcpkg dependency; hand-rolled gives no discovery/reporting.

### Extraction Scope (Phase 22 / Phase 24 boundary — DELIBERATELY SHIFTED)
- **D-02:** Extract a **driveable `LfoCore` orchestrator** plus its dependencies into `src/dsp/*.hpp`: phase accumulator, `Waveshape` (sine/tri/saw/square/pulse/morph/bleed), `ClockTracker`, `RatioTable`, `Swing`, **and `DriftEngine`**. The core takes a POD `forge::Inputs` and returns an output voltage.
- **D-03:** **DriftEngine is pulled FORWARD from Phase 24 into Phase 22** (user decision). Rationale: the fixed-seed determinism invariant (Success Criterion 3) must exercise **real stochastic drift output**, not a placeholder. This is an intentional expansion beyond the roadmap's "minimal core" note for Phase 22.
- **D-04 (CROSS-PHASE CONSEQUENCE — planner/roadmap must heed):** Because the full DSP core extraction now lands in Phase 22, **Phase 24 must be re-scoped**. Phase 24's remaining work becomes: display/thread cleanups (CLEAN-01..05), `process()` shell-thinning to a ~20-line pass-through, and the `ouLayers[0].state` → explicit `bleedLfo` parameter lift — **not** core extraction (already done here). TEST-02 ("full extraction") is effectively satisfied in Phase 22; confirm and update the requirement-ownership table.
- **D-05:** The `computeMorphedWave` bleed dependency on `ouLayers[0].state` must be lifted to an explicit `float bleedLfo` parameter during extraction (research extraction-checklist flag — easy to miss).
- **D-06:** Re-implement the small Rack DSP types the core needs as `forge::` equivalents in `src/dsp/RackCompat.hpp`: `SchmittTrigger` (hysteresis), `Timer` (accumulator), one-pole filter / pulse generator as needed; use `std::clamp` for `rack::math::clamp`.

### RNG / Determinism
- **D-07:** **Re-implement Xoroshiro128+** (~15 lines) in the core as `forge::Xoroshiro128Plus`, used by **both** the module and the tests. Drift output stays **bit-identical to the shipped plugin**, so the determinism test guards real behavior and survives the Phase 24 shell-thinning. Templating `DriftEngine<Rng>` + injecting `std::mt19937_64` was rejected — the tested drift stream would diverge from the shipped one. Keep `std::normal_distribution` (standard). The module may seed from `random_device`, but the core MUST accept an explicit seed.

### Verification (extraction-preserved-behavior gate)
- **D-08:** Prove the extracted core preserved behavior via an **automated golden-output regression**: capture reference output blocks from the CURRENT inline DSP **before** extraction, then assert the extracted core reproduces them within epsilon. This becomes a permanent test and pairs with the bit-identical Xoroshiro (D-07) — exact match expected where drift is the only stochastic term. A manual in-Rack audition is NOT required for this gate (distinct from the Phase 23 x1.5/÷1.5 listening audition).

### CI
- **D-09:** **Wire GitHub Actions CI now** in Phase 22 — cross-platform matrix (ubuntu / macos / windows) running `make test`. The harness is Rack-free, so no `RACK_DIR`, no display/`xvfb`, no SDK fetch. Runs on every push from day one **even though the repo stays private until Phase 28** (private-repo Actions work fine). Phase 26 then only *validates/packages*, it doesn't stand CI up from scratch.

### Claude's Discretion
- Invariant tolerances (frequency-accuracy %, phase-continuity click threshold, drift epsilon), golden-data storage format/location under `tests/`, exact `src/dsp/` file split, and test file organization are implementation details for the researcher/planner to fix.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Architecture & approach (LOCKED — read first)
- `.planning/research/ARCHITECTURE.md` — The decisive design: extract-the-core vs link-Rack verdict, target layering, what-to-extract-concretely, the `BlockDriver` sketch, the `make test` Makefile additions, the invariant→assertion mapping table, CI sketch, and the three phase-plan flags (audition once, RNG strategy, bleed-param lift). **This is the primary spec for the phase.**
- `.planning/research/PITFALLS.md` — Implementation landmines for the harness/extraction.
- `.planning/research/SUMMARY.md` — Research overview.

### Requirements
- `.planning/REQUIREMENTS.md` — TEST-01 (`make test` target), TEST-02 (Rack-independent core; scaffold here, see D-04 re: full extraction now landing in P22), TEST-04 (block-driver invariants). Also the "Linking the test harness against libRack" anti-pattern row.

### Source under test
- `src/AnalogLFO.cpp` (1,641 lines) — single-file module. Key extraction targets by line: waveshaping/`computeMorphedWave` (pure float math + spread members + `ouLayers[0].state` bleed), `processClockInput` (L407), `processResetInput` (L546), ratio table + beat-alignment, swing warp, drift/OU engine, `process()` (L659) orchestration, `dataToJson`/`dataFromJson` (L632/L645).

### Project framing
- `.planning/PROJECT.md` — v1.4 Tempered milestone goal (LFO feature-frozen; release hardening).
- `.planning/ROADMAP.md` §"Phase 22" and §"Phase 24" — phase goals, success criteria, and the dependency chain (P23/P24 depend on P22 green).

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **Waveshaping functions** (`computeSine/Triangle/Saw/Square/Pulse`, `computeMorphedWave`, `progressiveCurve`): already effectively pure `float→float` — near-trivial to lift into `Waveshape.hpp` verbatim (only touch spread members + `ouLayers[0].state`).
- **`RATIO_TABLE[15]`** + division/beat-alignment logic: pure integer math — the highest-value, easiest unit-test target. (`BEATS_PER_ALIGN[15]` is the Phase 23 #2 fix — NOT added here, but RatioTable.hpp should be shaped to receive it.)
- **Clock tracker EMA + outlier rejection + state machine**: algorithmically pure; reads happen via `inputs[CLK_INPUT]` today → become an injected `float clkVoltage`.

### Established Patterns
- Single-file plugin (`src/AnalogLFO.cpp`, `src/plugin.{hpp,cpp}`); no `tests/` or `src/dsp/` dirs exist yet — both created this phase.
- Lock-free double-buffer for audio→display transfer (atomics) stays in the shell; display is GUI-thread and out of DSP-test scope.
- Build workflow: relative `../Rack-SDK` (no worktrees), stale-install flush — see project memory `vcv_build_install_workflow.md`. `make test` must not perturb this.

### Integration Points
- Plugin `process()` will `#include "dsp/LfoCore.hpp"` and delegate — same headers, two consumers (module + tests), one source of truth.
- `../Rack-SDK/libRack.dylib` exists and exports engine symbols, but is deliberately NOT linked by the test target (anti-pattern).

</code_context>

<specifics>
## Specific Ideas

- The user explicitly wants the determinism invariant to test **real drift**, hence the deliberate scope expansion (D-03) and bit-identical Xoroshiro (D-07). Treat "the determinism test must be meaningful, not a stub" as a hard intent.
- "Safety net" framing: the golden-output regression (D-08) is the load-bearing proof that extraction changed nothing — prioritize getting that green before any later phase builds on the core.

</specifics>

<deferred>
## Deferred Ideas

- **Functional bug fixes (#1 clock lockout, #2 x1.5/÷1.5 alignment, #3 free-run swing desync, #4 patch-load guard)** → Phase 23. `RatioTable.hpp` / `ClockTracker.hpp` / `Swing.hpp` should be *shaped* to receive these fixes (e.g. room for `BEATS_PER_ALIGN[]`), but the fixes themselves and their regression tests land in Phase 23.
- **x1.5 / ÷1.5 in-Rack listening audition** → Phase 23 (distinct from the extraction golden-output gate in this phase).
- **Display/thread cleanups, `process()` shell-thinning, dead-code removal** → Phase 24 (now its primary scope, see D-04).
- **TEST-03 unit tests (waveshape ranges, ratio/alignment table, outlier recovery, swing math)** → Phase 23 per the requirement table; though the core extracted here makes them straightforward.

None of these are scope creep — all are roadmap-owned by later phases.

</deferred>

---

*Phase: 22-test-harness-foundation*
*Context gathered: 2026-06-14*
