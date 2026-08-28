---
phase: 33-hard-sync
plan: 01
subsystem: dsp
tags: [morphblep, polyblep, hostile-input, nan-guard, addresssanitizer, undefined-behavior, cpp11, bit-identity]

# Dependency graph
requires:
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "src/dsp/MorphBlep.hpp, its nine-site union, the D-03 character factor, the D-14 addStep seam, and tests/test_morph_blep.cpp's six-case unit suite"
  - phase: 30-vco-core-registration
    provides: "forge::VcoCore and its VcoCore.hpp:597-602 negated-comparison morph/character guard — the idiom Guard A mirrors on the callee side"
provides:
  - "Guard A — MorphBlep::step conditions morph and character into [0,1] with the negated comparison first, before any weight algebra or float-to-int cast"
  - "Guard B — a lower clamp on `segment` beside the existing upper one, so W[segment] and W[segment+1] are in bounds even if Guard A is later moved"
  - "Guard C — a jump finiteness clause on addStep's entry gate, written as `!(jump - jump == 0.f)`, which rejects non-finiteness without bounding magnitude"
  - "addStep's documented contract now carries the finiteness clause, so the advertised contract and the enforced contract agree"
  - "Three permanent regression TEST_CASEs — (D-04 / CR-01), (D-04 / CR-02), (D-04 third item) — each carrying its measured RED"
  - "The operator-scheduled prerequisite of deferred item 27 (decided 2026-08-27) is CLOSED: later plans of Phase 33 may now add the second MorphBlep call site"
affects: [33-05, 33-06, 33-07, hard-sync-seam, addPastStep, phase-34-drift]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "The closed-form finite predicate `x - x == 0.f` — exact, no <cmath>, no magnitude bound, verified across clang -O0/-O2/-O3 at both -std=c++11 and -std=c++17"
    - "Parameter reassignment in place rather than shadowing locals, so no name is left holding the unconditioned value"
    - "Revert-one-only probes as the evidence that each part of a multi-part fix is load-bearing, with a PAIRED probe when a defence-in-depth layer is green alone by construction"

key-files:
  created: []
  modified:
    - src/dsp/MorphBlep.hpp
    - tests/test_morph_blep.cpp

key-decisions:
  - "CONDITION, do not early-return, for a hostile morph/character — measured bit-identity with the one shipped caller is what preserves every recorded Phase 32 figure"
  - "The jump gate is `!(jump - jump == 0.f)`, NOT a magnitude pair — a bound on largeness would silently discard legitimate corrections at high character"
  - "Guard A reassigns its by-value parameters in place; C++ forbids shadowing them, and new names would leave correctness depending on every downstream expression having been switched over"
  - "Guard B's revert-one-only probe is GREEN by construction, and that is reported rather than resolved by re-typing an assertion; a paired A+B probe is what distinguishes it from dead code"
  - "The ASan reproduction stays a scoped one-shot probe outside the repository — no sanitizer target in Makefile, GUARD_SCRIPTS, TEST_CXXFLAGS or CI (register item 12)"

patterns-established:
  - "Pattern 1: every guard's comment carries the measured RED that found it, the test case that found it, and its own revert-one-only signature"
  - "Pattern 2: a borrowed narrative that measurement falsifies is corrected in place at both the header and the test, never silently inherited"
  - "Pattern 3: a hostile-parameter case must sweep at least one dt on which its property is falsifiable — D-03's exact zero can make a single-dt case vacuous while the defect is live"

requirements-completed: []  # SYNC-02 is in this plan's frontmatter but is NOT delivered here — see Deviations #5. Nine plans in this phase contribute to it; the last one marks it complete.

coverage:
  - id: D1
    description: "A negative or not-a-number morph cannot index W outside the float[5] — CR-01 closed by Guard A (unreachable) plus Guard B (safe regardless)"
    requirement: "SYNC-02"  # contributes to; not completed by this plan
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: (D-04 / CR-01) a negative or not-a-number morph cannot index W outside the float[5]"
        status: pass
      - kind: other
        ref: "one-shot ASan probe: clang++ -std=c++17 -O0 -g -fsanitize=address -fno-omit-frame-pointer -Isrc (run outside the repository)"
        status: pass
    human_judgment: false
  - id: D2
    description: "A non-finite character produces no non-finite correction, including at the three sites whose width is a literal 0.f — CR-02 closed by Guard A"
    requirement: "SYNC-02"  # contributes to; not completed by this plan
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: (D-04 / CR-02) a non-finite character produces no non-finite correction at the literal-zero-width sites"
        status: pass
    human_judgment: false
  - id: D3
    description: "A non-finite jump is rejected by addStep at the entry gate and leaves inject/pending exactly 0.0f; a finite jump of any magnitude is still accepted"
    requirement: "SYNC-02"  # contributes to; not completed by this plan
    verification:
      - kind: unit
        ref: "tests/test_morph_blep.cpp#morph blep: (D-04 third item) a non-finite jump is rejected by addStep and the instance recovers"
        status: pass
    human_judgment: false
  - id: D4
    description: "Every finite input produces bit-identical output to the pre-fix header — no recorded measurement in the suite moves"
    verification:
      - kind: other
        ref: "4096-sample forge::VcoCore block at 44.1 kHz / morph 0.75 / character 0.5, pre-guard vs post-guard binary, byte-compared (SHA-256 df432a36...)"
        status: pass
      - kind: unit
        ref: "./build-test/test -tc=\"*golden*\" (9 cases / 49,188 assertions, six shipped-LFO goldens byte-identical)"
        status: pass
    human_judgment: false
  - id: D5
    description: "Each guard is proved individually load-bearing by its own distinct revert-one-only red"
    verification:
      - kind: other
        ref: "three revert-one-only probes plus one paired A+B probe, signatures recorded in this SUMMARY and in each guard's comment"
        status: pass
    human_judgment: false

# Metrics
duration: 62min
completed: 2026-08-29
status: complete
---

# Phase 33 Plan 01: MorphBlep Hostile-Parameter Guards Summary

**`src/dsp/MorphBlep.hpp` now defends `morph`, `character` and `jump` as well as it already defended `dt` — three negated-comparison guards, each carrying its own measured RED and its own proof of biting, and every finite-input measurement in the suite unmoved.**

## Performance

- **Duration:** 62 min
- **Started:** 2026-08-29T13:31:00Z
- **Completed:** 2026-08-29T14:33:00Z
- **Tasks:** 3 of 3
- **Files modified:** 2

## Accomplishments

- **Closed the operator-scheduled prerequisite** (deferred item 27, decided 2026-08-27). `32-REVIEW.md` CR-01 and CR-02 plus the Phase 33 discussion's third defect are all fixed, so no later plan in this phase is blocked from adding the second `MorphBlep` call site.
- **Made the advertised contract the enforced contract.** The header's banner claimed caller-independence in capitals while checking only `dt`. It now checks all four hostile parameters, and `addStep`'s documented contract paragraph carries the finiteness clause it was missing.
- **Reproduced all three defects RED against the unmodified header first**, including a one-shot AddressSanitizer stack-buffer-underflow for CR-01, with no sanitizer target added anywhere in the repository.
- **Proved bit-identity by measurement, not assertion.** 4096 samples through `forge::VcoCore` against the pre-guard binary: 0 differing samples, identical SHA-256.
- **Falsified and corrected two inherited premises in place** rather than restating them — the CR-02 count and the "permanent poisoning" narrative (see Decisions Made).

## Task Commits

1. **Task 1: RED — reproduce all three defects before touching the header (D-04)** — `642af20` (test)
2. **Task 2: GREEN — the three guards, each carrying the RED that found it (D-04)** — `4213a0e` (fix)
3. **Task 3: Prove each guard bites on its own, and re-pin the seam equivalence** — `dd56013` (test)

## Files Created/Modified

- `src/dsp/MorphBlep.hpp` — Guard A (entry conditioning of `morph`/`character`), Guard B (lower clamp on `segment`), Guard C (`jump` finiteness clause on `addStep`'s entry gate), plus the finiteness clause added to `addStep`'s contract banner. **Four code lines, one modified line, one added line — the rest is the comment density this project holds a two-line guard to.**
- `tests/test_morph_blep.cpp` — three new `TEST_CASE`s (cases 7, 8, 9), three new anonymous-namespace helpers (`walkAgainstReference`, `walkFiniteness`, `cr02CharacterPoint`) and the shared D-04 banner. Case five parts A and B are **unchanged**.

## The Measured RED (Task 1, against the unmodified header)

The `(D-04 / CR-01)` case's subcase C **SIGSEGVs** against the unmodified header, and doctest's crash handler terminates the whole run at that point. The RED was therefore observed in four separate runs with subcase filters, and that is itself part of the evidence.

| Run | Selector | Result |
|-----|----------|--------|
| 1 | `(D-04 / CR-01)` subcase A | 12 assertions, **3 failed**. `differing` = 13 (morph −0.25, dt 0.0949), 2 (morph −1, dt 1/44100), 24 (morph −1, dt 0.0949) |
| 2 | `(D-04 / CR-01)` subcase B | 6 assertions, **4 failed**. `nonFinite`/`differing` = 4 at dt 1/44100 and 26 at dt 0.0949. `(int)(NaN * 4.f)` measures **0** on this arm64 host |
| 3 | `(D-04 / CR-01)` subcase C | **SIGSEGV**, 0 assertions reached |
| 4 | `(D-04 / CR-02)` | 3 assertions, **1 failed**. `nonFinitePoints` = **11 of 200** (16 hostile members, 1747 legitimate fires) |
| 5 | `(D-04 third item)` | 8 assertions, **7 failed**. Rejection subcase 6/6; recovery subcase `nonFinite` = **1 of 20**, `firstBadSample` = 0 |

Selector non-vacuity confirmed independently of exit status: `./build-test/test -tc="morph blep: (D-04*" --list-test-cases` reports **3** unskipped test cases passing the filter.

### The CR-02 count is 11, not the register's 16 — and why

The whole **−infinity class** (5 of the 16 hostile members) is benign, by accident of the frozen threshold this header replicates at `MorphBlep.hpp:317`:

```cpp
const float c = (character < 0.001f) ? 0.f : character * character;
```

`-inf < 0.001f` is **true**, so a minus-infinity character takes the early branch and becomes exactly the `c = 0.f` a legitimate zero character produces. The two classes that do reach the defect are the not-a-number six and the plus-infinity five. The −infinity members stay in the population as a **control**: they must be finite before the guard and after it.

### The one-shot ASan report, verbatim

Compiled and run **outside the repository** with `clang++ -std=c++17 -O0 -g -fsanitize=address -fno-omit-frame-pointer -Isrc`:

```
==34441==ERROR: AddressSanitizer: stack-buffer-underflow on address 0x00016d3c627c at pc 0x000102a39760 bp 0x00016d3c6250 sp 0x00016d3c6248
READ of size 4 at 0x00016d3c627c thread T0
    #0 0x102a3975c in forge::MorphBlep::step(forge::Waveshape const&, double, float, double, float, float) MorphBlep.hpp:332
    #1 0x102a39024 in main asan_cr01.cpp:13
    #2 0x190a23150  (<unknown module>)

Address 0x00016d3c627c is located in stack of thread T0 at offset 28 in frame
    #0 0x102a3917c in forge::MorphBlep::step(forge::Waveshape const&, double, float, double, float, float) MorphBlep.hpp:273

  This frame has 5 object(s):
    [32, 52) 'W' (line 325) <== Memory access at offset 28 underflows this variable
```

`MorphBlep.hpp:332` is `W[segment] += 1.f - frac;`. ASan reports **READ** of size 4 rather than WRITE because `+=` is a read-modify-write and the read half trips first; the write to the same address follows. Line 325 is the `float W[5]` declaration.

**No repository artifact was left:** `git status --porcelain -- Makefile .github tests/check_*.sh` is empty, and no ASan target exists in `Makefile`, `GUARD_SCRIPTS`, `TEST_CXXFLAGS` or `.github/workflows/`.

## Revert-One-Only Signatures (Task 3)

Each guard was reverted **alone** in the working tree, `make test` run, the guard restored, and the suite re-confirmed green before the next probe. `git status --porcelain src/dsp/MorphBlep.hpp` was empty after every restore.

| Probe | Cases red | Assertions red | Signature |
|-------|-----------|----------------|-----------|
| **Guard A alone** | 2 | **11** | `(D-04 / CR-01)` 10 — four in the negative-morph subcase (`differing` 2, 26, 2, 26), four in the not-a-number subcase (`nonFinite`/`differing` 4 and 26), two in the saturating-cast subcase; `(D-04 / CR-02)` 1 — `nonFinitePoints == 11`. `(D-04 third item)` green. **No SIGSEGV** — Guard B absorbs the saturating cast. |
| **Guard B alone** | 0 | **0** | 97 cases / 2,622,378 assertions, SUCCESS. Green **by construction** — see below. |
| **Guard C alone** | 1 | **7** | `(D-04 third item)` only — six exact-equality accumulator checks in the `rejection` subcase (`inject`/`pending` reported as `inf`, `-inf`, `nan`) plus `CHECK( nonFinite == 0 )` at `1 == 0` in the `recovery` subcase. CR-01 and CR-02 both green. |
| **Guards A + B paired** | — | — | `(D-04 / CR-01)` subcase C **SIGSEGVs**, and the ASan probe reproduces the stack-buffer-underflow at `MorphBlep.hpp:509`. |

The two signatures that are red (A and C) are **different from each other and share no failing assertion**.

### Guard B's probe is green — reported, not resolved

The plan required three different reds and instructed that a non-biting guard **be reported rather than resolved by re-typing an assertion**. Guard B's individual probe does not go red, and the reason is structural: Guard A conditions `morph` into `[0,1]`, so `scaled = morph * 4.f` lies in `[0,4]` and `(int)scaled` can never be negative. Guard B **cannot** bite alone.

Two paired measurements show it is not dead code:

1. With **Guard A alone removed and Guard B present**, the saturating-cast subcase does **not** crash — it reports two ordinary value differences. Guard B is what absorbs `(int)(-1e30f * 4.f)` saturating to `INT_MIN`.
2. With **Guard A and Guard B both removed**, that same subcase SIGSEGVs and ASan reproduces the underflow.

Guard B is the layer that converts "Guard A was edited" from a memory-safety failure into an ordinary test failure. That is precisely what defence in depth buys, and the honest statement is written into its comment block.

## Bit-Identity, Measured Not Asserted

A 4096-sample block was driven through `forge::VcoCore` at 44.1 kHz, morph 0.75, character 0.5, compiled with the same flags `make test` uses (`-std=c++17 -O2 -ffp-contract=off`), captured **before** any header edit and again **after** the plan completed:

| Capture | SHA-256 | Differing samples |
|---------|---------|-------------------|
| pre-guard | `df432a36caeb9b908d69b033f59fd42d186005f6e4a8af58161b75d8c968029b` | — |
| post-plan | `df432a36caeb9b908d69b033f59fd42d186005f6e4a8af58161b75d8c968029b` | **0 of 4096** (direct byte comparison) |

## Suite Totals, Before and After

Recorded as a pair so later plans in this phase can account for their own deltas.

| | Test cases | Assertions |
|---|---|---|
| Before plan 33-01 | 94 | 2,622,319 |
| After plan 33-01 | **97** | **2,622,378** |
| Delta | +3 | +59 |

## Gate Results

| Gate | Result |
|------|--------|
| `make test` | **PASS** — 97 cases, 2,622,378 assertions, 0 failures |
| `make strict` (C++11, `-pedantic-errors`) | **PASS**, exit 0 |
| `make guards` | **PASS**, exit 0 |
| `bash tests/check_canary.sh` | **PASS**, exit 0 |
| `bash tests/check_frozen.sh` | **PASS**, exit 0 — **15 pinned entries** unchanged, `FROZEN.sha256` byte-identical |
| `git diff --stat` on the four frozen shared headers | **empty** — `Waveshape.hpp`, `DriftEngine.hpp`, `RackCompat.hpp`, `MathConst.hpp` all untouched |
| `src/AnalogLFO.cpp` in the diff | **absent** |
| Six shipped-LFO goldens | **byte-identical** — 9 cases / 49,188 assertions, 0 failures |
| `git diff -U0 src/dsp/MorphBlep.hpp \| grep -c '^+#include'` | **0** — no new include acquired |
| Non-comment `clamp(` count in `MorphBlep.hpp` | **0** before, **0** after (the helper is named only in comments, where it is forbidden by name) |
| `-tc="morph blep: (D-04*"` | 3 cases, 3 passed, 0 failed |
| `-tc="*seam*"` | 3 cases matched, 66 assertions, 0 failed |

## Decisions Made

1. **Condition, do not early-return, for a hostile `morph`/`character`.** An early return would be cheaper but would *change the correction* on a sample the one shipped caller has already conditioned to exactly these values two lines above the call — and every recorded magnitude, alias threshold and envelope figure in the Phase 32 suite is measured through that caller. Conditioning makes the header's answer identical to the live signal path's, which is why bit-identity holds.

2. **The `jump` gate is `!(jump - jump == 0.f)`, not a magnitude pair.** `x - x` is exactly `0` for every finite float (including ±`FLT_MAX`, subnormals and both zeros) and is a not-a-number for `+inf`, `-inf` and any NaN — one expression covering all three classes, closed-form, no new include, written negated. The obvious alternative `!(jump > -3.4e38f) || !(jump < 3.4e38f)` is a **bound on largeness** that would silently discard legitimate corrections; naming an infinity as a literal (`1e40f`) is ill-formed under `-pedantic-errors`, which `make strict` runs. Verified across `clang -O0/-O2/-O3` at both `-std=c++11` and `-std=c++17`: seven finite classes accepted, three non-finite classes rejected, at all six settings, no `-Wall -Wextra` warning. **The idiom is sound only under IEEE semantics** — `-ffast-math` would fold it to `true`. This project forbids `-ffast-math` and pins `-ffp-contract=off`; the constraint is written into the guard's comment.

3. **Guard A reassigns its by-value parameters in place rather than shadowing them with new locals.** The plan asked for "conditioned locals", but C++ forbids redeclaring a parameter name in the function's top-level scope, so new names would have to be *different* names — and correctness would then depend on every one of the twenty-odd downstream expressions having been switched over, with one missed line silently reopening the defect. Reassignment leaves no name holding the raw value.

4. **Guard B stays despite a green individual probe**, with the green result and its two paired measurements written into its comment block.

5. **The ASan reproduction stays a scoped one-shot probe.** Register item 12 forbids a permanent repo-wide sanitizer gate because the shipped Analog LFO carries shared latent undefined behavior that is deliberately unowned.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] The CR-01 case was vacuous at the plan's single specified `dt`**

- **Found during:** Task 1
- **Issue:** The plan specified a single legitimate `dt` of `1.0/44100.0`. Measured against the unmodified header at that increment with `morph = -0.25`, the hostile and reference streams are **bit-identical on all 44,108 samples** while the header is writing one float before the array. D-03's exact zero kills every soft-edge site at that increment (`wSq = wPl = 0.04` against `2*dt = 4.5e-5`), leaving only the three literal-zero-width sites — whose magnitudes coincide, because `mag[0]` sums `hardSq + hardPl` and the square's hard step at `0.5f` sits at the same phase as the pulse's at `pulseDuty = 0.5`. The case's value assertions were therefore **unfalsifiable** — exactly the "green for free" class this file's banner is written against.
- **Fix:** Swept a second increment, `0.0949` (C8 at 44.1 kHz, already one of `FACTOR_DTS`), where `2*dt = 0.1898` exceeds the soft widths, the soft sites go live, and the square's soft edge at `dutySq = 0.51` no longer coincides with the pulse's at `0.50`. The finding and its measurement are recorded in the case banner so the sweep is not "simplified" back to one increment.
- **Files modified:** `tests/test_morph_blep.cpp`
- **Verification:** The Guard A revert-one-only probe now reports `differing` = 26 at `0.0949` where it reports 2 at `1/44100`, and morph −0.25 at `1/44100` reports 0 — the vacuity is visible in the probe output itself.
- **Committed in:** `642af20`

**2. [Rule 2 — Missing coverage] The plan's two named morphs never exercise the `W[3]` site they were chosen for**

- **Found during:** Task 1
- **Issue:** The plan named morph `0.75f` as "the square centre where `W[3]` is live". It is not. At morph 0.75 `scaled` is exactly 3.0, so `segment` is 3 and the frozen direct-duty special case (`MorphBlep.hpp:326-330`, `Waveshape.hpp:179-182`) puts **all** the weight on `W[4]`. Both named morphs (0.75 and 1.00) therefore exercise `hardPl` twice and `hardSq` — the `W[3]` literal-zero-width site CR-02 is about — never.
- **Fix:** Swept a third morph of `0.70f` alongside the two named ones. `scaled` 2.8 gives `segment` 2, `frac` 0.8 and `W[3] = 0.8` genuinely live. The falsified premise is corrected in place in the case banner.
- **Files modified:** `tests/test_morph_blep.cpp`
- **Verification:** `(D-04 / CR-02)` passes at all three morphs post-guard and reports `nonFinitePoints == 11` pre-guard.
- **Committed in:** `642af20`

**3. [Rule 1 — Bug] The "permanent poisoning" narrative for the `jump` defect is false**

- **Found during:** Task 1
- **Issue:** The plan, and deferred item 27's neighbours, described the `jump` defect as *"the identical permanent-poison mode plan 32-05 measured for a `+infinity` `dt`"* — one bad sample and the instance returns not-a-number forever. **Measured against the unmodified header:** after `addStep(0.5f, +infinity)`, exactly **one** of the next twenty `step()` calls returns a non-finite value — the first — and `pending` is finite after every one of the twenty. `step()`'s preamble drains `inject` and `pending` and zeroes both *unconditionally*, so an accumulator poisoned from outside is flushed on the next sample. The `dt` defect was permanent because it re-poisoned `pending` from *inside* the site loop, downstream of that drain.
- **Fix:** Recorded the corrected figure and the structural reason in three places — the case banner, the recovery subcase, and Guard C's comment block — and restated *why the guard is still required* on the corrected evidence: `forge::VcoCore` adds this correction to the naive sample, so one poisoned event puts a not-a-number on the module's output; and D-05's computed `morphedWave` difference turns "one sample" into "every sample" the moment the upstream value goes bad.
- **Files modified:** `tests/test_morph_blep.cpp`, `src/dsp/MorphBlep.hpp`
- **Verification:** `nonFinite == 1`, `firstBadSample == 0`, `nonFinitePending == 0` measured pre-guard; all zero post-guard.
- **Committed in:** `642af20`, `4213a0e`

**4. [Rule 3 — Blocking] The plan's assertion-count acceptance criterion cannot be met by one run**

- **Found during:** Task 1
- **Issue:** The acceptance criterion asked for a single `-tc="morph blep: (D-04*"` run reporting a non-zero failure count. Subcase C **SIGSEGVs** against the unmodified header, and doctest's crash handler terminates the whole run there, so cases 8 and 9 never execute and only 1 test case is reported.
- **Fix:** Observed the RED in **four filtered runs** (subcase filters on case 7, then cases 8 and 9 separately) and recorded all five signatures. The selector's matched-case count was confirmed independently with `--list-test-cases`, which reports **3** — satisfying the criterion's actual intent (a selector matching zero cases also exits 0 and prints SUCCESS).
- **Files modified:** none — a measurement-procedure change only
- **Verification:** `--list-test-cases` output quoted above; post-guard the single-run form works and reports 3 cases / 59 assertions / 0 failures.
- **Committed in:** n/a (procedure, recorded here and in `642af20`'s message)

**5. [Rule 1 — Bug] Marking SYNC-02 complete after this plan is a false-green signal**

- **Found during:** state updates, after Task 3
- **Issue:** This plan's frontmatter carries `requirements: [SYNC-02]`, so the state-update step marked SYNC-02 complete in `.planning/REQUIREMENTS.md` — both its checkbox and its traceability row. **SYNC-02 is not delivered.** It reads *"Sync reset uses sub-sample fractional placement plus a sync-BLEP (click-free), reusing the anti-aliasing machinery"*, and this plan implemented **no sync behavior at all** — it hardened the machinery SYNC-02 will reuse. **Nine** of this phase's twelve plans carry SYNC-02 in their frontmatter (33-01, 33-02, 33-05, 33-06, 33-07, 33-08, 33-09, 33-10, 33-11, 33-12); marking it complete at the first of them would tell the audit-open scanner, the milestone audit and the operator that hard sync ships, eight plans early.
- **Fix:** Reverted both edits — SYNC-02 is back to `[ ]` and `Pending`. It should be marked complete by the last contributing plan (33-12, or 33-11 at the phase gate), not the first.
- **Files modified:** `.planning/REQUIREMENTS.md` (reverted to its pre-plan state; `git diff` on it is empty)
- **Verification:** `git diff .planning/REQUIREMENTS.md` produces no output.
- **Committed in:** n/a — the file is unchanged from its pre-plan state

**Standing note for the rest of Phase 33:** the same auto-mark will fire on every one of the eight remaining SYNC-02 plans. Each should apply the same judgement, and only the final contributing plan should let it stand.

---

**Total deviations:** 5 auto-fixed (3 × Rule 1, 1 × Rule 2, 1 × Rule 3)
**Impact on plan:** All four were necessary for the plan's own stated goals. Two closed measured vacuities that would have shipped test cases incapable of failing; one corrected an inherited premise the project's own conventions require correcting in place; one adapted a measurement procedure to a crash the plan itself predicted. **No scope creep** — the diff is still exactly `src/dsp/MorphBlep.hpp` and `tests/test_morph_blep.cpp`.

## Issues Encountered

- **No real GCC available locally.** `g++` on this host is Apple clang. `make strict` runs the C++11 `-pedantic-errors` gate under clang and passes, but per the Phase 29 P-2 lesson, `-fsyntax-only` never links and cannot catch a link-class defect. Guard C's `jump - jump` idiom was verified across both standards and three optimization levels under clang; **T-33-08 remains open until the CI MinGW compile-and-link leg is observed green on this commit**, which is plan 33-11's job. Nothing here is tagged or submitted on local evidence alone.
- **`.planning/STATE.md` was modified by the GSD harness at execution start**, before any task ran. It is not part of this plan's source diff and is carried by the plan metadata commit.
- **Two untracked `.planning/research/.cache/*.json` files pre-existed** at session start and were left alone.

## Next Phase Readiness

**The scheduled prerequisite is closed.** `src/dsp/MorphBlep.hpp` now defends `morph`, `character` and `jump` as well as it defends `dt`, so:

- Plan 33-05 may add the second `MorphBlep` call site and, if its measurement selects candidate (b), add `addPastStep` — which should **mirror Guard C's entry gate exactly**, including the `jump` finiteness clause.
- Plan 33-06's D-05 sync seam may feed `jump` a computed difference of two `morphedWave` values. That is the first caller in the project's history that can put a non-finite value on this path, and it is now gated.
- The pinned seam equivalence in case five parts A and B is **unmoved and was re-run rather than re-derived**: `emitted` 0.25, `site.pending` −0.25, `seam.inject` 0.25, `seam.pending` −0.25 — the same `+0.250000 / −0.250000` split — with the summation-composition discriminators still firing (`injectAfterFirst` 0.5625, `pendingAfterFirst` −0.0625).

**Concerns carried forward:**

- **T-33-08 (toolchain divergence) is not discharged locally.** `(int)(NaN * 4.f)` measures 0 on this arm64 host and is `INT_MIN` under x86 `cvttss2si`. The guards close the defect on both, but the *proof* needs the CI MinGW link leg on this exact commit.
- **Guard C's idiom depends on IEEE semantics.** If `-ffast-math` is ever introduced, or `-ffp-contract=off` dropped, Guard C must be re-verified. The constraint is written into the guard's comment.
- **Register item 12 still stands:** no sanitizer target may be added to `Makefile`, `GUARD_SCRIPTS`, `TEST_CXXFLAGS` or CI while the shipped LFO's latent UB is unowned.

## Self-Check: PASSED

Verified against disk and git rather than asserted:

- **Files exist:** `src/dsp/MorphBlep.hpp`, `tests/test_morph_blep.cpp`, `.planning/phases/33-hard-sync/33-01-SUMMARY.md` — all FOUND.
- **Commits exist:** `642af20`, `4213a0e`, `dd56013`, `b6227b7` — all FOUND in `git log --all`.
- **All three guards are present in `HEAD`,** confirmed by reading the committed blob rather than the working tree: `if (!(morph > 0.f)) morph = 0.f;` at `:446` (Guard A), `if (segment < 0) segment = 0;` at `:526` (Guard B), and `!(jump - jump == 0.f)` inside `addStep`'s single early return at `:329` (Guard C).
- **Three `(D-04 ...)` `TEST_CASE`s are present in `HEAD`** — `grep -c` on the committed blob returns 3.

---
*Phase: 33-hard-sync*
*Completed: 2026-08-29*
