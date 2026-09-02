---
phase: 33-hard-sync
plan: 10
subsystem: tools
tags: [hard-sync, d-13, d-14, d-15, d-16, audition-renderer, ab-reference, register-item-26, guard-costs-prepaid, two-master-finding, sync-02-declined, apple-clang-only, never-committed]

# Dependency graph
requires:
  - phase: 33-hard-sync
    plan: 06
    provides: "THE SEAM AND THE TELEMETRY THIS RENDERER RECONSTRUCTS FROM: forge::MorphBlep::addPastStep called from forge::VcoCore, tel.syncCorrection populated off the accumulator, and the withheld-leg relationship WITH ITS MEASURED ONE-ULP ERROR BAR"
  - phase: 33-hard-sync
    plan: 05
    provides: "the sync sub-grid's five axes, its master generator (makeSyncMaster), its cell struct and the four seed literals with copy-and-assign POD construction"
  - phase: 33-hard-sync
    plan: 07
    provides: "the per-ratio spectral improvement table this renderer's honesty rests on, including the measured -1.0281 dB at ratio 5.50, and the written refusal of a Phase-32-shaped improvement gate"
  - phase: 33-hard-sync
    plan: 08
    provides: "the time-domain reset-step metric this renderer reimplements per render point, kSyncResetDeltaBoundV, and the 56-of-420 negative-margin finding this plan reproduces AND scopes"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "deferred register item 26 — the audition with no A/B reference — which is what this plan remedies; and the operator verdict that exposed it"
provides:
  - "tools/render_sync_ab.cpp — the reusable A/B audition renderer: TWO LEGS FROM ONE PASS through the real forge::VcoCore, parameterised by a render-point table and a core-configuration pair"
  - "the `make audition` target, AUDITION_BIN, AUDITION_OUT and the build-test/audition/ output directory — uncommitted by construction, at zero .gitignore cost"
  - "the tests/check_includes.sh VCO_SIDE_ALLOW entry \"tools/render_sync_ab.cpp\" — the list's FIRST entry outside src/ and tests/, pre-registered before the file existed"
  - "a 44-byte RIFF/WAVE 16-bit PCM writer with every field written little-endian by hand and commented by name — NO dependency added"
  - "IN-TOOL MECHANICAL PAIR VERIFICATION that re-runs on every invocation: same length, not bit-identical, neither silent, and differs AS ENCODED — proved able to fail"
  - "THE TWO-MASTER FINDING: this phase's spectral and time-domain grids do not share a master frequency, and 33-08's master never wraps between two samples (g == 1.0 exactly on every wrap)"
  - "the complete expected-results material plan 33-12 presents BEFORE the operator replies, with a named list of what the pair CANNOT evidence"
affects: [33-11, 33-12, 34]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Paying a new translation unit's guard costs BEFORE it lands, and MEASURING that the pre-registration is accepted with the source file absent rather than assuming it"
    - "Proving a build-system skip filter load-bearing with a CONTROL (an unfiltered goal under the same bogus SDK path) rather than by reading the filter"
    - "A rendered audition pair generated on demand into an already-ignored directory, so 'never becomes a pinned golden' is enforced by construction rather than by discipline"
    - "Expressing an A/B's legs as DATA (a scale factor k on a recording-only telemetry float) rather than as a branch in the render loop, so a later phase changes a table"
    - "Choosing render points HONESTLY — including the region where the correction measurably loses, and a null-point control where the two legs should be indistinguishable"
    - "Verifying the artefact AS ENCODED and not only in the float domain, because a difference below one quantisation step vanishes in the file the operator actually opens"
    - "Discovering that two instruments' identically-named cells differ on an axis their labels do not carry, and landing the finding in the table rather than retyping the inherited figure"

key-files:
  created:
    - tools/render_sync_ab.cpp
  modified:
    - Makefile
    - tests/check_includes.sh

key-decisions:
  - "BOTH guard costs were paid before the translation unit landed, and the allowlist entry was MEASURED accepted by `make guards` with the source file ABSENT — so no reordering was needed"
  - "The skip-filter entry was proved LOAD-BEARING by a control, not by inspection: `make -n audition RACK_DIR=/definitely/not/here` reaches the rule, while the unfiltered `make -n strict` under the same path dies on the missing plugin.mk"
  - "THE TWO LEGS COME FROM ONE PASS through the real core via tel.syncCorrection — one construction site, no mirror, no second pass, no flag in the shipped body — and NO bit-exact equality is written against the reconstruction, per 33-06's measured one-ulp error bar"
  - "THE SCALE IS DERIVED FROM THE SUITE'S OWN PINNED BOUND: 10.0 V = full scale, which is kHostileBoundV, so a clipped sample means the core exceeded the tier the suite asserts. Measured clip count: 0"
  - "NEVER normalised per leg, stated in the source. The LEVEL-MATCH metric is RMS, not peak — the peak lands ON a reset sample, which is exactly the sample the correction modifies, so the peaks are SUPPOSED to differ (measured by up to 1.075448 V)"
  - "THE TWO PHASE-33 INSTRUMENTS DO NOT SHARE A MASTER FREQUENCY, and the five-axis cell labels do not say so. 33-08's 1/128 dyadic increment gives wrap fraction g == 1.000000000 EXACTLY on every wrap, so its master NEVER wraps between two samples; the spectral master's coprime K_m spreads g over [0.010752688, 1.0]. That alone reverses the sign of the correction's benefit"
  - "kSyncResetDeltaBoundV = 9.90 V is EXCEEDED on the spectral master (measured 9.999983 V) and reproduces EXACTLY at K_m = 32 (9.793601 V). NOT widened, NOT touched — escalated to plan 33-11 per the anti-softening rule"
  - "The mechanical pair verification lives IN THE TOOL, not in a one-off script, so it re-runs on every invocation and Phase 34 inherits it"
  - "SYNC-02 is DECLINED — the TWELFTH consecutive decline. This plan builds the apparatus for a perceptual verdict; it does not take the verdict"

patterns-established:
  - "When a plan's own render reproduces one instrument's figure EXACTLY under one parameterisation and contradicts it under another, the discriminating parameter is the finding — report it, scope both instruments, and touch neither's constants"
  - "An A/B renderer's level-match claim must be made on RMS, because the peak of a hard-synced waveform is the artefact under audition"
  - "Verify an audition artefact AS ENCODED: 16-bit at a 10 V full scale resolves 305.2 uV, and corrections below that are real in the buffer and absent from the file"

requirements-completed: []  # SYNC-02 is in this plan's frontmatter and is DECLINED — see Decisions #9

coverage:
  - id: D1
    description: "Both of the renderer's guard costs are paid deliberately, with rationale, before the file that incurs them exists"
    verification:
      - kind: other
        ref: "tests/check_includes.sh VCO_SIDE_ALLOW grew 8 -> 9 entries (array range 378-388, quote-line count 8 -> 9). `make guards` exits 0 with the entry present and tools/render_sync_ab.cpp ABSENT — the pre-registration was ACCEPTED, so no reordering was needed. `ls tests/render_sync_ab.cpp` returns non-zero"
        status: pass
      - kind: other
        ref: "Makefile:30 `ifeq ($(filter test capture guards audition,$(MAKECMDGOALS)),)`. PROVED LOAD-BEARING BY A CONTROL: `make -n audition RACK_DIR=/definitely/not/here` reaches the rule; the unfiltered `make -n strict` under the same RACK_DIR fails with `/definitely/not/here/plugin.mk: No such file or directory`"
        status: pass
    human_judgment: false
  - id: D2
    description: "The target compiles with the TEST compiler flags, not a new variable"
    verification:
      - kind: other
        ref: "`$(CXX) $(TEST_CXXFLAGS) tools/render_sync_ab.cpp -o $@`; observed compile line `c++ -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off`. No new flags variable added"
        status: pass
    human_judgment: false
  - id: D3
    description: "The two legs come from the SAME pass through the real core, with no second core, mirror, pass or flag"
    verification:
      - kind: other
        ref: "`leg_k[n] = out_shipped[n] + 5.f*(k-1.f)*tel.syncCorrection[n]`, reading forge::VcoCore::Telemetry::syncCorrection back AFTER step(). ONE construction site (`forge::VcoCore core;`, tools/render_sync_ab.cpp:439). grep -c 'Mirror' = 0, 'NaiveVcoCoreMirror' = 0. Confirmed at system level: differing samples EQUAL reset count on all 7 points (2003/2003/2002/2003/2002/2002/689)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The renderer is parameterised by grid and by a configuration pair, so a later phase changes a table"
    verification:
      - kind: other
        ref: "RENDER_POINTS[] (10 fields incl. masterKm and duration) and CORE_PAIR[] (fileTag, legName, correctionScale k, description), both separate from renderPoint(). PROVED REUSABLE IN THIS PLAN: render point 07 differs from 06 ONLY in masterKm, which is what produced the two-master finding"
        status: pass
    human_judgment: false
  - id: D5
    description: "The four documented seed literals are copied verbatim and the drift field is zeroed"
    verification:
      - kind: other
        ref: "tools/render_sync_ab.cpp:135-138 `0x1234ULL / 0x5678ULL / 0x9E3779B9ULL / 0x7F4A7C15ULL` against tests/VcoBlockDriver.hpp:42-43 — identical. `base.drift = 0.f`. No seed invented; the (0,0) fixed point is named in the banner (T-33-34)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The output is valid uncompressed audio with a correct, auditable header"
    verification:
      - kind: other
        ref: "Header decoded byte by byte: RIFF / ChunkSize 176436 / WAVE / 'fmt ' / 16 / AudioFormat 1 / NumChannels 1 / SampleRate 44100 / ByteRate 88200 / BlockAlign 2 / BitsPerSample 16 / 'data' / 176400. File 176444 bytes = 44 + 176400, and 176436 = 176444 - 8. Independently validated by macOS `afinfo`: WAVE, 1 ch 44100 Hz Int16, duration 2.000000 s"
        status: pass
    human_judgment: false
  - id: D7
    description: "The scale is stated and reported, no leg is normalised, and clipping is counted per leg per point"
    verification:
      - kind: other
        ref: "kVoltsToFullScale = 0.1f (10.0 V = full scale = kHostileBoundV) printed in the run header and derived in the source. Per-leg per-point clipped and non-finite counts printed: 0 and 0 on all 14 files. Per-leg normalisation forbidden in writing; RMS reported as the level-match metric"
        status: pass
    human_judgment: false
  - id: D8
    description: "The pair is verified mechanically before any human hears it, and the verification is proved able to fail"
    verification:
      - kind: other
        ref: "In-tool checks per point: equal file length, NOT bit-identical, neither silent, and differs AS ENCODED. 7 of 7 PAIR CHECK PASS. PROVED ABLE TO FAIL: withheld leg k 0.0f -> 1.0f reds all 7 points (21 FAIL lines) and the program EXITS 1. Cross-checked against an INDEPENDENT Python read of the written files — encoded difference counts agree exactly (1981/2003/2002/1981/2002/2002/689)"
        status: pass
    human_judgment: false
  - id: D9
    description: "Nothing is committed under the build output directory and no ignore-file edit was needed"
    verification:
      - kind: other
        ref: "`git check-ignore -q build-test/audition` succeeds; `git status --porcelain build-test/` empty; `git diff --stat .gitignore` empty. Render is DETERMINISTIC: all 14 files byte-identical across two consecutive `make audition` runs"
        status: pass
    human_judgment: false
  - id: D10
    description: "Nothing shipped moved"
    verification:
      - kind: other
        ref: "Whole-plan diff (1994056..HEAD) is EXACTLY Makefile, tests/check_includes.sh, tools/render_sync_ab.cpp — 784 insertions, 1 deletion. src/AnalogLFO.cpp absent (grep count 0). make test 109 cases / 2,638,713 assertions / 0 failures, UNCHANGED from 33-09. Six LFO goldens byte-identical (9 cases / 49,188 assertions). check_frozen.sh PASS. make strict and make guards exit 0. Zero compiler warnings"
        status: pass
    human_judgment: false
  - id: D11
    description: "The perceptual coverage this pair CANNOT evidence is named rather than booked"
    verification:
      - kind: other
        ref: "Six named items in the 'What This Pair CANNOT Evidence' section below, including the two the plan requires (anything about the module inside Rack, since this is headless; anything about the shipped Analog LFO) plus the residual-versus-intended-step separation that remains SYNC-02's open gap"
        status: pass
    human_judgment: true

# Metrics
duration: 30min
completed: 2026-09-02
status: complete
---

# Phase 33 Plan 10: The A/B Audition Renderer Summary

**Phase 32's operator audition asked whether an improvement was audible and gave the operator nothing to compare against. The verbatim reply — *"Seems to work well enough - but it's hard to remember what the old audio sounded like"* — was correct and useful about a question that was unanswerable by construction. That is deferred register item 26, and from this commit it has a remedy: `make audition` puts the reference in the room.**

Seven matched pairs, fourteen uncommitted 16-bit WAVs, both legs from **one pass through the real `forge::VcoCore`**. Plan 33-12 can now ask a perceptual question the operator can actually answer, and Phase 34's audition-gated DRIFT-03 value inherits the apparatus rather than the debt.

**And building it measured something this phase did not know.** The two instruments Phase 33 has built over what look like the same 420 cells **do not share a master frequency**, and the five-axis cell labels do not say so. Plan 33-08's SC-3 grid drives a master whose every edge lands exactly on a sample boundary — so the sub-sample fraction the whole seam exists to handle **is never exercised there**. That single difference is enough to reverse the sign of the correction's benefit on the same cell. Nothing is red, both instruments are correct about their own grids, and no constant of either was touched.

## Performance

- **Duration:** 30 min
- **Tasks:** 3 of 3
- **Files:** 1 created, 2 modified

## Task Commits

| # | Task | Commit | Type |
|---|---|---|---|
| 1 | Pay both guard costs BEFORE the translation unit lands (the Phase 31 D-23 lesson) | `2aa2109` | chore |
| 2 | The renderer — two legs from one pass, reusable by grid and configuration (D-13 / D-14 / D-16) | `d3fbf39` | feat |
| 3 | The renderer verifies its own pair mechanically before any human hears it (D-15 / D-17) | `bc239e0` | test |

## Files Created/Modified

- `tools/render_sync_ab.cpp` — **created**, 753 lines, of which roughly 300 are banner and rationale. The renderer, its render-point table, its configuration pair, its master generator, its RIFF/WAVE writer and its self-verification.
- `Makefile` — the `audition` skip-filter entry with its reason, `AUDITION_BIN`, `AUDITION_OUT` and the target.
- `tests/check_includes.sh` — one `VCO_SIDE_ALLOW` entry plus a 21-line rationale paragraph.

**Whole-plan diff: 3 files, 784 insertions, 1 deletion.** The single deletion is the `ifeq` filter line being replaced by its widened form.

---

# TASK 1 — BOTH GUARD COSTS, PAID FIRST

## Part A — the allowlist entry, and the finding the plan asked for

`tests/check_includes.sh` section `[1/7]` derives its LFO-side scan set from `find src tests tools` **minus** the named VCO-side files. A file under `tools/` is therefore LFO-side **by default**, exactly as a VCO test TU is, and the renderer includes `dsp/VcoCore.hpp` by construction.

| Measurement | Value |
|---|---|
| Array line range (after) | **378–388** |
| Quote-line count, before | **8** |
| Quote-line count, after | **9** |
| Entry added | `"tools/render_sync_ab.cpp"` — the list's **first entry outside `src/` and `tests/`** |
| `make guards` with the entry present and the source **absent** | **exit 0 — ACCEPTED** |
| Reordering required | **none** |

> **The plan asked to record whether the entry was accepted with the source file absent. It was.** The mechanism is why: the exemption list is consulted only to *skip* paths that `find` actually returned, so a named path that does not exist on disk changes the scan set by nothing at all. The audit's own missing-file detector (`SCAN_MISSING`) fires on paths handed to the *detector*, not on unmatched exemptions. Pre-registration is therefore free here, which is what the Phase-29 and Phase-32 precedents already recorded and what Phase 30's reactive entry paid for by turning the section red.

The 21-line rationale paragraph also records the fact that makes this **one** entry rather than three: Phase 33's sync sub-grid landed in `tests/test_vco_spectrum.cpp` and its time-domain SC-3 gate in `tests/test_vco_core.cpp`, **both already on the list**, so the renderer is the only entry the whole phase needs.

## Part B — the skip filter, and a CONTROL rather than an inspection

```make
ifeq ($(filter test capture guards audition,$(MAKECMDGOALS)),)
include $(RACK_DIR)/plugin.mk
endif
```
`Makefile:30`. The comment gained one clause naming the renderer and the reason: the target is Rack-free and must run on a machine with no SDK checked out, exactly like its three siblings.

**And that was measured rather than read**, because `../Rack-SDK` exists on this host and a green `make audition` proves nothing about a runner without one:

| Goal | `RACK_DIR=/definitely/not/here` | Reading |
|---|---|---|
| `make -n audition` (**in** the filter) | reaches the rule (`No rule to make target 'tools/render_sync_ab.cpp'` — the source did not yet exist) | **plugin.mk was never included** |
| `make -n strict` (**not** in the filter) | `/definitely/not/here/plugin.mk: No such file or directory` | **the filter is what does the work** |

The unfiltered control is the half that makes this evidence. Without it, the first row is equally consistent with `plugin.mk` being irrelevant to every target.

## The target, quoted, with the load-bearing flag

```make
AUDITION_BIN := build-test/audition-render
AUDITION_OUT := build-test/audition

.PHONY: audition
audition: $(AUDITION_BIN)
	@mkdir -p $(AUDITION_OUT)
	./$(AUDITION_BIN)

$(AUDITION_BIN): tools/render_sync_ab.cpp $(TEST_HEADERS)
	@mkdir -p build-test
	$(CXX) $(TEST_CXXFLAGS) tools/render_sync_ab.cpp -o $@
```

**`TEST_CXXFLAGS`, not a new variable**, and the comment says why in terms: same standard, same `-O2`, same `-ffp-contract=off`, so the rendered audio is bit-comparable with what `make test` measures. Observed compile line:

```
c++ -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off tools/render_sync_ab.cpp -o build-test/audition-render
```

A comment also records why the renderer is **not** in `tests/`: `TEST_SOURCES` is a wildcard over that directory, so it would link into the doctest binary — `main()` colliding with doctest's — and would run on every `make test` invocation, contradicting the generated-on-demand decision outright. GNU Make 3.81 compatible throughout: no `$(file ...)`, no `::=`, no `.ONESHELL`.

## Acceptance criteria, checked

| Criterion | Result |
|---|---|
| `make guards` with the entry present, source absent | **exit 0**, finding recorded above |
| `make test` / `make strict` | **exit 0**, 0 failures |
| Allowlist grew by exactly one | **8 → 9** over range 378–388 |
| Skip filter carries the goal | `Makefile:30` quoted above |
| Target uses the TEST flags | **confirmed**, compile line quoted, no new variable |
| `git check-ignore -q build-test/audition` | **`ignored`** |
| `git diff --stat .gitignore` | **no output** |
| `ls tests/render_sync_ab.cpp` | **non-zero** |
| `git diff --name-only` | `Makefile`, `tests/check_includes.sh` — and **not** `src/AnalogLFO.cpp` |

---

# TASK 2 — THE RENDERER

## The reconstruction, quoted, and the member it reads

```cpp
const float s     = core.step(in);
const float corr  = core.tel.syncCorrection;   // read back AFTER step(), off the accumulator
const bool  fired = core.tel.syncFired;
...
const float k = CORE_PAIR[c].correctionScale;
const float x = s + 5.f * (k - 1.f) * corr;
```

The member is **`forge::VcoCore::Telemetry::syncCorrection`**, recording-only, in the pre-multiply domain — the factor of five is `step()`'s own output multiplier. At `k = 1` this is the shipped leg; at `k = 0` it is the leg with the sync correction withheld entirely. Plan 33-08 uses the same expression, and this is the second consumer to decline to write a bit-exact equality against it.

**No second core, no mirror, no second pass, no flag in the shipped body.**

| Check | Result |
|---|---|
| `grep -c 'Mirror'` | **0** |
| `grep -c 'NaiveVcoCoreMirror'` | **0** |
| `grep -c 'VcoCore '` | **2** — see the reported criterion note below |
| Actual construction sites | **1**, `forge::VcoCore core;` at `tools/render_sync_ab.cpp:439`, inside `renderPoint()` |

> **REPORTED, NOT SILENTLY SATISFIED — the twelfth instance in this project of a criterion's mechanism being wider than its own prose.** The criterion asks that `grep -c 'VcoCore ' tools/render_sync_ab.cpp` show *"a single construction site per render point"*. It outputs **2**. The second hit is line 312, inside a configuration-table string: `"A / SHIPPED (sync BLEP active - what forge::VcoCore does today)"`. A bare `grep` counts a comment and a string literal exactly like a declaration — the same failure class as Phase 30's LFO-filename zero-count and Phase 31's `std::pow` prohibition. **Nothing was renamed to make the number 1.** The property the criterion reaches for is fully satisfied and is reported directly: one `forge::VcoCore` object exists per render point, constructed at line 439, and it is the only one in the file. `grep -c 'bandLimit'` is likewise **1**, and that hit is the banner sentence *forbidding* such a flag.

## The nothing-owed-forward property, observed at system level on all seven points

Differing samples between the two legs **equal the reset count**, exactly, on every render point:

| point | resets fired | differing samples (float) | ratio |
|---|---|---|---|
| 01 | 2003 | **2003** | 1.000 |
| 02 | 2003 | **2003** | 1.000 |
| 03 | 2002 | **2002** | 1.000 |
| 04 | 2003 | **2003** | 1.000 |
| 05 | 2002 | **2002** | 1.000 |
| 06 | 2002 | **2002** | 1.000 |
| 07 | 689 | **689** | 1.000 |

One differing sample per reset, over 618,589 samples of audio. That is 33-06's D-13 accumulator property re-observed on a fourth independent apparatus, and it is what licenses the per-sample subtraction.

## The seed literals, side by side

| | Literals |
|---|---|
| `tools/render_sync_ab.cpp:135-138` | `0x1234ULL` / `0x5678ULL` / `0x9E3779B9ULL` / `0x7F4A7C15ULL` |
| `tests/VcoBlockDriver.hpp:42-43` | `0x1234ULL` / `0x5678ULL` / `0x9E3779B9ULL` / `0x7F4A7C15ULL` |

**Identical.** `base.drift = 0.f`. The banner names the hazard: a `forge::Xoroshiro128Plus` seeded `(0, 0)` is a degenerate fixed point emitting an all-zero stream, which makes `std::normal_distribution`'s rejection loop never terminate — a hung render here, and a **hang while opening a patch** in Rack (T-33-34).

## The output header, decoded field by field

`01-ratio0.50-saw-bandlimited-master1001Hz__leg-A-shipped.wav`:

```
00000000: 5249 4646 34b1 0200 5741 5645 666d 7420  RIFF4...WAVEfmt
00000010: 1000 0000 0100 0100 44ac 0000 8858 0100  ........D....X..
00000020: 0200 1000 6461 7461 10b1 0200            ....data....
```

| Field | Value | Intended |
|---|---|---|
| ChunkID / Format | `RIFF` / `WAVE` | ✓ |
| ChunkSize | 176436 | = file bytes − 8 ✓ |
| Subchunk1Size / AudioFormat | 16 / **1 (linear PCM)** | ✓ |
| NumChannels | **1 (mono)** | ✓ |
| **SampleRate** | **44100** | ✓ |
| ByteRate / BlockAlign | 88200 / 2 | ✓ |
| **BitsPerSample** | **16** | ✓ |
| Subchunk2Size | 176400 | = 88200 samples × 2 ✓ |
| File size | **176444** | = 44 + 176400 ✓ |

**Independently validated by the OS audio stack**, not only by my own decoder — macOS `afinfo`: `File type ID: WAVE`, `Data format: 1 ch, 44100 Hz, Int16`, `estimated duration: 2.000000 sec`, `audio bytes: 176400`.

## Acceptance criteria, checked

| Criterion | Result |
|---|---|
| `make audition` exits 0, at least one matched pair per point | **exit 0, 14 files = 7 matched pairs** |
| `make guards` / `make test` / `make strict` with the renderer present | **all exit 0**, 109 cases / 2,638,713 assertions / 0 failures |
| Two legs from one pass; reconstruction and member quoted | **quoted above**; one construction site |
| Seed literals match the documented four | **quoted side by side above** |
| Valid audio file; header matches intent | **decoded above; `afinfo` concurs** |
| Scale stated and reported; per-leg per-point clip counts | **0.1 V→FS printed; 0 clipped, 0 non-finite on all 14 files** |
| `git status --porcelain build-test/` | **no output**; `git check-ignore -q build-test/audition` succeeds |
| `grep -c 'Mirror'` | **0** |
| `git diff --name-only` for this task | `tools/render_sync_ab.cpp` only; `src/AnalogLFO.cpp` absent |

---

# THE TWO-MASTER FINDING — THE PLAN'S BIGGEST UNPLANNED EVENT

**Task 2 as drafted attributed plan 33-08's per-cell figures to render points 02 and 05 by matching their five axes. The render contradicted both, so the numbers were measured instead of retyped.**

## What went wrong with the attribution

| Cell (five axes) | 33-08 recorded | This render measured | Verdict |
|---|---|---|---|
| 44.1k, hard-edge, ratio 5.50, square, char 1.00 | margin **−0.246492 V** (correction WORSE) | margin **+1.419190 V** (correction HELPS) | **opposite sign** |
| 44.1k, band-limited, ratio 0.50, pulse 5%, char 0.00 | worst step **9.793601 V** | worst step **9.999983 V** | **above 33-08's pinned 9.90 V bound** |

## The discriminating parameter, found by a controlled experiment

Same renderer, same core, same cell on all five named axes; **only the master cycle count changed** (93 → 32, i.e. master 1001.2939 Hz → 344.5312 Hz, which is the 1/128 dyadic increment 33-08's grid uses):

| `masterKm` | master | margin on 33-08's worst cell | worst step, pulse cell |
|---|---|---|---|
| **93** (spectral sub-grid) | 1001.2939 Hz | **+1.419190 V** | **9.999983 V** |
| **32** (33-08's SC-3 grid) | 344.5312 Hz | **−0.427492 V** | **9.793601 V — 33-08's figure, EXACTLY** |

**Reproducing 33-08's number to six decimal places under its own parameterisation is what makes this a scope finding rather than a contradiction.** The instrument is not wrong; its cell labels are incomplete.

## The mechanism, MEASURED rather than argued

Over 4096 samples, on the master wrap fraction `g`:

| master | wraps | `g` range | spread |
|---|---|---|---|
| `K_m = 93` (coprime to 4096) | 93 | **[0.010752688, 1.000000000]** | **0.989247312** |
| `K_m = 32` (= 1/128, divides the sample grid) | 32 | **[1.000000000, 1.000000000]** | **0.000000000** |

> **PLAN 33-08's TIME-DOMAIN GRID NEVER WRAPS ITS MASTER BETWEEN TWO SAMPLES.** `1/128` divides the sample grid exactly, so `g` is **exactly 1.0 on every one of its 32 wraps** — every master edge lands *on* a sample boundary. The sub-sample fraction the entire hard-sync seam exists to handle is not exercised anywhere on that grid. The spectral sub-grid's coprime `K_m` spreads `g` across almost the whole unit interval instead.

And that explains both discrepancies in one stroke. The seam deposits `−f²·jump/2`, **proportional to `f` squared**, so a reset detected at `f` near zero receives essentially no correction and reproduces the full naive step. On a coprime master `f` is equidistributed, so such a reset always occurs and the *worst-case* step converges on the uncorrected value — measured, the correction removes **eighteen microvolts of a ten-volt step**. On the `1/128` master `f` takes few distinct values and no such reset exists.

## Scanned across all 140 same-axis cells, both masters

| master | cells with a NEGATIVE reset-step margin | worst |
|---|---|---|
| `K_m = 32` (33-08's) | **20 of 140 (14.3%)** | **−0.427492 V** |
| `K_m = 93` (spectral) | **7 of 140 (5.0%)**, all at the ratio-1.00 null point | **−0.003739 V** |
| *33-08's own figure, for comparison* | *56 of 420 (13.3%)* | *−0.246492 V* |

The `K_m = 32` proportion matches 33-08's to within a percentage point, which is the corroboration that this renderer's metric is 33-08's metric.

## What was done, and what was deliberately NOT done

- **Not done:** no constant of plan 33-08's was touched. `kSyncResetDeltaBoundV` is still `9.90f`, `kSyncAntiCircularityMarginV` still `0.04f`, and its two `TEST_CASE`s are unmodified and green. **Widening the bound to 10.0 V would have made this go away and would have deleted the property that makes it evidence** — which is exactly the move register item 2 forbids in writing.
- **Not done:** the render-point table was not quietly re-pointed at `K_m = 32` to make the inherited numbers true.
- **Done:** `masterKm` is a table field, so render points **06 and 07 are the same cell on the two masters**, adjacent in the output, for the operator to hear the region flip. That pair is the whole justification for parameterising by master rather than hard-coding one.
- **Done:** the finding is written into the renderer's own banner (a 40-line two-master note), into two new STATE.md Blockers entries, and into deferred register item 1 below, pointed at plan 33-11.

**Nothing is red.** Each instrument is correct about its own grid. What is now known is that a per-cell figure from either one carries an unstated sixth axis.

---

# TASK 3 — THE PAIR, VERIFIED BEFORE ANY HUMAN HEARS IT

The mechanical checks live **in the tool**, not in a one-off script, so they re-run on every invocation and Phase 34 inherits them along with the renderer.

## Verified per render point, from the files on disk

| point | bytes A | bytes B | same length | NOT bit-identical | neither silent | differs as encoded | PAIR CHECK |
|---|---|---|---|---|---|---|---|
| 01 | 176444 | 176444 | ✓ | ✓ 2003 | ✓ | ✓ 1981 | **PASS** |
| 02 | 176444 | 176444 | ✓ | ✓ 2003 | ✓ | ✓ 2003 | **PASS** |
| 03 | 176444 | 176444 | ✓ | ✓ 2002 | ✓ | ✓ 2002 | **PASS** |
| 04 | 176444 | 176444 | ✓ | ✓ 2003 | ✓ | ✓ 1981 | **PASS** |
| 05 | 176444 | 176444 | ✓ | ✓ 2002 | ✓ | ✓ 2002 | **PASS** |
| 06 | 176444 | 176444 | ✓ | ✓ 2002 | ✓ | ✓ 2002 | **PASS** |
| 07 | 176444 | 176444 | ✓ | ✓ 689 | ✓ | ✓ 689 | **PASS** |

**Cross-checked against an INDEPENDENT Python read of the written files**, so the self-check is not reading its own buffers back at itself. The encoded difference counts agree exactly: `1981 / 2003 / 2002 / 1981 / 2002 / 2002 / 689`.

## The legs' non-identity, quantified per point

| point | differing samples (float) | largest \|A−B\| | differing (16-bit) | below one LSB |
|---|---|---|---|---|
| 01 | 2003 of 88200 (2.2710%) | **2.427443 V** | 1981 | **22** |
| 02 | 2003 of 88200 (2.2710%) | **4.853238 V** | 2003 | 0 |
| 03 | 2002 of 88200 (2.2698%) | **1.004344 V** | 2002 | 0 |
| 04 | 2003 of 88200 (2.2710%) | **2.435740 V** | 1981 | **22** |
| 05 | 2002 of 88200 (2.2698%) | **0.041873 V** | 2002 | 0 |
| 06 | 2002 of 88200 (2.2698%) | **1.421893 V** | 2002 | 0 |
| 07 | 689 of 88200 (0.7812%) | **0.427492 V** | 689 | 0 |

> **A SMALL HONEST FINDING THE PLAN DID NOT ASK FOR: at points 01 and 04, 22 of the 2003 per-reset corrections are SMALLER THAN ONE QUANTISATION STEP and vanish in the file the operator actually opens.** 16-bit at a 10 V full scale resolves **305.2 microvolts**. Those 22 corrections are real in the float buffer and absent from the WAV. They are at roughly −84 dBFS and are irrelevant to an audition, but *"the legs differ"* must be a claim about the artefact rather than about a buffer nobody hears, so the renderer now reports both counts and fails if the encoded files are identical while the float legs differ.

## LEVEL MATCH — and why the metric is RMS, not peak

| point | RMS A | RMS B | delta | in dB | peak A | peak B | peak delta |
|---|---|---|---|---|---|---|---|
| 01 | 2.861173 | 2.891436 | −0.030264 | **−0.0914** | 4.887732 | 4.998322 | −0.110590 |
| 02 | 4.915878 | 4.945823 | −0.029945 | **−0.0527** | 5.000000 | 5.000000 | −0.000000 |
| 03 | 2.766418 | 2.799046 | −0.032628 | **−0.1018** | 4.665223 | 4.868432 | −0.203209 |
| 04 | 2.411413 | 2.444549 | −0.033136 | **−0.1185** | 3.906088 | 4.981536 | **−1.075448** |
| 05 | 2.798800 | 2.798833 | −0.000033 | **−0.0001** | 4.870414 | 4.912288 | −0.041873 |
| 06 | 3.973077 | 3.987660 | −0.014582 | **−0.0318** | 4.879554 | 4.879554 | 0.000000 |
| 07 | 4.010931 | 4.011824 | −0.000893 | **−0.0019** | 4.898814 | 4.898814 | 0.000000 |

> **THE PLAN'S CRITERION SAYS "the two legs' peak levels are close, because they are supposed to be level-matched". AT POINT 04 THEY DIFFER BY 1.075448 V — 21 percent — AND THAT IS CORRECT BEHAVIOUR, NOT A RIG FAULT.** The peak of a hard-synced waveform lands **on a reset sample**, which is precisely the sample the correction modifies, so the two legs' peaks are *supposed* to differ wherever the correction is doing its job. Judging level-matching on the peak would report the DSP as an apparatus defect — the exact inversion this whole plan exists to prevent.
>
> **The level-match evidence is RMS, and it is unambiguous: every point matches within 0.1185 dB, and the null-point control matches within 0.0001 dB.** That is a property of the design rather than of tuning: **one** stated scale factor is applied identically to both legs and no leg is ever normalised. The criterion's intent is satisfied; its stated mechanism is not the right measurement, and the numbers are given rather than the verdict.

## Determinism, and the self-check proved able to fail

- **Deterministic:** all 14 files **byte-identical** across two consecutive `make audition` invocations. The operator regenerating per session gets the same audio.
- **Proved able to fail:** the withheld leg's `correctionScale` changed `0.0f → 1.0f`, so both legs are the shipped leg. **All 7 render points red** (21 `FAIL` lines: bit-identical, identical-as-encoded, `PAIR CHECK FAIL`) and the program **exits 1**, which reds `make audition`. Restored and re-verified: **7 of 7 PASS**.

## Acceptance criteria, checked

| Criterion | Result |
|---|---|
| `make audition` / `make test` / `make strict` / `make guards` | **all exit 0** |
| Per point: both sizes, non-identity, both peaks, both clip counts | **all seven tabulated above** |
| Non-identity quantified: differing samples and largest difference | **tabulated above, float AND encoded** |
| Expected-results material complete against the seven bullets | **enumerated and marked below — 7 of 7 present** |
| What the pair cannot evidence, ≥ 2 named items | **6 named below**, including both required |
| `git status --porcelain` shows nothing under the build output directory | **no output** |
| Whole-plan diff is exactly the three files; LFO goldens byte-identical | **confirmed** — `Makefile`, `tests/check_includes.sh`, `tools/render_sync_ab.cpp`; goldens **9 cases / 49,188 assertions, 0 failures** |

---

# BANKED FOR PLAN 33-12 — THE EXPECTED-RESULTS BLOCK, IN FULL, BEFORE THE OPERATOR REPLIES

D-17 precedent 2 is binding: the whole block is presented **before** the operator answers, so an absence of complaint is an absence of complaint rather than an absence of exposure. All seven required bullets, each marked present.

### ✅ 1. The exact command, and where the files land

```
cd "<repo root>" && make audition
```
Output: **`build-test/audition/`** — 14 files, ~2.4 MB total, about 3 seconds to render.
**They are NOT committed and must be regenerated in every session that needs them.**

### ✅ 2. The file naming scheme, and which name is which leg

`<point-label>__leg-A-shipped.wav` and `<point-label>__leg-B-withheld.wav`.

- **`leg-A-shipped`** = **what the module does today.** The sync BLEP is active — `forge::VcoCore` calling `forge::MorphBlep::addPastStep`, landed by plan 33-06.
- **`leg-B-withheld`** = **the reference.** The identical reset with the sync correction removed entirely; what the module did before plan 33-06.

The point label leads the filename, so **each pair sorts adjacently** in any file browser and `A` precedes `B`. Verified:

```
01-ratio0.50-saw-bandlimited-master1001Hz__leg-A-shipped.wav
01-ratio0.50-saw-bandlimited-master1001Hz__leg-B-withheld.wav
02-ratio0.50-pulse-bandlimited-master1001Hz__leg-A-shipped.wav
02-ratio0.50-pulse-bandlimited-master1001Hz__leg-B-withheld.wav
...
07-ratio5.50-square-hardedge-master344Hz__leg-A-shipped.wav
07-ratio5.50-square-hardedge-master344Hz__leg-B-withheld.wav
```

### ✅ 3. The render points, in plain language

All at 44.1 kHz, mono, 2.000 seconds each, drift off.

| # | Master note | Slave note | Ratio | Shape knob | Character knob | What it is |
|---|---|---|---|---|---|---|
| **01** | 1001.29 Hz (≈ B5 +24¢) | 500.65 Hz (≈ B4 +24¢) | 0.50 | **saw** (centre) | **0.00** (min) | Where the correction wins most spectrally. A conventional hard-synced saw. |
| **02** | 1001.29 Hz | 500.65 Hz | 0.50 | **narrow pulse** (max) | **0.00** | The largest reset step anywhere — a 10 V single-sample jump, 2003 times a second. |
| **03** | 1001.29 Hz | 1501.94 Hz (≈ F♯6 +26¢) | 1.50 | **saw** | **0.00** | The middle of the sweep, and the clearest time-domain benefit in the set. |
| **04** | 1001.29 Hz | 5507.12 Hz (≈ F8 −25¢) | 5.50 | **saw** | **0.00** | Where the correction measurably **loses** spectrally. |
| **05** | 1001.29 Hz | 1001.29 Hz | 1.00 | **saw** | **0.00** | **THE CONTROL.** Unity sync barely moves the waveform. The two legs should be indistinguishable. |
| **06** | 1001.29 Hz | 5507.12 Hz | 5.50 | **square** | **1.00** (max) | Plan 33-08's worst cell, on the spectral master — here the correction helps. |
| **07** | **344.53 Hz** (≈ F4 −23¢) | 1894.92 Hz (≈ A♯6 +28¢) | 5.50 | **square** | **1.00** | **The same cell on 33-08's master, where the correction LOSES.** Audition against 06. |

### ✅ 4. The expected difference, described honestly — IT IS SMALL

**Tell the operator to expect a subtle difference, not a dramatic one.** An operator told to expect an obvious "click disappearing" would be told something **no instrument in this phase supports**, and would then report a defect that is not there. Three instruments, in their own words:

| Instrument | Plan | What it measured |
|---|---|---|
| Spectral alias floor, grid-wide mean | **33-07** | **+0.5827 dB** improvement. Phase 32's own improvement-gate shape (≥ 8.0 dB) fails here **by construction** and was refused in writing. |
| Spectral, plan 33-05's leg table (`none` − `pastEdge`, mean dB) | **33-05** | **hard-edge +0.061 / −0.010 / +0.174 dB** and **band-limited +1.053 / +0.996 / +1.222 dB** at 44.1 / 48 / 96 kHz. The plan asked for this figure specifically: **it is under one and a quarter decibels everywhere, and within noise on a hard-edged master.** |
| Time-domain worst reset step | **33-08** | **9.793601 V** shipped vs **10.000000 V** withheld on its grid — the BLEP removes about **2 %** of the worst-case step. |
| Time-domain worst reset step | **this plan**, spectral master | **9.999983 V** vs **10.000000 V** — **eighteen microvolts of a ten-volt step.** |

**And it is NOT a uniform improvement.** Per-ratio spectral mean, `none` − `pastEdge` (positive = shipped better), plan 33-07 over 60 cells each:

```
0.50  +2.4495     1.50  +0.7247     3.50  -0.1911
0.75  +1.9150     2.50  +0.2051     5.50  -1.0281   <-- WORSE THAN NONE
1.00  +0.0037
```

Per-point measured differences the operator will actually be listening to:

| # | Largest single-sample difference | RMS difference | Reset-step margin | Honest expectation |
|---|---|---|---|---|
| 01 | 2.427443 V | −0.0914 dB | +0.003555 V | Subtle, on 2.27 % of samples |
| 02 | 4.853238 V | −0.0527 dB | +0.000018 V | Big per-sample change; **the worst-case step is essentially uncorrected** |
| 03 | 1.004344 V | −0.1018 dB | **+1.001893 V** | The most likely point to hear a real improvement |
| 04 | 2.435740 V | −0.1185 dB | +0.019630 V | Spectrally **worse**; may sound *rougher*, not better |
| 05 | 0.041873 V | **−0.0001 dB** | −0.001738 V | **Should be indistinguishable** |
| 06 | 1.421893 V | −0.0318 dB | **+1.419190 V** | Audible improvement expected |
| 07 | 0.427492 V | −0.0019 dB | **−0.427492 V** | **Correction makes the worst step LARGER** |

### ✅ 5. The expected artefact behaviour

- **No click at the reset instants** on the shipped leg (`leg-A`) — this is the perceptual half of SC-3 and it has **no automated instrument**, which is why it is being auditioned at all.
- **Sharp rather than smeared character.** The shipped leg should read *buzzy*, not *dull* or *softened*. The correction is a one-sample band-limiting residual, so it must not remove the bite of the reset; a leg that sounds smoothed or muffled relative to `leg-B` is a **defect report**, not a success.
- **Reset rate for reference:** points 01–06 reset **2002–2003 times per second** (audible as a ~1 kHz buzz component); point 07 resets **689 times per second**.
- **The correction affects 2.27 % of samples** and nothing else. Any difference in the *body* of the tone — pitch, level, timbre away from the reset instants — is not something this change can cause.

### ✅ 6. The stated scale and the clipped-sample counts

- **Scale: 0.100000 volts-to-full-scale — 10.0 V maps to digital full scale.** The number is not arbitrary: 10.0 V is `kHostileBoundV`, the outer output tier every scenario in `tests/test_vco_core.cpp` asserts, **so a clipped sample would mean the core exceeded the bound the test suite pins.**
- **Clipped samples: 0. Non-finite samples: 0.** On all 14 files, both legs, all seven points.
- **Expected peak level: about −6 dBFS** (measured peaks 3.906088 V to 5.000000 V, i.e. 0.39–0.50 of full scale). **The files will sound quiet.** That is the stated scale, not a fault, and it is deliberate: the headroom is what guarantees no clipping can hide a level excursion.
- **No leg is normalised**, so `leg-A` and `leg-B` are directly comparable at the same monitor setting. **Do not adjust level between legs** — that is what destroys an A/B.

### ✅ 7. What this pair CANNOT evidence — REFUSED, not booked

The 32-11 precedent is binding: perceptual coverage the script cannot evidence is **refused**, not recorded as passed. This pair cannot evidence:

1. **Anything about the module inside Rack.** This is a **headless render** of `forge::VcoCore`. It says nothing about the SYNC jack, the panel, the widget, patch load/save, CPU cost, or the shell's field forwarding. The in-Rack check remains a separate manual row, with the whole-tree `rsync -a dist/ForgeAudio-AnalogSeries/` flush and the module named as **"the Analog VCO under Forge Audio Analog Series"** (D-17.4 — the operator's Rack tree carries a second, differently-slugged `ForgeAudio` plugin).
2. **Anything about the shipped Analog LFO.** Not one sample here comes from `forge::LfoCore`. The LFO guardrail is discharged for this plan by **automated** evidence only — six goldens byte-identical (9 cases / 49,188 assertions), `check_frozen.sh` PASS, `src/AnalogLFO.cpp` absent from the whole-plan diff — and that is the kind of evidence it is.
3. **The RESIDUAL discontinuity SC-3 actually forbids, separated from the INTENDED reset step.** Both legs contain the intended near-full-scale step, so their difference measures the correction's **size**, not the residual's. This is 33-08's register item 1, it is still **SYNC-02's whole remaining gap**, and a favourable audition **must not be booked as closing it**.
4. **Anything cross-toolchain.** Every volt in this document is an **Apple-clang** figure, on this arm64 host, at `-O2 -ffp-contract=off`. Phase 32 measured this project's spectral instrument toolchain-dependent by up to **3.02596 dB** with no `src/` behaviour differing at all.
5. **Anything about drift, spread, or the audio-rate analog engine.** `drift = 0.f` throughout and no `*Spread` coefficient is exercised beyond the fixed seeds. Phase 34 owns those.
6. **Any parameter combination outside the seven points.** Seven cells of a 420-cell grid, at one sample rate, at two of the seven ratios' extremes plus the null point. A verdict on these seven is a verdict on these seven.

---

# Gate Results

| Gate | Result |
|------|--------|
| `make audition` | **PASS**, exit 0 — 14 files, 7 of 7 `PAIR CHECK PASS`, 0 clipped, 0 non-finite |
| `make test` | **PASS** — 109 cases, 2,638,713 assertions, 0 failures (**unchanged from 33-09**) |
| `make strict` (C++11, `-pedantic-errors`) | **PASS**, exit 0 |
| `make guards` | **PASS**, exit 0 (all three scripts) |
| `bash tests/check_frozen.sh` | **PASS** — D-05 manifest + goldens + negative control |
| Six shipped-LFO goldens | **byte-identical** — 9 cases / 49,188 assertions, 0 failures |
| Compiler warnings in the new TU | **0** (`-Wall -Wextra`) |
| Render determinism | **all 14 files byte-identical** across two consecutive runs |
| Self-check proved able to fail | **7 of 7 points red, exit 1** under the `k 0.0f → 1.0f` probe |
| `git check-ignore -q build-test/audition` | **`ignored`**; `git diff --stat .gitignore` **empty** |
| `git status --porcelain build-test/` | **no output** — nothing tracked or staged |
| `src/AnalogLFO.cpp` in the whole-plan diff | **absent** (grep count 0) |
| `git diff --name-only 1994056..HEAD` | `Makefile`, `tests/check_includes.sh`, `tools/render_sync_ab.cpp` |
| Four frozen shared headers in the diff | **none** |

---

# Decisions Made

1. **BOTH GUARD COSTS WERE PAID FIRST, AND THE PRE-REGISTRATION WAS MEASURED RATHER THAN ASSUMED.** The plan asked to record whether the allowlist accepts an entry whose path does not exist. **It does**, and the mechanism is now written down: the exemption list is consulted only to skip paths `find` actually returned, so an entry for an absent file changes the scan set by nothing. That makes pre-registration free, which is what the Phase-29 and Phase-32 precedents asserted and what Phase 30 paid for by adding its entry reactively.

2. **THE SKIP FILTER WAS PROVED LOAD-BEARING BY A CONTROL, NOT BY READING IT.** `../Rack-SDK` exists on this host, so a green `make audition` is evidence about nothing. Running the goal under a deliberately bogus `RACK_DIR` **and** running an unfiltered goal under the same path is what distinguishes "the filter works" from "`plugin.mk` was never needed". This is the `check_includes.sh` `[6/7]` two-direction-control discipline applied to a build file.

3. **THE SCALE IS DERIVED FROM THE SUITE'S OWN PINNED BOUND RATHER THAN CHOSEN FOR LOUDNESS.** 10.0 V = full scale **is** `kHostileBoundV`. A clipped sample therefore *means something*: the core exceeded the tier five scenarios in `tests/test_vco_core.cpp` assert. A scale picked to make the files loud would have made the clip count a mixing artefact instead of evidence. Cost: the files sit around −6 dBFS and sound quiet, which is disclosed in the expected-results block.

4. **THE LEVEL-MATCH METRIC IS RMS, AND THAT IS A MEASUREMENT RATHER THAN A PREFERENCE.** The plan's criterion asks that the legs' **peaks** be close. Measured, they differ by up to **1.075448 V** — because the peak of a hard-synced waveform lands *on a reset sample*, the one sample the correction modifies. Judging level-matching there would report the DSP as a rig fault, inverting the plan's whole purpose. RMS matches within **0.1185 dB** everywhere and within **0.0001 dB** on the null-point control, which is the property one scale factor and no per-leg normalisation actually guarantee.

5. **THE TWO-MASTER FINDING WAS LANDED RATHER THAN THE INHERITED NUMBERS RETYPED.** Two of the table's `why` strings were drafted attributing plan 33-08's per-cell figures to same-named cells; the render contradicted both. Rather than adjust the prose to fit or the table to fit the prose, the discriminating parameter was isolated by controlled experiment (only `masterKm` changed), the mechanism was measured (`g ≡ 1.0` versus `g ∈ [0.0108, 1.0]`), and **33-08's figure was reproduced exactly under its own parameterisation** — which is what makes this a scope finding rather than a disagreement. See the dedicated section above.

6. **NO CONSTANT OF PLAN 33-08's WAS TOUCHED, DESPITE ITS BOUND BEING EXCEEDED ON A DIFFERENT MASTER.** `kSyncResetDeltaBoundV = 9.90 V` versus a measured 9.999983 V on the spectral master. Widening it to 10.0 V would have deleted the two-sided derivation that makes it evidence — register item 2's forbidden move, performed for the most tempting possible reason. Escalated to plan 33-11 as a **scope** question instead: either re-derive the bound over a master-frequency axis, or scope it explicitly in its own banner to the grid that measured it.

7. **THE MECHANICAL PAIR VERIFICATION LIVES IN THE TOOL, NOT IN A SCRIPT.** The plan's Task 3 reads as a one-off checklist. Landing it in the renderer costs 53 lines and buys two things a script cannot: it re-runs on **every** invocation, and Phase 34 — which will change the table and the configuration pair — inherits it. It sets the exit status, so `make audition` goes red. And it was proved able to fail rather than assumed sensitive.

8. **THE PAIR IS VERIFIED AS ENCODED, NOT ONLY AS FLOATS.** 16-bit at a 10 V full scale resolves 305.2 µV, and **22 of 2003 per-reset corrections at two render points are smaller than that** — real in the buffer, absent from the file. "The legs differ" has to be a claim about the artefact the operator opens.

9. **SYNC-02 IS DECLINED — THE TWELFTH CONSECUTIVE DECLINE, AND THIS TIME FOR THE SIMPLEST REASON YET: THIS PLAN BUILDS THE APPARATUS, IT DOES NOT TAKE THE VERDICT.**

   SYNC-02 reads *"Sync reset uses sub-sample fractional placement plus a sync-BLEP (click-free), reusing the anti-aliasing machinery."* The mechanism has been complete since 33-06. What is missing is unchanged from 33-08's statement of it and is **owned by two later plans**:

   - **The residual-versus-intended-step separation** — the quantity SC-3 is actually about — is measured by **no instrument in this phase**, and both legs of every instrument contain the intended step. That is 33-08's register item 1, resolve-at **33-11**.
   - **The perceptual half** — "click-free", "buzzy not smeared" — now has an apparatus and **does not yet have a verdict**. Plan **33-12** owns it. Ticking SYNC-02 here on the strength of having built the rendering tool would be booking coverage on an instrument nobody has listened to, which is precisely register item 26's failure mode reproduced one level up.
   - **And this plan added a reason of its own:** the correction removes **eighteen microvolts of a ten-volt worst-case step** on the spectral master. That is measured, it is in the source, and it is not what "click-free" describes.

   **`.planning/REQUIREMENTS.md` was CHECKED against disk, not assumed, after this plan finished:** line 39 `- [x] **SYNC-01**` and line 134 `| SYNC-01 | Phase 33 | Complete |`; line 40 `- [ ] **SYNC-02**` and line 135 `| SYNC-02 | Phase 33 | Pending |`. **SYNC-02 remains `[ ]` / `Pending`. PITCH-04 was not touched — its re-tick is plan 33-11's.**

---

# Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] Two render-point rationales attributed plan 33-08's per-cell figures to cells on a different master**

- **Found during:** Task 2, on the first run of the finished renderer
- **Issue:** The table's `why` strings for the pulse cell and the square cell quoted 33-08's `9.793601 V` and `−0.246492 V` as properties of those render points, matched by five axes. **Both are wrong**, and one is wrong in *sign*: measured on the spectral master the same cells give `9.999983 V` and `+1.419190 V`. Left in, the operator-gate plan would have presented a number contradicted by the audio in front of the operator — and would have told them to listen for a region where the correction loses at a point where it clearly wins.
- **Fix:** The discriminating parameter was isolated by a controlled experiment (only `masterKm` changed, 93 → 32), the mechanism measured (`g ≡ 1.0` on 33-08's master versus `g ∈ [0.010752688, 1.0]` on the spectral one), and 33-08's figure reproduced **exactly** at `K_m = 32`. Both rationales now state which master their figures belong to; a 40-line two-master note was added to the banner; and **render point 07 was added** — the same cell on 33-08's master — so the losing region is genuinely auditionable. The table now spans two masters, which is what `masterKm` being a field was for.
- **Files modified:** `tools/render_sync_ab.cpp`
- **Verification:** The controlled-experiment table and the `g`-range measurement above; 20-of-140 versus 7-of-140 negative-margin scans on the two masters.
- **Committed in:** `d3fbf39`

**2. [Rule 2 — Missing correctness] The renderer reported peak level but not RMS, so its own level-match claim was unsupportable**

- **Found during:** Task 2, checking the plan's "peak levels are close" criterion
- **Issue:** Peak differs between legs by up to **1.075448 V** for a *correct* reason — the peak sits on a reset sample. With only peak reported, the plan's own level-match criterion reads as **failed** on a correctly level-matched pair, and there was no measurement in the tool that could show otherwise.
- **Fix:** Added `rmsV` per leg and a `LEVEL MATCH` line reporting both legs' RMS, the delta in volts and in dB. The struct comment states in terms why RMS and not peak is the metric, so a later editor does not "simplify" it back.
- **Files modified:** `tools/render_sync_ab.cpp`
- **Verification:** RMS matches within 0.1185 dB on all seven points, 0.0001 dB on the null-point control.
- **Committed in:** `d3fbf39`

**3. [Rule 2 — Missing correctness] The renderer's own honesty rested on inherited figures rather than on this render**

- **Found during:** Task 2, drafting the expected-results material
- **Issue:** The plan requires the audition not to oversell the correction, and requires at least one point where it measurably loses. With no metric of its own, the renderer could only *cite* 33-07 and 33-08 — and deviation 1 then showed those citations were not transferable to its own render points. The expected-results block would have been built entirely on numbers this apparatus could not reproduce.
- **Fix:** Added `worstResetStepV` per leg — plan 33-08's own metric, on reset samples identified from `tel.syncFired` (never inferred from the waveform, for 33-08's stated anti-circularity reason) — plus a `RESET STEP` line reporting both legs and the margin in 33-08's sign convention, labelled helps/does-not-help per point. Every per-point figure in the operator-gate block above now comes from **this** render.
- **Files modified:** `tools/render_sync_ab.cpp`
- **Verification:** Reproduces 33-08's `9.793601 V` and its negative-margin proportion exactly at `K_m = 32`.
- **Committed in:** `d3fbf39`

**4. [Rule 2 — Missing correctness] The float-domain difference count is not a claim about the file the operator opens**

- **Found during:** Task 3, verifying the written files independently
- **Issue:** An independent Python read of the WAVs found **1981** differing 16-bit samples at points 01 and 04 where the renderer reported **2003** differing floats. Cause: 22 per-reset corrections are smaller than one quantisation step (305.2 µV at this scale) and vanish on encoding. A "the legs are not bit-identical" check that passes in the float domain could in principle pass while the two **files** were identical.
- **Fix:** The writer now returns its PCM, and the self-check compares the pair **as encoded** as well as as floats, reports the sub-LSB count explicitly, and **fails** if the encoded files are identical while the float legs differ.
- **Files modified:** `tools/render_sync_ab.cpp`
- **Verification:** Renderer's encoded counts agree exactly with the independent Python read on all seven points.
- **Committed in:** `bc239e0`

### Reported, not fixed

**5. [Reported] `grep -c 'VcoCore '` outputs 2, not 1 — the twelfth instance in this project of a criterion's mechanism being wider than its own prose**

- **Found during:** Task 2, checking acceptance
- **Issue:** The criterion asks that this grep show *"a single construction site per render point"*. It outputs **2**. The second hit is a **string literal** inside the configuration table: `"A / SHIPPED (sync BLEP active - what forge::VcoCore does today)"`. A bare `grep -c` cannot distinguish a declaration from a string or a comment — the same failure class as Phase 30's LFO-filename zero-count, Phase 31's `std::pow`-under-`src` prohibition and 32-10's four reported criterion artifacts.
- **Fix:** **Reported rather than satisfied. Nothing was renamed and no string was reworded to make the number 1.** The property the criterion reaches for is given directly instead: exactly one `forge::VcoCore` object exists per render point, constructed at `tools/render_sync_ab.cpp:439`, and it is the only construction site in the file. `grep -c 'Mirror'` is **0** and `grep -c 'NaiveVcoCoreMirror'` is **0**, both unambiguous. Following 33-05's deviation 3 and 33-06's deviation 5, the numbers are given rather than the verdict.
- **Files modified:** none — a criterion-interpretation decision
- **Verification:** The construction-site table above.
- **Committed in:** n/a

**6. [Reported] `grep -c 'bandLimit'` outputs 1, and the hit is the sentence forbidding it**

- **Found during:** Task 2, same check
- **Issue:** The plan's design forbids `a bool bandLimit flag in the shipped core`. A negative grep for that name returns **1** — the banner sentence recording the prohibition. Same class as 30-05's banner, which had to document four forbidden C++ constructs while being grepped against their literal spellings.
- **Fix:** **Reported.** The prohibition is worth more in the source than a zero count is in a summary, and `src/dsp/VcoCore.hpp` is unmodified by this plan (it is absent from the whole-plan diff), which is the claim that actually matters.
- **Files modified:** none
- **Verification:** Whole-plan diff is three files, none of them under `src/`.
- **Committed in:** n/a

**7. [Reported] `kSyncResetDeltaBoundV = 9.90 V` is exceeded on the spectral master, and was deliberately left alone**

- **Found during:** Task 2, measuring render point 02
- **Issue:** Measured worst per-sample reset step on the spectral master: **9.999983 V**, above plan 33-08's pinned bound. 33-08's register item 2 records that the bound has only 0.206 V of total room and cannot be widened without becoming vacuous.
- **Fix:** **Reported and escalated, not absorbed.** No constant was touched, no `TEST_CASE` modified, and 33-08's two cases remain green — its grid does not contain this master, and this renderer reproduces its 9.793601 V exactly when pointed at `K_m = 32`, so the bound is correct about what it measured. Filed as deferred register item 1 below and as a STATE.md Blockers entry, resolve-at plan **33-11**.
- **Files modified:** none — the finding is recorded in the renderer's banner
- **Verification:** The two-master tables above.
- **Committed in:** `d3fbf39` (the banner note)

---

**Total deviations:** 4 auto-fixed (1 × Rule 1, 3 × Rule 2) + 3 reported
**Impact on plan:** The Rule 1 fix is the plan's substantive event — an inherited per-cell figure that was wrong in *sign* on the cell the plan told me to use, caught only because the renderer measured its own points instead of quoting a table. The three Rule 2 additions all exist because the plan's expected-results block had to be supportable **by this render**: without RMS its level-match criterion reads as failed on correct behaviour, without the reset-step metric its honesty is borrowed, and without the encoded comparison its non-identity check is about a buffer nobody hears. **The whole-plan diff is still exactly the three planned files.** No shipped source, no test file, and no constant belonging to another plan was modified.

---

# Known Stubs

**None.** Every symbol, field and table row this plan adds is consumed by the render loop, the output writer or the self-verification in the same commit or the next. Nothing is placeholder, nothing returns a hardcoded empty value, and the render points are populated with measured cells rather than sketches.

One thing is *absent by design*:

| Absence | Owner | Why it is not a stub here |
|---|---|---|
| The pair has been rendered and mechanically verified but **not listened to by a human** | **plan 33-12** | This plan's output is an apparatus plus the expected-results block. D-17 precedent 1 requires a **blocking `.continue-here.md` written before the UAT plan**, and the perceptual verdict is 33-12's to take. Taking it here would be the thing this phase's register exists to prevent. |

---

# Deferred Register Items

**1. NEW — THIS PHASE'S TWO INSTRUMENTS DO NOT SHARE A MASTER FREQUENCY, AND THEIR CELL LABELS DO NOT SAY SO.**
The spectral sub-grid (33-05 / 33-07) drives `K_m` coprime to 4096; plan 33-08's SC-3 grid drives a `1/128` dyadic increment. **MEASURED:** `g ∈ [0.010752688, 1.000000000]` over 93 wraps versus **`g ≡ 1.000000000` exactly on all 32 wraps** — 33-08's master **never wraps between two samples**, so the sub-sample fraction the seam exists to handle is unexercised there. That reverses the sign of the correction's benefit on the same cell (**+1.419190 V** versus **−0.427492 V**), and the negative-margin population is **7 of 140** on one master and **20 of 140** on the other. Neither instrument is wrong; a per-cell figure from either carries an unstated sixth axis.
**Proposed Resolve-at:** plan **33-11**, as a documentation-scope decision — each grid's banner should state its master's `g` behaviour, and any cross-instrument citation should name the master. It should **not** be resolved by re-parameterising either grid, which would move pinned columns in both.

**2. NEW — `kSyncResetDeltaBoundV = 9.90 V` IS A PROPERTY OF PLAN 33-08's MASTER AND IS EXCEEDED ON THE SPECTRAL ONE.**
Measured **9.999983 V** here (against 10.000000 V withheld) on the spectral master, while reproducing 33-08's **9.793601 V** exactly at `K_m = 32`. Mechanism: the deposit is proportional to `f²`, so a reset at `f → 0` receives essentially no correction and reproduces the full step; on a coprime master such a reset always occurs. **33-08's case is not red and its constant is untouched.** This sharpens its own register item 2 from a cross-*toolchain* risk into a demonstrated cross-*parameterisation* one — and it is the same anti-softening rule: **the bound must not be moved toward 10.0.**
**Proposed Resolve-at:** plan **33-11**, alongside register item 2 — either re-derive over a master-frequency axis or scope the constant explicitly in its banner.

**3. NEW — THE CORRECTION'S BENEFIT AT THE WORST-CASE RESET STEP IS ESSENTIALLY ZERO ON AN EQUIDISTRIBUTED MASTER.**
Eighteen microvolts of a ten-volt step. This is not a defect and not fixable within the past-edge placement: the deposit is `−f²·jump/2`, so `sup` over resets of `|step|` tends to the uncorrected value as the sampled `f` distribution fills the unit interval. It is a **property of any one-sided step correction**, and it bears directly on whether "click-free" can ever be discharged on a worst-case step bound.
**Proposed Resolve-at:** plan **33-11**, as input to the SYNC-02 disposition. It argues *for* 33-08's register item 1 — the residual-versus-intended separation — being the quantity that matters, and *against* any worst-case-step bound as evidence of click-freeness.

**4. NEW (minor) — 16-BIT AT THIS SCALE DOES NOT RESOLVE THE SMALLEST CORRECTIONS.**
305.2 µV per LSB; **22 of 2003** per-reset corrections at render points 01 and 04 fall below it. Irrelevant to an audition (≈ −84 dBFS) and now reported by the tool on every run. If a future phase needs the full float difference audible, the writer would move to 32-bit float — a ~10-line change, deliberately not made now because research assumption A4 records 16-bit PCM as the safest format across the operator's players.
**Proposed Resolve-at:** no owner needed; revisit only if an audition question requires it.

**5. CARRIED — EVERY VOLT IN THIS SUMMARY IS AN APPLE-CLANG FIGURE.**
Unchanged in kind from 33-01 through 33-09. The exposure's shape here is different and milder: this plan pins **no constant** and adds **no assertion**, so nothing it produces can red on another toolchain. What is toolchain-scoped is the *rendered audio* and every number in the expected-results block — which is exactly why D-15 keeps the output uncommitted and out of golden status. `make strict` passes locally at C++11 `-pedantic-errors`; T-33-08 is not discharged locally and the CI MinGW leg remains plan 33-11's.

**6. CARRIED — 33-08's items 1, 2 and 3, 33-07's 1, 3 and 5, 33-06's 1, 4 and 5, 33-05's 2/3/5, and 33-02/03/04's six, are unchanged by this plan.**
33-08's item 1 (the residual-versus-intended separation, still SYNC-02's whole gap and still unowned before 33-11) is the one this plan's apparatus is most often going to be *mistaken* for closing. It does not close it: both legs contain the intended step.

**7. CLOSED — PHASE 32's DEFERRED ITEM 26.**
*"The in-Rack audition asks whether an improvement is AUDIBLE but supplies no A/B reference, so that half is unanswerable by construction."* The apparatus exists, is verified, is reusable by table, and is generated on demand. **The item's remedy is delivered; the audition itself is plan 33-12's.** Note that 32-11 proposed `NaiveVcoCoreMirror` as the mechanism — it was **not** used, and did not need to be: the recording-only telemetry subtraction 33-06 landed gives both legs from one pass through the *real* core, which is strictly stronger than a bit-exact mirror because there is no second code path to keep in step. `grep -c 'Mirror' tools/render_sync_ab.cpp` is **0**.

---

# Issues Encountered

- **The plan told me which cells to use and the inherited numbers for those cells were wrong — one of them in sign.** Populating the table "from the sub-grid points plan 33-05 measured" is exactly right, and quoting 33-08's per-cell figures against those same five axes is exactly wrong, and nothing in either document says so because the axis that matters is not one of the five. The only reason it was caught is that the renderer measures its own points; had it merely cited the tables, the operator-gate block would have shipped a figure contradicted by the audio in front of the operator.
- **The most tempting single edit in this plan was to widen `kSyncResetDeltaBoundV` to 10.0 V.** It would have taken one character, made a real measurement stop being awkward, and destroyed the two-sided derivation that plan 33-08 spent a task building and register item 2 explicitly protects. Not taken; escalated instead.
- **`make -n audition` under a bogus `RACK_DIR` looked like sufficient evidence for the skip filter and was not.** It only became evidence once the *unfiltered* control was run. Half a control is a claim.
- **The peak-level criterion would have read as a failure on correct behaviour**, and the fix was a measurement (RMS) rather than a re-reading. This is the fourth plan in this phase to find that a criterion's stated mechanism is not the right instrument for its own prose.
- **`check_canary.sh [2b/5]` remains measurement-blind on this host** (33-03's finding). Irrelevant here — this plan adds no shipped code and touches no `src/` file — but `make guards` going green was again treated as evidence of nothing about behaviour.
- **Two untracked `.planning/research/.cache/*.json` files** pre-existed at session start, as they have since 33-02, and were left alone.

---

# Next Phase Readiness

**The apparatus exists, it is verified, and the expected-results block is written before anyone has listened.**

- **Plan 33-11** inherits **three new register items**, and items 1 and 2 are the ones that need decisions rather than notes: the two grids' unstated master axis, and whether `kSyncResetDeltaBoundV` is re-derived or explicitly scoped. Item 3 is input to the SYNC-02 disposition and argues that a worst-case-step bound cannot evidence click-freeness at all. Its CI MinGW leg is unaffected by this plan — no constant was pinned and no assertion added — but every number in this SUMMARY is Apple-clang-only.
- **Plan 33-12 owns the operator UAT and should read the banked expected-results block above verbatim rather than rewriting it.** All seven required bullets are present and every per-point figure comes from this render rather than from an inherited table. Three things it must carry through unchanged: **the difference is small** (eighteen microvolts of the worst-case step; +0.5827 dB grid-wide; under 1.25 dB on 33-05's own leg table); **points 04 and 07 are where the correction LOSES**, and the operator should be told so before listening; and **point 05 is a control where the two legs should be indistinguishable**. It must also write the blocking `.continue-here.md` **before** the UAT plan (D-17 precedent 1), and refuse the six named coverage items rather than booking them.
- **Phase 34 inherits the apparatus rather than the debt**, which was D-16's entire purpose. Its audition-gated DRIFT-03 value needs a different configuration pair and a different table; both are data, and `renderPoint()` does not change. The mechanical pair verification comes with it and runs on the new table for free. The one thing Phase 34 must check: the reconstruction shortcut is **specific to the past-edge placement**, and a drift A/B needs its own withholding mechanism rather than `tel.syncCorrection`.
- **The LFO guardrail is discharged for this plan by automated evidence only** — six goldens byte-identical (9 cases / 49,188), `check_frozen.sh` PASS, `src/AnalogLFO.cpp` absent from the whole-plan diff — and this plan touches no `src/` file at all.

**Concerns carried forward:**

- **SYNC-02's remaining gap is unchanged and this plan did not narrow it.** The residual-versus-intended-step separation is still unmeasured and still unowned before 33-11. **A favourable audition in 33-12 must not be booked as closing it.**
- **Two instruments in this phase have now been shown to disagree in sign on identically-named cells.** The disagreement is fully explained and neither is wrong, but every per-cell citation in this phase's documents should be read as carrying an unstated master.
- **The correction's benefit at the worst-case reset step is eighteen microvolts on an equidistributed master.** Measured, in the source, and the least flattering number this phase has produced.

---

# Self-Check: PASSED

Verified against disk and git rather than asserted:

- **Files exist:** `tools/render_sync_ab.cpp` (753 lines), `Makefile`, `tests/check_includes.sh`, `.planning/phases/33-hard-sync/33-10-SUMMARY.md` — all **FOUND**.
- **Commits exist:** `2aa2109`, `d3fbf39`, `bc239e0` — all **FOUND** in `git log`.
- **The target really exists and really runs:** `make audition` exits 0 and writes **14 files** into `build-test/audition/`, **7 of 7 `PAIR CHECK PASS`**, 0 clipped, 0 non-finite.
- **The allowlist really grew by one:** array range **378–388**, quote-line count **8 → 9**, and `make guards` was **green with the source file absent**.
- **The skip filter is really load-bearing:** `make -n audition RACK_DIR=/definitely/not/here` reaches the rule; the unfiltered `make -n strict` under the same path dies on the missing `plugin.mk`.
- **There is really only one core:** `forge::VcoCore core;` at `tools/render_sync_ab.cpp:439`; `grep -c 'Mirror'` = **0**; differing samples equal reset count on all 7 points.
- **The output is really valid audio:** header decoded field by field **and** independently confirmed by macOS `afinfo` (WAVE, 1 ch, 44100 Hz, Int16, 2.000000 s).
- **The self-check is really able to fail:** the `k 0.0f → 1.0f` probe reds all 7 points (21 `FAIL` lines) and the program **exits 1**. Restored and re-verified 7 of 7 PASS.
- **The render is really deterministic:** all 14 files byte-identical across two consecutive runs.
- **Nothing is really committed:** `git status --porcelain build-test/` **empty**; `git check-ignore -q build-test/audition` succeeds; `git diff --stat .gitignore` **empty**.
- **Nothing shipped moved:** whole-plan diff (`1994056..HEAD`) is **exactly** `Makefile`, `tests/check_includes.sh`, `tools/render_sync_ab.cpp` (784 insertions, 1 deletion); `src/AnalogLFO.cpp` **absent**; suite **unchanged** at 109 cases / 2,638,713 assertions / 0 failures; six LFO goldens **byte-identical**; `check_frozen.sh` PASS; `make strict` and `make guards` exit 0; **zero** compiler warnings.
- **`.planning/REQUIREMENTS.md` was CHECKED, not assumed:** SYNC-01 remains `[x]` / `Complete` (line 39 / 134); **SYNC-02 remains `[ ]` / `Pending`** (line 40 / 135). PITCH-04 untouched.
- **`.planning/STATE.md` was CHECKED for the known tooling bug and REPAIRED BY HAND:** `state.advance-plan` clobbered `stopped_at`, `last_activity_desc` and the body's `Last activity:` line with plan 33-09's text for the **fourth consecutive plan**. All three now read 33-10; `grep -c "Plan 33-09 complete."` returns **0**. Plan counter advanced correctly to **11 of 12**.

---
*Phase: 33-hard-sync*
*Completed: 2026-09-02*
