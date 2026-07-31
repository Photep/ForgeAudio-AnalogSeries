# Phase 32: Morph-Aware Anti-Aliasing (polyBLEP/polyBLAMP) — Research

**Researched:** 2026-07-31
**Domain:** Closed-form band-limiting (polyBLEP / polyBLAMP) of a character-deformed 5-shape morph crossfade, plus the spectral gate that proves it
**Confidence:** HIGH on kernels, site map, jump magnitudes, the D-03 factor shape, and the achievable alias floor (all **measured** against the frozen `Waveshape.hpp` in this session). MEDIUM on the exact per-shape threshold *numbers* (prototype, not final implementation — D-08's measure-then-pin loop still runs).

> **Method note.** Every numeric claim below tagged `[MEASURED]` was produced this session by compiling a prototype against the real, unmodified `src/dsp/Waveshape.hpp` with the project's own flags (`-O2 -Isrc -ffp-contract=off`) and analysing the output with an exact-integer-cycle FFT. Nothing here is recalled from training data. Where this research contradicts `.planning/research/STACK.md`, the contradiction is called out explicitly with the measurement that settles it.

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

**BLOCKING PRE-PLANNING ACTIONS** (one commit, before planning starts):
- **D-02:** `.planning/ROADMAP.md` §"Phase 32" Success Criterion 2 drops the word "measured" → *"...driven by the characterized jump..."*.
- **D-06:** `.planning/REQUIREMENTS.md` AA-01 widens "scaled by the morph weights" to "scaled by the morph and bleed weights".

**Jump magnitude (AA-04)**
- **D-01:** Analytic, character-aware jump magnitudes — closed form, no probing. *Rejected:* probing `morphedWave` at phase ±ε (cost). *Rejected:* pristine character-independent jumps (`research/STACK.md:40`) — **the research recommendation is overridden by decision, not by oversight; downstream agents must not "restore" it.** The derivation duplicates `Waveshape`'s internals in a second header, acceptable only because that file is frozen; record the dependency in `MorphBlep.hpp`'s banner.
- **D-03:** The character-scaling factor's SHAPE is locked; the exact expression goes to research. Binding constraints: a closed-form function of edge-width versus one sample's phase advance; **→ 0** when the softened edge is much wider than a sample; **→ 1** when the edge is much narrower than a sample; table-free, pure `+ - * /`. Tying it to one sample's phase advance makes the correction sample-rate-aware for free.
- **D-03b:** One unified rule governs every site — the same closed-form character factor scales polyBLEP value-step corrections *and* polyBLAMP slope-break corrections; each site supplies its own pristine magnitude and its own character-dependent width. *Rejected:* per-site-type derivations. **Corollary the planner must honor:** the saw's soft reset (`:91-97`) and the triangle's rounded peaks (`:71-78`) are fast but continuous, not discontinuities; a full-authority correction there would be actively wrong, and the D-03 factor is what makes them self-limiting.

**Discontinuity site map (AA-01, AA-02, AA-03)**
- **D-04:** A fixed union of sites is evaluated every sample; magnitudes fall to zero when a shape carries no weight. Site set: **phase 0** (saw wrap + square rising edge + pulse rising edge + triangle peak, coincident), the **square duty edge**, the **pulse duty edge**, the **triangle valley**. **Positions are recomputed per sample from the live `Waveshape` fields, never cached.** *Rejected:* computing the active set per sample.
- **D-05:** Corrections ride through the bleed normalization — computed against pre-normalization magnitudes and divided by the same `1/(1 + bleedIntensity)` factor (`Waveshape.hpp:212`). Linear, therefore **exact**.
- **D-07:** Overlapping edges are each placed at their own sub-sample position and summed — never overwritten, and **no duty floor**. When `duty < dt` the two opposite-sign corrections partially cancel, and that cancellation is the physically correct band-limited answer. *Rejected:* flooring the effective duty at `dt`.

**The alias-floor gate (TEST-03)**
- **D-08:** Plan 32-01 builds the spectral helper and baselines the NAIVE core, before any band-limiting exists. Keep the naive path callable (mechanism is Claude's discretion).
- **D-09:** Per-shape, evidence-set thresholds — not one global number; each carries its measured justification in the test. *Rejected:* a single global threshold; a global floor with a pulse carve-out.
- **D-10:** Integer cycles per analysis block; rectangular window; zero leakage. "Alias energy" is simply the magnitude at non-harmonic bins. **A gate-correctness decision, not a convenience one.** *Rejected:* a Blackman-Harris window. Test frequencies sit at bin centres rather than exact equal-tempered notes — **irrelevant to aliasing behavior, must not be "fixed" by a later agent.** libm is available (test, not `src/`).
- **D-11:** 44.1 kHz is the binding assertion; 48 and 96 kHz run as regression (a correction scaled wrongly by `dt` fails rate-dependently). Coverage: morph at the five shape centres; character at both **0** and **1**. Exact note grid, block lengths and cycle counts are research/planner discretion.

**Structure & seams (CORE-02, AA-05)**
- **D-12:** One new header — `src/dsp/MorphBlep.hpp`; kernels and site logic together. **`research/STACK.md:61`'s suggestion to put the kernels in `RackCompat.hpp` is REJECTED and must not be revisited.** *Rejected:* a separate `Blep.hpp`.
- **D-13:** A pending-residual accumulator delivers the next-sample half of each correction. **Zero added latency.** Not a one-sample output delay buffer. Composes for free with D-07.
- **D-14:** The Phase 33 sync seam is DESIGNED here, not built here — an entry point accepting an externally-supplied `(sub-sample position, value jump)` event feeding the same accumulator. No sync behavior, no sync fields in `VcoInputs`. **CORE-03 constraint, binding: all `MorphBlep` state lives per-`VcoCore`-instance. No static, no global mutable voice state.**
- **D-15:** Deferred item 6 (the hostile-timing grid) stays in Phase 32 on a **corrected rationale** — the old premise (an oversampled inner loop) is falsified by AA-05. The surviving reason: this phase introduces division by `dt` and by `edgeWidth`, so a zero, subnormal or non-finite `sampleTime` now reaches arithmetic that did not exist before. Extend `tests/test_vco_core.cpp` scenario four to `±inf`, subnormal and very-large-finite `sampleRate`/`sampleTime`. Record the corrected rationale in the deferred register. Guards use the **negated-comparison idiom** (`if (!(x > 0.f)) ...`), never `forge::clamp`.

**MORPH control surface (MORPH-01, MORPH-02)**
- **D-16:** The MORPH CV jack and attenuverter are declared in THIS phase. **Phase 31's CONTEXT lumped "MORPH/CHARACTER CV + their attenuverters" into Phase 34; that lumping is corrected here, not followed.** No document edit needed.
- **D-17:** The shell mixes knob + CV × attenuverter; the POD boundary does not change. `VcoInputs::morph` is already documented post-CV/post-clamp `[0,1]`. Does not violate Phase 31's D-17. Zero POD change protects deferred item 9. Attenuverter styling follows Phase 31's D-07 (bipolar `-1..+1`, linear taper, default `0`, displayed `-100%..+100%`). `res/AnalogVCO.svg` gains two control positions. **`src/AnalogLFO.cpp` must remain absent from this phase's diff.**

### Claude's Discretion

- The exact closed-form character-scaling expression — routed here per D-03, within D-03's locked shape constraints.
- The mechanism for keeping the naive path callable for D-08's baseline (flag, second entry point, or test-only shim).
- Block lengths, cycle counts, the exact bin-centred frequency grid, and the DFT implementation for D-10; the exact note grid for D-11.
- Whether the D-04 site set is expressed as a small fixed-size array walked per sample or as unrolled straight-line code. **If an array: namespace-scope `static constexpr` only, never in-class `static constexpr` indexed at runtime** (`research/PITFALLS.md:190`).
- Updating `tests/test_vco_core.cpp:416` (the oracle) and the `±5.55 V` bound reasoning at `:511` — **the bound must be re-derived, not assumed.**
- Whether `MorphBlep` is a struct held by value inside `VcoCore` or a free-function set with explicit state — subject to D-14's per-instance constraint.

### Deferred Ideas (OUT OF SCOPE)

- Measured/probed jump magnitudes as a refinement (narrow escalation only if D-09 thresholds prove unreachable *and* research attributes it to magnitude error). Broad escalation is **v2.1 oversampling, explicitly not minBLEP**.
- Higher-order (4-point) polyBLEP — not scoped for v2.0; operator decision with impact assessment, not a silent implementation choice.
- CHARACTER CV + attenuverter → Phase 34 (CHAR-01).
- Hard sync → Phase 33 (SYNC-01/02); never snap the reset to exactly 0.
- The output stage and drift → Phase 34; `bleedLfo` stays `0.f`; the `×5` output stays unconditioned. **Phase 34 must re-read D-04.**
- The shipped LFO's shared latent UB — pointed at no phase, unfixed by decision. **A permanent repo-wide UBSan gate cannot be adopted;** any UBSan use here stays a scoped one-shot probe.
- Per-instance seed entropy + patch persistence → Phase 34/35.
- Amplitude fade near the Nyquist ceiling — rejected in Phase 31 (D-10).
- COARSE octave/semitone snap → Phase 35 or v2.1.
- `tests/check_docs.sh` into CI → Phase 36 (carried forward, not re-litigated).

</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| MORPH-01 | Morph engine (`Waveshape`) runs at audio rate, reused verbatim from the frozen shared core | Confirmed: `MorphBlep` calls `morphedWave` once per sample and adds an additive correction; zero edits to the frozen header. §Architecture Patterns. |
| MORPH-02 | MORPH knob + CV + attenuverter sweep the continuous 5-shape crossfade at audio rate | §Standard Stack (shell wiring), §Common Pitfalls P-13 (audio-rate morph is safe — measured finite and bounded to \|out\| ≤ 1.32 at 2 kHz morph rate). |
| AA-01 | Morph-aware polyBLEP band-limits the value-step discontinuities, scaled by the morph **and bleed** weights | §Site Map (7 value-step entries), §Weight Algebra, measured bleed-ring jump table. |
| AA-02 | polyBLAMP band-limits the triangle slope-corner discontinuities | §polyBLAMP Kernel (exact 2-point cubic residual, derived + corroborated against DAFx-16). MEASURED: 15.0 dB alias improvement on the triangle at C8/character 0. |
| AA-03 | Correctly handles multiple/overlapping discontinuities within one sample at narrow pulse widths | The pending accumulator sums (`+=`) rather than overwrites; measured at `dt = 0.19` (C9) with `duty = 0.05` — 0.26 samples wide, both edges fire in one sample. |
| AA-04 | BLEP/BLAMP magnitude is driven by the characterized (actual) jump so CHARACTER edge-softening auto-scales the correction | §Jump Magnitudes — full analytic derivation per site, **verified numerically to 6 decimal places** against the frozen code. |
| AA-05 | Table-free, Rack-free, closed-form; C++11-strict; no minBLEP, no oversampling | Every expression in §Code Examples is `+ - * /` plus comparisons. One optional refinement (pulse reach) uses a rational tanh approximation, also `+ - * /`. |
| CORE-02 | Anti-aliasing lives in a new additive header (`MorphBlep.hpp`) that *calls* the frozen `Waveshape.hpp` | §Architecture Patterns; guard wiring already pre-staged (see §Guard Wiring). |
| TEST-03 | An alias-floor / spectral invariant asserts high-note aliasing stays below a defined threshold | §Validation Architecture — complete construction with measured baselines, bin grid, block sizes, and threshold candidates. |

</phase_requirements>

## Summary

The crux answer in `research/STACK.md` is correct and holds: the morph output is a linear weighted sum, so band-limiting reduces to injecting scaled polyBLEP/polyBLAMP residuals at a small set of analytically-known discontinuity sites. This research closes the four gaps D-03 delegated and, in doing so, **falsifies three assumptions that the phase would otherwise have inherited**.

**First, the D-03 factor must have compact support.** Any factor that merely *decays* — `1/(1+W)`, `1/(1+W²)`, the sinc-Padé fit — leaves a small non-zero correction on edges that are already many samples wide, and that residual correction *injects* far more alias energy than it removes. MEASURED regressions versus the naive path reach **−60 dB** with full authority and **−30 dB** even with `1/(1+W²)`. The recommended factor is `k = max(0, 1 − w/(2·dt))²`, whose cutoff is not a tuned constant: it is the 2-sample kernel's own support. `research/STACK.md:40`'s "erring toward the pristine jump is the safe direction" and CONTEXT's `<specifics>` restatement of it are **falsified by measurement** — over-correction is not benign here.

**Second, the achievable alias floor is nowhere near −60 dB, and this is a property of 2-sample polyBLEP itself, not of this implementation.** The prototype's harmonic gain matches `sinc²(f/f_s)` to **0.01 dB across twelve harmonics** — i.e. the kernel is exactly canonical — and `sinc²` is only about −8 dB at Nyquist. MEASURED best-achievable peak alias at C8 / 44.1 kHz: saw −25.8 dB, square −31.9 dB, narrow pulse −11.6 dB. The DAFx-16 BLAMP paper independently reports 46 dB SNR for a *four*-point polyBLAMP triangle at C8 versus 45 dB for 4× oversampling — the same ceiling. **D-09's per-shape, evidence-set thresholds are not a convenience; they are the only honest way to gate this phase**, and the roadmap's ≈−60 dB must be treated as the target it is labelled as, not a pass criterion.

**Third, two site-map details in D-04 are load-bearing in a way the spectral metric alone does not reveal.** The square's *hard* step lives at exactly `0.5f` (`computeSquare:104`'s branch) while its *soft* tanh edge lives at `duty = 0.5 + c(0.04+spread)` — they are different positions. Placing the hard step at `duty` produces single-sample, full-amplitude spikes: MEASURED `max|out|` rises from **±5.52 V to ±9.78 V at every sample rate**. Splitting them restores the existing envelope exactly. Relatedly, the sub-sample "which side of the edge am I on?" test must use the **same float phase and the same strict comparison the frozen code uses**, or the correction desynchronises from the naive branch and injects a full-amplitude error; the double-precision phase is the right source for the *distance*, but the wrong source for the *side*.

Two smaller corrections: the saw's cosine soft reset does **not** reduce the wrap's value jump — MEASURED `+2.000000` at every character (D-03's corollary *conclusion* survives; its stated premise does not). And `research/STACK.md:100-104`'s polyBLAMP snippet is wrong (quartic where the 2-point form is cubic, with `dt` folded into the wrong place); the correct forms are derived and cited below.

**Primary recommendation:** implement the 9-entry fixed site table below with `k = max(0, 1 − w/(2·dt))²`, the float-side / double-distance crossing test, and the two-field pending accumulator; build the spectral gate on `N = 4096` with **odd** cycle counts `K`, and pin per-`(shape, note, character)` thresholds from the D-08 baseline rather than from the roadmap's −60 dB.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Band-limiting arithmetic (kernels, site map, jump magnitudes, D-03 factor) | Rack-free DSP core (`src/dsp/MorphBlep.hpp`) | — | CORE-02 names it; AA-05 requires zero Rack coupling and table-free closed form. |
| Per-sample orchestration (phase → naive call → correction → `×5`) | Rack-free DSP core (`src/dsp/VcoCore.hpp:484`) | — | The single call site; the surrounding pitch/Nyquist/accumulator sequence is settled and must not move. |
| MORPH knob + CV + attenuverter mixing and clamping | Rack shell (`src/AnalogVCO.cpp`) | — | D-17: param plumbing into a documented `[0,1]` POD field is not DSP. The shell's "THIS FILE DOES NO DSP" rule stands. |
| Per-instance correction state (`pending`, `inject`) | DSP core, held by value inside `VcoCore` | — | CORE-03 / D-14: no static, no global mutable voice state. |
| Sync event injection (Phase 33) | DSP core seam (`MorphBlep::addStep`) | — | D-14: designed here, built there. |
| Spectral measurement (DFT, bin classification, thresholds) | Test target (`tests/`) | — | D-10: libm is available in tests but not in `src/` (the D-18 precedent). |
| Panel control positions | `res/AnalogVCO.svg` (throwaway) | Phase 35 | D-17; replaced wholesale in Phase 35. |

## Standard Stack

This is a technique stack. **No packages are installed, in any ecosystem.** See §Package Legitimacy Audit.

### Core

| Technique | Source | Purpose | Why Standard |
|-----------|--------|---------|--------------|
| **2-point polyBLEP** (quadratic residual) | Välimäki & Huovilainen 2007; Pekonen 2010 `[CITED: research/STACK.md:50]`, canonical form cross-checked `[VERIFIED: numerically, sinc² match to 0.01 dB]` | Band-limit value steps at saw wrap, square edges, pulse edges | Table-free closed form, pure `+ - * /`, O(1) per site, superposes linearly with the morph weighted sum. |
| **2-point polyBLAMP** (cubic residual) | Derived as `∫polyBLEP`; corroborated against Esqueda/Välimäki/Bilbao DAFx-16 `[CITED: dafx.de/paper-archive/2016/dafxpapers/18-DAFx-16_paper_33-PN.pdf]` | Band-limit the triangle's slope corners | The triangle contributes no value jump; without BLAMP its alias floor is **15.0 dB worse at C8/character 0** `[MEASURED]`. |
| **Compact-support character factor** `k = max(0, 1 − w/(2·dt))²` | Derived and calibrated this session `[MEASURED]` | D-03's character-scaling factor | Only compact-support forms avoid alias regressions at high character; the cutoff is the kernel's own 2-sample support, not a tuned constant. |
| **Pending-residual accumulator** | D-13 | Delivers the next-sample half at zero latency | One consistent `dt` and one consistent jump magnitude for both halves — strictly more robust than recomputing at the next sample under audio-rate MORPH/FM (D-16's hardest case). |
| **Exact-integer-cycle rectangular DFT** | D-10 | The TEST-03 alias gate | Zero leakage by construction; alias energy becomes "magnitude at non-harmonic bins" with no window coefficients. |

### Supporting

| Component | Source | Purpose | When to Use |
|-----------|--------|---------|-------------|
| Radix-2 FFT or Goertzel, `double`, test-only | New, `tests/` | The spectral helper | Always for TEST-03. `N = 4096` real-input FFT is ~0.5 ms; a full sweep of 6 characters × 4 notes × 5 morphs × naive+corrected runs in seconds. |
| Rational `tanh` approximation `x(27+x²)/(27+9x²)` | Padé 3/2 `[ASSUMED]` (standard identity; error < 4e-4 on \|x\| ≤ 2, spot-checked this session) | Optional narrow-pulse "reach" factor | **Optional.** MEASURED benefit: 1.3 dB at the single worst grid point, ~0.1 dB mean. Recommend deferring unless the pulse threshold needs it. |
| `tests/VcoBlockDriver.hpp` | Exists | 44.1/48/96 kHz drive | D-11's cross-rate regression. **Caveat:** its unconditional `sampleTime` overwrite constrains the D-10 frequency construction — see §Validation Architecture. |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| 2-point polyBLEP/BLAMP | 4-point (quintic B-spline) forms | Alias attenuation goes from `sinc²` to `sinc⁴` — roughly **doubles the dB attenuation**, so saw@C8 would move from −25.8 to about −36 dB. Still not −60. Explicitly a deferred, operator-level decision (CONTEXT `<deferred>`); do **not** adopt silently. |
| `k = max(0, 1 − w/(2·dt))²` | `k = max(0, 1 − w/(2·dt))` (p = 1) | Statistically equivalent on the alias metric (mean +7.4 vs +7.3 dB `[MEASURED]`), but has a slope discontinuity at the cutoff. Under audio-rate MORPH/CHARACTER modulation that discontinuity is a per-sample step in the correction gain. `p = 2` is C¹ at the cutoff. Prefer `p = 2`. |
| Compact support | `1/(1+W)`, `1/(1+W²)`, sinc-Padé | **Rejected on measurement**: worst-case regressions of −42.7, −29.8 and −36.6 dB respectively versus the naive path. Non-compact factors never stop correcting. |
| Splitting the square into hard@0.5 + soft@duty | One combined site at `duty` (D-04 as literally worded) | Spectrally identical `[MEASURED, 0.0 dB across the whole grid]` but produces **±9.78 V single-sample spikes** at every rate. Split is mandatory for magnitude safety. |

**Installation:**

```bash
# none — no packages, in any ecosystem
```

**Version verification:** not applicable. Nothing is added to `plugin.json`, `package.json`, `requirements.txt`, `Cargo.toml` or any other manifest. The only new build inputs are one header (`src/dsp/MorphBlep.hpp`) and test sources, both picked up by existing Makefile globs (`make test` globs `tests/*.cpp`; a new `src/dsp/*.hpp` needs no Makefile change).

## Package Legitimacy Audit

**Not applicable — this phase installs zero external packages.**

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| *(none)* | — | — | — | — | — | — |

**Packages removed due to [SLOP] verdict:** none — none were proposed.
**Packages flagged as suspicious [SUS]:** none.

Every dependency this phase touches is already in-tree and frozen (`Waveshape.hpp`, `RackCompat.hpp`, `MathConst.hpp`) or standard-library (`<cmath>`, `<cstdint>`). The plan should **not** add a `checkpoint:human-verify` install gate, because there is no install step.

## Architecture Patterns

### System Architecture Diagram

```
 VcoInputs (POD, unchanged)
   pitchCV, coarse, fine, fmVolts, fmAtten, fmConnected,
   morph[0,1], character[0,1], drift, sampleTime, sampleRate
        │
        ▼
 ┌─────────────────────── VcoCore::step()  (src/dsp/VcoCore.hpp) ───────────────────────┐
 │                                                                                       │
 │  volt-domain pitch sum ─► kVcoMaxPitchVolts bound ─► ONE exp2_taylor5 ─► freq         │
 │        │                                                                              │
 │        ▼                                                                              │
 │  Nyquist ceiling ─► NaN-safe floor ─► deltaPhase (double) ─► bound to 0.5             │
 │        │                                                                              │
 │        ▼                                                                              │
 │  phase += deltaPhase ; single-subtract wrap        ──────────► p = (float)phase        │
 │        │                                                              │                │
 │        │                              ┌───────────────────────────────┘                │
 │        ▼                              ▼                                                │
 │  ┌── MorphBlep::step(...) ───────────────────────────────────────────────────┐        │
 │  │                                                                            │        │
 │  │  (A) replicate morphedWave's weight algebra                                │        │
 │  │      segment / frac / pulseDuty ─► W[5] ─► +bleed ring ─► ÷(1+bleedInt)    │        │
 │  │                     │                                                      │        │
 │  │  (B) recompute site GEOMETRY from live Waveshape fields (never cached)     │        │
 │  │      dutySq, pulseDuty, valley, edgeWidths, roundAmount                    │        │
 │  │                     │                                                      │        │
 │  │  (C) emit the FIXED 9-entry site table (pos, magnitude, width, kind)       │        │
 │  │                     │                                                      │        │
 │  │  (D) per site: side ← float compare vs p   (agrees with the frozen branch) │        │
 │  │                dist ← double phase          (exact once-per-cycle tiling)  │        │
 │  │                k    ← max(0, 1 − w/(2dt))²  (D-03)                         │        │
 │  │                now += h·k·r(−s)      pending += h·k·r(1−s)                 │        │
 │  │                                                                            │        │
 │  │  (E) return  inject + now + pending_from_previous_sample                   │        │
 │  │      ▲                                                                     │        │
 │  │      └── addStep(xAhead, jump)  ◄── Phase 33 hard sync (D-14 seam, unused) │        │
 │  └────────────────────────────────────────────────────────────────────────────┘        │
 │        │                                                     │                         │
 │        ▼                                                     ▼                         │
 │  FROZEN Waveshape::morphedWave(p, morph, character, 0.f)  +  correction                │
 │        │                                                                              │
 │        ▼                                                                              │
 │  × 5, returned UNCONDITIONED (Phase 34 owns the output stage)                          │
 └───────────────────────────────────────────────────────────────────────────────────────┘
        │
        ▼
 src/AnalogVCO.cpp  ── knob + CV × attenuverter ─► clamp [0,1] ─► VcoInputs::morph
```

### Recommended Project Structure

```
src/dsp/
├── MorphBlep.hpp        # NEW — kernels + site map + weight algebra (D-12)
├── VcoCore.hpp          # MODIFIED — holds a MorphBlep by value; :484 call site
├── Waveshape.hpp        # FROZEN — called, never edited
└── RackCompat.hpp       # FROZEN — no BLEP kernels here (D-12)
src/
├── AnalogVCO.cpp        # MODIFIED — MORPH CV jack + attenuverter (D-16/D-17)
├── AnalogLFO.cpp        # MUST NOT APPEAR IN THIS PHASE'S DIFF
└── vco_compile_canary.cpp  # MODIFIED — must gain an ACTIVE MorphBlep include
tests/
├── test_vco_spectrum.cpp   # NEW — DFT helper + naive baseline + per-shape gate
├── test_vco_core.cpp       # MODIFIED — :416 oracle, :511 bound, scenario-four grid
└── check_includes.sh       # MODIFIED — VCO_SIDE_ALLOW gains the new test file
```

### Pattern 1: The exact 2-point kernels and the sub-sample convention

**What:** the two residual functions, their sign convention, and precisely what `s` means.

**Derivation (self-contained, so nothing rests on a recalled formula).** Approximate the band-limited impulse by the 2-sample triangular (linear B-spline) basis `h(x) = 1 − |x|` on `[−1,1]`, which has unit area. Integrating once gives the band-limited step; subtracting the trivial unit step gives the **BLEP residual**

```
r(x) =  (x+1)²/2      for x ∈ [−1, 0)      (the sample BEFORE the edge)
r(x) = −(x−1)²/2      for x ∈ [ 0, 1]      (the sample AFTER  the edge)
r(x) =  0             otherwise
```

Integrating `r` once more gives the **BLAMP residual**

```
R(x) = (x+1)³/6       for x ∈ [−1, 0]
R(x) = (1−x)³/6       for x ∈ [ 0, 1]
R(x) = 0              otherwise
```

`∫r = 0` over its support (`+1/6` then `−1/6`), so `R → 0` outside `[−1,1]` as required, and `R` is continuous at `0` with peak `1/6`.

**Sign convention (unambiguous).** Define the jump at a site as `h = value_after − value_before`, signed. Then

```
corrected(x) = naive(x) + h · r(x)                      // value step
corrected(x) = naive(x) + (Δslope_perSample) · R(x)     // slope break
```

where `Δslope_perSample = Δ(dy/dφ) · dt`. Sanity check at the edge itself (`x = 0⁺`, naive has already jumped): `r(0) = −1/2`, so the corrected sample is `pre + h/2` — the band-limited midpoint. Correct.

**Equivalence to the canonical form.** The widely-published two-branch function (Välimäki/Huovilainen; Finke) returns `2·r(x)`, and is applied as `value += (h/2) · polyBLEP(t, dt)`. `[VERIFIED: numerically — max |A−B| = 7e-7 at dt = 0.094, and the corrected saw's harmonic gain matches sinc²(f/fs) to 0.01 dB over 12 harmonics]`

**Contradiction with `research/STACK.md:100-104`.** That snippet returns a **quartic** (`u⁴/6`) and folds `dt` inside the kernel. It is neither the 2-point polyBLAMP (cubic, above) nor the DAFx-16 four-point polyBLAMP (quintic — the paper's Table 1 gives `d⁵/120`, `−d⁵/40 + d⁴/24 + d³/12 + d²/12 + d/24 + 1/120`, `d⁵/40 − d⁴/12 + d²/3 − d/2 + 7/30`, `−d⁵/120 + d⁴/24 − d³/12 + d²/12 − d/24 + 1/120`). The paper says explicitly *"A two-point version of the polyBLAMP function can be found in [21]. However, due to its superior performance, this work focuses solely on the four-point method."* `[CITED: DAFx-16 paper 33]` **Do not copy the STACK.md snippet.**

**When to use:** every site, every sample.

### Pattern 2: The crossing test — forward lookahead with a split source of truth

**What:** the rule that decides whether a site fires this sample and at what sub-sample position.

`VcoCore` advances `phase` and *then* evaluates `morphedWave(p, …)`. The two samples straddling an edge are therefore sample *n−1* (already emitted) and sample *n*. Zero latency requires **forward** detection: at sample *n* we ask *"does site φₑ lie inside the step from `p` to `p + dt`?"* If yes, sample *n* sits at `x = −s` and sample *n+1* at `x = 1 − s`, where `s ∈ (0, 1]` is the distance to the edge in samples. That is exactly D-13's model: *"the sample containing it"* is *n*, *"the sample after the edge"* is *n+1*, and the accumulator carries the second half.

```
s → 0⁺  (edge immediately after sample n) : now += h/2,  pending += 0        ✔ x = 0⁻
s → 1⁻  (edge lands on sample n+1)        : now += 0,    pending += −h/2     ✔ x = 0⁺
```

**The landmine.** `d = φₑ − p` must answer two different questions, and they want two different number types:

| Question | Correct source | Why |
|----------|---------------|-----|
| *Which side of the edge is this sample on?* | the **float** `p`, with the same strict comparison the frozen code uses | `computeSquare:104` tests `phase < 0.5f`; `computePulse:128` tests `phase < duty`. If the correction's side decision disagrees with the frozen branch — which it will whenever `\|φₑ − p\| < 6e-8` — the correction is applied on the wrong side and the error is the **full** `h`. |
| *How far away is the edge?* | the **double** `phase` | `phase` advances by exactly `deltaPhase` each sample in double, so `d` decreases by exactly `dt` and each site fires **exactly once per cycle**. Deriving the distance from the float `p` makes the per-sample interval tiling ragged at the `ulp` level, and a ragged tiling either **misses** an edge (no correction at all, error `h/2`) or **double-fires** it (error `h/2`). |

MEASURED: a pure-double side test produced a systematic `±1.0`-amplitude spike (`max|out|` 2.145 vs 1.957) whenever the phase grid resonated with a site position. The split rule below removes it.

**Example:**

```cpp
// pos is a float; p is THE SAME float handed to morphedWave; phase is the double accumulator
double d = (double)pos - phase;
if (!(pos > p)) d += 1.0;        // strict float compare: mirrors the frozen branch exactly
const float s = (float)(d / dt); // dt is the double deltaPhase
if (s <= 1.f) { /* fire */ }
```

**Anti-patterns to avoid:**
- **Recomputing the "after" half at sample *n+1* from the then-current phase** (Finke's two-branch form). Valid for a fixed-frequency saw; wrong here, because under audio-rate MORPH/FM the jump magnitude, the site position and `dt` have all moved by then. The accumulator uses one consistent set of values for both halves.
- **A one-sample output delay buffer.** Rejected by D-13: it adds declared latency and desyncs the VCO against every other oscillator in the patch.
- **Widening the fire gate above `s = 1` to "catch misses".** It creates double-fires on the following sample instead. Use the double distance; the tiling is then exact to ~1e-16.

### Pattern 3: The site map — nine entries, derived from the frozen code's own branches

**What:** the D-04 fixed union, made precise. **The hard-step positions are exactly the three phase-branch points in `Waveshape.hpp`**, and nothing else in that file steps in value.

| # | Position | Kind | Magnitude (pre-`W`) | Width `w` | Frozen-code origin |
|---|----------|------|---------------------|-----------|--------------------|
| 1 | `0.f` | BLEP hard | `W[2]·2 + W[3]·2(1−c) + W[4]·2(1−c)` | `0` | wrap: `saw = 1−2φ` `:84`; `(phase<0.5)?1:−1` `:104`; `(phase<duty)?1:−1` `:128` |
| 2 | `0.f` | BLEP soft | `W[3]·2c` | `2·(c·0.08)` | `computeSquare` tanh, zero-crossings at `0` and `duty` `:114-120` |
| 3 | `0.f` | BLEP soft | `W[4]·2c` | `2·ewPl` | `computePulse` tanh `:142-148` |
| 4 | `0.5f` | BLEP hard | `W[3]·(−2)(1−c)` | `0` | **`computeSquare:104` branches at `0.5f`, NOT at `duty`** |
| 5 | `dutySq` | BLEP soft | `W[3]·(−2c)` | `2·(c·0.08)` | `duty = 0.5 + c·(0.04 + squareDutySpread)` `:108` |
| 6 | `pulseDuty` | BLEP hard | `W[4]·(−2)(1−c)` | `0` | `computePulse:128` |
| 7 | `pulseDuty` | BLEP soft | `W[4]·(−2c)` | `2·ewPl` | coincident with #6 — `computePulse` uses the same `duty` for both |
| 8 | `0.f` | BLAMP | `W[1]·(−Δ)` | `0.5·(c·0.35)` | triangle peak; `Δ = 2/valley + 2/(1−valley)` |
| 9 | `valley` | BLAMP | `W[1]·(+Δ)` | `0.5·(c·0.35)` | `valley = 0.5 + c(0.10+triAsymmetrySpread)·0.5` `:61` |

with

```
c        = (character < 0.001f) ? 0 : character*character         // :36-38, and the early returns
dutySq   = 0.5f + c*(0.04f + squareDutySpread)                    // :108
pulseDuty= 0.50f − 0.45f*min(max(0, morph*4 − 3), 1)              // :170-171
ewPl     = c*min(0.08f, min(pulseDuty, 1−pulseDuty)*0.8f) / (1 + pulseEdgeSpread)   // :134-139
valley   = 0.5f + c*(0.10f + triAsymmetrySpread)*0.5f             // :60-61
Δ        = 2/valley + 2/(1 − valley)                              // :65,:68 slopes
```

**Why the square splits and the pulse does not.** `computeSquare` derives its hard rectangle from `phase < 0.5f` but centres its tanh on `duty/2` with half-width `duty/2`, so the soft edge sits at `duty` — a different place. `computePulse` derives *both* from the same `duty`, so they coincide. This asymmetry is a quirk of the original LFO code, and it is frozen. `[VERIFIED: measured — at character 0.5 the hard jump at 0.5 is exactly −1.500000 = −2(1−c) while sq(0.49) = +0.940 and sq(0.53) = −0.940 bracket a separate soft edge centred on 0.51]`

**Why "recompute, never cache" is right and will matter more later.** `dutySq`, `valley` and `ewPl` all move with `character` *now*, and move again with `squareDutySpread` / `triAsymmetrySpread` / `pulseEdgeSpread` once Phase 34's drift starts writing those fields per sample. D-04's rule is what keeps this correct then.

### Pattern 4: Weight algebra — replicate, do not approximate

```cpp
const float scaled = m * 4.f;
int   segment = (int)scaled; if (segment > 3) segment = 3;   // mirrors std::min((int)scaled, 3)
const float frac = scaled - (float)segment;

float W[5] = {0,0,0,0,0};
if (segment == 3) W[4] = 1.f;                       // :179-182 — the direct-duty special case
else { W[segment] += 1.f - frac; W[segment+1] += frac; }

if (character >= 0.001f) {                          // :188
    const float effectiveBleed = fmax(0.f, 0.04f + bleedSpread);        // :192
    float bleedIntensity = fmax(0.f, c * effectiveBleed);               // :193-198 (bleedLfo = 0)
    W[(segment - 1 + 5) % 5] += bleedIntensity * (1.f - frac);          // :201,:205,:208
    W[(segment + 2) % 5]     += bleedIntensity * frac;                  // :202,:206,:208
    const float norm = 1.f / (1.f + bleedIntensity);                    // :212  ← D-05
    for (int i = 0; i < 5; ++i) W[i] *= norm;
}
```

Folding D-05's normalization into `W` once is exact and cheaper than dividing every correction. **Verification of the whole algebra:** at `morph = 0`, `character = 0.5` the predicted `morphedWave` jump at phase 0 is `bleedIntensity·2(1−c)/(1+bleedIntensity) = 0.014851`; MEASURED `0.014853`. `[VERIFIED]` The bleed ring D-04 flagged is real: at `morph = 0` (a pure sine to the user) the narrow pulse bleeds in at full left-weight and produces a genuine −23 dB alias floor at C9/character 1 `[MEASURED]`.

### Pattern 5: The D-03 character factor

```
w = the site's equivalent-ramp width, in units of phase (table above; 0 for hard sites)

k = max(0, 1 − w/(2·dt))²
```

**Why this shape, and why the constant is not arbitrary.** The 2-sample kernel's support *is* two samples. An edge already ≥ 2 samples wide lies entirely inside the kernel's own support: it is already band-limited on the sample grid, and a step-shaped residual is the wrong correction for it. So the cutoff `w = 2·dt` is read off the kernel, not fitted.

**Limits (D-03's binding constraints):**
- `w = 0` (a true hard step) → `k = 1` exactly. Every hard site therefore gets full authority at every character.
- `w ≫ dt` → `k = 0` exactly, not asymptotically. **The exactness is the load-bearing property** (see §Common Pitfalls P-1).
- Pure `+ − * /` plus one comparison. **Divides by `dt` only — never by `edgeWidth`** (relevant to D-15's rationale).
- Monotone decreasing, `C¹` at the cutoff (`p = 2`), and sample-rate-aware for free.

**Equivalent-ramp widths** (the phase span over which the softened edge traverses its excursion, matching the maximum slope):

| Site family | Frozen quantity | `w` | Derivation |
|-------------|-----------------|-----|------------|
| square tanh | `edgeWidth = c·0.08` `:110` | `2·edgeWidth` | `tanh(δφ/edgeWidth)` has unit slope at centre and swings 2 → equivalent ramp width 2·edgeWidth `[VERIFIED: sq(duty+0.05) = −0.5546 = tanh(−0.625) exactly]` |
| pulse tanh | `edgeWidth = c·min(0.08, maxEdge)/(1+pulseEdgeSpread)` `:134-139` | `2·edgeWidth` | same |
| triangle rounding | `roundAmount = c·0.35` `:71` | `0.5·roundAmount` | the two half-widths sum to `roundAmount·valley/2 + roundAmount(1−valley)/2 = roundAmount/2` `[VERIFIED: derived and confirmed against the measured slope breaks]` |
| all hard sites | — | `0` | |

**C++11 form (one divide, no `pow`, no branch on `w`):**

```cpp
static inline float charFactor(float w, float dt) {
    const float u = 2.f * dt - w;
    if (!(u > 0.f)) return 0.f;          // negated: also catches NaN dt
    const float t = u / (2.f * dt);      // == 1 - w/(2*dt)
    return t * t;
}
```

**Calibration evidence** — sweep over `k = max(0, 1 − a·w/dt)^p`, 6 characters × 4 notes × 5 morph centres, floors capped at −80 dB (below which nothing is audible), `Δ` versus the naive path `[MEASURED]`:

| `a` | `p` | worst regression (dB) | mean improvement (dB) |
|-----|-----|----------------------:|----------------------:|
| 0.25 | 1 | **−20.3** | 6.8 |
| 0.33 | 1 | **−17.6** | 7.4 |
| 0.50 | 1 | −8.2 | 7.4 |
| **0.50** | **2** | **−1.7** | **7.3** |
| 0.67 | 1 | −1.7 | 7.4 |
| 0.67 | 2 | −1.7 | 6.8 |
| 1.00 | 2 | −1.7 | 6.4 |

`a = 0.5, p = 2` sits at the start of the plateau: it reaches the −1.7 dB worst-regression floor while retaining the highest mean improvement. `a = 0.5` on the equivalent-ramp width is exactly `k = max(0, 1 − w/(2·dt))²`.

**Comparison against non-compact forms** (same metric, worst regression / mean improvement) `[MEASURED]`:

| Factor | worst regression | mean improvement |
|--------|-----------------:|-----------------:|
| `k = 1` (full authority) | **−60.4** | 3.7 |
| `1/(1+W)` | **−42.7** | 5.2 |
| `1/(1+0.4112·W²)` (sinc Padé) | **−36.6** | 5.7 |
| `1/(1+W²)` | **−29.8** | 6.1 |
| **`max(0, 1 − W/2)²`** | **−1.7** | **7.3** |
| soft sites uncorrected (`k = 0`) | −1.2 | 5.6 |

### Anti-Patterns to Avoid

- **Assuming over-correction is benign.** `research/STACK.md:40` and CONTEXT `<specifics>` both say "err toward full authority". MEASURED, at character 1 / C6 / morph = square: naive −60.1 dB, full authority −29.9 dB. **A 30 dB regression.** The argument (character is a lowpass-ish coloration, so extra correction only adds HF rolloff) is wrong because the injected residual is a *step*-shaped correction added to a signal that has no step — it is new broadband energy, not a filter.
- **Placing the square's hard step at `duty`.** See P-2.
- **Deriving the side decision from the double phase.** See P-3.
- **Reducing the saw's correction with character.** See P-4.
- **Caching site positions.** D-04; they move with `character` now and per sample in Phase 34.
- **`if constexpr`, `inline constexpr` variables, `std::clamp`, in-class `static constexpr` arrays indexed at runtime.** The last one is the exact bug that got v2.0.0 rejected from the VCV Library (`research/PITFALLS.md:190`). If the 9-entry table is an array, make it **function-local** (`const` / plain local) or namespace-scope `static constexpr`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Deciding whether the current sample is before or after a moving edge | A tolerance-based "close enough" test, or a widened `s ≤ 1 + ε` gate | The strict float compare against the *same* `p` the frozen call saw | A widened gate trades a missed edge for a double-fired edge; both are single-sample, full-amplitude clicks. The strict compare is exact by construction because it *is* the frozen branch. |
| Discovering the effective jump at a site | Probing `morphedWave(φ ± ε)` | The analytic table above | D-01 (cost); and the table is now verified to 6 decimals against the code. |
| Knowing whether an edge is "already band-limited" | A frequency-domain estimate, a filter, or an oversampled probe | `k = max(0, 1 − w/(2·dt))²` | The kernel's own support answers the question in three flops. |
| Measuring alias energy | Windowing + guard bands + a window-dependent floor | Integer cycles per block + rectangular window (D-10) | Leakage is exactly zero; the floor depends on one thing instead of two. |
| Separating aliases from harmonics in the DFT | Heuristics about "which bins look like harmonics" | `N = 2^k` with an **odd** cycle count `K` | Any odd `K` is coprime with a power-of-two `N`, so folded aliases provably never land on a true-harmonic bin (see §Validation Architecture). |
| Bit-stability of the polynomials | Nothing — but do not lose it | Keep `-ffp-contract=off` | polyBLEP/BLAMP are FMA-friendly `a*b+c` chains; without the flag GCC and clang contract them differently (`research/STACK.md:75`). |

**Key insight:** in this phase nearly every "clever" shortcut trades a *statistical* improvement for a *deterministic* full-amplitude glitch. The spectral metric averages those glitches into a broadband floor and under-weights them; the output-magnitude envelope sees them immediately. Gate on both.

## Common Pitfalls

### P-1: A character factor without compact support injects more alias than it removes
**What goes wrong:** at high character the real edges are several samples wide and the naive path is already clean (e.g. square at C6/character 1 measures −60.1 dB). A factor like `1/(1+W²)` still returns ~0.05–0.3 there, and that residual step-shaped correction is broadband injected energy.
**Why it happens:** the residual is the correct shape for a *step*, not for a wide smooth ramp. Scaling it down reduces the amplitude of the wrong correction; it does not make it the right one.
**How to avoid:** use a factor that reaches **exactly zero** at a finite width. `max(0, 1 − w/(2·dt))²`.
**Warning signs:** the corrected alias floor is *worse* than the naive floor at `character = 1` and low notes. `[MEASURED: −60.4 dB worst regression at k = 1]`

### P-2: Placing the square's hard step at `duty` produces full-amplitude spikes
**What goes wrong:** `computeSquare` flips its rectangle at `phase < 0.5f` but its tanh crosses zero at `duty = 0.5 + c(0.04+spread)`. If the hard-step correction is placed at `duty`, then for every sample landing in `[0.5, duty)` the naive has already flipped while the correction still thinks the edge is ahead — and injects `≈ h/2 ≈ ±1`.
**Why it happens:** D-04's prose says "the square duty edge" (singular); the code has two.
**How to avoid:** two site entries — hard at `0.5f`, soft at `dutySq`.
**Warning signs:** `max|out|` measured at `1.96` (±9.78 V) at *every* `dt`, versus `1.1047` (±5.52 V) with the split. The spectral metric shows **0.0 dB difference** — it cannot see this. `[MEASURED]`
**Scale:** the window is `c·(0.04 + spread)` wide and the spike is `(1−c)`-weighted, so the damage peaks at `c ≈ 0.5` — around `character ≈ 0.71`, mid-knob.

### P-3: Splitting the side decision and the distance across two number types is required, not optional
**What goes wrong:** using the double `phase` for both gives a systematic sign error whenever `|φₑ − phase| < ulp(float)`; using the float `p` for both gives a ragged per-sample interval tiling that misses or double-fires edges at a rate of ~`6e-8/dt` per cycle.
**Why it happens:** `p = (float)phase` loses ~6e-8 of absolute resolution, and the frozen branch compares against `p`, not `phase`.
**How to avoid:** side from `pos > p` (float, strict); distance from `(double)pos − phase`.
**Warning signs:** a `max|out|` scan across `dt ∈ [0.0005, 0.5]` shows the naive envelope exceeded at a `dt` that happens to divide a site position evenly. The prototype hit exactly this at `dt = 0.0005`, `pulseDuty = 0.374` (748 samples per edge, exactly). `[MEASURED]`

### P-4: The saw's soft reset does **not** soften the wrap
**What goes wrong:** D-03's corollary states *"as character rises the wrap's effective step shrinks and its correction shrinks with it."* It does not. `curvedSaw(0) = 1` before the reset is applied, and the reset blends *from* `resetValue = 1` *toward* `curvedSaw`, so at `φ = 0` both are `1`. The wrap jump is `+2.000000` at character 0, 0.25, 0.5, 0.75 and 1.0 `[MEASURED, six decimals]`.
**Why it matters:** the saw site's width is `0` and its `k` is `1` at every character. A "self-limiting" correction there would silently under-correct the one shape whose alias floor never improves with character (naive saw C8: −15.6 dB at character 0, −14.7 dB at character 1).
**How to avoid:** give site #1 the saw's full `+2` with `w = 0`.
**What the reset actually does:** it forces the slope to `0` at `φ = 0⁺` — a first-derivative break sitting on top of a value step. Its BLAMP magnitude is `≈ 4·dt²/6` versus the BLEP's `≈1`, i.e. **~3.5 orders of magnitude smaller at `dt = 0.02`**. Ignore it. D-03's *conclusion* (don't correct the reset separately) is right; only its stated premise is wrong. **Record the corrected premise so no later phase inherits the falsified one** — the same discipline D-15 applied to deferred item 6.

### P-5: −60 dB is unreachable, and this is a property of the technique
**What goes wrong:** the phase is planned against the roadmap's ≈−60 dB and fails.
**Why it happens:** 2-sample polyBLEP multiplies the spectrum by `sinc²(f/f_s)`, which is only −7.8 dB at Nyquist and −10.5 dB at the first alias of a C8 saw. `[VERIFIED: measured harmonic gain matches sinc² to 0.01 dB across 12 harmonics]` The first alias of a saw at C8 is the 6th harmonic at `1/6` of the fundamental (−15.6 dB); ~10 dB of attenuation lands it at −25.8 dB.
**Corroboration:** DAFx-16 Table 2 reports 46 dB SNR for a *four*-point polyBLAMP triangle at C8 versus 45 dB for 4× oversampling and 30 dB trivial. `[CITED: DAFx-16 paper 33]` Four-point would roughly double our dB attenuation (`sinc⁴`) and still land near −36 dB for the saw.
**How to avoid:** pin D-09's per-shape thresholds from the D-08 baseline. Treat ≈−60 dB as the roadmap's stated *target*, which the roadmap itself already qualifies with "pinned empirically".

### P-6: A single per-shape threshold cannot be both RED-on-naive and green-on-corrected
**What goes wrong:** the triangle at C8 improves by 15.0 dB at `character = 0` (−33.8 → −48.8) and by **0.0 dB** at `character = 1` (−33.5 → −33.5, because the corner is already 7.7 samples wide and `k = 0` correctly). Any single number for "triangle at C8" is either passed by the naive path (vacuous) or failed by the correct implementation.
**How to avoid:** D-09's thresholds must be indexed by `(morph region, note, character)`, not by morph region alone. The full matrix is in §Validation Architecture.

### P-7: The gate's own frequency error can dominate the floor it asserts
**What goes wrong:** `deltaPhase` cannot be placed exactly on `K/N` by choosing `pitchCV`. MEASURED best achievable over a ±4096-ULP scan: **1.5e-3 bins**, giving rectangular-window leakage into the adjacent bin at **−56.5 dB** (44.1 kHz, K = 389). Any threshold tighter than about −50 dB would then be measuring `exp2_taylor5`'s output granularity, not the DSP. That is precisely D-10's "the gate measures its own window" failure in a different costume.
**How to avoid:** see §Validation Architecture — either bound the thresholds above the measured leakage floor and *assert that bound in the test*, or micro-tune the injected `sampleTime` (measured: **−100 dB** leakage at ≤ 5 ppm deviation from nominal).

### P-8: A new test file silently breaks `check_includes.sh [1/7]`
**What goes wrong:** `[1/7]` is a **denylist** — every source file not in `VCO_SIDE_ALLOW` is scanned for VCO includes. A new `tests/test_vco_spectrum.cpp` that includes `dsp/VcoCore.hpp` is treated as an LFO TU and fails.
**How to avoid:** make the `VCO_SIDE_ALLOW` entry a plan task with its own rationale (Phase 31 D-23's lesson), not a gate-time discovery.

### P-9: `src/vco_compile_canary.cpp` must gain an ACTIVE include the moment the header lands
**What goes wrong:** `check_canary.sh [5/5]` strips comment lines first, so *"the commented Phase 32 placeholder in the canary does NOT satisfy the check once `dsp/MorphBlep.hpp` actually exists."* — the guard's own words (`tests/check_canary.sh:400-402`).
**How to avoid:** same commit as the header. **Good news:** `MorphBlep.hpp` is already pre-wired in `check_includes.sh` (lines 261, 317, 337) and `check_canary.sh` (lines 414, 452-453, 466) — `[VERIFIED: grepped]` — so no guard *edits* are needed for the header itself, only the canary include and the new test-file allowlist entry.

### P-10: The `±5.55 V` bound at `test_vco_core.cpp:511` must be re-derived from measurement
**What goes wrong:** the existing derivation reasons about `morphedWave` alone; corrections are additive and bipolar.
**MEASURED, with the recommended implementation** (split square, float side, double distance, `a=0.5 p=2`), over `morph × character` grids of 400 × 40 points at each `dt`:

| `dt` | ≈ note @44.1 kHz | `max\|correction\|` | `max\|out\|` | `× 5` |
|------|------------------|--------------------:|-------------:|------:|
| 0.0005 | 22 Hz | 1.166 | **1.1047** | 5.523 V |
| 0.0200 | 882 Hz | 1.199 | 1.1012 | 5.506 V |
| 0.0949 | 4185 Hz | 1.271 | 1.1047 | 5.523 V |
| 0.1897 | 8366 Hz | 1.602 | 1.1047 | 5.523 V |
| 0.2500 | 11025 Hz | 1.705 | 1.1047 | 5.523 V |
| 0.4000 | 17640 Hz | 1.836 | 1.1659 | 5.829 V |
| **0.4950** | **21830 Hz (the Nyquist ceiling)** | 1.878 | **1.8395** | **9.198 V** |
| 0.5000 | 22050 Hz (`kVcoMaxDeltaPhase`) | 1.882 | 1.2300 | 6.150 V |

**The result is better than expected and worth stating plainly: at every musically reachable rate the corrected output stays inside the existing `1.1047` envelope — the corrections consistently *reduce* the excursion at edges, exactly as CONTEXT's discretion note anticipated.** The existing 6.0 V loose bound therefore survives for all normal use. It is exceeded only at `dt ≈ 0.495`, i.e. when the guarded frequency is pinned at `0.495·f_s` and the morph is at the 5%-duty pulse with `character ≈ 0` — the deliberate D-07 overlapping-edge case at literally Nyquist, where the naive path is already meaningless.
**How to handle:** the planner should decide between (a) raising the loose bound to ~10.0 V with this table as provenance, or (b) keeping 6.0 V and excluding the at-ceiling case with an explicit, documented carve-out. **Recommendation: (a)** — a single number with measured provenance, still comfortably inside Rack's ±12 V norm, and no exception to invite a second exception (D-09's own reasoning about carve-outs).

### P-11: `-ffp-contract=off` is load-bearing here specifically
`research/STACK.md:75`. The residuals are chains of `a*b+c`. Do not add `-ffast-math`; do not drop the flag; capture nothing with different flags.

### P-12: Mirror the frozen code's exact expressions, including its early-return thresholds
`character < 0.001f` short-circuits every `compute*` and gates the whole bleed block at `:188`. `MorphBlep` must use `c = (character < 0.001f) ? 0.f : character*character` and must gate the bleed weights on the same comparison, or the weight vector disagrees with the naive path in a band the tests will sweep through.

### P-13: Audio-rate MORPH is the hardest case and it is safe
`[MEASURED]` at 44.1/48/96 kHz × notes 2093/4186/8372 Hz × morph LFO rates 50/500/2000 Hz, `character = 1`, 20 000 samples each: every sample finite, `max|out| ≤ 1.3171` (the single worst point being 8372 Hz with a 2 kHz morph sweep). No special handling needed — but this is the D-16 UAT case and the grid worth keeping.

### P-14: Hostile timing reaches only one divisor
`[MEASURED]` after `VcoCore`'s existing guards, `dt ∈ {0, −1, subnormal, 1e300, NaN, +inf}` all produce finite, bounded output. The recommended formulation divides by `dt` **only** — `charFactor` divides by `2·dt`, and the sub-sample position divides by `dt`. There is **no division by `edgeWidth`** unless the optional pulse-reach refinement is adopted (and that one is already guarded by `ewPl > 0`). D-15's guards should still be added — `MorphBlep` must not rely on its caller — using the negated-comparison idiom: `if (!(dt > 0.f)) return pending_only;`.

## Code Examples

### The header skeleton (C++11-strict, Rack-free, table-free)

```cpp
#pragma once
// src/dsp/MorphBlep.hpp
//
// DEPENDENCY ON FROZEN INTERNALS (D-01). This header duplicates knowledge of
// src/dsp/Waveshape.hpp's internals: its crossfade weights, its bleed ring and
// normalization, its per-shape branch positions and its character-deformation
// widths. That duplication is safe ONLY because Waveshape.hpp is byte-pinned by
// tests/check_frozen.sh and cannot drift underneath this file. If that pin is
// ever lifted, every magnitude and position below must be re-derived.

#include <cmath>
#include "dsp/Waveshape.hpp"   // FROZEN — read, never edit

namespace forge {

// D-03: character-scaling factor. Reaches EXACTLY zero once the softened edge is
// as wide as the 2-sample kernel's own support; exactly one when the edge is a
// true step. Divides by dt only — never by an edge width.
inline float morphBlepCharFactor(float w, float dt) {
    const float twoDt = dt + dt;
    const float u = twoDt - w;
    if (!(u > 0.f)) return 0.f;      // negated: NaN dt lands here too
    const float t = u / twoDt;
    return t * t;
}

struct MorphBlep {
    float pending = 0.f;   // residual owed to the NEXT sample (D-13)
    float inject  = 0.f;   // residual owed to THIS sample, supplied externally (D-14)

    void reset() { pending = 0.f; inject = 0.f; }

    // D-14 SYNC SEAM — designed here, used by Phase 33. xAhead is the edge's
    // position in samples relative to THIS output sample: 0 = the edge lands
    // immediately after this sample, 1 = it lands on the next sample. jump is
    // (value_after - value_before), already scaled by whatever weights the
    // caller owns. Feeds the same accumulator as the morph sites, so several
    // events and several morph sites compose by summation (D-07).
    void addStep(float xAhead, float jump) {
        if (!(xAhead >= 0.f) || xAhead > 1.f) return;
        const float u = 1.f - xAhead;
        inject  += jump * ( 0.5f) * u * u;
        pending += jump * (-0.5f) * xAhead * xAhead;
    }

    // One sample of correction. Call AFTER the phase update, with the SAME
    // float p that is handed to Waveshape::morphedWave.
    float step(const Waveshape& wv, double phase, float p, double dt,
               float morph, float character);
};

} // namespace forge
```

### The per-sample body

```cpp
inline float forge::MorphBlep::step(const Waveshape& wv, double phase, float p,
                                    double dt, float morph, float character) {
    float now = inject + pending;
    inject = 0.f; pending = 0.f;

    const float fdt = (float)dt;
    if (!(fdt > 0.f)) return now;          // D-15: negated, never forge::clamp

    // --- (A) weight algebra, replicating Waveshape::morphedWave ---------------
    const float c = (character < 0.001f) ? 0.f : character * character;
    const float scaled = morph * 4.f;
    int segment = (int)scaled; if (segment > 3) segment = 3;
    const float frac = scaled - (float)segment;
    const float pulseFrac = (scaled > 3.f) ? (scaled - 3.f) : 0.f;
    const float pulseDuty = 0.50f - 0.45f * ((pulseFrac < 1.f) ? pulseFrac : 1.f);

    float W[5] = { 0.f, 0.f, 0.f, 0.f, 0.f };
    if (segment == 3) W[4] = 1.f;
    else { W[segment] += 1.f - frac; W[segment + 1] += frac; }

    if (character >= 0.001f) {
        float eb = 0.04f + wv.bleedSpread; if (eb < 0.f) eb = 0.f;
        float bi = c * eb;                 if (bi < 0.f) bi = 0.f;
        W[(segment - 1 + 5) % 5] += bi * (1.f - frac);
        W[(segment + 2) % 5]     += bi * frac;
        const float norm = 1.f / (1.f + bi);          // D-05, exact
        for (int i = 0; i < 5; ++i) W[i] *= norm;
    }

    // --- (B) geometry, recomputed every sample (D-04) -------------------------
    const float dutySq = 0.5f + c * (0.04f + wv.squareDutySpread);
    const float wSq    = 2.f * (c * 0.08f);
    const float lo     = (pulseDuty < 1.f - pulseDuty) ? pulseDuty : 1.f - pulseDuty;
    const float capPl  = (0.08f < lo * 0.8f) ? 0.08f : lo * 0.8f;
    const float wPl    = 2.f * (c * capPl / (1.f + wv.pulseEdgeSpread));
    const float valley = 0.5f + c * (0.10f + wv.triAsymmetrySpread) * 0.5f;
    const float wTri   = 0.5f * (c * 0.35f);
    const float triBrk = 2.f / valley + 2.f / (1.f - valley);
    const float hardSq = W[3] * 2.f * (1.f - c);
    const float hardPl = W[4] * 2.f * (1.f - c);

    // --- (C) the fixed 9-entry site set --------------------------------------
    // Function-local arrays: NOT an in-class static constexpr indexed at runtime
    // (research/PITFALLS.md:190 — the exact form that got v2.0.0 rejected).
    const float pos [9] = { 0.f,   0.f,      0.f,   0.5f,   dutySq,      pulseDuty, pulseDuty, 0.f,             valley        };
    const float mag [9] = { W[2]*2.f + hardSq + hardPl,
                                   W[3]*2.f*c, W[4]*2.f*c,
                                          -hardSq, W[3]*(-2.f)*c, -hardPl,   W[4]*(-2.f)*c,
                                                                                    W[1]*(-triBrk), W[1]*triBrk  };
    const float wid [9] = { 0.f,   wSq,      wPl,   0.f,    wSq,         0.f,       wPl,       wTri,            wTri          };
    const int   kind[9] = { 0,     0,        0,     0,      0,           0,         0,         1,               1             };

    // --- (D) place each correction -------------------------------------------
    for (int i = 0; i < 9; ++i) {
        if (mag[i] == 0.f) continue;
        // SIDE from the float compare — the frozen branch's own test (P-3).
        // DISTANCE from the double accumulator — exact once-per-cycle tiling.
        double d = (double)pos[i] - phase;
        if (!(pos[i] > p)) d += 1.0;
        const float s = (float)(d / dt);
        if (!(s <= 1.f)) continue;
        const float k = morphBlepCharFactor(wid[i], fdt);
        if (k == 0.f) continue;
        const float u = 1.f - s;
        if (kind[i] == 0) {                       // polyBLEP: r(-s), r(1-s)
            const float h = mag[i] * k;
            now     += h * ( 0.5f) * u * u;
            pending += h * (-0.5f) * s * s;
        } else {                                  // polyBLAMP: R(-s), R(1-s)
            const float g = mag[i] * fdt * k;     // slope-per-phase -> per-sample
            now     += g * (u * u * u) * (1.f / 6.f);
            pending += g * (s * s * s) * (1.f / 6.f);
        }
    }
    return now;
}
```

### The `VcoCore.hpp:484` call site

```cpp
    const float p = (float)phase;
    const float morph = clamp(in.morph, 0.f, 1.f);
    const float character = clamp(in.character, 0.f, 1.f);

    // D-12: ONE call into the frozen Waveshape — a call, never an edit.
    const float naive = wave.morphedWave(p, morph, character, 0.f);
    // Phase 32 (CORE-02 / AA-01..05): additive band-limiting correction.
    const float sample = naive + blep.step(wave, phase, p, deltaPhase, morph, character);
    tel.displayPhase = p;
```

**Keeping the naive path callable for D-08** (CONTEXT discretion). The cleanest mechanism given the above: `naive` is already a named local, so a test-only second entry point on `VcoCore` — `float stepNaive(const VcoInputs&)` that runs the identical body and returns `5.f * naive` — costs one small duplicated function and no runtime branch in `step()`. A boolean member would put a per-sample branch in the audio path; a preprocessor flag would make the baseline and the gate different builds, which defeats the before/after comparison.

### The shell wiring (MORPH-02 / D-16 / D-17)

```cpp
// src/AnalogVCO.cpp — param plumbing only; no DSP (D-17).
float morph = params[MORPH_PARAM].getValue();
if (inputs[MORPH_CV_INPUT].isConnected())
    morph += inputs[MORPH_CV_INPUT].getVoltage() * 0.1f     // ±10 V -> ±1.0
           * params[MORPH_ATTEN_PARAM].getValue();          // bipolar -1..+1
in.morph = rack::math::clamp(morph, 0.f, 1.f);              // the POD's documented range
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Oversampling for VCV oscillator anti-aliasing | minBLEP (`dsp::MinBlepGenerator<16,32>`) | Rack 2, Fundamental VCO-1/2 | Confirms BLEP-family is the standard route; table-based, so SDK-coupled and unavailable to a Rack-free core `[CITED: research/STACK.md:112]` |
| polyBLEP only for value steps | polyBLEP + polyBLAMP for slope corners | DAFx-2016 | The triangle end of a morph aliases badly without BLAMP; MEASURED 15.0 dB at C8 `[CITED: DAFx-16 paper 33]` |
| Two-point polyBLAMP | Four-point (quintic) polyBLAMP | DAFx-2016 | The paper explicitly declines the two-point form "due to its superior performance" of the four-point one. We use two-point by scope decision; the escalation path is recorded, not taken. |
| "BLEP the active shape" | Weighted-sum jumps across all shapes at fixed sites | DAFx-2017 polygonal oscillator | Direct precedent for D-04 `[CITED: research/STACK.md:116]` |

**Deprecated/outdated for this phase:**
- `research/STACK.md:100-104`'s polyBLAMP snippet — wrong polynomial order and wrong `dt` placement. Superseded by the derivation in Pattern 1.
- `research/STACK.md:40`'s pristine-jump recommendation and its "over-correction is benign" rationale — overridden by D-01 *and* falsified by measurement.
- `research/STACK.md:61`'s kernel placement in `RackCompat.hpp` — rejected by D-12.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | doctest (header-only, in-tree), driven by `make test` |
| Config file | `Makefile` — `TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off`; `TEST_SOURCES` globs `tests/*.cpp` |
| Quick run command | `make test` |
| Full suite command | `make test && make strict && make guards` |
| New wiring needed | **none for the build** (glob); **one line** in `tests/check_includes.sh` `VCO_SIDE_ALLOW`; **one active include** in `src/vco_compile_canary.cpp` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| TEST-03 / AA-01 | Alias floor per `(morph region, note, character)` at 44.1 kHz | spectral invariant | `make test` (`test_vco_spectrum.cpp`) | ❌ Wave 0 |
| TEST-03 / D-08 | Naive baseline recorded and the gate proven RED before `MorphBlep` | spectral invariant | `make test` | ❌ Wave 0 |
| D-11 | Same assertions at 48 kHz and 96 kHz | regression | `make test` | ❌ Wave 0 |
| AA-02 | Triangle BLAMP fires and improves the floor | spectral invariant | `make test` (tri rows at character 0) | ❌ Wave 0 |
| AA-03 | Overlapping pulse edges sum, not overwrite | unit + spectral | `make test` (pulse at C9, `duty=0.05`) | ❌ Wave 0 |
| AA-04 | Characterized jumps: correction → 0 as character → 1 on softened sites | unit | `make test` — assert `morphBlepCharFactor` limits and one measured jump per site | ❌ Wave 0 |
| AA-05 | Table-free, Rack-free, C++11 | build gate | `make strict && make guards` | ✅ exists |
| CORE-02 | Frozen headers untouched; new header in the seam | build gate | `make guards` (`check_frozen`, `check_canary`, `check_includes`) | ✅ exists |
| CORE-03 / D-14 | No static/global mutable state in `MorphBlep`; two interleaved cores do not interact | unit | `make test` (`test_vco_core.cpp` interleave case) | ✅ pattern exists |
| MORPH-02 | Audio-rate MORPH sweep stays finite and bounded | unit | `make test` | ❌ Wave 0 |
| D-15 | Hostile `sampleTime`/`sampleRate` grid extended to ±inf, subnormal, very-large-finite | unit | `make test` (`test_vco_core.cpp` scenario four) | ✅ file exists, grid ❌ |
| D-10 self-check | The gate's own leakage floor is below every threshold it asserts | unit | `make test` | ❌ Wave 0 |

### The spectral construction (D-10), concretely

**Block length:** `N = 4096`. At 44.1 kHz that is 92.9 ms — long enough for 97 cycles of the lowest test note and short enough that a full 6 × 4 × 5 × 2 sweep runs in seconds.

**Cycle count `K` must be ODD.** With `N = 2¹² `, `gcd(K, N) = 1` for every odd `K`. Harmonic `n` sits at bin `nK`; a folded alias lands on bin `nK mod N` reflected. Aliases collide with true harmonics only if `N | (n ± m)K`, which for `gcd(K,N)=1` requires `|n ± m| ≥ N = 4096` — i.e. never, for any harmonic the waveform actually carries. **Choosing `N` a power of two reduces D-10's coprimality requirement to "pick an odd number".** Record that reasoning in the test; it is the whole justification for the bin classification.

**Test frequencies** (`f = K·f_s/N`):

| Rate | `K` | `f` (Hz) | ≈ note | harmonics below Nyquist |
|------|-----|---------:|--------|------------------------:|
| 44100 | 97 | 1044.4 | C6 | 21 |
| 44100 | 195 | 2099.5 | C7 | 10 |
| 44100 | 389 | 4188.2 | **C8** | 5 |
| 44100 | 777 | 8366.9 | **C9** | 2 |
| 48000 | 357 | 4183.6 | C8 | 5 |
| 96000 | 179 | 4195.3 | C8 | 11 |

The 48 kHz and 96 kHz entries are chosen to land on **the same note** as the 44.1 kHz C8 row, so D-11's cross-rate regression compares like with like. All are odd.

**Alias metric.** Discard one warm-up block of `N` samples (the pending accumulator and the phase must be in steady state; with `dt = K/N` the following block is exactly periodic). FFT the next `N` samples with a rectangular window. Let `H = { nK : 1 ≤ n ≤ ⌊(N/2 − 1)/K⌋ }`. Then

```
aliasPeak_dB = 20·log10( max_{i ∈ [1, N/2] \ H} |X_i| / |X_K| )
```

Bin 0 is excluded (the narrow pulse legitimately carries DC; Phase 34 owns the DC blocker). Report alias RMS alongside the peak — the peak is the gate, the RMS is the diagnostic.

**Placing `deltaPhase` on the bin centre — the one genuinely hard part.** `[MEASURED]`

| Method | best `deltaPhase` error | implied adjacent-bin leakage | cost |
|--------|------------------------:|-----------------------------:|------|
| Bisect on `pitchCV`, use `VcoBlockDriver` unchanged | 2.3e-4 … 1.5e-3 bins | **−56 dB … −73 dB** | zero |
| Bisect on `pitchCV`, then nudge the injected `sampleTime` to the nearest float of `(K/N)/freq` | ~1e-5 bins | **≈ −100 dB** | a local sample loop instead of `VcoBlockDriver::run` (whose `sampleTime` overwrite is documented as unconditional and must not become conditional) |

**Recommendation:** start with the first method — it keeps the shared harness and needs no new driver — and make the gate **self-checking**: measure the achieved `|deltaPhase − K/N|·N`, compute the implied leakage, and `REQUIRE` that it is at least 10 dB below the threshold being asserted in that case. If any threshold ends up tighter than about −50 dB (only the sine rows do), switch that case to the second method. Either way the gate must assert its own noise floor — otherwise it can pass by measuring `exp2_taylor5`'s output granularity, which is D-10's stated failure mode wearing a different costume.

### D-08 baseline and D-09 threshold evidence

Peak alias, dB relative to the fundamental, 44.1 kHz, spreads = 0, **naive → corrected with the recommended implementation.** `[MEASURED — prototype; D-08's job is to reproduce these against the real `MorphBlep` and pin from *those* numbers]`

| character | note | sine | triangle | saw | square | pulse (5%) |
|-----------|------|------|----------|-----|--------|------------|
| **0.00** | C6 | −150.7 → −150.7 | −54.5 → **−64.0** | −26.8 → **−35.4** | −27.2 → **−36.7** | −13.2 → **−26.4** |
| | C7 | −150.7 → −150.7 | −41.7 → **−50.3** | −20.8 → **−29.5** | −20.8 → **−29.5** | −4.8 → **−13.5** |
| | C8 | −150.7 → −150.7 | −33.8 → **−48.8** | −15.6 → **−25.8** | −16.9 → **−31.9** | −1.3 → **−11.6** |
| | C9 | −150.7 → −150.7 | −19.1 → **−28.5** | −9.5 → **−19.0** | −9.5 → **−19.0** | −0.3 → **−9.8** |
| **0.50** | C6 | −67.5 → −76.6 | −55.1 → −55.1 | −26.6 → −35.1 | −29.4 → −38.6 | −14.9 → −27.2 |
| | C7 | −60.0 → −68.4 | −40.7 → −42.1 | −20.6 → −29.2 | −22.0 → −30.3 | −5.3 → −13.1 |
| | C8 | −55.5 → −71.5 | −33.2 → −38.1 | −15.4 → −25.7 | −17.5 → −33.2 | −1.5 → −11.1 |
| | C9 | −36.1 → −34.6 | −19.0 → −24.9 | −9.4 → −18.9 | −9.6 → −19.6 | −0.4 → −9.5 |
| **1.00** | C6 | −117.3 → −117.3 | −60.5 → −60.5 | −26.4 → **−35.0** | −60.1 → −68.7 | −36.8 → −36.8 |
| | C7 | −102.4 → −102.4 | −47.4 → −47.4 | −20.0 → **−28.0** | −53.0 → −60.1 | −19.7 → −20.3 |
| | C8 | −70.6 → −76.4 | −33.5 → −33.5 | −14.7 → **−23.9** | −40.1 → −47.7 | −7.6 → −10.8 |
| | C9 | −23.2 → −22.7 | −18.5 → −19.8 | −8.9 → −17.5 | −15.5 → −21.7 | −2.3 → −5.6 |

Intermediate characters (0.25, 0.707, 0.9) were also measured and interpolate monotonically; the full sweep is reproducible with the same prototype.

**Readings the planner needs:**
1. **The saw is the only shape whose correction is character-independent** (~9 dB improvement at every character) — the P-4 finding made visible.
2. **The triangle's correction vanishes at character 1** by design (`k → 0`, the corner is 7.7 samples wide) — hence P-6.
3. **The sine row is entirely the bleed ring** (a sine has no discontinuity of its own). At character 1 / C9 it is −23 dB. D-04's "the trap this phase would most plausibly have missed" is confirmed and quantified.
4. **The only regressions are ≈1.5 dB, all at C9 on the sine row**, where the naive floor is already −23 to −48 dB. Acceptable; document rather than chase.
5. **Nothing reaches −60 dB above C7 except the sine.** See P-5.

**Suggested starting thresholds** (assert `aliasPeak ≤ threshold`), = measured corrected worst-over-character **+ 3 dB margin**, rounded outward:

| morph region | C7 | C8 | C9 |
|--------------|---:|---:|---:|
| sine | −62 | −64 | −19 |
| triangle | −39 | −30 | −16 |
| saw | −25 | −20 | −14 |
| square | −26 | −28 | −15 |
| pulse | −10 | −7 | −2 |

The C8 triangle and C9 rows are **not RED against the naive path** at high character (P-6). Two options for the planner, both honest:
- **(a)** index thresholds by `(region, note, character)` — the fully evidence-set form D-09 asks for, and the one this table supports;
- **(b)** gate only `character ∈ {0, 0.5}` on the tight thresholds and gate `character = 1` on a "no regression versus the recorded naive baseline, within 2 dB" invariant. That directly asserts what the D-03 factor is *for*.

**Recommendation: (a) for the C7/C8 grid, plus (b) as an additional invariant covering every cell.** (b) is the assertion that would have caught every regression in the §Alternatives table, and it costs one comparison against a recorded constant.

### Sampling Rate

- **Per task commit:** `make test` (the spectral cases run in seconds).
- **Per wave merge:** `make test && make strict && make guards`.
- **Phase gate:** full suite green, plus in-Rack UAT of the audio-rate MORPH sweep (D-16), before `/gsd-verify-work`.

### Wave 0 Gaps

- [ ] `tests/test_vco_spectrum.cpp` — DFT helper, bin classification, gate self-check, naive baseline (D-08), per-shape gate (D-09/D-10/D-11) — covers TEST-03, AA-01, AA-02, AA-03
- [ ] `tests/check_includes.sh` — `VCO_SIDE_ALLOW` entry for the new test file, with rationale (P-8)
- [ ] `src/vco_compile_canary.cpp` — active `#include "dsp/MorphBlep.hpp"` in the same commit as the header (P-9)
- [ ] `tests/test_vco_core.cpp` — `:416` oracle updated; `:511` bound re-derived from the P-10 table; scenario-four hostile grid extended (D-15)
- [ ] No framework install needed — doctest and the Makefile globs already cover it

## Security Domain

`security_enforcement` is not set in `.planning/config.json` → treated as enabled.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | offline audio plugin, no identity |
| V3 Session Management | no | no sessions |
| V4 Access Control | no | no privileged operations |
| V5 Input Validation | **yes** | Rack does not sanitise cable voltages. Every externally-sourced float (`morph` CV, `sampleTime`, `sampleRate`) must be range- and NaN-guarded with the **negated-comparison idiom**, never `forge::clamp` (which is NaN-transparent — Phase 30 deferred item 3 / CR-02, Phase 31 D-14). |
| V6 Cryptography | no | none present |
| V7 Error Handling / Logging | no | realtime audio path; no logging in `step()` |
| V12 Files & Resources | no | no file or network I/O in this phase |

### Known Threat Patterns for a realtime Rack-free DSP core

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Hostile cable voltage (NaN/±inf) reaching a divisor or an `(int32_t)` cast | Denial of Service | `if (!(x > 0.f))` guards at every entry; `kVcoMaxPitchVolts` bound before `exp2_taylor5` (already in place). MorphBlep adds `if (!(dt > 0.f)) return pending_only;` — **do not rely on the caller** (D-15). |
| Unbounded accumulator growth from an unguarded increment | Denial of Service | `kVcoMaxDeltaPhase = 0.5` already bounds it; `MorphBlep` inherits the bound and must not widen it (D-12 of Phase 31). |
| Out-of-range output damaging a downstream module | Tampering (signal integrity) | The P-10 magnitude envelope, asserted in `test_vco_core.cpp`. Measured worst case 9.198 V, inside Rack's ±12 V norm. |
| Undefined behavior from an in-class `static constexpr` odr-used at runtime | — (correctness/supply-chain: it is what got v2.0.0 rejected) | Function-local or namespace-scope only; `make strict` + the CI MinGW link leg. |
| **Scope note** | | A repo-wide UBSan gate **cannot** be adopted — the shipped LFO carries an unfixed latent UB by decision (Phase 31 D-24). Any UBSan use here is a scoped one-shot probe. |

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| C++ toolchain (clang, `-std=c++17`) | `make test` | ✓ | Apple clang (darwin 23.6.0) | — |
| C++11 syntax gate (`make strict`) | AA-05 / TEST-06 | ✓ | same compiler, `-std=c++11 -pedantic-errors` | — |
| GCC / MinGW `-std=c++11` | CI link leg — the real VCV toolchain | ✓ (CI) | GitHub Actions, already wired | none — this is the gate clang masks |
| doctest | `make test` | ✓ | in-tree header | — |
| libm (`<cmath>`) | test-side DFT only | ✓ | — | — |
| `../Rack-SDK` | plugin build only, not `make test` | ✓ | per `vcv_build_install_workflow` memory | — |
| Python 3 / numpy | **not required** | — | — | the DFT is written in C++ inside the test |

**Missing dependencies with no fallback:** none.
**Missing dependencies with fallback:** none.

Everything this phase needs is already installed and already wired into `make`. The probe programs used for this research compiled and ran first time with `c++ -std=c++17 -O2 -Isrc -ffp-contract=off`.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | The rational `tanh` approximation `x(27+x²)/(27+9x²)` is the standard Padé 3/2 form with error < 4e-4 on \|x\| ≤ 2 | Supporting stack | Low — it is only used by the **optional** pulse-reach refinement, which is recommended to be deferred. Spot-checked numerically this session but not traced to a primary source. |
| A2 | "Top two octaves" means C7–C8 musically, with C9 measured but gated loosely | Validation Architecture | Medium — if the operator means C8–C9, the pulse and triangle thresholds must relax by ~8 dB. **Worth confirming during planning.** |
| A3 | The prototype's alias numbers will be reproduced by the real `MorphBlep` to within ~1 dB | D-08 baseline table | Low — the prototype uses the frozen header unmodified and the recommended arithmetic verbatim; but D-08 exists precisely to re-measure, so any drift is caught by design. |
| A4 | `test_vco_spectrum.cpp` is an acceptable new test filename | Wave 0 gaps | Nil — naming is planner discretion; only the `VCO_SIDE_ALLOW` entry matters. |
| A5 | Rack's `±10 V` bipolar CV convention (`×0.1` in the shell mix) is the right MORPH CV scaling | Code Examples | Low — matches the existing LFO shell and Phase 31's FM attenuverter precedent, but the exact scaling is a Phase 35 panel-behaviour question the operator may want to sign off. |

## Open Questions

1. **Does "top two octaves" mean C7–C8 or C8–C9?**
   - What we know: MEASURED floors at both; C8 is the top of a piano and the frequency DAFx-16 itself uses as "the limits of the proposed method".
   - What's unclear: whether the operator's mental model of "high notes" includes 8.4 kHz fundamentals.
   - Recommendation: gate C7 and C8 tightly, **record** C9 in the same test with a loose threshold and a comment. Costs one extra row and removes the ambiguity permanently.

2. **Threshold form: `(region, note, character)` matrix, or matrix + a no-regression invariant?**
   - What we know: a `(region, note)` table alone cannot be non-vacuous at high character (P-6).
   - Recommendation: both (see §D-08 baseline). The no-regression invariant is the one that directly asserts what D-03's factor is for, and it is one comparison.

3. **Which `deltaPhase` placement method for the gate?**
   - What we know: `pitchCV` alone gives a −56 dB leakage floor; nudging `sampleTime` gives −100 dB at ≤ 5 ppm deviation but cannot use `VcoBlockDriver::run`.
   - Recommendation: `pitchCV` + a mandatory self-check assertion; escalate per-case only if a threshold demands it.

4. **Should the optional pulse-reach factor ship in v2.0?**
   - What we know: MEASURED +1.3 dB at the single worst grid point, ~+0.1 dB mean; it adds the only division by an edge width in the whole header.
   - Recommendation: **no** for the first iteration. Keep it documented in `MorphBlep.hpp`'s banner as the first refinement to try if the pulse threshold at C8 is missed, so the iteration budget has somewhere cheap to go.

5. **Loose output bound: raise to ~10.0 V, or keep 6.0 V with a Nyquist-ceiling carve-out?**
   - What we know: MEASURED 1.1047 (±5.52 V) at every musical rate; 1.8395 (±9.20 V) only at `dt ≈ 0.495` with `morph = 1`, `character ≈ 0`.
   - Recommendation: raise to a single number with this table as provenance. D-09's own reasoning — an exception invites a second exception — applies here too.

## Sources

### Primary (HIGH confidence)

- **Direct numerical measurement against `src/dsp/Waveshape.hpp`, this session.** Six probe programs compiled with the project's own flags. Covers: per-shape jump magnitudes vs character; the square hard/soft position split; the saw wrap's character independence; triangle slope-break sizes and rounding widths; pulse narrow-duty reach; the bleed-ring weight algebra; kernel equivalence to the canonical two-branch polyBLEP; `sinc²` harmonic-gain validation; the D-03 factor calibration sweep; the full naive→corrected alias matrix at three sample rates; the output-magnitude envelope; hostile-timing behaviour; achievable bin-centred `deltaPhase` precision.
- `src/dsp/Waveshape.hpp` (frozen), `src/dsp/VcoCore.hpp`, `tests/test_vco_core.cpp`, `tests/VcoBlockDriver.hpp`, `tests/check_includes.sh`, `tests/check_canary.sh`, `tests/check_frozen.sh`, `Makefile` — read directly.
- `.planning/phases/32-.../32-CONTEXT.md` — the 19 locked decisions.

### Secondary (MEDIUM confidence)

- [DAFx-16, "Rounding Corners with BLAMP" — Esqueda, Välimäki, Bilbao](https://www.dafx.de/paper-archive/2016/dafxpapers/18-DAFx-16_paper_33-PN.pdf) — Table 1 (four-point polyBLAMP residual, quintic), the explicit note that a two-point version exists elsewhere, the residual-scaling rule ("scaled by the magnitude and direction of the discontinuity introduced in the first derivative"), and Table 2's SNR figures (triangular C8: 30 dB trivial / 42 dB OS×2 / 46 dB OS×4 / 45 dB polyBLAMP). Text extracted directly from the PDF.
- [DAFx'16 project page, Aalto](http://research.spa.aalto.fi/publications/papers/dafx16-blamp/) — paper metadata.
- `.planning/research/STACK.md` — the crux argument (`:19-38`), the five-step algorithm (`:33-36`), the DAFx-2017 precedent (`:116`), the minBLEP/oversampling rejections (`:132-135`), `-ffp-contract=off` (`:75`). **Three specific claims in this document are contradicted by measurement and are called out inline.**
- `.planning/research/ARCHITECTURE.md`, `FEATURES.md`, `PITFALLS.md` — placement, the character/aliasing coupling, the in-class `static constexpr` trap.

### Tertiary (LOW confidence)

- The Padé 3/2 `tanh` identity (A1) — recalled, numerically spot-checked, not traced to a primary source. Only affects an optional refinement that is recommended for deferral.

## Metadata

**Confidence breakdown:**
- Kernels and sign convention: **HIGH** — derived from first principles, cross-validated numerically against the canonical form and against `sinc²` to 0.01 dB, and corroborated against the DAFx-16 paper.
- Site map and jump magnitudes: **HIGH** — every entry verified to six decimal places against the frozen code.
- D-03 factor: **HIGH** on shape and on the compact-support requirement (the alternatives fail by 30–60 dB); **MEDIUM-HIGH** on `p = 2` versus `p = 1` (a plateau, chosen on the C¹ argument rather than on a measured gap).
- Alias-floor numbers: **HIGH** as prototype measurements; **MEDIUM** as final thresholds — D-08 must re-measure against the real implementation, which is exactly what D-08 is for.
- Output-magnitude envelope: **HIGH** — dense grid, both spread configurations, all `dt` regimes.
- Test-frequency construction: **HIGH** — the coprimality argument is proven and the precision limits are measured.

**Research date:** 2026-07-31
**Valid until:** stable — this is a frozen-header derivation plus first-principles DSP, not a moving ecosystem. Re-derive only if `FROZEN.sha256` for `Waveshape.hpp` ever changes.
