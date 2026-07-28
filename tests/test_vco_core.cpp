// tests/test_vco_core.cpp
//
// CORE-01 behavioral suite over the live forge::VcoCore, driven headless through
// tests/VcoBlockDriver.hpp. Where tests/test_vco_harness.cpp proves the harness
// PLUMBING — timing injection, non-degenerate seeding, seam determinism — this
// file proves the OSCILLATOR ITSELF: does it play the note its input asks for,
// does it stay inside a stated voltage bound, and does a differently-seeded
// instance actually sound different.
//
// Organising principle. Phase 29 measured its ENTIRE local gate returning exit 0
// on a commit that could not link. The lesson that produced is the shape of this
// file: a test that cannot fail is not evidence. So each case below is written
// against the specific MEASURED trap that would otherwise have made it vacuous.
// For invariant 1 that trap is the telemetry shortcut — asserting on
// `tel.freqHz` only re-reads the number step(...) computed three lines earlier,
// so it would stay green even if the phase accumulator ignored the frequency
// entirely. The invariant is therefore measured on the RETURNED SAMPLES, the
// signal a user would actually hear.
//
// Invariants (numbered so plan 30-04 can append 4 and 5 for CORE-03 without
// renumbering anything here):
//   1. naive pitch tracks the C4 reference ON THE OUTPUT within 1 % across the
//      full measured-safe grid (D-16) — explicitly NOT the TEST-02 tracking
//      gate; see the label below
//   2. |out| stays inside a LOOSE 6.0 V bound over the harness sweep, the fixed
//      worst case and hostile V/OCT — and the worst case is proven to actually
//      exceed 5.1 V, so the bound is exercised rather than merely satisfied
//      (D-18b / T-30-01)
//   3. two instances differing ONLY in spread seed diverge measurably at
//      character = 1.0, with bit-identity at character = 0 pinned as the
//      in-test control (D-18a / D-10 / D-11)
//
// THE D-16 LABEL, WHICH MUST NOT BE SOFTENED. Invariant 1 is NOT the TEST-02
// V/Oct tracking gate. TEST-02 belongs to Phase 31 and requires better than one
// cent ACROSS THE PITCH RANGE WITH COARSE, FINE AND FM SUMMING — none of which
// exist yet, because this step(...) body reads in.pitchCV alone. The worst error
// measured anywhere inside invariant 1's grid is 0.0078 %, which is already
// better than one cent (0.0578 %), and that is exactly why the label is written
// twice, here and at the case: a reader who sees those numbers could reasonably
// but WRONGLY conclude Phase 31's gate is already met. Do not soften the label
// because the numbers look good.
//
// THE PHASE-30 OSCILLATOR ALIASES BY DESIGN. step() is a naive, deliberately
// unband-limited morphed oscillator (D-12). The crude timbre is the expected
// Phase-30 result, not a defect. Phase 32 (CORE-02 / AA-01..05) owns
// band-limiting via morph-aware polyBLEP/polyBLAMP and owns the alias floor.
// NO assertion in this file may be written about alias content, harmonic
// structure or spectral cleanliness — not now, and not when the numbers here
// start looking respectable.
//
// Deliberately NOT here: harness plumbing (tests/test_vco_harness.cpp), the
// < 1-cent V/Oct tracking gate (Phase 31, TEST-02), the alias floor (Phase 32),
// and output conditioning plus the MOVING drift engine (Phase 34, OUT-01..03 /
// DRIFT-*). Also not here yet: the CORE-03 independence PAIR — the interleave
// test and its deliberately-broken shared-state positive control — which plan
// 30-04 appends to this same file as invariants 4 and 5, reusing the helpers
// below and adding its own into the same anonymous namespace.
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "VcoBlockDriver.hpp"

#include <vector>
#include <cmath>
#include <cstdint>

namespace {

// The three production sample rates every invariant is parametrized over.
// Plan 30-04 appends its CORE-03 helpers — the deliberately-broken shared-state
// stand-in and the interleave runner — into this SAME anonymous namespace, and
// owns none of the three helpers defined here.
constexpr double SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0};

// Baseline core input. Built by default construction + field assignment, never a
// brace value-list (VcoInputs has NSDMIs, so under C++11 it is not an aggregate
// and a value-list init is a hard error — P-8). Kept C++11-shaped even though
// the tests build at C++17, because this same idiom is copied into src/ where
// C++11 is binding.
//
// Deliberately NEUTRAL: every case below overrides pitch, morph and character
// explicitly, so no grid point can silently inherit a value it did not state.
forge::VcoInputs coreBase() {
	forge::VcoInputs in;
	in.pitchCV   = 0.f;
	in.coarse    = 0.f;   // Phase 31 — unread by this step() body
	in.fine      = 0.f;   // Phase 31 — unread
	in.morph     = 0.f;
	in.character = 0.f;
	in.drift     = 0.f;   // Phase 34 — unread
	return in;
}

// Frequency estimator: rising zero crossings with linear SUB-SAMPLE
// interpolation, measured first-crossing to last-crossing. Returns Hz, reports
// the crossing count through nUp, and returns a negative sentinel when fewer
// than two crossings were found.
//
// Why counting crossings is structurally sound here, rather than a hopeful
// heuristic: across a 401 x 101 grid of (morph, character) = 40,501 points,
// sampled at 20,000 points per cycle with this driver's default spread, the
// continuous waveform produced EXACTLY two sign changes per cycle at every
// single point — zero exceptions — hence exactly one RISING crossing per cycle.
// The maximum per-cycle DC anywhere on that grid was 0.8995 (the 5 %-duty pulse
// at morph = 1.00, character = 0.00), and even there the count is exactly 2. The
// only measured failure mode is sampling loss in the narrow-pulse region, which
// the grid below stays out of by construction.
//
// Why the sub-sample interpolation is load-bearing rather than a nicety: the
// naive `crossings / 2 / duration` form carries a 0.5 / cyclesInWindow
// quantization error, MEASURED at -2.15 % on a 250 ms window at pitchCV = -2.
// Adopting it would force a tolerance roughly 200x looser than the 1 % asserted
// below, at which point the case stops being evidence of anything. Do not
// "simplify" this back.
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

} // namespace

// ---------------------------------------------------------------------------
// 1. Naive pitch tracking, measured ON THE OUTPUT (D-16 / CORE-01).
//
//    NOT the TEST-02 tracking gate — see the file banner. TEST-02 is Phase 31's
//    exit criterion and needs coarse/fine/FM summing that does not exist yet.
//
//    Why this case is non-vacuous WITHOUT needing a separate control: it reads
//    the returned samples rather than the telemetry the same call just wrote,
//    and the five pitch points have expectations a full octave apart. An
//    accumulator that ignored `freq` — or one that latched a single frequency
//    for all inputs — could satisfy at most one of the five, never all of them
//    at three sample rates.
// ---------------------------------------------------------------------------
TEST_CASE("vco core: naive pitch tracks the C4 reference on the OUTPUT within 1 percent (NOT the TEST-02 tracking gate)") {
	// The measured-safe grid. Every point was verified against exactly this
	// step() body. The grid deliberately STOPS at pitchCV = +2: at morph = 1.00
	// the pulse duty is 0.05, and once the +1 region falls under ~2 samples the
	// sampler steps over it and the estimate collapses toward half the true
	// frequency (measured -24.53 % at pitchCV = +3.5 and -46.89 % at +4.0, both
	// at 44.1 kHz). That is sampling loss in a deliberately unband-limited
	// oscillator, not a pitch defect — Phase 32 owns it.
	static const float PITCHES[]    = {-2.f, -1.f, 0.f, 1.f, 2.f};
	static const float MORPHS[]     = {0.00f, 0.25f, 0.50f, 0.75f, 1.00f};
	static const float CHARACTERS[] = {0.0f, 0.5f, 1.0f};

	for (double sr : SAMPLE_RATES) {
		const int n = (int)std::lround(sr * 0.25);   // 250 ms window

		for (float pitchCV : PITCHES) {
			for (float morph : MORPHS) {
				for (float character : CHARACTERS) {
					CAPTURE(sr);
					CAPTURE(pitchCV);
					CAPTURE(morph);
					CAPTURE(character);

					forge::VcoInputs base = coreBase();
					base.pitchCV   = pitchCV;
					base.morph     = morph;
					base.character = character;

					// Held CONSTANT across the whole block. This is a steady-tone
					// measurement, not a sweep: a swept input has no single
					// frequency to be right about.
					forge::VcoBlockDriver d(sr);
					std::vector<float> out = d.run(n, [=](int) { return base; });
					REQUIRE(out.size() == (size_t)n);

					const double expected = (double)forge::kVcoFreqC4 * std::pow(2.0, (double)pitchCV);

					int nUp = 0;
					const double measured = estimateFreqRising(out, sr, &nUp);

					// A real tone anywhere on this grid produces at least 16
					// rising crossings in 250 ms (the slowest point, pitchCV = -2,
					// is 65.4 Hz => 16.35 cycles). Requiring 8 says the block is
					// genuinely oscillating; a handful would mean the measurement
					// below is meaningless rather than merely wrong.
					REQUIRE(nUp >= 8);

					// TOLERANCE PROVENANCE. 1 % is roughly 128x the worst error
					// measured ANYWHERE in this grid (0.0078 %). The tightest
					// point is pitchCV = +2 with morph = 1.00 at 44.1 kHz, where
					// the pulse is 2.11 samples wide and the measured error is
					// +0.0002 %. The tolerance is loose ON PURPOSE: this is a
					// sanity invariant proving the oscillator plays the note it
					// was asked for, and it is NOT the TEST-02 tracking gate,
					// which belongs to Phase 31 and requires better than one cent
					// with coarse, fine and FM summed. Tightening this number
					// would not turn it into that gate; it would only make it
					// brittle across toolchains.
					const double relErr = std::fabs(measured - expected) / expected;
					CHECK(relErr < 0.01);
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------
// 2. Output magnitude stays inside a LOOSE 6.0 V bound (D-18b / T-30-01).
//
//    BOUND PROVENANCE. 6.0 V sits about 8 % above the hard ANALYTIC ceiling of
//    5.55 V. That ceiling is derived, not guessed: for character >= 0.001 the
//    sine path is f(s) = 0.32s^3 + 0.06s^2 + 0.76s - 0.03 whose derivative is
//    strictly positive, so f is monotone on [-1,1] with range [-1.05, +1.11];
//    triangle, saw, square and pulse are each bounded by 1; the morph crossfade
//    is a linear interpolation and cannot exceed the larger of its two shapes;
//    and the bleed step is a convex combination, which cannot raise a maximum.
//    Hence |morphedWave| <= 1.11 and |out| = 5 * |morphedWave| <= 5.55 V. That
//    was then confirmed numerically over 161 million (morph, character, phase)
//    points at five spread configurations. 6.0 V is a REAL bound, not a round
//    number — and it is roughly six orders of magnitude below the measured
//    unguarded runaway this case also has to catch.
//
//    THIS IS EXPLICITLY NOT A +/-5 V OUTPUT-RANGE ASSERTION. D-13 returns the
//    waveform UNCONDITIONED by decision — no DC blocker, no saturation, no
//    clamp — so a >5 V overshoot at high character is the expected behavior,
//    not a defect. Phase 34's OUT-01..03 owns output conditioning; writing a
//    +/-5 V assertion here would contradict D-13 and pin an output stage that
//    has not been designed yet.
// ---------------------------------------------------------------------------
TEST_CASE("vco core: output magnitude stays inside the 6.0 V loose bound (D-18b)") {
	const float kLooseBoundV = 6.0f;

	for (double sr : SAMPLE_RATES) {
		// --- Scenario one: the harness sweep. -------------------------------
		// Included because it is the drive every other VCO invariant uses, so a
		// regression that only the sweep can see still lands here. On its own it
		// is NOT sufficient — see scenario two.
		{
			CAPTURE(sr);
			// INFO with a stream insertion, not CAPTURE: doctest stringifies a
			// bare const char* as a POINTER, which would name the scenario as a
			// hex address and defeat the whole point of labelling it.
			INFO("scenario: sweepScenario");

			const int n = (int)std::lround(sr);   // one second
			forge::VcoBlockDriver d(sr);
			std::vector<float> out = d.run(n, forge::VcoBlockDriver::sweepScenario(n, coreBase()));
			REQUIRE(out.size() == (size_t)n);

			float maxAbs = 0.f;
			for (size_t i = 0; i < out.size(); ++i) {
				const float a = std::fabs(out[i]);
				if (a > maxAbs) maxAbs = a;
			}
			// Captured so the measured figure appears in `-s` output on a PASS,
			// not only on a failure: this suite's job is to record what the
			// oscillator actually does, and doctest does not decompose the
			// values of a successful assertion.
			CAPTURE(maxAbs);
			CHECK(maxAbs <= kLooseBoundV);
		}

		// --- Scenario two: the fixed worst case. ----------------------------
		// THE LOAD-BEARING SCENARIO. The `> 5.1f` assertion below must not be
		// softened or deleted.
		//
		// sweepScenario sets morph = t and character = 1-t, so the two are
		// ANTI-CORRELATED: the peak combination (morph = 0 WITH character = 1)
		// exists only in the sweep's first few samples, before the phase
		// accumulator has reached the peak phase of 0.2499. The consequence is
		// that the sweep's maximum is an ACCIDENT OF BLOCK LENGTH rather than a
		// property of the oscillator. Measured, with the driver's default spread
		// seed, at all three sample rates:
		//
		//     n = 1024        -> 5.000000 V   (the harness's own block size)
		//     n = 0.05 s      -> 5.000000 V
		//     n = 0.25 s      -> 5.2104 .. 5.2114 V
		//     n = 1.00 s      -> 5.4383 .. 5.4385 V   (this case's block)
		//
		// So a bound test driven ONLY by the sweep is worse than weak: at the
		// block sizes the rest of this suite uses it maxes at exactly 5.0000 V
		// and would still pass with the bound set to 5.001 V, and its margin
		// silently changes if anyone edits the block length. It can never be the
		// evidence that the D-13 overshoot exists.
		//
		// The fixed scenario has no such dependence: measured at 5.51803 V at
		// 44.1 / 48 / 96 kHz, against an analytic ceiling of 5.55 V. Asserting
		// that it EXCEEDS 5.1 V is what makes the 6.0 V bound evidence rather
		// than decoration — it proves the bound is exercised, and it fails
		// loudly if a future change quietly conditions the output here instead
		// of in Phase 34 where that work belongs.
		{
			CAPTURE(sr);
			INFO("scenario: fixed worst case - morph 0.0 / character 1.0 / pitchCV 0");

			const int n = (int)std::lround(sr);   // one second
			forge::VcoInputs base = coreBase();
			base.pitchCV   = 0.f;
			base.morph     = 0.f;
			base.character = 1.f;

			forge::VcoBlockDriver d(sr);
			std::vector<float> out = d.run(n, [=](int) { return base; });
			REQUIRE(out.size() == (size_t)n);

			float maxAbs = 0.f;
			for (size_t i = 0; i < out.size(); ++i) {
				const float a = std::fabs(out[i]);
				if (a > maxAbs) maxAbs = a;
			}
			// Captured so `-s` records the OBSERVED overshoot at every sample
			// rate, which is the audit trail plan 30-07's phase gate compares
			// the CI figures against.
			CAPTURE(maxAbs);
			CHECK(maxAbs <= kLooseBoundV);
			CHECK(maxAbs > 5.1f);
		}

		// --- Scenario three: hostile V/OCT. ---------------------------------
		// What this guards, measured rather than imagined: with the Nyquist
		// clamp removed, pitchCV = +10 drove the phase accumulator to 1,014,986
		// and the output to -8,655,011 V — and EVERY SINGLE SAMPLE of that
		// catastrophe stayed std::isfinite. Finiteness therefore cannot see a
		// runaway accumulator; this magnitude bound is the only invariant in the
		// suite that can. That is precisely why the two assertions sit side by
		// side here instead of one being deleted as redundant with the other, or
		// with the harness suite's finiteness case. Do not merge them.
		//
		// The residual, deliberately not asserted on: a NaN pitchCV. The frozen
		// forge::exp2Floor casts with (int32_t)x, which is UB for NaN, and the
		// header cannot change. PITCH-04 (Phase 31) hardens the correct surface
		// by clamping the summed pitch BEFORE the exp2.
		{
			static const float HOSTILE_PITCH[] = {10.f, 14.f};
			for (float pitchCV : HOSTILE_PITCH) {
				CAPTURE(sr);
				INFO("scenario: hostile V/OCT - morph 0.0 / character 1.0");
				CAPTURE(pitchCV);

				const int n = 4096;
				forge::VcoInputs base = coreBase();
				base.pitchCV   = pitchCV;
				base.morph     = 0.f;
				base.character = 1.f;

				forge::VcoBlockDriver d(sr);
				std::vector<float> out = d.run(n, [=](int) { return base; });
				REQUIRE(out.size() == (size_t)n);

				float maxAbs = 0.f;
				bool allFinite = true;
				for (size_t i = 0; i < out.size(); ++i) {
					if (!std::isfinite(out[i])) { allFinite = false; break; }
					const float a = std::fabs(out[i]);
					if (a > maxAbs) maxAbs = a;
				}
				CAPTURE(maxAbs);
				CHECK(maxAbs <= kLooseBoundV);
				CHECK(allFinite);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// 3. Spread-seed divergence (D-18a / D-10 / D-11).
//
//    THE MECHANISM, written here so the assertions below are readable without
//    cross-referencing three other files. Divergence in this phase comes from
//    STATIC PER-INSTANCE COMPONENT SPREAD and from nothing else: the five
//    coefficients — triAsymmetry, sawCurvature, squareDuty, pulseEdge, bleed —
//    that VcoCore::setSpreadSeed copies out of the DriftEngine into the
//    instance's OWN forge::Waveshape. There is no per-sample RNG draw and no OU
//    drift stepping anywhere in step(), so a different seed changes the
//    waveform permanently and for free. (That is also exactly why landing this
//    DSP could not move the shipped LFO's goldens.) Phase 34 owns the MOVING
//    drift engine; this case is the direct evidence for the roadmap's
//    "a different seed diverges" criterion.
// ---------------------------------------------------------------------------
TEST_CASE("vco core: spread seed divergence at character 1.0 (D-18a)") {
	const int n = 2048;

	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);

		forge::VcoInputs base = coreBase();
		base.pitchCV   = 0.f;
		base.morph     = 0.25f;
		base.character = 1.f;

		// All four seeds are spelled out at EACH construction site, the idiom
		// tests/test_vco_harness.cpp's determinism case uses. The drift pair is
		// IDENTICAL and only the spread pair differs, so anything this case
		// observes can only have come from the five-coefficient spread copy.
		// These are the researcher's measured pairs — substituting different
		// ones would leave the figures recorded below describing a variant of
		// this code rather than this code.
		forge::VcoBlockDriver a(sr, 0xC0FFEEULL, 0xBADF00DULL, 0x9E3779B9ULL, 0x7F4A7C15ULL);
		forge::VcoBlockDriver b(sr, 0xC0FFEEULL, 0xBADF00DULL, 0xDEADBEEFULL, 0xCAFEF00DULL);
		std::vector<float> oa = a.run(n, [=](int) { return base; });
		std::vector<float> ob = b.run(n, [=](int) { return base; });
		REQUIRE(oa.size() == (size_t)n);
		REQUIRE(ob.size() == (size_t)n);

		// Bit-exact float != throughout, NEVER doctest::Approx: Approx's
		// epsilon(0) still applies a relative-scaling margin and is therefore
		// not a true bit-exact comparator. This is the same reasoning the
		// harness suite's determinism case carries, and it matters in both
		// directions — an Approx-based scan would call two audibly different
		// blocks identical near the zero crossings.
		float maxAbsDiff = 0.f;
		int differing = 0;
		for (size_t i = 0; i < oa.size(); ++i) {
			if (oa[i] != ob[i]) ++differing;
			const float d = std::fabs(oa[i] - ob[i]);
			if (d > maxAbsDiff) maxAbsDiff = d;
		}
		CAPTURE(maxAbsDiff);
		CAPTURE(differing);

		// MARGINS. This exact configuration was measured at 0.2332 V with
		// 2048 / 2048 samples differing, at all three sample rates. Across five
		// different seed pairs at morph in {0.25, 0.50} with character = 1.0 the
		// max-abs difference ranged 0.1380 - 0.4804 V, so a 0.01 V threshold
		// carries at least a 13-fold margin against the LEAST divergent pair
		// measured, while sitting roughly four orders of magnitude above float
		// noise. The two assertions are not redundant: the first says the
		// difference is AUDIBLE, the second says it is PERVASIVE rather than a
		// single transient sample.
		CHECK(maxAbsDiff > 0.01f);
		CHECK(differing > (n * 9) / 10);
	}

	// --- THE CONTROL that keeps this case honest. ------------------------
	// Identical construction, identical drive, one change: character = 0.
	// Assert the two blocks are BIT-IDENTICAL.
	//
	// This is NOT a redundant check — it is the measured trap written down.
	// Every spread coefficient in the frozen forge::Waveshape is consumed only
	// behind a `character >= 0.001f` gate, so at character = 0 the divergence
	// above was measured at EXACTLY 0.000000 V with 0 of 2048 samples differing.
	// A version of this case written at character = 0 would therefore not merely
	// be weak, it would be GUARANTEED TO FAIL. Pinning that here means a future
	// reader who moves the case to a lower character value to "simplify" it
	// breaks a green test instead of quietly producing one that proves nothing —
	// and it is the same fact that forces plan 30-04's independence pair to run
	// at full character.
	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		INFO("control: character = 0 gates every spread coefficient, so the two seeds MUST be indistinguishable here");

		forge::VcoInputs base = coreBase();
		base.pitchCV   = 0.f;
		base.morph     = 0.25f;
		base.character = 0.f;

		forge::VcoBlockDriver a(sr, 0xC0FFEEULL, 0xBADF00DULL, 0x9E3779B9ULL, 0x7F4A7C15ULL);
		forge::VcoBlockDriver b(sr, 0xC0FFEEULL, 0xBADF00DULL, 0xDEADBEEFULL, 0xCAFEF00DULL);
		std::vector<float> oa = a.run(n, [=](int) { return base; });
		std::vector<float> ob = b.run(n, [=](int) { return base; });
		REQUIRE(oa.size() == ob.size());

		bool identical = true;
		for (size_t i = 0; i < oa.size(); ++i) {
			if (oa[i] != ob[i]) { identical = false; break; }
		}
		CHECK(identical);
	}
}
