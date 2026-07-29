# Phase 31: Pitch, Tuning & Exponential FM - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-07-29
**Phase:** 31-pitch-tuning-exponential-fm
**Areas discussed:** Tune knob ranges & feel, FM depth calibration, Nyquist ceiling behavior, The <1 cent gate (TEST-02), Hostile-input hardening, Shell surface

---

## Area Selection

All four offered gray areas were selected for discussion. A fifth question routed the one matching pending todo.

| Option | Description | Selected |
|--------|-------------|----------|
| Tune knob ranges & feel | COARSE ±5 oct / FINE ±2 st per roadmap vs FINE ±1 st per research; snap; taper | ✓ |
| FM depth calibration | Attenuverter topology and octaves-per-volt at full CW | ✓ |
| Nyquist ceiling behavior | What replaces the PROVISIONAL 0.49 constant, and what a player hears at the ceiling | ✓ |
| The <1 cent gate (TEST-02) | What the phase gate measures, against what reference, over what range | ✓ |

---

## Pending Todo Routing

| Option | Description | Selected |
|--------|-------------|----------|
| Defer to Phase 36 | 'Wire tests/check_docs.sh into CI' matched on generic keywords only; Phase 36 owns CI + release | ✓ |
| Fold into Phase 31 | Add the CI step and Makefile entry during this phase | |

**User's choice:** Defer to Phase 36
**Notes:** Same scope-creep reasoning that kept it out of Phase 29. Stays visible via the `GUARD_WIRING_EXEMPT` entry in `tests/check_includes.sh` §`[7/7]`.

---

## Tune Knob Ranges & Feel

Raised up front: `.planning/REQUIREMENTS.md:18` (PITCH-03) and roadmap success criterion 2 both say **±2 semitones**, while `.planning/research/FEATURES.md:31` recommends **±1 semitone**. A live conflict between planning documents, surfaced rather than silently resolved.

### FINE tune range

| Option | Description | Selected |
|--------|-------------|----------|
| ±2 semitones *(Claude's recommendation)* | Keep what REQUIREMENTS.md and the roadmap already say; shift-drag gives fine resolution regardless of range; no requirements edit | |
| ±1 semitone (±100 cents) | Follow research/FEATURES.md — the classic hardware convention, double the raw knob resolution for unison beating; costs a REQUIREMENTS.md + ROADMAP.md edit | ✓ |
| ±7 semitones (perfect fifth) | Wide trim reaching musical intervals from FINE alone; blurs the coarse/fine split | |

**User's choice:** ±1 semitone — **against Claude's recommendation**
**Notes:** Consequence recorded as D-00, a blocking pre-planning action: PITCH-03 and roadmap criterion 2 must both be edited to ±1 semitone before the plan is written, or the plan will be checked against a gate it deliberately contradicts.

### COARSE snap

| Option | Description | Selected |
|--------|-------------|----------|
| Continuous only *(recommended)* | Honors PITCH-02's word "continuously" literally; snap is a new capability needing a menu item, a persisted bool, and patch serialization the VCO has none of | ✓ |
| Continuous + octave snap toggle | Right-click 'Snap COARSE to octaves'; useful for octave stacking; the LFO's ratio snap proved the pattern | |
| Continuous + octave AND semitone snap | Three-way toggle making COARSE an interval selector | |

**User's choice:** Continuous only
**Notes:** Snap preserved as a deferred idea pointed at Phase 35 or v2.1. Noted that PROJECT.md's Out-of-Scope entry for "Octave snap / semitone selector" was written about the LFO, where sub-audio rates made it meaningless — it is not a ruling on the VCO.

### Tooltip readout units

| Option | Description | Selected |
|--------|-------------|----------|
| COARSE in octaves, FINE in cents *(recommended)* | Each knob reads in its own natural musical unit; matches how the roadmap describes them | ✓ |
| Both in cents | One unit everywhere; large numbers on COARSE read awkwardly | |
| COARSE in Hz, FINE in cents | Concrete, but the number is a lie once a cable is patched into V/OCT | |
| You decide | Planner's call | |

**User's choice:** COARSE in octaves, FINE in cents

---

## FM Depth Calibration

Context given: the LFO's shipped path is `freq *= exp2_taylor5(fmCV × fmAtten × 0.6)`, but `0.6` is an LFO number for sub-audio wobble, and the VCO sums into volts *before* the exp2 instead.

### Octaves per volt at full clockwise

| Option | Description | Selected |
|--------|-------------|----------|
| 1.0 oct/V — a second V/OCT *(recommended)* | Full CW makes the FM jack behave exactly like a second 1V/oct input; the most predictable contract, a known 1:1 reference the attenuverter scales down from | ✓ |
| 0.6 oct/V — the LFO's constant | Series-consistent and audition-proven — but proven at sub-audio rates, for a different job | |
| 0.5 oct/V — ±2.5 oct at ±5V | Tamer ceiling, more of the attenuverter's travel musically useful; loses the 1:1 mental model | |

**User's choice:** 1.0 oct/V

### Attenuverter taper

| Option | Description | Selected |
|--------|-------------|----------|
| Linear, center = zero *(recommended)* | Standard Eurorack attenuverter behavior; shift-drag provides fine adjustment | |
| Progressive (x², sign-preserving) | Mirrors the CHARACTER knob's curve; spreads the subtle audio-rate FM range across most of the knob | |
| You decide | Planner's call | |
| **Other: "Match the LFO style"** | Free-text response | ✓ |

**User's choice:** Free text — "Match the LFO style"

**Notes — this answer required a follow-up because it was ambiguous against a requirement.** Inspection of `src/AnalogLFO.cpp:214` showed the shipped LFO's controls named "atten" are **unipolar attenuators**, not attenuverters: `configParam(FM_ATTEN_PARAM, 0.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f)`. But FM-02 and roadmap criterion 3 both specify **bipolar** for the VCO. Two readings were put to the user as plain text:

1. Keep bipolar, borrow the LFO's styling — `-1..+1`, linear, default `0`, `-100%..+100%`. Honors FM-02 as written. *(Claude's recommendation.)*
2. Copy the LFO literally — unipolar `0..1`. Drops the bipolar requirement, needing an FM-02 + roadmap edit like PITCH-03 now does.

**User selected option 1.** No requirements edit needed for FM-02.

### FM control form factor

| Option | Description | Selected |
|--------|-------------|----------|
| Full knob — a performance control *(recommended)* | FM depth is swept while playing; same visual weight as MORPH/CHARACTER; costs 18HP panel space | |
| Scalloped trimpot — matches the LFO | Series-consistent, space-efficient, harder to perform with | |
| Defer entirely to Phase 35 | Declare the param now, let Phase 35 decide the physical form when it has the whole control budget in view | ✓ |

**User's choice:** Defer entirely to Phase 35

---

## Nyquist Ceiling Behavior

Framing given: this stopped being an edge case once 1.0 oct/V was chosen — a ±5V audio-rate modulator at full depth swings instantaneous pitch ±5 octaves, so deep FM slams the ceiling on most cycles. The behavior there is an audible timbral decision, not just a safety clamp.

### Behavior at the ceiling

| Option | Description | Selected |
|--------|-------------|----------|
| Hard clamp — pitch stops rising *(recommended)* | Frequency pins, oscillator keeps sounding, deep FM peaks flatten out; simplest to reason about; consequence is that 1V/oct tracking is intentionally broken above the ceiling | ✓ |
| Clamp + fade the output above threshold | Cleaner under extreme FM; adds a gain stage colliding with Phase 34's OUT-01..03 | |
| Wrap / fold the pitch back down | Wild inharmonic sweeps — a deliberate effect, not what PITCH-04 asks for | |

**User's choice:** Hard clamp

### Ceiling constant

| Option | Description | Selected |
|--------|-------------|----------|
| 0.495 — research's recommendation *(recommended)* | `min(freq, 0.5 × sampleRate × 0.99)` per research/STACK.md:122; ~21.8kHz at 44.1kHz, above human hearing so inaudible in normal use | ✓ |
| 0.49 — keep Phase 30's provisional value | Keeps the number unchanged so no measurement can shift — but leaves a PROVISIONAL constant standing in the phase that owns it | |
| 0.45 — conservative | Real margin for Phase 32's band-limiting and Phase 34's drift; costs the top ~1kHz of reachable pitch | |

**User's choice:** 0.495

### Low-end floor

| Option | Description | Selected |
|--------|-------------|----------|
| Allow stall at 0 Hz *(recommended)* | Extreme negative pitch freezes the phase and outputs a constant; honest, and PITCH-04 speaks only to the top end | ✓ |
| Floor at 0.001 Hz — mirror the LFO | `std::fmax(freq, 0.001f)`; series-consistent, easier for Phase 32's crossing placement | |
| Floor at a musical minimum (e.g. 0.01 Hz) | Arbitrary; silently overrides what the user dialed in | |

**User's choice:** Allow stall at 0 Hz

---

## The <1 Cent Gate (TEST-02)

Two findings raised before the questions, both of which changed the shape of the area:

1. **The research contradicts itself on the polynomial's error.** `.planning/research/STACK.md:53` says ~1e-6 relative (≪0.002 cents); `.planning/research/PITFALLS.md:94` says ~1e-4 relative (≈0.1 cent). Both clear 1 cent, but the gate must *measure* rather than inherit either figure.
2. **Output-derived measurement cannot cover the top of the range.** 1 cent is 0.058% frequency error; at 44.1kHz a 16kHz tone is 2.6 samples per cycle, and the ceiling clamps anything above +6.38V anyway. The honest measurement band and the full pitch range are not the same interval.

### Ground truth

| Option | Description | Selected |
|--------|-------------|----------|
| libm `std::exp2` computed in the test *(recommended)* | The only option measuring actual cents error including the polynomial's own; libm is banned in `src/` for bit-identity but the test is not `src/`, and using it there is what makes the assertion independent | ✓ |
| Hardcoded expected-frequency table | Maximally independent and reviewable against a tuning chart; fixes the test to points rather than sweeping | |
| `exp2_taylor5` itself | Cheap — and exactly the vacuous-coverage trap Phases 29 and 30 were both bitten by. Listed only so the record shows it was rejected | |

**User's choice:** libm `std::exp2` in the test

### Observation method

| Option | Description | Selected |
|--------|-------------|----------|
| Both, as two tiers *(recommended)* | Output-derived interpolated zero-crossings as the primary gate where measurable; `tel.freqHz` covering the octaves where crossings can't resolve. Honors Phase 30's D-16 while still covering the top | ✓ |
| Output period only | Purest D-16 reading; TEST-02 simply doesn't cover the top octaves, documented as a gap | |
| `tel.freqHz` only | Precise and trivial — but only re-reads the number `step()` just computed; would stay green if the accumulator ignored the frequency entirely. Rejected by D-16 | |

**User's choice:** Both, as two tiers

### Range handling

| Option | Description | Selected |
|--------|-------------|----------|
| Sweep the full range, measure high notes at 96kHz *(recommended)* | The harness already runs 44.1/48/96kHz; each pitch tested at whichever rate can resolve it, with the mapping stated explicitly | ✓ |
| Test the reliably-measurable band only | Restrict to ~-4V..+2V and document the untested top as a known gap for Phase 36 | |
| Full range at every rate, tolerance scaled by resolution | Most coverage — but a moving tolerance is a gate harder to trust, and this project has been bitten by gates wider than their prose | |

**User's choice:** Sweep the full range, measure high notes at 96kHz

---

## Hostile-Input Hardening

Surfaced during scoping rather than pre-selected: FM puts an unsanitized cable voltage into the `exp2_taylor5` argument for the first time, and that function performs `(int32_t)x` on it (`src/dsp/RackCompat.hpp:106`). Casting a NaN or infinite float to `int32_t` is undefined behavior.

| Option | Description | Selected |
|--------|-------------|----------|
| Clamp pitch volts before exp2 *(recommended)* | The existing negated floor catches the non-finite *result* but not the UB on the way there; fix must be local to VcoCore since RackCompat.hpp is frozen and shipped | ✓ |
| Rely on the existing floor | The UB is real but never observed to misbehave on a shipped toolchain; defers to Phase 34 alongside deferred item 3 | |
| Clamp, and add hostile FM inputs to the test grid | As option 1 plus extending scenario four with ±inf / subnormal / very-large-finite, closing deferred item 6 early | |

**User's choice:** Clamp pitch volts before exp2
**Notes:** Deferred item 6 therefore stays pointed at Phase 32. D-14 still requires its own RED-first case per standing project practice — that case covers the new pitch/FM fields, which are a different input class from the timing grid.

---

## Shell Surface

| Option | Description | Selected |
|--------|-------------|----------|
| Yes — add all four *(recommended)* | COARSE, FINE, FM in, FM depth join the existing controls; continues Phase 30's D-07 rule and its converse — DSP no control can reach cannot be auditioned in Rack | ✓ |
| Core + tests only | Smallest diff, cleanest guardrail story — but the <1 cent gate would be proven only in the harness, never heard | |
| Tune knobs only, FM in Phase 35 | Splits the phase's own deliverable across two phases, against the roadmap's FM-01..03 mapping | |

**User's choice:** Yes — add all four

---

## Claude's Discretion

- Exact numeric bound for the pitch-volt clamp and where the constant lives.
- Cycle count, block length, and interpolation method for the crossing measurement; the pitch/sample-rate assignment table.
- Whether TEST-02 extends `tests/test_vco_core.cpp` or lands in a new file.
- Throwaway-panel geometry and widget placement for the four new controls.
- Whether the FM contribution carries an explicit named depth constant or leaves the 1.0 oct/V factor implicit.
- Whether `kVcoFreqC4`'s comment needs updating now that PITCH-01 is genuinely delivered.

## Deferred Ideas

- COARSE octave/semitone snap → Phase 35 or v2.1.
- Amplitude fade near the Nyquist ceiling → revisitable in Phase 34, which owns the output stage.
- Hostile-timing grid extension (deferred item 6) → stays at Phase 32.
- Per-instance seed entropy + patch persistence (deferred item 2) → stays at Phase 34/35.
- `tests/check_includes.sh [2/7]` unanchored exemption (deferred item 5) → next phase that touches that script.
- "Wire `tests/check_docs.sh` into CI" todo → Phase 36.
