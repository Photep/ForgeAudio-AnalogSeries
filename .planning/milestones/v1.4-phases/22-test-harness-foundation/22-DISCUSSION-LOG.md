# Phase 22: Test Harness Foundation - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-06-14
**Phase:** 22-test-harness-foundation
**Areas discussed:** Test framework, Extraction scope, RNG / determinism, Verify, CI

---

## Test framework

| Option | Description | Selected |
|--------|-------------|----------|
| doctest | Single drop-in header, zero deps, fastest compile, readable output + auto discovery. Research's pick. | ✓ |
| Catch2 v3 | Common in VCV land (Surge XT); more features but submodule/vcpkg dependency, heavier compile. | |
| Hand-rolled asserts | Squinky Labs style; zero tree additions but no discovery/reporting. | |

**User's choice:** doctest
**Notes:** Aligns with the project's no-external-deps / single-header lean.

---

## Extraction scope (Phase 22 / Phase 24 boundary)

| Option | Description | Selected |
|--------|-------------|----------|
| Driveable core, no drift | Extract phase accumulator + waveshape + ClockTracker + RatioTable + Swing; LfoCore emits voltage with drift=0; DriftEngine stays in Phase 24; determinism trivial. | |
| Pull DriftEngine forward too | Also extract DriftEngine + RNG now so the fixed-seed determinism invariant tests real stochastic output. Bigger phase; overlaps Phase 24's owned work. | ✓ |
| Isolated units only, no orchestrator | Extract ClockTracker/RatioTable/Waveshape as standalone headers, unit-test each; BlockDriver drives only waveshape+phase. Most minimal. | |

**User's choice:** Pull DriftEngine forward too
**Notes:** Deliberate scope expansion so the determinism invariant exercises real drift output, not a placeholder. Flagged as a cross-phase consequence: Phase 24 must be re-scoped to display/thread cleanups + shell-thinning (core extraction now lands in Phase 22); TEST-02 effectively satisfied here.

---

## RNG / determinism

| Option | Description | Selected |
|--------|-------------|----------|
| Re-impl Xoroshiro128+ in core | Port Xoroshiro128+ into the core, used by module + tests; drift bit-identical to shipped; determinism test guards real behavior and survives Phase 24 refactor. | ✓ |
| Template DriftEngine<Rng>, inject mt19937_64 in tests | Simpler core (no port) but tested drift stream differs from shipped — fine for invariants, not bit-faithful. | |

**User's choice:** Re-impl Xoroshiro128+ in core
**Notes:** Bit-fidelity matters now that DriftEngine is pulled forward and drift output is under determinism test.

---

## Verify (extraction-preserved-behavior gate)

| Option | Description | Selected |
|--------|-------------|----------|
| Golden-output regression | Capture reference output blocks from current inline DSP before extraction; assert extracted core reproduces within epsilon. Automated, permanent, no listening. | ✓ |
| One-time manual in-Rack audition | Load module, eyeball/listen unchanged, delete inline copies. Fast but not automated. | |
| Both — golden + manual sanity | Belt and suspenders. | |

**User's choice:** Golden-output regression
**Notes:** Distinct from the Phase 23 x1.5/÷1.5 listening audition. Pairs with bit-identical Xoroshiro — exact match expected.

---

## CI

| Option | Description | Selected |
|--------|-------------|----------|
| Wire GitHub Actions CI now | Cross-platform matrix (make test) in Phase 22; runs on every push from day one even on private repo; Phase 26 just validates packaging. | ✓ |
| Defer CI to Phase 26 | Keep Phase 22 local-only; wire CI at packaging time. | |

**User's choice:** Wire GitHub Actions CI now
**Notes:** Harness is Rack-free so CI is trivial (no RACK_DIR, no display). Private-repo Actions work fine.

---

## Claude's Discretion

- Invariant tolerances (frequency-accuracy %, phase-continuity click threshold, drift epsilon)
- Golden-data storage format/location under `tests/`
- Exact `src/dsp/` file split and test file organization

## Deferred Ideas

- Functional bug fixes #1–#4 and their regression tests → Phase 23 (headers shaped to receive them here)
- x1.5 / ÷1.5 in-Rack listening audition → Phase 23
- Display/thread cleanups, `process()` shell-thinning, dead-code removal → Phase 24
- TEST-03 unit tests → Phase 23 per requirement table
