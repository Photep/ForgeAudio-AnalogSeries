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
  - "The operator verdict recorded VERBATIM as a QUALIFIED pass, with the no-artefact half separated from the audible-improvement half that the session had no way to answer"
  - "A new methodological finding — the audition asks whether an improvement is audible but supplies no A/B reference — filed as deferred item 26 with an in-tree remedy and an owner"
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
  - "The verdict is recorded as a QUALIFIED pass and Q1 is SPLIT. The no-artefact half is discharged under T-32-30 because the expected-results block was presented in full first. The audible-improvement half is recorded as UNEVIDENCED BY CONSTRUCTION — the session offered no A/B reference, so it could not be answered either direction — and 'seems to work well enough' is explicitly not allowed to stand in for it."
  - "No operator attestation is recorded for the LFO guardrail. The operator did not mention step 8, and inferring an attestation from a general remark is the fabrication this plan's protocol exists to refuse. T-32-12 is discharged for this phase by its automated evidence, and the file says which one it is."

requirements-completed: [MORPH-01, MORPH-02, AA-05, TEST-03]

# Metrics
duration: 22 min (Task 1 ~12 min; Task 2 operator session + close-out)
completed: 2026-08-27
status: complete
---

# Phase 32 Plan 11: Operator In-Rack UAT Summary

**The install identity is pinned five ways with a measured before/after that caught a genuinely stale Phase 31 binary hiding behind an already-correct manifest, the guardrail subject is pinned by name against a stale third browser entry, and the operator's verdict is recorded verbatim as a QUALIFIED pass: no artefact was heard across the audio-rate MORPH sweep, but the audible-improvement half of the question turned out to be unanswerable by construction because the session supplied no A/B reference — recorded as unevidenced rather than as passed, and filed with its remedy and an owner.**

## Performance

- **Duration:** ~22 min
- **Completed:** 2026-08-27
- **Tasks:** 2
- **Files:** 0 source files created or modified — this plan produces no code artefact, it produces the verdict

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

## Task 2 — OPERATOR CHECKPOINT — ANSWERED

### The operator's reply, VERBATIM

> "Seems to work well enough - but it's hard to remember what the old audio sounded like. Let's continue."

That is the complete reply, reproduced exactly as written, before any interpretation of it. It is **a qualified pass, not an unqualified one**, and the rest of this section splits it honestly rather than flattening it into "approved".

### What was presented before the reply was taken (T-32-30)

The expected-results block was presented **in full, before** the operator answered — as the plan requires, and for the reason the Phase 30 UAT record states by name: an absence of complaint means something only if the operator was told what to expect. What was presented:

- **The measured per-shape alias floors** from `32-07-SUMMARY.md` at CHARACTER 0, 44.1 kHz, naive → band-limited, at all three notes:

  | note | sine | triangle | saw | square | pulse 5% |
  |---|---|---|---|---|---|
  | **C7** | −125.4 → −125.4 (0.0) | −41.6 → −50.3 (+8.7) | −20.8 → −29.5 (+8.7) | −20.8 → −29.5 (+8.6) | −4.8 → −13.5 (+8.7) |
  | **C8** | −101.5 → −101.5 (0.0) | −33.8 → −48.8 (+15.0) | −15.6 → −25.8 (+10.3) | −16.9 → −31.9 (+15.0) | −1.3 → −11.6 (+10.3) |
  | **C9** | −91.9 → −91.9 (0.0) | −19.1 → −28.5 (+9.5) | −9.5 → −19.0 (+9.5) | −9.5 → −19.0 (+9.5) | −0.3 → −9.8 (+9.5) |

- **The corrected roadmap figure.** The earlier "≈ −60 dB" target was stated as **corrected, not met**: 2-sample polyBLEP attenuates by `sinc²`, only about −8 dB at Nyquist, so the achievable floor sits well above −60 dB and the per-shape measured thresholds are the honest gate.
- **The plain statement that the narrow-pulse end at the top of the keyboard will still be the roughest** (corrected pulse floor −9.8 dB at C9), that this is the technique's ceiling and **not a defect**, and that the escalation path is recorded and deliberately not taken in v2.0.
- The honest limits: sine gains nothing (0.0 dB, correct); the shipped `MorphBlep` under-performs the prototype by 3–5 dB on bleed-ring-dominated cells; two of 90 cells regress slightly (2.33 and 0.74 dB); the `dt = 0.0005` measure-zero missed edge; and that `morph = 1.00` is **not** a crossable boundary while 0.75 (square→pulse) is where the modulated excess peaks.
- The headless counterpart figures (6.289864 V worst under audio-rate MORPH vs 5.518032 V static at the same note and rate; the 0.771832 V excess vanishing at 96 kHz) and a monitoring-level warning ahead of step 5 (T-32-03).
- The full nine-step script with all ten controls named by position **and** hover tooltip, since the panel carries 0 `<text>` elements and cannot be read.

### Question 1 — split, because the reply splits it

**Q1(a) — no zipper noise, no boundary artefacts, nothing clicks or drops out: PASSED.**
The operator exercised the sweep and reported **no artefact**. Because the expected-results block was presented in full first, this absence of complaint counts as evidence under T-32-30 — it is an absence of complaint, not an absence of exposure. This is the half the phase exists to survive, and it survived.

**Q1(b) — is the alias reduction AUDIBLE: NOT ESTABLISHED. Not passed, and not failed.**
The operator states plainly: *"it's hard to remember what the old audio sounded like."* **The audition as designed offered no A/B reference**, so this question had **no way of being answered in either direction**. It is **unevidenced by construction** — a defect in the session's design, not a result and not an operator failing.

**"Seems to work well enough" is not allowed to stand in for it.** That phrase is recorded as what it is — a general impression covering Q1(a) — and is explicitly **not** read as an affirmative answer to Q1(b). The audible-improvement claim rests today on the **automated** spectral evidence alone: the measured per-shape floors above, `failing == 0` over 45 gated cells (TEST-03), and the anti-circularity assertion `naiveDb − correctedDb >= 8.0` at five named cells, which compares two measurements and consults no pinned number. That is strong evidence — it is simply not *ear* evidence, and this file says which one it is.

**MORPH-02's qualification (deferred item 24) — operator-attested, on absence of fault.**
The operator drove the **MORPH CV** jack and the **MORPH CV DEPTH** attenuverter (steps 5–7) without reporting a fault. That is the **first and only** evidence the shell-side `MORPH_PARAM + MORPH_CV_INPUT * 0.1 * MORPH_ATTEN_PARAM` mix works, because D-17 added zero POD fields and no headless driver can reach the attenuverter. Recorded honestly: this rests on **absence of reported fault**, not on a positive measurement, and it cannot become one until something can reach that path headlessly.

### Question 2 — the LFO guardrail: NOT separately attested by the operator

**The operator did not mention step 8 or the Analog LFO.** No attestation is recorded, and none is inferred from *"seems to work well enough"* — inferring one is precisely the fabrication this plan's whole protocol exists to refuse, and it would re-create the inferred-subject problem that Task 1's plugin enumeration was written to eliminate.

**T-32-12 therefore rests on its AUTOMATED evidence for this phase.** That evidence is strong and independent of any ear:

- All **six** shipped LFO `.f32` goldens replay **byte-identical** inside `make test` (three portable drift-off, three `#if defined(__APPLE__)` drift-ON; `0 skipped` on every run).
- `src/AnalogLFO.cpp`, `src/dsp/Waveshape.hpp`, `src/dsp/RackCompat.hpp` and **all 15** paths named in `src/dsp/FROZEN.sha256` return **0 changed lines** over the whole phase diff, checked individually.
- `FROZEN.sha256` is **byte-identical**, asserted by reading bytes: both blobs hash to `734276a7c5579bfda7cad2ecfe214216cee9e894a21e20208c2849d5726d1488` and `cmp` reports identical bytes.

**That is a genuine discharge of T-32-12 — it is just not the operator's eyes and ears.** The step-8 subject was pinned (Task 1: **"Analog LFO"** from `ForgeAudio-AnalogSeries` 2.0.1, versus the stale plain **"LFO"** from `ForgeAudio` 2.0.0), so the *pinning* work stands and is reusable; the *attestation* was not given.

### New finding, ROUTED — the audition asks a question it gives no way to answer

*"It's hard to remember what the old audio sounded like"* is a **methodological defect in this project's UAT design**, not an operator failing, and it will recur in **every** perceptual audition left in this milestone (Phases 33, 34, 35 all end in an in-Rack check). It is therefore not absorbed here — it is filed.

**Filed as `deferred-items.md` item 26**, with the remedy that already exists in-tree: `NaiveVcoCoreMirror` in `tests/test_vco_spectrum.cpp` is a **bit-exact** non-band-limited mirror of the live core (proved by the D-08 reconstruction case at 0 mismatches over 184,320 samples), so a future phase can render matched naive/corrected `.wav` or `.f32` pairs from the same grid points the spectral gate already uses and hand the operator a **switchable A/B** instead of a memory test. Owner: **Phase 36 (goldens/CI), or whichever phase next needs a perceptual verdict, whichever is sooner.**

### Routing table for the reply

| observation | classification | routing |
|---|---|---|
| "Seems to work well enough" — no artefact reported across steps 3–7 | Q1(a) **PASS**; contradicts no assertion | none needed; recorded as evidence under T-32-30 |
| "it's hard to remember what the old audio sounded like" | **Not an artefact report.** A gap in the session's design | **`deferred-items.md` item 26**, owner Phase 36 or the next perceptual phase |
| step 8 / Analog LFO not mentioned | **Absence of attestation**, not an adverse finding | T-32-12 recorded as discharged by automated evidence only; noted as an open attestation gap |

**No observation was invented, paraphrased into a verdict, or omitted.** No artefact was reported, so no gap-closure plan is opened and nothing contradicts an assertion this phase makes.

### Post-session gates — the verdict attaches to a measured-green tree

Re-run **after** the operator session, as the plan requires:

| gate | result |
|---|---|
| `make test` | **94 cases / 94 passed / 0 failed**, **2,622,319** assertions |
| `make strict` | `strict C++11 gate: PASS` over 4 TUs |
| `make guards` | `guard suite: PASS` |

---

## The Code Review Landed After The Checkpoint — And The Phase Closes With Its Items OPEN

`32-REVIEW.md` (commit `3c42652`) ran while this plan was paused at the checkpoint. **It is recorded here so that nobody later reads a green phase as meaning the review was clean. It was not.**

It found **two critical-severity defects in `src/dsp/MorphBlep.hpp`** — CR-01 (`MorphBlep::step` writes out of bounds on a negative `morph`; platform-divergent for a NaN, benign `0` on arm64 but `INT_MIN` on x86-64) and CR-02 (a NaN `character` produces NaN corrections, slipping past `morphBlepCharFactor`'s NaN trap at the three literal-zero-width sites) — plus seven warnings and two info items.

**Both criticals were verified NOT reachable through the shipped call path**, checked rather than assumed: `blep.step` has exactly **one** call site in all of `src/` (`VcoCore.hpp:645`), and `VcoCore.hpp:598-602` conditions **both** arguments immediately above it with the NaN-safe negated pair, with `AnalogVCO.cpp:286-288` applying the same conditioning a second time at the shell boundary. The reviewer reproduced them by calling the header directly with hostile arguments — something no shipping path does.

**Consequence, stated plainly:** they are **High-priority hardening, not ship blockers**; they do **not** invalidate this operator verdict, the phase's green gates, or the CI legs. But `MorphBlep.hpp`'s own banner claims caller-independence in capitals while defending only `dt`, and Phases 33 and 34 are both likely to add call sites — the first unguarded one turns CR-01 into a live out-of-bounds write on the toolchains that actually ship. **Phase 32 closes with these items open and routed** (recommended disposition: a Phase 32 gap-closure plan, or the first task of Phase 33, before any second call site exists).

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

**3. [Recorded, not a code change] Advancement was withheld at the checkpoint and released only after the verdict arrived**

- **Issue:** while Task 2 was unanswered the plan's output — the verdict — did not exist. Ticking anything then would have booked the false green that 32-10's own deviation 3 refused for this exact plan.
- **What was done:** the interim commit `9fac167` carried `status: checkpoint-pending`, `completed: null`, `requirements-completed: []`, ROADMAP at 10/11 and 32-11 unticked, plus a blocking `.continue-here.md` so a resumed `/gsd-execute-phase 32` could not fall through the checkpoint on file-existence alone. All of that was released **only after** the operator replied, and `.continue-here.md` was deleted in the same commit that records the reply.

---

**4. [Rule 2 — Missing Critical] The plan's `<how-to-verify>` asks a question its own script cannot answer, and the operator's reply is what exposed it**

- **Found during:** Task 2, in the reply itself — *"it's hard to remember what the old audio sounded like."*
- **Issue:** step 3 asks the operator to judge that the alias ringing "should be substantially reduced". The script provides **no A/B reference** — no naive rendering to switch against — so the operator is being asked to compare against a memory of a build they last heard weeks ago. That half of Q1 is **unanswerable by construction**, in either direction. This is not specific to this plan: every remaining perceptual audition in the milestone (Phases 33, 34, 35) inherits the same script shape.
- **What was done:** the half was recorded as **unevidenced**, explicitly not as passed and not as failed, and the general remark was refused as a substitute for it. The defect was **filed rather than absorbed** — `deferred-items.md` item 26, with the in-tree remedy (`NaiveVcoCoreMirror` is bit-exact, so matched naive/corrected pairs can be rendered from the grid points the spectral gate already uses) and a named owner.
- **Why this is a Rule 2 and not a complaint about the plan:** an audition that cannot answer its own question produces a verdict that *looks* like coverage and is not — the same vacuous-assertion hazard section one of the deferred register is full of, moved from the test suite into the UAT.

**Total deviations:** 1 recorded ambiguity resolved, 1 auto-added coverage gap closed, 1 recorded withheld-then-released advancement, 1 methodological defect found by the operator's reply and routed. **No `src/` file was touched. `src/AnalogLFO.cpp` and every frozen path are absent from this plan's diff — it modifies no source at all.**

---

## Threat Mitigations Applied

| Threat | Mitigation as landed |
|---|---|
| **T-32-29** (a verdict recorded against a stale install) | Whole-directory flush with `--checksum --delete`; identity pinned by manifest (byte-identical), asset (12 rects, byte-identical), symbols (both models), hash (installed = dist, exact) — **plus a measured before/after showing the binary and the panel asset genuinely moved and the manifest did not**, which is the one check that would have caught this specific stale install. Extended past the plan's list to cover re-extractable archives and the x64 tree. |
| **T-32-12** (the shipped Analog LFO) | The guardrail subject is **pinned by name**: "Analog LFO" from `ForgeAudio-AnalogSeries` 2.0.1, distinguished from the stale "LFO" from `ForgeAudio` 2.0.0 by module name, plugin slug, exported symbol (`_modelAnalogLFO` vs `_modelLFO`) and panel asset — so step 8's subject is no longer inferred. **The operator did not separately attest step 8, and no attestation is inferred from the general remark.** For this phase T-32-12 is discharged by its **automated** evidence: six LFO goldens byte-identical inside `make test`, all 15 `FROZEN.sha256` paths at 0 changed lines over the whole phase diff, `FROZEN.sha256` byte-identical by `cmp`. Strong and independent — but not the operator's eyes and ears, and the file says which. |
| **T-32-30** (an absence of complaint mistaken for an absence of problems) | The expected-results block — measured per-shape floors at all three notes, the corrected roadmap figure, and the honest statement of what will still sound rough — was presented **in full before the reply was taken**, so the no-artefact half of Q1 is an absence of complaint rather than an absence of exposure. The reply is recorded **verbatim** and then **split**: the half the session could evidence is discharged, the half it could not is recorded as unevidenced with its cause, and **"seems to work well enough" is explicitly refused as a stand-in** for the question it does not answer. Nothing was invented, paraphrased into a verdict, or omitted. |
| **T-32-03** (out-of-range output reaching speakers) | Invariant 6's outer bound (10.0 V against Rack's ±12 V norm) is asserted across the audio-rate MORPH grid and green in the pre-session `make test`. The session script starts at CHARACTER 0 and a mid-keyboard note, and the checkpoint carries an explicit monitoring-level warning ahead of step 5, the loudest case. |
| **T-32-SC** (package installs) | Zero packages installed. `plugin.json` gains no dependency. |

## Known Stubs

None in this plan's output. **Two things that could be mistaken for stubs, named so they are not:**

1. **Q1(b) is recorded as unevidenced, not as a placeholder.** It is the honest state of a question the session had no instrument to answer, with the cause stated, the remedy identified in-tree, and an owner attached (item 26). A blank or an optimistic tick would have been the stub.
2. **The review's CR-01 and CR-02 remain open.** They are not stubs in this plan's output — this plan modifies no source — but the phase closes with them open and routed, stated in its own section above so a green phase is not read as a clean review.

## Threat Flags

None. No network, auth, file-access or schema surface introduced; no source file modified.

## Issues Encountered

**One anomaly, investigated and closed:** four control-name literals absent from the installed binary's string pool. Resolved to arm64 libc++ short-string immediate materialisation (`mov x8, #0x4d46` / `movk … #0x4420` / `#0x7065` / `#0x6874` = `"FM Depth"`), confirmed by disassembly, identical across all three binaries. Not an install-identity finding.

**One finding routed rather than absorbed:** the audition asks whether an improvement is audible but supplies no A/B reference — `deferred-items.md` item 26.

## Next Phase Readiness

**Phase 32 is complete — 11/11 plans — and closes with three things deliberately left open and each pointed somewhere:**

- **`32-REVIEW.md` CR-01 and CR-02** — real defects in `MorphBlep.hpp`, verified unreachable through the single shipped call site, High-priority hardening. **Recommended: a Phase 32 gap-closure plan or the first task of Phase 33, before any second call site exists.** Phase 33 adds exactly such a call site (`addStep` at the hard-sync seam), which is what makes the ordering matter.
- **Deferred item 26 — the missing A/B reference.** Owner: Phase 36, or whichever phase next needs a perceptual verdict, whichever is sooner. Phases 33, 34 and 35 all end in an in-Rack check and all inherit the defect.
- **Deferred item 24 — MORPH-02's shell-mix qualification.** Now operator-attested on absence of fault, which is the strongest evidence available while no headless driver can reach the attenuverter. It does not become a measurement until one can.

Phase 33 also inherits, unchanged: `forge::VcoCore` and `forge::MorphBlep` with no source modification from this plan, the pinned install-and-flush procedure, and the browser-disambiguation fact that two Forge plugins share the brand string `Forge Audio`.

## Self-Check: PASSED

- `dist/ForgeAudio-AnalogSeries/` and `dist/ForgeAudio-AnalogSeries-2.0.1-mac-arm64.vcvplugin` — FOUND on disk
- Installed `plugin.json` — FOUND; version 2.0.1, 2 modules, byte-identical to repo
- Installed `res/AnalogVCO.svg` — FOUND; 896 bytes, 12 rects, byte-identical to repo
- Installed `plugin.dylib` — FOUND; sha256 `431b9de6…` equals the dist artefact exactly; differs from the pre-flush `5061619c…`
- `nm` — both `_modelAnalogLFO` and `_modelAnalogVCO` FOUND
- Two Forge plugin directories enumerated with slug, version and module list
- Operator reply recorded **verbatim**, one sentence, unparaphrased; Q1 split; no attestation invented for Q2
- `deferred-items.md` item 26 — FOUND on disk with owner and in-tree remedy
- `.continue-here.md` — DELETED; the checkpoint it blocked is answered
- `make test` 94/94/0 at 2,622,319, `make strict` PASS, `make guards` PASS — re-run **after** the operator session

---
*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Completed: 2026-08-27*
