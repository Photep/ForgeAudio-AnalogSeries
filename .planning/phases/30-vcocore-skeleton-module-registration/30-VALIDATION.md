---
phase: 30
slug: vcocore-skeleton-module-registration
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-07-28
---

# Phase 30 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> Derived from `30-RESEARCH.md` § Validation Architecture.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | doctest 2.4.11, vendored at `tests/doctest.h` |
| **Config file** | none — `tests/main.cpp` (`DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`) + `Makefile` `TEST_CXXFLAGS` |
| **Quick run command** | `make test` |
| **Full suite command** | `make test && make strict && make guards` |
| **Filtered run** | `./build-test/test -tc="vco core*"` (new) · `-tc="vco harness*"` · `-tc="golden*"`; `-ltc` lists all cases |
| **Estimated runtime** | ~0.5 s incremental / ~10.5 s clean for `make test`; ~4.3 s for the full local gate |
| **Measured baseline** *(phase start, 2026-07-28)* | **67 cases / 67 passed / 0 failed / 2,615,121 assertions**; `make strict` PASS; `make guards` PASS |
| **CI legs** | 3-OS `test` job + `toolchain-gate` (strict C++11 + MinGW compile **and link** vs `libRack`) |

---

## Sampling Rate

- **After every task commit:** `make test` (~0.5 s incremental). Add `make strict` on any commit touching `src/`.
- **After every plan wave:** `make test && make strict && make guards`.
- **Before `/gsd-verify-work`:** full local gate green **and** both CI jobs green on the pushed commit.
- **Max feedback latency:** ~5 s locally; CI on every push.

> **Standing rule (Phase 29, binding here):** the entire local gate returned exit 0 on code that could not
> link — `-fsyntax-only` never invokes a linker on any platform. `make strict` green is **not** evidence of
> link health. Only the CI `toolchain-gate` MinGW **link** step can see the link-class defect that got
> v2.0.0 rejected. No phase close on local evidence alone.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 30-W0-01 | 00 | 0 | all | — | `check_includes.sh [2/7]` no longer false-positives on the mandated `dsp/RackCompat.hpp` include; exemption carries its own negative control | property/static | `make guards` | ❌ W0 — patch `tests/check_includes.sh` | ⬜ pending |
| 30-W0-02 | 00 | 0 | CORE-01, CORE-03 | — | N/A | unit | `./build-test/test -tc="vco core*"` | ❌ W0 — `tests/test_vco_core.cpp` | ⬜ pending |
| 30-xx-xx | — | 1 | CORE-01 | — | N/A | unit | `./build-test/test -tc="vco harness*"` — swept block is **not** all-zero and **not** constant (D-15 tombstone inversion) | ✅ `tests/test_vco_harness.cpp` case 7, rewritten | ⬜ pending |
| 30-xx-xx | — | 1 | CORE-01 | T-30-01 | Hostile V/OCT cannot drive the phase accumulator out of range: `if (!(freq > 0.f)) freq = 0.f;` then clamp to `0.49 × sampleRate` | unit | `./build-test/test -tc="vco core: naive pitch*"` — output period matches `261.6256 · 2^pitchCV` within **1 %**, measured on the **OUTPUT** via a sub-sample-interpolated rising-crossing estimator. **Labelled NOT the TEST-02 tracking gate.** | ❌ W0 | ⬜ pending |
| 30-xx-xx | — | 1 | CORE-01 | T-30-01 | Bounds the runaway that finiteness cannot catch (measured: −8,655,011 V while every sample stayed `isfinite`) | unit | `./build-test/test -tc="vco core: output magnitude*"` — `\|out\| <= 6.0 V` over `sweepScenario` **and** the fixed `morph 0 / character 1` worst case (analytic ceiling ±5.55 V) | ❌ W0 | ⬜ pending |
| 30-xx-xx | — | 1 | CORE-01 | — | N/A | unit | `./build-test/test -tc="vco harness: output is finite*"` — **D-19 re-evidenced** under real DSP | ✅ exists; banner rewritten | ⬜ pending |
| 30-xx-xx | — | 1 | CORE-01 | — | N/A | unit | `./build-test/test -tc="vco harness: seam determinism*"` — **D-19 re-evidenced** under real DSP | ✅ exists; banner rewritten | ⬜ pending |
| 30-xx-xx | — | 1 | CORE-01 | — | N/A | unit | `./build-test/test -tc="vco core: spread seed divergence*"` — different spread seed → `maxAbsDiff > 0.01 V` and >90 % of samples differ, **at `character = 1.0`** | ❌ W0 | ⬜ pending |
| 30-xx-xx | — | 1 | CORE-03 | — | N/A | unit | `./build-test/test -tc="vco core: two-instance independence*"` — two instances interleaved sample-by-sample reproduce their solo blocks bit-exactly, with the 5 non-vacuity preconditions | ❌ W0 | ⬜ pending |
| 30-xx-xx | — | 1 | CORE-03 | — | N/A | unit | `./build-test/test -tc="vco core: independence positive control*"` — a deliberately-shared static accumulator **fails** the same check | ❌ W0 | ⬜ pending |
| 30-xx-xx | — | 2 | PANEL-03 | T-30-04 | VCO code must not enter the shipped LFO's build graph | structural | `make && nm -gU plugin.dylib \| grep modelAnalog` — both `modelAnalogLFO` and `modelAnalogVCO` exported | ✅ verified on the research prototype | ⬜ pending |
| 30-xx-xx | — | 2 | PANEL-03 | — | N/A | structural | `python3 -m json.tool plugin.json` + the D-05 operator-surfaced `git diff` — exactly two `modules[]` entries, LFO entry **byte-unchanged** | ✅ property of the diff | ⬜ pending |
| 30-xx-xx | — | 2 | PANEL-03 | — | N/A | structural | `res/AnalogVCO.svg` parses to 18.00 HP with ≥1 shape — **no `<text>` element** (SDK nanosvg has no text parser; `<text>` is silently dropped) | ✅ verified on the research prototype | ⬜ pending |
| 30-xx-xx | — | 2 | PANEL-03 | T-30-04 | Shipped LFO cannot regress | regression | `./build-test/test -tc="golden*"` + `make guards` — LFO goldens byte-identical | ✅ standing guardrail | ⬜ pending |
| 30-xx-xx | — | all | all | — | C++11/ODR class that rejected v2.0.0 | compile gate | `make strict` | ✅ verified PASS on the research prototype | ⬜ pending |
| 30-xx-xx | — | phase gate | all | T-30-04 | Link-class defect detection | **CI-only** link gate | `toolchain-gate` job on push — `AnalogVCO.cpp` rides the existing `src/*.cpp` glob | ✅ job exists | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

**Vacuity traps — measured, not assumed.** Both must be respected or the corresponding test is worthless:
- At `character = 0`, a clobbered shared `Waveshape` is **0/1024 detectable** and seed divergence is **exactly 0.000000 V**. D-17 and D-18a therefore **must** run at `character = 1.0` with different spread seeds.
- `sweepScenario` alone maxes at exactly 5.0000 V, so it never exercises the >5 V overshoot D-13 is about. The magnitude test needs the fixed `morph 0 / character 1` worst case in addition to the sweep.

---

## Wave 0 Requirements

- [ ] **`tests/check_includes.sh` `[2/7]` exemption — BLOCKING.** The Rack-SDK detector matches any include path containing `[Rr]ack`, so D-14's mandated `#include "dsp/RackCompat.hpp"` (for `exp2_taylor5`) is a false positive. Reproduced: `make guards` exits 1 on the prototype. This directly contradicts `check_canary.sh [5b/5]`, which already allow-lists `RackCompat.hpp` for the VCO seam. `make guards` and CI fail without this patch. Ships with its own negative control so the exemption is validated at the moment it is introduced.
- [ ] `tests/test_vco_core.cpp` — CORE-01 (pitch, magnitude, divergence) and CORE-03 (independence + positive control)
- [ ] `src/AnalogVCO.cpp` — PANEL-03 and the in-Rack surface
- [ ] `res/AnalogVCO.svg` — 91.44 mm × 128.5 mm stub, **no `<text>` element**
- [ ] Framework install: **none needed** — doctest vendored, compiler present, Rack SDK present

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| The VCO appears in Rack's module browser as "Analog VCO" and makes sound | PANEL-03, CORE-01 | Requires the Rack application and a human ear; the crude aliased timbre is the **expected result, not a defect** | Build, install, **flush the stale install first**, restart Rack, add the module, patch a V/Oct source to V/OCT and OUT to an audio module, sweep MORPH and CHARACTER, confirm every visible control audibly does something (D-07). A "missing" VCO is far more likely a stale install than a registration bug. |
| The LFO is visually and audibly unchanged | milestone guardrail | Visual/audible | In the same Rack session, add the Analog LFO and confirm panel + behavior are unchanged |
| The D-05 registration diff and the `[2/7]` guard patch | PANEL-03 | Operator judgement on a one-way door (the permanent slug `ForgeAnalogVCO`) | Present the `plugin.cpp` / `plugin.hpp` / `plugin.json` diff **plus the `check_includes.sh [2/7]` patch** on the same operator surface before commit |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s locally
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
