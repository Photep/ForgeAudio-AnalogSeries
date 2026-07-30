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
// MEASURED WORST-CASE TRACKING ERROR — THIS PHASE'S OWN FIGURES, harvested from
// an actual run of these cases during plan 31-05. Worst ABSOLUTE cents per rate,
// with the volt at which it occurred:
//   PRIMARY tier (invariant 2, measured on the returned samples):
//     44100 Hz  0.00967639 c @ +5.5 V    48000 Hz  0.00870829 c @ +6.0 V
//     96000 Hz  0.00239614 c @ +7.0 V
//   SECONDARY tier (invariant 3, reads telemetry — the WEAKER tier):
//     44100 Hz  0.0013924  c @ +6.20392 V   48000 Hz  0.00123964 c @ +6.32617 V
//     96000 Hz  0.00123964 c @ +7.32617 V
// The fixed tolerance is 0.05 cents at every point, every rate and both tiers:
// 5.17x above the worst figure above and 20x under the one cent PITCH-01 asks
// for. THIS PHASE MEASURED ALL SIX FIGURES ITSELF and cites neither of the two
// prior-milestone research figures, which disagree with each other by two orders
// of magnitude about the frozen polynomial's error and were both later measured
// wrong (D-18).
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
// TOLERANCE PROVENANCE — MEASURED BY THIS PHASE, on this code, in the run that
// landed plan 31-05. Worst ABSOLUTE cents error over the primary tier's whole
// derived-boundary sweep, per rate, with the volt at which it occurred:
//
//     44100 Hz   0.00967639 cents   at +5.5 V   (24 grid points)
//     48000 Hz   0.00870829 cents   at +6.0 V   (23 grid points)
//     96000 Hz   0.00239614 cents   at +7.0 V   (25 grid points)
//
// Every one of those is NEGATIVE in sign — the measured frequency sits very
// slightly under the reference — and the worst of the three is 0.00967639.
//
// THE TWO MARGINS. This tolerance is 5.17x ABOVE the worst measurement anywhere
// in the sweep, so it is not brittle; and it is 20x UNDER the one-cent figure
// PITCH-01 actually requires, so passing it is a considerably stronger statement
// than the requirement asks for. The secondary tier's own measured figures are in
// invariant 3's comment and share this same constant.
//
// IT DOES NOT WIDEN. Not with samples per cycle, not with pitch, not with morph,
// not with sample rate (D-20). A tolerance that moves with the measurement is a
// gate wider than the prose it encodes, and this project has been bitten four
// times in one phase by exactly that. The apparatus limit is handled by BOUNDING
// THE SWEEP (invariant 1), which is the honest way to keep a fixed tolerance
// meaningful — not by loosening the number where the ruler gets coarse.
//
// WHY NO RESEARCH FIGURE IS CITED HERE (D-18). The two prior-milestone research
// documents disagree with EACH OTHER by two orders of magnitude about the
// frozen polynomial's error, and both were later measured wrong. Inheriting
// either would put a number in this file that nothing in this repository has
// ever observed. So the figures above are this phase's own, harvested out of
// this very sweep, and neither prior figure appears anywhere in this file.
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

// One documented EXTREME low point, run at 44.1 kHz only and deliberately NOT
// part of the regular grid: roughly two hertz, needing an eight-second window.
// It is the low-end extreme, included to show the pitch law still holds where
// the window rule below has to stretch by more than thirty times its floor.
constexpr double kExtremeLowVolts = -7.0;

// The SECONDARY tier's low end: 0.26 Hz, five octaves below anything the primary
// tier can reach. Resolving that frequency from zero crossings would need a
// window over a minute long, which is exactly the range this tier exists for.
constexpr double kSecondaryLowVolts = -10.0;

// Block length for a telemetry read. The telemetry frequency is written on every
// step(...), so a handful of samples is all this tier needs — and taking only a
// handful is part of what makes it obvious that this tier is NOT measuring the
// signal, merely reading back a field.
constexpr int kTelemetryBlockSamples = 8;

// How much clear air the grid keeps below a policy boundary. Applied to the
// binding limit in invariant 1 and to the clamp ceiling in invariant 3, because
// it is the same quantity in both places: a grid point must never land ON a
// boundary, or a later constant change turns a boundary collision into a
// mysterious cents failure instead of a loud one.
constexpr double kGridHeadroomVolts = 0.05;

// Baseline core input, built by default construction then FIELD ASSIGNMENT,
// never a brace value list: forge::VcoInputs has member initialisers, so under
// the C++11 rules that idiom is copied into src/ under, a value-list init is a
// hard error. The shape is kept here even though tests/ builds at C++17,
// because this is the idiom that travels into src/ where C++11 is binding.
//
// Deliberately NEUTRAL, and EVERY field this file's cases depend on is assigned
// explicitly — including the three FM fields and both tune fields — so that no
// grid point can silently inherit a value it did not state.
forge::VcoInputs pitchBase() {
	forge::VcoInputs in;
	in.pitchCV     = 0.f;
	in.coarse      = 0.f;
	in.fine        = 0.f;
	in.fmVolts     = 0.f;
	in.fmAtten     = 0.f;
	in.fmConnected = false;
	in.morph       = 0.f;
	in.character   = 0.f;
	in.drift       = 0.f;
	return in;
}

// Frequency estimator: rising zero crossings with linear SUB-SAMPLE
// interpolation, measured first-crossing to last-crossing. Returns Hz, reports
// the crossing count through nUp, and returns a NEGATIVE SENTINEL when fewer
// than two crossings were found.
//
// COPIED VERBATIM from tests/test_vco_core.cpp, on purpose, and it must not be
// shared out of that translation unit: both copies sit in an anonymous
// namespace, which is exactly what makes the duplication correct rather than a
// one-definition-rule hazard.
//
// Why counting crossings is structurally sound here rather than a hopeful
// heuristic: across a 401 x 101 grid of (morph, character) = 40,501 points,
// sampled at 20,000 points per cycle with this driver's default spread, the
// continuous waveform produced EXACTLY two sign changes per cycle at every
// single point — zero exceptions — hence exactly one RISING crossing per cycle.
//
// Why the sub-sample interpolation is LOAD-BEARING rather than a nicety: the
// naive `crossings / 2 / duration` form carries a 0.5 / cyclesInWindow
// quantization error, MEASURED at -2.15 % on a 250 ms window — roughly
// thirty-seven times this file's entire one-cent budget, and about seven hundred
// times the tolerance it actually asserts. Adopting it would force a tolerance
// at which the gate stops being evidence of anything. Do not "simplify" this
// back, and do not try to derive it from the crossing count alone.
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

// Block length for a steady-tone measurement: the rate times the GREATER of a
// quarter second and sixteen periods of the expected frequency.
//
// MEASURED BASIS for both halves. A quarter second is adequate everywhere this
// sweep goes; a twentieth of a second is measurably NOT — at the top of the
// range a 0.05 s window carries about a tenth of a cent, twice the tolerance
// this file asserts, and at the low end it degenerates entirely and hands back
// the estimator's negative sentinel. The sixteen-period floor is what stretches
// the block instead of degenerating: the low end of the grid is 8.18 Hz and the
// extreme point is roughly 2 Hz, where a quarter second is a fraction of one
// cycle. Sixteen periods is also what makes the crossing-count precondition
// below a real precondition rather than an arithmetic certainty.
int windowSamples(double sampleRate, double expectedHz) {
	return (int)std::lround(sampleRate * std::fmax(0.25, 16.0 / expectedHz));
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

// ---------------------------------------------------------------------------
// 2. TEST-02, THE PRIMARY TIER. THIS IS THE PHASE'S EXIT GATE (PITCH-01 /
//    D-18 / D-19 / D-20 / D-21).
//
//    1 V/oct tracking, measured ON THE RETURNED SAMPLES against a libm
//    reference computed in double, across the derived-boundary sweep at all
//    three production rates, at a FIXED tolerance twenty times tighter than the
//    one-cent requirement.
//
//    THE FIVE THINGS THAT MAKE THIS NON-VACUOUS BY CONSTRUCTION, each ASSERTED
//    rather than argued:
//      a. the measurement is taken on the samples the driver returned, not on
//         the telemetry the same call wrote (TRAP 1);
//      b. the expectation comes from libm in double, so the polynomial is never
//         compared against itself (TRAP 2);
//      c. consecutive expectations are REQUIRED to differ by more than a factor
//         of 1.4 — half a volt is half an octave, so this holds by construction,
//         and asserting it means an accumulator that latched a single frequency
//         can satisfy AT MOST ONE point in the whole grid;
//      d. the crossing count is REQUIRED to be at least eight BEFORE any
//         tolerance check runs, so a silent non-oscillation is a hard failure
//         rather than a wrong number, and the estimator's negative sentinel can
//         never reach the comparison;
//      e. the sweep's upper bound is DERIVED (invariant 1), so the gate stops
//         exactly where the decided behavior and the apparatus stop.
// ---------------------------------------------------------------------------
TEST_CASE("vco pitch TEST-02 PRIMARY TIER: v/oct tracking measured on the RETURNED SAMPLES is better than one cent across the derived-boundary sweep at all three rates") {
	for (double sr : SAMPLE_RATES) {
		// THE GRID, ASCENDING. The regular grid is the 0.5 V lattice from
		// kPrimaryLowVolts up to whichever derived ceiling binds at this rate.
		//
		// Ahead of it, at 44.1 kHz ONLY, sits one documented EXTREME point at
		// kExtremeLowVolts. It is NOT part of the regular grid: it exercises
		// roughly two hertz and an eight-second window, and it is here as the
		// low-end extreme rather than as a lattice point. Prepending it keeps
		// the list ascending, so the consecutive-expectation ratio check below
		// still applies to it — two octaves is a factor of four, comfortably
		// past the required 1.4.
		const bool includesExtremePoint = (sr == SAMPLE_RATES[0]);
		const int  steps = gridStepCount(kPrimaryLowVolts, topTestVolts(sr));

		std::vector<double> voltsGrid;
		if (includesExtremePoint) voltsGrid.push_back(kExtremeLowVolts);
		for (int k = 0; k <= steps; ++k)
			voltsGrid.push_back(kPrimaryLowVolts + kGridStepVolts * (double)k);

		double prevExpected = 0.0;

		for (size_t gi = 0; gi < voltsGrid.size(); ++gi) {
			const double volts    = voltsGrid[gi];
			const double expected = expectedFreqHz(volts);

			CAPTURE(sr);
			CAPTURE(volts);
			CAPTURE(expected);

			// (c) The grid cannot degenerate. Asserted, not left in prose.
			if (gi > 0) {
				const double expectedRatio = expected / prevExpected;
				CAPTURE(expectedRatio);
				CHECK(expectedRatio > 1.4);
			}
			prevExpected = expected;

			// FIXED INPUTS: ZERO MORPH, ZERO CHARACTER — a bare sine.
			//
			// MEASURED REASON, and it is about the apparatus rather than about
			// taste. The estimator's linear sub-sample interpolation is very
			// nearly exact through a sine's zero crossing and progressively
			// worse through the morphed shapes' KINKED crossings — measured
			// roughly a hundred times worse at mid morph. At any other morph
			// this case measures how kinked the waveform is at its crossing,
			// not whether the oscillator plays the right note.
			//
			// If anyone ever wants a morph-robustness pass, it belongs in a
			// SEPARATE case, at the SAME fixed tolerance, and it must never be
			// allowed to loosen this one.
			forge::VcoInputs base = pitchBase();
			base.pitchCV   = (float)volts;
			base.morph     = 0.f;
			base.character = 0.f;

			// Held CONSTANT across the whole block. This is a steady-tone
			// measurement, not a sweep: a swept input has no single frequency
			// to be right about, which is also why the driver's own sweep
			// helper is unusable for this gate. Rates are injected through the
			// driver's constructor, so there is one driver per rate — run()
			// overwrites sampleTime and sampleRate on every sample and a test
			// cannot inject timing through its own functor.
			const int n = windowSamples(sr, expected);
			CAPTURE(n);

			forge::VcoBlockDriver d(sr);
			std::vector<float> out = d.run(n, [=](int) { return base; });
			REQUIRE(out.size() == (size_t)n);

			int nUp = 0;
			const double measured = estimateFreqRising(out, sr, &nUp);
			CAPTURE(nUp);
			CAPTURE(measured);

			// (d) THE NON-OSCILLATION PRECONDITION, AND IT RUNS FIRST. Every
			// point on this grid is driven for at least sixteen periods, so a
			// genuinely oscillating block yields about sixteen rising
			// crossings. Requiring eight says the block is oscillating at all:
			// without it, a dead accumulator would return the estimator's -1.0
			// sentinel and be reported as a wrong frequency instead of as a
			// hard failure.
			REQUIRE(nUp >= 8);

			const double cents = centsError(measured, expected);
			CAPTURE(cents);
			CHECK(std::fabs(cents) < kTrackingToleranceCents);
		}
	}
}

// ---------------------------------------------------------------------------
// 3. TEST-02, THE SECONDARY TIER — AND IT IS THE WEAKER ONE (D-19).
//
//    WHY IT IS WEAKER, STATED PLAINLY AND NOT TO BE SOFTENED. The core's
//    telemetry frequency is the value step(...) wrote a few lines before it
//    returned — after the clamp and BEFORE the accumulator. An assertion on it
//    therefore says nothing whatsoever about whether the accumulator USED that
//    frequency: this tier would stay green on a core whose phase accumulator
//    ignored the pitch entirely and emitted silence, or DC, or a fixed tone.
//    That is the whole reason Phase 30's D-16 chose output measurement over
//    telemetry, and invariant 2 is where the actual evidence lives.
//
//    WHAT IT IS FOR, then, since it is not evidence of the same kind. It covers
//    the octaves where zero crossings CANNOT resolve a frequency at all:
//      - the very low end. At kSecondaryLowVolts the tone is about a quarter of
//        a hertz, where the primary tier's sixteen-period window rule would
//        demand a block over a minute long at every rate.
//      - the narrow band above the estimator's resolution ceiling but still
//        below the clamp's, where the oscillator is correct and TRACKING but the
//        ruler has run out. MEASURED, that band is only 0.30743 V wide at every
//        rate — 0.25743 V once kGridHeadroomVolts is taken off the top — and both
//        figures are NARROWER THAN THE 0.5 V GRID STEP, so no lattice point can
//        ever land inside it. It is therefore reached by one explicitly DERIVED
//        point per rate, and the case ASSERTS that the point lands in the band
//        rather than assuming it: a claim about coverage that the grid could not
//        actually deliver would be exactly the false-comment class this
//        repository keeps paying for.
//
//    THE OVERLAP WITH THE PRIMARY TIER IS DELIBERATE AND USEFUL. Over the shared
//    band, an output-derived measurement and a telemetry-derived measurement of
//    the same thing agreeing is itself a check on the apparatus. IF THE TWO
//    TIERS EVER DISAGREE, THE PRIMARY TIER IS THE EVIDENCE.
//
//    The clamp is never firing anywhere in this tier: every point is strictly
//    below the clamp-derived ceiling less kGridHeadroomVolts, derived here from
//    the same helper invariant 1 checks. PITCH-04's clamp-FIRES case is a
//    separate invariant and belongs to plan 31-07.
//
//    MEASURED THIS PHASE, worst ABSOLUTE cents on THIS tier, per rate, with the
//    volt at which it occurred — labelled as the SECONDARY tier so these figures
//    are never conflated with invariant 2's:
//      44100 Hz   0.0013924  cents   at +6.20392 V   (34 points)
//      48000 Hz   0.00123964 cents   at +6.32617 V   (34 points)
//      96000 Hz   0.00123964 cents   at +7.32617 V   (36 points)
//
//    THE WORST POINT AT EVERY RATE IS THE DERIVED BAND POINT, which is direct
//    evidence that the band point is doing work rather than decorating the grid:
//    it is measured at 2.28662 samples per cycle, comfortably inside the region
//    the primary tier's derived ceiling excludes. Restricted to the 0.5 V lattice
//    alone the worst figure is 0.000164011 cents at -9.5 V at all three rates —
//    an order of magnitude smaller, and it would have hidden the band entirely.
//
//    These are about an order of magnitude tighter than the primary tier's worst,
//    and that is the EXPECTED shape rather than a reason to trust this tier more.
//    It is reading an arithmetic result back out, so of course it agrees closely.
//    Reading a number back accurately is not the same fact as producing the right
//    tone, and the tightness of these figures is the clearest possible reminder
//    of why this tier is the weaker one.
// ---------------------------------------------------------------------------
TEST_CASE("vco pitch TEST-02 SECONDARY TIER (the WEAKER tier, reads telemetry): v/oct tracking against the same reference over the octaves zero crossings cannot resolve") {
	for (double sr : SAMPLE_RATES) {
		// The bound: strictly below the CLAMP-derived ceiling, less the same
		// headroom invariant 1 demands, so the clamp cannot fire at any point.
		// Derived, never typed in (D-21).
		const double clampBound = clampCeilingVolts(sr) - kGridHeadroomVolts;
		const int    steps      = gridStepCount(kSecondaryLowVolts, clampBound);

		std::vector<double> voltsGrid;
		for (int k = 0; k <= steps; ++k)
			voltsGrid.push_back(kSecondaryLowVolts + kGridStepVolts * (double)k);

		// The one derived point inside the band the lattice cannot reach: the
		// midpoint between the estimator's ceiling and the clamp bound. Both
		// ends come from the helpers, so this point moves with the constants.
		const double bandV = 0.5 * (estimatorCeilingVolts(sr) + clampBound);
		CAPTURE(sr);
		CAPTURE(clampBound);
		CAPTURE(bandV);
		REQUIRE(bandV > estimatorCeilingVolts(sr));
		REQUIRE(bandV < clampBound);
		voltsGrid.push_back(bandV);

		for (size_t gi = 0; gi < voltsGrid.size(); ++gi) {
			const double volts    = voltsGrid[gi];
			const double expected = expectedFreqHz(volts);

			// Zero morph and zero character here too, so the only difference
			// between the two tiers is WHERE the number is read from.
			forge::VcoInputs base = pitchBase();
			base.pitchCV   = (float)volts;
			base.morph     = 0.f;
			base.character = 0.f;

			forge::VcoBlockDriver d(sr);
			std::vector<float> out = d.run(kTelemetryBlockSamples, [=](int) { return base; });
			REQUIRE(out.size() == (size_t)kTelemetryBlockSamples);

			const double telHz = (double)d.core.tel.freqHz;

			CAPTURE(volts);
			CAPTURE(expected);
			CAPTURE(telHz);

			// NEGATED, exactly the way the core's own floors are written and the
			// way tests/test_vco_core.cpp guards its Nyquist pin: EVERY
			// comparison against a not-a-number is FALSE, so a plainly written
			// `telHz > 0.0` could not tell a NaN apart from a legitimate small
			// frequency. Written negated, a non-finite or non-positive telemetry
			// value lands on the FAILING branch instead of passing silently, and
			// it does so BEFORE the cents comparison — which would otherwise
			// take a logarithm of it.
			bool telemetryUsable = true;
			if (!(telHz > 0.0)) telemetryUsable = false;
			CHECK(telemetryUsable);

			// THE SAME SINGLE TOLERANCE CONSTANT THE PRIMARY TIER USES. There is
			// deliberately no second tolerance in this file: a weaker tier with a
			// looser number would be two gates pretending to be one. The
			// comparison is written negated for the same NaN reason as above.
			const double cents = centsError(telHz, expected);
			CAPTURE(cents);
			bool withinTolerance = true;
			if (!(std::fabs(cents) < kTrackingToleranceCents)) withinTolerance = false;
			CHECK(withinTolerance);
		}
	}
}

// ---------------------------------------------------------------------------
// 4. PITCH-02 / D-02: COARSE TUNE, MEASURED ACROSS ITS WHOLE DECLARED RANGE
//    AND AT NON-INTEGER VALUES.
//
//    THE CLAIM. A coarse value of n octaves shifts the MEASURED output pitch by
//    exactly n octaves, anywhere in the -5..+5 octave range src/AnalogVCO.cpp
//    declares for the knob, and it COMPOSES with the V/OCT jack rather than
//    replacing it. Measured on the returned samples against the same libm
//    reference and the same single tolerance constant invariant 2 uses.
//
//    THE TRAP THIS GRID IS WRITTEN AGAINST, and the reason the non-integer
//    points below are here BY REQUIREMENT rather than out of thoroughness.
//    PITCH-02 says the sweep is CONTINUOUS and D-02 honours that word
//    literally: no snap to whole octaves, no snap to semitones, no snap of any
//    kind. AN INTEGER-ONLY GRID CANNOT TELL A CONTINUOUS SWEEP FROM A SNAPPED
//    ONE, because every point of an integer grid lands exactly where a
//    whole-octave snap would put it. So the four non-integer values are the
//    control for the word "continuously", and the two snap hypotheses they
//    exclude are different sizes:
//      - a snap to WHOLE OCTAVES would move -2.37 to -2.0 (444 cents), -0.5 or
//        +0.5 to zero (600 cents) and 3.75 to 4.0 (300 cents). Any of the four
//        would fail by four orders of magnitude.
//      - a snap to SEMITONES is much subtler and only ONE of the four catches
//        it: -0.5, +0.5 and 3.75 octaves are all whole numbers of semitones
//        (-6, +6 and +45), so a semitone snap would not move them at all.
//        -2.37 octaves is -28.44 semitones and a semitone snap moves it 44
//        cents, which is 880x this file's tolerance. That row is the only
//        reason the semitone hypothesis is excluded, which is why it is not
//        interchangeable with the others.
//    An OPTIONAL octave/semitone snap is a possible future feature and is
//    recorded as a DEFERRED ITEM, not as something missing: D-02 decided
//    against it for this milestone, so this case pins the decided behavior and
//    would correctly go red if a snap were added without revisiting D-02.
//
//    WHY THREE COMBINED POINTS WITH OPPOSITE SIGNS. A grid that only ever moves
//    ONE term at a time is satisfied by an implementation where one term
//    OVERWRITES the other -- `pitchVolts = in.coarse` instead of
//    `pitchVolts = in.pitchCV + in.coarse` passes every coarse-only row at zero
//    V/OCT and every V/OCT row at zero coarse. The combined rows require a
//    SUMMED expectation, and opposite signs mean neither term's magnitude can
//    stand in for the sum.
//
//    POINTS ABOVE THE BINDING LIMIT ARE SKIPPED, NEVER CLIPPED. The last row
//    sums to +7.0 V deliberately: that is above the derived binding limit at
//    44.1 and 48 kHz (invariant 1's ruler, not a pitch claim) and below it at
//    96 kHz, so it is measured at one rate and skipped at two. The skip count
//    is asserted non-zero across the case so the skip path is EXERCISED rather
//    than being a mechanism this grid never reaches -- a coverage claim the grid
//    could not deliver is the same defect class as a false arithmetic comment.
//
//    MEASURED THIS PHASE, worst ABSOLUTE cents on THIS case, per rate, with the
//    row at which it occurred (harvested from an actual run during plan 31-06
//    with a temporary print that was removed before the commit):
//      44100 Hz   0.004658187  cents   at coarse +5.0    (18 rows measured, 1 skipped)
//      48000 Hz   0.0033790525 cents   at coarse -2.37   (18 rows measured, 1 skipped)
//      96000 Hz   0.00337679175 cents  at coarse -2.37   (19 rows measured, 0 skipped)
//    AT THE TWO RANGE ENDPOINTS specifically, at zero V/OCT, signed:
//      coarse -5.0   ->  +0.0000385655 c (44.1k)  +0.0000493257 c (48k)  +0.0000521421 c (96k)
//      coarse +5.0   ->  +0.004658187  c (44.1k)  -0.00186869   c (48k)  +0.000283846  c (96k)
//    Two things in that table are worth reading rather than skimming. First, the
//    -2.37 row is the WORST row at two of the three rates -- the non-integer
//    values are not only the snap control, they are also where the polynomial
//    works hardest, since an integer argument is bit-exact by construction in
//    the frozen helper's exponent-field path. Second, the low endpoint measures
//    four orders of magnitude better than the high one, because the estimator
//    has whole cycles to work with down there; at coarse -5.0 (8.18 Hz) the
//    44.1 kHz block yields 15 rising crossings, the tightest crossing count
//    anywhere in this case and still comfortably past the required eight.
// ---------------------------------------------------------------------------
TEST_CASE("vco pitch PITCH-02 COARSE tune: a coarse value of n octaves shifts the MEASURED pitch by exactly n octaves over the whole declared -5..+5 range, non-integer values included") {
	// One grid row: the V/OCT volts, the coarse octaves, and what the row is
	// FOR. The role string is CAPTUREd, so a red point names the tier it belongs
	// to instead of leaving a bare pair of numbers to be interpreted by hand.
	struct Row { float voct; float coarse; const char* role; };

	static const Row GRID[] = {
		// (a) THE DECLARED RANGE. -5 and +5 are exactly the bounds
		//     src/AnalogVCO.cpp declares for COARSE_PARAM, so these two rows are
		//     what proves the whole range is reachable rather than its middle.
		{ 0.f, -5.f,   "range endpoint, minus five octaves" },
		{ 0.f, -4.f,   "integer octave" },
		{ 0.f, -3.f,   "integer octave" },
		{ 0.f, -2.f,   "integer octave" },
		{ 0.f, -1.f,   "integer octave" },
		{ 0.f,  0.f,   "knob at its ctrl-click default" },
		{ 0.f,  1.f,   "integer octave" },
		{ 0.f,  2.f,   "integer octave" },
		{ 0.f,  3.f,   "integer octave" },
		{ 0.f,  4.f,   "integer octave" },
		{ 0.f,  5.f,   "range endpoint, plus five octaves" },

		// (b) NON-INTEGER -- the control for the word "continuously" (D-02).
		{ 0.f, -2.37f, "non-integer: the ONLY row that excludes a semitone snap (44 cents)" },
		{ 0.f, -0.5f,  "non-integer: half an octave, 600 cents from either whole octave" },
		{ 0.f,  0.5f,  "non-integer: half an octave, 600 cents from either whole octave" },
		{ 0.f,  3.75f, "non-integer: 300 cents from the nearest whole octave" },

		// (c) COMBINED, OPPOSITE SIGNS -- a summed expectation is required.
		{  2.f, -3.5f, "combined, opposite signs, sum -1.5 V" },
		{ -4.f,  2.25f, "combined, opposite signs, sum -1.75 V" },
		{  3.5f, -5.f, "combined, opposite signs, sum -1.5 V at the coarse range end" },

		// (d) DELIBERATELY ABOVE THE BINDING LIMIT at the two lower rates, so
		//     the skip path below is exercised and per-rate coverage differs
		//     VISIBLY rather than by assumption.
		{  2.f,  5.f,  "combined, same signs, sum +7.0 V: skipped at 44.1 and 48 kHz, measured at 96 kHz" },
	};
	const size_t nRows = sizeof(GRID) / sizeof(GRID[0]);

	int totalMeasured = 0;
	int totalSkipped  = 0;

	for (double sr : SAMPLE_RATES) {
		const double top = topTestVolts(sr);

		int    measuredPoints = 0;
		int    skippedPoints  = 0;
		double worstAbsCents  = 0.0;

		for (size_t gi = 0; gi < nRows; ++gi) {
			const float  voctF   = GRID[gi].voct;
			const float  coarseF = GRID[gi].coarse;
			const char*  role    = GRID[gi].role;

			// The expectation is the libm reference at the SUMMED volts, and the
			// sum is taken over the FLOAT values the POD actually carries so the
			// only residual against the core is float ADDITION rounding rather
			// than the representation error of each literal as well.
			const double summed = (double)voctF + (double)coarseF;

			CAPTURE(sr);
			CAPTURE(voctF);
			CAPTURE(coarseF);
			CAPTURE(role);
			CAPTURE(summed);
			CAPTURE(top);

			// SKIPPED, NOT CLIPPED. Above the binding limit the apparatus or
			// D-10's clamp takes over and a cents assertion would fail on
			// CORRECT behavior. Clipping the row to the limit instead would
			// quietly test a different input than the grid states.
			if (summed > top) {
				++skippedPoints;
				continue;
			}

			const double expected = expectedFreqHz(summed);
			const int    n        = windowSamples(sr, expected);

			// Zero morph and zero character, restated even though pitchBase()
			// already zeroes them: the base helper is deliberately neutral and
			// the case deliberately names what it depends on.
			forge::VcoInputs base = pitchBase();
			base.pitchCV   = voctF;
			base.coarse    = coarseF;
			base.morph     = 0.f;
			base.character = 0.f;

			forge::VcoBlockDriver d(sr);
			std::vector<float> out = d.run(n, [=](int) { return base; });
			REQUIRE(out.size() == (size_t)n);

			int nUp = 0;
			const double measured = estimateFreqRising(out, sr, &nUp);
			CAPTURE(n);
			CAPTURE(nUp);
			CAPTURE(expected);
			CAPTURE(measured);

			// The same precondition invariant 2 runs, and for the same reason:
			// it runs BEFORE the cents value is even computed, so the
			// estimator's negative sentinel can never reach the comparison.
			REQUIRE(nUp >= 8);

			const double cents = centsError(measured, expected);
			CAPTURE(cents);
			CHECK(std::fabs(cents) < kTrackingToleranceCents);

			if (std::fabs(cents) > worstAbsCents) worstAbsCents = std::fabs(cents);
			++measuredPoints;
		}

		CAPTURE(sr);
		CAPTURE(measuredPoints);
		CAPTURE(skippedPoints);
		CAPTURE(worstAbsCents);
		CHECK(measuredPoints > 0);

		totalMeasured += measuredPoints;
		totalSkipped  += skippedPoints;
	}

	// The grid reaches something at every rate, and the skip path is REACHED.
	// Without the second check the +7.0 V row could silently become measurable
	// everywhere (or the skip could stop working) and the comment above would
	// become a claim about coverage that nothing observes.
	CAPTURE(totalMeasured);
	CAPTURE(totalSkipped);
	CHECK(totalMeasured > 0);
	CHECK(totalSkipped > 0);
}

// ---------------------------------------------------------------------------
// 5. PITCH-03 / D-03 / D-00: FINE TUNE, AND THE ASSERTION THAT PINS THE
//    SEMITONE-TO-OCTAVE DIVISOR.
//
//    THE DECLARED RANGE IS ONE SEMITONE, +/-100 cents (D-03, per the D-00
//    correction that landed in .planning/REQUIREMENTS.md and
//    .planning/ROADMAP.md BEFORE this phase was planned). src/AnalogVCO.cpp
//    declares FINE_PARAM as -1..+1 with a x100 display multiplier, so the knob
//    reads in cents while the POD carries SEMITONES (D-05). Any +/-2 semitone
//    wording anywhere is stale and predates that correction.
//
//    THE LOAD-BEARING ASSERTION IS THE HUNDRED-CENT ONE, and the specific
//    number is what makes it load-bearing. The core owns the semitone-to-octave
//    division (D-05): `in.fine * (1.f / 12.f)`. At the range end, fine = 1:
//      - the CORRECT divisor of twelve gives 100 cents;
//      - NO conversion at all -- treating the semitone field as octaves --
//        gives 1200 cents;
//      - a divisor of one hundred, i.e. reading the field as the knob's
//        DISPLAYED cents, gives 12 cents.
//    A hundred-cent shift is therefore the one measurement that separates the
//    correct implementation from BOTH plausible wrong ones, and neither wrong
//    answer is anywhere near the tolerance. A tracking check alone would not do
//    this: it compares against an expectation this test computes with the same
//    /12 it is trying to pin, so the two would agree on any consistent divisor.
//    The hundred-cent check is a RELATIVE measurement between two runs of the
//    core, so it names the number the requirement names.
//
//    NON-INTEGER FINE VALUES ARE HERE FOR INVARIANT 4'S REASON. D-03 says
//    linear in cents with no snap, so quarter- and half-semitone rows are the
//    control for that: a snap to whole semitones would move +/-0.25 and
//    +/-0.5 by 25 and 50 cents.
//
//    TWO V/OCT VALUES, SO THE TERM IS PROVEN TO COMPOSE. Every row runs at
//    V/OCT 0.0 and again at V/OCT +2.0. A fine term that only worked at concert
//    pitch -- or an implementation that overwrote the V/OCT value with the fine
//    value -- passes a single-pitch grid and fails here.
//
//    MEASURED THIS PHASE, worst ABSOLUTE cents on THIS case, per rate, with the
//    row at which it occurred:
//      44100 Hz   0.00628057135 cents  at V/OCT +2.0, fine -0.25
//      48000 Hz   0.006281158   cents  at V/OCT +2.0, fine -0.25
//      96000 Hz   0.00633285261 cents  at V/OCT +2.0, fine -0.25
//
//    AND THE HUNDRED-CENT SHIFTS THEMSELVES, as measured -- the numbers this
//    case exists for:
//      44100 Hz   V/OCT  0.0  ->  up +100.003243  down -100.002971
//      44100 Hz   V/OCT +2.0  ->  up  +99.9940002 down  -99.9938641
//      48000 Hz   V/OCT  0.0  ->  up +100.003244  down -100.002972
//      48000 Hz   V/OCT +2.0  ->  up  +99.9941645 down  -99.9937555
//      96000 Hz   V/OCT  0.0  ->  up +100.003237  down -100.002974
//      96000 Hz   V/OCT +2.0  ->  up  +99.9940819 down  -99.9937695
//    Every one of the twelve is inside 0.007 cents of a hundred, i.e. inside a
//    seven-thousandth of the shift being measured, at BOTH V/OCT values. Part of
//    that residual is the core rather than the estimator and is accounted for
//    rather than absorbed: (1.f / 12.f) as a float is 0.08333333582, so the
//    core's own shift is 100.0000029 cents by construction. The rest is the
//    estimator, and it changes SIGN with the V/OCT value -- slightly over a
//    hundred at concert pitch, slightly under two octaves up -- which is the
//    signature of apparatus error rather than of a wrong divisor. Nothing here
//    is anywhere near 12 or 1200.
// ---------------------------------------------------------------------------
TEST_CASE("vco pitch PITCH-03 FINE tune: +/-1 semitone shifts the MEASURED pitch by exactly +/-100 cents, which pins the core's semitone-to-octave divisor") {
	// SEMITONES, per the POD's documented units (D-05). The two range ends are
	// the rows the hundred-cent assertion below reads.
	static const float FINE_VALUES[] = { -1.f, -0.5f, -0.25f, 0.f, 0.25f, 0.5f, 1.f };
	static const float VOCT_VALUES[] = { 0.f, 2.f };

	const size_t nFine = sizeof(FINE_VALUES) / sizeof(FINE_VALUES[0]);
	const size_t nVoct = sizeof(VOCT_VALUES) / sizeof(VOCT_VALUES[0]);

	for (double sr : SAMPLE_RATES) {
		double worstAbsCents = 0.0;

		for (size_t vi = 0; vi < nVoct; ++vi) {
			const float voctF = VOCT_VALUES[vi];

			// The three measurements the divisor-pinning check reads. Seeded
			// NEGATIVE so an unwritten slot cannot be mistaken for a frequency:
			// the REQUIREs below reject it before any logarithm is taken.
			double mAtMinusOne = -1.0;
			double mAtZero     = -1.0;
			double mAtPlusOne  = -1.0;

			for (size_t fi = 0; fi < nFine; ++fi) {
				const float fineF = FINE_VALUES[fi];

				// The reference at the V/OCT volts plus the fine value divided
				// by twelve, computed in DOUBLE here. The core divides in float,
				// which is a 0.0000029-cent difference at the range end -- named
				// in the comment above rather than absorbed.
				const double summed   = (double)voctF + (double)fineF / 12.0;
				const double expected = expectedFreqHz(summed);
				const int    n        = windowSamples(sr, expected);

				forge::VcoInputs base = pitchBase();
				base.pitchCV   = voctF;
				base.fine      = fineF;
				base.morph     = 0.f;
				base.character = 0.f;

				forge::VcoBlockDriver d(sr);
				std::vector<float> out = d.run(n, [=](int) { return base; });
				REQUIRE(out.size() == (size_t)n);

				int nUp = 0;
				const double measured = estimateFreqRising(out, sr, &nUp);

				CAPTURE(sr);
				CAPTURE(voctF);
				CAPTURE(fineF);
				CAPTURE(summed);
				CAPTURE(expected);
				CAPTURE(n);
				CAPTURE(nUp);
				CAPTURE(measured);

				REQUIRE(nUp >= 8);

				const double cents = centsError(measured, expected);
				CAPTURE(cents);
				CHECK(std::fabs(cents) < kTrackingToleranceCents);

				if (std::fabs(cents) > worstAbsCents) worstAbsCents = std::fabs(cents);

				if (fineF == -1.f) mAtMinusOne = measured;
				if (fineF ==  0.f) mAtZero     = measured;
				if (fineF ==  1.f) mAtPlusOne  = measured;
			}

			// THE ASSERTION THAT PINS THE DIVISOR (see the comment above). A
			// RELATIVE measurement between two runs of the core, so it does not
			// inherit the /12 the expectation above uses.
			REQUIRE(mAtZero     > 0.0);
			REQUIRE(mAtPlusOne  > 0.0);
			REQUIRE(mAtMinusOne > 0.0);

			const double upCents   = centsError(mAtPlusOne,  mAtZero);
			const double downCents = centsError(mAtMinusOne, mAtZero);

			CAPTURE(sr);
			CAPTURE(voctF);
			CAPTURE(mAtMinusOne);
			CAPTURE(mAtZero);
			CAPTURE(mAtPlusOne);
			CAPTURE(upCents);
			CAPTURE(downCents);

			// 100, not 1200 (no conversion) and not 12 (a divisor of a hundred).
			CHECK(std::fabs(upCents - 100.0) < kTrackingToleranceCents);
			CHECK(std::fabs(downCents + 100.0) < kTrackingToleranceCents);
		}

		CAPTURE(sr);
		CAPTURE(worstAbsCents);
	}
}
