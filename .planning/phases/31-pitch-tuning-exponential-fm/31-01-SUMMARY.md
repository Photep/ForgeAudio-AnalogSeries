---
phase: 31-pitch-tuning-exponential-fm
plan: 01
subsystem: testing
tags: [bash, guard-script, negative-controls, dependency-direction, regex-anchoring]

# Dependency graph
requires:
  - phase: 29-lfo-non-regression-guardrail
    provides: "tests/check_includes.sh [1/7] derived scan set + VCO_SIDE_ALLOW, and the pre-registration precedent (src/AnalogVCO.cpp added before the file existed)"
  - phase: 30-vcocore-skeleton-module-registration
    provides: "deferred item 5 (WR-05) — the reproduced [2/7] exemption evasion; and the reactive tests/test_vco_core.cpp allowlist edit that D-23 exists to not repeat"
provides:
  - "tests/test_vco_pitch.cpp pre-registered in VCO_SIDE_ALLOW by exact path, with its own rationale paragraph, BEFORE the file exists — plans 31-05/31-06/31-07 land on a green `make guards`"
  - "detect_rack_sdk_includes's exemption anchored to a whole `grep -n` output line, closing WR-05"
  - "a fifth [6/7] negative control (VcoNcRackSdkCommentEvasionProbe.hpp / nc5_hits) that FIRES on the evasion shape every invocation"
  - "Phase 30 deferred item 5 carries a RESOLVED block naming this plan"
affects: [31-05, 31-06, 31-07, 31-08, 32-morph-blep, 36-release]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Guard-allowlist pre-registration: add the exact-path entry in its own reviewable task BEFORE the file exists"
    - "Exclusion filters inside a `grep -n` pipeline are anchored to the whole OUTPUT LINE, line-number prefix included"
    - "Every exemption gets negative controls in BOTH directions AND one pinning its WIDTH"

key-files:
  created: []
  modified:
    - "tests/check_includes.sh"
    - ".planning/phases/30-vcocore-skeleton-module-registration/deferred-items.md"

key-decisions:
  - "Folded Phase 30 deferred item 5 (WR-05) in rather than re-deferring it — the item's own Resolve-at named 'the next phase that touches tests/check_includes.sh', and D-23 made Phase 31 that phase"
  - "The anchored exclusion deliberately permits a trailing // or /* comment, because src/dsp/VcoCore.hpp:74 carries one on its real exempted line — forbidding it would turn [2/7] red on the LIVE VCO header"
  - "The anchor deliberately permits the `grep -n` line-number prefix, because the exclusion runs downstream of `grep -nE`"
  - "TEST-02 deliberately NOT marked complete despite appearing in this plan's frontmatter — 31-01 registers a filename, it asserts no cents error"
  - "Corrected three now-false banner claims rather than leaving them, including one that literally asserted two fixtures pinned the exemption 'to exactly the width it is documented to have'"

patterns-established:
  - "Fix-by-A/B, not fix-by-inspection: a regex hardening is accepted only after the OLD pattern is shown to MISS the fixture and the NEW pattern to HIT it, with the legitimate line still exempt under both"
  - "A guard-prose correction is part of the guard edit — a banner that overstates a detector's width is the same defect class as the widened detector"

requirements-completed: []

coverage:
  - id: D1
    description: "tests/test_vco_pitch.cpp is exempt from [1/7]'s LFO-side scan by exact path, with its rationale recorded in the script, before the file exists"
    requirement: "TEST-02"
    verification:
      - kind: integration
        ref: "bash tests/check_includes.sh — [1/7] reports 'zero VCO includes'; grep -c '\"tests/test_vco_pitch.cpp\"' == 1; grep -c 'tests/test_vco_pitch.cpp' == 2"
        status: pass
      - kind: integration
        ref: "make guards — 'guard suite: PASS'"
        status: pass
    human_judgment: false
  - id: D2
    description: "The [2/7] exemption is anchored to a whole directive rather than a whole line, so a genuine Rack SDK include can no longer hide behind a trailing comment that quotes the exempted path (WR-05)"
    verification:
      - kind: integration
        ref: "A/B of the old vs anchored pattern on two fixtures — evade.hpp: OLD silent / NEW hit; legit.hpp: silent under both"
        status: pass
      - kind: integration
        ref: "bash tests/check_includes.sh — [6/7] nc5 control fires on VcoNcRackSdkCommentEvasionProbe.hpp"
        status: pass
    human_judgment: false
  - id: D3
    description: "The four pre-existing [6/7] controls still behave — one-hop leak, two-hop leak and the genuine SDK include all fire; the exempted shim stays silent"
    verification:
      - kind: integration
        ref: "bash tests/check_includes.sh — grep -c 'TWO-HOP violation detected' == 1; grep -c 'OK: [2/7] detector ignores the one exempted path' == 1"
        status: pass
    human_judgment: false
  - id: D4
    description: "Phase 30 deferred item 5 carries a RESOLVED block naming this plan; item 6 untouched and still pointed at Phase 32"
    verification:
      - kind: integration
        ref: "grep -c 'RESOLVED' .planning/phases/30-.../deferred-items.md == 2; git diff HEAD~2 HEAD on that file shows zero content deletions"
        status: pass
    human_judgment: false
  - id: D5
    description: "The shipped LFO is untouched — no src/ file, no tests/*.cpp file, no FROZEN.sha256 in this plan's diff, and the regression floor holds"
    requirement: "TEST-02"
    verification:
      - kind: integration
        ref: "git diff --name-only HEAD~2 HEAD == {tests/check_includes.sh, .planning/.../deferred-items.md}"
        status: pass
      - kind: unit
        ref: "make test — 72 cases / 2616112 assertions / 0 failed (exactly the measured floor)"
        status: pass
    human_judgment: false

# Metrics
duration: 9min
completed: 2026-07-30
status: complete
---

# Phase 31 Plan 01: Guard Landmine & WR-05 Summary

**`tests/test_vco_pitch.cpp` pre-registered in `VCO_SIDE_ALLOW` before it exists (D-23), and `[2/7]`'s exemption re-anchored from whole output lines to whole directives with a fifth negative control that fires on the reproduced WR-05 evasion.**

## Performance

- **Duration:** 9 min
- **Started:** 2026-07-30T01:01:33Z
- **Completed:** 2026-07-30T01:10:xxZ
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- **The recurring Phase-30 landmine is disarmed before it can fire.** `tests/test_vco_pitch.cpp` is exempt from `[1/7]`'s derived LFO-side scan by exact path, carrying its own rationale paragraph, while the file itself does not yet exist. Plans 31-05/31-06/31-07 will land on a green `make guards` instead of red.
- **WR-05 is closed, and closed by observation rather than argument.** The `[2/7]` exemption used to be a bare unanchored `grep -vE`, so any output line containing the shim's include directive *anywhere* was discarded — including a line whose real directive is a genuine Rack SDK include. It is now anchored to a whole `grep -n` output line.
- **The evasion shape is now a standing control.** `[6/7]` gained a fifth fixture that must FIRE on every invocation, so a future regression of the anchor fails the gate instead of passing it quietly.
- **Three false banner claims corrected**, including one asserting that the two pre-existing Rack fixtures pinned the exemption "to exactly the width it is documented to have". WR-05 proved they did not.

## Task Commits

1. **Task 1: Pre-register `tests/test_vco_pitch.cpp` in `VCO_SIDE_ALLOW` with its own rationale (D-23)** — `2874104` (chore)
2. **Task 2: Anchor the `[2/7]` exemption filter and add the fifth `[6/7]` control (deferred item 5 / WR-05)** — `98c7cb9` (fix)

## Files Created/Modified

- `tests/check_includes.sh` — one `VCO_SIDE_ALLOW` entry + its rationale paragraph; the `detect_rack_sdk_includes` exclusion anchored + its own comment; the fifth `[6/7]` control; three banner corrections
- `.planning/phases/30-vcocore-skeleton-module-registration/deferred-items.md` — item 5 RESOLVED block naming this plan

## The exact allowlist entry

One tab-indented, double-quoted entry, placed last, five existing entries byte-unchanged:

```bash
VCO_SIDE_ALLOW=(
	"src/vco_compile_canary.cpp"
	"src/AnalogVCO.cpp"
	"tests/VcoBlockDriver.hpp"
	"tests/test_vco_harness.cpp"
	"tests/test_vco_core.cpp"
	"tests/test_vco_pitch.cpp"
)
```

The match at `tests/check_includes.sh:313` is unchanged and remains `if [[ "${rel}" == "${a}" ]]; then skip=1; fi` — a **quoted** RHS, so the comparison stays a literal exact-path test: no glob, no substring, no basename. The entry removes exactly one file from the LFO-side scan set and that file is VCO code.

## The before/after exclusion patterns

**Before** (unanchored — matches the directive text *anywhere* on the line):

```bash
| grep -vE '#[[:space:]]*include[[:space:]]*"dsp/RackCompat\.hpp"' \
```

**After** (anchored to a whole `grep -n` output line):

```bash
| grep -vE '^[0-9]+:[[:space:]]*#[[:space:]]*include[[:space:]]*"dsp/RackCompat\.hpp"[[:space:]]*((//|/\*).*)?$' \
```

Two allowances are deliberate and both are load-bearing:

- **`^[0-9]+:`** — the exclusion runs *downstream* of `grep -nE`, so every line it sees carries a line-number prefix. Anchoring to `^#` would have excluded nothing at all and turned `[2/7]` red on the live header.
- **the optional trailing `//` or `/*` comment** — `src/dsp/VcoCore.hpp:74` is literally `#include "dsp/RackCompat.hpp"    // forge::exp2_taylor5, forge::clamp`. Forbidding trailing comments would have hard-failed the live VCO header.

What the anchor forbids is any **preceding** directive, because the shim directive must now be the first non-whitespace content after the prefix. That is the whole of the WR-05 evasion.

### The fix verified by A/B, not by inspection

Both patterns run against two fixtures through the real first-stage `grep -nE`:

| Fixture | Line | OLD pattern | NEW pattern |
|---|---|---|---|
| `evade.hpp` | `#include <rack.hpp>  // superseded by #include "dsp/RackCompat.hpp"` | **silent** (evasion reproduced) | **HIT** |
| `legit.hpp` | `#include "dsp/RackCompat.hpp"    // forge::exp2_taylor5, forge::clamp` | silent | silent |

The top-left cell is the defect Phase 30's reviewer reported, reproduced independently here. The bottom row is the exemption still working in the direction it exists for (D-14 mandates the shim).

## The new control's printed hit, verbatim

```
  OK: [2/7] detector still reports a genuine Rack SDK include whose trailing comment quotes the exempted shim path (WR-05's evasion shape):
    <scratch>/VcoNcRackSdkCommentEvasionProbe.hpp:5:#include <rack.hpp>  // superseded by #include "dsp/RackCompat.hpp"
```

All five `[6/7]` controls behave as required — three fire, one stays silent, and the new one fires:

```
  OK: direct (one-hop) violation detected by the same detector [1/7] uses:
  OK: TWO-HOP violation detected through the same include closure [1/7] uses:
  OK: [2/7] detector still reports a genuine Rack SDK include:
  OK: [2/7] detector ignores the one exempted path, dsp/RackCompat.hpp
  OK: [2/7] detector still reports a genuine Rack SDK include whose trailing comment quotes the exempted shim path (WR-05's evasion shape):
```

## `make guards` output

```
=== tests/check_frozen.sh ===
PASS: frozen-source gate clean (D-05 manifest + golden fixtures + negative control).
=== tests/check_includes.sh ===
PASS: dependency-direction audit clean (D-06 + R-9 ODR + hasher placement + negative control + guard wiring).
=== tests/check_canary.sh ===
PASS: VCO compile canary guard clean (emits code + C++11 clean + C++17-isms rejected + D-08 complete).
guard suite: PASS
```

`bash tests/check_includes.sh` exits 0 with `[1/7]` reporting `29 LFO-side root file(s), 29 file(s) opened across their transitive include closure, zero VCO includes`. The root count is unchanged from baseline, which is the expected and desired result: the new entry is **inert** until `tests/test_vco_pitch.cpp` lands, at which point it is what keeps the count from rising by a file that would fail.

## Regression floor

| Gate | Floor | Observed |
|---|---|---|
| `make test` | 72 cases / 2,616,112 assertions / 0 failed | **72 / 2,616,112 / 0** — exact |
| `make guards` | PASS | **PASS** |

This plan touched no test translation unit, so an exact match was the requirement rather than a coincidence.

## Milestone guardrail compliance

- `git diff --name-only HEAD~2 HEAD` = `tests/check_includes.sh`, `.planning/phases/30-.../deferred-items.md`. Nothing else.
- No frozen header edited. `src/dsp/FROZEN.sha256` absent from the diff; `check_frozen.sh` PASSes inside `make guards`.
- `src/AnalogLFO.cpp` absent from the diff (D-16). No `src/` file and no `tests/*.cpp` file touched.
- No `GUARD_WIRING_EXEMPT` entry added or removed — wiring `tests/check_docs.sh` into CI stays Phase 36's.
- No `VCO_SIDE_ALLOW` entry removed; no `[6/7]` control deleted, renamed or weakened (all four pre-existing variable names — `nc_hits`, `nc2_direct`, `nc2_hits`, `nc3_hits`, `nc4_hits` — untouched).
- Deferred item 6 untouched and still pointed at Phase 32 (D-15): `git diff` on that file shows zero content deletions.

## Decisions Made

1. **Deferred item 5 was folded in, not re-deferred.** The item's own "Resolve at" read *"the next phase that touches `tests/check_includes.sh`"*, and D-23 made Phase 31 exactly that phase. Restating the deferral with a new owner would have left it pointed at a condition already satisfied — the same false-comment class plan 30-08 existed to remove.
2. **The anchor permits a trailing comment on purpose.** Discovered by reading `src/dsp/VcoCore.hpp:74` before writing the pattern rather than after: the live exempted line carries `// forge::exp2_taylor5, forge::clamp`, so a stricter `...\.hpp"$` anchor would have turned `[2/7]` red on the shipped-adjacent VCO header on the very next run.
3. **The anchor permits the `grep -n` line-number prefix on purpose**, because the exclusion is a downstream pipeline stage. Recorded in the script so a future reader does not "tighten" it to `^#` and silently disable the exemption.
4. **Three banner claims corrected rather than left standing** (see Deviations, Rule 2).
5. **TEST-02 was NOT marked complete** (see Deviations / Deferred).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 — Missing Critical] Corrected three guard banners that the anchor edit made false**

- **Found during:** Task 2
- **Issue:** Three comment blocks documented the Rack detector as validated by **two** fixtures. One went further and asserted that those two *"pin the exemption to exactly the width it is documented to have"* — a claim WR-05 disproved by construction. Leaving them would have shipped a guard whose prose overstates its own coverage, which is precisely the defect class the `[6/7]` section exists to prevent and which this repo treats as load-bearing (`31-PATTERNS.md`: "Load-bearing comments"). The plan's own objective names this the "false comment" class plan 30-08 existed to remove.
- **Fix:** Updated the top-of-file `[6/7]` summary, the `detect_rack_sdk_includes` banner, and the `[6/7]` section banner to describe **three** Rack fixtures and the width they now pin. The section banner explicitly records that the previous claim was wrong: *"This section previously claimed the first two pinned the exemption 'to exactly the width it is documented to have'. They did not. Three do."*
- **Files modified:** `tests/check_includes.sh`
- **Verification:** `bash tests/check_includes.sh` exits 0; `make guards` PASS; no detector logic touched by these edits (comment-only).
- **Committed in:** `98c7cb9` (Task 2 commit)

### Deferred / not done, deliberately

**2. `TEST-02` left unchecked in `REQUIREMENTS.md`, against the plan frontmatter**

- **Found during:** the `requirements mark-complete` step.
- **Issue:** `31-01-PLAN.md` declares `requirements: [TEST-02]`, and TEST-02 reads *"V/Oct tracking accuracy is asserted (< 1 cent error) across the pitch range."* This plan asserts no cents error and creates no test case — it registers a **filename** in a guard allowlist. Marking TEST-02 complete here would reproduce, exactly, the false green Phase 30 deferred item 1 recorded for `PANEL-03` (checked `[x]` at `docs(30-01)`, before plan 30-06 did any of the work its text names).
- **Action:** TEST-02 stays `- [ ]` / `Pending`. The assertions land in **31-05** (tracking), **31-06** (COARSE/FINE/exp-FM) and **31-07** (Nyquist/hostile). The phase gate should confirm TEST-02 only after 31-07, the way 30-07's gate confirmed PANEL-03.
- **Recorded in:** STATE.md § Accumulated Context, as a Phase 31 decision.

### Verification-command note (no code impact)

Task 1's acceptance criterion `grep -c '\[\[ "${rel}" == "${a}" \]\]'` returns `0` on this machine's BSD `grep`, which treats the mid-pattern `$` specially in a BRE. The substantive claim it encodes is **true and verified** — the match is a single quoted-literal comparison — via both `grep -cF '[[ "${rel}" == "${a}" ]]'` → `1` and the escaped form `grep -c '\[\[ "\${rel}" == "\${a}" \]\]'` → `1`. This is a plan-authoring artifact in the criterion's shell quoting, not a finding about the script. Future plans writing `grep` criteria against `${...}` text on macOS should escape the `$` or use `-F`.

---

**Total deviations:** 1 auto-fixed (Rule 2, missing critical — guard prose truth), 1 deliberate non-action (requirement not falsely marked).
**Impact on plan:** No scope creep. The Rule 2 fix is comment-only and touches no detector logic. Both edits the plan prescribed landed exactly as specified.

## Issues Encountered

- `gsd-tools query state.record-metric` rejected positional arguments (`{"error": "phase, plan, and duration required"}`); it takes named flags (`--phase`, `--plan`, `--duration`, `--tasks`, `--files`). Re-run with flags, recorded successfully. Same for `state.add-decision` (`--phase`, `--summary`, `--rationale`) and `state.record-session` (`--stopped-at`). The `execute-plan.md` workflow's positional examples are stale for this SDK version.

## User Setup Required

None — no external service configuration required.

## Next Phase Readiness

- **Ready for 31-02.** `make guards` is green and `make test` is at its exact floor, so the next plan starts from a clean, validated baseline.
- **31-05's file lands green.** The `VCO_SIDE_ALLOW` entry is in place, so `tests/test_vco_pitch.cpp` will not trip `[1/7]` on its first run. This is the one thing the Phase-30 experience says to expect otherwise.
- **`[2/7]` is now safe for the D-14 include.** When `src/dsp/VcoCore.hpp` grows its use of `forge::exp2_taylor5`, the exemption is pinned to exactly the width its banner claims, and any future widening fails `[6/7]`.
- **Open for the phase gate:** TEST-02 remains unchecked by design and must be confirmed after 31-07, not assumed. `31-08` owns the phase's own `deferred-items.md`.
- **No blockers.**

## Self-Check: PASSED

- `tests/check_includes.sh` — FOUND
- `.planning/phases/30-vcocore-skeleton-module-registration/deferred-items.md` — FOUND
- Commit `2874104` — FOUND
- Commit `98c7cb9` — FOUND
- No file deletions in either task commit; no untracked residue (`[6/7]` fixtures live in a `mktemp -d` removed on exit).

---
*Phase: 31-pitch-tuning-exponential-fm*
*Completed: 2026-07-30*
