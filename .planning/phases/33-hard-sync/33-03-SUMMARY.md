---
phase: 33-hard-sync
plan: 03
subsystem: rack-shell
tags: [hard-sync, sync-jack, rack-shell, compile-canary, odr, constant-fold, cpp11, panel-svg]

# Dependency graph
requires:
  - phase: 33-hard-sync
    plan: 02
    provides: "forge::VcoInputs::syncVolts / ::syncConnected and the whole sync block inside forge::VcoCore::step — the DSP this plan makes reachable"
  - phase: 33-hard-sync
    plan: 01
    provides: "MorphBlep Guards A/B/C — unrelated to this plan's files, but the reason a second MorphBlep call site is contemplatable at all"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "the MORPH CV jack and MORPH DEPTH control, the most recent precedent for wiring a jack through all five places in src/AnalogVCO.cpp"
  - phase: 30-vco-core-registration
    provides: "src/AnalogVCO.cpp, res/AnalogVCO.svg, the THIS FILE DOES NO DSP banner and the hand-maintained field-count sentence this plan moves"
  - phase: 29-vco-foundations
    provides: "src/vco_compile_canary.cpp and tests/check_canary.sh — the LOAD-BEARING feed block and the [2b/5] section this plan extends and re-measures"
provides:
  - "AnalogVCO::SYNC_INPUT — the hard-sync jack, wired through all five places the FM jack is wired"
  - "Two unconditional POD assignments in AnalogVCO::process — raw volts and the connected flag, no arithmetic"
  - "Two runtime-derived compile-canary feeds — syncVolts from bits 13-15, syncConnected from bit 16"
  - "One additional placeholder rect in res/AnalogVCO.svg at the SYNC widget's centre less the half-width"
  - "A MEASURED root cause for deferred register item 2, and a measurement method that bites where [2b/5] cannot"
  - "SYNC-01 is now REACHABLE by a user for the first time — the jack exists and the core's DSP is behind it"
affects: [33-04, 33-05, 33-09, 33-11, 33-12, 35-panel]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Proving a compile-gate feed landed by DISASSEMBLING the emitted probe for its bit-extract instruction, rather than by reading the gate's own PASS"
    - "A witness symbol planted inside the branch under test, in a scratch header, with the pre-task commit as the negative control"
    - "Forcing the inliner to reach the seam so a fold-away hazard that stock -O3 hides becomes measurable"
    - "Restating a hand-maintained count from the landed source, and naming the part of the count the mechanism cannot see"

key-files:
  created: []
  modified:
    - src/AnalogVCO.cpp
    - src/vco_compile_canary.cpp
    - res/AnalogVCO.svg

key-decisions:
  - "The SVG rect landed in Task 1's commit, not Task 3's, because src/AnalogVCO.cpp's own banner forbids splitting a widget coordinate from its marker rect across two commits — the file's stated invariant beat the plan's task split"
  - "SYNC placed at x = 15.24 on the existing y = 100 jack row, extending its uniform 15.24 mm pitch by one slot; row symmetry about the centre line was DECLINED because buying it would move Phase-30 coordinates for a panel Phase 35 replaces wholesale"
  - "The [2b/5] field count does NOT move in this plan and cannot — the section enumerates float members only, so syncConnected is invisible to it and syncVolts joined the enumeration back in 33-02"
  - "The fold-away trap was reproduced by FORCING the inliner, after measuring that stock -O3 on this host never inlines VcoCore::step into the probe — which is the actual root cause of deferred register item 2"
  - "No requirement marked complete. SYNC-01 is now reachable but no permanent test asserts it; 33-04 owns that"

patterns-established:
  - "When a gate is measured insensitive, do not merely distrust it — find WHY it is insensitive, because the reason is usually more useful than the workaround"
  - "A mutation control must move exactly one column of the measurement table; if a mutation moves nothing, the measurement is not yet the measurement"
  - "A count sentence should name the part of the count its mechanism cannot see, or the number reads as total coverage"

requirements-completed: []  # SYNC-01 declined — see Decisions #5 and Deviations #4

coverage:
  - id: D1
    description: "The SYNC jack is wired through all five places the FM jack is wired and the module still constructs, registers and links"
    requirement: "SYNC-01"  # contributes to; not completed by this plan
    verification:
      - kind: other
        ref: "grep -n SYNC_INPUT src/AnalogVCO.cpp -> lines 125 (enum), 209 (configInput), 363/364 (two POD reads), 496 (widget); make links plugin.dylib and nm exports _modelAnalogVCO"
        status: pass
    human_judgment: false
  - id: D2
    description: "The shell does no arithmetic on either new field — the two assignment lines carry no operator but `=`"
    requirement: "SYNC-01"
    verification:
      - kind: other
        ref: "both lines quoted verbatim in this SUMMARY; no conditional, no zeroing, no scaling"
        status: pass
    human_judgment: false
  - id: D3
    description: "Both new POD fields are runtime-derived in the compile canary, from bit slices that collide with none of the nine existing feeds"
    requirement: "SYNC-01"
    verification:
      - kind: other
        ref: "disassembly of __ZN5forge21vcoCompileCanaryProbeEi at -O3: `ubfx w8, w19, #13, #3` and `ubfx w8, w19, #16, #1` both present; absent on the pre-task commit; bit budget of all eleven feeds enumerated below"
        status: pass
    human_judgment: false
  - id: D4
    description: "The sync branch is genuinely emitted at -O3 rather than folded away, and the measurement DISCRIMINATES — the pre-task commit and a literal-false mutant both go red"
    requirement: "SYNC-01"
    verification:
      - kind: other
        ref: "witness symbol planted inside the sync branch in a scratch header: pre-task ABSENT, this commit PRESENT, literal-false ABSENT, literal-true PRESENT (inliner forced to reach the seam)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The panel gained exactly one rect at the widget centre less the half-width, with no styling introduced"
    verification:
      - kind: other
        ref: "grep -c '<rect' 12 -> 13; grep -cE '<g |id=|class=|style=' -> 0; x = 15.24 - 5 = 10.24, derivation cross-checked against the V/OCT and FM pairs"
        status: pass
    human_judgment: false
  - id: D6
    description: "Nothing shipped moved — the frozen headers, the manifest and the six LFO goldens are all byte-identical, and src/AnalogLFO.cpp is absent from the whole-plan diff"
    verification:
      - kind: other
        ref: "check_frozen.sh PASS at 15 pinned entries; cmp on FROZEN.sha256 against the pre-plan copy exits 0; goldens 9 cases / 49,188 assertions; git diff --name-only cea071f..HEAD lists exactly three files"
        status: pass
      - kind: unit
        ref: "make test — 97 cases, 2,622,378 assertions, 0 failures"
        status: pass
    human_judgment: false
  - id: D7
    description: "In-Rack audition of the SYNC jack — a cable patched into it audibly hard-syncs the oscillator"
    requirement: "SYNC-01"
    verification:
      - kind: other
        ref: "NOT VERIFIED HERE — no operator session in this plan. The jack links and registers; whether it sounds right is plan 33-12's UAT, and the sync BLEP seam it will be judged with does not exist yet (plan 33-06)"
        status: deferred
    human_judgment: true

# Metrics
duration: 17min
completed: 2026-08-29
status: complete
---

# Phase 33 Plan 03: The SYNC Jack, the Canary Feeds and the Panel Rect Summary

**The hard-sync DSP that landed in plan 33-02 is now reachable from Rack through a jack the shell forwards without computing anything — and the compile canary's coverage of it was proved by disassembling the emitted object and by three discriminating mutants, because the gate that was supposed to prove it is structurally blind to the field that mattered most.**

## Performance

- **Duration:** 17 min
- **Started:** 2026-08-29T07:07:00+10:00
- **Completed:** 2026-08-29T07:24:00+10:00
- **Tasks:** 3 of 3
- **Files modified:** 3

## Accomplishments

- **Wired the SYNC jack through all five places the FM jack is wired**, with both POD fields crossing unconditionally and the shell computing nothing — the `THIS FILE DOES NO DSP` banner still holds after the change.
- **Diagnosed deferred register item 2 to its root cause rather than working around it.** `[2b/5]` is not insensitive because Apple clang "keeps the symbol"; it is insensitive because **`forge::VcoCore::step` is never inlined into the canary probe at stock `-O3` on this host**, so no field constant can propagate into the seam and the perturbed table survives whatever the canary feeds. Measured: `nm` shows `forge::VcoCore::step` as an out-of-line `T` symbol in the canary object.
- **Reproduced the fold-away trap that Pitfall 9 names**, which no local gate can currently see, by forcing the inliner to reach the seam. With a witness symbol planted inside the sync branch: **the pre-task commit deletes the branch entirely**, this commit keeps it, a literal-`false` flag deletes it again, a literal-`true` flag keeps it. The trap is real; it was simply hidden behind an inlining decision.
- **Proved both feeds landed by disassembly, with a mutation table where each mutant moves exactly one column** — the standard the blocker asked for, and one that a `[2b/5]` PASS could never meet.
- **Caught that the plan's `[2b/5]` "+2" criterion is unsatisfiable in principle**, not merely unmet (Deviations #2), and said so with the mechanism rather than adjusting the number quietly.
- **Honoured the source file's own single-commit rule over the plan's task split** (Deviations #1), so no commit in this plan's history shows a panel that lies about where its controls are.

## Task Commits

1. **Task 1: The SYNC jack in the shell, and the field-count sentence it moves (D-18)** — `d326cf8` (feat) — `src/AnalogVCO.cpp` + `res/AnalogVCO.svg`
2. **Task 2: The compile canary's two runtime-derived feeds, and the fold-away trap (D-02 / register item 15)** — `5af7f8a` (feat) — `src/vco_compile_canary.cpp`
3. **Task 3: One jack position on the throwaway panel, and the full local gate (D-18)** — **no commit; the rect landed in `d326cf8` by Deviations #1.** Task 3 executed as the full-gate task; every result is recorded below.

## Files Created/Modified

- `src/AnalogVCO.cpp` — the enum member, the `configInput`, the two POD assignments, the widget, and four hand-maintained prose blocks corrected in the commit that falsified them.
- `src/vco_compile_canary.cpp` — 73 insertions, 0 deletions. **Three new code lines**; the rest is the banner obligation this file holds a 15-line rationale block to.
- `res/AnalogVCO.svg` — one line.

## The Five Wiring Sites — Actual Number and Line Numbers, As Required

The criterion says `grep -c 'SYNC_INPUT' src/AnalogVCO.cpp` outputs **4**, and instructs that a different number be recorded with the reason rather than adjusted for.

**Measured: 5.**

| # | Site | Line |
|---|------|------|
| 1 | `InputId` enum member | **125** |
| 2 | `configInput(SYNC_INPUT, "Sync")` | **209** |
| 3 | POD read — voltage | **363** |
| 4 | POD read — connected flag | **364** |
| 5 | Port widget | **496** |

**No site uses a different spelling.** All five are the literal token `SYNC_INPUT`. The criterion's number is simply one lower than the parenthetical that immediately follows it in the same sentence — *"(enum, `configInput`, two POD-assignment reads, widget)"* enumerates **five** things and then calls them four. This is the **sixth** instance in this project of a gate mechanism narrower than the prose beside it, after the five 33-02 catalogued.

## The Shell Does No Arithmetic — Both Lines Quoted

```cpp
		in.syncVolts = inputs[SYNC_INPUT].getVoltage();
		in.syncConnected = inputs[SYNC_INPUT].isConnected();
```

No operator but `=`. No `if`, no zeroing, no scaling, no conditional. They sit immediately below the FM pair and above the single `core.step(in)` call, and they are the only two lines this plan adds to `process()`.

The banner paragraph above them was extended with what is **different** about the sync pair rather than with the FM argument repeated. Two of FM's three reasons carry over verbatim; the third is stronger in kind (the core's gate skips the entire detector, not just a term's arithmetic) and a fourth is new and decisive (a shell-side trigger would hand the core an already-decided boolean, and no headless test could ever watch the detection go wrong).

## Both Count Sentences, Restated From the Landed Source

`grep -n 'VcoInputs DSP fields' src/AnalogVCO.cpp` → **line 376**, in this sentence:

> this shell now feeds runtime-derived values into **EIGHT of the nine** VcoInputs DSP fields, while `src/vco_compile_canary.cpp` feeds all **NINE**. The single field the canary feeds and this shell does not is **drift**.

Both numbers were restated from the landed source, not derived by arithmetic on the sentence they replace:

| Claim | Checked against | Result |
|---|---|---|
| Nine enumerated DSP fields | `[2b/5]`'s own awk over `struct VcoInputs` | `pitchCV, coarse, fine, fmVolts, fmAtten, syncVolts, morph, character, drift` = **9** |
| Shell feeds eight of them | the `process()` body | all but `drift` = **8** |
| Canary feeds all nine | the feed block | **9** |
| Drift is still the single field only the canary reaches | both | **true** — this commit adds two fields to *both* sides at once |

**And the claim the sentence now carries that it did not before.** Neither `fmConnected` nor `syncConnected` is enumerated by `[2b/5]` at all — the section's awk matches `$1 == "float"`, and both flags are `bool`. So "nine fields runtime-live" is a claim about **nine floats** and says nothing about the two flags. That gap is not cosmetic for this phase: it is the exact route by which the fold-away trap arrives. The sentence now says so, so a later reader of `src/AnalogVCO.cpp` cannot mistake the nine for total coverage.

Three further hand-maintained numbers in the same file moved with them and were corrected in the same commit: the banner's control count (**ten → eleven**), the panel's rect count (**twelve → thirteen**), and the widget block's coordinate count (**ten → eleven**).

## The Canary's Bit Budget — All Eleven Feeds Enumerated

Required by the plan because the two naming groups deliberately overlap, so "non-overlapping" cannot be read off the shift amounts.

| Feed | Expression | Bits |
|------|-----------|------|
| `pitchCV` | `(i & 7)` | 0–2 |
| `coarse` | `(i >> 3) & 3` | 3–4 |
| `fine` | `(i >> 5) & 3` | 5–6 |
| `fmVolts` | `(i >> 7) & 7` | 7–9 |
| `fmAtten` | `(i >> 10) & 3` | 10–11 |
| `fmConnected` | `(i >> 12) & 1` | 12 |
| `morph` | `(i & 15)` | 0–3 |
| `character` | `(i >> 4) & 15` | 4–7 |
| `drift` | `(i >> 8) & 15` | 8–11 |
| **`syncVolts`** *(new)* | `(i >> 13) & 7` | **13–15** |
| **`syncConnected`** *(new)* | `(i >> 16) & 1` | **16** |
| *(loop trip count)* | `(i & 3) + 1` | 0–1 |

Group A (pitch/tune/FM) spans bits 0–12; group B (morph/character/drift) spans bits 0–11 and overlaps A on purpose. **The highest bit spoken for anywhere is 12**, so 13–15 and 16 collide with nothing. This enumeration is also written into the source above the feed block, so the next phase to add a field does not have to re-derive it.

### The two new feeds, quoted

```cpp
	in.syncVolts   = (float)((i >> 13) & 7) - 3.f;   // bits 13-15 -> -3..+4 V: crosses BOTH 0.1 V and 1.0 V
	in.syncConnected = ((i >> 16) & 1) != 0;         // bit 16, a bit test exactly like fmConnected — never a literal
```

Both right-hand sides reference the runtime parameter `i`. The `-3.f` offset maps the slice to **−3 V … +4 V**: four of its eight values sit below the 0.1 V low threshold and four at or above the 1.0 V high threshold, so the slice straddles the hysteresis band rather than idling on one side of it.

A third line was added inside the loop, beside the existing `in.pitchCV += (float)n * 0.125f;`:

```cpp
		in.syncVolts += (float)(1 - 2 * (n & 1)) * 4.f;
```

**It oscillates rather than accumulates, and that is the whole point of it.** `pitchCV`'s in-loop term is deliberately monotonic, and a monotonic `syncVolts` would walk past the thresholds once and latch — leaving the `LOW → HIGH` arm of `forge::SchmittTrigger`, the only arm that returns `true` and therefore the only route into the sub-sample solve and the reset, permanently unreached. The `1 - 2*(n & 1)` factor alternates ±4 V, wider than the 0.9 V band, so part of the parameter's domain drives a genuine transition **within a single call**.

## The Measurement That Bites — Because `[2b/5]` Does Not

**Deferred register item 2, filed by plan 33-02, is confirmed and its root cause is now known.**

### First: `[2b/5]` did not move, and could not have

| | Pre-task (`d326cf8`) | Post-task (`5af7f8a`) |
|---|---|---|
| `[2b/5]` runtime-live field count | **9** | **9** |
| `bash tests/check_canary.sh` | PASS | PASS |

The plan's criterion requires "exactly two higher". **That is unsatisfiable in principle** — see Deviations #2. Both numbers are recorded as required.

### Second: why the section is blind

```
$ nm canary.o | c++filt | grep 'VcoCore::step'
0000000000000280 T forge::VcoCore::step(forge::VcoInputs const&)
```

**`forge::VcoCore::step` is emitted OUT OF LINE.** Apple clang at `-O3` declines to inline it into `vcoCompileCanaryProbe` — the body is far too large, and `forge::MorphBlep::step` and `forge::Waveshape::morphedWave` are out-of-line beside it. An out-of-line body must be correct for **any** caller, so no value the canary feeds can ever constant-propagate into the seam, and `[2b/5]`'s perturbed table survives unconditionally. That is a sharper diagnosis than 33-02's "the symbol survives a constant-fed field": the symbol survives because **constant propagation into the seam never happens at all on this host**, not because clang is reluctant to drop symbols.

### Third: the disassembly, which discriminates

Command:

```
c++ -std=c++11 -O3 -Isrc -c <canary-variant> -o d.o
objdump -d --disassemble-symbols='__ZN5forge21vcoCompileCanaryProbeEi' d.o
```

| Canary variant | `ubfx …#13, #3` (syncVolts slice) | `ubfx …#16, #1` (flag bit test) | `strb wzr` into the flag byte |
|---|---|---|---|
| **PRE-TASK** `d326cf8` — no sync feeds, NSDMI defaults | 0 | 0 | **1** |
| **LIVE** — this commit | **1** | **1** | 0 |
| **MUTANT A** — flag replaced by literal `false` | 1 | **0** | **1** |
| **MUTANT B** — volts replaced by literal `5.f` | **0** | 1 | 0 |

**Each mutant moves exactly one column.** The pre-task object stores the zero register into the flag byte; the live object extracts bit 16 and stores that instead. Both feeds are present, both are runtime-derived, and the measurement can tell the difference — which is precisely what a `[2b/5]` PASS cannot do.

### Fourth: the fold-away trap itself, reproduced

A witness function was declared but never defined, and its call planted **inside** the sync branch of a scratch copy of `dsp/VcoCore.hpp`. The canary is copied next to the perturbed header (a quoted `#include` resolves against the including file's own directory first — the same trick `check_canary.sh` uses to avoid picking up the real header). If the branch survives, the object carries an **undefined reference** to the witness; if the branch is deleted, the reference is gone.

At stock `-O3` the witness is present for **every** variant, including literal-`false` — because the branch lives in the out-of-line `step` body, exactly as diagnosed above. Forcing the inliner to reach the seam (`-mllvm -inline-threshold=1000000`, which drops `VcoCore::step` to **zero** out-of-line symbols) makes the trap visible:

| Canary variant | Witness reference | Meaning |
|---|---|---|
| **PRE-TASK** `d326cf8` | **ABSENT** | the sync branch was **deleted entirely** — the canary covered the newest code in the seam with nothing |
| **LIVE** — this commit | **PRESENT** | branch survives |
| **MUTANT A** — literal `false` | **ABSENT** | branch deleted — **the trap, reproduced** |
| **MUTANT C** — literal `true` | **PRESENT** | branch survives; the check discriminates the *fold-away*, not merely "a literal" |

Object size corroborates independently: with the inliner forced, `__TEXT` is **3888** bytes live and **3664** with the literal-`false` flag — **224 bytes of code deleted** by changing one flag to a constant.

**The trap Pitfall 9 names is real, it was live on the pre-task commit, and this task closes it.** No repository artifact was created; everything ran under the scratch directory.

## Panel: One Rect, Derivation Cross-Checked

The half-width offset was confirmed against **two** existing pairs before the new line was written, as the plan requires:

| Widget centre (mm) | Rect `x` | Difference |
|---|---|---|
| V/OCT 30.48 | 25.48 | 5 |
| FM 45.72 | 40.72 | 5 |
| **SYNC 15.24** | **10.24** | **5** |

```xml
  <rect x="40.72" y="95" width="10" height="10" fill="#2a2a30"/>   <!-- existing: FM -->
  <rect x="10.24" y="95" width="10" height="10" fill="#2a2a30"/>   <!-- new: SYNC -->
```

`width`, `height` and `fill` are byte-identical to the existing jack rect. `grep -c '<rect'` moved **12 → 13**. `grep -cE '<g |id=|class=|style=' res/AnalogVCO.svg` outputs **0** — no group, id, class or style was introduced, and no Forge Noir design language was applied.

**Placement, and the symmetry that was declined.** SYNC sits at `x = 15.24` on the existing `y = 100` jack row, extending that row's uniform 15.24 mm pitch by one slot to the left. That keeps the three inputs contiguous — SYNC, V/OCT, FM — with OUT still to the right of them, which is the only legibility the panel has given it carries no labels. It does **not** restore the row's symmetry about the 91.44 mm panel's 45.72 mm centre line, and the source says so rather than glossing it: a symmetric four-jack row would require moving two Phase-30 coordinates, every phase so far has left earlier coordinates unmoved, and D-18 makes this asset a throwaway that Phase 35 replaces wholesale. Paying to move shipped-adjacent coordinates for a layout that does not survive the phase after next is the wrong trade.

## Gate Results — The Full Local Gate

| Gate | Result |
|------|--------|
| `make test` | **PASS** — 97 cases, 2,622,378 assertions, 0 failures |
| `make strict` (C++11, `-pedantic-errors`, all four `src/*.cpp`) | **PASS**, exit 0 |
| `make guards` | **PASS**, exit 0 |
| `make` against the real `../Rack-SDK` | **PASS** — `plugin.dylib` linked, 169,328 bytes |
| Module registers | `nm -gU plugin.dylib` exports `_modelAnalogVCO`; `src/plugin.cpp:8` still calls `p->addModel(modelAnalogVCO)` |
| `bash tests/check_frozen.sh` | **PASS** — **15 pinned entries**, unchanged |
| `FROZEN.sha256` byte-identical | **YES** — `cmp` against a pre-plan copy, exit 0 (not a diff-marker count) |
| Six shipped-LFO goldens | **byte-identical** — 9 cases / **49,188** assertions, 0 failures, non-zero matched case count |
| `bash tests/check_includes.sh` | **PASS** |
| `git diff --stat cea071f HEAD -- tests/check_includes.sh` | **empty** — no `VCO_SIDE_ALLOW` entry was needed |
| Whole-plan `git diff --name-only cea071f..HEAD` | exactly `res/AnalogVCO.svg`, `src/AnalogVCO.cpp`, `src/vco_compile_canary.cpp` |
| `src/AnalogLFO.cpp` in the whole-plan diff | **absent** (count 0) |
| `bash tests/check_canary.sh` | **PASS**, exit 0 |
| `[2b/5]` field count, pre → post | **9 → 9** — unmoved, and unmovable; see Deviations #2 |

### No new allowlist entry was needed, and that is worth stating

`tests/check_includes.sh` scans `src/`, `tests/` and `tools/`, and both files this plan touches are already registered in `VCO_SIDE_ALLOW` (`:356-357`). **This plan adds no translation unit**, so it pays none of the new-TU cost. That is worth recording because plan 33-10's `tools/render_sync_ab.cpp` **does** add one and pays it explicitly — one `VCO_SIDE_ALLOW` line, one `MAKECMDGOALS` filter entry and one target. Verified, not assumed: the diffstat above is empty.

### Suite totals, before and after

| | Test cases | Assertions |
|---|---|---|
| Before plan 33-03 | 97 | 2,622,378 |
| After plan 33-03 | **97** | **2,622,378** |
| Delta | **0** | **0** |

**Zero delta is the correct result for this plan**, as it was for 33-02, and is stated so it is not read as a gap. The plan's `files_modified` names three files, none of them under `tests/`, and all three tasks' acceptance criteria constrain the diff to exactly those. **Plan 33-04 owns every assertion.** See Deviations #3.

## Decisions Made

1. **The SVG rect landed in Task 1's commit, not Task 3's.** `src/AnalogVCO.cpp`'s widget block carries an explicit, reasoned in-source rule: *"The two files are written together; move one and the panel starts lying about where its controls are — which is exactly why both edits land in a single commit rather than in two."* Following the plan's task split would have produced a bisectable commit in which the module has eleven ports and the panel has ten markers — the exact state that sentence forbids, and one in which the plugin builds and runs with an invisible SYNC jack. The file's stated invariant is substance; the plan's task boundary is mechanism. See Deviations #1.

2. **SYNC sits at `x = 15.24` and the row is left asymmetric.** Recorded above with its reasoning; the alternative (moving Phase-30 coordinates) was rejected explicitly rather than not considered, and the rejection is written into the source beside the coordinate.

3. **The fold-away was measured by forcing the inliner, and the forcing is disclosed rather than buried.** `-mllvm -inline-threshold=1000000` is not how the plugin is built and the result must not be read as "the trap fires in the shipped build on this host" — it does not, because `VcoCore::step` stays out of line. What it establishes is that the trap is a **real property of the code**, that the pre-task commit was genuinely exposed to it, and that this task's feeds are what close it. Which toolchains actually inline the seam is a CI question (plan 33-11), not a local one.

4. **The count sentence names its own blind spot.** It would have been enough to move "seven of eight" to "eight of nine" and stop. The sentence now also states that neither bool flag is enumerated at all, because the number without that clause reads as total coverage — and the one field whose omission would have been most damaging is precisely the one the number cannot see.

5. **No requirement is marked complete, and SYNC-01 is declined for a reason that is now NARROWER than 33-02's.** Plan 33-02 declined SYNC-01 on two grounds: no jack, and no permanent assertion. **This plan removes the first ground** — `AnalogVCO::SYNC_INPUT` exists, the plugin links, the model registers, and a user can patch a cable into it and hear a hard sync. The second ground stands untouched: **no permanent test asserts SYNC-01**. Every behaviour 33-02 measured came from a one-shot probe built outside the repository and discarded, and `make test` gained exactly zero assertions in both plans. Marking SYNC-01 here would book a requirement whose entire evidence base is two deleted scratch binaries. Plan 33-04 lands the permanent cases; it marks the requirement. **Seventh consecutive decline** in this project's history of them.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 — Missing critical correctness] The plan's task split violates an explicit in-source invariant of the file it edits**

- **Found during:** Task 1, while reading `src/AnalogVCO.cpp`'s widget block
- **Issue:** Task 1 owns `src/AnalogVCO.cpp` (including the port widget) and Task 3 owns `res/AnalogVCO.svg` (the marker rect for that same widget), in separate commits. The widget block's own comment forbids exactly that: *"The two files are written together; move one and the panel starts lying about where its controls are — which is exactly why both edits land in a single commit rather than in two."* Executed literally, `d326cf8` would have been a commit in which the module exposes eleven ports and the panel draws ten markers — and it builds, links and runs in that state, so nothing would have caught it.
- **Fix:** Landed the one SVG rect in Task 1's commit alongside the widget coordinate it marks, and the `twelve rectangles → thirteen rectangles` banner correction with it. Task 3 executed as its remaining substance — the derivation cross-check and the full local gate — and produced no commit.
- **Files modified:** `res/AnalogVCO.svg` (moved from Task 3's commit to Task 1's)
- **Verification:** Every commit in this plan leaves the widget count and the rect count in agreement. `grep -c '<rect'` 12 → 13 in `d326cf8`, the same commit that adds the eleventh `addInput`.
- **Committed in:** `d326cf8`

**2. [Rule 3 — Blocking] Task 2's headline `[2b/5]` criterion is unsatisfiable in principle, not merely unmet**

- **Found during:** Task 2, and predicted from deferred register item 2 before the edit was made
- **Issue:** The criterion reads *"its `[2b/5]` section reports a runtime-live field count exactly two higher than the count it reported on the pre-task commit"*. `[2b/5]` derives its field list from `struct VcoInputs` **in the header**, matching `$1 == "float"`. This plan adds no field to the header — 33-02 did — so the enumeration is 9 before and 9 after. Worse, it could never have been +2 even had both fields landed here: `syncConnected` is a `bool` and is not enumerable by that awk at all. 33-02's SUMMARY already recorded the enumeration moving 8 → 9 on its own commit.
- **Fix:** Recorded both numbers as required (**9 → 9**) and stated the mechanism rather than adjusting the criterion. Replaced the criterion's *evidentiary role* with the disassembly table and the witness experiment above, which is what the plan's own stronger criterion and the blocker both asked for. The criterion's **prose intent** — that both new fields be genuinely runtime-live — is satisfied and measured; only its arithmetic is impossible.
- **Files modified:** none — a criterion-interpretation decision
- **Verification:** `[2b/5]` at `d326cf8` and at `5af7f8a` both report `all 9 VcoInputs DSP field(s) stay runtime-live`; the disassembly table above shows both feeds present and both mutants discriminated.
- **Committed in:** n/a (recorded here and in `5af7f8a`'s message)

**3. [Rule 3 — Blocking] All three tasks are marked `tdd="true"` while all three forbid touching a test file**

- **Found during:** Task 1
- **Issue:** Identical in shape to 33-02's Deviation #2, and it recurs for the same structural reason. Every task carries `tdd="true"`, which mandates a failing-test commit first. The plan's `files_modified` is three non-test files, its `artifacts` are the same three, Task 3's criterion requires the whole-plan diff to list exactly those three, and the plan names **plan 33-04** as the owner of the assertions. A RED commit would put a `tests/*.cpp` file in the diff and fail Task 3's criterion.
- **Fix:** Honoured the acceptance criteria over the `tdd` attribute, following the precedent 33-02 set. **The RED was measured, not skipped** — three canary mutants (literal `false`, literal `true`, literal volts) and the pre-task commit itself as a fourth negative control, each producing a distinct, reproducible signature. Nothing was committed to `tests/`.
- **Files modified:** none — an execution-procedure decision
- **Verification:** The two mutation tables above; `git diff --name-only cea071f HEAD` returns exactly the three planned files.
- **Committed in:** n/a (procedure)

**4. [Rule 1 — Bug] The automatic requirement mark would book SYNC-01 on a plan with no permanent assertion**

- **Found during:** state updates, after Task 3
- **Issue:** `SYNC-01` is in this plan's frontmatter, and this is the most tempting decline yet — unlike 33-02, this plan genuinely does make the requirement **reachable by a user**. But `make test` gained zero assertions across 33-01, 33-02 and 33-03 combined for the sync path. A green SYNC-01 would tell the audit-open scanner that a behaviour is delivered when the only evidence for it is two scratch binaries that no longer exist, and would remove the pressure that plan 33-04 exists under.
- **Fix:** Neither ID marked. `.planning/REQUIREMENTS.md` left at its pre-plan state; the auto-tick was reverted.
- **Files modified:** `.planning/REQUIREMENTS.md` (verified unchanged)
- **Verification:** `git diff .planning/REQUIREMENTS.md` produces no output; both rows still read `Pending`.
- **Committed in:** n/a — the file is unchanged

**5. [Rule 2 — Missing critical documentation] Three further hand-maintained counts in the same file would have gone stale**

- **Found during:** Task 1
- **Issue:** The plan names one hand-maintained sentence (`:320-331`, the field counts). `src/AnalogVCO.cpp` carries **three more**: the banner's *"Ten controls, no more"* enumeration, the banner's *"res/AnalogVCO.svg is twelve rectangles"*, and the widget block's *"These ten coordinates are the ten marker rects"*. All three are falsified by this plan. Leaving them would have made the file misstate its own contents in the commit that changed them — the failure the named sentence exists to prevent, arriving through three sentences the plan did not name.
- **Fix:** All three corrected in `d326cf8`. The control count got a paragraph rather than a number, because the eleventh control needed justifying under D-07's converse clause: the DSP landed first (plan 33-02) and the control follows it, which is the ordering D-07 asks for rather than an exception to it. The Phase-32 *"re-read that rule and found nothing to do"* finding was answered with a Phase-33 *"re-read it and found work"* counterpart, since its numbers were quoted inline and had all moved.
- **Files modified:** `src/AnalogVCO.cpp`
- **Verification:** No stale count remains — control count 11 against 11 declared, rect count 13 against `grep -c '<rect'` = 13, coordinate count 11 against 11 `addParam`/`addInput`/`addOutput` calls.
- **Committed in:** `d326cf8`

---

**Total deviations:** 5 auto-fixed (2 × Rule 1/2 correctness, 2 × Rule 3 blocking, 1 × Rule 2 documentation)
**Impact on plan:** All five served the plan's own stated goals. One honoured an in-source invariant the task split would have broken; one replaced an impossible criterion with a stronger measurement rather than quietly weakening it; one resolved the recurring TDD/diff contradiction without skipping the RED; one refused a false green; one kept the file from misstating itself. **No scope creep** — the whole-plan diff is exactly the three files the plan owns.

## Known Stubs

**None.** Every line this plan adds is wired end to end: the jack reaches the POD, the POD reaches the core's sync block, the panel marks the jack, and both canary feeds are odr-used at `-O3` with the emission measured rather than assumed.

Two things are *absent by design* and belong to named later plans, and neither is a stub in this plan's output:

| Absence | Owner | Why it is not a stub here |
|---|---|---|
| No permanent test asserts the jack or the reset | **plan 33-04** | This plan's `files_modified` excludes `tests/` and all three tasks' criteria enforce that |
| The sync BLEP seam is still withheld, so a reset is currently un-band-limited | **plans 33-05 / 33-06** | Deliberate: the core is measurement leg `none` so 33-05 has a bit-exact reference. Unchanged by this plan |

## Deferred Register Items

Recorded here so plan 33-11 files them with a Resolve-at.

**1. RESOLVED-WITH-A-CAVEAT — deferred register item 2 (`[2b/5]` insensitivity), filed by plan 33-02 against this plan.**
The register asked this plan to either observe `[2b/5]` on the CI GCC leg or add a sensitivity control that bites on this host. **The second was done**: the disassembly table and the witness experiment above both discriminate, with the pre-task commit as a negative control, and neither reads `[2b/5]`'s PASS as evidence. **The root cause is now known and is more useful than the workaround**: `forge::VcoCore::step` is never inlined into the canary probe at stock `-O3` on Apple clang, so no field constant propagates into the seam and the perturbed table survives whatever the canary feeds. That explains the whole section's local blindness in one sentence, and it explains why the CI GCC leg is where the teeth are.
**The caveat, which is the part still open:** the measurement above is a **manual, one-shot** procedure run in a scratch directory. `tests/check_canary.sh` is unchanged, so **`make guards` is exactly as blind tomorrow as it was yesterday**. Nothing prevents a later editor from replacing either feed with a literal and seeing a full green.
**Proposed Resolve-at:** plan 33-11, as a `check_canary.sh [2c/5]` section that plants a witness in a scratch header and asserts its reference survives — the mechanism is proven here and is roughly twenty lines of the shape `[2b/5]` already uses. Alternatively 33-11 observes the CI GCC leg for the phase-gate commit and records that instead. **Not fixed here:** the fix lives in `tests/check_canary.sh`, outside this plan's three-file scope.

**2. NEW — `[2b/5]` structurally cannot see a `bool` field, and the VCO POD now has two of them.**
The section's field derivation is `$1 == "float" && $3 == "="`. `fmConnected` and `syncConnected` are therefore invisible to it, permanently and by construction — not as a host quirk. Both are **outer gates on whole code blocks**, which makes them the highest-leverage fields in the POD to constant-fold and the only ones the guard cannot report on. The prose in `src/AnalogVCO.cpp` and `src/vco_compile_canary.cpp` now names this gap, but prose is not a gate.
**Proposed Resolve-at:** the same plan-33-11 section as item 1, which should enumerate `bool` members alongside `float` ones. Cheap: one added awk clause, and the perturbation for a bool is a branch witness rather than a table.

## Issues Encountered

- **T-33-08 (toolchain divergence) is not discharged locally**, unchanged from 33-01 and 33-02. `make strict` is `-fsyntax-only` and never links. This plan's new code is four lines of shifts, masks and float conversion — an integer-shift shape that `-O3` on x86 GCC treats identically — but the shifts are new where 33-02's addition had none, so the exposure is marginally larger than last plan's rather than smaller. The CI MinGW compile-and-link leg on the exact commit is plan 33-11's.
- **The plugin was not auditioned in Rack.** It links, exports `_modelAnalogVCO` and `src/plugin.cpp` still registers it, but no operator session ran and none is in this plan's scope. Judging the jack by ear before plan 33-06 lands the seam would in any case be judging an un-band-limited reset.
- **The `-mllvm -inline-threshold` measurement is Apple-clang-specific.** It establishes that the trap is a real property of the code and that the pre-task commit was exposed to it. It says nothing about which production toolchain actually inlines the seam, and it is not how the plugin is built. Disclosed in Decisions #3 so it is not over-read.
- **Two untracked `.planning/research/.cache/*.json` files** pre-existed at session start, as they did for 33-02, and were left alone.

## Next Phase Readiness

**SYNC-01 is reachable by a user for the first time. It is not yet asserted by anything permanent.**

- **Plan 33-04** owns every assertion. Beyond the three specifics 33-02 handed it (a withdrawal phase on the poisoned-instance case, a live increment for the strictly-positive reset, and driven sync voltages inside the CORE-03 interleave window), it now also owns **the mark for SYNC-01** — the jack half of 33-02's two-part decline is discharged, so 33-04's assertions are the only thing standing between that requirement and complete.
- **Plan 33-05** is unaffected by this plan: the core is byte-unchanged, so it is still exactly measurement leg `none` and its probe's bit-exactness reference still holds.
- **Plan 33-06** is unaffected likewise. The seam is still absent from the source.
- **Plan 33-10's renderer** should note that this plan paid **no** new-TU cost and confirmed it by empty diffstat, whereas the renderer pays one `VCO_SIDE_ALLOW` line, one `MAKECMDGOALS` filter entry and one target.
- **Plan 33-11** inherits both register items above, and the mechanism for closing them is already written and measured here.
- **Phase 35** inherits a thirteen-rect throwaway panel with an asymmetric jack row, and the source says in two places that the physical form and the placement are its call.

**Concerns carried forward:**

- **`make guards` is still blind to the failure class this plan spent most of its measurement on.** The trap was closed and proved closed, but by hand. Register item 1's caveat is the sharpest thing on this list.
- **Guard C's IEEE dependence** from 33-01, extended by 33-02's negated lower comparison, is unchanged. `-ffast-math` would defeat both.
- **No requirement is complete.** SYNC-01 needs 33-04's assertions; SYNC-02 has seven further contributing plans.

## Self-Check: PASSED

Verified against disk and git rather than asserted:

- **Files exist:** `src/AnalogVCO.cpp`, `src/vco_compile_canary.cpp`, `res/AnalogVCO.svg`, `.planning/phases/33-hard-sync/33-03-SUMMARY.md` — all FOUND.
- **Commits exist:** `d326cf8`, `5af7f8a` — both FOUND in `git log`.
- **The landed code is present in `HEAD`:** `SYNC_INPUT` at five sites (125, 209, 363, 364, 496), the two POD assignments with no operator but `=`, the two canary feeds at 173–174 and the in-loop oscillation at 197, and the new rect at `res/AnalogVCO.svg:14`.
- **The emitted object carries both feeds,** confirmed by disassembly rather than by the gate's PASS: `ubfx w8, w19, #13, #3` and `ubfx w8, w19, #16, #1` both present in `__ZN5forge21vcoCompileCanaryProbeEi` at `-O3`, both absent on the pre-task commit.
- **Nothing shipped moved:** `check_frozen.sh` PASS at 15 entries, `cmp` on `FROZEN.sha256` exits 0, the six LFO goldens replay byte-identical, and `src/AnalogLFO.cpp` is absent from `git diff --name-only cea071f HEAD`.
- **The whole-plan diff is three files** and exactly the three the plan owns.
- **`.planning/REQUIREMENTS.md` is unchanged:** `git diff` produces no output; SYNC-01 and SYNC-02 both still read `Pending`.

---
*Phase: 33-hard-sync*
*Completed: 2026-08-29*
