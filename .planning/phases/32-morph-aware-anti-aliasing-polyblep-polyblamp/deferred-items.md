# Phase 32 — Deferred Items

Two sections. **Section one** records every premise this phase FALSIFIED and corrected, each
naming the source file where its correction now lives, so the record and the code cannot drift
apart. **Section two** records every refinement this phase deliberately declined and every item
it inherited, each with an owner — **except items pointed at NO PHASE, which say so and say why.**

Same format as `.planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md`
(found during · observation · why it is worth a note · why it is not fixed here · resolve at).

**Why section one is unusually long.** Nine consecutive plans in this phase found a premise that
measurement falsified, and several of those premises would have produced assertions that passed
**vacuously** — green against exactly the defect they existed to catch. That is the phase's
strongest signal and the reason every entry below names a file and a line.

---

# SECTION ONE — FALSIFIED PREMISES, CORRECTED IN THIS PHASE

## 1. D-15's own premise: "Phase 32's oversampled inner loop is the first real source of exotic timing"

- **Found during:** plan **32-06** (which corrected the header's copy) and plan **32-09** (which
  corrected the test suite's copy at the point of use). **The premise appeared in TWO places, not
  one — which is why two corrections are named.**
- **Observation:** Phase 31 deferred item 3 pointed the hostile-timing grid extension at Phase 32
  on the stated reasoning that *"Phase 32's oversampled inner loop is the named future caller that
  decouples `sampleTime` from `sampleRate` on purpose, and is therefore the first real source of
  exotic timing."* **`AA-05`'s own wording is "no minBLEP, no oversampling in v2.0."** No such loop
  exists in this phase, and none will be added to it.
- **The CONCLUSION survives on the real reason.** The grid extension was right; the reason was
  wrong. This phase is the first to put a **division by `dt`** behind `forge::VcoInputs::sampleTime`
  — `forge::MorphBlep::step` divides the sub-sample distance by the phase increment — so exotic
  timing became reachable for a reason that has nothing to do with oversampling.
- **Corrections landed at:**
  - **`tests/test_vco_core.cpp:1101`**, in the block opening *"THE FALSIFIED PREMISE THAT POINTED
    THIS WORK HERE, CORRECTED IN PLACE"*, sitting directly above `HOSTILE_TIMES`.
  - **`src/dsp/VcoCore.hpp:491`**, in the increment-bound paragraph.
  - Both verified this session: `grep -n 'AA-05 forbids oversampling'` returns exactly one hit in
    each file.
- **Why it is worth a note:** a deferral that carries its own false reason is worse than no
  deferral. The next phase to read Phase 31's register would have gone looking for an oversampled
  loop, failed to find one, and had to guess whether the item still applied.
- **Not "fixed" beyond the correction** — there is nothing to fix; the work was done (48 → 176
  configurations) and only the rationale was wrong.
- **Resolve at:** CLOSED. Recorded here so no later phase inherits the old reason.

---

## 2. D-03's saw corollary (P-4): "as character rises the saw wrap's effective step shrinks"

- **Found during:** plan **32-04**, and made permanent by plan **32-05**.
- **Observation:** the claim is **false**. The curved saw evaluates to 1 at phase 0 *before* the
  reset is applied, and the reset blends **from** a reset value of 1 **toward** the curved saw, so
  both are 1 there. **MEASURED on `computeSaw` itself: the wrap jump is +2 at every character** —
  1.999600 / 1.999805 / 1.999821 / 1.999847 / 1.999884 at characters 0.00 / 0.25 / 0.50 / 0.75 /
  1.00, the residual being the `eps` probe bracket rather than character. The saw site therefore
  has width 0 and a D-03 factor of exactly 1 at **every** character.
- **The CONCLUSION survives.** D-03 said not to correct the soft reset separately, and that is
  still right — its slope-break magnitude is about three and a half orders of magnitude under the
  value step. Only the premise was wrong.
- **A SECOND, RELATED FALSIFICATION, from the same probe:** plan 32-05's own plan text asserted the
  **morphed** wrap jump is +2.0 at every character. **MEASURED 1.922966 at character 1.** The saw
  is unchanged; the difference is **D-05's bleed normalization**, which divides the whole result by
  `(1 + bleedIntensity)`. The assertion was moved to the closed form `2/(1 + bi)`, which is
  **stricter** than the constant would have been — it pins the normalization as well as the saw.
- **Corrections landed at:** **`src/dsp/MorphBlep.hpp:422-427`**, site table ENTRY 1, opening
  *"P-4, A FALSIFIED PREMISE CORRECTED IN PLACE"*; and permanently asserted at five character
  values in **`tests/test_morph_blep.cpp`** (case *"the site magnitudes ARE the characterized jumps
  of the frozen Waveshape (AA-04 / D-01)"*, part C).
- **Resolve at:** CLOSED.

---

## 3. `research/STACK.md:40`: "erring toward over-correction is the safe direction"

- **Found during:** plan **32-04**. The same argument is restated in this phase's own `32-CONTEXT.md`
  under specific ideas, which is why it needed correcting in the code rather than only in research.
- **Observation:** **FALSIFIED BY MEASUREMENT.** The argument was that character is a lowpass-ish
  coloration, so extra correction only adds harmless HF rolloff. At **character 1 / C6 / the square
  centre**, the naive floor is **−60.1 dB** and full authority measures **−29.9 dB** — a **30 dB
  REGRESSION**. The residual is a **step-shaped correction added to a signal that has no step**:
  new broadband energy, not a filter.
- **Why it is worth a note, and it is the load-bearing one in this section:** this is what makes
  the D-03 factor's **compact support a requirement rather than a preference**. The factor must
  reach *exactly* zero at a finite edge width, and the phase's own rejected alternatives — the
  non-compact forms — regress by **−60.4, −42.7, −36.6 and −29.8 dB**. The 90-cell no-regression
  invariant separates the shipped form from all four by at least **25.8 dB**.
- **Correction landed at:** **`src/dsp/MorphBlep.hpp:38-49`**, in the banner beside the character
  factor, naming the source and quoting both measured figures.
- **Resolve at:** CLOSED.

---

## 4. `research/STACK.md:100-104`'s polyBLAMP snippet — wrong polynomial order, wrong placement of `dt`

- **Found during:** plan **32-04**.
- **Observation:** the published snippet returns a **quartic** where the 2-point form is **cubic**,
  and folds `dt` into the **kernel** rather than into the **slope**. It is neither the 2-point form
  used here nor the DAFx-16 four-point form — it is not a form of anything.
- **Why it is worth a note:** it is a copy-paste hazard sitting in this project's own research
  directory. An editor reaching for "the polyBLAMP snippet we already have" would silently install
  a kernel of the wrong order.
- **Correction landed at:** **`src/dsp/MorphBlep.hpp:50-60`**, opening *"A SECOND FALSIFIED SOURCE,
  NAMED"*, carrying the derivation that replaces it. The shipped forms were derived from first
  principles and cross-validated: **7e-7** against the canonical two-branch form at `dt = 0.094`,
  and the corrected saw's harmonic gain matches a squared sinc to **0.01 dB over twelve harmonics**.
- **A reconciliation, NOT a falsification, recorded alongside it** at `src/dsp/MorphBlep.hpp:173-179`:
  the widely published two-branch kernel returns **twice** this residual and is applied at **half**
  the jump. A future editor cross-reading against a published listing will find a factor of two,
  and that paragraph is the reconciliation rather than a bug report.
- **Resolve at:** CLOSED.

---

## 5. D-04's site map as literally worded: "the square duty edge", singular

- **Found during:** plan **32-04**, and pinned permanently by plan **32-05**.
- **Observation:** "the square duty edge" is **one** site in the prose and **two** in the frozen
  code — a **hard step at `0.5f`** and a **soft edge at the duty**. Merging them produces
  single-sample full-amplitude spikes measuring **0.0 dB spectrally**, i.e. completely invisible to
  the alias-floor gate, which is why this needed a dedicated assertion rather than trust in the
  spectral metric.
- **MEASURED at morph 0.70 / character 0.50:** the jump at `0.5` is **−1.201655** and the jump at
  `dutySq = 0.510000` is **−0.002073** — a difference of **1.199583**. A character-1.00 complement
  row proves the discriminator vanishes at full character (0.000386), so nobody generalises the
  case to a character where it cannot fail.
- **Corrections landed at:** **`src/dsp/MorphBlep.hpp:446-447`** as two separate site entries —
  entry 4 carrying the comment *"THE SQUARE'S HARD STEP — at 0.5f, NOT at dutySq"* and entry 5 the
  soft edge — and as an assertion in **`tests/test_morph_blep.cpp`** (case two, part D).
- **⚠ A COMPOUNDING FALSIFICATION worth its own paragraph, because it is the phase's clearest
  example of a vacuous assertion.** Both plan 32-04 and plan 32-05 specified this probe at
  **`morph = 0.75`**. At exactly 0.75, `scaled` is exactly 3.0, `segment` is 3, and the frozen
  direct-duty special case (`Waveshape.hpp:179-182`) sets `W[4] = 1` and leaves **`W[3] = 0`** —
  the square carries **no weight at all**. The probe there measures a difference of **1.485677 and
  would have PASSED** — but on the **PULSE's** hard step, and the pulse does **not** split. **The
  assertion would have been green against exactly the merged-square defect it exists to catch.**
  Relocated to `morph = 0.70`, where `W[3] = 0.8` is genuinely live. Unsatisfiable is a nuisance;
  **vacuous is a hazard**, and this one was vacuous.
- **Resolve at:** CLOSED.

---

## 6. `tests/test_vco_core.cpp` invariant 1's grid attribution — resolved by measurement, and the answer was the opposite of the expected one

- **Found during:** plan **32-08**, Task 3.
- **Observation:** invariant 1's narrow-pulse grid stopped at `pitchCV +2` with a forward reference
  saying **"Phase 32 owns it"** — the implication being that band-limiting would fix the tracking
  error at higher notes. Re-measured against the band-limited core, **band-limiting made the
  measurement WORSE**:

  | pitchCV | 5 % region | pre-Phase-32 | NOW (band-limited) |
  |---|---|---|---|
  | +2.00 | 2.11 samples | — | +0.0010 % |
  | +3.00 | 1.05 samples | — | +0.0031 % |
  | **+3.50** | 0.74 samples | **−24.53 %** | **−34.7383 %** |
  | **+4.00** | 0.53 samples | **−46.89 %** | **NO CROSSINGS AT ALL** |

- **The +4.0 point is wrong in KIND, not just in magnitude.** It is not a −100 % tracking error:
  the output never reaches zero — **max −0.426132 V, 0 of 11025 samples at or above zero** — so
  `estimateFreqRising` returns its negative sentinel and the existing `REQUIRE(nUp >= 8)` fires
  before the tolerance is ever consulted. The estimator is not measuring badly; it is reporting
  that it cannot measure.
- **The corrected premise names the ESTIMATOR, not the DSP.** It counts **rising zero crossings**;
  the narrow pulse's positive region falls under about two samples above pitchCV +2; and softening
  the edges *lowers the peak further inside an already-marginal window*, which is why the figures
  moved the wrong way. **That the numbers got worse is the strongest possible evidence for the
  corrected premise** — a plan that had assumed improvement would have recorded the opposite of
  the truth.
- **Where high-note behaviour is asserted INSTEAD:** spectrally, in
  **`tests/test_vco_spectrum.cpp`** at C7, C8 and C9 (2099, 4188 and 8367 Hz), which is the right
  instrument. Correction landed at **`tests/test_vco_core.cpp:594`**, *"THE CORRECTED PREMISE.
  estimateFreqRising counts RISING ZERO CROSSINGS."*
- **The grid's stop at +2 is CONSERVATIVE rather than the exact edge** — +3.0 sits at 1.05 samples
  and still tracks to +0.0031 % — and the source now says so instead of implying a sharp threshold.
- **Resolve at:** CLOSED as an attribution. The estimator's sub-two-sample blindness is a property
  of that instrument, not a defect to fix.

---

## 7. A FOURTH and a THIRD stale banner sentence, neither named by any plan

- **Found during:** plan **32-06** (`src/dsp/VcoCore.hpp`) and plan **32-08**
  (`tests/test_vco_core.cpp`) — both by reading a file in full because `<read_first>` said to.
- **Observation:** two files described themselves as producing deliberately aliased output **after
  the commit that band-limited them**:
  - `src/dsp/VcoCore.hpp:7-11` — *"step() is now a NAIVE, DELIBERATELY ALIASED morphed oscillator"*,
    plus a phase-ownership list saying Phase 32 *"owns"* band-limiting as future work.
  - `tests/test_vco_core.cpp` banner — *"THE PHASE-30 OSCILLATOR ALIASES BY DESIGN"*.
- **Why it is worth a note:** a file whose self-description contradicts its body is the exact
  failure the house rule about falsified premises exists to prevent, and **neither plan named
  either sentence.** The plan's silence about a banner is not permission to leave it wrong.
- **Corrections landed in place**, both recording what the sentence **used to** say. The test
  banner's **operative half survives with its real reason**: no assertion in that file may make a
  spectral claim — not because the oscillator aliases, but because **it is the wrong instrument**,
  and spectral claims belong to `tests/test_vco_spectrum.cpp`.
- **A related correction, same class:** `src/AnalogVCO.cpp`'s banner carried a *"Phase 32 owns
  band-limiting"* forward reference, corrected by plan **32-02**.
- **Resolve at:** CLOSED.

---

## 8. The `measuredDb` column reproduces on Apple clang ONLY — the phase's own measuring instrument is toolchain-dependent

**This is a first-class discovery of this phase, not a bookkeeping note. It was found by the CI
observation this plan exists to perform, and it could not have been found any other way.**

- **Found during:** plan **32-10**, Task 2 — the first time this phase's 90-cell spectral grid ran
  on a non-Apple toolchain. Run **30680251253** on SHA **a110a9a**.
- **Observation:** `make test` was **RED on ubuntu-latest (GCC/libstdc++) and windows-latest
  (MinGW g++) and GREEN on macos-latest**, failing 21 assertions in one case — the ±1.0 dB
  `recordedDrift` reproduction CHECK plan 32-07 added at `tests/test_vco_spectrum.cpp`. The two
  non-Apple legs disagree with the recorded `measuredDb` column by up to **3.02596 dB** (cell
  **i = 86**, 96 kHz, morph 0.75 square, character 1.00). The column had been pinned from **one
  toolchain's run**.
- **NOTHING IN `src/` BEHAVES DIFFERENTLY, and that was established before anything was changed:**
  `make strict` and the **MinGW compile-and-link leg** are green on that same SHA, the **TEST-03
  gate itself passes on all three legs**, the no-regression and cross-rate invariants pass
  everywhere, and the six shipped LFO goldens replay bit-exact on every leg. **What differs is the
  INSTRUMENT, not the oscillator.**
- **The mechanism, and it is checkable:** `aliasPeakDb` reports a **max over 2043 non-harmonic
  bins**. The FFT twiddles and the frozen `Waveshape`'s own trig come from the platform's libm, and
  libm results differ in the last unit in the last place between implementations. One ULP cannot
  move a peak standing tens of decibels clear of its neighbours — but where the alias spectrum is a
  near-**flat plateau of near-tied bins**, one ULP **reorders which bin wins** and the reported max
  steps to a different bin at a materially different level.
- **Why it is worth a note beyond the fix:** it means **every absolute decibel figure this phase
  recorded is an Apple-clang figure**, to within about 3 dB in the plateau regime and about 1 dB
  elsewhere. Phase 36's goldens and any future re-pin must not treat the recorded column as
  platform-neutral. It is also a second, independent vindication of the Phase 29 rule that local
  green is a precondition and never evidence: **the entire local gate was green on this defect**,
  exactly as it was on the commit that could not link.
- **How it was fixed, and why that is not a softening.** The bound was **split on a PHYSICAL
  criterion stated before the population was enumerated**, so the split could not be a rename of
  "the cells that failed today":
  - **STEP-DOMINATED** — the waveform at that (region, character) carries a true value-step
    discontinuity, whose 6 dB/octave series folds to peaks standing clear of the spectrum, so the
    arg-max is stable. This is **saw at every character** (the +2.000000 jump of item 2 above),
    **pulse at every character**, and **square below full character** (−1.201655 at 0.50).
    **Bound stays 1.0 dB, unchanged.**
  - **PLATEAU** — no value step, so alias content is a slope break or the bleed ring alone.
    **Sine**, **triangle**, and **square at character 1.00**, where plan 32-05 measured the jump
    collapse to **−0.001661**. **Bound 4.0 dB**, pinned from the measured worst of 3.02596 rounded
    outward, leaving 0.974 dB of headroom — the same outward-rounding rule the MEASURE-TO-PIN
    PROTOCOL's step 2 states, and the same shape as 32-07's two re-pinned tolerances.
  - The populations **fall out of the criterion** as **48 and 42**, both asserted exactly, so a
    classifier that emptied either side fails loudly instead of making one bound vacuous.
  - **The split is a strict SUPERSET of the observed failures.** 21 cells drifted; all 21 fall
    inside the 42. The other 21 plateau cells reproduced within 1.0 dB anyway — they are on the
    looser bound because the criterion says they **can** be fragile, not because they were seen to
    be. And **all 48 step-dominated cells reproduced within 1.0 dB on all three legs**, which is
    the criterion's own prediction and the evidence that it is the right criterion.
  - **Sensitivity proved by discriminating mutation probe:** a +2.0 dB offset on `recordedDrift`
    fails **exactly 48** assertions — all saw, all pulse, square at characters 0 and 0.5, and **no
    plateau cell** — and a +5.0 dB offset fails **exactly 90**. Both branches bite, at the boundary
    they claim.
  - **T-32-15 is left at least as well defended.** No threshold and no `measuredDb` value was
    edited (**0 grid rows in the diff**); the standing **STOP-AND-REPORT** instruction is preserved,
    now binds **both** branches, and gained a clause forbidding **reclassifying a cell across the
    split** to green a build; and every gated threshold is still independently derived from
    `measuredDb` by the TEST-03 gate's derivation assertion, which fires on **any** re-typing.
- **What is NOT fixed and is genuinely open:** the phase has no cross-toolchain **golden** for the
  spectral figures, and the plateau cells' true cross-platform spread is known only from a single
  CI run's 21 exceedances plus 69 cells known to be within 1.0 dB. A second data point would tell
  us whether 3.03 dB is the tail or the typical worst.
- **Resolve at: Phase 36 (TEST-05 / REL-01)**, which owns goldens and cross-platform CI and is
  already required to decide what is portable and what is macOS-gated. It should treat the
  plateau population as **explicitly non-portable at the decibel level** and must not capture a
  spectral golden from Apple clang alone. **The fix landed here is a bound, not a golden**, and
  the distinction matters.

---

# SECTION TWO — DEFERRED, WITH OWNERS

## 9. The narrow-pulse "reach" refinement — the phase's one known DSP gap, MEASURED

- **Found during:** plans **32-04** (proposed), **32-06** and **32-07** (magnitude measured).
- **Observation:** the shipped `forge::MorphBlep` **under-performs the 32-RESEARCH prototype by a
  consistent 3–5 dB on the cells where the BLEED RING dominates** — sine and square at high
  character — and by **~0 dB everywhere else**: 34 of 45 gated cells reproduce the prototype within
  **0.64 dB**. The same regime produces the grid's **only two regressions** (2.3344 dB and
  0.7442 dB, both C9 sine, where the 5 % pulse the bleed ring introduces is 0.05 wide and its two
  edges fall inside a single kernel span, so the two corrections partly work against each other).
  The refinement — a rational approximation in place of the exact hyperbolic tangent — was measured
  at **+1.3 dB at the single worst grid point and about +0.1 dB mean**.
- **Why it is worth a note:** it is the phase's remaining known gap, it is **bounded, explained and
  attributed**, and it is where the next iteration budget on the alias floor should go.
- **NOT taken, deliberately:** it would add **the only division by an edge width** in the header,
  which is precisely the divisor class `T-32-02` exists to keep out. It does not prevent TEST-03
  from being achieved — every gated cell passes without it.
- **Resolve at:** **the first plan that misses a pulse threshold.** It is the documented cheap first
  step of the anti-softening escalation, and `src/dsp/MorphBlep.hpp`'s banner points at it by name.

---

## 10. Four-point (quintic) polyBLEP and polyBLAMP — an OPERATOR DECISION, never a silent choice

- **Found during:** phase context gathering; carried through planning as an explicit `<deferred>`.
- **Observation:** raising the kernel order would roughly **double the decibel attenuation**, moving
  the saw at C8 from about **−25.8 to about −36 dB** — **still not −60**. `AA-05` forbids **minBLEP
  and oversampling by name** but **does not speak to kernel order**, so this is not forbidden; it is
  unscoped.
- **Why it is worth a note:** the tempting reading of AA-05 is that nothing about the kernel may
  change. That reading is wrong, and leaving it unrecorded invites either a silent order bump or a
  wrongly-refused one.
- **Not taken here.** D-09's evidence-set thresholds are the intended response to a floor that
  cannot be reached, and they were all met.
- **Resolve at:** **an OPERATOR DECISION with an impact assessment — never a silent implementation
  choice.** The broad escalation path remains **v2.1 oversampling, explicitly NOT minBLEP**
  (`research/STACK.md:154-156`; minBLEP is on `PROJECT.md`'s Out of Scope list because it is
  SDK-coupled and builds a startup table, which breaks Rack-free `make test` and golden
  bit-stability).

---

## 11. Probed rather than analytic jump magnitudes

- **Found during:** phase context gathering (`research/STACK.md:40` frames it as a v2.1 nicety).
- **Observation:** D-01 chose analytic character-aware magnitudes, which land between pristine and
  probed. Probing a region's magnitudes directly is the **narrow** escalation.
- **Not taken:** nothing required it. Every site magnitude is instead **asserted** against a direct
  probe of the frozen `Waveshape` in `tests/test_morph_blep.cpp`, which is what makes AA-04
  evidence rather than a restatement of the header's own table.
- **Resolve at:** **only if a D-09 threshold proves unreachable AND research attributes it to
  magnitude error rather than kernel order.** Both conditions, not either.

---

## 12. The shipped Analog LFO's shared latent undefined behavior — pointed at NO PHASE

Inherited as **Phase 31 deferred item 1 (D-24)**. **Restated unchanged, still deliberately unowned.**

- **Observation:** `src/AnalogLFO.cpp:320` → `src/dsp/LfoCore.hpp:183-186` → the frozen
  `src/dsp/RackCompat.hpp:106` float-cast-overflow and `:109` left-shift overflow. A hostile or
  non-finite cable voltage into the **live, library-published** module's FM jack reaches undefined
  behavior **today**. It has never been reported because the frequency floor downstream sanitises
  the *result*, so every behavioral assertion a reasonable person would write is already green.
- **UNFIXED BY DECISION.** Fixing it means editing a byte-pinned frozen header consumed by a module
  live in the VCV Library behind six bit-exact goldens. That is a **GUARDRAIL EVENT** requiring
  operator sign-off and golden re-verification, not a VCO fix.
- **THE DIRECT CONSEQUENCE, and it still binds this phase:** **a permanent repository-wide
  undefined-behavior sanitizer gate cannot be adopted while this stands.** It would turn the
  **shipped** module red on the first hostile-input probe. **Phase 32 used no sanitizer at all**,
  and any future use stays a **scoped one-shot probe**.
- **Resolve at: NO PHASE.** Deliberately unowned. Whoever picks this up is **opening a guardrail
  event and must open it as one** — impact, remediation options and a recommendation surfaced to
  the operator before any edit.

---

## 13. Hard sync — the seam is `forge::MorphBlep::addStep`, and the reset must never snap to exactly zero

- **Found during:** D-14, honoured by plans 32-04 and 32-05.
- **Observation:** the seam exists and is **permanently pinned**: `tests/test_morph_blep.cpp` case
  five parts A and B assert that a driven morph site at `s = 0.5` and `addStep(0.5, 2)` produce the
  **same** `+0.250000 / −0.250000` split — they feed the **same** accumulator — that events
  **compose by summation**, and that the entry gate rejects a negative, over-range or
  not-a-number position **without touching per-instance state**.
- **Phase 32 added NO sync field to `forge::VcoInputs`**, as D-14 requires, and **no header change
  is needed** to plug sync in.
- **Resolve at: Phase 33 (SYNC-01/02).** Two binding constraints carry with it: **never snap the
  reset to exactly 0** (`research/STACK.md:149`), and **PITCH-04 must be RE-CONFIRMED, not
  inherited** — Phase 31 deferred item 11 records that PITCH-04 was marked complete on evidence
  covering extreme pitch and extreme FM only, because **sync did not exist to drive**.

---

## 14. Phase 34's binding re-read of D-04: drift makes every discontinuity position move PER SAMPLE

- **Found during:** plan **32-04**, written into the header as a forward consequence.
- **Observation:** `forge::MorphBlep` recomputes its site geometry **every sample and caches
  nothing**. Today the positions move with `character`; once Phase 34's drift engine starts writing
  the component-spread fields, **they will move per sample**. A cached site table would
  **desynchronise SILENTLY** — no error, no gate, just a correction placed where the edge used to
  be.
- **The four fields it reads**, named so Phase 34 cannot miss one:
  **`wv.squareDutySpread`**, **`wv.pulseEdgeSpread`**, **`wv.bleedSpread`**, and
  **`wv.triAsymmetrySpread`**.
- **Resolve at: Phase 34.** The "recompute, never cache" rule at `src/dsp/MorphBlep.hpp:380-388` is
  what keeps `MorphBlep` correct then, and Phase 34 must re-read it before touching `DriftEngine.hpp`.

---

## 15. CHARACTER's CV input and attenuverter

- **Observation:** D-16 pulled **MORPH's** CV jack and attenuverter forward into Phase 32 and
  explicitly left **CHARACTER's** where they were — they were **deliberately not added alongside
  them, however symmetric the panel would have looked.**
- **Two consequences carry forward.** (a) Closing this gap also closes the **compile canary's
  one-field margin** — `drift` is still the only `VcoInputs` DSP field the shell does not feed, so
  `src/vco_compile_canary.cpp` must keep feeding **every** field a runtime-derived value or
  `check_canary.sh [2b/5]` silently stops proving anything the shell does not already prove.
  (b) **Phase 30 deferred item 3 / CR-02's remaining half lands here**: `forge::clamp` is
  NaN-transparent, and the fix must stay **local to `VcoCore`** and must **never** edit the frozen
  shared helper. Phase 32 already applied the negated-comparison pair to `morph` and `character`
  inside `forge::VcoCore::step`, so **the worked example now exists in the same function**.
- **Resolve at: Phase 34 (CHAR-01).**

---

## 16. Per-instance seed entropy and patch persistence in the shell

Inherited as **Phase 30 deferred item 2 / Phase 31 item 5**. **Restated unchanged.**

- **Observation:** `src/AnalogVCO.cpp` hardcodes both the drift seed and the spread seed, so every
  live VCO instance in a patch is a **bit-identical clone** — measured at 0 of 2048 differing
  samples. The `forge::VcoCore`-level divergence invariants drive two *differently-seeded* cores,
  which the shell never constructs.
- **Resolve at: Phase 34/35.** **ANY implementation MUST re-validate a deserialised value:** a
  `(0,0)` Xoroshiro pair is a fixed point emitting an all-zero stream, so the rejection loop inside
  `std::normal_distribution` never terminates. **In Rack that is a hang while opening a patch, not
  a failing test.**

---

## 17. The FM DEPTH knob's affordance — the VCO panel's first operator feedback

Inherited as **Phase 31 deferred item 14**. **Restated; behavior still closed, affordance still open.**

- **The split that matters:** **FM-02's bipolar BEHAVIOR is locked and verified** (31-06 invariant 6
  proved it behaviorally — `fmAtten = -1` inverts `+1` bit-exactly and `0` is a bit-exact no-op).
  Only **how the control reads** is open. Phase 35 changes how it reads, **never what it does**.
- **The real in-house precedent, verified rather than assumed:** the shipped LFO has **zero**
  bipolar params, so there is **no in-house visual language for bipolar to copy** — Phase 35 must
  invent one. The convention that *does* exist is about **role and size**: all five of the LFO's
  CV-depth controls are `ForgeTrimpot`, while the VCO's identically-named `FM_ATTEN_PARAM` is a
  full-size `RoundBlackKnob`.
- **Resolve at: Phase 35 (PANEL-01/PANEL-02).**

---

## 18. A COARSE octave (or semitone) snap

Inherited as **Phase 31 deferred item 6**. **Restated unchanged.**

- **⚠ Do not misread the out-of-scope list.** `PROJECT.md` § Out of Scope lists *"Octave snap /
  semitone selector"* — **that entry was written about the shipped LFO**, where sub-audio rates made
  it meaningless. **It is not a ruling on the VCO.**
- **Not built:** PITCH-02 says *"continuously"* and Phase 31 honoured that literally; invariant 4 of
  `tests/test_vco_pitch.cpp` *measures* continuity with non-integer coarse values that would fail by
  four orders of magnitude under an octave snap.
- **Resolve at: Phase 35** (first phase where the VCO plausibly gains patch state) **or v2.1.**

---

## 19. The documentation gate is still not wired into CI

Inherited as a **reviewed todo, not folded**. **Restated unchanged.**

- **Observation:** `tests/check_docs.sh` is a real guard script that **nothing invokes**. It is not
  silently invisible: `tests/check_includes.sh [7/7]` carries it in `GUARD_WIRING_EXEMPT` with a
  written reason and reports it as **`EXEMPT` on every `make guards` run** — verified again by this
  phase's gate, and visible in this phase's CI logs.
- **Not folded into Phase 32:** it matched at score 0.4 on the single generic keyword *phase*, and
  has nothing to do with band-limiting.
- **Resolve at: Phase 36**, which owns CI and the release.

---

## 20. `plugin.json` still declares version `2.0.1` while shipping a second module

Inherited as **Phase 30 deferred item 4 / Phase 31 item 12**. **Restated unchanged.**

- **Observation:** `v2.0.1` is tagged and live in the VCV Library with a **single** module; the
  manifest now declares **two** modules under that same version string. The library builds from git
  tags, so two artifacts carrying one version string describe different plugins.
- **Not fixed here, deliberately** — this is **D-04**, an explicit Phase 30 decision to hold the
  version, recorded so a stale version is distinguishable from a forgotten one. Phase 32 added no
  module and changed no manifest field.
- **Resolve at: Phase 36 (REL-01)**, which **must re-observe the CI link leg on whatever commit IT
  tags**, per the standing no-tag-on-local-evidence rule.

---

## 21. `kSelfCheckDb` should stay at −72.0 — an answered question, recorded so it is not re-opened cold

- **Found during:** plan **32-01** flagged it; plan **32-07** answered it from measurement.
- **Observation:** `kSelfCheckDb` is `−62.0 − 10.0 = −72.0`. The tightest threshold the grid asserts
  on any **gated** cell is **−75.0** (the six floored sine cells), which would imply a bar of −85.0;
  every one of those cells is measured by method two at a leakage of **−91.95 to −125.51 dB** and
  clears it comfortably.
- **RECOMMENDATION: leave it where it is.** The Part C self-check's −72.0 is **looser** than the
  per-cell `REQUIRE(impliedLeakage <= threshold − 10.0)` that already runs in front of **every**
  measurement in the measure pass, the TEST-03 gate and the D-11 case — so it is a floor on the
  instrument and **not the operative bar**. Tightening it to −85.0 would make Part C assert what the
  per-cell REQUIREs already assert, and would couple a global constant to the floored subset of the
  column.
- **Resolve at:** nobody, unless the solver itself improves. **`kThresholdFloorDb = −75 dB` bounds
  how tight any future threshold can be**; to assert tighter than −75 dB anywhere, **the solver —
  not the floor — is what has to move.**

---

## 22. At `dt = 0.0005` exactly, the pulse edge at 0.374000013 is missed once per cycle

- **Found during:** plan **32-05**, the P-3 resonant-tiling case.
- **Observation:** the preceding sample sits **1.0000257 samples** away from the site — just outside
  the `s <= 1` gate — and the next sample is already past it, so the correction is **never placed**.
  A lost half-jump on a **measure-zero set** of increments, not an envelope spike, which is why the
  resonant case stays green (per-row maxima 1.000000 / 0.997545 / 0.988873 against a 1.11 bound).
- **RECORDED RATHER THAN "FIXED", deliberately:** widening the gate trades a **missed** edge for a
  **double-fired** one, which `src/dsp/MorphBlep.hpp` explicitly rejects as the companion
  anti-pattern.
- **It did not bite anywhere in this phase** — plans 32-06, 32-07, 32-08 and 32-09 each checked and
  none showed the signature; `dt = 0.0005` is not on the 44.1/48/96 kHz rate set (which give
  2.2676e-5, 2.0833e-5 and 1.0417e-5).
- **Resolve at:** **the first plan where a single grid cell misses its threshold at a suspiciously
  round sample rate.** This is the first thing to check there.

---

## 23. `MorphBlep`'s `dt` upper bound is UNREACHABLE from `forge::VcoCore` — a live tripwire for a future phase

- **Found during:** plan **32-09**, measured rather than assumed.
- **Observation:** the guard `!(dt <= 1.f)` — added by plan 32-05 after a `+infinity` `dt` was found
  to poison an instance **permanently** with a NaN — **fires 0 times in 176 hostile configurations
  and provably cannot fire from this call site**, because `forge::VcoCore` clamps at
  `kVcoMaxDeltaPhase = 0.5`, a full factor of two below it. The guard is real and is exercised, but
  **only through `tests/test_morph_blep.cpp`**, never through the core.
- **Recorded as an unreachability rather than papered over**, and the census that establishes it is
  in the source: **140 of 176 configurations reach the guard, 36 pass it, UPPER fires 0.**
- **Resolve at:** **any future phase that raises `kVcoMaxDeltaPhase` above 1.0, or adds a caller
  that does not clamp.** At that moment the guard changes from unreachable to live and **scenario
  four's census must be re-measured rather than assumed to still read `UPPER = 0`.**

---

## 24. MORPH-02's shell-side mix is asserted by NO test case

- **Found during:** plan **32-10**, at the requirement-traceability step.
- **Observation:** MORPH-02 reads *"MORPH knob + CV + attenuverter sweep the continuous 5-shape
  crossfade … at audio rate."* The **crossfade-at-audio-rate** half is discharged by a named case
  (`vco core: audio-rate MORPH sweeping through every segment boundary…`, 27 configurations). The
  **knob + CV × attenuverter** half is **not asserted anywhere**: D-17 added **zero POD fields**, so
  `forge::VcoInputs::morph` is post-CV and post-clamp and **no headless driver can reach the
  attenuverter**. The mix lives in `src/AnalogVCO.cpp::process()` and is covered only by
  `make strict` and the real link.
- **Why it is worth a note:** this is the same shape as Phase 31 deferred item 11 (PITCH-04 marked
  complete on two of the three input classes it names). MORPH-02 **is** marked Complete — it has a
  named case that names it — but the mark is **qualified**, and recording the qualification is the
  difference between a documented gap and a PANEL-03-style false green.
- **Resolve at: plan 32-11**, the operator in-Rack UAT, which is the only instrument that can see a
  cable driving a jack through an attenuverter into a running module. If a later phase wants this
  headless, it needs either a shell-level test harness or POD fields — and **adding POD fields
  re-opens the canary margin question in item 15.**

---

## 25. A second, older, differently-slugged Forge plugin in the Rack plugins tree — operator housekeeping

Inherited as **Phase 31 deferred item 15**. **Restated; still open, still the operator's call.**

- **Observation:** `~/Library/Application Support/Rack2/plugins-mac-arm64/` contains **two** Forge
  directories — the current `ForgeAudio-AnalogSeries` (slug `ForgeAudio-AnalogSeries`, v2.0.1, two
  modules) and a stale February `ForgeAudio` (slug `ForgeAudio`, v2.0.0, module `ForgeAudioLFO`).
- **It cannot shadow anything** — Rack keys plugins by manifest slug and the slugs differ — but it
  puts a **second Forge LFO in the module browser**, which makes any in-Rack guardrail sign-off's
  subject **inferred rather than pinned**.
- **Not acted on:** deleting a plugin from the operator's own Rack installation is not an
  executor's call.
- **Resolve at: the operator, before plan 32-11's in-Rack session.** The **verification-protocol**
  half is one line and **plan 32-11 owns it**: name the **plugin directory** as well as the module
  when asking for an audition — *"the Analog LFO under Forge Audio Analog Series"*. Phase 36 may
  additionally want to note the stale slug, since `ForgeAudio` is the slug v2.0.0 shipped under.

---

## Phase 32's own measured figures

Repeated here so a reader of this register alone sees what was measured. Full roll-up with
provenance is in `32-10-SUMMARY.md`.

**The alias floor, naive versus corrected, at the five gated large-margin cells** (the
anti-circularity assertion — it compares two measurements and consults **no pinned number**):

| cell | naive dB | corrected dB | improvement | threshold | clears by |
|---|---|---|---|---|---|
| 44.1k C8 · triangle · char 0.00 | −33.8085 | −48.7878 | **+14.979** | −45 | 3.79 |
| 44.1k C8 · saw · char 0.00 | −15.5630 | −25.8423 | **+10.279** | −22 | 3.84 |
| 44.1k C8 · square · char 0.00 | −16.9030 | −31.8772 | **+14.974** | −28 | 3.88 |
| 44.1k C8 · pulse · char 0.00 | −1.2931 | −11.5704 | **+10.277** | −8 | 3.57 |
| 44.1k C9 · saw · char 0.00 | −9.5424 | −19.0075 | **+9.465** | −16 | 3.01 |

**TEST-03:** `failing = 0` over **45 gated cells** at C7, C8 and C9. The naive core failed **32 of
45**; that RED is on record in `32-03-SUMMARY.md` and the gate was inverted **in its own slot**.

**Worst regression anywhere on the 90-cell grid:** **2.3344 dB** (44.1k C9 sine char 0.50). Exactly
**2 of 90** cells regress at all. The phase's rejected design alternatives regress by **−60.4,
−42.7, −36.6 and −29.8 dB**, so the 4.0 dB invariant separates the shipped form from all four by at
least **25.8 dB**.

**Cross-rate:** worst 48 kHz excess **+4.7059 dB**; worst 96 kHz excess **−0.8114 dB** — 96 kHz is
**never** worse than 44.1 kHz, not once, across fifteen same-note triples.

**Output envelope, two nested measured tiers:** `kHostileBoundV` **10.0 V** (no exceptions
anywhere) against a worst measured **7.201301 V**; `kMusicalBoundV` **5.55 V** with the worst
entitled scenario at **5.518030 V** — a margin of **0.032 V**. Audio-rate MORPH reaches
**6.289864 V**, exceeding the musical tier by **0.771832 V**, and the withholding of the tighter
tier is **itself asserted**.

**Reconstruction:** the band-limited core differs from `NaiveVcoCoreMirror` by **exactly** the
`MorphBlep` correction — **0 mismatches over 184,320 samples** at three rates, by direct float `==`,
with the sine-centre zero-correction control at exactly **0**.

**Cross-toolchain (item 8):** worst plateau-cell drift **3.02596 dB**; all **48** step-dominated
cells within **1.0 dB** on all three legs.

**The three-OS matrix:** macOS **94 / 2,622,319**, Ubuntu and Windows **91 / 2,597,737** — a gap of
exactly **3 cases and 24,582 assertions**, unchanged from before the phase, matching the three
`#if defined(__APPLE__)` drift-ON goldens measured locally. Per-leg delta across the phase:
**+13 cases and +4,266 assertions on every leg, identically.**

---

*Phase: 32-morph-aware-anti-aliasing-polyblep-polyblamp*
*Register written: 2026-08-01 (plan 32-10, the phase gate). 25 items — 8 falsified premises
corrected, 17 deferred with owners.*
*Item 8 is a discovery of the phase gate itself: the CI observation this plan exists to perform is
what found it, and it could not have been found locally.*
