---
phase: 29-vco-test-harness-lfo-non-regression-guardrail
plan: 04
subsystem: build-ci
tags: [frozen-manifest, sha256, dependency-direction, odr, guard-script, negative-control, ci, makefile, crlf]

# Dependency graph
requires:
  - phase: 29-01
    provides: "src/dsp/VcoCore.hpp — the VCO header whose include hygiene [2/7] and [3/7] assert; tests/BlockDriver.hpp identified as an R-2 manifest entry"
  - phase: 29-02
    provides: "tests/golden/SHA256SUMS — the fixture manifest re-verified by check_frozen.sh [2/3]; tests/test_golden.cpp identified as an R-3 manifest entry"
  - phase: 29-03
    provides: "tests/check_canary.sh — the third GUARD_SCRIPTS entry; src/vco_compile_canary.cpp — the sanctioned src/->VCO include direction the audit must not flag"
  - phase: 27-user-manual
    provides: "tests/check_docs.sh — the guard-script scaffold copied by both new scripts, and the uninvoked-guard failure mode [7/7] now gates against"
provides:
  - "src/dsp/FROZEN.sha256 — 15 hash-pinned repo-root-relative paths covering the shipped LFO's entire behavioral surface"
  - "tests/check_frozen.sh — the D-05 tripwire; CR-normalized text digests, raw binary digests, permanent negative control"
  - "tests/check_includes.sh — the D-06 dependency-direction audit in 7 sections, including the standing guard-wiring gate"
  - "Makefile target `guards` and variable GUARD_SCRIPTS — one Rack-free local runner for the whole guard suite"
  - "CI steps `Frozen-header hash guard (D-05)` and `Include / dependency-direction audit (D-06)` in the toolchain-gate job"
  - ".planning/todos/pending/wire-check-docs-into-ci.md — the tracked P-5 gap"
affects: [29-05-ci-observed-negative-control, 30-vco-core-registration, 32-morph-blep, 34-drift-engine-additive-edit, 35-vco-shell]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Frozen-source manifest + verifier: additive-only enforced mechanically, with a deliberate same-commit manifest bump as the sanctioned escape hatch"
    - "CR-stripped digests for text entries, raw digests for binary entries — one manifest that is correct on both LF and CRLF checkouts"
    - "Pure digest lines with zero comments, so the manifest stays runnable by `shasum -a 256 -c` by hand; all rationale lives in the verifier banner"
    - "Shared-detector negative control: the control invokes the SAME function the real scan invokes, so it validates the real code path rather than a parallel re-implementation"
    - "Explicit allowlist over glob for boundary scans, with the exclusions and their reasons written into the file"
    - "Guard wiring is itself a gate, with a commented exemption array — an exemption must carry a reason and a tracked todo"

key-files:
  created:
    - src/dsp/FROZEN.sha256
    - tests/check_frozen.sh
    - tests/check_includes.sh
    - .planning/todos/pending/wire-check-docs-into-ci.md
  modified:
    - Makefile
    - .github/workflows/test.yml

key-decisions:
  - "The manifest is a strict superset of D-05's four named headers: all eleven LFO closure headers plus src/AnalogLFO.cpp, tests/BlockDriver.hpp, tests/test_golden.cpp and tests/golden/freerun_seeds.txt. PatchParse.hpp, DisplayFill.hpp and Anim.hpp have zero golden coverage and are the named VCO-shell reuse candidates — a literal four-header manifest would leave the likeliest silent-regression surface unguarded."
  - "plugin.json, src/plugin.cpp and src/plugin.hpp are deliberately NOT pinned: Phase 30 legitimately edits all three and is already guardrailed, so pinning them would manufacture a mandatory bump and train a reader to bump without thinking."
  - "The manifest contains pure digest lines with no comment lines, so `shasum -a 256 -c src/dsp/FROZEN.sha256` works by hand; the entire rationale lives in the verifier banner instead."
  - "check_frozen.sh does NOT use the checksum tool's own -c check mode: that mode reads raw bytes and cannot normalize, so on a CRLF checkout every text entry would false-fail (P-3)."
  - "Hasher detection order is shasum -> sha256sum -> openssl, macOS-first, with a hard failure if none is present. Each wrapper prints only the 64-hex digest, and uses awk rather than head to avoid a SIGPIPE under `set -o pipefail`."
  - "[5/7]'s filename rule was scoped to C/C++ source files. The plan's literal wording (any file under src/ whose name contains `ha256`) would have failed on src/dsp/FROZEN.sha256, the very artifact the rule exists to support. The unrestricted 0x6a09e667 constant check still catches an implementation under any filename."
  - "The negative controls call the same functions the real scans call — detect_vco_includes in [6/7], hash_norm in [3/3] — so a broken detector cannot pass its own control."
  - "check_docs.sh is recorded as a documented exemption plus a tracked todo rather than being wired in this phase (out of scope) or silently ignored."

requirements-completed: [TEST-04, TEST-06]

coverage:
  - id: D1
    description: "D-05 — a checked-in SHA-256 manifest pins the frozen shared headers and CI hard-fails on any un-bumped edit"
    requirement: "TEST-04"
    verification:
      - kind: other
        ref: "wc -l < src/dsp/FROZEN.sha256 -> 15; awk '{print $2}' | sort -u | wc -l -> 15 (fifteen distinct paths)"
        status: pass
      - kind: other
        ref: "shasum -a 256 -c src/dsp/FROZEN.sha256 from the repo root -> exit 0, OK for all 15"
        status: pass
      - kind: other
        ref: "research digests unchanged: Waveshape.hpp e8ae0700... and DriftEngine.hpp 698146fd... each grep -c -> 1; RackCompat.hpp and MathConst.hpp also matched exactly"
        status: pass
      - kind: integration
        ref: "CI step 'Frozen-header hash guard (D-05)' -> bash tests/check_frozen.sh (grep -c -> 1)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The frozen guard demonstrably goes RED — it is validated, not merely green"
    requirement: "TEST-04"
    verification:
      - kind: integration
        ref: "tests/check_frozen.sh [3/3] permanent negative control — a perturbed scratch copy of MathConst.hpp hashes 4a5bba22... vs manifest 091eba70..., detected on every run"
        status: pass
      - kind: other
        ref: "HAND-RUN RED DEMO: manifest copied to a scratch checkout with one digest digit corrupted (e8ae0700 -> e8ae0701); script run against the copy -> OBSERVED EXIT 1 with 'FROZEN FILE CHANGED: src/dsp/Waveshape.hpp' plus expected/actual digests and the bump instruction. Scratch discarded; the real manifest and MathConst.hpp were never touched."
        status: pass
    human_judgment: false
  - id: D3
    description: "D-06 — dependency direction, VCO Rack-freedom, the single-forge::Inputs ODR rule and hasher placement are mechanically enforced"
    requirement: "TEST-04"
    verification:
      - kind: integration
        ref: "check_includes.sh [1/7] 25 LFO-side files scanned, zero Vco/MorphBlep includes"
        status: pass
      - kind: integration
        ref: "[2/7] src/dsp/VcoCore.hpp has no Rack include; [3/7] its includes are <cstdint> plus \"dsp/DriftEngine.hpp\" only"
        status: pass
      - kind: integration
        ref: "[4/7] exactly one 'struct Inputs' under src/, in src/dsp/LfoCore.hpp (the VCO POD is forge::VcoInputs)"
        status: pass
      - kind: integration
        ref: "[5/7] no SHA-256 source file and no 0x6a09e667 constant under src/"
        status: pass
      - kind: other
        ref: "bash tests/check_includes.sh | grep -cE '^\\[[1-7]/7\\]' -> 7"
        status: pass
    human_judgment: false
  - id: D4
    description: "The include audit demonstrably goes RED, via the same detector the real scan uses"
    requirement: "TEST-04"
    verification:
      - kind: integration
        ref: "[6/7] permanent negative control — a synthetic TU including dsp/VcoCore.hpp is reported as a hit by detect_vco_includes, the same function [1/7] calls"
        status: pass
      - kind: other
        ref: "HAND-RUN RED DEMO: scratch checkout with '#include \"dsp/VcoCore.hpp\"' appended to a copy of src/dsp/LfoCore.hpp -> OBSERVED EXIT 1, [1/7] reported 'src/dsp/LfoCore.hpp:253:#include \"dsp/VcoCore.hpp\"' under 'VCO header(s) reached the LFO build graph'. Scratch discarded; no working-tree file modified."
        status: pass
    human_judgment: false
  - id: D5
    description: "P-5 — every guard is invoked, locally and in CI, and a standing audit keeps it that way"
    requirement: "TEST-06"
    verification:
      - kind: other
        ref: "make guards -> exit 0, runs all three scripts, prints 'guard suite: PASS'"
        status: pass
      - kind: other
        ref: "make guards RACK_DIR=/nonexistent-rack-sdk -> exit 0 (R-11 / P-6: the goal joined the plugin.mk skip filter)"
        status: pass
      - kind: integration
        ref: "[7/7] reports check_canary.sh, check_frozen.sh and check_includes.sh as wired, check_docs.sh as the single documented EXEMPT entry"
        status: pass
      - kind: other
        ref: "ruby YAML parse — toolchain-gate 6 -> 8 steps by pure append, last two names exact; test job unchanged at 3 steps"
        status: pass
    human_judgment: false
  - id: D6
    description: "R-4 / R-5 — TEST_CXXFLAGS byte-identical in both copies, every CI change appended, shipped LFO files identical to v2.0.1"
    verification:
      - kind: other
        ref: "grep -cF 'TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -I$(TEST_DIR) -Wall -Wextra -ffp-contract=off' Makefile -> 1"
        status: pass
      - kind: other
        ref: "git diff -- .github/workflows/test.yml | grep -c '^-[^-]' -> 0; | grep -c 'ffp-contract' -> 0"
        status: pass
      - kind: other
        ref: "git diff --exit-code v2.0.1 over AnalogLFO.cpp, plugin.cpp/hpp, plugin.json, BlockDriver.hpp, test_golden.cpp, the six .f32 goldens, freerun_seeds.txt and all 11 frozen dsp headers -> exit 0"
        status: pass
      - kind: other
        ref: "make test -> 64/64 passed, 0 failed, 2,615,099 assertions; make strict -> exit 0; make (plugin build) -> exit 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "P-3 — text digests are correct on a CRLF checkout as well as on the LF runners"
    verification:
      - kind: other
        ref: "check_frozen.sh [1/3] pipes each text entry through `tr -d '\\r'` before hashing and does NOT use the checksum tool's -c mode; on this LF checkout the normalized digests equal the raw ones, proven by `shasum -a 256 -c` also exiting 0 against the same manifest"
        status: pass
    human_judgment: true
    rationale: "No Windows CRLF runner was available to observe the CRLF case directly. The LF-equivalence half is proven mechanically (both the normalizing verifier and the raw check-mode run agree on the same file); the CRLF half rests on the property that stripping 0x0D from a CRLF checkout reproduces the LF bytes exactly. Whether that is sufficient without a Windows observation is an editorial judgment, and it is stated rather than implied."

# Metrics
duration: 8 min
completed: 2026-07-28
status: complete
---

# Phase 29 Plan 04: Frozen-Header Hash Guard, Dependency-Direction Audit and Guard Wiring Summary

**`src/dsp/FROZEN.sha256` hash-pins the shipped LFO's entire behavioral surface — all eleven closure headers, the shell, and the two test files that were previously their own only witnesses — while `tests/check_includes.sh` makes the VCO→LFO dependency boundary one-way by construction; both guards prove they go red on every single run, and `make guards` plus two appended CI steps mean nothing in the suite can quietly stop being invoked.**

## Performance

- **Duration:** 8 min
- **Started:** 2026-07-28T07:28:44Z
- **Completed:** 2026-07-28T07:36:40Z
- **Tasks:** 3
- **Files:** 4 created, 2 modified (+652 lines, −2 lines)

## Accomplishments

- **D-05 is now mechanical.** Fifteen files are pinned by digest. Before this plan, "the shared headers are frozen" was a sentence in a ROADMAP. It is now a gate that runs on every push and in one local command, with a bump protocol that makes the sanctioned exception (Phase 34's `DriftEngine.hpp` edit) a visible one-line diff in the same commit rather than invisible byte drift.
- **The manifest closes the milestone's real gap, not the nominal one.** D-05 names four headers. The LFO's behavioral include closure is eleven, and three of the seven unnamed ones — `PatchParse.hpp`, `DisplayFill.hpp`, `Anim.hpp` — have **zero golden-fixture coverage** and are exactly the headers flagged as VCO-shell reuse candidates for Phase 35. A literal four-header manifest would have pinned the well-tested files and left the untested, most-likely-to-be-touched ones open. The superset and its reasoning are written into the verifier banner so a future reader sees why the file is larger than the decision's text, and every extra entry has the same one-line escape hatch D-05 already defines.
- **`tests/BlockDriver.hpp` and `tests/test_golden.cpp` are no longer unwitnessed.** They verify the goldens; nothing verified them. A subtle edit to either could have made a golden comparison vacuously pass. Both are now pinned (R-2, R-3).
- **Both guards are validated by an observed red, not by a green run** — twice each: a permanent negative control that runs on every invocation, plus a one-off hand-run demonstration against a scratch checkout. Observed exit codes are recorded below.
- **The negative controls exercise the real code path.** `[6/7]` calls `detect_vco_includes`, the same function `[1/7]` calls. `[3/3]` calls `hash_norm`, the same function `[1/3]` calls. A control that re-implements the check it validates proves only that two greps agree; these prove the shipped detector works.
- **The R-9 ODR trap is closed.** A second `forge::Inputs` would compile cleanly in every translation unit that sees only one definition, link without a diagnostic on Apple clang, and be undefined behavior in the field — structurally the same shape as the construct that got v2.0.0 rejected. `[4/7]` asserts exactly one, in `src/dsp/LfoCore.hpp`.
- **P-5 is now a gate rather than a hope.** `[7/7]` fails if any `tests/check_*.sh` stops being referenced by the workflow. The repository had already demonstrated this failure mode, and that finding is reported plainly below rather than being quietly absorbed.
- **`make guards` is Rack-free and proven so.** `make guards RACK_DIR=/nonexistent-rack-sdk` exits 0, which is the actual proof that the goal joined the `plugin.mk` skip filter (R-11 / P-6) — a bare `include` of `plugin.mk` hard-fails on any runner without `../Rack-SDK`.

## Task Commits

1. **Task 1: D-05 frozen manifest + CR-normalizing verifier** — `ffb2024` (test)
2. **Task 2: D-06 dependency-direction audit** — `0a1e503` (test)
3. **Task 3: guard-suite wiring — `make guards`, two CI steps, `[7/7]` audit, todo** — `fa8cb97` (chore)

## Observed Failure Demonstrations (the evidence the guards go red)

Both were run against throwaway scratch checkouts under `mktemp -d`. **No working-tree file was modified in either demonstration**, confirmed by `git status --porcelain` immediately afterwards.

| Demonstration | Method | Observed exit | Observed output |
|---|---|---|---|
| **Task 1 — frozen manifest** | Full scratch copy of `src/` and `tests/`; one digest digit corrupted in the manifest copy (`e8ae0700…` → `e8ae0701…`); script run from the scratch tree | **1** | `FAIL: FROZEN FILE CHANGED: src/dsp/Waveshape.hpp` with the expected digest, the actual digest, and the bump instruction; summary `FAIL: frozen-source gate found problems` |
| **Task 2 — include audit** | Full scratch copy; `#include "dsp/VcoCore.hpp"` appended to the scratch copy of `src/dsp/LfoCore.hpp`; script run from the scratch tree | **1** | `[1/7] FAIL: VCO header(s) reached the LFO build graph — this changes what the SHIPPED module compiles to:` followed by `src/dsp/LfoCore.hpp:253:#include "dsp/VcoCore.hpp"`; summary `FAIL: dependency-direction audit found problems` |

The two permanent, every-run controls are separate from these and also observed green: `[3/3]` reports the perturbed `MathConst.hpp` copy as `4a5bba22…` against the manifest's `091eba70…`, and `[6/7]` reports the synthetic leak fixture as a hit.

## Files Created/Modified

- **`src/dsp/FROZEN.sha256`** (created, 15 lines) — Pure `shasum -a 256` output, repo-root-relative paths, no comment lines. Order: D-05's four named headers, then the remaining seven closure headers, then `src/AnalogLFO.cpp`, then `tests/BlockDriver.hpp` and `tests/test_golden.cpp`, then `tests/golden/freerun_seeds.txt`. All four research-recorded digests matched exactly on generation.
- **`tests/check_frozen.sh`** (created, 240 lines) — `check_docs.sh` scaffold. Banner records D-05, the bump protocol naming Phase 34, the superset rationale naming the three zero-golden-coverage headers, the registration-file exclusion and its reason, and the CR-normalization note. `[1/3]` text entries CR-normalized, `[2/3]` golden fixtures raw, `[3/3]` permanent negative control that also asserts the real file was untouched. Hasher detection `shasum` → `sha256sum` → `openssl`, hard-failing if none is present.
- **`tests/check_includes.sh`** (created, 369 lines) — Seven sections. Shared `detect_vco_includes` function; explicit 25-file LFO-side allowlist with the exclusions (`src/vco_compile_canary.cpp`, Phase 30's `src/AnalogVCO.cpp`, this phase's VCO test files) and their reasons written in.
- **`Makefile`** (modified, +30 / −1) — Exactly the two sanctioned changes: skip filter `test capture` → `test capture guards` (the one replaced line, explicitly sanctioned by the plan), and an appended `GUARD_`-namespaced target block. GNU Make 3.81 compatible: plain shell `for`, doubled dollars, no `$(file …)`, no `::=`, no `.ONESHELL`.
- **`.github/workflows/test.yml`** (modified, +18 / −0) — Two appended `toolchain-gate` steps with the exact required names, each preceded by a comment explaining what the gate covers and that it needs no Rack SDK.
- **`.planning/todos/pending/wire-check-docs-into-ci.md`** (created) — The tracked P-5 gap, with the one-line fix, why it was out of scope here, and the close-out instruction.

## Decisions Made

Recorded in full in the frontmatter `key-decisions`. The three that most affect future phases:

- **Phase 34 must bump `src/dsp/DriftEngine.hpp`'s digest line in the same commit as its edit.** `tests/check_frozen.sh` will hard-fail otherwise, and the failure message says exactly this. That is the feature working, not an obstacle.
- **Phase 30 must not need a manifest bump.** `plugin.json`, `src/plugin.cpp` and `src/plugin.hpp` are deliberately unpinned for precisely that reason. If Phase 30 finds itself wanting to bump the manifest, something has gone wrong.
- **Phase 35 reuse of `PatchParse.hpp` / `DisplayFill.hpp` / `Anim.hpp` is now gated.** Those three are pinned despite having no golden coverage. Reusing them read-only is free; editing them requires a deliberate bump and a justification.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] `[5/7]`'s filename rule would have failed on the manifest this plan creates**

- **Found during:** Task 2
- **Issue:** The plan specifies "fail if any file under `src/` has a name containing `ha256`". `src/dsp/FROZEN.sha256` — created by Task 1 of this same plan, and deliberately placed beside the headers it pins — contains that substring. Implemented literally, the D-06 audit would have hard-failed on the D-05 artifact, and the two halves of this plan would have been mutually exclusive.
- **Fix:** The filename check is scoped to C/C++ source files (`*.cpp`, `*.hpp`, `*.h`, `*.cc`, `*.cxx`). The rule's actual subject is a hashing *implementation* entering the shipped C++11 build graph; a data manifest is not one. The second half of the section — the unrestricted `0x6a09e667` initial-state constant grep across all of `src/` — is unchanged and still catches an implementation smuggled in under any filename whatsoever. The scoping and its reason are written into the script as a "Scope note (a deliberate refinement of the rule as written)" so the narrowing is visible rather than looking like sloppiness.
- **Files modified:** `tests/check_includes.sh`
- **Commit:** `0a1e503`

**Total deviations:** 1

No Rule 2, Rule 3 or Rule 4 condition arose. No package was installed. No architectural question surfaced.

## Findings Reported Plainly

**`tests/check_docs.sh` has been an uninvoked gate since Phase 27, and the new `[7/7]` audit confirms it.** This was predicted by plan 29-03's handoff and is a real finding about this repository, not a defect in this plan's work. It is a complete, correct, currently-passing documentation gate (brand denylist, section-file existence, code-fact tokens against `src/AnalogLFO.cpp`) that neither the `Makefile` nor the workflow references, and never has. It has therefore delivered zero assurance for its entire existence while looking, to anyone browsing `tests/`, like an active guard.

Handled per the plan's hard prohibition 5: **not** wired here (it is a Phase 27 artifact, unrelated to the LFO guardrail, and wiring it would be scope creep in a phase with a published-module risk profile). Instead it is the single entry in `GUARD_WIRING_EXEMPT`, printed as `EXEMPT:` on every run with a pointer to `.planning/todos/pending/wire-check-docs-into-ci.md`, so the gap is loud rather than silent. Closing it is one CI step, documented in the todo.

## LFO Non-Regression Guardrail

This plan modified `Makefile` and `.github/workflows/test.yml` — shared build/CI infrastructure the shipped LFO depends on. Verified before the commits landed:

| Risk | Status |
|---|---|
| R-4 — `TEST_CXXFLAGS` changed (would move golden float results) | **Untouched in both copies.** `grep -cF` on the exact 79-character flags line → 1; the workflow's verbatim duplicate at line 38 is untouched (`git diff … \| grep -c 'ffp-contract'` → 0). |
| R-5 — an existing CI step weakened | **0 removed lines.** Ruby YAML parse confirms `toolchain-gate` went 6 → 8 steps by pure append with the two new names exact, and the `test` job is unchanged at 3 steps. |
| R-6 — `make capture` regenerating goldens | Never run. The six `.f32` fixtures and `freerun_seeds.txt` are byte-identical to `v2.0.1` and now hash-pinned twice over. |
| R-11 / P-6 — a new make goal that needs a Rack SDK | `make guards RACK_DIR=/nonexistent-rack-sdk` → exit 0. The goal is in the `plugin.mk` skip filter. |
| Existing goals broken by the filter change | `make` → exit 0, `make test` → exit 0, `make strict` → exit 0, `make capture` untouched. |
| Shipped-LFO behavior | `make test` → **64 cases / 2,615,099 assertions / 0 failed** after every task. |
| Shipped LFO files vs `v2.0.1` | `git diff --exit-code v2.0.1` over `AnalogLFO.cpp`, `plugin.cpp`, `plugin.hpp`, `plugin.json`, `BlockDriver.hpp`, `test_golden.cpp`, the six `.f32` goldens, `freerun_seeds.txt` and all eleven frozen `src/dsp/*.hpp` → **exit 0**. |

One note for the operator, since it is easy to misread: `git diff v2.0.1 -- tests/golden` (the broad directory form used in the plan's `<verification>` block, as opposed to the enumerated form in the Task 3 acceptance criteria) reports **one added file**, `tests/golden/SHA256SUMS`. That is plan 29-02's fixture manifest — a pure addition alongside the goldens, not a modification of any of them. The enumerated form over the six `.f32` files and `freerun_seeds.txt` exits 0, and `git diff --name-status v2.0.1 -- tests/golden` shows exactly one line: `A  tests/golden/SHA256SUMS`.

## Threat Flags

None. No new network endpoint, auth path, file-access pattern, or schema change at a trust boundary. The register's `mitigate` dispositions were implemented as specified:

| Threat ID | Status |
|---|---|
| T-29-16 (frozen headers tampered) | Mitigated — 15-entry manifest, CI step + `make guards`, deliberate-bump protocol stated in the failure message itself. |
| T-29-17 (`PatchParse`/`DisplayFill`/`Anim` — zero golden coverage) | Mitigated — all three pinned by the superset manifest. |
| T-29-18 (second `forge::Inputs`, ODR) | Mitigated — `[4/7]` asserts exactly one declaration and names `forge::VcoInputs` as the correct alternative in its failure text. |
| T-29-19 (VCO header in an LFO TU) | Mitigated — `[1/7]` over a 25-file explicit allowlist, validated by `[6/7]` and by an observed exit 1. |
| T-29-20 (a guard nothing invokes) | Mitigated — `[7/7]` standing audit, `make guards` local runner, `check_docs.sh` exemption documented in code **and** tracked as a todo. |
| T-29-21 (CRLF changing text digests) | Mitigated — CR-stripped digests for text, raw for binary; the checksum tool's own `-c` mode deliberately not used because it cannot normalize. Not observed on a Windows runner (see coverage D7 `human_judgment`). |
| T-29-22 (new Rack-free goal outside the skip filter) | Mitigated — filter extended in the same edit, proven by `make guards RACK_DIR=/nonexistent-rack-sdk` → exit 0. |
| T-29-23 / T-29-SC | Accepted as planned — guards read repository files, write only into `mktemp -d` scratch removed by an `EXIT` trap; no packages installed, no CI dependency added. |

## Known Stubs

None. Every artifact is fully implemented, invoked by CI and by `make guards`, and exercised in the same commit range.

The `GUARD_WIRING_EXEMPT` array is not a stub — it is a working, documented exemption mechanism with exactly one entry whose reason and tracking file are both recorded. `[7/7]` reports it on every run rather than skipping it.

## Issues Encountered

One, resolved and documented above as the single deviation: the plan's literal `[5/7]` filename wording collided with this plan's own `src/dsp/FROZEN.sha256`.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Ready for plan 29-05.** All three guard scripts are green, wired into `toolchain-gate`, and runnable in one command via `make guards`. When 29-05 temporarily perturbs a VCO header to observe the CI MinGW link leg go red, `tests/check_frozen.sh` will **not** interfere — `src/dsp/VcoCore.hpp` is deliberately not in the frozen manifest (it is still mutable through Phase 30). If 29-05 perturbs a *frozen* file instead, the frozen guard will fire first, which is correct behavior and should be expected.
- **Phase 30 obligation:** none from this plan. `plugin.json`, `src/plugin.cpp` and `src/plugin.hpp` are unpinned on purpose. When `src/AnalogVCO.cpp` lands, it does **not** go into `[1/7]`'s `LFO_SCAN` array — it is VCO code, and the array comment says so.
- **Phase 32 obligation:** when `src/dsp/MorphBlep.hpp` lands, `[2/7]` and `[3/7]` pick it up automatically (the collector already checks for it), so it must be Rack-free and include only `dsp/` siblings and standard headers from day one.
- **Phase 34 obligation (the important one):** the sanctioned additive edit to `src/dsp/DriftEngine.hpp` **must** update its digest line in `src/dsp/FROZEN.sha256` in the same commit. `make guards` and CI will both hard-fail until it does, and the failure message spells out the protocol.
- **Phase 35 note:** `PatchParse.hpp`, `DisplayFill.hpp` and `Anim.hpp` are pinned. Reading/reusing them is free; editing them requires a deliberate bump and a justification, which is the entire point of extending the manifest past D-05's four.
- **Any future phase adding a `tests/check_*.sh`:** wire it into `.github/workflows/test.yml` and add it to `GUARD_SCRIPTS` in the same commit, or `[7/7]` will fail the build.
- No blockers. No operator decision outstanding.

## Self-Check: PASSED

- Files claimed created exist on disk: `src/dsp/FROZEN.sha256` FOUND, `tests/check_frozen.sh` FOUND, `tests/check_includes.sh` FOUND, `.planning/todos/pending/wire-check-docs-into-ci.md` FOUND.
- Commits claimed exist in git: `ffb2024` FOUND, `0a1e503` FOUND, `fa8cb97` FOUND.
- All plan-level `<verification>` commands re-run: `make guards` exit 0 / `guard suite: PASS`; `make guards RACK_DIR=/nonexistent-rack-sdk` exit 0; `make test` exit 0 / 64 passed / 0 failed; `make strict` exit 0; `make` exit 0; both `shasum -a 256 -c` runs exit 0 from the repo root; `[1-7]/7` section count → 7; CI removed-line count → 0; enumerated `git diff --exit-code v2.0.1` → exit 0 (the broad `tests/golden` form's single added `SHA256SUMS` explained above).
- Every Task 1, Task 2 and Task 3 acceptance criterion re-evaluated and passing, including the two hand-run RED demonstrations, both recorded with their observed exit code of 1.
- Working tree clean apart from planning docs; both scratch checkouts discarded and `git status --porcelain` confirmed clean after each.

---
*Phase: 29-vco-test-harness-lfo-non-regression-guardrail*
*Completed: 2026-07-28*
