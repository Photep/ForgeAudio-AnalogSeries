# Phase 33: Hard Sync - Research

**Researched:** 2026-08-28
**Domain:** Band-limited hard sync (sub-sample phase reset + sync polyBLEP) on a frozen morphing waveshaper, C++11 Rack-free DSP
**Confidence:** HIGH on the placement question (measured in-session), HIGH on the guard/landmine analysis (read from source), MEDIUM on the grid figures (prototype, not the real core)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

Copied from `.planning/phases/33-hard-sync/33-CONTEXT.md` `<decisions>`. Each decision's rationale
and its rejected alternatives live in CONTEXT.md and are **not** restated here — read them there
before planning. Research below addresses the **execution** of these decisions and never re-opens one.

- **D-04: Minimum fix, PLUS a finiteness guard on `addStep`'s `jump` argument.** Clamp `segment` from
  below as well as above (CR-01), make the entry guard reject a non-finite `morph`/`character`
  (CR-02), and — the third item, found during the discussion and NOT in the register — guard
  `addStep`'s `jump`, which is currently unguarded and is the identical permanent-poison mode plan
  32-05 measured for a `+infinity` `dt`. All three use the **negated-comparison idiom**; never a
  clamp ladder, never `forge::clamp`. RED-first. **This is the one place the phase touches
  `MorphBlep.hpp` by plan.**
- **D-01: The sub-sample instant comes from linear interpolation to the SchmittTrigger's HIGH
  threshold.** On a rising edge, `f = (high − prev) / (now − prev)`. **The reset phase is the
  fractional overshoot: `phase = (1 − f) · deltaPhase`. NEVER snap to exactly 0.**
- **D-02: Raw volts cross the POD boundary — the core owns the detection.** `forge::VcoInputs` gains
  `syncVolts` and `syncConnected`; `forge::VcoCore` owns the `SchmittTrigger`, the previous-sample
  store and the interpolation. Two new POD fields re-open the compile canary's obligation (register
  item 15) — **an explicit plan task with its own rationale, not a gate-time discovery.** `CORE-03`
  binds: trigger state and previous-voltage store are **per-instance**, never static.
- **D-03: Thresholds are `0.1f, 1.0f`.** Verbatim what the shipped LFO uses at both trigger sites.
- **D-05: `jump` comes from ONE extra `morphedWave` call at the post-reset phase; the pre-reset value
  reuses the naive sample already computed this step.** This deliberately departs from Phase 32's
  "compute it, don't measure it" through-line, for a stated reason. **Downstream agents must not
  "restore" the analytic approach for consistency.** Phase 32 D-05 bleed normalization is satisfied
  by CONSTRUCTION.
- **D-06: THE PLACEMENT CONVENTION IS MEASURED, NOT ARGUED. Routed to `gsd-phase-researcher`.** Three
  candidates: (a) `addStep(f, jump)` at the detection sample; (b) an additive past-edge entry point
  giving the current sample the "after" residual `−f²/2·jump`; (c) `xAhead = 0`. **Hard constraints,
  all three binding: no output delay buffer; `addStep`'s `[0,1]` contract may be *extended*
  additively but not reinterpreted; the reset never snaps to exactly 0.** Measure against the sync
  sub-grid (D-10) on the Phase 32 spectral apparatus; pin by the D-08/D-09 measure-then-pin protocol;
  D-11's cross-rate discipline applies (44.1 kHz binding, 48/96 kHz regression).
- **D-07: In a sync sample the nine free-run sites evaluate at the POST-reset phase, and any site the
  reset JUMPED OVER is suppressed for that sample.** Advance and wrap normally, overwrite `phase`
  with the fractional overshoot, then evaluate the naive sample and `blep.step` there. **The
  suppression rule is a named deliverable, not an implementation detail.**
- **D-08: BLEP only — no polyBLAMP on the sync path.** The BLAMP is the documented FIRST escalation.
- **D-09: SC-3's "≥1 sync event within a single sample" is discharged by ASSERTING THE DETECTOR'S
  STRUCTURAL CEILING**, not by claiming coverage that cannot exist: every observable edge fires
  exactly once, missed edges are missed identically at all three sample rates, output stays bounded
  and finite. **This phase requires no blocking pre-planning document edit.**
- **D-10: SC-3's instrument is a MEASURED per-sample delta bound, paired with a naive-vs-corrected
  anti-circularity delta.** Sweep a sync grid, record the worst `|x[n] − x[n−1]|` on reset samples,
  pin **outward**, prove sensitivity with a **discriminating mutation probe**. Assert
  `uncorrectedResetDelta − correctedResetDelta >= margin` — a comparison of two measurements
  consulting no pinned number. Time-domain by necessity (register item 5).
- **D-11: Sync gets its own sub-grid in `tests/test_vco_spectrum.cpp`.** Classic sync sweep at several
  master/slave ratios × the five shape centres × character **0** and **1**, at **44.1 kHz binding
  with 48 and 96 kHz as regression**, on **bin-centred frequencies with integer cycles per block**
  (test frequencies sit at bin centres rather than exact equal-tempered notes, **and that must not be
  "fixed"**). Per-cell thresholds pinned from measurement, each carrying its measured justification.
  **Register item 8 binds it**; `kThresholdFloorDb = −75 dB` still bounds how tight any threshold can be.
- **D-12: PITCH-04 is re-confirmed with SYNC AS A THIRD INPUT CLASS on Phase 31's existing case**, and
  the requirement is re-ticked **only where sync is observed FIRING behind the claim**. It is also
  where the phase's **NEW DIVISOR** is exercised. Guards use the **negated-comparison idiom**, never
  `forge::clamp`.
- **D-13: The A/B renderer is BUILT IN THIS PHASE.**
- **D-14: Two legs — the shipped band-limited sync, and the same reset with the sync BLEP withheld.**
  Rendered from the **same driver in the same pass** at the D-11 sync sub-grid points.
- **D-15: `.wav`, generated on demand, NOT committed.** A `make` target the operator runs before the
  UAT session, writing to a gitignored output directory.
- **D-16: Built REUSABLE — parameterised by grid and by a pair of core configurations.**
- **D-17: All four hard-won precedents apply to this phase's in-Rack checkpoint.** (1) a blocking
  `.continue-here.md` written before the UAT plan; (2) the full expected-results block presented
  BEFORE the operator replies; (3) perceptual coverage the script cannot evidence is REFUSED, not
  booked; (4) the session protocol names the plugin DIRECTORY as well as the module, and the install
  is refreshed by a **whole-tree `rsync -a dist/ForgeAudio-AnalogSeries/`**.
- **D-18: `res/AnalogVCO.svg` gains one SYNC jack position; the shell reads the voltage and does
  nothing else.** `src/AnalogVCO.cpp`'s *"THIS FILE DOES NO DSP"* banner holds. **`src/AnalogLFO.cpp`
  must remain absent from this phase's diff.**

### Claude's Discretion

- The exact master/slave ratio set, note grid, block lengths and cycle counts for the D-11 sync
  sub-grid — within D-10/D-11-of-Phase-32's bin-centred integer-cycle and cross-rate constraints.
- The mechanism by which the sync BLEP is withheld for D-14's second leg (a flag, a second entry
  point, or a test-only shim) — the same latitude Phase 32's D-08 gave the naive path.
- Whether the D-07 suppression rule is expressed as a per-site predicate or as a swept-interval test
  — subject to the constraint that it must be **recomputed per sample and never cached**.
- Whether the previous-sync-voltage store is reset, held, or invalidated on a sample-rate change —
  provided the choice is stated and asserted rather than inherited.
- The renderer's file naming, output directory, and `make` target name (D-15/D-16).
- Whether the sync sub-grid lands as new `TEST_CASE`s in `tests/test_vco_spectrum.cpp` or a new TU —
  **if a new TU, it costs an explicit `check_includes.sh` `VCO_SIDE_ALLOW` entry, and that must be a
  plan task with its own rationale, not a gate-time discovery.**

### Deferred Ideas (OUT OF SCOPE)

- A cosine-crossfade third A/B leg — *Resolve at: the first UAT where the operator's verdict on smear
  is equivocal.*
- Applying the A/B renderer to Phase 32's morph pair — *Resolve at: any later phase, or Phase 36.*
- polyBLAMP on the sync path (D-08) — *Resolve at: the first plan that misses a sync threshold.*
- Four-point (quintic) polyBLEP/polyBLAMP (register item 10) — *Resolve at: an OPERATOR DECISION with
  an impact assessment, never a silent implementation choice.*
- The narrow-pulse "reach" refinement (register item 9) — *Resolve at: the first plan that misses a
  pulse threshold.*
- A full `MorphBlep.hpp` banner/contract audit — *Resolve at: any phase that adds a third `MorphBlep`
  call site.*
- CHARACTER CV + attenuverter → Phase 34; the output stage, drift and DRIFT-03 → Phase 34;
  per-instance seed entropy + patch persistence → Phase 34/35; FM DEPTH affordance and COARSE snap →
  Phase 35; the spectral column's cross-toolchain portability → Phase 36; `plugin.json` version →
  Phase 36.
- The shipped Analog LFO's shared latent UB → **NO PHASE**, deliberately unowned. **Direct
  consequence that binds this phase: no permanent repo-wide UBSan/ASan gate may be adopted, so Task
  1's ASan RED stays a scoped one-shot probe.**
- The `dt = 0.0005` resonant-tiling miss (item 22) and the unreachable `dt <= 1` upper guard (item 23)
  — two live tripwires.
- The second, stale `ForgeAudio` plugin directory in the operator's Rack tree (item 25).
- *Reviewed Todos (not folded):* wiring `tests/check_docs.sh` into CI → **Phase 36, carried forward,
  not re-litigated.**
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| **SYNC-01** | Hard sync input resets oscillator phase on a master rising edge | §"The Sync Path, Sample by Sample" gives the exact ordering; §"Common Pitfalls" 4–7 give the four guard landmines on the new divisor and the trigger's state machine; §"D-09" gives the structural-ceiling assertion. `forge::SchmittTrigger` is called as-is from the frozen `RackCompat.hpp:46-58` `[VERIFIED: source read]`. |
| **SYNC-02** | Sync reset uses sub-sample fractional placement plus a sync-BLEP (click-free), reusing the anti-aliasing machinery — not the LFO's 3 ms cosine crossfade | §"D-06 Placement Measurement" specifies the apparatus, grid, metric, decision rule and per-candidate prediction, and reports an in-session prototype measurement over 135 cells. §"Pitfall 1" measures the snap-to-zero landmine (4.5–5.0 dB) so the *sub-sample* half of SYNC-02 becomes an assertable claim rather than an inherited warning. §"Pitfall 2" catches the sign inversion in the project's own prior research. |
| **PITCH-04** *(re-confirmation, D-12)* | Extreme pitch × extreme FM × hostile sync, non-vacuously | §"Pitfall 5/6/7" trace the exact reachability of the divide-by-zero and the NaN path through `f`, including the finding that a NaN **cannot fire the trigger** but **can** poison `phase` on the next finite sample. |
</phase_requirements>

---

## Summary

This phase is a small amount of code standing on a very sharp analytical question, and the question
has a measurable answer. `forge::MorphBlep::addStep(xAhead, jump)` places a two-sample polyBLEP
residual on the **current and next** samples for an edge that is **ahead**. A Schmitt-detected sync
edge is always **behind** — it happened between sample *n−1* and sample *n*, and *n−1* is gone. The
2-point polyBLEP residual `r(x)` has **symmetric support on `[−1, 1]`** (`MorphBlep.hpp:195-199`), so
for a past edge exactly half the correction — the `+jump·(1−f)²/2` that belonged on sample *n−1* — is
structurally unrecoverable without the output delay buffer Phase 32 D-13 forbids. This is the real
content of D-06, and it is also why minBLEP-based designs (VCV Fundamental) do not have this problem:
a minimum-phase kernel puts all of its correction *after* the edge. AA-05 forbids minBLEP by name, so
the phase must take the one recoverable half — and the measurement says taking the *wrong* half is
worse than taking none.

An in-session prototype (135 measurements: 3 master edge shapes × 3 sample rates × 5–6 slave
frequencies, C++/double, `-ffp-contract=off`, the repo's own alias metric and block construction)
gives a decisive, falsifiable prediction: **candidate (b) — the past-edge residual `−f²/2·jump` on the
current sample and nothing pending — wins, candidate (a) is worse than applying no sync BLEP at all
in 50 of 54 cells, and candidate (c) is worse still.** The a-vs-b penalty is rate-dependent in exactly
the way D-06 anticipates (0.90 / 0.78 / 0.33 dB at 44.1 / 48 / 96 kHz on the same cell), which
confirms it is a *placement* error and confirms 44.1 kHz as the binding rate. Two further results
matter more than the ranking: **the spectral instrument barely discriminates the sync BLEP from no
sync BLEP at all** (mean improvement ≈ 0.5 dB) — so a gate written as "the sync BLEP improves the
alias floor by N dB" will fail — while the **snap-to-zero landmine measures 4.5–5.0 dB**, which makes
the *sub-sample* half of SYNC-02 the half the spectral grid can actually evidence. D-10's time-domain
instrument is therefore not a supplement; it is the only place the sync BLEP's own evidence can live.

Three code-level findings sit alongside the measurement and each is worth a plan task. First,
candidate (b) is reachable **with zero header change** as `addStep(0.f, -f*f*jump)`, which resolves
register item 13's contested claim in item 13's favour — but the explicit additive entry point is
clearer and D-06 permits it. Second, `f` is **not** confined to `[0,1]` by arithmetic alone, and
`f = 1.0` exactly — reachable from any master whose sample lands exactly on `1.0 V` — sets
`phase = 0`, which is the `STACK.md:149` landmine firing through the front door. Third, a NaN sync
voltage **cannot fire the trigger** (all three of `SchmittTrigger::process`'s comparisons are false
for a NaN) but **can** be stored as `prev`, so the *next* finite crossing computes `f = NaN` and
writes it into `phase` — which has **no guard today** (`VcoCore.hpp:539-540` guards `deltaPhase`, not
`phase`) and is therefore permanently poisoned, exactly the shape plan 32-05 measured for `dt`.

**Primary recommendation:** run the D-06 measurement as specified in §"D-06" before any seam code is
written, expect candidate (b), implement it as an additive past-edge entry point on `MorphBlep`
(or `addStep(0.f, -f*f*jump)` if the plan prefers zero header change beyond Task 1), guard `f` with a
negated-comparison pair whose fallback is **not** `f = 1`, and put SC-3's evidence in the time domain
where register item 5 already proved the spectral instrument is blind.

---

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Reading the SYNC jack voltage and its connected state | Rack shell (`src/AnalogVCO.cpp`) | — | D-18: the shell marshals Rack I/O into the POD and does no arithmetic; its own banner says so. |
| Rising-edge detection (`SchmittTrigger`, 0.1/1.0 V) | DSP core (`VcoCore.hpp`) | — | D-02: edge detection is DSP, and core ownership is what makes SYNC-01 assertable headlessly through `VcoBlockDriver`. A shell-side trigger hands the core a decided boolean and no test can see a mis-detected edge (the register item 24 vacuity shape). |
| Sub-sample crossing estimate `f`, and its guard | DSP core (`VcoCore.hpp`) | — | D-01/D-12. The new divisor lives beside the store it divides; it must not be split across the boundary. |
| Phase reset to `(1−f)·deltaPhase` | DSP core (`VcoCore.hpp:539-541` region) | — | D-01/D-07: the reset overwrites the accumulator immediately after the normal advance-and-wrap. |
| Sync jump magnitude `h = after − before` | DSP core (`VcoCore.hpp`) | frozen `Waveshape::morphedWave` (called, never edited) | D-05: one extra `morphedWave` call at the post-reset phase, on sync samples only. |
| Placing the band-limited correction | `MorphBlep` (the seam) | DSP core supplies `f` and `h` | D-06/D-14 of Phase 32: the seam owns the residual algebra; the core owns the weights and positions it hands in. |
| Free-run site suppression across a reset | `MorphBlep::step` site loop, driven by the core's post-reset phase | — | D-07: recomputed per sample, never cached (Phase 32 D-04). |
| Sync alias-floor gating | `tests/test_vco_spectrum.cpp` | — | D-11: extend the standing apparatus, never build a second instrument. |
| SC-3 continuity gating | `tests/test_vco_core.cpp` (or the sync TU) | — | D-10: time-domain, because register item 5 measured that single-sample full-amplitude spikes read 0.0 dB spectrally. |
| Perceptual A/B evidence | `tools/` renderer + a `make` target | `NaiveVcoCoreMirror` precedent | D-13..D-16: an on-demand, uncommitted `.wav` pair from the same driver in the same pass. |
| Panel affordance | `res/AnalogVCO.svg` | — | D-18: one jack position; the SVG is replaced wholesale in Phase 35. |

**Tier assignment to sanity-check in review:** nothing in this phase belongs in `src/AnalogLFO.cpp`,
in any of the four frozen shared headers, or in the shell beyond two POD assignments.

---

## D-06: The Placement Measurement — Specification

> This is the phase's research flag. D-06 asks for the measurement to be specified precisely enough
> that a plan can execute it — apparatus, grid, metric, decision rule, and per-candidate prediction —
> **not** for a winner picked by argument. The prediction below comes from an actual prototype run in
> this session; the **binding** measurement is the in-repo one against the real `forge::VcoCore`.

### The problem, restated in the code's own terms

`MorphBlep::step`'s site loop and `addStep` are the *same* placement convention, and this is pinned by
test (`tests/test_morph_blep.cpp` case five parts A and B): a site `s` samples **ahead** contributes
`+h·(1−s)²/2` to the current sample and `−h·s²/2` to the next `[VERIFIED: MorphBlep.hpp:257-262
vs :530-533]`. That is `r(−s)` and `r(1−s)` under the banner's residual
`r(x) = (x+1)²/2` before the edge, `r(x) = −(x−1)²/2` after it, **zero outside `[−1, 1]`**
`[VERIFIED: MorphBlep.hpp:195-199]`.

Now place the sync edge. D-01 defines `f` as the fraction of the way from sample *n−1* to sample *n*
at which the master crossed HIGH, so the edge is at absolute time `(n−1) + f`. Therefore:

| sample | position `x` relative to the edge | ideal residual | available? |
|--------|-----------------------------------|----------------|------------|
| *n−1*  | `−f`      | `+h·(1−f)²/2` | **no — already emitted** |
| *n*    | `+(1−f)`  | `−h·f²/2`     | yes |
| *n+1*  | `+(2−f)`  | `0` (outside support) | n/a |

So the ideal two-sample correction for a past edge is **one-sided by the time you can act**, and its
recoverable half is exactly candidate (b)'s `−f²/2·jump`. This is a property of the *kernel*, not of
the implementation: the 2-point polyBLEP residual straddles the edge symmetrically, whereas a
minimum-phase kernel (minBLEP, which AA-05 forbids by name) places all of its correction after the
edge and so has no forfeited half. **That is the whole of D-06's tension, and it is why register item
13's "no header change is needed" was conditional rather than false.**

**A finding that resolves item 13 in its favour.** Candidate (b) is reachable through the *existing,
pinned* `addStep` with **no header change at all**:

```
addStep(0.f, -f*f*jump)
  ->  u = 1 - 0 = 1
  ->  inject  += (-f*f*jump) * 0.5 * 1 * 1   ==   -f*f/2 * jump      // exactly candidate (b)
  ->  pending += (-f*f*jump) * -0.5 * 0 * 0  ==   0                  // nothing owed forward
```

`[VERIFIED: algebra against MorphBlep.hpp:257-262]`. The `xAhead` gate passes (`0 >= 0` true,
`0 > 1` false), so the `[0,1]` contract is **honoured, not reinterpreted**. Whether this counts as
within the documented `jump` contract is a judgement call the plan should make explicitly: the banner
says *"`jump` is (value_after − value_before), **already scaled by whatever weights the caller
owns**"* `[VERIFIED: MorphBlep.hpp:242-243]`, which textually permits a caller-owned scale factor. The
**recommended** form is nevertheless the explicit additive entry point D-06(b) contemplates
(e.g. `void addPastStep(float xBehind, float jump)`), because a `-f*f*jump` argument at the call site
is opaque and invites a later "simplification" back to `addStep(f, jump)` — i.e. straight into
candidate (a). Both forms are numerically identical; choose on legibility, and note that the
zero-header-change form is the fallback if the plan wants Task 1 to be the only `MorphBlep.hpp` touch.

### Apparatus — extend, do not rebuild

Reuse verbatim from `tests/test_vco_spectrum.cpp`: `fftRadix2`, `aliasPeakDb`, `impliedLeakageDb`,
`driveSecondBlock`, `kSpectrumN = 4096`, the one-block warm-up discard, the four seed literals
(`0x1234 / 0x5678 / 0x9E3779B9 / 0x7F4A7C15`), `base.drift = 0.f`, and `NaiveVcoCoreMirror`
`[VERIFIED: source read of tests/test_vco_spectrum.cpp:86-1055]`.

**The one structural change the sync sub-grid needs, and its derivation.** *The fundamental bin is the
MASTER's, not the slave's.* Under hard sync the slave's whole trajectory is determined by the master,
so the ideal continuous-time output is exactly periodic at the master's period `P`. With
`K_m` master cycles per `N`-sample block, `P·f_s = N/K_m` samples, so every true harmonic of the ideal
band-limited signal lands exactly on bin `n·K_m`. Everything off that lattice is alias energy — the
same coprimality argument the file banner already makes, with `K_m` substituted for `K`. Consequences:

1. Call the existing `aliasPeakDb(block, K_master)` unchanged, with `H = { n·K_m }`.
2. **The slave frequency does not need to be bin-centred and must not be solved for.** Neither
   `binCentredPitchCV` nor `binCentredSampleTime` is used for the slave; the slave is set by a plain
   `pitchCV` and is free. This is a simplification, not a weakening — the leakage argument attaches to
   the fundamental being measured, which is the master's.
3. **The master's bin error is exactly zero.** Generate the master inside the test with a phase
   increment of `K_m / 4096 = K_m · 2⁻¹²`, which is exactly representable in binary floating point and
   accumulates over the block with zero rounding error. ⚠️ `impliedLeakageDb(0.0)` returns the
   `−999.0` sentinel through its `!(binError > 0.0)` branch `[VERIFIED: test_vco_spectrum.cpp:406-409]`
   — semantically right here but only by accident. **Assert the exact-zero bin error directly**
   (`REQUIRE(binError == 0.0)`) rather than letting a sentinel stand in for a measurement; that is the
   same posture D-10 takes everywhere else in the file.

**Master waveform.** A ±5 V *falling* saw, `v = 5·(1 − 2·φ_m)`, whose wrap is a **rising** jump — this
is the Forge saw's own polarity (`MorphBlep.hpp:422-431` records the wrap jump as `+2.000000`), so it
models what an operator actually patches. The falling ramp re-arms the trigger by crossing `0.1 V`
downward mid-cycle and fires it at the wrap.

### Grid (D-11 sync sub-grid) — recommended

| Axis | Values | Why |
|------|--------|-----|
| Rate × master cycles | 44.1 kHz `K_m = 93` (1001.4 Hz) · 48 kHz `K_m = 85` (996.1 Hz) · 96 kHz `K_m = 43` (1007.8 Hz) | All odd (the coprimality requirement reduces to "pick an odd number"). Chosen so the master lands within 1.2 % of 1 kHz at all three rates, so the cross-rate rows compare like with like. **Record the achieved master Hz on every row** — with `N` pinned at 4096 no single master frequency is integer-cycle at all three rates below ~3.4 kHz, and that spread is a property of the instrument that must be written down, not hidden. |
| Master/slave ratio | slave ≈ **0.5×**, 1×, 2×, 3×, 4×, 6× the master | ⚠️ **The `< 1` ratio is not optional.** It is where the candidates separate by 4–7 dB; in the classic sweep region (`ratio ≥ 2`) they separate by under 1 dB, which is below register item 8's step-dominated reproduction bound and therefore cannot carry a cross-toolchain decision. |
| Morph | the five shape centres 0.00 / 0.25 / 0.50 / 0.75 / 1.00 | matches the standing grid's third index discipline (P-6). |
| Character | 0.00 and 1.00 | D-11 as written. |
| Master edge shape | **at least two**: a hard-jump saw, and a 2-point-polyBLEP'd saw | see the hazard below — this axis is what makes SYNC-02's sub-sample clause testable at all. |

### Metric

`aliasPeakDb(block, K_master)` — the existing function, unchanged. Alias RMS reported alongside as the
diagnostic, exactly as today: *the peak is the gate, the RMS is the diagnostic.*

### Candidate legs — six, all from the same driver in the same pass

| leg | what it does |
|-----|--------------|
| `none` | reset applied, sync BLEP withheld. **This is also D-14's second audition leg** — the same leg serves both purposes, which is what "pays for itself twice" buys. |
| `a` | `addStep(f, jump)` at the detection sample (zero header change, one sample late) |
| `b` | past-edge: current sample `+= −f²/2·jump`, nothing pending |
| `c` | `addStep(0.f, jump)` (a flat half-jump on the detection sample) |
| `oracle-b` | leg `b` with `f` replaced by the master's **true** wrap fraction, which the test's own master generator knows exactly |
| `snap` | leg `b` with the reset snapped to `phase = 0` (the `STACK.md:149` landmine, rendered as a measurement) |

Two further legs are cheap and worth adding as **mutation probes** rather than candidates:
`mis` = `addStep(1−f, jump)` (the natural mis-mapping of "the edge is `1−f` samples behind"), and
`badsign` = leg `b` with `jump` computed as `before − after` (see Pitfall 2).

### Decision rule — three conditions, all required

Evaluated at **44.1 kHz binding**, with 48 and 96 kHz as regression.

1. **Sign consistency.** The winner is ≤ every other candidate on ≥ 90 % of 44.1 kHz cells, and is
   never worse than the runner-up by more than **1.0 dB** (register item 8's step-dominated
   reproduction bound) on any cell.
2. **Margin above the reproduction bound.** The winner's margin over the runner-up **exceeds 1.0 dB
   (step-dominated) / 4.0 dB (plateau)** on at least the `ratio < 1` cells. **A decision taken only on
   cells where the separation is under 1.0 dB is not defensible cross-toolchain and must not be
   taken** — every absolute decibel figure this milestone has recorded is an Apple-clang figure
   (register item 8), and the plateau/step-dominated classification must be stated on the physical
   criterion **before** the population is enumerated, never as a rename of the cells that failed.
3. **Rate signature.** The winner's margin **shrinks** from 44.1 → 48 → 96 kHz on the `ratio ≥ 1`
   cells. That shrink is the signature of a *one-sample placement* error: the error is a fixed
   fraction of the jump delivered one sample away, so its audible weight falls as the sample period
   shortens. **A margin that is flat across rates means the legs differ in jump MAGNITUDE, not in
   placement, and the measurement has not answered D-06** — stop and report rather than picking.

The file's standing **STOP-AND-REPORT** instruction applies unchanged: if the candidate ordering
inverts between rates, or if a cell's ordering contradicts the other two conditions, report the
finding; do not resolve it by re-typing a number.

### Predicted outcome — measured in-session on a prototype

**Method.** A standalone C++11 prototype (`-O2 -ffp-contract=off`, all-double), reproducing this
repo's block construction (`N = 4096`, one warm-up block discarded, exact dyadic master increment) and
this repo's `aliasPeakDb` verbatim. The slave is a falling saw with the 2-point wrap polyBLEP —
**not** the full nine-site `morphedWave`, so absolute figures are prototype figures and must not be
copied into any threshold column. 135 measurements: 3 master edge shapes × 3 rates × 5–6 slave
frequencies × 6 legs. `[VERIFIED: measured this session]`

| leg | mean vs `b` | worst case | verdict |
|-----|-------------|-----------|---------|
| **`b`** | — | — | **predicted winner: best or tied-best in every one of 54 cells** |
| `none` | +0.5 dB | +3.9 dB | second |
| `a` | +1.2 to +1.6 dB | +7.1 dB | **predicted loser — worse than `none` in 50 of 54 cells**, by up to +4.2 dB |
| `c` | +1 to +4 dB | +8.4 dB | worst of the three named candidates |
| `mis` | between `a` and `b` | | useful as a mutation probe |
| `badsign` | +0.3 to +4.3 dB | | useful as a mutation probe |

**The rate signature, measured.** `(a − b)` on the polyBLEP'd-master rows at the same 4186 Hz slave:
**0.90 dB at 44.1 kHz, 0.78 dB at 48 kHz, 0.33 dB at 96 kHz.** The penalty roughly halves at 96 kHz.
This satisfies condition 3 and confirms 44.1 kHz as binding; a single-rate comparison would indeed
have been worthless, exactly as D-06 says.

**Why `a` is worse than doing nothing, stated so the plan can predict it rather than be surprised by
it.** Under D-07 the naive output has already stepped between *n−1* and *n*, so sample *n* is
**post-reset**. Candidate (a) adds `+h(1−f)²/2` to a post-edge sample — pushing it *further past* the
new value instead of pulling it back toward the band-limited midpoint. Per-sample error against the
ideal is `h·[(1−f)² + f²]/2`, i.e. between `h/4` and `h/2`, versus `h·f²/2` for doing nothing; the
difference `h(1−f)²/2 ≥ 0` is why `a` cannot beat `none`. This is register item 3's falsified premise
in a new costume: **a step-shaped correction added on the wrong side of a step is new broadband
energy, not a filter.**

### What else this measurement buys — "it pays for itself twice"

- **`snap` vs `b` — the `STACK.md:149` landmine, measured.** Prototype: with an informative master,
  `snap` is **4.5 dB worse at a 3136 Hz slave and 4.95 dB worse at 4186 Hz** (44.1 kHz). This turns
  SYNC-02's *"sub-sample fractional placement"* clause from an inherited warning into a permanent,
  non-circular assertion. **Recommend making it a permanent test case**, because it is the one sync
  claim the spectral instrument can evidence with a comfortable margin.
- **`oracle-b` vs `b` — residual attribution.** The gap between them is the cost of `f`'s *accuracy*;
  the gap between `b` and `none` is the cost of the *placement convention*. Prototype: with a
  hard-jump master the oracle is 3.5–5.5 dB better than `b` (so almost all the residual is `f`'s, and
  is the *master's* fault, not the VCO's); with a band-limited or ramped master the two coincide to
  ≈0.1 dB. That decomposition is precisely what tells a later phase whether to escalate to the BLAMP
  (item 9) or to accept the residual as inherent.

### ⚠️ The measurement-design hazard the grid must avoid

`f = (high − prev)/(now − prev)` recovers the master's wrap fraction **only when the master's edge
spans the threshold over ≥ 1 sample.** For a master whose wrap is a single-sample full-scale jump — a
naive saw or a gate, which is what most patched masters are — the interpolation returns the *voltage*
fraction of the threshold within the jump, which is very nearly **constant**. Worked, for a ±5 V
falling saw with an instantaneous rising wrap at `HIGH = 1.0 V` and true wrap fraction `g`:

```
prev = −5 + 10·g·dt_m ,  now = 5 − 10·(1−g)·dt_m
f = (6 − 10·g·dt_m) / (10 − 10·dt_m)  ≈  0.6 − g·dt_m
```

`g` enters only at order `dt_m`. **Prototype confirmation:** with a hard-jump master, `snap` and `b`
measure **identically** (−31.10 vs −31.10 dB at 44.1 kHz / 523 Hz slave) — the sub-sample reset buys
literally nothing, because there is no sub-sample information to use. With a polyBLEP'd master the
same pair is −32.57 vs −35.52 dB. **Therefore: at least one master edge shape in the sub-grid must
span the threshold over ≥ 1 sample**, or SYNC-02's sub-sample clause is untested and the placement
candidates' differences are swamped by a much larger `f`-estimation error.

This is not a defect in D-01 — it is the standard technique and VCV's own VCO does the same thing
(`syncSubsample = −lastSync / deltaSync`) `[CITED: github.com/VCVRack/Fundamental v2 src/VCO.cpp]`.
It is a property of the *instrument* that must be named before a gate is written against it, exactly
the register item 6 move D-09 already applies elsewhere in this phase.

### A related question raised and CLOSED by measurement — do not spend a task on it

D-05 says the pre-reset value *"reuses the naive sample already computed this step."* Under D-07's
ordering there are three defensible readings of *which* phase that value sits at: the fully advanced
pre-reset phase `φ_adv`, the previous sample's phase `φ(n−1)`, or the true edge phase
`φ(n−1) + f·dt` (which would need a second extra `morphedWave` call and so is decision-adjacent to
D-05's explicit rejection of "probing both sides"). Prototype, candidate `b`, polyBLEP'd master, 18
cells: the three readings spread by **at most 0.09 dB**, far below the 1.0 dB reproduction bound.
**D-05 as literally worded is measurably adequate; do not add a second `morphedWave` call.** Caveat:
the prototype slave carries one discontinuity site, not nine, so the in-repo measurement should carry
this as a cheap third leg on a handful of cells rather than assume it transfers.

---

## Standard Stack

**No new dependency. No package is installed by this phase.** Everything is already in-tree, and the
phase's entire "stack" decision is *which existing primitive to call and which to leave alone*.

### Core

| Component | Location | Purpose | Why standard |
|-----------|----------|---------|--------------|
| `forge::SchmittTrigger` | `src/dsp/RackCompat.hpp:46-58` — **FROZEN, call it, never edit it** | Rising-edge detection on the sync input | Already the plugin's one convention, used at both shipped LFO trigger sites. Its `UNINITIALIZED` handling is documented as load-bearing and is what makes the first sample after construction non-firing `[VERIFIED: source read]`. |
| Thresholds `0.1f, 1.0f` | `LfoCore.hpp:137`, `ClockTracker.hpp:111` | Hysteresis band | D-03. Two in-house sites, `research/STACK.md:64`'s recommendation, one convention across the plugin. |
| `forge::MorphBlep::addStep` | `src/dsp/MorphBlep.hpp:257-262` | The sync seam | Permanently pinned by `tests/test_morph_blep.cpp` case five A/B: a driven morph site at `s = 0.5` and `addStep(0.5, 2)` produce the same `+0.250000 / −0.250000` split, and events compose by summation. |
| `forge::Waveshape::morphedWave` | `src/dsp/Waveshape.hpp:158-216` — **FROZEN** | The one extra call D-05 adds, at the post-reset phase | Its result is already through the `1/(1+bleedIntensity)` divide (`:212`), so a jump computed as a difference of two of its outputs is automatically in the normalized domain — D-05's "by construction". |
| `double` phase accumulator | `VcoCore.hpp:276` | Carries `(1−f)·deltaPhase` | Already in place; the fractional overshoot needs no new precision machinery. |
| `tel.syncFired` | `VcoCore.hpp:285` | Telemetry: "a hard-sync reset fired this sample (Phase 33)" | **Already declared and reserved for this phase** — populate it; do not add a parallel field. |

### Supporting (test / tooling side)

| Component | Location | Purpose | When to use |
|-----------|----------|---------|-------------|
| `forge::VcoBlockDriver` | `tests/VcoBlockDriver.hpp` | Drives real master voltages through the whole sync path | Everywhere. D-02's raw-volts boundary means no new driver is needed. **Never template or subclass with `tests/BlockDriver.hpp`.** |
| `driveSecondBlock` | `tests/test_vco_spectrum.cpp:937-949` | Warm-up-and-measure loop | The spectral sub-grid and the renderer. Do **not** fork it into near-copies. |
| `aliasPeakDb` | `tests/test_vco_spectrum.cpp:186-223` | The alias metric | Called with `K_master` for sync cells. |
| `NaiveVcoCoreMirror` | `tests/test_vco_spectrum.cpp:463-584` | Bit-exact naive baseline | Only if the renderer needs a second code path — see §"Don't Hand-Roll" for a way to avoid needing one. |
| ASan (`clang++ -fsanitize=address`) | available on this host `[VERIFIED: probed]` | Task 1's CR-01 RED | **Scoped one-shot probe only.** Register item 12 forbids a permanent repo-wide sanitizer gate. |

### Alternatives Considered

| Instead of | Could use | Tradeoff |
|------------|-----------|----------|
| Additive `addPastStep` entry point on `MorphBlep` | `addStep(0.f, -f*f*jump)` — numerically identical, zero header change | Zero header change (keeps Task 1 as the only `MorphBlep.hpp` touch, and vindicates register item 13) versus legibility. The scaled-`jump` form invites a later "simplification" into candidate (a), which the measurement says is worse than doing nothing. Recommend the named entry point; the trick is a valid fallback. |
| 2-point polyBLEP for the sync step | minBLEP (minimum-phase, no forfeited half) | **Forbidden by AA-05 by name.** Worth recording *why* it would help here specifically: a minimum-phase kernel places all correction after the edge, so a past edge costs it nothing. |
| One-sided (b) correction | Full two-sample correction via a one-sample output delay buffer | **Forbidden by Phase 32 D-13**, which rejected it partly citing this phase. Prototype quantifies the cost of the constraint: the ideal two-sided correction beats `b` by only **0.2–0.6 dB** at `ratio ≥ 2`, but by **4.6–6.0 dB** at `ratio < 1`. The constraint is cheap where sync is normally used. |
| Time-domain SC-3 gate | Spectral-only SC-3 gate | Rejected on measured grounds by register item 5 (single-sample full-amplitude spikes read 0.0 dB spectrally) — and independently confirmed here: the sync BLEP's own spectral improvement averages ≈0.5 dB, so the spectral instrument has almost no discriminating power for it. |

**Installation:** none.

---

## Package Legitimacy Audit

**Not applicable — this phase installs no external packages.**

Every component named in §"Standard Stack" is an existing in-repo header or an existing test fixture,
verified by direct source read this session. There is no npm/PyPI/crates dependency, no vendored
third-party addition, and no build-tool change beyond one new `make` target and (if a new TU lands)
one `VCO_SIDE_ALLOW` line. The project's dependency posture is deliberate: `src/dsp/*.hpp` carries
**zero** Rack-SDK includes, and the test target links nothing outside the Rack-free core.

| Package | Registry | Verdict | Disposition |
|---------|----------|---------|-------------|
| *(none)* | — | — | — |

**Packages removed due to `[SLOP]` verdict:** none.
**Packages flagged as suspicious `[SUS]`:** none.

---

## Architecture Patterns

### System architecture — one sample through the synced core

```
                  Rack SYNC jack           Rack V/OCT, FM, MORPH knobs
                        |                              |
                        v                              v
        src/AnalogVCO.cpp  -- assigns syncVolts, syncConnected (NO arithmetic, D-18) -->
                                    forge::VcoInputs (POD)
                                              |
                                              v
                              +-------------------------------+
                              |   forge::VcoCore::step()      |
                              +-------------------------------+
                                              |
        (1) pitch volts -> guard -> ONE exp2_taylor5 -> freq -> Nyquist ceiling -> floor
                                              |
        (2) deltaPhase = freq * dt -> floor -> ceiling            [unchanged]
                                              |
        (3) phase += deltaPhase ; single-subtract wrap            [unchanged]
                                              |
                                        phi_adv (pre-reset)
                                              |
        (4) SYNC BLOCK  --- syncConnected? ---+--- no ---> (store prev, skip) ----+
                |  yes                                                            |
                v                                                                 |
          trig.process(syncVolts, 0.1f, 1.0f)  ---- false ----> (store prev) -----+
                | true                                                            |
                v                                                                 |
          f = (1.0f - prev) / (syncVolts - prev)                                  |
          GUARD f  (negated pair; fallback MUST NOT be 1.0)   <<< new divisor >>> |
                |                                                                 |
                v                                                                 |
          before = morphedWave(phi_adv)          [the sample already computed]    |
          phase  = (1 - f) * deltaPhase          [NEVER exactly 0]                |
          after  = morphedWave(phase)            [ONE extra call, D-05]           |
          h      = after - before                [SIGN: after MINUS before]       |
          seam:  addPastStep(f, h)   ==   addStep(0.f, -f*f*h)                    |
          tel.syncFired = true                                                    |
                |                                                                 |
                +---------------------------------+-------------------------------+
                                                  |
        (5) p = (float)phase   [POST-reset]       v
        (6) naive = morphedWave(p, morph, character, 0.f)     [the emitted value]
        (7) correction = blep.step(wave, phase, p, deltaPhase, morph, character)
                          |
                          +-- drains inject (the sync residual just deposited)
                          +-- drains pending (the second half of LAST sample's edge)
                          +-- nine free-run sites, evaluated at the POST-reset phase (D-07)
                                                  |
        (8) return 5.f * (naive + correction)     v      [unconditioned, Phase 34 owns OUT-*]
```

Two ordering facts a plan must not invert:
- The sync block sits **between** the wrap (step 3) and the `p` snapshot (step 5). `p`, `naive` and
  `blep.step` all see the **post-reset** phase. This is D-07.
- `blep.step` is still called **exactly once** per sample, with the same `p` handed to `morphedWave`
  and the `double phase` handed over separately. That split is load-bearing (`VcoCore.hpp:613-630`).

### Pattern 1 — the sync jump's sign

```cpp
// h is (value_after - value_before), matching MorphBlep's documented convention
// (MorphBlep.hpp:188-204). `before` is the naive value at the PRE-reset advanced
// phase; `after` is the ONE extra morphedWave call D-05 pays for.
const float h = after - before;
```

⚠️ `research/STACK.md:124` writes the same quantity as `out_preReset − morphedWave(newPhase)`, which
is `before − after` — **the negation**. Transcribing that expression into `addStep` inverts the
correction into an anti-correction. See Pitfall 2.

### Pattern 2 — the guarded sub-sample solve

```cpp
// The negated comparison FIRST, exactly as VcoCore.hpp:597-602 and MorphBlep.hpp:180 do.
// NEVER forge::clamp: both of its comparisons are false for a NaN, so it is inert
// against the one input class this guard exists to stop.
float f = (1.0f - prevSyncVolts) / (in.syncVolts - prevSyncVolts);
if (!(f >= 0.f) || !(f < 1.f)) f = 0.f;   // fallback is 0, NOT 1 — see Pitfall 4
phase = (double)(1.f - f) * deltaPhase;   // strictly positive; never exactly 0
```

Three properties, each of which a test should pin:
- `!(f >= 0.f)` is **true** for a NaN, so a NaN `f` lands on the fallback and never reaches `phase`.
- The upper bound is **strict** (`f < 1.f`, not `f <= 1.f`) so `phase == 0.0` is unreachable.
- The fallback `f = 0` means "treat the edge as coincident with the previous sample", giving
  `phase = deltaPhase`. Any fallback is acceptable **except** `f = 1`.

### Pattern 3 — the previous-voltage store's invariant

```cpp
// UNCONDITIONAL, every sample, whatever syncConnected says.
prevSyncVolts = in.syncVolts;
```

The invariant is: **`prevSyncVolts` is the voltage the trigger saw on the immediately preceding
sample.** It is the *only* thing that makes `now − prev == 0` unreachable (see Pitfall 5). A store
gated on `syncConnected`, or skipped on any branch, breaks it and re-opens the divide-by-zero.
D-02's discretion item — reset / hold / invalidate on a sample-rate change — should be resolved in
favour of whatever preserves this invariant, and the choice must be **stated and asserted**.

### Pattern 4 — D-07's suppression, and where it actually bites

D-07's rule as written ("any site the reset jumped over is suppressed for that sample") is, at the
current-sample site loop, **satisfied by the ordering alone**: after the reset the phase is
`(1−f)·deltaPhase < deltaPhase`, and the site loop only fires sites within `dt` *ahead* of that
(`s = d/dt`, `if (!(s <= 1.f)) continue;`, with the `d += 1.0` wrap making a behind-site's `s` huge)
`[VERIFIED: MorphBlep.hpp:508-511]`. A jumped-over site is behind and cannot fire.

⚠️ **Where the phantom actually lives is the previous sample's `pending`.** At sample *n−1* the loop
fired every site within `dt` ahead and deposited both halves. Sites at `s ∈ (f, 1]` were never
traversed — the reset jumped them — so *both* halves are phantom, and the `pending` half is still
sitting in the accumulator when sample *n* drains it. The window is `(1−f)·dt` wide, so at C7
(`dt ≈ 0.048`) with a handful of live sites this is on the order of one sync event in five, not a
corner case.

**`pending` is a scalar sum, so retroactive per-site cancellation is not possible without recording
per-site contributions** `[VERIFIED: MorphBlep.hpp:233]`. Three dispositions, in the plan's gift:

1. **Accept and document.** The prototype's `b` leg carries this phantom and still wins every cell;
   the effect is partly self-cancelling because `before = morphedWave(φ_adv)` is taken *past* the
   jumped-over site, so `h` already contains the site's step with the opposite sign.
2. **Express D-07's rule as a swept-interval predicate at sample *n−1*** — impossible, since the sync
   is not yet known at *n−1*.
3. **Restructure `MorphBlep` to carry per-site pending** — a substantial change to the header the
   phase is otherwise only hardening. Out of proportion.

**Recommendation: disposition 1, made explicit.** Write the suppression rule as the *ordering* claim
it actually is ("free-run sites are evaluated at the post-reset phase, which structurally cannot fire
a jumped-over site"), name the residual `pending` phantom in the same comment, and add it to the
deferred register rather than to the implementation. That is an honest discharge of D-07; silently
implementing a per-site predicate that the ordering already provides would be a rule that gates
nothing.

### Anti-patterns to avoid

- **Calling `addStep(f, jump)` at the detection sample without measuring.** The phase's central
  question, skipped. Predicted to be worse than applying no correction at all.
- **Routing sync through the LFO's cosine crossfade** (`LfoCore.hpp:134-141`). Forbidden by SYNC-02 by
  name; 3 ms is ≈130 samples at audio rate and smears the buzz away.
- **Snapping `phase = 0`.** Measured at 4.5–5.0 dB worse. Note it is reachable *by arithmetic*, not
  only by choice — see Pitfall 4.
- **Making `VcoBlockDriver`'s per-sample `sampleTime`/`sampleRate` overwrite conditional.** It is
  documented as load-bearing and re-opens the R-2 / P-4 driver-independence argument.
- **A second measurement function for the corrected sync path.** The naive/corrected delta must be a
  like-for-like comparison through the same `measureCellDb`-shaped function.
- **Bin-centring the *slave*.** Under sync the slave's frequency is not the fundamental; solving for it
  wastes effort and implies a leakage claim that does not attach there.
- **Caching site geometry across the reset.** Phase 32 D-04 — and Phase 34's drift will move every
  position per sample.

---

## Don't Hand-Roll

| Problem | Don't build | Use instead | Why |
|---------|-------------|-------------|-----|
| Rising-edge detection with hysteresis | A `bool wasHigh` latch | `forge::SchmittTrigger` (frozen, `RackCompat.hpp:46-58`) | Its three-state `UNINITIALIZED` handling is documented as load-bearing and is what stops a spurious fire on the first sample. A hand-rolled latch gets that wrong silently. |
| The band-limited step residual | A second polyBLEP formula on the sync path | `MorphBlep`'s existing residual algebra, via the seam | `MorphBlep.hpp:206-212` records that the widely published two-branch form returns **twice** this residual and is applied at **half** the jump. A second, independently transcribed kernel is a factor-of-two bug waiting to happen — the header even documents the reconciliation. |
| Bleed normalization for the sync jump | A `1/(1+bleedIntensity)` correction factor | Nothing — `morphedWave` already applies it (`Waveshape.hpp:212`) | D-05: a difference of two `morphedWave` outputs is already in the normalized domain. Applying a second factor over-corrects by exactly `(1 + bleedIntensity)`. |
| A NaN-safe range check | `forge::clamp(f, 0.f, 1.f)` | The negated-comparison pair | `forge::clamp` is `x < lo ? lo : (x > hi ? hi : x)` — **both** comparisons are false for a NaN, so it is inert against exactly the input class the guard exists to stop `[VERIFIED: RackCompat.hpp:97]`. |
| A withheld-BLEP audition leg | A second core, a mirror, or a `bool bandLimit` flag in `forge::VcoCore` | **Subtract a recording-only telemetry float.** Under candidate (b) the sync correction deposits **nothing** into `pending`, so it is purely additive per-sample: `leg_none[n] == leg_full[n] − syncCorrection[n]` **exactly**. One recording-only `float` on `Telemetry` (which is already documented as "NOT part of the audio path") makes the withheld leg reconstructible from the *same pass*, satisfying register item 26's like-for-like constraint by construction, with no mirror to maintain and no flag in the shipped core. ⚠️ This shortcut is **specific to candidate (b)**; under (a) or (c) the correction touches `pending` and the reconstruction needs both halves. |
| A `.wav` writer | A dependency | ~30 lines of RIFF header + PCM | A 16-bit PCM WAV header is 44 bytes of fixed layout. No package is justified for it, and adding one would be this phase's only external dependency. |
| An "is this cell plateau or step-dominated" judgement | A post-hoc rename of the cells that failed | The physical criterion, stated **before** the population is enumerated | Register item 8, binding. |

**Key insight:** every primitive this phase could plausibly hand-roll already exists in a *frozen*
header, which means hand-rolling it is not merely duplication — it is a second, unpinned copy of a
byte-pinned behaviour, sitting next to the one the shipped LFO depends on.

---

## Common Pitfalls

### Pitfall 1: Snapping the reset to exactly `phase = 0` — and it is reachable by arithmetic

**What goes wrong:** the sub-sample timing is discarded and sync aliases even with a correct BLEP.
`research/STACK.md:149` names it; register item 13 makes it binding.
**Measured cost:** 4.5 dB at a 3136 Hz slave and 4.95 dB at 4186 Hz (44.1 kHz, informative master),
prototype `[VERIFIED: measured this session]`.
**Why it happens — the non-obvious half:** `f = (high − prev)/(now − prev)` equals **exactly 1.0**
whenever `now == high` exactly, i.e. whenever a master sample lands precisely on 1.0 V. A gate output
that idles at exactly 1.0 V, or a quantised CV, does this routinely — it is not a measure-zero event.
`phase = (1 − 1)·deltaPhase = 0`, and the landmine fires without anyone having chosen to snap.
**How to avoid:** bound `f` **strictly** below 1 (`!(f < 1.f)` in the negated guard), and make the
guard's fallback anything except 1.
**Warning signs:** a sync cell whose alias floor is ~5 dB worse than its neighbours at one specific
master amplitude; a `snap`-vs-`b` regression test that passes for the wrong reason.

### Pitfall 2: The project's own prior research states the jump with the opposite sign

**What goes wrong:** `research/STACK.md:124` prescribes *"a polyBLEP scaled by `out_preReset −
morphedWave(newPhase)`"* — that is `before − after`, while `MorphBlep`'s documented convention is
`h = value_after − value_before` (`MorphBlep.hpp:188-204`). Transcribing STACK.md's expression
verbatim into the seam inverts the correction: instead of pulling the stepped sample toward the
band-limited midpoint it pushes it away, doubling the artefact.
**Why it happens:** STACK.md is a canonical reference this phase is instructed to read, and the
expression looks authoritative. Both documents are internally consistent; only the composition is wrong.
**How to avoid:** write `h = after - before` with the convention quoted in the comment, and add the
inverted-sign leg (`badsign`) to the D-06 mutation probes — prototype shows it costs 0.3–4.3 dB, so
the probe discriminates.
**Warning signs:** the corrected leg measures *worse* than `none` across the whole grid rather than on
scattered cells.

### Pitfall 3: Expecting the spectral grid to show the sync BLEP working

**What goes wrong:** a plan writes an "improvement" gate in the shape of Phase 32's
`naiveDb − correctedDb >= 8.0` and it cannot be met.
**Measured:** the sync BLEP's mean spectral improvement over `none` is **≈0.5 dB**, and it is better
in only 6–11 of 18 cells per master shape `[VERIFIED: measured this session]`. The forfeited pre-edge
half is why: the recoverable correction is one-sided.
**How to avoid:** put the sync BLEP's own non-circular evidence in D-10's **time domain**, where the
anti-circularity comparison `uncorrectedResetDelta − correctedResetDelta >= margin` has real margin.
Use the spectral grid for what it *can* evidence: the ranking of the placement candidates, the
snap-to-zero landmine, and per-cell floor thresholds.
**Warning signs:** a gate that has to be loosened repeatedly; a threshold and its `measuredDb`
drifting together.

### Pitfall 4: SC-3's bound is a measured envelope, not a smallness claim

**What goes wrong:** SC-3 says "no full-scale artifact", and a plan reads that as "the per-sample step
must be small". It cannot be. A legitimate hard-sync reset at a slave at or below the master's rate
genuinely steps the output by nearly its full peak-to-peak range in one sample.
**Measured:** prototype worst `|x[n] − x[n−1]|` on reset samples ranges from **0.22 V to 5.23 V**
across the grid; the `uncorrected − corrected` margin ranges from **0.003 V to 1.00 V** and is
positive in all 45 cells measured `[VERIFIED: measured this session]`.
**How to avoid:** pin the absolute bound **outward from measurement**, per D-10, and let the
anti-circularity comparison carry the evidence. ⚠️ Note the margin can be as small as 0.003 V, so the
pinned `margin` must come from the **minimum over the gated cells, rounded outward**, and if the plan
wants a larger margin it must restrict the assertion to cells where the correction bites — with the
restriction stated on a physical criterion **before** the population is enumerated.
**Also:** `tests/test_vco_core.cpp:511`'s two measured output tiers (`kHostileBoundV = 10.0`,
`kMusicalBoundV = 5.55`) must be **re-derived for sync, not assumed**. The prototype already reaches
±5.2 V from a bare saw slave with no character; audio-rate MORPH already reaches 6.289864 V.

### Pitfall 5: The divide-by-zero is unreachable in steady state — and reachable through state management

**What goes wrong:** `f = (high − prev)/(now − prev)` divides by zero when two consecutive samples are
equal (D-12).
**The reachability analysis, done:** when `SchmittTrigger::process` returns `true`, the state was
`LOW`, which means the previous sample did **not** satisfy `in >= highThreshold` (or it would have
fired then). So `prev < 1.0 <= now`, hence `now − prev > 0` and `f ∈ (0, 1]` **by construction**
`[VERIFIED: RackCompat.hpp:50-56]`. **The zero divisor is unreachable while `prev` is genuinely the
previous sample's voltage.**
**Therefore the defect, if it appears, is a state-management defect:** a `prevSyncVolts` updated
conditionally (gated on `syncConnected`, skipped on a branch, or stale across a
patch/sample-rate/reset event) can be *anything*, including exactly `now`. Plug a cable carrying a
steady 5 V into a core whose trigger is `LOW` and whose store is stale at 5 V, and you get `0/0`.
**How to avoid:** update the store unconditionally every sample (Pattern 3), and assert the invariant
directly rather than asserting `f` is finite after the fact.

### Pitfall 6: A NaN cannot fire the trigger, but it can poison `phase` one sample later

**What goes wrong — the exact path:** a NaN `syncVolts` makes all three of `SchmittTrigger::process`'s
comparisons false, so the state is unchanged and it returns `false` — the trigger looks *robust*. But
the NaN is stored into `prevSyncVolts`. On the next finite sample that crosses HIGH, the trigger
fires and `f = (1 − NaN)/(now − NaN) = NaN`. Unguarded, `phase = (1 − NaN)·deltaPhase = NaN`, and
**`phase` has no guard today** — `VcoCore.hpp:537-540` guards `deltaPhase`, not the accumulator. From
that sample on: `phase += deltaPhase` stays NaN, `phase >= 1.0` is false so the wrap never fires,
`p = (float)phase` is NaN, and `morphedWave`'s `(int)(morph * 4.f)` cast is reached with a NaN phase.
**One hostile sample kills the instance permanently, after the input has recovered** — the identical
shape plan 32-05 measured for a `+infinity` `dt`, and the identical shape D-04's third item describes
for `jump`.
**How to avoid:** the negated-comparison guard on `f` (Pattern 2) is the fix, and it is load-bearing
rather than defensive. Its RED is a poisoned-instance trace: drive one NaN, withdraw it, and show the
instance still returning NaN.
**Warning signs:** a hostile-input test that only asserts *"output is finite during the hostile
sample"* — this defect is invisible during the hostile sample and only appears after recovery.

### Pitfall 7: `f` is not confined to `[0,1]` once the master is band-limited

**What goes wrong:** the analysis in Pitfall 5 gives `f ∈ (0,1]` for the *raw* threshold crossing. But
a band-limited master's samples around the wrap carry BLEP residuals, and the interpolated `f` can
land outside the range. Worked, for a polyBLEP'd ±5 V falling saw at true wrap fraction `g`:
`f = (6 − 5(1−g)²)/(10 − 5g² − 5(1−g)²)`, which is 0.2 at `g = 0`, 0.633 at `g = 0.5`, and **1.2 at
`g = 1`** — out of range.
**Corroboration:** VCV Fundamental guards exactly this, with
`syncOccurred = (0 < syncSubsample) & (syncSubsample <= 1) & (deltaSync >= 0)`
`[CITED: github.com/VCVRack/Fundamental v2 src/VCO.cpp]`.
**How to avoid:** the same guard from Pattern 2 covers it. Do not assume the range from the trigger's
semantics alone.

### Pitfall 8: The new TU and the new `make` target both have guard consequences

**What goes wrong:** `make guards` exits 1 at the end of the phase, or `make <render-target>` hard-fails
on a machine with no Rack SDK.
**Three specifics, verified:**
1. `tests/check_includes.sh` derives its LFO-side scan from `find src tests tools`, so **a renderer TU
   placed in `tools/` is LFO-side by default** and needs an explicit `VCO_SIDE_ALLOW` entry — the same
   cost CONTEXT.md flags for a new *test* TU `[VERIFIED: tests/check_includes.sh:355-379]`.
2. The Makefile's `ifeq ($(filter test capture guards,$(MAKECMDGOALS)),)` guard is what lets those
   targets run without `../Rack-SDK`. **A new render target must join that filter list** or it hard-fails
   on a runner `[VERIFIED: Makefile:21-26]`.
3. `TEST_SOURCES := $(wildcard tests/*.cpp)` means a renderer placed in `tests/` would be **linked into
   `make test` and run on every invocation**, which contradicts D-15's "generated on demand". Put it in
   `tools/`, beside `capture_golden.cpp`.
**Output directory:** `build-test/` is already in `.gitignore`, so `build-test/audition/` costs **zero**
`.gitignore` edits and satisfies D-15 by construction `[VERIFIED: .gitignore]`.

### Pitfall 9: The compile canary silently stops proving anything

**What goes wrong:** `check_canary.sh [2b/5]` requires **every** `VcoInputs` DSP field to be fed a
runtime-derived value, because a constant-fed field lets `-O2/-O3` constant-propagate a runtime-indexed
in-class `static constexpr` out of existence before the MinGW linker ever sees it — *the exact construct
that got v2.0.0 rejected* `[VERIFIED: tests/check_canary.sh:125-142]`.
**The Phase 33 specific:** two new fields. The current canary packs bits as `i & 7`, `(i>>3)&3`,
`(i>>5)&3`, `(i>>7)&7`, `(i>>10)&3`, `(i>>12)&1`, `i & 15`, `(i>>4)&15`, `(i>>8)&15`
`[VERIFIED: src/vco_compile_canary.cpp:104-112]`. Add non-overlapping runtime derivations for both new
fields — e.g. `in.syncVolts = (float)((i >> 13) & 7) - 3.f;` and
`in.syncConnected = ((i >> 16) & 1) != 0;`.
⚠️ **And note the trap the register does not name:** if `syncConnected` were fed a constant `false`, the
entire sync branch would fold away at `-O3` and the canary would cover the new code path with *nothing*,
while `[2b/5]` still reported PASS on the field's presence. The flag must vary **and** be true on some
iterations. Make this an explicit plan task with its own rationale (the Phase 31 D-23 lesson), not a
gate-time discovery.

### Pitfall 10: A grid whose masters all have hard edges tests nothing about sub-sample placement

Covered in full in §"D-06 ⚠️ The measurement-design hazard". Restated here because it is the failure a
plan is most likely to walk into: it produces a green grid that says nothing about SYNC-02's central
clause, and it does so silently.

---

## Code Examples

### Deriving `f` and the reset, with the two guards

```cpp
// src/dsp/VcoCore.hpp, in step(), immediately after the single-subtract wrap.
//
// SIGN AND ORDER, both load-bearing:
//   - `before` is the naive value at the PRE-reset advanced phase (D-05: the value
//     already computed this step). `after` is the ONE extra morphedWave call.
//   - h = after - before matches MorphBlep.hpp:188-204's convention. research/STACK.md:124
//     writes the same quantity NEGATED; that expression must not be transcribed here.
bool syncFired = false;
if (in.syncConnected && syncTrig.process(in.syncVolts, 0.1f, 1.0f)) {
    // The new divisor (D-12). prevSyncVolts is unconditionally the previous sample's
    // voltage (see the store below), which is what makes (now - prev) != 0 in steady
    // state; the guard exists for the cases where that invariant is broken by a
    // connect/rate transition, and for a NaN that reached the store without firing
    // the trigger.
    float f = (1.0f - prevSyncVolts) / (in.syncVolts - prevSyncVolts);

    // NEGATED FIRST — a NaN f fails `f >= 0.f`, so the negation fires and it lands
    // on the fallback instead of reaching `phase`, which carries NO guard of its own.
    // The upper bound is STRICT so that phase == 0.0 is unreachable (research/STACK.md:149).
    // The fallback is 0, never 1: f == 1 IS the snap-to-zero landmine.
    if (!(f >= 0.f) || !(f < 1.f)) f = 0.f;

    const float before = wave.morphedWave((float)phase, morph, character, 0.f);
    phase = (double)(1.f - f) * deltaPhase;              // fractional overshoot, never 0
    const float after  = wave.morphedWave((float)phase, morph, character, 0.f);

    blep.addPastStep(f, after - before);                 // == blep.addStep(0.f, -f*f*(after-before))
    syncFired = true;
}
prevSyncVolts = in.syncVolts;   // UNCONDITIONAL — this is the invariant the guard rests on
tel.syncFired = syncFired;
```

*(Shape only. `morph` and `character` must already be through their negated-comparison conditioning —
`VcoCore.hpp:597-602` — before either `morphedWave` call, and the plan owns where the conditioning
block moves to relative to the sync block.)*

### The additive past-edge entry point

```cpp
// src/dsp/MorphBlep.hpp — additive, next to addStep. EXTENDS the [0,1] contract,
// never reinterprets it.
//
// THE ASYMMETRY, STATED SO IT IS NOT "FIXED" LATER. addStep's edge is AHEAD, so both
// halves of the 2-sample residual are still deliverable. This entry point's edge is
// BEHIND: it happened between the previous sample and this one, at fraction `f` of
// the way across. The residual r(x) is nonzero on [-1, 1], so the pre-edge half
// +jump*(1-f)^2/2 belonged on a sample that has ALREADY BEEN EMITTED. It is
// FORFEITED, deliberately and permanently: recovering it needs a one-sample output
// delay buffer, which Phase 32's D-13 rejects on two grounds — declared latency, and
// a VCO that silently delays by a sample desyncs against every other oscillator in
// the patch. MEASURED cost of the forfeit: 0.2-0.6 dB at slave >= 2x master,
// 4.6-6.0 dB at slave < master.
//
// The gate is the SAME negated shape as addStep's, for the same reason.
void addPastStep(float f, float jump) {
    if (!(f >= 0.f) || f > 1.f) return;
    inject += jump * (-0.5f) * f * f;    // r(1-f), the only recoverable half
    // pending is deliberately untouched: the residual's support is exhausted at this sample.
}
```

### Master generation for the sync sub-grid

```cpp
// tests/test_vco_spectrum.cpp — the master is generated IN THE TEST, not by a second core.
//
// dtm is exactly representable: Km / 4096 == Km * 2^-12. The accumulator therefore
// returns EXACTLY to its start after the block, the block is exactly periodic, and
// the achieved bin error is EXACTLY ZERO rather than merely small.
const double dtm = (double)Km / (double)kSpectrumN;
double phim = 0.0;
// ... per sample:
phim += dtm;
if (phim >= 1.0) phim -= 1.0;
const float masterVolts = 5.f * (1.f - 2.f * (float)phim);   // falling saw: RISING jump at the wrap
```

---

## State of the Art

| Old approach | Current approach | Where | Impact for this phase |
|--------------|------------------|-------|-----------------------|
| Snap the synced phase to 0 | Reset to `deltaPhase · (1 − syncSubsample)` | VCV Fundamental VCO v2 `[CITED: github.com/VCVRack/Fundamental v2 src/VCO.cpp]` | **Independent industry corroboration of D-01's locked formula, term for term.** Fundamental computes `endPhase = deltaPhase * (1.f - syncSubsample)` and assigns it; it does not snap. |
| Detect the sync edge at sample resolution | Interpolate the sync signal linearly to a crossing between the two samples | VCV Fundamental: `syncSubsample = -lastSync / deltaSync` `[CITED: same]` | Confirms D-01's technique is standard practice. Fundamental interpolates to **zero**; D-01 interpolates to the **HIGH threshold** — the difference is the hysteresis coupling D-01 chose deliberately. |
| Trust the interpolated fraction | Gate it: `(0 < s) & (s <= 1) & (deltaSync >= 0)` | VCV Fundamental `[CITED: same]` | Confirms Pitfall 7: the fraction leaves `[0,1]` in practice and a range guard is standard, not paranoia. |
| Process all the slave's own crossings for the whole sample | Process crossings only over the post-sync sub-interval, from phase 0 to `endPhase` | VCV Fundamental `[CITED: same]` | Corroborates D-07's direction: sites the reset jumped over are simply not in the swept interval. |
| minBLEP tables | polyBLEP closed form | this repo, by AA-05 | **Records a real, bounded cost here:** minBLEP is minimum-phase so all of its correction lies after the edge, which is why a past-edge sync costs it nothing. The 2-point polyBLEP's support straddles the edge, so half the sync correction is forfeited. `[ASSUMED — minimum-phase property is training knowledge; the symmetric-support half is VERIFIED at MorphBlep.hpp:195-199]` |

**Deprecated/outdated for this phase:**
- `research/STACK.md:124`'s jump expression `out_preReset − morphedWave(newPhase)` — sign-inverted
  relative to the seam's documented convention. See Pitfall 2. The *rest* of the Q3 answer stands.
- `research/STACK.md:100-104`'s polyBLEP/polyBLAMP snippets — already recorded as falsified by Phase
  32 (`MorphBlep.hpp:50-58`); they are not a source for anything on the sync path either.
- Register item 13's *"no header change is needed"* — recovered as **true** (via
  `addStep(0.f, -f*f*jump)`) but only under candidate (b), and only as a numerical identity rather
  than as the obvious reading. Do not cite it as licence for candidate (a).

---

## Assumptions Log

| # | Claim | Section | Risk if wrong |
|---|-------|---------|---------------|
| A1 | minBLEP's minimum-phase kernel places all correction after the edge, which is why past-edge sync costs it nothing | State of the Art; Summary | Low. It is explanatory colour for why AA-05's constraint bites here; nothing in the plan depends on it. The operative half (2-point polyBLEP's support is `[−1,1]`) is verified in-repo. |
| A2 | The prototype's candidate ranking transfers to the real nine-site `morphedWave` core | D-06 | **Medium-high.** This is exactly why D-06 says *measured, not argued*: the in-repo measurement is binding and the prototype is a prediction. If it does not transfer, the decision rule in §"D-06" still stands and the plan follows it to whatever the real core says. |
| A3 | The recommended per-rate `K_m` values (93 / 85 / 43) put the master within 1.2 % across rates and are all odd | D-06 grid | Low — arithmetic, re-checkable in one line. If the plan prefers a different master frequency it must re-derive `K_m` per rate and keep it odd. |
| A4 | 16-bit PCM WAV is the safest audition format across the operator's players | Don't Hand-Roll; Validation | Low. Float32 WAV is also widely supported; the plan may choose either, provided the scale factor and any clipping are reported rather than silent. |
| A5 | `syncVolts`/`syncConnected` bit positions `(i>>13)&7` and `(i>>16)&1` do not collide with the canary's existing packing | Pitfall 9 | Low — mechanical, and `check_canary.sh [2b/5]` is the thing that would catch a collision. |
| A6 | D-07's suppression rule is discharged by the ordering alone, with a residual `pending` phantom that is accepted rather than cancelled | Pattern 4 | **Medium.** The ordering analysis is verified against `MorphBlep.hpp:508-511`; the *disposition* is a recommendation the plan (or the operator) may take differently. Named as an open question below. |

---

## Open Questions

1. **Does the phantom `pending` from the pre-reset sample need cancelling?**
   - *What we know:* `MorphBlep`'s site loop, evaluated at the post-reset phase, structurally cannot
     fire a jumped-over site `[VERIFIED: MorphBlep.hpp:508-511]`. The residual phantom is the
     `pending` half deposited at sample *n−1* for a site the reset then skipped; the window is
     `(1−f)·dt` wide. `pending` is a scalar sum, so per-site cancellation is not possible without
     restructuring the header.
   - *What's unclear:* the magnitude of this term against the real nine-site geometry, and how much of
     it is self-cancelling against `h` being taken past the same site.
   - *Recommendation:* accept and document (§Pattern 4), and add a diagnostic-tier grid column that
     reports it, so a later phase has a number rather than an argument. Do **not** restructure
     `MorphBlep` for it inside this phase.

2. **Does the D-10 anti-circularity margin survive on the real core at the cells where it is smallest?**
   - *What we know:* prototype margins run 0.003 V to 1.00 V and are positive in all 45 cells.
   - *What's unclear:* whether the smallest cells stay positive with the nine-site waveform and with
     character at 1, where several sites go to zero width.
   - *Recommendation:* measure first, then decide the gated population by a physical criterion stated
     **before** enumeration (register item 8's discipline), and pin the margin from the minimum over
     that population, rounded outward.

3. **Which master edge shapes belong in the *gated* tier versus the diagnostic tier?**
   - *What we know:* the hard-jump master is what operators actually patch; the band-limited master is
     the only one on which SYNC-02's sub-sample clause is testable.
   - *Recommendation:* gate both, but write the per-cell provenance so the two are never compared
     across shapes — the hard-jump rows measure the *system including a lossy `f`*, the band-limited
     rows measure the *VCO*. This is a real distinction, not bookkeeping.

4. **`.wav` bit depth and scaling for the audition renderer.**
   - *What we know:* the output is unconditioned and reaches at least ±5.2 V on a bare saw slave;
     Phase 34 owns the output stage.
   - *Recommendation:* fix a stated scale (e.g. 1 V → 0.1 FS), 16-bit PCM, and **report the clip
     count** in the render's stdout so a clipped audition is visible rather than silent. Never
     normalise per-leg — that would destroy the A/B's level match, which is the whole point.

---

## Environment Availability

| Dependency | Required by | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Apple `clang++` | `make test`, `make strict`, the prototype and the renderer | ✓ | 16.0.0 (clang-1600.0.26.6) | — |
| GNU `make` | every target | ✓ | 3.81 | New targets must stay 3.81-compatible: plain shell loops, no `$(file ...)`, no `::=` |
| `bash` | the three guard scripts | ✓ | 3.2.57 (arm64) | Guard scripts already target this vintage |
| `shasum` | `check_frozen.sh` | ✓ | `/usr/bin/shasum` | — |
| `doctest.h` | `make test` | ✓ | vendored at `tests/doctest.h` | — |
| `../Rack-SDK` | `make`, `make strict`, `make install` | ✓ | present | `test`/`capture`/`guards` already run without it; a new render target must join that filter |
| AddressSanitizer | Task 1's CR-01 RED | ✓ | `clang++ -fsanitize=address` compiles | — |
| `afplay` | the operator's D-13..D-16 audition | ✓ | `/usr/bin/afplay` | any audio player; `.wav` is chosen precisely so no conversion is needed |
| CI MinGW link leg | the ODR gate that `make strict` cannot provide | ✓ (GitHub Actions) | — | **None. `make strict` is `-fsyntax-only` and never links.** No tag or resubmission on local evidence alone. |

**Missing dependencies with no fallback:** none.
**Missing dependencies with fallback:** none.

---

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | doctest (header-only, vendored at `tests/doctest.h`), driven by `make test` |
| Config file | `Makefile` — `TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off`; `TEST_SOURCES := $(wildcard tests/*.cpp)` |
| Quick run command | `make test` |
| Full suite command | `make test && make strict && make guards` |
| New wiring needed | **none for the build** (glob) if the sub-grid lands in existing TUs. **If a new TU lands:** one `VCO_SIDE_ALLOW` line in `tests/check_includes.sh`. **For the renderer:** one `tools/*.cpp`, one `make` target, one entry in the Makefile's `MAKECMDGOALS` skip filter, and one `VCO_SIDE_ALLOW` line (because `check_includes.sh` scans `tools/` too). **For the POD:** two runtime-derived field feeds in `src/vco_compile_canary.cpp`. |

### Phase Requirements → Test Map

| Req ID | Behavior | Test type | Automated command | File exists? |
|--------|----------|-----------|-------------------|--------------|
| D-04 / CR-01 | `MorphBlep::step` does not write `W[segment]` out of bounds for a negative or NaN `morph` | unit + one-shot ASan probe | `make test` (`test_morph_blep.cpp`); ASan RED run once, not wired | ✅ file exists, cases ❌ Wave 0 |
| D-04 / CR-02 | A non-finite `character` produces no NaN correction at the three literal-zero-width sites | unit | `make test` (`test_morph_blep.cpp`) | ✅ file, cases ❌ |
| D-04 / new | A non-finite `jump` is rejected by `addStep` and leaves per-instance state untouched; a poisoned-instance trace shows recovery | unit | `make test` (`test_morph_blep.cpp`) | ✅ file, cases ❌ |
| SYNC-01 | A master rising edge resets the phase; the trigger is per-instance; `syncConnected == false` never resets | unit | `make test` (`test_vco_core.cpp` via `VcoBlockDriver`) | ✅ file, cases ❌ |
| SYNC-01 / D-09 | At most one rising edge is observed per sample, by construction; every observable edge fires exactly once; the missed-edge rule is identical at 44.1 / 48 / 96 kHz; output stays finite and bounded | unit | `make test` | ❌ Wave 0 |
| SYNC-02 / D-06 | The placement candidates are measured and the winner is pinned by the three-condition decision rule | spectral | `make test` (`test_vco_spectrum.cpp` sync sub-grid) | ✅ apparatus, sub-grid ❌ |
| SYNC-02 / D-01 | The fractional-overshoot reset beats a snap to `phase = 0` by a measured margin on an informative master | spectral | `make test` | ❌ Wave 0 |
| SYNC-02 / D-01 | `phase` is never exactly 0 after a reset, including when `f` would compute to exactly 1 | unit | `make test` | ❌ Wave 0 |
| SYNC-02 / D-11 | Per-cell sync alias thresholds at 44.1 kHz, with 48/96 kHz as regression | spectral | `make test` | ❌ Wave 0 |
| SC-3 / D-10 | Worst `\|x[n] − x[n−1]\|` on reset samples stays under a measured, outward-rounded bound | unit (time domain) | `make test` | ❌ Wave 0 |
| SC-3 / D-10 | `uncorrectedResetDelta − correctedResetDelta >= margin` (consults no pinned number) | unit (time domain) | `make test` | ❌ Wave 0 |
| SC-3 / D-10 | A discriminating mutation probe fails a **stated** population exactly | unit | `make test` | ❌ Wave 0 |
| PITCH-04 / D-12 | Extreme pitch × extreme FM × hostile sync, re-ticked only where sync is observed FIRING | unit | `make test` (`test_vco_pitch.cpp`) | ✅ file, sync leg ❌ |
| D-12 / new divisor | Equal consecutive samples and a NaN cable voltage cannot poison `phase`; the recovered instance returns finite samples | unit | `make test` (`test_vco_core.cpp` scenario four grid + sync voltages) | ✅ grid exists, sync rows ❌ |
| D-02 / item 15 | Both new POD fields are fed runtime-derived values by the canary | build gate | `make guards` (`check_canary.sh [2b/5]`) | ✅ gate exists, feeds ❌ |
| CORE-03 | The sync trigger and previous-voltage store are per-instance; two interleaved cores do not interact | unit | `make test` (`test_vco_core.cpp` interleave invariant) | ✅ pattern exists, sync in window ❌ |
| Guardrail | No frozen header edited; `src/AnalogLFO.cpp` absent from the diff; LFO goldens unmoved | build gate | `make guards` + `make test` (`test_lfo_guardrail.cpp`) | ✅ exists |
| C++11 / ODR | New core code and the new shell wiring compile under `-std=c++11 -pedantic-errors` and link under MinGW | build gate | `make strict` + **CI MinGW link leg** | ✅ exists |
| D-13..D-16 | The A/B renderer produces a matched pair from the same driver in the same pass, on demand, uncommitted | manual (operator UAT) | `make <render-target>` then listen | ❌ Wave 0 |

### Sampling Rate

- **Per task commit:** `make test` (the whole doctest binary; the suite runs in seconds).
- **Per wave merge:** `make test && make strict && make guards`.
- **Phase gate:** full suite green, plus the **CI MinGW link leg observed green on the exact commit**,
  before `/gsd-verify-work`. `make strict` is `-fsyntax-only` and cannot substitute.

### Wave 0 Gaps

- [ ] `tests/test_morph_blep.cpp` — Task 1's three RED cases (CR-01 ASan reproduction, CR-02 measured
      non-finite count, `jump` poisoned-instance trace) and their permanent assertions.
- [ ] `tests/test_vco_spectrum.cpp` — the sync sub-grid: a master generator, `SPECTRUM_GRID`-shaped
      sync cells keyed on `K_master`, the six candidate legs, the exact-zero bin-error assertion.
- [ ] `tests/test_vco_core.cpp` — SC-3's delta bound + anti-circularity + mutation probe; D-09's
      structural-ceiling case; the sync rows in scenario four's `HOSTILE_TIMES` grid; re-derived
      output tiers.
- [ ] `tests/test_vco_pitch.cpp` — PITCH-04's third input class, with sync observed firing.
- [ ] `src/vco_compile_canary.cpp` — runtime-derived feeds for `syncVolts` and `syncConnected`.
- [ ] `tools/render_sync_ab.cpp` + Makefile target + skip-filter entry + `VCO_SIDE_ALLOW` entry.
- [ ] `tests/check_includes.sh` — `VCO_SIDE_ALLOW` entries for any new TU.
- [ ] Framework install: **none** — doctest is vendored and `make test` globs.

---

## Security Domain

`security_enforcement` is not set to `false` in `.planning/config.json`, so this section is included.
This is an offline, single-process audio plugin with no network, no filesystem input, no user
credentials, and no serialization added by this phase. The applicable surface is **memory safety and
untrusted numeric input**, which is where the phase's real risk lives.

### Applicable ASVS categories

| ASVS category | Applies | Standard control in this phase |
|---------------|---------|-------------------------------|
| V2 Authentication | no | No identity surface. |
| V3 Session Management | no | No sessions. |
| V4 Access Control | no | No multi-user surface. |
| **V5 Input Validation** | **yes** | Every value crossing `forge::VcoInputs` is treated as hostile. The negated-comparison guard idiom (`if (!(x > lo)) x = lo;`) is the project's validated pattern; `forge::clamp` is banned for this purpose because it is NaN-transparent. This phase adds `syncVolts` (an arbitrary cable voltage) and a new divisor behind it. |
| V6 Cryptography | no | None. The only RNG is `Xoroshiro128Plus` for analog drift, not a security primitive, and this phase does not touch it. |
| V12 Files & Resources | **partly** | The audition renderer writes `.wav` files. Path is fixed and repo-local (`build-test/audition/`); no user-supplied path is accepted. |
| V14 Configuration | **yes** | The C++11 `-pedantic-errors` + MinGW link gate is the supply-chain-adjacent control: it is what stopped an ODR defect shipping once already. |

### Known threat patterns for this stack

| Pattern | STRIDE | Standard mitigation | Status in this phase |
|---------|--------|---------------------|----------------------|
| Out-of-bounds write from an unclamped array index derived from a float | Tampering / EoP | Clamp the index on **both** sides before use | **CR-01, open.** `MorphBlep.hpp:319-320` clamps `segment` only from above; `(int)NaN` is `0` on this arm64 host and `INT_MIN` under x86 `cvttss2si` on the builds that ship. Closed by Task 1, RED-first with an ASan reproduction. |
| Undefined behaviour from a float-to-int cast of a non-finite value | Tampering | Reject non-finite before the cast, with a NaN-safe comparison | **CR-02, open.** Closed by Task 1. |
| Permanent state poisoning from one hostile sample (NaN/Inf carried in an accumulator) | Denial of Service | Guard at the accumulator's write, with the negated comparison first | **Three instances.** `pending` via `dt` (closed, plan 32-05); `pending` via `jump` (**open**, D-04's third item); `phase` via `f` (**new in this phase**, Pitfall 6, and `phase` has no guard today). |
| Division by an attacker-influenced denominator | DoS | Establish and assert the invariant that makes the zero unreachable, then guard the result | **New in this phase.** Pitfall 5 gives the reachability analysis; the mitigation is the unconditional store plus the guard on `f`. |
| Unbounded output amplitude reaching the host | DoS | Measured envelope bounds asserted in test | Existing tiers at `test_vco_core.cpp:511`; **must be re-derived for sync, not assumed.** |
| Toolchain-divergent UB invisible on the development host | — | Two-standard compilation + CI MinGW **link** leg | Existing and proven to bite (run 30339957128). `make strict` is syntax-only and cannot substitute. |
| Infinite loop on patch load from a degenerate RNG seed | DoS | Never seed `Xoroshiro128Plus` with `(0,0)` | Existing; this phase adds no seeding path, and the renderer must copy the four documented seed literals verbatim. |

**Sanitizer policy:** register item 12 forbids a permanent repo-wide UBSan/ASan gate (the shipped LFO
carries shared latent UB that is deliberately unowned). Task 1's ASan use is a **scoped one-shot
probe** run to produce a RED, not wired into `make test`, `make guards` or CI.

---

## Project Constraints

**No `./CLAUDE.md` or `./.claude/CLAUDE.md` exists in this repository, and no `.claude/skills/` or
`.agents/skills/` directory exists** `[VERIFIED: checked this session]`. The project's binding
directives therefore come from `.planning/PROJECT.md`, `.planning/ROADMAP.md:100`, and the source
banners, and are reproduced here so the planner can check compliance mechanically:

- **C++11 `-pedantic-errors` is the shipping toolchain** (plus C++17 for tests). No `inline constexpr`
  variables, no `if constexpr`, no `std::clamp`, no `[[maybe_unused]]`, no structured bindings, no
  nested-namespace syntax, **no in-class `static constexpr` indexed at runtime** (that construct got
  v2.0.0 rejected from the VCV Library), no brace value-list init of `VcoInputs` (it has NSDMIs, so
  under C++11 it is not an aggregate).
- **`-ffp-contract=off`, never `-ffast-math`** — load-bearing for the BLEP polynomials specifically.
- **Zero Rack-SDK includes under `src/dsp/`.**
- **The four frozen shared headers** — `Waveshape.hpp`, `DriftEngine.hpp`, `RackCompat.hpp`,
  `MathConst.hpp` — are byte-pinned by `check_frozen.sh` and consumed by the shipped LFO. Proposing an
  edit to any of them is a **guardrail event**: stop and surface impact + options + a recommendation.
- **`src/AnalogLFO.cpp` must remain absent from this phase's diff.**
- **`src/dsp/VcoCore.hpp`'s source-shape contract:** `struct VcoCore` and `float step(...)` must each
  stay on one line with their brace, or `make guards` hard-fails.
- **No tag or VCV Library resubmission on local evidence alone** — the CI toolchain-gate link leg must
  be observed green on the exact commit.
- **Gates are artifacts needing review in their own right.** Bare `grep -c` criteria produce artifact
  counts, because these headers deliberately quote the constructs they forbid; count criteria must be
  comment-stripped or anchored and compared against a baseline rather than zero.
- **Measure, then pin, rounding outward**, with a discriminating mutation probe proving the bound bites
  at the boundary it claims. **RED-first.** A requirement is ticked only where a control is observed
  *firing* behind the claim.

---

## Sources

### Primary (HIGH confidence — read or measured this session)

- `src/dsp/MorphBlep.hpp` (full read) — `addStep`'s contract `:257-262`, the residual sign convention
  and its `[−1,1]` support `:188-212`, the site loop and crossing test `:469-541`, the `segment` clamp
  `:319-320`, the `dt` guard `:282-306`, `pending`/`inject` `:213-236`, the D-13 delay-buffer rejection
  `:221-232`.
- `src/dsp/VcoCore.hpp` — `VcoInputs` `:232-244`, `tel.syncFired` `:285`, the advance/wrap `:536-542`,
  the morph/character negated pair `:597-602`, the single `blep.step` `:645`.
- `src/dsp/RackCompat.hpp:44-58` — `SchmittTrigger`'s exact state machine; `:97` — `forge::clamp`'s
  NaN transparency.
- `tests/test_vco_spectrum.cpp` — `aliasPeakDb` `:186-223`, `impliedLeakageDb` `:390-409`,
  `NaiveVcoCoreMirror` `:463-584`, `SpectrumCell`/`kThresholdFloorDb` `:611-648`, the grid `:744+`,
  `driveSecondBlock` `:937-949`, `measureCellDb` `:1001-1055`.
- `tests/VcoBlockDriver.hpp`, `tests/check_includes.sh:355-379`, `tests/check_canary.sh:19-31, 125-142`,
  `src/vco_compile_canary.cpp:83-124`, `Makefile`, `.gitignore`.
- **In-session prototype measurement** — 135 alias-floor measurements over six legs, three master edge
  shapes, three sample rates and five to six slave frequencies, using this repo's own block
  construction and alias metric, compiled `clang++ -O2 -std=c++11 -ffp-contract=off`.

### Secondary (MEDIUM confidence — project documents and cross-checked external sources)

- `.planning/research/STACK.md:19-38, :54, :64, :75, :124, :149, :154-156`
- `.planning/research/PITFALLS.md:114-131, :127, :190, :211, :382, :409`
- `.planning/research/ARCHITECTURE.md:50-62`
- `.planning/phases/32-.../deferred-items.md` items 5, 6, 8, 9, 10, 12, 13, 15, 22, 23, 25, 26, 27
- `.planning/phases/32-.../32-REVIEW.md` (CR-01, CR-02 and the orchestrator's reachability note)
- `.planning/phases/32-.../32-RESEARCH.md:705+` (the Validation Architecture shape this section follows)
- `.planning/STATE.md` §Accumulated Context
- [VCVRack/Fundamental v2 `src/VCO.cpp`](https://github.com/VCVRack/Fundamental/blob/v2/src/VCO.cpp) —
  `syncSubsample`, its range gate, `endPhase = deltaPhase * (1 - syncSubsample)`, and the post-sync
  crossing sub-interval.

### Tertiary (LOW confidence — community sources, corroborative only)

- [KVR: PolyBLEP hard sync sawtooth oscillator](https://www.kvraudio.com/forum/viewtopic.php?p=5788101)
  and [Modulating (poly)BLEP hard-sync saw](https://www.kvraudio.com/forum/viewtopic.php?t=425054) —
  the ordering trap between the slave's own wrap BLEP and the sync BLEP within one sample, and the
  "advance the slave to the master's reset point, then process the remainder" idiom. Consistent with
  D-07; not relied on for any recommendation.
- [PD forum: polyBLEP/BLEP/BLIT with oscillator sync](https://forum.pdpatchrepo.info/topic/13673/polyblep-blep-blit-etc-with-oscillator-sync),
  [Teensy: polyBLEP oscillator with band-limited hard sync](https://forum.pjrc.com/threads/62240-New-Audio-Object-polyBLEP-Oscillator-with-bandlimited-hard-sync),
  [Wikipedia: Oscillator sync](https://en.wikipedia.org/wiki/Oscillator_sync).
- [VCV Community: understanding syncCrossing on minblep](https://community.vcvrack.com/t/can-you-help-me-to-understand-synccrossing-on-minblep/20925)
  — confirms the sub-sample framing; does not settle the past/future position convention.

---

## Metadata

**Confidence breakdown:**

| Area | Level | Reason |
|------|-------|--------|
| D-06 placement analysis (which half is recoverable, and why) | **HIGH** | Derived directly from `MorphBlep.hpp`'s own documented residual and support, verified by reading; the `addStep(0.f, -f*f*jump)` identity is checkable algebra. |
| D-06 predicted candidate ranking | **HIGH for the ranking, MEDIUM for the figures** | Measured over 135 prototype cells with a consistent, large-margin ordering; the prototype slave carries one site rather than nine, so absolute decibels must not be transferred. |
| Guard landmines (`f = 1`, the NaN path to `phase`, the divide-by-zero reachability) | **HIGH** | Traced line by line through `RackCompat.hpp:50-56` and `VcoCore.hpp:536-542`; each has an exact, statable trigger condition. |
| Sign inversion in `research/STACK.md:124` | **HIGH** | Direct textual comparison of two in-repo documents. |
| Grid construction (master-bin fundamental, exact dyadic increment, per-rate `K_m`) | **HIGH on the derivation, MEDIUM on the specific `K_m` values** | The periodicity argument follows the file's own coprimality reasoning; the `K_m` choices are a recommendation the plan may re-derive. |
| Guard/build consequences (`tools/` scan, Makefile filter, canary fields) | **HIGH** | Read from the scripts and the Makefile. |
| Architecture / ordering | **HIGH** | Sequenced against the existing `step()` body. |
| Pitfalls | **HIGH** | Every one is either read from source or measured this session. |

**Research date:** 2026-08-28
**Valid until:** ~2026-09-27 for the external corroboration; the in-repo findings are valid until the
files they cite change — and `check_frozen.sh` guarantees four of them cannot.
