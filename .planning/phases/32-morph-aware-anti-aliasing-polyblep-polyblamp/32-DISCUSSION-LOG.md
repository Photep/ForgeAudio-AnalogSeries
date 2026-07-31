# Phase 32: Morph-Aware Anti-Aliasing (polyBLEP/polyBLAMP) - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-07-31
**Phase:** 32-morph-aware-anti-aliasing-polyblep-polyblamp
**Areas discussed:** Jump magnitude source, Discontinuity site map, Alias-floor gate (TEST-03), MorphBlep API & seams, MORPH-02 control surface

---

## Area selection

| Option | Description | Selected |
|--------|-------------|----------|
| Jump magnitude source | AA-04 + roadmap SC-2 require the *measured characterized* jump; research/STACK.md:40 recommends the *pristine* analytic jump and calls measured "a v2.1 nicety, not a v2.0 requirement". Whichever loses needs a source edit before planning. | ✓ |
| Discontinuity site map | The bleed ring injects NON-adjacent shapes; square duty and triangle valley move with character every sample, and again with Phase 34's drift spreads. | ✓ |
| Alias-floor gate (TEST-03) | −60 dB is a target the roadmap says to pin empirically, not a measurement. One threshold or per-shape? Baseline the naive core first? | ✓ |
| MorphBlep API & seams | One header or two? 2-sample polyBLEP corrects the NEXT sample too, so MorphBlep carries state. Does the API take external (position, jump) events so Phase 33 sync plugs in? | ✓ |

**User's choice:** all four.

---

## Jump magnitude source

### Q1 — How should BLEP/BLAMP magnitude be derived?

| Option | Description | Selected |
|--------|-------------|----------|
| Analytic, character-aware | Closed-form jumps INCLUDING the character terms — the tanh edge width is known, so the effective step across one sample is computable, not measurable. Satisfies AA-04's intent at pristine-analytic cost. Duplicates knowledge of Waveshape's internals in a second header. | ✓ |
| Measured (AA-04 literal) | Probe `morphedWave` at phase ±ε and difference it. Auto-handles character, bleed normalization and crossfade weights. Costs ~8 extra transcendental-heavy calls per sample (~400k/sec at 48 kHz). | |
| Pristine analytic (research) | Character-independent jumps, weighted by morph, at full authority. Cheapest, errs in the safe direction. Requires editing AA-04 and roadmap SC-2 — the requirement would be knowingly unmet. | |

**User's choice:** Analytic, character-aware → **D-01**
**Notes:** Cost framing supplied before the question: `morphedWave` computes all five shapes per call (1 `sin`, 2 `exp`, 2 `tanh`, 1 `cos`, plus branches). The research recommendation at STACK.md:40 is overridden **by decision**, not oversight — recorded in CONTEXT.md so downstream agents do not "restore" it.

### Q2 — Roadmap SC-2 says "measured". D-01 chose analytic. Reconcile how?

| Option | Description | Selected |
|--------|-------------|----------|
| Edit SC-2 before planning | Drop "measured" in ROADMAP.md; AA-04 in REQUIREMENTS.md already reads "characterized (actual)" and needs no change on this point. Same D-00 pattern Phase 31 used. One word. | ✓ |
| Leave it, interpret in CONTEXT | Record that "measured" is read as "characterized". Cheaper now; leaves a gate whose prose contradicts the implementation. | |

**User's choice:** Edit SC-2 before planning → **D-02** (blocking pre-planning action)

### Q3 — What sets the character scaling?

| Option | Description | Selected |
|--------|-------------|----------|
| Let research settle it | Lock the SHAPE here (closed-form factor of edge-width vs one sample's phase advance, → 0 when the edge is much wider than a sample, → 1 when much narrower, table-free); `gsd-phase-researcher` derives the exact expression against measured spectra. The roadmap already flags this item MEDIUM-confidence and names `--research-phase`. | ✓ |
| Ratio heuristic, tuned empirically | Fix the form now as a scale factor of `dt/edgeWidth` with a tunable constant, pinned against the alias-floor gate during execution. Faster to plan; the constant becomes a magic number. | |
| Exact closed-form tanh step | Derive the true effective step of the tanh integrated over one sample. Most faithful, no free constants. Heaviest derivation; saw soft reset and rounded peaks each need separate treatment. | |

**User's choice:** Let research settle it → **D-03**

### Q4 — Does one rule govern value steps, slope breaks, and the saw's soft reset?

| Option | Description | Selected |
|--------|-------------|----------|
| One unified rule | The same closed-form character factor scales every correction; each site supplies its own pristine magnitude and character-dependent width. One derivation to validate. The saw's soft reset falls out naturally. | ✓ |
| Per-site-type treatment | Derive value-step and slope-break scaling independently. More faithful in principle; three derivations, three places for the gate to disagree. | |

**User's choice:** One unified rule → **D-03b**
**Notes:** Corollary recorded in CONTEXT.md — the saw's soft reset and the triangle's rounded peaks are fast but *continuous*, not discontinuities; full-authority correction there would be actively wrong, and the D-03 factor is what makes them self-limiting.

---

## Discontinuity site map

### Q1 — Which sites does MorphBlep enumerate each sample?

| Option | Description | Selected |
|--------|-------------|----------|
| Fixed union, every sample | Always evaluate the same site set; magnitude falls to zero when a shape carries no weight. Constant cost, no branching, structurally cannot miss the bleed ring. Positions recomputed live, never cached. | ✓ |
| Active + bleed neighbours | Compute the set per sample from segment + `leftIdx`/`rightIdx`. Smaller working set; branchy, and encodes the ring topology in a second header. | |
| Active segment only | AA-01 as literally worded. Cheapest. Known-wrong — leaves the bleed ring's non-adjacent pulse and saw edges uncorrected. | |

**User's choice:** Fixed union, every sample → **D-04**
**Notes:** Measurement presented before the question: at `morph = 0`, `character = 1`, `frac = 0` gives `leftWeight = 1` and `leftIdx = 4`, so the **narrow pulse** bleeds at full `bleedIntensity ≈ 0.04` inside what the user hears as a pure sine — a ~0.077 step in a ±1 wave after normalization, roughly −22 dB. Found by reading `Waveshape.hpp:200-208` and doing the arithmetic, not by reading the requirement.

### Q2 — Where do corrections sit relative to the bleed normalization?

| Option | Description | Selected |
|--------|-------------|----------|
| Ride through it | Computed against pre-normalization magnitudes and divided by the same `1/(1+bleedIntensity)` factor. Matches research/STACK.md:36. The normalization is linear, so this is exact. | ✓ |
| Apply after, unnormalized | Add at full magnitude to the already-normalized value. Simpler wiring; over-corrects by exactly `(1+bleedIntensity)` whenever character > 0. | |

**User's choice:** Ride through it → **D-05**

### Q3 — AA-01's "scaled by the morph weights" vs the bleed-ring sites

| Option | Description | Selected |
|--------|-------------|----------|
| Fold into the same edit | One pre-planning commit fixes both D-02 and this: widen AA-01 to "scaled by the morph and bleed weights". Consistent with D-02, one commit, gate matches implementation. | ✓ |
| Leave AA-01, record in CONTEXT | Edit only SC-2; note that "morph weights" is read as covering bleed weights. Smaller diff; verifier reads a narrower requirement than the implementation satisfies. | |

**User's choice:** Fold into the same edit → **D-06** (blocking pre-planning action)

### Q4 — The narrow-pulse degenerate case

| Option | Description | Selected |
|--------|-------------|----------|
| Sum both, no duty floor | Each edge at its OWN sub-sample position, corrections summed, never overwritten. When `duty < dt` they partially cancel — physically right, since a pulse narrower than a sample carries less energy. Matches roadmap SC-3. | ✓ |
| Floor the duty at `dt` | Guarantee the pulse always spans a sample. Introduces a sample-rate-dependent timbre change the frozen `Waveshape` knows nothing about; naive and band-limited paths stop being the same waveform. | |

**User's choice:** Sum both, no duty floor → **D-07**
**Notes:** At C8 a 5%-duty pulse is ≈0.57 samples wide, so both edges land inside one sample.

---

## Alias-floor gate (TEST-03)

### Q1 — When does the spectral harness get built?

| Option | Description | Selected |
|--------|-------------|----------|
| Baseline the naive core first | Plan 32-01 builds the helper and records today's aliased floor per shape and note, before band-limiting exists. Threshold set from measurement; objective iteration metric for the phase; genuine RED. Costs one plan up front. | ✓ |
| Build gate with the implementation | Write the assertion alongside MorphBlep and tune afterwards. Fewer plans; no before/after delta, and a threshold chosen after seeing the result is fitted to the result. | |

**User's choice:** Baseline the naive core first → **D-08**

### Q2 — What shape does the assertion take?

| Option | Description | Selected |
|--------|-------------|----------|
| Per-shape, evidence-set | Separate thresholds per morph region, each pinned from baseline plus what band-limiting achieves, each carrying its measured justification. | ✓ |
| Single global threshold | One number (≈−60 dB) everywhere. Simplest; 2-sample polyBLEP without oversampling is unlikely to hit it on the narrow pulse, so the number ends up set by the worst case. | |
| Global floor + pulse carve-out | One threshold with a justified exception for the narrow-pulse region. Middle ground; an exception invites a second exception. | |

**User's choice:** Per-shape, evidence-set → **D-09**

### Q3 — How does the helper avoid measuring its own leakage?

| Option | Description | Selected |
|--------|-------------|----------|
| Integer cycles per block | Whole number of cycles exactly fills the block → leakage exactly zero, rectangular window exact, harmonics and folded aliases land on bin centres. "Alias energy" = magnitude at non-harmonic bins. Test frequencies sit at bin centres rather than exact equal-tempered notes, which is irrelevant to aliasing. | ✓ |
| Blackman-Harris window | Arbitrary musical frequencies, ≈−92 dB sidelobes. Adds window math, guard bands around widened harmonic bins, and a floor depending on the window choice. | |

**User's choice:** Integer cycles per block → **D-10**
**Notes:** Framed as a gate-correctness decision, not convenience — leakage above −60 dB would make the gate measure its own window rather than the DSP, the "mechanism wider than the prose" failure logged four times in Phase 30.

### Q4 — What does the gate sweep?

| Option | Description | Selected |
|--------|-------------|----------|
| 44.1k binding + 48/96k regression | 44.1 kHz is worst case and binding; the other two catch `dt`-scaling bugs, which fail rate-dependently and are invisible to a single-rate gate. VcoBlockDriver already drives all three. Morph at the five shape centres, character at 0 and 1. | ✓ |
| All three rates binding | Every rate asserts its own thresholds. Most thorough; three sets of numbers to justify when two are strictly easier cases. | |
| 44.1 kHz only | Single worst-case rate, broader note/morph coverage for the same runtime. Blind to `dt`-scaling bugs. | |

**User's choice:** 44.1k binding + 48/96k regression → **D-11**

---

## MorphBlep API & seams

**Ruled out before the questions:** `research/STACK.md:61` suggests putting the kernels in `RackCompat.hpp`. That file is byte-pinned by `tests/check_frozen.sh` and consumed by the shipped LFO, so it is a milestone-guardrail event, not a VCO fix. Both options below leave it untouched.

### Q1 — One new header or two?

| Option | Description | Selected |
|--------|-------------|----------|
| Single MorphBlep.hpp | Kernels and site logic together, exactly as CORE-02 words it. One file, one include, one entry in each of three guards. Phase 33 reaches the kernels through the API. | ✓ |
| MorphBlep.hpp + Blep.hpp | Kernels split out; Phase 33 could call one directly. Two files to wire into `check_includes.sh` / `check_frozen.sh` / the canary, and CORE-02 names one. | |

**User's choice:** Single MorphBlep.hpp → **D-12**

### Q2 — How is the next-sample half of each correction delivered?

| Option | Description | Selected |
|--------|-------------|----------|
| Pending accumulator | Residual summed into a small accumulator, added at the top of the following `step()`. Zero added latency, and composes for free with D-07's multi-edge case. | ✓ |
| One-sample output delay | Buffer the naive output so both halves apply to a sample still held. Simpler bookkeeping; adds declarable latency that desyncs against other oscillators, and complicates Phase 33. | |

**User's choice:** Pending accumulator → **D-13**

### Q3 — Does the API make room for Phase 33's sync discontinuity now?

| Option | Description | Selected |
|--------|-------------|----------|
| Design the seam, don't build it | Entry point taking an externally-supplied (sub-sample position, value jump) event feeding the same accumulator. No sync behavior, no POD sync fields this phase. State per-instance, never static, to hold CORE-03. | ✓ |
| Defer entirely to Phase 33 | Keep the API morph-only. Avoids speculative surface; no VCO goldens until Phase 36 so reopening is cheap. Phase 33 would modify the header whose per-shape thresholds were just pinned and re-validate them. | |

**User's choice:** Design the seam, don't build it → **D-14**

### Q4 — Where does deferred item 6 go?

| Option | Description | Selected |
|--------|-------------|----------|
| Keep in Phase 32, new rationale | The original reason (an oversampled inner loop) is dead per AA-05, but this phase adds division by `dt` and `edgeWidth`, so a zero/subnormal/non-finite `sampleTime` reaches arithmetic that did not exist before. Record the corrected rationale. | ✓ |
| Re-point to Phase 36 | Route to the goldens/CI phase. Keeps Phase 32 focused; new division sites ship unguarded through Phases 33-35 including operator auditions. | |
| Close it, no phase | Judge the input class unreachable and retire it. Honest if true; the project has twice rejected guards whose failing case was never observed. | |

**User's choice:** Keep in Phase 32, new rationale → **D-15**

---

## MORPH-02 control surface

Raised at the close as an unresolved boundary: MORPH-02 maps to Phase 32 and reads "MORPH knob + CV + attenuverter", but Phase 31's CONTEXT deferred "MORPH/CHARACTER CV + their attenuverters" to Phase 34, `VcoInputs` annotates `morph` as "(Phase 34)", and the shell has a MORPH knob with no CV jack and no attenuverter. User chose to settle it rather than leave it to planning.

### Q1 — Does Phase 32 declare the MORPH CV jack and attenuverter?

| Option | Description | Selected |
|--------|-------------|----------|
| Declare them here | What both source documents already say. Also serves the phase: audio-rate MORPH CV through segment boundaries is the hardest case the alias floor must survive, and Phase 30's D-07 rule means the operator can audition it. No document edit needed. | ✓ |
| Knob-only, move MORPH-02 to 34 | Keep Phase 32 purely DSP; MORPH-02 lands with CHAR-01 for one consistent treatment. Costs a third document edit and the phase cannot audition audio-rate morph sweeps against its own alias floor. | |

**User's choice:** Declare them here → **D-16**

### Q2 — Where does knob + CV × attenuverter get mixed?

| Option | Description | Selected |
|--------|-------------|----------|
| Shell mixes, POD unchanged | `VcoInputs::morph` is already documented "post-CV, post-clamp"; the boundary was designed for this and has been stable since Phase 29. Phase 31's D-05 is directly on point. Zero POD change protects deferred item 9's canary margin. Not a D-17 violation — param conditioning is not DSP. | ✓ |
| POD gains morphCV/morphAtten | Mirror the Phase 31 FM pattern. Most consistent with the last phase; contradicts the existing field comment, adds fields under the canary's runtime-live obligation, reopens a boundary stable for three phases. | |

**User's choice:** Shell mixes, POD unchanged → **D-17**

---

## Closing gate

| Option | Description | Selected |
|--------|-------------|----------|
| I'm ready for context | 17 decisions captured, two blocking pre-planning edits identified, the one genuinely uncertain item routed to research. | ✓ |
| Explore more gray areas | Identify further ambiguities before writing. | |

---

## Claude's Discretion

- The exact closed-form character-scaling expression (routed to `gsd-phase-researcher` within D-03's locked shape constraints).
- The mechanism for keeping the naive path callable for D-08's baseline.
- Block lengths, cycle counts, the bin-centred frequency grid, the DFT implementation, and the note grid for D-10/D-11.
- Whether the D-04 site set is a namespace-scope `static constexpr` array or unrolled straight-line code (never in-class `static constexpr` indexed at runtime).
- Updating `tests/test_vco_core.cpp:416`'s naive-path oracle and re-deriving the `:511` ±5.55 V bound.
- Whether `MorphBlep` is a by-value struct member of `VcoCore` or a free-function set with explicit state.

## Deferred Ideas

- Measured/probed jump magnitudes as a narrow escalation if one morph region misses its threshold for magnitude reasons.
- Higher-order (4-point) polyBLEP — operator decision with impact assessment, not a silent implementation choice.
- CHARACTER CV + attenuverter → Phase 34; hard sync behavior → Phase 33; output stage and drift → Phase 34 (which must re-read D-04, since drift moves discontinuity positions).
- The shipped LFO's shared latent UB — no phase, unfixed by decision (Phase 31 D-24); still blocks a permanent repo-wide UBSan gate.
- Per-instance seed entropy + patch persistence → Phase 34/35. Amplitude fade near the Nyquist ceiling → revisitable in Phase 34. COARSE octave/semitone snap → Phase 35 or v2.1.
- **Reviewed, not folded:** "Wire `tests/check_docs.sh` into CI" — carried forward from Phase 31's deferral to Phase 36, not re-litigated.
