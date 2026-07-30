---
phase: 31-pitch-tuning-exponential-fm
plan: 03
subsystem: dsp
tags: [cpp11, constexpr, pitch, exponential-fm, undefined-behavior, ubsan, nan-guard, comment-truth, hostile-input]

# Dependency graph
requires:
  - phase: 30-vcocore-skeleton-module-registration
    provides: "the single-term pitch expression, the correctly-ordered Nyquist ceiling/negated-floor pair (CR-01), the sanitised rate (WR-06), the double accumulator and its independent increment bound — every one of which this plan leaves byte-identical"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 01
    provides: "a green guard suite at its exact measured floor, so this plan's identical post-edit counts are meaningful"
  - phase: 31-pitch-tuning-exponential-fm
    plan: 02
    provides: "kVcoNyquistGuardFrac settled at 0.495f, and a banner that deliberately said the volt-domain summation was still owed — the clause this plan had to falsify and rewrite"
provides:
  - "forge::kVcoMaxPitchVolts = 64.f — the D-14 hostile-input bound on the exp2 argument, with its measured envelope, its measured reachable musical worst case and its finite-at-both-extremes rationale in the source"
  - "the four-term volt-domain pitch summation: in.pitchCV + in.coarse + in.fine/12, plus an fmConnected-GATED in.fmVolts * in.fmAtten, through EXACTLY ONE forge::exp2_taylor5 call"
  - "the negated-comparison NaN-rejecting bound between the summation and the exponential — forge::clamp rejected by name in the source"
  - "VcoInputs::coarse, ::fine, ::fmVolts, ::fmAtten and ::fmConnected change from declared-and-unread to READ (canary [2b/5] now reports all 8 DSP fields runtime-live at -O3)"
  - "a recorded RED-then-clean UBSan transcript pair naming both frozen UB sites by file, line and column"
  - "a banner and a kVcoFreqC4 comment that describe a DELIVERED pitch chain, plus the measured +0.0000685-cent float-representation offset the C4 reference carries"
affects: [31-04, 31-05, 31-06, 31-07, 31-08, 32-morph-blep, 33-hard-sync]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "When a behavioral RED is MEASURED vacuous, escalate the evidence tier rather than weakening the claim: a one-shot sanitizer probe over a scoped TU is the defect itself, not a proxy for it"
    - "A sanitizer probe that would light up a SHIPPED module is kept one-shot and out of the tree on purpose — the diagnostic is the evidence, the gate is a separate decision"
    - "A comment that asserts declaration ADJACENCY is falsified by inserting a constant between the two it names — adjacency claims are as perishable as arithmetic ones"
    - "Every line-number citation written into a load-bearing comment is verified by reading the cited file in the same session"

key-files:
  created: []
  modified:
    - "src/dsp/VcoCore.hpp"

key-decisions:
  - "kVcoMaxPitchVolts = 64.f, exactly as research recommended and CONTEXT.md marked discretionary — power of two (no rounding in the comparisons), 2.2x outside the reachable musical worst case, 2.0x inside the UB boundary, and finite at BOTH extremes where 120/126 hand the downstream ceiling an infinity"
  - "The bound is the negated-comparison idiom with the negated line FIRST; forge::clamp is rejected BY NAME in the source comment because both of its comparisons are false for a NaN"
  - "No depth constant for the FM term (D-06): the 1.0 octave-per-volt contract lives in the comment, not in a multiply-by-one the compiler removes"
  - "The FM gate is `if (in.fmConnected)` in the CORE, so the term is NOT EVALUATED when unpatched — not merely zero (D-09/D-17)"
  - "The RED evidence is the UBSan probe, NOT a behavioral case: research MEASURED that every behavioral assertion a reasonable person would write was ALREADY GREEN before the fix, and the probe reproduced that measurement in the same run that produced the diagnostics"
  - "No requirement marked complete — third consecutive plan making that call, for the same reason (see Deviations)"

patterns-established:
  - "A RED transcript and the measurement proving a behavioral RED would have been vacuous come from the SAME probe run, so the two cannot disagree"
  - "The GREEN re-run EXTENDS the hostile grid with the rows the fix made reachable, and keeps every RED row verbatim so the two transcripts are directly comparable"

requirements-completed: []

coverage:
  - id: D1
    description: "The four pitch terms are summed in the VOLT domain and pass through EXACTLY ONE forge::exp2_taylor5 call — no frequency multiply, no second exponential"
    requirement: "FM-03"
    verification:
      - kind: integration
        ref: "grep -v '^[[:space:]]*//' src/dsp/VcoCore.hpp | grep -c 'exp2_taylor5(' == 1; 'freq \\*=' non-comment == 0; 'std::exp2|std::pow' non-comment == 0; the four-term sum line and 'float freq = kVcoFreqC4 * exp2_taylor5(pitchVolts);' each == 1"
        status: pass
      - kind: integration
        ref: "make strict — 'strict C++11 gate: PASS' over all four TUs including src/vco_compile_canary.cpp"
        status: pass
    human_judgment: false
  - id: D2
    description: "The core performs the semitone-to-octave conversion; the POD's documented units are unchanged and no field was added"
    verification:
      - kind: integration
        ref: "grep -c 'float pitchVolts = in.pitchCV + in.coarse + in.fine \\* (1.f / 12.f);' == 1; `struct VcoInputs` and `struct Telemetry` extracted and diffed against HEAD~2 — BOTH IDENTICAL"
        status: pass
    human_judgment: false
  - id: D3
    description: "The FM term is gated on in.fmConnected in the CORE, so the unpatched path does not evaluate it at all"
    verification:
      - kind: integration
        ref: "grep -c 'if (in.fmConnected) pitchVolts += in.fmVolts \\* in.fmAtten;' == 1; grep -ci 'depthScale' == 0 (no LFO depth scaling borrowed); src/AnalogLFO.cpp absent from the diff"
        status: pass
      - kind: unit
        ref: "make test — 72 / 2,616,112 / 0, IDENTICAL to the 31-02 baseline. The existing grid holds coarse/fine/FM at zero with the jack unpatched, so the gate's inertness on the unpatched path is what keeps the counts bit-stable"
        status: pass
    human_judgment: false
  - id: D4
    description: "The summed pitch volts are bounded to [-kVcoMaxPitchVolts, +kVcoMaxPitchVolts] BEFORE the exponential using the negated-comparison idiom, and no clamp helper appears in that region"
    requirement: "FM-01"
    verification:
      - kind: integration
        ref: "negated bound line at :338 strictly BEFORE plain bound line at :339 (NaN catcher first); region-scoped `sed -n '/float pitchVolts = in.pitchCV/,/exp2_taylor5(pitchVolts)/p' | grep -v '^[[:space:]]*//' | grep -c clamp` == 0; 'std::fmax|std::fmin|std::clamp' non-comment == 0"
        status: pass
      - kind: integration
        ref: "UBSan probe over the guarded core, 24 hostile configurations: ZERO `runtime error:` lines, stderr 0 bytes — against two named diagnostics in the pre-guard run"
        status: pass
    human_judgment: false
  - id: D5
    description: "Both frozen undefined-behavior sites were OBSERVED red before the bound and OBSERVED clean after it, with the literal diagnostic text recorded"
    verification:
      - kind: integration
        ref: "clang 16 -fsanitize=undefined: RackCompat.hpp:106:24 float-cast-overflow and RackCompat.hpp:109:11 left-shift overflow, both quoted verbatim below; re-run transcript empty. Same probe run also reproduced research's measurement that every behavioral assertion was already green pre-fix"
        status: pass
    human_judgment: false
  - id: D6
    description: "Nothing downstream of the exponential moved (CR-01 order, telemetry write, both deltaPhase casts, kVcoMaxDeltaPhase, the wrap, the morph/character clamps, the Waveshape call, the unconditioned x5) and PITCH-05 is non-regressed"
    requirement: "PITCH-05"
    verification:
      - kind: integration
        ref: "the region from `const float safeRate` to the closing brace of struct VcoCore, extracted from HEAD~2 and from the working tree, diffed: BYTE-IDENTICAL. grep -c 'double deltaPhase = (double)freq \\* (double)in.sampleTime;' == 1; 'double phase = 0.0;' == 1; 'constexpr double kVcoMaxDeltaPhase = 0.5;' == 1; ceiling :406 < floor :407"
        status: pass
    human_judgment: false
  - id: D7
    description: "No probe artifact and no sanitizer wiring survives in the repository; no frozen header and no shipped-LFO file in the diff"
    verification:
      - kind: integration
        ref: "git status --porcelain --untracked-files=all clean after each run; grep -rc 'fsanitize' Makefile .github/workflows/test.yml == 0 for both; git diff --name-only == src/dsp/VcoCore.hpp alone; make guards PASS (frozen manifest + 6 LFO .f32 goldens + dependency audit + canary)"
        status: pass
    human_judgment: false
  - id: D8
    description: "Every comment in the header describes the code that is actually there — the banner and the kVcoFreqC4 comment describe a DELIVERED pitch chain"
    verification:
      - kind: integration
        ref: "grep -ci 'knowingly incomplete' == 0; grep -ci 'reads in.pitchCV alone' == 0; grep -c '261.6256103515625' == 1; grep -ci 'aliased|aliases' == 2 (Phase 32's boundary survives); grep -c 'Source-shape contract' == 1; grep -c 'R-9' == 1; Task 3 diff has no added or removed line ending in ';' or '{'"
        status: pass
    human_judgment: false

# Metrics
duration: 12min
completed: 2026-07-30
status: complete
---

# Phase 31 Plan 03: The Real Pitch Chain and the Hostile-Input Bound Summary

**The pitch became four terms summed in the volt domain through exactly ONE `forge::exp2_taylor5` call, with an `fmConnected`-gated FM contribution at 1.0 octave per volt and a NaN-rejecting `kVcoMaxPitchVolts = 64.f` bound ahead of the exponential — landed on a RED-then-clean UBSan transcript pair that named both frozen undefined-behavior sites by file, line and column, at a non-comment cost of six lines and with everything downstream of the exponential byte-identical.**

## Performance

- **Duration:** 12 min
- **Started:** 2026-07-30T01:23Z
- **Completed:** 2026-07-30T01:35Z
- **Tasks:** 3 (Task 1 changes no repository file by design, so it carries no commit of its own)
- **Files modified:** 1

## Accomplishments

- **The pitch chain stopped being one term.** `pitchVolts = in.pitchCV + in.coarse + in.fine/12`, plus a gated `in.fmVolts * in.fmAtten`, bounded, then **one** exponential off C4. That is the shape PITCH-01/02/03 and FM-01/02/03 describe, and it is what makes the FM *musical* exponential FM: a fixed modulator voltage shifts the pitch by a fixed number of semitones no matter what note is playing.
- **The RED went red on the defect itself, not a proxy for it.** Research had MEASURED that a behavioral RED here would be vacuous. Rather than write the vacuous case and call it evidence — the trap Phases 29 and 30 were each bitten by — the evidence tier was escalated to a one-shot UBSan probe, and the sanitizer named **both** frozen sites by file, line and column. The same probe run **reproduced the vacuity measurement in the same output**, so the two claims cannot disagree.
- **The clean re-run is an observed absence, quoted against the transcript it replaces.** Zero `runtime error:` lines and a **0-byte stderr** over an extended 24-configuration grid — including the FM-connected non-finite `fmVolts` and non-finite `fmAtten` rows that only became reachable when Task 2 wired the term in.
- **The negative bound behaves exactly as D-13 predicted, and the difference is visible in the transcript.** `tel.freqHz` at the negative bound moved from `0` to `1.41828e-17` — matching research's independently measured `1.418e-17` to six figures. That is not a regression: the guarded value is **positive**, so the existing negated frequency floor correctly does not fire. Before the bound, that same row read `0` only because the UB produced garbage the floor happened to catch.
- **`forge::clamp` is rejected in the source, by name, with the reason.** The obvious tool is inert against exactly the input class the guard exists to stop: both of its comparisons are false for a NaN, so a NaN passes through unchanged. The comment says so where the next editor will read it, rather than leaving it in a deferred-items file.
- **The blast radius of the change was tracked, not assumed.** Everything from `const float safeRate` to the closing brace of `struct VcoCore` was extracted from `HEAD~2` and from the working tree and **diffed**: byte-identical. `struct VcoInputs` and `struct Telemetry`: byte-identical. The non-comment delta across both code commits is **six lines**.
- **The comment-truth debt 31-02 deliberately left was paid in the same phase that created it.** 31-02's own hand-off said "31-03 must rewrite the banner again," and it did — plus one adjacency claim 31-02 could not have anticipated (below).

## Task Commits

1. **Task 1: RED — one-shot UBSan probe naming both frozen UB sites** — *no commit; this task creates and modifies no repository file by design (D-24: the probe lives outside the working tree and nothing about it becomes permanent). Its evidence is the transcript recorded below.*
2. **Task 2: The volt-domain summation, the `fmConnected` gate and the `kVcoMaxPitchVolts` bound before the single exponential** — `2643c97` (feat)
3. **Task 3: GREEN re-run, then the banner and the C4 comment brought in line with a delivered pitch chain** — `d25e672` (docs, comment-only)

## Files Created/Modified

- `src/dsp/VcoCore.hpp` — one new constant with a 55-line rationale; four new lines of arithmetic with two comment blocks; the banner; the `kVcoFreqC4` comment; two Rule-2 comment corrections

---

## Task 1 — the RED transcript, verbatim

### The exact commands

```
$ PROBE_DIR=$(mktemp -d /tmp/vco_ubsan_probe.XXXXXX)      # -> /tmp/vco_ubsan_probe.v6oIOO

$ clang++ -std=c++17 -O0 -g -fsanitize=undefined -fno-omit-frame-pointer \
    -I "/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/src" \
    -o /tmp/vco_ubsan_probe.v6oIOO/vco_ubsan_probe \
       /tmp/vco_ubsan_probe.v6oIOO/vco_ubsan_probe.cpp

$ /tmp/vco_ubsan_probe.v6oIOO/vco_ubsan_probe \
    > /tmp/vco_ubsan_probe.v6oIOO/red.stdout \
    2> /tmp/vco_ubsan_probe.v6oIOO/red.stderr
```

Compiler: `Apple clang version 16.0.0 (clang-1600.0.26.6)`, target `arm64-apple-darwin23.6.0`.
The translation unit was self-contained (`<cmath>`, `<cstdio>`, `<limits>` and `dsp/VcoCore.hpp`
only — **no doctest, nothing from `tests/`**), and it lived entirely outside the repository
working tree.

### The sanitizer diagnostics — the RED evidence D-22 requires

Both frozen sites, verbatim and unedited, complete with file, line, column and message:

```
/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/src/dsp/RackCompat.hpp:106:24: runtime error: nan is outside the range of representable values of type 'int'
SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior /Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/src/dsp/RackCompat.hpp:106:24 in 
/Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/src/dsp/RackCompat.hpp:109:11: runtime error: left shift of 2147483647 by 23 places cannot be represented in type 'int32_t' (aka 'int')
SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior /Users/mrcbrown/Claude/Software/Forge Audio/Analog Series/src/dsp/RackCompat.hpp:109:11 in 
```

Those are the two sites named in the plan's threat register, and they are exactly the two lines
in the frozen header:

- **`RackCompat.hpp:106`** — `int32_t xi = (int32_t)x;`  (T-31-01, the float-to-int cast)
- **`RackCompat.hpp:109`** — `yii = xi << 23;`  (T-31-02, the left shift)

Both line numbers were confirmed by reading `src/dsp/RackCompat.hpp:104-111` directly.

**The process exit status is not the signal.** UBSan **recovers** by default, so the probe exited
`0` in the RED run just as it did in the GREEN run. The presence — or absence — of the diagnostic
text is the evidence. A plan that gated on `$?` here would have seen green both times.

### The behavioral table from the SAME run — and why a behavioral RED was rejected

```
| configuration            | fm  |       fmVolts |    tel.freqHz |  max|out| | allFinite |        phase |
|--------------------------|-----|---------------|---------------|-----------|-----------|--------------|
| pitchCV = 0 (control)    | no  |             0 |       261.626 |    4.9998 |      true |     0.373021 |
| pitchCV = quiet NaN      | no  |             0 |             0 |         5 |      true |            0 |
| pitchCV = +inf           | no  |             0 |             0 |         5 |      true |            0 |
| pitchCV = -inf           | no  |             0 |             0 |         5 |      true |            0 |
| pitchCV = +1e30          | no  |             0 |             0 |         5 |      true |            0 |
| pitchCV = -1e30          | no  |             0 |             0 |         5 |      true |            0 |
| pitchCV = +200           | no  |             0 |             0 |         5 |      true |            0 |
| pitchCV = -200           | no  |             0 |             0 |         5 |      true |            0 |
| pitchCV = +130           | no  |             0 |             0 |         5 |      true |            0 |
| pitchCV = -130           | no  |             0 |             0 |         5 |      true |            0 |
| fm NaN, atten 1.0        | yes |           nan |       261.626 |    4.9998 |      true |     0.373021 |
| fm +inf, atten 1.0       | yes |           inf |       261.626 |    4.9998 |      true |     0.373021 |
| fm +1e30, atten 1.0      | yes |         1e+30 |       261.626 |    4.9998 |      true |     0.373021 |
| fm +200, atten 1.0       | yes |           200 |       261.626 |    4.9998 |      true |     0.373021 |
| fm 5 V, atten NaN        | yes |             5 |       261.626 |    4.9998 |      true |     0.373021 |
```

400 `step(...)` calls per configuration at a legitimate 44.1 kHz timing pair, `morph = 0.5`,
`character = 1.0`, seeded `0xC0FFEE / 0xBADF00D` — the proven non-degenerate literals used
everywhere else in this repository (`tests/test_vco_core.cpp:708/950/1064`). **Never a pair of
zeros:** a degenerate Xoroshiro seed is a fixed point emitting an all-zero stream, which makes the
rejection loop inside `std::normal_distribution` never terminate — in Rack that is a hang on patch
load, not a test failure (T-31-08).

**This table is why a behavioral RED case was rejected as the evidence tier, and it is stated
plainly:** for every non-finite and out-of-range input, **every sample was finite**, the telemetry
frequency was **`0`**, and the maximum absolute output was **at or below 5 V**. Every behavioral
assertion a reasonable person would have written as the D-14 RED — `allFinite`, `tel.freqHz == 0`,
`|out| <= 5` — was **already GREEN before the fix**. The existing negated frequency floor catches
the *garbage result* of the undefined behavior, so a behavioral case cannot see the defect at all.
Landing the guard on a case that never went red is the vacuous-coverage trap that bit Phase 29 and
Phase 30, and this project has twice rejected a fix whose failing case was never observed. The
evidence tier was escalated instead of the claim weakened.

**The five FM rows are recorded for what they do NOT show.** They carry hostile `fmVolts` with the
jack connected, yet `tel.freqHz` reads `261.626` — identical to the control. That is the direct
evidence that at this commit `step(...)` did not read those fields at all: the FM route was **not
yet reachable**. Task 2 made it reachable, which is precisely why the GREEN grid below had to be
extended rather than merely repeated.

### Nothing survived

```
$ git status --porcelain --untracked-files=all
(empty)
$ git status --porcelain --untracked-files=all | grep -c 'ubsan\|sanitize'
0
$ git diff --name-only
(empty)
$ grep -rc 'fsanitize' Makefile .github/workflows/
Makefile:0
.github/workflows/test.yml:0
```

The scratch directory was deleted. No file was added under `tests/` or `tools/`, no `make` target
was added, no CI step was added, and `-fsanitize=undefined` was wired into nothing. **D-24:** the
SHIPPED Analog LFO reaches the identical latent UB through its own FM path
(`src/AnalogLFO.cpp:320` feeds an unsanitized `inputs[FM_INPUT].getVoltage()` into
`src/dsp/LfoCore.hpp:183-184`, both **verified by reading them this session**), so a permanent
repo-wide sanitizer gate would turn the live module red and convert this phase into an unplanned
guardrail event.

`make test` after Task 1: **72 / 2,616,112 / 0** — unchanged, as it must be for a task that
touches no repository file.

---

## Task 3 — the GREEN transcript, directly beneath the RED

### The diagnostics: an observed absence

```
$ /tmp/vco_ubsan_green.g1JNBj/vco_ubsan_probe \
    > /tmp/vco_ubsan_green.g1JNBj/green.stdout \
    2> /tmp/vco_ubsan_green.g1JNBj/green.stderr
$ cat /tmp/vco_ubsan_green.g1JNBj/green.stderr
$ wc -c < /tmp/vco_ubsan_green.g1JNBj/green.stderr
0
$ grep -c 'runtime error:' /tmp/vco_ubsan_green.g1JNBj/green.stderr
0
```

**Zero `runtime error:` lines. stderr is 0 bytes.** Same compiler, same flags, same probe source
plus nine additional rows, same `-I` — the only variable is the guard that landed in `2643c97`.
Against the two diagnostics quoted immediately above, that is the RED-then-clean pair D-22
requires.

### The extended grid

```
| configuration            | fm  |       fmVolts |    tel.freqHz |  max|out| | allFinite |        phase |
|--------------------------|-----|---------------|---------------|-----------|-----------|--------------|
| pitchCV = 0 (control)    | no  |             0 |       261.626 |    4.9998 |      true |     0.373021 |
| pitchCV = quiet NaN      | no  |             0 |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| pitchCV = +inf           | no  |             0 |       21829.5 |   4.99883 |      true |     0.999996 |
| pitchCV = -inf           | no  |             0 |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| pitchCV = +1e30          | no  |             0 |       21829.5 |   4.99883 |      true |     0.999996 |
| pitchCV = -1e30          | no  |             0 |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| pitchCV = +200           | no  |             0 |       21829.5 |   4.99883 |      true |     0.999996 |
| pitchCV = -200           | no  |             0 |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| pitchCV = +130           | no  |             0 |       21829.5 |   4.99883 |      true |     0.999996 |
| pitchCV = -130           | no  |             0 |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| fm NaN, atten 1.0        | yes |           nan |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| fm +inf, atten 1.0       | yes |           inf |       21829.5 |   4.99883 |      true |     0.999996 |
| fm +1e30, atten 1.0      | yes |         1e+30 |       21829.5 |   4.99883 |      true |     0.999996 |
| fm +200, atten 1.0       | yes |           200 |       21829.5 |   4.99883 |      true |     0.999996 |
| fm 5 V, atten NaN        | yes |             5 |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| fm -inf, atten 1.0       | yes |          -inf |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| fm -1e30, atten 1.0      | yes |        -1e+30 |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| fm -200, atten 1.0       | yes |          -200 |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| fm +130, atten 1.0       | yes |           130 |       21829.5 |   4.99883 |      true |     0.999996 |
| fm 5 V, atten +inf       | yes |             5 |       21829.5 |   4.99883 |      true |     0.999996 |
| fm 5 V, atten -inf       | yes |             5 |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| fm 12 V, atten 1.0       | yes |            12 |       21829.5 |   4.99883 |      true |     0.999996 |
| pitch NaN + fm NaN       | yes |           nan |   1.41828e-17 |         5 |      true |  1.28642e-19 |
| pitch +12, fm +12 @1.0   | yes |            12 |       21829.5 |   4.99883 |      true |     0.999996 |
```

Every RED row is present verbatim so the two transcripts are directly comparable. The nine
appended rows are the ones Task 2 made reachable, and they cover both configurations the plan
required: **the FM jack connected with a non-finite `fmVolts`** (`fm NaN`, `fm ±inf`, `fm ±1e30`,
`pitch NaN + fm NaN`) and **a non-finite `fmAtten`** (`atten NaN` from the RED grid, plus
`atten +inf` and `atten -inf`).

**The one number that MOVED, and why it is not a regression.** `tel.freqHz` at the negative bound
is now `1.41828e-17` rather than `0`. The guarded pitch is `-64 V`, so the frequency is
denormal-small but **positive**, and the existing negated frequency floor `if (!(freq > 0.f))`
therefore correctly does **not** fire. That is **D-13's stated intent** — no low-end floor is
added, extreme negative pitch freezes the phase and the output becomes effectively DC — and the
figure matches the `1.418e-17` recorded independently in `kVcoNyquistGuardFrac`'s own rationale
comment to six significant figures. Before the bound, that row read `0` only because the UB
produced garbage that the floor happened to sanitise.

Two further rows are worth naming. `fm 12 V, atten 1.0` and `pitch +12, fm +12 @1.0` are
**legitimate deep-FM patches**, not hostile inputs, and both land on `tel.freqHz = 21829.5` — the
exact 44.1 kHz Nyquist ceiling. That is **D-10's hard clamp firing as designed**: the frequency
pins at the ceiling, the oscillator keeps sounding, `max|out|` stays at `4.99883`. The clause in
`kVcoNyquistGuardFrac`'s comment predicting that ("under deep FM the clamp fires on most cycles")
was written in 31-02 while FM was still anticipated; it is now demonstrated.

The scratch directory was deleted again, and the post-run repository state was re-verified:
`git status --porcelain --untracked-files=all` lists only `M src/dsp/VcoCore.hpp`, and
`grep -rc 'fsanitize'` returns `0` for both `Makefile` and `.github/workflows/test.yml`.

---

## The pitch block as it appears in the file

```cpp
		float pitchVolts = in.pitchCV + in.coarse + in.fine * (1.f / 12.f);
		if (in.fmConnected) pitchVolts += in.fmVolts * in.fmAtten;

		// [ ~22 lines of D-14 rationale — see the header ]
		if (!(pitchVolts > -kVcoMaxPitchVolts)) pitchVolts = -kVcoMaxPitchVolts;
		if (pitchVolts > kVcoMaxPitchVolts) pitchVolts = kVcoMaxPitchVolts;

		float freq = kVcoFreqC4 * exp2_taylor5(pitchVolts);
```

And the constant, in the existing block, after the Nyquist fraction and before the increment bound
so `kVcoFreqC4` and `kVcoNyquistGuardFrac` stay adjacent:

```cpp
constexpr float kVcoMaxPitchVolts = 64.f;       // UB bound on the exp2 argument (D-14), well inside the frozen helper's [-127, +128]
```

Namespace-scope **plain** `constexpr float` in `namespace forge`, the same idiom as its
neighbours — never `inline constexpr`, never an in-class `static constexpr` (declaration-only
under C++11, which odr-uses at runtime and fails to link on MinGW; that exact construct got v2.0.0
rejected from the VCV Library).

**The whole non-comment delta of this plan is six lines:**

```
$ git diff HEAD~2..HEAD -- src/dsp/VcoCore.hpp | grep -E '^[-+]' | grep -v '^[-+][-+]' \
    | grep -vE '^[-+][[:space:]]*//' | grep -vE '^[-+][[:space:]]*$'
+constexpr float kVcoMaxPitchVolts = 64.f;       // UB bound on the exp2 argument (D-14), ...
-		float freq = kVcoFreqC4 * exp2_taylor5(in.pitchCV);
+		float pitchVolts = in.pitchCV + in.coarse + in.fine * (1.f / 12.f);
+		if (in.fmConnected) pitchVolts += in.fmVolts * in.fmAtten;
+		if (!(pitchVolts > -kVcoMaxPitchVolts)) pitchVolts = -kVcoMaxPitchVolts;
+		if (pitchVolts > kVcoMaxPitchVolts) pitchVolts = kVcoMaxPitchVolts;
+		float freq = kVcoFreqC4 * exp2_taylor5(pitchVolts);
```

(plus the `kVcoFreqC4` declaration line, whose **code is byte-identical** — only its trailing
comment changed.)

## Test counts, pre-edit vs post-edit

| Run | Cases | Assertions | Failed |
|---|---|---|---|
| Pre-edit baseline (at `d8abc30`) | 72 | 2,616,112 | 0 |
| After Task 1 (no repository file touched) | 72 | **2,616,112** | 0 |
| After Task 2 (`2643c97`) | 72 | **2,616,112** | 0 |
| After Task 3 (`d25e672`) | 72 | **2,616,112** | 0 |
| Phase regression floor | 72 | 2,616,112 | 0 |

**Identical is the requirement, and it holds.** The plan's instruction was explicit: if any
existing case moves, STOP and report. None did. The existing grid holds `coarse`, `fine`,
`fmVolts` and `fmAtten` at zero with the jack unpatched, so the three new terms are arithmetically
inert there — `pitchVolts` reduces to `in.pitchCV` exactly, and the `fmConnected` gate means the
FM term is not even evaluated. `tests/test_vco_core.cpp` **was** recompiled against the changed
header, so the bit-stable count is evidence rather than a coincidence.

## Nothing downstream of the exponential moved

Verified by extraction and diff, not by reading:

```
$ git show HEAD~2:src/dsp/VcoCore.hpp | sed -n '/const float safeRate/,/^};/p' > before.txt
$ sed -n '/const float safeRate/,/^};/p' src/dsp/VcoCore.hpp                   > after.txt
$ diff before.txt after.txt
(empty — BYTE-IDENTICAL)
```

That region covers the sanitised-rate ternary, `maxFreq`, the ceiling, the negated frequency floor
and their **order** (CR-01: ceiling `:406` strictly before floor `:407`), the `tel.freqHz` write,
`deltaPhase` with **both** casts intact (PITCH-05), the negated increment floor,
`kVcoMaxDeltaPhase` (D-12), the single-subtract wrap, the `morph`/`character` clamps, the frozen
`Waveshape` call, the `tel.displayPhase` write and the unconditioned `x5` return.

The two POD structures were diffed the same way:

```
$ diff <(git show HEAD~2:... | sed -n '/^struct VcoInputs {/,/^};/p') <(sed -n ... )
(empty)      VcoInputs: IDENTICAL — no field added, no documented unit changed
$ diff <(... Telemetry ...)
(empty)      Telemetry: IDENTICAL — D-22 declined the pitch-volt telemetry field
```

## Gate results

| Gate | Required | Observed |
|---|---|---|
| `make guards` | exit 0, `guard suite: PASS` | **PASS** — run immediately after **every** save, per Pitfall 5 |
| `make strict` | exit 0, `strict C++11 gate: PASS` | **PASS** — `-std=c++11 -pedantic-errors` over all four TUs incl. `src/vco_compile_canary.cpp` |
| `make test` | 0 failed, unchanged counts | **72 / 2,616,112 / 0** — exact |
| UBSan RED | both frozen sites named | **`RackCompat.hpp:106:24` + `:109:11`** |
| UBSan GREEN | zero diagnostics | **0 lines, 0-byte stderr**, 24 configurations |
| CR-01 ordering | ceiling line < floor line | `:406` ceiling, `:407` floor |
| Source shape `[2b/5]` | `^struct VcoCore {$` == 1, step signature == 1 | **1 / 1** — signature never quoted into a comment line containing a brace |

**One guard output changed, and it is corroborating evidence.** `check_canary.sh [2b/5]` now
reports *"all 8 VcoInputs DSP field(s) stay runtime-live through step() at -O3"*. That is an
independent confirmation, from a guard this plan did not touch, that `coarse`, `fine`, `fmVolts`,
`fmAtten` and `fmConnected` are genuinely **read** at `-O3` and not constant-folded away.

## Milestone guardrail compliance

- `git diff --name-only HEAD~2..HEAD` = **`src/dsp/VcoCore.hpp`**. Nothing else, in either commit.
- **No frozen header edited.** `src/dsp/LfoCore.hpp`, `src/dsp/Waveshape.hpp`,
  `src/dsp/RackCompat.hpp`, `src/dsp/MathConst.hpp` and `src/dsp/FROZEN.sha256` are all absent
  from the diff. `check_frozen.sh` PASSes inside `make guards` and the six LFO `.f32` goldens
  replay byte-identical inside `make test`. The bound is deliberately LOCAL to `VcoCore` for
  exactly this reason — the natural place to fix the UB is inside the frozen helper, and going
  there would have been a guardrail event.
- **`src/AnalogLFO.cpp` absent from the diff** (D-16). It was recompiled by `make strict` and
  stayed clean.
- **No sanitizer wiring anywhere.** `grep -rc 'fsanitize' Makefile .github/workflows/test.yml`
  → `0` for both, before and after. No probe artifact in the working tree at any point.
- **C++11 clean:** the new constant is namespace-scope plain `constexpr`; non-comment counts for
  `inline constexpr` and `static constexpr` are both **0**; `std::exp2` / `std::pow` still absent
  from `src/` as code; no `std::clamp`, no `if constexpr`, no `[[maybe_unused]]`, no brace
  value-list init of `VcoInputs`.
- **No test file touched**, so nothing weakened a gate to accommodate the change.

## Decisions Made

1. **`kVcoMaxPitchVolts = 64.f`, exactly as research recommended.** CONTEXT.md marks the numeric
   value discretionary and research's table shows `[48, 96]` all satisfy every stated constraint.
   64 was taken for the four reasons now recorded in the source: it is a power of two so the two
   comparisons carry no rounding; it is 2.2× outside the measured reachable musical worst case
   (29.08 V) so it can never fire on a legitimate patch; it is 2.0× inside the UB boundary
   (±127 V); and the frequency stays **finite at both extremes** (4.826e21 / 1.418e-17) where
   ±120 or ±126 hand the downstream ceiling a positive infinity instead of a number. ±32 V was
   rejected in the comment as too tight (1.10× over the reachable sum).
2. **The negated comparison first, and `forge::clamp` rejected by name in the source.** A NaN
   fails `> -64`, so the negation is true and the NaN lands on the fallback branch as `-64`, which
   then fails the plain upper comparison and survives. `forge::clamp` is a comparison ladder whose
   **both** comparisons are false for a NaN, so it returns the NaN unchanged — inert against
   precisely the input class the guard exists to stop. Naming it in the comment closes deferred
   item 3 / CR-02 where the next editor will actually read it.
3. **No depth constant for the FM term** (D-06). Full clockwise is 1.0 octave per volt, so the
   product `in.fmVolts * in.fmAtten` *is* the contract; a named constant equal to one would be a
   multiply the compiler removes and a number with no requirement behind it. The contract lives in
   the comment, in words. The LFO's sub-audio scaling factor is named as rejected — but the
   *identifier* is deliberately not written anywhere in the file, since an acceptance criterion
   negative-greps it case-insensitively across the whole file.
4. **The gate is `if (in.fmConnected)` in the core** (D-09/D-17), so the unpatched path does not
   merely multiply by zero — it does not evaluate the term at all, whatever `fmVolts` and
   `fmAtten` hold. The shipped LFO's shell does this gating in `src/AnalogLFO.cpp:320`; that
   placement is the anti-pattern here, and `src/AnalogVCO.cpp` (31-04) must forward both fields
   **unconditionally**.
5. **The RED evidence tier was escalated rather than the claim weakened.** Research had measured
   the behavioral case vacuous. The alternative tier — a new `Telemetry::lastPitchVolts` field —
   was declined by D-22 because it changes the boundary shape for a diagnostic. UBSan is the
   defect itself rather than a proxy for it, and it went red at the exact two lines.
6. **The vacuity measurement and the RED diagnostic come from the SAME probe run.** That is
   deliberate: had they come from two runs, a reader would have to trust that the configurations
   matched. Now the table and the diagnostics are the two output streams of one process.
7. **The float summation stays in float.** The measured summation error is 0.0011 cents, four
   orders of magnitude under the one-cent gate; promoting it to double would diverge from the
   float-typed POD fields for no measurable gain and put a needless conversion in the audio path.
8. **Every line-number citation written into the header was verified by reading the cited file**
   this session: `RackCompat.hpp:104-111` (and `:106` / `:109` specifically),
   `LfoCore.hpp:181-187` and `:183-184`, `AnalogLFO.cpp:320`, `tests/check_frozen.sh`. This repo
   has now been bitten several times by prose that outlived its mechanism, and 31-02 established
   the practice explicitly.
9. **No requirement marked complete** — see Deviations.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 — Missing Critical] `kVcoNyquistGuardFrac`'s closing paragraph asserted a declaration ADJACENCY that Task 2 falsified**

- **Found during:** Task 3's mandated full-file comment re-read.
- **Issue:** The paragraph read *"It remains a different KIND of constant from the wrap-correctness
  bound on the phase INCREMENT **declared just below**, which D-12 leaves untouched."* Task 2
  inserted `kVcoMaxPitchVolts` and its 55-line rationale **between** the two, so
  `kVcoMaxDeltaPhase` is no longer "just below" — it is 70 lines below. This is the same
  false-comment class 31-02 cleared twice and plan 30-08 existed to remove, and it is a *new*
  variety of it worth recording: an **adjacency** claim is as perishable as an arithmetic one, and
  31-02 could not have anticipated it.
- **Fix:** Minimal restatement that drops the positional claim and names all three kinds instead:
  *"...different KIND of constant from the wrap-correctness bound on the phase INCREMENT, which
  D-12 leaves untouched — and from the undefined-behavior bound on the PITCH VOLTS, which Phase 31
  added between the two. All three are declared in this block and none of them substitutes for
  another."* Every substantive conclusion in the paragraph is preserved.
- **Files modified:** `src/dsp/VcoCore.hpp` (comment only).
- **Committed in:** `d25e672`.

**2. [Rule 2 — Missing Critical] The `kVcoMaxPitchVolts` rationale did not name the two UB lines individually**

- **Found during:** Task 3, after the sanitizer diagnostics were in hand.
- **Issue:** The block cited `src/dsp/RackCompat.hpp:104-111` (the whole helper) but not the two
  specific lines. The sanitizer named `:106:24` and `:109:11` precisely; a reader tracing the guard
  back to the defect deserves the same precision, and a range citation degrades faster than a
  line one.
- **Fix:** *"...casts the result to int32_t **at :106**, and then shifts that integer LEFT BY 23
  into a float union **at :109**"*. Both verified by reading the frozen header.
- **Committed in:** `d25e672`.

**3. [Prescribed, recorded for completeness] The banner and the `kVcoFreqC4` comment**

Task 3 prescribed both, so neither is strictly a deviation. Recorded because they are the *third
and fourth* forward-references this phase has had to retire, and because 31-02's hand-off
predicted this one by name: *"31-03 must rewrite the banner again."* The banner had said the
pitch handling *"reads in.pitchCV alone and is knowingly incomplete"* — true at `070af28`, false
at `2643c97`.

### Deferred / not done, deliberately

**4. No requirement marked complete, against the plan frontmatter's `[PITCH-01, PITCH-02, PITCH-03, PITCH-05, FM-01, FM-02, FM-03]`**

- **Found during:** the `requirements mark-complete` step.
- **Issue:** This plan lands the **arithmetic** and adds **no behavioral assertion**. The plan's
  own `<verification>` block says so explicitly: *"This plan's automated verification is
  non-regression plus source assertions by construction; the behavioral gate is those three plans'
  and TEST-02 is not claimed until they land."* Reading the requirement texts confirms it
  individually:
  - **PITCH-01** asserts V/Oct *"tracks* 1V/octave across the audio range"* — a measured tracking
    claim, owned by **31-05** (`-tc="*v/oct tracking*"`).
  - **PITCH-02** and **PITCH-03** describe a **knob** that sweeps ±5 octaves / trims ±1 semitone.
    Neither knob exists yet — `src/AnalogVCO.cpp` gains `COARSE_PARAM` and `FINE_PARAM` in
    **31-04**, and **31-06** owns the assertions. The arithmetic is here; the control is not, so
    the requirement is not user-reachable at all.
  - **FM-01** ("modulates pitch at audio rate") and **FM-02** ("a dedicated bipolar
    attenuverter") likewise need the `FM_INPUT` jack and `FM_ATTEN_PARAM` from **31-04** plus
    **31-06**'s audio-rate case.
  - **FM-03** ("sums into the volt domain before the single exponential") is the closest to
    provable by construction here, and this summary's D1 coverage entry records the source
    assertions. But **31-06** owns the `*exponential FM*` case and the
    `DeliberatelyMultiplicativeFmCore` broken control that make the claim non-vacuous, and marking
    it now would claim a gate that does not yet exist.
  - **PITCH-05**'s mechanism was already in place before this phase; this plan's obligation was
    non-regression only, and it is discharged and evidenced above. The *"so high-frequency
    phase-crossing placement stays accurate"* clause is what **31-05**'s high-pitch tracking gate
    demonstrates.
- **Action:** all seven stay `- [ ]` / `Pending`. Marking them here would reproduce the false
  green that Phase 30 deferred item 1 recorded for `PANEL-03`, that 31-01 declined for `TEST-02`,
  and that 31-02 declined for `PITCH-04`. **This is the third consecutive plan in this phase
  making the same call, which makes it a consistent standard rather than an arbitrary one.**
- **For the phase gate:** confirm `PITCH-01` after 31-05; `PITCH-02`, `PITCH-03`, `FM-01`,
  `FM-02`, `FM-03` after 31-04 **and** 31-06; `PITCH-04` after 31-07; `PITCH-05` after 31-05;
  `TEST-02` after 31-05/06/07.
- **Recorded in:** STATE.md § Accumulated Context.

**5. The shipped LFO's shared latent UB is NOT fixed, by decision (D-24)**

MEASURED and verified by reading both files this session: `src/AnalogLFO.cpp:320` feeds an
unsanitized `inputs[FM_INPUT].getVoltage()` into `src/dsp/LfoCore.hpp:183-184`, which reaches the
identical `(int32_t)` cast and `<< 23` shift in the frozen helper. A hostile or non-finite cable
voltage into the **shipped** module's FM jack therefore reaches the same undefined behavior this
plan just closed for the VCO. It is **out of scope for Phase 31**, any fix is a **guardrail event**
requiring operator sign-off and golden re-verification, and it is the direct reason no permanent
sanitizer gate was added. **31-08 owns writing this into
`.planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md`.** It is recorded here, and in
the header's own `kVcoMaxPitchVolts` rationale, so it cannot be rediscovered as a surprise.

### Verification-command notes (no code impact)

**Task 2 criterion `grep -cE 'inline constexpr' src/dsp/VcoCore.hpp` returns `2`, not `0`.**
Both hits are **pre-existing comment lines that name the banned construct in order to forbid it** —
`:60` (`// - No \`inline constexpr\` variables (C++17 inline variables)`) and `:95` (`// banner
mandates above. NOT \`inline constexpr\` (C++17)...`). The count at the pre-plan commit `d8abc30`
is also `2`, so this plan introduced neither. The substantive claim is **true and verified** via
the non-comment form the sibling criterion already uses:

```
$ grep -v '^[[:space:]]*//' src/dsp/VcoCore.hpp | grep -cE 'inline constexpr'
0
```

Corroborated by `make strict` (a `-pedantic-errors` C++11 leg rejects a real `inline constexpr`
variable) and by `check_canary.sh [4/5]`, whose negative control confirms a namespace-scope
`inline constexpr` variable *is* rejected for the expected reason. **This is the same
criterion-filter artifact 31-02 documented**, and it recurred exactly as 31-02 predicted it would:
this file deliberately quotes every construct it bans, so any file-wide grep for a banned
construct must be scoped to non-comment lines. Two of this plan's own criteria already are; this
one was not.

**Task 3 criterion "no line ending in `;` or `{` differs" — holds, with one line needing a word.**
Exactly one line in the Task 3 diff is not comment-prefixed: the `kVcoFreqC4` declaration, whose
**code is byte-identical** (`constexpr float kVcoFreqC4 = 261.6256f;`) and whose only change is
its trailing comment. That line ends with the comment text, not with `;`, so the criterion passes
literally as written:

```
$ git diff -U0 src/dsp/VcoCore.hpp | grep -E '^[-+]' | grep -v '^[-+][-+]' | grep -E '[;{][[:space:]]*$'
(empty)
```

---

**Total deviations:** 2 auto-fixed (both Rule 2, comment-truth), 2 prescribed-but-recorded, 2
deliberate non-actions (requirements not falsely marked; the shipped LFO's shared UB left unfixed
by decision), 2 criterion artifacts documented.
**Impact on plan:** No scope creep. Every prescribed edit landed exactly as specified. The
non-comment footprint is the six lines the plan predicted ("one new constant, four new lines of
arithmetic"), plus the one-line replacement of the old pitch expression.

## Issues Encountered

- **None blocking.**
- **Note for later plans (third confirmation):** `gsd-tools query state.record-metric` /
  `state.add-decision` / `state.add-blocker` take **named flags**, not the positional arguments the
  `execute-plan.md` workflow shows. Carried forward from 31-01 and 31-02, confirmed again here by
  invoking each handler with no arguments and reading its required-argument error.
- **UBSan deduplicates by default**, so each site reports once no matter how many hostile inputs
  reach it. That is why the RED transcript is four lines for fifteen configurations rather than
  thirty. Worth knowing before anyone reads a short transcript as weak coverage.

## User Setup Required

None — no external service configuration required. The probe used only the system compiler
(`Apple clang 16.0.0`) and in-tree headers; **zero registry packages this phase**, matching
`31-RESEARCH.md` §Package Legitimacy Audit's empty table (T-31-SC).

## Next Phase Readiness

- **Ready for 31-04 (the shell).** The core reads `coarse`, `fine`, `fmVolts`, `fmAtten` and
  `fmConnected`; `src/AnalogVCO.cpp` must now supply them. **Forward all five raw and
  unconditionally** — do **not** copy the shipped LFO shell's `in.fmCV = in.fmConnected ? ... :
  0.f;` conditional at `src/AnalogLFO.cpp:320`. D-17 makes "the shell computes nothing"
  load-bearing, Rack already returns `0.f` from `getVoltage()` on an unpatched input, and D-09's
  gate now lives in the core.
- **Ready for 31-05/31-06/31-07 (the gates).** The arithmetic they assert against is in place and
  the regression floor is exact. Three specific hand-offs:
  - The **crossover volts** are `+6.3826 / +6.5049 / +7.5049 V` and D-21 requires the gate to
    **derive** them from `kVcoNyquistGuardFrac` rather than hardcode them.
  - `kVcoFreqC4`'s new comment records that TEST-02 **deliberately** computes its ground truth
    from the **decimal** `261.6256`, not from the float `261.6256103515625`. That independence is
    the point; do not "fix" the gate to read the constant. The offset it costs is
    **+0.0000685 cents**.
  - 31-07's standing behavioral case should be pinned **at the bound** (`pitchVolts` driven to
    exactly `±kVcoMaxPitchVolts` yields a finite, in-range output), symbolically through the
    constant. The measured values it will see are in the GREEN table above:
    `tel.freqHz = 21829.5` at 44.1 kHz on the positive side, `1.41828e-17` on the negative side.
    **Note that a `tel.freqHz == 0` assertion for hostile pitch would now FAIL** — the negative
    bound produces a small positive frequency by design (D-13), and that is the one behavioral
    number this plan moved.
- **31-07 must also update `tests/test_vco_core.cpp`'s `DeliberatelyBrokenSharedStateCore::step`
  mirror** to match the new pitch block, per the phase's artifact list. This plan did not touch it
  and did not need to — the mirror compiled and passed unchanged, because its divergence is in
  state sharing, not in the pitch expression.
- **32-morph-blep** inherits an unchanged double accumulator and both `deltaPhase` casts, so the
  sub-sample crossing fraction it needs is still computable in double. No float round-trip was
  introduced between `freq` and `phase`.
- **The header's comments are truthful at this commit and will go stale again.** Everything the
  banner now attributes to Phases 32/33/34 is a forward reference by construction; the
  `kVcoMaxPitchVolts` rationale's citation of `AnalogLFO.cpp:320` and `LfoCore.hpp:183-184` will
  drift if either shipped file is ever renumbered.
- **No blockers.**

## Self-Check: PASSED

- `src/dsp/VcoCore.hpp` — FOUND
- `.planning/phases/31-pitch-tuning-exponential-fm/31-03-SUMMARY.md` — FOUND
- Commit `2643c97` — FOUND
- Commit `d25e672` — FOUND
- No file deletions in either task commit (`git diff --diff-filter=D` empty for both)
- No untracked residue after either commit (`git status --porcelain --untracked-files=all` empty)
- No probe artifact, no `fsanitize` wiring, no frozen header and no `src/AnalogLFO.cpp` in the
  two-commit diff

---
*Phase: 31-pitch-tuning-exponential-fm*
*Completed: 2026-07-30*
