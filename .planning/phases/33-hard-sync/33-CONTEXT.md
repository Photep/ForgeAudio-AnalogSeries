# Phase 33: Hard Sync - Context

**Gathered:** 2026-08-28
**Status:** Ready for planning

<domain>
## Phase Boundary

**Hard sync.** A Schmitt-triggered SYNC input that resets the oscillator's phase to a sub-sample fractional overshoot and fires the *already-built* `forge::MorphBlep::addStep` seam so the reset is band-limited rather than clicked — the classic buzzy sync timbre, explicitly **not** the shipped LFO's 3 ms cosine crossfade.

`forge::VcoCore` gains sync ownership: the `SchmittTrigger`, the previous-sync-voltage store, the threshold interpolation, the phase reset, and the seam call. `forge::VcoInputs` gains two fields. `src/AnalogVCO.cpp` gains a SYNC jack that does nothing but read a voltage into the POD.

**Requirements:** SYNC-01, SYNC-02. Plus a re-confirmation of **PITCH-04**, which Phase 31 marked complete on evidence that could not include sync (register item 13 / Phase 31 deferred item 11).

**⚠ TASK 1 IS A SCHEDULED PREREQUISITE, NOT PART OF THE SYNC WORK.** The operator decided on 2026-08-27 that `32-REVIEW.md` CR-01 and CR-02 are fixed **as Phase 33, Task 1, before any `addStep` seam work begins**. See D-04. Phase 33 planning must not begin its seam work until these are closed.

**LFO guardrail status: NOT a guardrail event.** Every file in scope is VCO-only — `src/dsp/MorphBlep.hpp`, `src/dsp/VcoCore.hpp`, `src/AnalogVCO.cpp`, `res/AnalogVCO.svg`, and the test tree. **No frozen shared header is touched**, so `FROZEN.sha256` needs no bump and the six shipped-LFO goldens are unaffected by construction. `src/AnalogLFO.cpp` must remain absent from this phase's diff, as in Phases 30, 31 and 32. If any plan finds itself proposing an edit to `Waveshape.hpp`, `DriftEngine.hpp`, `RackCompat.hpp` or `MathConst.hpp`, that is a guardrail event: stop and surface it to the operator with impact + remediation options + a recommendation before proceeding.

**NOT in this phase (deliberate boundaries):**
- **CHARACTER CV + attenuverter, DRIFT (knob/CV/attenuverter, the moving OU layers), the output stage (DC blocker, soft saturation) → Phase 34** (CHAR-01, DRIFT-01..03, OUT-01..03). CHARACTER stays knob-only; the output stays unconditioned at `×5`; `bleedLfo` stays `0.f`.
- **The 18HP Forge Noir panel, CRT display, patch serialization, per-instance seed entropy → Phase 35** (PANEL-01/02, DISP-01..03). `res/AnalogVCO.svg` stays a throwaway that gains one jack position.
- **VCO goldens, version bump, tag, #929 update → Phase 36** (TEST-05, REL-01). There are still no VCO goldens, so this phase's output change breaks no fixture.
- **Oversampling, minBLEP, through-zero FM, phase distortion → v2.1.** AA-05 forbids the first two outright in v2.0.
- **Re-auditioning Phase 32's morph improvement.** The A/B renderer built here (D-13..D-16) closes register item 26's *mechanism*, but this phase renders sync legs only. Applying it retroactively to the morph pair is available to any later phase at near-zero cost and is deliberately not spent here.

</domain>

<decisions>
## Implementation Decisions

### Task 1 — the scheduled MorphBlep prerequisite

- **D-04: Minimum fix, PLUS a finiteness guard on `addStep`'s `jump` argument.** The register's own prescription is: clamp `segment` from below as well as above (`MorphBlep.hpp:319-320` currently clamps only from above, then writes `W[segment]` into a `float[5]`; a negative `morph` is a reproduced ASan stack-buffer-underflow, and `(int)NaN` measures `0` on this arm64 host but is `INT_MIN` under `cvttss2si` on **the x86 MinGW/Linux builds that actually ship**), and make the entry guard reject a non-finite `morph`/`character` rather than relying on every caller. Mirror the existing **negated-comparison idiom**; **never a clamp ladder** — a clamp has both comparisons false for NaN, which is the entire reason `VcoCore` uses the negated pair (`VcoCore.hpp:392`, T-32-01).
  - **The third item, found during this discussion and NOT in the register:** `addStep` guards `xAhead` with a negated comparison but accepts **any** `jump`. A non-finite `jump` goes straight into `inject`/`pending`, which is the **identical permanent-poison mode** plan 32-05 measured for a `+infinity` `dt` — one bad sample and every later sample returns NaN even after the input recovers. Phase 33 is precisely what makes that path live, because D-05 feeds `jump` a computed difference of two `morphedWave` values. **Guard it in the same commit as CR-01/CR-02.**
  - **RED-first, per the standing discipline.** CR-01's RED is the ASan reproduction; CR-02's is the measured non-finite count (16 of 200); the `jump` guard's is a poisoned-instance trace showing the instance still returning NaN after the hostile input is withdrawn.
  - **Rejected: the minimum fix exactly as worded**, with the `jump` hole left latent — it is reachable for the first time in this very phase.
  - **Rejected: a full banner/contract audit** of every caller-independence claim in `MorphBlep.hpp`. The complaint is legitimate (the advertised contract is not the enforced one), but it converts a scheduled prerequisite into an open-ended audit ahead of the phase's real work. Record it as still-open in the deferred register instead.
  - **This is the one place the phase touches `MorphBlep.hpp` by plan.** Whether the D-06 placement measurement adds a second, additive touch is decided by measurement, not now.

### Sync instant & input surface (SYNC-01)

- **D-01: The sub-sample instant comes from linear interpolation to the SchmittTrigger's HIGH threshold.** `forge::VcoCore` stores the previous sync voltage; on a rising edge it solves `f = (high − prev) / (now − prev)`. One new `float` member, one divide, on sync samples only. This is what *"the master's wrap fraction"* (`research/PITFALLS.md:118`) means in practice — a fast master ramp crosses the threshold within a sample or two of its own wrap, and the interpolation recovers where.
  - **Rejected: interpolating to 0 V.** It would decouple placement from the hysteresis band, but a master pulse that never goes negative has no zero crossing, and the guard for that case is a failure mode the threshold version does not have.
  - **Rejected: no interpolation (`xAhead = 0` always).** Named so the rejection is on record: it contradicts SYNC-02's *"sub-sample fractional placement"* outright and is the exact failure `research/STACK.md:149` warns produces sync that aliases even with a BLEP.
  - **The reset phase is the fractional overshoot: `phase = (1 − f) · deltaPhase`.** **NEVER snap to exactly 0** (`research/STACK.md:124`, `:149` — binding, inherited via register item 13).
- **D-02: Raw volts cross the POD boundary — the core owns the detection.** `forge::VcoInputs` gains `syncVolts` and `syncConnected`; `forge::VcoCore` owns the `SchmittTrigger`, the previous-sample store and the interpolation.
  - **Why the core and not the shell:** edge detection and sub-sample solving are DSP, and Phase 31's D-17 plus `src/AnalogVCO.cpp`'s own banner both say the shell does none. Decisively, it is what makes SYNC-01/02 **assertable headlessly** — `tests/VcoBlockDriver.hpp` can drive real master voltages through the whole sync path. A shell-side trigger would hand the core an already-decided boolean, so no test could ever see a mis-detected edge: the same vacuity shape as register item 24 (MORPH-02's shell mix asserted by no test).
  - **`syncConnected` follows the in-house precedent**: `LfoCore.hpp:136-137` gates `resetTrigger.process` on `in.resetConnected`, and `VcoInputs` already carries `fmConnected`. **Rejected: `syncVolts` alone**, which would make "unpatched" and "patched but idle at 0 V" indistinguishable and give the VCO two conventions for one question.
  - **Two new POD fields re-open the compile canary's obligation (register item 15).** `src/vco_compile_canary.cpp` must feed **every** `VcoInputs` field a runtime-derived value or `check_canary.sh [2b/5]` silently stops proving what the shell does not already prove. **Make that an explicit plan task with its own rationale, not a gate-time discovery** (the Phase 31 D-23 lesson). Note the shell *will* feed both new fields, so the canary's unique-field margin is not narrowed further — `drift` remains the one field only the canary reaches.
  - **`CORE-03` binds:** the trigger state and the previous-voltage store are **per-instance**, never static.
- **D-03: Thresholds are `0.1f, 1.0f`.** Verbatim what the shipped LFO uses at both its trigger sites (`LfoCore.hpp:137`, `ClockTracker.hpp:111`) and what `research/STACK.md:64` recommends. One convention across the plugin. A ±5 V saw master jumps the full band inside one sample at its wrap, which is exactly the case D-01's interpolation resolves.
  - **Rejected: `0.f, 1.f`** (RackCompat's defaults) — no in-house precedent, and a master idling near 0 V with noise re-arms more readily than a 0.1 V floor allows.
  - **Rejected: routing the threshold choice to research.** The in-house convention is unambiguous and the interpolation makes the exact threshold second-order.

### Reset jump & correction ordering (SYNC-02)

- **D-05: `jump` comes from ONE extra `morphedWave` call at the post-reset phase; the pre-reset value reuses the naive sample already computed this step.** Cost is paid only on samples where sync actually fires.
  - **This deliberately departs from Phase 32's "compute it, don't measure it" through-line, for a stated reason: there is nothing to compute.** D-01 (Phase 32) could derive its magnitudes because each morph site is a *known* discontinuity with a closed form. A sync reset lands at an **arbitrary** phase, so the jump is the difference of two arbitrary points on the crossfade. Deriving it would be approximating a whole waveform, not a known edge — a far weaker claim than Phase 32's D-01 makes. **Downstream agents must not "restore" the analytic approach for consistency.**
  - **Phase 32's D-05 bleed normalization is satisfied by CONSTRUCTION, not by re-derivation.** `morphedWave` returns a value that has already been through the `1/(1 + bleedIntensity)` divide (`Waveshape.hpp:212`), so a jump computed as a difference of two of its outputs is automatically in the normalized domain. No separate correction factor.
  - **Rejected: probing both sides** (two extra calls) — it recomputes a value already in hand at the phase's most expensive function (1 `sin`, 2 `exp`, 2 `tanh`, 1 `cos`).
- **D-06: THE PLACEMENT CONVENTION IS MEASURED, NOT ARGUED. Routed to `gsd-phase-researcher`.** This is the phase's risk concentrator and its research flag.
  - **The problem, stated so the researcher does not have to rediscover it.** `addStep`'s contract is **future-facing**: `xAhead ∈ [0,1]`, the current sample receives `jump·(1−xAhead)²/2` and the next receives `jump·(−xAhead²/2)`. But a Schmitt-detected edge is always in the **past** — the crossing happened between sample *n−1* and sample *n*, and *n−1* has already been emitted. Calling `addStep(f, jump)` at the detection sample therefore produces a correction pair of exactly the right *shape*, **one sample late**. Register item 13's claim that *"no header change is needed to plug sync in"* holds only under some of the resolutions below.
  - **The three candidates to measure:** (a) `addStep(f, jump)` at the detection sample, accepting the one-sample shift — zero header change, uses the pinned seam verbatim; (b) an **additive past-edge entry point** on `MorphBlep` that gives the current sample the "after" residual `−f²/2·jump` and explicitly accounts for sample *n−1*'s forfeited share — the mathematically correct zero-latency answer, and contradicts item 13's claim; (c) `xAhead = 0` — coarsest, and note the reset phase stays fractional either way so SYNC-02's sub-sample requirement is met on the *timing* regardless.
  - **Hard constraints on the answer, all three binding:** **no output delay buffer** (Phase 32 D-13 rejected it on two grounds — declared latency, and a VCO that silently delays by a sample desyncs against every other oscillator in the patch); `addStep`'s `[0,1]` contract may be *extended* additively but not reinterpreted; and the reset **never** snaps to exactly 0.
  - **The instrument already exists.** Measure all three against the sync sub-grid (D-10) on the Phase 32 spectral apparatus, and pin by the **D-08/D-09 measure-then-pin protocol**. A one-sample placement error is precisely the class that fails **rate-dependently**, which is why D-11's cross-rate discipline (44.1 kHz binding, 48/96 kHz as regression) applies here too and a single-rate comparison would be worthless.
- **D-07: In a sync sample the nine free-run sites evaluate at the POST-reset phase, and any site the reset JUMPED OVER is suppressed for that sample.** Advance and wrap normally, overwrite `phase` with the fractional overshoot, then evaluate the naive sample and `blep.step` there.
  - **The suppression rule is a named deliverable, not an implementation detail.** A reset **jumps over** sites rather than traversing them. Without suppression, `MorphBlep` places a free-run correction for an edge the waveform never actually crossed, on top of the sync BLEP that is already correcting the real discontinuity — a silent double-count. `research/PITFALLS.md:127` names this case explicitly: *"the case where the reset itself makes the slave cross its own discontinuity in the same step."*
  - **Rejected: staying pre-reset this sample and deferring the reset to the next.** It would make the nine sites structurally incapable of a phantom crossing, which is attractive. But landing at the right phase next sample then needs either a skip-the-advance flag or a negative pre-phase — new state and a new way to be off by one sample, trading a visible rule for an invisible one.
  - **Interaction with D-06:** these are the same ordering question seen twice. The placement measurement must be run against **this** ordering, not a different one.
- **D-08: BLEP only — no polyBLAMP on the sync path.** `research/STACK.md:124` is explicit: *"A BLAMP for the slope change is optional; the step BLEP is the audible fix and is sufficient for lean v2.0."* The step is what buzzes and what clicks.
  - **The BLAMP is the documented FIRST escalation** if a sync alias threshold proves unreachable — recorded alongside register item 9's narrow-pulse "reach" refinement, so a later phase escalates deliberately instead of reaching for kernel order (which register item 10 makes an **operator decision with an impact assessment, never a silent choice**).
  - **Rejected: adding it now.** `addStep` carries a value jump only, so a slope seam is a second header change; and the slope difference across the reset has no closed form, so it would need a numerical derivative of the crossfade at two phases — new transcendental cost and a fresh cross-libm divergence risk (`research/PITFALLS.md:211`).

### Edge cases & the SC-3 gate

- **D-09: SC-3's "≥1 sync event within a single sample" is discharged by ASSERTING THE DETECTOR'S STRUCTURAL CEILING, not by claiming coverage that cannot exist.** A `SchmittTrigger` reading one voltage per sample can observe **at most one rising edge per sample, by construction**. The gate therefore asserts what actually happens when the master exceeds that: every observable edge fires **exactly once**, missed edges are missed **identically at all three sample rates**, and the output stays **bounded and finite** throughout.
  - **This is the register item 6 move applied ahead of time.** There, `estimateFreqRising`'s sub-two-sample blindness was recorded as a property of the instrument rather than a defect to fix, and the high-note claim was moved to the instrument that could actually make it. Here the limit is named **before** a gate is written against it.
  - **Rejected: inferring the master's rate** to fire multiple events per sample. The VCO sees a voltage, not a phase; estimating a period from edge timings is the `ClockTracker` machinery the LFO uses at clock rates and is the wrong instrument at audio rate.
  - **Rejected: amending ROADMAP.md SC-3.** No document edit is needed — the criterion is satisfiable as a statement about *handling*, which is what D-09 asserts. **This phase requires no blocking pre-planning document edit** (unlike Phase 32's D-02/D-06).
- **D-10: SC-3's instrument is a MEASURED per-sample delta bound, paired with a naive-vs-corrected anti-circularity delta.** Sweep a sync grid (master/slave ratio × morph × character × all three rates), record the worst `|x[n] − x[n−1]|` on reset samples, pin **outward** per the D-08/D-09 measure-then-pin protocol, and prove sensitivity with a **discriminating mutation probe** (a probe that fails a stated population exactly, not merely "some assertions").
  - **The anti-circularity half is what makes it evidence.** Assert `uncorrectedResetDelta − correctedResetDelta >= margin` — a comparison of two **measurements** that consults **no pinned number**. This is the Phase 32 move that made TEST-03 evidence rather than a restatement of its own table.
  - **Rejected: an analytic excursion bound** from the waveform's peak-to-peak range. Cheap and permanently valid, and close to vacuous: a full-scale artifact is exactly what a full-excursion bound admits, so the gate would be green on the defect it names.
  - **Rejected: spectral-only.** On measured grounds: register item 5 found that single-sample full-amplitude spikes measure **0.0 dB spectrally** — *"completely invisible to the alias-floor gate"* — so the spectral instrument structurally cannot see the artifact SC-3 is about.
- **D-11: Sync gets its own sub-grid in `tests/test_vco_spectrum.cpp`.** Classic sync sweep at several master/slave ratios × the five shape centres × character **0** and **1**, at **44.1 kHz binding with 48 and 96 kHz as regression** (D-11 of Phase 32), on **bin-centred frequencies with integer cycles per block** (D-10 of Phase 32 — leakage exactly zero, rectangular window exact; **test frequencies sit at bin centres rather than exact equal-tempered notes, and that must not be "fixed"**). Per-cell thresholds pinned from measurement, each carrying its measured justification in the test (D-09 of Phase 32).
  - **It pays for itself twice:** it is also the instrument that settles D-06's placement question.
  - **Register item 8 binds it.** Every absolute decibel figure this milestone has recorded is an **Apple-clang figure**. If any sync cell turns out to be **plateau**-class (no true value step, so the arg-max over non-harmonic bins is decided by near-tied bins and one libm ULP reorders them), it inherits the 4.0 dB reproduction bound rather than the 1.0 dB one — and the classification must be stated on the **physical criterion before the population is enumerated**, never as a rename of the cells that failed. `kThresholdFloorDb = −75 dB` still bounds how tight any threshold can be.
  - **Rejected: the continuity bound alone** — the placement measurement would then need its own throwaway apparatus rather than extending the standing one. **Rejected: a measure-only grid with no thresholds** — a grid that gates nothing cannot go red.
- **D-12: PITCH-04 is re-confirmed with SYNC AS A THIRD INPUT CLASS on Phase 31's existing case** — extreme pitch × extreme FM × hostile sync — and the requirement is re-ticked **only where sync is observed FIRING behind the claim** (the Phase 31 non-vacuity discipline, where four consecutive plans declined to tick a requirement their own frontmatter claimed).
  - **It is also where the phase's NEW DIVISOR is exercised.** `f = (high − prev) / (now − prev)` **divides by zero when the two samples are equal** and **propagates a NaN straight from a cable voltage**. This is the same *"a new division appeared behind an input field"* rationale that moved the hostile-timing grid into Phase 32 under D-15 — and note that the Phase 32 version of that rationale was itself a **correction** of a falsified one, so it must not be inherited casually. Guards use the **negated-comparison idiom**, never `forge::clamp`.
  - **Rejected: a dedicated separate case** (splits one requirement's evidence across two files) and **folding it into scenario four's hostile grid** (that grid asserts finiteness and boundedness, not pitch accuracy — the wrong instrument for PITCH-04's claim, though the grid should still gain sync voltages).

### Perceptual evidence — closing register item 26

- **D-13: The A/B renderer is BUILT IN THIS PHASE.** Register item 26's Resolve-at reads *"Phase 36 (goldens / CI), or whichever phase next needs a perceptual verdict, whichever is sooner."* **Phase 33 is that phase**: *"buzzy, not smeared"* and *"no click per sync"* (`research/PITFALLS.md:129`) are pure listening calls with no automated instrument. Running this phase's UAT on memory would repeat verbatim the defect the operator's own 32-11 reply exposed.
- **D-14: Two legs — the shipped band-limited sync, and the same reset with the sync BLEP withheld.** Rendered from the **same driver in the same pass** at the D-11 sync sub-grid points, so what the operator hears is what the gate measured (item 26's first constraint, binding).
  - **Rejected: a third cosine-crossfade leg.** It would turn *"buzzy, not smeared"* from an assertion into an evidenced three-way comparison — genuinely attractive, since SYNC-02 names the crossfade as the wrong answer by name. Not taken: it means implementing the wrong design on purpose inside the phase that is trying to establish the right one. **Recorded as deferred**, and it is the cheap escalation if the operator's verdict on smear is equivocal.
  - **Rejected: also rendering the Phase 32 morph pair** to close item 26's original debt. The mechanism is what was missing; applying it to the morph cells is near-free for any later phase and re-opens a closed phase's UAT question inside this phase's budget.
- **D-15: `.wav`, generated on demand, NOT committed.** A `make` target the operator runs before the UAT session, writing to a gitignored output directory. Playable in anything without conversion, and staying out of the repo **enforces item 26's second constraint by construction**: a rendered pair is a listening aid and **must never become a pinned golden captured from one toolchain**.
  - **Rejected: `.f32`** (matches `tests/golden/freerun_*.f32`, needs no WAV header writer — but costs the operator a conversion step for an artifact whose entire purpose is being listened to). **Rejected: committed fixtures** — committed audio drifts toward being treated as a reference, which is exactly what item 26 forbids.
- **D-16: Built REUSABLE — parameterised by grid and by a pair of core configurations.** Phase 34's **DRIFT-03 value is explicitly audition-gated**: it will be *decided* on exactly this kind of comparison, not calculated. Building the apparatus narrowly now guarantees Phase 34 hits the same mid-checkpoint budget wall that left item 26 unfixed in Phase 32 — the register says so in as many words (*"plan 32-11 produces no code artefact and had no budget to build a rendering harness mid-checkpoint"*).
  - **The mirror already exists:** `NaiveVcoCoreMirror` in `tests/test_vco_spectrum.cpp` is a **bit-exact** non-band-limited mirror of the live core, proved by the D-08 reconstruction case at **0 mismatches over 184,320 samples** at three rates by direct float `==`. The renderer drives configurations through the same driver, not a second copy of the loop.

### The operator UAT gate

- **D-17: All four hard-won precedents apply to this phase's in-Rack checkpoint.**
  1. **A blocking `.continue-here.md`** written before the UAT plan. A checkpoint-pending `SUMMARY.md` otherwise makes a resumed session walk straight past the operator gate — observed in this project, and the reason plan 32-11 shipped one.
  2. **The full expected-results block is presented BEFORE the operator replies** (the Phase 30 precedent), so an absence of complaint is an absence of complaint rather than an absence of exposure.
  3. **Perceptual coverage the script cannot evidence is REFUSED, not booked** (the 32-11 precedent — *"seems to work well enough"* was recorded as unevidenced rather than allowed to stand in). D-13..D-16 exist so that this time the script *can* evidence it.
  4. **The session protocol names the plugin DIRECTORY as well as the module** — *"the Analog VCO under Forge Audio Analog Series"* — because the operator's Rack tree carries a second, differently-slugged `ForgeAudio` plugin (register item 25), and the install is refreshed by a **whole-tree `rsync -a dist/ForgeAudio-AnalogSeries/`**, never by copying `plugin.dylib` and `res/` alone (the Phase 30 stale-install lesson: a stale install is a stale plugin *version*, not a stale binary, and the naive refresh satisfies a hash assertion while Rack keeps reading the old manifest).

### Shell & panel surface

- **D-18: `res/AnalogVCO.svg` gains one SYNC jack position; the shell reads the voltage and does nothing else.** Phase 30's D-07 rule stands — every visible control does something, so an in-Rack check is honest — and the operator cannot audition sync without a jack to patch. `src/AnalogVCO.cpp`'s *"THIS FILE DOES NO DSP"* banner holds: it assigns `syncVolts` and `syncConnected` into the POD and delegates. The SVG is replaced wholesale in Phase 35; physical form and placement are Phase 35's call. **`src/AnalogLFO.cpp` must remain absent from this phase's diff.**

### Claude's Discretion

- The exact master/slave ratio set, note grid, block lengths and cycle counts for the D-11 sync sub-grid — within D-10/D-11-of-Phase-32's bin-centred integer-cycle and cross-rate constraints.
- The mechanism by which the sync BLEP is withheld for D-14's second leg (a flag, a second entry point, or a test-only shim) — the same latitude Phase 32's D-08 gave the naive path.
- Whether the D-07 suppression rule is expressed as a per-site predicate or as a swept-interval test — subject to the constraint that it must be **recomputed per sample and never cached**, for the same reason D-04 of Phase 32 gives (the site geometry already moves with `character` and will move per sample once Phase 34's drift writes the `*Spread` fields).
- Whether the previous-sync-voltage store is reset, held, or invalidated on a sample-rate change — provided the choice is stated and asserted rather than inherited.
- The renderer's file naming, output directory, and `make` target name (D-15/D-16).
- Whether the sync sub-grid lands as new `TEST_CASE`s in `tests/test_vco_spectrum.cpp` or a new TU — **if a new TU, it costs an explicit `check_includes.sh` `VCO_SIDE_ALLOW` entry, and that must be a plan task with its own rationale, not a gate-time discovery** (the Phase 31 D-23 lesson; a new VCO test TU is LFO-side by default and `make guards` exits 1 the moment it lands).

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### The scheduled prerequisite — read FIRST
- `.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/deferred-items.md` **item 27** — the operator decision of 2026-08-27 that CR-01 and CR-02 are fixed as **Phase 33, Task 1, before the seam work**, with both defects reproduced and the minimum fix specified. **Item 13** — the hard-sync seam, the never-snap-to-0 constraint, and the PITCH-04 re-confirmation obligation. **Item 26** — the A/B audition gap and its named remedy (`NaiveVcoCoreMirror`) with its two binding constraints. **Item 15** — the compile canary's runtime-live obligation, re-opened by D-02's two new POD fields. **Item 9** — the narrow-pulse "reach" refinement, the documented first escalation. **Item 10** — kernel order is an **operator decision with an impact assessment, never a silent choice**. **Item 8** — every recorded decibel figure is an Apple-clang figure; the step-dominated / plateau split and its 1.0 / 4.0 dB bounds. **Item 5** — single-sample full-amplitude spikes measure 0.0 dB spectrally, which is why D-10 is time-domain. **Item 6** — the instrument-limitation precedent D-09 follows. **Item 25** — the second, stale `ForgeAudio` plugin directory in the operator's Rack tree. **Item 22 / item 23** — two live tripwires (`dt = 0.0005` resonant miss; the unreachable `dt <= 1` upper guard, whose census must be re-measured if any caller stops clamping).
- `.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/32-REVIEW.md` — CR-01 and CR-02 as originally written.

### v2.0 VCO research (locks approach)
- `.planning/research/STACK.md:124` — **Q3, the hard-sync answer**: `SchmittTrigger`, sub-sample crossing time, reset to fractional overshoot (**do not snap to 0**), polyBLEP scaled by `out_preReset − morphedWave(newPhase)` at the sync fractional time; the BLAMP is optional for lean v2.0 (D-08 adopts this).
- `.planning/research/STACK.md:149` — the "snapping sync reset to exactly `phase = 0`" landmine, **binding**. **:54** — sync as one more polyBLEP on the same machinery. **:64** — `SchmittTrigger` thresholds consistent with the existing reset trigger (D-03). **:19-38** — BLEP/BLAMP linear superposition and the per-sample algorithm. **:75** — why `-ffp-contract=off` is load-bearing for BLEP polynomials specifically. **:154-156** — the escalation path is v2.1 oversampling, **explicitly not minBLEP**.
- `.planning/research/PITFALLS.md:114-131` — **Pitfall 3, the phase's central landmine**: placing the sync BLEP at the slave's phase instead of the **master's** wrap fraction; and re-using the LFO's 3 ms cosine crossfade (≈130 samples at audio rate — it smears and mutes the sync and destroys the buzzy timbre). **:127** — handle ≥1 event per sample **and** the case where the reset makes the slave cross its own discontinuity in the same step (D-07 and D-09). **:190** — the in-class `static constexpr` table trap that got v2.0.0 rejected. **:211** — any new transcendental in the BLEP/sync path is a fresh chance at platform divergence (D-08's rejection rests on this). **:382**, **:409** — the sync verification checklist rows.
- `.planning/research/FEATURES.md:34` — hard sync as a table stake, coupled to the anti-aliasing work. **:98** — why anti-aliasing is the critical path for sync as well as morph.
- `.planning/research/ARCHITECTURE.md:55-58` — `VcoCore.hpp` owns the hard-sync reset; `RackCompat.hpp` supplies `SchmittTrigger` and is **REUSED as-is**.

### Requirements & roadmap
- `.planning/ROADMAP.md` §"Phase 33" — goal and 3 success criteria. **No pre-planning document edit is required** (D-09) — unlike Phase 32's D-02/D-06.
- `.planning/ROADMAP.md:100` — the **milestone guardrail** paragraph: all VCO work is additive, the four shared headers stay frozen, and any LFO-regression risk is surfaced to the operator with impact + options + a recommendation.
- `.planning/REQUIREMENTS.md:39-40` — SYNC-01, SYNC-02. **:134-135** — both mapped to Phase 33.
- `.planning/PROJECT.md` §Constraints — the LFO non-regression guardrail and the four frozen shared headers. §"Current Milestone" — the lean-scope decision deferring oversampling to v2.1.

### Prior-phase hand-offs (inherited decisions — do not re-litigate)
- `.planning/phases/32-morph-aware-anti-aliasing-polyblep-polyblamp/32-CONTEXT.md` — **D-13** (the pending-residual accumulator; the output delay buffer is rejected on two grounds, one of them being Phase 33 specifically), **D-14** (the seam, and its per-instance CORE-03 constraint), **D-07** (overlapping edges sum, never overwrite), **D-05** (corrections ride through the bleed normalization), **D-04** (recompute site geometry every sample, never cache), **D-08/D-09/D-10/D-11** (the measure-then-pin protocol, per-shape evidence-set thresholds, integer cycles / rectangular window, 44.1 kHz binding with 48/96 kHz regression), **D-15** (the negated-comparison NaN idiom, never `forge::clamp`), **D-01** (analytic magnitudes — and **D-05 here states why sync does not follow it**).
- `.planning/phases/31-pitch-tuning-exponential-fm/31-CONTEXT.md` — D-05 (do not churn POD field semantics), D-10/D-11 (the Nyquist clamp), D-12 (`kVcoMaxDeltaPhase = 0.5` untouched), D-14 (the negated-comparison idiom), D-17 (the shell does no DSP — **D-02 here rests on it**), D-19 (measure the output, not telemetry).
- `.planning/phases/31-pitch-tuning-exponential-fm/deferred-items.md` — **item 11** (PITCH-04 marked complete on two of three input classes — **D-12 closes it**), item 3 (`forge::clamp` is NaN-transparent), item 9 (the canary's unique-field margin).
- `.planning/phases/30-vcocore-skeleton-module-registration/30-CONTEXT.md` — D-07 (every visible control does something — **D-18 continues it**), D-16 (measure the output, not telemetry).
- `.planning/STATE.md` §Accumulated Context — the standing **"no tag or resubmission on local evidence alone"** rule; the **R-9 `VcoInputs`-not-`Inputs` ODR landmine**; the whole-tree install-flush lesson (**D-17.4**); and the Phase 30/31 gate-design lessons (gates are artifacts needing review in their own right; bare `grep -c` criteria produce artifact counts).

### Code to write, call, and not touch
- `src/dsp/MorphBlep.hpp` — **Task 1 edits it (D-04); D-06 may additively extend it.** **:213-269** the struct, `reset()`, and the `addStep` seam with its `xAhead` gate (**`jump` is currently unguarded — D-04 closes that**). **:273-306** the preamble, accumulator drain and the `dt` guard. **:319-332** the `segment` clamp and the `W[segment]` write (**CR-01**). **:380-388** the recompute-never-cache rule. **:399-467** the nine-site union (**D-07's suppression rule attaches here**). **:38-49** the compact-support requirement and the falsified over-correction argument.
- `src/dsp/VcoCore.hpp` — the caller and the owner of sync state. **:229-249** `VcoInputs` (**D-02 adds two fields**) and the per-instance state block. **:540-541** the phase advance and single-subtract wrap (**D-07's reset overwrite lands here**). **:592-602** the morph/character negated-comparison guards — the worked example D-04 mirrors. **:645** the single `blep.step` call. Read the banner first: the **source-shape contract** (`struct VcoCore` and `float step(...)` must each stay on one line with their brace or `make guards` hard-fails), the binding **C++11 rules**, and **zero Rack-SDK includes**.
- `src/dsp/RackCompat.hpp` — **FROZEN, byte-pinned, shipped-LFO-consumed.** **:44-58** `SchmittTrigger` (UNINITIALIZED handling is load-bearing) — **called, never edited**. `forge::clamp` is NaN-transparent.
- `src/dsp/Waveshape.hpp` — **FROZEN. Called, never edited.** **:212** the bleed normalization D-05 rides through. **:158-216** `morphedWave`, the one call D-05 adds.
- `src/dsp/LfoCore.hpp:134-141` — the shipped LFO's reset trigger: the `0.1f, 1.0f` thresholds (D-03) and the `resetConnected` gating precedent (D-02). **Also the cosine-crossfade path SYNC-02 forbids for sync** — read it to know what is being rejected.
- `src/dsp/ClockTracker.hpp:108-114` — the second `0.1f, 1.0f` site confirming the convention.
- `src/AnalogVCO.cpp` — gains the SYNC jack and two POD assignments (D-18). Its *"THIS FILE DOES NO DSP"* banner stands.
- `src/AnalogLFO.cpp` — **must remain absent from this phase's diff**, as in Phases 30, 31 and 32.
- `src/vco_compile_canary.cpp` — **must feed both new `VcoInputs` fields a runtime-derived value** or `check_canary.sh [2b/5]` stops proving anything (D-02, register item 15).
- `tests/VcoBlockDriver.hpp` — the harness; already drives 44.1/48/96 kHz with correct timing-injection discipline. **Never template or subclass it with `tests/BlockDriver.hpp`**, which feeds the shipped LFO's bit-exact golden leg.
- `tests/test_morph_blep.cpp` — **case five parts A and B pin the seam** (a driven morph site at `s = 0.5` and `addStep(0.5, 2)` produce the same `+0.250000 / −0.250000` split; events compose by summation; the entry gate rejects hostile positions without touching per-instance state). Task 1's RED and the `jump` guard's permanent assertions land here.
- `tests/test_vco_core.cpp` — **:594** the corrected `estimateFreqRising` premise (the estimator counts rising zero crossings and is blind under ~2 samples — do not use it for sync high-note claims). **:1101** scenario four's `HOSTILE_TIMES` grid, which gains sync voltages. **:511** the two measured output tiers (`kHostileBoundV` 10.0 V, `kMusicalBoundV` 5.55 V) — **re-derive for sync, do not assume**; audio-rate MORPH already reaches 6.289864 V and the withholding of the tighter tier is itself asserted.
- `tests/test_vco_spectrum.cpp` — the spectral apparatus D-06/D-11 extend: `SPECTRUM_GRID`, the integer-cycle DFT with its leakage self-check, the `measuredDb` column with its **STOP-AND-REPORT** instruction and its ban on reclassifying a cell across the step-dominated / plateau split, and **`NaiveVcoCoreMirror`** — the bit-exact naive mirror D-14's renderer drives.
- `tests/test_vco_pitch.cpp` — where PITCH-04's existing evidence lives; D-12 extends it with sync as a third input class.
- `tests/check_frozen.sh`, `tests/check_canary.sh`, `tests/check_includes.sh` — the standing guards. **A new test TU costs an explicit `VCO_SIDE_ALLOW` entry; make it a plan task with its own rationale, not a gate-time discovery.**

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`forge::MorphBlep::addStep`** (`src/dsp/MorphBlep.hpp:257-262`) — the seam exists, is permanently pinned by test, and needs no change to *accept* sync events. Its `xAhead ∈ [0,1]` future-facing contract is what D-06 measures against.
- **`forge::SchmittTrigger`** (`src/dsp/RackCompat.hpp:46-58`) — already in the frozen shared header, already used at both LFO trigger sites, and its `UNINITIALIZED` state handling is documented as load-bearing. No new primitive needed.
- **The `0.1f / 1.0f` threshold convention** — established at `LfoCore.hpp:137` and `ClockTracker.hpp:111`, so D-03 inherits rather than invents.
- **`in.resetConnected` gating** (`LfoCore.hpp:136`) — the in-house precedent for D-02's `syncConnected`, matching `VcoInputs::fmConnected`.
- **`double` phase accumulator** — already in place. The fractional overshoot `(1 − f) · deltaPhase` comes out of the same arithmetic; no new precision machinery.
- **`tests/VcoBlockDriver.hpp`** — already runs 44.1/48/96 kHz; D-02's raw-volts boundary means it can drive the whole sync path with no new driver.
- **`NaiveVcoCoreMirror`** (`tests/test_vco_spectrum.cpp`) — a bit-exact non-band-limited mirror proved at 0 mismatches over 184,320 samples; D-14's renderer drives configurations through the same driver in the same pass rather than reimplementing the loop.
- **The Phase 32 spectral apparatus** — integer-cycle DFT, leakage self-check, per-cell threshold derivation. D-06 and D-11 extend it rather than building a second instrument.
- **Makefile / CI globs** — `make test` globs `tests/*.cpp`; `make strict` and the CI MinGW link leg glob `src/*.cpp`. New test files need zero build wiring but **do** need guard-allowlist entries.

### Established Patterns
- Rack-free header-only DSP under `src/dsp/*.hpp` with **zero `rack/` includes**; the shell owns params/inputs/outputs and delegates per-sample work to the core.
- Two-standard compilation: **both** `-std=c++11 -pedantic-errors` (the shipped plugin toolchain) and `-std=c++17` (the test target). C++11 forbids `inline constexpr` variables, `if constexpr`, `std::clamp`, in-class `static constexpr` indexed at runtime, and brace value-list init of `VcoInputs`.
- **Negated-comparison guards for non-finite input** — `if (!(x > 0.f)) x = 0.f;`, written negated *specifically* so NaN lands on the fallback branch. Never `forge::clamp`, which is NaN-transparent.
- **RED-first fixes and non-vacuous gates.** A requirement is ticked only where a control is observed *firing* behind the claim.
- **Gates are artifacts needing review in their own right.** Bare `grep -c` criteria produce artifact counts because these headers deliberately quote the constructs they forbid; count criteria must be comment-stripped or anchored and compared against a baseline rather than zero.
- **Measure, then pin, rounding outward** — with a discriminating mutation probe proving the bound bites at the boundary it claims.
- `-ffp-contract=off`, no `-ffast-math` — load-bearing for BLEP polynomials specifically.
- **Every phase from 30 onward ends in an in-Rack operator check**, and every one of them requires the whole-tree install flush.

### Integration Points
- **`src/dsp/MorphBlep.hpp`** — Task 1's guard hardening (D-04); possibly an additive past-edge entry point (D-06, measurement-dependent).
- **`src/dsp/VcoCore.hpp`** — two new `VcoInputs` fields; per-instance `SchmittTrigger` + previous-voltage store; the reset overwrite and site suppression at the phase-advance site; the seam call.
- **`src/AnalogVCO.cpp`** — SYNC jack, two POD assignments, no arithmetic.
- **`src/vco_compile_canary.cpp`** — both new fields fed runtime-derived values.
- **`res/AnalogVCO.svg`** — one jack position; replaced wholesale in Phase 35.
- **`tests/`** — the sync sub-grid (D-11), the SC-3 delta bound + anti-circularity (D-10), PITCH-04's third input class and the new-divisor probe (D-12), Task 1's RED evidence and permanent assertions (D-04), and the reusable A/B renderer (D-13..D-16).
- **Standing tripwires stay green:** no frozen header is edited, so `FROZEN.sha256` needs no bump; the LFO golden byte-lock is unaffected because no LFO behavior changes; the include-direction audit holds because no LFO TU includes any VCO file.

</code_context>

<specifics>
## Specific Ideas

- **"The seam is future-facing; the edge is always in the past."** This is the phase's sharpest finding and it was not in any prior document. `addStep`'s contract places corrections on the current and *next* sample, but a Schmitt trigger cannot know about an edge until the sample *after* it happened. Register item 13's assurance that no header change is needed rests on a reading that this discussion falsified as *conditional* rather than false — which is exactly why D-06 measures rather than argues. **A plan that quietly assumes item 13's claim and calls `addStep(f, jump)` without measuring has skipped the phase's central question.**
- **The one place this phase deliberately breaks Phase 32's method, and why.** Phase 32's through-line was *"compute it, don't measure it"* — safe because every morph site is a known discontinuity in a frozen function. A sync reset lands at an arbitrary phase, so there is no closed form to compute. D-05 pays one `morphedWave` call on sync samples rather than approximating a whole waveform. **This is a reasoned departure, not an oversight, and must not be "restored" for consistency.**
- **A suppression rule is the safe direction here, the way compact support was in Phase 32.** Register item 3 measured that erring toward over-correction is *not* safe — a step-shaped correction added to a signal with no step was a 30 dB regression. The same logic applies to D-07: a free-run correction placed for an edge the waveform *jumped over rather than crossed* is new broadband energy, not a filter. Suppression is the compact-support instinct applied to sync.
- **The gate must be able to see the artifact it names.** D-10 is time-domain because register item 5 *measured* that single-sample full-amplitude spikes read 0.0 dB spectrally. A sync gate built only on the alias floor would be structurally blind to the click SC-3 exists to forbid — the same "mechanism narrower than the prose" failure, inverted.
- **Item 26 gets fixed here because this is the first phase that can't fake it.** Phase 32's automated evidence was strong enough that the unevidenced audition half cost little. *"Buzzy, not smeared"* has no automated instrument at all, so an audition without a reference would be the whole verdict, unanswerable by construction. Building the renderer reusably (D-16) is what stops Phase 34's DRIFT-03 — a value that is *decided* by listening — inheriting the same problem.
- **Task 1 is genuinely load-bearing, not bookkeeping.** CR-01's failure mode is `INT_MIN` on the x86 toolchains that ship and a benign `0` on the arm64 host where all development happens. That is precisely the invisible-on-Apple-clang class that got v2.0.0 rejected from the VCV Library, and the seam this phase adds is the second call site that makes it live.

</specifics>

<deferred>
## Deferred Ideas

- **A cosine-crossfade third A/B leg** (D-14's rejected option) — would make *"buzzy, not smeared"* an evidenced three-way comparison by rendering the design `research/PITFALLS.md:114-131` and SYNC-02 both forbid. Not taken: implementing the wrong design on purpose inside the phase establishing the right one. **Resolve at: the first UAT where the operator's verdict on smear is equivocal** — it is the cheap escalation, and D-16's renderer makes it a configuration rather than a build.
- **Applying the A/B renderer to Phase 32's morph pair** — closes register item 26's *original* debt (the unevidenced audible-improvement half of the 32-11 audition). D-16 makes it near-free. **Resolve at: any later phase, or Phase 36 alongside the goldens.**
- **polyBLAMP on the sync path** (D-08) — the documented **first** escalation if a sync alias threshold proves unreachable, ahead of any kernel-order change. **Resolve at: the first plan that misses a sync threshold.**
- **Four-point (quintic) polyBLEP/polyBLAMP** — inherited as register item 10, **restated unchanged**. AA-05 forbids minBLEP and oversampling by name but says nothing about kernel order, so it is unscoped rather than forbidden. **Resolve at: an OPERATOR DECISION with an impact assessment, never a silent implementation choice.** The broad escalation path remains **v2.1 oversampling, explicitly not minBLEP**.
- **The narrow-pulse "reach" refinement** — inherited as register item 9, **restated unchanged**. Worth ~+1.3 dB at the worst grid point; not taken because it would add the only division by an edge width in the header. **Resolve at: the first plan that misses a pulse threshold.**
- **A full `MorphBlep.hpp` banner/contract audit** (D-04's rejected option) — the header advertises caller-independence in capitals and enforces it for `dt` and, after Task 1, for `morph`/`character`/`jump`. Whether every remaining claim is enforced is unaudited. **Resolve at: any phase that adds a third `MorphBlep` call site.**
- **CHARACTER's CV input and attenuverter → Phase 34** (CHAR-01) — inherited as register item 15, unchanged, including its two consequences (closing the canary's one-field margin; the remaining half of Phase 30 CR-02).
- **The output stage, drift, and DRIFT-03's audition-gated value → Phase 34** (OUT-01..03, DRIFT-01..03). **Phase 34 must re-read Phase 32's D-04**: once drift writes the `*Spread` fields, every discontinuity position moves per sample and the recompute-never-cache rule is what keeps `MorphBlep` correct. **D-16's renderer is the instrument DRIFT-03's decision should be made on.**
- **Per-instance seed entropy + patch persistence in the shell → Phase 34/35** — inherited as register item 16, unchanged, including the `(0,0)` Xoroshiro fixed-point hazard (a hang while opening a patch, not a failing test).
- **The FM DEPTH knob's affordance → Phase 35** (register item 17) and **a COARSE octave/semitone snap → Phase 35 or v2.1** (register item 18) — both restated unchanged.
- **The spectral column's cross-toolchain portability → Phase 36** (register item 8). Any sync cells added by D-11 join that problem and must not be captured as a golden from Apple clang alone.
- **`plugin.json` still declares version `2.0.1` while shipping two modules → Phase 36** (register item 20, REL-01), restated unchanged.
- **The shipped Analog LFO's shared latent UB → NO PHASE** (register item 12), **restated unchanged and still deliberately unowned.** Whoever picks it up is **opening a guardrail event and must open it as one**. The direct consequence still binds: **no permanent repo-wide UBSan gate may be adopted**, so any sanitizer use in this phase — including Task 1's ASan RED for CR-01 — stays a **scoped one-shot probe**.
- **The `dt = 0.0005` resonant-tiling miss** (register item 22) and **the unreachable `dt <= 1` upper guard** (register item 23) — two live tripwires. If a sync grid cell misses its threshold at a suspiciously round sample rate, item 22 is the first thing to check; if any change makes a caller reach `MorphBlep` without clamping, scenario four's `UPPER = 0` census must be **re-measured, not assumed**.
- **The second, stale `ForgeAudio` plugin directory in the operator's Rack tree** (register item 25) — still open, still the operator's housekeeping call. D-17.4 carries the verification-protocol half.

### Reviewed Todos (not folded)
- **"Wire `tests/check_docs.sh` into CI"** (`.planning/todos/wire-check-docs-into-ci.md`) — matched Phase 33 at score 0.4 on the single generic keyword *phase*. Already reviewed and deferred to **Phase 36** during both Phase 31 and Phase 32 on identical reasoning; that decision is **carried forward, not re-litigated**. It is a one-line CI step for a Phase 27 documentation gate and has nothing to do with hard sync. Remains visible via the `GUARD_WIRING_EXEMPT` entry in `tests/check_includes.sh` §`[7/7]`, which reports it as `EXEMPT` on every `make guards` run.

</deferred>

---

*Phase: 33-hard-sync*
*Context gathered: 2026-08-28*
