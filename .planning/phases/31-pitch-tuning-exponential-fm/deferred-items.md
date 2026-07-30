# Phase 31 — Deferred Items

Out-of-scope discoveries logged during execution, plus the status of every item this
phase inherited. Not fixed by the plan that found them; each names the plan or phase
that should resolve it — **except item 1, which is deliberately pointed at no phase.**

Same format as `.planning/phases/30-vcocore-skeleton-module-registration/deferred-items.md`
(found during · observation · why it is worth a note · why it is not fixed here · resolve at).

---

## 1. The SHIPPED Analog LFO carries the identical latent undefined behavior this phase hardened the VCO against (D-24)

- **Found during:** plan **31-03**, Task 1 — the one-shot undefined-behavior sanitizer probe
  that produced this phase's D-14 RED evidence. **MEASURED, not suspected.**
- **Observation** — both call sites read directly this session, not paraphrased from a
  research document:
  - **`src/AnalogLFO.cpp:320`** — `in.fmCV = in.fmConnected ? inputs[FM_INPUT].getVoltage() : 0.f;`
    The shipped module's shell reads a raw cable voltage off its FM jack. Rack does **not**
    sanitize cable voltages, and there is no finiteness check on this path.
  - **`src/dsp/LfoCore.hpp:183-184`** — `float depthScale = isClocked ? 0.5f : 0.6f;` then
    `float fmPitch = in.fmCV * in.fmAtten * depthScale;`. That value is handed, unbounded,
    to `exp2_taylor5` on the very next line (**`src/dsp/LfoCore.hpp:185`**,
    `freq *= exp2_taylor5(fmPitch);`).
  - **`src/dsp/RackCompat.hpp:106`** — `int32_t xi = (int32_t)x;` (float-cast-overflow) and
    **`:109`** — `yii = xi << 23;` (left-shift overflow). These are the two frozen lines the
    sanitizer named by file, line **and column**, and they are the same two lines this phase
    guarded the VCO against with `forge::kVcoMaxPitchVolts`.
  - Scaling by `0.5f`/`0.6f` does **not** rescue it: a NaN stays a NaN, an infinity stays an
    infinity, and a very large finite magnitude stays out of `int32_t` range.
  - The LFO's `freq = std::fmax(freq, 0.001f)` at `src/dsp/LfoCore.hpp:186` sanitises the
    **result**, exactly as the VCO's negated frequency floor did — which is precisely why a
    behavioral case cannot see this defect. See "why it is worth a note".
- **Evidence of record:** `31-03-SUMMARY.md` § "Task 1 — the RED transcript, verbatim" carries
  the sanitizer's literal diagnostic text, and § "Task 3 — the GREEN transcript" carries the
  clean re-run. Compiler: `Apple clang 16.0.0`, `-fsanitize=undefined`, over a self-contained
  translation unit that lived **outside** the working tree. Nothing about the probe became
  permanent.
- **Why it is worth a note:** a hostile or non-finite cable voltage into the **live,
  library-published** module's FM jack reaches undefined behavior today. And the reason it has
  never been reported is the same reason this phase had to escalate its own evidence tier: the
  frequency floor downstream catches the *garbage result*, so every behavioral assertion a
  reasonable person would write is **already green**. The defect is invisible to the test suite
  by construction. An unrecorded finding of that shape is rediscovered cold.
- **Not fixed here — UNFIXED BY DECISION.** Fixing it means editing `src/dsp/LfoCore.hpp`, a
  header that is **byte-pinned** by `tests/check_frozen.sh` and consumed by a module that is
  live in the VCV Library behind six bit-exact `.f32` goldens. That is a **GUARDRAIL EVENT**
  requiring **operator sign-off and golden re-verification**, not a VCO fix, and it is
  explicitly outside Phase 31's boundary (D-24). This phase's VCO guard was deliberately made
  **local to `forge::VcoCore`** for exactly this reason.
- **Resolve at: NO PHASE.** Deliberately unowned. Whoever picks this up is opening a guardrail
  event and must open it as one — with the impact, the remediation options and a recommendation
  surfaced to the operator before any edit, per the milestone guardrail.
- **THE DIRECT CONSEQUENCE, which is the reason this entry exists at all:** a **permanent
  repository-wide undefined-behavior sanitizer gate cannot be adopted while this stands** —
  no `-fsanitize=undefined` target in the `Makefile`, no sanitizer step in
  `.github/workflows/`, no sanitized leg in the guard suite. It would turn the **shipped**
  module red on the first hostile-input probe and convert a later phase into an unplanned
  guardrail event. That is why D-22's sanitizer use was a **scoped one-shot probe** and why
  this phase's gate asserts the *absence* of sanitizer wiring rather than its presence.
  The constraint is written in two more places so it survives this file being missed:
  `src/dsp/VcoCore.hpp`'s `kVcoMaxPitchVolts` rationale, and
  `tests/test_vco_pitch.cpp`'s invariant-9 banner ("do not add `-fsanitize=undefined` to the
  build or to CI on the strength of this case").

---

## 2. `forge::clamp` is NaN-transparent — **HALF closed by this phase**

Inherited as **Phase 30 deferred item 3 / CR-02**, pointed at *"Phase 31 or Phase 34, in the
same plan that adds the MORPH/CHARACTER CV inputs."* Phase 31 is not that plan — it adds no
MORPH/CHARACTER CV input — so half of the item is closed and half is not. Recording that split
honestly is the point; marking it resolved would be a false green.

- **The half this phase HONOURED.** The item's binding constraint was *"the fix is a NaN-safe
  helper **local to `VcoCore`**, bit-identical to the shared primitive for finite inputs"* and
  *"editing that shared primitive is a guardrail event, not a VCO fix."* Plan **31-03**'s new
  pitch-volt bound obeys both:
  - it is **local to `src/dsp/VcoCore.hpp`** — no frozen header is in this phase's diff at all;
  - it uses the **negated-comparison idiom with the negated line FIRST**
    (`if (!(pitchVolts > -kVcoMaxPitchVolts)) ...` before `if (pitchVolts > kVcoMaxPitchVolts) ...`),
    so a NaN lands on the fallback branch instead of passing through;
  - `forge::clamp` is **rejected by name, in the source**, with the reason — both of its
    comparisons are false for a NaN, so it returns the NaN unchanged;
  - and the behavior is **externally observable**: invariant 9 rows 2, 13 and 24 of
    `tests/test_vco_pitch.cpp` drive a quiet NaN down the pitch route, the FM route and the
    attenuverter, and all three land on the **negative plateau** at `tel.freqHz = 1.418275276e-17`.
    That is a live tripwire on the one substitution the item warns about: swap the pair for
    `forge::clamp` and those three rows stop landing there.
  - The bound is **bit-identical to nothing it replaced** — there was no clamp on that path
    before Phase 31, so no finite-input behavior changed.
- **The half that REMAINS OPEN.** `src/dsp/VcoCore.hpp`'s **morph and character clamps** still
  call the NaN-transparent shared helper and are still **inert against that input class**. This
  phase did not touch them, and deliberately: their inputs are param values, which Rack
  sanitizes in `ParamQuantity::setValue` before they are read, so they are **unreachable by a
  cable** until MORPH/CHARACTER CV inputs land.
- **Not fixed here, deliberately.** No requirement in this phase asks for MORPH/CHARACTER CV,
  and hardening a path no cable can reach would be scope creep with no observable red behind it
  — the vacuous-coverage shape this whole phase exists to refuse.
- **Resolve at:** **the phase that adds the MORPH and CHARACTER control-voltage inputs**
  (currently **Phase 34**, CHAR-01 / DRIFT-01..03). **The original constraint carries over
  unchanged: any fix stays LOCAL to `VcoCore`, and must never be an edit to
  `forge::clamp` in the frozen shared header.** The negated-comparison idiom this phase
  landed for the pitch volts is the worked example to copy.

---

## 3. `IN-05` — the hostile **timing** grid does not cover `±inf`, subnormal, or very-large-finite values

Inherited as **Phase 30 deferred item 6**. **Still pointed at its own phase; this phase
deliberately did not extend it.**

- **Observation (restated):** scenario four of `tests/test_vco_core.cpp` drives `sampleRate`
  over `{-44100, 0, 44100, NaN}` and `sampleTime` over `{-1/44100, 0, 1/44100, 1/1000, 999,
  NaN}`. Neither grid includes `+inf`, `-inf`, a subnormal, or a very-large-finite value.
- **Not extended here, by decision (D-15).** The operator scoped this phase's hostile-input
  work to the **pitch-volt** clamp. **This phase's hostile coverage is a different input
  class:** invariant 9 of `tests/test_vco_pitch.cpp` drives 26 hostile rows over the **pitch
  and FM fields** — `pitchCV`, `fmVolts`, `fmAtten` — at **deliberately legitimate timing at
  every single point**, and a comment in the case restates the deferral so the two are not
  confused.
- **Verified untouched by this phase's gate:** the `HOSTILE_RATES[]` and `HOSTILE_TIMES[]`
  arrays are **byte-identical** to the pre-phase commit, and the whole-phase diff contains
  **zero** hits on either identifier.
- **Resolve at:** the **next phase that extends scenario four** — most likely **Phase 32**,
  whose oversampled inner loop is the named future caller that decouples `sampleTime` from
  `sampleRate` on purpose, and is therefore the first real source of exotic timing.

---

## 4. `tests/check_includes.sh [2/7]`'s unanchored exemption filter (WR-05) — **RESOLVED in this phase**

- **RESOLVED — 2026-07-30, plan 31-01 Task 2.** Both halves of the prescribed fix landed: the
  `[2/7]` exclusion is now anchored to a whole `grep -n` output line, and `[6/7]` gained a
  fifth negative-control fixture (`VcoNcRackSdkCommentEvasionProbe.hpp`, `nc5_hits`) carrying
  the reviewer's exact evasion shape, which must produce a HIT on every invocation.
- **Cross-reference:** the full resolution record is appended to
  **`.planning/phases/30-vcocore-skeleton-module-registration/deferred-items.md` item 5**,
  where the item was originally filed. It is not duplicated here.
- **Why THIS phase owned it, rather than re-deferring:** the item's own resolution clause named
  *"the next phase that touches `tests/check_includes.sh`"*, and decision **D-23** made that
  condition true by requiring an explicit, reviewable `VCO_SIDE_ALLOW` entry for
  `tests/test_vco_pitch.cpp`. Restating the deferral with a new owner would have left it
  pointed at a condition that was **already satisfied** — the same false-comment class plan
  30-08 existed to remove.
- **Confirmed by this phase's gate:** `make guards` reports the `nc5` control firing on every
  run, and `tests/check_includes.sh` is the only guard script in this phase's diff.
- **The four Phase-30 `Info` findings recorded alongside that item are still only RECORDED,
  not planned.** `30-REVIEW.md` remains their record. Nothing in Phase 31 promoted them.

---

## 5. Per-instance seed entropy and patch persistence in the shell (WR-02)

Inherited as **Phase 30 deferred item 2**. **Restated unchanged.**

- **Observation:** `src/AnalogVCO.cpp` hardcodes both the drift seed and the spread seed, so
  every live VCO instance in a patch is a **bit-identical clone** — measured at 0 of 2048
  differing samples. The `forge::VcoCore`-level divergence invariants drive two
  *differently-seeded* cores, which the shell never constructs.
- **Not addressed here, deliberately.** No Phase 31 requirement asks for shell entropy, and
  this phase touched the shell only to declare and forward four pitch/FM controls (31-04).
- **Resolve at:** **Phase 34/35**, which own analog-engine wiring and patch state. The pattern
  already exists in the shipped LFO — draw from `std::random_device`, reject a `(0,0)` pair,
  persist the drawn spread seed, restore through a non-throwing hex parser.
  **ANY implementation MUST re-validate a deserialised value the same way:** a `(0,0)`
  Xoroshiro pair is a fixed point emitting an all-zero stream, so the rejection loop inside
  `std::normal_distribution` never terminates. **In Rack that is a hang while opening a patch,
  not a failing test.**

---

## 6. A COARSE octave (or semitone) snap

- **Found during:** phase context gathering; carried through planning as an explicit
  `<deferred>` entry.
- **Observation:** a right-click menu toggle snapping COARSE to whole octaves is genuinely
  useful on a VCO for exact octave stacking, and the shipped LFO's 15-ratio snap already proved
  the pattern.
- **Why it is worth a note:** it is a real ergonomic gap, not a whim — and the reasoning for
  declining it now is worth preserving so the next reader does not re-argue it.
- **Not built here, deliberately.** **PITCH-02 says "continuously" and this phase honoured that
  literally** — invariant 4 of `tests/test_vco_pitch.cpp` *measures* continuity, with four
  non-integer coarse values that would fail by four orders of magnitude under an octave snap,
  and `-2.37` specifically excluding a semitone snap. A snap is a **new capability**: it needs
  a menu item, a persisted flag, and patch serialization the VCO does not have.
- **⚠ Do not misread the out-of-scope list.** `.planning/PROJECT.md` § Out of Scope lists
  *"Octave snap / semitone selector"* — but **that entry was written about the shipped LFO**,
  where sub-audio rates made it meaningless. **It is not a ruling on the VCO.**
- **Resolve at:** **Phase 35** (which owns the panel and is the first phase where the VCO
  plausibly gains patch state) or a **v2.1** increment.

---

## 7. Amplitude fade near the Nyquist ceiling — considered and REJECTED for this phase

- **Found during:** the D-10 Nyquist-policy decision, and re-confirmed by this phase's
  measurements.
- **Observation:** an alternative to D-10's hard clamp is to fade the amplitude above a
  threshold instead of pinning the frequency.
- **Rejected for this phase because it introduces a gain stage that collides with Phase 34's
  output stage** (OUT-01..03). Two gain stages settled independently, in different phases,
  against different criteria, is the kind of decision that is cheap to make and expensive to
  unpick.
- **What this phase MEASURED about the alternative's premise:** at the ceiling the peak is the
  **full 5.000 V**, with `blockMin = -5` and `blockMax = +5` exactly, and the crossing counts
  are in the thousands (5457 / 5939 / 11879). So "peaks flatten out" is a flattening of the
  waveform **shape**, not a loss of level — nothing at the ceiling is quiet, let alone silent.
  That is the decided sound, on record with numbers.
- **Resolve at:** **Phase 34**, and only if wanted. If the flattened-peak sound proves harsh
  under deep FM during that phase's in-Rack audition, **that phase owns the output stage** and
  could revisit it there, where the gain-stage collision does not exist.

---

## 8. Param display precision — a deliberate, recorded divergence rather than an open item

- **Found during:** plan **31-04**, declaring the COARSE and FINE controls.
- **Observation:** D-04 illustrates the tooltip readout as `+2.00 oct` / `-14.0 cents`. Rack's
  default `displayPrecision = 5` actually yields `+2.0000 oct` / `-14.000 cents`.
- **Why it is not a defect:** **the UNITS are what D-04 fixed; the digit count is not.** This
  phase left the default and thereby **matches the shipped LFO, which sets no precision
  anywhere** — so the two modules read consistently.
- **Not changed here, deliberately.** The divergence is recorded in `src/AnalogVCO.cpp` beside
  the FINE declaration rather than silently absorbed, so a reader of the source finds it.
- **Resolve at:** nobody, unless exactness ever matters. If it does, it is **one line per
  parameter** on the `ParamQuantity*` the `configParam` declaration returns.

---

## 9. The compile canary's field-count margin has NARROWED to a single field — a note for the next phase that adds a POD field

- **Found during:** plan **31-04**, and re-measured by this phase's gate.
- **Observation:** `forge::VcoInputs` carries **eight** floating-point DSP fields — `pitchCV`,
  `coarse`, `fine`, `fmVolts`, `fmAtten`, `morph`, `character`, `drift`. Before this phase the
  shell fed three of them. **After 31-04 the shell feeds seven**, plus `fmConnected` and the
  two injected timing fields. **`drift` is the only DSP field the shell does not feed** — so
  `src/vco_compile_canary.cpp`'s remaining *unique* field coverage is exactly one field.
  `tests/check_canary.sh [2b/5]` still reports *"all 8 VcoInputs DSP field(s) stay runtime-live
  through step() at -O3"*, and it is the canary that keeps the eighth in that count.
- **Why it is worth a note:** **the canary is still not redundant**, for the structural reason
  written into `src/AnalogVCO.cpp`'s own comment — it is the translation unit the guard
  compiles against a *perturbed* `VcoCore` header, and the only VCO TU that is link-checkable
  without the Rack SDK. The field count was always corroboration, not the argument. But the
  corroboration is now thin.
- **Not acted on here.** There is nothing to fix; the margin narrowed because the shell
  correctly grew.
- **Resolve at:** **the next phase that wires the last field or adds a new one — currently
  Phase 34**, which owns `drift`. **That phase must keep the canary feeding EVERY `VcoInputs`
  field a runtime-derived value**, or the constant-fold guard silently loses its margin and
  `[2b/5]` stops proving anything the shell does not already prove.

---

## 10. `tests/test_vco_core.cpp`'s `coreBase()` carries two comments that plan 31-03 falsified

- **Found during:** plan **31-07**, Task 3 — handed forward explicitly, not dropped.
- **Observation:** `tests/test_vco_core.cpp:106-107` read:

  ```cpp
  in.coarse    = 0.f;   // Phase 31 — unread by this step() body
  in.fine      = 0.f;   // Phase 31 — unread
  ```

  Plan **31-03** made the real core **read both fields** (`pitchVolts = in.pitchCV + in.coarse
  + in.fine * (1.f / 12.f);`), so **both annotations are now FALSE.** Independently confirmed
  by `tests/check_canary.sh [2b/5]`, which reports all eight DSP fields runtime-live at `-O3`.
- **⚠ The THIRD annotation is still TRUE and must be left alone:**
  `tests/test_vco_core.cpp:110` — `in.drift = 0.f;   // Phase 34 — unread`. `drift` genuinely
  is unread by today's `step()` body. **Do not "fix" all three as a set.**
- **Why it is worth a note:** this is the same false-comment class plan **30-08** existed to
  remove, and that Phase 31 has already corrected four times elsewhere (31-02's banner twice,
  31-03's adjacency claim, 31-06's banner, 31-07's banner twice). It is a two-line edit with
  **no behavioral consequence** — nothing asserts against these comments — which is exactly why
  it would otherwise be rediscovered cold by someone trusting them.
- **Not fixed here, deliberately, twice over.** 31-07's Task 3 action **explicitly forbade
  touching "its base-input helper"** and its acceptance criteria required the diff to stay
  confined to the stand-in struct and its banner. And **this** plan's prohibitions forbid
  editing anything under `tests/` at all. Violating a stated prohibition to correct a comment
  nothing asserts against would be the wrong trade in both plans.
- **Resolve at:** the **next phase that edits `tests/test_vco_core.cpp`** for any other reason
  — most likely **Phase 32** (which extends scenario four per item 3) or **Phase 34** (which
  wires `drift` and will have to retire line 110's annotation anyway). Fold it into that
  commit; it does not deserve one of its own.

---

## 11. PITCH-04's `sync` clause names a control that does not exist yet — Phase 33 must RE-CONFIRM, not inherit

- **Found during:** plan **31-07**, at the `requirements mark-complete` step for PITCH-04.
- **Observation:** PITCH-04 reads *"Frequency is clamped just below Nyquist so extreme
  **pitch/FM/sync** never aliases via out-of-range frequency."* **Sync does not exist.**
  `forge::VcoInputs` carries no sync field; Phase 33 (SYNC-01/02) adds it. So the requirement
  is marked complete on evidence covering **two of the three** input classes it names:
  extreme pitch (invariants 8 and 9) and extreme FM (invariant 9's FM-routed rows).
- **Why it is worth a note:** the clamp sits **downstream of the frequency**, so a sync-driven
  pitch source cannot bypass it *structurally* — and that structural argument is exactly the
  kind of forward claim this phase has repeatedly declined to make on another phase's behalf.
  A green mark for an input class that was **unreachable when the assertion was written** is
  not coverage of that class.
- **Not resolvable here.** The input does not exist to drive.
- **Resolve at:** **Phase 33 (hard sync).** That phase **must re-confirm the clamp still
  binds** — by adding its sync inputs to invariant 9's hostile grid in
  `tests/test_vco_pitch.cpp`, or to an equivalent case of its own — rather than inheriting this
  phase's green. The same obligation is recorded in `31-07-SUMMARY.md` § Next Phase Readiness
  and in PITCH-04's completion reasoning, so it survives this file being missed.

---

## 12. `plugin.json` still declares version `2.0.1` while shipping a second module (WR-04)

Inherited as **Phase 30 deferred item 4**. **Restated unchanged; still open, still Phase 36's.**

- **Observation:** `v2.0.1` is tagged and live in the VCV Library with a **single** module; the
  manifest now declares **two** modules under that same version string. The library builds from
  git tags, so two artifacts carrying one version string describe different plugins.
- **Not fixed here, deliberately.** This is **D-04**, an explicit Phase 30 decision to hold the
  version — recorded so a stale version is distinguishable from a forgotten one. Phase 31 added
  no module and changed no manifest field.
- **Resolve at:** **Phase 36** (REL-01), which owns the tag, the changelog entry and the #929
  update comment — and which **must re-observe the CI link leg on whatever commit IT tags**,
  per the standing no-tag-on-local-evidence rule.

---

## 13. The documentation gate is still not wired into CI

Inherited as a **reviewed todo, not folded**. **Restated unchanged.**

- **Observation:** `tests/check_docs.sh` is a real guard script that **nothing invokes**.
  `.planning/todos/pending/wire-check-docs-into-ci.md` is the record.
- **Not folded into this phase.** It matched Phase 31 at score 0.6 on generic keywords only
  (*phase*, *correct*, *gate*). It is a one-line CI step for a Phase 27 documentation gate and
  has nothing to do with the pitch chain; folding it here would be the scope creep Phase 29
  explicitly declined.
- **It is not silently invisible.** `tests/check_includes.sh [7/7]` carries it in
  `GUARD_WIRING_EXEMPT` with a written reason, and reports it as **`EXEMPT` on every single
  `make guards` run** — verified again by this phase's gate.
- **Resolve at:** **Phase 36**, which owns CI and the release.

---

## Phase 31's own measured figures

Repeated here so a reader of this register alone can see what was measured without opening five
summaries. **Every figure below was read out of an actual run's output**, and **neither of the
two contradictory prior-milestone research figures for the polynomial's error is cited anywhere
in the delivered code or tests** (D-18). Full roll-up with provenance is in `31-08-SUMMARY.md`.

**V/OCT tracking — worst absolute cents, per tier per rate, with the volt where it occurred:**

| Tier | 44.1 kHz | 48 kHz | 96 kHz |
|---|---|---|---|
| **PRIMARY** (measured on the returned samples) | **0.00967639** @ +5.5 V | **0.00870829** @ +6.0 V | **0.00239614** @ +7.0 V |
| **SECONDARY** (reads telemetry — the weaker tier) | **0.0013924** @ +6.20392 V | **0.00123964** @ +6.32617 V | **0.00123964** @ +7.32617 V |

The fixed **0.05-cent** tolerance is **5.17×** above the worst measurement anywhere and **20×**
under PITCH-01's one cent — so the worst point measured is **103× inside the requirement**.

**COARSE and FINE — worst absolute tracking cents per rate:**

| Control | 44.1 kHz | 48 kHz | 96 kHz |
|---|---|---|---|
| COARSE (19 rows, −5..+5 oct) | **0.004658187** @ +5.0 | **0.0033790525** @ −2.37 | **0.00337679175** @ −2.37 |
| FINE (7 values × 2 V/OCT) | **0.00628057135** | **0.006281158** | **0.00633285261** |

**FINE's twelve measured hundred-cent shifts** — every one inside **0.007 cents** of a hundred:
`+100.003243 / −100.002971` and `+99.9940002 / −99.9938641` (44.1 kHz);
`+100.003244 / −100.002972` and `+99.9941645 / −99.9937555` (48 kHz);
`+100.003237 / −100.002974` and `+99.9940819 / −99.9937695` (96 kHz).

**The Nyquist clamp, observed FIRING (PITCH-04 / D-10):** at nine above-ceiling points
`tel.freqHz` equals the guard fraction times the float rate **EXACTLY** — 21829.5 / 23760.0 /
47520.0 Hz — with **5457 / 5939 / 11879** rising crossings and a **full 5.000 V** peak
(`blockMin = −5`, `blockMax = +5`). Three below-ceiling controls read **strictly under**
(10914.73438 / 11879.96484 / 23759.92969 Hz) and are still the right note
(−0.00242652678 / −0.00507139295 / −0.00507139295 cents).

**The reachable-envelope margin (T-31-24), built from four declared control ranges:**
`12 + 5 + 0.083333333333333329 + 12 = ` **29.083333333333332 V** against a bound of **64 V** —
ratio **2.2005730659025788**, clearing the required factor of two by ten percent.

**The exponential-FM identity and its negative control (FM-03):** 0 mismatching samples on all
eight grid rows at three rates for the real core; the multiplicative stand-in fails the same
identity on **1809 + 1817 + 1717 = 5343** samples. **Four grid rows measure exactly ZERO and are
marked BLIND** — the identity cannot distinguish summing from multiplying whenever **either**
pitch term is a whole number of volts, and **zero is whole**, so the obvious test (V/OCT left at
its default, sweep the FM jack) is vacuous at every FM voltage.

**The hostile grid's three outcomes (D-14 / D-22), 26 rows × 3 rates × 4000 steps:** the control
reads **261.6256104**; the positive plateau reads the rate's ceiling exactly; the negative
plateau reads **1.418275276e-17** at all three rates with the **largest** peak magnitude of the
three — a stalled oscillator is not a quiet one. `firstBadStep = −1` on all 78 configurations.

**The two sanitizer transcripts (item 1's evidence):** RED — `RackCompat.hpp:106:24`
float-cast-overflow and `:109:11` left-shift overflow, quoted verbatim in `31-03-SUMMARY.md`.
GREEN — **zero `runtime error:` lines, 0-byte stderr** over an extended 24-configuration grid.

---

*Phase: 31-pitch-tuning-exponential-fm*
*Register written: 2026-07-30 (plan 31-08, the phase gate)*
