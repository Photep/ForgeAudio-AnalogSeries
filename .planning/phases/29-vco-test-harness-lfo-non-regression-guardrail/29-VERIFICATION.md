---
phase: 29-vco-test-harness-lfo-non-regression-guardrail
verified: 2026-07-28T09:39:00Z
status: passed
score: 7/7 must-haves verified
behavior_unverified: 0
overrides_applied: 0
---

# Phase 29: VCO Test Harness + LFO Non-Regression Guardrail Verification Report

**Phase Goal:** Before any VCO DSP exists, stand up the Rack-free VCO test harness and lock in the shipped-LFO guardrail as a standing, always-green canary — so no later phase can silently threaten the live LFO.
**Verified:** 2026-07-28T09:39:00Z (initial pass 09:25:15Z; re-checked 09:39:00Z after CI evidence arrived)
**Status:** passed
**Re-verification:** No — initial verification (single follow-up check to close one item, not a re-verification cycle)

## Goal Achievement

### Observable Truths

| # | Truth (from ROADMAP success criteria) | Status | Evidence |
|---|------|--------|----------|
| 1 | A single `make test` run replays the shipped LFO's `.f32` goldens byte-identical *and* drives a new Rack-free VCO harness | ✓ VERIFIED | Independently ran `make test` on clean HEAD: `67 \| 67 passed \| 0 failed` (2,615,121 assertions). Filtered: `-tc="golden*"` → 6/6 passed, 49,164 assertions; `-tc="lfo guardrail*"` → 10/10 passed (D-04 byte lock + FIPS vectors); `-ltc \| grep -c 'vco harness:'` → 7; TOMBSTONE case present (1). |
| 2 | `VcoBlockDriver` drives `forge::VcoCore` over sample blocks at 44.1/48/96 kHz with no libRack linked, using non-degenerate default seeds | ✓ VERIFIED | `tests/VcoBlockDriver.hpp` exists, includes only `dsp/VcoCore.hpp`; harness case "drives VcoCore over blocks at 44.1 / 48 / 96 kHz Rack-free" passes for all three `SAMPLE_RATES`. Seed defaults `0x1234ULL`/`0x9E3779B9ULL` present verbatim (proven non-degenerate by the harness's live-spread-coefficient assertion). `make test` links no `-lRack` per Makefile `TEST_SOURCES`/`TEST_CXXFLAGS`. |
| 3 | `make strict` (C++11, `-pedantic-errors`) and the CI MinGW **link** leg both cover the new VCO translation unit and VCO headers | ✓ VERIFIED (via documented interim design) | Independently ran `make strict` → `strict C++11 gate: PASS`, compile line includes `src/vco_compile_canary.cpp` (which `#include`s and ODR-uses `dsp/VcoCore.hpp`). `.github/workflows/test.yml` `toolchain-gate` job's MinGW leg globs `src/*.cpp`, which also picks up the canary. `src/AnalogVCO.cpp` does not exist yet (Phase 30 per REQUIREMENTS.md traceability) — Phase 29 deliberately substitutes `src/vco_compile_canary.cpp`, an ODR-using probe TU documented in the CI workflow comment as the interim stand-in "until Phase 30 lands src/AnalogVCO.cpp." This satisfies the criterion's intent (VCO headers pass both gates) given the roadmap's own phase ordering places `AnalogVCO.cpp` one phase later. Independently ran `make guards`: `check_canary.sh` [2/5]/[2b/5] confirm the canary ODR-uses the seam and emits a real link-time relocation (`T __ZN5forge21vcoCompileCanaryProbeEi`), and [4/5] confirms four C++17-isms are hard-rejected under `-std=c++11 -pedantic-errors`. Code review's CR-03 (canary previously constant-folded the landmine away, silently passing) was reproduced pre-fix and confirmed fixed post-fix via `nm` on the exact CI compile line — independently re-run here and still green. **Now also confirmed on the CI MinGW leg itself** — see Truth 4. |
| 4 | The full test + strict + MinGW canary runs in CI on every push and is green | ✓ VERIFIED | Independently confirmed via `git fetch origin` (`origin/main` now level with local `main` at `2049969`, `git status -sb` shows no ahead/behind) and `gh run view 30346638975 --json headSha,status,conclusion,jobs`: `conclusion: "success"`, `headSha: "20499696dfe422e330c46d229334e145d3bd941e"` — matches the exact commit verified locally throughout this report, not an earlier one. All four jobs report `success`: `toolchain-gate`, `test (ubuntu-latest)`, `test (macos-latest)`, `test (windows-latest)`. `toolchain-gate` step detail confirms all 8 steps `success`, including step 6 "win-x64 leg reproduction (compile + full link vs libRack)", step 7 "VCO compile canary guard (D-07/D-08)", step 8 "Frozen-header hash guard (D-05)", step 9 "Include / dependency-direction audit (D-06)", and step 10 "LFO non-regression guard suite via make (P-5)" — the `make guards` step's first-ever runner execution, per the WR-07 fix. Run: [30346638975](https://github.com/Photep/ForgeAudio-AnalogSeries/actions/runs/30346638975). |

**Score:** 4/4 ROADMAP criteria independently verified.

### Requirement-Level Truths (PLAN frontmatter must_haves, merged)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 5 | TEST-01: `src/dsp/VcoCore.hpp` is a bare POD seam (D-01) — `step()` returns `0.f`, no DSP | ✓ VERIFIED | Read full file: `struct VcoInputs` (11 fields, exact names/order/defaults per plan), `struct VcoCore` (drift member, nested `Telemetry`, `seed`/`setSpreadSeed`, `step()` returns `0.f` after recording telemetry). `grep -c 'return 0.f'` → line 108, sole return. |
| 6 | TEST-04: SHA-256 of every LFO golden pinned as string literals; hash lock runs in the same `make test` invocation | ✓ VERIFIED | `tests/golden/SHA256SUMS` digests independently recomputed via `shasum -a 256` and matched byte-for-byte against all 6 `.f32` fixtures. `tests/test_lfo_guardrail.cpp` runs inside `make test` (10 cases, 60 assertions, all pass). |
| 7 | R-2/R-3: `tests/BlockDriver.hpp` and `tests/test_golden.cpp` are byte-unchanged vs. shipped LFO | ✓ VERIFIED | `git diff v2.0.1 --stat -- src/dsp/ src/AnalogLFO.cpp tests/BlockDriver.hpp tests/test_golden.cpp tests/golden/ src/plugin.cpp src/plugin.hpp plugin.json` → only 3 files changed, all new additive files (`src/dsp/FROZEN.sha256`, `src/dsp/VcoCore.hpp`, `tests/golden/SHA256SUMS`); zero shipped LFO file touched. |

### Deferred Items

None — no gap identified maps to a later phase's stated goal or success criteria (Phase 30's `AnalogVCO.cpp` is not a deferred gap; it is the documented, deliberate design of criterion 3's interim satisfaction, per the CI workflow's own comment).

### Required Artifacts

| Artifact | Expected | Status | Details |
| -------- | ------------- | ------ | ------- |
| `src/dsp/VcoCore.hpp` | Bare POD seam | ✓ VERIFIED | 112 lines, `VcoInputs`/`VcoCore` present, C++11+C++17 clean (re-ran `make strict`), no Rack include, no second `forge::Inputs` (confirmed by `check_includes.sh` [4/7]). |
| `tests/VcoBlockDriver.hpp` | Independent copy of `BlockDriver.hpp`, retargeted | ✓ VERIFIED | 91 lines, `struct VcoBlockDriver`, no template/subclass relationship to `BlockDriver.hpp`, `sweepScenario` present. |
| `tests/test_vco_harness.cpp` | 7 doctest cases incl. TOMBSTONE | ✓ VERIFIED | 207 lines; `-ltc` confirms 7 `vco harness:` cases + 1 TOMBSTONE; all pass. |
| `tests/Sha256.hpp` | Vendored SHA-256, test-scope only | ✓ VERIFIED | Present; `check_includes.sh` [5/7] confirms no hashing implementation leaks into `src/`. |
| `tests/test_lfo_guardrail.cpp` | Golden hash lock + negative controls | ✓ VERIFIED | 10 passing cases incl. FIPS vectors and one-byte-perturbation negative control. |
| `tests/golden/SHA256SUMS` | Human-runnable digest mirror | ✓ VERIFIED | 6 entries, all independently reproduced. |
| `src/vco_compile_canary.cpp` | ODR-using probe TU | ✓ VERIFIED | Present, linked into `plugin.dylib` locally (confirmed via `make` after `rm plugin.dylib`), covered by `make strict` and CI MinGW glob — and now confirmed on the CI MinGW leg itself (run 30346638975, step 6 `success`). |
| `tests/check_canary.sh` | D-07/D-08 guard + negative controls | ✓ VERIFIED | `make guards` → PASS locally, incl. [2b/5] ODR-use-under-`-O3` check (CR-03 fix) and [4/5] observed-red C++17-ism controls; CI step 7 `success` on the post-fix version. |
| `src/dsp/FROZEN.sha256` | 15-entry manifest | ✓ VERIFIED | 15 entries present; `check_frozen.sh` confirms all 15 + 6 golden fixtures OK, plus completeness sweep and negative control; CI step 8 `success`. |
| `tests/check_frozen.sh` | Hash guard + coverage floor | ✓ VERIFIED | `make guards` output shows CR-02 fix (coverage-floor + completeness sweep) live; CI step 8 `success` on the post-fix version. |
| `tests/check_includes.sh` | Dependency-direction audit | ✓ VERIFIED | 7/7 sections pass locally incl. two-hop negative control (CR-01 fix) and guard-wiring self-audit (WR-06/WR-07 fixes); CI step 9 `success` on the post-fix version. |
| `.github/workflows/test.yml` toolchain-gate steps | Append-only CI wiring | ✓ VERIFIED | Read full file: `toolchain-gate` job present with strict gate, MinGW compile+link, 3 guard steps, `make guards` step — all append-only per REVIEW-FIX claim ("19 added / 0 removed"), confirmed to actually execute and pass on a runner (run 30346638975, all 8 steps `success`, including step 10 `make guards` — its first-ever runner execution per WR-07). |

### Key Link Verification

| From | To | Via | Status | Details |
| ---- | -- | --- | ------ | ------- |
| `tests/VcoBlockDriver.hpp` | `src/dsp/VcoCore.hpp` | `#include "dsp/VcoCore.hpp"` only | ✓ WIRED | Confirmed by read; no Rack include path passed by `make test`. |
| `tests/test_vco_harness.cpp` | `make test` | `TEST_SOURCES := $(wildcard tests/*.cpp)` | ✓ WIRED | Zero Makefile edits per plan; file picked up automatically, ran green locally and in CI (all 3 OS legs). |
| `src/vco_compile_canary.cpp` | `make strict` / CI MinGW link | `src/*.cpp` glob in both `strict:` target and `toolchain-gate` win-x64 loop | ✓ WIRED | Confirmed via `make strict` compile line locally and CI run 30346638975 step 6 `success`. |
| `tests/check_*.sh` | `.github/workflows/test.yml` + `make guards` | `GUARD_SCRIPTS` Makefile var, explicit CI steps, `[7/7]` self-audit | ✓ WIRED | `check_includes.sh` [7/7] independently confirms wiring for all 4 guard scripts except the documented `check_docs.sh` exemption; CI run 30346638975 confirms all 4 execute and pass (steps 7-10). |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| -------- | ------- | ------ | ------ |
| Full local gate green on clean tree | `make test && make strict && make guards && make` | All exit 0 (independently re-run, not just trusted from SUMMARY/local_state) | ✓ PASS |
| LFO goldens byte-identical to shipped tag | `git diff v2.0.1 --stat` over 15 frozen paths + 6 golden fixtures | Only 3 new additive files changed; zero existing file touched | ✓ PASS |
| Golden digests match independently computed SHA-256 | `shasum -a 256` on all 6 `.f32` files vs. `SHA256SUMS` | Byte-for-byte match | ✓ PASS |
| Canary produces a real link-time symbol | `check_canary.sh` [2/5] | `T __ZN5forge21vcoCompileCanaryProbeEi` present at `-O3` | ✓ PASS |
| Plugin links with canary TU | `rm plugin.dylib && make` | Links clean, `build/src/vco_compile_canary.cpp.o` in the link line | ✓ PASS |
| CI green on exact current HEAD | `git fetch origin` + `gh run view 30346638975 --json headSha,status,conclusion,jobs` | `headSha` matches local HEAD exactly (`2049969`); `conclusion: success`; all 4 jobs `success`; all 8 `toolchain-gate` steps `success` | ✓ PASS |

### Probe Execution

No `scripts/*/tests/probe-*.sh` convention exists in this repo. Phase 29's own guard scripts (`tests/check_*.sh`) serve this role and were executed directly above via `make guards` — all three passed with their built-in negative controls firing correctly, and confirmed passing on a CI runner (run 30346638975).

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| ----------- | ---------- | ----------- | ------ | -------- |
| TEST-01 | 29-01, 29-05 | Rack-free test harness drives `VcoCore` over sample blocks, runnable via `make test` with no libRack | ✓ SATISFIED | 7 harness cases green locally and in CI (all 3 OS legs, run 30346638975); no `-lRack` in `TEST_CXXFLAGS`. |
| TEST-04 | 29-02, 29-04, 29-05 | Shipped LFO's `.f32` goldens replayed byte-identical, byte-locked via SHA-256 | ✓ SATISFIED | Golden replay (6/6) + hash lock (10/10) + independent digest recomputation, all green locally; frozen-header hash guard `success` in CI (step 8). |
| TEST-06 | 29-03, 29-04, 29-05 | `make strict` and CI MinGW link leg cover new VCO TU and headers | ✓ SATISFIED | Local: fully verified. CI: `toolchain-gate` step 4 (strict) and step 6 (MinGW link) both `success` on the exact current HEAD (run 30346638975), including the post-code-review versions of the guard scripts. |

No orphaned requirements: REQUIREMENTS.md's phase-rollup lists exactly `TEST-01, TEST-04, TEST-06` for Phase 29, matching the union of `requirements:` fields across all 5 plans.

### Anti-Patterns Found

None. Scanned all 12 phase-modified/created files (`src/dsp/VcoCore.hpp`, `tests/VcoBlockDriver.hpp`, `tests/test_vco_harness.cpp`, `tests/Sha256.hpp`, `tests/test_lfo_guardrail.cpp`, `src/vco_compile_canary.cpp`, `tests/check_canary.sh`, `tests/check_frozen.sh`, `tests/check_includes.sh`, `Makefile`, `.github/workflows/test.yml`, `src/dsp/FROZEN.sha256`) for `TBD|FIXME|XXX|TODO|HACK|PLACEHOLDER` and "not yet implemented"-style language. Zero matches. The one deliberately deferred item (`tests/check_docs.sh` remaining unwired) is tracked in `.planning/todos/pending/wire-check-docs-into-ci.md`, explicitly out of Phase 29 scope, and self-documented as `EXEMPT` (not silently absorbed) by `check_includes.sh` [7/7].

### Human Verification Required

None. The one open item from the initial pass — CI observation of the exact current HEAD — has been independently confirmed closed (see Truth 4 and the "CI green on exact current HEAD" spot-check above): `gh run view` shows `headSha` matching local HEAD exactly, all four CI jobs `success`, and `git fetch` confirms `origin/main` is level with local `main`.

### Gaps Summary

No gaps. All four ROADMAP success criteria and all seven PLAN-frontmatter must-haves are independently verified against the codebase and against CI — not merely asserted by SUMMARY.md or by the coordinator's message. The CI-currency claim in particular was independently re-checked via `git fetch` + `gh run view` rather than taken on trust, and the headSha, conclusion, and per-step results all matched the claim exactly.

**Two items carried forward for later phases, not gaps in this phase:**

1. **Two TEST-01 rows are green-but-weak (P-7).** "Seam determinism" and "output is finite" pass only because `VcoCore::step()` returns `0.f` by construction (D-01) — they currently compare/check all-zero blocks and are not full DSP coverage. **Phase 30**, which deletes the TOMBSTONE case and lands the first real waveform, must re-evidence both rows once `step()` produces real output.
2. **Standing rule from `29-VALIDATION.md`, reconfirmed by this verification's own sequence.** A fully green local gate (`make test` + `make strict` + `make guards`) is necessary but was proven in this phase not sufficient — the MinGW link leg is the only gate that catches the ODR-class defect that rejected v2.0.0, and it is CI-only. No tag or VCV Library resubmission may be cut on local evidence alone; `toolchain-gate` must be observed green on the exact commit being tagged. Today's verification is itself a worked example: the first pass correctly declined to certify criterion 4 from local evidence alone (17 unpushed commits, including a full code review that rewrote the guard scripts), and only certified it after independently observing CI green on the exact HEAD sha via `gh run view`.

---

_Verified: 2026-07-28T09:25:15Z; CI-currency item independently re-checked and closed 2026-07-28T09:39:00Z_
_Verifier: Claude (gsd-verifier)_
