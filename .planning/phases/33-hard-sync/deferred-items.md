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

---

# SECTION ONE — THE DEFERRED REGISTER

Every numbered item carries an explicit **Resolve at**. Items marked *restated unchanged* are
carried forward verbatim in substance and are **not** re-litigated here; saying so is the point,
because a register that silently re-argues an inherited item loses the record of when it was
decided.

---

## 1. NEW / ESCALATED — the 1.0 dB step-dominated reproduction bound does not hold on hard-synced cells off Apple clang

- **Opened by:** plan **33-07** as a prediction (its register item 1); **measured RED** by this gate.
- **Evidence:** CI run 33607312137 on `da9e611`. 14 of 420 cells miss `CHECK(delta <= bound)` at
  `tests/test_vco_spectrum.cpp:5768` on both GCC/Ubuntu and MinGW/Windows. Worst **3.79738 dB**
  (`ci = 273`: 48 kHz, band-limited, ratio 5.50, triangle, character 1.00). Full table in §0.3.
  **All fourteen are at character 1.00, at ratio ≥ 2.50, and are triangle or pulse-5% only.**
- **Why it matters:** Phase 32 measured this instrument toolchain-dependent by up to **3.02596 dB**
  on *free-running* cells and split the bound on a physical criterion (step-dominated 1.0 dB,
  plateau 4.0 dB). 33-07 inherited the step-dominated half onto hard-synced cells with only
  1.00245–1.99488 dB of headroom on 192 gated rows and wrote down that it had never been measured
  there. It now has been, and it fails **further** than the population it came from.
- **Rejected: widening the column.** Named as forbidden in `SYNC_PINS`' own banner, in 33-07's
  register item 1 and in the anti-softening rule. Widening 1.0 → 4.0 for the whole class would
  delete the step/plateau distinction Phase 32's operator explicitly chose over a global widen.
- **Rejected: reclassifying the fourteen cells as plateau.** They are step-dominated on a physical
  criterion; 32-10's anti-reclassification clause says in terms that a step-dominated cell firing
  is *"a finding about the criterion, not a cell to reclassify"*.
- **Not diagnosed here, and the shape of the diagnosis is named:** the fourteen share a signature
  (full character, high ratio, triangle/pulse) that points at the **arg-max instability of
  `aliasPeakDb` over 2043 bins** when the spectrum is near-flat — the same mechanism Phase 32
  identified — reached at full character where 32-05 measured the square's jump collapse. If that
  is right, the honest fix is a **third bound class**, measured, not a widened column.
- **Resolve at:** **Phase 36**, alongside register item 26 (the spectral column's portability),
  and it must be measured on all three toolchains rather than pinned from one. **A tag or a VCV
  Library resubmission must not be cut while this stands** — the standing rule already forbids
  cutting either on local evidence alone.

---

## 2. NEW — the step/plateau classifier floor is a knife edge, and it is the one classifier in this phase that was never placed inside a measured gap

- **Found during:** this gate, on the CI legs.
- **Evidence:** cell `ci = 21` (44.1 kHz, hard-edge, ratio 1.00, sine centre, character 1.00,
  tier `gated`) has mean absolute `syncJump` **0.0101** against the floor **0.0100** — recorded in
  this repository's own source at `tests/test_vco_spectrum.cpp:1674`. It classifies step-dominated
  on Apple clang and **plateau** on GCC/MinGW, taking `nStepDominated` 402 → 401, `nPlateau`
  18 → 19, the derived threshold −31 → −28, `nGatedStep` 192 → 191, `probe2Step` 192 → 191 and
  `probe2Plateau` 0 → 1. Ten assertions, one cause.
- **Why it matters more than the ten assertions:** plan 33-08 established the rule this violates
  — *"measure the distribution BEFORE pinning a population count"* — and rejected its own
  physically-obvious threshold of 1.0 for having a boundary gap of `1.0e-06`. The floor here has a
  boundary gap of `1.0e-04`. 33-08's own gate sat in a **1.44× measured empty gap** and held on
  GCC; 33-09's ρ window had no gap, was flagged as weaker in its own register, and held anyway;
  this one had no gap, was **inherited from 33-05 unchanged and never checked for one**, and broke.
- **Rejected: moving the floor to 0.005 or 0.02.** Both would make the assertion green and neither
  is a measurement. The floor's value is not the problem; the absence of a measured gap around it
  is.
- **The right fix, named:** measure the full distribution of mean `|syncJump|` over all 420 cells,
  locate an empty gap, place the floor inside it and **assert both edges**, exactly as
  `tests/test_vco_core.cpp` does for the 0.75 anti-circularity classifier. If no gap exists on this
  grid — and at the ratio-1.00 null point there may genuinely not be one — then the population
  counts `402 / 18` must stop being asserted as equalities, which is the same call 33-08 made about
  its 56 negative margins.
- **Resolve at:** **Phase 36**, together with item 1 — the two failures are in the same case and a
  single re-measurement pass addresses both.

---

## 3. OPEN, STILL NO OWNER — SYNC-02's whole remaining gap: the RESIDUAL discontinuity, separated from the INTENDED reset step

- **Opened by:** plan **33-08** (its register item 1), the first precise statement in the phase.
  Restated by 33-09 and 33-10. **Its Resolve-at was this plan, and this plan does not close it.**
- **Evidence that nothing measures it:** the spectral gate is structurally blind to a
  single-sample full-amplitude spike (**measured 0.0 dB**, register item 11). 33-08's time-domain
  gate sees the *step*, but a legitimate hard-sync reset genuinely steps the output by nearly its
  full range in one sample (**measured 9.793601 V corrected, 10.000000 V withheld**), so seeing
  the step is not seeing the artefact — **both of its legs contain the intended step**, so their
  difference measures the correction's *size*, not the residual's. 33-10's renderer inherits the
  same property and names it as coverage it cannot supply.
- **33-10 added an argument that this is the quantity that matters:** the deposit is `−f²·jump/2`,
  so `sup` over resets of `|step|` tends to the *uncorrected* value as the sampled `f` distribution
  fills the unit interval. **Measured: eighteen microvolts of a ten-volt step** on an
  equidistributed master. A worst-case-step bound therefore cannot evidence click-freeness even in
  principle.
- **Disposition taken at this gate: SYNC-02 is NOT ticked, for the thirteenth time.** Three
  independent reasons now, and each is sufficient on its own: (a) the case carrying its spectral
  clause is red on two toolchains; (b) this quantity is unmeasured; (c) the perceptual verdict has
  not been taken.
- **Rejected: discharging SYNC-02 on "the mechanism is complete plus a favourable audition."**
  That was one of the two routes 33-08 offered. It is refused here because a favourable audition on
  seven render points is a verdict on seven render points, 33-10 said so in writing, and booking it
  as closing an unmeasured quantity is register item 26's failure mode reproduced one level up.
- **Resolve at:** **an explicit operator decision at plan 33-12**, presented as a *carry* rather
  than a *closure*: either SYNC-02 is carried to v2.1 with the mechanism shipped and the
  requirement's "click-free" wording acknowledged as unevidenced, or the requirement's wording is
  changed to what this phase can actually measure. **It must NOT be absorbed by widening 33-08's
  bound.**

---

## 4. OPEN — this phase's two instruments do not share a master frequency, and their cell labels do not say so

- **Opened by:** plan **33-10**, by controlled experiment; its register item 1. Resolve-at was
  this plan.
- **Evidence:** over 4096 samples, on the master wrap fraction `g` — the spectral sub-grid
  (33-05 / 33-07) uses `K_m` coprime to 4096 and gives `g ∈ [0.010752688, 1.000000000]` over 93
  wraps; plan 33-08's SC-3 grid uses a `1/128` dyadic increment and gives **`g ≡ 1.000000000`
  exactly on all 32 wraps**. 33-08's master **never wraps between two samples**, so the sub-sample
  fraction the whole seam exists to handle is **unexercised on that grid**. The same five-axis cell
  measures **+1.419190 V** on one master and **−0.427492 V** on the other; the negative-margin
  population is **7 of 140** versus **20 of 140**.
- **Disposition taken at this gate: this is a DOCUMENTATION-SCOPE item and it stays open.** No
  banner was edited here, because this plan's file scope is `.planning/` only — the edits belong in
  `tests/test_vco_core.cpp` and `tests/test_vco_spectrum.cpp`.
- **Rejected: re-parameterising either grid.** It would move pinned columns in both, and both
  instruments are correct about their own grid.
- **The binding consequence, recorded so it survives this register:** **the phase's most-cited
  uncomfortable number — "the correction is worse on 56 of 420 cells" — was measured on the grid
  where the sub-sample feature does not apply.** That does not make it wrong; it makes it a
  statement about hard sync on a sample-aligned master. Any later citation of a per-cell figure
  from either instrument **must name its master**.
- **Resolve at:** **Phase 36** — each grid's banner states its master's `g` behaviour, and the
  five-axis cell label gains the master as a sixth axis.

---

## 5. PARTLY DISCHARGED — `kSyncResetDeltaBoundV = 9.90 V` held cross-toolchain, and remains a property of one master

- **Opened by:** plan **33-08** (item 2: only 0.206 V of total room, cannot be widened) and
  sharpened by plan **33-10** (item 2: exceeded at 9.999983 V on the spectral master).
- **DISCHARGED HALF, measured at this gate:** the two SC-3 cases in `tests/test_vco_core.cpp` are
  **green on Ubuntu and Windows**. The bound's **first cross-toolchain measurement passed.** So did
  `kSyncAntiCircularityMarginV = 0.04 V` over 277 cells, the three mutation-probe populations
  (69 / 0 / 277), and the re-derived sync output tier at 8.218569 V. That was the open question
  33-08 handed forward and the answer is favourable.
- **OPEN HALF, unchanged:** the bound is still a property of 33-08's `1/128` master. 33-10 measured
  **9.999983 V** on the spectral master — above it — while reproducing 33-08's **9.793601 V**
  exactly at `K_m = 32`. 33-08's case is not red and its constant was not touched.
- **Rejected, twice, for the most tempting possible reason:** moving the constant toward 10.0 V.
  One character; it would have made two awkward measurements go away and would have deleted the
  two-sided derivation that makes the bound evidence at all.
- **Resolve at:** **Phase 36**, with item 4 — either re-derive over a master-frequency axis or
  scope the constant explicitly in its own banner to the grid that measured it. **Scoping is the
  cheaper and more honest option** and is the recommendation.

---

## 6. OPEN, NO CODE CHANGE — the correction's benefit at the worst-case reset step is essentially zero on an equidistributed master

- **Opened by:** plan **33-10**, its register item 3. Restated unchanged in substance.
- **Evidence:** **eighteen microvolts of a ten-volt step** (9.999983 V shipped against 10.000000 V
  withheld) on the spectral master.
- **Why it is not a defect:** the deposit is `−f²·jump/2`, so a reset detected at `f → 0` receives
  essentially no correction and reproduces the full step. On a coprime master `f` is equidistributed
  and such a reset always occurs. **This is a property of any one-sided step correction**, not of
  this implementation.
- **Resolve at:** no code change in v2.0. It is **input to item 3's disposition** and it argues
  against any worst-case-step bound ever being evidence of click-freeness. Re-open only if v2.1
  oversampling changes the reachable `f` distribution.

---

## 7. CARRIED, ASSERTED IN TWO INSTRUMENTS — the landed leg is measurably WORSE than no correction at high master/slave ratios

- **Opened by:** plan **33-06** (item 3), asserted permanently by **33-07** (item 2), reproduced in
  the time domain by **33-08** (item 3). **Restated unchanged.**
- **Evidence, three instruments agreeing on region and sign:** 33-06 measured 0.15–1.09 dB worse at
  ratio 5.5; 33-07 measured a mean **−1.0281 dB** over 60 cells with 47 of 60 worse and a worst
  single cell of 7.0218 dB, and asserted the sign permanently; 33-08 measured **56 of 420** cells
  with a negative time-domain margin, worst **−0.246492 V at ratio 5.50** — subject to item 4's
  master caveat.
- **Rejected:** a ratio-conditional correction. Not available to a core that cannot know the
  master's frequency, which is D-09's boundary.
- **Resolve at:** unchanged — **no code change in v2.0**. Re-open only if v2.1 gives the core the
  master's rate.

---

## 8. CARRIED WITH A MEASUREMENT — D-07's residual carried-forward phantom in `MorphBlep`'s accumulator

- **Opened by:** plan **33-02** (its item 1) as arithmetic; **given a number** by plan **33-05**
  (its item 4).
- **The mechanism:** at sample *n−1* the site loop fires every site within one increment ahead and
  deposits **both** halves; sites the reset then jumps over are never traversed, so both halves are
  phantom and the second is still in the carried accumulator when sample *n* drains it.
- **The window width:** `(1 − f) · dt`. At C7 and 44.1 kHz the increment is ≈ 0.047 of a cycle, so
  a mid-crossing reset leaves a window ≈ **0.024** wide — on the order of one phantom site every
  few sync events at that note.
- **The measured diagnostic figure (33-05, over 30,940 reset samples):** **mean 0.0569, MAXIMUM
  0.9624.** The maximum is very nearly full-scale and is **much larger than the header's arithmetic
  estimate reads**, which is why the register now carries the measurement instead of the paragraph.
- **Why per-site cancellation is impossible without restructuring:** `MorphBlep`'s accumulator is a
  **scalar sum** (`src/dsp/MorphBlep.hpp:233`). There is no per-site ledger to subtract from, so
  cancelling one site's phantom half requires the header to grow per-site accounting.
- **Accepted disposition, unchanged: accept and document.** Two reasons, both recorded in 33-02:
  the effect is partly self-cancelling because the pre-reset jump term is taken *past* the
  jumped-over site, and the restructure is out of proportion to a header this phase is otherwise
  only hardening.
- **Resolve at:** the first phase that restructures `MorphBlep` for per-site accounting, or
  **v2.1's oversampling work** — whichever comes first. Not before.

---

## 9. OPEN — the full contract audit of `src/dsp/MorphBlep.hpp`'s advertised claims

- **Carried from:** this phase's own rejected option (D-04's), recorded in `33-CONTEXT.md`.
- **Where it stands:** the header advertises caller-independence **in capitals** and now *enforces*
  it for **four** parameters — `dt` (Phase 32), and `morph`, `character` and `jump` (plan 33-01,
  the operator-scheduled prerequisite that closed 32-REVIEW's CR-01 and CR-02). **Whether every
  remaining claim in the banner is enforced is unaudited.**
- **Why it did not happen here:** Phase 33 added the second call site and paid for exactly the
  guards that site made live. A full banner-versus-behaviour audit is a different piece of work and
  would have grown a phase already carrying its own risk concentrator.
- **Resolve at:** **any phase that adds a THIRD `MorphBlep` call site.** That is the event that
  makes the unaudited claims live, exactly as the second call site made CR-01 live.

---

## 10. RESOLVED FOR THE INSTANCE, OPEN AS POLICY — what happens when a measurement declines to decide

- **Opened by:** plan **33-05** (its item 1), when the D-06 three-condition rule **REFUSED**: all
  three conditions failed and no winner was declared, while plan 33-06 was already scheduled to
  *"implement the decision 33-05 records."*
- **The instance is CLOSED, by an OPERATOR DECISION of 2026-08-30** — the first of the three routes
  33-05 proposed. The operator was shown the refusal, its figures, the four-leg deficit table and
  the Apple-clang caveat, and chose to land the **past-edge** leg and to add the named
  `addPastStep` entry point on legibility grounds. Both shipped headers label the choice
  **EVIDENCE-BASED, NOT RULE-SANCTIONED**. 33-05's recommendation paragraph was preserved, not
  promoted.
- **The evidence the decision rests on, recorded so it is reachable:** condition 1's second clause
  — `pastEdge`'s worst single-cell deficit is **0.8553 dB**, the **only** candidate inside register
  item 8's 1.0 dB reproduction bound; `none` 3.93 dB; `detect` **eliminated** at 0 wins of 54 and a
  worst deficit of 5.05 dB, worse than doing nothing; `flatHalf` **eliminated on variance** at
  10.4567 dB.
- **What is STILL OPEN is the general process question, and it is filed as such rather than treated
  as answered:** *who decides when a measurement declines to?* The operator decision answered **this
  instance**. It did not establish a policy. What the phase does now have is a **precedent**:
  escalate with the figures, decide explicitly, and label the result in the source as
  evidence-based rather than rule-sanctioned.
- **Resolve at:** **the next phase that writes a multi-condition decision rule** — such a rule must
  state its refusal behaviour before it is run, not after. Phase 34's audition-gated DRIFT-03 value
  is the next candidate.

---

## 11. CARRIED — the spectral instrument is blind on hard-edge masters and on 210 of 420 cells

- **Opened by:** plan **33-05** (item 2); **built into the tier partition** by plan 33-07 rather
  than only recorded.
- **Evidence:** 210 of 420 cells fail the fundamental-dominance check and are `diagnostic` **by
  decision**, never CHECKed. Separately, every candidate leg measures within about a decibel of
  every other on hard-edge masters at any ratio. And register item 11's own root cause: a
  single-sample full-amplitude spike measures **0.0 dB** spectrally.
- **How to read it:** a hard-edge row is *"no information"*, never *"no difference"*.
- **Resolve at:** no action. This is a permanent property of the instrument and is now asserted as
  the second half of the snap case. Recorded so a later reader does not mistake a diagnostic row
  for a passing one.

---

## 12. CLOSED ON EVIDENCE — Phase 31 deferred item 11, PITCH-04's third input class

- **Opened by:** Phase 31, which ticked PITCH-04 on two of the three input classes its text names
  and declined to close the third by structural argument. Pointed at Phase 33; plan **33-09**
  landed the evidence and referred the closure here (its item 6).
- **The argument Phase 31 refused to make is now a measurement:** over **783 cells** (29 pitch/FM
  rows × 9 sync shapes × 3 rates, 4000 steps each, **3,132,000 core steps**),
  `forge::VcoCore::Telemetry::freqHz` under every hostile sync shape is **EXACTLY EQUAL by float
  `==`** — not by tolerance — to the same core's value with the jack unpatched.
- **Observed with the detector FIRING, asserted in both directions per cell:** six sync rows must
  fire and three provably cannot. Measured per-row firing counts over 87 cells each — unpatched
  0/0, musical 110 Hz **5–10**, musical 55 Hz **3–5**, master far above the rate **84–167**, idling
  0/0, held NaN 0/0, exactly on the threshold **62/62**, NaN glitch **5–10**, low-then-steady 1/1.
  **Every must-fire row fires on every one of its cells; every cannot-fire row fires on none.**
- **Proved able to fail:** unpatching the grid reds the firing assertion **522** times; a targeted
  probe reds all **27** of the second subcase's; a one-line `if (in.syncConnected) freq *= 1.0001f;`
  in the shipped header reds the item-11 equality **384** times.
- **Cross-toolchain:** the case is **green on all three CI legs**, so this is not an Apple-clang
  result.
- **Decision taken here: item 11 is CLOSED, and PITCH-04's checkbox is NOT edited.** It has been
  `[x]` since Phase 31 and stays `[x]`; the condition for re-confirmation is met on the evidence,
  and no status line is touched because nothing is ticked on a red gate. **Item 11's own file under
  `.planning/phases/31-pitch-tuning-exponential-fm/` was deliberately left unedited** — a
  cross-phase document rewrite is not this plan's mandate, and the closure is recorded here and in
  `REQUIREMENTS.md`'s dated footer instead.
- **Resolve at:** CLOSED.

---

## 13. OPEN — `tests/check_canary.sh [2b/5]` is measurement-blind on this host, and the `[2c/5]` witness section was proposed to THIS plan

- **Opened by:** plan **33-02** (item 2), root-caused by plan **33-03** (items 1 and 2), whose
  proposed Resolve-at was plan 33-11 — **this plan, which did not implement it.**
- **Evidence of the blindness:** the section reports *"all 9 `VcoInputs` DSP field(s) stay
  runtime-live"* — it reported **9** again on this gate — while a direct sensitivity probe
  replacing `in.morph` with the literal `0.5f` **still emits** `kCanaryOdr_morph` at `-O3`.
- **The root cause, known and more useful than the workaround:** `forge::VcoCore::step` is never
  inlined into the canary probe at stock `-O3` on Apple clang, so no field constant propagates into
  the seam and the perturbed table survives whatever the canary feeds.
- **A second, structural gap:** the section's field derivation is `$1 == "float" && $3 == "="`, so
  `fmConnected` and `syncConnected` are **permanently invisible** to it — and both are **outer
  gates on whole code blocks**, the highest-leverage fields in the POD to constant-fold.
- **Why this plan did not implement `[2c/5]`:** plan 33-11's file scope is `.planning/` only;
  `tests/check_canary.sh` is outside it. **The mechanism is already proven** by 33-03's witness
  experiment (pre-task commit ABSENT, landed commit PRESENT, literal-false ABSENT) and is roughly
  twenty lines of the shape `[2b/5]` already uses, plus one awk clause to enumerate `bool` members.
- **What this gate CAN report instead, and it is the stronger evidence:** the canary guard step ran
  **on the GCC/Ubuntu runner** in run 33607312137 and its own step conclusion is **success**. That
  is where the teeth are, and it was observed rather than assumed.
- **Resolve at:** **Phase 36**, as the `[2c/5]` witness section — a scratch-header witness whose
  reference must survive, plus `bool` enumeration. Until then, **`make guards` going green remains
  evidence of nothing about behaviour**, and every plan in this phase said so.

---

## 14. OPEN — a mutation probe's BUILD must be verified before its result is read

- **Opened by:** plan **33-04** (item 2), which nearly believed a green from a stale binary.
- **Evidence:** the first `prevSyncVolts` shared-static probe deleted the member; two test-file
  references stopped compiling; `make test >/dev/null 2>&1` swallowed the error; `./build-test/test`
  ran the **stale binary** and reported 33 of 33 assertions passing. For about a minute that read
  as *"the probe does not discriminate."*
- **Why it matters:** this is the Phase-29 local-gate-exits-0-on-a-commit-that-cannot-link failure,
  in miniature, **inside the very technique this repository uses to validate its own guards** —
  and this phase ran more than twenty mutation probes.
- **The convention this gate writes down:** *a probe's build must be verified before its result is
  read.* Cheap — drop the `>/dev/null 2>&1` on the mutant build, or assert a non-empty rebuild.
- **Resolve at:** **every subsequent plan that runs a mutation probe**, as a standing convention
  rather than a code change. No file enforces it; that is a known weakness of this entry and is
  stated rather than hidden.

---

## 15. OPEN, NO CODE CHANGE — the sub-sample fraction guard's two halves are redundant against a not-a-number

- **Opened by:** plan **33-04** (item 1). **Restated unchanged.**
- **Evidence:** `!(f >= 0.f)` and `!(f < 1.f)` are each individually true for a NaN, so either
  alone catches it. Their non-redundant duties are the finite out-of-range directions, and only one
  is reachable: `f >= 1` is reachable and asserted; **`f < 0` is unreachable at a firing sample**,
  because the trigger only fires from `LOW` with `now >= 1.0`, so `prev < 1.0 <= now` by
  construction.
- **Why it is filed rather than acted on:** the correct action is almost certainly *none* — one
  comparison on sync samples only, the file's standing idiom, and a future master-conditioning
  stage could make `f < 0` reachable. **What must not happen** is a later reader aiming a mutation
  probe at the lower half, seeing green, and concluding the case is passing.
- **Resolve at:** no code change. **Re-check the reachability argument in any phase that adds a
  second caller of the sync block or conditions `syncVolts` before it reaches the core.**

---

## 16. OPEN, NO CODE CHANGE — the late-fire rate is the binding error term, and the BLAMP escalation is specifically NOT indicated

- **Opened by:** plan **33-05** (item 3). **Restated unchanged.**
- **Evidence:** **1,820 of 30,940 resets (5.88 %)** land on a sample the master did not wrap on,
  **all** on band-limited masters. The oracle leg — the master's *true* wrap fraction — measures
  **0.45–0.71 dB WORSE** than the detector's own, because a perfect fraction is sized for an edge
  the reset did not correspond to. **The fraction that matters is the one consistent with the
  reset.**
- **The consequence, and it redirects future work:** any effort aimed at improving sub-sample
  accuracy should target the **detection threshold** under a band-limited master, **not** the
  interpolation. The polyBLAMP escalation (item 23) is specifically **not** indicated by this
  measurement.
- **Resolve at:** no code change in v2.0. Re-open only if a later phase conditions `syncVolts`
  before it reaches the core.

---

## 17. OPEN — invariant 10's output-envelope claim is scoped to one shape point, and this phase already exceeds it elsewhere

- **Opened by:** plan **33-09** (item 1). **Restated unchanged.**
- **Evidence:** invariant 10 asserts `|out| <= kPitchLooseBoundV = 6.0 V` and measures
  **5.000000 V**, but only at `morph = 0.50, character = 1.00`. Plan 33-08 measured the sync class
  reaching **8.218569 V** at other shape centres.
- **They do not conflict** and must not be read as a phase-wide envelope claim in either direction.
- **The trap, named:** a later plan that sweeps morph or character in invariant 10's grid will turn
  it **red on correct behaviour**. The right response is to **re-derive the tier for the widened
  population**, as 33-08 did — **never** to widen `kPitchLooseBoundV`, which four other pitch
  scenarios depend on.
- **Resolve at:** whichever plan next widens invariant 10's shape coverage. **Phase 34** is the
  likely one, since CHAR-01 puts CHARACTER on a CV input.

---

## 18. OPEN — the ρ lock window is the one classifier in this phase pinned WITHOUT a measured gap that survived

- **Opened by:** plan **33-09** (item 2). **Restated unchanged, and now with a cross-toolchain
  result.**
- **Evidence:** ρ = 0.315 does not lock and ρ = 0.320 does, at a sweep resolution of 0.005. There
  is **no gap**. The claim is protected by **margin** (0.18 and 0.31, both asserted) plus two
  out-of-window controls.
- **New at this gate: it HELD on GCC and MinGW.** That is a favourable result and it does not make
  the item safe — item 2 above is the same weakness in a different classifier, and it broke.
- **The trap, unchanged:** if a shape change moves the lower edge above 0.32, the ρ = 0.50 cells
  fail — and that is **a finding about the waveform, not a reason to move the window**.
- **Resolve at:** **Phase 34**, which changes the waveform by putting CHARACTER and DRIFT on the
  audio path.

---

## 19. OPEN — `kSyncLockToleranceCents`' cushion is one-sided and cannot go red the way a two-sided bound can

- **Opened by:** plan **33-09** (item 3). **Restated unchanged.**
- **Evidence:** the tolerance's lower constraint is a measurement (0.020443 cents) and its upper
  one is a **requirement figure** (1.0 cent), so the interval is two-sided — but the upper
  constraint is **not something another toolchain could falsify**, unlike 33-08's
  `kSyncResetDeltaBoundV`. Cushion factor **4.89×**. Its real protection is the **4,980×**
  separation from the un-synced alternative, asserted per cell.
- **New at this gate:** green on GCC and MinGW.
- **Resolve at:** no code change. Recorded so a later reader does not treat its green as the same
  kind of evidence as a two-sided bound's.

---

## 20. OPEN — three of this phase's own documents carry superseded figures

- **Opened by:** plan **33-05** (item 5) and plan **33-07** (item 3), both with a Resolve-at of
  *"plan 33-11, when it reconciles the phase's documents."* **This plan reconciles them HERE, in
  this register, and does not rewrite the source documents** — rewriting a research or validation
  document after the fact destroys the record of what was believed when the work was planned.
- **The three discrepancies, stated so a reader is not misled:**
  1. **`33-RESEARCH.md`'s grid recommendation and `33-VALIDATION.md`'s Threshold Policy both
     contain the FALSIFIED ratio set.** Three of the six ratios they recommend are **integer**
     master/slave ratios, which 33-05's HAZARD THREE measured to be **null points** where hard sync
     is a near-no-op and the reported figure goes *positive*. The corrected evidence lives in
     `tests/test_vco_spectrum.cpp`'s `SYNC_RATIOS` banner: the shipped ratio axis is non-integer.
  2. **33-05's SUMMARY records the plateau split as `378 / 42`; 33-07's landed criterion gives
     `402 / 18`.** Both are correct about their own criterion: 33-05 used clause (i) alone (mean
     `|syncJump| >= 0.01`), 33-07 added clause (ii) (the **slave's** own discontinuity by Phase 32's
     measured shape partition), which moves the 24 ratio-1.0 saw, pulse and hard-square cells into
     step-dominated. **Neither document is wrong; a reader comparing them without the criterion in
     front of them will think one is.**
  3. **`33-RESEARCH.md` Pitfall 7** (a band-limited master can push the fraction out of `[0,1]`)
     was **falsified by measurement** in plan 33-04: the trigger only fires when `now >= 1.0` and
     only from `LOW`, so `prev < 1.0 <= now` by construction. And **Pitfall 4**'s warning that the
     anti-circularity margin *"can be as small as 0.003 V"* understated the problem — 33-08
     measured the grid-wide minimum **negative**, at −0.246492 V.
- **Resolve at:** CLOSED as a reconciliation. The corrections live in the source banners, which are
  the copies that cannot rot, and are now reachable from one place.

---

## 21. DEFERRED BY DECISION — the cosine-crossfade third audition leg

- **Carried from:** this phase's own rejected option (D-14's), recorded in `33-CONTEXT.md`.
  **Restated unchanged.**
- **What it would buy:** it would make *"buzzy, not smeared"* an evidenced **three-way**
  comparison, by rendering the design that `research/PITFALLS.md:114-131` and SYNC-02 both
  **forbid** — the LFO's 3 ms cosine crossfade — as a third leg beside `leg-A-shipped` and
  `leg-B-withheld`.
- **Rejected here:** implementing the wrong design on purpose, inside the phase that establishes
  the right one, is not a good use of a phase whose central question was already a risk
  concentrator.
- **Now near-free:** plan 33-10's `CORE_PAIR[]` is a table and `renderPoint()` does not change, so
  a third leg is a table row plus a withholding mechanism.
- **Resolve at:** **the first UAT where the operator's verdict on smear is EQUIVOCAL.** It is the
  cheap escalation and it should be reached for before any DSP change.

---

## 22. DEFERRED — apply the A/B audition renderer to Phase 32's morph pair

- **Carried from:** `33-CONTEXT.md`; it closes Phase 32's deferred item **26**'s *original* debt —
  the unevidenced audible-improvement half of the 32-11 audition, where the operator was asked to
  compare against a memory.
- **Now near-free:** D-16's renderer is parameterised by a render-point table and a configuration
  pair. A morph A/B needs a different table and a different withholding mechanism; `renderPoint()`
  is unchanged. **The one thing it must not inherit:** `tel.syncCorrection` is specific to the
  past-edge sync placement and is not a general withholding lever.
- **Resolve at:** **any later phase, or Phase 36 alongside the goldens.** Phase 34 is the natural
  host, since its DRIFT-03 value is audition-gated and it will build the apparatus anyway.

---

## 23. DEFERRED — polyBLAMP / slope correction on the sync path, the documented FIRST escalation

- **Carried from:** D-08 and `33-CONTEXT.md`. **Restated unchanged.**
- **What it is:** the documented **first** escalation if a sync alias threshold proves unreachable,
  **ahead of any kernel-order change** (item 24) and far ahead of oversampling.
- **A measurement that bears on it, recorded here so it is not lost:** item 16 found that the
  binding error term under a band-limited master is the **5.88 % late-fire rate**, not the
  interpolation — so **a BLAMP escalation is specifically NOT indicated by this phase's
  measurement.** If a threshold is missed, check the detection threshold first.
- **Resolve at:** **the first plan that misses a sync threshold** — and it must read item 16 before
  reaching for the kernel.

---

## 24. DEFERRED — the kernel-order question: four-point (quintic) polyBLEP/polyBLAMP

- **Inherited as Phase 32's register item 10. Restated unchanged.**
- **Where it stands:** `AA-05` forbids **minBLEP** and **oversampling** by name and says **nothing
  about kernel order**, so a four-point kernel is **unscoped rather than forbidden**. That
  distinction is the whole of the item.
- **Resolve at:** **an OPERATOR DECISION with an impact assessment — NEVER a silent implementation
  choice.** The broad escalation path remains **v2.1 oversampling, explicitly not minBLEP.**

---

## 25. DEFERRED — the narrow-pulse "reach" refinement

- **Inherited as Phase 32's register item 9. Restated unchanged.**
- **Worth:** about **+1.3 dB** at the worst grid point.
- **Not taken because:** it would add the **only division by an edge width** in
  `src/dsp/MorphBlep.hpp`, and this header's hostile-parameter history (a `+inf` `dt` reaching a
  divisor and leaving `pending = NaN` permanently, measured in Phase 32) is the reason a new
  divisor is not a free change.
- **Resolve at:** **the first plan that misses a pulse threshold.**

---

## 26. OPEN AND NO LONGER HYPOTHETICAL — the spectral column's cross-toolchain portability

- **Inherited as Phase 32's register item 8, pointed at Phase 36. This phase's sync rows joined it,
  and this gate turned it from a risk into a measurement.**
- **Evidence, before:** Phase 32 measured this instrument toolchain-dependent by up to
  **3.02596 dB** with **no `src/` behaviour differing at all** — `aliasPeakDb` is a max over 2043
  bins and a libm ULP difference reorders near-tied bins.
- **Evidence, now:** items 1 and 2 above. **420 pinned decibels and three pinned population counts
  were added by this phase, and 14 columns plus 1 classification do not reproduce off Apple clang.**
- **The binding consequence, restated:** **no spectral golden may be captured from one toolchain**,
  and no tag or VCV Library resubmission may be cut while items 1 and 2 stand.
- **Resolve at:** **Phase 36**, and it now has a concrete list of 15 cells to start from rather
  than a general warning.

---

## 27. OPEN, DELIBERATELY UNOWNED, POINTED AT NO PHASE — the shipped module's shared latent undefined behaviour

- **Inherited as Phase 31's register item 12 / deferred item 1. Restated unchanged and still
  deliberately unowned.**
- **Evidence:** a one-shot UBSan probe reported `RackCompat.hpp:106:24` float-cast-overflow and
  `:109:11` left-shift. **The SHIPPED Analog LFO reaches the identical latent UB** via
  `src/AnalogLFO.cpp:320 → src/dsp/LfoCore.hpp:183-186 → src/dsp/RackCompat.hpp:106/:109`. Every
  behavioural assertion was already **green** before and after the VCO-side fix.
- **Why it has NO PHASE:** picking it up means editing a frozen, golden-pinned, live-in-the-library
  header. **Whoever picks it up is opening a GUARDRAIL EVENT and must open it as one** — impact,
  remediation options and a recommendation to the operator, before acting.
- **The binding consequence, and it constrained this phase:** **no permanent repository-wide
  sanitizer gate may be adopted** while this stands. Every sanitizer use in Phase 33 — including
  plan 33-01's ASan RED for CR-01 — was a **scoped one-shot probe** run outside the repository, and
  no sanitizer target exists in `Makefile`, `GUARD_SCRIPTS`, `TEST_CXXFLAGS` or CI.
- **Resolve at:** **NO PHASE.** This is deliberate and is stated rather than left as an omission.

---

## 28. OPEN — the two live tripwires carried from Phase 32

- **Inherited as Phase 32's register items 22 and 23. Restated unchanged.** Two entries kept
  together because both are *tripwires*: neither is work to schedule; each is the **first thing to
  check** when a specific symptom appears.
  1. **The `dt = 0.0005` resonant-tiling miss (item 22).** If a sync grid cell misses its threshold
     at a suspiciously **round** sample rate, this is the first thing to check.
  2. **The unreachable `dt <= 1` upper guard (item 23).** If any change makes a caller reach
     `MorphBlep` **without clamping**, scenario four's `UPPER = 0` census must be **re-measured, not
     assumed**. Phase 33 added the second call site and its arguments are conditioned; a third would
     need the same check.
- **Resolve at:** on symptom, in whichever phase produces it. No scheduled work.

---

## 29. OPEN — the second, differently-slugged plugin directory in the operator's Rack tree

- **Inherited as Phase 32's register items 15 and 25. Restated unchanged.**
- **What it is:** `~/Library/Application Support/Rack2/plugins-mac-arm64/ForgeAudio` — slug
  `ForgeAudio`, version 2.0.0, module `ForgeAudioLFO`, dated Feb 14. **Harmless**: a different slug,
  so it cannot shadow the current `ForgeAudio-AnalogSeries` install. But **both plugins declare the
  same brand string "Forge Audio"**, so the browser shows **three entries under one heading**.
- **The protocol half is CLOSED** (plan 32-11): the guardrail subject is pinned **BY NAME** —
  shipped is **"Analog LFO"** from `ForgeAudio-AnalogSeries` 2.0.1 (`_modelAnalogLFO`); stale is
  plain **"LFO"** from `ForgeAudio` 2.0.0 (`_modelLFO`). Plan 33-12's UAT must use those names.
- **Not done, and not an executor's call:** deleting a plugin from the operator's Rack installation.
- **Resolve at:** **the operator's housekeeping, at their discretion.** No phase owns it.

---

## 30. OPEN, MINOR — 16-bit at the audition scale does not resolve the smallest corrections

- **Opened by:** plan **33-10** (item 4). **Restated unchanged.**
- **Evidence:** 305.2 µV per LSB at a 10 V full scale; **22 of 2003** per-reset corrections at
  render points 01 and 04 fall below it — real in the float buffer, **absent from the WAV**. About
  −84 dBFS, so irrelevant to an audition, and now **reported by the tool on every run**.
- **Rejected:** moving to 32-bit float now. Research assumption A4 records 16-bit PCM as the safest
  format across the operator's players, and the change is ~10 lines whenever it is actually needed.
- **Resolve at:** **no owner needed.** Revisit only if an audition question requires it.

---

## 31. PARTLY DISCHARGED — every decibel, volt and cent in this phase is an Apple-clang figure

- **Carried unchanged in kind from 33-01 through 33-10, where every plan registered it.** This gate
  is the first measurement of it.
- **DISCHARGED:** the shipped-code toolchain exposure. `toolchain-gate` is green on every step
  including the **MinGW compile and full link against `libRack`** — **T-33-08 is discharged on the
  exact commit** — and the time-domain volt-scale constants, the cent-scale tolerance, the ρ edges,
  the population counts `277 / 143 / 69`, the output tier and every SYNC-01 assertion **held on
  GCC and MinGW**.
- **NOT DISCHARGED:** the **spectral decibel** exposure, which is items 1, 2 and 26 and is now
  measured rather than feared.
- **The useful summary for a later phase:** *this phase's volts crossed the toolchain; its decibels
  did not.* That is a sharper statement than "everything is Apple-clang" and it is the one to carry
  forward.
- **Resolve at:** **Phase 36**, for the decibels only, with items 1, 2 and 26.

---

## 32. PROCESS — a criterion whose MECHANISM is narrower or wider than its own PROSE

- **Opened repeatedly.** By this gate's count there have been **at least thirteen** recorded
  instances across Phases 30–33, and **this gate produced two more**.
- **The instances this phase recorded:** 33-07's reproduction-check criterion (the tenth); 33-08's
  Task-2 wording anticipating a margin *"too small to be a useful gate"* when the measured problem
  was the **sign** (the eleventh); 33-10's `grep -c 'VcoCore '` returning 2 because a **string
  literal** counts like a declaration (the twelfth), and its peak-level criterion, which reads as
  **failed on correct behaviour** because the peak of a hard-synced waveform lands on the sample the
  correction modifies.
- **The two more, produced here:** (a) `-tc="<the PITCH-04 case's exact full title>"` matches
  **zero** cases and prints `Status: SUCCESS!`, because the title contains commas and doctest reads
  a comma as a filter separator — the *exact* title is the one selector that cannot select it;
  (b) `-ts="*golden*"` — a plausible one-character-different form of the working `-tc` selector —
  likewise matches **0 cases**, runs nothing, and prints `Status: SUCCESS!`.
- **The habit that has worked every time:** report the numbers rather than the verdict, and **never
  rename a symbol, reword a string or retype an assertion to make a criterion's mechanism true**.
  Not once in this phase was that done.
- **Resolve at:** no code change. This is a **standing reading rule** for every future plan: read a
  criterion's mechanism against its prose *before* running it, and record the divergence rather
  than satisfying it.

---

## 33. PROCESS — the zero-warning gate has no teeth if nobody reads the warnings

- **Opened by:** plan **33-09**, deviation 1.
- **Evidence:** a real defect — `kSyncStaleStore` falling through a `switch`, so a whole sync row
  presented a constant 0 V and never fired — was caught by **`-Wall -Wextra`, which emitted
  `warning: enumeration value 'kSyncStaleStore' not handled in switch [-Wswitch]` on that build.**
  It was missed because the build output was read with **`tail`**, which shows the doctest summary
  and hides everything above it. The bug surfaced instead as 87 red assertions.
- **What this gate did about it:** both build logs were captured to files and grepped **in full** —
  `grep -cE 'warning:|error:'` over the whole log, reported as **0** in §0.1 rather than inferred
  from a clean tail.
- **Resolve at:** no code change; a **standing convention**, like item 14. The stronger fix — adding
  `-Werror` to `TEST_CXXFLAGS` — is deliberately **not** proposed, because it would turn a
  third-party or toolchain-version warning on a CI leg into a hard red and this project has three
  legs on two compiler families.

---

## 34. PROCESS — `state.advance-plan` clobbers three `STATE.md` fields with the previous plan's text

- **Opened by:** plans **33-07, 33-08, 33-09 and 33-10** — **four consecutive plans**, each
  repairing it by hand.
- **Evidence:** the verb overwrites `stopped_at`, `last_activity_desc` and the body's
  `Last activity:` line with the **previous** plan's text. 33-10's self-check records finding all
  three still reading *"Plan 33-09 complete."* after the verb ran, and repairing them.
- **Why it is worth a register entry rather than four repairs:** an unrepaired `STATE.md` tells the
  next session the wrong plan finished last, which is precisely the field a resume reads first.
- **Resolve at:** **the GSD toolchain, not this project.** Recorded here so the fifth plan to hit it
  does not think it is the first. Every plan in this phase must continue to **check the three fields
  against disk after running the verb**, which this plan did.

---

## 35. CLOSED — Phase 32's deferred item 26: the audition with no A/B reference

- **Closed by:** plan **33-10**.
- **The debt:** Phase 32's operator audition asked whether an improvement was **audible** and gave
  the operator nothing to compare against. The verbatim reply — *"Seems to work well enough - but
  it's hard to remember what the old audio sounded like"* — was correct and useful about a question
  that was **unanswerable by construction**.
- **The remedy delivered:** `make audition` renders **seven matched pairs / fourteen uncommitted
  16-bit WAVs**, both legs from **one pass** through the real `forge::VcoCore`, into the
  already-ignored `build-test/audition/`.
- **Note for the record:** 32-11 proposed `NaiveVcoCoreMirror` as the mechanism. **It was not used
  and did not need to be** — the recording-only telemetry subtraction 33-06 landed is *strictly
  stronger* than a bit-exact mirror, because there is no second code path to keep in step.
  `grep -c 'Mirror' tools/render_sync_ab.cpp` is **0**.
- **Resolve at:** CLOSED. **The audition itself is plan 33-12's**, and a favourable verdict must not
  be booked as closing item 3.

---

## 36. DEFERRED TO PHASE 34 — CHARACTER's CV input and attenuverter (CHAR-01)

- **Inherited as Phase 32's register item 15. Restated unchanged**, including its two consequences:
  it **closes the compile canary's one-field margin** (the shell currently feeds runtime-derived
  values into three of eight `VcoInputs` DSP fields where the canary feeds all eight), and it
  carries **the remaining half of Phase 30's CR-02** (the morph/character half of the clamp-helper
  item, whose pitch-volt half was closed in Phase 31).
- **Resolve at:** **Phase 34.**

---

## 37. DEFERRED TO PHASE 34 — the output stage, drift, and DRIFT-03's audition-gated value

- **Carried from `33-CONTEXT.md`. Restated unchanged.** OUT-01..03, DRIFT-01..03.
- **The warning Phase 34 must read first:** once drift writes the `*Spread` fields, **every
  discontinuity position moves per sample**, and Phase 32's D-04 **recompute-never-cache** rule is
  what keeps `MorphBlep` correct. And item 17 above will turn invariant 10 red on correct behaviour
  the moment CHARACTER sweeps.
- **The instrument for DRIFT-03 now exists:** D-16's renderer, inherited rather than rebuilt.
- **Resolve at:** **Phase 34.**

---

## 38. DEFERRED TO PHASE 34/35 — per-instance seed entropy and patch persistence in the shell

- **Inherited as Phase 32's register item 16. Restated unchanged**, including the hazard: a
  `forge::Xoroshiro128Plus` seeded **(0,0)** is a degenerate fixed point emitting an all-zero
  stream, which makes `std::normal_distribution`'s rejection loop **never terminate** — a **hang
  while opening a patch** in Rack, not a failing test (T-33-34). Any deserialisation path **MUST
  re-validate** the seed; the shipped LFO's draw / reject-(0,0) / persist / non-throwing-parse
  pattern is the model.
- **Today every `AnalogVCO` in a patch is a bit-identical clone** (0 of 2048 differing samples).
- **Resolve at:** **Phase 34 or 35.**

---

## 39. DEFERRED TO PHASE 35 — the FM DEPTH knob's affordance, and a COARSE octave/semitone snap

- **Inherited as Phase 32's register items 17 and 18. Both restated unchanged.**
- **The first is the operator's first piece of VCO panel feedback.** FM-02's bipolar behaviour is
  locked and verified; only the widget is open.
- **Resolve at:** **Phase 35** for the affordance; **Phase 35 or v2.1** for the snap.

---

## 40. DEFERRED TO PHASE 36 — `plugin.json` still declares version `2.0.1` while shipping two modules

- **Inherited as Phase 32's register item 20 (REL-01). Restated unchanged.**
- **Resolve at:** **Phase 36**, with the VCV Library feature-update mechanics on thread **#929**.
  The standing rule binds: **no tag and no resubmission on local evidence alone** — and items 1, 2
  and 26 must be resolved first.

---

## 41. DEFERRED TO PHASE 36 — wire `tests/check_docs.sh` into CI

- **Carried from `33-CONTEXT.md`'s reviewed-todos section. Restated unchanged and deliberately not
  re-litigated:** it was reviewed and deferred to Phase 36 during **both** Phase 31 and Phase 32 on
  identical reasoning. It is a one-line CI step for a Phase 27 documentation gate and has nothing to
  do with hard sync.
- **It stays visible:** the `GUARD_WIRING_EXEMPT` entry in `tests/check_includes.sh` `[7/7]` reports
  it as **`EXEMPT`** on every `make guards` run, which it did on this gate.
- **Resolve at:** **Phase 36.**

---

# SECTION TWO — WHAT PHASE 33 MEASURED

**A later phase should be able to reach every number this phase measured without opening a plan
summary.** Every figure below is an **Apple-clang** figure unless the row says otherwise; item 31
records which of them have since crossed a toolchain and which have not.

---

## 2.1 The D-06 placement measurement — the phase's central question

**The rule REFUSED: all three conditions FAILED and no winner was declared** (plan 33-05). The
past-edge leg was landed by an **operator decision** of 2026-08-30 and is labelled EVIDENCE-BASED,
NOT RULE-SANCTIONED in both shipped headers.

**The four-leg table, over the valid step-dominated population:**

| candidate | wins (all 140) | frac | wins (step-dom 124) | frac | **wins (valid+step 54)** | **frac** | **worst deficit** |
|---|---|---|---|---|---|---|---|
| `none` | — | — | — | — | 10 | 0.1852 | **3.93 dB** |
| `detect` | 6 | 0.0429 | 2 | 0.0161 | **0** | **0.0000** | **5.0518 dB** |
| **`pastEdge`** ← landed | **66** | **0.4714** | **55** | **0.4435** | **34** | **0.6296** | **0.8553 dB** |
| `flatHalf` | 60 | 0.4286 | 51 | 0.4113 | 10 | 0.1852 | **10.4567 dB** |

**The decisive figure is the last column.** `pastEdge`'s worst single-cell deficit against the best
other candidate — **0.8553 dB** — is the **only** one inside register item 8's 1.0 dB
step-dominated reproduction bound. It is the only candidate that is never *materially* worse than
anything else, anywhere on the valid grid.

**The three conditions, and how each failed:**

| Condition | Result |
|---|---|
| 1 — 90 % of valid step-dominated 44.1 kHz cells | **FAIL** at 34 of 54 = **0.6296** … but its **deficit clause PASSES** at 0.8553 dB |
| 2 — sub-unity cells clearing the bound | **FAIL** — 22 of 38 |
| 3 — rate signature | **FAIL FLAT** — 0.897 / 0.901 / 0.849 dB on the common cell across a factor of 2.2 in rate, which by the rule's own wording means the legs differ in jump **MAGNITUDE**, not placement |

**Why condition 3 could never have separated them:** `pastEdge` and `flatHalf` differ by a factor
of `f²`, which **is** a magnitude difference. The only pair differing purely in placement is
`pastEdge` against `detect`, and `detect` is eliminated on condition 1's own evidence.

---

## 2.2 The snap-to-zero landmine — SYNC-02's sub-sample clause, and it cuts both ways

| master edge | 44.1 kHz | 48 kHz | 96 kHz | reading |
|---|---|---|---|---|
| **band-limited** | **+5.040** | **+4.985** | **+5.610** | **snap is WORSE** — the sub-sample reset is a real win |
| **hard-edge** | **−0.813** | **−1.040** | **−0.766** | **snap is BETTER** |

- **On band-limited masters this is SYNC-02's sub-sample clause turned into evidence with a
  comfortable margin**, landing within a decibel of 33-RESEARCH's 4.5–4.95 dB prototype
  prediction. It is the **only** sync claim on this grid clearing both reproduction bounds at every
  rate, and its sign is asserted permanently.
- **On hard-edge masters 33-RESEARCH's prediction that the two measure IDENTICALLY is FALSIFIED IN
  DIRECTION.** With a single-sample master wrap there is **no sub-sample information to preserve**;
  the detector's fraction is a near-constant ≈ 0.5968 that has nothing to do with the true
  crossing. **The sub-sample reset is a win only when the master is band-limited** — which is what
  another Forge VCO produces — and must not be claimed generally.
- Magnitudes are **recorded and deliberately NOT gated**; the case is
  `tests/test_vco_spectrum.cpp:6015`, **1 case / 226 assertions**, green on all three toolchains.

---

## 2.3 The SC-3 time-domain instrument — the reset-step envelope

| | 44.1 kHz | 48 kHz | 96 kHz | pinned bound | margin |
|---|---|---|---|---|---|
| **Shipped (corrected) leg** | **9.793601** | **9.793601** | **9.793601** | 9.90 V | **0.106 V** |
| Withheld leg (diagnostic) | 10.000000 | 10.000000 | 10.000000 | — | −0.100 V (**above** the bound, by design) |

- **The headline, reported rather than buried: the shipped sync BLEP removes about TWO PERCENT of
  the worst-case reset step.** 0.206 V of a 10 V step.
- `kSyncResetDeltaBoundV = 9.90 V` is pinned from a **TWO-SIDED** interval `[9.793601, 10.000000)`,
  midpoint 9.896800, rounded outward. Constraint (b) — strictly below what a seam-free core
  measures — is what makes it falsifiable, and **it fires**: commenting the seam out of
  `src/dsp/VcoCore.hpp` reds the case at **4 of 32** assertions (`10 <= 9.9`) and the suite at
  2 cases / 416 assertions.
- The **analytic** bound `2 × kHostileBoundV = 20.0 V` is rejected **in an assertion**, not in
  prose, and a **floor** of 9.0 V is asserted too, because this is not a smallness claim: a
  legitimate reset genuinely steps by nearly full scale.
- **Grid:** 420 cells, **1,720,320 core steps, 13,230 resets**, 0.12 s per pass. Reset samples
  identified from `tel.syncFired`, with the recorder's linkage to the live core asserted by exact
  equality **before** anything is measured through it.
- **Subject to item 4:** this grid's `1/128` master gives `g ≡ 1.0` on all 32 wraps and never wraps
  between two samples. On the spectral master the same metric reads **9.999983 V**.

---

## 2.4 The anti-circularity margin distribution — the shape of the evidence, not one number

Margin ≡ (worst withheld reset step) − (worst shipped reset step), over the **277** gated cells.

| | Value | Cell |
|---|---|---|
| **minimum** | **0.095148 V** | 48 kHz, band-limited, ratio 5.50, square centre, character 1.00 |
| median | **0.874437 V** | |
| maximum | **1.781152 V** | |

**Per rate:** 44.1 kHz — n 91, min 0.095156, median 0.206400, max 1.781152 · 48 kHz — n 95, min
0.095148, median 0.882165, max 1.781152 · 96 kHz — n 91, min 0.095148, median 0.206400, max
1.781152.

**Per master edge shape** — and this is the informative split:

| edge | n | min | median | max |
|---|---|---|---|---|
| hard-edge | 139 | **0.874437** | 1.597610 | 1.781152 |
| band-limited | 138 | **0.095148** | 0.173168 | 0.206400 |

**The binding cells are the band-limited ones by an order of magnitude, and that is the expected
direction:** on a hard-edged master the detected fraction is a nearly constant 0.5968, so `f²` is a
stable 0.36; on a band-limited master `f` ranges across most of the unit interval and the deposit
is small wherever `f` is small.

**Provenance of the pin:** 0.095148 → **0.09** (rounded outward, downward — it is a floor) →
**0.04** (halved, rounded outward again) = `kSyncAntiCircularityMarginV`. **Cushion 2.38×**, spent
on the Apple-clang exposure and stated as such. (33-07 spent 5× on its snap floor for the same
reason.)

**Population:** gated **277**, ungated **143**, both asserted exactly, with the classifier placed
inside a **MEASURED EMPTY GAP 0.639500 … 0.921976** — a **1.44×** window — and **both edges
asserted**. **This is the classifier that held on GCC**; contrast item 2.

**Three mutation probes, each failing a STATED population EXACTLY:** quarter deposit **69 of 277**
(stated 69), half deposit **0** (stated 0 — the control that makes the first discriminating rather
than trivial), inverted sign **277 of 277** (stated 277). The derivation they rest on was measured,
not assumed: the departure of `margin(0.25)` from `0.25 × margin(1)` over all 277 cells is
**exactly 0.000e+00**.

**And the uncomfortable half:** **56 of 420 cells have a NEGATIVE margin**, worst **−0.246492 V at
ratio 5.50**, none of them inside the gated population. Negatives by ratio — 0.50 → 6, 0.75 → 0,
1.00 → 17, 1.50 → 6, 2.50 → 9, 3.50 → 9, 5.50 → 9. The **count is deliberately not pinned as an
equality** (20 of the 56 are within a millionth of a volt of zero); the worst value and its ratio
are pinned instead. **Read this figure with item 4 in front of you.**

---

## 2.5 The output tier, re-derived for sync rather than inherited

| | 44.1 kHz | 48 kHz | 96 kHz | grid-wide |
|---|---|---|---|---|
| **Worst \|out\| on the SC-3 sweep** | **8.218569** | **8.216589** | **8.216589** | **8.218569 V** |
| the cell | hard-edge, ratio 1.50, square, char 0.00 | hard-edge, ratio 5.50, square, char 0.00 | hard-edge, ratio 5.50, square, char 0.00 | 44.1 kHz row |

**This is the largest envelope measured anywhere in the VCO suite.** Scenario five's Nyquist
ceiling reaches 7.150281 V; the audio-rate MORPH sweep reaches 6.289864 V; hard sync reaches
**8.218569 V** — 1.07 V above the previous maximum and still 1.78 V inside `kHostileBoundV`.

| Tier | Decision |
|---|---|
| `kHostileBoundV` = 10.0 V | **ASSERTED** unconditionally, grid-wide **and** per rate |
| `kMusicalBoundV` = 5.55 V | **WITHHELD, and the withholding ASSERTED** — exercise floor 6.70 V, grid-wide **and** per rate, plus `CHECK(kSyncExerciseFloorV > kMusicalBoundV)` |

**Neither constant was widened.** The **per-rate** form of the withholding was **checked before it
was written**, because invariant 6 records exactly where the same form would be red at 96 kHz on
correct behaviour; here all three rates exceed the tighter tier by at least **2.67 V**, so the
stronger form is true and is the one landed.

**A stale figure the seam falsified, corrected by APPENDING rather than overwriting:** invariant 7's
recorded sync envelope was 4.920715 / 4.920976 / 4.921710 V, measured pre-seam by plan 33-04.
**Re-measured on the shipped past-edge leg: 4.908170 / 4.910800 / 4.920170 V.**

---

## 2.6 The spectral sub-grid, as a gate

- **420 cells pinned per cell** via a `SYNC_PINS` lookup; **210 instrument-valid cells CHECKed**
  (70 gated at 44.1 kHz binding, 140 regression at 48/96 kHz); **210 instrument-invalid cells
  diagnostic BY DECISION.**
- `thresholdDb == max(ceil(measuredDb + bound), −75)`, bound **1.0 dB** step-dominated / **4.0 dB**
  plateau. **Worst gate headroom 1.00245 dB.** The static floor binds on **0** cells.
- **Populations 402 / 18**, both asserted exactly, from a **two-clause** criterion: (i) mean
  `|syncJump| >= 0.01`, and (ii) the **slave's** own discontinuity by Phase 32's measured shape
  partition. **Both clauses measured to do work — 192 jump-only, 24 shape-only, 186 both.** Clause
  (i) alone would have called a unity-ratio **saw** cell a plateau. **See items 1 and 2: these are
  the numbers that do not reproduce off Apple clang.**
- **The Phase-32-shaped improvement gate is REFUSED IN WRITING** at
  `tests/test_vco_spectrum.cpp:1312-1382`, with its measured reason: the sync correction's own
  spectral improvement is **+0.5827 dB grid-wide** (33-VALIDATION predicted ≈ 0.5 in advance), so a
  `naiveDb − correctedDb >= 8.0` gate fails **by construction**.
- **Per-ratio mean of `none − pastEdge`** (positive = shipped better), 60 cells each:

  | 0.50 | 0.75 | 1.00 | 1.50 | 2.50 | 3.50 | 5.50 |
  |---|---|---|---|---|---|---|
  | **+2.4495** | **+1.9150** | +0.0037 | **+0.7247** | +0.2051 | **−0.1911** | **−1.0281** |

  At ratio 5.50 the shipped leg is worse on **47 of 60** cells, worst single cell **7.0218 dB**.
  The sign at 5.50 is asserted permanently.
- **A single-sample full-amplitude spike measures 0.0 dB spectrally** — the measurement that sent
  the click claim to the time domain.
- **Late fires:** **1,820 of 30,940 resets (5.88 %)**, all on band-limited masters. The oracle leg
  is **0.45–0.71 dB worse** than the detector's own fraction.
- **D-07's residual phantom:** mean **0.0569**, **max 0.9624** over 30,940 reset samples.

---

## 2.7 SYNC-01's detector, and its structural ceiling

**The reset:** a master rising edge resets phase to the **fractional overshoot**, never to exactly
zero. Guards use the negated-comparison idiom, never `forge::clamp` (both of a clamp's comparisons
are false for a NaN).

**The structural ceiling — the detector fires AT MOST ONCE per sample**, measured identically at
all three rates over 8192 samples:

| `dtm` | master vs. rate | wraps | fired | what the ceiling costs |
|---|---|---|---|---|
| 0.0625 | 1/16× | 512 | **512** | nothing |
| 0.125 | 1/8× | 1024 | **1024** | nothing |
| 0.25 | 1/4× | 2048 | **2048** | nothing |
| 0.75 | 3/4× | 6144 | **2048** | 2 of every 3 |
| **1.0** | **1×** | 8192 | **0** | **every edge** |
| 1.5 | 3/2× | 12288 | **4096** | 2 of every 3 |
| 2.5 | 5/2× | 20480 | **4096** | 4 of every 5 |
| **4.0** | **4×** | 32768 | **0** | **every edge** |

Each row is asserted as a **triple** — (total wraps, fired cycles, FNV-1a hash of the whole
fired/missed pattern indexed by the master's cycle) — so two rates cannot agree on *how many* edges
were missed while disagreeing on *which*. **The `1×` and `4×` rows are the honest face of the
ceiling and are kept for that reason:** at an integer ratio the master is a constant +5 V and the
trigger never re-arms, so the edges are not under-sampled, they are **invisible**.

---

## 2.8 PITCH-04's third input class

**783 cells** = 29 pitch/FM rows × 9 sync shapes × 3 rates, 4000 steps each = **3,132,000 core
steps**; **1 case / 6,478 assertions**, green on all three toolchains.

- `tel.freqHz` under every hostile sync shape is **EXACTLY EQUAL by float `==`** to the unpatched
  row's, on all 783 cells. Proved able to fail **384** times by a one-line
  `if (in.syncConnected) freq *= 1.0001f;` in the shipped header.
- Firing counts asserted in **both** directions; per-row min/max over 87 cells each: unpatched 0/0 ·
  musical 110 Hz 5/10 · musical 55 Hz 3/5 · master far above the rate 84/167 · idling 0/0 · held
  NaN 0/0 · exactly on the threshold 62/62 · NaN glitch 5/10 · low-then-steady 1/1. Proved able to
  fail at **522** assertions (grid unpatched) and **27** (targeted probe).
- Tracking: unison row worst **0.006936 cents** against an unwidened 0.05; master-locked rows
  **0.020443 cents** against `kSyncLockToleranceCents = 0.10`, with the un-synced alternative
  **4,980 tolerances** away.
- The sanitised NaN pitch path lands at **1.418275e-17 Hz**, not 0 — an `== 0.f` expectation would
  have redded **81 of 783** cells on correct behaviour.

---

## 2.9 The audition pair — properties, so 33-12 need not re-derive them

| Property | Value |
|---|---|
| Command / output | `make audition` → `build-test/audition/`, **14 files ≈ 2.4 MB**, ~3 s |
| Committed? | **No, by construction** — already-ignored directory, `git diff --stat .gitignore` empty |
| Format | RIFF/WAVE, mono, **44100 Hz, 16-bit PCM**, 2.000000 s, 176,444 bytes each; validated field-by-field **and** independently by macOS `afinfo` |
| Legs | `leg-A-shipped` (what the module does today) / `leg-B-withheld` (the reference), from **one pass** through the real core via `x = s + 5·(k−1)·tel.syncCorrection` |
| Scale | **0.1 V→FS: 10.0 V = full scale = `kHostileBoundV`**, so a clipped sample *means* the core exceeded the tier the suite pins. **Clipped: 0. Non-finite: 0.** Peaks ≈ −6 dBFS — **the files sound quiet, deliberately** |
| Normalisation | **Never, per leg.** Stated in the source |
| Level match | **RMS, not peak** — within **0.1185 dB** everywhere, **0.0001 dB** on the null-point control. Peaks differ by up to **1.075448 V** and that is CORRECT: the peak lands on a reset sample |
| Nothing owed forward | Differing samples **equal the reset count exactly** on all 7 points (2003/2003/2002/2003/2002/2002/689) |
| Self-check | In the tool, on every run: same length · not bit-identical · neither silent · **differs AS ENCODED**. Proved able to fail (`k 0.0f → 1.0f` reds all 7 and exits 1) |
| Determinism | All 14 files **byte-identical** across two consecutive runs |
| Sub-LSB | 305.2 µV per LSB; **22 of 2003** corrections at points 01 and 04 fall below it (item 30) |

**The seven render points** — all 44.1 kHz, mono, 2.000 s, drift off:

| # | Master | Slave | Ratio | Shape | Char | Largest \|A−B\| | RMS Δ | Reset-step margin | Honest expectation |
|---|---|---|---|---|---|---|---|---|---|
| 01 | 1001.29 Hz | 500.65 | 0.50 | saw | 0.00 | 2.427443 V | −0.0914 dB | +0.003555 V | Subtle, on 2.27 % of samples |
| 02 | 1001.29 | 500.65 | 0.50 | pulse 5% | 0.00 | 4.853238 V | −0.0527 dB | +0.000018 V | Biggest per-sample change; **worst-case step essentially uncorrected** |
| 03 | 1001.29 | 1501.94 | 1.50 | saw | 0.00 | 1.004344 V | −0.1018 dB | **+1.001893 V** | **Most likely point to hear a real improvement** |
| 04 | 1001.29 | 5507.12 | 5.50 | saw | 0.00 | 2.435740 V | −0.1185 dB | +0.019630 V | Spectrally **worse**; may sound *rougher* |
| 05 | 1001.29 | 1001.29 | 1.00 | saw | 0.00 | 0.041873 V | **−0.0001 dB** | −0.001738 V | **THE CONTROL — should be indistinguishable** |
| 06 | 1001.29 | 5507.12 | 5.50 | square | 1.00 | 1.421893 V | −0.0318 dB | **+1.419190 V** | Audible improvement expected |
| 07 | **344.53** | 1894.92 | 5.50 | square | 1.00 | 0.427492 V | −0.0019 dB | **−0.427492 V** | **Correction makes the worst step LARGER** |

**06 and 07 are the SAME cell on the two masters** (item 4) — the pair that lets the operator hear
the region flip. Points 01–06 reset 2002–2003 times per second; point 07 resets 689 times.

---

## 2.10 Suite growth, plan by plan

| After plan | Cases | Assertions |
|---|---|---|
| *(pre-phase, `9de82cf`)* | 94 | 2,622,319 |
| 33-01 | 97 | 2,622,378 |
| 33-02 / 33-03 | 97 | 2,622,378 *(both added zero, and both said so)* |
| 33-04 | 100 | 2,623,356 |
| 33-05 | 103 | 2,624,784 |
| 33-06 | 104 | 2,625,138 |
| 33-07 | 106 | 2,631,627 |
| 33-08 | 108 | 2,632,235 |
| 33-09 | **109** | **2,638,713** |
| 33-10 | 109 | 2,638,713 *(a tool, not a test)* |
| **33-11 (this gate)** | **109** | **2,638,713** |

**Phase total: +15 cases, +16,394 assertions** on macOS; **+15 / +16,392** on Ubuntu and Windows.

---

# SECTION THREE — RESUME MATERIAL

## What is complete

- **All eleven executed plans of Phase 33 (33-01 … 33-11).** Every one landed its artefacts and
  every one is summarised.
- **The mechanism ships.** `forge::VcoCore` detects a master rising edge, solves the sub-sample
  fraction, resets to the fractional overshoot and band-limits the reset through
  `forge::MorphBlep::addPastStep`.
- **SYNC-01 is Complete** (ticked by plan 33-04) and its two cases are green on all three
  toolchains.
- **T-33-08 is DISCHARGED.** The MinGW compile-and-full-link reproduction succeeded on `da9e611`,
  observed by hash equality, with its own step conclusion recorded.
- **The LFO guardrail is discharged for this phase by AUTOMATED evidence, and the KIND is stated:**
  zero changed lines on all 15 frozen paths over the whole phase diff; `FROZEN.sha256`
  **byte-identical by `cmp`**; all six `.f32` fixtures byte-identical by `cmp`; the six goldens
  replaying at **9 cases / 49,188 assertions / 0 failures**; and `src/AnalogLFO.cpp` **absent** from
  the phase diff. **No operator attestation has been given and none is inferred** — plan 33-12 owns
  the asking.

## What is NOT complete

- **THE PHASE GATE IS RED.** Two of three CI test legs fail. Items **1** and **2** are the findings
  and both are pointed at **Phase 36**. Neither was absorbed by widening anything.
- **SYNC-02 is Pending — declined for the thirteenth consecutive time.** Its remaining gap
  (item **3**) is the residual-versus-intended-step separation, which **no instrument in this phase
  measures** and which **still has no owner**.
- **The roadmap's Phase 33 entry is NOT marked complete**, and no requirement status changed.

## THE OPERATOR GATE HAS NOT BEEN ANSWERED

**Plan 33-12 is what answers it, and until then Phase 33 is not closed.**

The A/B pair exists, is mechanically verified and has **not been listened to by a human**. Plan
33-12 owns the perceptual verdict and must:

1. Write the **blocking `.continue-here.md` BEFORE** the UAT plan (D-17 precedent 1). A
   checkpoint-pending SUMMARY otherwise lets a resume skip the operator gate entirely.
2. Present plan 33-10's **banked expected-results block verbatim, in full, BEFORE the operator
   replies** (D-17 precedent 2), so an absence of complaint is an absence of complaint rather than
   an absence of exposure. **Do not rewrite it** — every per-point figure in it comes from that
   render rather than from an inherited table.
3. Tell the operator the difference is **SMALL**. Eighteen microvolts of the worst-case step;
   +0.5827 dB grid-wide; under 1.25 dB on 33-05's own leg table. **An operator told to expect an
   obvious "click disappearing" would be told something no instrument in this phase supports.**
4. Say **before** they listen that **points 04 and 07 are where the correction LOSES**, and that
   **point 05 is a control where the two legs should be indistinguishable**.
5. **REFUSE** the six coverage items the pair cannot evidence rather than booking them — in
   particular that **a favourable audition does NOT close item 3.**
6. Use the module names, not the brand: shipped is **"Analog LFO"** from
   `ForgeAudio-AnalogSeries` 2.0.1; the stale one is plain **"LFO"** (item 29). Flush the **whole**
   extracted plugin directory with `rsync -a dist/ForgeAudio-AnalogSeries/`, never just
   `plugin.dylib` and `res/`.
7. **Take the SYNC-02 disposition explicitly** (item 3) — a carry, not a closure — and ask for the
   LFO guardrail attestation, recording it **only if actually given**.

## And carry this sentence forward

**The full local gate was green on the same commit the CI legs failed.** Phase 29 measured that
combination green on code that could not link; Phase 33 has now measured it green on code whose
pinned columns do not reproduce. *Local green is a precondition. It has never been the gate.*

---

*Phase: 33-hard-sync*
*Register filed: 2026-09-02 by plan 33-11*



