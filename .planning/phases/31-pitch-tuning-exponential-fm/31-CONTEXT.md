# Phase 31: Pitch, Tuning & Exponential FM - Context

**Gathered:** 2026-07-29
**Status:** Ready for planning

<domain>
## Phase Boundary

The **pitch chain**. Everything that decides what frequency the oscillator runs at, and the proof that it is right.

V/OCT + COARSE + FINE + exponential FM are summed **in the volt domain** and passed through **one** `forge::exp2_taylor5` call. The provisional Nyquist constant Phase 30 left behind is replaced with a real policy. The result is proven to **< 1 cent** against an independent reference — TEST-02 is this phase's exit gate.

**Requirements:** PITCH-01, PITCH-02, PITCH-03, PITCH-04, PITCH-05, FM-01, FM-02, FM-03, TEST-02.

**PITCH-05 is already satisfied.** Phase 30 landed double-precision phase accumulation (`src/dsp/VcoCore.hpp:127`). This phase must not regress it, but has no work to do for it beyond that.

**NOT in this phase (deliberate boundaries):**
- Band-limiting / polyBLEP / polyBLAMP → **Phase 32** (CORE-02, AA-01..05, TEST-03). **The oscillator still aliases on purpose.** No assertion added by this phase may claim anything about spectral cleanliness, and the TEST-02 gate must be designed to survive an aliased output (see D-17).
- Hard sync → **Phase 33** (SYNC-01/02). `VcoInputs` still carries no sync fields.
- OU drift stepping, MORPH/CHARACTER CV + their attenuverters, the output stage (DC blocker + soft saturation) → **Phase 34** (CHAR-01, DRIFT-01..03, OUT-01..03). The output stays unconditioned at `×5`.
- The real 18HP Forge Noir panel, the CRT display, patch-state serialization, per-instance seed entropy → **Phase 35** (PANEL-01/02, DISP-01..03) and deferred item 2.
- VCO goldens, version bump, tag, #929 update → **Phase 36** (TEST-05, REL-01).

</domain>

<decisions>
## Implementation Decisions

### ✓ RESOLVED PRE-PLANNING ACTION

- **D-00 — RESOLVED 2026-07-29 (commit `5ab0cb7`).** Both edits landed before planning: `.planning/REQUIREMENTS.md` PITCH-03 and `.planning/ROADMAP.md` Phase 31 Success Criterion 2 now read **±1 semitone (±100 cents)**. Original decision text retained below for provenance.

- **D-00: PITCH-03 changes from ±2 semitones to ±1 semitone, and the source documents must be edited BEFORE planning.** The operator chose the research recommendation over what the planning documents currently say. Two files disagree with the decision and must be corrected first, or the plan will be checked against a gate it deliberately contradicts:
  - `.planning/REQUIREMENTS.md:18` — PITCH-03 currently reads *"FINE tune knob trims ±2 semitones for detuning/beating"*.
  - `.planning/ROADMAP.md` §"Phase 31" Success Criterion 2 — currently reads *"FINE trims ±2 semitones"*.

  Both become **±1 semitone (±100 cents)**. Rationale for the change: `.planning/research/FEATURES.md:31` recommends ±1 semitone as the classic hardware convention (a full knob sweep = one semitone), which doubles raw knob resolution for the unison-beating job the control exists to do. This is a *value* correction inside an already-scoped requirement, not a scope change — PITCH-03 still delivers exactly one FINE knob for detuning.

### Pitch Summation & Tune Controls (PITCH-01, PITCH-02, PITCH-03)

- **D-01: One summation, one exponential.** `pitchVolts = pitchCV + coarse + (fine / 12) + fmContribution`, then `freq = kVcoFreqC4 * exp2_taylor5(pitchVolts)`. Exactly one `exp2_taylor5` call in the whole chain. Never multiply frequencies, never call exp twice — that is Pitfall 4 (`.planning/research/PITFALLS.md:100`) and yields linear-FM behavior or per-octave detuning instead of musical exponential FM. `forge::exp2_taylor5` is used verbatim; **never** `std::exp2` or `std::pow` in `src/` (Pitfall 2, bit-identity landmine).
- **D-02: COARSE sweeps ±5 octaves, continuous, linear in octaves.** No snap of any kind. PITCH-02 says *"continuously"* and that word is honored literally. Rack's native ctrl-click-to-default already returns the knob to exactly 0, so "get back to concert pitch" needs no extra mechanism.
- **D-03: FINE trims ±1 semitone (±100 cents), linear in cents.** Per D-00. Rack's shift-drag supplies arbitrarily fine resolution on top.
- **D-04: Tooltip readout is COARSE in octaves, FINE in cents.** e.g. `+2.00 oct` and `-14.0 cents`. Each control reads in its own natural musical unit so the two are visibly different tools. Rejected: both-in-cents (COARSE reads as awkward four-digit numbers) and COARSE-in-Hz (the displayed frequency becomes a lie the instant a cable is patched into V/OCT, because `configParam` cannot see the input).
- **D-05: The POD keeps its documented units — `coarse` in octaves, `fine` in semitones.** `src/dsp/VcoCore.hpp:103-104` already declares them that way and Phase 30 shipped those comments. The core performs the `/12`; the shell stays dumb and forwards raw param values. Do **not** re-document these fields as volts mid-milestone: the boundary shape has been stable since Phase 29 and churning field semantics buys nothing.

### Exponential FM (FM-01, FM-02, FM-03)

- **D-06: Full clockwise attenuverter = 1.0 octave per volt.** The FM jack at full depth behaves exactly like a second 1V/oct input — the most predictable contract in Eurorack, and a known 1:1 reference the attenuverter scales down from. Explicitly **not** the LFO's `0.6` constant: that number was chosen and auditioned for sub-audio wobble, for a different job, and reusing it here would be cargo-culting a value rather than setting one.
- **D-07: The FM depth control is bipolar `-1..+1`, linear taper, default `0`, displayed `-100%..+100%`.** This resolves an ambiguity the operator's answer surfaced and which downstream agents would otherwise trip on: **the shipped LFO's controls named "atten" are unipolar attenuators, not attenuverters** — `src/AnalogLFO.cpp:214` is `configParam(FM_ATTEN_PARAM, 0.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f)`. FM-02 and roadmap criterion 3 both specify **bipolar** for the VCO. The decision keeps FM-02 as written (bipolar range, so negative settings give inverted FM) while borrowing the LFO's *styling*: linear taper, default-off, percentage display, and the same `"FM Depth"` param name. No requirements edit is needed for FM-02.
- **D-08: The FM control's physical form (full knob vs scalloped trimpot) is deferred to Phase 35.** This phase declares the param and gives it *a* widget on the throwaway panel; Phase 35 decides what it looks like when it lays out the real 18HP panel and has the whole control budget in view.
- **D-09: FM is gated on `in.fmConnected`.** Mirrors `src/dsp/LfoCore.hpp:182`. Rack reports 0V for an unpatched input, so the arithmetic is already a no-op — the gate is for explicitness and for keeping the unpatched path's instruction sequence identical to the pre-FM one.

### Nyquist & Range Policy (PITCH-04)

- **D-10: Hard clamp — frequency pins at the ceiling and the oscillator keeps sounding.** Under deep FM the instantaneous pitch will hit the ceiling constantly (D-06 means a ±5V audio-rate modulator at full depth swings ±5 octaves), so peaks "flatten out" at the top rather than going silent. Rejected: amplitude-fade above threshold (it adds a gain stage that collides with Phase 34's OUT-01..03) and pitch fold-back (a deliberate effect, not the guard PITCH-04 asks for).
- **D-11: `kVcoNyquistGuardFrac` becomes `0.495f`.** `min(freq, 0.5 × sampleRate × 0.99)`, per `.planning/research/STACK.md:122`. ~21.8 kHz at 44.1 kHz — above human hearing, so the clamp is inaudible in normal use. The constant's `PROVISIONAL` comment at `src/dsp/VcoCore.hpp:84` is removed and replaced with the settled rationale; Phase 31 is the phase that comment named, so it must not leave it standing.
- **D-12: `kVcoMaxDeltaPhase = 0.5` is NOT touched.** `src/dsp/VcoCore.hpp:86-95` documents at length that this is a *wrap-correctness* bound on the phase increment, a different kind of constant from the Nyquist policy bound on frequency, and that Phase 31 must leave it alone when it retires the other. Honor that. The two guards remain independent because nothing in `VcoInputs` couples `sampleRate` to `sampleTime` (WR-01, MEASURED).
- **D-13: No low-end floor — the oscillator may stall at 0 Hz.** Extreme negative pitch freezes the phase and outputs a constant, and that is honest: a VCO tuned absurdly low *is* effectively DC, and the user asked for it. PITCH-04 speaks only to the top end. Rejected: mirroring `LfoCore`'s `std::fmax(freq, 0.001f)` — it would silently override the dialed value and add a constant with no requirement behind it. The existing negated floor `if (!(freq > 0.f)) freq = 0.f;` stays exactly as written, including its ordering (CR-01 — **do not swap those two lines**).

### Hostile-Input Hardening

- **D-14: The summed pitch volts are bounded to a finite range BEFORE the `exp2_taylor5` call.** FM introduces an unsanitized cable voltage into the exponent argument for the first time, and `forge::exp2_taylor5` performs `(int32_t)x` on it (`src/dsp/RackCompat.hpp:106`). **Casting a NaN or infinite float to `int32_t` is undefined behavior**, and the subsequent `xi << 23` on a negative int is UB as well. Rack does not sanitize cable voltages. Today's negated frequency floor catches the non-finite *result* — which is why Phase 30 survives a NaN V/OCT — but it does not prevent the UB on the way there. Binding constraints on the fix:
  - **Local to `VcoCore`.** `src/dsp/RackCompat.hpp` is byte-pinned by `tests/check_frozen.sh` and consumed by the **shipped** LFO. Editing it is a guardrail event, not a VCO fix.
  - **Must reject NaN.** `forge::clamp` is a comparison ladder, so both comparisons are false for NaN and the value passes straight through (deferred item 3 / CR-02). A plain `forge::clamp` here would be inert against exactly the input class this guard exists to stop. Use the negated-comparison idiom the frequency floor already uses.
  - **Must not fire for any reachable musical input.** Worst-case reachable sum is roughly ±29 V (Rack's ±12 V cable norm on V/OCT, ±5 octaves COARSE, ±1/12 octave FINE, ±12 V × 1.0 oct/V FM). Pick a bound comfortably outside that and inside `int32_t` safety.
  - **RED-first, per standing project practice.** A guard whose failing case was never observed is the exact class of evidence this project has twice rejected. The case must fail before the fix lands.
- **D-15: Deferred item 6 stays pointed at Phase 32.** The operator scoped this round to the pitch-volt clamp only. Extending scenario four's *timing* grid (`±inf`, subnormal, very-large-finite on `sampleRate`/`sampleTime`) is not in this phase. D-14's own RED case is not that extension — it covers the new pitch/FM fields, which are a different input class.

### Shell Surface

- **D-16: All four new controls are declared on the throwaway panel this phase.** COARSE knob, FINE knob, FM input jack, FM depth attenuverter — joining the existing V/OCT, MORPH, CHARACTER, OUT. This continues Phase 30's D-07 rule (*every visible control does something, so an in-Rack check is honest*) and its converse: DSP that no control can reach cannot be auditioned in Rack, and this phase's UAT is operator-driven in Rack the way Phase 30's was. Param/input **ID churn is still free** — nothing has shipped, so no user patch contains this module. Still stock SDK widgets, still `res/AnalogVCO.svg` as a throwaway at final 18HP geometry (Phase 30 D-06/D-08). **`src/AnalogLFO.cpp` must remain absent from this phase's diff**, as it was in Phase 30 — the cleanest position against the milestone guardrail.
- **D-17: `src/AnalogVCO.cpp` still does no DSP.** Its banner declares this load-bearing: the shell owns Rack indices, the core owns arithmetic, and the headless suite only describes what Rack produces for as long as that holds. The tune knobs forward raw param values (D-05); the `/12`, the summation, the clamp and the exponential all live in `VcoCore::step`.

### The < 1 Cent Gate (TEST-02) — this phase's exit criterion

- **D-18: Ground truth is `std::exp2` from libm, computed inside the test.** The test computes `261.6256 * std::exp2(volts)` and measures deviation in cents. This is the only option that measures the *actual* error including the polynomial's own. libm is banned in `src/` for bit-identity reasons — **the test is not `src/`**, and using it there is precisely what makes the assertion independent rather than self-referential. Explicitly rejected: comparing `exp2_taylor5` against itself, which is the vacuous-coverage trap Phases 29 and 30 were both bitten by.
  - **The research contradicts itself on the expected magnitude and neither number may be inherited.** `.planning/research/STACK.md:53` says the polynomial's fractional error is ~1e-6 relative (≪0.002 cents); `.planning/research/PITFALLS.md:94` says ~1e-4 relative (≈0.1 cent). Both clear 1 cent, so the gate holds either way — but the phase must **measure and record the observed figure**, not cite one of them.
- **D-19: Two tiers of observation.**
  - **Primary — output-derived.** Interpolated zero-crossings averaged over many cycles, measuring what Rack actually hears. Sub-sample interpolation is required: 1 cent is 0.058% frequency error, and raw integer-sample crossing counts cannot resolve that. This honors Phase 30's D-16, which chose output measurement over telemetry precisely because *"a telemetry assertion only re-reads the number `step()` just computed and would stay green even if the phase accumulator ignored the frequency entirely."*
  - **Secondary — `tel.freqHz`** against the same libm reference, covering the octaves where crossings cannot resolve. Recorded as the weaker tier in the phase's own verification, not presented as equivalent evidence.
  - Neither tier may assert anything about spectral content. The output is aliased on purpose until Phase 32.
- **D-20: Sweep the full pitch range; measure the high octaves at 96 kHz.** `tests/VcoBlockDriver.hpp` already runs 44.1/48/96 kHz. Each pitch is tested at whichever rate can actually resolve it, and the test states that mapping explicitly rather than leaving it implicit. Rejected: a tolerance that widens with samples-per-cycle — this project has been bitten four times in one phase by gates wider than the prose they encode, and a moving tolerance is that failure mode by construction.
- **D-21: The gate's range must respect D-10's clamp, and say so.** The hard clamp intentionally breaks 1V/oct tracking above the ceiling — that is the decided behavior, not a bug. At 44.1 kHz the ceiling is ~21.8 kHz, which C4×2^v reaches at about **+6.38 V**, so the tracking assertion must stop below that at that rate (the headroom is larger at 96 kHz, which is part of why D-20 routes high notes there). A test that swept past the ceiling would fail on correct behavior. The boundary must be **derived from the constant, not hardcoded**, so it stays correct if the constant ever moves.

### Post-Research Decisions (operator-confirmed 2026-07-29, after 31-RESEARCH.md)

Research measured three things that changed what the plan can assume. Each was surfaced with options and a recommendation; the operator took the recommendation in all three cases.

- **D-22: D-14's RED evidence is a one-shot UBSan probe, plus a permanent behavioral case pinned at the bound.** Research MEASURED that a behavioral RED case for D-14 is **vacuous** — today's core already survives NaN, ±inf, ±1e30, ±130 V and ±200 V pitch input with `allFinite == true`, `tel.freqHz == 0` and `|out| ≤ 5 V`, because the existing negated frequency floor catches the UB-produced garbage. A behavioral RED would be green *before* the fix, which is exactly the vacuous-coverage trap Phases 29 and 30 were both bitten by. The only non-vacuous red is UBSan (`-fsanitize=undefined`), verified working this session, which names both sites: `src/dsp/RackCompat.hpp:106` (float-cast-overflow) and `:109` (left-shift overflow). Therefore: **(a)** the plan records a one-shot UBSan probe with the **literal diagnostic text** as the RED evidence in the phase's verification, and **(b)** the suite carries a *standing* behavioral case driving `pitchVolts` to exactly `±kVcoMaxPitchVolts` and asserting a finite, in-range output. Case (b) is explicitly **not** the RED — it is the permanent regression check. Rejected: a new `Telemetry` pitch-volt field, which would be a genuinely non-vacuous permanent RED but adds boundary-adjacent surface that Phase 32+ inherits; the operator declined that trade.
- **D-23: The pitch/FM tests land in a NEW file, `tests/test_vco_pitch.cpp`, and the guard allowlist edit is an explicit task.** `tests/test_vco_core.cpp` is already 1109 lines. `TEST_SOURCES := $(wildcard tests/*.cpp)` picks the new file up with **zero** build wiring, but it costs **one exact-path `VCO_SIDE_ALLOW` entry in `tests/check_includes.sh`** or `make guards` exits 1 — the same landmine Phase 30 hit, where the analogous edit was NOT operator-checkpointed. The allowlist addition must therefore be a **plan task carrying its own rationale**, not a gate-time discovery. This resolves the former discretion item "whether TEST-02 extends `test_vco_core.cpp` or lands in a new file".
- **D-24: The shipped LFO's shared latent UB is recorded as a deferred item, unfixed by decision.** Research MEASURED that `src/AnalogLFO.cpp:320` feeds an unsanitized cable voltage into the same frozen `(int32_t)` cast via `src/dsp/LfoCore.hpp:183-184` — the shipped, golden-pinned LFO carries the identical latent UB this phase is hardening the VCO against. **Fixing it is a milestone-guardrail event** requiring operator sign-off and golden re-verification, and it is explicitly **not** this phase's work. Two consequences the plan must honor: **(1)** a permanent repo-wide UBSan gate **cannot** be adopted — it would fail on the shipped module — so D-22's UBSan use stays a scoped one-shot probe; **(2)** the item is logged in `<deferred>` pointed at **no phase**, so Phase 33/34 does not rediscover it cold.

### Claude's Discretion

- Exact numeric bound for D-14's pitch-volt clamp, and where the constant lives (namespace-scope `constexpr` per the `VcoCore.hpp` banner — never in-class `static constexpr`). Research recommends ±64 V and marks the specific value discretionary.
- The cycle count, block length, and interpolation method for D-19's crossing measurement, and the exact pitch/sample-rate assignment table for D-20.
- Throwaway-panel geometry and widget placement for D-16's four new controls; the panel is replaced wholesale in Phase 35.
- **Param display precision:** leave Rack's default `displayPrecision = 5` (which yields `+2.0000 oct` / `-14.000 cents`, not D-04's illustrative `+2.00 oct`). The *units* are what D-04 fixes; the digit count is not. This matches the shipped LFO, which sets no precision anywhere. Note the divergence in the plan; if exactness ever matters it is one line per param on the returned `ParamQuantity*`.
- **The `src/dsp/VcoCore.hpp:249-250` comment correction** ("clears that maximum by roughly two percent" becomes ~1.0 % at `0.495`) folds into the same task that touches `kVcoNyquistGuardFrac`, with the arithmetic restated. The constant itself must not change (D-12). It is a one-sentence edit, but it is exactly the "false comment" class that plan 30-08 existed to remove.
- Whether the FM contribution is computed as `fmVolts * fmAtten` with the 1.0 oct/V factor implicit, or carries an explicit named depth constant. D-06 fixes the *behavior*; the expression's shape is the planner's call.
- Whether `kVcoFreqC4`'s comment needs updating now that PITCH-01 is genuinely delivered rather than anticipated.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase 30 hand-off (read first — this phase inherits its seam, its constants and its debts)
- `.planning/phases/30-vcocore-skeleton-module-registration/30-CONTEXT.md` — D-07 (only controls the DSP consumes get declared; D-16 here continues it), D-08 (stock widgets, `AnalogLFO.cpp` out of the diff), D-14 (the pitch chain this phase completes), D-16 (measure the output, not telemetry).
- `.planning/phases/30-vcocore-skeleton-module-registration/deferred-items.md` — **item 3** (`forge::clamp` is NaN-transparent; binding constraints on any fix — D-14 above operates under them), **item 6** (the hostile-timing grid gap, which D-15 leaves at Phase 32).
- `.planning/phases/30-vcocore-skeleton-module-registration/30-VERIFICATION.md` — what Phase 30 proved and what it explicitly did not (the zero-crossing pitch check is labelled *"not the tracking gate"*; TEST-02 is unclaimed until this phase).
- `.planning/STATE.md` §Accumulated Context — the standing "no tag on local evidence alone" rule, the R-9 `VcoInputs`-not-`Inputs` ODR landmine, and the Phase 30 gate-design lesson (gates are artifacts needing review in their own right).

### v2.0 VCO research (locks approach)
- `.planning/research/STACK.md:52-53` — the Rack V/Oct pitch law and the `exp2_taylor5` reuse mandate; **:122** — the pitch-summation formula and the `0.5 × sampleRate × 0.99` Nyquist recommendation behind D-11.
- `.planning/research/PITFALLS.md:94-110` — why `exp2_taylor5` does not drift sharp/flat across octaves (integer octaves exact via the `exp2Floor` bit trick), Pitfall 4 (double-exponentiation / multiplicative FM), and the warning-sign taxonomy (fixed cents offset = wrong C4; growing offset per octave = accumulation/linear-FM bug; dropouts at extremes = overflow/clamp). **:381** — the full-MIDI-range < 1 cent checklist item. **:190** — the in-class `static constexpr` table trap that got v2.0.0 rejected; VCO modules invite exactly this via semitone/coarse tables.
- `.planning/research/ARCHITECTURE.md:190-206` — the canonical signal-chain ordering and its rationale (expo FM lands in the volt domain **before** the single `exp2`). **:123-126** — the `VcoInputs` pitch field set. **:330** — invariant 1, the V/oct tracking test.
- `.planning/research/FEATURES.md:31` — the coarse/fine range recommendation that D-00/D-03 adopt. **:30** — the 1V/oct contract and 7–10 octave expectation.

### Requirements & roadmap
- `.planning/REQUIREMENTS.md` — PITCH-01..05, FM-01..03, TEST-02 (this phase). **Line 18 requires the D-00 edit before planning.**
- `.planning/ROADMAP.md` §"Phase 31" — goal and 4 success criteria. **Criterion 2 requires the D-00 edit before planning.** Plus the v2.0 milestone guardrail block.
- `.planning/PROJECT.md` §Constraints — the LFO non-regression guardrail and the four frozen shared headers.

### Code to modify, mirror, and call
- `src/dsp/VcoCore.hpp` — **the file this phase modifies.** Read the banner before editing: the C++11 rules are binding (no `inline constexpr` variables, no `if constexpr`, no `std::clamp`, no in-class `static constexpr` indexed at runtime, no brace value-list init of `VcoInputs`), and the **source-shape contract** at lines 20-29 means the `struct VcoCore` and `float step(...)` lines must each stay on one line with their opening brace or `make guards` hard-fails. Lines 86-95 explain why `kVcoMaxDeltaPhase` must survive this phase untouched (D-12); lines 203-233 explain why the two frequency-guard lines must not be reordered (CR-01).
- `src/dsp/RackCompat.hpp:100-121` — **FROZEN.** `forge::exp2_taylor5` and its `(int32_t)` cast, which is the UB D-14 guards. `forge::clamp`, which is NaN-transparent and must not be relied on for that guard.
- `src/dsp/LfoCore.hpp:181-187` — the shipped FM path (`freq *= exp2_taylor5(in.fmCV * in.fmAtten * depthScale)`). **Precedent for the `fmConnected` gate (D-09), and the counter-example for everything else**: the LFO multiplies frequency *after* the pitch computation, the VCO sums into the volt domain *before* it. Do not copy the shape.
- `src/AnalogLFO.cpp:203-214` — the shipped `configParam` styling D-07 borrows (linear, default-off, `"%"` display) and the unipolar-vs-bipolar discrepancy D-07 resolves. **Do not edit this file.**
- `src/AnalogVCO.cpp` — the shell that gains four controls. Its banner's "THIS FILE DOES NO DSP" rule is load-bearing (D-17).
- `tests/VcoBlockDriver.hpp` — the harness. Already sweeps `pitchCV` and runs 44.1/48/96 kHz. **Never template or subclass it with `tests/BlockDriver.hpp`** (R-2/P-4 — that file feeds the shipped LFO's bit-exact golden leg).
- `tests/test_vco_core.cpp` — the existing VCO invariants, including scenario four's hostile-timing grid (D-15 leaves it alone) and the Phase 30 zero-crossing check explicitly labelled *not* the tracking gate.
- `tests/check_frozen.sh`, `tests/check_canary.sh`, `tests/check_includes.sh` — the standing guards. No frozen header is edited this phase, so `FROZEN.sha256` needs no bump.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`src/dsp/VcoCore.hpp:175`** — the existing single line `float freq = kVcoFreqC4 * exp2_taylor5(in.pitchCV);` is the exact insertion point. This phase widens its argument from one field to a summed, bounded expression and changes nothing else about the surrounding guard sequence.
- **`VcoInputs` already carries every field this phase needs** — `coarse`, `fine`, `fmVolts`, `fmAtten`, `fmConnected` were declared in Phase 29/30 and left unread, each annotated `(Phase 31)`. **No POD boundary change is required**, so `VcoBlockDriver` and the harness keep working untouched.
- **`tests/VcoBlockDriver.hpp::sweepScenario`** — already sweeps `pitchCV` across −2..+2 V at three sample rates. TEST-02 needs a wider, more precise sweep than this, but the driver's timing-injection discipline and non-degenerate default seeds are already correct.
- **Makefile / CI globs** — `make test` globs `tests/*.cpp`; `make strict` and the CI MinGW link leg glob `src/*.cpp`. A new test file and any shell edits are picked up automatically. **No build or CI wiring is needed.**

### Established Patterns
- Rack-free header-only DSP under `src/dsp/*.hpp` with **zero `rack/` includes**; the shell owns params/inputs/outputs and delegates per-sample work to the core.
- Two-standard compilation: every VCO header compiles clean under **both** `-std=c++11 -pedantic-errors` (the shipped plugin toolchain) and `-std=c++17` (the test target).
- **Negated-comparison guards for non-finite input.** `if (!(freq > 0.f)) freq = 0.f;` and `if (!(deltaPhase > 0.0)) deltaPhase = 0.0;` are written negated *specifically* so NaN lands on the fallback branch. D-14's clamp must follow this idiom, not `forge::clamp`.
- **RED-first fixes.** Every Phase 30 fix landed with a case that failed before it. D-14 is a fix, so it lands the same way.
- Hard-fail mechanical gates over conventions; `-ffp-contract=off`, no `-ffast-math` in the test build.

### Integration Points
- **`src/dsp/VcoCore.hpp`** — modified in place: the pitch expression, the `kVcoNyquistGuardFrac` value and comment, and the new pitch-volt bound. Boundary shape unchanged.
- **`src/AnalogVCO.cpp`** — four new `configParam`/`configInput` declarations and four new POD assignments in `process()`. No arithmetic.
- **`res/AnalogVCO.svg`** — throwaway panel gains four control positions. Replaced wholesale in Phase 35.
- **`tests/`** — the TEST-02 gate, plus D-14's RED case.
- **`.planning/REQUIREMENTS.md` and `.planning/ROADMAP.md`** — the D-00 edit, which must land before planning.
- **The standing tripwires are untouched:** no frozen header is edited, the LFO golden checksum lock is unaffected (no LFO behavior changes), and the include-direction audit is satisfied because no LFO TU includes any VCO file.

</code_context>

<specifics>
## Specific Ideas

- **"A second V/OCT" is the mental model for the FM jack at full depth.** D-06 was chosen for predictability over musical taming: a user who patches a sequencer into FM with the attenuverter fully clockwise gets 1:1 tracking, and everything else is that reference scaled down. The attenuverter is the taming mechanism, not the constant.
- **The LFO is a precedent for *style*, not for *arithmetic*.** Two decisions here (D-07's param styling, D-09's connected-gate) borrow from the shipped module; two (D-06's depth constant, D-01's summation order) deliberately do not. The LFO multiplies frequency after the fact; the VCO sums volts before. Copying the LFO's FM shape would produce the exact defect Pitfall 4 describes.
- **Deep FM will live at the Nyquist ceiling, so D-10 is a timbral decision.** With 1.0 oct/V and a ±5 V audio-rate modulator, the instantaneous pitch swings ±5 octaves and the clamp fires on most cycles. "Peaks flatten out" is the chosen sound.
- **The gate must measure, not cite.** Two research documents give figures for `exp2_taylor5`'s error that differ by two orders of magnitude. Both clear the bar, which is exactly why it would be easy to inherit a number instead of producing one. The phase records what it observed.
- **Vacuous coverage remains the failure mode this project has actually been bitten by.** D-18's libm ground truth and D-19's output-primary tier both descend from that lesson. A gate that compares the implementation to itself, or that reads back the number the implementation just wrote, is not evidence.
- **The clamp intentionally breaks the thing the gate measures, above a derived boundary.** D-21 is the reconciliation: 1V/oct tracking is guaranteed *up to the ceiling*, and the ceiling is a decided behavior. Deriving the test boundary from the constant rather than hardcoding it keeps the two from silently diverging.

</specifics>

<deferred>
## Deferred Ideas

- **COARSE octave/semitone snap → a later phase or v2.1.** A right-click menu toggle snapping COARSE to whole octaves (and possibly semitones) is genuinely useful on a VCO for exact octave stacking, and the LFO's 15-ratio snap already proved the pattern. Deferred because PITCH-02 specifies *"continuously"* and a snap toggle is a new capability: it needs a menu item, a persisted bool, and patch serialization the VCO currently has none of. Natural home is **Phase 35** (which owns the panel and is the first phase where the VCO plausibly gains patch state) or a v2.1 increment. Note `.planning/PROJECT.md` §Out of Scope lists "Octave snap / semitone selector" — but that entry was written about the **LFO**, where sub-audio rates made it meaningless; it is not a ruling on the VCO.
- **Amplitude fade near the Nyquist ceiling** — considered and rejected for D-10 because it introduces a gain stage that collides with Phase 34's OUT-01..03. If the flattened-peak sound proves harsh under deep FM during Phase 34's audition, that is the phase that owns the output stage and could revisit it.
- **Extending scenario four's hostile-timing grid** (`±inf`, subnormal, very-large-finite on `sampleRate`/`sampleTime`) — deferred item 6, still pointed at **Phase 32**, whose oversampled inner loop is the first real source of exotic timing. D-15.
- **Per-instance seed entropy + patch persistence in the shell** — deferred item 2, still pointed at **Phase 34/35**. Every live VCO in a patch is currently a bit-identical clone.
- **`tests/check_includes.sh [2/7]`'s unanchored exemption filter** — deferred item 5, resolved by the next phase that touches that script. ⚠ **Superseded by D-23:** this phase *does* now touch `tests/check_includes.sh` (the `VCO_SIDE_ALLOW` entry for `tests/test_vco_pitch.cpp`). The planner must decide whether to fold the exemption-filter fix into that same task or restate the deferral with a new owner — it may no longer sit idle on "the next phase that touches the script", because that is this phase.
- **The shipped LFO's shared latent UB** — `src/AnalogLFO.cpp:320` feeds an unsanitized cable voltage into the same frozen `(int32_t)` cast via `src/dsp/LfoCore.hpp:183-184`, the identical UB D-14 hardens the VCO against. Pointed at **no phase**. **Guardrail event — requires operator sign-off and golden re-verification; unfixed by decision** (D-24). Recorded so Phase 33/34 does not rediscover it cold. Direct consequence: a permanent repo-wide UBSan gate cannot be adopted while this stands, because it would fail on the shipped, golden-pinned module.

### Reviewed Todos (not folded)
- **"Wire `tests/check_docs.sh` into CI"** (`.planning/todos/wire-check-docs-into-ci.md`) — matched Phase 31 at score 0.6 on generic keywords only (*phase*, *correct*, *gate*). Reviewed and **deferred to Phase 36**, which owns CI and the release. It is a one-line CI step for a Phase 27 documentation gate and has nothing to do with the pitch chain; folding it here would be the same scope creep Phase 29 explicitly declined. It remains visible via the `GUARD_WIRING_EXEMPT` entry in `tests/check_includes.sh` §`[7/7]`, which reports it as `EXEMPT` on every run.

</deferred>

---

*Phase: 31-pitch-tuning-exponential-fm*
*Context gathered: 2026-07-29*
