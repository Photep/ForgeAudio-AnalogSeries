---
phase: 30-vcocore-skeleton-module-registration
plan: 09
subsystem: docs
tags: [vco, wr-02, gap-closure, seeding, comment-accuracy, deferred-items, guardrail]
status: complete

# Dependency graph
requires:
  - phase: 30-vcocore-skeleton-module-registration
    provides: "src/AnalogVCO.cpp and its T-30-02 hardcoded seeding block (plan 30-05); the 0-of-2048 clone measurement (30-REVIEW.md, 30-UAT.md); the operator's UAT test 3 option-(a) answer"
provides:
  - "A src/AnalogVCO.cpp constructor comment that states the MEASURED clone behavior instead of claiming a per-instance property the shipped module does not have"
  - "The true scope of the D-11 spread written at the call site: divergence from an UNSPREAD default core, not from the next instance the user adds"
  - "An explicit warning that tests/test_vco_core.cpp's divergence invariants drive two DIFFERENTLY-seeded cores the shell never constructs"
  - "deferred-items.md items 2-5 — WR-02 owned by Phase 34/35, CR-02 owned by Phase 31/34, WR-04 owned by Phase 36, WR-05 owned by the next phase touching check_includes.sh"
  - "The CR-02 frozen-header constraint written down: any NaN fix is a VcoCore-LOCAL helper, never an edit to the shared forge::clamp the shipped LFO consumes"
affects: [31-pitch-tuning-fm, 34-analog-engine-output-stage, 35-shell-panel-display, 36-release]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "A source comment that asserts a property must carry the measurement that proves it, the scope it does NOT cover, and the phase that owns the gap"
    - "A test suite's evidence is scoped at the call site when the shell cannot reach the configuration the tests drive"
    - "Every deferred item names an owning phase or requirement — an entry with no owner is a note nobody will act on"

key-files:
  created: []
  modified:
    - src/AnalogVCO.cpp
    - .planning/phases/30-vcocore-skeleton-module-registration/deferred-items.md

key-decisions:
  - "The seed literals are byte-unchanged. They were a deliberate 30-05 must-have (T-30-02) chosen to avoid a real Rack hang from a degenerate (0,0) Xoroshiro pair; only the comment above them was wrong, and the operator scoped only the comment into this gap closure"
  - "The plan's Task-1 gate 3 folded the LFO SOURCE FILENAME (AnalogLFO.cpp) into the same zero-count regex as the LFO model symbol and slug. That is unsatisfiable by construction: plan 30-05 deliberately wrote two AnalogLFO.cpp mentions into this file's D-08 banner. Split into the landmine's canonical assertion (ForgeAnalogLFO|modelAnalogLFO = 0) plus a baseline-preservation assertion (AnalogLFO.cpp unchanged at 2), which is what landmine 3 actually asks for"
  - "The LFO precedent is cited as 'the shipped LFO module' with no filename and no model symbol, so this file's zero-counts for the LFO identifiers are untouched and the milestone diff still contains no LFO-side file"

patterns-established:
  - "Byte-identity of an untouched code region is proved by hashing the extracted lines at both revisions (sha256 ba3ec29a...), not by reading a diff"
  - "A comment-only claim is pinned by a diff-shape gate — every changed line must match ^[+-][[:space:]]*(//|$) — so a 'documentation' commit cannot smuggle in behavior"

metrics:
  duration: 14 min
  completed: 2026-07-29
  tasks: 2
  files: 2
---

# Phase 30 Plan 09: WR-02 Comment Closure Summary

Removed the false per-instance-variation claim from `src/AnalogVCO.cpp`'s T-30-02 constructor
comment — replacing it with the measured 0-of-2048 clone result, the true scope of the D-11
spread, and Phase 34/35 as the named owner — and filed WR-02, CR-02, WR-04 and WR-05 as owned
entries in `deferred-items.md` so no later phase inherits them silently.

## What was built

Nothing executable. This plan changed comment lines in one translation unit and appended four
tracking sections to a planning document. `src/AnalogVCO.cpp`'s diff contains **zero changed
non-comment lines**, and no source, panel or manifest file was touched by task 2.

## Task 1 — the corrected comment (before / after)

Recorded in full so the correction is auditable without a git checkout.

### BEFORE (`src/AnalogVCO.cpp:81-95` at `badde5e`)

```cpp
		// T-30-02. BOTH calls are required and neither is optional. seed()
		// seeds only the drift RNG; without the spread seed every
		// component-spread coefficient stays at zero and the instance carries
		// no per-instance analog variation at all — which is precisely the
		// divergence D-11 exists to produce.
		//
		// The four literals are copied VERBATIM from tests/VcoBlockDriver.hpp,
		// which already documents them as proven non-degenerate. Do not invent
		// values, and never seed with a pair of zeros: forge::Xoroshiro128Plus
		// seeded (0,0) is a fixed point emitting an all-zero stream, which makes
		// the rejection loop inside std::normal_distribution never terminate.
		// In Rack that is a HANG ON PATCH LOAD, not a failing test — the user's
		// Rack stops responding while opening a patch. Phases 34/35 replace
		// these literals with shell-forwarded entropy plus patch persistence,
		// and must re-validate any deserialized value the same way.
```

The final clause of the first paragraph was the defect: it asserted that the spread seeding is
"precisely the divergence D-11 exists to produce", which reads at the call site as a claim that
this constructor produces per-instance variation between live modules. It does not.

### AFTER (`src/AnalogVCO.cpp:81-113` at `4cc5cc7`)

```cpp
		// T-30-02. BOTH calls are required and neither is optional. seed()
		// seeds only the drift RNG; without the spread seed every
		// component-spread coefficient stays at zero and the D-11 spread
		// mechanism does nothing at all.
		//
		// READ THIS BEFORE TRUSTING THE DIVERGENCE TESTS. The four literals
		// below are HARDCODED, so every AnalogVCO in a patch is constructed
		// from the same pair of seeds: two instances are bit-identical clones
		// of one another, measured at 0 of 2048 differing samples and
		// reproduced independently by this phase's code review and by its
		// verification. What the D-11 spread actually buys here is divergence
		// from an UNSPREAD default core — not divergence from the next VCO the
		// user adds. tests/test_vco_core.cpp's divergence invariants drive two
		// DIFFERENTLY-seeded cores, which this shell never constructs, so they
		// are evidence about forge::VcoCore and must NOT be read as describing
		// the shipped module. Shell-forwarded per-instance entropy plus patch
		// persistence is Phase 34/35's, tracked as item 2 in this phase's
		// deferred-items.md. The pattern does not need designing — the shipped
		// LFO module already draws its seed from std::random_device and
		// persists the drawn spread seed in the patch.
		//
		// The four literals are copied VERBATIM from tests/VcoBlockDriver.hpp,
		// which already documents them as proven non-degenerate. Do not invent
		// values, and never seed with a pair of zeros: forge::Xoroshiro128Plus
		// seeded (0,0) is a fixed point emitting an all-zero stream, which makes
		// the rejection loop inside std::normal_distribution never terminate.
		// In Rack that is a HANG ON PATCH LOAD, not a failing test — the user's
		// Rack stops responding while opening a patch. That prohibition covers
		// the clone behavior too: do NOT "fix" the cloning here by hand-picking
		// a different pair of literals — route it through the deferred item.
		// Phases 34/35 replace these literals with shell-forwarded entropy plus
		// patch persistence, and must re-validate any deserialized value the
		// same way.
```

Four things changed, and one thing was deliberately kept:

1. **The false half is gone.** The load-bearing true half — that both calls are required, and
   that without the spread seed every component-spread coefficient stays at zero — is preserved
   verbatim; only its trailing claim was rewritten.
2. **The measurement is quoted with its provenance:** 0 of 2048 differing samples, reproduced
   independently by the code review and by the verification.
3. **The correction does not overshoot** into a second inaccuracy. It states what the D-11
   mechanism *does* do — diverge this instance from an unspread default core — rather than
   implying the spread is useless.
4. **The test evidence is scoped in place**, so a reader who finds the passing divergence
   invariants cannot mistake them for evidence about the shipped module.
5. **The hang warning is kept and not weakened**, and extended by one clause forbidding a future
   editor from "fixing" the cloning by hand-picking new literals.

## Preserved grep counts (landmine 2), measured

Measured on the committed file at `4cc5cc7`. Every value is the plan's required value.

| Assertion | Required | Measured | Verdict |
|---|---|---|---|
| `per-instance analog variation` (the false phrase) | `0` | **0** | PASS |
| `bit-identical` | `>= 1` | **1** | PASS |
| `2048` | `>= 1` | **1** | PASS |
| `deferred-items` | `>= 1` | **1** | PASS |
| `core.seed(` | `1` | **1** | PASS |
| `core.setSpreadSeed(` | `1` | **1** | PASS |
| `0x1234ULL` | `1` | **1** | PASS |
| `0x9E3779B9ULL` | `1` | **1** | PASS |
| `mm2px` | `4` | **4** | PASS |
| `std::clamp\|if constexpr\|inline constexpr\|static constexpr` | `0` | **0** | PASS |
| `dataToJson` (patch-state serialization) | `0` | **0** | PASS |
| `ForgeAnalogLFO\|modelAnalogLFO` (LFO model symbol + slug) | `0` | **0** | PASS |
| `AnalogLFO.cpp` (pre-existing D-08 banner mentions) | unchanged at `2` | **2** | PASS (see deviation 1) |
| `ForgeAnalogVCO` | `1` | **1** | PASS |

The new text names the forbidden C++ constructs by description where it needed to, names the
LFO precedent as "the shipped LFO module" with no filename and no model symbol, and repeats no
coordinate — so all four of plan 30-05's self-documentation traps stayed disarmed.

## Byte-identity of the two seeding calls

The two calls at `src/AnalogVCO.cpp:96-97` are byte-unchanged. Proved by hashing the extracted
lines at both revisions rather than by counting diff markers (the Phase-30 decision "assert byte
identity by reading BYTES, not by counting git diff markers"):

```
git show HEAD:src/AnalogVCO.cpp | sed -n '/core\.seed(/p;/core\.setSpreadSeed(/p' | shasum -a 256
  ba3ec29af017232a5ae1e5f91d57457059f0f4eac203de72bbee4b808e8f8fb1  -
sed -n '/core\.seed(/p;/core\.setSpreadSeed(/p' src/AnalogVCO.cpp       | shasum -a 256
  ba3ec29af017232a5ae1e5f91d57457059f0f4eac203de72bbee4b808e8f8fb1  -
```

Identical, including the leading tabs.

## Nothing outside comment lines moved

```
git diff HEAD~1 HEAD -- src/AnalogVCO.cpp | grep -E '^[+-][^+-]' | grep -vE '^[+-][[:space:]]*(//|$)'
  (empty)
```

Every added and removed line in commit `4cc5cc7` matches `^[+-][[:space:]]*(//|$)` — i.e. is a
comment line or blank. `git diff --stat` reports `1 file changed, 24 insertions(+), 6
deletions(-)`, all of them comment lines.

## Task 2 — the five deferred-items headings and their owners

| # | Heading | Owner named |
|---|---|---|
| 1 | `PANEL-03` is marked complete in REQUIREMENTS.md ahead of the work that satisfies it | plan 30-07's phase gate — **RESOLVED 2026-07-29**, preserved verbatim |
| 2 | Every live VCO instance in a patch is a bit-identical clone (WR-02) | **Phase 34/35** |
| 3 | `forge::clamp` is NaN-transparent, so VcoCore's defensive clamps are inert (CR-02) | **Phase 31 or Phase 34**, in the plan that adds the MORPH/CHARACTER CV inputs |
| 4 | `plugin.json` still declares version 2.0.1 while shipping a second module (WR-04) | **Phase 36** (REL-01) |
| 5 | `tests/check_includes.sh [2/7]`'s exemption filter is unanchored (WR-05), plus the remaining Info findings | the **next phase that touches `tests/check_includes.sh`**; the four Info findings recorded as tracked-but-unplanned |

Item 1 is preserved verbatim including its `RESOLVED — 2026-07-29, plan 30-07` block. Proved
directly: `git diff` for the file at commit `05b0db2` contains **zero deleted lines** (93
insertions, 0 deletions).

Two entries carry a constraint that exists to stop a future fix starting in the wrong place:

- **Item 2** carries the re-validation requirement forward. The shipped LFO's precedent is not
  just "draw from `std::random_device`" — it is draw, reject `(0,0)`, persist, and restore
  through a non-throwing hex parser. A corrupt or zero pair restored from a patch file is the
  same T-30-02 hang, reached through deserialization instead of through a literal.
- **Item 3** states the guardrail explicitly: `forge::clamp` is byte-pinned by
  `tests/check_frozen.sh` and consumed by the **shipped** Analog LFO at
  `src/dsp/LfoCore.hpp:168,212-213,216`. Editing it is a guardrail event, not a VCO fix. The
  entry specifies the correct shape — a NaN-safe helper local to `VcoCore`, bit-identical to the
  shared primitive for finite inputs, pinned by a case that fails before it lands.

## Verification

| # | Plan verification item | Result |
|---|---|---|
| 1 | False claim at zero; corrected text carries the measurement and points at `deferred-items` | PASS |
| 2 | Every counted 30-05 literal unchanged; four forbidden C++ constructs still zero | PASS (table above) |
| 3 | `src/AnalogVCO.cpp` diff contains zero changed non-comment lines | PASS (empty filter output) |
| 4 | `make strict` and `make guards` pass | PASS — `strict C++11 gate: PASS` (4 TUs), `guard suite: PASS` |
| 5 | `deferred-items.md` has five sections, item 1 verbatim with its RESOLVED block | PASS (5 headings, 0 deleted lines) |
| 6 | `git status --porcelain src res plugin.json` clean after the documentation task | PASS (0 lines) |

Guardrail sanity beyond the plan's requirements: `make test` reports **72/72 cases, 2,616,064
assertions, 0 failed**, so the shipped LFO's goldens are untouched — as they must be for a
comment-only change, and now measured rather than assumed.

## Deviations from Plan

### Auto-fixed issues

**1. [Rule 3 - Blocking] Task 1's gate 3 folded the LFO source FILENAME into a zero-count that
plan 30-05 deliberately made non-zero**

- **Found during:** Task 1, before any edit — the gates were measured against the unmodified
  file first.
- **Issue:** the plan's third automated gate asserts
  `grep -cE 'ForgeAnalogLFO|modelAnalogLFO|AnalogLFO.cpp' src/AnalogVCO.cpp` equals `0`. The
  measured baseline at `badde5e` was **2**, from two pre-existing D-08 banner lines that plan
  30-05 wrote on purpose: `src/AnalogVCO.cpp:25` ("LOCAL to `src/AnalogLFO.cpp`, the shipped
  module's translation unit") and `:27` ("why `src/AnalogLFO.cpp` does not appear in this
  milestone's diff at all"). Those two sentences are the file's record of *why* it uses stock
  SDK widgets, and deleting them to satisfy the gate would have destroyed load-bearing
  documentation of the milestone's strongest guardrail position. The gate was therefore
  unsatisfiable by construction — the same failure class the 30-08 executor hit with doctest's
  embedded source line numbers.
- **Fix:** the gate was split to match the plan's own prose rather than its regex. Landmine 2's
  canonical list is "the shipped LFO's **model symbol and slug** zero times", and landmine 3
  says do not *name* the LFO source file **when citing the `std::random_device` precedent** —
  i.e. do not ADD a mention. Both were asserted separately and both pass:
  `grep -cE 'ForgeAnalogLFO|modelAnalogLFO'` = **0**, and `grep -c 'AnalogLFO.cpp'` = **2**,
  unchanged from its pre-edit baseline. The corrected comment adds zero LFO-file, zero
  LFO-symbol and zero LFO-slug mentions, so threat T-30-04's actual mitigation — no VCO change
  reaching the shipped LFO's build graph — holds exactly as designed.
- **Files modified:** none beyond the plan (`src/AnalogVCO.cpp`).
- **Commit:** `4cc5cc7`.

No other deviations. The seed literals, the panel, the manifest and every guard script are
untouched.

## Threat model outcomes

| Threat | Disposition | Outcome |
|---|---|---|
| T-30-02 (hang on degenerate `(0,0)` seed) | mitigate | The proven non-degenerate literals are byte-unchanged (sha256 identical). The hang warning is kept and strengthened with an explicit instruction not to invent seed values here, and `deferred-items.md` item 2 carries the re-validation requirement to whichever phase adds patch persistence. |
| T-30-13 (comments asserting properties the code does not have) | mitigate | The false claim is removed and replaced by the measured figure and its provenance; the acceptance grep pins the phrase at 0 occurrences. |
| T-30-04 (VCO code entering the shipped LFO build graph) | mitigate | Comment-only change to one VCO-side TU. LFO model symbol and slug pinned at 0; the pre-existing filename mentions unchanged at 2; `git status --porcelain src res plugin.json` clean after task 2; `make test` 72/72 with the LFO goldens byte-identical. |
| T-30-SC (package-manager supply chain) | accept | Zero packages installed; no package manager invoked. |

## Known Stubs

None. This plan added no code, no data source and no UI surface.

## Threat Flags

None. No network endpoint, auth path, file access pattern or schema at a trust boundary was
added or modified.

## Commits

| Task | Commit | Description |
|---|---|---|
| 1 | `4cc5cc7` | `docs(30-09): correct the false T-30-02 per-instance-variation claim (WR-02)` |
| 2 | `05b0db2` | `docs(30-09): file WR-02, CR-02, WR-04 and WR-05 as owned deferred items` |

## What this leaves for later phases

- **Phase 34/35** inherits the actual WR-02 behavior fix in writing, with the shipped LFO's
  four-step pattern named and the re-validate-on-deserialize requirement attached.
- **Phase 31 or 34** inherits CR-02 with the frozen-header constraint stated, so the fix cannot
  start in `src/dsp/RackCompat.hpp`.
- **Phase 36** inherits WR-04 (the held `2.0.1`) as a decision, not an oversight.
- The next phase to touch `tests/check_includes.sh` inherits WR-05 with the concrete remedy
  (anchor the exclusion to a whole line; add the evasion shape as a third `[6/7]` control).

## Self-Check: PASSED

All claimed files exist on disk (`src/AnalogVCO.cpp`,
`.planning/phases/30-vcocore-skeleton-module-registration/deferred-items.md`,
`.planning/phases/30-vcocore-skeleton-module-registration/30-09-SUMMARY.md`) and both claimed
commits resolve in `git log` (`4cc5cc7`, `05b0db2`).
