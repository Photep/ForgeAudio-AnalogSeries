---
phase: 30-vcocore-skeleton-module-registration
plan: 07
subsystem: verification
tags: [phase-gate, ci-observation, mingw-link-leg, uat, stale-install, standing-rule, d-07, d-13, t-30-12, t-30-13]

# Dependency graph
requires:
  - phase: 29-vco-test-harness-lfo-guardrail
    provides: "the standing rule (no phase closes on local evidence alone), the CI toolchain-gate job with its win-x64 MinGW compile-and-link leg, and the measured proof that the ENTIRE local gate returns exit 0 on code that cannot link"
  - phase: 30-vcocore-skeleton-module-registration
    plan: 01
    provides: "the [2/7] exact-path exemption and its two-direction control; the recorded operator approval of the permanent slug"
  - phase: 30-vcocore-skeleton-module-registration
    plan: 02
    provides: "the live VcoCore::step() body; the D-15 tombstone inversion; the D-19 closure"
  - phase: 30-vcocore-skeleton-module-registration
    plan: 03
    provides: "tests/test_vco_core.cpp CORE-01 cases; the [1/7] VCO_SIDE_ALLOW entry flagged for confirmation here"
  - phase: 30-vcocore-skeleton-module-registration
    plan: 04
    provides: "the CORE-03 interleave pair and the permanent positive control whose pass-by-detection is confirmed here"
  - phase: 30-vcocore-skeleton-module-registration
    plan: 05
    provides: "src/AnalogVCO.cpp, res/AnalogVCO.svg, and the four control coordinates this UAT names"
  - phase: 30-vcocore-skeleton-module-registration
    plan: 06
    provides: "the additive registration; the plugin.json diff-shape discrepancy flagged for judgement here; the PANEL-03 retroactive-truth claim"
provides:
  - "An OBSERVED CI record on the exact Phase-30 commit: SHA 7933fae36ad98882ac8964f17d6c1b15f60087fd, run 30407971115, toolchain-gate = success, win-x64 leg reproduction step's OWN conclusion = success — the record Phase 36 stands on when it cuts the tag and updates VCV Library issue #929"
  - "Closure of research assumption A5: the CORE-01 and CORE-03 tolerances measured on Apple clang are confirmed under GCC/libstdc++ (Ubuntu) and MinGW g++ (Windows)"
  - "Operator sign-off that the Analog VCO appears and all four controls are audibly live, and that the shipped Analog LFO is unchanged in the same session"
  - "The D-01..D-19 decision-to-plan coverage map and the four roadmap criteria mapped to evidence"
  - "A durable UAT lesson: the extracted Rack install can be a WHOLE stale plugin (manifest + art + dylib), so a flush that refreshes only the dylib and res/ is insufficient"
affects: [31-pitch-tuning-fm, 32-morph-blep, 34-analog-engine-output, 35-panel-display, 36-release-library-update]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Flush the WHOLE extracted plugin directory from dist/, not selected files — a stale install is a stale plugin version, not a stale binary"
    - "Read a CI step's OWN conclusion, never the job's: a step that fail-fasts upstream is reported `skipped`, which scans as 'not red'"
    - "Cross-check a cumulative count against the number each contributing plan RECORDED, not against a recomputation — the ladder catches a case silently renamed out of its suite prefix, which an endpoint check cannot"
    - "Confirm a positive control is green FOR THE RIGHT REASON by reading its -s output: invariant 5 must show the mismatches it detected, not merely a pass"

key-files:
  created: []
  modified: []

key-decisions:
  - "Executor: the Task 3 flush was WIDENED beyond the plan's Step 2. The extracted install was a complete v2.0.0 LFO-only plugin — stale manifest (one modules[] entry) and stale res/ (no AnalogVCO.svg) as well as a stale dylib. Refreshing only the dylib and res/ as the plan's text specifies would have left Rack reading a one-module manifest, so the VCO would still not have appeared and the operator would have hit the exact misdiagnosis the task exists to prevent."
  - "Executor: the [1/7] VCO_SIDE_ALLOW exactness was confirmed by READING the guard's matching code, not by creating a near-miss fixture. This plan modifies no file and must end with a clean tree; 30-03 already proved the point by construction, and the quoted-RHS [[ == ]] form is direct evidence."
  - "Executor: the 69-vs-72 CI case count was investigated rather than accepted or reported as a failure. It is exactly the three #if defined(__APPLE__) drift-ON golden cases (Phase-26 decision), so all 12 VCO cases ran on all three OSes — which is what actually closes assumption A5."

requirements-completed: [CORE-01, CORE-03, PANEL-03]

coverage:
  - id: D1
    description: "The full local gate is green at the final case count, and the cross-plan ladder was checked against each plan's RECORDED number rather than accepted at its endpoint"
    requirement: "CORE-01"
    verification:
      - kind: unit
        ref: "make test -> 72 cases / 72 passed / 0 failed, 2,615,872 assertions — assertion count identical to the figure 30-04 and 30-06 both recorded"
        status: pass
      - kind: integration
        ref: "./build-test/test -ltc | grep -c 'vco harness:' -> 7; grep -c 'vco core:' -> 5; the five vco core case names enumerated and matched to 30-03's three and 30-04's two"
        status: pass
      - kind: integration
        ref: "ladder: 67 (30-VALIDATION.md:27, 2,615,121) -> 67 (30-02 row 1, 2,615,122) -> 70 (30-03 row 1, 2,615,848) -> 72 (30-04 row 1, 2,615,872) -> 72 (30-06 row 9) -> 72 measured here. No discrepancy"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every standing tripwire fired correctly and the shipped LFO is provably untouched across the whole phase (T-30-04)"
    requirement: "CORE-01"
    verification:
      - kind: unit
        ref: "./build-test/test -tc='golden*' -> 6 cases / 6 passed, 49,164 assertions byte-identical"
        status: pass
      - kind: integration
        ref: "check_frozen.sh -> (15 pinned entries checked), no bump; check_canary.sh [2b/5] -> 'all 8 VcoInputs DSP field(s) stay runtime-live through step() at -O3'; check_includes.sh -> exactly 2 '[2/7] detector' lines, both directions"
        status: pass
      - kind: integration
        ref: "git diff --name-only 93cca2f..HEAD names NONE of src/AnalogLFO.cpp, res/AnalogLFO.svg, src/dsp/FROZEN.sha256, Waveshape.hpp, RackCompat.hpp, MathConst.hpp, DriftEngine.hpp, tests/BlockDriver.hpp, tests/VcoBlockDriver.hpp"
        status: pass
      - kind: integration
        ref: "make strict -> PASS over 4 TUs; make guards -> PASS; make guards RACK_DIR=/nonexistent-rack-sdk -> PASS; git status --porcelain -> empty"
        status: pass
    human_judgment: false
  - id: D3
    description: "The CI toolchain-gate job AND its win-x64 leg reproduction step were OBSERVED to conclude success on a run whose head SHA is the exact commit under test — the standing rule satisfied, not asserted (T-30-12 / T-30-09)"
    verification:
      - kind: integration
        ref: "run 30407971115 headSha == 7933fae36ad98882ac8964f17d6c1b15f60087fd; run conclusion success; toolchain-gate conclusion success; step [6] 'win-x64 leg reproduction (compile + full link vs libRack)' OWN conclusion success, explicitly not skipped; log line 'win-x64 link gate: PASS'"
        status: pass
      - kind: integration
        ref: "three-OS test matrix: test (ubuntu-latest), test (macos-latest), test (windows-latest) all conclusion success"
        status: pass
    human_judgment: false
  - id: D4
    description: "The stale-install path was excluded MECHANICALLY before any judgement about whether the module works (T-30-13 / hard prohibition 8)"
    verification:
      - kind: integration
        ref: "pre-flush: installed plugin.dylib a9c4731e (Jul 9) exporting ONLY _modelAnalogLFO, plugin.json version 2.0.0 with one modules[] entry, res/ without AnalogVCO.svg — hashes DIFFERED from the built f1b9cb16"
        status: pass
      - kind: integration
        ref: "post-flush: shasum plugin.dylib == shasum installed plugin.dylib (f1b9cb16); installed manifest 2.0.1 with ForgeAnalogLFO + ForgeAnalogVCO; res/AnalogVCO.svg present; both symbols exported; codesign -v valid"
        status: pass
    human_judgment: false
  - id: D5
    description: "A human confirmed the module appears as Analog VCO, that all four controls are audibly live (D-07), and that the shipped Analog LFO is visually and audibly unchanged in the same session"
    requirement: "PANEL-03"
    verification: []
    human_judgment: true
    rationale: "Nothing automatable can hear an oscillator or judge that a shipped module is unchanged to a user. The acceptance criterion is an operator statement, recorded verbatim below."

# Metrics
duration: 27 min
completed: 2026-07-29
status: complete
---

# Phase 30 Plan 07: Phase Gate Summary

**The milestone's own standing rule is satisfied rather than asserted: the CI `win-x64 leg reproduction` step — the only gate in this project that invokes a linker on the VCV Library's own Windows toolchain — was read individually and observed `success` on the exact commit Phase 30 produced, behind a local gate green at 72/72/0 with every rung of the five-plan ladder checked against the number its own summary recorded, and in front of an operator who heard all four controls work beside an unchanged Analog LFO.**

## Performance

- **Duration:** 27 min (including the blocking operator checkpoint)
- **Started:** 2026-07-28T23:06:10Z
- **Completed:** 2026-07-28T23:33Z
- **Tasks:** 3
- **Files modified:** **0** — `files_modified: []` by design, and it held

## This plan changed no file

No source, test, guard, asset, manifest or workflow file was modified. No commit was produced by any of the three tasks. `git status --porcelain` was empty at the start, empty after the full local gate, and empty after the build-and-install of Task 3 (`plugin.dylib`, `dist/` and `build/` are all gitignored). Nothing was tagged, released, version-bumped or submitted; `plugin.json` stays at `2.0.1` (D-04) and Phase 36 owns REL-01 in full.

The only artifact this plan produces is evidence. That is the entire point of a gate.

---

## Task 1 — the full local gate at the final case count

| # | Check | Result |
|---|-------|--------|
| 1 | `make test` | exit 0 — **72 cases / 72 passed / 0 failed**, **2,615,872 assertions** |
| 2 | `./build-test/test -ltc \| grep -c 'vco harness:'` | **7** |
| 3 | `./build-test/test -ltc \| grep -c 'vco core:'` | **5** |
| 4 | `./build-test/test -tc="golden*"` | exit 0 — **6 / 6**, 49,164 assertions, byte-identical |
| 5 | `make strict` | exit 0 — `strict C++11 gate: PASS`, four translation units |
| 6 | `make guards` | exit 0 — `guard suite: PASS` |
| 7 | `make guards RACK_DIR=/nonexistent-rack-sdk` | exit 0 — still runnable on an SDK-less runner |
| 8 | `bash tests/check_canary.sh \| grep -c 'all 8 VcoInputs DSP field'` | **1** — eight fields, not three |
| 9 | `bash tests/check_includes.sh \| grep -c '\[2/7\] detector'` | **2** — both directions fired |
| 10 | `bash tests/check_frozen.sh` | exit 0 — **(15 pinned entries checked)**, no bump |
| 11 | `make RACK_DIR=../Rack-SDK` + `nm -gU plugin.dylib \| grep -c modelAnalog` | exit 0 — **2** (`_modelAnalogLFO` @ 0x14380, `_modelAnalogVCO` @ 0x14388) |
| 12 | `git status --porcelain` | **empty** |

The plan's single chained assertion returned its success sentence verbatim:

```
OK: full local gate green at 72/72/0, both models exported, tree clean - and this is NOT yet phase closure
```

### The five `vco core:` case names, enumerated

Recorded because a count alone cannot see a case renamed out of its suite prefix — the failure mode the ladder exists to catch.

```
vco core: naive pitch tracks the C4 reference on the OUTPUT within 1 percent (NOT the TEST-02 tracking gate)
vco core: output magnitude stays inside the 6.0 V loose bound (D-18b)
vco core: spread seed divergence at character 1.0 (D-18a)
vco core: two-instance independence under sample-by-sample interleaving (D-17)
vco core: independence positive control - a shared static accumulator FAILS the same check (D-17)
```

Three from 30-03 (CORE-01), two from 30-04 (CORE-03). The D-16 case still carries its `NOT the TEST-02 tracking gate` label in the registered case name, so nobody can mistake it for Phase 31's exit gate by reading a test list.

### The cross-plan case-count ladder — checked, not assumed

Each rung against the number the corresponding summary **recorded**, not a recomputation:

| Rung | Recorded where | Cases | Assertions |
|------|----------------|-------|------------|
| Phase-29 baseline | `30-VALIDATION.md` line 27 | **67** | 2,615,121 |
| after 30-01 | `30-01-SUMMARY.md` verification row 4 | **67** | — |
| after 30-02 | `30-02-SUMMARY.md` verification row 1 | **67** (D-15 inverted a case in place) | 2,615,122 |
| after 30-03 | `30-03-SUMMARY.md` verification row 1 | **70** (+3 CORE-01) | 2,615,848 |
| after 30-04 | `30-04-SUMMARY.md` verification row 1 | **72** (+2 CORE-03) | 2,615,872 |
| after 30-06 | `30-06-SUMMARY.md` verification row 9 | **72** | 2,615,872 |
| **measured here** | this gate | **72** | **2,615,872** |

**No discrepancy at any rung.** The assertion count is identical to the figure 30-04 and 30-06 each recorded, which is a stronger match than the case count alone — a case swapped for a different case of equal count would move it.

One rung reads out of order and is correct: **30-05 recorded 70, not 72**, because it executed in Wave 3 before 30-04 landed, and it said so in place while predicting 72. 30-06 confirmed the prediction. That is a wave-ordering artifact, not a dropped case.

### The phase diff never names a protected file

`git diff --name-only 93cca2f..HEAD` (the whole phase, from the commit before 30-01's first) returns 20 files and **none** of: `src/AnalogLFO.cpp`, `res/AnalogLFO.svg`, `src/dsp/FROZEN.sha256`, `src/dsp/Waveshape.hpp`, `src/dsp/RackCompat.hpp`, `src/dsp/MathConst.hpp`, `src/dsp/DriftEngine.hpp`, `tests/BlockDriver.hpp`, `tests/VcoBlockDriver.hpp`.

The eight non-planning files the phase touched: `.github/workflows/test.yml` (comment-only), `plugin.json` (+9), `res/AnalogVCO.svg` (+8), `src/AnalogVCO.cpp` (+159), `src/dsp/VcoCore.hpp`, `src/plugin.cpp` (+1), `src/plugin.hpp` (+1), `tests/check_includes.sh`, `tests/test_vco_core.cpp` (+917), `tests/test_vco_harness.cpp`.

### This task is a PRECONDITION, not phase closure

Stated explicitly because the distinction is the reason this plan exists. Phase 29 measured **this exact combination of green local signals** — `make test`, `make strict`, `make guards` and `check_canary.sh` all returning exit 0, and the strict gate reporting success on the Ubuntu runner too — on commit `e117cff`, code that **could not link**. `make strict` is `-fsyntax-only` and never invokes a linker on any platform. Only the CI `toolchain-gate` MinGW link leg caught it (run `30339957128` red on `undefined reference`, run `30340075121` green after revert). Everything in this section is Apple clang evidence and closes nothing on its own.

---

## Task 2 — the CI observation (the record Phase 36 stands on)

**This is the section Phase 36 must read before cutting a tag or updating VCV Library issue #929.**

| Field | Value |
|-------|-------|
| **Commit SHA under test** | **`7933fae36ad98882ac8964f17d6c1b15f60087fd`** |
| Push | `2049969..7933fae  HEAD -> main` (branching strategy `none`) |
| **Run ID** | **`30407971115`** |
| **Run URL** | **https://github.com/Photep/ForgeAudio-AnalogSeries/actions/runs/30407971115** |
| Run head SHA == commit under test | **YES** — asserted, not assumed |
| Run conclusion | **success** |

### The four job conclusions

| Job | Conclusion |
|-----|------------|
| **`toolchain-gate`** | **success** |
| `test (ubuntu-latest)` | success |
| `test (macos-latest)` | success |
| `test (windows-latest)` | success |

### The `toolchain-gate` steps, read INDIVIDUALLY

The job's conclusion is not sufficient and was not treated as such. Phase 29 recorded the specific trap: when an earlier step fail-fasts, later steps are reported `skipped`, and a `skipped` link step scans as "not red" to anyone reading a job summary.

| # | Step | Own conclusion |
|---|------|----------------|
| 1 | Set up job | success |
| 2 | Run actions/checkout@v4 | success |
| 3 | Fetch Rack SDKs (linux headers + windows link stub) | success |
| 4 | Strict C++11 pedantic gate (our code only) | success |
| **6** | **win-x64 leg reproduction (compile + full link vs libRack)** | **success** — not `skipped` |
| 7 | VCO compile canary guard (D-07/D-08) | success |
| 8 | Frozen-header hash guard (D-05) | success |
| 9 | Include / dependency-direction audit (D-06) | success |
| 10 | LFO non-regression guard suite via make (P-5) | success |

The linker genuinely ran. From the step's own log:

```
toolchain-gate  win-x64 leg reproduction (compile + full link vs libRack)
  2026-07-28T23:26:43.6171654Z win-x64 link gate: PASS
```

That step cross-compiles every `src/*.cpp` with `x86_64-w64-mingw32-g++ -std=c++11 -O3` and then links all four objects into `plugin.dll` against `libRack` with full symbol resolution. `src/plugin.cpp.o` now carries a genuinely new cross-TU reference to `modelAnalogVCO`, and `src/AnalogVCO.cpp` is a brand-new translation unit — research Pitfall 4 names exactly that as where the C++11 / ODR rejection class regrows. It resolved.

**The standing rule is therefore satisfied for Phase 30, and satisfied on the only gate that has ever caught this project's rejection-class defect.**

### Assumption A5 is CLOSED — and the 69-vs-72 case count explained

The three-OS `test` matrix reported **69** cases on Ubuntu and Windows against **72** on macOS. This was investigated rather than accepted or reported as a failure.

| Leg | Cases | Assertions |
|-----|-------|------------|
| `test (ubuntu-latest)` | 69 / 69 / 0 failed | 2,591,290 |
| `test (windows-latest)` | 69 / 69 / 0 failed | 2,591,290 |
| `test (macos-latest)` | 72 / 72 / 0 failed | 2,615,872 |

The gap is exactly three, and it is exactly the three `#if defined(__APPLE__)` drift-ON bit-exact golden cases in `tests/test_golden.cpp` (lines 111–132 and 150–162). That is the **Phase-26 decision** — portable drift-off goldens for 3-OS CI, drift-on macOS-gated, because `std::normal_distribution` is not portable across standard libraries. The three cross-platform drift-off cases run everywhere. **No Phase-30 case was dropped, skipped or gated.**

The consequence matters more than the explanation: because the only conditional compilation in the suite is those three golden cases, **all seven `vco harness:` and all five `vco core:` cases ran and passed on Ubuntu (GCC / libstdc++) and Windows (MinGW g++)**, not only on Apple clang. That is precisely what research **assumption A5** deferred to this first CI run: 30-03's 1 % pitch tolerance, its 6.0 V magnitude bound with the `> 5.1 V` exercise assertion, its 0.01 V divergence threshold, and 30-04's 0/1024 interleave and 512/512 control figures all hold cross-toolchain. **A5 is closed.**

---

## Task 3 — the in-Rack UAT

### Operator sign-off, recorded verbatim

> "Approved"

That is the plan's `<resume-signal>` acceptance token. Per the resume signal's own definition it confirms:

- the module appears in Rack's browser as **Analog VCO** beside **Analog LFO**;
- **all four controls are audibly live** — V/OCT changes pitch, MORPH changes timbre, CHARACTER changes timbre, OUT carries audio (**D-07**);
- the shipped **Analog LFO is visually and audibly unchanged in the same session** — the milestone guardrail's user-visible half.

**No timbre or output-level observations were raised, and none are invented here.** The operator reported no failures and volunteered nothing about harshness, aliasing or level. This is recorded explicitly rather than left silent so **Phase 32** knows its starting point is the crude aliased baseline **as designed** — not an unreported problem, and not a claim that the operator judged the sound acceptable, which was never the question. The expected-results block (crude/buzzy/harsh timbre; excursions above ±5 V at high CHARACTER, measured ceiling 5.55 V and 5.51803 V in the suite; the deliberately ugly unlabelled panel) was presented in full before the operator replied, so its absence from their answer is an absence of complaint, not an absence of exposure.

**Phase 34** likewise inherits no reported level problem; the D-13 overshoot remains an expected, measured, unconditioned-output property and OUT-01..03 own conditioning.

### The stale-install flush — it was genuinely needed, and the plan's Step 2 was insufficient

**This is a durable lesson for every future in-Rack UAT in this project, not an incident report.**

The plan opens the UAT with a flush and forbids diagnosing a missing module by reading source (hard prohibition 8), because the false-negative signature — *the NEW behavior is missing but every OLD behavior passes* — has already cost this project a debugging detour once. That precaution paid for itself immediately.

**Pre-flush state, measured before any opinion was formed:**

| Artifact | State |
|----------|-------|
| installed `plugin.dylib` | `a9c4731e7cac0f86b46673126016e3e2551cbd2f`, dated **Jul 9**, exporting **only** `_modelAnalogLFO` (1 symbol) |
| freshly built `plugin.dylib` | `f1b9cb16bb268bc8ef6e460569f745017319afb2`, exporting **both** (2 symbols) |
| **hashes matched?** | **NO** |
| installed `plugin.json` | **version `2.0.0`**, **one** `modules[]` entry: `ForgeAnalogLFO` |
| installed `res/` | `AnalogLFO.svg`, `PANEL-SPEC.md`, `components`, `fonts` — **no `AnalogVCO.svg`** |

The extracted directory was not a stale binary. **It was a whole stale plugin — a complete v2.0.0 LFO-only install.** `make install` builds the `.vcvplugin` archive and copies it to `plugins-mac-arm64/`; it does not touch the extracted directory Rack actually loads, so all three artifacts had sat unchanged since 2026-07-09.

**The plan's Step 2 as written would have been insufficient.** It names two refreshes — `rsync` of `res/` and `cp` of `plugin.dylib`. Following it literally would have produced a matching dylib hash and a passing automated check, while Rack continued reading a manifest advertising a single module. **The Analog VCO would still not have appeared in the browser** — and the operator would then have been staring at a green hash comparison and a missing module, which is the precise misdiagnosis loop the task was designed to prevent, arrived at *through* the safeguard rather than despite it.

**The correction, and the rule to carry forward:** flush the **whole** extracted directory from `dist/ForgeAudio-AnalogSeries/`, because that tree is by definition the set of bytes Rack would extract from the archive. Selected-file refreshes encode an assumption about which files changed, and a module-registration phase is exactly the case where the manifest is what changed.

> **Rule for every future in-Rack UAT in this project:**
> `rsync -a dist/ForgeAudio-AnalogSeries/ "$HOME/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio-AnalogSeries/"`
> — the entire directory, covering `plugin.json`, `res/`, `LICENSE` and `NOTICES`, not just `plugin.dylib` and `res/`. A stale install is a stale **plugin version**, not a stale binary. Phases 31, 32, 33, 34 and 35 all end in an in-Rack check; each should use the whole-tree flush.

**Post-flush state, verified mechanically before the checkpoint was presented:**

| Artifact | State |
|----------|-------|
| `shasum plugin.dylib` | `f1b9cb16bb268bc8ef6e460569f745017319afb2` |
| `shasum` installed `plugin.dylib` | `f1b9cb16bb268bc8ef6e460569f745017319afb2` — **MATCH** |
| installed `plugin.json` | version **`2.0.1`**, two entries: `ForgeAnalogLFO → Analog LFO`, `ForgeAnalogVCO → Analog VCO` |
| installed `res/` | now includes **`AnalogVCO.svg`** |
| installed dylib exports | `_modelAnalogLFO` **and** `_modelAnalogVCO` |
| `codesign -v` | valid |

The plan's automated assertion returned its sentence verbatim:

```
OK: the installed dylib is the freshly built one - a missing module is now a real finding, not a stale install
```

The repository tree stayed clean throughout — `plugin.dylib`, `dist/` and `build/` are gitignored (`.gitignore` lines 7, 5, 2), and `git status --porcelain` was empty after the build and after the install.

---

## Gate findings on the four items earlier plans deferred to this phase gate

### 1. 30-03's `check_includes.sh [1/7]` `VCO_SIDE_ALLOW` entry — CORRECT, exact-path. No operator action required.

30-03 added one entry for `tests/test_vco_core.cpp` and flagged it for operator confirmation here, because 30-01 routed the analogous `[2/7]` change through a blocking checkpoint.

**Judgement: correct as landed, and materially different from the `[2/7]` change that was checkpointed.**

- The list holds exactly five entries: `src/vco_compile_canary.cpp`, `src/AnalogVCO.cpp`, `tests/VcoBlockDriver.hpp`, `tests/test_vco_harness.cpp`, `tests/test_vco_core.cpp`.
- The match at `tests/check_includes.sh:294` is `[[ "${rel}" == "${a}" ]]` with a **quoted** right-hand side. A quoted RHS in `[[ == ]]` disables pattern matching, so this is a literal string comparison — **no glob, no substring, no basename**. Exact-path, by direct reading of the guard's own code.
- It **weakens no detector.** `[2/7]`'s change altered what counts as a violation; this change only states which side of an existing boundary a file sits on. `tests/test_vco_harness.cpp` has been on the same list for the same documented reason since Phase 29.
- 30-03 additionally proved it by construction with a near-miss fixture (`tests/test_vco_core_probe.cpp`, identical include) that still **failed** `[1/7]`.

Confirmed here by **reading** the guard rather than creating a fixture, because this plan modifies no file and must end with a clean tree. `check_includes.sh` exits 0 and reports both `[2/7] detector` control directions on every invocation.

### 2. `PANEL-03` — retroactively true. CONFIRMED, not un-checked. `deferred-items.md` item 1 resolved in the affirmative.

The checkbox was marked `[x]` at `docs(30-01)` (commit `048d22d`), before 30-06 landed the work its text names. 30-06 reported it now retroactively true and asked this gate to confirm rather than un-check.

PANEL-03 reads: *"The VCO is registered as a second module (`addModel` + `plugin.hpp` extern + `plugin.json` `modules[]` entry) without altering the LFO's registration."* All three named edits are present:

| Element | Location | Content |
|---------|----------|---------|
| `plugin.hpp` extern | `src/plugin.hpp:8` | `extern Model* modelAnalogVCO;` |
| `addModel` | `src/plugin.cpp:8` | `p->addModel(modelAnalogVCO);` |
| `plugin.json` entry | `plugin.json:26` | `"slug": "ForgeAnalogVCO"` |

And the LFO's registration is unaltered: the manifest parses to exactly **2** modules, `ForgeAnalogLFO` is still `modules[0]`, and `version` is still `2.0.1`. The slug matches `src/AnalogVCO.cpp:159`'s `createModel<AnalogVCO, AnalogVCOWidget>("ForgeAnalogVCO")` character for character. Task 3's operator sign-off confirms the user-visible half — it appears in the browser and it sounds.

**PANEL-03 is genuinely satisfied.** The premature marking was a bookkeeping ordering quirk, not a false green.

### 3. 30-06's `plugin.json` diff-shape discrepancy — INDEPENDENTLY REPRODUCED. Judgement recorded, not inherited.

30-06's plan asserted `git diff -U0 plugin.json | grep -c '^-[^-]'` returns **1**. The executor measured **0**, refused to edit the number, measured the underlying invariant directly instead, and recorded the discrepancy so this gate would see it rather than inherit an arranged green.

**Reproduced here from scratch, on commit `299e77c`:**

| Diff algorithm | Deleted lines in `plugin.json` |
|----------------|-------------------------------|
| `myers` (default) | **0** |
| `minimal` | **0** |
| `patience` | **0** |
| `histogram` | **0** |

`src/plugin.hpp` and `src/plugin.cpp` are 0 as the plan predicted.

**The underlying byte-identity invariant, measured directly against `299e77c^`:**

- `plugin.json` went 27 → 36 lines.
- **Lines 1–23 byte-identical** — every top-level field including `version`, plus the entire LFO element body.
- `old[23] = '    }'`, `new[23] = '    },'`, and `new[23] == old[23] + ','` — the single altered pre-existing line is a closing brace gaining exactly one comma and nothing else.
- Tail `['  ]', '}', '']` byte-identical.

**Judgement: the executor was right on all three counts.** `0 < 1` is the *safe* direction — the assertion was a ceiling on damage, not a floor; the dangerous reading is `> 1`, which would mean an existing line was rewritten. Git anchors the retained `    }` to the new VCO element and renders the whole change as a contiguous 9-line insertion, which no algorithm choice alters. And substituting a direct byte read for a marker count is **strictly stronger**: the count would have passed a diff that deleted the brace and re-added it with a changed description; the byte comparison would not.

**The lesson stands as 30-06 wrote it, and this gate endorses it:** assert byte identity by reading bytes. `git diff`'s hunk alignment is a presentation choice the renderer is free to make, and counting `+`/`-` markers measures the renderer as much as the change.

### 4. Invariant 5 passes BY DETECTING — the check 30-04 said was worth making here.

30-04 noted the question at this gate is not *"is invariant 5 green"* but *"is invariant 5 green for the right reason"* — if the positive control ever passes **without** detecting its own defect, invariant 4 is meaningless regardless of its own verdict. Read from `-s` output, not inferred from a pass:

**Invariant 5 — `vco core: independence positive control` (n = 512):**

| | 44.1 kHz | 48 kHz | 96 kHz |
|---|----------|--------|--------|
| `r.mismatchA` | **512** | **512** | **512** |
| `r.mismatchB` | **512** | **512** | **512** |
| `totalMismatch` (the asserted quantity) | **1024** | **1024** | **1024** |

`CHECK( totalMismatch > 0 )` reports SUCCESS at all three rates **because it measured 1024 mismatches**, not because it measured zero. The `DeliberatelyBrokenSharedStateCore` is detected on every single invocation of the suite. 512/512 matches 30-04's recorded figure exactly.

**Invariant 4 — `vco core: two-instance independence` (n = 1024)**, therefore meaningful:

| | all three rates |
|---|---|
| `REQUIRE( helperMatchesHarness )` | SUCCESS — the helper is bit-identical to `VcoBlockDriver::run()` before anything below it is trusted |
| `r.mismatchA` / `r.mismatchB` | **0 / 0** |
| `r.soloEqual` (distinguishability) | **0** of 1024 |

CORE-03 is closed behaviorally, and the check that closes it is validated on every run.

---

## Phase decision coverage — every locked decision to the plan that implemented it

| Decision | Implemented by |
|----------|----------------|
| D-01 permanent slug `ForgeAnalogVCO` | 30-01 (operator approval), 30-05 (model factory), 30-06 (manifest entry) |
| D-02 display name `Analog VCO` | 30-06 |
| D-03 tags `Voltage-controlled oscillator` + `Waveshaper` | 30-06 |
| D-04 `version` held at `2.0.1` | 30-06 |
| D-05 additive, operator-surfaced registration | 30-01 (blocking checkpoint), 30-06 (byte-identity proof, three ways) |
| D-06 throwaway 18 HP `res/AnalogVCO.svg` | 30-05 |
| D-07 four controls only, every one live | 30-05 (declaration), **30-07 Task 3 (audible confirmation — operator "Approved")** |
| D-08 stock widgets; `src/AnalogLFO.cpp` never in the diff | 30-05 |
| D-09 no display widget | 30-05 |
| D-10 CHARACTER required for spread visibility | 30-05 (the knob), 30-03 / 30-04 (tests run at `character = 1.0`) |
| D-11 spread-only divergence, no OU stepping | 30-02 (`setSpreadSeed` copy), 30-03 (divergence case) |
| D-12 one call into frozen `Waveshape::morphedWave`, `bleedLfo = 0` | 30-02 |
| D-13 unconditioned ×5 output | 30-02 (no clamp added), 30-03 (loose 6.0 V bound, 5.51803 V observed), **30-07 (overshoot recorded as expected, not a defect)** |
| D-14 `exp2_taylor5` pitch chain, double phase, Nyquist guard | 30-01 (the guard exemption that makes the include possible), 30-02 |
| D-15 tombstone inverted in place | 30-02 |
| D-16 pitch measured on the OUTPUT, labelled not the TEST-02 gate | 30-03 |
| D-17 two-instance interleave independence | 30-04 |
| D-18 divergence case + loose magnitude bound | 30-03 |
| D-19 the two green-but-weak rows re-evidenced | 30-02 |

All nineteen locked decisions are implemented and attributed. None was silently dropped or deferred.

## Roadmap success criteria mapped to evidence

| Criterion | Evidence |
|-----------|----------|
| **1.** POD `Inputs` → `step()` → output + telemetry boundary mirroring `LfoCore`, driven headlessly | 30-02 (`src/dsp/VcoCore.hpp` body, boundary shape unchanged); `-tc="vco harness*"` **7 cases green** here, including the inverted D-15 liveness case |
| **2.** Naive aliased morphed waveform at audio rate via `exp2_taylor5`, no static/global mutable voice state | 30-02 (DSP); 30-04 (interleave independence **0/1024 both instances, three rates**, plus its permanent positive control measured **512/512 detecting** at this gate) |
| **3.** Second selectable module via `addModel` + `plugin.hpp` extern + `plugin.json` entry, LFO registration and slug untouched | 30-05, 30-06 (`nm -gU plugin.dylib` exports **both** models; LFO entry proven byte-unchanged three ways, reproduced at this gate); **30-07 Task 3 — the operator confirmed it appears and sounds, beside an unchanged LFO** |
| **4.** Same seed → bit-identical block; different seed diverges | 30-02 (D-19 determinism re-evidenced under real DSP), 30-03 (divergence 0.2332 V with 2048/2048 samples differing at `character = 1.0`, bit-identical at `character = 0`) |

All four criteria are satisfied with evidence, and criterion 3 is the only one that required a human — which is why Task 3 existed.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Task 3's stale-install flush had to be widened beyond the plan's Step 2**

- **Found during:** Task 3, automated half, immediately after `make install`.
- **Issue:** The plan's Step 2 refreshes two things — `res/` and `plugin.dylib`. The extracted install was a complete **v2.0.0 LFO-only plugin**: `plugin.json` at version `2.0.0` with a single `modules[]` entry, and `res/` without `AnalogVCO.svg`, alongside the Jul 9 dylib. Executing Step 2 literally would have satisfied the plan's own hash assertion while leaving Rack reading a one-module manifest, so the Analog VCO would not have appeared — the exact false negative the task exists to prevent, reached *through* the safeguard.
- **Fix:** Flushed the whole extracted directory from `dist/ForgeAudio-AnalogSeries/` (manifest, `res/`, LICENSE, NOTICES) with `rsync -a`, then copied the root `plugin.dylib` last so the plan's literal hash assertion holds character for character. Verified post-state: manifest `2.0.1` with both modules, `res/AnalogVCO.svg` present, both symbols exported, `codesign -v` valid.
- **Files modified:** **none in the repository.** This touched only the operator's Rack install directory, which the plan's own `<files>` declares is not committed. `files_modified: []` holds; `git status --porcelain` empty.
- **Commit:** n/a — no source change.
- **Impact on plan:** None on substance; it made the human half start from a genuinely known-good install rather than a partially-refreshed one. Recorded above as a **durable rule** for Phases 31–35, each of which ends in an in-Rack check.

**2. [Rule 3 - Blocking] The `[1/7]` exactness confirmation was made by reading the guard, not by a fixture**

- **Found during:** Task 1, at the deferred-item review.
- **Issue:** 30-01 and 30-03 each proved an exemption's width by creating a near-miss fixture, running the guard, and deleting it. This plan's hard prohibition 1 forbids modifying any file, and its acceptance criteria require `git status --porcelain` empty.
- **Fix:** Confirmed exactness by reading `tests/check_includes.sh:293-294` directly — `[[ "${rel}" == "${a}" ]]` with a quoted RHS is a literal comparison — and by citing 30-03's already-recorded by-construction proof. This is direct evidence about the matching code itself, which is what a fixture demonstrates indirectly.
- **Files modified:** none.
- **Commit:** n/a.
- **Impact on plan:** None. The judgement is recorded in full above.

---

**Total deviations:** 2, both Rule 3, neither touching a repository file.
**Impact on plan:** No task was skipped, no acceptance criterion was relaxed, no gate was patched under its own authority, and `files_modified: []` held exactly as designed.

## Issues Encountered

None beyond the two deviations above.

One note for the record: `make RACK_DIR=../Rack-SDK` reported `Nothing to be done for 'all'` on its first invocation, because `plugin.dylib` was already current with `HEAD` from 30-06's build. Confirmed genuinely current rather than assumed — `nm -gU` reports both model symbols, and make's own dependency tracking covers all four objects. The SDK path is relative on purpose: the repository path contains a space, which breaks an absolute `RACK_DIR`.

## Known Stubs

None introduced — this plan ships no code at all.

Carried, unchanged, from earlier plans and stated at their sites, each owned by a named later phase rather than left silent:

- **`res/AnalogVCO.svg` is a deliberate throwaway** (D-06). Six rects, no typography. Phase 35 replaces the art wholesale at the same filename and 18 HP geometry. The operator saw it and it is meant to look like that.
- **Only three of eight `VcoInputs` fields are fed by the shell.** `coarse`, `fine`, `fmVolts`, `fmAtten`, `fmConnected` are Phase 31's; `drift` is Phase 34's. `check_canary.sh [2b/5]` keeps all eight runtime-live meanwhile — confirmed reporting **8** at this gate, not 3.
- **`kVcoNyquistGuardFrac = 0.49f` is PROVISIONAL**, with PITCH-04 (Phase 31) named as owner at its site.
- **The output is unconditioned by decision** (D-13). Phase 34's OUT-01..03 own the DC blocker and saturation.

**The oscillator is crude and aliased on purpose.** Phase 32 owns CORE-02 / AA-01..05. Nothing in this phase asserts or should be judged on spectral cleanliness, and the operator raised no observation to the contrary.

## Threat Flags

None. This plan adds no network endpoint, no auth path, no file-access pattern and no schema change, and installs zero packages. The five threats the plan's `<threat_model>` assigns to it:

- **T-30-12** (the phase recorded as verified on local evidence alone) — **MITIGATED, and this is the plan's whole purpose.** The `toolchain-gate` job **and** its `win-x64 leg reproduction` step were both read individually and observed `success` on run `30407971115`, whose head SHA was asserted equal to `7933fae36ad98882ac8964f17d6c1b15f60087fd`. The three ways this is usually fudged were all forbidden and all avoided: green local gates were explicitly recorded as a precondition and not closure; the step's own conclusion was read rather than inferred from the job not failing; and the run was located **by SHA**, never by recency.
- **T-30-09** (a C++11 / ODR link-class defect surviving every local gate) — **MITIGATED.** The MinGW leg compiled all four translation units at `-std=c++11 -O3` and linked them against `libRack` with full symbol resolution, printing `win-x64 link gate: PASS`. `src/AnalogVCO.cpp` is a brand-new TU and `src/plugin.cpp.o` carries a new cross-TU reference to `modelAnalogVCO` — the exact surface research Pitfall 4 names.
- **T-30-13** (a stale extracted install misdiagnosed as a registration bug) — **MITIGATED, and the threat MATERIALISED and was caught.** The install was a whole stale v2.0.0 plugin. Hashes were compared before any judgement, no source was inspected, and the flush was widened once the manifest was found stale too. The durable rule is recorded above.
- **T-30-04** (VCO code entering the shipped LFO's build graph, or the shipped module regressing) — **MITIGATED.** The whole tripwire set re-ran green: golden replay 6/6 byte-identical at 49,164 assertions, frozen manifest at 15 entries with no bump, both `[2/7]` control directions firing, the canary's 8-field constant-fold report intact, and the phase's entire diff naming no LFO shell, no LFO panel, no frozen header and neither block driver. Closed by a human confirming the LFO unchanged in the same Rack session.
- **T-30-SC** (supply chain) — **not applicable.** This plan installs no packages; `30-RESEARCH.md` § Package Legitimacy Audit records the whole phase as adding zero external dependencies.

## User Setup Required

None. The Analog VCO is installed and confirmed working in the operator's Rack at `~/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio-AnalogSeries/`, manifest version `2.0.1`, two modules.

## Next Phase Readiness

- **Phase 30 is closed on the evidence the project learned it cannot get locally.** The standing rule — no phase closes on local evidence alone — is satisfied for the first time in this phase, on the only gate that has ever caught this project's rejection-class defect.
- **Phase 36 has the record it needs for REL-01.** SHA `7933fae36ad98882ac8964f17d6c1b15f60087fd`, run `30407971115`, URL above, `toolchain-gate` = `success`, `win-x64 leg reproduction` step's own conclusion = `success`, three-OS matrix green. The standing rule applies there too: a tag is only as good as the run observed on **its** commit, so Phase 36 must re-observe on whatever commit it tags, not reuse this one. Nothing here tagged, released, bumped `plugin.json` or touched issue #929.
- **Phase 32 starts from a confirmed crude aliased baseline.** The operator raised **no** timbre observation — recorded explicitly so Phase 32 reads that as designed-and-unremarked rather than as an unreported problem. Its input figures: worst-case magnitude 5.51803 V, the 6.0 V loose bound, and the D-16 pitch grid at 1 %.
- **Phase 34 inherits no reported level problem.** The D-13 overshoot stands as an expected, measured property; OUT-01..03 own conditioning and the measured ceiling (5.55 V analytic, 5.51803 V observed) is well inside Rack's ±12 V norms.
- **Phase 31 inherits a clean surface.** `kVcoNyquistGuardFrac` is marked PROVISIONAL with PITCH-04 named as owner, the D-16 case is labelled NOT the TEST-02 gate in both the banner and the registered case name, and five `VcoInputs` fields are wired but unread, waiting.
- **Phase 35 inherits an art swap.** `res/AnalogVCO.svg` is the final filename at the final 18 HP geometry; the four control coordinates are in `30-05-SUMMARY.md`.
- **Every future in-Rack UAT should use the whole-tree flush** recorded under Task 3. Phases 31–35 all end in one.
- **Assumption A5 is closed** — the tolerances hold under GCC/libstdc++ and MinGW g++, not only Apple clang.
- **All four deferred/flagged items are resolved:** the `[1/7]` entry confirmed exact-path, PANEL-03 confirmed genuinely satisfied, the `plugin.json` diff-shape discrepancy reproduced and judged, and invariant 5 confirmed green **by detecting**.
- No blockers.

## Self-Check: PASSED

- `.planning/phases/30-vcocore-skeleton-module-registration/30-07-SUMMARY.md` — FOUND on disk.
- `tests/check_includes.sh`, `tests/test_vco_core.cpp`, `src/AnalogVCO.cpp`, `res/AnalogVCO.svg`, `src/plugin.hpp`, `src/plugin.cpp`, `plugin.json` — all FOUND on disk, all unmodified by this plan.
- CI run `30407971115` — FOUND, head SHA `7933fae36ad98882ac8964f17d6c1b15f60087fd`, conclusion `success`.
- Commit `7933fae36ad98882ac8964f17d6c1b15f60087fd` — FOUND in `git log --oneline --all`, and confirmed pushed to `origin/main`.
- No task commit exists for this plan **by design** — `files_modified: []`; the only commit is the plan-metadata `docs(30-07)` commit following this SUMMARY.
- `git status --porcelain` — empty before Task 1, after Task 1, and after Task 3's build and install.
- No file was deleted by this plan.

---
*Phase: 30-vcocore-skeleton-module-registration*
*Completed: 2026-07-29*
