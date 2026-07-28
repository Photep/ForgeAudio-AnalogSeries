---
phase: 30-vcocore-skeleton-module-registration
plan: 01
subsystem: testing
tags: [bash, guard-script, negative-control, dependency-audit, d-05, d-06, d-14]

# Dependency graph
requires:
  - phase: 29-vco-test-harness-lfo-guardrail
    provides: "tests/check_includes.sh sections [1/7]-[7/7], tests/check_canary.sh [5b/5]'s D-05 frozen-shared-header allow-list, src/dsp/VcoCore.hpp seam, `make guards` wired into the CI toolchain-gate job"
provides:
  - "Shell function `detect_rack_sdk_includes` in tests/check_includes.sh — the [2/7] Rack-free detector, extracted from an inline grep"
  - "A single documented exact-path exemption for the quoted include \"dsp/RackCompat.hpp\", so src/dsp/VcoCore.hpp can carry D-14's mandated forge::exp2_taylor5 without `make guards` exiting 1"
  - "Two permanent negative controls inside [6/7] that validate that exemption in BOTH directions on every guard invocation"
  - "Agreement between check_includes.sh [2/7] and check_canary.sh [5b/5] about RackCompat.hpp — the two guards no longer contradict each other"
  - "Recorded operator approval of the permanent one-way-door slug ForgeAnalogVCO and its four-file registration diff (consumed by plan 30-06, audit trail for Phase 36 / VCV issue #929)"
affects: [30-02, 30-05, 30-06, 32-morph-blep, 36-release-library-update]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Detector-as-a-function: a guard section's detection logic lives in a named shell function so its negative control can run the EXACT code the section runs"
    - "Two-direction negative control: an exemption is pinned by proving BOTH that the detector still fires without it AND that it does not fire with it"

key-files:
  created: []
  modified:
    - tests/check_includes.sh

key-decisions:
  - "Operator selected option-a (exact-path exemption) over option-b (transitive-include escape hatch) for the check_includes.sh [2/7] guard weakening"
  - "Operator confirmed the permanent module slug ForgeAnalogVCO as specified — display name \"Analog VCO\", description \"Audio-rate morphing oscillator with analog character\", tags \"Voltage-controlled oscillator\" + \"Waveshaper\", plugin.json version held at 2.0.1"
  - "The exemption is exact-path (quoted \"dsp/RackCompat.hpp\"), never basename and never substring — a vendored dsp/rack.hpp, a dsp/RackSDK.hpp and any <rack...> angle include all still fail"
  - "The two exemption controls were added INSIDE the existing [6/7] section rather than as a new numbered section, so the [N/7] numbering and the [7/7] guard-wiring audit are undisturbed"

patterns-established:
  - "Detector-as-a-function: [2/7] now calls detect_rack_sdk_includes, mirroring [1/7]'s detect_vco_includes. A control that re-implements the check it validates proves only that two greps agree."
  - "Exemption controls come in pairs: the fire-direction alone still passes if the exemption is widened to a substring; the ignore-direction alone still passes if the detector is deleted. Both are required to pin the exemption to its documented width."

requirements-completed: [CORE-01, PANEL-03]

coverage:
  - id: D1
    description: "check_includes.sh [2/7] no longer false-positives on the repo's own Rack-free shim, so src/dsp/VcoCore.hpp can include \"dsp/RackCompat.hpp\" (D-14 / CORE-01) without turning the CI toolchain-gate red"
    requirement: "CORE-01"
    verification:
      - kind: integration
        ref: "throwaway fixture src/dsp/VcoTmpGoodProbe.hpp containing only #include \"dsp/RackCompat.hpp\" -> bash tests/check_includes.sh exits 0"
        status: pass
      - kind: integration
        ref: "make guards"
        status: pass
    human_judgment: false
  - id: D2
    description: "The exemption is exact-path and still fails every other Rack-named include, including a vendored lowercase sibling under dsp/ and an angle-bracket SDK include"
    requirement: "CORE-01"
    verification:
      - kind: integration
        ref: "throwaway fixture src/dsp/VcoTmpBadProbe.hpp containing #include \"dsp/rack.hpp\" -> bash tests/check_includes.sh exits non-zero"
        status: pass
      - kind: integration
        ref: "tests/check_includes.sh [6/7] control -> 'OK: [2/7] detector still reports a genuine Rack SDK include' on a <rack.hpp> fixture"
        status: pass
    human_judgment: false
  - id: D3
    description: "The exemption is validated in BOTH directions on every guard invocation by controls that call the same function [2/7] calls"
    requirement: "CORE-01"
    verification:
      - kind: integration
        ref: "bash tests/check_includes.sh | grep -c '\\[2/7\\] detector' -> 2"
        status: pass
      - kind: integration
        ref: "mutant with the exemption widened to a bare [Rr]ack substring -> guard exits 1 at the fire-direction control"
        status: pass
      - kind: integration
        ref: "mutant with the exemption removed -> guard exits 1 at the ignore-direction control"
        status: pass
    human_judgment: false
  - id: D4
    description: "No existing section, control or detector was weakened, renumbered or removed; the guard suite stays Rack-free and the shipped LFO is untouched"
    verification:
      - kind: integration
        ref: "bash tests/check_includes.sh | grep -c 'TWO-HOP violation detected' -> 1; grep -c '^echo \"\\[6/7\\]' and '^echo \"\\[7/7\\]' -> 1 each"
        status: pass
      - kind: integration
        ref: "make guards RACK_DIR=/nonexistent-rack-sdk -> exit 0"
        status: pass
      - kind: unit
        ref: "make test -> 67/67 doctest cases, 2,615,121 assertions, 0 failed"
        status: pass
      - kind: integration
        ref: "make strict -> strict C++11 gate: PASS"
        status: pass
    human_judgment: false
  - id: D5
    description: "The permanent one-way-door slug ForgeAnalogVCO, its display name, tags and the 2.0.1 version hold were approved by the operator on the same surface as the guard weakening, before any Phase 30 commit landed (D-05 / PANEL-03)"
    requirement: "PANEL-03"
    verification: []
    human_judgment: true
    rationale: "A permanent user-facing identifier written into every patch that ever contains the module. Nothing automatable can decide it; the acceptance criterion is an operator statement, recorded verbatim below."

# Metrics
duration: 9 min
completed: 2026-07-29
status: complete
---

# Phase 30 Plan 01: Guard-Fix — `[2/7]` Rack-Free Exemption Summary

**`check_includes.sh [2/7]` now runs through a named `detect_rack_sdk_includes` function carrying one exact-path exemption for the repo's own Rack-free shim, pinned in both directions by two permanent `[6/7]` controls — unblocking D-14's mandated `forge::exp2_taylor5` in `VcoCore.hpp` without leaving the narrowing unvalidated.**

## Performance

- **Duration:** 9 min
- **Started:** 2026-07-28T21:49Z
- **Completed:** 2026-07-28T21:58Z
- **Tasks:** 3 (Task 1 checkpoint pre-resolved by the orchestrator; Tasks 2 and 3 executed)
- **Files modified:** 1

## Operator Decision — Task 1 (recorded verbatim)

The plan's `<output>` block requires this recorded verbatim, because plan 30-06 acts on approval (B) and Phase 36 needs the audit trail when VCV library issue #929 is updated. The working tree was verified clean (`git status --porcelain tests/check_includes.sh src plugin.json` returned empty) at the moment the checkpoint was presented, so no Phase 30 commit preceded the approval.

**(A) Guard fix — selection: `option-a: exact-path exemption (Recommended)`**

> "Extract the [2/7] detector into detect_rack_sdk_includes(), exempt exactly one path — "dsp/RackCompat.hpp" — and ship a two-direction negative control in [6/7] in the same commit."

**(B) Slug — selection: `Confirmed as specified`**

> "Slug ForgeAnalogVCO, display name "Analog VCO", description "Audio-rate morphing oscillator with analog character", tags "Voltage-controlled oscillator" + "Waveshaper", plugin.json version held at 2.0.1. Mirrors the shipped ForgeAnalogLFO exactly. This is a one-way door."

Because `option-a` was selected rather than `option-b`, plans 30-02 through 30-07 stand as planned — no re-planning is required.

## Accomplishments

- Extracted `[2/7]`'s inline detector regex into a named function `detect_rack_sdk_includes`, placed beside the existing `detect_vco_includes` so both detectors sit together. The regex itself is byte-unchanged — this is a refactor plus one filter, not a retune.
- Added a single exact-path exemption for the quoted include `"dsp/RackCompat.hpp"`, with the reasoning written in place: it is the repository's own **Rack-FREE** compatibility shim, byte-pinned by `check_frozen.sh`, containing zero Rack includes, and already allow-listed by name in `check_canary.sh [5b/5]`'s four-entry D-05 list. Its *filename* merely carries a substring the detector cannot tell apart from a real SDK include.
- Ended a live contradiction between two standing guards: `[2/7]` and `[5b/5]` now agree about `RackCompat.hpp` instead of disagreeing about the same include line.
- Added two permanent negative controls inside the existing `[6/7]` section, both running the **same** `detect_rack_sdk_includes` function `[2/7]` calls — one requiring a hit on `<rack.hpp>`, one requiring silence on the exempted shim. Each emits a greppable `[2/7] detector` token.
- Mutation-proved both controls actually bite (see Verification Evidence) rather than asserting it.
- Cleared the phase's one genuinely blocking obstacle: `src/dsp/VcoCore.hpp` (plan 30-02) is now free to include what it uses, and the identical landmine waiting for Phase 32's `MorphBlep.hpp` is disarmed at the same time.

## Task Commits

Each task was committed atomically:

1. **Task 1: D-05 operator surface (guard weakening + permanent slug + registration diff)** — no commit; a `checkpoint:decision` resolved by the operator before any Phase 30 commit existed. Selections recorded verbatim above.
2. **Task 2: Extract the `[2/7]` detector into a function with an exact-path exemption** — `b3b514e` (test)
3. **Task 3: Two-direction negative control for the exemption inside `[6/7]`** — `7c72efb` (test)

**Plan metadata:** see the `docs(30-01)` commit following this SUMMARY.

## Files Created/Modified

- `tests/check_includes.sh` — new `detect_rack_sdk_includes` function with its exact-path exemption and rationale block; `[2/7]` rewritten to call it; two new exemption controls inside `[6/7]`; `Enforces:` banner entries for `[2/7]` and `[6/7]` updated; `[6/7]` heading updated to say it validates two detectors. **+124 / −16 lines across the two commits, and it is the only file the plan's commits touch.**

## Verification Evidence

Plan-level `<verification>`, run from the repository root:

| # | Check | Result |
|---|-------|--------|
| 1 | `bash tests/check_includes.sh` | exit 0; two `[2/7] detector` lines; one `TWO-HOP violation detected` |
| 2 | `make guards` | exit 0, `guard suite: PASS` (all three guard scripts) |
| 3 | `make guards RACK_DIR=/nonexistent-rack-sdk` | exit 0 — the suite stays Rack-free and runs on an SDK-less CI runner |
| 4 | `make test` | exit 0 — 67 test cases, 67 passed, 0 failed; 2,615,121 assertions |
| 5 | `make strict` | exit 0 — `strict C++11 gate: PASS` |
| 6 | `git status --porcelain src` | empty — no throwaway fixture survived |
| 7 | `git diff --stat HEAD~2 HEAD` | names exactly one file, `tests/check_includes.sh` |

Task 2's by-construction proof (fixtures created, run, then deleted):

- `src/dsp/VcoTmpGoodProbe.hpp` containing only `#include "dsp/RackCompat.hpp"` → guard exits **0** (`OK: exempted shim path is not flagged`).
- `src/dsp/VcoTmpBadProbe.hpp` containing `#include "dsp/rack.hpp"` → guard exits **non-zero** (`OK: a non-exempt Rack-named include is still detected`). Note this fixture is a *quoted, lowercase, `dsp/`-relative* sibling — the nearest miss to the exempted path — and it still fails.

Mutation evidence that the new `[6/7]` controls are load-bearing rather than decorative (run against throwaway copies of the script, deleted afterwards; the real file was never modified):

| Mutation | Expected | Observed |
|----------|----------|----------|
| Exemption widened from the exact path to a bare `[Rr]ack` substring | fire-direction control must fail the gate | exit **1** — `FAIL: the Rack-free negative control DID NOT FIRE ... the RackCompat exemption has swallowed the whole detector` |
| Exemption effectively removed (filter changed to a never-matching pattern) | ignore-direction control must fail the gate | exit **1** — `FAIL: the documented exemption is NOT in effect ...` |

Both directions are therefore demonstrated, not assumed — which is the posture the plan required and the one this repository has repeatedly insisted on.

Task-level acceptance criteria, spot-checked:

- `grep -c 'detect_rack_sdk_includes' tests/check_includes.sh` → **9** (≥ 4 required: definition, the `[2/7]` call, two control calls, plus comment references).
- `grep -c 'RackCompat' tests/check_includes.sh` → **6** (≥ 2 required).
- `bash tests/check_includes.sh | grep -c '\[2/7\] detector'` → **2** (exactly 2 required).
- `grep -c '^echo "\[6/7\]'` → **1**; `grep -c '^echo "\[7/7\]'` → **1** — numbering intact, no section added or renumbered.
- `git diff` hunk headers confirm four hunks only: the banner, the function insertion beside `detect_vco_includes`, the `[2/7]` comment block, the `[2/7]` detector call — plus Task 3's `[6/7]` edits. **Zero** changed lines touch `detect_vco_includes`, `resolve_quoted_include` or `expand_include_closure`, or the `[1/7]`, `[3/7]`, `[4/7]`, `[5/7]` or `[7/7]` section bodies.

## Decisions Made

- **Operator, (A): `option-a` — exact-path exemption.** Recorded verbatim above. Rationale on record: `VcoCore.hpp` includes what it uses (matching `LfoCore.hpp:29`, which includes `RackCompat.hpp` explicitly even though `DriftEngine.hpp` would supply it); the narrowing is validated in the same commit that introduces it; and Phase 32's `MorphBlep.hpp` is spared the identical trap.
- **Operator, (B): slug `ForgeAnalogVCO` confirmed as specified** — a one-way door, approved before any Phase 30 commit existed. Plan 30-06 is cleared to land the four-file registration diff with `plugin.json` version held at `2.0.1` (D-04), so the working tree never claims a release that was not cut.
- **Executor: the exemption filter is a literal in the function, not a configurable variable.** A list or a variable invites future entries; a single hard-coded quoted path forces any widening to be an explicit, reviewable diff on a line surrounded by the reasoning for why it is exactly one path.
- **Executor: the two exemption controls live inside `[6/7]`**, not in a new section — a new numbered section would renumber the file and disturb the `[7/7]` guard-wiring audit, which the plan forbids.
- **Executor: the section-1 and section-2 controls emit distinct greppable tokens.** The new ones carry `[2/7] detector`; the wording of the `[6/7]` heading was deliberately chosen not to contain that literal token, so `grep -c '\[2/7\] detector'` counts exactly the two controls.

## Deviations from Plan

None — plan executed exactly as written.

**Total deviations:** 0
**Impact on plan:** None. Task 1 was a pre-resolved blocking checkpoint (the orchestrator presented it and the operator replied before this executor started); Tasks 2 and 3 landed as specified, each in its own commit, touching only `tests/check_includes.sh`.

## Issues Encountered

None. One note for the record: `.planning/STATE.md` carried an uncommitted modification when this executor started — orchestrator-owned execution-start bookkeeping (`status: verifying` → `executing`, phase-name normalisation), not produced by this plan. It was deliberately left out of both task commits so that `git diff --stat HEAD~2 HEAD` names exactly one file, and is folded into the plan-metadata commit instead.

## Known Stubs

None. This plan ships no placeholder values, no empty data sources and no TODO markers. `kVcoNyquistGuardFrac` and the stub `res/AnalogVCO.svg` are provisional artifacts, but they belong to plans 30-02 and 30-05, not to this one.

## Threat Flags

None. This plan adds no network endpoint, no auth path, no file-access pattern and no schema change. It installs zero packages (`30-RESEARCH.md` § Package Legitimacy Audit records the whole phase as not applicable). The three threats the plan's `<threat_model>` assigns to it are all mitigated as written:

- **T-30-04** (VCO code entering the shipped LFO build graph) — `[1/7]` and its two existing controls are untouched; the `TWO-HOP violation detected` control is asserted still firing.
- **T-30-05** (the exemption widening into a real hole) — exact-path filter, rationale in place naming what still fails, and a permanent two-direction control that is mutation-proved to fail the gate if the detector is swallowed **or** if the exemption stops applying.
- **T-30-07** (a one-way-door identifier chosen without sign-off) — the slug was approved on the same surface as the guard weakening, before any Phase 30 commit existed, and is recorded verbatim above.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- **Plan 30-02 is unblocked.** `src/dsp/VcoCore.hpp` may now carry `#include "dsp/RackCompat.hpp"` — the only sanctioned source of `forge::exp2_taylor5` (D-14) and of `forge::clamp` — without `make guards` exiting 1 and without turning the CI `toolchain-gate` job red on the phase's first commit.
- **Plan 30-06 is cleared to proceed** on the recorded slug approval: `ForgeAnalogVCO`, display name `Analog VCO`, tags `Voltage-controlled oscillator` + `Waveshaper`, `plugin.json` version held at `2.0.1`.
- **Phase 32 benefit banked.** `MorphBlep.hpp` joins `VCO_HEADERS` the moment it lands and would have hit the identical false positive; it no longer will.
- **Phase 36 audit trail is in place** for the VCV library issue #929 update — the slug decision and its verbatim operator confirmation are recorded here.
- No blockers. The shipped LFO is untouched: no `src/` file, no golden fixture and no frozen header changed, and all six LFO goldens plus the `check_frozen.sh` manifest are green.

## Self-Check: PASSED

- `tests/check_includes.sh` — FOUND on disk.
- `.planning/phases/30-vcocore-skeleton-module-registration/30-01-SUMMARY.md` — FOUND on disk.
- Commit `b3b514e` (Task 2) — FOUND in `git log --oneline --all`.
- Commit `7c72efb` (Task 3) — FOUND in `git log --oneline --all`.
- `make guards` re-run after the STATE/ROADMAP/REQUIREMENTS updates — **PASS**.
- No files were deleted by either task commit (`git diff --diff-filter=D HEAD~2 HEAD` empty).

---
*Phase: 30-vcocore-skeleton-module-registration*
*Completed: 2026-07-29*
