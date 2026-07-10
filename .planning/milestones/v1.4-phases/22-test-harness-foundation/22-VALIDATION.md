---
phase: 22
slug: test-harness-foundation
status: planned
nyquist_compliant: true
wave_0_complete: false
created: 2026-06-14
---

# Phase 22 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | doctest (single-header, drop-in `tests/doctest.h`) |
| **Config file** | none — Plan 01 vendors `tests/doctest.h` and appends the `make test` recipe |
| **Quick run command** | `make test` |
| **Full suite command** | `make test` (runs the full doctest binary; multi-rate cases included) |
| **Estimated runtime** | ~5 seconds (header-only build + run) |

---

## Sampling Rate

- **After every task commit:** Run `make test`
- **After every plan wave:** Run `make test` + `make` (plugin build must still succeed unchanged)
- **Before `/gsd:verify-work`:** `make test` green AND `make` (plugin build) still succeeds unchanged
- **Max feedback latency:** ~5 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 22-01-01 | 01 | 1 | TEST-01 | T-22-SC | Vendored doctest pinned to release tag | smoke/build | `make test` | ❌ W0 | ⬜ pending |
| 22-01-02 | 01 | 1 | TEST-01 | T-22-01 | Additive test target, no libRack/RACK_DIR | smoke/build | `make test && make` | ❌ W0 | ⬜ pending |
| 22-01-03 | 01 | 1 | TEST-01 | — | N/A | unit | `make test` | ❌ W0 | ⬜ pending |
| 22-02-01 | 02 | 2 | TEST-02 | T-22-02-I | Primitives bit-identical to shipped | unit | `make test` | ❌ W0 | ⬜ pending |
| 22-02-02 | 02 | 2 | TEST-02 | T-22-02-I | bleedLfo lift (D-05), zero rack/ouLayers | unit | `make test` | ❌ W0 | ⬜ pending |
| 22-02-03 | 02 | 2 | TEST-02 | T-22-02-I | RatioTable/Swing lifted, behavior preserved | unit | `make test` | ❌ W0 | ⬜ pending |
| 22-02-04 | 02 | 2 | TEST-02 | — | N/A | unit | `make test` | ❌ W0 | ⬜ pending |
| 22-03-01 | 03 | 3 | TEST-02 | T-22-03-I | RNG injected, exact draw order, bleed retained | unit | `make test` | ❌ W0 | ⬜ pending |
| 22-03-02 | 03 | 3 | TEST-02 | T-22-03-I | core == inline bit-exact @ 3 rates (D-08 gate) | golden/extraction | `make test` | ❌ W0 | ⬜ pending |
| 22-03-03 | 03 | 3 | TEST-02 | T-22-03-D | Inline DSP deleted, plugin builds+loads unchanged | golden + build | `make test && make` | ❌ W0 | ⬜ pending |
| 22-04-01 | 04 | 4 | TEST-04 | T-22-04-I | ±5V/freq/phase/determinism @ 44.1/48/96k | invariant | `make test` | ❌ W0 | ⬜ pending |
| 22-04-02 | 04 | 4 | TEST-04 | T-22-04-I | Golden replay bit-exact (canonical OS) / epsilon | golden regression | `make test` | ❌ W0 | ⬜ pending |
| 22-04-03 | 04 | 4 | TEST-04 | T-22-04-T | 3-OS CI matrix, Rack-free, pinned checkout action | CI | `make test` (per OS) | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

*Populated during planning from RESEARCH.md §"Validation Architecture" — maps TEST-01, TEST-02 (full extraction lands here per D-04), TEST-04 and each Success Criterion (±5V bounds, frequency accuracy, phase continuity at reset, fixed-seed determinism, golden-output regression, multi-rate 44.1/48/96 kHz) to a doctest case.*

---

## Wave 0 Requirements

- [ ] `tests/doctest.h` — vendored single-header framework (Plan 01)
- [ ] `make test` recipe appended to `Makefile` (additive; does not perturb `make`/`make dist`/`make install`) (Plan 01)
- [ ] `tests/main.cpp` — doctest impl TU; `tests/test_smoke.cpp` — trivial passing test (Plan 01)
- [ ] `src/dsp/RackCompat.hpp` + `Waveshape.hpp` + `RatioTable.hpp` + `Swing.hpp` (Plan 02)
- [ ] `src/dsp/ClockTracker.hpp` + `DriftEngine.hpp` + `LfoCore.hpp` (Plan 03)
- [ ] `tests/golden/*.f32` + sidecar `.txt` captured from the validated core per D-08 ordering (Plan 03)
- [ ] `tests/BlockDriver.hpp`, `tests/test_invariants.cpp`, `tests/test_golden.cpp` (Plan 04)
- [ ] `.github/workflows/test.yml` — 3-OS matrix (Plan 04)

*All Wave 0 infrastructure is built within the phase's own plans (this is an extraction + harness-standup phase — no pre-existing test infrastructure).*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| — | — | — | — |

*All phase behaviors have automated verification — the in-Rack listening audition is deferred to Phase 23 (CONTEXT.md `<deferred>`), out of scope here.*

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify (every task verifies via `make test`)
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency < ~5s
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** planned
