// tests/test_morph_blep.cpp
//
// The AA-02 / AA-03 / AA-04 UNIT suite over forge::morphBlepCharFactor and
// forge::MorphBlep. Where tests/test_vco_spectrum.cpp measures the alias FLOOR
// — a broadband average over a 90-cell grid — this file measures the individual
// MAGNITUDES, POSITIONS and SIDE DECISIONS that produce it.
//
// WHY BOTH SUITES EXIST, WITH THE NUMBER THAT SETTLES IT. The spectral gate is
// PROVABLY BLIND to two of this phase's three most likely defects. Placing the
// square's HARD step at its SOFT edge's position swings the output envelope from
// +/-5.52 V to +/-9.78 V at EVERY sample rate — a pre-scale magnitude rising
// from 1.1047 to 1.96 — while showing 0.0 dB spectral difference across the
// whole grid. Sourcing the crossing test's SIDE decision from the double phase
// accumulator instead of from the float `p` injects a systematic
// full-amplitude spike wherever the phase grid RESONATES with a site position,
// and it too measures 0.0 dB spectrally. A suite that only watched the alias
// floor would ship both. Unit assertions are where a wrong magnitude, a wrong
// site position and a wrong side decision are visible, and AA-04 in particular
// is a claim about MAGNITUDES — a spectral floor can be met with the wrong ones.
//
// THE ORGANISING PRINCIPLE THIS REPOSITORY WRITES EVERY SUITE AGAINST, restated
// here because it is what makes this file evidence rather than decoration: Phase
// 29 measured its ENTIRE local gate returning exit 0 on a commit that could not
// link. A test that cannot fail is not evidence. So EACH CASE BELOW IS WRITTEN
// AGAINST A SPECIFIC MEASURED TRAP that would otherwise have made it vacuous,
// and the measurement is written beside the assertion rather than in a planning
// document that will not be read again. Two cases go further and are proved to
// DETECT: the square-split probe (case two, part D) and the resonant-tiling
// envelope (case six), whose sensitivity was observed by temporarily breaking
// the header and watching it go red.
//
// AA-04's MOST LITERAL READING IS THE ONE USED HERE. src/dsp/MorphBlep.hpp
// REJECTS probing forge::Waveshape::morphedWave at phase +/- epsilon for the
// RUNTIME path on cost grounds (D-01: morphedWave computes all five shapes per
// call, so probing both sides of the nine-site set is roughly eight extra
// transcendental-heavy calls per sample in the audio thread). That rejection
// does not apply here. A TEST may spend transcendentals the audio thread cannot,
// and probing is exactly the right instrument for a test whose whole job is to
// check the header's ANALYTIC table against the FROZEN code it was derived from.
// Asserting the table against a restatement of itself would prove nothing.
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it), and
// it is already exempt from check_includes.sh [1/7] — plan 32-01 pre-registered
// it in VCO_SIDE_ALLOW before the file existed, which is the Phase-29 precedent
// for disarming that recurring landmine rather than tripping it.

#include "doctest.h"

#include "dsp/MorphBlep.hpp"    // forge::morphBlepCharFactor, forge::MorphBlep
#include "dsp/Waveshape.hpp"    // forge::Waveshape (FROZEN — probed here, never edited)

#include <cmath>                // std::fabs, std::isfinite, std::nextafter
#include <limits>               // std::numeric_limits — the hostile dt/w grids

namespace {

// The eight phase increments every exact-limit assertion is parametrized over.
// The first three are the production sample rates at C4-ish pitches; 0.02 is the
// working point plan 32-04's probe used; 0.0949 and 0.1897 are C8 and C9 at
// 44.1 kHz, where a 5-percent pulse is about 0.57 and 0.26 samples wide; 0.25
// and 0.495 bracket the top of the reachable range, since forge::VcoCore clamps
// its per-sample increment at kVcoMaxDeltaPhase = 0.5.
const float FACTOR_DTS[] = {
	1.f / 44100.f, 1.f / 48000.f, 1.f / 96000.f,
	0.02f, 0.0949f, 0.1897f, 0.25f, 0.495f
};

// Wrap a probe position that ran off the bottom of the cycle, so a site sitting
// at phase 0 is probed ACROSS THE WRAP rather than at a clamped edge.
double wrapPhase(double x) { return (x < 0.0) ? x + 1.0 : x; }

// The frozen pulse duty, computed the way forge::Waveshape::morphedWave computes
// it (Waveshape.hpp:170-171), in FLOAT, in that order.
//
// THIS IS NOT PEDANTRY AND IT WAS NOT FREE. The obvious shortcut — writing the
// 5-percent duty as the literal `0.05f` — IS A DIFFERENT FLOAT. MEASURED:
// 0.5f - 0.45f = 0.0500000119209, while 0.05f = 0.0500000007451, one and a half
// ULP apart. The crossing test in MorphBlep::step compares a site position
// against the float `p` with a STRICT comparison, so a site placed at the
// literal lands on the OPPOSITE SIDE of a sample that falls between the two
// values, and the sub-sample position swings from about 0 to about 1. That
// showed up here as a case-two row reporting a full -1.0 correction where the
// hand-computed reference said nothing should fire at all. Compute the duty;
// never type it.
float frozenPulseDuty(float morph) {
	const float scaled = morph * 4.f;
	const float pulseFrac = (scaled > 3.f) ? (scaled - 3.f) : 0.f;
	return 0.50f - 0.45f * ((pulseFrac < 1.f) ? pulseFrac : 1.f);
}

// ---------------------------------------------------------------------------
// THE TWO PROBES. Both evaluate the FROZEN forge::Waveshape on either side of a
// site and return what actually happens there, so every magnitude assertion in
// case two is a comparison between MorphBlep's ANALYTIC table and the CODE THAT
// TABLE WAS DERIVED FROM. Asserting the table against a restatement of itself
// would be circular and would prove nothing — which is the whole of AA-04.
//
// WHY THIS IS STRUCTURALLY SOUND HERE AND REJECTED IN THE RUNTIME PATH. D-01
// rejects probing for the audio thread on COST: morphedWave computes all five
// shapes on every call, so probing both sides of the nine-site set is roughly
// eight extra transcendental-heavy calls per sample. None of that applies to a
// test, which may spend transcendentals the audio thread cannot. The rejection
// is about budget, not about validity.
//
// THE `eps` ENVELOPE, WHICH IS BOUNDED FROM BOTH DIRECTIONS AND WAS MEASURED.
// There are THREE constraints, and the middle one is the one a reader will miss:
//
//   (1) eps must be LARGE enough that the difference is not float noise.
//       morphedWave returns a float, so it carries about 6e-8 of absolute
//       rounding. probeSlopeBreak DIVIDES by eps, which amplifies that noise by
//       1/eps: at eps = 1e-4 the slope-break estimate carries about 1.2e-3 of
//       noise, MEASURED as -7.998944 against an exact -8. At eps = 1e-3 it
//       carries about 1.2e-4, MEASURED as -8.000016. That is why the slope-break
//       assertions below pass 1e-3 and the value-jump assertions pass 1e-4:
//       these are DIFFERENT optimal eps, and collapsing them to one constant
//       silently weakens one of the two.
//
//   (2) eps must be SMALL enough that the SMOOTH SLOPE across the bracket stays
//       under the tolerance. A probe over +/-eps returns (true discontinuity) +
//       (local slope * 2*eps), and the second term is not always negligible. At
//       the sine centre the sine's own slope at phase 0 is 2*pi, so the bracket
//       contributes 4*pi*eps: 1.26e-3 at eps = 1e-4, which is TWO ORDERS OF
//       MAGNITUDE larger than the 1e-5-scale bleed step being measured there.
//       MEASURED convergence of that probe toward its closed form, 0.014851485:
//           eps = 1e-4 -> 0.016045763   (residual 1.19e-3)
//           eps = 1e-5 -> 0.014970857   (residual 1.19e-4)
//           eps = 1e-6 -> 0.014863187   (residual 1.17e-5)
//           eps = 1e-7 -> 0.014852822   (residual 1.34e-6)
//       The clean factor of ten per decade is the signature of a SLOPE term, not
//       of a second discontinuity, and case two part D asserts that convergence
//       rather than merely picking the eps that happens to pass.
//
//   (3) eps must be small enough that NO OTHER SITE falls inside the bracket.
//       The narrowest site separation used anywhere below is the square's hard
//       step at 0.5 against its soft edge at dutySq, which at character 0.5 sit
//       0.01 apart — 100 times the largest eps used against them.
//
// DO NOT SIMPLIFY THESE INTO ONE HELPER WITH ONE FIXED eps. The three
// constraints above pull in different directions per site, which is exactly why
// eps is a parameter and why each call site states its own value with a reason.
// ---------------------------------------------------------------------------
double probeJump(const forge::Waveshape& wv, float sitePhase, float morph,
                 float character, float eps) {
	const double after  = (double)wv.morphedWave(sitePhase + eps, morph, character, 0.f);
	const double before = (double)wv.morphedWave(
		(float)wrapPhase((double)sitePhase - (double)eps), morph, character, 0.f);
	return after - before;
}

double probeSlopeBreak(const forge::Waveshape& wv, float sitePhase, float morph,
                       float character, float eps) {
	const double a1 = (double)wv.morphedWave(sitePhase + eps, morph, character, 0.f);
	const double a2 = (double)wv.morphedWave(sitePhase + 2.f * eps, morph, character, 0.f);
	const double b1 = (double)wv.morphedWave(
		(float)wrapPhase((double)sitePhase - (double)eps), morph, character, 0.f);
	const double b2 = (double)wv.morphedWave(
		(float)wrapPhase((double)sitePhase - 2.0 * (double)eps), morph, character, 0.f);
	const double slopeAfter  = (a2 - a1) / (double)eps;
	const double slopeBefore = (b1 - b2) / (double)eps;
	return slopeAfter - slopeBefore;
}

// ---------------------------------------------------------------------------
// THE DRIVER. One sample of the real call sequence, in EXACTLY the order
// forge::VcoCore::step uses it (VcoCore.hpp:470-484): advance the DOUBLE phase
// accumulator, apply the SINGLE-SUBTRACT wrap, cast to float, then hand BOTH the
// double `phase` and that same float `p` to MorphBlep::step.
//
// MIRRORING THAT ORDER IS THE POINT, NOT A CONVENIENCE. Pattern 2 makes one
// identity load-bearing: the float `p` handed to the correction must be the SAME
// float that would be handed to Waveshape::morphedWave this sample. The frozen
// branches compare against that float with a strict comparison, so if the
// correction's SIDE decision is taken from anything else the two disagree
// whenever the site and the phase differ by less than about 6e-8, the correction
// lands on the WRONG SIDE, and the error is the FULL jump rather than half of
// it. Nothing inside MorphBlep can detect a caller that breaks this, so the
// driver here is written to be structurally incapable of drifting from the real
// sequence — if VcoCore::step ever changes its order, this helper must change
// with it or every case below is measuring a call pattern that no longer exists.
//
// DEVIATION FROM THE PLAN'S SIGNATURE, DELIBERATE AND NECESSARY. Plan 32-05
// specifies `double startPhase` BY VALUE. That signature cannot express the
// plan's own instruction two sentences later — "run driveOneSite twice" — because
// a by-value phase resets to the start on every call and the second sample would
// re-walk the first. The accumulator's whole claim (D-13: the second half arrives
// on the FOLLOWING sample) is invisible under a by-value phase. The reference
// parameter is what makes the case able to fail.
float driveOneSite(forge::MorphBlep& b, const forge::Waveshape& wv, double& phase,
                   double dt, float morph, float character) {
	phase += dt;
	if (phase >= 1.0) phase -= 1.0;
	const float p = (float)phase;
	return b.step(wv, phase, p, dt, morph, character);
}

// What walkResonant hands back. Kept as a struct so the NON-VACUITY figures
// travel with the property figure and cannot be dropped at the call site.
struct ResonantWalk {
	double maxAbs;      // the pre-scale output envelope, |naive + correction|
	int    fired;       // samples on which the correction was non-zero
	int    cycles;      // full cycles actually walked
	bool   allFinite;
};

// Walk a band-limited stream at one (morph, character, dt) and report the
// envelope. The band-limited value is formed exactly the way plan 32-06 will
// form it in forge::VcoCore: the naive morphedWave sample PLUS the correction,
// both from the same float `p`.
ResonantWalk walkResonant(const forge::Waveshape& wv, float morph, float character,
                          double dt, int minCycles) {
	ResonantWalk r;
	r.maxAbs = 0.0;
	r.fired = 0;
	r.allFinite = true;

	const int n = (int)((double)minCycles / dt) + 4;
	r.cycles = (int)((double)n * dt);

	forge::MorphBlep b;
	double phase = 0.0;
	for (int i = 0; i < n; ++i) {
		phase += dt;
		if (phase >= 1.0) phase -= 1.0;
		const float p = (float)phase;
		const float naive = wv.morphedWave(p, morph, character, 0.f);
		const float corr  = b.step(wv, phase, p, dt, morph, character);
		if (corr != 0.f) ++r.fired;
		const double v = (double)naive + (double)corr;
		if (!std::isfinite(v)) r.allFinite = false;
		const double a = (v < 0.0) ? -v : v;
		if (a > r.maxAbs) r.maxAbs = a;
	}
	return r;
}

// forge::Waveshape::progressiveCurve (Waveshape.hpp:36-38) COMPOSED with the
// `character < 0.001f` gate its callers all sit behind (Waveshape.hpp:43 and
// its four siblings) — the same composition src/dsp/MorphBlep.hpp:270 makes.
// P-12: the `< 0.001f` comparison is the frozen code's exact one, and an epsilon
// "cleaned up" here would make every closed form below disagree with the path it
// is being checked against in a narrow character band.
float frozenCurve(float character) {
	return (character < 0.001f) ? 0.f : character * character;
}

} // namespace

// ---------------------------------------------------------------------------
// 1. The D-03 character factor: two EXACT limits, monotone between them, a
//    continuous slope at the cutoff, and inertness against hostile dt.
//
//    WHY THE EXACTNESS OF THE ZERO IS THE LOAD-BEARING PROPERTY (P-1), as
//    numbers rather than as an argument. Over 6 characters x 4 notes x 5 morph
//    centres, worst regression versus the naive path [MEASURED]:
//
//        full authority (k = 1)          -60.4 dB
//        reciprocal-linear  1/(1+W)      -42.7 dB
//        sinc-Pade 1/(1+0.4112*W^2)      -36.6 dB
//        reciprocal-quadratic 1/(1+W^2)  -29.8 dB
//        THIS compact-support form        -1.7 dB   (mean improvement 7.3 dB)
//
//    Every one of the three decaying forms still returns roughly 0.05 to 0.3 on
//    an edge that is ALREADY several samples wide, and that residual is a
//    step-shaped correction applied to a signal with no step: broadband INJECTED
//    energy, not a filter. A factor that merely decays never stops correcting.
//    That is why the assertions below use bit-exact float comparisons and not an
//    approximate comparator — "close to zero" is precisely the property that is
//    not good enough here.
// ---------------------------------------------------------------------------
TEST_CASE("morph blep: morphBlepCharFactor hits D-03's limits EXACTLY, is monotone between them, and is inert against hostile dt (D-03 / D-15)") {

	SUBCASE("A: the two exact limits") {
		for (int i = 0; i < (int)(sizeof(FACTOR_DTS) / sizeof(FACTOR_DTS[0])); ++i) {
			const float dt = FACTOR_DTS[i];
			CAPTURE(dt);

			// LIMIT ONE: a TRUE HARD STEP (w = 0) keeps FULL authority at every
			// character and every dt. This is what makes the saw's wrap
			// correction character-independent (P-4, pinned in case two part C).
			// Bit-exact: `== 1.0f`, never "within a tolerance of 1".
			const float atZero = forge::morphBlepCharFactor(0.f, dt);
			CAPTURE(atZero);
			CHECK(atZero == 1.0f);

			// LIMIT TWO: EXACTLY zero at and beyond twice dt. The cutoff is READ
			// OFF THE KERNEL rather than fitted: the 2-sample kernel's support IS
			// two samples, so an edge already that wide lies entirely inside the
			// kernel's own support and is already band-limited on the sample grid.
			const float widths[4] = {2.f * dt, 2.5f * dt, 10.f * dt, 1.f};
			for (int j = 0; j < 4; ++j) {
				const float w = widths[j];
				CAPTURE(w);
				const float k = forge::morphBlepCharFactor(w, dt);
				CAPTURE(k);
				CHECK(k == 0.0f);
			}
		}
	}

	SUBCASE("B: monotone decreasing, and the SLOPE argument that fixes the exponent at 2") {
		// dt = 0.02 is the working point; 200 steps puts the last sample of the
		// walk exactly on the cutoff.
		const float dt = 0.02f;
		const int steps = 200;

		float prev = forge::morphBlepCharFactor(0.f, dt);
		REQUIRE(prev == 1.0f);            // non-vacuity: the walk starts at the limit

		bool strictlyDecreasing = true;
		int firstBadStep = -1;
		float lastInside = 0.f;
		float atCutoff = 0.f;

		for (int i = 1; i <= steps; ++i) {
			const float w = (float)i * (2.f * dt) / (float)steps;
			const float v = forge::morphBlepCharFactor(w, dt);
			if (!(v < prev)) {
				strictlyDecreasing = false;
				if (firstBadStep < 0) firstBadStep = i;
			}
			if (i == steps - 1) lastInside = v;
			if (i == steps)     atCutoff   = v;
			prev = v;
		}

		CAPTURE(firstBadStep);
		CAPTURE(lastInside);
		CAPTURE(atCutoff);
		CHECK(strictlyDecreasing);
		CHECK(atCutoff == 0.0f);

		// THE SLOPE ARGUMENT, AND THE WHOLE REASON THE EXPONENT IS 2 RATHER THAN
		// 1. Both exponents sit on the SAME measured plateau — mean improvement
		// 7.3 versus 7.4 dB, an identical -1.7 dB worst regression — so the
		// metric does not choose between them. CONTINUITY does. The un-squared
		// form has a SLOPE DISCONTINUITY at the cutoff, and under audio-rate
		// MORPH and CHARACTER modulation that discontinuity is a per-sample STEP
		// in the correction gain: a new discontinuity manufactured by the very
		// thing whose job is to remove them.
		//
		// THE DISCRIMINATOR, MEASURED on this exact 200-step walk:
		//     squared form (shipped)   last first difference = 2.4999916e-05
		//     un-squared form          last first difference = 4.9999952e-03
		// The 1e-4 bound below sits between them by two orders of magnitude in
		// each direction, so THIS ASSERTION IS WHAT GOES RED if a later agent
		// "simplifies" the square away. Do not loosen it to 1e-2.
		const float lastFirstDifference = lastInside - atCutoff;
		CAPTURE(lastFirstDifference);
		CHECK(std::fabs(lastFirstDifference) < 1e-4f);
	}

	SUBCASE("C: hostile dt and hostile w (D-15 / P-14)") {
		// THE GUARD'S SHAPE, NOT JUST ITS EFFECT. The comparisons in
		// morphBlepCharFactor are written NEGATED so a not-a-number lands on the
		// fallback branch. A comparison-ladder helper (forge::clamp) would be
		// INERT here, for exactly the reason forge::VcoCore rejects it by name at
		// VcoCore.hpp:357-362 and Phase 30 deferred item 3 / CR-02 records: BOTH
		// of its comparisons are false for a not-a-number, so it is powerless
		// against the one input class this guard exists to stop.
		//
		// EACH ROW CARRIES ITS OWN EXPECTED VALUE rather than a blanket "returns
		// zero", because a blanket claim would be FALSE — and the one row where
		// it is false is D-03's own first limit, not a hole:
		//
		//   dt = the smallest positive subnormal float, w = 0  ->  EXACTLY 1.
		//
		// That is CORRECT. A true hard step keeps full authority at EVERY
		// positive dt, however small; there is nothing to soften. The guard is
		// there to stop a DIVISION that would produce a non-finite result, and
		// subnormal/subnormal is 1 exactly in IEEE arithmetic. (Inside
		// MorphBlep::step the same input class never reaches here at all: `dt` is
		// a DOUBLE there and the smallest positive subnormal double casts to
		// 0.0f, so the step-level guard fires first.)
		//
		// POSITIVE INFINITY IS THE ROW THIS SUBCASE WAS WRITTEN FOR. It passes
		// `dt > 0.f`, so the ORIGINAL guard let it through, and then
		// (2*inf - w) / (2*inf) is inf/inf = NOT-A-NUMBER. MEASURED against the
		// header as plan 32-04 landed it: morphBlepCharFactor(0, +inf) and
		// (0.01, +inf) both returned nan. The fix is the upper bound now written
		// into the guard; these two rows are what keep it there.
		struct Row { float dt; float w; float expected; };
		const float kInf  = std::numeric_limits<float>::infinity();
		const float kNaN  = std::numeric_limits<float>::quiet_NaN();
		const float kSub  = std::numeric_limits<float>::denorm_min();

		const Row rows[] = {
			// dt = 0 — no meaningful timing, every width is "already wide"
			{ 0.f,            0.f,   0.f },
			{ 0.f,            0.01f, 0.f },
			{ 0.f,            kNaN,  0.f },
			// dt negative — the reproduced CR-01 class
			{ -1.f / 44100.f, 0.f,   0.f },
			{ -1.f / 44100.f, 0.01f, 0.f },
			{ -1.f / 44100.f, kNaN,  0.f },
			// dt subnormal — see the note above; w = 0 is D-03's limit, not a hole
			{ kSub,           0.f,   1.f },
			{ kSub,           0.01f, 0.f },
			{ kSub,           kNaN,  0.f },
			// dt = +infinity — the row this subcase exists for
			{ kInf,           0.f,   0.f },
			{ kInf,           0.01f, 0.f },
			{ kInf,           kNaN,  0.f },
			// dt = -infinity — caught by the same negated comparison as a negative
			{ -kInf,          0.f,   0.f },
			{ -kInf,          0.01f, 0.f },
			{ -kInf,          kNaN,  0.f },
			// dt = not-a-number — the input class the negation exists for
			{ kNaN,           0.f,   0.f },
			{ kNaN,           0.01f, 0.f },
			{ kNaN,           kNaN,  0.f }
		};

		for (int i = 0; i < (int)(sizeof(rows) / sizeof(rows[0])); ++i) {
			const float dt = rows[i].dt;
			const float w  = rows[i].w;
			const float expected = rows[i].expected;
			CAPTURE(dt);
			CAPTURE(w);
			CAPTURE(expected);

			const float k = forge::morphBlepCharFactor(w, dt);
			CAPTURE(k);

			// FINITE FIRST, ALWAYS. A non-finite factor multiplies straight into
			// the accumulator and poisons per-instance state permanently — the
			// instance never recovers, because `pending` carries it forward
			// forever. This is the assertion that caught it.
			CHECK(std::isfinite(k));
			CHECK(k == expected);
		}
	}
}

// ---------------------------------------------------------------------------
// 2. The site magnitudes ARE the characterized jumps of the frozen Waveshape.
//
//    THIS IS THE WHOLE OF AA-04. src/dsp/MorphBlep.hpp carries an ANALYTIC,
//    CHARACTER-AWARE table of nine site magnitudes derived by hand from the
//    frozen header's internals. D-01 accepts that duplication only because
//    tests/check_frozen.sh byte-pins forge::Waveshape so it cannot drift
//    underneath. This case closes the other half of that trade: it PROBES the
//    frozen wave at each site and checks that what the header actually does
//    matches what the table says it does. If the pin is ever lifted, this case
//    is what goes red first.
// ---------------------------------------------------------------------------
TEST_CASE("morph blep: the site magnitudes ARE the characterized jumps of the frozen Waveshape (AA-04 / D-01)") {
	forge::Waveshape wv;

	// The value-jump eps. Large enough to clear float noise, small enough that
	// the smooth-slope term stays under 1e-3 at every site probed in parts A-C.
	// See the helper banner for the full three-sided envelope.
	const float kJumpEps = 1e-4f;

	SUBCASE("A: the HARD sites at character 0, where every soft magnitude is zero by construction") {
		// Character 0 is the cleanest possible isolation: c = 0, so every
		// soft-edge magnitude in the table carries a factor of c and vanishes,
		// and every hard magnitude carries (1 - c) and is at full strength. Each
		// shape therefore isolates without any weighting arithmetic in the way.

		// --- morph 0.50, the SAW centre. Site 1, magnitude W[2]*2 with W[2] = 1.
		{
			const double j = probeJump(wv, 0.f, 0.50f, 0.f, kJumpEps);
			CAPTURE(j);
			INFO("saw centre, wrap at phase 0: site 1 predicts W[2]*2 = +2");
			CHECK(std::fabs(j - 2.0) < 1e-3);
		}

		// --- morph 0.75. THE PLAN'S "SQUARE CENTRE", AND THE CORRECTION THAT
		// MATTERS MOST IN THIS FILE.
		//
		// The numbers below are +2.000000 and -2.000000 EXACTLY, and they are
		// right — but NOT for the reason a reader would assume, and the wrong
		// assumption is load-bearing for part D. At exactly morph = 0.75,
		// `scaled = morph * 4.f` is exactly 3.0, so `segment` is 3 and the FROZEN
		// DIRECT-DUTY SPECIAL CASE fires (Waveshape.hpp:179-182): the
		// square-to-pulse region does not crossfade two rectangles, it
		// interpolates the DUTY, so W[4] = 1 and W[3] STAYS 0. The square's own
		// two sites — entries 4 and 5 — carry NO WEIGHT AT ALL here. What
		// produces the -2 at 0.5 is site 6, the PULSE's hard step, whose position
		// pulseDuty happens to equal 0.5 at this morph.
		//
		// So this row asserts sites 1 and 6, not sites 1 and 4. The knob legend
		// says "square centre"; the frozen five-shape mapping has already handed
		// the whole square-to-pulse segment to the pulse by the time morph
		// reaches 0.75. Anything that needs W[3] LIVE must be probed at
		// morph < 0.75 — which is why part D moves to 0.70.
		{
			const double jWrap = probeJump(wv, 0.f,  0.75f, 0.f, kJumpEps);
			const double jHalf = probeJump(wv, 0.5f, 0.75f, 0.f, kJumpEps);
			CAPTURE(jWrap);
			CAPTURE(jHalf);
			INFO("morph 0.75: W[4] = 1 via the direct-duty special case, W[3] = 0; sites 1 and 6");
			CHECK(std::fabs(jWrap - 2.0) < 1e-3);
			CHECK(std::fabs(jHalf + 2.0) < 1e-3);
		}

		// --- morph 0.70, where the SQUARE's own weight is genuinely live. This
		// row is the one that actually exercises W[3] and therefore the one that
		// asserts the WEIGHT ALGEBRA rather than a saturated +/-2.
		//
		// scaled = 2.8, segment = 2, frac = 0.8, so W[2] = 0.2 (saw) and
		// W[3] = 0.8 (square), with no bleed at character 0. Site 1's magnitude
		// is W[2]*2 + hardSq + hardPl = 0.4 + 1.6 + 0 = +2 (the saw wrap and the
		// square's rectangle flip COINCIDE at phase 0 and SUM), and site 4's is
		// -hardSq = -W[3]*2*(1-c) = -1.6. MEASURED: +1.999920 and -1.600080.
		// A -1.6 that came back as -2.0 would mean the weights were ignored.
		{
			const double jWrap = probeJump(wv, 0.f,  0.70f, 0.f, kJumpEps);
			const double jHalf = probeJump(wv, 0.5f, 0.70f, 0.f, kJumpEps);
			CAPTURE(jWrap);
			CAPTURE(jHalf);
			INFO("morph 0.70: W[2] = 0.2, W[3] = 0.8; site 1 sums the coincident wrap, site 4 is weighted");
			CHECK(std::fabs(jWrap - 2.0) < 1e-3);
			CHECK(std::fabs(jHalf + 1.6) < 1e-3);
		}

		// --- morph 1.00, the PULSE centre, where the frozen duty is 5 percent.
		// Sites 1 and 6, at 0 and at the COMPUTED duty (never the literal 0.05f
		// — see the frozenPulseDuty banner for the measured ULP trap).
		{
			const float duty = frozenPulseDuty(1.f);
			CAPTURE(duty);
			REQUIRE(std::fabs(duty - 0.05f) < 1e-6f);   // non-vacuity: this really is the 5 % pulse

			const double jWrap = probeJump(wv, 0.f,  1.00f, 0.f, kJumpEps);
			const double jDuty = probeJump(wv, duty, 1.00f, 0.f, kJumpEps);
			CAPTURE(jWrap);
			CAPTURE(jDuty);
			CHECK(std::fabs(jWrap - 2.0) < 1e-3);
			CHECK(std::fabs(jDuty + 2.0) < 1e-3);
		}
	}

	SUBCASE("B: the SLOPE breaks at character 0 (AA-02)") {
		// AA-02 EXISTS AS A SEPARATE REQUIREMENT FROM AA-01 BECAUSE THE TRIANGLE
		// CONTRIBUTES NO VALUE JUMP AT ALL. Its discontinuity is in the FIRST
		// DERIVATIVE, so a polyBLEP sees nothing to correct and the shape aliases
		// unimpeded — MEASURED at 15.0 dB worse alias floor at C8 / character 0
		// without a slope-break correction. Sites 8 and 9 are the polyBLAMP pair
		// that fixes it.
		//
		// THE CLOSED FORM: triBrk = 2/valley + 2/(1 - valley), and at c = 0 the
		// frozen valley is exactly 0.5, giving 4 + 4 = 8. The triangle falls at
		// -4 per unit phase and rises at +4, so the break at the PEAK is
		// (-4) - (+4) = -8 and at the VALLEY is (+4) - (-4) = +8.
		//
		// eps = 1e-3, NOT the 1e-4 used for value jumps. probeSlopeBreak divides
		// by eps and so amplifies morphedWave's float rounding by 1/eps: MEASURED
		// -7.998944 at eps = 1e-4 against -8.000016 at eps = 1e-3. Both clear the
		// 1e-2 bound, but the tighter one is the honest instrument.
		const float kSlopeEps = 1e-3f;

		const double atPeak   = probeSlopeBreak(wv, 0.f,  0.25f, 0.f, kSlopeEps);
		const double atValley = probeSlopeBreak(wv, 0.5f, 0.25f, 0.f, kSlopeEps);
		CAPTURE(atPeak);
		CAPTURE(atValley);

		CHECK(std::fabs(atPeak + 8.0) < 1e-2);
		CHECK(std::fabs(atValley - 8.0) < 1e-2);

		// NON-VACUITY: the triangle must genuinely have NO value jump at either
		// site, or the two assertions above would be measuring the wrong thing.
		const double vPeak   = probeJump(wv, 0.f,  0.25f, 0.f, kJumpEps);
		const double vValley = probeJump(wv, 0.5f, 0.25f, 0.f, kJumpEps);
		CAPTURE(vPeak);
		CAPTURE(vValley);
		REQUIRE(std::fabs(vPeak) < 1e-2);
		REQUIRE(std::fabs(vValley) < 1e-2);
	}

	SUBCASE("C: P-4, the falsified premise, made permanent") {
		// D-03's stated COROLLARY claims that as character rises, the saw's soft
		// capacitor reset shrinks the wrap's effective step and the correction
		// shrinks with it. IT DOES NOT, AND THIS IS THE CASE THAT KEEPS THAT
		// CORRECTION FROM BEING RE-INTRODUCED.
		//
		// WHY IT DOES NOT: computeSaw evaluates the curved saw to 1 at phase 0
		// BEFORE the reset is applied, and the reset then blends FROM a reset
		// value of 1 TOWARD the curved saw (Waveshape.hpp:92-97). Both are 1
		// there, so the reset moves nothing. The saw site's width is 0 and its
		// D-03 factor is EXACTLY 1 at every character.
		//
		// WHY IT MATTERS: the saw is the only shape whose alias floor barely
		// moves with character (-15.6 dB at character 0 against -14.7 dB at
		// character 1), so a "self-limiting" correction here would silently
		// UNDER-CORRECT the one shape that never improves on its own. D-03's
		// CONCLUSION survives untouched — do not correct the reset separately,
		// its slope-break magnitude is about three and a half orders of magnitude
		// under the value step — only the stated premise was wrong.
		const float chars[5] = {0.f, 0.25f, 0.5f, 0.75f, 1.0f};

		for (int i = 0; i < 5; ++i) {
			const float ch = chars[i];
			CAPTURE(ch);

			// (i) THE LITERAL P-4 CLAIM, on the SAW ITSELF. MEASURED across the
			// five characters: 1.999600, 1.999805, 1.999821, 1.999847, 1.999884
			// — the residual is the eps bracket, not character.
			const double rawSawJump =
				(double)wv.computeSaw(kJumpEps, ch) - (double)wv.computeSaw(1.f - kJumpEps, ch);
			CAPTURE(rawSawJump);
			CHECK(std::fabs(rawSawJump - 2.0) < 1e-3);

			// (ii) THE SAME CLAIM ON THE MORPHED PATH, and the correction to the
			// plan's wording that measurement forced. On morphedWave the probed
			// jump at the saw centre is NOT 2.0 for character > 0 — MEASURED
			// 1.922966 at character 1 — and the plan's "+2.0 within 1e-3 at every
			// character" is unsatisfiable there. The difference is NOT the saw
			// changing: it is D-05's BLEED NORMALIZATION, which divides the whole
			// result by (1 + bleedIntensity). MorphBlep folds exactly that factor
			// into its weight vector ONCE (one multiply instead of nine divides),
			// so the number the header predicts is 2/(1 + bi), and asserting
			// against the closed form is STRICTER than asserting against 2.0
			// would have been, not weaker.
			//
			// At morph 0.50: segment = 2, frac = 0, so the ring's left index is 1
			// (triangle, which is continuous at phase 0 and contributes no jump)
			// with weight 1, and the right weight is 0.
			const float c  = frozenCurve(ch);
			const double bi = (double)c * 0.04;   // effectiveBleed = 0.04 + bleedSpread, spread 0
			const double predicted = 2.0 / (1.0 + bi);
			const double morphed = probeJump(wv, 0.f, 0.50f, ch, kJumpEps);
			CAPTURE(bi);
			CAPTURE(predicted);
			CAPTURE(morphed);
			CHECK(std::fabs(morphed - predicted) < 1e-3);
		}
	}

	SUBCASE("D: the bleed ring, and the square's TWO distinct positions") {
		// --- THE BLEED RING AT THE SINE CENTRE ------------------------------
		// A sine has no discontinuity of its own. This ENTIRE step is the bleed
		// ring: at morph = 0 the segment is 0 and frac is 0, so the ring's LEFT
		// index is 4 and THE NARROW PULSE BLEEDS IN AT FULL INTENSITY inside what
		// the user hears as a pure sine. An implementation that band-limits only
		// "the shapes being crossfaded" is silently wrong at EVERY morph
		// position, and nothing in AA-01's wording points at it.
		//
		// CLOSED FORM: bleedIntensity * 2 * (1 - c) / (1 + bleedIntensity).
		{
			const float ch = 0.5f;
			const float c  = frozenCurve(ch);
			const double bi = (double)c * 0.04;
			const double closedForm = bi * 2.0 * (1.0 - (double)c) / (1.0 + bi);
			CAPTURE(bi);
			CAPTURE(closedForm);

			// eps = 1e-6 here, four decades tighter than the value-jump default,
			// and the reason is measured rather than stylistic: the sine's own
			// slope at phase 0 is 2*pi, so a +/-eps bracket contributes 4*pi*eps
			// of SMOOTH slope on top of the discontinuity — 1.26e-3 at eps = 1e-4,
			// two orders of magnitude above the 1.5e-2-scale step being measured
			// and far outside the 1e-4 tolerance the plan asks for.
			const double j = probeJump(wv, 0.f, 0.f, ch, 1e-6f);
			CAPTURE(j);
			CHECK(std::fabs(j - 0.0148) < 1e-4);          // the recorded figure
			CHECK(std::fabs(j - closedForm) < 2e-5);      // and the closed form it comes from

			// THE RESIDUAL IS SLOPE, NOT A SECOND DISCONTINUITY — asserted, not
			// assumed. A smooth-slope term shrinks LINEARLY with eps; a missed
			// discontinuity would not shrink at all. MEASURED residuals against
			// the closed form: 1.19e-4 at eps = 1e-5, 1.17e-5 at 1e-6, 1.34e-6 at
			// 1e-7 — a clean factor of ten per decade.
			const double r5 = std::fabs(probeJump(wv, 0.f, 0.f, ch, 1e-5f) - closedForm);
			const double r6 = std::fabs(probeJump(wv, 0.f, 0.f, ch, 1e-6f) - closedForm);
			const double r7 = std::fabs(probeJump(wv, 0.f, 0.f, ch, 1e-7f) - closedForm);
			CAPTURE(r5);
			CAPTURE(r6);
			CAPTURE(r7);
			CHECK(r6 < r5 * 0.5);
			CHECK(r7 < r6 * 0.5);
		}

		// --- THE SQUARE'S TWO POSITIONS (P-2 / T-32-16) ----------------------
		// The frozen square flips its HARD rectangle at `phase < 0.5f`
		// (Waveshape.hpp:104) but centres its hyperbolic tangent on duty/2 with
		// half-width duty/2 (Waveshape.hpp:114-120), so its SOFT edge sits at
		// `duty` — A DIFFERENT POSITION. D-04's prose says "the square duty edge",
		// singular; the code has two, and src/dsp/MorphBlep.hpp keeps them as
		// separate table entries 4 and 5.
		//
		// WHAT MERGING THEM COSTS: for every sample landing between 0.5 and duty
		// the naive path HAS ALREADY FLIPPED while the correction still thinks
		// the edge is ahead, and it injects about half the jump. MEASURED: the
		// output magnitude envelope rises from 1.1047 to 1.96 — from +/-5.52 V to
		// +/-9.78 V — at EVERY sample rate, while the spectral metric in
		// tests/test_vco_spectrum.cpp shows 0.0 dB difference across the whole
		// grid. THIS ASSERTION IS ONE OF ONLY TWO PLACES THAT DEFECT IS VISIBLE.
		//
		// THE MORPH POSITION IS LOAD-BEARING AND THE PLAN'S 0.75 WOULD HAVE BEEN
		// VACUOUS. At morph = 0.75 the frozen direct-duty special case leaves
		// W[3] = 0 (see part A), so the square is not in the mix at all and this
		// comparison would pass on the PULSE's hard step — which does NOT split,
		// because the frozen pulse derives both its branch and its soft edge from
		// the same duty. The check would have been green against a merged square
		// entry. morph = 0.70 puts W[3] = 0.8 genuinely live, which is the same
		// relocation plan 32-04 made for the same reason.
		//
		// THE CHARACTER IS LOAD-BEARING TOO. The hard step is (1-c)-weighted and
		// the soft edge is c-weighted, so the discriminator VANISHES at both ends
		// of the knob. The third row below asserts that vanishing explicitly, so
		// nobody "generalises" this case to character 1 and quietly deletes it.
		struct SplitRow { float character; double minDifference; };
		const SplitRow rows[3] = {
			// character 0.50: MEASURED -1.201655 at 0.5 against -0.002073 at
			// dutySq = 0.51. The -1.2 is itself a D-07 summation — the square's
			// hard step (-1.188119) plus the pulse's, which the bleed ring brings
			// in at the same position (-0.011881) — the exact pair plan 32-04
			// measured.
			{ 0.50f, 0.1 },
			// character 0.71: MEASURED -0.795097 against -0.002064. This is where
			// the (1-c)-weighted spike PEAKS, mid-knob.
			{ 0.71f, 0.1 },
			// character 1.00: MEASURED -0.001661 against -0.002048, a difference
			// of 3.9e-4. The hard step is GONE, because the frozen square returns
			// the pure tanh once c = 1. NEGATIVE row: the discriminator must not
			// be run here.
			{ 1.00f, -1.0 }
		};

		for (int i = 0; i < 3; ++i) {
			const float ch = rows[i].character;
			const float c  = frozenCurve(ch);
			const float dutySq = 0.5f + c * (0.04f + wv.squareDutySpread);
			CAPTURE(ch);
			CAPTURE(dutySq);

			// The two positions must be far enough apart that the two 1e-4
			// brackets cannot overlap, or the comparison is meaningless.
			REQUIRE(dutySq - 0.5f > 20.f * kJumpEps);

			const double atHard = probeJump(wv, 0.5f,   0.70f, ch, kJumpEps);
			const double atSoft = probeJump(wv, dutySq, 0.70f, ch, kJumpEps);
			const double difference = std::fabs(atHard - atSoft);
			CAPTURE(atHard);
			CAPTURE(atSoft);
			CAPTURE(difference);

			if (rows[i].minDifference > 0.0) {
				CHECK(difference > rows[i].minDifference);
			} else {
				// The complement: at character 1 there is no hard step left to
				// find, so BOTH probes return slope-only values near zero.
				CHECK(difference < 0.01);
				CHECK(std::fabs(atHard) < 0.01);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// 3. The pending accumulator delivers the second half at ZERO latency, and
//    drains exactly once (D-13).
//
//    WHY THERE IS AN ACCUMULATOR AT ALL, and why the second half is CARRIED
//    rather than recomputed. A 2-sample polyBLEP corrects the sample AFTER the
//    edge as well as the one containing it. Recomputing that second half at the
//    next sample from the then-current phase is valid for a fixed-frequency saw
//    and WRONG here: under audio-rate MORPH and FM the jump magnitude, the site
//    position and dt have ALL moved by then, so the accumulator's one consistent
//    set of values is strictly more robust. A one-sample output delay buffer was
//    also rejected — a VCO that silently delays by a sample desyncs against every
//    other oscillator in the patch, and it would have to declare that latency.
//
//    THE SIGN-CONVENTION SANITY CHECK, which is the cheapest way to catch an
//    inversion by inspection: at the edge itself the naive value has ALREADY
//    jumped and the residual is minus one half, so the corrected sample is the
//    pre-edge value plus half the jump — the band-limited MIDPOINT.
// ---------------------------------------------------------------------------
TEST_CASE("morph blep: the pending accumulator delivers the second half at zero latency, and drains exactly once (D-13)") {
	forge::Waveshape wv;

	// morph 0.50 / character 0 is the ISOLATED saw wrap: site 1 carries magnitude
	// exactly +2 (W[2] = 1, no bleed at character 0), its width is 0 so the D-03
	// factor is exactly 1, and EVERY OTHER SITE HAS MAGNITUDE ZERO and is skipped
	// by the `mag[i] == 0.f` continue. Nothing else can contribute to the numbers
	// below, which is what makes them readable as a single edge.
	const float morph = 0.50f;
	const float character = 0.f;
	const double dt = 0.02;

	struct Row {
		double startPhase;
		double firstExpected;
		double secondExpected;
		const char* what;
	};
	const Row rows[2] = {
		// The site lands essentially ON this sample (sub-sample position s -> 0):
		// u -> 1, so the firing sample takes the whole +h/2 and the next takes
		// nothing. MEASURED: +0.998001, then -1.0e-6.
		{ 1.0 - dt - dt * 0.001,  1.0,  0.0, "s near 0: first sample carries half the jump" },
		// The site lands essentially on the NEXT sample (s -> 1): u -> 0, so the
		// firing sample takes nothing and the whole -h/2 defers to the pending
		// half. MEASURED: +1.0e-6, then -0.998001.
		{ 1.0 - dt * 1.999,       0.0, -1.0, "s near 1: the whole correction defers to pending" }
	};

	for (int i = 0; i < 2; ++i) {
		CAPTURE(rows[i].startPhase);
		INFO(rows[i].what);

		forge::MorphBlep b;
		double phase = rows[i].startPhase;

		const float first  = driveOneSite(b, wv, phase, dt, morph, character);
		const float second = driveOneSite(b, wv, phase, dt, morph, character);
		const float third  = driveOneSite(b, wv, phase, dt, morph, character);
		CAPTURE(first);
		CAPTURE(second);
		CAPTURE(third);

		CHECK(std::fabs((double)first  - rows[i].firstExpected)  < 0.05);
		CHECK(std::fabs((double)second - rows[i].secondExpected) < 0.05);

		// DRAINED EXACTLY ONCE. The third sample is past the site, nothing new
		// fires, and there is no residue left over from the first firing. This is
		// a bit-exact zero, not a small number: a `pending` that were assigned
		// rather than consumed, or consumed twice, cannot produce it.
		CHECK(third == 0.0f);
		CHECK(b.pending == 0.0f);
		CHECK(b.inject == 0.0f);
	}
}

// ---------------------------------------------------------------------------
// 4. Overlapping pulse edges SUM rather than overwrite at a narrow duty
//    (AA-03 / D-07).
//
//    At C8 a 5-percent-duty pulse is about 0.57 samples wide and at C9 about
//    0.26 samples wide, so BOTH of its edges land inside ONE sample. Each gets
//    its own sub-sample position and the two opposite-sign corrections SUM into
//    the same accumulator slot — `+=`, never `=`. When the duty is narrower than
//    dt they partially CANCEL, and that cancellation is the PHYSICALLY CORRECT
//    band-limited answer: a pulse narrower than a sample genuinely carries less
//    energy.
//
//    THE REJECTED ALTERNATIVE, recorded so it is not "fixed" back in. Flooring
//    the effective duty at dt would guarantee the pulse never thins out at the
//    top of the keyboard, but it would introduce a SAMPLE-RATE-DEPENDENT timbre
//    change that the frozen forge::Waveshape knows nothing about — so the naive
//    and band-limited paths would stop being the same waveform, which would also
//    invalidate D-08's before-and-after comparison, the entire basis of this
//    phase's alias thresholds.
// ---------------------------------------------------------------------------
TEST_CASE("morph blep: overlapping pulse edges SUM rather than overwrite at a narrow duty (AA-03 / D-07)") {
	forge::Waveshape wv;

	const float morph = 1.00f;      // the frozen duty here is 5 percent
	const float character = 0.f;    // both hard sites at full strength, both soft magnitudes zero
	const double dt = 0.19;         // C9 at 44.1 kHz: the pulse is about 0.26 samples wide

	const float duty = frozenPulseDuty(morph);
	CAPTURE(duty);
	REQUIRE(std::fabs(duty - 0.05f) < 1e-6f);
	// NON-VACUITY, GEOMETRIC: the two edges must genuinely be closer together
	// than one sample, or "they land in the same sample" is not a claim about
	// this configuration at all.
	REQUIRE((double)duty < dt);

	int overlaps = 0;
	int strongOverlaps = 0;
	for (int i = 0; i <= 40; ++i) {
		const double start = 0.60 + 0.01 * (double)i;

		// The two sub-sample positions, derived from the SITE GEOMETRY (positions
		// 0 and the frozen duty) and the sign convention stated in
		// src/dsp/MorphBlep.hpp's kernel banner — NOT read back out of the
		// implementation. The expected contributions below are therefore an
		// independent derivation of what should be emitted, which is what lets
		// this case distinguish a SUM from an overwrite.
		double advanced = start + dt;
		if (advanced >= 1.0) advanced -= 1.0;
		const float p = (float)advanced;

		double d0 = 0.0 - advanced;
		if (!(0.f > p)) d0 += 1.0;
		double d1 = (double)duty - advanced;
		if (!(duty > p)) d1 += 1.0;
		const double s0 = d0 / dt;
		const double s1 = d1 / dt;

		const bool both = (s0 <= 1.0) && (s1 <= 1.0);
		if (!both) continue;
		++overlaps;

		// h = +2 at the wrap (site 1) and -2 at the duty edge (site 6), both at
		// character 0 where the D-03 factor is exactly 1. r(-s) = +(1-s)^2 / 2.
		const double alone0 =  2.0 * 0.5 * (1.0 - s0) * (1.0 - s0);
		const double alone1 = -2.0 * 0.5 * (1.0 - s1) * (1.0 - s1);

		forge::MorphBlep b;
		double phase = start;
		const float emitted = driveOneSite(b, wv, phase, dt, morph, character);

		CAPTURE(start);
		CAPTURE(s0);
		CAPTURE(s1);
		CAPTURE(alone0);
		CAPTURE(alone1);
		CAPTURE(emitted);

		// (i) IT IS THE SUM. MEASURED agreement to about 1e-7 across the whole
		// overlap window.
		CHECK(std::fabs((double)emitted - (alone0 + alone1)) < 1e-5);

		// (ii) IT IS NOT EITHER ONE ALONE. This is the discriminator that
		// separates `+=` from `=`: an implementation that OVERWROTE would emit
		// exactly one of these two values. MEASURED at start = 0.80: emitted
		// 0.429363 against alone0 = 0.897507 and alone1 = -0.468144.
		//
		// RUN ONLY WHERE BOTH CONTRIBUTIONS HAVE ROOM, and that restriction is a
		// measurement rather than a convenience. At the two start phases where
		// the second edge is only just entering the sample its contribution is
		// 0.00277 and 0.01108 — SMALLER THAN THIS THRESHOLD — so an overwrite
		// would be indistinguishable from the sum there no matter how the check
		// is written. Asserting anyway would not catch more; it would only make
		// the case fail for a reason that has nothing to do with the property.
		// Those rows are still covered by (i) and (iii). MEASURED: 11 of the 13
		// overlap rows clear the 0.02 floor, and the 0.01 discriminator below is
		// a thousand times the 1e-5 agreement tolerance used in (i).
		const double smaller = (std::fabs(alone0) < std::fabs(alone1))
			? std::fabs(alone0) : std::fabs(alone1);
		CAPTURE(smaller);
		if (smaller > 0.02) {
			++strongOverlaps;
			CHECK(std::fabs((double)emitted - alone0) > 0.01);
			CHECK(std::fabs((double)emitted - alone1) > 0.01);
		}

		// (iii) THE CANCELLATION IS REAL. Opposite signs, so the summed magnitude
		// is strictly below the larger single contribution — the "a narrow pulse
		// carries less energy" claim, stated as an inequality.
		const double larger = (std::fabs(alone0) > std::fabs(alone1))
			? std::fabs(alone0) : std::fabs(alone1);
		CHECK(std::fabs((double)emitted) < larger);

		CHECK(std::isfinite(emitted));
	}

	// NON-VACUITY, EMPIRICAL AND LAST: a sweep that never actually overlapped
	// would have skipped every assertion above and passed for free. MEASURED: 13
	// of the 41 swept start phases put both edges inside one sample, and 11 of
	// those carry a second contribution large enough to run the sum-versus-
	// overwrite discriminator on.
	CAPTURE(overlaps);
	CAPTURE(strongOverlaps);
	REQUIRE(overlaps >= 10);
	REQUIRE(strongOverlaps >= 8);
}

// ---------------------------------------------------------------------------
// 5. The D-14 sync seam feeds the SAME accumulator, its entry gate rejects a
//    bad event, hostile dt reaches no divisor, and state is per-instance.
// ---------------------------------------------------------------------------
TEST_CASE("morph blep: the D-14 sync seam feeds the SAME accumulator, and hostile dt reaches no divisor (D-14 / D-15)") {
	forge::Waveshape wv;

	SUBCASE("A: the seam splits exactly like a morph site, and events COMPOSE") {
		// THE PHASE 32 BOUNDARY, STATED SO IT IS NOT CROSSED EARLY. This seam is
		// DESIGNED here and BUILT in Phase 33. Phase 32 implements no sync
		// behavior, adds no sync field to forge::VcoInputs, and never snaps a
		// reset to exactly zero. The seam exists so Phase 33 PLUGS IN rather than
		// reopening the one header this phase spends its whole iteration budget
		// stabilising and whose per-shape alias thresholds it has just pinned.
		const float xs[5] = {0.f, 0.25f, 0.5f, 0.75f, 1.f};
		const float jump = 2.f;

		for (int i = 0; i < 5; ++i) {
			const float x = xs[i];
			CAPTURE(x);

			forge::MorphBlep b;
			b.addStep(x, jump);
			CAPTURE(b.inject);
			CAPTURE(b.pending);

			// The documented two-half split, restated from the kernel banner's
			// sign convention rather than read out of the implementation.
			CHECK(std::fabs(b.inject  - jump * 0.5f * (1.f - x) * (1.f - x)) < 1e-5f);
			CHECK(std::fabs(b.pending + jump * 0.5f * x * x) < 1e-5f);
		}

		// THE SAME ACCUMULATOR, SHOWN RATHER THAN ASSERTED. Drive ONE morph site
		// to a known sub-sample position and require the seam to produce the
		// IDENTICAL pair. The saw wrap at morph 0.50 / character 0 has magnitude
		// exactly +2, so addStep(s, 2) is the reference. Start phase is chosen so
		// the site lands exactly half a sample ahead.
		{
			const double dt = 0.02;
			const double targetS = 0.5;

			forge::MorphBlep site;
			double phase = 1.0 - targetS * dt - dt;
			const float emitted = driveOneSite(site, wv, phase, dt, 0.50f, 0.f);

			forge::MorphBlep seam;
			seam.addStep((float)targetS, 2.f);

			CAPTURE(emitted);
			CAPTURE(site.pending);
			CAPTURE(seam.inject);
			CAPTURE(seam.pending);

			// MEASURED: both pairs are (+0.250000, -0.250000).
			CHECK(std::fabs((double)emitted - (double)seam.inject) < 1e-5);
			CHECK(std::fabs((double)site.pending - (double)seam.pending) < 1e-5);
		}

		// TWO EVENTS IN ONE SAMPLE COMPOSE BY SUMMATION, not by replacement. This
		// is what lets several sync events and several morph sites share one slot
		// (D-07). MEASURED: inject 0.531250, pending 0.218750 — neither of which
		// is what either call produces alone.
		{
			forge::MorphBlep b;
			b.addStep(0.25f,  2.f);
			const float injectAfterFirst  = b.inject;
			const float pendingAfterFirst = b.pending;
			b.addStep(0.75f, -1.f);

			CAPTURE(injectAfterFirst);
			CAPTURE(pendingAfterFirst);
			CAPTURE(b.inject);
			CAPTURE(b.pending);

			CHECK(std::fabs(b.inject  - ( 2.f * 0.5f * 0.75f * 0.75f + (-1.f) * 0.5f * 0.25f * 0.25f)) < 1e-5f);
			CHECK(std::fabs(b.pending - (-2.f * 0.5f * 0.25f * 0.25f - (-1.f) * 0.5f * 0.75f * 0.75f)) < 1e-5f);

			// The discriminator against replacement: neither member kept the
			// first call's value, and neither holds only the second's.
			CHECK(b.inject  != injectAfterFirst);
			CHECK(b.pending != pendingAfterFirst);
		}
	}

	SUBCASE("B: the seam's entry gate rejects a bad event without touching state") {
		// The gate is written with the NEGATED comparison FIRST so a not-a-number
		// `xAhead` is REJECTED rather than accumulated. That ordering is the whole
		// point: a not-a-number that reached per-instance state would poison every
		// following sample, because `pending` carries it forward forever — exactly
		// the failure mode measured for a +infinity dt and fixed in the same plan
		// as this file.
		const float bad[3] = {
			-0.1f, 1.1f, std::numeric_limits<float>::quiet_NaN()
		};
		for (int i = 0; i < 3; ++i) {
			CAPTURE(bad[i]);
			forge::MorphBlep b;
			b.addStep(bad[i], 2.f);
			CAPTURE(b.inject);
			CAPTURE(b.pending);
			CHECK(b.inject  == 0.0f);
			CHECK(b.pending == 0.0f);
		}
	}

	SUBCASE("C: hostile dt returns the drained accumulator and nothing else (D-15 / P-14 / T-32-02)") {
		// THE CORRECTED D-15 RATIONALE, IN FULL, because this is the assertion
		// that carries it. Phase 31's D-15 pointed the hostile-timing work at
		// Phase 32 on the grounds that "Phase 32's oversampled inner loop is the
		// first real source of exotic timing". THAT PREMISE IS FALSIFIED: AA-05
		// forbids oversampling in v2.0, so no such loop exists or will exist. The
		// CONCLUSION survives on better evidence — this phase introduces division
		// by `dt`, in the D-03 factor and in the sub-sample position, so a zero,
		// subnormal or non-finite sampleTime now reaches arithmetic that did not
		// exist before. The measured scope limit is worth recording too: the
		// shipped formulation divides by `dt` ONLY, there is no division by an
		// edge width anywhere in the header, and MorphBlep guards it ITSELF rather
		// than trusting its caller.
		//
		// THE BYPASS IS THE COVERAGE. These six calls go straight into step() with
		// no driver in the way. Routing them through forge::VcoCore would silently
		// DELETE the scenario while leaving it green, because VcoCore sanitises
		// its own deltaPhase first — that is the plan-30-08 lesson.
		//
		// +INFINITY IS WHY THIS SUBCASE EXISTS IN THIS FORM. It PASSES
		// `fdt > 0.f`, so before the guard gained its upper bound it reached the
		// divisor: `d / dt` went to 0, every live site fired at sub-sample
		// position 0, and morphBlepCharFactor(w, +inf) returned a not-a-number
		// that multiplied into `pending`. MEASURED against the header as plan
		// 32-04 landed it: step() returned nan AND left pending = nan, so ONE
		// hostile sample killed the instance permanently. The other five classes
		// were already correct.
		const double hostile[6] = {
			0.0,
			-1.0 / 44100.0,
			std::numeric_limits<double>::denorm_min(),
			 std::numeric_limits<double>::infinity(),
			-std::numeric_limits<double>::infinity(),
			std::numeric_limits<double>::quiet_NaN()
		};

		for (int i = 0; i < 6; ++i) {
			const double dt = hostile[i];
			CAPTURE(dt);

			forge::MorphBlep b;
			// Prime with a SINGLE-SIDED event so the primed total is genuinely
			// non-zero. A symmetric prime (addStep at 0.5) sums to exactly zero
			// and would make "returns the primed value" unfalsifiable.
			b.addStep(0.f, 0.5f);
			const float primed = b.inject + b.pending;
			CAPTURE(primed);
			REQUIRE(primed != 0.0f);

			const float returned = b.step(wv, 0.3, 0.3f, dt, 0.5f, 0.f);
			CAPTURE(returned);
			CAPTURE(b.inject);
			CAPTURE(b.pending);

			// Draining BEFORE the guard is deliberate: a residual already owed is
			// still owed on a sample the caller mistimed.
			CHECK(std::isfinite(returned));
			CHECK(returned == primed);
			CHECK(b.inject  == 0.0f);
			CHECK(b.pending == 0.0f);
		}
	}

	SUBCASE("D: two instances stepped interleaved never see each other's accumulators (CORE-03 / D-14 / T-32-18)") {
		// CORE-03 is a claim about what is ABSENT — no static, no global, no
		// engine accidentally shared between voices — and absence is what a
		// source-text grep proves badly: it catches the obvious declaration form
		// and misses a function-local static, a shared reference member, a
		// singleton behind an accessor and a shared pointer. So it is tested as
		// the PROPERTY polyphony actually needs.
		// 256 samples, NOT 64, and the reason is measured. The correction stream
		// is SPARSE — it is zero except within one sample of a site — so 64
		// samples contained only 3 non-zero samples per instance and the two
		// streams agreed on 59 of 64 purely because both were silent. A
		// distinguishability test written as "few samples are equal" is therefore
		// WRONG HERE, even though that is exactly how
		// tests/test_vco_core.cpp's invariant 4 states it: that case compares
		// DENSE audio, this one compares a sparse correction. MEASURED at 256
		// samples: 23 differing samples, 11 and 13 non-zero.
		const int n = 256;
		float soloA[256], soloB[256], interA[256], interB[256];

		// Two genuinely different drives: different morph, different character,
		// different phase increment, different start phase and different priming
		// events. Shared mutable state of ANY shape breaks the comparison below.
		const float morphA = 0.50f, charA = 0.f;
		const float morphB = 0.82f, charB = 0.5f;
		const double dtA = 0.02, dtB = 0.013;
		const double startA = 0.11, startB = 0.37;

		{
			forge::MorphBlep a; a.addStep(0.25f, 1.f);
			double ph = startA;
			for (int i = 0; i < n; ++i) soloA[i] = driveOneSite(a, wv, ph, dtA, morphA, charA);
		}
		{
			forge::MorphBlep b; b.addStep(0.75f, -2.f);
			double ph = startB;
			for (int i = 0; i < n; ++i) soloB[i] = driveOneSite(b, wv, ph, dtB, morphB, charB);
		}
		{
			forge::MorphBlep a; a.addStep(0.25f, 1.f);
			forge::MorphBlep b; b.addStep(0.75f, -2.f);
			double pa = startA, pb = startB;
			for (int i = 0; i < n; ++i) {
				interA[i] = driveOneSite(a, wv, pa, dtA, morphA, charA);
				interB[i] = driveOneSite(b, wv, pb, dtB, morphB, charB);
			}
		}

		// VALIDITY FIRST, IN TWO PARTS. Without these, "interleaved == solo" is
		// satisfied for free — by two identical signals, or (the sparse-stream
		// trap measured above) by two streams that are almost entirely SILENT.
		// Both are the same non-vacuity requirement tests/test_vco_core.cpp's
		// invariant 4 carries as requirement (iv), restated for a stream whose
		// resting value is zero.
		int soloDiffer = 0, nonZeroA = 0, nonZeroB = 0;
		for (int i = 0; i < n; ++i) {
			if (soloA[i] != soloB[i]) ++soloDiffer;
			if (soloA[i] != 0.f) ++nonZeroA;
			if (soloB[i] != 0.f) ++nonZeroB;
		}
		CAPTURE(soloDiffer);
		CAPTURE(nonZeroA);
		CAPTURE(nonZeroB);
		REQUIRE(soloDiffer >= 8);   // the two streams are genuinely distinguishable
		REQUIRE(nonZeroA >= 4);     // and each of them actually corrected something
		REQUIRE(nonZeroB >= 4);

		// THE PROPERTY. Compared with a direct float inequality and never an
		// approximate comparator: independence is a BIT-EXACTNESS claim or it is
		// nothing.
		int mismatchA = 0, mismatchB = 0;
		for (int i = 0; i < n; ++i) {
			if (interA[i] != soloA[i]) ++mismatchA;
			if (interB[i] != soloB[i]) ++mismatchB;
		}
		CAPTURE(mismatchA);
		CAPTURE(mismatchB);
		CHECK(mismatchA == 0);
		CHECK(mismatchB == 0);
	}
}

// ---------------------------------------------------------------------------
// 6. RESONANT phase increments do not spike the envelope — the side decision
//    comes from the float `p`, not from the double `phase` (P-3 / T-32-31).
//
//    THIS IS THE P-3 COUNTERPART OF CASE TWO PART D, and it exists for the same
//    reason: the defect it catches measures 0.0 dB on the spectral metric and is
//    visible ONLY on the output envelope.
//
//    WHAT IT CATCHES. Deriving the crossing test's SIDE decision from the double
//    `phase` instead of from the float `p`. The frozen branches compare against
//    `p` with a strict comparison (Waveshape.hpp:104, :128), so whenever the site
//    and the phase differ by less than about 6e-8 the two disagree, the
//    correction is applied on the WRONG SIDE, and the error is the FULL jump
//    rather than half of it. MEASURED as a systematic plus-or-minus-1.0-amplitude
//    spike whenever the phase grid RESONATES with a site position.
//
//    ITS SENSITIVITY IS MEASURED, NOT ARGUED. With src/dsp/MorphBlep.hpp's side
//    line temporarily changed from `if (!(pos[i] > p))` to the equivalent double
//    comparison against `phase`, this grid's envelope rose from 1.000000 to
//    1.999949 and FIVE ROWS breached the bound below. The probe was reverted and
//    `git status --porcelain src/dsp/MorphBlep.hpp` confirmed clean before the
//    commit. A green run of this case is therefore evidence, not an absence of
//    evidence.
//
//    WHY THESE SPECIFIC NUMBERS, because a reader will otherwise "simplify" the
//    grid to round decimals and delete the whole point. 0.0005 and 0.374 are not
//    arbitrary and are not merely "a low note with a narrow pulse": they are the
//    exact pair at which the prototype reproduced the defect, and they are exact
//    BECAUSE 0.374 / 0.0005 is exactly 748 — the sub-sample distance lands on a
//    whole number of samples every cycle, which is precisely the condition that
//    puts the float and the double on opposite sides of the comparison. Rounding
//    either value away destroys the resonance and turns this case green against
//    a broken implementation. The `duty / N` and `0.5 / N` grids are COMPUTED
//    for the same reason: the divisor relation must be exact by construction
//    rather than by luck. Do NOT replace them with hand-typed decimals.
//
//    WHY IT IS A SEPARATE CASE. A general magnitude sweep can miss a resonance
//    that only occurs on a measure-zero set of phase increments. This case goes
//    looking for that set instead of hoping to land in it.
//
//    THE COMPANION ANTI-PATTERN. Do NOT "fix" a suspected miss by widening the
//    fire gate above s = 1. That trades a missed edge for a double-fired edge on
//    the following sample, and both are single-sample full-amplitude clicks. The
//    tiling is exact to about 1e-16 when the distance comes from the double.
// ---------------------------------------------------------------------------
TEST_CASE("morph blep: RESONANT phase increments do not spike the envelope - the side decision comes from the float p, not the double phase (P-3)") {
	forge::Waveshape wv;

	// THE PRE-SCALE ENVELOPE, WITH ITS PROVENANCE. This is the pre-scale form of
	// the analytic ceiling tests/test_vco_core.cpp derives: the sine path is
	// monotone with range about -1.05 to +1.11, the other four shapes are each
	// bounded by 1, the crossfade is a linear interpolation and the bleed step is
	// a convex combination. It is the same number that becomes 5.55 V after the
	// unconditioned five-times output scale forge::VcoCore applies.
	//
	// DECLARED LOCALLY ON PURPOSE rather than shared across translation units. If
	// the two ever disagree, tests/test_vco_core.cpp's is the authority.
	const double kMorphBlepPreScaleEnvelope = 1.11;

	const float morph = 0.82f;

	// PIN THE GEOMETRY TO WHAT RESEARCH MEASURED, not to a morph value that might
	// drift. MEASURED: the frozen duty at morph 0.82 is bit-identical to 0.374f.
	const float duty = frozenPulseDuty(morph);
	CAPTURE(duty);
	REQUIRE(std::fabs(duty - 0.374f) < 1e-6f);

	// AND ASSERT THAT THE RESONANCE ACTUALLY EXISTS rather than assuming it. The
	// nominal pair divides exactly: 0.374 / 0.0005 = 748.000000000000.
	const double nominalRatio = 0.374 / 0.0005;
	CAPTURE(nominalRatio);
	REQUIRE(std::fabs(nominalRatio - 748.0) < 1e-9);

	// THE ULP-SCALE OFFSET IS THE POINT, NOT A BLEMISH. The FLOAT duty the frozen
	// path actually uses is 0.374000013, so the realised ratio is 748.0000257 —
	// 2.6e-5 of a sample away from the exact divisor. That gap is precisely the
	// regime where the float `p` and the double `phase` can land on opposite
	// sides of the site comparison, which is what this case hunts.
	const double realisedRatio = (double)duty / 0.0005;
	CAPTURE(realisedRatio);
	REQUIRE(std::fabs(realisedRatio - 748.0) < 1e-3);

	const int kMinCycles = 40;
	const float characters[3] = {0.f, 0.5f, 1.f};
	const int dutyDivisors[5]   = {373, 500, 748, 1000, 1871};
	const int squareDivisors[5] = {2, 3, 7, 64, 1000};

	int rowsWalked = 0;

	for (int ci = 0; ci < 3; ++ci) {
		const float character = characters[ci];

		// Three families. Family 0 targets the PULSE's hard site by dividing the
		// duty an exact integer number of times; family 1 targets the square's
		// hard site at 0.5 the same way; family 2 is the LITERAL MEASURED POINT,
		// carried as its own row so its provenance survives any later edit to the
		// computed families.
		for (int family = 0; family < 3; ++family) {
			const int count = (family == 2) ? 1 : 5;

			for (int k = 0; k < count; ++k) {
				double base;
				if (family == 0)      base = (double)duty / (double)dutyDivisors[k];
				else if (family == 1) base = 0.5 / (double)squareDivisors[k];
				else                  base = 0.0005;   // dt = 0.0005, 748 samples per edge

				// forge::kVcoMaxDeltaPhase makes anything at or above 0.5
				// unreachable, so a row there would describe nothing.
				if (base >= 0.5) continue;

				// A FEW-ULP NEIGHBOURHOOD around each resonant dt. The defect is a
				// TIE at the unit-in-last-place level: the exact divisor is where
				// the float and the double disagree about which side of the edge
				// the sample is on, so the two steps either side are where a
				// near-miss would show up if the bug were sensitive to rounding
				// rather than to the resonance itself.
				for (int step = -2; step <= 2; ++step) {
					double dt = base;
					for (int q = 0; q < ((step < 0) ? -step : step); ++q)
						dt = std::nextafter(dt, (step < 0) ? 0.0 : 1.0);

					const ResonantWalk r = walkResonant(wv, morph, character, dt, kMinCycles);
					++rowsWalked;

					CAPTURE(character);
					CAPTURE(family);
					CAPTURE(step);
					CAPTURE(dt);
					CAPTURE((double)duty / dt);
					CAPTURE(r.maxAbs);
					CAPTURE(r.fired);
					CAPTURE(r.cycles);

					// --- NON-VACUITY FIRST -----------------------------------
					// A configuration where no site ever fired would satisfy an
					// envelope bound trivially. MEASURED tightest margin across
					// the whole grid: fired - cycles = 0, i.e. every row fires at
					// least once per cycle and most fire three or four times.
					REQUIRE(r.cycles >= kMinCycles);
					REQUIRE(r.fired >= r.cycles);

					// --- THE PROPERTY ----------------------------------------
					// MEASURED maxima: 1.000000 at character 0, 0.997545 at 0.5
					// and 0.988873 at 1.0 — every row comfortably inside the
					// bound. Under a double-sourced side decision the same grid
					// reaches 1.999949.
					CHECK(r.allFinite);
					CHECK(r.maxAbs <= kMorphBlepPreScaleEnvelope);
				}
			}
		}
	}

	// The grid must actually have been walked: 3 characters x 11 base increments
	// x 5 unit-in-last-place steps.
	CAPTURE(rowsWalked);
	REQUIRE(rowsWalked == 165);
}
