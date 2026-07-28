---
phase: 30-vcocore-skeleton-module-registration
plan: 03
subsystem: testing
tags: [vco, doctest, zero-crossing-estimator, d-16, d-18a, d-18b, d-10, d-11, d-13, core-01, anti-vacuity]

# Dependency graph
requires:
  - phase: 29-vco-test-harness-lfo-guardrail
    provides: "tests/VcoBlockDriver.hpp with sweepScenario and the four-seed constructor, tests/main.cpp owning the doctest impl macro, the `make test` tests/*.cpp glob"
  - phase: 30-vcocore-skeleton-module-registration
    plan: 01
    provides: "tests/check_includes.sh [2/7] exact-path exemption and the VCO_SIDE_ALLOW list this plan appends one entry to"
  - phase: 30-vcocore-skeleton-module-registration
    plan: 02
    provides: "the live forge::VcoCore::step() body, forge::kVcoFreqC4, the Nyquist guard, and the D-11 five-coefficient setSpreadSeed — the exact code every tolerance asserted here was measured against"
provides:
  - "tests/test_vco_core.cpp — the CORE-01 behavioral suite, selectable as -tc=\"vco core*\""
  - "Anonymous-namespace helpers SAMPLE_RATES, coreBase() and estimateFreqRising(), which plan 30-04 reuses and must not redefine"
  - "D-16: naive pitch proven ON THE OUTPUT within 1 % over 5 pitches x 5 morphs x 3 characters x 3 sample rates, labelled NOT the TEST-02 tracking gate in both the file banner and the case name"
  - "D-18b: |out| <= 6.0 V over the harness sweep, a fixed worst case and hostile V/OCT, with the worst case OBSERVED at 5.51803 V so the bound is exercised rather than merely satisfied"
  - "D-18a: spread-seed divergence measured at 0.2332 V with 2048/2048 samples differing, plus the character = 0 bit-identity control that pins the Waveshape character gating"
  - "tests/check_includes.sh [1/7] VCO_SIDE_ALLOW entry for tests/test_vco_core.cpp — without it `make guards` exits 1 the moment this TU lands"
affects: [30-04, 30-07, 31-pitch-tuning-fm, 32-morph-blep, 34-analog-engine-output]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Measure the invariant on the OUTPUT, never on telemetry the same call just wrote — a telemetry assertion re-reads a number computed three lines earlier and stays green through a dead accumulator"
    - "Octave-apart expectations as a built-in non-vacuity argument: five pitch points an octave apart cannot all be satisfied by an oscillator that ignores its frequency input, so the case needs no separate control"
    - "Pair a threshold assertion with an EXERCISE assertion: `maxAbs <= bound` plus `maxAbs > 5.1f` proves the bound is approached, not merely permitted"
    - "In-test gating control: assert bit-identity at the parameter value where the mechanism is switched OFF, so a future simplification that moves the case there breaks a green test instead of producing a vacuous one"
    - "CAPTURE the measured figure, not just the verdict — doctest's `-s` then records the number on a PASS, giving later phases a value to compare CI against"
    - "INFO(stream) rather than CAPTURE for string labels: doctest stringifies a bare const char* as a POINTER"

key-files:
  created:
    - tests/test_vco_core.cpp
  modified:
    - tests/check_includes.sh

key-decisions:
  - "Executor: tests/test_vco_core.cpp had to be added to check_includes.sh [1/7]'s VCO_SIDE_ALLOW list — the section derives its LFO-side scan set as everything under src/, tests/ and tools/ MINUS the named VCO-side files, so a new VCO test TU is LFO-side by default and `make guards` exits 1 the moment it lands. The guard's own failure text names this remedy. Same kind of file and same documented reason as tests/test_vco_harness.cpp, which has been on that list since Phase 29"
  - "Executor: the researcher's Pitfall 3 figure (sweepScenario maxes at exactly 5.0000 V) is BLOCK-LENGTH DEPENDENT, measured this session. It is exactly 5.000000 V at n = 1024 and n = 0.05 s, 5.2104-5.2114 V at 0.25 s and 5.4383-5.4385 V at the 1 s block this plan specifies. Recorded in place rather than repeated as written, and it strengthens the argument for the fixed scenario: a sweep-only bound test has a margin that silently changes when someone edits the block length"
  - "Executor: scenario labels use INFO with a stream insertion, not CAPTURE — doctest stringified the bare const char* as a hex pointer address, which named every scenario `0x1025e27ff` and defeated the point of labelling them"
  - "Executor: CAPTURE(maxAbs) and CAPTURE(maxAbsDiff) were added so the measured figures appear in `-s` output on a PASS. doctest decomposes values for successful assertions under -s, so plan 30-07's phase gate has concrete numbers to compare the first CI run against (assumption A5: these were taken on Apple clang only)"

patterns-established:
  - "Anti-vacuity by construction, per case: each invariant is written against the specific measured trap that would have made it vacuous, and the trap is named in a comment at the assertion rather than in a planning document"
  - "Sensitivity proved by observed red: the divergence case was run against a setSpreadSeed reduced to the bare drift forward it replaced and OBSERVED to exit 1 with both scans failing at all three rates"

requirements-completed: [CORE-01]

coverage:
  - id: D1
    description: "Naive pitch tracks the C4 reference measured ON THE RETURNED SAMPLES within 1 % across the full measured-safe grid — 5 pitches x 5 morphs x 3 characters x 3 sample rates on a 250 ms steady tone (D-16 / CORE-01)"
    requirement: "CORE-01"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"vco core: naive pitch*\" -> 1 case, 675 assertions, 0 failed"
        status: pass
      - kind: integration
        ref: "grep -vE '^[[:space:]]*//' tests/test_vco_core.cpp | grep -c 'freqHz' -> 0; no executable line reads the telemetry frequency"
        status: pass
      - kind: integration
        ref: "grep -c 'estimateFreqRising' -> 2 (defined once, called once — not inlined per grid point)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The case cannot be mistaken for Phase 31's TEST-02 tracking gate — the label is carried in the file banner AND at the case name (D-16 / Pitfall 6)"
    requirement: "CORE-01"
    verification:
      - kind: integration
        ref: "grep -c 'TEST-02' tests/test_vco_core.cpp -> 7 (>=2 required); the case name itself ends '(NOT the TEST-02 tracking gate)'"
        status: pass
      - kind: integration
        ref: "./build-test/test -ltc | grep 'vco core:' shows the label in the registered case name"
        status: pass
    human_judgment: false
  - id: D3
    description: "|out| <= 6.0 V over the harness sweep, the fixed morph 0 / character 1 worst case and hostile V/OCT at +10 and +14 — with the worst case OBSERVED above 5.1 V and finiteness asserted alongside the bound (D-18b / T-30-01 / Pitfall 5)"
    requirement: "CORE-01"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"vco core: output magnitude*\" -s -> maxAbs := 5.51803 at 44100, 48000 and 96000; CHECK( 5.51803 > 5.1 ) passes at every rate"
        status: pass
      - kind: unit
        ref: "hostile V/OCT +10 and +14: maxAbs := 5.51803 and allFinite at all three rates — the guard 30-02 landed is observed working"
        status: pass
      - kind: integration
        ref: "grep -c '6.0f' -> 1; grep -c 'isfinite' -> 2; no assertion compares an output sample against 5.0f as an upper bound"
        status: pass
    human_judgment: false
  - id: D4
    description: "Two instances differing ONLY in spread seed diverge measurably at character = 1.0, and are bit-identical at character = 0 — the control that pins the Waveshape character gating (D-18a / D-10 / D-11)"
    requirement: "CORE-01"
    verification:
      - kind: unit
        ref: "./build-test/test -tc=\"vco core: spread seed divergence*\" -s -> maxAbsDiff := 0.233229 / 0.233235 / 0.233187 V with differing := 2048 of 2048 at 44.1 / 48 / 96 kHz"
        status: pass
      - kind: integration
        ref: "SENSITIVITY: with setSpreadSeed reduced to the bare drift forward it replaced, the case exits 1 with 6 failed assertions (both scans red at all three rates); src/dsp/VcoCore.hpp restored, git status --porcelain src/dsp empty"
        status: pass
      - kind: integration
        ref: "grep -c '0.01f' -> 1; grep -c 'character = 0' -> 8; grep -vE '^[[:space:]]*//' | grep -c 'Approx' -> 0"
        status: pass
    human_judgment: false
  - id: D5
    description: "The TU exists with its three helpers and is ready for plan 30-04 to append the CORE-03 pair with no helper conflict, and the shipped LFO is untouched"
    verification:
      - kind: unit
        ref: "make test -> 70 cases / 70 passed / 0 failed, 2,615,848 assertions (the 67-case Phase-29 baseline plus this plan's three)"
        status: pass
      - kind: unit
        ref: "./build-test/test -tc=\"golden*\" -> 6/6, 49,164 assertions — all six shipped-LFO goldens byte-identical"
        status: pass
      - kind: integration
        ref: "make strict -> PASS; make guards -> guard suite: PASS; make guards RACK_DIR=/nonexistent-rack-sdk -> exit 0"
        status: pass
      - kind: integration
        ref: "banner numbers invariants 1-3 and names 4 and 5 as plan 30-04's, so no renumbering is required when it appends"
        status: pass
    human_judgment: false
  - id: D6
    description: "The check_includes.sh [1/7] VCO_SIDE_ALLOW entry that unblocks this TU is exact-path and does not widen the boundary"
    verification:
      - kind: integration
        ref: "near-miss fixture tests/test_vco_core_probe.cpp carrying the identical include still FAILS section [1/7]; fixture deleted, git status clean"
        status: pass
      - kind: integration
        ref: "bash tests/check_includes.sh -> '29 LFO-side root file(s), 29 file(s) opened across their transitive include closure, zero VCO includes'; [6/7] one-hop and two-hop negative controls untouched and still firing"
        status: pass
    human_judgment: true
    rationale: "The guard's own comment flags its exemption list as the dangerous edit in that file, and plan 30-01 routed the analogous [2/7] change through an operator checkpoint (D-05). The change here is mechanical and precedent-matching, but a human should confirm the boundary is still where they want it. Flagged for plan 30-07's phase gate."

# Metrics
duration: 10 min
completed: 2026-07-29
status: complete
---

# Phase 30 Plan 03: CORE-01 Behavioral Suite Summary

**`tests/test_vco_core.cpp` now proves the oscillator rather than the plumbing — pitch measured on the OUTPUT within 1 % across 225 grid points, a 6.0 V bound observed being exercised at 5.51803 V and holding through a +14 V hostile V/OCT, and spread-seed divergence measured at 0.2332 V with the `character = 0` bit-identity control that keeps the case from ever becoming vacuous.**

## Performance

- **Duration:** 10 min
- **Started:** 2026-07-28T22:16:26Z
- **Completed:** 2026-07-28T22:27:07Z
- **Tasks:** 3
- **Files modified:** 2 (1 created, 1 modified)

## Accomplishments

- Created the CORE-01 behavioral TU with the three helpers plan 30-04 depends on: `SAMPLE_RATES`, `coreBase()` (default-construct + field assign, never a brace value-list, because `VcoInputs` has NSDMIs) and `estimateFreqRising()` (sub-sample-interpolated rising crossings, first-to-last).
- **Landed the pitch invariant where it can actually fail.** It reads the returned samples, never `tel.freqHz` — a telemetry assertion only re-reads the number `step(...)` computed three lines earlier and would stay green through a completely dead accumulator. `grep` over non-comment lines confirms zero executable reads of the telemetry frequency.
- Wrote the anti-vacuity argument into the case rather than into a planning document: the five pitch points have expectations a full octave apart, so an oscillator that ignored its frequency input could satisfy at most one of them, never all 225 grid points at three sample rates.
- **Observed the D-18b bound being exercised, not merely satisfied.** The fixed `morph 0 / character 1` worst case measures **5.51803 V** at all three rates against an analytic ceiling of 5.55 V, and `CHECK(maxAbs > 5.1f)` makes that overshoot a requirement. Without it the bound is decoration.
- Put the bound and finiteness side by side under hostile V/OCT, with the reason written at the site: the measured unguarded runaway is −8,655,011 V with **every sample `isfinite`**, so finiteness cannot see it. This is the observation half of the guard 30-02 implemented — and both `+10` and `+14` now measure a calm 5.51803 V.
- **Reproduced the researcher's divergence figures to six digits** — 0.233229 / 0.233235 / 0.233187 V with 2048/2048 samples differing — confirming the landed `step()` body is the one the margins were measured against.
- Pinned the measured trap with an in-test control: bit-identity at `character = 0`, where every `Waveshape` spread coefficient is gated off. A future reader who moves the case there to "simplify" it now breaks a green test instead of quietly producing one that proves nothing.
- **Proved the divergence case is sensitive rather than asserting it.** With `setSpreadSeed` spliced back down to the bare `drift` forward it replaced, the case exits 1 with six failed assertions — both scans red at all three sample rates.
- Corrected a research figure by measurement rather than repeating it (see Deviations), and recorded the correction in the source where the next reader will meet it.

## Task Commits

Each task was committed atomically, and every commit names exactly one file:

1. **Unblocking fix: register `tests/test_vco_core.cpp` on the VCO side of the `[1/7]` boundary** — `16eec8b` (fix) — `tests/check_includes.sh`
2. **Task 1: create the TU with its banner and helpers, and land the D-16 pitch invariant** — `6216e2b` (test) — `tests/test_vco_core.cpp`
3. **Task 2: the D-18b magnitude bound over sweep, fixed worst case and hostile V/OCT** — `fa3a546` (test) — `tests/test_vco_core.cpp`
4. **Task 3: the D-18a divergence invariant with its `character = 0` control** — `a7b60ce` (test) — `tests/test_vco_core.cpp`

**Plan metadata:** see the `docs(30-03)` commit following this SUMMARY.

## Files Created/Modified

- `tests/test_vco_core.cpp` — **created, 476 lines.** Banner (numbered invariants 1-3, the twice-written NOT-TEST-02 label, the "aliases by design / Phase 32 owns spectral content" prohibition, the "Deliberately NOT here" paragraph pointed at plan 30-04); three anonymous-namespace helpers; the three CORE-01 cases.
- `tests/check_includes.sh` — **+8 lines**, one `VCO_SIDE_ALLOW` entry plus its reasoning. No detector, section, control or numbering changed. See Deviations.

## Measured Results — required by the plan's `<output>` block

Recorded because plan 30-07's phase gate compares the CI figures against these, and **assumption A5 flags cross-toolchain confirmation as the first CI run's job — every number below was taken on Apple clang only.**

### Task 2 — output magnitude, `|out|` maxima (bound = 6.0 V)

| Scenario | 44.1 kHz | 48 kHz | 96 kHz |
|----------|----------|--------|--------|
| `sweepScenario`, 1 s block | 5.43849 V | 5.43849 V | 5.43829 V |
| **fixed worst case** — `morph 0.0 / character 1.0 / pitchCV 0`, 1 s | **5.51803 V** | **5.51803 V** | **5.51803 V** |
| hostile V/OCT `pitchCV = +10`, `morph 0 / character 1`, 4096 samples | 5.51803 V | 5.51803 V | 5.51803 V |
| hostile V/OCT `pitchCV = +14` | 5.51803 V | 5.51803 V | 5.51803 V |

The fixed worst case matches the researcher's 5.5180 V exactly and sits 0.6 % under the 5.55 V analytic ceiling. The two hostile-V/OCT rows are the guard 30-02 landed, observed working: unclamped, `pitchCV = +10` was measured at −8,655,011 V.

### Task 3 — spread-seed divergence at `character = 1.0`

Drift pair `(0xC0FFEE, 0xBADF00D)` held identical; spread pairs `(0x9E3779B9, 0x7F4A7C15)` vs `(0xDEADBEEF, 0xCAFEF00D)`; `morph = 0.25`, `pitchCV = 0`, n = 2048.

| | 44.1 kHz | 48 kHz | 96 kHz |
|---|----------|--------|--------|
| max abs difference | **0.233229 V** | **0.233235 V** | **0.233187 V** |
| differing samples | 2048 / 2048 | 2048 / 2048 | 2048 / 2048 |
| **control at `character = 0`** | bit-identical | bit-identical | bit-identical |

All three figures match the researcher's table to six decimal places, which is the strongest available evidence that the DSP body under test is the exact prototype the margins were measured against.

### Task 3 — sensitivity probe (required by the plan's acceptance criteria)

Procedure: delete the five `wave.*Spread = drift.*Spread;` lines from `VcoCore::setSpreadSeed`, leaving only the bare `drift.setSpreadSeed(s0, s1);` forward it replaced; rebuild; run the divergence case alone; restore from git.

**Observed — the case went RED, exit 1:**

```
tests/test_vco_core.cpp:438: ERROR: CHECK( differing > (n * 9) / 10 ) is NOT correct!
  values: CHECK( 0 >  1843 )
  logged: sr := 96000
          maxAbsDiff := 0
          differing := 0

[doctest] test cases:  1 |  0 passed | 1 failed | 69 skipped
[doctest] assertions: 18 | 12 passed | 6 failed |
```

Six failed assertions — **both** scans red at **all three** sample rates. The `character = 0` control stayed green, which is the correct result: with no spread copy the two instances are identical everywhere, so the control has nothing to distinguish. Post-probe state verified: `git status --porcelain src/dsp` empty, five coefficient lines restored, no `.bak` anywhere in the tree.

## Verification Evidence

Plan-level `<verification>`, run from the repository root:

| # | Check | Result |
|---|-------|--------|
| 1 | `make test` | exit 0 — **70 cases / 70 passed / 0 failed**, 2,615,848 assertions (the 67-case Phase-29 baseline plus this plan's three) |
| 2 | `./build-test/test -tc="vco core*"` | exit 0 — 3 cases; `-ltc \| grep -c 'vco core:'` → **3** |
| 3 | `./build-test/test -tc="vco harness*"` | exit 0 — 7 cases; plan 30-02's work undisturbed |
| 4 | `./build-test/test -tc="golden*"` | exit 0 — 6 cases, 49,164 assertions. All six shipped-LFO goldens byte-identical |
| 5 | `make strict` | exit 0 — `strict C++11 gate: PASS` (standing caveat: `-fsyntax-only`, never links) |
| 6 | `make guards` | exit 0 — `guard suite: PASS`; `make guards RACK_DIR=/nonexistent-rack-sdk` also exit 0 |
| 7 | `git status --porcelain src` | empty — the Task 3 sensitivity probe restored `src/dsp/VcoCore.hpp` from git |
| 8 | `git diff --stat HEAD~4 HEAD` | two files: `tests/test_vco_core.cpp` (+476) and `tests/check_includes.sh` (+8). Each individual commit names exactly one file |

Task-level acceptance criteria, spot-checked:

- `./build-test/test -tc="vco core: naive pitch*"` → exit 0, 1 case, **675 assertions** (≥225 required — the full 5 × 5 × 3 grid across 3 rates, two assertions per point plus the size precondition).
- `grep -c 'TEST-02'` → **7** (≥2 required). `grep -c 'estimateFreqRising'` → **2** (≥2 required).
- `grep -vE '^[[:space:]]*//' tests/test_vco_core.cpp | grep -c 'freqHz'` → **0**.
- `grep -c 'DOCTEST_CONFIG'` → **0**.
- `grep -c '6.0f'` → **1**; `grep -c 'isfinite'` → **2**; `grep -c '0.01f'` → **1**; `grep -c 'character = 0'` → **8**.
- `grep -vE '^[[:space:]]*//' tests/test_vco_core.cpp | grep -c 'Approx'` → **0**.
- `git diff --diff-filter=D --name-only HEAD~4 HEAD` → empty; no file deleted by any commit.
- Task 2's diff against Task 1 is **pure insertion** (162 insertions, 0 deletions) — the pitch case body is provably unchanged.

## Decisions Made

- **Executor: `tests/test_vco_core.cpp` had to be registered in `check_includes.sh [1/7]`'s `VCO_SIDE_ALLOW`.** See Deviations for the full reasoning and the by-construction proof that the exemption stays exact-path.
- **Executor: the researcher's "sweep maxes at exactly 5.0000 V" is block-length dependent, and the source now says so.** Measured this session across all three rates: 5.000000 V at n = 1024 and at 0.05 s, 5.2104–5.2114 V at 0.25 s, 5.4383–5.4385 V at the 1 s block this plan specifies. Writing the plan's sentence verbatim would have put a claim in the source that the same file's own block length contradicts.
- **Executor: scenario labels use `INFO` with a stream insertion, not `CAPTURE`.** doctest stringifies a bare `const char*` as a pointer, so every scenario was reported as a hex address — the opposite of "so a failure names itself".
- **Executor: `CAPTURE(maxAbs)` / `CAPTURE(maxAbsDiff)` were added deliberately.** doctest decomposes values for successful assertions under `-s`, so the measured figures are now part of the test output on a pass, not only on a failure. That is what gives plan 30-07 concrete numbers to compare the first CI run against.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `tests/test_vco_core.cpp` had to be registered on the VCO side of the `check_includes.sh [1/7]` boundary**

- **Found during:** Task 1, at the `make guards` verify step (the plan's acceptance criterion "`make guards` exits 0 — unchanged").
- **Issue:** Section `[1/7]` derives its LFO-side scan set as *everything* under `src/`, `tests/` and `tools/` **minus** the explicitly-named VCO-side files. A brand-new `tests/test_vco_core.cpp` is therefore LFO-side by default, and it includes `tests/VcoBlockDriver.hpp` → `src/dsp/VcoCore.hpp` **by construction** — the plan's entire objective. `make guards` exited 1 with `FAIL: VCO header(s) reached the LFO build graph`, naming both hops. There is no way to write this file without the include, so this blocked the plan absolutely.
- **Fix:** One entry added to `VCO_SIDE_ALLOW`, with the reason written in place. It is the same kind of file and the same documented reason as `tests/test_vco_harness.cpp`, which has been on that list since Phase 29 — and the guard's own failure text names this exact remedy: *"move the file to the VCO side of the boundary in `VCO_SIDE_ALLOW` above."*
- **Files modified:** `tests/check_includes.sh` (+8 lines: 6 comment, 1 list entry, 1 blank)
- **Verification:** Proved **by construction**, not by inspection, that the exemption stays exact-path: a near-miss fixture `tests/test_vco_core_probe.cpp` carrying the identical include still **FAILS** section `[1/7]` (the match is `[[ "${rel}" == "${a}" ]]`, exact). Fixture deleted; `git status` clean. `[1/7]` now reports *"29 LFO-side root file(s), 29 file(s) opened across their transitive include closure, zero VCO includes"*. The `[6/7]` one-hop and two-hop negative controls are untouched and still fire on every invocation, as is the `[2/7]` pair from plan 30-01. No detector, section, control or numbering changed.
- **Committed in:** `16eec8b`, deliberately kept as its own single-file commit so it is reviewable in isolation and so each task commit still names exactly one file.
- **Scope note:** this changes the LFO-side scan set by exactly one file, and that file is VCO test code that links into no LFO build graph. It does not weaken any detector and creates no LFO-regression risk. **It is nonetheless flagged for operator attention below**, because plan 30-01 routed the analogous `[2/7]` change through a checkpoint.

**2. [Rule 1 - Bug] The plan's mandated comment about the sweep maximum was factually wrong at the block length the same plan specifies**

- **Found during:** Task 2, reading the `-s` output.
- **Issue:** The plan requires a comment stating *"the sweep alone was measured to max at exactly 5.0000 V"*, from research § Pitfall 3. The measured value at the 1-second block the plan **also** specifies is **5.43849 V**. Writing the sentence verbatim would have shipped a claim the file's own code contradicts, and would have left the next reader unable to reconcile the comment with the test output.
- **Fix:** Measured the dependence directly with a throwaway probe rather than guessing: 5.000000 V at n = 1024 and n = 0.05 s, 5.2104–5.2114 V at 0.25 s, 5.4383–5.4385 V at 1 s, consistent at all three sample rates. Root cause is exactly research's own explanation — `sweepScenario` anti-correlates `morph = t` with `character = 1 − t`, so whether the accumulator reaches peak phase *while* morph is still near zero depends on how fast `t` advances, i.e. on the block length. The comment now records the measured table and draws the sharper conclusion: the sweep's margin **silently changes when anyone edits the block length**, so a sweep-only bound test can never be the evidence that the D-13 overshoot exists. That is a stronger argument for scenario two than the original sentence, not a weaker one.
- **Files modified:** `tests/test_vco_core.cpp` (comment only — no assertion changed, no threshold moved)
- **Verification:** probe compiled with the same flags as `make test` and deleted afterwards; `make test` 70/70; working tree clean.
- **Committed in:** `fa3a546` (Task 2 commit)

**3. [Rule 1 - Bug] `CAPTURE` on a scenario label reported a pointer address**

- **Found during:** Task 2, first `-s` run.
- **Issue:** The plan requires `CAPTURE`-ing "the scenario identity so a failure names itself". doctest stringifies a bare `const char*` as a **pointer**, so the output read `scenario := 0x1025e27ff` — the label was there and useless.
- **Fix:** Switched to `INFO("scenario: ...")`, whose stream insertion prints the text. The reason is recorded in a comment so nobody switches it back.
- **Files modified:** `tests/test_vco_core.cpp`
- **Verification:** `-s` now prints `scenario: fixed worst case - morph 0.0 / character 1.0 / pitchCV 0`.
- **Committed in:** `fa3a546` (Task 2 commit)

---

**Total deviations:** 3 auto-fixed (1 Rule 3 blocking, 2 Rule 1 bugs).
**Impact on plan:** No task was skipped, no acceptance criterion was relaxed, and no threshold was moved to make a test pass. Deviation 1 is the only one that touched a file outside `tests/test_vco_core.cpp`; it was unavoidable, is precedent-matching, and is surfaced for operator confirmation below. Deviations 2 and 3 improved the accuracy of the file without changing a single assertion.

## Operator Attention — one item

`tests/check_includes.sh` was modified (deviation 1). The milestone guardrail asks that guard-affecting changes be surfaced, and plan 30-01 routed the analogous `[2/7]` change through a `checkpoint:decision`. This one was **not** checkpointed, because it is a different and much narrower kind of change and it hard-blocked the plan:

- It does **not** weaken a detector. `[2/7]`'s change altered what counts as a violation; this change only states which side of an existing boundary a new file sits on.
- The list already contains `tests/test_vco_harness.cpp` for the identical documented reason, added in Phase 29.
- The match is exact-path, proved by a near-miss fixture that still fails.
- The alternative was to leave `make guards` red, which would turn the CI `toolchain-gate` red and block plans 30-04 through 30-07.

**Nothing needs to be undone for the phase to proceed.** If the operator would rather this entry not exist, the only alternative is to stop scanning `tests/` in `[1/7]` or to restructure the VCO test TU, both of which are larger changes than the one made. Recommend confirming at plan 30-07's phase gate.

## Issues Encountered

None beyond the three deviations above; each was diagnosed and resolved inside the task that surfaced it. The same-second mtime tie that 30-02 recorded did not recur — the Task 3 probe restore was followed by an explicit `touch src/dsp/VcoCore.hpp` before rebuilding, exactly as 30-02's deviation note advised.

## Known Stubs

None. This plan ships no placeholder values, no empty data sources and no TODO markers. Three things are deliberately **absent by decision** rather than stubbed, each stated in the file banner:

- No assertion about alias content, harmonic structure or spectral cleanliness — the Phase-30 oscillator aliases by design and Phase 32 owns CORE-02 / AA-01..05.
- No `< 1 cent` tracking claim — that is Phase 31's TEST-02 and needs coarse/fine/FM summing that does not exist.
- No ±5 V output-range assertion — D-13 returns the waveform unconditioned, and Phase 34's OUT-01..03 owns conditioning.

The CORE-03 interleave pair, `runInterleaveCheck()` and `DeliberatelyBrokenSharedStateCore` are **not** stubs either: they are plan 30-04's, the banner numbers them as invariants 4 and 5, and the helper namespace is laid out for them.

## Threat Flags

None. This plan adds no network endpoint, no auth path, no file-access pattern and no schema change, and installs zero packages. The threats the plan's `<threat_model>` assigns to it:

- **T-30-01** (runaway pitch accumulator under hostile V/OCT) — **mitigation OBSERVED.** Task 2's third scenario drives `pitchCV` at +10 and +14 and measures 5.51803 V with every sample finite, against an unguarded measurement of −8,655,011 V. Residual accepted as planned: a NaN `pitchCV` is deliberately not asserted on, because `forge::exp2Floor`'s `(int32_t)x` cast is UB for NaN and the header is frozen; PITCH-04 (Phase 31) hardens the correct surface.
- **T-30-03** (unbounded output voltage) — **accepted**, as decided (D-13). The 6.0 V bound is derived from the waveform's analytic maximum and is explicitly not a ±5 V output-range guarantee; the source says so at the case.
- **T-30-06** (a green test that proves nothing recorded as coverage) — **mitigated, and for the divergence case demonstrated.** Each case carries its own anti-vacuity mechanism, and the divergence case was additionally observed going red against a reverted `setSpreadSeed`.
- **T-30-SC** (supply chain) — not applicable; zero packages installed, doctest is already vendored.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- **Plan 30-04 is unblocked and has a clean surface to append to.** `tests/test_vco_core.cpp` exists with `SAMPLE_RATES`, `coreBase()` and `estimateFreqRising()` in a single anonymous namespace; the banner numbers invariants 1-3 and names 4 and 5 as 30-04's, so nothing needs renumbering. 30-04 owns `DeliberatelyBrokenSharedStateCore`, `runInterleaveCheck()` and the two CORE-03 cases, and defines no helper this plan already defines. **The `[1/7]` guard entry it also needs is already in place.**
- **The two plans were correctly kept out of the same wave** — they write the same file.
- **Plan 30-07's phase gate has concrete numbers to compare against.** Every figure in Measured Results is emitted by `-s` on a passing run, so the CI comparison is a diff of test output rather than a re-derivation. Assumption **A5 stands open**: all of it is Apple clang only, and cross-toolchain confirmation is the first CI run's job.
- **The shipped LFO is untouched.** No `src/` file, no frozen header, no `FROZEN.sha256` bump, no golden fixture, no driver that feeds one. All six LFO goldens replay byte-identical (49,164 assertions).
- **One standing caveat, unchanged:** local `make test` / `make strict` / `make guards` are all green, and Phase 29 proved that exact state is achievable on code that cannot link. No tag or library resubmission on local evidence alone.
- No blockers. One item for operator confirmation (see Operator Attention).

## Self-Check: PASSED

- `tests/test_vco_core.cpp` — FOUND on disk.
- `tests/check_includes.sh` — FOUND on disk.
- `.planning/phases/30-vcocore-skeleton-module-registration/30-03-SUMMARY.md` — FOUND on disk.
- Commit `16eec8b` (guard-list unblocking fix) — FOUND in `git log --oneline --all`.
- Commit `6216e2b` (Task 1) — FOUND in `git log --oneline --all`.
- Commit `fa3a546` (Task 2) — FOUND in `git log --oneline --all`.
- Commit `a7b60ce` (Task 3) — FOUND in `git log --oneline --all`.
- `git diff --diff-filter=D --name-only HEAD~4 HEAD` — empty; no file deleted by any commit.
- `git status --porcelain src` — empty; the Task 3 sensitivity probe left nothing behind.
- Working tree clean after all four commits; no untracked files, no `.bak`, no scratch probe.

---
*Phase: 30-vcocore-skeleton-module-registration*
*Completed: 2026-07-29*
