#pragma once
// src/dsp/VcoCore.hpp
//
// VCO core (CORE-01): a POD forge::VcoInputs goes in, an output voltage comes
// out, and last-step telemetry is left behind for the shell. The boundary shape
// is Phase 29's (D-03) and does not churn — Phase 30 filled the seam without
// touching it. step() is now a NAIVE, DELIBERATELY ALIASED morphed oscillator:
// one forge::exp2_taylor5 pitch off the C4 reference, a Nyquist-guarded
// frequency, a double-precision phase accumulator, and one call into the FROZEN
// forge::Waveshape::morphedWave, scaled x5 and returned unconditioned.
//
// The crude, aliased timbre is EXPECTED, not a defect. Phase 32 (CORE-02 /
// AA-01..05) owns band-limiting (morph-aware polyBLEP/polyBLAMP), and no
// assertion over this body claims anything about spectral cleanliness.
// PITCH-04's Nyquist policy is SETTLED as of this commit: kVcoNyquistGuardFrac
// is 0.495f (D-11), so the frequency ceiling below is final rather than a
// placeholder. What Phase 31 still owes is the REST of the pitch chain — coarse,
// fine and FM summed in the volt domain before a single exp2 (PITCH-01..03/05),
// landed by the plan that follows this one; until it does, the pitch handling
// here reads in.pitchCV alone and is knowingly incomplete.
// Phase 34 (OUT-01..03) owns output conditioning; see the x5 note in step().
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
#include "dsp/RackCompat.hpp"    // forge::exp2_taylor5, forge::clamp
#include "dsp/Waveshape.hpp"     // forge::Waveshape::morphedWave (FROZEN — call it, never edit it)
#include "dsp/DriftEngine.hpp"   // forge::DriftEngine

namespace forge {

// Namespace-scope plain constexpr — the src/dsp/MathConst.hpp idiom this file's
// banner mandates above. NOT `inline constexpr` (C++17), NOT an in-class
// `static constexpr` (declaration-only under C++11 → MinGW undefined reference).
constexpr float kVcoFreqC4 = 261.6256f;         // C4 = 0 V, the standard VCV V/OCT reference (PITCH-01)

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
// INCREMENT declared just below, which D-12 leaves untouched.
constexpr float kVcoNyquistGuardFrac = 0.495f;  // 0.5 x sampleRate x 0.99 (PITCH-04 / D-11)

// WRAP-CORRECTNESS bound on the phase increment (plan 30-08 / CR-01 / T-30-01).
// This is a DIFFERENT KIND OF CONSTANT from kVcoNyquistGuardFrac above and must
// not be confused with it. kVcoNyquistGuardFrac is a Nyquist POLICY bound on the
// FREQUENCY, expressed as a fraction of the sample rate, and Phase 31 (PITCH-04)
// replaces it. kVcoMaxDeltaPhase is a CORRECTNESS bound on the per-sample phase
// INCREMENT, and it exists solely so the single-subtract wrap in step(...) below
// is valid — any value strictly less than 1.0 satisfies the wrap. Phase 31 must
// leave this one alone when it retires the Nyquist constant.
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

	// Per-instance oscillator state, mirroring src/dsp/LfoCore.hpp:58-63. Both
	// members are INSTANCE state, not static or shared — which is literally what
	// CORE-03 asserts: two cores stepped interleaved must not see each other.
	// `phase` is double for the same reason LfoCore's is (PITFALLS 2.2): a float
	// accumulator loses low-order increments at audio rates over long blocks.
	double phase = 0.0;
	Waveshape wave;

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
		// bit-identity of the FM path (Phase 31) depends on this exact function.
		// Phase 31 sums coarse/fine/FM into the volt domain BEFORE this call.
		float freq = kVcoFreqC4 * exp2_taylor5(in.pitchCV);

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
		// at a perfectly legitimate sample rate. Phase 32's oversampled inner loop
		// is the obvious future caller that decouples them on purpose.
		//
		// WHY 0.5 AND NOT kVcoNyquistGuardFrac. At a COUPLED rate the guarded
		// frequency yields an increment of 0.49 plus float rounding, so a 0.49
		// ceiling could fire on a legitimate input and MOVE SAMPLES. 0.5 clears
		// that maximum by roughly two percent, leaves every existing measurement
		// bit-identical, and still satisfies the wrap (any bound < 1.0 does).
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
		const float morph = clamp(in.morph, 0.f, 1.f);
		const float character = clamp(in.character, 0.f, 1.f);

		// D-12: ONE call into the frozen Waveshape — a call, never an edit.
		// bleedLfo = 0.f is the OU-layer-0 read, and 0 is correct because this
		// phase steps no OU layer (D-11: no drift stepping, no per-sample RNG).
		// Phase 34 passes the real layer-0 state here.
		const float sample = wave.morphedWave(p, morph, character, 0.f);
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
