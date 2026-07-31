---
phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
plan: 04
subsystem: dsp
tags: [polyblep, polyblamp, band-limiting, morph-aware, d-01, d-03, d-04, d-05, d-07, d-13, d-14, canary, vco]

# Dependency graph
requires:
  - phase: 30-vco-core-skeleton-and-registration
    provides: "forge::VcoCore, the naive morphed oscillator and the negated-comparison guard idiom it rejects forge::clamp by name for"
  - phase: 29-vco-test-harness-and-lfo-guardrail
    provides: "src/vco_compile_canary.cpp and its D-08 growth rule; tests/check_canary.sh [5/5] and [5b/5]"
  - phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp
    provides: "plan 32-03 — the 90-cell naive alias baseline and kThresholdFloorDb = -75 dB, the measurement this header's corrections will be graded against"
provides:
  - "src/dsp/MorphBlep.hpp — the phase's single genuinely-new subsystem, holding kernels and site logic together (CORE-02 / D-12)"
  - "forge::morphBlepCharFactor(w, dt) — the ONE unified compact-support D-03 factor, max(0, 1 - w/(2*dt))^2, exact 1 at a hard step and exact 0 past the kernel's 2-sample support"
  - "struct forge::MorphBlep — pending/inject per-instance accumulator, reset(), addStep() (the D-14 sync seam) and step() (the per-sample correction)"
  - "The activated D-08 canary include, carrying the header into the C++11 strict gate and the CI MinGW link leg"
affects: [32-05, 32-06, 32-07, 32-08, 33-hard-sync, morph-blep]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Compact-support character factor: exact zero at a finite width, read off the kernel's own support rather than fitted (D-03 / P-1)"
    - "Split-source crossing test: the SIDE decision from the float the frozen branch compares against, the DISTANCE from the double accumulator (P-3)"
    - "Fixed site union with magnitudes that fall to zero, so absent shapes cost nothing and no site table is ever cached (D-04)"
    - "Pending-residual accumulator delivering a 2-sample kernel's second half at zero added latency, composing with overlapping edges by summation (D-13 / D-07)"
    - "Falsified-premise-corrected-in-place: keep the conclusion, replace the reason, and say the old reason was measured false"

key-files:
  created:
    - src/dsp/MorphBlep.hpp
  modified:
    - src/vco_compile_canary.cpp

key-decisions:
  - "The header is 499 lines, of which the overwhelming majority is the recorded justification the plan asks for; the executable body is ~120 lines of add, subtract, multiply, divide and compare with zero standard-library calls"
  - "Task 2's tdd=\"true\" RED was established out of tree, by a transient 22-assertion behavior probe, because tests/test_morph_blep.cpp is plan 32-05's deliverable and 32-04's own acceptance criteria require an UNCHANGED doctest case count"
  - "At exactly morph = 0.75 the frozen direct-duty special case zeroes W[3], so the square's own sites carry no weight there; the plan's behavior 2 was exercised at morph = 0.70 instead, where W[3] is live"
  - "The exact-alignment boundary (dt dividing a site position evenly) was asserted rather than avoided: s == 1, u == 0, and the whole correction defers to the pending half"

patterns-established:
  - "Out-of-tree RED probe: when a plan's tdd task has no permitted test file of its own, observe RED against a scratchpad translation unit that includes the header, then implement, then observe GREEN, and never commit the probe"

requirements-completed: []

coverage:
  - id: D1
    description: "src/dsp/MorphBlep.hpp exists with forge::morphBlepCharFactor and struct forge::MorphBlep (pending, inject, reset, addStep, step)"
    requirement: "CORE-02"
    verification:
      - kind: other
        ref: "standalone -std=c++11 -pedantic-errors -fsyntax-only -Wall -Wextra probe over the header alone -> PROBE_OK"
        status: pass
      - kind: integration
        ref: "make strict -> strict C++11 gate: PASS over all four src/ TUs"
        status: pass
    human_judgment: false
  - id: D2
    description: "The D-03 factor reaches its stated limits EXACTLY — 1 at a true hard step, 0 past the kernel's 2-sample support"
    requirement: "AA-01"
    verification:
      - kind: other
        ref: "behavior probe B1/B4: morphBlepCharFactor(0, dt) == 1.f exactly at dt = 0.02 and 0.0005; morphBlepCharFactor(0.175, dt at C2) == 0.f exactly"
        status: pass
      - kind: other
        ref: "discriminator: the SAME triangle site at the SAME character emits 0.0337 at dt = 0.15, proving the zero comes from the factor and not from an absent site"
        status: pass
    human_judgment: false
  - id: D3
    description: "The replicated weight algebra agrees with the frozen path, including the bleed ring D-05 folds into W"
    requirement: "AA-01"
    verification:
      - kind: other
        ref: "behavior probe B3: predicted wrap jump 0.00185643556 against closed form 0.0018564375; frozen morphedWave measures a 0.0148528 discontinuity against the predicted 0.0148515"
        status: pass
    human_judgment: false
  - id: D4
    description: "The square's hard step and soft edge are separate site entries at separate positions (P-2 / T-32-16)"
    requirement: "AA-02"
    verification:
      - kind: other
        ref: "behavior probe B2: the hard step fires at 0.5 (|corr| 0.4803) while NOTHING fires at dutySq = 0.51, where the soft factor is exactly 0; corrected |out| 0.976, inside the 1.11 naive envelope"
        status: pass
    human_judgment: false
  - id: D5
    description: "Hostile timing reaches no divisor — step() returns only the drained accumulator for zero, negative, subnormal and not-a-number dt (D-15 / P-14 / T-32-02)"
    requirement: "AA-05"
    verification:
      - kind: other
        ref: "behavior probe B5: all four hostile dt values return exactly inject+pending = 0.25 and leave both members zeroed"
        status: pass
    human_judgment: false
  - id: D6
    description: "The canary carries the header into BOTH C++11 gates in the same commit that created it (D-08 / P-9)"
    requirement: "CORE-02"
    verification:
      - kind: integration
        ref: "check_canary.sh [5/5] -> OK: dsp/MorphBlep.hpp is carried into both gates by the canary; git show --name-only 228242f lists both files"
        status: pass
    human_judgment: false
  - id: D7
    description: "No file-scope mutable state — all accumulator state is per-instance (CORE-03 / D-14 / T-32-18)"
    verification:
      - kind: other
        ref: "grep -cE '^[[:space:]]*(static|extern)[[:space:]]' src/dsp/MorphBlep.hpp -> 0"
        status: pass
    human_judgment: false
  - id: D8
    description: "The shipped Analog LFO is untouched: no frozen header edited, FROZEN.sha256 unmoved, src/AnalogLFO.cpp absent from the diff"
    verification:
      - kind: integration
        ref: "make guards + make test (85/85, 2,618,907 assertions, unchanged) + git show --name-only over both commits"
        status: pass
    human_judgment: false

# Metrics
duration: 35 min
completed: 2026-08-01
status: complete
---

# Phase 32 Plan 04: The MorphBlep Header Summary

**`src/dsp/MorphBlep.hpp` now exists in full — the compact-support D-03 character factor, the per-instance zero-latency accumulator, the D-14 sync seam, the replicated weight algebra, the live geometry, the fixed nine-site union and the split-source crossing test — and the compile canary carries it into both C++11 gates from the commit that created it.**

## Performance

- **Duration:** ~35 min
- **Completed:** 2026-08-01
- **Tasks:** 2
- **Files:** 1 created (`src/dsp/MorphBlep.hpp`, 499 lines), 1 modified (`src/vco_compile_canary.cpp`)

## Task Commits

1. **Task 1: The header skeleton, the D-03 factor, the D-14 sync seam, and the canary include** — `228242f` (feat)
2. **Task 2: The per-sample `step()` body — weight algebra, live geometry, the nine-site union, the split-source crossing test** — `bccff22` (feat)

## Accomplishments

- **The header is the phase's single new subsystem, and it holds kernels and site logic together** as CORE-02 / D-12 require. It calls the frozen `forge::Waveshape` and never edits it; `src/dsp/FROZEN.sha256` is unmoved and `src/AnalogLFO.cpp` is absent from both commits' diffs.
- **The weight algebra was verified against the frozen path, not assumed.** At `morph = 0`, `character = 0.5` the header predicts a wrap jump of `0.0148515`; driving `Waveshape::morphedWave` across phase 0 measures `0.0148528`. The bleed ring is real and it is carried: at `morph = 0` the narrow pulse bleeds into what the user hears as a pure sine, and this implementation band-limits it.
- **P-2 is defended by construction and demonstrated by measurement.** The square's hard step sits at `0.5f` and its soft edge at `dutySq = 0.51`, as separate entries. At a `dt` where the soft factor is exactly zero, a correction of magnitude `0.4803` fires at `0.5` and **nothing at all** fires at `0.51` — which is precisely the signature a merged entry would invert. Corrected `max|out|` measured `0.976`, comfortably inside the `1.1047` naive envelope P-10 records.
- **D-07's overlapping-edge summation was observed rather than argued.** At `morph = 0.70` the square's hard site at `0.5` and the pulse's hard site at `pulseDuty = 0.5` coincide; the two magnitudes summed to exactly `-1.2` (`1.188119 + 0.011881`), and the correction placed was `0.5999996`. Two sites, one slot, `+=` not `=`.
- **Hostile timing reaches no divisor.** Zero, negative, subnormal and not-a-number `dt` each return exactly the drained accumulator and leave both members zeroed — and the drain happens *before* the guard, deliberately, because a residual already owed is still owed on a sample the caller mistimed.
- **The canary include was activated in the same commit as the header**, so `make guards` was never red. `git show --name-only 228242f` lists both files.

## The Three Falsified Premises, Corrected In Place

Each correction lives in the source, not only in this summary, and each keeps the original conclusion while replacing the reason.

| # | The falsified claim | Where the correction now lives | The measurement that falsified it |
|---|---------------------|-------------------------------|-----------------------------------|
| 1 | `research/STACK.md:40` — "err toward over-correction; character is a lowpass-ish coloration, so extra correction only adds harmless HF rolloff" | `src/dsp/MorphBlep.hpp:37-49` | At character 1 / C6 / square-centre morph: naive **−60.1 dB**, full authority **−29.9 dB** — a **30 dB regression**. The residual is a step-shaped correction added to a signal with no step: new broadband energy, not a filter. This is why the D-03 factor must reach *exactly* zero. |
| 2 | `research/STACK.md:100-104`'s polyBLAMP snippet | `src/dsp/MorphBlep.hpp:50-60` | It returns a **quartic** where the 2-point form is **cubic**, and folds `dt` into the kernel rather than the slope. It is neither the 2-point form used here nor the DAFx-16 four-point form. The forms shipped here were derived from first principles and cross-validated: 7e-7 against the canonical two-branch form at `dt = 0.094`, and the corrected saw's harmonic gain matches a squared sinc to 0.01 dB over twelve harmonics. |
| 3 | D-03's corollary — "as character rises the saw wrap's effective step shrinks and its correction shrinks with it" (P-4) | `src/dsp/MorphBlep.hpp:375-392` | The curved saw evaluates to 1 at phase 0 *before* the reset is applied, and the reset blends **from** a reset value of 1 **toward** the curved saw, so both are 1 there. The wrap jump is **+2.000000** at character 0, 0.25, 0.5, 0.75 and 1.0. The saw site therefore has width 0 and factor exactly 1 at every character. D-03's *conclusion* survives; only its premise was wrong. |

A fourth reconciliation, not a falsification, is recorded at `src/dsp/MorphBlep.hpp:173-179`: the widely published two-branch kernel returns **twice** this residual and is applied at **half** the jump. A future editor cross-reading against a published listing will find a factor of two, and that paragraph is the reconciliation rather than a bug report.

## Verification Results

| Check | Result |
|-------|--------|
| `make strict` | `strict C++11 gate: PASS` over all four `src/` TUs |
| `make guards` | `guard suite: PASS` |
| `check_canary.sh [5/5]` | `OK: dsp/MorphBlep.hpp is carried into both gates by the canary` |
| `check_includes.sh [2/7]` | `OK: src/dsp/MorphBlep.hpp — no Rack include` |
| `check_includes.sh [3/7]` | `OK: src/dsp/MorphBlep.hpp — siblings and standard headers only` |
| `check_frozen.sh` | `PASS: frozen-source gate clean (D-05 manifest + golden fixtures + negative control)` |
| `make test` | **85 cases / 85 passed / 0 failed**, 2,618,907 assertions — **unchanged** from the plan's starting baseline, as required |
| Standalone C++11 probe | `c++ -std=c++11 -pedantic-errors -fsyntax-only -Wall -Wextra -Isrc` over the header alone → **`PROBE_OK`** |
| `grep -c 'morphBlepCharFactor'` | 2 (≥ 2 required) |
| `grep -cE 'if \(!\(u > 0\.f\)\) return 0\.f;'` | 1 |
| `grep -c 'void addStep(float xAhead, float jump)'` | 1 |
| active canary include (comments stripped) | 1 |
| `grep -cE '^[[:space:]]*(static\|extern)[[:space:]]'` | **0** — no file-scope mutable state (CORE-03 / D-14) |
| `grep -c 'std::'` (comments stripped) | **0** — add, subtract, multiply, divide, compare only (AA-05) |
| `grep -cE 'if \(!\(pos\[i\] > p\)\)'` | 1 — the float-sourced SIDE decision |
| `grep -cE '\(double\)pos\[i\] - phase'` | 1 — the double-sourced DISTANCE |
| `grep -cE 'if \(!\(fdt > 0\.f\)\) return now;'` | 1 |
| `dutySq` in comment-stripped source | 3 (≥ 2 required), on separate array elements from the `0.5f` entry |
| `git status --porcelain src/dsp/FROZEN.sha256` | empty |
| `git diff --name-only` per task | Task 1: `src/dsp/MorphBlep.hpp`, `src/vco_compile_canary.cpp`. Task 2: `src/dsp/MorphBlep.hpp` |
| `src/dsp/Waveshape.hpp`, `src/dsp/RackCompat.hpp`, `src/AnalogLFO.cpp` | absent from both diffs |

## TDD Gate Compliance

Task 2 is marked `tdd="true"`. Its `<behavior>` block is fully checkable, but **there is no permitted test file for it to land in**: `tests/test_morph_blep.cpp` is plan **32-05**'s named deliverable, and 32-04's own acceptance criteria require `make test` to exit 0 **with an unchanged case count** ("nothing calls `step()` yet — plan 32-06 wires it"). Committing a doctest case here would collide with 32-05 and violate this plan's own gate.

RED was therefore established the way this phase already establishes it (plan 32-03, Task 1): **observed, not argued**, against a transient out-of-tree probe translation unit that includes `src/dsp/MorphBlep.hpp` directly, compiled with the same `-std=c++11 -pedantic-errors -ffp-contract=off` flags P-11 makes load-bearing. The probe was never added to the repository and no repository file was modified to accommodate it.

| Gate | Evidence |
|------|----------|
| **RED** | Probe run against the Task 1 header (whose `step()` returns only the drained accumulator): **4 failures**, and precisely the four behaviors that require a placed site — the saw wrap magnitude, the hard step at `0.5`, the soft edge discrimination, and the bleed-ring `h`. The behaviors Task 1 genuinely delivered (`morphBlepCharFactor`'s two exact limits, the hostile-`dt` drain) passed at RED, which is correct: they were already implemented. |
| **GREEN** | Probe re-run after Task 2: **22 assertions, 0 failures.** |
| **Discriminator** | B4's zero is proven to come from the *factor*, not from an absent site: the same triangle site at the same character emits `0.0337` once `dt = 0.15` makes `2*dt` exceed the 0.175 corner width. |

Commit types are `feat(32-04)` for both tasks — new production behavior in both cases.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 — Bug] The plan's behavior 2 cites `morph = 0.75`, where the frozen direct-duty special case leaves the square's own sites with ZERO weight**

- **Found during:** Task 2, while building the RED probe for behavior 2.
- **Issue:** The plan's `<behavior>` reads *"with `morph = 0.75` (square centre) and `character = 0.5`, site 4's hard step sits at `0.5f` and site 5's soft edge sits at `dutySq`"*. At exactly `morph = 0.75`, `scaled = morph * 4.f` is exactly `3.0`, so `segment` is 3 and the frozen direct-duty special case at `Waveshape.hpp:179-182` fires: `W[4] = 1.f` and **`W[3]` stays 0**. Sites 4 and 5 therefore both carry magnitude zero and are skipped by the `mag[i] == 0.f` continue. The square's own sites are live only for `morph` strictly below 0.75.
- **Root cause:** the plan's parenthetical "(square centre)" describes the *knob legend*; the frozen code's five-shape mapping hands the whole square-to-pulse segment to the pulse at an interpolated duty, so at the segment boundary the square shape has already been handed over. This is the frozen behavior and the header mirrors it exactly.
- **Fix:** **no code change** — the implementation was already correct and matches the frozen path. The *probe* was corrected to exercise the position claim at `morph = 0.70` (`scaled = 2.8`, `segment = 2`, `frac = 0.8`, so `W[3] = 0.792` is live), which is what behavior 2 is actually asserting: that the two positions are distinct and never merged.
- **Verification:** printed the weight-vector derivation at `morph` 0.70 / 0.74 / 0.75 and confirmed `segment` flips to 3 exactly at 0.75. At `morph = 0.70` the hard step fires at `0.5` with `|corr| = 0.4803` and nothing fires at `0.51`.
- **Recorded for plan 32-05,** which owns the permanent unit suite and will otherwise inherit the same unsatisfiable wording.

---

**2. [Rule 2 — Missing Critical] The plan's behavior 2 probe is silent at a `dt` that divides the site position evenly; the exact-alignment boundary is asserted rather than avoided**

- **Found during:** Task 2, GREEN verification — the first probe attempt used `dt = 1/512`, which divides `0.5` exactly.
- **Issue:** when a site lands exactly on a sample, `s == 1` and `u == 0`, so the firing sample's own contribution is **legitimately zero** and the entire correction defers to the pending half on the next sample. A probe that only inspects the firing sample reads zero and reports a false failure. This is the same class of boundary P-3's measured resonance failure lives on (`dt = 0.0005` with `pulseDuty = 0.374`, exactly 748 samples per edge), so it is worth an assertion rather than a workaround.
- **Fix:** the non-aligned case moved to `dt = 0.0019`, **and** the aligned case was added as its own pair of assertions — the firing sample contributes exactly `0.0`, and the whole correction arrives as the pending half (`0.5999996`). No edge is lost and none is double-fired.
- **Why this matters beyond the probe:** it is direct evidence that the double-sourced distance tiles exactly, which is the property P-3 says the split rule exists to buy. **Recommended for plan 32-05's permanent suite.**

---

**Total deviations:** 2 auto-fixed (1 bug in the plan's stated behavior, 1 missing critical boundary case). **Zero production-code deviations** — both findings corrected the *verification*, not the implementation. No guard was weakened, no frozen file was touched, and the plan's file list is unchanged.

## Findings Recorded for Later Plans

- **Plan 32-05 (the permanent unit suite)** should exercise the square sites at `morph < 0.75`, not at `0.75`, and should carry the exact-alignment boundary pair described above. The 22 probe assertions used here are a ready starting inventory.
- **Plan 32-06 (the `VcoCore` wiring)** must pass the **same** float `p` to `morphedWave` and to `MorphBlep::step`, and the **double** `phase` and `deltaPhase` alongside it. Pattern 2 makes that identity load-bearing, and nothing in this header can detect a caller that breaks it.
- **Plan 32-07 (the threshold re-pin)** inherits 32-03's warning that a `dt`-scaling bug shows up as the 48 kHz row diverging from the 44.1 kHz row. This header's correction is a function of the fractional sub-sample edge position and `dt` only, so that agreement is expected to hold; if it does not, the slope-break branch's `fdt` factor is the first place to look.
- **Plan 32-08 (the output envelope)** should note the measured corrected `max|out|` of `0.976` at `morph = 0.70` / `character = 0.5` / `dt = 0.0019` — inside the `1.1047` naive envelope, consistent with P-10's table.
- **Phase 33 (hard sync)** plugs into `addStep(xAhead, jump)` and needs no change to this header. Phase 32 added no sync field to `forge::VcoInputs`, as D-14 requires.
- **The deferred first refinement** — the narrow-pulse "reach" factor, measured at +1.3 dB at the single worst grid point — is documented at `src/dsp/MorphBlep.hpp:88-95`. Try it first if the pulse threshold at C8 is missed in 32-07.

## Known Stubs

None. `MorphBlep::step` is complete: Task 1's deliberate intermediate state (returning only the drained accumulator, with an explicit growth-point marker) was fully replaced by Task 2 in the same plan, and the growth-point comment was removed with it. Nothing calls `step()` yet — plan 32-06 wires it — but that is the plan's stated sequencing, not a stub.

## Threat Flags

None. Every file touched is inside the plan's declared surface, no network, auth, file-access or schema surface was introduced, and the four threat-register entries assigned to this plan (T-32-02, T-32-04, T-32-05, T-32-16, T-32-17, T-32-18) are each mitigated in the source with the mitigation named in a comment beside it:

| Threat | Mitigation as landed |
|--------|----------------------|
| T-32-02 (division by `dt`) | Two negated-comparison guards, both asserted by grep; no division by an edge width anywhere in the file |
| T-32-04 (out-of-bounds indexing) | `segment` bounded above by 3 mirroring the frozen minimum, ring indices `% 5` on a provably non-negative sum, fixed nine-element arrays with a literal loop bound |
| T-32-05 (in-class constant table → MinGW undefined reference) | Site arrays are **function-local** `const`; carried into the MinGW link leg by the canary include activated in Task 1 |
| T-32-16 (spectrally invisible full-amplitude spike) | Square hard step at `0.5f` and soft edge at `dutySq` as separate entries; float SIDE, double DISTANCE. Both asserted by the probe |
| T-32-17 (out-of-range sync event) | `addStep`'s entry gate is negated-first, rejecting a not-a-number `xAhead` before it reaches per-instance state |
| T-32-18 (shared mutable voice state) | Both accumulator members are NSDMI struct fields held by value; zero file-scope `static`/`extern`, asserted by grep |

## Issues Encountered

None beyond the two deviations above. `make strict`, `make guards` and `make test` were green on every task commit.

## Deferred Items (out of scope, not fixed)

- **`.planning/research/.cache/` is untracked** and predates this plan (it was present in the working tree at plan start). It is a research-tooling cache, not a build artifact of this phase, and deciding whether it belongs in `.gitignore` or in the repository is outside 32-04's file list. Left untouched and recorded here.

## Next Phase Readiness

**Ready for 32-05.**

`forge::MorphBlep` is complete, compiles clean under both standards, is carried by the canary into both C++11 gates, and its behavior is characterised by 22 measured assertions whose inventory is written into this summary for 32-05 to make permanent. Nothing in the shipped LFO moved.

## User Setup Required

None — no external service configuration required.

## Self-Check: PASSED

- `src/dsp/MorphBlep.hpp` — FOUND on disk (499 lines)
- `src/vco_compile_canary.cpp` — FOUND on disk, active include verified with comments stripped
- `.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/32-04-SUMMARY.md` — FOUND on disk
- Commit `228242f` — FOUND in `git log`, `--name-only` lists both files
- Commit `bccff22` — FOUND in `git log`
- All plan `<success_criteria>` re-run and green; all task `<acceptance_criteria>` for both tasks re-run and green

---
*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Completed: 2026-08-01*
