---
phase: 29
slug: vco-test-harness-lfo-non-regression-guardrail
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-07-28
---

# Phase 29 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Derived from `29-RESEARCH.md` § Validation Architecture.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | doctest 2.4.11, vendored at `tests/doctest.h` |
| **Config file** | none — `tests/main.cpp` (`DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`) + `Makefile` `TEST_CXXFLAGS` |
| **Quick run command** | `make test` |
| **Full suite command** | `make test && make strict` locally; `git push` for the CI legs (3-OS `test` + `toolchain-gate`) |
| **Filtered run** | `./build-test/test -tc="golden*"` (6 passed / 44 skipped); `-ltc` lists all cases |
| **Estimated runtime** | ~10.5 s clean, ~0.5 s incremental; `make strict` ~2 s |
| **Measured baseline** | 50 cases / 50 passed / 2,615,027 assertions; `make strict` PASS |

---

## Sampling Rate

- **After every task commit:** `make test` (10.5 s clean / 0.5 s incremental). Add `make strict` on any commit touching `src/`.
- **After every plan wave:** `make test && make strict` plus the D-05 / D-06 guard scripts.
- **Phase gate:** push → both CI jobs green (3-OS `test` incl. all six golden replays + `toolchain-gate` strict + MinGW link) **and** the one-time ODR negative control observed red-then-green.
- **Max feedback latency:** 30 seconds locally; CI on `[push, pull_request]` = every commit.
- **Standing canary cadence (this phase's actual deliverable):** every subsequent phase (30–36) ends with the same command set.

---

## Per-Task Verification Map

> Populated by the planner from PLAN.md task IDs. Requirement → behavior mapping below is fixed by research.

| Req | Behavior to validate | Test Type | Automated Command | File Exists | Status |
|-----|----------------------|-----------|-------------------|-------------|--------|
| TEST-01 | `VcoBlockDriver` drives `VcoCore` at 44.1 / 48 / 96 kHz | unit | `./build-test/test -tc="vco harness*"` | ❌ W0 (`tests/test_vco_harness.cpp`) | ⬜ pending |
| TEST-01 | Harness links no libRack | structural | `make test` succeeds with no `-I$(RACK_DIR)/include` and no `-lRack` | ✅ property of target | ⬜ pending |
| TEST-01 | `sampleTime == 1/sr` injected every step | unit | same TU — assert core observed `1/sr` | ❌ W0 | ⬜ pending |
| TEST-01 | Default seeds non-degenerate (no `(0,0)` Xoroshiro fixed point) | unit | assert `VcoBlockDriver{}` ctor defaults ≠ `(0,0)`; drift RNG emits non-zero draw | ❌ W0 | ⬜ pending |
| TEST-01 | Seam determinism: identical runs bit-identical; different seed diverges | unit | mirrors `tests/test_invariants.cpp:156,175` | ❌ W0 — ⚠ vacuous while `step()` is silent (P-7) | ⬜ pending |
| TEST-01 | Output finite (no NaN/Inf) | unit | `CHECK(std::isfinite(out[i]))` | ❌ W0 — ⚠ vacuous while silent | ⬜ pending |
| TEST-04 | 6 LFO golden replays still green | regression | `./build-test/test -tc="golden*"` | ✅ exists — must stay **byte-unchanged** | ⬜ pending |
| TEST-04 | Golden bytes unchanged (D-04) | property/hash | new case in `tests/test_lfo_guardrail.cpp` | ❌ W0 | ⬜ pending |
| TEST-04 | Frozen headers unchanged (D-05) | property/hash | ubuntu CI step `sha256sum -c src/dsp/FROZEN.sha256` | ❌ W0 | ⬜ pending |
| TEST-04 | Dependency direction (D-06) | static analysis | `tests/check_includes.sh` in CI | ❌ W0 | ⬜ pending |
| TEST-06 | VCO headers compile at `-std=c++11 -pedantic-errors` | compile gate | `make strict` | ✅ target exists; ❌ canary TU missing | ⬜ pending |
| TEST-06 | VCO headers survive MinGW compile **+ link vs libRack** | CI-only link gate | `toolchain-gate` job on push | ✅ job exists; ❌ canary TU missing | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `src/dsp/VcoCore.hpp` — the seam TEST-01 drives
- [ ] `tests/VcoBlockDriver.hpp` — covers TEST-01
- [ ] `tests/test_vco_harness.cpp` — covers TEST-01
- [ ] `tests/test_lfo_guardrail.cpp` — covers TEST-04 (D-04) + hasher self-test
- [ ] `tests/Sha256.hpp` — if in-test hashing is chosen (D-04 Option A)
- [ ] `tests/golden/SHA256SUMS` and/or `src/dsp/FROZEN.sha256` — D-04/D-05 manifests (seed values in RESEARCH § "Current SHA-256 values")
- [ ] `tests/check_includes.sh` — covers TEST-04 (D-06)
- [ ] canary TU — covers TEST-06
- [ ] `.github/workflows/test.yml` additive steps — wires D-05/D-06 (and the canary, if Option B)

*Framework install: none needed — doctest vendored, compiler present, `../Rack-SDK` present.*

---

## Negative Controls (guards must be observed RED)

A green run proves nothing for a guard. Four deliverables are guards; each needs a failure sample.

| Guard | Negative control | Where it runs |
|-------|------------------|---------------|
| D-04 golden hash lock | Unit-test the hasher against the NIST vector `SHA-256("abc") = ba7816bf…f20015ad` **and** assert a one-byte-perturbed in-memory copy of a golden yields a different digest. Permanent, non-destructive | local + all CI |
| D-05 frozen-header guard | Run the guard against a synthetic fixture with an appended blank line; assert non-zero exit | local + ubuntu CI |
| D-06 include audit | Run the grep against a synthetic fixture directory containing a deliberate violation; assert non-zero exit | local + CI |
| **D-07 ODR link gate** | **CI-only.** Add a temporary in-class `static constexpr` array with runtime indexing (no out-of-line definition) to `VcoCore.hpp`, push, observe `toolchain-gate` MinGW link fail with `undefined reference`, revert. **Cannot be reproduced locally** — Apple clang links it clean at `-O0` and `-O3` (verified experimentally) | **push to CI only** |
| D-07 C++17-ism gate | `inline constexpr` / `if constexpr` / `[[maybe_unused]]` / `std::clamp` each hard-error under `-std=c++11 -pedantic-errors -fsyntax-only` (all four verified) | local |

**Plan requirements derived from this:**
- The D-04 hasher self-test and the D-06 fixture negative control become **permanent automated cases**.
- The D-07 ODR negative control becomes a **one-time `checkpoint:human-verify` task** — the operator pushes a deliberately-broken canary and confirms CI goes red. Without it, success criterion 3 is asserted, not demonstrated.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| MinGW ODR link gate actually fails on a real ODR violation | TEST-06 | Cannot be reproduced on macOS/clang — verified experimentally that clang links the v2.0.0 failure class clean at `-O0` and `-O3` | Introduce the known-bad in-class `static constexpr` array in `VcoCore.hpp`, push, confirm `toolchain-gate` fails with `undefined reference`, revert, confirm green |

*No in-Rack UAT required for Phase 29 — D-01 makes the VCO core silent by design, so there is no audio surface.*

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] Every guard has a negative control (observed red), not only a green run
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
