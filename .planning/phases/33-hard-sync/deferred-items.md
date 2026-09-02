# Phase 33 — Deferred Items

Three sections, in the format `.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/deferred-items.md`
established (numbered items, each with its evidence, its rejected alternatives, and an explicit
**Resolve at**).

- **Section zero** is this phase's **gate evidence**: the full local gate recorded as a
  **precondition**, and the CI toolchain gate observed by **hash equality**. It is first because
  the rest of this register is only worth reading once you know what the gate actually said.
- **Section one** is the numbered deferred register.
- **Section two** is this phase's **measured figures**, so a later phase can reach every number
  Phase 33 measured without opening a plan summary.

> **READ SECTION ZERO FIRST. THE PHASE GATE IS RED.** The full local gate is green — every
> component, with its numbers — and two of the three CI test legs are **not**. That is not an
> aside; it is the single most important thing this phase measured, and it is the reason no
> requirement was ticked by plan 33-11.

---

# SECTION ZERO — THE PHASE GATE, AS OBSERVED

## 0.1 The full local gate, recorded as a PRECONDITION and not as the gate

Every result below is recorded with its numbers rather than as a tick. **The word
"precondition" is load-bearing and this project has paid for it:** Phase 29 measured *this exact
combination* — `make test`, `make strict`, `make guards`, `check_canary.sh`, the frozen manifest
and the LFO goldens — returning **exit 0 on a deliberately broken commit that could not link**.
`make strict` is `-fsyntax-only` and never links, so no syntax-only gate on any platform can
catch a link-class defect. Only the real-link step catches it.

**And this phase has now added a second, sharper instance of the same lesson:** the table below
is entirely green and the CI test legs in §0.2 are red on two of three platforms. A green local
gate is a precondition. It is not evidence about what ships.

| Component | Command | Result — with numbers |
|---|---|---|
| Test suite | `make test` | **exit 0** — **109 test cases**, **2,638,713 assertions**, **0 failures**, 0 skipped |
| Strict C++11 | `make strict` | **exit 0** — `-std=c++11 -pedantic-errors -fsyntax-only -Wall -Wextra` over **4 translation units** (`AnalogLFO.cpp`, `AnalogVCO.cpp`, `plugin.cpp`, `vco_compile_canary.cpp`); prints `strict C++11 gate: PASS` |
| Guard suite | `make guards` | **exit 0** (all three scripts) |
| — frozen guard | `bash tests/check_frozen.sh` | **exit 0** — **15 pinned manifest entries** + **6 pinned golden fixtures** + the negative control |
| — include audit | `bash tests/check_includes.sh` | **exit 0** — 7 sections, `[7/7]` reports 3 wired + 1 documented `EXEMPT` (`check_docs.sh`) |
| — include audit, **no SDK** | `RACK_DIR=/definitely/not/here bash tests/check_includes.sh` | **exit 0** — the audit needs no compiler and no Rack SDK |
| — guard suite, **no SDK** | `RACK_DIR=/definitely/not/here make guards` | **exit 0** — `guards` is inside the Makefile's `plugin.mk` skip filter |
| — compile canary | `bash tests/check_canary.sh` | **exit 0** — **9 `VcoInputs` DSP fields reported runtime-live** through `step()` at `-O3`; probe symbol `T __ZN5forge21vcoCompileCanaryProbeEi` emitted |
| Frozen manifest, **byte comparison** | `cmp <base>:src/dsp/FROZEN.sha256 src/dsp/FROZEN.sha256` | **exit 0, NO OUTPUT — byte-identical.** Quoted verbatim below |
| Six shipped-LFO goldens | `./build-test/test -tc="*golden*"` | **9 matched cases**, **49,188 assertions**, **0 failures**; and all six `.f32` fixtures **byte-identical** to the pre-phase commit by `cmp` |
| Real plugin link | `make` (against `../Rack-SDK`) | **exit 0** — 4 objects compiled, `plugin.dylib` linked (169,328 bytes) |
| Compiler warnings | full build logs read **in full**, not with `tail` | **0** — `grep -cE 'warning:\|error:'` returns **0** over both the clean plugin build and the clean test build |

**The frozen-manifest comparison, quoted rather than described** — the Phase-30 rule is *assert
byte identity by reading BYTES, not by counting `git diff` markers*:

```
$ git show 9de82cf:src/dsp/FROZEN.sha256 > /tmp/frozen_base.sha256
$ cmp /tmp/frozen_base.sha256 src/dsp/FROZEN.sha256
$ echo $?
0
```

`cmp` printed **nothing** and exited **0**. That is the whole of the claim.

**The zero-warning gate was read rather than scanned.** Plan 33-09 recorded that a real
`-Wswitch` warning was scrolled past because the build output was read with `tail`. Both build
logs here were captured to a file and grepped in full; the counts above are over the whole log,
not over its last twenty lines.

### The frozen paths, over the WHOLE phase diff

Base commit **`9de82cf`** — the last commit before any Phase 33 source landed. (Its tree is
identical outside `.planning/` to `262e5c5`, the commit Phase 32's gate was observed on, so the
comparison below is also the comparison against the last CI-verified source tree.)

```
$ git diff --numstat 9de82cf..HEAD -- <all 15 FROZEN.sha256 paths> src/dsp/FROZEN.sha256
(no output)
```

**Zero changed lines on every frozen path, and on the manifest itself.**
`git diff --name-only 9de82cf..HEAD | grep -c 'src/AnalogLFO.cpp'` returns **0**.

The whole-phase non-`.planning` diff is **12 files, 9,411 insertions, 52 deletions**:

| File | Δ lines |
|---|---|
| `Makefile` | 57 |
| `res/AnalogVCO.svg` | 1 |
| `src/AnalogVCO.cpp` | 130 |
| `src/dsp/MorphBlep.hpp` | 341 |
| `src/dsp/VcoCore.hpp` | 601 |
| `src/vco_compile_canary.cpp` | 73 |
| `tests/check_includes.sh` | 24 |
| `tests/test_morph_blep.cpp` | 785 |
| `tests/test_vco_core.cpp` | 2,336 |
| `tests/test_vco_pitch.cpp` | 859 |
| `tests/test_vco_spectrum.cpp` | 3,503 |
| `tools/render_sync_ab.cpp` | 753 |

None of the four frozen shared headers, and not `src/AnalogLFO.cpp`.

---

## 0.2 The CI toolchain gate, observed BY HASH EQUALITY

### How the run was located, and why it matters

**Located by hash equality against the pushed commit. Never by recency, never by name.** Three
recorded failures behind that sentence: Phase 30's selector matched a **job** name against a
**run** name and returned nothing, so its own presence check failed regardless of CI health;
Phase 31 found the pre-phase commit's own green run sitting one line above the right one in
`gh run list`; and a job conclusion is never sufficient, because a step that fail-fasts upstream
reports **skipped**, which scans as not-red in a job summary.

| | Value |
|---|---|
| Pushed commit | **`da9e611cae0ad5851d031edf49a976d358471d70`** |
| Run's `headSha` | **`da9e611cae0ad5851d031edf49a976d358471d70`** |
| Equal? | **YES** — compared programmatically against every returned run before any conclusion was read |
| Run id | **33607312137** |
| Branch reached via | **`gsd/phase-33-ci`** |

**Why a branch and not the default branch.** The repository is **PUBLIC** and its default branch
is `main`. The standing operator decision of Phase 32 — public repo, `main` must not move — is
followed unchanged: `origin/main` is still at `80fb90a` and was not touched. The phase's commits
were pushed to `gsd/phase-33-ci`.

**Why `da9e611` is the right commit to gate.** It is the phase's last **source-bearing** commit.
Plans 33-11 and 33-12 modify only `.planning/`; `git diff da9e611..HEAD -- ':!.planning'` is
empty by construction. Gating `da9e611` is gating the whole of Phase 33's shipped and test source.

### The `toolchain-gate` job — GREEN, every step, none skipped

**The GATE is the reproduction step's OWN conclusion, not the job's.** Both are recorded; only
the second is the gate.

| Step | Name | Conclusion |
|---|---|---|
| 1 | Set up job | success |
| 2 | Run `actions/checkout@v4` | success |
| 3 | Fetch Rack SDKs (linux headers + windows link stub) | success |
| 4 | Strict C++11 pedantic gate (our code only) | success |
| **6** | **win-x64 leg reproduction (compile + full link vs libRack)** | **success ← THE GATE** |
| 5 | Install MinGW cross-compiler | success |
| 7 | VCO compile canary guard (D-07/D-08) | success |
| 8 | Frozen-header hash guard (D-05) | success |
| 9 | Include / dependency-direction audit (D-06) | success |
| 10 | LFO non-regression guard suite via make (P-5) | success |
| 20 | Post Run `actions/checkout@v4` | success |
| 21 | Complete job | success |

**Job conclusion: `success`. Every individual step conclusion: `success`. NONE skipped.**

**T-33-08 is discharged.** The MinGW cross-compile **and full link against `libRack`** succeeded
on the exact commit, printing `win-x64 link gate: PASS`. That is the class of defect that got
v2.0.0 rejected from the VCV Library, and `make strict` structurally cannot catch it.

### The `test` matrix — RED on two of three legs

| Job | Conclusion | Step 3 `make test (unix)` | Step 4 `make test (windows / direct g++)` |
|---|---|---|---|
| `test (macos-latest)` | **success** | **success** | skipped (correctly — `if: runner.os == 'Windows'`) |
| `test (ubuntu-latest)` | **FAILURE** | **FAILURE** | skipped (correctly) |
| `test (windows-latest)` | **FAILURE** | skipped (correctly) | **FAILURE** |

> **The two `skipped` step conclusions above are the CORRECT kind of skip and are named as such:**
> each is a platform `if:` that did not select, not a step that fail-fasted upstream. The
> distinction is exactly the one the observation protocol exists to make, so both are listed
> rather than filtered out.

| Leg | Cases | Assertions | Failures |
|---|---|---|---|
| macOS | **109** | **2,638,713** | **0** |
| Ubuntu | **106** | 2,614,129 | **1 case / 24 assertions** |
| Windows | **106** | 2,614,129 | **1 case / 24 assertions** |

**Both non-Apple legs fail identically**, which makes this a toolchain-class result
(GCC/libstdc++ and MinGW g++) rather than a platform accident.

### The three-platform delta, measured on BOTH sides, per leg

The "before" side is **measured on CI**, not predicted: run **30681442134** on
**`262e5c5`** — the last CI-observed commit, whose tree is identical to `9de82cf` outside
`.planning/`.

| Leg | Before (cases / assertions) | After (cases / assertions) | Δ cases | Δ assertions |
|---|---|---|---|---|
| macOS | 94 / 2,622,319 | **109 / 2,638,713** | **+15** | **+16,394** |
| Ubuntu | 91 / 2,597,737 | **106 / 2,614,129** | **+15** | **+16,392** |
| Windows | 91 / 2,597,737 | **106 / 2,614,129** | **+15** | **+16,392** |

**The case increment is +15, IDENTICALLY, on every leg.** That is what proves all fifteen new
Phase 33 cases ran on all three platforms — an absent platform guard in a new file is only an
argument; a per-leg before-and-after delta is evidence.

**The standing macOS-vs-others gap, and the 2 assertions that moved it:**

| | Before | After | Change |
|---|---|---|---|
| Case gap | **3** | **3** | **unchanged** — the three `#if defined(__APPLE__)` drift-ON bit-exact goldens in `tests/test_golden.cpp` (Phase-26 decision: `std::normal_distribution` is not portable across standard libraries) |
| Assertion gap | **24,582** | **24,584** | **+2 — the first time this gap has moved in this project** |

**The +2 is accounted for by naming the mechanism, not by expectation.** It is a direct
consequence of the classifier flip in §0.3: cell `ci = 21` is step-dominated on Apple clang and
plateau on GCC, and **two** `REQUIRE(slot >= 0)` sites sit inside loops that exclude plateau
cells at ratio ≥ 1.0 —

- `tests/test_vco_spectrum.cpp:5030`, inside `for (...) { if (grid[ci].ratio < 1.0) continue; if (!stepDom[ci]) continue; ... REQUIRE(slot >= 0); }`
- `tests/test_vco_spectrum.cpp:5264`, inside the instrument-valid form of the same loop

Cell 21 is `ratio == 1.00` and `tier == "gated"` (hence instrument-valid), so it enters **both**
loops on Apple clang and **neither** on GCC. **Two executions of `REQUIRE`, lost. 2 assertions,
exactly.** No other assertion site in the suite is guarded on `stepDom`.

---

## 0.3 STOP AND REPORT

**Both non-Apple legs fail the same single test case:**

```
tests/test_vco_spectrum.cpp:5522:
TEST CASE: vco spectrum: (SYNC-02 / D-11) the sync alias floor stays below its
           per-cell pinned threshold, and every pinned number reproduces
```

**That is the case carrying SYNC-02's spectral clause.** 24 failing assertions, in two
independent groups.

### Finding A — the inherited 1.0 dB reproduction bound does NOT hold on this signal class off Apple clang

**14 of the 24 failures**, all at `tests/test_vco_spectrum.cpp:5768`, `CHECK(delta <= bound)`
where `delta = |thisRunDb − cell.measuredDb|` and `bound = 1.0` for a step-dominated cell.

**This is exactly the escalation plan 33-07 wrote down in advance** (register item 1 below):
register item 8's 1.0 dB step-dominated reproduction bound was measured on Phase 32's
**free-running** cells and has never been measured on **hard-synced** ones. Plan 33-07 named
plan 33-11's CI leg as the first real measurement, and said in terms that a miss there is *"a
finding about the bound, escalated per the anti-softening rule, and not absorbed by widening the
column."*

| # | `ci` | rate | master edge | ratio | region | character | `measuredDb` (pinned) | `delta` (dB) |
|---|---|---|---|---|---|---|---|---|
| 1 | 63 | 44.1 k | hard-edge | 5.50 | triangle | 1.00 | 9.3091 | 1.28804 |
| 2 | 129 | 44.1 k | band-limited | 3.50 | pulse 5% | 1.00 | 3.3814 | 1.08691 |
| 3 | 133 | 44.1 k | band-limited | 5.50 | triangle | 1.00 | 3.0708 | **3.22702** |
| 4 | 139 | 44.1 k | band-limited | 5.50 | pulse 5% | 1.00 | 12.7872 | 1.48891 |
| 5 | 253 | 48 k | band-limited | 2.50 | triangle | 1.00 | −15.5315 | 2.44457 |
| 6 | 263 | 48 k | band-limited | 3.50 | triangle | 1.00 | −8.5238 | 2.57215 |
| 7 | 269 | 48 k | band-limited | 3.50 | pulse 5% | 1.00 | 4.3456 | 1.00873 |
| 8 | **273** | **48 k** | **band-limited** | **5.50** | **triangle** | **1.00** | **2.9139** | **3.79738 ← WORST** |
| 9 | 279 | 48 k | band-limited | 5.50 | pulse 5% | 1.00 | 12.6737 | 1.67792 |
| 10 | 343 | 96 k | hard-edge | 5.50 | triangle | 1.00 | 4.2204 | 1.47898 |
| 11 | 349 | 96 k | hard-edge | 5.50 | pulse 5% | 1.00 | 7.9403 | 1.00471 |
| 12 | 403 | 96 k | band-limited | 3.50 | triangle | 1.00 | −16.2546 | 1.12717 |
| 13 | 413 | 96 k | band-limited | 5.50 | triangle | 1.00 | −6.5042 | 1.48793 |
| 14 | 419 | 96 k | band-limited | 5.50 | pulse 5% | 1.00 | 4.9095 | 1.10557 |

**The pattern is clean and it is the finding, not the individual numbers.** All fourteen are at
**character 1.00**. All are at **ratio ≥ 2.50**. All are **triangle** or **pulse 5%**. Not one is
a saw, a square or a sine; not one is below ratio 2.50; not one is below full character.

**Worst divergence: 3.79738 dB**, against Phase 32's measured worst free-running divergence of
**3.02596 dB**. So the bound does not merely fail — it fails *further* than the free-running
population that produced it, which is the direct answer to 33-07's question and it is the
unfavourable one.

**Not absorbed.** No column was widened, no bound was moved, no cell was reclassified. The
anti-softening rule is written into `SYNC_PINS`' own banner and into 33-07's register item 1, and
this is the case it was written for.

### Finding B — the plateau/step classifier floor is a knife edge on one cell, and it flips off Apple clang

**The other 10 failures**, cascading from a single cell:

| Assertion | Apple clang | GCC / MinGW |
|---|---|---|
| `CHECK(nStepDominated == 402)` `:5593` | 402 | **401** |
| `CHECK(nPlateau == 18)` `:5594` | 18 | **19** |
| `CHECK(namesStepBound == (stepDom[ci] != 0))` `:5677` | true==true | **true==false** |
| `CHECK(namesPlateauBound == (stepDom[ci] == 0))` `:5678` | false==false | **false==true** |
| `CHECK((double)cell.thresholdDb == expected)` `:5690` | −31 == −31 | **−31 == −28** |
| `CHECK(nGatedStep == 192)` `:5719` | 192 | **191** |
| `CHECK(nGatedPlateau == 18)` `:5720` | 18 | **19** |
| `CHECK(probe2Step == 192)` `:5855` | 192 | **191** |
| `CHECK(probe2Plateau == 0)` `:5856` | 0 | **1** |
| `CHECK(probe5All == 210)` `:5857` | 210 | (reported) |

**One cell.** `ci = 21`: **44.1 kHz, hard-edge master, ratio 1.00, the sine centre, character
1.00**, tier `gated`.

**And the number that explains it is already written in this repository's own source.**
`tests/test_vco_spectrum.cpp:1674` carries the cell's provenance comment:

```
{ 44100.0, 0, 1.00, 0.00f, 1.00f,  -32.2934f, -31.0f, "gated", kProvSync441Step },
                                    // sine  step  fundDom +0.00  jump 0.0101  none -32.2916
```

**Its mean absolute `syncJump` is `0.0101`. The classifier floor is `0.0100`.** A margin of
**one ten-thousandth in pre-scale units — one percent of the floor.** On GCC it lands the other
side, the cell classifies as plateau, its bound becomes 4.0 dB instead of 1.0 dB, and its derived
threshold becomes `ceil(−32.2934 + 4.0) = −28` where the pinned column says `−31`.

**This is the one classifier in the whole phase that was NOT placed inside a measured empty gap,
and it is the one that broke.** The contrast is exact and was recorded before the fact:

| Classifier | Plan | Placement | Result on GCC |
|---|---|---|---|
| Anti-circularity gate, `max abs syncJump >= 0.75` | 33-08 | inside a **measured empty gap 0.639500 … 0.921976**, a **1.44×** window, **both edges asserted** | **held** |
| ρ lock window `[0.320, 1.310]` | 33-09 | **no gap** — protected by margin (0.18 / 0.31) plus two out-of-window controls; 33-09 filed this as weaker in its own register item 2 | **held** |
| Step-dominated jump floor, `mean abs syncJump >= 0.01` | 33-05, **inherited unchanged** by 33-07 | **no gap, and never measured for one.** Nearest cell **0.0101** | **BROKE** |

Plan 33-08's own decision 3 states the rule this violates in one line: *"Measure the distribution
BEFORE pinning a population count."* 33-08 rejected its physically-obvious threshold of 1.0
precisely because it had a boundary gap of `1.0e-06`; the floor here has a boundary gap of
`1.0e-04`, only a hundred times better, and was carried forward without the check.

> **Cell 21 is a ratio-1.00 hard-edge cell — 33-05's HAZARD THREE null point, where hard sync is
> a near-no-op.** Its mean jump is small *because the reset barely does anything there*. That is
> the physical reason it sits on the floor, and it is also the reason the flip changes nothing
> about the DSP.

### What is NOT red, stated as carefully as what is

**The alias-floor gate itself passed on all three legs.** `CHECK(runDb[ci] <= cell.thresholdDb)`
— the 210 gated and regression rows, the assertion that actually says *aliasing stays below its
threshold* — is **absent from the failure list on every leg**. Cell 21 even ran on GCC against
the **stricter** `−31` rather than its derived `−28`, and still passed.

**So the shipped DSP is not implicated by this red. The pinned bookkeeping is.** Both findings
are about numbers this repository recorded from Apple clang and then asserted as reproducible:
one a per-cell decibel column, one a population count. Neither is about `forge::VcoCore`'s
behaviour.

**Everything else crossed the toolchain intact**, and several of these were open questions
carried into this gate:

| Claim | Owner | Cross-toolchain result |
|---|---|---|
| `kSyncResetDeltaBoundV = 9.90 V` (0.106 V headroom, cannot be widened) | 33-08 | **HELD** on GCC and MinGW — its first cross-toolchain measurement |
| `kSyncAntiCircularityMarginV = 0.04 V` over 277 gated cells | 33-08 | **HELD** |
| The sync output tier `8.218569 V <= kHostileBoundV`, per rate | 33-08 | **HELD** |
| `kSyncLockToleranceCents = 0.10` and both ρ edges | 33-09 | **HELD** |
| PITCH-04's 783-cell third input class, exact float `==` | 33-09 | **HELD** |
| SYNC-01's detection, reset, hysteresis and structural ceiling | 33-04 | **HELD** |
| The snap-to-zero comparison (SYNC-02 / D-01) | 33-07 | **HELD** |
| `MorphBlep` hostile-parameter guards; the D-14 seam case | 33-01 / 33-06 | **HELD** |
| Strict C++11, MinGW compile **and full link**, canary, frozen, includes, guards | — | **ALL success** |

### The consequence, and it binds

**No requirement was ticked by plan 33-11**, and the roadmap's Phase 33 entry was **not** marked
complete. Plan 33-11's own acceptance criteria require exactly that on a red gate. The
requirement re-verification in §1 was still performed — it is what located the red — but every
status line on disk is unchanged.

<!-- gsd:write-continue -->
