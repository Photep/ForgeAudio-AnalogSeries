# Phase 32: Morph-Aware Anti-Aliasing (polyBLEP/polyBLAMP) - Context

**Gathered:** 2026-07-31
**Status:** Ready for planning

<domain>
## Phase Boundary

**Band-limiting.** Everything that decides how clean the oscillator sounds, plus the spectral gate that proves it.

A new additive `src/dsp/MorphBlep.hpp` *calls* the frozen `src/dsp/Waveshape.hpp` and band-limits the continuous, character-deformed morph crossfade: polyBLEP at value-step discontinuities, polyBLAMP at slope corners, both scaled by closed-form character-aware jump magnitudes. `VcoCore::step()` swaps its single `wave.morphedWave(...)` call at `src/dsp/VcoCore.hpp:484` for the band-limited path. The MORPH control gains its CV jack and attenuverter so the hardest case — audio-rate morph sweeps through segment boundaries — can actually be auditioned and measured.

**Requirements:** MORPH-01, MORPH-02, AA-01, AA-02, AA-03, AA-04, AA-05, CORE-02, TEST-03.

**⚠ TWO BLOCKING PRE-PLANNING DOCUMENT EDITS.** See D-02 and D-06. Both land in one commit BEFORE planning, following the Phase 31 D-00 precedent — otherwise the phase is checked against gates it deliberately contradicts.

**NOT in this phase (deliberate boundaries):**
- **Hard sync → Phase 33** (SYNC-01/02). D-14 designs the *seam* MorphBlep exposes for it; this phase implements no sync behavior and adds no sync fields to `VcoInputs`.
- **CHARACTER CV + attenuverter, DRIFT (knob/CV/attenuverter, the moving OU layers), the output stage (DC blocker, soft saturation) → Phase 34** (CHAR-01, DRIFT-01..03, OUT-01..03). CHARACTER stays a knob-only param this phase; the output stays unconditioned at `×5`. `bleedLfo` stays `0.f`.
- **MORPH-02 is the one exception to the "CV comes in Phase 34" reading** — see D-16. REQUIREMENTS.md maps it to Phase 32 and roadmap SC-2 spells it out; Phase 31's CONTEXT lumped MORPH and CHARACTER CV together, and that lumping is corrected here rather than followed.
- **The 18HP Forge Noir panel, CRT display, patch serialization, per-instance seed entropy → Phase 35** (PANEL-01/02, DISP-01..03).
- **VCO goldens, version bump, tag, #929 update → Phase 36** (TEST-05, REL-01). There are no VCO goldens yet, so this phase's output changes break no fixture.
- **Oversampling, minBLEP, through-zero FM, phase distortion → v2.1.** AA-05 forbids the first two outright in v2.0. Research's escalation path if the alias floor proves unreachable is v2.1 oversampling, explicitly *not* minBLEP (`research/STACK.md:154-156`).

</domain>

<decisions>
## Implementation Decisions

### ⚠ BLOCKING PRE-PLANNING ACTIONS

Both edits land in **one commit before planning starts**. Rationale is the Phase 31 D-00 precedent: a plan checked against a gate whose prose contradicts the decision produces either a false failure or a false pass, and this project has logged four separate cases of a gate's mechanism being wider than the prose it encoded.

- **D-02: `.planning/ROADMAP.md` §"Phase 32" Success Criterion 2 drops the word "measured".** It currently reads *"BLEP/BLAMP magnitude is driven by the **measured** characterized jump so CHARACTER edge-softening auto-scales the correction."* D-01 chose an analytic derivation, which is *characterized* but not *measured*. New text: *"...driven by the characterized jump..."*. Note `.planning/REQUIREMENTS.md` AA-04 already reads *"the characterized (actual) jump"* and needs no change on this point.
- **D-06: `.planning/REQUIREMENTS.md` AA-01 widens "scaled by the morph weights" to "scaled by the morph and bleed weights".** D-04 corrects sites the bleed ring injects, which are scaled by bleed weights, not morph weights. AA-01 is not *contradicted* today — bleed output is part of the continuous crossfade — but the phrase is incomplete, and the verifier would be reading a narrower requirement than the implementation satisfies.

### Jump Magnitude (AA-04)

- **D-01: Analytic, character-aware jump magnitudes — closed form, no probing.** BLEP/BLAMP magnitudes are derived in closed form *including* the character terms. The character deformations are all analytically known: the tanh edge width is `c × min(0.08, maxEdge)` (`Waveshape.hpp:134-135`), the square's is `c × 0.08` (`:110`), the triangle's rounding is `c × 0.35` (`:71`), the saw's soft reset spans `c × 0.08` (`:91`). So the effective step across one sample is *computable*, not measurable.
  - **Rejected: measuring by probing `morphedWave` at phase ±ε.** It would auto-handle character, bleed normalization and crossfade weights with zero derivation, and it is AA-04's most literal reading. Cost killed it: `morphedWave` computes all five shapes per call (1 `sin`, 2 `exp`, 2 `tanh`, 1 `cos`, plus branches), and probing both sides of the D-04 site set is roughly 8 extra calls per sample — about 400k transcendental-heavy calls/sec at 48 kHz for a mono VCO.
  - **Rejected: pristine character-independent jumps** (`research/STACK.md:40`, which explicitly calls magnitude-matching *"a v2.1 nicety, not a v2.0 requirement"*). It is the cheapest and errs in the safe direction, but it would leave AA-04 knowingly unmet and require a third document edit. **The research recommendation is overridden by decision, not by oversight** — downstream agents must not "restore" it.
  - The derivation duplicates knowledge of `Waveshape`'s internals in a second header. Acceptable *only* because `Waveshape.hpp` is frozen (`tests/check_frozen.sh`, `FROZEN.sha256`) and cannot drift underneath it. Record that dependency explicitly in `MorphBlep.hpp`'s banner.
- **D-03: The character-scaling factor's SHAPE is locked here; the exact expression goes to research.** Binding constraints: a closed-form function of edge-width versus one sample's phase advance; **→ 0** when the softened edge is much wider than a sample (already band-limited — correcting it would over-correct); **→ 1** when the edge is much narrower than a sample (a true step, full correction); table-free, pure `+ - * /`. `gsd-phase-researcher` derives and validates the exact expression against measured spectra. The roadmap already flags this exact item MEDIUM-confidence and names `--research-phase` for it.
  - A useful property of tying it to *one sample's phase advance* rather than a fixed constant: the correction becomes sample-rate-aware for free, which is also what makes the D-11 cross-rate regression meaningful.
- **D-03b: One unified rule governs every site.** The same closed-form character factor scales polyBLEP value-step corrections *and* polyBLAMP slope-break corrections; each site supplies its own pristine magnitude and its own character-dependent width. The saw's cosine soft reset then falls out naturally rather than needing separate treatment: as character rises the wrap's effective step shrinks and its correction shrinks with it.
  - **Rejected: per-site-type derivations.** More faithful in principle, but three derivations to validate instead of one, and three independent places for the alias-floor gate to disagree with.
  - **Corollary the planner must honor:** the saw's soft reset (`:91-97`) and the triangle's rounded peaks (`:71-78`) are **fast but continuous**, not discontinuities. Applying a full-authority correction there would be actively wrong. The D-03 factor is what makes them self-limiting.

### Discontinuity Site Map (AA-01, AA-02, AA-03)

- **D-04: A fixed union of sites is evaluated every sample; magnitudes fall to zero when a shape carries no weight.** The site set: **phase 0** (saw wrap + square rising edge + pulse rising edge + triangle peak, all coincident), the **square duty edge**, the **pulse duty edge**, the **triangle valley**. Constant cost, no branching on morph position, and structurally incapable of missing a segment-boundary case or a bleed-ring site.
  - **The measurement that drove this.** At `morph = 0`, `character = 1`: `segment = 0`, `frac = 0`, so `leftWeight = 1 - frac = 1` and the bleed ring's `leftIdx = (0 - 1 + 5) % 5 = 4` — the **narrow pulse** bleeds in at full `bleedIntensity ≈ 0.04` inside what the user hears as a pure sine. After the `1/(1+bleedIntensity)` normalization that is a step of ≈0.077 in a ±1 wave, roughly −22 dB. Real alias energy from a shape that is not in the crossfade at all. An "active segment only" implementation leaves it entirely uncorrected.
  - **Positions are recomputed per sample from the live `Waveshape` fields, never cached.** The square duty is `0.5 + c × (0.04 + squareDutySpread)` (`:108`) and the triangle valley is `0.5 + asymmetry × 0.5` (`:61`) — both move with character *now*, and both move again with `*Spread` when Phase 34's drift starts writing those fields. A cached site table would silently desynchronise in Phase 34.
  - **Rejected: computing the active set per sample** (segment shapes + `leftIdx` + `rightIdx`). Smaller working set, but branchy, and it encodes the bleed ring's topology in a second header where a future change to `Waveshape`'s ring would desynchronise it silently.
- **D-05: Corrections ride through the bleed normalization.** Corrections are computed against pre-normalization magnitudes and divided by the same `1/(1 + bleedIntensity)` factor the naive path applies (`Waveshape.hpp:212`). Matches `research/STACK.md:36`. The normalization is linear, so this is **exact**, not an approximation. Applying corrections at full magnitude after normalization would over-correct by exactly `(1 + bleedIntensity)` — precisely when `character > 0`, which is precisely when the bleed sites exist.
- **D-07: Overlapping edges are each placed at their own sub-sample position and summed — never overwritten, and no duty floor.** At C8 a 5%-duty pulse is ≈0.57 samples wide, so both its edges land inside one sample. Each gets its own sub-sample position; the two opposite-sign corrections sum into the accumulator. When `duty < dt` they partially cancel, and that cancellation is the **physically correct band-limited answer** — a pulse narrower than a sample genuinely carries less energy. Matches roadmap SC-3.
  - **Rejected: flooring the effective duty at `dt`** so the pulse always spans at least one sample. It would guarantee the pulse never thins out at the top of the keyboard, but introduces a sample-rate-dependent timbre change the frozen `Waveshape` call knows nothing about — the naive and band-limited paths would stop being the same waveform, which would also invalidate D-08's before/after comparison.

### The Alias-Floor Gate (TEST-03)

- **D-08: Plan 32-01 builds the spectral helper and baselines the NAIVE core, before any band-limiting exists.** It records the alias floor of today's deliberately-aliased oscillator per shape and per note. Three payoffs: the threshold is set from measurement rather than inherited (the D-18 *"the gate must measure, not cite"* precedent — the roadmap's ≈−60 dB is a target, not a measurement); the whole phase gets an objective iteration metric instead of ear-guessing, which matters because this is the phase with a deliberate iteration budget; and the RED is genuine — the gate provably fails before `MorphBlep` lands, rather than being written against already-passing code.
  - **Keep the naive path callable** for the baseline and for the before/after delta. The planner decides the mechanism (a flag, a second entry point, a test-only shim).
- **D-09: Per-shape, evidence-set thresholds — not one global number.** Each morph region gets its own threshold, pinned from the Phase 32 baseline plus what band-limiting actually achieves, and each carries its measured justification in the test. A 5%-duty pulse at the top two octaves is a genuinely harder case than a sine; 2-sample polyBLEP with no oversampling is unlikely to reach −60 dB there. One number would have to be set by the worst case and would then prove nothing about the easy cases.
  - **Rejected: a single global threshold** (simplest to state, near-meaningless in practice) and **a global floor with a pulse carve-out** (honest, but an exception invites a second exception).
- **D-10: Integer cycles per analysis block; rectangular window; zero leakage.** Each test frequency is chosen so a whole number of cycles exactly fills the block. Leakage is then exactly zero, the rectangular window is exact, and every harmonic — plus every folded alias — lands dead on a bin centre. "Alias energy" becomes simply the magnitude at non-harmonic bins: no guard bands, no window coefficients, no floor that depends on the window choice.
  - **This is a gate-correctness decision, not a convenience one.** If the frequency does not fit a whole number of cycles, rectangular-window DFT leakage smears the fundamental across every bin — easily above −60 dB — and the gate measures its own window rather than the DSP. That is the "mechanism wider than the prose" failure this project logged four times in Phase 30.
  - **Rejected: a Blackman-Harris window** (≈−92 dB sidelobes, comfortably under a −60 dB floor, allows arbitrary musical frequencies) — it works, but adds window math, widened harmonic bins needing excluded guard bands, and a floor that now depends on two things instead of one.
  - Test frequencies therefore sit at bin centres rather than exact equal-tempered notes. **This is irrelevant to aliasing behavior** and must not be "fixed" by a later agent.
  - libm is available here: the test is not `src/` (the D-18 precedent).
- **D-11: 44.1 kHz is the binding assertion; 48 and 96 kHz run as regression.** 44.1 kHz is the worst case — pass there and the higher rates follow. The other two rates are not decorative: a correction scaled wrongly by `dt` fails *rate-dependently*, and that is the most likely way this implementation goes subtly wrong, so a single-rate gate would never see it. `tests/VcoBlockDriver.hpp` already drives all three.
  - Coverage: morph positions at the five shape centres; character at both **0** (worst-case hard edges — `research/FEATURES.md:53` notes worst-case aliasing is at `character = 0`) and **1** (fully softened, where D-03's factor should be pulling corrections down). Exact note grid, block lengths and cycle counts are research/planner discretion.

### MorphBlep Structure & Seams (CORE-02, AA-05)

- **D-12: One new header — `src/dsp/MorphBlep.hpp`.** Kernels and site logic together, exactly as CORE-02 words it. One file, one include, one entry in `check_includes.sh` / `check_frozen.sh` / the compile canary, smallest new surface for Phases 33-36 to inherit.
  - **`research/STACK.md:61`'s suggestion to put the kernels in `RackCompat.hpp` is REJECTED and must not be revisited.** That file is byte-pinned by `tests/check_frozen.sh` and consumed by the **shipped** LFO — editing it is a milestone-guardrail event requiring operator sign-off and golden re-verification, not a VCO implementation detail.
  - **Rejected: a separate `Blep.hpp`** for the two kernels. Cleaner separation and Phase 33 could call a kernel directly, but it is two files to wire into three guards, and CORE-02 names one.
- **D-13: A pending-residual accumulator delivers the next-sample half of each correction. Zero added latency.** A 2-sample polyBLEP corrects the sample *after* the edge as well as the one containing it. `MorphBlep` carries a small residual accumulator; the next-sample half is summed into it and added at the top of the following `step()`.
  - **Why not a one-sample output delay buffer** (simpler bookkeeping, no carried state): it adds a sample of latency the module would have to declare, and a VCO that silently delays by a sample desyncs against every other oscillator in the patch. It also complicates Phase 33, which needs to act on the current sample.
  - The accumulator composes for free with D-07: several corrections `+=` into the same accumulator instead of fighting over one slot.
- **D-14: The Phase 33 sync seam is DESIGNED here, not built here.** `MorphBlep` exposes an entry point accepting an externally-supplied `(sub-sample position, value jump)` event that feeds the same accumulator as the morph sites. Phase 32 implements no sync behavior and adds no sync fields to `VcoInputs`. The point is that Phase 33 plugs in rather than reopening the one header this phase spends its entire iteration budget stabilising — and whose per-shape alias thresholds it has just pinned.
  - **CORE-03 constraint, binding:** all `MorphBlep` state lives per-`VcoCore`-instance. **No static, no global mutable voice state**, or the polyphony-ready guarantee breaks.
- **D-15: Deferred item 6 (the hostile-timing grid) stays in Phase 32 — on a corrected rationale.** Phase 31's D-15 pointed it here because *"Phase 32's oversampled inner loop is the first real source of exotic timing"*. **That premise is falsified**: AA-05 forbids oversampling in v2.0, so no such loop will exist. The conclusion survives on better evidence: **this phase introduces division by `dt` and by `edgeWidth`** (D-03's factor, and the polyBLEP/polyBLAMP kernels themselves), so a zero, subnormal, or non-finite `sampleTime` now reaches arithmetic that did not exist before. Extend `tests/test_vco_core.cpp` scenario four's grid to `±inf`, subnormal and very-large-finite `sampleRate`/`sampleTime`.
  - **Record the corrected rationale** in the deferred register so no later phase inherits the falsified one.
  - Guards use the **negated-comparison idiom** (`if (!(x > 0.f)) ...`), never `forge::clamp` — which is NaN-transparent (Phase 30 deferred item 3 / CR-02, Phase 31 D-14).

### MORPH Control Surface (MORPH-01, MORPH-02)

- **D-16: The MORPH CV jack and attenuverter are declared in THIS phase.** Both source documents already say so: `.planning/REQUIREMENTS.md` maps MORPH-02 to Phase 32, and roadmap SC-2 reads *"The MORPH control (knob + CV + attenuverter) sweeps the continuous 5-shape crossfade... at audio rate with band-limited output"*. **Phase 31's CONTEXT lumped "MORPH/CHARACTER CV + their attenuverters" into Phase 34; that lumping is corrected here, not followed** — CHARACTER's CV and attenuverter remain Phase 34 (CHAR-01), MORPH's do not.
  - It also serves this phase directly: audio-rate MORPH CV sweeping through segment boundaries is the hardest case the alias floor has to survive, and Phase 30's D-07 rule (*every visible control does something, so an in-Rack check is honest*) means the operator can actually audition it during UAT.
  - No document edit needed for this one — the documents are already right.
- **D-17: The shell mixes knob + CV × attenuverter; the POD boundary does not change.** `VcoInputs::morph` is already documented *"post-CV, post-clamp [0,1]"* (`src/dsp/VcoCore.hpp:229`) — the boundary was designed for exactly this and has been stable since Phase 29. Phase 31's D-05 is directly on point: *do not re-document these fields mid-milestone; churning field semantics buys nothing*.
  - **This does not violate Phase 31's D-17** ("the shell does no DSP"). Conditioning a param into its documented `[0,1]` range is param plumbing; FM was different because it genuinely had to enter the volt-domain summation *inside* the core, before the single exponential.
  - **Zero POD change also protects deferred item 9**: the compile canary's unique-field margin is down to exactly one field (`drift`), and adding `morphCV`/`morphAtten` fields would put new fields under the canary's "every `VcoInputs` field stays runtime-live" obligation.
  - Attenuverter styling follows Phase 31's D-07 precedent (bipolar `-1..+1`, linear taper, default `0`, displayed `-100%..+100%`). Physical form (knob vs scalloped trimpot) is Phase 35's call, per D-08 there.
  - The throwaway `res/AnalogVCO.svg` gains two control positions; it is replaced wholesale in Phase 35. **`src/AnalogLFO.cpp` must remain absent from this phase's diff**, as in Phases 30 and 31.

### Claude's Discretion

- The exact closed-form character-scaling expression — routed to `gsd-phase-researcher` per D-03, within D-03's locked shape constraints.
- The mechanism for keeping the naive path callable for D-08's baseline (flag, second entry point, or test-only shim).
- Block lengths, cycle counts, the exact bin-centred frequency grid, and the DFT implementation for D-10; the exact note grid for D-11.
- Whether the D-04 site set is expressed as a small fixed-size array walked per sample or as unrolled straight-line code. **If an array: namespace-scope `static constexpr` only, never in-class `static constexpr` indexed at runtime** — that exact C++11 form is what got v2.0.0 rejected from the VCV Library (`research/PITFALLS.md:190`; `VcoCore.hpp` banner).
- Updating `tests/test_vco_core.cpp:416` (the oracle that reimplements the naive `5.f * wave.morphedWave(...)` path) and the `±5.55 V` bound reasoning at `:511` — both move once band-limiting lands. Corrections are bipolar and *reduce* overshoot at edges, but the bound must be re-derived, not assumed.
- Whether `MorphBlep` is a struct held by value inside `VcoCore` or a free-function set with explicit state — subject to D-14's per-instance constraint.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### v2.0 VCO research (locks approach — read first)
- `.planning/research/STACK.md:19-38` — **the crux answer**: BLEP/BLAMP superpose linearly and apply to any discontinuity whose position and magnitude are known; the effective jump at each site is the weighted sum of per-shape jumps. **:21-31** the 5-shape discontinuity map. **:33-36** the five-step per-sample algorithm, including :36's bleed-normalization rule that D-05 adopts.
- `.planning/research/STACK.md:40` — the pristine-jump recommendation **that D-01 deliberately overrides**. Read it so the override is understood, not re-litigated.
- `.planning/research/STACK.md:50-51` — polyBLEP (Välimäki & Huovilainen 2007; Pekonen 2010) and polyBLAMP (Esqueda/Bilbao/Välimäki, DAFx-2016) rationale, including the `d⁴/6` BLAMP residual. **:61** the kernel-placement suggestion **D-12 rejects** (RackCompat.hpp is frozen). **:90-100** the canonical 2-sample kernel forms. **:116** the DAFx-2017 polygonal-oscillator precedent for the weighted-sum approach. **:132-135** why minBLEP, oversampling, DPW and BLIT are all rejected for v2.0. **:143-150** the landmine table, including :149-150. **:154-156** the escalation path if the alias floor proves unreachable (v2.1 oversampling, **not** minBLEP).
- `.planning/research/STACK.md:73` — the spectral-invariant test recommendation TEST-03 implements. **:75** why `-ffp-contract=off` is load-bearing for BLEP polynomials specifically.
- `.planning/research/ARCHITECTURE.md:29-33,55-57,81` — `MorphBlep.hpp`'s place in the three-layer pattern and the "single genuinely-new subsystem" framing. **:57** — `Waveshape.hpp` is REUSED AS-IS, bit-frozen for the LFO golden.
- `.planning/research/FEATURES.md:35` — anti-aliasing as the invisible table stake and the milestone's central risk. **:53** — the character/aliasing coupling and the key fact that worst-case aliasing is at `character = 0`. **:98** — why this is the critical path for MORPH, CHARACTER and hard sync alike.
- `.planning/research/PITFALLS.md:190` — the in-class `static constexpr` table trap that got v2.0.0 rejected from the VCV Library. Directly relevant if the D-04 site set becomes a table.

### Requirements & roadmap (both need the D-02/D-06 edits before planning)
- `.planning/ROADMAP.md` §"Phase 32" — goal and 5 success criteria. **SC-2 requires the D-02 edit.** SC-3 is what D-07 satisfies; SC-5 is AA-05.
- `.planning/REQUIREMENTS.md` — MORPH-01/02, AA-01..05, CORE-02, TEST-03. **AA-01 requires the D-06 edit.**
- `.planning/PROJECT.md` §Constraints — the LFO non-regression guardrail and the four frozen shared headers. §"Current Milestone" — the lean-scope decision that defers oversampling to v2.1.

### Prior-phase hand-offs (inherited decisions — do not re-litigate)
- `.planning/phases/31-pitch-tuning-exponential-fm/31-CONTEXT.md` — D-01 (one summation, one `exp2_taylor5`), D-05 (do not churn POD field semantics — D-17 here rests on it), D-10/D-11 (the hard Nyquist clamp at `0.495`), D-12 (`kVcoMaxDeltaPhase = 0.5` untouched), D-14 (the negated-comparison NaN idiom D-15 here reuses), D-17 (the shell does no DSP — D-17 here reconciles with it), D-19 (measure the output, not telemetry).
- `.planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md` — **item 3** (`forge::clamp` is NaN-transparent), **item 6** (the hostile-timing grid — **D-15 here corrects its falsified rationale**), **item 9** (the compile canary's unique-field margin is down to one field — D-17 here protects it), **item 10** (falsified comments in `test_vco_core.cpp`).
- `.planning/phases/30-vcocore-skeleton-module-registration/30-CONTEXT.md` — D-07 (every visible control does something, so in-Rack UAT is honest — D-16 here continues it), D-16 (measure the output, not telemetry).
- `.planning/STATE.md` §Accumulated Context — the standing "no tag on local evidence alone" rule, the R-9 `VcoInputs`-not-`Inputs` ODR landmine, and the Phase 30/31 gate-design lessons (gates are artifacts needing review in their own right; bare `grep -c` criteria produce artifact counts).

### Code to write, call, and not touch
- `src/dsp/MorphBlep.hpp` — **NEW, the file this phase creates.** Per D-12, kernels and site logic together.
- `src/dsp/Waveshape.hpp` — **FROZEN. Called, never edited** (CORE-02). The sites and magnitudes D-01/D-04 derive come from: **:36-38** `progressiveCurve` (the `c = character²` law every deformation uses), **:54-80** triangle (`:61` moving valley, `:71-78` rounded peaks), **:83-99** saw (`:88-89` exponential curvature, `:91-97` cosine soft reset), **:102-123** square (`:108` moving duty, `:110` tanh edge width), **:126-152** pulse (`:134-135` edge width clamped against narrow duty), **:158-216** `morphedWave` (`:165-172` segment/frac/pulseDuty, `:179-185` the crossfade with the segment-3 direct-duty special case, **:200-212 the bleed ring and its normalization**).
- `src/dsp/VcoCore.hpp` — the caller. **:484** is the single `wave.morphedWave(p, morph, character, 0.f)` call site the band-limited path replaces. Read the banner first: the **source-shape contract** (the `struct VcoCore` and `float step(...)` lines must each stay on one line with their brace, or `make guards` hard-fails), the binding **C++11 rules** (no `inline constexpr` variables, no `if constexpr`, no `std::clamp`, no in-class `static constexpr` indexed at runtime, no brace value-list init of `VcoInputs`), and **zero Rack-SDK includes**. **:229** documents `morph` as post-CV/post-clamp (D-17).
- `src/dsp/RackCompat.hpp` — **FROZEN, byte-pinned, shipped-LFO-consumed.** `forge::clamp` is NaN-transparent. **Do not add BLEP kernels here** (D-12).
- `src/AnalogVCO.cpp` — gains the MORPH CV jack, its attenuverter param, and the shell-side mix (D-16/D-17). Its banner's "THIS FILE DOES NO DSP" rule stands.
- `src/AnalogLFO.cpp` — **must remain absent from this phase's diff**, as in Phases 30 and 31.
- `src/vco_compile_canary.cpp` — the TU compiled against a perturbed `VcoCore` header; deferred item 9 applies if any POD field is added (D-17 avoids that).
- `tests/VcoBlockDriver.hpp` — the harness. Already drives 44.1/48/96 kHz (D-11). **Never template or subclass it with `tests/BlockDriver.hpp`**, which feeds the shipped LFO's bit-exact golden leg.
- `tests/test_vco_core.cpp` — **:416** the oracle reimplementing the naive path (moves this phase), **:511** the `±5.55 V` bound reasoning (re-derive, don't assume), scenario four's hostile-timing grid (extended per D-15).
- `tests/check_frozen.sh`, `tests/check_canary.sh`, `tests/check_includes.sh` — the standing guards. A new `src/dsp/` header and a new test file each cost an explicit allowlist entry; **make that a plan task with its own rationale, not a gate-time discovery** (Phase 31 D-23's lesson).

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`src/dsp/VcoCore.hpp:484`** — the single `wave.morphedWave(...)` call is the exact insertion point. The surrounding pitch/Nyquist/accumulator sequence is settled and must not move.
- **`VcoInputs` needs no new fields** (D-17). `morph` and `character` are already declared, documented post-CV/post-clamp, and fed by the shell as of Phase 31. `VcoBlockDriver` and the harness keep working untouched.
- **`tests/VcoBlockDriver.hpp`** already runs 44.1/48/96 kHz with correct timing-injection discipline and non-degenerate seeds — D-11's cross-rate regression needs no new driver.
- **Makefile / CI globs** — `make test` globs `tests/*.cpp`; `make strict` and the CI MinGW link leg glob `src/*.cpp`. New test files are picked up with zero build wiring. A new `src/dsp/*.hpp` needs no Makefile change either, but **does** need guard-allowlist entries.
- **`double` phase accumulator** — already in place (PITCH-05). Sub-sample discontinuity positions come out of the same accumulator arithmetic; no new precision machinery needed.

### Established Patterns
- Rack-free header-only DSP under `src/dsp/*.hpp` with **zero `rack/` includes**; the shell owns params/inputs/outputs and delegates per-sample work to the core.
- Two-standard compilation: **both** `-std=c++11 -pedantic-errors` (the shipped plugin toolchain) and `-std=c++17` (the test target).
- **Negated-comparison guards for non-finite input** — `if (!(freq > 0.f)) freq = 0.f;`. Written negated *specifically* so NaN lands on the fallback branch. D-15's new guards follow this, never `forge::clamp`.
- **RED-first fixes and non-vacuous gates.** Four consecutive Phase 31 plans declined to tick a requirement their own frontmatter claimed, on the grounds that the plan asserted no behavior for it. D-08's baseline-first ordering exists to make this phase's central gate genuinely RED before it is green.
- **Gates are artifacts needing review in their own right.** Bare `grep -c` acceptance criteria produced artifact counts five times in Phase 31 because these headers deliberately quote the constructs they forbid. Count criteria must be comment-stripped or anchored, and compared against a baseline rather than zero.
- `-ffp-contract=off`, no `-ffast-math`. **Load-bearing here specifically**: polyBLEP/BLAMP are FMA-friendly polynomials, and without the flag GCC and clang contract `a*b+c` differently (`research/STACK.md:75`).

### Integration Points
- **`src/dsp/MorphBlep.hpp`** — new file; the only place band-limiting arithmetic lives.
- **`src/dsp/VcoCore.hpp`** — the `:484` call site; plus per-instance `MorphBlep` state (D-14, CORE-03).
- **`src/AnalogVCO.cpp`** — MORPH CV jack, attenuverter param, shell-side mix (D-16/D-17). No arithmetic beyond the mix and clamp.
- **`res/AnalogVCO.svg`** — throwaway panel gains two control positions; replaced wholesale in Phase 35.
- **`tests/`** — the spectral helper + baseline (D-08), the per-shape gate (D-09/D-10/D-11), the extended hostile-timing grid (D-15), and updates to the `:416` oracle and `:511` bound.
- **`tests/check_includes.sh` / `check_canary.sh`** — allowlist entries for the new header and any new test file.
- **Standing tripwires stay green:** no frozen header is edited, so `FROZEN.sha256` needs no bump; the LFO golden byte-lock is unaffected because no LFO behavior changes; the include-direction audit holds because no LFO TU includes any VCO file.

</code_context>

<specifics>
## Specific Ideas

- **"Compute it, don't measure it" is the through-line of this phase's magnitude work.** The frozen `Waveshape` means every character deformation has a known closed form, so probing the function to discover what it just did is paying transcendental cost for information already in hand. The trade accepted in exchange: `MorphBlep.hpp` knows things about `Waveshape.hpp`'s internals, which is only safe because that file cannot change.
- **The bleed ring is the trap this phase would most plausibly have missed.** Nothing in AA-01's wording points at it, and a reasonable implementation that band-limits "the shapes being crossfaded" is silently wrong at every morph position — worst at the extremes, where a full-weight non-adjacent neighbour bleeds in. It was found by reading `Waveshape.hpp:200-208` and doing the arithmetic, not by reading the requirement.
- **The gate must not measure its own window.** D-10's integer-cycles choice is the same instinct as Phase 31's D-21 (derive the test boundary from the constant, don't hardcode it): make the measurement *exact* rather than *approximately fine*, so a failure means the DSP failed.
- **Two rates that are not the binding one still earn their runtime.** D-11 keeps 48 and 96 kHz because a `dt`-scaling error is the most likely subtle bug in this implementation and it is invisible at a single rate. This mirrors the Phase 31 lesson that the most natural version of a test can be bit-exactly vacuous.
- **Erring toward over-correction is the safe direction, and D-03 is where that lives.** Research's argument holds: character is a lowpass-ish coloration, so slight over-correction adds a hair more HF rolloff (benign), while under-correction leaves audible alias tones. If the D-03 expression proves ambiguous in its first spectral iteration, bias it toward full authority rather than toward zero.
- **A falsified rationale is worth more than a dropped item.** D-15 keeps deferred item 6 in this phase but replaces the reason. The old reason (an oversampled inner loop) would have been quietly inherited and would have justified nothing.

</specifics>

<deferred>
## Deferred Ideas

- **Measured/probed jump magnitudes as a refinement** — `research/STACK.md:40` frames magnitude-matching as a v2.1 nicety. D-01 chose analytic-character-aware instead, which lands between pristine and probed. If the D-09 per-shape thresholds prove unreachable in a specific morph region *and* research attributes it to magnitude error rather than kernel order, probing that one region is the narrow escalation — but the broad escalation path is **v2.1 oversampling, explicitly not minBLEP** (`research/STACK.md:154-156`).
- **Higher-order (4-point) polyBLEP** — not scoped for v2.0. AA-05 forbids minBLEP and oversampling by name but does not speak to kernel order. If the alias floor proves unreachable at 2-sample, raising the order is a decision for the operator with impact assessment, not a silent implementation choice; D-09's evidence-set thresholds are the intended response instead.
- **CHARACTER CV + attenuverter → Phase 34** (CHAR-01), alongside DRIFT's controls. D-16 pulls MORPH's forward; CHARACTER's stay where they are.
- **Hard sync → Phase 33** (SYNC-01/02). D-14 leaves the entry point; the behavior, the `SchmittTrigger`, the sub-sample crossing time, and the fractional-overshoot phase reset (`research/STACK.md:124`, and :149 — **never snap the reset to exactly 0**) are all Phase 33's.
- **The output stage and drift → Phase 34.** The `×5` output stays unconditioned this phase; `bleedLfo` stays `0.f`. **Phase 34 must re-read D-04**: once drift writes the `*Spread` fields, discontinuity positions move per sample, and the "recompute, never cache" rule is what keeps this correct.
- **The shipped LFO's shared latent UB** (`src/AnalogLFO.cpp:320` → `src/dsp/LfoCore.hpp:183-184` → the frozen `(int32_t)` cast) — pointed at **no phase**, unfixed by decision (Phase 31 D-24). **Guardrail event** requiring operator sign-off and golden re-verification. Direct consequence still binding here: **a permanent repo-wide UBSan gate cannot be adopted**, because it would fail on the shipped module. Any UBSan use in this phase stays a scoped one-shot probe.
- **Per-instance seed entropy + patch persistence in the shell** — Phase 31 deferred item 2, still pointed at **Phase 34/35**.
- **Amplitude fade near the Nyquist ceiling** — considered and rejected in Phase 31 (D-10); revisitable in Phase 34, which owns the output stage.
- **COARSE octave/semitone snap** — Phase 31 deferred; natural home is Phase 35 or v2.1.

### Reviewed Todos (not folded)
- **"Wire `tests/check_docs.sh` into CI"** (`.planning/todos/wire-check-docs-into-ci.md`) — matched Phase 32 at score 0.4 on the single generic keyword *phase*. Already reviewed and deferred to **Phase 36** during Phase 31 on the same reasoning; that decision is carried forward, not re-litigated. It is a one-line CI step for a Phase 27 documentation gate and has nothing to do with band-limiting. Remains visible via the `GUARD_WIRING_EXEMPT` entry in `tests/check_includes.sh` §`[7/7]`, which reports it as `EXEMPT` on every run.

</deferred>

---

*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Context gathered: 2026-07-31*
