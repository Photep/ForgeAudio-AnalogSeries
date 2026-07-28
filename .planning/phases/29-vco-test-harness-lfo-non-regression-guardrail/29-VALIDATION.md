---
phase: 29
slug: vco-test-harness-lfo-non-regression-guardrail
status: draft
nyquist_compliant: false
wave_0_complete: true
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
| **Measured baseline** *(phase start)* | 50 cases / 50 passed / 2,615,027 assertions; `make strict` PASS |
| **Measured at phase close** *(29-05 Task 1)* | 64 cases / 64 passed / 0 failed / 2,615,099 assertions; `make strict` PASS; `make guards` PASS |
| **Measured gate latency** *(29-05 Task 1)* | `make test` 0.47 s + `make strict` 1.61 s + `make guards` 2.24 s = **~4.3 s** incremental for the full local gate |

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

> **Status column recorded 2026-07-28 by plan 29-05 Task 1** from a single local gate run
> (`make test`, the three filtered suites, `make strict`, `make guards`,
> `make guards RACK_DIR=/nonexistent-rack-sdk`, `make clean && make`). Observed outputs are in
> `29-05-SUMMARY.md`.

| Req | Behavior to validate | Test Type | Automated Command | File Exists | Status |
|-----|----------------------|-----------|-------------------|-------------|--------|
| TEST-01 | `VcoBlockDriver` drives `VcoCore` at 44.1 / 48 / 96 kHz | unit | `./build-test/test -tc="vco harness*"` | ✅ `tests/test_vco_harness.cpp` | ✅ green — 7/7 passed, 34 assertions |
| TEST-01 | Harness links no libRack | structural | `make test` succeeds with no `-I$(RACK_DIR)/include` and no `-lRack` | ✅ property of target | ✅ green — `rm -rf build-test && make test RACK_DIR=/nonexistent-rack-sdk` → 64/64 passed; `otool -L build-test/test \| grep -ci rack` → 0 |
| TEST-01 | `sampleTime == 1/sr` injected every step | unit | same TU — assert core observed `1/sr` | ✅ W0 delivered | ✅ green — cases `overwrites caller sampleTime with 1/sampleRate every step` + `injects sampleRate every step` |
| TEST-01 | Default seeds non-degenerate (no `(0,0)` Xoroshiro fixed point) | unit | assert `VcoBlockDriver{}` ctor defaults ≠ `(0,0)`; drift RNG emits non-zero draw | ✅ W0 delivered | ✅ green — case `default seeds are non-degenerate` |
| TEST-01 | Seam determinism: identical runs bit-identical; different seed diverges | unit | mirrors `tests/test_invariants.cpp:156,175` | ✅ W0 delivered | ⚠️ **green-but-weak (P-7)** — the assertion passes, but `VcoCore::step()` is silent by construction (D-01), so it currently compares two all-zero blocks. **Not full coverage.** Becomes load-bearing in **Phase 30**, which deletes the TOMBSTONE case and gives `step()` real output; this row must be re-evidenced there. |
| TEST-01 | Output finite (no NaN/Inf) | unit | `CHECK(std::isfinite(out[i]))` | ✅ W0 delivered | ⚠️ **green-but-weak (P-7)** — `std::isfinite(0.f)` is trivially true while the seam is silent. **Not full coverage.** Becomes load-bearing in **Phase 30** once the core emits a real waveform; re-evidence there. |
| TEST-04 | 6 LFO golden replays still green | regression | `./build-test/test -tc="golden*"` | ✅ exists — byte-unchanged vs `v2.0.1` | ✅ green — 6/6 passed, 49,164 assertions; `git diff --exit-code v2.0.1` over the six `.f32` fixtures → exit 0 |
| TEST-04 | Golden bytes unchanged (D-04) | property/hash | new case in `tests/test_lfo_guardrail.cpp` | ✅ W0 delivered | ✅ green — 7/7 passed, incl. the single-byte-change negative control and three published FIPS vectors |
| TEST-04 | Frozen headers unchanged (D-05) | property/hash | `bash tests/check_frozen.sh` (via `make guards`) + ubuntu CI step | ✅ `src/dsp/FROZEN.sha256` + `tests/check_frozen.sh` | ✅ green **locally** — `make guards` exit 0, 15/15 entries OK. CI leg wired but **not yet observed executing** (see § CI Observation Status). |
| TEST-04 | Dependency direction (D-06) | static analysis | `bash tests/check_includes.sh` (via `make guards`) + CI step | ✅ `tests/check_includes.sh` | ✅ green **locally** — `make guards` exit 0, all 7 sections. CI leg wired but **not yet observed executing** (see § CI Observation Status). |
| TEST-06 | VCO headers compile at `-std=c++11 -pedantic-errors` | compile gate | `make strict` | ✅ target + `src/vco_compile_canary.cpp` | ✅ green — `strict C++11 gate: PASS`, canary TU named in the compile line |
| TEST-06 | VCO headers survive MinGW compile **+ link vs libRack** | CI-only link gate | `toolchain-gate` job on push | ✅ job + canary TU exist | ⬜ **pending** — the gate's *negative* control is CI-only and is plan 29-05 Task 2. Green locally proves nothing here: Apple clang was measured linking the v2.0.0 failure class cleanly at `-O0` **and** `-O3`. |

*Status: ⬜ pending · ✅ green · ⚠️ green-but-weak (passes, but cannot currently fail) · ❌ red*

### CI Observation Status (recorded 29-05 Task 1, honest scope statement)

At the time Task 1 ran, `main` was **28 commits ahead of `origin/main`** and the newest Actions run
on the repository dated from 2026-07-13 — i.e. **no Phase 29 commit has ever been pushed**, so the
three `toolchain-gate` steps added by plans 29-03 and 29-04
(`VCO compile canary guard (D-07/D-08)`, `Frozen-header hash guard (D-05)`,
`Include / dependency-direction audit (D-06)`) have **never executed on a runner**. They are wired
and are proven green locally through `make guards`, which runs the identical scripts, but their
CI-leg execution is unobserved until the phase's work is pushed. ROADMAP success criterion 4
("runs on every push and is green") is therefore **not yet evidenced** and is closed out by Task 2,
whose procedure pushes and observes both jobs.

---

## Wave 0 Requirements

All items verified present on disk 2026-07-28 (plan 29-05 Task 1).

- [x] `src/dsp/VcoCore.hpp` — the seam TEST-01 drives *(29-01)*
- [x] `tests/VcoBlockDriver.hpp` — covers TEST-01 *(29-01; kept a permanently independent file from `tests/BlockDriver.hpp`, never templated or subclassed — R-2/P-4)*
- [x] `tests/test_vco_harness.cpp` — covers TEST-01 *(29-01; 7 cases incl. the D-01 TOMBSTONE)*
- [x] `tests/test_lfo_guardrail.cpp` — covers TEST-04 (D-04) + hasher self-test *(29-02; 7 cases)*
- [x] `tests/Sha256.hpp` — in-test hashing (D-04 Option A) was chosen *(29-02)*
- [x] `tests/golden/SHA256SUMS` **and** `src/dsp/FROZEN.sha256` — **both were produced, not either-or.** They are different guards over different surfaces: `SHA256SUMS` (29-02) is the human-runnable fixture manifest for the six `.f32` goldens plus `freerun_seeds.txt`; `FROZEN.sha256` (29-04) pins 15 entries covering the shipped LFO's whole behavioral closure — eleven `src/dsp/` headers, `src/AnalogLFO.cpp`, `tests/BlockDriver.hpp`, `tests/test_golden.cpp` and `tests/golden/freerun_seeds.txt`. `tests/check_frozen.sh` re-verifies both.
- [x] `tests/check_includes.sh` — covers TEST-04 (D-06) *(29-04, 7 sections)*
- [x] canary TU — covers TEST-06 *(29-03: `src/vco_compile_canary.cpp`, operator chose option-a; ODR-**uses** the VCO headers rather than merely including them, so the CI link leg has a real relocation to resolve — P-1)*
- [x] `.github/workflows/test.yml` additive steps — three appended `toolchain-gate` steps wire the canary guard (29-03) plus D-05 and D-06 (29-04). Wiring verified; **CI execution not yet observed** — see § CI Observation Status.

Additional Wave 0 artifacts delivered beyond the original list: `src/dsp/FROZEN.sha256`,
`tests/check_frozen.sh`, `tests/check_canary.sh`, and the `make guards` runner
(Rack-free, proven by `make guards RACK_DIR=/nonexistent-rack-sdk` → exit 0).

*Framework install: none needed — doctest vendored, compiler present, `../Rack-SDK` present.*

---

## Negative Controls (guards must be observed RED)

A green run proves nothing for a guard. Four deliverables are guards; each needs a failure sample.

| Guard | Negative control | Where it runs | Where the control now lives | Observed firing? |
|-------|------------------|---------------|-----------------------------|------------------|
| D-04 golden hash lock | Unit-test the hasher against the NIST vector `SHA-256("abc") = ba7816bf…f20015ad` **and** assert a one-byte-perturbed in-memory copy of a golden yields a different digest. Permanent, non-destructive | local + all CI | `tests/test_lfo_guardrail.cpp` — case `golden hash lock detects a single-byte change (negative control)`, plus three published FIPS 180-4 vector cases | **Yes** — permanent, runs in every `make test`. Observed 2026-07-28: 7/7 `lfo guardrail*` cases pass, 38 assertions. The control asserts the perturbed copy's digest **differs**, so a hasher that returned a constant would fail it. |
| D-05 frozen-header guard | Run the guard against a synthetic fixture with an appended blank line; assert non-zero exit | local + ubuntu CI | `tests/check_frozen.sh` `[3/3]` — a perturbed scratch copy of `MathConst.hpp` hashed through `hash_norm`, the **same function** `[1/3]` uses | **Yes**, twice. Permanent control runs on every `make guards` (perturbed copy `4a5bba22…` vs manifest `091eba70…`). Plus a one-off hand-run RED demo in 29-04: a corrupted digest in a scratch manifest → **observed exit 1**, `FROZEN FILE CHANGED: src/dsp/Waveshape.hpp`. |
| D-06 include audit | Run the grep against a synthetic fixture directory containing a deliberate violation; assert non-zero exit | local + CI | `tests/check_includes.sh` `[6/7]` — a synthetic TU including `dsp/VcoCore.hpp`, run through `detect_vco_includes`, the **same function** `[1/7]` uses | **Yes**, twice. Permanent control runs on every `make guards`. Plus a one-off hand-run RED demo in 29-04: `#include "dsp/VcoCore.hpp"` appended to a scratch copy of `LfoCore.hpp` → **observed exit 1**, `VCO header(s) reached the LFO build graph`. |
| **D-07 ODR link gate** | **CI-only.** Add a temporary in-class `static constexpr` array with runtime indexing (no out-of-line definition) to `VcoCore.hpp`, push, observe `toolchain-gate` MinGW link fail with `undefined reference`, revert. **Cannot be reproduced locally** — Apple clang links it clean at `-O0` and `-O3` (verified experimentally) | **push to CI only** | Plan 29-05 Task 2 — a `checkpoint:human-verify` operator procedure on throwaway branch `p29-odr-negative-control` | ⬜ **PENDING — awaiting the Task 2 CI observation.** No local substitute exists and none was attempted; a local link check would report success and constitute a false green. Until this row is filled with a failing run URL and the verbatim `undefined reference` line, **ROADMAP success criterion 3 is asserted, not demonstrated.** |
| D-07 C++17-ism gate | `inline constexpr` / `if constexpr` / `[[maybe_unused]]` / `std::clamp` each hard-error under `-std=c++11 -pedantic-errors -fsyntax-only` (all four verified) | local | `tests/check_canary.sh` `[4/5]` — each of the four C++17-isms compiled against the real canary TU and required to hard-error | **Yes** — permanent, runs on every `make guards`. Observed 2026-07-28 as part of `PASS: VCO compile canary guard clean`. |

**A note on what these controls do and do not cover.** Four of the five above are *syntax/hash* class
controls that a local toolchain can genuinely fail. The fifth — the ODR link gate — is a *linker*
class control, and it is the one that matters most, because a linker-class bug is precisely what got
v2.0.0 rejected. Apple clang materialises an in-class `static constexpr` array as a per-translation-unit
local symbol and links the failure class cleanly at every optimisation level measured (`-O0`, `-O3`).
Local green on this machine is therefore **not evidence about the CI gate**, and the four green rows
above must not be read as covering the fifth.

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

- [x] All tasks have `<automated>` verify or Wave 0 dependencies — every task across plans 29-01…29-05 carries an `<automated>` block; the sole `<human-check>` (29-05 Task 2) is the CI-only ODR control, which has an `<automated>` post-revert companion.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify — all 15 executed tasks ran `make test` (and `make strict` where `src/` was touched) before commit.
- [x] Wave 0 covers all MISSING references — every ❌ W0 entry in the Per-Task Verification Map is now ✅, verified file-by-file on disk.
- [x] No watch-mode flags — `make test`, `make strict` and `make guards` are all one-shot; no `--watch`, no `-w`.
- [x] Feedback latency < 30s — **measured 4.3 s** for the complete local gate (`make test` 0.47 s + `make strict` 1.61 s + `make guards` 2.24 s, incremental).
- [ ] Every guard has a negative control (observed red), not only a green run — **4 of 5 observed red.** D-04, D-05, D-06 and the D-07 C++17-ism gate all have controls that fire; the **D-07 ODR link gate is unobserved** and blocks this item until plan 29-05 Task 2 completes.
- [ ] `nyquist_compliant: true` set in frontmatter — deliberately held at `false` until the Task 2 ODR evidence exists. Setting it before that observation would be the exact posture in which v2.0.0 was tagged and rejected.

**Approval:** pending — blocked solely on the plan 29-05 Task 2 CI observation.
