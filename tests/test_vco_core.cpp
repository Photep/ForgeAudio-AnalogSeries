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
// output conditioning and the moving drift engine (Phase 34, OUT-01..03 /
// DRIFT-*), and CORE-03 per-instance independence, which plan 30-04 appends to
// this same file.
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
