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
