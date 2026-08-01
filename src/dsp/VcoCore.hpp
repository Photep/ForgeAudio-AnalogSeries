#pragma once
// src/dsp/VcoCore.hpp
//
// VCO core (CORE-01): a POD forge::VcoInputs goes in, an output voltage comes
// out, and last-step telemetry is left behind for the shell. The boundary shape
// is Phase 29's (D-03) and does not churn — Phase 30 filled the seam without
// touching it. step() is a BAND-LIMITED morphed oscillator as of this commit:
// a four-term volt-domain pitch sum through ONE forge::exp2_taylor5 off the C4
// reference, a Nyquist-guarded frequency, a double-precision phase accumulator,
// one call into the FROZEN forge::Waveshape::morphedWave, PLUS an additive
// morph-aware polyBLEP/polyBLAMP correction from forge::MorphBlep, scaled x5 and
// returned unconditioned.
//
// UNTIL THIS COMMIT THAT SENTENCE READ "a NAIVE, DELIBERATELY ALIASED morphed
// oscillator", and the naive path was the live behavior for Phases 30 and 31.
// It is preserved — deliberately, and only — as NaiveVcoCoreMirror in
// tests/test_vco_spectrum.cpp, which is the D-08 baseline every threshold this
// phase pins is measured against. There is no flag, no second entry point and
// no naive branch in this body: the naive path lives in the test TU or nowhere.
//
// THE PITCH CHAIN IS DELIVERED as of this commit (Phase 31). The V/OCT input,
// coarse tune in OCTAVES, fine tune in SEMITONES and a connected-gated FM
// contribution are summed in the VOLT domain (PITCH-01..03, FM-01..03), the sum
// is BOUNDED to a finite range so the frozen exponential's integer cast and
// shift stay defined (D-14), and the bounded volts pass through EXACTLY ONE
// exponential off the C4 reference. PITCH-04's Nyquist policy is likewise
// SETTLED: kVcoNyquistGuardFrac is 0.495f (D-11), so the frequency ceiling below
// is final rather than a placeholder. The double-precision accumulator
// (PITCH-05) was already in place and is unchanged.
//
// WHAT REMAINS IS LATER PHASES' WORK, not a debt in this body:
//   - Phase 32 (CORE-02 / AA-01..05) has DELIVERED band-limiting at the
//     morphedWave call site below. It still owns the ALIAS FLOOR itself: the
//     per-shape thresholds are re-pinned from this repository's own measurement
//     in plan 32-07, so no assertion over this body may claim a spectral figure
//     that tests/test_vco_spectrum.cpp has not measured.
//   - Phase 33 owns hard sync, which is why the POD still carries no sync
//     fields (see the sync note below).
//   - Phase 34 (OUT-01..03) owns output conditioning and the MOVING drift
//     engine (the OU layers this phase deliberately does not step); see the x5
//     note in step().
//
// Source-shape contract (tests/check_canary.sh [2b/5]): the `struct VcoCore`
// declaration line and the `float step(...)` signature line must each stay on
// ONE line together with their opening brace. The canary line-matches both
// patterns to build a perturbed copy of this file; Allman braces, a wrapped
// parameter list or an added `noexcept` make `make guards` hard-fail with
// "could not perturb src/dsp/VcoCore.hpp" — a guard error, not a DSP error.
// NOTE for future editors: the step matcher is UNANCHORED, so quoting the full
// signature verbatim in a comment ON A LINE THAT ALSO CONTAINS `{` makes the
// canary perturb the COMMENT and the guard fails with unrelated compile errors.
// That is why the signature is abbreviated above. (Observed, not theorised.)
//
// Naming landmine (R-9): the POD is forge::VcoInputs, NOT forge::Inputs.
// forge::Inputs already belongs to the LFO (src/dsp/LfoCore.hpp:40). A second
// struct of that name in another header is a cross-TU ODR violation that
// compiles silently in any TU that includes only one of the two headers, and
// only detonates at link time on the strictest toolchain (MinGW/GCC in CI).
//
// Sync fields (syncVoltage / syncConnected) are deliberately ABSENT. Phase 33
// owns hard sync and adds them then; adding POD fields later is a non-breaking
// additive change because the VCO has no golden fixtures until Phase 36.
//
// Two-standard rule (TEST-06): this header must compile clean under BOTH
// -std=c++11 -pedantic-errors (the Rack plugin toolchain — this is the standard
// the shipped build uses, enforced by `make strict` and the CI MinGW leg) AND
// -std=c++17 (the Rack-free test target). The C++11 restrictions below are
// therefore binding on this file and on every VCO header that follows it:
//   - No `inline constexpr` variables (C++17 inline variables). Plain constexpr
//     at namespace scope has internal linkage per TU and is the C++11 form —
//     see src/dsp/MathConst.hpp for the same rationale.
//   - No `if constexpr`, no structured bindings, no [[maybe_unused]], no
//     nested-namespace definition syntax, no auto return-type deduction, no
//     generic lambdas.
//   - No std::clamp (C++17 <algorithm>) — use forge::clamp / forge::clampi from
//     dsp/RackCompat.hpp if clamping is ever needed here.
//   - No pi macro from <cmath> — use forge::kPi from dsp/MathConst.hpp.
//   - Any constant table must be a namespace-scope `static constexpr`, never an
//     in-class `static constexpr` that is indexed at runtime: under C++11 the
//     in-class form is a DECLARATION only, and runtime indexing odr-uses it,
//     producing an undefined reference at MinGW link time. That exact class of
//     bug got v2.0.0 rejected from the VCV Library. Phase 29 needs no table; the
//     rule is recorded here because Phases 30+ will.
//   - Do NOT brace-initialize VcoInputs with a value list. Under C++11 a class
//     with non-static data member initializers is not an aggregate, so
//     `VcoInputs in{1.f, 2.f}` is a hard error. `VcoInputs in;` and
//     `VcoInputs in{};` are both fine.
//
// Include hygiene (Pitfall 1 / TEST-01 / TEST-06): ZERO Rack-SDK includes.

#include <cstdint>

// RackCompat.hpp is included EXPLICITLY even though DriftEngine.hpp would supply
// it transitively — include-what-you-use, matching src/dsp/LfoCore.hpp:29. This
// is the include tests/check_includes.sh [2/7]'s exact-path exemption exists for
// (plan 30-01): the shim is Rack-FREE, only its filename carries the substring.
#include "dsp/RackCompat.hpp"    // forge::exp2_taylor5 (forge::clamp is no longer used here — see the T-32-01 note at the morph/character guard)
#include "dsp/Waveshape.hpp"     // forge::Waveshape::morphedWave (FROZEN — call it, never edit it)
#include "dsp/MorphBlep.hpp"     // forge::MorphBlep — the morph-aware band-limiting layer (CORE-02 / AA-01..05)
#include "dsp/DriftEngine.hpp"   // forge::DriftEngine

namespace forge {

// Namespace-scope plain constexpr — the src/dsp/MathConst.hpp idiom this file's
// banner mandates above. NOT `inline constexpr` (C++17), NOT an in-class
// `static constexpr` (declaration-only under C++11 → MinGW undefined reference).
// PITCH-01's reference, DELIVERED by Phase 31 rather than anticipated by it:
// the whole pitch chain in step(...) below is built off this one number.
//
// THE MEASURED FIXED OFFSET THIS REFERENCE CARRIES. As a float, 261.6256f is
// exactly 261.6256103515625 — a constant +0.0000685-cent offset from the
// decimal it is written as. That offset is FIXED and NON-CUMULATIVE (it shifts
// every note by the same amount, so intervals are exact) and it is four orders
// of magnitude under the one-cent tracking gate. It is worth stating because
// PITCH-04/TEST-02's tracking gate DELIBERATELY computes its ground truth from
// the DECIMAL value rather than from this float, which is part of what makes
// that gate an INDEPENDENT reference instead of a restatement of the code it
// checks. A future editor who "corrects" the gate to read this constant would
// silently remove that independence — and would recover only 0.0000685 cents.
constexpr float kVcoFreqC4 = 261.6256f;         // C4 = 0 V, the standard VCV V/OCT reference (PITCH-01, delivered Phase 31)

// PITCH-04's Nyquist policy, SETTLED (D-11; .planning/research/STACK.md:122).
// POLICY: clamp the oscillator frequency to kVcoNyquistGuardFrac * sampleRate,
// i.e. 0.5 x sampleRate x 0.99 — half the sample rate with a one-percent guard
// band. The DERIVED ceilings are what make the choice auditable:
//     44100 Hz -> 21829.5 Hz    48000 Hz -> 23760.0 Hz    96000 Hz -> 47520.0 Hz
// Every one of those is above human hearing, so the clamp is inaudible in
// normal use.
//
// Off the C4 reference those ceilings are the volts at which 1V/oct tracking
// INTENTIONALLY stops — log2(kVcoNyquistGuardFrac * sampleRate / kVcoFreqC4) —
// about +6.3826 V at 44100, +6.5049 V at 48000, +7.5049 V at 96000. Phase 31's
// TEST-02 gate DERIVES those volts from this constant instead of hardcoding
// them (D-21), so moving this constant moves the gate with it.
//
// D-10: this is a HARD CLAMP. The frequency PINS AT THE CEILING and the
// oscillator KEEPS SOUNDING — it does not mute, fade or fold. Under deep FM
// (1.0 oct/V at a full attenuverter means a +/-5 V audio-rate modulator swings
// +/-5 octaves) the clamp fires on most cycles and the peaks flatten out at the
// top. That flattening is the CHOSEN SOUND, not a defect. Amplitude fade above
// the threshold was considered and REJECTED because it adds a gain stage that
// collides with Phase 34's OUT-01..03; pitch fold-back was REJECTED because it
// is a deliberate effect, not the guard PITCH-04 asks for.
//
// D-13 is the counterpart at the LOW end: NO floor is added. Extreme negative
// pitch freezes the phase and the output becomes effectively DC — MEASURED at
// -64 V of pitch: freq = 1.418e-17 Hz, deltaPhase ~ 3.2e-22, an accumulator
// advancing by a denormal-scale amount. That is honest, and it is the decided
// behavior: PITCH-04 speaks only to the top end.
//
// This constant is a Nyquist POLICY bound on the FREQUENCY. It remains a
// different KIND of constant from the wrap-correctness bound on the phase
// INCREMENT, which D-12 leaves untouched — and from the undefined-behavior
// bound on the PITCH VOLTS, which Phase 31 added between the two. All three
// are declared in this block and none of them substitutes for another.
constexpr float kVcoNyquistGuardFrac = 0.495f;  // 0.5 x sampleRate x 0.99 (PITCH-04 / D-11)

// HOSTILE-INPUT bound on the SUMMED PITCH VOLTS, applied BEFORE the exponential
// (D-14 / T-31-01 / T-31-02). This is a third KIND of constant again: not a
// Nyquist policy on the frequency and not a correctness bound on the phase
// increment, but an UNDEFINED-BEHAVIOR bound on the argument handed to a FROZEN
// helper.
//
// WHAT IT GUARDS. forge::exp2Floor (src/dsp/RackCompat.hpp:104-111, reached
// through forge::exp2_taylor5) adds 127 to its argument, casts the result to
// int32_t at :106, and then shifts that integer LEFT BY 23 into a float union
// at :109:
//     x += 127.f;  int32_t xi = (int32_t)x;  ...  yii = xi << 23;
// Casting a NaN, an infinity or an out-of-int32_t float to int32_t is undefined
// behavior, and left-shifting a negative or over-large int32_t is undefined
// behavior as well. Rack does NOT sanitise cable voltages -- an upstream module
// may deliver NaN, +/-infinity or an arbitrary magnitude -- and the FM term
// summed in step(...) below is the FIRST term that routes such a voltage into
// that argument. Before FM, the only route was the V/OCT jack; FM widens it.
//
// WHY THE FIX IS HERE AND NOT THERE. The natural place to bound the argument is
// inside the helper. That helper lives in a FROZEN header: it is byte-pinned by
// tests/check_frozen.sh (which runs inside `make guards`) and it is consumed by
// the SHIPPED Analog LFO, whose goldens replay bit-exact inside `make test`.
// Editing it is a GUARDRAIL EVENT requiring operator sign-off and a golden
// re-verification, not a VCO fix. So the bound is LOCAL to this core, on this
// side of the call. (The shipped LFO reaches the identical latent UB through its
// own FM path -- src/AnalogLFO.cpp:320 into src/dsp/LfoCore.hpp:183-184. That is
// recorded as a deferred item, deliberately unfixed, and is why NO repo-wide
// sanitizer gate is wired into `make test` or CI: it would turn the live module
// red. D-24.)
//
// THE MEASURED ENVELOPE. The frozen helper is well defined only while its
// argument lands in [-127, +128], i.e. while x + 127 lands in [0, 255].
//
// THE MEASURED REACHABLE MUSICAL WORST CASE is about 29.08 V: Rack's +/-12 V
// cable norm on V/OCT, plus +/-5 octaves of coarse tune, plus one twelfth of an
// octave of fine tune, plus +/-12 V of FM at the 1.0 octave-per-volt contract a
// full-clockwise attenuverter delivers (D-06). 64 therefore sits about 2.2x
// OUTSIDE anything a legitimate patch can produce -- so this bound can never
// fire on real music -- and about 2.0x INSIDE the undefined-behavior boundary.
//
// WHY 64 AND NOT A NEIGHBOUR. It is a power of two and therefore exactly
// representable as a float, so neither comparison below carries any rounding.
// And the resulting frequency stays FINITE at BOTH extremes -- MEASURED
// 4.826e21 Hz at +64 V and 1.418e-17 Hz at -64 V -- so the ceiling and floor
// downstream operate on real numbers. A bound of 120 or 126 is equally safe for
// the shift, but MEASURED produces a positive INFINITY at the top and hands the
// downstream ceiling an infinity instead of a number, which is strictly less
// clean. 32 V was rejected as too tight (only 1.10x over the reachable 29.08 V).
//
// THE CONSEQUENCE AT THE NEGATIVE BOUND IS DELIBERATE, and it is D-13's stated
// behavior rather than a regression: at -64 V the frequency becomes
// denormal-small (1.418e-17 Hz), the existing negated frequency floor does NOT
// fire because that value is POSITIVE, the phase increment is about 3.2e-22,
// and the output is effectively DC. PITCH-04 speaks only to the top end; no
// low-end frequency floor is added here or anywhere.
constexpr float kVcoMaxPitchVolts = 64.f;       // UB bound on the exp2 argument (D-14), well inside the frozen helper's [-127, +128]

// WRAP-CORRECTNESS bound on the phase increment (plan 30-08 / CR-01 / T-30-01).
// This is a DIFFERENT KIND OF CONSTANT from kVcoNyquistGuardFrac above and must
// not be confused with it. kVcoNyquistGuardFrac is a Nyquist POLICY bound on the
// FREQUENCY, expressed as a fraction of the sample rate, and Phase 31 (PITCH-04)
// has now settled it. kVcoMaxDeltaPhase is a CORRECTNESS bound on the per-sample
// phase INCREMENT, and it exists solely so the single-subtract wrap in step(...)
// below is valid — any value strictly less than 1.0 satisfies the wrap. Phase 31
// left this one alone when it retired the Nyquist constant, exactly as D-12
// required; the margin that narrowed as a result is DOCUMENTED at the wrap
// below rather than absorbed by widening this bound.
// It is a double because the accumulator it bounds is a double.
constexpr double kVcoMaxDeltaPhase = 0.5;

// POD core boundary (D-03; RESEARCH "Recommended VcoInputs field set"). Each
// field maps to the params[]/inputs[]/ProcessArgs source the shell will read;
// the core never sees Rack indices. Field set covers the near-term Phase 30/31
// needs; sync is deferred to Phase 33.
struct VcoInputs {
	float pitchCV = 0.f;        // V/OCT input volts (Phase 30/31)
	float coarse = 0.f;         // coarse tune, octaves (Phase 31)
	float fine = 0.f;           // fine tune, semitones (Phase 31)
	float fmVolts = 0.f;        // exponential FM input volts, summed into the pitch volt domain before the single exp2 (Phase 31)
	float fmAtten = 0.f;        // bipolar FM attenuverter (Phase 31)
	bool  fmConnected = false;  // FM jack patched
	float morph = 0.f;          // post-CV, post-clamp [0,1] (Phase 34; name matches forge::Inputs::morph on purpose so the shared Waveshape/DriftEngine wiring is copy-paste later)
	float character = 0.f;      // post-CV, post-clamp [0,1] (Phase 34)
	float drift = 0.f;          // post-CV, post-clamp [0,1] (Phase 34)
	float sampleTime = 1.f / 44100.f;  // INJECTED by the harness/shell, never read from a global
	float sampleRate = 44100.f;        // INJECTED; the Nyquist clamp (PITCH-04, Phase 31) needs it
};

struct VcoCore {
	// Analog-character engine (CHAR-*). Held and seeded from day one even though
	// Phase 29 never steps it: it makes the driver's seeding discipline real
	// immediately, so Phases 30/34 do not have to change VcoBlockDriver when the
	// DSP lands. Holding + seeding an RNG is not DSP.
	DriftEngine drift;

	// Per-instance oscillator state, mirroring src/dsp/LfoCore.hpp:58-63. ALL
	// THREE members — the double phase accumulator, the Waveshape and now the
	// MorphBlep — are INSTANCE state, not static and not shared. That is
	// literally what CORE-03 asserts and what D-14 binds by name: two cores
	// stepped interleaved must not see each other. `phase` is double for the
	// same reason LfoCore's is (PITFALLS 2.2): a float accumulator loses
	// low-order increments at audio rates over long blocks.
	//
	// `blep` EXTENDS THAT CLAIM TO CARRIED CORRECTION STATE (Phase 32, CORE-02 /
	// AA-01..05 / T-32-18). forge::MorphBlep holds `pending` — the second half of
	// an edge placed on the PREVIOUS sample (D-13) — so from this commit the
	// polyphony-ready guarantee covers a band-limiting accumulator as well as a
	// phase accumulator. It is held BY VALUE, right here beside `wave`: no
	// static, no global, no shared table anywhere in src/dsp/MorphBlep.hpp.
	//
	// THE PROOF IS BEHAVIORAL, NOT A GREP. tests/test_vco_core.cpp's
	// two-instance interleave invariant (D-17) drives two cores sample by sample
	// and requires each to reproduce its solo block bit-exactly; its PERMANENT
	// positive control fails that same check when a deliberately shared static
	// accumulator is introduced, so the invariant is known to be able to fail.
	// `blep` sits inside the window both of them exercise from this commit
	// onward — a static or shared correction accumulator would show up there as
	// a red build rather than as a silent cross-voice bleed.
	double phase = 0.0;
	Waveshape wave;
	MorphBlep blep;

	// --- Last-step telemetry (the shell reads these to feed display atomics;
	//     NOT part of the audio path). Populated by step() each sample. ---
	struct Telemetry {
		float freqHz = 0.f;        // final oscillator frequency (Phase 31)
		float displayPhase = 0.f;  // wrapped phase for the animated display (Phase 35)
		bool  syncFired = false;   // a hard-sync reset fired this sample (Phase 33)

		// seam observability (TEST-01) — populated by step() even while the seam
		// is silent, so the harness's timing-injection assertions are NOT vacuous.
		float lastSampleTime = 0.f;
		float lastSampleRate = 0.f;
		uint32_t stepCount = 0;
	};
	Telemetry tel;

	void seed(uint64_t s0, uint64_t s1 = 0) { drift.seed(s0, s1); }

	// D-11: this five-coefficient copy is the WHOLE of the phase's per-instance
	// divergence. Static component variation only — no OU drift stepping and no
	// per-sample RNG draw anywhere in step(), which is exactly why the shipped
	// LFO's goldens cannot move. Mirrors src/dsp/LfoCore.hpp:102-112 field for
	// field. `characterSpread` is deliberately NOT copied: LfoCore does not copy
	// it either (its shell folds it into in.character), and folding it in here
	// would silently change what character = 1.0 means.
	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) {
		drift.setSpreadSeed(s0, s1);
		wave.triAsymmetrySpread = drift.triAsymmetrySpread;
		wave.sawCurvatureSpread = drift.sawCurvatureSpread;
		wave.squareDutySpread   = drift.squareDutySpread;
		wave.pulseEdgeSpread    = drift.pulseEdgeSpread;
		wave.bleedSpread        = drift.bleedSpread;
	}

	// step() — one sample of the naive morphed oscillator (CORE-01 / D-12/13/14).
	// Records the injected timing so the harness can prove it, counts the sample,
	// then does the pitch -> guard -> accumulate -> waveshape -> scale sequence.
	float step(const VcoInputs& in) {
		tel.lastSampleTime = in.sampleTime;
		tel.lastSampleRate = in.sampleRate;
		++tel.stepCount;

		// D-14 pitch: exp2 off C4 = 0 V, using the frozen Rack polynomial
		// approximation forge::exp2_taylor5 and NEVER libm std::exp2/std::pow —
		// bit-identity of the FM path depends on this exact function.
		//
		// ONE EXPONENTIAL, FED BY A VOLT-DOMAIN SUM (D-01 / FM-03). All four
		// pitch terms — the V/OCT jack, coarse tune in octaves, fine tune in
		// semitones and the gated FM contribution — are added together in the
		// VOLT domain, and the sum passes through EXACTLY ONE
		// forge::exp2_taylor5 call. That is what makes the FM musical
		// EXPONENTIAL FM: a fixed modulator voltage shifts the pitch by a fixed
		// number of SEMITONES no matter what note is playing.
		//
		// THE TWO WAYS TO GET THIS WRONG, both rejected here. Multiplying the
		// resolved FREQUENCY by an exponentiated modulator gives LINEAR-FM
		// behavior at the wrong place in the chain; calling the exponential
		// TWICE and combining the results detunes by a different amount in every
		// octave. This ordering exists to avoid both. The SHIPPED LFO's FM path
		// (src/dsp/LfoCore.hpp:181-187) is the COUNTER-EXAMPLE, not the
		// template: it resolves a frequency first and then multiplies it by an
		// exponentiated modulator, and it scales the modulator down for
		// sub-audio wobble. Exactly ONE thing is borrowed from it — the
		// `in.fmConnected` gate. The frequency multiply is rejected by D-01, the
		// sub-audio scaling factor by D-06, and its low-frequency clamp by D-13.
		//
		// NO DEPTH CONSTANT, BY DECISION (D-06). Full clockwise on the
		// attenuverter is 1.0 OCTAVE PER VOLT, so the FM jack at full depth is
		// simply a SECOND V/OCT input. That contract is expressed by the bare
		// product `in.fmVolts * in.fmAtten` and is recorded here in words: a
		// named constant equal to one would be a multiply the compiler removes
		// and a number with no requirement behind it.
		//
		// THE GATE LIVES IN THE CORE, NOT THE SHELL (D-09 / D-17). With the jack
		// unpatched the FM term is not merely zero, it is NOT EVALUATED, so the
		// unpatched instruction sequence is identical to the pre-FM one whatever
		// in.fmVolts and in.fmAtten happen to hold. The shell forwards raw
		// values and computes nothing.
		//
		// THE CORE OWNS THE SEMITONE-TO-OCTAVE DIVISION (D-05). in.fine is
		// documented in SEMITONES, so it is scaled by 1/12 here rather than in
		// the shell. The POD's documented units do not change.
		//
		// EVERYTHING STAYS IN FLOAT. The whole summation is upstream of the
		// (double) casts on the phase increment below, and the MEASURED float
		// summation error is 0.0011 cents — four orders of magnitude under the
		// one-cent gate. Promoting it to double would diverge from the
		// float-typed POD fields for no measurable gain and would put a needless
		// conversion in the audio path.
		float pitchVolts = in.pitchCV + in.coarse + in.fine * (1.f / 12.f);
		if (in.fmConnected) pitchVolts += in.fmVolts * in.fmAtten;

		// D-14: bound the summed volts BEFORE the exponential, because the thing
		// being protected is the (int32_t) cast and the << 23 shift that the
		// exponential reaches (see kVcoMaxPitchVolts above for the full
		// rationale and the measured envelope). The bound MUST sit between the
		// summation and the call: after the call the undefined behavior has
		// already happened.
		//
		// THE NEGATED COMPARISON COMES FIRST AND IS THE NaN CATCHER, exactly
		// like the frequency floor and the deltaPhase floor below. A NaN fails
		// `pitchVolts > -kVcoMaxPitchVolts`, so the negation is TRUE and the NaN
		// lands on the fallback branch and becomes -64 V; that value then fails
		// the plain upper comparison and survives unchanged. Every non-finite
		// and out-of-range input therefore leaves this pair as a real number
		// inside [-64, +64].
		//
		// forge::clamp IS REJECTED HERE BY NAME (deferred item 3 / CR-02). It is
		// a comparison ladder — `x < lo ? lo : (x > hi ? hi : x)` — and BOTH of
		// its comparisons are FALSE for a NaN, so a NaN passes straight through
		// it UNCHANGED. It is inert against precisely the input class this guard
		// exists to stop.
		//
		// A FALSIFIED PREMISE, CORRECTED IN PLACE (T-32-01). This paragraph used
		// to end with a parenthetical stating that forge::clamp was still the
		// right tool for morph and character further down, "where the inputs are
		// already finite". THAT PREMISE IS NOW FALSE. Plan 32-02 declared a MORPH
		// CV jack on the Rack shell; Rack does not sanitise cable voltages, so an
		// upstream module can deliver a not-a-number straight into
		// forge::VcoInputs::morph and the field is no longer guaranteed finite.
		// The CONCLUSION of this paragraph is untouched — the ladder is still
		// rejected here, for exactly the reason given above — but the exception
		// it granted is withdrawn: morph and character are conditioned with the
		// SAME negated pair, negated line first, at the call site below, and the
		// full rationale is recorded there rather than duplicated here.
		if (!(pitchVolts > -kVcoMaxPitchVolts)) pitchVolts = -kVcoMaxPitchVolts;
		if (pitchVolts > kVcoMaxPitchVolts) pitchVolts = kVcoMaxPitchVolts;

		float freq = kVcoFreqC4 * exp2_taylor5(pitchVolts);

		// The rate is sanitised BEFORE it is scaled, not after (WR-06). Written
		// positively rather than negated, because here the NaN case wants the
		// FALLBACK branch, not the pass-through: `in.sampleRate > 0.f` is false
		// for negatives, for zero and for NaN alike, and all three land on 0.f.
		//
		// MEASURED, computing maxFreq straight from in.sampleRate: a NaN rate
		// makes maxFreq NaN, and every comparison against NaN is false, so the
		// ceiling below never fires and freq passes through COMPLETELY
		// unclamped. At in.sampleRate = NaN, in.pitchCV = +10 that reached
		// tel.freqHz = 267904.625 Hz -- reproduced independently by the code
		// reviewer and the verifier. The floor could not catch it either, since
		// an unclamped freq is positive and so clears `freq > 0.f`, and the four
		// original assertions in scenario four could not see it because the
		// independent kVcoMaxDeltaPhase bound absorbs the oversized frequency
		// and leaves the OUTPUT healthy. Phase 35 is the named future consumer
		// of tel.freqHz for a display, which is where an arbitrary
		// non-Nyquist-relative number would have become user-visible.
		//
		// A non-positive or NaN rate has no meaningful Nyquist limit, so zero is
		// the only defensible ceiling: freq is driven to 0 rather than left
		// unguarded. BIT-IDENTICAL for every finite POSITIVE in.sampleRate --
		// i.e. every rate Rack has ever delivered -- because the ternary returns
		// in.sampleRate unchanged on that entire branch.
		const float safeRate = (in.sampleRate > 0.f) ? in.sampleRate : 0.f;
		const float maxFreq = kVcoNyquistGuardFrac * safeRate;

		// The guard is LOAD-BEARING, not cosmetic, and THE ORDER OF THESE TWO
		// LINES IS LOAD-BEARING TOO. The floor is written negated so a NaN also
		// lands at zero (NaN fails `freq > 0.f`), and it must run LAST so it is
		// always the final writer.
		//
		// MEASURED, with the two lines the other way round (CR-01, reproduced
		// independently by the code reviewer and the verifier at
		// in.sampleRate = -44100, morph = 0.5, 20000 steps): a non-positive
		// in.sampleRate makes maxFreq negative, the ceiling then writes that
		// negative frequency straight over the value the floor had just
		// sanitised, and the wrap below has no negative branch — so the
		// accumulator is unbounded DOWNWARD. Observed tel.freqHz = -21609.00,
		// phase = -9800.00, |out| = 1.476e38 V, std::isfinite = 0. DO NOT SWAP
		// THESE TWO LINES BACK. Scenario four of tests/test_vco_core.cpp pins it,
		// and plan 30-08's revert-one-only probe P1 observed that scenario going
		// red on freqNonNegative ALONE with this order undone.
		//
		// (Those four figures were measured under the pre-Phase-31 guard
		// fraction of 0.49. They are a HISTORICAL OBSERVATION of a reproduction
		// run, NOT a current expectation, so they are deliberately left at the
		// digits that were actually observed: do not recompute them against
		// 0.495 and do not search-and-replace them when the constant moves. No
		// assertion anywhere reads these numbers — the suite pins this behavior
		// symbolically, through kVcoNyquistGuardFrac itself.)
		//
		// Still true, and still the reason the ceiling exists at all: research
		// MEASURED that without any ceiling, pitchCV = +10 drives the accumulator
		// to phase 1,014,986 and the output to -8,655,011 V while EVERY sample
		// stays std::isfinite — so a finiteness test cannot see that failure; the
		// magnitude bound in tests/test_vco_core.cpp is what observes it.
		//
		// BIT-IDENTITY of the reordering, for every finite POSITIVE in.sampleRate
		// (i.e. every rate Rack has ever delivered): with maxFreq > 0 the two
		// orders agree on all five input classes — NaN, non-positive, in range,
		// above the ceiling, and positive infinity — so nothing the suite already
		// measures can move. Plan 30-08 Task 3 proved that MECHANICALLY with a
		// before/after doctest `-s` diff rather than by this argument.
		if (freq > maxFreq) freq = maxFreq;
		if (!(freq > 0.f)) freq = 0.f;
		tel.freqHz = freq;

		// Double-precision accumulate, mirroring LfoCore. The SINGLE subtract
		// wrap below is correct for any increment inside [0, 1) — and the
		// increment is therefore bounded DIRECTLY, immediately below, rather than
		// being inferred from the frequency guard above.
		//
		// WHY INFERRING IT FROM THE FREQUENCY GUARD IS WRONG (WR-01). The ceiling
		// above is computed from in.sampleRate; the increment is computed from
		// in.sampleTime; and NOTHING in forge::VcoInputs couples the two. MEASURED
		// at in.sampleRate = 44100 with in.sampleTime = 1/1000 and pitchCV = +6:
		// the increment reaches 16.7 and phase reaches 314,880 — an unbounded ramp
		// at a perfectly legitimate sample rate.
		//
		// A FALSIFIED PREMISE, CORRECTED IN PLACE. This paragraph used to end by
		// naming "Phase 32's oversampled inner loop" as the obvious future caller
		// that decouples the two fields on purpose. THAT PREMISE IS FALSE.
		// AA-05 forbids oversampling in v2.0 BY NAME — its own wording is "no
		// minBLEP, no oversampling in v2.0" — so no such loop exists in this
		// phase and none will be added to it. The CONCLUSION is untouched, and
		// it is the part worth keeping: nothing in forge::VcoInputs couples
		// the two fields, the 16.7 / 314,880 measurement above still stands, and
		// the bound below is still required.
		//
		// THE REAL REASON THE DECOUPLED CASE MATTERS, AND WHY IT MATTERS MORE NOW
		// THAN IT DID. in.sampleTime and in.sampleRate are independent POD fields
		// that ANY caller may set independently: the headless harness does
		// exactly that in scenario four of tests/test_vco_core.cpp, and it is the
		// only reason that scenario can reach hostile timing at all. What Phase
		// 32 changed is what sits DOWNSTREAM of that field. Until this commit
		// in.sampleTime was only ever MULTIPLIED — into the increment on the line
		// below. This phase puts a DIVISOR behind it for the first time: the D-03
		// character factor divides by dt, and the sub-sample crossing position in
		// the band-limiter divides by it again. A decoupled, zero, subnormal or
		// non-finite in.sampleTime therefore now reaches arithmetic that did not
		// exist in this file's earlier phases (T-32-02). src/dsp/MorphBlep.hpp
		// carries its OWN negated guard on both of those divisors and refuses to
		// rely on this caller (D-15 / P-14); the bound below remains this core's
		// own, and neither substitutes for the other.
		//
		// WHY 0.5 AND NOT kVcoNyquistGuardFrac. At a COUPLED rate
		// (in.sampleTime == 1 / in.sampleRate) the guarded frequency yields an
		// increment of 0.495 plus float rounding, so a 0.495 ceiling could fire
		// on a legitimate input and MOVE SAMPLES. 0.5 still clears that maximum
		// — by roughly one percent — leaves every existing measurement
		// bit-identical, and still satisfies the wrap (any bound < 1.0 does).
		//
		// That margin NARROWED, from about two percent to roughly one percent,
		// when PITCH-04's policy moved the guard fraction from 0.49 to 0.495.
		// D-12 FORBIDS widening this bound in response, and nothing is lost by
		// leaving it: 0.5 still clears the coupled-rate maximum, so the bound
		// still cannot fire on a legitimate coupled-rate input and still cannot
		// move a sample — which is why every existing measurement stayed
		// bit-identical across that constant change. The margin is DOCUMENTED
		// rather than absorbed. A future phase that raises kVcoNyquistGuardFrac
		// further must RE-CHECK this specific margin rather than assume it
		// survived, because at a guard fraction >= 0.5 this bound WOULD start
		// firing on a legitimate coupled-rate input.
		//
		// The floor is negated for the same reason the frequency floor is: a NaN
		// in.sampleTime makes deltaPhase NaN, which fails `deltaPhase > 0.0` and
		// lands at zero instead of poisoning the accumulator forever.
		double deltaPhase = (double)freq * (double)in.sampleTime;
		if (!(deltaPhase > 0.0)) deltaPhase = 0.0;
		if (deltaPhase > kVcoMaxDeltaPhase) deltaPhase = kVcoMaxDeltaPhase;
		phase += deltaPhase;
		if (phase >= 1.0) phase -= 1.0;

		const float p = (float)phase;

		// T-32-01 — THE MORPH/CHARACTER BOUND, and it is the corrected form of
		// the exception the forge::clamp rejection paragraph above used to grant.
		// The NEGATED comparison comes FIRST and is the NaN catcher, exactly like
		// the pitch-volt bound and the two frequency guards: a not-a-number fails
		// `morph > 0.f`, so the negation is TRUE and it lands at 0.f; the plain
		// upper comparison then leaves that 0.f unchanged.
		//
		// WHAT THE GUARD PROTECTS. The frozen forge::Waveshape::morphedWave
		// computes `morph * 4.f` and casts the result to int
		// (src/dsp/Waveshape.hpp:165-166). A float-to-int cast of a not-a-number
		// is UNDEFINED BEHAVIOR — the same class of bug kVcoMaxPitchVolts exists
		// to stop one call earlier.
		//
		// WHY THE BOUND IS HERE AND NOT THERE, in one sentence, because it is the
		// identical argument: forge::Waveshape.hpp is byte-pinned by
		// tests/check_frozen.sh and consumed by the SHIPPED Analog LFO, so
		// editing it is a guardrail event and not a VCO fix. The bound is
		// therefore LOCAL to this core, on THIS side of the call — the same
		// placement as the pitch-volt bound ahead of the frozen exponential.
		//
		// WHY THE CORE GUARDS EVEN THOUGH THE SHELL ALREADY DOES. Plan 32-02's
		// Rack shell conditions the MORPH CV before it fills forge::VcoInputs.
		// That is not a reason to skip this one: the core MUST NOT RELY ON ITS
		// CALLER, and the headless harness builds forge::VcoInputs DIRECTLY with
		// no shell anywhere in the way — scenario four of tests/test_vco_core.cpp
		// is precisely that caller, and it is the only caller that can reach this
		// line with a hostile value.
		//
		// THE BIT-IDENTITY, so no existing measurement can move. For every finite
		// input this pair returns exactly what the comparison ladder returned: at
		// or below 0 lands at 0, above 1 lands at 1, everything between passes
		// through untouched. The two differ for a not-a-number, where the ladder
		// was inert — and, MEASURED and recorded here rather than glossed, for
		// NEGATIVE ZERO, which this pair normalises to positive zero where the
		// ladder returned it unchanged. That one case is unobservable
		// downstream: the consumers are the float-to-int cast named above, which
		// yields 0 for either sign of zero, and multiply/add chains in which
		// -0.f and +0.f agree. MEASURED at C8, 44.1 kHz, over 4096 samples:
		// morph = -0.f against morph = +0.f differs on 0 samples, and
		// character = -0.f against character = +0.f differs on 0 samples. The
		// whole suite was re-run across this change and no recorded figure moved.
		//
		// MEASURED, the guard doing its job: with BOTH fields set to a
		// not-a-number, 4096 samples are every one of them std::isfinite and the
		// envelope is 5.000000 V — the sine this core falls back to at morph 0,
		// character 0. Before this pair the same input reached the frozen
		// float-to-int cast.
		//
		// CHARACTER IS HARDENED ALONGSIDE MORPH even though its own CV jack is
		// Phase 34's work (CHAR-01). The guard is the same shape for the same
		// reason, the two are handed to the same frozen call in the same argument
		// list, and guarding one of them would leave a half-guarded pair for a
		// later phase to trip over.
		float morph = in.morph;
		if (!(morph > 0.f)) morph = 0.f;
		if (morph > 1.f) morph = 1.f;
		float character = in.character;
		if (!(character > 0.f)) character = 0.f;
		if (character > 1.f) character = 1.f;

		// D-12: ONE call into the frozen Waveshape — a call, never an edit.
		// bleedLfo = 0.f is the OU-layer-0 read, and 0 is correct because this
		// phase steps no OU layer (D-11: no drift stepping, no per-sample RNG).
		// Phase 34 passes the real layer-0 state here.
		const float naive = wave.morphedWave(p, morph, character, 0.f);

		// CORE-02 / AA-01..05 — THE ONE LINE OF PHASE 32 WHERE BAND-LIMITING
		// BECOMES AUDIBLE. Three facts make it correct.
		//
		// (1) THE SAME `p`, WITH THE DOUBLE HANDED OVER SEPARATELY (P-3). The
		//     float given to the band-limiter is the SAME float given to the
		//     frozen call one line above, and the double phase accumulator
		//     travels alongside it as its own argument. THAT SPLIT IS
		//     LOAD-BEARING, NOT STYLISTIC. The correction's SIDE decision must
		//     agree with the frozen branch's own strict float comparison —
		//     disagreement puts the correction on the WRONG SIDE of the edge and
		//     costs the FULL jump, not half of it — while its DISTANCE must come
		//     from the double, because the double advances by exactly the phase
		//     increment and so makes every site fire exactly once per cycle.
		//     MEASURED: a pure-double side test produced a systematic
		//     full-amplitude spike, the envelope rising from 1.957 to 2.145,
		//     whenever the phase grid RESONATED with a site position —
		//     reproduced at dt = 0.0005 against a pulse duty of 0.374, exactly
		//     748 samples per edge. tests/test_morph_blep.cpp's resonant case
		//     walks 165 such increments and was OBSERVED failing (20 assertions,
		//     maximum 1.999949 against a 1.11 bound) with the side decision
		//     deliberately taken from the double.
		//
		// (2) ADDITIVE, NOT REPLACING. morphedWave is still CALLED, exactly once,
		//     exactly as before (CORE-02, D-12) — its result is the `naive` local
		//     above, and the frozen header is neither edited nor bypassed. The
		//     correction is a SEPARATE ADDITIVE TERM. bleedLfo stays 0.f for the
		//     reason given above; Phase 34 passes the real layer-0 state.
		//
		// (3) WHAT THE CORRECTION IS. Morph-aware polyBLEP at the value-step
		//     sites and polyBLAMP at the triangle's slope corners, scaled by the
		//     morph AND bleed weights and by the CHARACTERIZED jump of the frozen
		//     waveshaper, all of it closed-form and table-free (AA-01..05). The
		//     derivation, the nine-site union, the sign convention, the D-03
		//     character factor and every measured magnitude live in
		//     src/dsp/MorphBlep.hpp and are deliberately NOT restated here.
		const float sample = naive + blep.step(wave, phase, p, deltaPhase, morph, character);
		tel.displayPhase = p;

		// D-13: returned UNCONDITIONED by decision — no DC blocker, no
		// saturation, no hard +/-5 V clamp. The measured >5 V overshoot at high
		// character (ceiling ~+/-5.55 V, well inside Rack's +/-12 V norms) is the
		// behavior Phase 34's OUT-01..03 exists to fix; hiding it here would
		// create work Phase 34 must undo.
		return 5.f * sample;
	}
};

} // namespace forge
