# Phase 31: Pitch, Tuning & Exponential FM - Research

**Researched:** 2026-07-29
**Domain:** VCV Rack 2 audio-rate oscillator pitch chain (1V/oct exponential pitch law, volt-domain summation, exponential FM, Nyquist policy, sub-cent tracking verification) — C++11-strict, Rack-free DSP core
**Confidence:** HIGH (every load-bearing number in this document was MEASURED in this session against the live headers on this machine; nothing critical is inherited from the prior research documents)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

Copied verbatim from `.planning/phases/31-pitch-tuning-exponential-fm/31-CONTEXT.md` §Implementation Decisions.

#### ⚠ BLOCKING PRE-PLANNING ACTION

- **D-00: PITCH-03 changes from ±2 semitones to ±1 semitone, and the source documents must be edited BEFORE planning.** The operator chose the research recommendation over what the planning documents currently say. Two files disagree with the decision and must be corrected first, or the plan will be checked against a gate it deliberately contradicts:
  - `.planning/REQUIREMENTS.md:18` — PITCH-03 currently reads *"FINE tune knob trims ±2 semitones for detuning/beating"*.
  - `.planning/ROADMAP.md` §"Phase 31" Success Criterion 2 — currently reads *"FINE trims ±2 semitones"*.

  Both become **±1 semitone (±100 cents)**. Rationale for the change: `.planning/research/FEATURES.md:31` recommends ±1 semitone as the classic hardware convention (a full knob sweep = one semitone), which doubles raw knob resolution for the unison-beating job the control exists to do. This is a *value* correction inside an already-scoped requirement, not a scope change — PITCH-03 still delivers exactly one FINE knob for detuning.

#### Pitch Summation & Tune Controls (PITCH-01, PITCH-02, PITCH-03)

- **D-01: One summation, one exponential.** `pitchVolts = pitchCV + coarse + (fine / 12) + fmContribution`, then `freq = kVcoFreqC4 * exp2_taylor5(pitchVolts)`. Exactly one `exp2_taylor5` call in the whole chain. Never multiply frequencies, never call exp twice — that is Pitfall 4 (`.planning/research/PITFALLS.md:100`) and yields linear-FM behavior or per-octave detuning instead of musical exponential FM. `forge::exp2_taylor5` is used verbatim; **never** `std::exp2` or `std::pow` in `src/` (Pitfall 2, bit-identity landmine).
- **D-02: COARSE sweeps ±5 octaves, continuous, linear in octaves.** No snap of any kind. PITCH-02 says *"continuously"* and that word is honored literally. Rack's native ctrl-click-to-default already returns the knob to exactly 0, so "get back to concert pitch" needs no extra mechanism.
- **D-03: FINE trims ±1 semitone (±100 cents), linear in cents.** Per D-00. Rack's shift-drag supplies arbitrarily fine resolution on top.
- **D-04: Tooltip readout is COARSE in octaves, FINE in cents.** e.g. `+2.00 oct` and `-14.0 cents`. Each control reads in its own natural musical unit so the two are visibly different tools. Rejected: both-in-cents (COARSE reads as awkward four-digit numbers) and COARSE-in-Hz (the displayed frequency becomes a lie the instant a cable is patched into V/OCT, because `configParam` cannot see the input).
- **D-05: The POD keeps its documented units — `coarse` in octaves, `fine` in semitones.** `src/dsp/VcoCore.hpp:103-104` already declares them that way and Phase 30 shipped those comments. The core performs the `/12`; the shell stays dumb and forwards raw param values. Do **not** re-document these fields as volts mid-milestone: the boundary shape has been stable since Phase 29 and churning field semantics buys nothing.

#### Exponential FM (FM-01, FM-02, FM-03)

- **D-06: Full clockwise attenuverter = 1.0 octave per volt.** The FM jack at full depth behaves exactly like a second 1V/oct input — the most predictable contract in Eurorack, and a known 1:1 reference the attenuverter scales down from. Explicitly **not** the LFO's `0.6` constant: that number was chosen and auditioned for sub-audio wobble, for a different job, and reusing it here would be cargo-culting a value rather than setting one.
- **D-07: The FM depth control is bipolar `-1..+1`, linear taper, default `0`, displayed `-100%..+100%`.** This resolves an ambiguity the operator's answer surfaced and which downstream agents would otherwise trip on: **the shipped LFO's controls named "atten" are unipolar attenuators, not attenuverters** — `src/AnalogLFO.cpp:214` is `configParam(FM_ATTEN_PARAM, 0.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f)`. FM-02 and roadmap criterion 3 both specify **bipolar** for the VCO. The decision keeps FM-02 as written (bipolar range, so negative settings give inverted FM) while borrowing the LFO's *styling*: linear taper, default-off, percentage display, and the same `"FM Depth"` param name. No requirements edit is needed for FM-02.
- **D-08: The FM control's physical form (full knob vs scalloped trimpot) is deferred to Phase 35.** This phase declares the param and gives it *a* widget on the throwaway panel; Phase 35 decides what it looks like when it lays out the real 18HP panel and has the whole control budget in view.
- **D-09: FM is gated on `in.fmConnected`.** Mirrors `src/dsp/LfoCore.hpp:182`. Rack reports 0V for an unpatched input, so the arithmetic is already a no-op — the gate is for explicitness and for keeping the unpatched path's instruction sequence identical to the pre-FM one.

#### Nyquist & Range Policy (PITCH-04)

- **D-10: Hard clamp — frequency pins at the ceiling and the oscillator keeps sounding.** Under deep FM the instantaneous pitch will hit the ceiling constantly (D-06 means a ±5V audio-rate modulator at full depth swings ±5 octaves), so peaks "flatten out" at the top rather than going silent. Rejected: amplitude-fade above threshold (it adds a gain stage that collides with Phase 34's OUT-01..03) and pitch fold-back (a deliberate effect, not the guard PITCH-04 asks for).
- **D-11: `kVcoNyquistGuardFrac` becomes `0.495f`.** `min(freq, 0.5 × sampleRate × 0.99)`, per `.planning/research/STACK.md:122`. ~21.8 kHz at 44.1 kHz — above human hearing, so the clamp is inaudible in normal use. The constant's `PROVISIONAL` comment at `src/dsp/VcoCore.hpp:84` is removed and replaced with the settled rationale; Phase 31 is the phase that comment named, so it must not leave it standing.
- **D-12: `kVcoMaxDeltaPhase = 0.5` is NOT touched.** `src/dsp/VcoCore.hpp:86-95` documents at length that this is a *wrap-correctness* bound on the phase increment, a different kind of constant from the Nyquist policy bound on frequency, and that Phase 31 must leave it alone when it retires the other. Honor that. The two guards remain independent because nothing in `VcoInputs` couples `sampleRate` to `sampleTime` (WR-01, MEASURED).
- **D-13: No low-end floor — the oscillator may stall at 0 Hz.** Extreme negative pitch freezes the phase and outputs a constant, and that is honest: a VCO tuned absurdly low *is* effectively DC, and the user asked for it. PITCH-04 speaks only to the top end. Rejected: mirroring `LfoCore`'s `std::fmax(freq, 0.001f)` — it would silently override the dialed value and add a constant with no requirement behind it. The existing negated floor `if (!(freq > 0.f)) freq = 0.f;` stays exactly as written, including its ordering (CR-01 — **do not swap those two lines**).

#### Hostile-Input Hardening

- **D-14: The summed pitch volts are bounded to a finite range BEFORE the `exp2_taylor5` call.** FM introduces an unsanitized cable voltage into the exponent argument for the first time, and `forge::exp2_taylor5` performs `(int32_t)x` on it (`src/dsp/RackCompat.hpp:106`). **Casting a NaN or infinite float to `int32_t` is undefined behavior**, and the subsequent `xi << 23` on a negative int is UB as well. Rack does not sanitize cable voltages. Today's negated frequency floor catches the non-finite *result* — which is why Phase 30 survives a NaN V/OCT — but it does not prevent the UB on the way there. Binding constraints on the fix:
  - **Local to `VcoCore`.** `src/dsp/RackCompat.hpp` is byte-pinned by `tests/check_frozen.sh` and consumed by the **shipped** LFO. Editing it is a guardrail event, not a VCO fix.
  - **Must reject NaN.** `forge::clamp` is a comparison ladder, so both comparisons are false for NaN and the value passes straight through (deferred item 3 / CR-02). A plain `forge::clamp` here would be inert against exactly the input class this guard exists to stop. Use the negated-comparison idiom the frequency floor already uses.
  - **Must not fire for any reachable musical input.** Worst-case reachable sum is roughly ±29 V (Rack's ±12 V cable norm on V/OCT, ±5 octaves COARSE, ±1/12 octave FINE, ±12 V × 1.0 oct/V FM). Pick a bound comfortably outside that and inside `int32_t` safety.
  - **RED-first, per standing project practice.** A guard whose failing case was never observed is the exact class of evidence this project has twice rejected. The case must fail before the fix lands.
- **D-15: Deferred item 6 stays pointed at Phase 32.** The operator scoped this round to the pitch-volt clamp only. Extending scenario four's *timing* grid (`±inf`, subnormal, very-large-finite on `sampleRate`/`sampleTime`) is not in this phase. D-14's own RED case is not that extension — it covers the new pitch/FM fields, which are a different input class.

#### Shell Surface

- **D-16: All four new controls are declared on the throwaway panel this phase.** COARSE knob, FINE knob, FM input jack, FM depth attenuverter — joining the existing V/OCT, MORPH, CHARACTER, OUT. This continues Phase 30's D-07 rule (*every visible control does something, so an in-Rack check is honest*) and its converse: DSP that no control can reach cannot be auditioned in Rack, and this phase's UAT is operator-driven in Rack the way Phase 30's was. Param/input **ID churn is still free** — nothing has shipped, so no user patch contains this module. Still stock SDK widgets, still `res/AnalogVCO.svg` as a throwaway at final 18HP geometry (Phase 30 D-06/D-08). **`src/AnalogLFO.cpp` must remain absent from this phase's diff**, as it was in Phase 30 — the cleanest position against the milestone guardrail.
- **D-17: `src/AnalogVCO.cpp` still does no DSP.** Its banner declares this load-bearing: the shell owns Rack indices, the core owns arithmetic, and the headless suite only describes what Rack produces for as long as that holds. The tune knobs forward raw param values (D-05); the `/12`, the summation, the clamp and the exponential all live in `VcoCore::step`.

#### The < 1 Cent Gate (TEST-02) — this phase's exit criterion

- **D-18: Ground truth is `std::exp2` from libm, computed inside the test.** The test computes `261.6256 * std::exp2(volts)` and measures deviation in cents. This is the only option that measures the *actual* error including the polynomial's own. libm is banned in `src/` for bit-identity reasons — **the test is not `src/`**, and using it there is precisely what makes the assertion independent rather than self-referential. Explicitly rejected: comparing `exp2_taylor5` against itself, which is the vacuous-coverage trap Phases 29 and 30 were both bitten by.
  - **The research contradicts itself on the expected magnitude and neither number may be inherited.** `.planning/research/STACK.md:53` says the polynomial's fractional error is ~1e-6 relative (≪0.002 cents); `.planning/research/PITFALLS.md:94` says ~1e-4 relative (≈0.1 cent). Both clear 1 cent, so the gate holds either way — but the phase must **measure and record the observed figure**, not cite one of them.
- **D-19: Two tiers of observation.**
  - **Primary — output-derived.** Interpolated zero-crossings averaged over many cycles, measuring what Rack actually hears. Sub-sample interpolation is required: 1 cent is 0.058% frequency error, and raw integer-sample crossing counts cannot resolve that. This honors Phase 30's D-16, which chose output measurement over telemetry precisely because *"a telemetry assertion only re-reads the number `step()` just computed and would stay green even if the phase accumulator ignored the frequency entirely."*
  - **Secondary — `tel.freqHz`** against the same libm reference, covering the octaves where crossings cannot resolve. Recorded as the weaker tier in the phase's own verification, not presented as equivalent evidence.
  - Neither tier may assert anything about spectral content. The output is aliased on purpose until Phase 32.
- **D-20: Sweep the full pitch range; measure the high octaves at 96 kHz.** `tests/VcoBlockDriver.hpp` already runs 44.1/48/96 kHz. Each pitch is tested at whichever rate can actually resolve it, and the test states that mapping explicitly rather than leaving it implicit. Rejected: a tolerance that widens with samples-per-cycle — this project has been bitten four times in one phase by gates wider than the prose they encode, and a moving tolerance is that failure mode by construction.
- **D-21: The gate's range must respect D-10's clamp, and say so.** The hard clamp intentionally breaks 1V/oct tracking above the ceiling — that is the decided behavior, not a bug. At 44.1 kHz the ceiling is ~21.8 kHz, which C4×2^v reaches at about **+6.38 V**, so the tracking assertion must stop below that at that rate (the headroom is larger at 96 kHz, which is part of why D-20 routes high notes there). A test that swept past the ceiling would fail on correct behavior. The boundary must be **derived from the constant, not hardcoded**, so it stays correct if the constant ever moves.

### Claude's Discretion

- Exact numeric bound for D-14's pitch-volt clamp, and where the constant lives (namespace-scope `constexpr` per the `VcoCore.hpp` banner — never in-class `static constexpr`).
- The cycle count, block length, and interpolation method for D-19's crossing measurement, and the exact pitch/sample-rate assignment table for D-20.
- Whether TEST-02 extends `tests/test_vco_core.cpp` or lands in a new file — `make test` globs `tests/*.cpp` either way.
- Throwaway-panel geometry and widget placement for D-16's four new controls; the panel is replaced wholesale in Phase 35.
- Whether the FM contribution is computed as `fmVolts * fmAtten` with the 1.0 oct/V factor implicit, or carries an explicit named depth constant. D-06 fixes the *behavior*; the expression's shape is the planner's call.
- Whether `kVcoFreqC4`'s comment needs updating now that PITCH-01 is genuinely delivered rather than anticipated.

### Deferred Ideas (OUT OF SCOPE)

- **COARSE octave/semitone snap → a later phase or v2.1.** A right-click menu toggle snapping COARSE to whole octaves (and possibly semitones) is genuinely useful on a VCO for exact octave stacking, and the LFO's 15-ratio snap already proved the pattern. Deferred because PITCH-02 specifies *"continuously"* and a snap toggle is a new capability: it needs a menu item, a persisted bool, and patch serialization the VCO currently has none of. Natural home is **Phase 35** (which owns the panel and is the first phase where the VCO plausibly gains patch state) or a v2.1 increment. Note `.planning/PROJECT.md` §Out of Scope lists "Octave snap / semitone selector" — but that entry was written about the **LFO**, where sub-audio rates made it meaningless; it is not a ruling on the VCO.
- **Amplitude fade near the Nyquist ceiling** — considered and rejected for D-10 because it introduces a gain stage that collides with Phase 34's OUT-01..03. If the flattened-peak sound proves harsh under deep FM during Phase 34's audition, that is the phase that owns the output stage and could revisit it.
- **Extending scenario four's hostile-timing grid** (`±inf`, subnormal, very-large-finite on `sampleRate`/`sampleTime`) — deferred item 6, still pointed at **Phase 32**, whose oversampled inner loop is the first real source of exotic timing. D-15.
- **Per-instance seed entropy + patch persistence in the shell** — deferred item 2, still pointed at **Phase 34/35**. Every live VCO in a patch is currently a bit-identical clone.
- **`tests/check_includes.sh [2/7]`'s unanchored exemption filter** — deferred item 5, resolved by the next phase that touches that script. This phase does not.
- **"Wire `tests/check_docs.sh` into CI"** (`.planning/todos/wire-check-docs-into-ci.md`) — reviewed and **deferred to Phase 36**, which owns CI and the release.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| **PITCH-01** | V/Oct input tracks 1V/octave across the audio range (C4 = 0V reference), reusing `forge::exp2_taylor5` (no new exponential; shared-core bit-identity preserved) | §Pitch Law & the Measured Accuracy Budget — the exact formula, the verified `forge::exp2_taylor5` signature/domain, and a MEASURED end-to-end worst error of **0.0101 cents** (≈99× margin under 1 cent). Reuse is a one-line argument widening at `src/dsp/VcoCore.hpp:175`; no frozen header is touched. |
| **PITCH-02** | COARSE tune knob sweeps ±5 octaves continuously | §Param Contracts — verified `configParam` signature and display semantics from the real SDK; exact call given. Core adds `coarse` directly (already octaves per D-05). |
| **PITCH-03** | FINE tune knob trims **±1 semitone** for detuning/beating *(±2 in the source docs — D-00 edit REQUIRED before planning)* | §Param Contracts — exact `configParam` call with `displayMultiplier = 100` giving a `±100 cents` readout off a `±1` semitone raw range; core performs the `/12`. §Blocking Pre-Planning Action confirms both source docs still say ±2. |
| **PITCH-04** | Frequency is clamped just below Nyquist so extreme pitch/FM/sync never aliases via out-of-range frequency | §Nyquist Policy — `0.495f` crossover volts derived and MEASURED at all four rates; the clamp is already implemented and correctly ordered (CR-01), so this is a one-constant change with a MEASURED blast radius of two comment literals. |
| **PITCH-05** | Phase accumulation uses double precision so high-frequency phase-crossing placement stays accurate for band-limiting | §Double-Precision Phase & the Phase-32 Interface — **already satisfied** at `src/dsp/VcoCore.hpp:127/258-262`. This phase's only obligation is non-regression, plus not introducing a float round-trip in the new pitch expression. |
| **FM-01** | Exponential FM input modulates pitch at audio rate | §Signal-Chain Ordering — the FM term joins the volt-domain sum before the single `exp2`; no rate limit exists in the chain, so audio-rate is structural, not a feature to add. |
| **FM-02** | A dedicated bipolar attenuverter sets FM depth | §Param Contracts — `configParam(FM_ATTEN_PARAM, -1.f, +1.f, 0.f, "FM Depth", "%", 0.f, 100.f)`; the unipolar/bipolar discrepancy against the shipped LFO is resolved by D-07 and needs no requirements edit. |
| **FM-03** | FM sums into the volt domain before the single exponential (musical exponential FM) | §Signal-Chain Ordering + §Don't Hand-Roll — one `exp2_taylor5` call, one summation; the shipped LFO's `freq *= exp2_taylor5(...)` shape at `src/dsp/LfoCore.hpp:181-187` is the explicit counter-example, not the pattern. |
| **TEST-02** | < 1 cent V/Oct tracking, the phase gate | §Validation Architecture — a MEASURED pitch × sample-rate assignment table, the estimator's MEASURED resolution floor (fails at ≈2.03 samples/cycle, good to ≈2.63), MEASURED per-point cents errors, the derived clamp boundary, and the recommended fixed tolerance. |
</phase_requirements>

---

## Summary

This phase is **small in code and large in evidence**. The entire DSP change is the widening of one expression at `src/dsp/VcoCore.hpp:175` from `exp2_taylor5(in.pitchCV)` to `exp2_taylor5(<bounded sum of four terms>)`, one constant value change (`kVcoNyquistGuardFrac` `0.49f` → `0.495f`), one new namespace-scope constant for the D-14 pitch-volt bound, four `configParam`/`configInput` declarations plus four POD assignments in the shell, four rects in a throwaway SVG — and a substantial new test that has to prove sub-cent tracking without being vacuous. No frozen header is edited, no `VcoInputs` field is added, no build or CI wiring changes, and `src/AnalogLFO.cpp` stays out of the diff entirely. The LFO guardrail is not stressed by this phase at all.

**The accuracy question is settled, and it is settled in the implementation's favor by two orders of magnitude.** The two prior research documents disagree by 100× about `forge::exp2_taylor5`'s error and **both are wrong**. Measured this session, exhaustively over all 167,364,675 float values in one octave cell, the polynomial's own worst error is **±0.00487 cents**; measured end-to-end through `kVcoFreqC4 * exp2_taylor5(v)` against a double-precision `261.6256 * std::exp2(v)` reference across −10 V .. +10 V, the worst error is **+0.01006 cents** (worst relative error 5.77e-6). Integer octaves are **bit-exact** at all 21 volt values from −10 to +10, confirming the `exp2Floor` bit-trick claim. Float summation in the volt domain contributes at most **+0.0011 cents**. The 1-cent gate therefore has roughly a **99× margin**, and there is no plan-shaping decision to make about replacing or supplementing the polynomial.

**Two findings genuinely shape the plan, and both concern evidence rather than DSP.** First, D-14's RED-first requirement cannot be met with a behavioral assertion: it was MEASURED this session that today's core *already* survives NaN, ±inf, ±1e30, ±130 V and ±200 V pitch input with `allFinite == true`, `tel.freqHz == 0` and `|out| ≤ 5 V`, because the existing negated frequency floor catches the UB-produced garbage. A behavioral RED case would be green before the fix — exactly the vacuous-coverage trap this project has been bitten by twice. UBSan, by contrast, gives a precise and immediate red: `clang -fsanitize=undefined` reports the two UB sites by name and line (`RackCompat.hpp:106` float-cast-overflow, `RackCompat.hpp:109` left-shift overflow). Second, the D-19 crossing estimator has a hard, MEASURED resolution floor at roughly **2.03 samples per cycle** (error blows to −11.5 cents) while remaining accurate to **0.003 cents at 2.63 samples/cycle** — which is what makes D-20's rate-assignment table constructible rather than guesswork, and which happens to sit just below D-21's clamp boundary at every rate.

**Primary recommendation:** widen `src/dsp/VcoCore.hpp:175` to `float pitchVolts = in.pitchCV + in.coarse + in.fine * (1.f / 12.f); if (in.fmConnected) pitchVolts += in.fmVolts * in.fmAtten; if (!(pitchVolts > -kVcoMaxPitchVolts)) pitchVolts = -kVcoMaxPitchVolts; if (pitchVolts > kVcoMaxPitchVolts) pitchVolts = kVcoMaxPitchVolts; float freq = kVcoFreqC4 * exp2_taylor5(pitchVolts);` with `constexpr float kVcoMaxPitchVolts = 64.f;` — change nothing else in `step()` — and build TEST-02 at `morph = 0, character = 0` over a derived-boundary pitch sweep with a **fixed 0.05-cent tolerance** on the primary output-derived tier.

---

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Pitch summation (V/OCT + COARSE + FINE + FM) | DSP core (`forge::VcoCore::step`) | — | D-17 makes "the shell does no DSP" load-bearing: the headless suite only describes what Rack produces for as long as every arithmetic operation lives in the core. |
| Semitone→octave conversion (`fine / 12`) | DSP core | — | D-05 fixes the POD's units as semitones; the core owns the conversion so the shell stays a pure forwarder. |
| Exponential evaluation (`exp2_taylor5`) | Frozen shared header (`RackCompat.hpp`) — **called, never edited** | — | Byte-pinned by `tests/check_frozen.sh` and consumed by the shipped LFO. Reuse is mandated by PITCH-01 and by the Pitfall-2 bit-identity landmine. |
| Pitch-volt hostile-input bound (D-14) | DSP core (new local constant + inline guard) | — | The UB is in the frozen header; the *fix* must be local to `VcoCore` because editing `RackCompat.hpp` is a guardrail event. |
| Nyquist frequency clamp (PITCH-04) | DSP core | — | Already implemented and correctly ordered; only the constant's value and comment change. |
| Double-precision phase accumulation | DSP core | — | Already satisfied (`double phase` at line 127). Phase 32 consumes the sub-sample crossing fraction from it. |
| Rack param/input declaration + index ownership | Rack shell (`src/AnalogVCO.cpp`) | — | The shell owns `params[]`/`inputs[]` indices and nothing else. |
| Tooltip units and display formatting | Rack shell (`configParam` display args) | — | `ParamQuantity` display math is an SDK concern; the core never sees display units. |
| Control geometry | Panel asset (`res/AnalogVCO.svg`) + widget coords | Rack shell | Throwaway this phase; replaced wholesale in Phase 35. |
| Tracking proof (TEST-02) | Headless test target (`tests/`) | — | libm is banned in `src/` and required in `tests/` — that asymmetry is what makes D-18's ground truth independent. |

---

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `forge::exp2_taylor5` (`src/dsp/RackCompat.hpp:112-121`) | In-tree, FROZEN, verbatim copy of Rack SDK `dsp/approx.hpp` | The single `2^volts` evaluation for the whole pitch chain | The exact function VCV Fundamental VCO uses for pitch. Reuse mandated by PITCH-01 and by the golden bit-identity landmine. **MEASURED accurate to 0.0101 cents end-to-end** — no supplement needed. [VERIFIED: local measurement + `src/dsp/RackCompat.hpp:100-103` banner citing `../Rack-SDK/include/dsp/approx.hpp`] |
| `forge::kVcoFreqC4` (`src/dsp/VcoCore.hpp:83`) | In-tree, `261.6256f` | C4 = 0 V pitch reference | The VCV standard (`rack::dsp::FREQ_C4`). Already declared and already used. As a float it is `261.6256103515625`, a **fixed** +0.0000685-cent offset from the decimal — non-cumulative and 4 orders of magnitude under the gate. [VERIFIED: local measurement] |
| `forge::VcoCore` / `forge::VcoInputs` (`src/dsp/VcoCore.hpp`) | In-tree, Phase 29/30 | The DSP seam this phase completes | **All five pitch/FM fields already exist and are unread** (`coarse`, `fine`, `fmVolts`, `fmAtten`, `fmConnected`, lines 103-107, each annotated `(Phase 31)`). No POD boundary change is required. [VERIFIED: read the header] |
| `rack::engine::Module::configParam` | Rack SDK 2.x (`../Rack-SDK/include/engine/Module.hpp:125`) | Param declaration + tooltip units | Verified signature: `configParam(int paramId, float min, float max, float def, std::string name = "", std::string unit = "", float displayBase = 0.f, float displayMultiplier = 1.f, float displayOffset = 0.f)`. [VERIFIED: read the SDK header at `../Rack-SDK`] |
| doctest 2.4.11 (`tests/doctest.h`) | Vendored, in-tree | The TEST-02 harness | Already the project's only test framework; `make test` globs `tests/*.cpp`. [VERIFIED: `Makefile:36`, baseline run] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `tests/VcoBlockDriver.hpp` | In-tree, Phase 29 | Headless block driver over the VCO seam | Every TEST-02 drive. Already injects timing per sample and seeds non-degenerately. **Never** template/subclass it with `tests/BlockDriver.hpp` (R-2/P-4). |
| `std::exp2` (libm) | C++ stdlib | D-18's independent ground truth | **In `tests/` only.** Banned in `src/` for bit-identity reasons — that asymmetry is the point. |
| `clang -fsanitize=undefined` | Apple clang (local) | D-14's RED evidence | One-shot RED demonstration for the pitch-volt UB. **Not** a permanent repo-wide gate — see §Common Pitfalls, Pitfall 3. |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `forge::exp2_taylor5` | `std::exp2` / `std::pow` | **Rejected, non-negotiable.** Breaks the Pitfall-2 bit-identity landmine and PITCH-01's explicit wording. Also unnecessary: the measured error is 99× under the gate. |
| `forge::exp2_taylor5` | A wider-range or higher-order polynomial | **Rejected.** `.planning/research/STACK.md:122` already ruled "no wider-range exp needed"; this session's measurement makes that quantitative (0.0101 cents worst). Would be a shared-header edit — a guardrail event for zero benefit. |
| `forge::clamp` for the D-14 bound | Negated-comparison ladder | **`forge::clamp` is rejected by D-14 itself.** It is NaN-transparent (deferred item 3 / CR-02) and would be inert against the exact input class the guard exists to stop. |
| Behavioral RED case for D-14 | UBSan RED case | Behavioral is **MEASURED vacuous** today (see §Common Pitfalls, Pitfall 3). UBSan is the only tier that goes red on the actual defect. A pitch-volt telemetry field is a viable third option — see §Open Questions Q1. |
| New file `tests/test_vco_pitch.cpp` | Extend `tests/test_vco_core.cpp` | A new file costs one exact-path entry in `tests/check_includes.sh` `VCO_SIDE_ALLOW` (line 279-285) or `make guards` exits 1 — a known Phase-30 landmine. Extending costs nothing but grows an already-1109-line file. Recommend the new file **plus the allowlist edit surfaced as a plan step**, not discovered at gate time. |

**Installation:**
```bash
# No package installation. This phase adds ZERO external dependencies.
# Existing toolchain only:
make test     # Rack-free doctest suite (globs tests/*.cpp — new files auto-included)
make guards   # frozen + includes + canary guard suite
make strict   # -std=c++11 -pedantic-errors gate over src/*.cpp (needs ../Rack-SDK)
make          # plugin build (needs ../Rack-SDK)
```

**Version verification:** Not applicable — no package manager is involved. All code is in-tree or from the pinned local Rack SDK at `../Rack-SDK`. The SDK's `configParam` signature and `ParamQuantity` display formula were read directly from `../Rack-SDK/include/engine/Module.hpp:125` and `../Rack-SDK/include/engine/ParamQuantity.hpp:42-49` rather than assumed. [VERIFIED: local SDK read]

## Package Legitimacy Audit

**Not applicable — this phase installs no external packages.** Every symbol it consumes is either in-tree (`src/dsp/*.hpp`, `tests/*.hpp`, vendored `doctest.h`) or from the locally pinned VCV Rack 2 SDK at `../Rack-SDK` (a symlink to `/Users/mrcbrown/Claude/Software/Forge Audio/Rack-SDK`, verified present). No npm/PyPI/crates registry is touched, no `Makefile` dependency line changes, and no CI wiring is added.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| *(none)* | — | — | — | — | — | — |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

---

## Pitch Law & the Measured Accuracy Budget

> **This section exists because D-18 forbids inheriting a number.** Everything below was measured on this machine, this session, against the live `src/dsp/RackCompat.hpp` and `src/dsp/VcoCore.hpp`, compiled with the project's own test flags (`-std=c++17 -O2 -Isrc -ffp-contract=off`). The planner may cite these figures as a *starting expectation*; the phase must still re-measure them inside the test, per D-18.

### The formula

```
pitchVolts = pitchCV + coarse + fine/12 + (fmConnected ? fmVolts * fmAtten : 0)
freq       = kVcoFreqC4 * exp2_taylor5(pitchVolts)          // kVcoFreqC4 = 261.6256f
```
[CITED: `.planning/research/STACK.md:122`; `.planning/research/ARCHITECTURE.md:190-206`] and [VERIFIED: the same law is already half-implemented at `src/dsp/VcoCore.hpp:175`]

### `forge::exp2_taylor5` — verified signature and mechanism

```cpp
// src/dsp/RackCompat.hpp:104-121  — FROZEN. Read, never edit.
inline float exp2Floor(float x, float* xf) {
    x += 127.f;
    int32_t xi = (int32_t)x;            // <-- line 106: UB for NaN / ±inf / out-of-int32
    if (xf) *xf = x - (float)xi;
    union { float yi; int32_t yii; };
    yii = xi << 23;                     // <-- line 109: UB for negative xi, or xi > 255
    return yi;
}
inline float exp2_taylor5(float x) {
    float xf;
    float yi = exp2Floor(x, &xf);
    const float a[6] = {1.0f, 0.69315169353961f, 0.2401595990753f,
                        0.055817908652f, 0.008991698010f, 0.001879100722f};
    float yf = a[5];
    for (int i = 4; i >= 0; --i) yf = yf * xf + a[i];   // Horner
    return yi * yf;
}
```
- **Signature:** `float exp2_taylor5(float x)`, free function in `namespace forge`, `inline`, header-only. Takes octaves (volts), returns `2^x`. [VERIFIED: read the header]
- **Mechanism:** the integer octave is produced *exactly* by writing the IEEE-754 exponent field; only the fractional remainder goes through the degree-5 polynomial. This is why error does not accumulate across octaves. [VERIFIED: measured — see below]

### Measured accuracy (the numbers that settle the STACK/PITFALLS contradiction)

| Measurement | Result | Method |
|-------------|--------|--------|
| **Polynomial-only worst error, exhaustive** | **+0.0048690 cents** at `xf = 0.93600082` | Every one of the **167,364,675** float values in `[1e-6, 1.0)`, `exp2_taylor5(x)` vs `std::exp2((double)x)` |
| **Integer-octave exactness** | **bit-exact at all 21 points** `v ∈ [−10, +10]` | `(double)exp2_taylor5((float)v) == std::exp2((double)v)` exactly |
| **End-to-end worst, −10 .. +10 V** | **+0.0100571 cents** at `v = +9.4515` | `kVcoFreqC4 * exp2_taylor5((float)v)` vs `261.6256 * std::exp2(v)` (double), 4,000,001 points |
| **End-to-end worst, −7 .. +7.5 V (the gate's range)** | **+0.0098433 cents** at `v = +7.4679` | same, restricted grid |
| **End-to-end worst, −7 .. +6.38 V (44.1 kHz range)** | **+0.0098246 cents** at `v = +5.0533` | same, restricted grid |
| **Worst relative error** | **5.765332e-06** | same sweep |
| **`kVcoFreqC4` float-rounding offset** | **+0.0000685 cents, fixed** (`261.6256103515625` vs decimal `261.6256`) | direct |
| **Float volt-domain summation error** | **+0.0011057 cents** worst | `pitchCV + coarse + fine/12` in float vs double, over a 121×101×21 grid |

**Verdict on the contradiction:** `.planning/research/STACK.md:53` ("~1e-6 relative, ≪0.002 cents") is optimistic by ~5×. `.planning/research/PITFALLS.md:94` ("~1e-4 relative, ≈0.1 cent") is pessimistic by ~17×. **Neither may be cited.** The measured figure is **5.8e-6 relative ≈ 0.010 cents end-to-end**.

### The 1-cent error budget

| Contributor | Worst contribution (cents) | Cumulative? |
|-------------|---------------------------|-------------|
| `exp2_taylor5` polynomial | 0.0049 | no (per-sample, bounded) |
| Float rounding of the summed volts | 0.0011 | no |
| `kVcoFreqC4` float representation | 0.0000685 | no (fixed offset) |
| Float rounding of `yi * yf` and the `kVcoFreqC4 *` multiply | included in the 0.0101 end-to-end figure | no |
| **Total, MEASURED end-to-end** | **0.0101** | — |
| **Gate** | **1.000** | — |
| **Margin** | **≈ 99×** | — |

The estimator's own error (see §Validation Architecture) is the *larger* term in the test, not the DSP's. That is the correct posture: the gate measures the measurement apparatus more tightly than it measures the oscillator.

---

## Nyquist Policy (PITCH-04 / D-10 / D-11 / D-21)

### The change

`src/dsp/VcoCore.hpp:84`: `constexpr float kVcoNyquistGuardFrac = 0.49f;` → `0.495f`, and the `PROVISIONAL` comment is replaced with settled rationale. **That is the entire code change** — the clamp itself already exists at lines 200-233, is already correctly ordered (ceiling first, NaN-safe floor last, CR-01), and already sanitizes a non-positive/NaN `sampleRate` (WR-06). [VERIFIED: read the header]

### Derived ceiling and clamp-crossover volts (MEASURED)

| Sample rate | `maxFreq` @ 0.495 | Tracking breaks above | `maxFreq` @ 0.49 (today) | Crossover @ 0.49 |
|-------------|-------------------|----------------------|--------------------------|------------------|
| 44100 Hz | 21829.500 Hz | **+6.382632 V** | 21609.000 Hz | +6.367985 V |
| 48000 Hz | 23760.000 Hz | **+6.504887 V** | 23520.000 Hz | +6.490240 V |
| 96000 Hz | 47520.000 Hz | **+7.504887 V** | 47040.000 Hz | +7.490240 V |
| 192000 Hz | 95040.000 Hz | +8.504887 V | 94080.000 Hz | +8.490240 V |

Crossover volts are `log2(kVcoNyquistGuardFrac * sampleRate / kVcoFreqC4)`. **D-21 requires the test to derive this, not hardcode it** — the one-line form is `const double vCeil = std::log2((double)forge::kVcoNyquistGuardFrac * sr / (double)forge::kVcoFreqC4);`. [VERIFIED: local measurement]

### Blast radius of the constant change (MEASURED by repo-wide grep)

| Site | Kind | Action |
|------|------|--------|
| `src/dsp/VcoCore.hpp:84` | the definition | change value + rewrite comment |
| `src/dsp/VcoCore.hpp:201` | `maxFreq = kVcoNyquistGuardFrac * safeRate` | **symbolic — follows automatically** |
| `tests/test_vco_core.cpp:347` | broken-control mirror, `forge::kVcoNyquistGuardFrac * in.sampleRate` | **symbolic — follows automatically** |
| `tests/test_vco_core.cpp:736` | scenario four's `expectedMaxFreq` | **symbolic — follows automatically** |
| `src/dsp/VcoCore.hpp:214` | comment literal `tel.freqHz = -21609.00` | **stale prose** — a *historical measurement* under the old constant; leave or annotate, do not silently "update" a recorded observation |
| `tests/test_vco_core.cpp:636` | comment literal `freqHz = -21609.00` | same |
| `src/dsp/VcoCore.hpp:249-250` | comment reasoning "0.49 plus float rounding" vs `kVcoMaxDeltaPhase = 0.5` | **must be re-checked:** at 0.495 the coupled-rate increment maximum rises to 0.495, so 0.5 still clears it but by only ~1 %, not the ~2 % the comment claims. The comment's *number* must be corrected; the constant must not (D-12). |

**No golden, no `FROZEN.sha256` entry, no CI file and no Makefile line references this constant.** [VERIFIED: `grep -rn "kVcoNyquistGuardFrac\|21609\|0\.49" src tests tools docs .github`]

> ⚠ **The `src/dsp/VcoCore.hpp:249-250` comment is the one non-obvious consequence of D-11.** It argues 0.5 was chosen because it "clears that maximum by roughly two percent". At `0.495` the margin becomes ~1.0 %. The constant stays (D-12 is explicit), but leaving the arithmetic claim uncorrected reproduces exactly the "false comment" class that plan 30-08 existed to remove.

### Low end (D-13)

No floor is added. `if (!(freq > 0.f)) freq = 0.f;` stays as the last writer. MEASURED consequence at `pitchVolts = −64`: `freq = 1.418e-17 Hz` → `deltaPhase ≈ 3.2e-22` → the accumulator advances by a denormal-scale amount; the output is effectively DC. That is D-13's stated intent, not a defect.

---

## Signal-Chain Ordering (the contract this phase implements)

```
 1. pitchVolts = in.pitchCV + in.coarse + in.fine * (1/12)        // volts / octaves
 2. if (in.fmConnected) pitchVolts += in.fmVolts * in.fmAtten     // 1.0 oct/V at full CW (D-06)
 3. BOUND pitchVolts to [-kVcoMaxPitchVolts, +kVcoMaxPitchVolts]  // D-14, NaN-rejecting, negated idiom
 4. freq = kVcoFreqC4 * exp2_taylor5(pitchVolts)                  // THE SINGLE exp2 (D-01)
 5. safeRate = (in.sampleRate > 0.f) ? in.sampleRate : 0.f        // EXISTING, unchanged (WR-06)
 6. maxFreq  = kVcoNyquistGuardFrac * safeRate                    // EXISTING, constant value changes
 7. if (freq > maxFreq) freq = maxFreq;                           // EXISTING — DO NOT REORDER (CR-01)
 8. if (!(freq > 0.f)) freq = 0.f;                                // EXISTING — must stay LAST writer
 9. tel.freqHz = freq;                                            // EXISTING
10. deltaPhase = (double)freq * (double)in.sampleTime;            // EXISTING, double (PITCH-05)
11. NaN-safe floor + kVcoMaxDeltaPhase bound + single-subtract wrap  // EXISTING, DO NOT TOUCH (D-12)
12. morphedWave / x5 / return                                     // EXISTING, Phase 32/34 own these
```

**Steps 1-4 are new; steps 5-12 already exist verbatim.** The only edit inside the existing block is the *value* of the constant used at step 6. [VERIFIED: read `src/dsp/VcoCore.hpp:166-281`]

### Why the order is what it is

- **Steps 1-2 before step 4** is FM-03 and D-01 literally: summing in the exponent domain and exponentiating once is what makes FM *musical exponential FM*. Multiplying frequencies or calling `exp2` twice produces linear-FM behavior or per-octave detuning. [CITED: `.planning/research/PITFALLS.md:100`; `.planning/research/ARCHITECTURE.md:190-206`]
- **Step 3 between the sum and the exp** is the only correct placement for D-14: the guard exists to protect `(int32_t)x`, so it must be the last thing before that cast is reached, and it must be after FM (which is the term that introduces unsanitized cable voltage).
- **Steps 7-8 must not swap.** MEASURED at `sampleRate = -44100`: with the floor first, `maxFreq` is negative, the ceiling overwrites the sanitized value, and the accumulator runs unbounded downward — observed `tel.freqHz = -21609.00`, `phase = -9800.00`, `|out| = 1.476e38 V`. [CITED: `src/dsp/VcoCore.hpp:203-231`, an existing recorded measurement]

### The LFO is the counter-example, not the template

```cpp
// src/dsp/LfoCore.hpp:181-187 — the SHIPPED module. Do NOT copy this shape.
if (in.fmConnected) {
    float depthScale = isClocked ? 0.5f : 0.6f;
    float fmPitch = in.fmCV * in.fmAtten * depthScale;
    freq *= exp2_taylor5(fmPitch);          // MULTIPLIES a frequency AFTER pitch is resolved
    freq = std::fmax(freq, 0.001f);
}
```
Borrow **only** the `if (in.fmConnected)` gate (D-09). The `freq *=` shape, the `depthScale` constants (`0.5`/`0.6`) and the `std::fmax` floor are all explicitly rejected by D-06 / D-01 / D-13. [VERIFIED: read `src/dsp/LfoCore.hpp:150-190`]

---

## The D-14 Pitch-Volt Bound — measured envelope and recommended constant

### The UB, measured

`forge::exp2Floor` is well-defined only while `x + 127` lands in `[0, 255]`, i.e. `x ∈ [−127, +128]`. Outside that, `(int32_t)x` overflows (or is NaN) and `xi << 23` shifts a negative or over-large int. MEASURED outputs of `forge::exp2_taylor5(x)`:

| `x` | `xi = (int)(x+127)` | shift well-defined | `exp2_taylor5(x)` returns |
|-----|--------------------|--------------------|---------------------------|
| NaN | UB | — | `nan` (and a UBSan float-cast-overflow report) |
| +inf | UB | — | **`-inf`** (sign inverted) |
| −inf | UB | — | `nan` |
| +1e30 | UB | — | **`-inf`** |
| −1e30 | UB | — | `nan` |
| **+200** | 327 | **no** | **`-1.38778e-17`** (positive input → negative result) |
| **−200** | −73 | **no** | **`-7.20576e+16`** |
| **+130** | 257 | **no** | **`-1.17549e-38`** |
| **−130** | −3 | **no** | **`-8.50706e+37`** |
| +128 | 255 | yes | `+inf` (correct: overflow of float) |
| +127 | 254 | yes | `1.70141e+38` |
| −127 | 0 | yes | `0` |
| −126 | 1 | yes | `1.17549e-38` |

UBSan (`clang -fsanitize=undefined`, `-O0`) reports exactly two sites, by name:
```
src/dsp/RackCompat.hpp:106:24: runtime error: nan is outside the range of representable values of type 'int'
src/dsp/RackCompat.hpp:109:11: runtime error: left shift of 2147483647 by 23 places cannot be represented in type 'int32_t'
```
[VERIFIED: local UBSan run this session]

### Reachable musical envelope

| Scenario | Max \|sum\| |
|----------|------------|
| Rack ±12 V cable norm on V/OCT + ±5 oct COARSE + ±1/12 oct FINE + ±12 V FM at 1.0 oct/V | **29.083 V** |
| Common ±10 V cables, same controls | 25.083 V |

[VERIFIED: local computation from D-06's 1.0 oct/V and D-02/D-03's ranges]

### Candidate bounds (MEASURED behavior at the bound)

| Bound | `freq` at `+bound` | finite? | `freq` at `−bound` | `xi` range | shift safe | Margin over 29.08 V | Margin under UB edge (127 V) |
|-------|-------------------|---------|--------------------|------------|-----------|--------------------|------------------------------|
| ±32 V | 1.124e12 | yes | 6.09e-08 | 95..159 | yes | 1.10× — **too tight** | 4.0× |
| ±48 V | 7.364e16 | yes | 9.29e-13 | 79..175 | yes | 1.65× | 2.6× |
| **±64 V** | **4.826e21** | **yes** | **1.418e-17** | **63..191** | **yes** | **2.20×** | **2.0×** |
| ±96 V | 2.073e31 | yes | 3.30e-27 | 31..223 | yes | 3.30× | 1.32× |
| ±120 V | **inf** | **no** | 1.97e-34 | 7..247 | yes | 4.13× | 1.06× |
| ±126 V | **inf** | **no** | 3.08e-36 | 1..253 | yes | 4.33× | 1.01× |

**Recommendation: `constexpr float kVcoMaxPitchVolts = 64.f;`**
- Exactly representable in float (a power of two — no rounding in the comparison).
- 2.20× outside the worst reachable musical sum, so it can never fire on a legitimate patch.
- 2.0× inside the `±127 V` UB boundary — a full octave of headroom on the *bound itself*.
- `freq` stays **finite** at both extremes (4.83e21 and 1.42e-17), so the downstream ceiling/floor operate on real numbers rather than on infinities. `±120`/`±126` are safe but produce `inf` at the top, which is strictly less clean.
- `kVcoMaxDeltaPhase` absorbs the resulting oversized increment as it already does. [VERIFIED: local measurement]

### The guard's shape — must be the negated idiom, not `forge::clamp`

```cpp
// NaN lands on the fallback branch on BOTH lines, exactly like the frequency
// floor at line 233 and the deltaPhase floor at line 259.
// forge::clamp is a comparison ladder: BOTH of its comparisons are false for
// NaN, so a NaN passes straight through it (deferred item 3 / CR-02).
if (!(pitchVolts > -kVcoMaxPitchVolts)) pitchVolts = -kVcoMaxPitchVolts;
if (pitchVolts > kVcoMaxPitchVolts)     pitchVolts =  kVcoMaxPitchVolts;
```
Note the ordering: the **negated** comparison must come first so it is the NaN catcher, and the plain comparison second — a NaN routed through the first line becomes `-64.f`, which then fails `> +64.f` and survives. Reversing them leaves NaN uncaught (`NaN > 64` is false, `!(NaN > -64)` is true — actually still caught; but only the negated line catches it, so it must be present at all). The safest and most auditable form keeps a single negated line as the NaN catcher.

**Behavior at `pitchVolts = -64`:** `freq = 1.418e-17` → the existing `if (!(freq > 0.f))` does **not** fire (it is positive), `deltaPhase ≈ 3.2e-22` → the existing `if (!(deltaPhase > 0.0))` does **not** fire → the accumulator advances by a denormal. Output is a constant. Consistent with D-13.

---

## Param Contracts (verified against the real SDK)

`ParamQuantity` display formula, read from `../Rack-SDK/include/engine/ParamQuantity.hpp:42-49`:
> `displayValue = f(value) * displayMultiplier + displayOffset`, where `f(value) = value` for `displayBase = 0`.

[VERIFIED: local SDK read] — so `displayBase = 0.f` is the linear form, which is the shape every LFO attenuator already uses (`src/AnalogLFO.cpp:203-214`).

| Control | Recommended `configParam` call | Raw range → POD | Tooltip |
|---------|-------------------------------|-----------------|---------|
| COARSE | `configParam(COARSE_PARAM, -5.f, 5.f, 0.f, "Coarse Tune", " oct");` | −5..+5 **octaves** → `in.coarse` | `+2.00000 oct` |
| FINE | `configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine Tune", " cents", 0.f, 100.f);` | −1..+1 **semitones** → `in.fine` | `-14.000 cents` |
| FM DEPTH | `configParam(FM_ATTEN_PARAM, -1.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f);` | −1..+1 → `in.fmAtten` | `-100.00%` .. `+100.00%` |
| FM IN | `configInput(FM_INPUT, "FM");` | volts → `in.fmVolts` | — |

Notes:
- **FINE's raw range is semitones, its display is cents.** This is the only way to satisfy D-05 (POD in semitones, shell forwards raw) *and* D-04 (tooltip in cents) simultaneously. `displayMultiplier = 100` does the conversion inside `ParamQuantity`, never in the shell.
- **Unit-string convention:** leading space for word units (`" oct"`, `" cents"`), none for `"%"`. Matches the shipped LFO (`" Hz"`, `" deg"`, `"%"` at `src/AnalogLFO.cpp:200-212`). [VERIFIED: read `src/AnalogLFO.cpp`]
- **Display precision:** `ParamQuantity::displayPrecision` defaults to `5` significant digits, so the raw tooltips read `+2.0000 oct` rather than D-04's illustrative `+2.00 oct`. D-04's examples are illustrative of *units*, not of digit count. If the operator wants exactly two decimals, that is `q->displayPrecision = 3;` on the returned pointer — **flag as a cosmetic call for the planner, not a requirement.**
- **ctrl/cmd-click reset** returns each knob to `defaultValue = 0.f`, which D-02 relies on for "get back to concert pitch". No extra mechanism needed. [CITED: standard Rack `ParamWidget` behavior]

### Shell wiring (D-17-compliant — no arithmetic)

```cpp
in.coarse      = params[COARSE_PARAM].getValue();
in.fine        = params[FINE_PARAM].getValue();
in.fmAtten     = params[FM_ATTEN_PARAM].getValue();
in.fmVolts     = inputs[FM_INPUT].getVoltage();
in.fmConnected = inputs[FM_INPUT].isConnected();
```

> **Deliberate divergence from the shipped LFO shell.** `src/AnalogLFO.cpp:319-320` writes `in.fmCV = in.fmConnected ? inputs[FM_INPUT].getVoltage() : 0.f;` — a conditional in the shell. The VCO shell should **not** copy that: D-17 makes "the shell computes nothing" load-bearing, and Rack already returns `0.f` from `getVoltage()` on an unpatched input, so the conditional buys nothing and moves a decision out of the core where D-09's gate lives. Forward both fields unconditionally; let `VcoCore::step` gate.

---

## Double-Precision Phase & the Phase-32 Interface (PITCH-05)

**PITCH-05 is already satisfied and this phase has no work to do for it beyond non-regression.** [VERIFIED: read the header]

| Element | Location | State |
|---------|----------|-------|
| `double phase = 0.0;` | `src/dsp/VcoCore.hpp:127` | present, per-instance (not static) |
| `double deltaPhase = (double)freq * (double)in.sampleTime;` | line 258 | both operands cast to double **before** the multiply |
| NaN-safe negated floor on `deltaPhase` | line 259 | present |
| `kVcoMaxDeltaPhase` bound | line 260 | present, **do not touch (D-12)** |
| Single-subtract wrap `if (phase >= 1.0) phase -= 1.0;` | line 262 | present, valid for any increment in `[0,1)` |
| `const float p = (float)phase;` | line 264 | the **only** float narrowing, and it is downstream of everything Phase 32 needs |

**What Phase 32 will need from this, and what this phase must not break.** PolyBLEP placement needs the sub-sample crossing fraction `frac = (phase - 1.0) / deltaPhase` computed *in double* at the wrap. Today the wrap discards that information (`phase -= 1.0` with no fraction captured) — that is Phase 32's job to add, and adding it is a pure extension of the existing double accumulator. This phase's obligation is narrow: **do not introduce a float round-trip anywhere between `freq` and `phase`.** In particular, `deltaPhase` must keep both casts (`(double)freq * (double)in.sampleTime`); writing `(double)(freq * in.sampleTime)` would compute the product in float and quantize the crossing fraction to ~6e-8 — Pitfall 2 item 2, and precisely the residual noise floor Phase 32 would then be unable to remove. [CITED: `.planning/research/PITFALLS.md:94-110`]

The new pitch expression is all **float**, and that is correct: it lives entirely upstream of the `(double)freq` cast, and the MEASURED float summation error is 0.0011 cents. Promoting the pitch sum to double would buy 0.001 cents at the cost of diverging from the `float`-typed POD fields for no measurable gain.

---

## Architecture Patterns

### System Architecture Diagram

```
 [Rack engine]
      |
      |  ProcessArgs{sampleRate, sampleTime}, params[], inputs[]
      v
 +--------------------------------------------------------------+
 | src/AnalogVCO.cpp :: AnalogVCO::process()   -- NO ARITHMETIC  |
 |                                                               |
 |   V/OCT jack ---------------------> in.pitchCV                |
 |   COARSE knob (raw, octaves) -----> in.coarse                 |
 |   FINE knob   (raw, semitones) ---> in.fine                   |
 |   FM jack voltage ----------------> in.fmVolts                |
 |   FM jack isConnected() ----------> in.fmConnected            |
 |   FM DEPTH knob (raw, -1..+1) ----> in.fmAtten                |
 |   MORPH / CHARACTER knobs --------> in.morph / in.character   |
 |   args.sampleTime / sampleRate ---> in.sampleTime/.sampleRate |
 +----------------------------|---------------------------------+
                              |  forge::VcoInputs (POD, by const&)
                              v
 +--------------------------------------------------------------+
 | src/dsp/VcoCore.hpp :: VcoCore::step()   -- ALL ARITHMETIC    |
 |                                                               |
 |   pitchCV --+                                                 |
 |   coarse ---+--> [ SUM in the VOLT domain ]                   |
 |   fine/12 --+          |                                      |
 |                        |     fmConnected? --+                 |
 |   fmVolts * fmAtten ---+---> (yes) --------+                  |
 |                        |     (no) ---------+                  |
 |                        v                                      |
 |            [ D-14 BOUND +/-kVcoMaxPitchVolts ]  <-- NaN-safe  |
 |                        |            (guards the frozen        |
 |                        |             (int32_t) cast below)    |
 |                        v                                      |
 |            [ ONE call: forge::exp2_taylor5 ] ---> FROZEN      |
 |                        |                          RackCompat  |
 |                        v                                      |
 |            freq = kVcoFreqC4 * 2^volts                        |
 |                        |                                      |
 |            [ ceiling: kVcoNyquistGuardFrac * safeRate ]       |
 |            [ floor  : negated, LAST writer ]  <-- CR-01 order |
 |                        |                                      |
 |                        +---> tel.freqHz  (Phase 35 display)   |
 |                        v                                      |
 |            deltaPhase = (double)freq * (double)sampleTime     |
 |            [ negated floor ][ kVcoMaxDeltaPhase bound ]       |
 |                        v                                      |
 |            double phase accumulator + single-subtract wrap    |
 |                        |                                      |
 |                        +---> (Phase 32 will take the          |
 |                        |      sub-sample crossing fraction    |
 |                        |      from HERE, in double)           |
 |                        v                                      |
 |            [ frozen Waveshape::morphedWave ] --> x5 --> out   |
 +--------------------------------------------------------------+
                              |
                              v
                    outputs[OUTPUT].setVoltage()

 --- test path, no Rack anywhere ---
 tests/VcoBlockDriver.hpp --(injects timing, seeds)--> VcoCore::step()
        |                                                    |
        |                                          returned samples
        v                                                    v
 tests/test_vco_*.cpp  <-- interpolated rising zero crossings (D-19 tier 1)
        |             <-- tel.freqHz                         (D-19 tier 2)
        +--> compared against  261.6256 * std::exp2(v)  (libm, D-18)
```

### Recommended Project Structure

```
src/
├── dsp/
│   ├── VcoCore.hpp          # MODIFIED: pitch expression, kVcoNyquistGuardFrac value,
│   │                        #   new kVcoMaxPitchVolts. Boundary shape UNCHANGED.
│   ├── RackCompat.hpp       # FROZEN — read exp2_taylor5, never edit
│   ├── Waveshape.hpp        # FROZEN — called, never edited
│   └── LfoCore.hpp          # SHIPPED — read for the fmConnected gate only, never edit
├── AnalogVCO.cpp            # MODIFIED: +3 configParam, +1 configInput, +5 POD assignments,
│                            #   +4 widget placements. NO arithmetic.
├── AnalogLFO.cpp            # MUST NOT APPEAR IN THIS PHASE'S DIFF (D-16)
└── vco_compile_canary.cpp   # unchanged (no new VCO header this phase)
res/
└── AnalogVCO.svg            # MODIFIED: +4 marker rects (throwaway)
tests/
├── test_vco_pitch.cpp       # NEW (recommended): TEST-02 gate + D-14 RED case
├── test_vco_core.cpp        # unchanged, or extended (planner's call, D-discretion)
├── VcoBlockDriver.hpp       # unchanged
└── check_includes.sh        # +1 exact-path VCO_SIDE_ALLOW entry IF a new test file lands
```

### Pattern 1: Negated-comparison guard for non-finite input

**What:** Write defensive comparisons so the NaN case lands on the *fallback* branch, not the pass-through branch.
**When to use:** Every guard in `VcoCore` that must survive a hostile cable voltage. **Never** use `forge::clamp` for this.
**Example:**
```cpp
// Source: src/dsp/VcoCore.hpp:233 and :259 (existing, shipped in Phase 30)
if (!(freq > 0.f)) freq = 0.f;                 // NaN fails `> 0.f`, so it lands at 0
if (!(deltaPhase > 0.0)) deltaPhase = 0.0;     // same idiom, same reason

// The D-14 guard follows the SAME idiom:
if (!(pitchVolts > -kVcoMaxPitchVolts)) pitchVolts = -kVcoMaxPitchVolts;
if (pitchVolts > kVcoMaxPitchVolts)     pitchVolts =  kVcoMaxPitchVolts;
```
Contrast with the trap:
```cpp
// Source: src/dsp/RackCompat.hpp:97 — FROZEN, and NaN-TRANSPARENT.
inline float clamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
// NaN < lo  -> false
// NaN > hi  -> false
// => NaN is returned UNCHANGED. Inert against exactly the input class it looks like it stops.
```

### Pattern 2: Namespace-scope `constexpr`, never in-class `static constexpr`

**What:** Any new constant goes at namespace scope inside `namespace forge`, as a plain `constexpr`.
**When to use:** `kVcoMaxPitchVolts`, and any future constant in a VCO header.
**Example:**
```cpp
// Source: src/dsp/VcoCore.hpp:80-95 (the existing idiom, and its own banner's mandate)
namespace forge {
constexpr float kVcoFreqC4 = 261.6256f;
constexpr float kVcoNyquistGuardFrac = 0.495f;
constexpr float kVcoMaxPitchVolts = 64.f;     // <-- the new one goes here
constexpr double kVcoMaxDeltaPhase = 0.5;
```
**Never:** `inline constexpr` (C++17 inline variables), and never an in-class `static constexpr` array indexed at runtime — that construct is a declaration-only under C++11, odr-used by the indexing, and produces a MinGW `undefined reference`. **That exact class got v2.0.0 rejected from the VCV Library.** [CITED: `.planning/research/PITFALLS.md:185-200`; `src/dsp/VcoCore.hpp:55-60`]

### Pattern 3: Independent ground truth in the test, never self-comparison

**What:** The gate computes its expectation with a *different* implementation from the one under test.
**Example:**
```cpp
// D-18. libm is banned in src/ for bit-identity; using it HERE is what makes
// the assertion independent rather than vacuous.
const double expected = 261.6256 * std::exp2((double)volts);
const double measured = estimateFreqRising(out, sr, &nUp);
const double cents    = 1200.0 * std::log2(measured / expected);
CHECK(std::fabs(cents) < kTrackingToleranceCents);
```
**Anti-example, explicitly rejected by D-18:**
```cpp
// VACUOUS: compares the implementation to itself. Stays green if exp2_taylor5
// is replaced with `return 1.f;`.
const double expected = forge::kVcoFreqC4 * forge::exp2_taylor5(volts);
```

### Anti-Patterns to Avoid

- **`freq *= exp2_taylor5(fmPitch)` (the LFO's shape).** Produces per-octave detuning / linear-FM behavior, not exponential FM. Explicitly forbidden by D-01 / FM-03.
- **Two `exp2` calls in the chain.** Same defect, harder to see.
- **`std::exp2` / `std::pow` anywhere in `src/`.** Breaks the bit-identity landmine that PITCH-01 names.
- **`forge::clamp` for the D-14 bound.** Inert against NaN — the one input class it exists to stop.
- **Reordering the two frequency-guard lines.** MEASURED catastrophic (`|out| = 1.476e38 V`). CR-01.
- **Touching `kVcoMaxDeltaPhase`.** D-12; it is a wrap-correctness bound, not a Nyquist bound.
- **A tolerance that widens with samples-per-cycle.** D-20 rejects it explicitly; a moving tolerance is a gate wider than the prose it encodes.
- **Computing anything in `src/AnalogVCO.cpp`.** D-17; the headless suite stops describing the module the moment arithmetic appears there.
- **Multi-line `struct VcoCore {` or `float step(...) {`.** Hard-fails `make guards` — see §Common Pitfalls, Pitfall 5.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| `2^volts` | A new exp approximation, a table, `std::exp2` | `forge::exp2_taylor5` (`RackCompat.hpp:112`) | MEASURED 0.0101 cents end-to-end — 99× under the gate. Anything else is a shared-header edit (guardrail event) or a bit-identity break (Pitfall 2). |
| NaN-safe bounding | `forge::clamp`, `std::clamp`, `fmin`/`fmax` chains | The negated-comparison idiom already in the file | `forge::clamp` is MEASURED NaN-transparent. `std::clamp` is C++17 — banned. `fmin`/`fmax` would work but diverge from the established, reviewed idiom. |
| Ground truth for the gate | A hand-rolled cents formula against `exp2_taylor5` | `1200 * log2(measured / (261.6256 * std::exp2(v)))` with libm | Self-comparison is the vacuous-coverage trap D-18 names by name. |
| Frequency estimation from samples | Integer crossing counts; FFT/DFT peak-picking | The existing sub-sample-interpolated rising-crossing estimator (`tests/test_vco_core.cpp:135-149`) | Integer counting carries a MEASURED −2.15 % quantization error at 250 ms — ~37× the whole 1-cent budget. A DFT would need a helper this phase does not need (Phase 32 owns spectral work). |
| Block driving / timing injection | A new loop in the test | `forge::VcoBlockDriver` (`tests/VcoBlockDriver.hpp`) | Already injects `sampleTime`/`sampleRate` unconditionally and seeds non-degenerately. Rebuilding it invites the exact bugs its banner documents. |
| Param display conversion | Arithmetic in the shell to turn semitones into cents | `configParam(..., displayMultiplier = 100.f)` | `ParamQuantity` owns display math (verified formula). Doing it in the shell would be DSP in `AnalogVCO.cpp` — D-17 violation. |
| Clamp-boundary constant in the test | Hardcoded `+6.38f` | `std::log2(kVcoNyquistGuardFrac * sr / kVcoFreqC4)` | D-21: derived, so the test cannot silently diverge from the constant. |

**Key insight:** essentially every primitive this phase needs already exists in-tree, already ships, and is already pinned by a guard. The phase's real work is *composition plus evidence*, and every hand-rolled substitute listed above trades a guarded, measured primitive for an unguarded, unmeasured one.

---

## Common Pitfalls

### Pitfall 1: The D-14 behavioral RED case is MEASURED vacuous — today's core already survives every hostile pitch input

**What goes wrong:** The plan writes a RED-first case asserting "with `pitchCV = NaN`, the output stays finite and `tel.freqHz` is 0", observes it **green** before the fix, and either (a) concludes the guard is unnecessary, or (b) lands the guard on a case that never went red — the exact class of evidence Phases 29 and 30 both rejected.

**Why it happens:** The existing negated frequency floor (`if (!(freq > 0.f)) freq = 0.f;`) catches the *garbage result* of the UB. MEASURED this session, 4096 steps each at 44.1 kHz, `morph = 0.5`, `character = 1.0`:

| `in.pitchCV` | `exp2_taylor5` returns | `tel.freqHz` | `max|out|` | `allFinite` | `phase` |
|--------------|----------------------|--------------|-----------|-------------|---------|
| NaN | `nan` | 0 | 5.0 V | **true** | 0 |
| +inf | `-inf` | 0 | 5.0 V | **true** | 0 |
| −inf | `nan` | 0 | 5.0 V | **true** | 0 |
| ±1e30 | `-inf` / `nan` | 0 | 5.0 V | **true** | 0 |
| ±130 | `-1.175e-38` / `-8.507e37` | 0 | 5.0 V | **true** | 0 |
| ±200 | `-1.388e-17` / `-7.206e16` | 0 | 5.0 V | **true** | 0 |

Every behavioral assertion a reasonable person would write is already satisfied.

**How to avoid:** Use one of the three tiers below; the plan should pick one and say why.

| Tier | RED signal | Strength | Cost |
|------|-----------|----------|------|
| **A. UBSan (RECOMMENDED)** | `clang -fsanitize=undefined` reports `RackCompat.hpp:106` and `:109` by name. VERIFIED red this session. | **Strongest — it is the defect itself, not a proxy.** | A one-shot demonstration or a narrowly-scoped `make` target. **Must not become a permanent repo-wide gate** — see Pitfall 3. |
| **B. Pitch-volt telemetry** | Add `float lastPitchVolts` to `VcoCore::Telemetry`; assert `std::isfinite(tel.lastPitchVolts) && fabs(...) <= kVcoMaxPitchVolts`. Red today because the field does not exist (add field + failing assertion first, then the guard). | Medium. Telemetry is the *right* observable here — the property genuinely is "what value reached the function" — but D-19's warning about telemetry weakness will invite a reviewer challenge. | One new telemetry field; a two-step RED. |
| **C. Direct unit test of the guard's arithmetic** | Test a small helper in isolation. | **Weakest — tests the fix, not the defect.** Do not use alone. | Low. |

**Warning signs:** A RED-first task whose "before" state is green; a plan that asserts `allFinite` as the D-14 evidence; a phrase like "the guard prevents a crash" (it does not — there is no crash to prevent on this toolchain today; there is UB whose *manifestation* is toolchain-dependent).

---

### Pitfall 2: The crossing estimator has a hard resolution floor at ~2 samples/cycle, and it fails *silently and plausibly*

**What goes wrong:** The TEST-02 sweep runs one octave too high and reports a −11.5-cent error that looks like a pitch bug. Or, worse, the plan widens the tolerance to accommodate it and the gate stops being evidence.

**Why it happens:** Near two samples per cycle the sampled waveform alternates sign at roughly `sr/2`, so the rising-crossing count saturates and no longer tracks the true frequency at all. MEASURED at `morph = 0`, `character = 0`:

| Sample rate | Highest point measured GOOD | samples/cycle there | cents error | First point measured BROKEN | samples/cycle | cents error |
|-------------|----------------------------|--------------------|-------------|----------------------------|---------------|-------------|
| 44100 | `v = +6.0` (16744 Hz) | **2.634** | −0.0026 | `v = +6.5` (also above clamp) | 1.862 | −158.4 |
| 48000 | `v = +6.0` (16744 Hz) | **2.867** | −0.0087 | `v = +6.5` (23680 Hz, **below** the clamp) | **2.027** | **−11.61** |
| 96000 | `v = +7.0` (33488 Hz) | **2.867** | −0.0024 | `v = +7.5` (47359 Hz, **below** the clamp) | **2.027** | **−11.58** |

**The 48 kHz `v = +6.5` row is the trap:** 23679.6 Hz is *below* that rate's 23760 Hz ceiling, so D-21's clamp-derived boundary does **not** exclude it — yet the estimator is already broken there. The clamp boundary and the estimator boundary are two different limits and the test needs both.

**How to avoid:** Bound the sweep by **samples-per-cycle ≥ 2.5** in addition to D-21's derived clamp boundary, and take whichever is lower at each rate. `2.5` sits 1.23× above the first measured-broken point (2.027) and just below the last measured-good point (2.634), so every measured-good point survives the cutoff and every measured-broken one is excluded. Equivalently: `f_expected <= sampleRate / 2.5`.

**Warning signs:** A cents error in the ±10 range (not ±0.01 and not ±100) at exactly one grid point per rate; a `nUp` value that is identical across several adjacent pitches (MEASURED: 5879 at 48 kHz for `v ∈ {+6.5, +7.0, +7.5, +8.0}` — a saturation signature).

---

### Pitfall 3: A permanent repo-wide UBSan gate would light up the SHIPPED LFO

**What goes wrong:** Tier A of Pitfall 1 is so convincing that the plan wires `-fsanitize=undefined` into `make test` or CI. The suite immediately reports UB in `RackCompat.hpp:106/109` reached through the *LFO's* FM path, and the phase is now staring at a guardrail event it did not sign up for.

**Why it happens:** The shipped LFO has the identical latent UB. `src/AnalogLFO.cpp:320` reads `in.fmCV = in.fmConnected ? inputs[FM_INPUT].getVoltage() : 0.f;` — an **unsanitized cable voltage** — and `src/dsp/LfoCore.hpp:183-184` feeds `in.fmCV * in.fmAtten * depthScale` straight into `exp2_taylor5`. A hostile or non-finite cable voltage into the LFO's FM jack reaches the same `(int32_t)` cast. [VERIFIED: read both files]

**How to avoid:**
- Scope any UBSan run to a **VCO-only translation unit** (a standalone probe over `VcoCore`), never the whole `tests/*.cpp` glob.
- If a `make` target is added, name and document it as a one-shot VCO diagnostic, not a gate.
- **Do not "fix" `RackCompat.hpp`.** It is byte-pinned by `tests/check_frozen.sh` and consumed by the shipped LFO — D-14's own binding constraint says so.
- **Do surface the LFO finding** in the phase's deferred items so it is recorded rather than rediscovered: *the shipped LFO shares this latent UB; it is unfixed by decision, out of scope for Phase 31, and any fix is a guardrail event requiring operator sign-off and a golden re-verification.*

**Warning signs:** `FROZEN.sha256` appears in the phase diff; `make guards` fails at `check_frozen.sh`; a plan task phrased as "harden `exp2Floor`".

---

### Pitfall 4: `morph` choice silently changes the gate's own noise floor by 100×

**What goes wrong:** TEST-02 is written at the harness's habitual `morph` sweep or at `morph = 0.5`, the measured errors come in around 0.1 cents instead of 0.005, and the tolerance gets loosened "because that's what it measures."

**Why it happens:** The estimator's linear sub-sample interpolation is near-exact through a sine's zero crossing and progressively worse through the morphed shapes' kinked crossings. MEASURED, same grid, same windows:

All rows below are 48 kHz, `character = 0`, identical windows:

| pitch | `morph = 0` cents err | `morph = 0.5` cents err | ratio |
|-------|----------------------|-------------------------|-------|
| +1.5 V | +0.00011 | −0.07733 | ~700× |
| +2.0 V | −0.00003 | −0.08333 | ~2800× |
| +3.0 V | −0.00022 | −0.10500 | ~480× |
| +3.5 V | +0.00062 | **+0.11506** | ~185× |
| +5.0 V | −0.00187 | +0.04179 | ~22× |
| +6.0 V | −0.00871 | +0.02342 | ~2.7× |

`morph = 0.5` still clears 1 cent comfortably — but the margin drops from ~100× to **~8.7×** (worst measured 0.11506), and the *measured quantity* stops being "does the oscillator play the right note" and starts being "how kinked is the waveform at its zero crossing."

**How to avoid:** Run the **primary** TEST-02 tier at `morph = 0.f, character = 0.f` (pure `sin(2π·phase)`, VERIFIED at `src/dsp/Waveshape.hpp:39-51` — `character < 0.001f` returns the bare sine). Optionally add a **secondary** pass at one or two other morph values with the *same* fixed tolerance, recorded as timbre-robustness rather than as the tracking gate. Never let the morph value drive the tolerance.

**Warning signs:** A tolerance above ~0.2 cents; a comment justifying the tolerance in terms of waveform shape rather than in terms of the oscillator.

---

### Pitfall 5: The `check_canary.sh [2b/5]` source-shape contract hard-fails on ordinary formatting

**What goes wrong:** The `struct VcoCore` line or the `float step(...)` line gets reformatted (Allman brace, wrapped parameter list, added `noexcept`) and `make guards` fails with `could not perturb src/dsp/VcoCore.hpp` — a *guard* error that reads like a DSP error.

**Why it happens:** `tests/check_canary.sh [2b/5]` line-matches both patterns to build a perturbed copy of the header. Worse, **the step matcher is UNANCHORED**, so writing the full `step()` signature in a *comment* on a line that also contains `{` makes the canary perturb the comment and fail with unrelated compile errors (`unknown type name VcoInputs`). This is recorded in the file's own banner at lines 20-29 and was observed, not theorized. [VERIFIED: read `src/dsp/VcoCore.hpp:20-29` and `tests/check_canary.sh:124-158`]

**How to avoid:** Keep `struct VcoCore {` and `float step(const VcoInputs& in) {` each on one line with the opening brace. In new comments, abbreviate as `step(...)`. Run `make guards` after every header edit, not only at the end.

**Warning signs:** `make guards` reporting `[2b/5]` failure, or compile errors naming `VcoInputs` in a scratch directory path.

---

### Pitfall 6: A new `tests/*.cpp` VCO file fails `make guards` on its first run

**What goes wrong:** `tests/test_vco_pitch.cpp` lands, `make test` is green, and `make guards` exits 1 at `[1/7] No LFO translation unit includes a VCO file`.

**Why it happens:** `tests/check_includes.sh [1/7]` derives its LFO-side scan set as *everything* under `src/`, `tests/` and `tools/` **minus** a hardcoded exact-path allowlist. A new VCO test TU is LFO-side by default and its `#include "VcoBlockDriver.hpp"` is a hit by construction. The current allowlist is exactly five entries:
```bash
# tests/check_includes.sh:279-285
VCO_SIDE_ALLOW=(
	"src/vco_compile_canary.cpp"
	"src/AnalogVCO.cpp"
	"tests/VcoBlockDriver.hpp"
	"tests/test_vco_harness.cpp"
	"tests/test_vco_core.cpp"
)
```
This exact thing happened in Phase 30 with `tests/test_vco_core.cpp`. [VERIFIED: read the script; STATE.md Phase 30 entry]

**How to avoid:** If a new test file is chosen, make the `VCO_SIDE_ALLOW` addition an **explicit plan task with its own rationale**, not a fix discovered at gate time. Note Phase 30's own record: that edit "weakens no detector" (the match is `[[ "${rel}" == "${a}" ]]` — a quoted RHS, so literal comparison: no glob, no substring, no basename) **but it was NOT operator-checkpointed and was flagged at the phase gate.** Surfacing it up front closes that loop. Extending `tests/test_vco_core.cpp` instead avoids the edit entirely.

**Warning signs:** `make guards` red on `[1/7]` immediately after a new test file lands.

---

### Pitfall 7: Reasoning about C++11 compatibility from a green local build

**What goes wrong:** The code compiles under Apple clang at `-std=c++17` (what `make test` uses), passes review, and fails the VCV Library's `-std=c++11` GCC/MinGW leg.

**Why it happens:** `make test` is `-std=c++17` and never links the shell; local clang masks C++17-isms and materializes the ODR-hazard construct as a TU-local symbol. Only the CI MinGW **link** leg catches the ODR class — `make strict` is `-fsyntax-only` and cannot. [CITED: `.planning/research/PITFALLS.md:185-200`; STATE.md Phase 29 entry, where the entire local gate returned exit 0 on a commit that could not link]

**How to avoid for this phase specifically:** the new code is small and the risks are narrow —
- `constexpr float kVcoMaxPitchVolts = 64.f;` at namespace scope: **C++11-legal.** Not `inline constexpr`.
- `in.fine * (1.f / 12.f)`: plain float arithmetic, legal.
- No `std::clamp`, no `if constexpr`, no `[[maybe_unused]]`, no structured bindings, no nested-namespace syntax, no auto return deduction.
- No constant tables of any kind — this phase needs none, and a semitone/coarse table is exactly the construct that got v2.0.0 rejected. [CITED: `.planning/research/PITFALLS.md:190`]
- `configParam` with default trailing args in `src/AnalogVCO.cpp`: matches the shipped LFO's usage, C++11-legal.

Run `make strict` (needs `../Rack-SDK`, present) after every `src/` edit. Do not tag or resubmit on local evidence — but that is Phase 36's problem, not this phase's.

**Warning signs:** `make strict` errors; MinGW CI `undefined reference`; any new `static constexpr` inside a struct.

---

### Pitfall 8: Updating a recorded historical measurement as if it were a stale value

**What goes wrong:** The `kVcoNyquistGuardFrac` change prompts a search-and-replace over `21609`, and two comments that record *what was observed under the old constant* get rewritten to numbers nobody measured.

**Why it happens:** `src/dsp/VcoCore.hpp:214` and `tests/test_vco_core.cpp:636` both contain `tel.freqHz = -21609.00` as part of a MEASURED CR-01 reproduction narrative. Those are historical observations, not current expectations.

**How to avoid:** Leave the recorded figures intact; if clarity demands it, annotate (`measured under the pre-Phase-31 0.49 constant`). Only `src/dsp/VcoCore.hpp:249-250`'s **arithmetic claim** about the 0.49-vs-0.5 margin is genuinely wrong after the change and must be corrected (see §Nyquist Policy).

**Warning signs:** A diff that changes a number inside a paragraph beginning "MEASURED, ...".

---

## Code Examples

### The complete pitch block (drop-in replacement for `src/dsp/VcoCore.hpp:171-175`)

```cpp
// Source: composed from src/dsp/VcoCore.hpp:171-175 (existing) + D-01/D-06/D-09/D-14.
// D-14 pitch: coarse/fine/FM summed in the VOLT domain, bounded, then ONE
// exp2 off C4 = 0 V using the frozen forge::exp2_taylor5 -- NEVER libm
// std::exp2/std::pow (bit-identity landmine, Pitfall 2).
float pitchVolts = in.pitchCV + in.coarse + in.fine * (1.f / 12.f);

// D-06/D-09: full-clockwise attenuverter = 1.0 octave per volt, so the FM jack
// at full depth is a second V/OCT. NOT the LFO's 0.6 (that constant was
// auditioned for sub-audio wobble, a different job).
if (in.fmConnected) pitchVolts += in.fmVolts * in.fmAtten;

// D-14: bound BEFORE the exp. forge::exp2Floor casts with (int32_t)x, which is
// UB for NaN/inf, and shifts with xi << 23, which is UB for a negative or
// over-large xi. Written NEGATED so a NaN lands on the fallback branch --
// forge::clamp would NOT work here, it is a comparison ladder and passes NaN
// straight through (deferred item 3 / CR-02). MEASURED safe envelope of the
// frozen helper is x in [-127, +128]; the worst reachable musical sum is
// +/-29.08 V, so 64 V sits 2.2x outside the reachable range and 2.0x inside
// the UB boundary, and keeps freq finite at both extremes.
if (!(pitchVolts > -kVcoMaxPitchVolts)) pitchVolts = -kVcoMaxPitchVolts;
if (pitchVolts > kVcoMaxPitchVolts)     pitchVolts =  kVcoMaxPitchVolts;

float freq = kVcoFreqC4 * exp2_taylor5(pitchVolts);
```
*(Everything from `const float safeRate = ...` onward is unchanged.)*

### The shell additions (`src/AnalogVCO.cpp`)

```cpp
// Source: pattern from src/AnalogLFO.cpp:197-216; signature VERIFIED against
// ../Rack-SDK/include/engine/Module.hpp:125.
enum ParamId {
    MORPH_PARAM,
    CHARACTER_PARAM,
    COARSE_PARAM,        // new
    FINE_PARAM,          // new
    FM_ATTEN_PARAM,      // new
    PARAMS_LEN
};
enum InputId {
    VOCT_INPUT,
    FM_INPUT,            // new
    INPUTS_LEN
};

// in the constructor:
configParam(COARSE_PARAM,   -5.f, 5.f, 0.f, "Coarse Tune", " oct");
configParam(FINE_PARAM,     -1.f, 1.f, 0.f, "Fine Tune",   " cents", 0.f, 100.f);
configParam(FM_ATTEN_PARAM, -1.f, 1.f, 0.f, "FM Depth",    "%",      0.f, 100.f);
configInput(FM_INPUT, "FM");

// in process() -- assignment only, NO arithmetic (D-17):
in.coarse      = params[COARSE_PARAM].getValue();
in.fine        = params[FINE_PARAM].getValue();
in.fmAtten     = params[FM_ATTEN_PARAM].getValue();
in.fmVolts     = inputs[FM_INPUT].getVoltage();
in.fmConnected = inputs[FM_INPUT].isConnected();
```

### The derived clamp boundary (D-21)

```cpp
// Source: D-21. DERIVED from the constant, never hardcoded, so the test cannot
// silently diverge from src/dsp/VcoCore.hpp if the policy ever moves.
// MEASURED today: +6.382632 V @ 44.1k, +6.504887 V @ 48k, +7.504887 V @ 96k.
static double clampCeilingVolts(double sr) {
    return std::log2((double)forge::kVcoNyquistGuardFrac * sr
                     / (double)forge::kVcoFreqC4);
}

// The estimator ALSO has a limit, and MEASURED it is the tighter one at ALL
// THREE rates (Pitfall 2): reject any point with fewer than 2.5 samples/cycle.
static double estimatorCeilingVolts(double sr) {
    return std::log2((sr / 2.5) / (double)forge::kVcoFreqC4);
}
// Test upper bound at each rate = min(clampCeilingVolts, estimatorCeilingVolts),
// minus a small guard margin.
```

### The existing estimator, reused verbatim

```cpp
// Source: tests/test_vco_core.cpp:135-149. Sub-sample linear interpolation is
// LOAD-BEARING -- the naive crossings/2/duration form carries a MEASURED
// -2.15 % quantization error on a 250 ms window, ~37x the entire 1-cent budget.
double estimateFreqRising(const std::vector<float>& o, double sr, int* nUp) {
    double first = -1.0, last = -1.0;
    int count = 0;
    for (size_t i = 1; i < o.size(); ++i) {
        if (o[i - 1] < 0.f && o[i] >= 0.f) {
            const double frac = (double)(-o[i - 1]) / ((double)o[i] - (double)o[i - 1]);
            const double t = ((double)(i - 1) + frac) / sr;
            if (count == 0) first = t;
            last = t;
            ++count;
        }
    }
    *nUp = count;
    return (count < 2) ? -1.0 : (count - 1) / (last - first);
}
```

### Throwaway panel geometry (suggestion — D-discretion)

Existing durable coords (do not move): MORPH `(30.48, 40)`, CHARACTER `(60.96, 40)`, V/OCT `(30.48, 100)`, OUT `(60.96, 100)` mm on a `91.44 × 128.5` mm panel. `res/AnalogVCO.svg` marks each with a `10×10` rect at `(cx−5, cy−5)`.

```
Suggested additions (three knobs on a new row, one jack between the existing two):
  COARSE     (20.32, 60)    rect x=15.32 y=55
  FINE       (45.72, 60)    rect x=40.72 y=55
  FM DEPTH   (71.12, 60)    rect x=66.12 y=55
  FM INPUT   (45.72, 100)   rect x=40.72 y=95
```
The SVG and the widget coordinates are written together — the panel starts lying about its controls the moment one moves without the other. [VERIFIED: read `res/AnalogVCO.svg` and `src/AnalogVCO.cpp:146-165`]

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Pitch reads `in.pitchCV` alone (`VcoCore.hpp:175`) | Four-term volt-domain sum, bounded, one `exp2` | This phase | PITCH-01/02/03, FM-01/02/03 delivered |
| `kVcoNyquistGuardFrac = 0.49f` marked `PROVISIONAL` | `0.495f` with settled rationale | This phase (D-11) | Ceiling rises 220 Hz at 44.1 kHz; PITCH-04's policy becomes real |
| Pitch verified to 1 % on the output, explicitly *"NOT the TEST-02 gate"* | < 1 cent (0.058 %) across a derived-boundary sweep | This phase (TEST-02) | ~17× tighter; becomes the phase's exit gate |
| Exponent argument is a single param-sourced field Rack pre-sanitizes | Exponent argument includes a raw cable voltage | This phase (FM) | First time the frozen `(int32_t)` cast is reachable from unsanitized input → D-14 |
| VCV Fundamental VCO / Rack SDK reference | `FREQ_C4 = 261.6256f`, `freq = FREQ_C4 · approxExp2_taylor5(pitch)`, minBLEP for AA | Rack 2 (AA switched away from oversampling) | Confirms this phase's pitch law and `exp2_taylor5` choice; the AA divergence is Phase 32's, not this phase's [CITED: `.planning/research/STACK.md:110`] |

**Deprecated/outdated in this repo's own prior research:**
- `.planning/research/PITFALLS.md:94` — "~1e-4 relative ≈ ~0.1 cent" for `exp2_taylor5`. **Superseded by measurement: 5.8e-6 relative ≈ 0.010 cents.** Pessimistic by ~17×.
- `.planning/research/STACK.md:53` — "~1e-6 relative (≪0.002 cents)". **Superseded by the same measurement.** Optimistic by ~5×.
- `.planning/research/ARCHITECTURE.md:330` invariant 1 — suggests "`pitchCV ∈ {−2..+4} V` … within ~0.05 % (a few cents)". That was written as a *skeleton-phase* sanity bound; TEST-02 supersedes it with a wider sweep and a ~17× tighter tolerance.
- `.planning/REQUIREMENTS.md:17` and `.planning/ROADMAP.md:201` — "±2 semitones" for PITCH-03. **Superseded by D-00; both still say ±2 as of this research** (verified by direct read this session).

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `±64 V` is the right numeric bound for D-14 (vs `±48` or `±96`, all of which are also safe) | The D-14 Pitch-Volt Bound | Low. Any value in `[48, 96]` satisfies every stated constraint; the choice is a margin-balancing judgment and is explicitly Claude's discretion per CONTEXT.md. |
| A2 | `2.5` samples/cycle is the right sweep cutoff (MEASURED good at 2.634, broken at 2.027) | Pitfall 2, Validation Architecture | Low-medium. Any cutoff in `[2.3, 3.0]` is defensible; `2.5` was chosen to sit between the two measured points so every measured-good pitch survives and every measured-broken one is excluded. Above ~3.0 the sweep loses a usable top octave for no benefit; below ~2.2 the gate starts measuring the estimator rather than the oscillator. The phase should re-measure at whatever cutoff it picks. |
| A3 | `0.05 cents` is the right fixed tolerance for the primary tier (measured worst 0.0134 at morph 0 within the recommended sweep) | Validation Architecture | Medium. Chosen for ~3.7× margin over the worst measurement while staying 20× under the 1-cent requirement. A tighter number risks cross-toolchain flake; a looser one weakens the gate. **The phase must re-measure and justify whatever it picks, per D-18.** |
| A4 | The suggested throwaway panel coordinates are aesthetically acceptable for a one-phase placeholder | Code Examples | None — the panel is replaced wholesale in Phase 35 and geometry is explicitly Claude's discretion. |
| A5 | The measured figures hold under GCC/libstdc++ and MinGW g++, not only Apple clang | Whole document | Low. Phase 30 closed the identical assumption (STATE.md: "Research assumption A5 is CLOSED — the Apple-clang-only CORE-01/CORE-03 tolerances hold under GCC/libstdc++ and MinGW g++", verified across a 3-OS matrix with `-ffp-contract=off`). The pitch chain uses the same flags and the same frozen helper. Still worth confirming on the 3-OS CI leg rather than asserting. |
| A6 | Rack's ctrl/cmd-click-to-default returns a knob to exactly `defaultValue` (D-02 relies on this) | Param Contracts | Low. Standard `ParamWidget` behavior; not re-verified against the SDK source this session. |
| A7 | `configParam` returning a pointer whose `displayPrecision` can be set is the right way to get D-04's two-decimal formatting, if the operator wants it | Param Contracts | None — cosmetic, and flagged as a question rather than a recommendation. |

---

## Open Questions

1. **Which RED-first tier does D-14 use?**
   - What we know: a behavioral RED case is MEASURED vacuous (Pitfall 1 table). UBSan is MEASURED red at the exact two lines. A telemetry field would also work.
   - What's unclear: whether the operator wants a new `Telemetry` field (a boundary-adjacent change) or a UBSan-based one-shot demonstration (no production surface change, but not a permanent suite check either).
   - Recommendation: **UBSan tier A as the RED evidence, recorded in the phase's verification with the literal diagnostic text**, plus a permanent behavioral case pinned at the *bound* (e.g. `pitchVolts` driven to exactly `±kVcoMaxPitchVolts` produces a finite, in-range output) so the suite carries a standing check even though that check is not the RED one. Surface the choice as a plan decision, not a silent one.

2. **New test file or extend `tests/test_vco_core.cpp`?**
   - What we know: `make test` globs `tests/*.cpp` either way. A new file costs one exact-path `VCO_SIDE_ALLOW` entry in `tests/check_includes.sh` (Pitfall 6). `tests/test_vco_core.cpp` is already 1109 lines.
   - What's unclear: whether the operator wants the `check_includes.sh` edit at all this phase, given Phase 30's record that the analogous edit "was NOT operator-checkpointed".
   - Recommendation: **new file `tests/test_vco_pitch.cpp`**, with the allowlist addition as an explicit, rationale-carrying plan task rather than a gate-time discovery.

3. **Does the operator want D-04's exact `+2.00 oct` / `-14.0 cents` digit count?**
   - What we know: Rack's default `displayPrecision = 5` yields `+2.0000 oct` and `-14.000 cents`. The units are right; the digit count is not what D-04 illustrates.
   - Recommendation: leave the default (matching the shipped LFO, which sets no precision anywhere) and note the divergence. If exactness matters it is one line per param on the returned `ParamQuantity*`.

4. **Should the phase record the shipped LFO's shared latent UB as a deferred item?**
   - What we know: MEASURED — `src/AnalogLFO.cpp:320` feeds an unsanitized cable voltage into the same frozen `(int32_t)` cast via `src/dsp/LfoCore.hpp:183-184`.
   - What's unclear: nothing technical; it is purely a bookkeeping decision.
   - Recommendation: **yes, record it as a deferred item pointed at no phase**, explicitly marked *"guardrail event — requires operator sign-off and golden re-verification; unfixed by decision."* Rediscovering it in Phase 33 or 34 without the reasoning would cost more than writing it down now.

5. **Does the `src/dsp/VcoCore.hpp:249-250` comment correction need its own task?**
   - What we know: the comment's "clears that maximum by roughly two percent" becomes ~1.0 % at `0.495`. The constant itself must not change (D-12).
   - Recommendation: fold it into the same task that changes `kVcoNyquistGuardFrac`, with the arithmetic restated. It is a one-sentence edit but it is exactly the "false comment" class plan 30-08 existed to remove.

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Apple clang (`c++`) | `make test`, `make strict`, guard suite | ✓ | Apple clang (Darwin 23.6.0, arm64) | — |
| VCV Rack 2 SDK at `../Rack-SDK` | `make`, `make strict`, plugin link | ✓ | present (symlink to `/Users/mrcbrown/Claude/Software/Forge Audio/Rack-SDK`); `include/engine/Module.hpp` and `ParamQuantity.hpp` read directly | — |
| GNU Make | all targets | ✓ | system make (Makefile is 3.81-compatible by design) | — |
| doctest 2.4.11 | `make test` | ✓ | vendored at `tests/doctest.h` | — |
| `nm` | `check_canary.sh [2/5]`, `[2b/5]` | ✓ | system | guard degrades explicitly if absent |
| `-fsanitize=undefined` (UBSan) | D-14 RED evidence (recommended tier A) | ✓ | verified working this session; reports both UB sites | Tier B (telemetry field) or tier C (unit test) |
| VCV Rack 2 application | in-Rack operator UAT (every phase 31-35 ends in one) | assumed ✓ (Phase 30's UAT was performed) | — | none — the UAT is operator-driven |
| Baseline: `make test` | regression floor | ✓ **72 cases / 2,616,112 assertions / 0 failed** | measured this session | — |
| Baseline: `make guards` | regression floor | ✓ **PASS** (frozen + includes + canary, incl. `[2b/5]` "all 8 VcoInputs DSP fields stay runtime-live at -O3") | measured this session | — |

**Missing dependencies with no fallback:** none.
**Missing dependencies with fallback:** none.

---

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | doctest 2.4.11, vendored at `tests/doctest.h`; impl macro owned by `tests/main.cpp` |
| Config file | `Makefile:34-48` (`TEST_*` namespaced target; `TEST_CXXFLAGS := -std=c++17 -O2 -g -Isrc -Itests -Wall -Wextra -ffp-contract=off`) |
| Quick run command | `make test` |
| Full suite command | `make test && make guards && make strict` |
| Current baseline | **72 cases / 2,616,112 assertions / 0 failed**; guards **PASS** (measured this session) |
| Source globbing | `TEST_SOURCES := $(wildcard tests/*.cpp)` — a new test file is picked up with **zero** build wiring |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PITCH-01 | V/OCT tracks 1V/oct off C4 = 0 V within **< 1 cent**, measured on the OUTPUT | unit (headless DSP) | `./build-test/test -tc="*v/oct tracking*"` | ❌ Wave 0 — `tests/test_vco_pitch.cpp` |
| PITCH-01 | Secondary tier: `tel.freqHz` vs libm reference where crossings cannot resolve | unit | same file | ❌ Wave 0 |
| PITCH-02 | COARSE `+n` octaves shifts measured pitch by exactly `n` octaves; full `±5` range reachable | unit | same file | ❌ Wave 0 |
| PITCH-03 | FINE `±1` semitone shifts pitch by `±100 cents ± tolerance`; `fine/12` conversion correct | unit | same file | ❌ Wave 0 |
| PITCH-04 | `tel.freqHz <= kVcoNyquistGuardFrac * safeRate` for every input incl. hostile; clamp fires and the oscillator keeps sounding (D-10) | unit | `-tc="*Nyquist*"` | ⚠️ partial — `tests/test_vco_core.cpp:753` already pins `freqNyquistBounded` symbolically; needs a *pitch-driven* case above the ceiling |
| PITCH-05 | Non-regression: `phase` is `double`, `deltaPhase` computed with both casts | unit + review | existing `test_vco_core.cpp` scenario four (`phaseInRange`) | ✅ exists |
| FM-01 | FM input at audio rate modulates pitch (a non-trivial FM signal changes the measured spectrum-free pitch trajectory) | unit | `-tc="*exponential FM*"` | ❌ Wave 0 |
| FM-02 | Attenuverter is bipolar: `fmAtten = -1` inverts the pitch shift of `fmAtten = +1`; `fmAtten = 0` is a no-op | unit | same | ❌ Wave 0 |
| FM-03 | **The summation identity:** a static `fmVolts = V` at `fmAtten = 1` produces exactly the same output block as `pitchCV += V` with FM disconnected — bit-exact | unit | same | ❌ Wave 0 |
| FM-03 | Negative control: a *multiplicative* stand-in (`freq *= exp2(fm)`) must FAIL the same identity | unit (positive control) | same | ❌ Wave 0 |
| D-09 | `fmConnected = false` makes the FM term a no-op bit-exactly, whatever `fmVolts`/`fmAtten` are | unit | same | ❌ Wave 0 |
| D-14 | Hostile pitch volts (NaN, ±inf, ±1e30, ±200 V) never reach `exp2_taylor5` outside `[-kVcoMaxPitchVolts, +kVcoMaxPitchVolts]` | UBSan RED + standing behavioral case | `-tc="*hostile pitch*"` + one-shot UBSan probe | ❌ Wave 0 |
| TEST-02 | The phase gate: the union of the PITCH-01 rows above | unit | `make test` | ❌ Wave 0 |
| guardrail | Six LFO goldens byte-identical; no frozen header edited | existing | `make test` (`tests/test_golden.cpp`, `tests/test_lfo_guardrail.cpp`) + `make guards` | ✅ exists |

### The TEST-02 sweep design (MEASURED, not proposed)

**Fixed inputs for the primary tier:** `morph = 0.f`, `character = 0.f` (pure sine — the estimator's error is 100× lower there; see Pitfall 4), all other POD fields at their neutral values, timing injected by `VcoBlockDriver`.

**Upper bound per rate** = `min(clampCeilingVolts(sr), estimatorCeilingVolts(sr))`, both derived (D-21), never hardcoded:

| Sample rate | Clamp ceiling (D-21) | Estimator ceiling (2.5 samp/cyc) | **Binding limit** | Recommended top test point |
|-------------|---------------------|----------------------------------|-------------------|---------------------------|
| 44100 | +6.3826 V | **+6.0745 V** | **estimator** | **+6.0 V** (2.634 samp/cyc, MEASURED −0.0026 cents) |
| 48000 | +6.5049 V | **+6.1968 V** | **estimator** | **+6.0 V** (2.867 samp/cyc, MEASURED −0.0087 cents) |
| 96000 | +7.5049 V | **+7.1968 V** | **estimator** | **+7.0 V** (2.867 samp/cyc, MEASURED −0.0024 cents) |

*(Estimator ceiling = `log2((sr / 2.5) / 261.6256)`.)*

> **The estimator binds at every rate, and the clamp binds at none — but D-21 still requires the clamp boundary to be computed and `min()`-ed.** Two reasons. First, it is the *requirement-level* boundary: PITCH-04's clamp is the decided behavior that legitimately breaks tracking, and the test must state that it stops below it rather than leave it implicit. Second, it moves if the constant moves — a future rate or a different `kVcoNyquistGuardFrac` can flip which limit binds, and a test that only computed the estimator limit would silently start sweeping past the ceiling.
>
> The concrete trap this arrangement avoids: at 48 kHz the clamp ceiling is `+6.5049 V`, so a clamp-only bound would admit a `+6.5 V` test point — MEASURED at **−11.61 cents** while the oscillator is perfectly correct, because 23679.6 Hz is 2.027 samples/cycle. Correct behavior, broken apparatus, and the failure reads exactly like a pitch bug.

**Lower bound:** `−7.0 V` (2.044 Hz) is measurable and MEASURED accurate to `+0.00005 cents`, but needs an ~8 s window for 16 cycles. `−5.0 V` (8.18 Hz) is the practical floor for suite runtime and covers well below C-1. Recommend the sweep run `−5.0 V .. top` in `0.5 V` steps at each rate, with `−7.0 V` added at one rate as a documented extreme point.

**Window rule (MEASURED):** `n = round(sr * max(0.25, 16.0 / f_expected))`. Measured behavior at 48 kHz, `morph = 0`:

| pitch | 0.05 s | 0.10 s | 0.25 s | 0.50 s | 1.00 s | 2.00 s |
|-------|--------|--------|--------|--------|--------|--------|
| `−4.0 V` | *0 crossings* | *1 crossing* | +0.000052 | +0.000051 | +0.000051 | +0.000051 |
| `0.0 V` | +0.000049 | +0.000045 | +0.000049 | +0.000051 | +0.000050 | +0.000050 |
| `+4.0 V` | +0.001635 | −0.001056 | −0.000526 | +0.000284 | +0.000222 | +0.000153 |
| `+6.0 V` | **−0.096765** | +0.001014 | −0.008708 | −0.002396 | −0.000133 | +0.000021 |

The `0.25 s` column is adequate everywhere the sweep goes (worst `−0.0087` cents) and the `0.05 s` row shows why a shorter window must not be used at the top. A `REQUIRE(nUp >= 8)` precondition (the Phase-30 idiom) catches the `−4.0 V @ 0.05 s` degenerate case rather than letting it silently return the `-1.0` sentinel.

**Measured worst error anywhere in the recommended sweep** (`morph = 0`, `character = 0`, all three rates, within the binding limits, 0.5 V grid): **−0.00968 cents**, at `+5.5 V` / 44.1 kHz. Per-rate worsts: 44.1 kHz **−0.00968** (`+5.5 V`), 48 kHz **−0.00871** (`+6.0 V`), 96 kHz **−0.00240** (`+7.0 V`). The `−7.0 V` extreme point measures **+0.00004**.

**Recommended tolerance: a fixed `0.05` cents on the primary tier**, at every point and every rate — no widening with samples-per-cycle (D-20 forbids it). Provenance: **5.2× above the worst measured point** (0.00968), **20× under** the 1-cent requirement, and ~5× above the DSP's own measured 0.0101-cent end-to-end worst, so the gate is dominated by the oscillator's error rather than by the apparatus. The phase must re-measure and record its own figures (D-18) rather than inheriting this table.

**Non-vacuity requirements (each must be implemented, per the Phase 29/30 lesson):**
1. **Measure the OUTPUT, not `tel.freqHz`,** on the primary tier (D-19 / Phase 30's D-16).
2. **Ground truth from libm**, never from `exp2_taylor5` (D-18).
3. **Expectations one octave apart** across the grid, so an accumulator that latched a single frequency can satisfy at most one point.
4. **`REQUIRE(nUp >= 8)` before any tolerance check**, so a silent non-oscillation reads as a hard failure rather than as a wrong number.
5. **An FM negative control:** a stand-in core that multiplies (`freq *= exp2_taylor5(fm)`) instead of summing must FAIL the FM-03 identity through the same helper. Without it, "sum equals shifted V/OCT" is satisfiable by an implementation that does neither. Model it on `DeliberatelyBrokenSharedStateCore` (`tests/test_vco_core.cpp:316-366`) — anonymous namespace, test TU only, never under `src/`.
6. **A clamp-boundary case that proves the clamp FIRES:** at `pitchVolts` just above the derived ceiling, `tel.freqHz` must equal `kVcoNyquistGuardFrac * sampleRate` exactly, and the output must keep oscillating (D-10's "peaks flatten out", not silence).

### Sampling Rate

- **Per task commit:** `make test` (full suite; it is fast enough — 2.6 M assertions today)
- **Per wave merge:** `make test && make guards && make strict`
- **Phase gate:** `make test` + `make guards` + `make strict` + a real plugin link + the 3-OS CI matrix observed green **by SHA** (never by recency) + operator in-Rack UAT with a full `dist/` flush (STATE.md Phase 30: a partial flush silently leaves a stale plugin *version*)

### Wave 0 Gaps

- [ ] `tests/test_vco_pitch.cpp` — TEST-02 tracking gate (PITCH-01), COARSE/FINE range cases (PITCH-02/03), FM summation identity + multiplicative negative control (FM-01/02/03, D-09), D-14 hostile-pitch standing case
- [ ] `tests/check_includes.sh` — one exact-path `VCO_SIDE_ALLOW` entry **if** the new file is chosen (Pitfall 6); explicit plan task, not a gate-time fix
- [ ] A pitch-driven Nyquist-clamp case (PITCH-04 / D-10): the existing `freqNyquistBounded` pin at `tests/test_vco_core.cpp:753` is driven by hostile *timing*, not by hostile *pitch*, and does not observe the clamp firing on a legitimate high note
- [ ] D-14 RED evidence — a one-shot UBSan probe (recommended) or a `Telemetry` field; see Open Question 1
- [ ] No framework install needed; no `conftest`-equivalent needed (`tests/main.cpp` already owns the doctest impl macro)

---

## Security Domain

`security_enforcement` is absent from `.planning/config.json`, so it is treated as enabled. This is an offline audio plugin with no network, no persistence *(the VCO serializes nothing until Phase 35)*, no authentication and no user accounts — so most ASVS categories are structurally inapplicable. The one that genuinely applies is the one this phase is already addressing.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | No identity surface exists in a Rack module. |
| V3 Session Management | no | No sessions. |
| V4 Access Control | no | No multi-principal surface. |
| **V5 Input Validation** | **yes** | **The whole of D-14.** Rack does not sanitize cable voltages; a patched cable can deliver NaN, ±inf, or arbitrary magnitude into `in.fmVolts`. The control is the bound-before-exponentiate guard, written with the negated-comparison idiom because `forge::clamp` is MEASURED NaN-transparent. |
| V6 Cryptography | no | No crypto in the audio path. (`Xoroshiro128Plus` is a simulation RNG, not a CSPRNG, and is correctly not used as one.) |
| V7 Error Handling / Logging | partial | No logging in the audio thread by design (real-time constraint). Failure modes must degrade to silence or a clamped value, never to UB or a hang. |
| V12 File & Resources | no | No file I/O this phase. |
| V14 Configuration | partial | The C++11-strict / MinGW toolchain gates are the build-integrity controls; unchanged by this phase. |

### Known Threat Patterns for a VCV Rack 2 DSP core

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Non-finite cable voltage reaching a float→int cast (`(int32_t)NaN`) | Denial of Service / Tampering | Bound the value before the cast, using a NaN-rejecting comparison. **This is D-14.** MEASURED reachable via the new FM jack. |
| Left-shift of a negative or over-large `int32_t` (`xi << 23`) | Denial of Service | Same bound; keeps `x + 127` inside `[0, 255]`. |
| Unbounded phase accumulation from an out-of-range frequency | Denial of Service (audio-thread runaway, downstream patch poisoning) | Already mitigated: the Nyquist ceiling + the `kVcoMaxDeltaPhase` increment bound. MEASURED without them: `phase = 1,014,986`, `|out| = 8.6e6 V`, every sample `isfinite`. |
| Non-finite sample propagating into the user's whole patch | Denial of Service | The negated frequency floor as the last writer (CR-01), plus the `deltaPhase` floor. Both already present. |
| Degenerate `(0,0)` RNG seed causing an infinite rejection loop in `std::normal_distribution` — **a hang on patch load, not a test failure** | Denial of Service | Out of scope this phase (no new seeding), but the prohibition is standing: never seed with `(0,0)`, and re-validate any deserialized seed. |
| Shared-header edit silently changing the shipped LFO's output | Tampering (supply-chain-adjacent) | `tests/check_frozen.sh` byte-pinning + six `.f32` goldens + the milestone guardrail. Untouched by this phase. |

**Recorded, deliberately not fixed:** the shipped Analog LFO reaches the same `(int32_t)` UB through `src/AnalogLFO.cpp:320` → `src/dsp/LfoCore.hpp:183-184` with an equally unsanitized cable voltage. Fixing it means editing a frozen, shipped-module header — a guardrail event requiring operator sign-off and golden re-verification. See Open Question 4.

---

## Sources

### Primary (HIGH confidence)

**Direct measurement performed this session** (all on Apple clang, `-std=c++17 -O2 -Isrc -ffp-contract=off`, against the live headers):
- `forge::exp2_taylor5` accuracy: exhaustive over 167,364,675 floats in one cell; 4,000,001-point sweeps over four volt ranges; 21-point integer-octave bit-exactness check
- `forge::exp2Floor` UB envelope: 11-point probe table + `clang -fsanitize=undefined` run reporting `RackCompat.hpp:106` and `:109` by line
- Hostile-pitch behavior of the live `VcoCore`: 9 probes × 4096 steps
- Crossing-estimator resolution: 2 morph values × 3 sample rates × 31 pitch points, plus a 4×6 window-length grid
- Nyquist crossover volts at 0.49 and 0.495 across 4 sample rates
- Candidate D-14 bound behavior at 6 magnitudes
- Baselines: `make test` (72/2,616,112/0), `make guards` (PASS)

**Source files read in full or in the cited range:**
- `src/dsp/VcoCore.hpp` (whole file, 285 lines) — the pitch insertion point, both constants, the guard sequence, the C++11 rules, the source-shape contract
- `src/dsp/RackCompat.hpp` (whole file) — `exp2_taylor5`, `exp2Floor`, `clamp`
- `src/AnalogVCO.cpp` (whole file) — the shell contract, existing geometry, the D-17 banner
- `tests/test_vco_core.cpp` (whole file, 1109 lines) — the estimator, the five invariants, the negative-control pattern, scenario four
- `tests/VcoBlockDriver.hpp` (whole file) — the harness contract
- `Makefile` (whole file) — every target and its flags
- `src/dsp/LfoCore.hpp:150-200`, `src/AnalogLFO.cpp:195-225,319-321` — the shipped FM path and param styling
- `src/dsp/Waveshape.hpp:1-60` — `morph = 0, character = 0` is a bare `sin(2π·phase)`
- `tests/check_canary.sh:16-160`, `tests/check_includes.sh:269-332` — the two guards this phase can trip
- `src/vco_compile_canary.cpp`, `res/AnalogVCO.svg`
- `../Rack-SDK/include/engine/Module.hpp:120-137` — `configParam` signature
- `../Rack-SDK/include/engine/ParamQuantity.hpp:42-53` — the display formula

### Secondary (MEDIUM confidence)

- `.planning/phases/31-pitch-tuning-exponential-fm/31-CONTEXT.md` — all 22 decisions, copied verbatim above
- `.planning/STATE.md` §Accumulated Context — the Phase 29/30 lessons, the "no tag on local evidence" rule, the R-9 ODR landmine, the `dist/` flush lesson, the closed A5 cross-toolchain assumption
- `.planning/phases/30-vcocore-skeleton-module-registration/deferred-items.md` — items 2, 3, 4, 5
- `.planning/PROJECT.md:148-156` §Constraints — the LFO guardrail and the four frozen headers
- `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md:193-205` — requirement text and success criteria (both confirmed still carrying the pre-D-00 "±2 semitones")
- `.planning/research/STACK.md:52-53,110-122`; `PITFALLS.md:85-115,185-200,375-390`; `ARCHITECTURE.md:118-130,185-210,325-335`; `FEATURES.md:25-40` — prior-milestone research. **Two of its numeric claims about `exp2_taylor5` are superseded by this session's measurement** (see §State of the Art).

### Tertiary (LOW confidence)

- Rack `ParamWidget` ctrl/cmd-click-to-default behavior (A6) — standard, widely relied on, not re-verified against SDK source this session.
- The suggested throwaway-panel coordinates (A4) — a layout judgment on an asset that is discarded in Phase 35.

**No external web search was performed and none was needed:** every question this phase asks is answerable from the local Rack SDK, the local source tree, or direct measurement, and doing so is strictly stronger evidence than any third-party page would be.

---

## Metadata

**Confidence breakdown:**
- Standard stack: **HIGH** — no new dependencies; every symbol was read in its own source file and the SDK signature was verified against `../Rack-SDK`.
- Pitch law & accuracy budget: **HIGH** — measured exhaustively, resolving a documented 100× contradiction between two prior research files. The margin (99×) is large enough that no plausible measurement error changes the conclusion.
- Nyquist policy & blast radius: **HIGH** — crossovers computed at four rates; blast radius established by repo-wide grep, and every non-comment reference is symbolic.
- D-14 bound: **HIGH** on the UB envelope (measured, and independently confirmed by UBSan naming the exact lines); **MEDIUM** on the specific `±64 V` choice, which is a margin judgment inside an explicitly discretionary decision.
- Architecture / signal-chain ordering: **HIGH** — the ordering is already documented in the live header with its own recorded measurements, and this phase adds four lines ahead of it.
- Validation architecture: **HIGH** on the estimator's measured limits and the per-point cents figures; **MEDIUM** on the recommended `0.05` cents tolerance and the `3.0` samples/cycle cutoff, both of which the phase must re-measure and justify per D-18.
- Pitfalls: **HIGH** — Pitfalls 1, 2, 3, 4 are each backed by a measurement made this session; 5, 6, 7, 8 are backed by the repo's own recorded history plus a direct read of the guard scripts.

**Research date:** 2026-07-29
**Valid until:** 2026-08-28 (30 days — the domain is a frozen local SDK and an in-tree codebase; the only invalidation risk is the repo itself changing. **Re-verify immediately if:** the D-00 edits land, `src/dsp/VcoCore.hpp` is edited by another phase, or `../Rack-SDK` is updated.)

**⚠ BLOCKING, as of this research:** `.planning/REQUIREMENTS.md:17` and `.planning/ROADMAP.md:201` both still read **"±2 semitones"**. D-00 requires both to say **"±1 semitone (±100 cents)"** *before* planning begins. Verified by direct read this session.
