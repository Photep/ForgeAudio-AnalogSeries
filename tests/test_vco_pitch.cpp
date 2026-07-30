// tests/test_vco_pitch.cpp
//
// The PITCH LAW suite over the live forge::VcoCore, driven headless through the
// VCO block-driver harness. Where tests/test_vco_harness.cpp proves the harness
// PLUMBING and tests/test_vco_core.cpp proves the OSCILLATOR (it plays a note,
// it stays inside a voltage bound, two differently-seeded instances sound
// different), this file proves that the note it plays lands where 1 V/oct says
// it must — and, once plan 31-06 lands, that coarse tune, fine tune and
// exponential FM move it by exactly the stated amount.
//
// THIS FILE HOLDS TEST-02, PHASE 31'S EXIT GATE. Invariants 2 and 3 below are
// the criterion the phase is judged on. Every structural choice in them is a
// response to how a gate like this actually fails — not by going red, but by
// going GREEN FOR THE WRONG REASON.
//
// ORGANISING PRINCIPLE, inherited from tests/test_vco_core.cpp and hardened:
// each case is written against a SPECIFIC MEASURED trap that would otherwise
// have made it vacuous. Phase 29 measured its entire local gate returning
// exit 0 on a commit that could not link. Phase 30 measured a hostile-input RED
// case that was ALREADY GREEN before the fix it was supposed to justify. A test
// that cannot fail is not evidence. The four traps this file is written
// against, by name:
//
//   TRAP 1 — THE TELEMETRY SHORTCUT. An assertion on the core's telemetry
//   frequency only re-reads the number step(...) computed a few lines before it
//   returned, so it stays green through a dead phase accumulator that never
//   used that frequency at all. The primary tier therefore measures the
//   RETURNED SAMPLES, the signal a user would actually hear. The telemetry tier
//   exists only for the octaves where zero crossings cannot resolve a
//   frequency, and it is recorded as the WEAKER tier rather than as equivalent
//   evidence (D-19, honouring Phase 30's D-16).
//
//   TRAP 2 — SELF-COMPARISON. Computing the expectation from the frozen
//   polynomial the pitch chain calls would prove only that the polynomial
//   equals itself: such a test stays green even if that function is replaced by
//   a constant. That is the vacuous-coverage trap D-18 names, and it is the
//   trap Phases 29 and 30 were each bitten by. The ground truth here is libm's
//   base-2 exponential, evaluated in DOUBLE. libm's exponentials are BANNED
//   under src/ — bit-identity of the shipped LFO's goldens and of the VCO's FM
//   path depends on the frozen approximation being the ONLY exponential in the
//   audio chain — and they are REQUIRED here for precisely that reason. The
//   asymmetry is the point, not an inconsistency: independence is what makes
//   the comparison mean anything at all.
//
//   TRAP 3 — THE ESTIMATOR'S OWN RESOLUTION FLOOR. The crossing estimator has a
//   MEASURED failure region near two samples per cycle, and it does not fail
//   loudly there: it returns a plausible wrong answer, tens of cents off, on a
//   PERFECTLY CORRECT oscillator. That reads exactly like a pitch bug. Hence
//   kEstimatorMinSamplesPerCycle below, and hence invariant 1.
//
//   TRAP 4 — THE NYQUIST CLAMP. D-10's hard clamp INTENTIONALLY breaks 1 V/oct
//   tracking above its ceiling; that is decided behavior, not a defect. A sweep
//   that ran past the ceiling would fail on correct behavior. Both limits are
//   derived from the forge:: constants at runtime and THE LESSER ONE BINDS
//   (D-21). No Hz ceiling and no volt ceiling is typed into code anywhere in
//   this file.
//
// NO ASSERTION IN THIS FILE MAY BE WRITTEN ABOUT ALIAS CONTENT, HARMONIC
// STRUCTURE OR SPECTRAL CLEANLINESS. The Phase 30/31 oscillator is naive and
// unband-limited and it ALIASES ON PURPOSE; Phase 32 (CORE-02 / AA-01..05) owns
// band-limiting and owns the alias floor. This prohibition holds even when the
// numbers measured here start looking respectable.
//
// THE LABELS IN THIS FILE MUST NOT BE SOFTENED. "Weaker tier" means weaker.
// "Self-check on the apparatus" is not a claim about the oscillator. Both
// phrases exist because a reader who sees the measured figures below could
// reasonably but WRONGLY promote the secondary tier to equivalent evidence, or
// read invariant 1 as part of the gate rather than as a check on the ruler.
//
// Invariants (numbered; 4 through 9 are RESERVED so plans 31-06 and 31-07 can
// append without renumbering anything here):
//   1. the DERIVED-BOUNDARY SELF-CHECK: both per-rate ceilings are computed
//      from the forge:: constants, the lesser one binds, and the 0.5 V grid
//      keeps real headroom below it (D-20 / D-21)                  [plan 31-05]
//   2. TEST-02, PRIMARY TIER: 1 V/oct tracking measured ON THE RETURNED
//      SAMPLES against the libm reference, across the derived-boundary sweep at
//      all three rates, at a FIXED tolerance                       [plan 31-05]
//   3. TEST-02, SECONDARY TIER: the same reference against the core's telemetry
//      frequency, over the octaves crossings cannot resolve — the WEAKER tier
//                                                                  [plan 31-05]
//   4. PITCH-02 COARSE tune                                        [plan 31-06]
//   5. PITCH-03 FINE tune                                          [plan 31-06]
//   6. FM-01/02/03 exponential FM: the volt-domain summation identity, the
//      attenuverter's bipolarity, the connected gate, audio-rate modulation
//                                                                  [plan 31-06]
//   7. the PERMANENT NEGATIVE CONTROL for invariant 6: a deliberately
//      multiplicative stand-in core required to FAIL the same identity
//                                                                  [plan 31-06]
//   8. PITCH-04 / D-10: the Nyquist clamp FIRING on a legitimate high note
//      while the oscillator keeps sounding                         [plan 31-07]
//   9. D-14: the standing hostile-pitch case, pinned AT the bound   [plan 31-07]
//
// MEASURED WORST-CASE TRACKING ERROR — THIS PHASE'S OWN FIGURES:
//   <filled in by tasks 2 and 3 of plan 31-05, from an actual run>
//
// This file is registered in tests/check_includes.sh's VCO_SIDE_ALLOW array by
// plan 31-01, with its own rationale paragraph there. Why it needs an entry:
// section [1/7] of that guard scans every non-exempt file under src/, tests/ and
// tools/ for a transitive VCO include, and this is a VCO-side test translation
// unit whose entire purpose is to drive forge::VcoCore. The entry is an
// EXACT-PATH string comparison, so it removes exactly this one file from the
// LFO-side scan set and weakens no detector. Without it `make guards` exits 1 on
// the file's very first run — the landmine Phase 30 hit.
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "VcoBlockDriver.hpp"

#include <vector>
#include <functional>   // std::function — the driver's run() takes its per-sample input functor as one
#include <cmath>        // std::exp2 / std::log2 / std::fabs / std::fmin / std::isfinite / std::lround
#include <cstdint>      // the fixed-width seed types the driver's constructor takes
#include <limits>       // reserved for 31-07's hostile-pitch grid

namespace {

// ---------------------------------------------------------------------------
// Everything in this file's helper block lives in ONE anonymous namespace, and
// that is load-bearing rather than tidy. tests/test_vco_core.cpp defines
// SAMPLE_RATES and estimateFreqRising too, and both files link into the SAME
// binary: at external linkage that would be a one-definition-rule violation.
// The anonymous namespace is exactly what makes COPYING the estimator into this
// file correct instead of hazardous. Same family of landmine as the POD naming
// rule (forge::VcoInputs, never forge::Inputs) — it costs nothing to respect
// and it is expensive to discover.
// ---------------------------------------------------------------------------

// The three production sample rates every invariant here is parametrized over,
// as DOUBLES. Every derivation below is done in double and only narrowed to
// float at the POD boundary, where the core's own fields are float.
constexpr double SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0};

// THE ONE TOLERANCE IN THIS FILE, shared by both observation tiers. FIXED at
// every point and every rate.
//
// TOLERANCE PROVENANCE: written by task 2 of plan 31-05 from figures measured in
// this phase. D-20 forbids a tolerance that widens with samples per cycle,
// pitch, morph or sample rate: a moving tolerance is a gate wider than the prose
// it encodes, and this project has been bitten by exactly that.
constexpr double kTrackingToleranceCents = 0.05;

// The crossing estimator's resolution cutoff, in samples per cycle (TRAP 3).
//
// WHY 2.5. The estimator was MEASURED GOOD at 2.634 samples per cycle and
// MEASURED BROKEN at 2.027 — and "broken" there means tens of cents of error on
// an oscillator that is perfectly correct, not a loud failure. A cutoff of 2.5
// sits between the two measured points, so it excludes every measured-broken
// point while keeping every measured-good one. This phase does NOT inherit the
// error figures that came with those two samples-per-cycle observations: it
// RE-MEASURES at its own cutoff, and invariant 2 records what it actually saw.
constexpr double kEstimatorMinSamplesPerCycle = 2.5;

// The pitch grid. Half a volt is half an octave, which is what makes the
// consecutive-expectation ratio check in invariant 2 hold by construction.
constexpr double kGridStepVolts = 0.5;

// The primary tier's low end: 8.18 Hz, comfortably inside the audio range the
// requirement speaks about, and slow enough that the window rule below has to
// stretch the block rather than take its floor.
constexpr double kPrimaryLowVolts = -5.0;

// How much clear air the grid keeps below a policy boundary. Applied to the
// binding limit in invariant 1 and to the clamp ceiling in invariant 3, because
// it is the same quantity in both places: a grid point must never land ON a
// boundary, or a later constant change turns a boundary collision into a
// mysterious cents failure instead of a loud one.
constexpr double kGridHeadroomVolts = 0.05;

// THE GROUND TRUTH, AND THE ENTIRE REASON THIS FILE IS NOT SELF-REFERENTIAL
// (D-18 / TRAP 2). libm's base-2 exponential, in double, against the DECIMAL C4
// reference.
//
// WHY THE DECIMAL AND NOT THE HEADER'S FLOAT CONSTANT, DELIBERATELY. As a float
// that constant is exactly 261.6256103515625, a fixed +0.0000685-cent offset
// from the decimal it is written as. Including that offset in the measurement is
// part of what makes this reference INDEPENDENT rather than half-derived from
// the code under test; src/dsp/VcoCore.hpp says so at kVcoFreqC4 itself, and
// warns that "correcting" the gate to read the constant would silently remove
// the independence to recover 0.0000685 cents.
//
// CONTRAST WITH THE DERIVED CEILINGS BELOW, and note that the two rules do not
// conflict. The ground truth must be INDEPENDENT of the implementation. The
// POLICY BOUNDARIES must be SYMBOLIC — read from the forge:: constants — so
// that moving a constant moves the gate with it (D-21). One is about evidence,
// the other about coupling.
double expectedFreqHz(double volts) {
	return 261.6256 * std::exp2(volts);
}

// Cents from a measured frequency to the reference it is checked against: 1200
// times the base-2 logarithm of the ratio. One cent of frequency error is
// 0.0578 %, which is why nothing in this file is expressed as a percentage.
double centsError(double measured, double expected) {
	return 1200.0 * std::log2(measured / expected);
}

// The volt at which D-10's HARD CLAMP starts pinning the frequency, and
// therefore the volt above which 1 V/oct tracking is INTENTIONALLY broken
// (TRAP 4). Derived symbolically from the two forge:: constants, in double, so
// that moving the Nyquist policy moves this gate's reach automatically instead
// of leaving a typed-in number silently disagreeing with the code (D-21).
double clampCeilingVolts(double sampleRate) {
	return std::log2((double)forge::kVcoNyquistGuardFrac * sampleRate
	                 / (double)forge::kVcoFreqC4);
}

// The volt above which the APPARATUS stops being able to measure, as opposed to
// the volt above which the OSCILLATOR stops tracking. Same symbolic C4
// reference; the limit itself is the samples-per-cycle cutoff above.
double estimatorCeilingVolts(double sampleRate) {
	return std::log2((sampleRate / kEstimatorMinSamplesPerCycle)
	                 / (double)forge::kVcoFreqC4);
}

// The binding limit: the LESSER of the two ceilings.
//
// WHY THE MINIMUM IS REQUIRED EVEN THOUGH THE ESTIMATOR BINDS AT ALL THREE
// RATES TODAY. The clamp is the REQUIREMENT-LEVEL boundary — it is the volt
// above which the product deliberately stops tracking — so it must be STATED
// rather than left implicit, and a future sample rate or a different guard
// fraction can flip which of the two binds.
//
// THE CONCRETE TRAP THIS AVOIDS, MEASURED: at 48 kHz a clamp-only bound admits a
// +6.5 V test point, and that point measures about MINUS TWELVE CENTS while the
// oscillator is perfectly correct, because the frequency there is barely two
// samples per cycle. Correct behavior, broken apparatus, and a failure that
// reads exactly like a pitch bug.
double topTestVolts(double sampleRate) {
	return std::fmin(clampCeilingVolts(sampleRate), estimatorCeilingVolts(sampleRate));
}

// How many whole kGridStepVolts steps above `low` stay at or below `top`. The
// grid is walked by integer step index rather than by accumulating a double,
// so the top of the sweep cannot drift by a fencepost. The iteration cap is a
// runaway guard only: `top` is finite by construction at every call site.
int gridStepCount(double low, double top) {
	int k = 0;
	while (k < 1024 && low + kGridStepVolts * (double)(k + 1) <= top) ++k;
	return k;
}

} // namespace

// ---------------------------------------------------------------------------
// 1. THE DERIVED-BOUNDARY SELF-CHECK (D-20 / D-21 / TRAP 3 / TRAP 4).
//
//    THIS IS A CLAIM ABOUT THE APPARATUS, NOT ABOUT THE OSCILLATOR, and the
//    label must not be softened: nothing here measures pitch. It proves that
//    the limits invariants 2 and 3 sweep to are COMPUTED from the forge::
//    constants rather than typed in, that the lesser of the two is the one that
//    binds, and that the 0.5 V grid keeps real headroom below it.
//
//    WHAT IT WOULD CATCH. A future change to the Nyquist guard fraction, or a
//    new sample rate, that pushed a grid point onto or past a limit. Without
//    this case that shows up later as a mysterious cents failure in the gate
//    itself; with it, it fires here, in a case whose name says the problem is
//    the ruler rather than the thing being measured.
//
//    OBSERVED THIS PHASE, printed out of this very case during plan 31-05 and
//    recorded at the precision it was printed at:
//      44100 Hz -> clamp +6.38263 V, estimator +6.0752 V, ESTIMATOR binds,
//                  top grid point +6.0 V (22 steps), headroom 0.075203 V
//      48000 Hz -> clamp +6.50489 V, estimator +6.19746 V, ESTIMATOR binds,
//                  top grid point +6.0 V (22 steps), headroom 0.197459 V
//      96000 Hz -> clamp +7.50489 V, estimator +7.19746 V, ESTIMATOR binds,
//                  top grid point +7.0 V (24 steps), headroom 0.197459 V
//    The estimator binds at every rate and the clamp binds at NONE — which is
//    exactly why the minimum, and this case, are both still required. Note how
//    little clear air 44.1 kHz has: 0.075 V, one and a half times the headroom
//    this case demands. That is the rate to look at first if a constant moves.
// ---------------------------------------------------------------------------
TEST_CASE("vco pitch apparatus self-check: the sweep's per-rate limits are DERIVED from the forge constants and the grid keeps headroom below the binding one") {
	for (double sr : SAMPLE_RATES) {
		const double clampV  = clampCeilingVolts(sr);
		const double estV    = estimatorCeilingVolts(sr);
		const double binding = topTestVolts(sr);

		const int    steps   = gridStepCount(kPrimaryLowVolts, binding);
		const double topGrid = kPrimaryLowVolts + kGridStepVolts * (double)steps;

		// Which limit binds, captured so the mapping is VISIBLE in a failure
		// rather than having to be re-derived by hand from the numbers.
		const bool estimatorBinds = (estV < clampV);

		CAPTURE(sr);
		CAPTURE(clampV);
		CAPTURE(estV);
		CAPTURE(binding);
		CAPTURE(steps);
		CAPTURE(topGrid);
		CAPTURE(estimatorBinds);

		// Both ceilings are real numbers and both are above the sweep's low end,
		// so there is a sweep to run at all.
		REQUIRE(std::isfinite(clampV));
		REQUIRE(std::isfinite(estV));
		CHECK(clampV > kPrimaryLowVolts);
		CHECK(estV > kPrimaryLowVolts);

		// The minimum is ACTUALLY being taken: the binding limit is at or below
		// each ceiling and is equal to one of them. Stated this way rather than
		// by recomputing the minimum, so the assertion does not simply echo the
		// helper's implementation back at itself. The disjunction is hoisted into
		// a named bool because doctest's expression decomposition rejects `||`
		// inside an assertion macro ("Expression Too Complex").
		const bool bindingIsOneOfTheTwo = (binding == clampV) || (binding == estV);
		CHECK(binding <= clampV);
		CHECK(binding <= estV);
		CHECK(bindingIsOneOfTheTwo);

		// The grid reaches something, and its top point keeps clear air below
		// the binding limit instead of landing on the boundary.
		CHECK(steps > 0);
		CHECK(binding - topGrid >= kGridHeadroomVolts);

		// THE INDEPENDENT RESTATEMENT, FROM THE OTHER DIRECTION. Above, the
		// clamp volt was derived by taking a logarithm of the constants. Here
		// the ground-truth frequency is evaluated AT that volt and compared
		// against the guard fraction times the rate — the same boundary,
		// reached by exponentiating instead of by taking a logarithm. A sign
		// error or a transposed division in the derivation cannot survive both.
		//
		// The residual is the known FIXED offset between the decimal reference
		// the ground truth uses and the float constant the policy is written in
		// (+0.0000685 cents; see expectedFreqHz above). Two percent of the
		// gate's own tolerance is the bar: the apparatus's restatement error has
		// to be a small fraction of the number the gate actually spends.
		const double guardHz        = (double)forge::kVcoNyquistGuardFrac * sr;
		const double roundTripCents = centsError(expectedFreqHz(clampV), guardHz);
		CAPTURE(guardHz);
		CAPTURE(roundTripCents);
		CHECK(std::fabs(roundTripCents) < kTrackingToleranceCents * 0.02);
	}
}
