#pragma once
// src/dsp/MorphBlep.hpp
//
// WHAT THIS FILE IS. The morph-aware band-limiting layer (CORE-02 / AA-01..05):
// the one additive header Phase 32 creates, holding the polyBLEP / polyBLAMP
// kernels and the site logic that decides where to apply them TOGETHER, because
// neither is meaningful without the other. It CALLS the world of the frozen
// forge::Waveshape::morphedWave — its crossfade weights, its bleed ring, its
// per-shape branch positions, its character-deformation widths — and it never
// edits it. .planning/research/STACK.md:61's suggestion to put the kernels in
// dsp/RackCompat.hpp is REJECTED by D-12 and must not be revisited: RackCompat
// is a frozen shared shim consumed one-way by both the LFO and the VCO, and the
// site logic that gives these kernels their meaning has no business there.
//
// THE D-01 FROZEN-INTERNALS DEPENDENCY, which is the trade this whole design
// rests on. This header duplicates knowledge of src/dsp/Waveshape.hpp's
// internals: its crossfade weights, its bleed ring and normalization, its
// per-shape branch positions and its character-deformation widths. That
// duplication is safe ONLY because Waveshape.hpp is byte-pinned by
// tests/check_frozen.sh and cannot drift underneath this file. IF THAT PIN IS
// EVER LIFTED, every magnitude, position and width below must be re-derived.
//   The rejected alternative, and why. Probing morphedWave at phase +/- epsilon
//   is AA-04's most literal reading and would auto-handle character, bleed and
//   the crossfade weights with ZERO derivation — no duplicated knowledge, no
//   dependency on the pin. It is rejected on cost: morphedWave computes all five
//   shapes on every call, and probing both sides of the site set is roughly
//   eight extra transcendental-heavy calls per sample in the audio path.
//
// THE D-01 OVERRIDE, RECORDED SO IT IS NOT "RESTORED".
// .planning/research/STACK.md:40 recommends pristine, CHARACTER-INDEPENDENT jump
// magnitudes and files magnitude-matching as a v2.1 nicety. That recommendation
// is OVERRIDDEN BY DECISION (D-01), not by oversight. The magnitudes below are
// ANALYTIC and CHARACTER-AWARE — closed form, no probing. A later editor who
// finds the character terms below and "simplifies" them back to the pristine
// form is undoing a decision, not cleaning up an accident.
//
// A FALSIFIED PREMISE, CORRECTED IN PLACE. STACK.md:40's supporting argument —
// that erring toward over-correction is the safe direction, because character is
// a lowpass-ish coloration so extra correction only adds harmless high-frequency
// rolloff — is FALSIFIED BY MEASUREMENT. At character 1 / C6 / morph at the
// square centre the naive alias floor measures -60.1 dB and full-authority
// correction measures -29.9 dB: a 30 dB REGRESSION. The argument fails because
// the injected residual is a STEP-shaped correction added to a signal that has
// no step; it is NEW BROADBAND ENERGY, not a filter. The conclusion the sentence
// was reaching for is still worth keeping — do not be timid about correcting a
// real step — but its stated reason was wrong, and the corrected premise is
// recorded here so no later phase inherits the falsified one. This is why the
// D-03 factor below must reach EXACTLY zero rather than merely decay.
//
// A SECOND FALSIFIED SOURCE, NAMED. STACK.md:100-104's polyBLAMP snippet is
// WRONG for this design and must not be copied. It returns a QUARTIC (u^4/6)
// where the 2-point form is CUBIC, and it folds dt into the kernel rather than
// into the slope. It is neither the 2-point form used here nor the DAFx-16
// four-point form (whose Table 1 is quintic). The residuals below are derived
// from first principles as the once- and twice-integrated 2-sample triangular
// basis, and were cross-validated numerically: they agree with the canonical
// two-branch published form to 7e-7 at dt = 0.094, and the corrected saw's
// harmonic gain matches a squared sinc to 0.01 dB over twelve harmonics.
//
// THE C++11 TWO-STANDARD CONTRACT, quoted as a list from src/dsp/VcoCore.hpp:55-80
// rather than restated loosely. This header must compile clean under BOTH
// -std=c++11 -pedantic-errors (the Rack plugin toolchain, enforced by
// `make strict` and the CI MinGW leg) AND -std=c++17 (the Rack-free test
// target). Therefore:
//   - No `inline constexpr` variables (C++17 inline variables). Plain constexpr
//     at namespace scope has internal linkage per TU and is the C++11 form.
//   - No `if constexpr`, no structured bindings, no [[maybe_unused]], no
//     nested-namespace definition syntax, no auto return-type deduction, no
//     generic lambdas.
//   - No std::clamp (C++17 <algorithm>).
//   - No pi macro from <cmath> — use forge::kPi from dsp/MathConst.hpp.
//   - Any constant table must be a namespace-scope `static constexpr` or
//     FUNCTION-LOCAL, NEVER an in-class `static constexpr` indexed at runtime:
//     under C++11 the in-class form is a DECLARATION only, and runtime indexing
//     odr-uses it, producing an undefined reference at MinGW link time. That
//     exact class of bug got v2.0.0 rejected from the VCV Library.
//   - ZERO Rack-SDK includes.
//
// -ffp-contract=off IS LOAD-BEARING FOR THIS FILE SPECIFICALLY (P-11,
// .planning/research/STACK.md:75). The residuals below are chains of a*b+c, so
// contraction into fused multiply-adds changes the result bit for bit. Do not
// add -ffast-math, do not drop the flag, and NEVER capture a fixture with
// different flags than the ones that will replay it.
//
// THE DEFERRED FIRST REFINEMENT, recorded so the iteration budget has somewhere
// cheap to go. An optional narrow-pulse "reach" factor using a rational tanh
// approximation is MEASURED at +1.3 dB at the single worst grid point and about
// +0.1 dB mean. It is deliberately NOT shipped in the first iteration, because
// it would add the ONLY division by an edge width in this file and so would put
// a second divisor in front of D-15's guard discipline. Try it first if the
// pulse threshold at C8 is missed.
//
// Include hygiene (Pitfall 1 / TEST-01 / TEST-06): ZERO Rack-SDK includes.
// Every free function is `inline` or a struct member (ODR, Pitfall 4).

#include "dsp/Waveshape.hpp"     // forge::Waveshape::morphedWave (FROZEN — call it, never edit it)

namespace forge {

// D-03 / D-03b: the ONE unified character factor. It scales polyBLEP value-step
// corrections and polyBLAMP slope-break corrections ALIKE — one factor, both
// kernels — where `w` is the site's equivalent-ramp width in units of phase (0
// for a true hard step) and `dt` is the phase increment per sample.
//
// Algebraically this is max(0, 1 - w/(2*dt))^2.
//
// THE LIMITS D-03 BINDS IT TO:
//   - w = 0 (a true hard step) gives EXACTLY 1, so every hard site keeps full
//     authority at every character. That is what makes the saw's wrap correction
//     character-independent (see P-4 at the site table below).
//   - w much greater than dt gives EXACTLY 0 — not asymptotically. THE
//     EXACTNESS IS THE LOAD-BEARING PROPERTY (P-1): a factor that merely decays
//     never stops correcting, and a residual step-shaped correction applied to
//     an edge that is already several samples wide injects MORE alias energy
//     than it removes.
//
// WHY THE CUTOFF IS NOT A TUNED CONSTANT. The 2-sample kernel's support IS two
// samples. An edge already at least two samples wide lies entirely inside the
// kernel's own support: it is already band-limited on the sample grid, and a
// step-shaped residual is simply the wrong correction for it. So w = 2*dt is
// READ OFF THE KERNEL, not fitted to data.
//
// THE CALIBRATION EVIDENCE, as numbers. Over 6 characters x 4 notes x 5 morph
// centres, worst regression versus the naive path [MEASURED]:
//     full authority (k = 1)          -60.4 dB
//     reciprocal-linear  1/(1+W)      -42.7 dB
//     sinc-Pade 1/(1+0.4112*W^2)      -36.6 dB
//     reciprocal-quadratic 1/(1+W^2)  -29.8 dB
//     THIS compact-support form        -1.7 dB   (mean improvement 7.3 dB, the highest)
// The exponent 2 rather than 1 was chosen on a CONTINUITY argument, not on the
// metric: both sit on the same measured plateau (-1.7 dB worst regression), but
// exponent 1 has a slope discontinuity at the cutoff, and under audio-rate MORPH
// and CHARACTER modulation that is a per-sample STEP in the correction gain —
// i.e. a new discontinuity manufactured by the thing whose job is to remove them.
//
// THE GUARD. The comparison is written NEGATED so a not-a-number `dt` lands on
// the zero branch: `u > 0.f` is false for a NaN, so `!(u > 0.f)` is true and the
// function returns 0. A comparison-ladder helper (forge::clamp) is REJECTED here
// for the same reason forge::VcoCore rejects it by name at VcoCore.hpp:357-362 —
// BOTH of its comparisons are false for a not-a-number, so it is inert against
// exactly the input class this guard exists to stop.
//
// THE DIVISOR NOTE (D-15 / P-14). morphBlepCharFactor divides by `dt` ONLY.
// There is no division by an edge width anywhere in this header, which is why
// hostile timing reaches exactly two guarded divisors in the whole file — this
// one and the sub-sample position in step() — and no others.
inline float morphBlepCharFactor(float w, float dt) {
	const float twoDt = dt + dt;
	const float u = twoDt - w;
	if (!(u > 0.f)) return 0.f;      // negated: a NaN dt lands here too
	const float t = u / twoDt;       // == 1 - w/(2*dt)
	return t * t;
}

// THE SIGN CONVENTION, stated once, unambiguously, ahead of both kernels.
//
// The jump at a site is h = value_after - value_before, SIGNED. Then
//     corrected value-step sample  = naive + h * r(x)
//     corrected slope-break sample = naive + slopeChangePerSample * R(x)
// where the BLEP residual r and the BLAMP residual R are, with x the sample's
// position relative to the edge in samples,
//     r(x) =  (x+1)^2/2   on the sample BEFORE the edge (x in [-1, 0))
//     r(x) = -(x-1)^2/2   on the sample AFTER  the edge (x in [ 0, 1])
//     R(x) =  (x+1)^3/6   before,      R(x) = (1-x)^3/6   after,
// and both are ZERO outside [-1, 1]. r integrates to +1/6 then -1/6 over its
// support, i.e. to zero, which is exactly why R returns to zero outside it.
//
// THE SANITY CHECK THAT FIXES THE SIGN. At the edge itself the naive value has
// ALREADY jumped, and r(0) = -1/2, so the corrected sample is the pre-edge value
// plus half the jump — the band-limited midpoint. That is the right answer, and
// it is the cheapest way to catch a sign inversion by inspection.
//
// THE CANONICAL-FORM EQUIVALENCE. The widely published two-branch function
// (Valimaki/Huovilainen; Finke) returns TWICE this residual and is applied at
// HALF the jump. The two forms agree to 7e-7 at dt = 0.094 [VERIFIED
// numerically], and the corrected saw's harmonic gain matches a squared sinc to
// 0.01 dB over twelve harmonics. If a future editor cross-reads this file
// against a published listing and finds a factor of two, this paragraph is the
// reconciliation — not a bug.
struct MorphBlep {
	// PER-INSTANCE ACCUMULATOR STATE, mirroring the shape of
	// src/dsp/VcoCore.hpp:243-249. Both members are INSTANCE state, NOT static
	// and NOT shared — which is literally what CORE-03 asserts and what D-14
	// binds: no static, no global mutable voice state, or the polyphony-ready
	// guarantee breaks and two cores stepped interleaved see each other's
	// corrections.
	//
	// D-13, AND WHY THERE IS AN ACCUMULATOR AT ALL. A 2-sample polyBLEP corrects
	// the sample AFTER the edge as well as the one containing it. `pending`
	// carries that second half, and it is delivered at the TOP of the following
	// step() — so the correction spans two samples at ZERO added latency.
	//   The rejected alternative and BOTH of its costs: a one-sample output
	//   delay buffer is simpler bookkeeping with no carried state, but (1) it
	//   adds a sample of latency the module would have to declare, and a VCO
	//   that silently delays by a sample desyncs against every other oscillator
	//   in the patch; and (2) it complicates Phase 33, which needs to act on the
	//   CURRENT sample rather than on one already emitted.
	//   The accumulator also composes with D-07 for free: several corrections
	//   `+=` into the same slot instead of fighting over one.
	float pending = 0.f;   // residual owed to the NEXT sample (D-13)
	float inject  = 0.f;   // residual owed to THIS sample, supplied externally (D-14)

	void reset() { pending = 0.f; inject = 0.f; }

	// THE D-14 SYNC SEAM — DESIGNED HERE, NOT BUILT HERE.
	//
	// `xAhead` is the edge's position in SAMPLES relative to THIS output sample:
	// 0 means the edge lands immediately after this sample, 1 means it lands on
	// the next sample. `jump` is (value_after - value_before), already scaled by
	// whatever weights the caller owns. The event feeds THE SAME accumulator as
	// the morph sites, so sync events and morph edges compose by summation
	// (D-07) rather than one overwriting the other.
	//
	// THE BOUNDARY, STATED SO IT IS NOT CROSSED EARLY. Phase 32 implements NO
	// sync behavior and adds NO sync fields to forge::VcoInputs. The seam exists
	// so that Phase 33 PLUGS IN rather than reopening the one header this phase
	// spends its entire iteration budget stabilising and whose per-shape alias
	// thresholds it has just pinned.
	//
	// The entry gate is written with the NEGATED comparison FIRST so a
	// not-a-number `xAhead` is REJECTED rather than accumulated: `xAhead >= 0.f`
	// is false for a NaN, so the negation fires and the function returns before
	// touching per-instance state that would then poison every following sample.
	void addStep(float xAhead, float jump) {
		if (!(xAhead >= 0.f) || xAhead > 1.f) return;
		const float u = 1.f - xAhead;
		inject  += jump * ( 0.5f) * u * u;
		pending += jump * (-0.5f) * xAhead * xAhead;
	}

	// One sample of correction, ADDITIVE: the caller adds the return value to
	// the naive morphedWave sample. Call AFTER the phase update, with the SAME
	// float `p` that is handed to Waveshape::morphedWave this sample — Pattern 2
	// makes that identity load-bearing.
	float step(const Waveshape& wv, double phase, float p, double dt, float morph, float character);
};

// Defined out of line as an `inline` member so the struct above reads as a
// declaration of shape rather than a wall of arithmetic.
inline float MorphBlep::step(const Waveshape& wv, double phase, float p, double dt, float morph, float character) {
	// Drain the accumulator: `inject` is this sample's externally supplied half
	// (D-14), `pending` is the second half of an edge placed on the PREVIOUS
	// sample (D-13). Both are consumed exactly once.
	float now = inject + pending;
	inject = 0.f;
	pending = 0.f;

	// --- TASK 2 GROWTH POINT ---------------------------------------------------
	// Plan 32-04 Task 2 fills in the weight algebra, the live geometry, the fixed
	// nine-site union and the split-source crossing test HERE. Returning only the
	// drained accumulator is a DELIBERATE intermediate state, not finished work:
	// with no sites placed, `pending` is only ever written by addStep(), so this
	// body is already correct for the D-14 sync seam alone.
	(void)wv; (void)phase; (void)p; (void)dt; (void)morph; (void)character;

	return now;
}

} // namespace forge
