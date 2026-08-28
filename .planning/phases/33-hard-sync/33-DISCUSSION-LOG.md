# Phase 33: Hard Sync - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-08-28
**Phase:** 33-hard-sync
**Areas discussed:** Sync instant & input surface, Reset jump & correction ordering, Edge cases & the SC-3 gate, Perceptual evidence (A/B harness)

**Gray areas offered:** all four above. The operator selected all four.

---

## Sync instant & input surface

### Q1 — How is the sub-sample sync instant derived?

| Option | Description | Selected |
|--------|-------------|----------|
| Interpolate to the high threshold | Store the previous sync voltage in `VcoCore`; on a rising edge solve `f = (high − prev) / (now − prev)`. One new float member, one divide on sync samples only. Standard technique; recovers the master's wrap fraction to first order. | ✓ |
| Interpolate to 0 V | Same interpolation, solved for the zero crossing instead — decouples placement from the hysteresis band. But a master pulse that never goes negative has no zero crossing, and guarding that is a new failure mode. | |
| No interpolation (`xAhead = 0`) | Sample-quantized sync. Cheapest, no new state, no divide. Contradicts SYNC-02's "sub-sample fractional placement" and is the exact failure `research/STACK.md:149` warns about. | |

**User's choice:** Interpolate to the high threshold.
**Notes:** The reset phase is the fractional overshoot `(1 − f) · deltaPhase`; never snapped to exactly 0 (inherited constraint, not re-litigated). → CONTEXT D-01.

### Q2 — Where does edge detection live, and what crosses the POD boundary?

| Option | Description | Selected |
|--------|-------------|----------|
| Raw volts into the core | `VcoInputs` gains `syncVolts` + `syncConnected`; `VcoCore` owns the `SchmittTrigger`, previous-sample store and interpolation. Honours Ph31 D-17; lets the headless driver reach the whole sync path. Costs two POD fields, re-opening the canary's runtime-live obligation. | ✓ |
| Trigger in the shell | Shell runs the trigger and interpolation; POD carries `syncTrigger` + `syncFrac`. Same field count, but puts DSP in the file whose banner forbids it, and no test could ever see a mis-detected edge — the register item 24 vacuity shape. | |
| Raw volts, one field only | `syncVolts` alone; unpatched reads 0 V and never triggers. One less field, but makes "unpatched" and "patched but idle" indistinguishable and gives the VCO two conventions for one question (`fmConnected` already exists). | |

**User's choice:** Raw volts into the core.
**Notes:** → CONTEXT D-02. The compile-canary obligation (register item 15) is called out as an explicit plan task rather than a gate-time discovery.

### Q3 — What Schmitt threshold pair?

| Option | Description | Selected |
|--------|-------------|----------|
| 0.1 / 1.0 V — LFO convention | Verbatim what the shipped LFO uses at both trigger sites (`LfoCore.hpp:137`, `ClockTracker.hpp:111`) and what `research/STACK.md:64` recommends. One convention across the plugin. | ✓ |
| 0.0 / 1.0 V — RackCompat defaults | The struct's own defaults, so no magic numbers at the call site. No in-house precedent, and re-arms more readily on a noisy master idling near 0 V. | |
| Route to research | A sync input sees audio-rate masters, and reference modules differ (Fundamental VCO's sync behaves closer to a zero-crossing detect). Let the researcher survey Befaco / Fundamental / Bogaudio first. | |

**User's choice:** 0.1 / 1.0 V — LFO convention.
**Notes:** Both LFO call sites were read and confirmed during the discussion. → CONTEXT D-03.

### Q4 — How wide should Task 1's MorphBlep hardening be?

| Option | Description | Selected |
|--------|-------------|----------|
| Minimum fix + guard `addStep`'s `jump` | The register's prescription (lower clamp on `segment`; negated-comparison entry guard on non-finite `morph`/`character`) plus a finiteness guard on `jump`. Phase 33 makes `addStep` a live call site fed by a computed `morphedWave` difference, so a non-finite jump becomes reachable now and poisons the instance permanently. | ✓ |
| Minimum fix, exactly as worded | Smallest diff to the header Phase 32 spent its iteration budget stabilising. Leaves the `jump` hole latent. | |
| Minimum fix + full banner audit | Also reconcile every caller-independence claim in the banner against what `step()` enforces. Most honest, but turns a scheduled prerequisite into an open-ended audit ahead of the phase's real work. | |

**User's choice:** Minimum fix + guard `addStep`'s `jump`.
**Notes:** The unguarded `jump` was found during this discussion by reading `MorphBlep.hpp:257-262` — it is **not** in the Phase 32 register. It is the identical permanent-poison mode plan 32-05 measured for a `+infinity` `dt`. → CONTEXT D-04. The rejected full audit is preserved as a deferred item.

---

## Reset jump & correction ordering

### Q1 — How is `jump = value(postReset) − value(preReset)` obtained?

| Option | Description | Selected |
|--------|-------------|----------|
| One extra `morphedWave` call | Pre-reset value reuses the naive sample already computed this step; one additional call at the post-reset phase. Cost paid only on sync samples, and the bleed normalization is satisfied by construction. | ✓ |
| Probe both sides | Two extra calls. Self-consistent, but recomputes a value already in hand at the phase's most expensive function. | |
| Analytic approximation | Derive from the site geometry the way Ph32 D-01 does. Keeps the no-probing discipline, costs no transcendentals — but the reset lands at an arbitrary phase, so this approximates a whole waveform rather than a known edge. | |

**User's choice:** One extra `morphedWave` call.
**Notes:** Recorded explicitly as a **reasoned departure** from Phase 32's "compute it, don't measure it" through-line — there is nothing to compute here. → CONTEXT D-05.

### Q2 — Where is the correction placed, given the edge is detected one sample late?

| Option | Description | Selected |
|--------|-------------|----------|
| Measure all three, then pin | Route the placement convention to `gsd-phase-researcher` with hard constraints (no output delay buffer per D-13; `addStep`'s `[0,1]` contract; never snap to 0) and measure all three against a sync alias floor on the Phase 32 apparatus. | ✓ |
| `addStep(f, jump)`, accept the shift | Uses the pinned seam verbatim, zero header change, zero latency; correction pair has the right shape but lands one sample late. Cost unmeasured, and a one-sample error fails rate-dependently. | |
| Add a past-edge entry point | The mathematically correct zero-latency answer — current sample takes the "after" residual `−f²/2·jump`, sample *n−1*'s share explicitly forfeited. Contradicts register item 13's "no header change needed". | |

**User's choice:** Measure all three, then pin.
**Notes:** The underlying tension was surfaced during this discussion, not inherited: `addStep`'s contract is future-facing (`xAhead ∈ [0,1]`) while a Schmitt-detected edge is always in the past. Register item 13's "no header change is needed to plug sync in" holds only under some resolutions. This is the phase's research flag and risk concentrator. → CONTEXT D-06.

### Q3 — Which phase do the nine free-run sites see in a sync sample?

| Option | Description | Selected |
|--------|-------------|----------|
| Post-reset, sites jumped over suppressed | Advance and wrap, overwrite `phase` with the fractional overshoot, evaluate naive and the nine sites there. Sites the reset **jumped over** must be suppressed or they double-count against the sync BLEP. `PITFALLS.md:127` names this case. | ✓ |
| Pre-reset this sample, reset next | Nine sites see an unbroken advance, structurally incapable of a phantom crossing. But landing at the right phase next sample needs a skip-the-advance flag or a negative pre-phase — new state and a new off-by-one. | |
| Fold into the placement measurement | Let the same measurement settle both. | |

**User's choice:** Post-reset, sites jumped over suppressed.
**Notes:** The suppression rule is elevated to a named deliverable. → CONTEXT D-07.

### Q4 — Does the sync path get a polyBLAMP?

| Option | Description | Selected |
|--------|-------------|----------|
| BLEP only, escalation recorded | `research/STACK.md:124` is explicit that the BLAMP is optional and the step BLEP is the audible fix for lean v2.0. The BLAMP becomes the documented first escalation, ahead of any kernel-order change. | ✓ |
| BLEP + BLAMP now | More complete, reuses the existing BLAMP kernel. But `addStep` carries a value jump only, and the slope difference has no closed form — a numerical derivative means new transcendental cost and fresh cross-libm divergence risk. | |
| Let the measurement decide | Render with and without and see whether it moves the floor. Widens a research scope that already has three placements to compare. | |

**User's choice:** BLEP only, escalation recorded.
**Notes:** → CONTEXT D-08; the BLAMP escalation is recorded as a deferred item alongside register item 9.

---

## Edge cases & the SC-3 gate

### Q1 — How is SC-3's "≥1 sync event within a single sample" discharged?

| Option | Description | Selected |
|--------|-------------|----------|
| Assert the limit as the behaviour | One-edge-per-sample is the detector's structural ceiling. Assert what actually happens when the master exceeds it: every observable edge fires exactly once, missed edges missed identically at all three rates, output bounded and finite. | ✓ |
| Detect multiple crossings per sample | Would satisfy the literal wording, but the VCO sees a voltage not a phase — it means estimating a period from edge timings, which is the `ClockTracker` machinery and the wrong instrument at audio rate. | |
| Amend the criterion | Edit ROADMAP.md SC-3 before planning, following the Ph32 D-02/D-06 precedent. | |

**User's choice:** Assert the limit as the behaviour.
**Notes:** No pre-planning document edit is required for this phase, unlike Phase 32. Follows the register item 6 precedent (an instrument's limitation recorded as a property, not a defect). → CONTEXT D-09.

### Q2 — What is SC-3's instrument?

| Option | Description | Selected |
|--------|-------------|----------|
| Measured delta bound + anti-circularity | Sweep a sync grid, record worst `|x[n] − x[n−1]|` on reset samples, pin outward with a discriminating mutation probe; pair with an uncorrected-vs-corrected delta that consults no pinned number. | ✓ |
| Analytic excursion bound | Derived from peak-to-peak excursion. Cheap and permanently valid — and close to vacuous, since a full-scale artifact is exactly what a full-excursion bound admits. | |
| Spectral only | Rejected on measured grounds: register item 5 found single-sample full-amplitude spikes measure 0.0 dB spectrally, so the alias-floor gate is structurally blind to the artifact SC-3 names. | |

**User's choice:** Measured delta bound + anti-circularity.
**Notes:** → CONTEXT D-10.

### Q3 — Should sync get its own spectral cells?

| Option | Description | Selected |
|--------|-------------|----------|
| Yes — a sync sub-grid | Sync sweep at several master/slave ratios × five shape centres × character 0 and 1; 44.1 kHz binding, 48/96 kHz regression; bin-centred frequencies per D-10; per-cell thresholds pinned from measurement per D-09. Also the instrument that settles the placement question. | ✓ |
| No — continuity bound only | Keeps the phase narrow and leaves the 90-cell grid untouched, but the placement measurement would need its own throwaway apparatus. | |
| Yes, but measure-only at first | A recording pass with no thresholds, deferring pinning to Phase 36. Avoids pinning Apple-clang figures, but a grid that gates nothing cannot go red. | |

**User's choice:** Yes — a sync sub-grid.
**Notes:** Register item 8's step-dominated / plateau split and its 1.0 / 4.0 dB reproduction bounds apply to any new cells. → CONTEXT D-11.

### Q4 — How is PITCH-04 re-confirmed?

| Option | Description | Selected |
|--------|-------------|----------|
| Sync as a third input class | Extend Phase 31's existing case with extreme pitch × extreme FM × hostile sync; re-tick only where sync is observed firing. Also exercises the phase's new divisor `(high − prev)/(now − prev)`, which divides by zero on equal samples and propagates NaN from a cable voltage. | ✓ |
| A dedicated sync PITCH-04 case | Cleaner provenance, but splits one requirement's evidence across two files. | |
| Fold into the hostile-timing grid | Reuses standing apparatus and catches the divisor, but that grid asserts finiteness and boundedness, not pitch accuracy. | |

**User's choice:** Sync as a third input class.
**Notes:** Closes Phase 31 deferred item 11 / register item 13's obligation. The new-divisor rationale mirrors Ph32 D-15 — which was itself a *corrected* rationale, so it must not be inherited casually. → CONTEXT D-12.

---

## Perceptual evidence (A/B harness)

### Q1 — What legs does the renderer produce?

| Option | Description | Selected |
|--------|-------------|----------|
| BLEP'd vs unBLEP'd reset | Two legs from the same driver in the same pass at the sync sub-grid points. Answers what the UAT actually asks — does it click, does it alias — against a switchable reference rather than a memory. | ✓ |
| Add a cosine-crossfade third leg | Would turn "buzzy, not smeared" into an evidenced three-way comparison, since SYNC-02 names the crossfade as the wrong answer. Costs implementing the wrong design on purpose. | |
| Sync legs plus the Phase 32 morph pair | Also closes item 26's original debt. Broadest value, but re-opens a closed phase's UAT question inside this phase's budget. | |

**User's choice:** BLEP'd vs unBLEP'd reset.
**Notes:** Both rejected options are preserved as deferred ideas with explicit resolve-at conditions. → CONTEXT D-13, D-14.

### Q2 — What form do the pairs take?

| Option | Description | Selected |
|--------|-------------|----------|
| `.wav`, generated, not committed | A `make` target writing to a gitignored directory. Playable without conversion, and staying out of the repo enforces item 26's "must never become a pinned golden" constraint by construction. | ✓ |
| `.f32`, generated, not committed | Matches the existing golden format, no WAV header writer needed — but costs a conversion step for an artifact whose purpose is being listened to. | |
| `.wav`, committed as fixtures | Reproducible without a build, but committed audio drifts toward being treated as a reference — exactly what item 26 forbids. | |

**User's choice:** `.wav`, generated, not committed.
**Notes:** → CONTEXT D-15.

### Q3 — Should the renderer be reusable?

| Option | Description | Selected |
|--------|-------------|----------|
| Reusable, parameterised by leg | Takes a grid and a pair of core configurations, so Phase 34 supplies drift-off/drift-on legs without rebuilding the apparatus mid-checkpoint — which is exactly why item 26 went unfixed in Phase 32. | ✓ |
| Sync-specific, generalise later | Smallest diff, no speculative abstraction. Risk: Phase 34 hits the same budget wall and DRIFT-03 gets decided on memory. | |
| Reusable and note it for 34 | Reusable plus a register entry pointing Phase 34 at it. | |

**User's choice:** Reusable, parameterised by leg.
**Notes:** DRIFT-03's value is explicitly audition-*gated* — it is decided by listening, not calculated. The CONTEXT deferred section points Phase 34 at the renderer regardless. → CONTEXT D-16.

### Q4 — What shape should the in-Rack checkpoint take?

| Option | Description | Selected |
|--------|-------------|----------|
| All four precedents applied | Blocking `.continue-here.md`; expected-results block presented before the reply; explicit refusal to book unevidenced perceptual coverage; plugin **directory** named and a whole-tree `rsync` install flush. | ✓ |
| Blocking checkpoint only | Write the `.continue-here.md` and leave the script's shape to the planner. Less prescription — but all three of the other items have already gone wrong once in this milestone. | |
| Standard plan checkpoint | Simplest, and the exact configuration under which a resume previously walked past an operator UAT in this project. | |

**User's choice:** All four precedents applied.
**Notes:** → CONTEXT D-17.

---

## Claude's Discretion

The operator made an explicit call on every question asked; nothing was returned as "you decide." The following were recorded as discretion in CONTEXT.md because they are implementation detail within a locked decision, not open questions:

- The master/slave ratio set, note grid, block lengths and cycle counts for the sync sub-grid, within the bin-centred integer-cycle and cross-rate constraints.
- The mechanism for withholding the sync BLEP on the A/B renderer's second leg (flag, second entry point, or test-only shim).
- Whether the site-suppression rule is a per-site predicate or a swept-interval test — subject to recompute-never-cache.
- Whether the previous-sync-voltage store is reset, held or invalidated on a sample-rate change, provided the choice is stated and asserted.
- Renderer file naming, output directory and `make` target name.
- Whether the sync sub-grid lands as new cases in `tests/test_vco_spectrum.cpp` or a new TU (a new TU costs an explicit guard-allowlist entry as a plan task).

Two items were recorded **without** being asked, both settled by standing precedent rather than open:

- `res/AnalogVCO.svg` gains one SYNC jack position (Ph30 D-07 — every visible control does something, so in-Rack UAT is honest); replaced wholesale in Phase 35.
- `src/AnalogLFO.cpp` stays absent from this phase's diff, as in Phases 30, 31 and 32.

## Deferred Ideas

Raised or rejected during this discussion and preserved in CONTEXT.md's `<deferred>` section:

- A cosine-crossfade third A/B leg — resolve at the first UAT where the smear verdict is equivocal.
- Applying the A/B renderer to Phase 32's morph pair — closes register item 26's original debt; resolve at any later phase or Phase 36.
- polyBLAMP on the sync path — the documented first escalation if a sync threshold is missed.
- A full `MorphBlep.hpp` banner/contract audit — resolve at any phase adding a third call site.

Inherited from the Phase 32 register and restated unchanged: four-point kernels (operator decision), the narrow-pulse reach refinement, CHARACTER CV → Phase 34, the output stage and drift → Phase 34, per-instance seed entropy → Phase 34/35, the FM attenuverter affordance and COARSE snap → Phase 35, spectral cross-toolchain portability and the `plugin.json` version → Phase 36, the shipped-LFO latent UB → **no phase**, the two `MorphBlep` tripwires (items 22 and 23), and the stale `ForgeAudio` plugin directory.

**No scope creep occurred** — every question stayed inside SYNC-01/02 plus the operator-scheduled Task 1 and the PITCH-04 re-confirmation that register item 13 attaches to this phase.
