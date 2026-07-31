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
	// --- PREAMBLE -------------------------------------------------------------
	// Drain the accumulator: `inject` is this sample's externally supplied half
	// (D-14), `pending` is the second half of an edge placed on the PREVIOUS
	// sample (D-13). Both are consumed exactly once.
	float now = inject + pending;
	inject = 0.f;
	pending = 0.f;

	// D-15 / P-14: MorphBlep REFUSES TO RELY ON ITS CALLER. forge::VcoCore
	// already guards its own deltaPhase, but this header carries its own guard so
	// that any caller — the headless harness, a future polyphonic shell, Phase 33
	// — cannot reach the divisor below with hostile timing. Written NEGATED so a
	// not-a-number `dt` lands here too: `fdt > 0.f` is false for a NaN, zero,
	// a negative and a subnormal that flushed to zero alike. NEVER a
	// comparison-ladder helper (forge::clamp), which is inert against a NaN.
	// Draining the accumulator BEFORE this guard is deliberate: an already-owed
	// residual is still owed even on a sample the caller mistimed.
	const float fdt = (float)dt;
	if (!(fdt > 0.f)) return now;

	// --- SECTION A — WEIGHT ALGEBRA, REPLICATED, NOT APPROXIMATED -------------
	// (AA-01 / D-05 / P-12.) Every expression here mirrors
	// forge::Waveshape::morphedWave's own, INCLUDING its early-return threshold.
	//
	// P-12, EXPLICITLY: the `< 0.001f` threshold below and the `>= 0.001f` gate
	// on the bleed block must be the FROZEN CODE'S EXACT COMPARISONS
	// (Waveshape.hpp:44, :188). A tolerance "cleaned up" to `> 0.f` or to a
	// different epsilon makes this weight vector disagree with the naive path in
	// a narrow character band that the spectral grid sweeps straight through.
	const float c = (character < 0.001f) ? 0.f : character * character;
	const float scaled = morph * 4.f;
	int segment = (int)scaled;
	if (segment > 3) segment = 3;                    // mirrors the frozen minimum-of-3 (:166)
	const float frac = scaled - (float)segment;
	const float pulseFrac = (scaled > 3.f) ? (scaled - 3.f) : 0.f;                 // :170
	const float pulseDuty = 0.50f - 0.45f * ((pulseFrac < 1.f) ? pulseFrac : 1.f); // :171

	float W[5] = { 0.f, 0.f, 0.f, 0.f, 0.f };
	if (segment == 3) {
		// The frozen direct-duty special case (:179-182): the square-to-pulse
		// region does NOT crossfade two rectangles, it interpolates the duty, so
		// all of the weight sits on the pulse.
		W[4] = 1.f;
	} else {
		W[segment]     += 1.f - frac;
		W[segment + 1] += frac;
	}

	// THE BLEED RING (:188-212), gated on the SAME comparison the frozen block
	// uses.
	//
	// D-05, AND WHY FOLDING THE NORMALIZATION IN HERE IS EXACT RATHER THAN AN
	// APPROXIMATION. The frozen path divides its whole result by (1 + bleedIntensity).
	// That normalization is LINEAR, so scaling the weight vector by 1/(1+bi) ONCE
	// is algebraically identical to scaling every correction by it individually —
	// and it is one multiply instead of nine divides. Applying corrections at
	// full magnitude after the naive path has been normalized would over-correct
	// by exactly (1 + bleedIntensity), and precisely when character > 0, which is
	// precisely when the bleed sites exist at all.
	//
	// THE BLEED-RING FINDING, WITH ITS NUMBERS, because nothing in AA-01's
	// wording points at it. At morph = 0 and character = 1 the segment is 0 and
	// frac is 0, so the left weight is 1 and the ring's LEFT index is 4 — THE
	// NARROW PULSE BLEEDS IN AT FULL INTENSITY inside what the user hears as a
	// pure sine. That is a step of about 0.077 in a +/-1 wave, roughly -22 dB,
	// and it is MEASURED as a genuine -23 dB alias floor at C9 / character 1. AN
	// IMPLEMENTATION THAT BAND-LIMITS ONLY "THE SHAPES BEING CROSSFADED" IS
	// SILENTLY WRONG AT EVERY MORPH POSITION.
	//   The verification of the whole algebra: at morph = 0, character = 0.5 the
	//   jump predicted here at phase 0 is bi*2*(1-c)/(1+bi) = 0.014851, and the
	//   frozen path MEASURES 0.014853.
	if (character >= 0.001f) {
		float eb = 0.04f + wv.bleedSpread;           // :192, clamped non-negative
		if (eb < 0.f) eb = 0.f;
		float bi = c * eb;                           // :193-198 with bleedLfo = 0
		if (bi < 0.f) bi = 0.f;
		W[(segment - 1 + 5) % 5] += bi * (1.f - frac);   // :201, :205, :208
		W[(segment + 2) % 5]     += bi * frac;           // :202, :206, :208
		const float norm = 1.f / (1.f + bi);             // :212 — D-05, exact
		for (int i = 0; i < 5; ++i) W[i] *= norm;
	}

	// --- SECTION B — GEOMETRY, RECOMPUTED EVERY SAMPLE (D-04) -----------------
	// Read from the LIVE `wv` fields, never cached.
	//
	// THE WIDTH DERIVATIONS, since these are what the D-03 factor consumes:
	//   - A hyperbolic-tangent edge of half-width `edgeWidth` has unit slope at
	//     its centre and swings 2, so its EQUIVALENT RAMP WIDTH is 2*edgeWidth.
	//     [VERIFIED against the frozen square at duty + 0.05, measuring -0.5546.]
	//   - The triangle's two rounding half-widths sum to roundAmount/2 across the
	//     cycle, so its equivalent width is 0.5 * roundAmount.
	//
	// "RECOMPUTE, NEVER CACHE" AND ITS FORWARD CONSEQUENCE: dutySq, valley and
	// wPl all move with `character` NOW, and will move again PER SAMPLE once
	// Phase 34's drift engine starts writing the spread fields. A cached site
	// table would desynchronise SILENTLY then — no error, no gate, just a
	// correction placed where the edge used to be.
	//
	// The two minima use ternaries rather than library helpers, so this file
	// stays free of standard-library calls (AA-05: table-free, closed form).
	const float dutySq = 0.5f + c * (0.04f + wv.squareDutySpread);              // :108
	const float wSq    = 2.f * (c * 0.08f);                                     // :110
	const float lo     = (pulseDuty < 1.f - pulseDuty) ? pulseDuty : 1.f - pulseDuty;  // :134
	const float capPl  = (0.08f < lo * 0.8f) ? 0.08f : lo * 0.8f;               // :135
	const float wPl    = 2.f * (c * capPl / (1.f + wv.pulseEdgeSpread));        // :135-139
	const float valley = 0.5f + c * (0.10f + wv.triAsymmetrySpread) * 0.5f;     // :60-61
	const float wTri   = 0.5f * (c * 0.35f);                                    // :71
	const float triBrk = 2.f / valley + 2.f / (1.f - valley);                   // :65, :68 slopes
	const float hardSq = W[3] * 2.f * (1.f - c);
	const float hardPl = W[4] * 2.f * (1.f - c);

	// --- SECTION C — THE FIXED NINE-SITE UNION (D-04 / AA-01 / AA-02) ---------
	// FUNCTION-LOCAL const arrays. This form is MANDATORY, not stylistic: an
	// in-class `static constexpr` table is a DECLARATION ONLY under C++11, so
	// indexing it at runtime odr-uses it and the MinGW linker fails with an
	// undefined reference. That exact class of bug got v2.0.0 rejected from the
	// VCV Library (.planning/research/PITFALLS.md:190).
	//
	// ENTRIES 4 AND 5 — P-2 IN FULL, because the spectral gate CANNOT SEE this
	// bug and a later agent will otherwise "simplify" it back into one entry.
	// The frozen square derives its hard rectangle from `phase < 0.5f` (:104) but
	// centres its hyperbolic tangent on duty/2 with half-width duty/2 (:114-120),
	// so the SOFT edge sits at `duty` — A DIFFERENT PLACE. D-04's prose says "the
	// square duty edge", singular; the code has two. If the hard-step correction
	// is placed at `duty`, then for every sample landing between 0.5 and duty the
	// naive path HAS ALREADY FLIPPED while the correction still thinks the edge
	// is ahead, and it injects about half the jump. MEASURED: the output
	// magnitude envelope rises from 1.1047 to 1.96 — from +/-5.52 V to +/-9.78 V
	// — at EVERY sample rate, while the spectral metric shows 0.0 dB difference
	// across the whole grid. The damage window is c*(0.04 + spread) wide and the
	// spike is (1-c)-weighted, so it PEAKS near character 0.71, mid-knob.
	// The PULSE does NOT split: the frozen pulse derives both its branch (:128)
	// and its soft edge (:142-148) from the same duty, so they coincide.
	//
	// ENTRY 1 — P-4, A FALSIFIED PREMISE CORRECTED IN PLACE. D-03's corollary
	// states that as character rises the saw wrap's effective step shrinks and
	// its correction shrinks with it. IT DOES NOT. The curved saw evaluates to 1
	// at phase 0 BEFORE the reset is applied, and the reset blends FROM a reset
	// value of 1 TOWARD the curved saw (:92-97), so at phase 0 both are 1. The
	// wrap jump is +2.000000 at character 0, 0.25, 0.5, 0.75 and 1.0, MEASURED to
	// six decimals. The saw site therefore has width 0 and factor exactly 1 at
	// EVERY character — which is why the saw is the only shape whose correction
	// is character-independent, improving about 9 dB at every character.
	//   What the soft reset ACTUALLY does is force the slope to zero just after
	//   phase 0: a first-derivative break sitting on top of a value step. Its
	//   slope-break magnitude is about 4*dt*dt/6 against the value step's roughly
	//   1 — about three and a half orders of magnitude smaller at dt = 0.02.
	//   IGNORE IT. D-03's CONCLUSION survives untouched; only its stated premise
	//   was wrong, and the corrected premise is recorded here so that no later
	//   phase inherits the falsified one.
	//
	// Entry 1 is the COINCIDENT WRAP: the saw's 1 - 2*phase reset (:84), the
	// square's rectangle flip (:104) and the pulse's rectangle flip (:128) all
	// land on phase 0 together, so they SUM into one magnitude.
	const float pos[9] = {
		0.f,            // 1  coincident wrap
		0.f,            // 2  square soft edge at 0
		0.f,            // 3  pulse soft edge at 0
		0.5f,           // 4  THE SQUARE'S HARD STEP — at 0.5f, NOT at dutySq
		dutySq,         // 5  the square's soft edge at its duty
		pulseDuty,      // 6  pulse hard step
		pulseDuty,      // 7  pulse soft edge, coincident with 6
		0.f,            // 8  triangle peak (slope break)
		valley          // 9  triangle valley (slope break)
	};
	const float mag[9] = {
		W[2] * 2.f + hardSq + hardPl,
		W[3] * 2.f * c,
		W[4] * 2.f * c,
		-hardSq,
		W[3] * (-2.f) * c,
		-hardPl,
		W[4] * (-2.f) * c,
		W[1] * (-triBrk),
		W[1] * (triBrk)
	};
	const float wid[9] = { 0.f, wSq, wPl, 0.f, wSq, 0.f, wPl, wTri, wTri };
	// kind: 0 = value step (polyBLEP), 1 = slope break (polyBLAMP).
	const int kind[9] = { 0, 0, 0, 0, 0, 0, 0, 1, 1 };

	// --- SECTION D — PLACEMENT (D-07 / D-13 / P-3) ----------------------------
	for (int i = 0; i < 9; ++i) {
		// D-04's "magnitudes fall to zero when a shape carries no weight" — and
		// this skip is what makes a FIXED site union cost nothing at the morph
		// positions where a shape is absent.
		if (mag[i] == 0.f) continue;

		// THE CROSSING TEST, AND IT MUST BE WRITTEN EXACTLY THIS WAY.
		//
		// DISTANCE COMES FROM THE DOUBLE. The `phase` accumulator advances by
		// exactly the phase increment each sample, so this distance decreases by
		// exactly dt and each site fires EXACTLY ONCE PER CYCLE. Deriving the
		// distance from the float makes the per-sample interval tiling ragged at
		// the unit-in-last-place level, and a ragged tiling either MISSES an edge
		// or DOUBLE-FIRES it — both half-jump errors.
		//
		// SIDE COMES FROM THE FLOAT, using the SAME float handed to
		// Waveshape::morphedWave this sample. The frozen branches compare against
		// that float with a strict comparison (:104, :128), and if the
		// correction's side decision disagrees with the frozen branch — which it
		// will whenever the site and the phase differ by less than about 6e-8 —
		// the correction is applied on the WRONG SIDE and the error is the FULL
		// jump, not half of it.
		//
		// THE MEASUREMENT THAT SETTLES IT: a pure-double side test produced a
		// systematic full-amplitude spike, the envelope rising from 1.957 to
		// 2.145, whenever the phase grid RESONATED with a site position —
		// reproduced at dt = 0.0005 with a pulse duty of 0.374, exactly 748
		// samples per edge.
		//
		// THE THREE REJECTED SHORTCUTS:
		//   (1) Recomputing the second half at the next sample from the
		//       then-current phase. Valid for a fixed-frequency saw; WRONG here,
		//       because under audio-rate MORPH and FM the jump magnitude, the
		//       site position and dt have all moved by then. The accumulator uses
		//       ONE consistent set of values for both halves.
		//   (2) A one-sample output delay buffer — rejected by D-13 above.
		//   (3) Widening the fire gate above 1 to "catch misses". It creates
		//       double-fires on the following sample instead; the double-sourced
		//       distance already tiles exactly.
		double d = (double)pos[i] - phase;
		if (!(pos[i] > p)) d += 1.0;
		const float s = (float)(d / dt);
		if (!(s <= 1.f)) continue;      // negated: a NaN s is skipped, never fired

		const float k = morphBlepCharFactor(wid[i], fdt);
		if (k == 0.f) continue;         // D-03's exact zero: the edge is already band-limited

		const float u = 1.f - s;

		// BOTH BRANCHES USE `+=`, NEVER ASSIGNMENT (D-07 / AA-03). At C8 a
		// 5-percent-duty pulse is about 0.57 samples wide and at C9 about 0.26
		// samples wide, so BOTH of its edges land inside one sample. Each gets
		// its own sub-sample position and the two opposite-sign corrections SUM.
		// When the duty is narrower than dt they partially CANCEL, and that
		// cancellation is the PHYSICALLY CORRECT band-limited answer — a pulse
		// narrower than a sample genuinely carries less energy.
		//   Flooring the effective duty at dt was REJECTED: it introduces a
		//   sample-rate-dependent timbre change the frozen Waveshape knows
		//   nothing about, so the naive and band-limited paths would stop being
		//   the same waveform — which would also invalidate D-08's before-and-
		//   after comparison, the entire basis of this phase's alias thresholds.
		if (kind[i] == 0) {
			const float h = mag[i] * k;                  // value step: r(-s), r(1-s)
			now     += h * ( 0.5f) * u * u;
			pending += h * (-0.5f) * s * s;
		} else {
			// fdt converts a slope expressed PER UNIT PHASE into a slope PER
			// SAMPLE, which is the unit the BLAMP residual is defined in.
			const float g = mag[i] * fdt * k;            // slope break: R(-s), R(1-s)
			now     += g * (u * u * u) * (1.f / 6.f);
			pending += g * (s * s * s) * (1.f / 6.f);
		}
	}

	return now;
}

} // namespace forge
