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
