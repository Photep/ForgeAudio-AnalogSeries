---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
plan: 11
subsystem: verification
tags: [operator-uat, in-rack, install-identity, whole-directory-flush, guardrail, morph-02, aa-05, test-03, t-32-29, t-32-30, t-32-12, t-32-03]

# Dependency graph
requires:
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-10 — the green phase gate, the CI observation by SHA, and the deferred register whose items 24 and 25 this plan is pointed at"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-07 — the measured per-shape alias floors that ARE this session's expected-results block"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-09 — invariant 6, the headless counterpart whose control surface, sample rates and modulation rates this session mirrors"
  - phase: 30-vcocore-skeleton-module-registration
    provides: "the MEASURED false negative that makes a whole-directory flush mandatory, and the D-07 every-visible-control-does-something rule"
provides:
  - "The install identity PINNED five ways against the freshly built artefact — manifest, asset, symbols, hash, and a measured before/after on the two files that actually moved"
  - "The guardrail subject PINNED rather than inferred: the browser carries THREE Forge Audio entries, and the one under test is named"
  - "Two install hazards checked that the plan's five pins do not cover: no .vcvplugin archive to re-extract over the flush, and no Forge plugin in the x64 tree"
affects: [33-hard-sync, 34-output-and-drift, 35-shell-panel-display, 36-goldens-ci-library]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Prove an install is FRESH by measuring the before state and showing which files moved, not only by showing the after state matches the source"
    - "Enumerate the install hazards the checklist does NOT cover (re-extractable archives, a second architecture tree) before declaring a perceptual subject pinned"

key-files:
  created:
    - .planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/32-11-SUMMARY.md
  modified: []

key-decisions:
  - "The SHA-256 comparison is installed-versus-dist, not installed-versus-repo-root. `make dist` runs `strip -S`, `install_name_tool` and `codesign` after the link, so the repo-root `plugin.dylib` differs from the distributable by construction. Recording the repo-root hash as if it should match would have manufactured a failure; recording only the matching pair without saying why would have hidden a real difference. Both hashes are recorded with the reason."
  - "The stale `ForgeAudio` plugin was NOT deleted. Removing a plugin from the operator's Rack installation is not an executor's call — the plan says so explicitly and STATE already carries it as optional housekeeping."

requirements-completed: []

# Metrics
duration: 12 min (Task 1 only — Task 2 is the operator session, not yet run)
completed: null
status: checkpoint-pending
---

# Phase 32 Plan 11: Operator In-Rack UAT Summary

**Task 1 is complete and the install identity is pinned five ways with a measured before/after; Task 2 is an OPERATOR CHECKPOINT and has NOT run. This plan is deliberately NOT marked complete, and no requirement, roadmap row or plan counter has been advanced — the verdict does not exist yet, and booking it would be exactly the PANEL-03 false green this phase's discipline exists to refuse.**

## Performance

- **Duration so far:** ~12 min (Task 1)
- **Tasks:** 1 of 2 executed
- **Files:** 0 source files created or modified — this plan produces no code artefact

---

## Task 1 — Build, Whole-Directory Flush, And The Pinned Identity

### Build

`make clean` → `make -j4` → `make dist`, all against the **relative** `../Rack-SDK`. No worktree copy, no absolute SDK path.

| Step | Result |
|---|---|
| `make clean` | removed `build/` and `plugin.dylib` — the relink is real, not a no-op |
| `make -j4` | exit 0; all four TUs compiled (`AnalogLFO.cpp`, `AnalogVCO.cpp`, `plugin.cpp`, `vco_compile_canary.cpp`), linked to `plugin.dylib` |
| `make dist` | exit 0; produced `dist/ForgeAudio-AnalogSeries/` **and** `dist/ForgeAudio-AnalogSeries-2.0.1-mac-arm64.vcvplugin` (152,677 bytes) |

### The flush was a WHOLE-DIRECTORY replacement, and it is recorded as one

```
rsync -a --delete --checksum --itemize-changes \
      dist/ForgeAudio-AnalogSeries/ \
      ~/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio-AnalogSeries/
cp    dist/ForgeAudio-AnalogSeries/plugin.dylib  <dest>/plugin.dylib
```

`--checksum` was used deliberately rather than rsync's default size-plus-mtime heuristic, because a same-size file with a preserved mtime is exactly what a default `rsync -a` skips. `--delete` makes it a true replacement rather than an overlay. **0 files were deleted** — nothing stale was orphaned in the directory.

### What the flush ACTUALLY MOVED — the freshness proof

This is the part a hash-of-the-after-state cannot give you. The pre-flush install was measured **before** it was overwritten:

| file | BEFORE (Jul 30 13:42 — the Phase 31 install) | AFTER (Aug 1 13:24) | moved? |
|---|---|---|---|
| `plugin.dylib` | sha256 `5061619cbe45e53b013c1cbdafc07926191f7e92c0675fe03308b8666fb97e43`, 175,056 bytes | sha256 `431b9de68e2076ac5d24d8dc04e9aba20cb45e257e208b1403e46a9a44b86e35`, 175,216 bytes | **YES** |
| `res/AnalogVCO.svg` | 766 bytes, **10** `<rect>` | 896 bytes, **12** `<rect>` | **YES** |
| `plugin.json` | version 2.0.1, 2 modules | version 2.0.1, 2 modules | no — already correct |
| everything else (`LICENSE`, `NOTICES`, `res/AnalogLFO.svg`, 9 component SVGs, 2 fonts, `PANEL-SPEC.md`) | — | — | no — content-identical |

**The two files that moved are precisely the two this phase changed**, and the SVG's `10 → 12` rect delta is the two MORPH marker rects plan 32-02 added at (25.48, 75) and (55.96, 75).

**This is a live instance of the Phase 30 hazard, caught by the flush.** The installed `plugin.json` was *already* at version 2.0.1 with *already* both modules — so a manifest check alone would have reported the install healthy while the operator faced a Phase 31 binary with no `MorphBlep` in it and a panel with no MORPH CV marker. The manifest is the check that passes on a stale install; the binary hash and the rect count are the checks that catch it.

### PIN 1 — the INSTALLED manifest, read from the installed file

```
slug   : ForgeAudio-AnalogSeries
version: 2.0.1                     (repository: 2.0.1 — MATCH)
modules: 2                         (required: 2 — MATCH)
   - ForgeAnalogLFO | Analog LFO
   - ForgeAnalogVCO | Analog VCO
```

`cmp` against the repository's `plugin.json` reports **IDENTICAL BYTES**.

### PIN 2 — the installed panel asset

```
-rw-r--r--  896 bytes  Aug  1 13:24  res/AnalogVCO.svg
grep -c '<rect'  ->  12          (acceptance criterion: 12 — MATCH)
marker x-coords  ->  x="25.48" and x="55.96" both present
cmp vs repo      ->  IDENTICAL BYTES
```

The panel carries **0** `<text>` elements — it is unlabelled by design (Phase 30 D-08, stock SDK widgets). **This is load-bearing for the session script:** the operator cannot read a knob's name off the panel and must identify controls by position and by hover tooltip. The script below names both.

### PIN 3 — the installed binary's exported model symbols

```
nm -gU <installed>/plugin.dylib | grep -i model
0000000000014380 S _modelAnalogLFO
0000000000014388 S _modelAnalogVCO
```

Both present. The LFO model is exported from the same binary the VCO is, which is what makes step 8's guardrail check a check on *this* build rather than on a neighbouring one.

### PIN 4 — SHA-256, installed versus freshly built

| artefact | sha256 |
|---|---|
| **installed** `plugin.dylib` | **`431b9de68e2076ac5d24d8dc04e9aba20cb45e257e208b1403e46a9a44b86e35`** |
| **freshly built** `dist/ForgeAudio-AnalogSeries/plugin.dylib` | **`431b9de68e2076ac5d24d8dc04e9aba20cb45e257e208b1403e46a9a44b86e35`** |
| repo-root `plugin.dylib` (raw link output) | `9b4ed6ff2d3527632b9b2a2d13323fa6218427cd226c8ee5346be3605f79bf81` |

**MATCH on the pair that matters.** The third row differs **by construction, not by accident**: `make dist` runs `strip -S`, then `install_name_tool -change libRack.dylib /tmp/Rack2/libRack.dylib`, then `codesign -f -s -` on its copy. The distributable is the thing that gets installed, so the distributable is the thing the installed file must equal. Recorded rather than quietly omitted, because "the freshly built one" is ambiguous between the two and a future reader comparing against the wrong one would see a spurious failure.

### PIN 5 — every installed Forge plugin directory, enumerated

Scanned all 81 directories under `plugins-mac-arm64/`; two carry a Forge slug, name or brand.

| directory | slug | plugin name | brand | version | modules | mtime |
|---|---|---|---|---|---|---|
| `ForgeAudio-AnalogSeries/` | `ForgeAudio-AnalogSeries` | Forge Audio - Analog Series | Forge Audio | **2.0.1** | `ForgeAnalogLFO` \| **Analog LFO**<br>`ForgeAnalogVCO` \| **Analog VCO** | Aug 1 13:24 |
| `ForgeAudio/` | `ForgeAudio` | Forge Audio | Forge Audio | 2.0.0 | `ForgeAudioLFO` \| **LFO** | Feb 14 18:13 |

The stale directory exports `_modelLFO` (not `_modelAnalogLFO`) — a different symbol, a different slug, and a different panel asset (`res/LFO.svg`, 13,423 bytes, pre-Forge-Noir) from the shipped `res/AnalogLFO.svg` (29,878 bytes). It **cannot shadow** the current install. What it does is put a third entry in the browser.

### The guardrail subject, PINNED — not inferred

Both plugins declare the **same brand string, `Forge Audio`**, so they collapse into one brand heading in the module browser. Rack's own log from the last session (Aug 1 11:08) shows the display convention is brand-then-module-name:

```
Loaded plugin ForgeAudio-AnalogSeries 2.0.1
Loaded plugin ForgeAudio 2.0.0
Creating module Forge Audio Analog VCO
Creating module Forge Audio Analog LFO
```

**Under "Forge Audio" the operator will see THREE modules:**

| browser entry | plugin | version | is it the guardrail subject? |
|---|---|---|---|
| **Analog LFO** | `ForgeAudio-AnalogSeries` | 2.0.1 | **YES — this is the shipped module under test** |
| **Analog VCO** | `ForgeAudio-AnalogSeries` | 2.0.1 | the module this phase changed |
| **LFO** | `ForgeAudio` | 2.0.0 | **NO — stale, pre-rename, Feb 14. Ignore it.** |

The distinction is a **name difference, not a version tooltip**: the shipped one is called **"Analog LFO"**, the stale one is called plain **"LFO"**. That is stronger than STATE's "a second Forge LFO" phrasing implied, and it is what converts Phase 31's inferred guardrail subject into a pinned one. Deferred item 25 is discharged for this session.

**Not deleted.** Removing a plugin from the operator's Rack installation is not an executor's call — the plan says so and STATE already carries it as optional housekeeping (`deferred-items.md` item 15). It stays an outstanding housekeeping item.

### Two hazards the plan's five pins do not cover — checked anyway

Both belong to the same family as the Phase 30 false negative (something the operator's Rack reads that is not the thing just flushed), so leaving them unchecked would have left the pin incomplete in exactly the way the checklist was written to prevent.

1. **A `.vcvplugin` archive that Rack would re-extract over the flush at startup.** `find ~/Library/Application Support/Rack2 -maxdepth 2 -iname '*.vcvplugin'` returns **nothing**, and there is no `plugins/` staging directory. Nothing will overwrite the flushed directory on launch.
2. **A second architecture tree.** `plugins-mac-x64/` exists (48 plugins, Aug 2024). It contains **no Forge directory at all**, so an x64 or Rosetta Rack would find no Forge plugin rather than an old one. The host is `/Applications/VCV Rack 2 Pro.app` and the last session loaded both Forge plugins from `plugins-mac-arm64/`.

**Rack is not currently running** (`pgrep` returns no process), so it will read the flushed directory on next launch with no restart-ordering hazard.

### One forensic loose end, chased down rather than waved off

An extra probe of the installed binary's string pool found `Morph CV Depth`, `Coarse Tune`, `Fine Tune`, `V/Oct` and `Audio` present as literals but **`FM Depth`, `FM`, `Morph CV` and ` oct` absent** — including from the unstripped repo-root binary. Since "a control name in the source that is not in the binary" is a legitimate stop-and-report shape, it was resolved rather than assumed benign.

**Cause: libc++ short-string materialisation on arm64.** Those names are built in registers as immediates, so their ASCII never appears contiguously in the file. Disassembly confirms it exactly:

```
00000000000018b4   mov   x8, #0x4d46            ; "FM"
00000000000018b8   movk  x8, #0x4420, lsl #16   ; " D"
00000000000018bc   movk  x8, #0x7065, lsl #32   ; "ep"
00000000000018c0   movk  x8, #0x6874, lsl #48   ; "th"
0000000000001924   mov   w8, #0x4d46            ; "FM"  (the input name)
```

A compiler encoding artefact, identical across all three binaries (repo-root, dist, installed). **Not an install-identity finding.** Recorded so the next agent who runs the same probe does not re-open it.

### Task 1 acceptance criteria

| criterion | result |
|---|---|
| `make -j4` and `make dist` exit 0, `dist/` contains the extracted directory | **PASS** |
| INSTALLED `plugin.json` version matches repository and lists 2 modules | **PASS** — 2.0.1, 2 modules, byte-identical |
| Installed `res/AnalogVCO.svg` exists and `grep -c '<rect'` outputs `12` | **PASS** — 896 bytes, Aug 1 13:24, 12 rects |
| `nm` lists both the LFO and the VCO model symbol | **PASS** — `_modelAnalogLFO`, `_modelAnalogVCO` |
| Installed binary SHA-256 equals the freshly built one's | **PASS** — `431b9de6…` = dist artefact (see PIN 4 on the repo-root third value) |
| Every installed Forge plugin directory enumerated with slug, version, module list | **PASS** — 2 directories, both tabulated |

Nothing disagreed with the freshly built artefact, so the plan's STOP condition did not fire.

### The tree the verdict will attach to — measured BEFORE the session as a precondition

| gate | result |
|---|---|
| `make test` | **94 cases / 94 passed / 0 failed**, **2,622,319** assertions |
| `make strict` | `strict C++11 gate: PASS` over 4 TUs |
| `make guards` | `guard suite: PASS` |

Recorded as a **precondition only**. Phase 29 measured this exact combination green on code that could not link, so local green is never evidence on its own. The plan requires all three to be re-run *after* the session; that re-run belongs to Task 2 and has not happened.

---

## Task 2 — OPERATOR CHECKPOINT — NOT RUN

**Status: awaiting the operator.** The audition script, the expected-results block and the two judgements being asked for were composed and handed to the orchestrator as a structured checkpoint. **No verdict has been recorded, because no verdict exists.**

The executor cannot hear anything. Nothing in this file infers, simulates or self-answers a perceptual result, and no observation has been invented. Per T-32-30, an absence of complaint counts only when the operator was told what to expect first — which is why the expected-results block is presented in full before the reply is taken, and why this section stays empty until it is.

**On completion, the continuation agent must record here:** the expected-results block exactly as presented; the operator's reply **verbatim**, including any observation however minor, and including an explicit note if none was raised; the routing decision for every observation (gap-closure plan if it contradicts a phase assertion, `deferred-items.md` with an owner if out of scope); and the post-session re-run of `make test`, `make strict` and `make guards`.

---

## Deviations from Plan

**1. [Recorded] The SHA-256 criterion is ambiguous between two artefacts, and the ambiguity is resolved in the record rather than in silence**

- **Found during:** Task 1, PIN 4.
- **Issue:** the criterion reads "the installed binary's SHA-256 equals the freshly built one's". `make dist` strips, re-points the install name and codesigns its copy, so `plugin.dylib` at the repository root and `dist/ForgeAudio-AnalogSeries/plugin.dylib` have **different hashes by construction**. Only one of them is the thing that gets installed.
- **What was done:** the installed file was compared to the **dist** artefact (exact match) and **all three hashes were recorded** with the reason the third differs. Comparing against the repo-root binary would have manufactured a failure; recording only the matching pair would have hidden a real difference from the next reader.

**2. [Rule 2 — Missing Critical] Two install hazards outside the plan's five pins were checked before the subject was declared pinned**

- **Found during:** Task 1, after the five pins passed.
- **Issue:** the five pins all measure the flushed directory. They say nothing about a `.vcvplugin` archive Rack would re-extract over it at startup, or about a second architecture tree Rack might read instead. Both are the same failure family as the Phase 30 false negative — something the operator's Rack reads that is not the thing just flushed — so the pin would have been incomplete in precisely the way it exists to prevent.
- **What was done:** both checked and recorded (no archives anywhere under `Rack2/`; no Forge directory in `plugins-mac-x64/`). Neither was a problem, which is the useful outcome — the pin is now closed against them rather than silent about them.

**3. [Recorded, not a code change] Neither requirement status, roadmap progress nor the STATE plan counter was advanced**

- **Issue:** the plan is incomplete — its output *is* the operator verdict, and Task 2 has not run. MORPH-02's qualification (deferred item 24) is discharged only by the operator's hands-on check of the MORPH CV path, and that check has not happened.
- **What was done:** `status: checkpoint-pending`, `completed: null`, `requirements-completed: []`, ROADMAP left at 10/11 with 32-11 unticked. Advancing any of them would book the false green that 32-10's own deviation 3 refused for this exact plan.

**Total deviations:** 1 recorded ambiguity resolved, 1 auto-added coverage gap closed, 1 recorded non-advancement. **No `src/` file was touched. `src/AnalogLFO.cpp` and every frozen path are absent from this plan's diff — it modifies no source at all.**

---

## Threat Mitigations Applied

| Threat | Mitigation as landed |
|---|---|
| **T-32-29** (a verdict recorded against a stale install) | Whole-directory flush with `--checksum --delete`; identity pinned by manifest (byte-identical), asset (12 rects, byte-identical), symbols (both models), hash (installed = dist, exact) — **plus a measured before/after showing the binary and the panel asset genuinely moved and the manifest did not**, which is the one check that would have caught this specific stale install. Extended past the plan's list to cover re-extractable archives and the x64 tree. |
| **T-32-12** (the shipped Analog LFO) | The guardrail subject is **pinned by name**: "Analog LFO" from `ForgeAudio-AnalogSeries` 2.0.1, distinguished from the stale "LFO" from `ForgeAudio` 2.0.0 by module name, plugin slug, exported symbol (`_modelAnalogLFO` vs `_modelLFO`) and panel asset. Step 8's subject is no longer inferred. |
| **T-32-30** (an absence of complaint mistaken for an absence of problems) | The expected-results block — measured per-shape floors, the corrected roadmap figure, and the honest statement of what will still sound rough — is presented **in full before the reply is taken**. Task 2's section is left empty rather than filled with an inferred verdict. |
| **T-32-03** (out-of-range output reaching speakers) | Invariant 6's outer bound (10.0 V against Rack's ±12 V norm) is asserted across the audio-rate MORPH grid and green in the pre-session `make test`. The session script starts at CHARACTER 0 and a mid-keyboard note, and the checkpoint carries an explicit monitoring-level warning ahead of step 5, the loudest case. |
| **T-32-SC** (package installs) | Zero packages installed. `plugin.json` gains no dependency. |

## Known Stubs

None in this plan's output. The empty Task 2 section is **not a stub** — it is the honest state of a checkpoint that has not been answered, and this plan is marked `checkpoint-pending` precisely so nothing downstream reads it as done.

## Threat Flags

None. No network, auth, file-access or schema surface introduced; no source file modified.

## Issues Encountered

None. The one anomaly investigated — four control-name literals absent from the binary's string pool — resolved to arm64 short-string immediate materialisation, confirmed by disassembly, and is not an install-identity finding.

## Next Phase Readiness

**Blocked on the operator.** Phase 32 cannot close until Task 2's verdict is recorded verbatim and any observation is routed.

## Self-Check: PASSED

- `dist/ForgeAudio-AnalogSeries/` and `dist/ForgeAudio-AnalogSeries-2.0.1-mac-arm64.vcvplugin` — FOUND on disk
- Installed `plugin.json` — FOUND; version 2.0.1, 2 modules, byte-identical to repo
- Installed `res/AnalogVCO.svg` — FOUND; 896 bytes, 12 rects, byte-identical to repo
- Installed `plugin.dylib` — FOUND; sha256 `431b9de6…` equals the dist artefact exactly; differs from the pre-flush `5061619c…`
- `nm` — both `_modelAnalogLFO` and `_modelAnalogVCO` FOUND
- Two Forge plugin directories enumerated with slug, version and module list
- `make test` 94/94/0 at 2,622,319, `make strict` PASS, `make guards` PASS — all re-run at this commit
- Task 2 — **NOT RUN**, and recorded as not run

---
*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Status: Task 1 complete, awaiting operator checkpoint*
