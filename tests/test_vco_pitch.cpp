// tests/test_vco_pitch.cpp
//
// The PITCH LAW suite over the live forge::VcoCore, driven headless through the
// VCO block-driver harness. Where tests/test_vco_harness.cpp proves the harness
// PLUMBING and tests/test_vco_core.cpp proves the OSCILLATOR (it plays a note,
// it stays inside a voltage bound, two differently-seeded instances sound
// different), this file proves that the note it plays lands where 1 V/oct says
// it must, and that coarse tune, fine tune and exponential FM move it by exactly
// the stated amount.
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
// that cannot fail is not evidence. The five traps this file is written
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
//   TRAP 5 — AN IDENTITY THE WRONG IMPLEMENTATION ALSO SATISFIES, and it is the
//   sharpest one here because the natural test is entirely vacuous rather than
//   merely weak. FM-03's summation identity — a static FM voltage equals the same
//   volts on V/OCT — is satisfied BIT-EXACTLY by the MULTIPLICATIVE shape the
//   shipped LFO uses, on every input where EITHER pitch term is a WHOLE number of
//   volts. Zero is a whole number, so holding V/OCT at its default and sweeping
//   the FM jack distinguishes nothing at all. MEASURED, with the rule derived
//   from the frozen exponential's exponent-field path and then confirmed
//   numerically: see invariant 7's per-point table, where four rows sit at
//   exactly zero mismatches and four diverge from the first or second sample.
//   Hence invariant 6's grid pairs FRACTIONAL V/OCT values with fractional FM
//   products, hence it keeps the blind rows and marks them, and hence invariant 7
//   exists at all.
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
// Invariants (numbered; nine were landed as of plan 31-07, and plan 33-09
// appended the tenth exactly as that plan's note asked — as invariant 10, with
// NOTHING below it renumbered. A later plan that adds one appends as 11):
//   1. the DERIVED-BOUNDARY SELF-CHECK: both per-rate ceilings are computed
//      from the forge:: constants, the lesser one binds, and the 0.5 V grid
//      keeps real headroom below it (D-20 / D-21)                  [plan 31-05]
//   2. TEST-02, PRIMARY TIER: 1 V/oct tracking measured ON THE RETURNED
//      SAMPLES against the libm reference, across the derived-boundary sweep at
//      all three rates, at a FIXED tolerance                       [plan 31-05]
//   3. TEST-02, SECONDARY TIER: the same reference against the core's telemetry
//      frequency, over the octaves crossings cannot resolve — the WEAKER tier
//                                                                  [plan 31-05]
//   4. PITCH-02 COARSE tune: n octaves shifts the measured pitch by exactly n
//      octaves over the whole declared range, non-integer values included
//                                                                  [plan 31-06]
//   5. PITCH-03 FINE tune: +/-1 semitone is exactly +/-100 cents, which pins
//      the semitone-to-octave divisor                              [plan 31-06]
//   6. FM-01/02/03 exponential FM: the volt-domain summation identity, the
//      attenuverter's bipolarity, the connected gate, audio-rate modulation
//                                                                  [plan 31-06]
//   7. the PERMANENT NEGATIVE CONTROL for invariant 6: a deliberately
//      multiplicative stand-in core required to FAIL the same identity
//                                                                  [plan 31-06]
//   8. PITCH-04 / D-10: the Nyquist clamp FIRING on a legitimate high note —
//      the telemetry frequency pinned EXACTLY at the derived ceiling while the
//      oscillator keeps sounding, plus a below-ceiling control proving the clamp
//      does not fire early                                         [plan 31-07]
//   9. D-14 / D-22: the STANDING hostile-pitch case, pinned at BOTH ends of the
//      pitch-volt bound and explicitly NOT the red for it; the arithmetic proof
//      that the bound cannot fire on any reachable patch; and PITCH-05's two
//      non-regression pins, one of them at COMPILE TIME     [plan 31-07]
//  10. PITCH-04 / D-12: the clamp RE-CONFIRMED with SYNC as the third input
//      class, on invariant 9's own pitch/FM population, closing Phase 31
//      deferred item 11 — with the sync detector OBSERVED FIRING behind every
//      claim and the pitch claim confined to where the estimator can see
//                                                           [plan 33-09]
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
#include <limits>       // std::numeric_limits<float>::quiet_NaN() / ::infinity() — invariant 9's hostile grid
#include <type_traits>  // the standard same-type comparison behind invariant 9's compile-time accumulator pin (PITCH-05)

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

// ===========================================================================
// FM SUPPORT (plan 31-06). Everything below this line belongs to invariants 6
// and 7. Nothing above it is redefined, wrapped or shadowed here.
// ===========================================================================

// A LOOSE magnitude bound on the returned waveform, in volts.
//
// PROVENANCE. The core returns its waveform UNCONDITIONED by decision (D-13):
// no DC blocker, no saturation, no hard five-volt clamp. The analytic ceiling
// of that unconditioned shape is about 5.55 V, and Phase 30 MEASURED 5.51803 V
// as the worst excursion at high character at all three rates. This bound sits
// a little above the analytic figure so it is a real bound rather than a
// restatement of the measurement.
//
// THIS IS DELIBERATELY NOT A PLUS-OR-MINUS-FIVE-VOLT OUTPUT ASSERTION. An
// excursion above five volts at high character is EXPECTED behavior on this
// body, well inside Rack's plus-or-minus-twelve-volt norms, and Phase 34's
// OUT-01..03 is the phase that owns output conditioning. Tightening this to 5.0
// would turn decided behavior red; loosening it past the analytic ceiling would
// stop it from catching the unbounded-accumulator failure it exists for (Phase
// 30 measured -8,655,011 V with every sample still finite, so a finiteness test
// alone cannot see that class of failure).
constexpr float kPitchLooseBoundV = 6.0f;

// The four PROVEN NON-DEGENERATE seed literals, copied verbatim from
// tests/VcoBlockDriver.hpp's constructor defaults so a core seeded through the
// helper below is seeded IDENTICALLY to one the driver constructed.
//
// NEVER a pair of zeros, and never a newly invented pair. forge::Xoroshiro128Plus
// seeded with a zero pair is a fixed point that emits an all-zero stream, which
// makes the rejection loop inside std::normal_distribution never terminate --
// in Rack that is a hang while opening a patch, not a test failure.
constexpr uint64_t kDriverSeed0  = 0x1234ULL;
constexpr uint64_t kDriverSeed1  = 0x5678ULL;
constexpr uint64_t kDriverSpread0 = 0x9E3779B9ULL;
constexpr uint64_t kDriverSpread1 = 0x7F4A7C15ULL;

// One seeding callable for BOTH core types. Parameterised on the type rather
// than duplicated per type precisely so the negative control in invariant 7
// cannot drift into being seeded differently from the real core -- a control
// that starts from different RNG state is not a control for anything.
template <typename SeedableT>
void seedLikeDriver(SeedableT& c) {
	c.seed(kDriverSeed0, kDriverSeed1);
	c.setSpreadSeed(kDriverSpread0, kDriverSpread1);
}

// THE DRIVE HELPER FOR INVARIANTS 6 AND 7.
//
// WHY IT IS A TEMPLATE, AND WHY IT MUST STAY ONE. Invariant 7's deliberately
// multiplicative stand-in has to run through BYTE-IDENTICALLY the same drive
// loop as the real core, because a control that exercises different code than
// the check proves nothing about the check. That is the same argument
// tests/check_includes.sh [6/7]'s banner makes about its own negative controls
// (every one of them calls the SAME function its section calls) and the same
// argument tests/test_vco_core.cpp's interleave helper makes. Do not fork this
// into two near-copies, one per core type.
//
// TIMING IS OWNED HERE, exactly as the VCO block driver owns it. sampleTime and
// sampleRate are ALWAYS overwritten, on EVERY sample, and that overwrite is
// LOAD-BEARING: it must never become conditional on what the caller's callable
// happened to put there. Invariant 6's opening REQUIRE pins this loop
// bit-for-bit against the driver's own run() over the same seeds and inputs
// precisely so it cannot silently drift -- and that check deliberately feeds
// NONSENSE timing through the callable, so a helper that stopped overwriting
// would produce a visibly different block instead of an equal one.
//
// THE DRIVER ITSELF IS NOT TEMPLATED, SUBCLASSED, ALIASED OR EDITED. This is a
// separate function in this test translation unit. The OTHER block driver under
// tests/ -- the one that feeds the SHIPPED LFO's bit-exact golden replay leg --
// is not included by this file at all, and its filename is deliberately not
// spelled here: a boundary-anchored grep for it is one of this suite's standing
// mechanical checks, and a comment mentioning it would answer that check with a
// false positive.
template <typename CoreT>
std::vector<float> runBlockOn(CoreT& core, double sampleRate, int nSamples,
                              const std::function<forge::VcoInputs(int)>& inputAt) {
	std::vector<float> out;
	out.reserve((size_t)nSamples);
	const float dt  = (float)(1.0 / sampleRate);
	const float srf = (float)sampleRate;
	for (int i = 0; i < nSamples; ++i) {
		forge::VcoInputs in = inputAt(i);
		in.sampleTime = dt;
		in.sampleRate = srf;
		out.push_back(core.step(in));
	}
	return out;
}

// How two blocks differ, sample for sample.
//
// THE COMPARISON IS A DIRECT FLOAT INEQUALITY AND NOTHING ELSE. doctest's
// approximate comparator is forbidden everywhere in this file and specifically
// here: even its zero-epsilon form still applies a RELATIVE scaling margin, so
// it is not a bit-exact comparator. A multiplicative FM implementation is wrong
// by roughly a part in a million, which such a margin would quietly accept --
// the identity below would then pass the very implementation invariant 7 exists
// to exclude. Bit-exactness is the claim or there is no claim.
//
// firstBad is carried out so a red point names a sample index instead of only a
// count, and the counts are what invariant 7 records as its evidence.
struct BlockDiff {
	int mismatches = 0;
	int firstBad   = -1;
};

BlockDiff diffBlocks(const std::vector<float>& a, const std::vector<float>& b) {
	BlockDiff d;
	const size_t n = (a.size() < b.size()) ? a.size() : b.size();
	for (size_t i = 0; i < n; ++i) {
		if (a[i] != b[i]) {
			++d.mismatches;
			if (d.firstBad < 0) d.firstBad = (int)i;
		}
	}
	return d;
}

// THE FM-03 IDENTITY GRID, shared UNCHANGED by invariant 6 (where the real core
// must satisfy it) and invariant 7 (where the multiplicative stand-in must fail
// it). One table, two consumers, so the control is provably driven over the
// same inputs the check is.
//
// THE `blindRow` COLUMN IS THE WHOLE POINT OF THIS TABLE, AND THE RULE IT
// ENCODES WAS MEASURED HERE RATHER THAN ASSUMED. A row is BLIND when a
// multiplicative implementation satisfies the identity on it BIT-EXACTLY, so the
// row cannot tell the two implementations apart. The measured rule is:
//
//   A ROW IS BLIND IF EITHER TERM IS A WHOLE NUMBER OF VOLTS.
//
// Not "if the SUM is a whole number", which is the shape the trap first looks
// like, and not "if BOTH terms are whole". Either one suffices, and the reason is
// exact rather than statistical: the frozen exponential splits its argument into
// an exponent field and a fractional remainder, so adding a WHOLE number of
// volts changes only the exponent field and leaves the polynomial's argument
// untouched. Its value at `a + b` with `a` whole is therefore EXACTLY
// `2^a` times its value at `b`; scaling a float by an exact power of two is
// itself exact and so commutes with the rounding of the multiply; and the two
// implementations land on the same bits. Verified directly, outside doctest, on
// this toolchain: at V/OCT 0.0 the two forms agree bit-for-bit at FM values
// +0.75, +0.481 and -4.9 alike, while at V/OCT +0.25 they differ at +0.75.
//
// THE CONSEQUENCE IS THE DANGEROUS PART, AND IT IS WHY THIS COLUMN IS STATED BY
// HAND. Zero is a whole number. So the single most natural FM test anyone would
// write -- leave V/OCT unpatched at 0 V, sweep the FM voltage, assert the
// identity -- IS COMPLETELY BLIND TO THE MULTIPLICATIVE IMPLEMENTATION, at every
// FM voltage, fractional or not. Row 1 below is exactly that test, kept in the
// grid and marked blind, so the next person to widen this grid can see the shape
// of the hole rather than rediscover it. THE V/OCT TERM MUST BE FRACTIONAL for a
// row to be evidence.
struct FmIdentityPoint {
	float voct;        // V/OCT volts the connected block runs at
	float fmVolts;     // FM jack volts
	float fmAtten;     // the bipolar attenuverter setting
	bool  blindRow;    // a multiplicative core satisfies this row BIT-EXACTLY
	const char* role;
};

const FmIdentityPoint FM_IDENTITY_GRID[] = {
	{ 0.f,    0.75f,  1.0f,  true,  "BLIND: V/OCT at its 0 V default with a fractional FM voltage - the obvious test, and it proves nothing" },
	{ 0.25f,  0.75f,  1.0f,  false, "both terms fractional, integer SUM +1.0 - an integer sum is not what matters" },
	{ 1.f,    2.f,    1.0f,  true,  "BLIND: both terms whole, sum +3.0" },
	{ -1.5f,  0.5f,   0.5f,  false, "both terms fractional, sum -1.25, half attenuverter" },
	{ 2.f,   -3.f,    1.0f,  true,  "BLIND: both terms whole again, sum -1.0, negative FM volts" },
	{ 0.5f,   1.3f,   0.37f, false, "attenuverter 0.37 - the product itself is not exactly representable, sum about +0.981" },
	{ 3.25f, -1.75f,  1.0f,  false, "both terms fractional, sum +1.5 reached from a negative FM voltage" },
	{ -2.f,   0.6f,  -0.5f,  true,  "BLIND for the V/OCT term ALONE: the product -0.3 is fractional but -2.0 is whole" },
};
const size_t FM_IDENTITY_GRID_N = sizeof(FM_IDENTITY_GRID) / sizeof(FM_IDENTITY_GRID[0]);

// The FM term and the shifted V/OCT value, computed as FLOAT arithmetic in the
// SAME ASSOCIATION src/dsp/VcoCore.hpp uses: the product FIRST, then the sum
// onto the V/OCT value. Written as two named steps rather than one expression
// because the association is the whole point -- a different grouping would
// round differently and the identity below is bit-exact or it is nothing.
float fmProductOf(const FmIdentityPoint& p) { return p.fmVolts * p.fmAtten; }
float shiftedVoctOf(const FmIdentityPoint& p) { return p.voct + fmProductOf(p); }

// ---------------------------------------------------------------------------
// DeliberatelyMultiplicativeFmCore — THE PERMANENT POSITIVE CONTROL FOR
// INVARIANT 6. Read this banner before touching anything below it.
//
// THIS TYPE IS A TEST CONTROL. IT IS NOT PRODUCTION CODE, IT IS NOT A WORK IN
// PROGRESS, AND IT MUST NEVER BE MOVED UNDER src/ — least of all into
// src/dsp/VcoCore.hpp, which is the file it is a deliberately-broken copy of.
// It implements exactly the shape FM-03 and D-01 forbid, and it is the shape the
// SHIPPED Analog LFO uses: resolve a frequency from the pitch, then MULTIPLY
// that frequency by an exponentiated modulator. Landing that in the VCO would
// give linear-FM behavior at the wrong place in the chain — a fixed modulator
// voltage would shift the pitch by a different number of semitones in every
// octave — and the guard that would catch it is the very case this type exists
// to validate.
//
// WHY IT EXISTS, AND WHY THE ARGUMENT IS SHARPER HERE THAN ANYWHERE ELSE IN THIS
// SUITE. A check that has only ever been observed green is unvalidated: it is
// indistinguishable from a check that cannot fail. Invariant 6's identity is
// worse than merely unvalidated without this control, because the identity is
// SATISFIABLE BY THE WRONG IMPLEMENTATION over part of its own input space. At
// pure-integer terms the frozen polynomial produces exact powers of two by
// writing the floating-point exponent field, and multiplying a float by an exact
// power of two is itself exact — so the multiplying form and the summing form
// agree BIT FOR BIT there. An integer-only grid would have passed this type.
// This control is what turns that from an argument into a measurement, and the
// per-point mismatch table in invariant 7's banner is the measurement.
//
// DO NOT delete it, do not disable it, do not convert invariant 7 into a skipped
// or commented-out case, and do not "clean this up" because a deliberately wrong
// oscillator looks like dead code. If it goes away, or if invariant 7 ever
// passes by NOT detecting the defect, invariant 6 stops being evidence and FM-03
// reverts to an assertion nobody has tested.
//
// IT IS CONTAINED BY PLACEMENT: this file's single anonymous namespace, inside a
// test translation unit. It has internal linkage, it is in no header and in no
// shipped build graph, so check_includes.sh, check_canary.sh and the strict
// C++11 gate never see it — all three scan src/ only. Containment is ASSERTED
// rather than assumed: plan 31-06's acceptance criteria require a recursive grep
// for this type's name under src/ to find nothing.
//
// NO SHARED-STATE DEFECT LIVES HERE. The phase accumulator below is a per-
// instance MEMBER, deliberately. The function-local-static defect belongs to
// tests/test_vco_core.cpp's own stand-in, which validates a different invariant.
// Two defects in one stand-in would make it prove nothing specific: a control
// that fails an identity because it also shares state across instances does not
// tell you the identity is sensitive to the FM ORDERING.
//
// THE GUARD SEQUENCE IS MIRRORED FROM src/dsp/VcoCore.hpp AND MUST BE KEPT IN
// STEP WITH IT. The volt-domain summation of V/OCT, coarse and the divided fine
// value; the D-14 pitch-volt bound against the same constant, written with the
// same negated comparison FIRST as the NaN catcher; one exponential off the C4
// reference; the sanitised rate; the Nyquist ceiling; the negated floor as the
// LAST writer; the increment with both casts; the increment bound; the
// single-subtract wrap; the input clamps; the x5 scaling. If the real core's
// sequence changes and this one does not, this type diverges from it in TWO
// things rather than the one its banner promises, and this banner becomes
// exactly the class of false comment plan 30-08 existed to remove.
//
// ONE CONSEQUENCE OF THE DEFECT IS NOT A SECOND DEFECT, and it is stated here so
// nobody counts it as one: because the FM contribution no longer enters the
// summed volts, it no longer passes through the D-14 bound either, so a hostile
// FM voltage would reach this type's frequency unbounded. That is downstream of
// the single divergence, not additional to it. This control is driven only over
// the benign identity grid, and the hostile-input claim belongs to invariant 6's
// gate subcase over the REAL core.
// ---------------------------------------------------------------------------
struct DeliberatelyMultiplicativeFmCore {
	// Per-instance, exactly as the real core holds them. The accumulator is a
	// MEMBER — see the banner's shared-state note.
	forge::DriftEngine drift;
	forge::Waveshape wave;
	double phase = 0.0;

	void seed(uint64_t s0, uint64_t s1 = 0) { drift.seed(s0, s1); }

	// The D-11 five-coefficient copy, mirroring forge::VcoCore::setSpreadSeed
	// field for field so ONE seeding callable drives both types.
	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) {
		drift.setSpreadSeed(s0, s1);
		wave.triAsymmetrySpread = drift.triAsymmetrySpread;
		wave.sawCurvatureSpread = drift.sawCurvatureSpread;
		wave.squareDutySpread   = drift.squareDutySpread;
		wave.pulseEdgeSpread    = drift.pulseEdgeSpread;
		wave.bleedSpread        = drift.bleedSpread;
	}

	// Signature matches forge::VcoCore's step(...) exactly, so runBlockOn accepts
	// this type with NO change whatsoever to the helper.
	float step(const forge::VcoInputs& in) {
		// Mirrored: the volt-domain sum of the three NON-FM pitch terms, with the
		// semitone-to-octave division owned by the core (D-05).
		float pitchVolts = in.pitchCV + in.coarse + in.fine * (1.f / 12.f);

		// Mirrored: the D-14 bound, negated comparison FIRST as the NaN catcher.
		if (!(pitchVolts > -forge::kVcoMaxPitchVolts)) pitchVolts = -forge::kVcoMaxPitchVolts;
		if (pitchVolts > forge::kVcoMaxPitchVolts) pitchVolts = forge::kVcoMaxPitchVolts;

		// Mirrored: exactly one exponential off the C4 reference.
		float freq = forge::kVcoFreqC4 * forge::exp2_taylor5(pitchVolts);

		// >>> THE DELIBERATE DEFECT, AND THE ONLY ONE. <<<
		//
		// The real core adds the FM contribution into the SUMMED VOLTS ABOVE,
		// BEFORE the single exponential. This applies a SECOND exponential to the
		// FM contribution and MULTIPLIES the already-resolved frequency by it —
		// the shipped LFO's shape, and the whole of this type's divergence from
		// src/dsp/VcoCore.hpp.
		//
		// DO NOT "FIX" THIS. Making the FM term sum into pitchVolts above would
		// quietly turn invariant 7 green — by ceasing to detect anything — and
		// that is precisely the failure mode this warning exists for. Invariant 7
		// requires a NON-ZERO mismatch total on the fractional grid rows, so a
		// fixed control fails loudly rather than silently.
		if (in.fmConnected) freq *= forge::exp2_taylor5(in.fmVolts * in.fmAtten);

		// Mirrored: the rate is sanitised BEFORE it is scaled.
		const float safeRate = (in.sampleRate > 0.f) ? in.sampleRate : 0.f;
		const float maxFreq  = forge::kVcoNyquistGuardFrac * safeRate;

		// Mirrored: ceiling first, then the NaN-safe floor as the LAST writer.
		if (freq > maxFreq) freq = maxFreq;
		if (!(freq > 0.f)) freq = 0.f;

		// Mirrored: both casts, the negated floor, the direct increment bound and
		// the single-subtract wrap.
		double deltaPhase = (double)freq * (double)in.sampleTime;
		if (!(deltaPhase > 0.0)) deltaPhase = 0.0;
		if (deltaPhase > forge::kVcoMaxDeltaPhase) deltaPhase = forge::kVcoMaxDeltaPhase;
		phase += deltaPhase;
		if (phase >= 1.0) phase -= 1.0;

		// Mirrored: the input clamps and the unconditioned x5 scaling.
		const float p = (float)phase;
		const float morph = forge::clamp(in.morph, 0.f, 1.f);
		const float character = forge::clamp(in.character, 0.f, 1.f);
		return 5.f * wave.morphedWave(p, morph, character, 0.f);
	}
};

// ===========================================================================
// SYNC SUPPORT (plan 33-09). Everything below this line belongs to invariant
// 10. Nothing above it is redefined, wrapped or shadowed here, and no constant
// above it is widened by anything below it.
// ===========================================================================

// THE SYNC AXIS'S MASTER SIGNAL SHAPES.
//
// forge::VcoCore's sync block reads ONE VOLTAGE PER SAMPLE and hands it to a
// frozen forge::SchmittTrigger at hysteresis 0.1 / 1.0 V. Everything the core
// knows about the master is that voltage, so a "master" here is nothing more
// than a per-sample voltage function -- there is deliberately no second
// oscillator, no phase model and no rate estimate anywhere in this file.
//
// WHY EACH SHAPE IS PRESENT. The first three are legitimate cables. The last
// four exist because plan 33-09 is where PITCH-04's third input class meets the
// phase's NEW DIVISOR, `f = (1 - prevSyncVolts) / (syncVolts - prevSyncVolts)`
// at src/dsp/VcoCore.hpp:863 -- a division that appeared behind a new POD field
// this phase, which is the same "a new division appeared behind an input field"
// rationale that moved the hostile-timing grid into Phase 32 under that phase's
// D-15. That rationale is applied DELIBERATELY here rather than inherited:
// Phase 32's version of it was itself a correction of a falsified one, so it is
// re-argued for this divisor rather than copied.
enum SyncKind {
	kSyncUnpatched,      // the jack is not patched at all -- the control row
	kSyncRamp,           // a rising 0..5 V ramp at a stated rate: a normal master
	kSyncIdleHigh,       // a steady 5 V present from sample ZERO
	kSyncNanHeld,        // a not-a-number cable voltage, held for the whole block
	kSyncOnThreshold,    // a sample landing EXACTLY on the 1.0 V high threshold
	kSyncNanGlitch,      // a legitimate master with intermittent not-a-number samples
	kSyncStaleStore      // LOW, then a steady 5 V held against the store
};

struct SyncAxisRow {
	SyncKind    kind;
	double      masterHz;   // the ramp rate; 0 for the shapes that have no rate
	bool        mustFire;   // whether the detector CAN fire on this shape at all
	const char* role;
};

// The master voltage this row presents on sample `i`.
//
// The three not-a-number and threshold shapes are written so their MECHANISM is
// visible, because each one is the physical case behind a specific clause of
// the core's own guard:
//
//   kSyncIdleHigh    -- the trigger's UNINITIALIZED state sees a voltage at or
//                       above the high threshold on sample zero, goes HIGH and
//                       returns FALSE. It then never returns to LOW, so it never
//                       fires and the divisor is never evaluated. This is the
//                       steady-voltage case the core's store paragraph names,
//                       and it is the reason a 0/0 quotient is UNREACHABLE
//                       rather than merely guarded.
//   kSyncStaleStore  -- the same steady 5 V, but reached from LOW, so it DOES
//                       fire exactly once and then holds the store at 5 V
//                       against a HIGH trigger for the rest of the block.
//   kSyncNanHeld     -- every comparison against a not-a-number is false, so the
//                       trigger cannot leave UNINITIALIZED and cannot fire. The
//                       cable value never reaches the divisor.
//   kSyncNanGlitch   -- the case that DOES reach it. A not-a-number sample is
//                       STORED unconditionally into prevSyncVolts, and the next
//                       legitimate rising edge then evaluates
//                       (1 - NaN) / (volts - NaN) = NaN. The core's negated
//                       guard, `if (!(f >= 0.f) || !(f < 1.f)) f = 0.f`, is what
//                       stops that reaching the accumulator.
//   kSyncOnThreshold -- a sample of EXACTLY 1.0 V from a previous sample of
//                       exactly 0.0 V makes the quotient EXACTLY 1.0, which is
//                       the open end of the [0,1) contract. The guard's second
//                       clause is the one that fires.
float syncMasterVolts(const SyncAxisRow& s, int i, double sampleRate) {
	const float qnan = std::numeric_limits<float>::quiet_NaN();
	switch (s.kind) {
		case kSyncUnpatched:
			return 0.f;
		case kSyncRamp:
			return (float)(5.0 * std::fmod((double)i * (s.masterHz / sampleRate), 1.0));
		case kSyncIdleHigh:
			return 5.f;
		case kSyncNanHeld:
			return qnan;
		case kSyncOnThreshold:
			// EXACTLY 1.0f against EXACTLY 0.0f. Never 0.999999 or 1.000001 --
			// the whole point of the row is the equality case.
			return ((i % 64) == 0) ? 1.0f : 0.0f;
		case kSyncNanGlitch:
			// Sample zero is deliberately NOT the glitch, so the trigger reaches
			// LOW normally first and the block's first edges are legitimate.
			if (i > 0 && (i % 977) == 0) return qnan;
			return (float)(5.0 * std::fmod((double)i * (s.masterHz / sampleRate), 1.0));
		case kSyncStaleStore:
			// Eight samples at zero so the trigger reaches LOW, then a steady
			// five volts forever. It fires EXACTLY ONCE, on the transition, and
			// from then on the store holds 5 V against a HIGH trigger -- which
			// is the configuration a 0/0 quotient would need and cannot reach.
			return (i < 8) ? 0.f : 5.f;
	}
	return 0.f;
}

// What the CLAMP is required to have done on a pitch/FM row, stated per row in
// invariant 10's table rather than recomputed from the core's own summation --
// recomputing it here would be TRAP 2, a mirror of the code under test.
enum ClampExpectation {
	kClampBelow,       // the driven pitch is below the ceiling: the clamp must NOT fire
	kClampAtCeiling,   // the driven pitch is above the ceiling: the clamp must pin EXACTLY
	kClampSilent       // a sanitised not-a-number path: the frequency floors to silence
};

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

// ---------------------------------------------------------------------------
// 6. FM-01 / FM-02 / FM-03 / D-06 / D-09: EXPONENTIAL FM. THE CENTRAL
//    REQUIREMENT OF THIS PHASE, AND THE ONE WITH THE SHARPEST TRAP IN IT.
//
//    THE CLAIM. The FM contribution is summed into the VOLT domain BEFORE the
//    single exponential, so a static FM voltage at a given attenuverter setting
//    produces exactly the same audio as that same voltage added onto V/OCT with
//    the jack unpatched -- BIT for BIT, not approximately.
//
//    THE TRAP, AND IT DECIDES WHETHER THIS REQUIREMENT IS ACTUALLY TESTED. A
//    MULTIPLICATIVE IMPLEMENTATION -- the shipped LFO's shape, and the explicit
//    counter-example this phase was written against -- SATISFIES THIS IDENTITY
//    BIT-EXACTLY OVER MOST OF ITS NATURAL INPUT SPACE. The measured rule, derived
//    and then verified numerically (see FM_IDENTITY_GRID's comment and invariant
//    7's table): the identity cannot tell the two implementations apart whenever
//    EITHER term is a WHOLE NUMBER of volts. Zero is a whole number, so leaving
//    V/OCT at its default and sweeping the FM voltage -- the obvious test -- is
//    blind at every FM value. THE V/OCT TERM MUST BE FRACTIONAL for a row to be
//    evidence, which is why FM_IDENTITY_GRID pairs fractional V/OCT values with
//    fractional products, and why it KEEPS four blind rows and marks them: the
//    hole is recorded rather than merely avoided. Invariant 7 is what turns all
//    of this from an argument into a measurement.
//
//    WHY THE COMPARATOR MATTERS AS MUCH AS THE GRID. doctest's approximate
//    comparator appears NOWHERE in this file. Its zero-epsilon form still
//    applies a relative margin, and a multiplicative implementation is wrong by
//    roughly a part in a million -- comfortably inside such a margin. The
//    comparison is a direct float inequality (see diffBlocks above).
//
//    VALIDITY BEFORE ANY FM CLAIM. The case opens by REQUIRING that
//    runBlockOn() reproduces forge::VcoBlockDriver::run() bit-for-bit over the
//    same seeds and the same inputs, and it feeds NONSENSE timing through the
//    callable so that a helper which quietly stopped overwriting sampleTime and
//    sampleRate would produce a visibly different block. Without that REQUIRE, a
//    drifted helper would pass everything below it.
//
//    NO CLAIM IS MADE ANYWHERE HERE ABOUT THE SPECTRUM OF THE RESULT. The
//    oscillator is naive and unband-limited by design until Phase 32, and
//    audio-rate FM on a naive oscillator is loud in exactly the way Phase 32
//    owns. The audio-rate subcase asserts finiteness, a magnitude bound,
//    non-constancy and bit-wise difference from the unpatched block -- nothing
//    else, on purpose.
// ---------------------------------------------------------------------------
TEST_CASE("vco pitch FM-01/FM-02/FM-03 exponential FM: the FM voltage sums into the volt domain BEFORE the single exponential, bit-exactly, and the attenuverter and the connected gate both do real work") {
	// -------------------------------------------------------------------
	// VALIDITY FIRST. This REQUIRE precedes every FM claim in the case, and
	// deliberately so: it is the only thing standing between a drifted drive
	// helper and a green suite.
	// -------------------------------------------------------------------
	for (double sr : SAMPLE_RATES) {
		const int n = 256;

		// NONSENSE TIMING ON PURPOSE. Both loops overwrite sampleTime and
		// sampleRate unconditionally, so these two values must never reach the
		// core. If either loop stopped overwriting, the blocks would diverge
		// immediately and this REQUIRE is what would say so.
		const std::function<forge::VcoInputs(int)> validityInput = [](int i) {
			forge::VcoInputs in = pitchBase();
			in.pitchCV     = -1.f + 2.f * ((float)i / 255.f);
			in.morph       = 0.5f;
			in.character   = 0.25f;
			in.fmVolts     = 0.75f;
			in.fmAtten     = 0.5f;
			in.fmConnected = true;
			in.sampleTime  = 1.f;      // nonsense
			in.sampleRate  = 1.f;      // nonsense
			return in;
		};

		forge::VcoCore helperCore;
		seedLikeDriver(helperCore);
		std::vector<float> viaHelper = runBlockOn(helperCore, sr, n, validityInput);

		// The driver's constructor defaults ARE kDriverSeed0/1 and
		// kDriverSpread0/1, which is why the two cores start from identical RNG
		// state and why no seed literal was invented for this file.
		forge::VcoBlockDriver d(sr);
		std::vector<float> viaDriver = d.run(n, validityInput);

		REQUIRE(viaHelper.size() == (size_t)n);
		REQUIRE(viaDriver.size() == (size_t)n);

		const BlockDiff vd = diffBlocks(viaHelper, viaDriver);
		CAPTURE(sr);
		CAPTURE(n);
		CAPTURE(vd.mismatches);
		CAPTURE(vd.firstBad);
		REQUIRE(vd.mismatches == 0);
	}

	SUBCASE("FM-03: a static FM voltage is BIT-IDENTICAL to the same volts added onto V/OCT with the jack unpatched") {
		const int n = 512;

		for (double sr : SAMPLE_RATES) {
			for (size_t gi = 0; gi < FM_IDENTITY_GRID_N; ++gi) {
				const FmIdentityPoint& p = FM_IDENTITY_GRID[gi];

				const float  product = fmProductOf(p);
				const float  shifted = shiftedVoctOf(p);
				const char*  role    = p.role;
				const bool   blindRow = p.blindRow;

				// The jack PATCHED, at this point's voltage and attenuverter.
				forge::VcoInputs conn = pitchBase();
				conn.pitchCV     = p.voct;
				conn.fmVolts     = p.fmVolts;
				conn.fmAtten     = p.fmAtten;
				conn.fmConnected = true;
				conn.morph       = 0.f;
				conn.character   = 0.f;

				// The jack UNPATCHED, at the shifted V/OCT value.
				forge::VcoInputs unp = pitchBase();
				unp.pitchCV     = shifted;
				unp.fmConnected = false;
				unp.morph       = 0.f;
				unp.character   = 0.f;

				forge::VcoCore connCore;
				seedLikeDriver(connCore);
				std::vector<float> connBlock =
					runBlockOn(connCore, sr, n, [=](int) { return conn; });

				forge::VcoCore unpCore;
				seedLikeDriver(unpCore);
				std::vector<float> unpBlock =
					runBlockOn(unpCore, sr, n, [=](int) { return unp; });

				REQUIRE(connBlock.size() == (size_t)n);
				REQUIRE(unpBlock.size() == (size_t)n);

				const BlockDiff bd = diffBlocks(connBlock, unpBlock);

				CAPTURE(sr);
				CAPTURE(role);
				CAPTURE(blindRow);
				CAPTURE(p.voct);
				CAPTURE(p.fmVolts);
				CAPTURE(p.fmAtten);
				CAPTURE(product);
				CAPTURE(shifted);
				CAPTURE(bd.mismatches);
				CAPTURE(bd.firstBad);

				// ONE CHECK per grid point rather than one per sample: 512
				// comparisons per point would add tens of thousands of
				// assertions to a 2.6-million assertion suite for no extra
				// information, and firstBad above already names the sample.
				CHECK(bd.mismatches == 0);
			}
		}
	}

	SUBCASE("FM-02: a full NEGATIVE attenuverter inverts the shift bit-exactly AND produces a different block from the full positive one") {
		const int   n     = 512;
		const float voct  = 0.5f;
		const float fmV   = 1.5f;

		for (double sr : SAMPLE_RATES) {
			forge::VcoInputs plus = pitchBase();
			plus.pitchCV     = voct;
			plus.fmVolts     = fmV;
			plus.fmAtten     = 1.f;
			plus.fmConnected = true;

			forge::VcoInputs minus = pitchBase();
			minus.pitchCV     = voct;
			minus.fmVolts     = fmV;
			minus.fmAtten     = -1.f;
			minus.fmConnected = true;

			// The NEGATED shift on V/OCT, same float association as the core's.
			const float negShifted = voct + fmV * (-1.f);

			forge::VcoInputs unp = pitchBase();
			unp.pitchCV     = negShifted;
			unp.fmConnected = false;

			forge::VcoCore cPlus;  seedLikeDriver(cPlus);
			forge::VcoCore cMinus; seedLikeDriver(cMinus);
			forge::VcoCore cUnp;   seedLikeDriver(cUnp);

			std::vector<float> blockPlus  = runBlockOn(cPlus,  sr, n, [=](int) { return plus; });
			std::vector<float> blockMinus = runBlockOn(cMinus, sr, n, [=](int) { return minus; });
			std::vector<float> blockUnp   = runBlockOn(cUnp,   sr, n, [=](int) { return unp; });

			REQUIRE(blockPlus.size()  == (size_t)n);
			REQUIRE(blockMinus.size() == (size_t)n);
			REQUIRE(blockUnp.size()   == (size_t)n);

			const BlockDiff inversion = diffBlocks(blockMinus, blockUnp);
			const BlockDiff signDiff  = diffBlocks(blockMinus, blockPlus);

			CAPTURE(sr);
			CAPTURE(voct);
			CAPTURE(fmV);
			CAPTURE(negShifted);
			CAPTURE(inversion.mismatches);
			CAPTURE(inversion.firstBad);
			CAPTURE(signDiff.mismatches);
			CAPTURE(signDiff.firstBad);

			// BOTH halves are asserted, and the second one is not decoration.
			// The inversion claim ALONE is satisfied by an implementation that
			// ignored the attenuverter's sign entirely -- it would fail the
			// negated-shift comparison, yes, but only if the shift is where the
			// evidence sits. Requiring the two SETTINGS to differ pins the sign
			// as observable in the audio rather than only in the arithmetic.
			CHECK(inversion.mismatches == 0);
			CHECK(signDiff.mismatches > 0);
		}
	}

	SUBCASE("FM-02: an attenuverter of zero is a BIT-EXACT no-op even with the jack patched and a finite FM voltage present") {
		const int   n    = 512;
		const float voct = 0.5f;

		for (double sr : SAMPLE_RATES) {
			forge::VcoInputs zeroAtten = pitchBase();
			zeroAtten.pitchCV     = voct;
			zeroAtten.fmVolts     = 2.75f;
			zeroAtten.fmAtten     = 0.f;
			zeroAtten.fmConnected = true;

			forge::VcoInputs unp = pitchBase();
			unp.pitchCV     = voct;
			unp.fmConnected = false;

			forge::VcoCore cZero; seedLikeDriver(cZero);
			forge::VcoCore cUnp;  seedLikeDriver(cUnp);

			std::vector<float> blockZero = runBlockOn(cZero, sr, n, [=](int) { return zeroAtten; });
			std::vector<float> blockUnp  = runBlockOn(cUnp,  sr, n, [=](int) { return unp; });

			REQUIRE(blockZero.size() == (size_t)n);
			REQUIRE(blockUnp.size()  == (size_t)n);

			const BlockDiff bd = diffBlocks(blockZero, blockUnp);
			CAPTURE(sr);
			CAPTURE(voct);
			CAPTURE(bd.mismatches);
			CAPTURE(bd.firstBad);
			CHECK(bd.mismatches == 0);
		}
	}

	SUBCASE("D-09: with the jack UNPATCHED, ANY finite or non-finite FM voltage and attenuverter is a BIT-EXACT no-op") {
		const int   n    = 256;
		const float voct = 1.25f;

		const float NAN_F = std::numeric_limits<float>::quiet_NaN();
		const float INF_F = std::numeric_limits<float>::infinity();

		// Both fields carry hostile values, because Rack does NOT sanitise cable
		// voltages and the attenuverter is a param rather than a cable but is
		// forwarded raw all the same (D-17).
		const float HOSTILE_VOLTS[] = { NAN_F, INF_F, -INF_F, 1e30f, -1e30f, 3.7f, 0.f };
		const float HOSTILE_ATTEN[] = { NAN_F, INF_F, -1e30f, -1.f, 0.5f };
		const size_t nV = sizeof(HOSTILE_VOLTS) / sizeof(HOSTILE_VOLTS[0]);
		const size_t nA = sizeof(HOSTILE_ATTEN) / sizeof(HOSTILE_ATTEN[0]);

		for (double sr : SAMPLE_RATES) {
			// The reference: both FM fields at zero, jack unpatched.
			forge::VcoInputs ref = pitchBase();
			ref.pitchCV     = voct;
			ref.fmVolts     = 0.f;
			ref.fmAtten     = 0.f;
			ref.fmConnected = false;

			forge::VcoCore cRef; seedLikeDriver(cRef);
			std::vector<float> refBlock = runBlockOn(cRef, sr, n, [=](int) { return ref; });
			REQUIRE(refBlock.size() == (size_t)n);

			for (size_t vi = 0; vi < nV; ++vi) {
				for (size_t ai = 0; ai < nA; ++ai) {
					const float hv = HOSTILE_VOLTS[vi];
					const float ha = HOSTILE_ATTEN[ai];

					forge::VcoInputs hostile = pitchBase();
					hostile.pitchCV     = voct;
					hostile.fmVolts     = hv;
					hostile.fmAtten     = ha;
					hostile.fmConnected = false;

					forge::VcoCore cH; seedLikeDriver(cH);
					std::vector<float> hBlock = runBlockOn(cH, sr, n, [=](int) { return hostile; });
					REQUIRE(hBlock.size() == (size_t)n);

					const BlockDiff bd = diffBlocks(hBlock, refBlock);
					CAPTURE(sr);
					CAPTURE(voct);
					CAPTURE(hv);
					CAPTURE(ha);
					CAPTURE(bd.mismatches);
					CAPTURE(bd.firstBad);

					// WHY THIS IS THE GATE'S REAL PROOF AND NOT A FORMALITY. If
					// the `if (in.fmConnected)` guard were removed, a
					// not-a-number FM voltage would poison the summed volts; the
					// D-14 bound is written negated, so the poisoned value lands
					// on the fallback branch and becomes MINUS sixty-four volts;
					// the frequency collapses to a denormal-scale number and the
					// block changes COMPLETELY. So this subcase does not merely
					// restate that zero times anything is zero -- it fails
					// loudly the moment the gate disappears, which is exactly
					// what a boundary control has to do.
					CHECK(bd.mismatches == 0);
				}
			}
		}
	}

	SUBCASE("FM-01 / D-06: one volt of FM at a full attenuverter MEASURES exactly one octave, and a per-sample-varying FM voltage stays finite, bounded and non-constant") {
		// --- Part one: the number D-06 promises, measured on the OUTPUT. ---
		//
		// D-06 says full clockwise is 1.0 OCTAVE PER VOLT, i.e. the FM jack at
		// full depth is simply a second V/OCT input. That is a NUMBER, so it is
		// measured as one here rather than left as a comment beside a bare
		// product.
		for (double sr : SAMPLE_RATES) {
			const double baseHz = expectedFreqHz(0.0);
			const int    n      = windowSamples(sr, baseHz);

			forge::VcoInputs atZero = pitchBase();
			atZero.pitchCV     = 0.f;
			atZero.fmVolts     = 0.f;
			atZero.fmAtten     = 1.f;
			atZero.fmConnected = true;

			forge::VcoInputs atOneVolt = pitchBase();
			atOneVolt.pitchCV     = 0.f;
			atOneVolt.fmVolts     = 1.f;
			atOneVolt.fmAtten     = 1.f;
			atOneVolt.fmConnected = true;

			forge::VcoCore c0; seedLikeDriver(c0);
			forge::VcoCore c1; seedLikeDriver(c1);

			std::vector<float> block0 = runBlockOn(c0, sr, n, [=](int) { return atZero; });
			std::vector<float> block1 = runBlockOn(c1, sr, n, [=](int) { return atOneVolt; });
			REQUIRE(block0.size() == (size_t)n);
			REQUIRE(block1.size() == (size_t)n);

			int nUp0 = 0, nUp1 = 0;
			const double measured0 = estimateFreqRising(block0, sr, &nUp0);
			const double measured1 = estimateFreqRising(block1, sr, &nUp1);

			CAPTURE(sr);
			CAPTURE(n);
			CAPTURE(nUp0);
			CAPTURE(nUp1);
			CAPTURE(measured0);
			CAPTURE(measured1);

			// The same precondition every measured case in this file runs, and
			// it runs on BOTH blocks before either measurement is used.
			int nUp = (nUp0 < nUp1) ? nUp0 : nUp1;
			CAPTURE(nUp);
			REQUIRE(nUp >= 8);

			const double octaveCents = centsError(measured1, measured0);
			CAPTURE(octaveCents);
			CHECK(std::fabs(octaveCents - 1200.0) < kTrackingToleranceCents);
		}

		// --- Part two: audio-rate FM, structurally rather than as a feature. ---
		//
		// THE FM PATH IS PER-SAMPLE ARITHMETIC WITH NO RATE LIMIT ANYWHERE IN
		// THE CHAIN: the modulator is read, scaled and summed inside step(...),
		// so audio-rate operation is a STRUCTURAL property of the ordering
		// rather than a mode to switch on. Nothing here needed enabling.
		//
		// AND NOTHING HERE CLAIMS ANYTHING ABOUT THE SPECTRUM OF THE RESULT.
		// The oscillator aliases on purpose until Phase 32 owns band-limiting,
		// so the only honest assertions at audio rate are that the output is a
		// real number, that it stays inside the loose magnitude bound, that it
		// is not a constant, and that the jack being patched CHANGED it.
		for (double sr : SAMPLE_RATES) {
			const int n = 2048;

			// A two-valued square modulator at the SAMPLE RATE -- the most
			// hostile audio-rate case the chain can be handed, and four octaves
			// of swing per sample.
			const std::function<forge::VcoInputs(int)> modulated = [](int i) {
				forge::VcoInputs in = pitchBase();
				in.pitchCV     = 0.f;
				in.fmVolts     = ((i & 1) != 0) ? 2.f : -2.f;
				in.fmAtten     = 1.f;
				in.fmConnected = true;
				return in;
			};

			const std::function<forge::VcoInputs(int)> unpatched = [](int i) {
				forge::VcoInputs in = pitchBase();
				in.pitchCV     = 0.f;
				in.fmVolts     = ((i & 1) != 0) ? 2.f : -2.f;
				in.fmAtten     = 1.f;
				in.fmConnected = false;
				return in;
			};

			forge::VcoCore cMod; seedLikeDriver(cMod);
			forge::VcoCore cUnp; seedLikeDriver(cUnp);

			std::vector<float> modBlock = runBlockOn(cMod, sr, n, modulated);
			std::vector<float> unpBlock = runBlockOn(cUnp, sr, n, unpatched);
			REQUIRE(modBlock.size() == (size_t)n);
			REQUIRE(unpBlock.size() == (size_t)n);

			// ACCUMULATED, not asserted per sample: 2048 samples at three rates
			// would otherwise add six thousand assertions for four facts.
			bool   allFinite   = true;
			bool   insideBound = true;
			bool   notConstant = false;
			int    firstBad    = -1;
			double maxAbs      = 0.0;

			for (size_t i = 0; i < modBlock.size(); ++i) {
				const double v = (double)modBlock[i];
				if (!std::isfinite(v))                        { allFinite = false;   if (firstBad < 0) firstBad = (int)i; }
				if (!(std::fabs(v) <= (double)kPitchLooseBoundV)) { insideBound = false; if (firstBad < 0) firstBad = (int)i; }
				if (std::fabs(v) > maxAbs) maxAbs = std::fabs(v);
				if (modBlock[i] != modBlock[0]) notConstant = true;
			}

			const BlockDiff bd = diffBlocks(modBlock, unpBlock);

			CAPTURE(sr);
			CAPTURE(n);
			CAPTURE(maxAbs);
			CAPTURE(firstBad);
			CAPTURE(bd.mismatches);
			CAPTURE(bd.firstBad);

			CHECK(allFinite);
			CHECK(insideBound);
			CHECK(notConstant);
			CHECK(bd.mismatches > 0);
		}
	}
}

// ---------------------------------------------------------------------------
// 7. THE PERMANENT NEGATIVE CONTROL FOR INVARIANT 6 (FM-03 non-vacuity,
//    validation-contract requirement 5).
//
//    THIS CASE PASSING MEANS THE CONTROL DETECTED SOMETHING. It drives
//    DeliberatelyMultiplicativeFmCore through the SAME runBlockOn() helper and
//    over the SAME FM_IDENTITY_GRID invariant 6 uses, and REQUIRES the stand-in
//    to FAIL the summation identity on the fractional grid rows. A green
//    invariant 7 is therefore evidence that invariant 6's identity is SENSITIVE
//    to the FM ordering rather than merely satisfied by the current code.
//
//    MEASURED PER-POINT MISMATCH TABLE, harvested from an actual run during plan
//    31-06 over 512-sample blocks, with a temporary print that was removed before
//    the commit. Row order is FM_IDENTITY_GRID's; "mismatches" counts samples out
//    of 512 on which the patched block differs from the shifted-V/OCT block.
//
//      row  V/OCT   fmV    atten  product  sum      44.1kHz  48kHz  96kHz
//      ---  ------  -----  -----  -------  -------  -------  -----  -----
//       1    0.00   +0.75  +1.00   +0.750   +0.750        0      0      0   <-- BLIND
//       2   +0.25   +0.75  +1.00   +0.750   +1.000      492    491    469
//       3   +1.00   +2.00  +1.00   +2.000   +3.000        0      0      0   <-- BLIND
//       4   -1.50   +0.50  +0.50   +0.250   -1.250      328    342    297
//       5   +2.00   -3.00  +1.00   -3.000   -1.000        0      0      0   <-- BLIND
//       6   +0.50   +1.30  +0.37   +0.481   +0.981      487    483    463
//       7   +3.25   -1.75  +1.00   -1.750   +1.500      502    501    488
//       8   -2.00   +0.60  -0.50   -0.300   -2.300        0      0      0   <-- BLIND
//
//    Sighted-row totals: 1809 at 44.1 kHz, 1817 at 48 kHz, 1717 at 96 kHz, and
//    5343 across the three rates -- which is the number the REQUIRE at the bottom
//    of this case observes to be non-zero. Every sighted row diverges from the
//    FIRST OR SECOND SAMPLE (firstBad is 0 on five of them and 2 on row 4), so
//    the detection is immediate rather than something that needs a long block to
//    accumulate.
//
//    THE FOUR ZEROES ARE THE MEASURED TRAP, NOT A GAP IN THE CONTROL, and the
//    rule they follow is broader and more dangerous than "integer volt sums":
//
//      A ROW IS BLIND IF EITHER TERM IS A WHOLE NUMBER OF VOLTS.
//
//    Rows 3 and 5 have both terms whole. Row 8 has a FRACTIONAL product (-0.3)
//    and is blind anyway, purely because its V/OCT volt is -2.0. And row 1 is the
//    one that matters most: V/OCT AT ITS 0 V DEFAULT. Zero is a whole number, so
//    the most natural FM test anyone would write -- leave V/OCT alone, sweep the
//    FM voltage, assert the identity -- CANNOT DISTINGUISH THE TWO
//    IMPLEMENTATIONS AT ANY FM VOLTAGE WHATSOEVER. Note row 1 and row 2 differ
//    only in the V/OCT term, by a quarter of a volt, and that quarter volt is the
//    entire difference between a vacuous test and a decisive one.
//
//    WHY THE RULE IS EXACT RATHER THAN STATISTICAL, since a reader will want to
//    know whether the zeroes are luck. The frozen exponential splits its argument
//    into an exponent field and a fractional remainder. Adding a WHOLE number of
//    volts changes only the exponent field and leaves the polynomial's argument
//    untouched, so its value at `a + b` with `a` whole is EXACTLY `2^a` times its
//    value at `b`. Scaling a float by an exact power of two is itself exact and
//    therefore commutes with the rounding of the multiply. The two
//    implementations land on identical bits, necessarily, for every `b`.
//
//    IF ANY OF THE FOUR ZEROES EVER BECOMES NON-ZERO, THE FROZEN POLYNOMIAL
//    CHANGED. Look at tests/check_frozen.sh and the frozen-header manifest first,
//    not at this test: the exponent-field path is what makes whole-number
//    arguments exact, and it is byte-pinned. The shipped LFO's goldens would move
//    in the same commit.
//
//    THE HEALTH CHECKS DISTINGUISH THE TWO WAYS A CONTROL CAN FAIL AN IDENTITY.
//    A stand-in that produced garbage — non-finite samples, or an unbounded
//    accumulator — would also "fail" the identity, and would prove nothing about
//    the ORDERING. So every block driven here is also required to be finite and
//    inside the loose magnitude bound: this control fails the identity because it
//    multiplies, and that is asserted rather than assumed.
// ---------------------------------------------------------------------------
TEST_CASE("vco pitch FM-03 exponential FM NEGATIVE CONTROL: a deliberately MULTIPLICATIVE stand-in core FAILS the same summation identity through the same drive helper - this case passing means the control DETECTED the defect") {
	const int n = 512;

	int fractionalMismatchTotal = 0;
	int fractionalRowsSeen      = 0;

	for (double sr : SAMPLE_RATES) {
		for (size_t gi = 0; gi < FM_IDENTITY_GRID_N; ++gi) {
			const FmIdentityPoint& p = FM_IDENTITY_GRID[gi];

			const float product      = fmProductOf(p);
			const float shifted      = shiftedVoctOf(p);
			const char* role         = p.role;
			const bool  blindRow = p.blindRow;

			forge::VcoInputs conn = pitchBase();
			conn.pitchCV     = p.voct;
			conn.fmVolts     = p.fmVolts;
			conn.fmAtten     = p.fmAtten;
			conn.fmConnected = true;
			conn.morph       = 0.f;
			conn.character   = 0.f;

			forge::VcoInputs unp = pitchBase();
			unp.pitchCV     = shifted;
			unp.fmConnected = false;
			unp.morph       = 0.f;
			unp.character   = 0.f;

			// THE SAME HELPER, UNCHANGED. runBlockOn accepts this type because
			// its step(...) signature matches the real core's exactly — which is
			// the whole reason the control is evidence about invariant 6's loop
			// rather than about a loop of its own.
			DeliberatelyMultiplicativeFmCore connCore;
			seedLikeDriver(connCore);
			std::vector<float> connBlock =
				runBlockOn(connCore, sr, n, [=](int) { return conn; });

			DeliberatelyMultiplicativeFmCore unpCore;
			seedLikeDriver(unpCore);
			std::vector<float> unpBlock =
				runBlockOn(unpCore, sr, n, [=](int) { return unp; });

			REQUIRE(connBlock.size() == (size_t)n);
			REQUIRE(unpBlock.size() == (size_t)n);

			const BlockDiff bd = diffBlocks(connBlock, unpBlock);

			// Health, accumulated over both blocks: a control that failed the
			// identity by producing garbage is a different fact from one that
			// failed it by multiplying.
			bool   allFinite   = true;
			bool   insideBound = true;
			int    firstBad    = -1;
			double maxAbs      = 0.0;
			for (size_t i = 0; i < connBlock.size(); ++i) {
				const double a = (double)connBlock[i];
				const double b = (double)unpBlock[i];
				if (!std::isfinite(a)) { allFinite = false; if (firstBad < 0) firstBad = (int)i; }
				if (!std::isfinite(b)) { allFinite = false; if (firstBad < 0) firstBad = (int)i; }
				if (!(std::fabs(a) <= (double)kPitchLooseBoundV)) { insideBound = false; if (firstBad < 0) firstBad = (int)i; }
				if (!(std::fabs(b) <= (double)kPitchLooseBoundV)) { insideBound = false; if (firstBad < 0) firstBad = (int)i; }
				if (std::fabs(a) > maxAbs) maxAbs = std::fabs(a);
				if (std::fabs(b) > maxAbs) maxAbs = std::fabs(b);
			}

			CAPTURE(sr);
			CAPTURE(role);
			CAPTURE(blindRow);
			CAPTURE(p.voct);
			CAPTURE(p.fmVolts);
			CAPTURE(p.fmAtten);
			CAPTURE(product);
			CAPTURE(shifted);
			CAPTURE(bd.mismatches);
			CAPTURE(bd.firstBad);
			CAPTURE(maxAbs);
			CAPTURE(firstBad);

			CHECK(allFinite);
			CHECK(insideBound);

			if (blindRow) {
				// THE MEASURED TRAP. Zero here is the expected, recorded fact —
				// see the banner. This is a CHECK rather than a REQUIRE because a
				// change in it is a report about the frozen polynomial, not a
				// reason to abandon the rest of the case.
				CHECK(bd.mismatches == 0);
			} else {
				fractionalMismatchTotal += bd.mismatches;
				++fractionalRowsSeen;
			}
		}
	}

	// THE ASSERTION THAT MAKES THIS A CONTROL RATHER THAN A DECORATION: the
	// stand-in is OBSERVED failing the identity, on every invocation of the
	// suite, over the rows whose terms are not whole numbers.
	CAPTURE(fractionalRowsSeen);
	CAPTURE(fractionalMismatchTotal);
	REQUIRE(fractionalRowsSeen > 0);
	REQUIRE(fractionalMismatchTotal > 0);
}

// ---------------------------------------------------------------------------
// 8. PITCH-04 / D-10: THE NYQUIST CLAMP FIRES ON A LEGITIMATE HIGH NOTE, AND
//    THE OSCILLATOR IS STILL OSCILLATING WHEN IT DOES.
//
//    WHY THIS CASE EXISTS WHEN THE SUITE ALREADY PINS THE NYQUIST BOUND. It
//    does already pin it: tests/test_vco_core.cpp's scenario four recomputes the
//    ceiling symbolically and requires the telemetry frequency to stay at or
//    below it. But that pin is driven by hostile TIMING — nonsense sample rates
//    and nonsense sample times — and it asserts an INEQUALITY. Two things follow
//    that it therefore cannot see. It never observes the clamp firing on a
//    legitimate high note reached through the PITCH, which is the only way a
//    user ever reaches it. And an inequality is satisfied by a DEAD oscillator:
//    `freqHz <= ceiling` is true at zero, so a core that silenced itself above
//    the ceiling would pass it. This case closes both gaps, and closing the
//    second one is validation requirement 6.
//
//    THE TWO HALVES OF THE CLAIM, and neither is sufficient alone:
//
//      (a) THE CLAMP'S OWN ACTION IS OBSERVED, not merely its side effects.
//          Above the ceiling the telemetry frequency must equal the guard
//          fraction times the sample rate EXACTLY — recomputed here in FLOAT,
//          from the forge:: constant and the same non-positive-rate sanitising
//          rule the core applies, so an exact equality is a meaningful
//          statement rather than an approximate one. An exact equality is a far
//          stronger fact than a bound: it says the ceiling was APPLIED, at its
//          derived value, and that nothing downstream perturbed it.
//
//      (b) THE OSCILLATOR KEEPS SOUNDING. Real rising crossings, a block whose
//          maximum differs from its minimum, and a peak magnitude above one
//          volt. This is D-10's DECIDED BEHAVIOR stated as numbers: the
//          frequency PINS at the ceiling and the peaks FLATTEN OUT at the top,
//          and that flattening is the CHOSEN SOUND rather than a defect. Under
//          deep FM the clamp fires on most cycles, which is exactly why the
//          sound it makes is a timbral decision and not an edge case. The
//          rejected alternative was an AMPLITUDE FADE above the threshold: it
//          was rejected because it adds a gain stage that collides with the
//          later phase that owns output conditioning. Pitch fold-back was
//          rejected too — that is a deliberate effect, not the guard PITCH-04
//          asks for. If someone ever implements either, half (b) goes red.
//
//    NO TRACKING ASSERTION ABOVE THE CEILING, AND DO NOT ADD ONE. Above the
//    ceiling the pitch is INTENTIONALLY wrong — that is what a hard clamp
//    means — so a 1 V/oct assertion there would fail on correct behavior. The
//    estimator is run at the above-ceiling points for its CROSSING COUNT ONLY,
//    and its returned frequency is deliberately discarded. Nothing in this case
//    compares a measured frequency against the reference at any above-ceiling
//    point.
//
//    THE BELOW-CEILING CONTROL IS WHAT MAKES THIS A BOUNDARY RATHER THAN A
//    CONSTANT. Without it, every assertion in half (a) would be satisfied by a
//    core that pinned the frequency to the ceiling ALWAYS, at every pitch, which
//    would be a catastrophic bug reading as a green case. One volt below the
//    ceiling the telemetry frequency must therefore be STRICTLY LESS than the
//    recomputed maximum and must still match the libm reference inside this
//    file's one tolerance — so the clamp is proven not to fire early.
//
//    ALL FOUR DRIVEN VOLTS ARE FAR INSIDE THE D-14 PITCH-VOLT BOUND, and that
//    is asserted rather than assumed, so this case exercises the Nyquist clamp
//    and nothing else. Invariant 9 owns the pitch-volt bound.
//
//    NO Hz LITERAL AND NO VOLT LITERAL (D-21). The ceiling volt comes from the
//    same helper invariants 1, 2 and 3 use; the expected maximum frequency comes
//    from forge::kVcoNyquistGuardFrac. Move the constant and this case moves
//    with it.
//
//    MEASURED FIGURES, harvested from an actual run of this case during plan
//    31-07 with a temporary print that was removed before the commit. Blocks are
//    250 ms at morph = 0 and character = 0 above the ceiling, and the short
//    telemetry-read block below it. `offset` is volts relative to that rate's
//    derived clamp ceiling; nUp is rising crossings in the block; peak is the
//    largest absolute returned sample in volts.
//
//      rate    ceiling V     offset   telemetry Hz   vs guard*rate     nUp   peak V
//      ------  -----------  -------  -------------  ---------------  ------  ------
//      44100   +6.38263152    +0.25    21829.5      EXACT              5457   5.000
//      44100   +6.38263152    +1.00    21829.5      EXACT              5457   5.000
//      44100   +6.38263152    +3.00    21829.5      EXACT              5457   5.000
//      44100   +6.38263152    -1.00    10914.73438  strictly under       --      --
//      48000   +6.50488727    +0.25    23760.0      EXACT              5939   5.000
//      48000   +6.50488727    +1.00    23760.0      EXACT              5939   5.000
//      48000   +6.50488727    +3.00    23760.0      EXACT              5939   5.000
//      48000   +6.50488727    -1.00    11879.96484  strictly under       --      --
//      96000   +7.50488727    +0.25    47520.0      EXACT             11879   5.000
//      96000   +7.50488727    +1.00    47520.0      EXACT             11879   5.000
//      96000   +7.50488727    +3.00    47520.0      EXACT             11879   5.000
//      96000   +7.50488727    -1.00    23759.92969  strictly under       --      --
//
//    The three below-ceiling points against the libm reference, which is the
//    other half of the boundary claim: -0.00242652678 cents at 44.1 kHz
//    (reference 10914.74967 Hz), -0.00507139295 at 48 kHz (11879.99964 Hz) and
//    -0.00507139295 at 96 kHz (23759.99929 Hz) — every one inside a tenth of the
//    0.05-cent tolerance this file asserts, so the note one volt down is not
//    merely under the ceiling, it is the RIGHT note.
//
//    READ THREE THINGS OUT OF THAT TABLE. The telemetry frequency, the crossing
//    count and the peak are IDENTICAL across all three above-ceiling offsets at
//    a given rate — a quarter volt above and three volts above produce the very
//    same block, which is exactly what a HARD clamp looks like from the outside
//    and is not what a soft one would look like. The peak at the ceiling is the
//    FULL five volts (the accumulator lands on the sine's own maximum, so
//    blockMin and blockMax measure -5 and +5 exactly), so "peaks flatten out" is
//    a flattening of the WAVEFORM SHAPE and not a loss of level: nothing here is
//    quiet, let alone silent. And the crossing counts are in the thousands, so
//    the eight this case requires is not a bar the block barely clears.
// ---------------------------------------------------------------------------
TEST_CASE("vco pitch PITCH-04 / D-10: the Nyquist clamp FIRES at its derived ceiling on a legitimate high note and the oscillator KEEPS SOUNDING - and a point below the ceiling proves it does not fire early") {
	// Volts ABOVE that rate's derived clamp ceiling. A quarter volt is the
	// nearest point that still clears the frozen polynomial's own error by four
	// orders of magnitude; three volts is eight times the ceiling frequency
	// before clamping, so if the ceiling were applied loosely rather than
	// exactly, this offset is where it would show.
	static const double ABOVE_CEILING_OFFSET_VOLTS[] = {0.25, 1.0, 3.0};
	static const size_t ABOVE_CEILING_N =
		sizeof(ABOVE_CEILING_OFFSET_VOLTS) / sizeof(ABOVE_CEILING_OFFSET_VOLTS[0]);

	// THE BOUNDARY CONTROL. One volt below the ceiling is half the ceiling
	// frequency, still inside the estimator's reach and still a note the
	// requirement guarantees tracking for.
	const double belowCeilingOffsetVolts = -1.0;

	for (double sr : SAMPLE_RATES) {
		const double ceilingVolts = clampCeilingVolts(sr);

		// THE EXPECTED MAXIMUM, RECOMPUTED THE WAY THE CORE COMPUTES IT: the
		// guard fraction times the FLOAT sample rate, in FLOAT, through the same
		// non-positive-rate sanitising rule. Doing it in double and narrowing
		// afterwards would land on a different value and turn the exact equality
		// below into a coin flip. This is the symbolic-recomputation idiom
		// tests/test_vco_core.cpp's Nyquist pin uses, and it is what makes the
		// gate move automatically when the constant does (D-21).
		const float srf = (float)sr;
		const float expectedMaxFreq = forge::kVcoNyquistGuardFrac * ((srf > 0.f) ? srf : 0.f);

		CAPTURE(sr);
		CAPTURE(ceilingVolts);
		CAPTURE(expectedMaxFreq);
		REQUIRE(std::isfinite(ceilingVolts));
		REQUIRE(expectedMaxFreq > 0.f);

		// -------------------------------------------------------------------
		// ABOVE THE CEILING: the clamp must fire, exactly, and the oscillator
		// must still be oscillating.
		// -------------------------------------------------------------------
		for (size_t ai = 0; ai < ABOVE_CEILING_N; ++ai) {
			const double offsetVolts  = ABOVE_CEILING_OFFSET_VOLTS[ai];
			const double drivenVolts  = ceilingVolts + offsetVolts;

			// This case is about the NYQUIST clamp, so it must stay clear of the
			// OTHER bound in the chain. Asserted rather than assumed.
			REQUIRE(std::fabs(drivenVolts) < (double)forge::kVcoMaxPitchVolts);

			forge::VcoInputs in = pitchBase();
			in.pitchCV   = (float)drivenVolts;
			in.morph     = 0.f;
			in.character = 0.f;

			// A quarter second. At the ceiling that is thousands of cycles, so
			// the crossing-count requirement below is a statement about the
			// oscillator rather than about the window length.
			const int n = (int)std::lround(sr * 0.25);
			forge::VcoBlockDriver d(sr);
			std::vector<float> out = d.run(n, [=](int) { return in; });
			REQUIRE(out.size() == (size_t)n);

			// THE ESTIMATOR IS CALLED FOR nUp AND NOTHING ELSE HERE. Its
			// returned frequency is deliberately dropped: above the ceiling the
			// pitch is intentionally wrong and comparing it to anything would be
			// asserting against decided behavior.
			int nUp = 0;
			estimateFreqRising(out, sr, &nUp);

			bool   allFinite = true;
			int    firstBad  = -1;
			double peakAbs   = 0.0;
			double blockMin  = (double)out[0];
			double blockMax  = (double)out[0];
			for (size_t i = 0; i < out.size(); ++i) {
				const double s = (double)out[i];
				if (!std::isfinite(s)) { allFinite = false; if (firstBad < 0) firstBad = (int)i; }
				if (std::fabs(s) > peakAbs) peakAbs = std::fabs(s);
				if (s < blockMin) blockMin = s;
				if (s > blockMax) blockMax = s;
			}

			const float telFreq = d.core.tel.freqHz;

			// THE ASSERTION THIS WHOLE CASE EXISTS FOR, and the one thing the
			// existing timing-driven pin cannot say: the ceiling was applied, at
			// exactly its derived value. Written NEGATED so a not-a-number
			// telemetry frequency lands on the FAILING branch — every comparison
			// against a NaN is false, so a plainly written equality would be
			// false for a NaN and for a wrong number alike, but the negated form
			// keeps the intent visible next to the core's own negated floors.
			bool clampAppliedExactly = true;
			if (!(telFreq == expectedMaxFreq)) clampAppliedExactly = false;

			// D-10's decided behavior, as numbers. The block must not be a
			// constant, and the level must be real rather than residual.
			const bool blockIsNotConstant = (blockMax != blockMin);

			bool peakAudible = true;
			if (!(peakAbs > 1.0)) peakAudible = false;

			bool peakInsideLooseBound = true;
			if (!(peakAbs <= (double)kPitchLooseBoundV)) peakInsideLooseBound = false;

			CAPTURE(offsetVolts);
			CAPTURE(drivenVolts);
			CAPTURE(telFreq);
			CAPTURE(nUp);
			CAPTURE(peakAbs);
			CAPTURE(blockMin);
			CAPTURE(blockMax);
			CAPTURE(firstBad);
			INFO("above the derived clamp ceiling: the frequency must PIN at the ceiling exactly and the oscillator must keep sounding (D-10) - no tracking assertion belongs here");

			// The oscillation precondition comes FIRST, the same discipline the
			// tracking tiers use: a silent block makes every number below it
			// meaningless rather than merely wrong.
			REQUIRE(nUp >= 8);
			CHECK(blockIsNotConstant);
			CHECK(peakAudible);
			CHECK(allFinite);
			CHECK(peakInsideLooseBound);
			CHECK(clampAppliedExactly);
		}

		// -------------------------------------------------------------------
		// BELOW THE CEILING: the clamp must NOT fire. Without this point the
		// case above is satisfied by a core that clamps everything, always.
		// -------------------------------------------------------------------
		{
			const double drivenVolts = ceilingVolts + belowCeilingOffsetVolts;
			REQUIRE(std::fabs(drivenVolts) < (double)forge::kVcoMaxPitchVolts);

			forge::VcoInputs in = pitchBase();
			in.pitchCV   = (float)drivenVolts;
			in.morph     = 0.f;
			in.character = 0.f;

			// The telemetry frequency is written on every step(...), so the
			// short-block idiom the secondary tier uses is all this point needs.
			forge::VcoBlockDriver d(sr);
			std::vector<float> out = d.run(kTelemetryBlockSamples, [=](int) { return in; });
			REQUIRE(out.size() == (size_t)kTelemetryBlockSamples);

			const double telHz   = (double)d.core.tel.freqHz;
			const double expected = expectedFreqHz(drivenVolts);

			CAPTURE(drivenVolts);
			CAPTURE(telHz);
			CAPTURE(expected);
			INFO("one volt BELOW the derived clamp ceiling: the clamp must not fire here, which is what makes the points above pin a BOUNDARY rather than a constant");

			// Negated for the NaN reason, and checked before the logarithm.
			bool telemetryUsable = true;
			if (!(telHz > 0.0)) telemetryUsable = false;
			REQUIRE(telemetryUsable);

			// THE STRICT INEQUALITY IS THE POINT. Not `<=`: at-or-below would be
			// satisfied by a clamp that fired here too.
			bool belowCeilingStrictly = true;
			if (!(telHz < (double)expectedMaxFreq)) belowCeilingStrictly = false;
			CHECK(belowCeilingStrictly);

			// And it is still the RIGHT frequency, against the same libm
			// reference and the same single tolerance the tracking tiers use.
			const double cents = centsError(telHz, expected);
			CAPTURE(cents);
			bool withinTolerance = true;
			if (!(std::fabs(cents) < kTrackingToleranceCents)) withinTolerance = false;
			CHECK(withinTolerance);
		}
	}
}

// ---------------------------------------------------------------------------
// 9. D-14 / D-22: THE STANDING HOSTILE-PITCH CASE, PINNED AT BOTH ENDS OF THE
//    PITCH-VOLT BOUND — PLUS THE ARITHMETIC PROOF THAT THE BOUND CANNOT FIRE ON
//    ANY REACHABLE PATCH, AND PITCH-05'S TWO NON-REGRESSION PINS.
//
//    READ THIS FIRST, BECAUSE THE HONEST LABEL MATTERS MORE HERE THAN ANYWHERE
//    ELSE IN THIS FILE. THIS CASE IS THE PERMANENT REGRESSION CHECK FOR THE
//    PITCH-VOLT BOUND. IT IS **NOT** THE EVIDENCE THAT THE BOUND WAS NEEDED, AND
//    IT MUST NEVER BE CITED AS SUCH.
//
//    WHY NOT, MEASURED RATHER THAN ARGUED. A behavioral red for this bound was
//    MEASURED VACUOUS before the bound was written. With the guard absent, the
//    core ALREADY survived a quiet not-a-number, both infinities, very large
//    finite magnitudes of both signs and two-hundred-volt inputs with every
//    sample finite, a telemetry frequency of zero and a peak magnitude inside
//    five volts. Every assertion a reasonable person would have written as the
//    red — all samples finite, frequency zero, output in range — was ALREADY
//    GREEN. The reason is that the existing NEGATED frequency floor catches the
//    garbage RESULT the undefined behavior produces, so the output never shows
//    the defect at all. Landing a guard behind a case that never went red is the
//    vacuous-coverage trap this repository has twice rejected a fix over.
//
//    WHERE THE RED ACTUALLY CAME FROM. A ONE-SHOT UNDEFINED-BEHAVIOR SANITIZER
//    PROBE, run outside the working tree during plan 31-03, which named BOTH
//    frozen sites by file, line and column: the float-to-int cast at
//    src/dsp/RackCompat.hpp:106 (a not-a-number outside the representable range
//    of int) and the left shift at :109 (a shift of 2147483647 by 23 places).
//    Its verbatim transcript, and the clean re-run against it, live in that
//    plan's summary. The probe exits 0 in BOTH runs — the sanitizer recovers by
//    default — so the DIAGNOSTIC TEXT is the evidence and the exit status is
//    not.
//
//    AND WHY THAT PROBE STAYS ONE-SHOT (D-24). A PERMANENT REPOSITORY-WIDE
//    SANITIZER GATE IS FORBIDDEN. The SHIPPED module reaches the identical
//    latent problem through its own FM path, so a repo-wide gate would turn the
//    live, golden-pinned module red. Fixing that is a milestone-guardrail event
//    requiring operator sign-off and a golden re-verification, and nobody has
//    signed off. Do not add `-fsanitize=undefined` to the build or to CI on the
//    strength of this case.
//
//    WHAT THE HOSTILE GRID BELOW THEREFORE IS. A standing regression check that
//    the guarded chain keeps five properties over an input class Rack can
//    genuinely deliver, because Rack does NOT sanitise cable voltages. Per
//    configuration, accumulated over a few thousand steps rather than asserted
//    per sample: every returned sample finite; the peak magnitude inside this
//    file's loose bound; the phase accumulator inside its half-open zero-to-one
//    range; the telemetry frequency non-negative; and the telemetry frequency at
//    or below the Nyquist ceiling recomputed symbolically from the guard
//    fraction and the same sanitised-rate rule the core applies. The last two
//    are written NEGATED so a not-a-number reads as a FAILURE rather than
//    passing silently.
//
//    THE TWO PINNED POINTS ARE D-22'S STANDING CHECK: the pitch volts driven to
//    EXACTLY plus and EXACTLY minus the bound, taken from the forge:: constant
//    rather than typed in, so moving the constant moves this case with it. The
//    same two points are also driven through the FM route, because the FM term
//    is the reason the bound exists.
//
//    NO NON-CONSTANT ASSERTION APPEARS ANYWHERE IN THIS CASE, AND THE NEGATIVE
//    PINNED POINT IS WHY. At minus the bound the frequency is denormal-scale but
//    POSITIVE (measured 1.418e-17 Hz), so the negated frequency floor correctly
//    does NOT fire, the phase increment is about 3.2e-22, and the output is
//    effectively direct current. THAT IS THE STATED LOW-END DECISION RATHER THAN
//    A DEFECT (D-13): no low-end frequency floor is added anywhere, because a
//    VCO tuned absurdly low genuinely IS direct current and the user asked for
//    it. PITCH-04 speaks only to the top end, and invariant 8 is where the
//    oscillator is required to keep sounding. A DIFFERENT WARNING FROM THE SAME
//    DECISION: do not write a telemetry-frequency-equals-zero assertion for
//    hostile pitch. That value USED to be zero, before the bound existed, only
//    because the undefined behavior produced garbage the floor happened to
//    sanitise. It is now a small POSITIVE number and such an assertion would
//    fail on correct behavior.
//
//    THE HOSTILE TIMING GRIDS IN THE OTHER VCO CORE TEST FILE ARE NOT EXTENDED
//    BY THIS CASE, ON PURPOSE (D-15). Infinities, subnormals and very-large
//    finite values on the sample rate and sample time remain DEFERRED to the
//    phase whose oversampled inner loop is the first real source of exotic
//    timing — that loop decouples the rate from the sample time deliberately,
//    which is what makes exotic timing reachable there rather than merely
//    imaginable. This case covers a DIFFERENT INPUT CLASS: the pitch and FM
//    fields. Its own timing is deliberately LEGITIMATE at every point, so a
//    failure here can only be about the pitch chain.
//
//    MEASURED RESULTS, harvested from an actual run during plan 31-07 with a
//    temporary print that was removed before the commit. All 78 configurations
//    (26 grid rows at three rates) pass all five accumulated properties with
//    firstBadStep = -1 — no step of any configuration violated anything.
//
//    AND HERE IS THE HONEST SHAPE OF WHAT THE GRID DISTINGUISHES, recorded the
//    same way invariant 7's blind rows are, because a grid's ZEROES and its
//    COLLAPSES are as informative as its hits. Because the bound maps every
//    hostile input onto one of two values, the 26 rows collapse onto exactly
//    THREE observable outcomes per rate:
//
//      outcome            rows                          telemetry Hz      peak V
//      -----------------  ----------------------------  ---------------  --------
//      the CONTROL        the zero-volt row              261.6256104     5.000000 (44.1 kHz)
//                                                                       4.999999523 (48/96 kHz)
//      POSITIVE plateau   +inf, +1e30, +200, +130,       the rate's      4.998722553 (44.1 kHz)
//                         +bound, and the same five      ceiling         4.998712063 (48/96 kHz)
//                         through FM, plus a finite       EXACTLY:
//                         voltage times a POSITIVELY     21829.5 /
//                         infinite attenuverter          23760 / 47520
//      NEGATIVE plateau   a not-a-number, -inf, -1e30,   1.418275276e-17 5.000000
//                         -200, -130, -bound, the same    at ALL THREE
//                         through FM, and a finite        rates
//                         voltage times a NOT-A-NUMBER
//                         or negatively infinite
//                         attenuverter
//
//    TWO THINGS ARE WORTH READING OUT OF THAT COLLAPSE rather than skimming it.
//    A NOT-A-NUMBER lands on the NEGATIVE plateau, and that is the negated-first
//    comparison in the core made visible: a not-a-number fails
//    `pitchVolts > -bound`, so the negation is true and it becomes minus the
//    bound. If someone ever "simplified" that pair into the comparison ladder
//    helper — which is transparent to a not-a-number and passes it straight
//    through — those rows would stop landing here. And the peak magnitude at the
//    negative plateau is the LARGEST of the three, not the smallest, precisely
//    because the output there is a frozen constant at whatever the waveform's
//    value is: a stalled oscillator is not a quiet one, which is why the loose
//    magnitude bound is a real check on these rows and a finiteness test alone
//    would not be.
//
//    THE MARGIN SUBCASE MEASURED: terms 12 + 5 + 0.083333333333333329 + 12, a
//    worst reachable total of 29.083333333333332 V against a bound of 64 V, for
//    a ratio of 2.2005730659025788. The requirement is a factor of two; the
//    measured margin clears it by ten percent.
//
//    PITCH-05'S RUNTIME HALF MEASURED: over 100000 steps one volt above each
//    rate's clamp ceiling the accumulator finishes at 0.99909167672740296 (44.1
//    kHz) and 0.99946984462440014 (48 and 96 kHz), and never once left its
//    half-open range.
//
//    THE MARGIN SUBCASE IS ARITHMETIC ON PURPOSE, AND THERE IS NO BEHAVIORAL
//    ALTERNATIVE. See its own comment.
//
//    PITCH-05'S TWO PINS live here as well, because they are non-regression
//    obligations of the same phase rather than claims of their own.
// ---------------------------------------------------------------------------
TEST_CASE("vco pitch D-14 / D-22 hostile pitch and FM volts: the STANDING regression check pinned at BOTH ends of the pitch-volt bound, the arithmetic proof the bound cannot fire on a reachable patch, and PITCH-05's non-regression pins - this case is explicitly NOT the RED") {
	// -------------------------------------------------------------------
	// PITCH-05, HALF ONE: THE ACCUMULATOR'S DECLARED TYPE, PINNED AT COMPILE
	// TIME rather than by a comment or by review.
	//
	// WHAT IT PROTECTS, and it is a future phase's problem rather than this
	// one's. The band-limiting work that follows this phase places waveform
	// discontinuities at a SUB-SAMPLE position, and the fraction it needs is
	// computed from this accumulator. A float accumulator carries about
	// twenty-four bits of mantissa, so at audio rates it loses the low-order
	// part of every increment and the loss COMPOUNDS over a long block — the
	// crossing fraction degrades exactly where the correction has to be most
	// precise. A one-word type change would do that silently: nothing about the
	// pitch, the tracking or the guards would move, and no behavioral case in
	// this suite would notice. So the type is asserted at compile time, which is
	// the only place this particular regression is visible.
	// -------------------------------------------------------------------
	static_assert(std::is_same<decltype(forge::VcoCore::phase), double>::value,
	              "PITCH-05: forge::VcoCore's phase accumulator must remain a double. "
	              "A float accumulator loses low-order increments at audio rates over long "
	              "blocks, which degrades the sub-sample crossing fraction a later phase's "
	              "band-limiting depends on. Do not narrow this type.");

	// How many steps each hostile configuration is driven for. A few thousand is
	// enough for a wrapping accumulator to visit its whole range at the positive
	// plateau, and short enough that 26 rows at three rates stay cheap.
	const int kHostileSteps = 4000;

	SUBCASE("the hostile grid: pitch-routed and FM-routed, with both bound points PINNED symbolically") {
		// One row per configuration. Written as a table so the grid is readable
		// as data rather than reconstructed from nested loops, and so the role
		// string travels into the failure output with the numbers.
		struct HostilePitchPoint {
			float pitchCV;
			float fmVolts;
			float fmAtten;
			bool  fmConnected;
			const char* role;
		};

		const float qnan  = std::numeric_limits<float>::quiet_NaN();
		const float pinf  = std::numeric_limits<float>::infinity();
		const float ninf  = -std::numeric_limits<float>::infinity();
		const float huge  = 1e30f;
		// THE BOUND, READ FROM THE forge:: CONSTANT AND NEVER TYPED IN. The two
		// rows that use it are D-22's standing pinned points; a literal here
		// would silently stop pinning the bound the moment the constant moved.
		const float bound = forge::kVcoMaxPitchVolts;

		const HostilePitchPoint GRID[] = {
			// --- routed through the V/OCT pitch field, FM jack unpatched ---
			{  0.f,     0.f,  0.f,  false, "the CONTROL: a legitimate zero-volt pitch, jack unpatched" },
			{  qnan,    0.f,  0.f,  false, "pitch: a quiet not-a-number" },
			{  pinf,    0.f,  0.f,  false, "pitch: positive infinity" },
			{  ninf,    0.f,  0.f,  false, "pitch: negative infinity" },
			{  huge,    0.f,  0.f,  false, "pitch: a very large POSITIVE finite magnitude" },
			{ -huge,    0.f,  0.f,  false, "pitch: a very large NEGATIVE finite magnitude" },
			{  200.f,   0.f,  0.f,  false, "pitch: plus two hundred volts" },
			{ -200.f,   0.f,  0.f,  false, "pitch: minus two hundred volts" },
			{  130.f,   0.f,  0.f,  false, "pitch: plus one hundred and thirty volts, just outside the frozen helper's own domain" },
			{ -130.f,   0.f,  0.f,  false, "pitch: minus one hundred and thirty volts" },
			{  bound,   0.f,  0.f,  false, "PINNED (D-22): pitch driven to EXACTLY PLUS the bound, from the forge:: constant" },
			{ -bound,   0.f,  0.f,  false, "PINNED (D-22): pitch driven to EXACTLY MINUS the bound, from the forge:: constant" },
			// --- routed through the FM field instead, jack CONNECTED, attenuverter FULL ---
			{  0.f,  qnan,    1.f,  true,  "FM route: a quiet not-a-number on the FM jack at a full attenuverter" },
			{  0.f,  pinf,    1.f,  true,  "FM route: positive infinity at a full attenuverter" },
			{  0.f,  ninf,    1.f,  true,  "FM route: negative infinity at a full attenuverter" },
			{  0.f,  huge,    1.f,  true,  "FM route: a very large POSITIVE finite magnitude at a full attenuverter" },
			{  0.f, -huge,    1.f,  true,  "FM route: a very large NEGATIVE finite magnitude at a full attenuverter" },
			{  0.f,  200.f,   1.f,  true,  "FM route: plus two hundred volts at a full attenuverter" },
			{  0.f, -200.f,   1.f,  true,  "FM route: minus two hundred volts at a full attenuverter" },
			{  0.f,  130.f,   1.f,  true,  "FM route: plus one hundred and thirty volts at a full attenuverter" },
			{  0.f, -130.f,   1.f,  true,  "FM route: minus one hundred and thirty volts at a full attenuverter" },
			{  0.f,  bound,   1.f,  true,  "PINNED through the FM route: EXACTLY PLUS the bound at a full attenuverter" },
			{  0.f, -bound,   1.f,  true,  "PINNED through the FM route: EXACTLY MINUS the bound at a full attenuverter" },
			// --- a NON-FINITE ATTENUVERTER with a perfectly finite voltage: the
			//     hostile value arrives through the KNOB rather than the cable ---
			{  0.f,  5.f,  qnan,     true,  "FM route: a FINITE five volts through a NOT-A-NUMBER attenuverter" },
			{  0.f,  5.f,  pinf,     true,  "FM route: a FINITE five volts through a POSITIVELY INFINITE attenuverter" },
			{  0.f,  5.f,  ninf,     true,  "FM route: a FINITE five volts through a NEGATIVELY INFINITE attenuverter" },
		};
		const size_t GRID_N = sizeof(GRID) / sizeof(GRID[0]);

		for (double sr : SAMPLE_RATES) {
			const float srf = (float)sr;
			const float dt  = (float)(1.0 / sr);

			// The ceiling the core is SUPPOSED to have applied, recomputed from
			// the same constant and the same sanitising rule the header uses, so
			// this case states the contract independently instead of echoing
			// whatever the header happened to compute.
			const float expectedMaxFreq = forge::kVcoNyquistGuardFrac * ((srf > 0.f) ? srf : 0.f);

			for (size_t gi = 0; gi < GRID_N; ++gi) {
				const HostilePitchPoint& p = GRID[gi];
				const char* role = p.role;

				forge::VcoCore core;
				seedLikeDriver(core);

				// TIMING IS DELIBERATELY LEGITIMATE at every point (D-15): this
				// case is about the pitch and FM fields, and hostile timing
				// belongs to a later phase.
				forge::VcoInputs in = pitchBase();
				in.pitchCV     = p.pitchCV;
				in.fmVolts     = p.fmVolts;
				in.fmAtten     = p.fmAtten;
				in.fmConnected = p.fmConnected;
				in.morph       = 0.5f;
				in.character   = 1.f;
				in.sampleTime  = dt;
				in.sampleRate  = srf;

				// ACCUMULATED, not asserted per sample: 78 configurations at
				// 4000 steps would otherwise add over a million assertions to
				// the suite. Same idiom as the other VCO core file's hostile
				// scenarios, with a first-bad-step index carried out so a red
				// names a step rather than only a property.
				bool   allFinite          = true;
				bool   insideLooseBound   = true;
				bool   phaseInRange       = true;
				bool   freqNonNegative    = true;
				bool   freqNyquistBounded = true;
				double maxAbs             = 0.0;
				double maxFreqSeen        = 0.0;
				int    firstBadStep       = -1;

				for (int i = 0; i < kHostileSteps; ++i) {
					const float s = core.step(in);
					const double a = std::fabs((double)s);
					if (a > maxAbs) maxAbs = a;
					if ((double)core.tel.freqHz > maxFreqSeen) maxFreqSeen = (double)core.tel.freqHz;

					bool bad = false;
					if (!std::isfinite(s))                       { allFinite = false;          bad = true; }
					if (!(a <= (double)kPitchLooseBoundV))        { insideLooseBound = false;   bad = true; }
					if (!(core.phase >= 0.0))                    { phaseInRange = false;       bad = true; }
					if (!(core.phase < 1.0))                     { phaseInRange = false;       bad = true; }
					// Negated so a not-a-number telemetry frequency reads as a
					// failure, the same way the core's own floors are written.
					if (!(core.tel.freqHz >= 0.f))               { freqNonNegative = false;    bad = true; }
					if (!(core.tel.freqHz <= expectedMaxFreq))   { freqNyquistBounded = false; bad = true; }
					if (bad && firstBadStep < 0) firstBadStep = i;
				}

				CAPTURE(sr);
				CAPTURE(role);
				CAPTURE(p.pitchCV);
				CAPTURE(p.fmVolts);
				CAPTURE(p.fmAtten);
				CAPTURE(p.fmConnected);
				CAPTURE(maxAbs);
				CAPTURE(maxFreqSeen);
				CAPTURE(expectedMaxFreq);
				CAPTURE(firstBadStep);
				INFO("standing hostile-pitch regression check with LEGITIMATE timing - the RED for this bound was the one-shot sanitizer probe recorded in plan 31-03's summary, NOT this case");

				CHECK(allFinite);
				CHECK(insideLooseBound);
				CHECK(phaseInRange);
				CHECK(freqNonNegative);
				CHECK(freqNyquistBounded);
			}
		}
	}

	SUBCASE("D-14 reachable-envelope margin: the worst sum a legitimate patch can produce sits at least a factor of two inside the bound") {
		// WHY THIS ASSERTION IS ARITHMETIC, AND WHY NO BEHAVIORAL FORM OF IT
		// EXISTS. The property being guaranteed is that the pitch-volt bound can
		// never fire on a patch a musician could actually build. That property is
		// NOT OBSERVABLE THROUGH THE OUTPUT AT ALL: at the volts in question the
		// Nyquist ceiling dominates the frequency completely, so a block driven
		// at twenty-nine volts and a block driven at sixty-four volts return the
		// SAME SAMPLES. Whether the pitch-volt bound fired is invisible. That
		// invisibility is not a gap in the test — IT IS THE WHOLE REASON THE
		// BOUND IS INAUDIBLE IN NORMAL USE, and it is precisely why the
		// guarantee has to be stated as arithmetic rather than measured.
		//
		// BUILT FROM THE DECLARED CONTROL RANGES, NOT FROM A TYPED-IN TOTAL, so
		// it is THIS ASSERTION that fires if a future phase widens a control's
		// range or lowers the bound — which is the only way this threat becomes
		// real. A typed-in total would keep passing while the thing it summarised
		// went stale.
		//
		// Computed in double so the sum carries no float rounding of its own.

		// Term one: the conventional maximum cable magnitude on the V/OCT input.
		// The platform's documented signal-cable norm.
		const double maxCableVolts = 12.0;

		// Term two: the coarse tune control's declared magnitude, in OCTAVES,
		// which is already the volt domain at one octave per volt.
		const double coarseRangeVolts = 5.0;

		// Term three: the fine tune control's declared magnitude, in SEMITONES,
		// divided by twelve exactly as the core divides it (D-05).
		const double fineRangeVolts = 1.0 / 12.0;

		// Term four: the same maximum cable magnitude on the FM input, times a
		// FULL attenuverter, which the declared range makes unity and which D-06
		// fixes at one octave per volt with no depth constant anywhere.
		const double fmTermVolts = maxCableVolts * 1.0;

		const double worstReachableVolts =
			maxCableVolts + coarseRangeVolts + fineRangeVolts + fmTermVolts;

		// THE BOUND ITSELF, SYMBOLIC. Never a literal.
		const double boundVolts  = (double)forge::kVcoMaxPitchVolts;
		const double marginRatio = boundVolts / worstReachableVolts;

		CAPTURE(maxCableVolts);
		CAPTURE(coarseRangeVolts);
		CAPTURE(fineRangeVolts);
		CAPTURE(fmTermVolts);
		CAPTURE(worstReachableVolts);
		CAPTURE(boundVolts);
		CAPTURE(marginRatio);
		INFO("if this goes red, either a control's declared range widened or the pitch-volt bound was lowered - the bound has become reachable from a legitimate patch and that is a REQUIREMENT-LEVEL regression, not a test problem");

		CHECK(worstReachableVolts < boundVolts);
		CHECK(marginRatio >= 2.0);
	}

	SUBCASE("PITCH-05 non-regression: the double accumulator stays inside its half-open range over a long block at a high pitch") {
		// PITCH-05, HALF TWO: the runtime counterpart to the compile-time type
		// pin at the top of this case. A long block driven ABOVE the clamp
		// ceiling is the hardest condition for the accumulator — the ceiling
		// fires on every sample, so the increment sits at its bound and the
		// single-subtract wrap runs constantly. An accumulator that drifted, or a
		// wrap that failed to keep up with a maximal increment, shows here.
		//
		// (The other half of PITCH-05 is a SOURCE-level property rather than a
		// behavioral one — the phase increment is computed with BOTH operands
		// cast to double — and it is verified by this plan's acceptance criteria
		// against the core header directly, because a cast that was dropped would
		// still produce a passing block at these tolerances.)
		const int kLongBlockSteps = 100000;

		for (double sr : SAMPLE_RATES) {
			forge::VcoCore core;
			seedLikeDriver(core);

			forge::VcoInputs in = pitchBase();
			in.pitchCV    = (float)(clampCeilingVolts(sr) + 1.0);
			in.morph      = 0.f;
			in.character  = 0.f;
			in.sampleTime = (float)(1.0 / sr);
			in.sampleRate = (float)sr;

			bool phaseInRange = true;
			int  firstBadStep = -1;
			for (int i = 0; i < kLongBlockSteps; ++i) {
				core.step(in);
				if (!(core.phase >= 0.0)) { phaseInRange = false; if (firstBadStep < 0) firstBadStep = i; }
				if (!(core.phase < 1.0))  { phaseInRange = false; if (firstBadStep < 0) firstBadStep = i; }
			}

			const double finalPhase = core.phase;
			CAPTURE(sr);
			CAPTURE(in.pitchCV);
			CAPTURE(finalPhase);
			CAPTURE(firstBadStep);
			CHECK(phaseInRange);
		}
	}
}

// ---------------------------------------------------------------------------
// 10. PITCH-04 / D-12: THE NYQUIST CLAMP RE-CONFIRMED WITH **SYNC** AS THE
//     THIRD INPUT CLASS — AND THE DEBT THAT MADE THE RE-CONFIRMATION NECESSARY.
//
//     ADDED BY PLAN 33-09. Invariants 1-9 above are renumbered by nothing here.
//
//     ---------------------------------------------------------------------
//     (1) WHAT THIS CASE RE-CONFIRMS, AND WHO TICKED IT FIRST.
//
//     PITCH-04, in the requirement's own words: *"Frequency is clamped just
//     below Nyquist so extreme pitch/FM/sync never aliases via out-of-range
//     frequency."* It was marked COMPLETE by **PHASE 31**, on the evidence of
//     invariant 8 (the clamp firing at its derived ceiling) and invariant 9
//     (the hostile pitch and FM grid). Nothing in that evidence is withdrawn
//     here and nothing in it is repeated here; this case is the third leg.
//
//     ---------------------------------------------------------------------
//     (2) THE EXACT DEBT BEING CLOSED — Phase 31 deferred item 11.
//
//     PITCH-04 names THREE input classes and Phase 31 could reach only TWO.
//     `forge::VcoInputs` carried no sync field at all when invariants 8 and 9
//     were written, so the requirement was ticked on **two of the three input
//     classes it names**. Item 11 records the reason it declined to close the
//     gap by argument, and the sentence is worth having in front of a reader
//     of this case: *"the clamp sits downstream of the frequency, so a
//     sync-driven pitch source cannot bypass it STRUCTURALLY — and that
//     structural argument is exactly the kind of forward claim this phase has
//     repeatedly declined to make on another phase's behalf. A green mark for
//     an input class that was UNREACHABLE when the assertion was written is not
//     coverage of that class."* Its Resolve-at names **Phase 33** and requires
//     the clamp to be RE-CONFIRMED rather than inherited. This is that phase,
//     and this is that case. The structural argument item 11 refused to lean on
//     is now MEASURED instead: see `telemetryUnmovedBySync` below.
//
//     ---------------------------------------------------------------------
//     (3) WHAT THIS CASE ASSERTS.
//
//     Over 783 cells — 29 pitch/FM rows x 9 sync shapes x 3 sample rates, at
//     4000 steps each:
//       - every returned sample FINITE;
//       - every magnitude inside kPitchLooseBoundV, this file's outer tier;
//       - the phase accumulator inside its half-open [0, 1) range;
//       - the clamped frequency non-negative and at or below the ceiling
//         recomputed symbolically from the forge:: constants;
//       - the clamp behaving as the row CLASSIFIES it — pinned EXACTLY at the
//         ceiling above it, strictly under it below it, floored to silence on a
//         sanitised not-a-number path;
//       - the clamped frequency EXACTLY EQUAL to the unpatched row's on every
//         patched sync shape, which is item 11's structural claim measured;
//       - and the sync firing count matching the detector's own state machine,
//         in BOTH directions, on every cell.
//     Over a further 45 cells, at a half-second window:
//       - 1 V/oct TRACKING ACCURACY against the libm reference at unison, at
//         this file's own kTrackingToleranceCents, unwidened;
//       - the MASTER-LOCK — the output period is the master's — over the
//         subset where the estimator is valid, at its own pinned tolerance;
//       - and two out-of-window controls that keep that subset a measurement.
//
//     ---------------------------------------------------------------------
//     (4) WHAT THIS CASE DELIBERATELY DOES **NOT** ASSERT, AND WHERE EACH OF
//         THOSE CLAIMS ACTUALLY LIVES.
//
//     Naming the neighbours is what stops a later reader treating this case as
//     broader than it is. Every title below RESOLVES as a doctest selector.
//
//       (a) NO PITCH CLAIM WHERE THE ESTIMATOR IS BLIND. Twelve of the 29
//           pitch/FM rows are driven past the clamp ceiling, where the pinned
//           frequency is 1 / 0.495 = 2.0202 samples per cycle — BELOW
//           kEstimatorMinSamplesPerCycle. TRAP 3 records that the estimator
//           does not fail loudly there, it returns a plausible wrong answer on
//           a perfectly correct oscillator. Those rows are asserted for
//           finiteness, bound, accumulator, clamp and firing, and for NOTHING
//           about pitch. There is no neighbouring case for this one because
//           there is no instrument for it — that is the point of stating it.
//
//       (b) NO CLAIM ABOUT THE SYNC CORRECTION'S SPECTRAL QUALITY. That lives
//           in `"vco spectrum: (SYNC-02 / D-11) the sync alias floor stays
//           below its per-cell pinned threshold, and every pinned number
//           reproduces"`, 1 case / 5286 assertions.
//           NOTE FOR ANYONE RUNNING THAT SELECTOR: the title contains a COMMA
//           and doctest treats a comma inside -tc as a FILTER SEPARATOR, so the
//           full title matches ZERO cases. The form that resolves is the
//           wildcard prefix `-tc="vco spectrum: (SYNC-02 / D-11) the sync alias
//           floor*"`. Recorded here because a selector that silently matches
//           nothing is precisely the failure this project keeps logging.
//
//       (c) NO CLAIM ABOUT THE PER-SAMPLE STEP ACROSS A RESET. That is the
//           time-domain continuity gate, `"vco sync: (SC-3 / D-10) the
//           per-sample step across a reset is bounded by a measured envelope"`
//           (1 case / 32 assertions), with its anti-circularity half in
//           `"vco sync: (SC-3 / D-10) the corrected reset delta beats the
//           uncorrected one by a measured margin"` (1 case / 576 assertions).
//           Those two own SC-3. This case owns PITCH-04 and nothing else.
//
//     A FOURTH NEIGHBOUR, named because it is a SIBLING rather than a boundary:
//     the new divisor is exercised in the core suite too, by `"vco sync: (D-12)
//     the new divisor cannot poison the phase accumulator"` (1 case / 639
//     assertions). It is exercised in BOTH places on purpose. A new division
//     appearing behind a new input field is the rationale that moved a hostile
//     grid into the preceding phase under that phase's D-15 — and that
//     rationale was itself a CORRECTION of a falsified one, so it is applied
//     deliberately here rather than inherited. What the core suite cannot say
//     is what this case says: that the divisor's hostile inputs cannot move the
//     PITCH, which is a claim about PITCH-04 and belongs in the pitch file.
//
//     ---------------------------------------------------------------------
//     (5) THE NON-VACUITY CONDITION, IN ONE SENTENCE.
//
//     EVERY PITCH CLAIM IN THIS CASE STANDS ONLY WHILE ITS SYNC-FIRING COUNTS
//     ARE NON-ZERO, AND THE FIRING ASSERTIONS ARE WHAT ENFORCE THAT — remove
//     the reset and 522 of them go red in the first subcase and 27 in the
//     second, which was measured rather than assumed before this banner was
//     written.
// ---------------------------------------------------------------------------
TEST_CASE("vco pitch: (PITCH-04 / D-12) the Nyquist clamp RE-CONFIRMED with SYNC as the THIRD input class, on the SAME hostile pitch and FM population invariant 9 drives - closing Phase 31 deferred item 11, with the detector OBSERVED FIRING behind every claim") {
	// -------------------------------------------------------------------
	// WHY THIS CASE IS IN THIS FILE AND NOT A NEW ONE SOMEWHERE ELSE.
	//
	// D-12 rejects a dedicated separate case BY NAME, on the ground that it
	// SPLITS ONE REQUIREMENT'S EVIDENCE ACROSS TWO FILES. PITCH-04's evidence
	// lives here -- invariant 8 is its clamp case and invariant 9 its hostile
	// grid -- so the third input class lives here too, immediately below them,
	// driving the SAME pitch/FM population invariant 9 drives.
	//
	// AND WHY IT IS NOT FOLDED INTO INVARIANT 9 ITSELF. D-12 rejects that too,
	// for a reason that is about instruments rather than about tidiness:
	// invariant 9 asserts FINITENESS AND BOUNDEDNESS, and it is explicitly NOT
	// the red for the bound it pins. It is the wrong instrument for a claim
	// about PITCH. Subcase two below makes a claim about pitch, measured on the
	// RETURNED SAMPLES, and it needed a case that could say so in its own title.
	// Both halves therefore live here, in ONE case, in ONE file.
	// -------------------------------------------------------------------

	// How many steps each cell of the hostile product is driven for. The same
	// figure invariant 9 uses, deliberately: this subcase is that grid with a
	// third axis, not a new drive with new timing.
	const int kSyncHostileSteps = 4000;

	SUBCASE("the three input classes driven TOGETHER: extreme pitch x extreme FM x hostile sync, with the firing count ASSERTED per cell") {
		// -------------------------------------------------------------------
		// THE PITCH / FM AXIS. Rows 1-26 are invariant 9's population, copied
		// row for row so this is demonstrably the SAME two input classes rather
		// than a fresh selection that happens to look similar. Rows 27-29 are
		// invariant 8's own points -- one volt below the clamp ceiling, one volt
		// above, three volts above -- because a re-confirmation of a CLAMP has
		// to include the points where the clamp is the thing being observed.
		//
		// THE `expect` COLUMN IS STATED PER ROW AND IS NOT RECOMPUTED FROM THE
		// CORE'S OWN SUMMATION. Deriving it here from pitchCV + fmVolts *
		// fmAtten would be TRAP 2: a mirror of the code under test, green even
		// if that code were replaced by a constant. Each row states what the
		// clamp is required to have done and the role string says why.
		// -------------------------------------------------------------------
		struct SyncPitchPoint {
			float pitchCV;
			int   ceilingRelative;   // 1 = pitchCV is IGNORED, the row sits at ceiling + ceilingOffsetVolts
			double ceilingOffsetVolts;
			float fmVolts;
			float fmAtten;
			bool  fmConnected;
			ClampExpectation expect;
			const char* role;
		};

		const float qnan  = std::numeric_limits<float>::quiet_NaN();
		const float pinf  = std::numeric_limits<float>::infinity();
		const float ninf  = -std::numeric_limits<float>::infinity();
		const float huge  = 1e30f;
		// Read from the forge:: constant and never typed in, exactly as
		// invariant 9 reads it.
		const float bound = forge::kVcoMaxPitchVolts;

		const SyncPitchPoint PITCH_GRID[] = {
			// --- routed through the V/OCT pitch field, FM jack unpatched ---
			{  0.f,    0, 0.0, 0.f, 0.f, false, kClampBelow,     "the CONTROL: a legitimate zero-volt pitch, FM jack unpatched" },
			{  qnan,   0, 0.0, 0.f, 0.f, false, kClampSilent,    "pitch: a quiet not-a-number" },
			{  pinf,   0, 0.0, 0.f, 0.f, false, kClampAtCeiling, "pitch: positive infinity" },
			{  ninf,   0, 0.0, 0.f, 0.f, false, kClampBelow,     "pitch: negative infinity" },
			{  huge,   0, 0.0, 0.f, 0.f, false, kClampAtCeiling, "pitch: a very large POSITIVE finite magnitude" },
			{ -huge,   0, 0.0, 0.f, 0.f, false, kClampBelow,     "pitch: a very large NEGATIVE finite magnitude" },
			{  200.f,  0, 0.0, 0.f, 0.f, false, kClampAtCeiling, "pitch: plus two hundred volts" },
			{ -200.f,  0, 0.0, 0.f, 0.f, false, kClampBelow,     "pitch: minus two hundred volts" },
			{  130.f,  0, 0.0, 0.f, 0.f, false, kClampAtCeiling, "pitch: plus one hundred and thirty volts" },
			{ -130.f,  0, 0.0, 0.f, 0.f, false, kClampBelow,     "pitch: minus one hundred and thirty volts" },
			{  bound,  0, 0.0, 0.f, 0.f, false, kClampAtCeiling, "PINNED (D-22): pitch driven to EXACTLY PLUS the bound" },
			{ -bound,  0, 0.0, 0.f, 0.f, false, kClampBelow,     "PINNED (D-22): pitch driven to EXACTLY MINUS the bound" },
			// --- routed through the FM field instead, jack CONNECTED, attenuverter FULL ---
			{  0.f, 0, 0.0, qnan,   1.f, true, kClampSilent,    "FM route: a quiet not-a-number at a full attenuverter" },
			{  0.f, 0, 0.0, pinf,   1.f, true, kClampAtCeiling, "FM route: positive infinity at a full attenuverter" },
			{  0.f, 0, 0.0, ninf,   1.f, true, kClampBelow,     "FM route: negative infinity at a full attenuverter" },
			{  0.f, 0, 0.0, huge,   1.f, true, kClampAtCeiling, "FM route: a very large POSITIVE finite magnitude" },
			{  0.f, 0, 0.0, -huge,  1.f, true, kClampBelow,     "FM route: a very large NEGATIVE finite magnitude" },
			{  0.f, 0, 0.0, 200.f,  1.f, true, kClampAtCeiling, "FM route: plus two hundred volts" },
			{  0.f, 0, 0.0, -200.f, 1.f, true, kClampBelow,     "FM route: minus two hundred volts" },
			{  0.f, 0, 0.0, 130.f,  1.f, true, kClampAtCeiling, "FM route: plus one hundred and thirty volts" },
			{  0.f, 0, 0.0, -130.f, 1.f, true, kClampBelow,     "FM route: minus one hundred and thirty volts" },
			{  0.f, 0, 0.0, bound,  1.f, true, kClampAtCeiling, "PINNED through the FM route: EXACTLY PLUS the bound" },
			{  0.f, 0, 0.0, -bound, 1.f, true, kClampBelow,     "PINNED through the FM route: EXACTLY MINUS the bound" },
			// --- a NON-FINITE ATTENUVERTER with a perfectly finite voltage ---
			{  0.f, 0, 0.0, 5.f, qnan, true, kClampSilent,    "FM route: a FINITE five volts through a NOT-A-NUMBER attenuverter" },
			{  0.f, 0, 0.0, 5.f, pinf, true, kClampAtCeiling, "FM route: a FINITE five volts through a POSITIVELY INFINITE attenuverter" },
			{  0.f, 0, 0.0, 5.f, ninf, true, kClampBelow,     "FM route: a FINITE five volts through a NEGATIVELY INFINITE attenuverter" },
			// --- invariant 8's own boundary points, brought in so the clamp is
			//     observed BINDING and observed NOT firing early, under sync ---
			{  0.f, 1, -1.0, 0.f, 0.f, false, kClampBelow,     "one volt BELOW the derived clamp ceiling: the clamp must NOT fire" },
			{  0.f, 1, +1.0, 0.f, 0.f, false, kClampAtCeiling, "one volt ABOVE the derived clamp ceiling: the clamp must pin EXACTLY" },
			{  0.f, 1, +3.0, 0.f, 0.f, false, kClampAtCeiling, "three volts ABOVE the ceiling: eight times the ceiling frequency before clamping" },
		};
		const size_t PITCH_N = sizeof(PITCH_GRID) / sizeof(PITCH_GRID[0]);

		// -------------------------------------------------------------------
		// THE SYNC AXIS -- the third input class, and the phase's new divisor.
		//
		// `mustFire` is a claim about the DETECTOR'S MECHANISM, written from
		// forge::SchmittTrigger's own state machine rather than from what the
		// run happened to produce. Three rows CANNOT fire, and each cannot for
		// a different named reason (see syncMasterVolts above); the other five
		// must. Both directions are asserted, because a firing counter that is
		// only ever checked for "greater than zero" has not been shown to be
		// counting the right thing.
		// -------------------------------------------------------------------
		const SyncAxisRow SYNC_AXIS[] = {
			{ kSyncUnpatched,   0.0,     false, "the CONTROL: the SYNC jack UNPATCHED - invariant 9's own evidence, preserved as a ROW rather than replaced" },
			{ kSyncRamp,        110.0,   true,  "a legitimate master at a musical rate (110 Hz)" },
			{ kSyncRamp,        55.0,    true,  "a legitimate master an octave lower (55 Hz)" },
			{ kSyncRamp,        70000.0, true,  "a master FAR ABOVE the sample rate (70 kHz)" },
			{ kSyncIdleHigh,    0.0,     false, "NEW DIVISOR: a master IDLING at a steady five volts from sample zero" },
			{ kSyncNanHeld,     0.0,     false, "NEW DIVISOR: a NOT-A-NUMBER cable voltage, held for the whole block" },
			{ kSyncOnThreshold, 0.0,     true,  "NEW DIVISOR: a master sample landing EXACTLY on the 1.0 V high threshold" },
			{ kSyncNanGlitch,   110.0,   true,  "NEW DIVISOR: a NOT-A-NUMBER GLITCH stored against an otherwise legitimate master" },
			{ kSyncStaleStore,  0.0,     true,  "NEW DIVISOR: LOW, then a steady five volts held against the store" },
		};
		const size_t SYNC_N = sizeof(SYNC_AXIS) / sizeof(SYNC_AXIS[0]);

		// The populations are pinned so a later edit that drops a row is loud.
		REQUIRE(PITCH_N == 29u);
		REQUIRE(SYNC_N  == 9u);

		for (double sr : SAMPLE_RATES) {
			const float  srf = (float)sr;
			const double ceilingVolts = clampCeilingVolts(sr);

			// Recomputed the way the core computes it, in FLOAT, through the
			// same non-positive-rate sanitising rule -- invariant 8's symbolic
			// idiom, so the exact equality below is not a coin flip (D-21).
			const float expectedMaxFreq = forge::kVcoNyquistGuardFrac * ((srf > 0.f) ? srf : 0.f);

			for (size_t pi = 0; pi < PITCH_N; ++pi) {
				const SyncPitchPoint& p = PITCH_GRID[pi];
				const float pitchVolts = (p.ceilingRelative == 1)
					? (float)(ceilingVolts + p.ceilingOffsetVolts)
					: p.pitchCV;

				// The UNPATCHED row's clamped frequency, captured on the first
				// pass of the inner loop and compared against every patched row
				// on the same pitch point. See the assertion's own comment.
				float unpatchedTelFreq = 0.f;

				for (size_t si = 0; si < SYNC_N; ++si) {
					const SyncAxisRow& s = SYNC_AXIS[si];
					const char* role     = p.role;
					const char* syncRole = s.role;

					forge::VcoBlockDriver d(sr);

					forge::VcoInputs base = pitchBase();
					base.pitchCV     = pitchVolts;
					base.fmVolts     = p.fmVolts;
					base.fmAtten     = p.fmAtten;
					base.fmConnected = p.fmConnected;
					base.morph       = 0.5f;
					base.character   = 1.f;
					base.syncConnected = (s.kind != kSyncUnpatched);

					// ACCUMULATED, not asserted per sample: 783 cells at 4000
					// steps would otherwise add over three million assertions.
					// Same idiom as invariant 9, with a first-bad-step index
					// carried out so a red names a step rather than a property.
					long   fireCount          = 0;
					bool   allFinite          = true;
					bool   insideLooseBound   = true;
					bool   phaseInRange       = true;
					double maxAbs             = 0.0;
					int    firstBadStep       = -1;

					for (int i = 0; i < kSyncHostileSteps; ++i) {
						forge::VcoInputs in = base;
						in.syncVolts = syncMasterVolts(s, i, sr);
						// sampleTime and sampleRate are the driver's to own, but
						// this loop calls step(...) directly so that the sync
						// voltage can vary per sample; both are written here on
						// every sample for the same reason the driver writes
						// them, and never conditionally.
						in.sampleTime = (float)(1.0 / sr);
						in.sampleRate = srf;

						const float sample = d.core.step(in);
						if (d.core.tel.syncFired) ++fireCount;

						const double a = std::fabs((double)sample);
						if (a > maxAbs) maxAbs = a;

						bool bad = false;
						if (!std::isfinite(sample))            { allFinite = false;        bad = true; }
						if (!(a <= (double)kPitchLooseBoundV)) { insideLooseBound = false; bad = true; }
						if (!(d.core.phase >= 0.0))            { phaseInRange = false;     bad = true; }
						if (!(d.core.phase < 1.0))             { phaseInRange = false;     bad = true; }
						if (bad && firstBadStep < 0) firstBadStep = i;
					}

					const float telFreq = d.core.tel.freqHz;
					if (si == 0) unpatchedTelFreq = telFreq;

					// Every comparison below is written NEGATED so that a
					// not-a-number telemetry frequency lands on the FAILING
					// branch, which is the shape the core's own floors use and
					// the shape invariant 8 uses for exactly this reason.
					bool freqNonNegative = true;
					if (!(telFreq >= 0.f)) freqNonNegative = false;

					bool freqNyquistBounded = true;
					if (!(telFreq <= expectedMaxFreq)) freqNyquistBounded = false;

					// THE CLAMP, AS CLASSIFIED BY THE ROW. Above the ceiling it
					// must pin EXACTLY; below it must be strictly under, which is
					// what stops a core that clamps everything from passing; on
					// a sanitised not-a-number path it must floor to silence.
					// MEASURED for the silence rows: 1.418275e-17 Hz at all three
					// rates and all nine sync shapes -- seventeen orders of
					// magnitude below the one hertz asserted here.
					bool clampBehavedAsClassified = true;
					if (p.expect == kClampAtCeiling) { if (!(telFreq == expectedMaxFreq)) clampBehavedAsClassified = false; }
					else if (p.expect == kClampBelow) { if (!(telFreq < expectedMaxFreq))  clampBehavedAsClassified = false; }
					else                              { if (!(telFreq < 1.f))              clampBehavedAsClassified = false; }

					// THE ASSERTION PHASE 31 DEFERRED ITEM 11 ASKED FOR, AND THE
					// ONE IT REFUSED TO MAKE ON FAITH. Item 11 records that "the
					// clamp sits downstream of the frequency, so a sync-driven
					// pitch source cannot bypass it STRUCTURALLY -- and that
					// structural argument is exactly the kind of forward claim
					// this phase has repeatedly declined to make on another
					// phase's behalf." So it is MEASURED here instead of argued:
					// the clamped frequency on every patched sync shape is
					// compared to the UNPATCHED row's, on the same pitch point
					// and the same rate, by EXACT float equality. Not a
					// tolerance. If sync ever perturbs the frequency chain by
					// one bit, this is red.
					bool telemetryUnmovedBySync = true;
					if (!(telFreq == unpatchedTelFreq)) telemetryUnmovedBySync = false;

					// THE NON-VACUITY HALF, AND THE POINT OF THE PLAN.
					//
					// A hostile grid in which sync never fires reports green
					// while covering the sync path with NOTHING. Every assertion
					// above would hold on a core that ignored in.syncVolts
					// entirely -- the clamp claims especially, since the clamp is
					// upstream of the reset. This project has ELEVEN consecutive
					// plans on record declining to tick a requirement whose own
					// frontmatter already claimed it, and the discipline behind
					// those declines is exactly this: observe the control FIRING
					// behind the claim rather than inferring it from the patch.
					//
					// So the firing count is ASSERTED, in BOTH directions, and
					// the direction is a property of the detector's state machine
					// rather than of the run. MEASURED counts over 4000 steps,
					// as (minimum .. maximum) across all 87 cells of each row:
					//   UNPATCHED                       0 ..   0
					//   musical master 110 Hz           5 ..  10
					//   musical master 55 Hz            3 ..   5
					//   master far above the rate      84 .. 167
					//   idling at a steady 5 V          0 ..   0
					//   not-a-number cable, held        0 ..   0
					//   exactly on the high threshold  62 ..  62
					//   not-a-number glitch             5 ..  10
					//   low then steady 5 V             1 ..   1
					bool firingAsClassified = true;
					if (s.mustFire) { if (!(fireCount > 0))  firingAsClassified = false; }
					else            { if (!(fireCount == 0)) firingAsClassified = false; }

					CAPTURE(sr);
					// Streamed rather than CAPTUREd: doctest renders a
					// `const char*` capture as its ADDRESS, which is useless in
					// a 783-cell grid where the row name is the only thing that
					// identifies the failure.
					INFO("pitch/FM row: " << role);
					INFO("sync row    : " << syncRole);
					CAPTURE(pitchVolts);
					CAPTURE(p.fmVolts);
					CAPTURE(p.fmAtten);
					CAPTURE(fireCount);
					CAPTURE(s.mustFire);
					CAPTURE(maxAbs);
					CAPTURE(telFreq);
					CAPTURE(unpatchedTelFreq);
					CAPTURE(expectedMaxFreq);
					CAPTURE(firstBadStep);
					INFO("PITCH-04's THIRD input class. The clamp claims here would all be green on a core that ignored in.syncVolts entirely - the firing assertion is the only thing standing between this grid and that vacuity");

					CHECK(allFinite);
					CHECK(insideLooseBound);
					CHECK(phaseInRange);
					CHECK(freqNonNegative);
					CHECK(freqNyquistBounded);
					CHECK(clampBehavedAsClassified);
					CHECK(telemetryUnmovedBySync);
					CHECK(firingAsClassified);
				}
			}
		}
	}

	SUBCASE("the PITCH claim, made ONLY where the crossing estimator can see - and NOT made where it is blind") {
		// -------------------------------------------------------------------
		// THE RESTRICTION, STATED ON THE INSTRUMENT'S KNOWN LIMITATION, BEFORE
		// ANY POPULATION IS ENUMERATED.
		//
		// estimateFreqRising counts RISING ZERO CROSSINGS. TRAP 3 at the top of
		// this file records what it does near its resolution floor, and the
		// wording matters: it does NOT fail loudly there, it returns a plausible
		// wrong answer, tens of cents off, on a PERFECTLY CORRECT oscillator.
		// kEstimatorMinSamplesPerCycle = 2.5 is where this file stops trusting
		// it, and that cutoff was MEASURED -- good at 2.634 samples per cycle,
		// broken at 2.027.
		//
		// AT THE NYQUIST CEILING THE ESTIMATOR IS STRUCTURALLY BLIND, and this
		// is arithmetic from the forge:: constant rather than an opinion: a
		// frequency pinned at kVcoNyquistGuardFrac x the sample rate is
		// 1 / 0.495 = 2.0202 samples per cycle at EVERY rate, which is BELOW the
		// cutoff. The subcase above therefore drives twelve pitch rows past the
		// ceiling and makes NO PITCH CLAIM ON ANY OF THEM. That is not a gap and
		// it is not the cells that failed being quietly dropped -- it is the one
		// place in this file where the ruler provably cannot read.
		const double samplesPerCycleAtCeiling = 1.0 / (double)forge::kVcoNyquistGuardFrac;
		CAPTURE(samplesPerCycleAtCeiling);
		CAPTURE(kEstimatorMinSamplesPerCycle);
		INFO("if this goes red the Nyquist guard fraction has moved far enough that the estimator can suddenly see the clamped frequency - which would mean this subcase's whole restriction needs re-deriving, not relaxing");
		CHECK(samplesPerCycleAtCeiling < kEstimatorMinSamplesPerCycle);

		// AND THE OTHER HALF OF THE SAME HONESTY. Band-limiting (Phase 32) made
		// this estimator's narrow-pulse tracking measurably WORSE at high pitch.
		// That was measured to be a property of the estimator's SAMPLING -- the
		// polyBLEP residual moves the sample either side of a crossing, and near
		// the resolution floor there are too few samples per cycle for the linear
		// interpolation to recover it -- and NOT a defect in the DSP. HARD SYNC
		// MUST NOT BE BLAMED FOR IT EITHER. Nothing in this subcase asserts
		// anything at a samples-per-cycle count where that effect lives; the
		// smallest count anywhere in the population below is 200.45.

		// -------------------------------------------------------------------
		// THE PHYSICAL CRITERION FOR THE POPULATION, ALSO STATED BEFORE THE
		// ENUMERATION.
		//
		// Hard sync REPLACES the slave's period with the master's: the phase is
		// reset on every master rising edge, so the output repeats at the
		// MASTER's rate whenever the reset arrives before the slave has finished
		// a second cycle AND after the slave has passed its own rising zero
		// crossing. Writing rho for slaveHz / masterHz, that is a window in rho,
		// and it was MEASURED on this waveform (morph 0.50, character 1.00)
		// rather than reasoned about: over a sweep of rho in steps of 0.005 at
		// all three rates and at both master rates, the output locks to the
		// master over
		//
		//     rho in [0.320, 1.310]
		//
		// identically at every rate. BELOW 0.320 the slave never reaches its own
		// rising crossing between resets and the estimator returns its negative
		// sentinel; ABOVE 1.310 a second crossing appears inside each master
		// period and the reading walks toward twice the master rate.
		//
		// THE EDGES ARE SHARP, NOT A GAP -- rho = 0.315 does not lock and
		// rho = 0.320 does -- and that is recorded rather than glossed, because
		// plan 33-08 measured a threshold in this phase whose "physically
		// obvious" value had a boundary gap of one part in a million and would
		// plausibly have gone red on another toolchain. THE ANSWER TO A SHARP
		// EDGE IS MARGIN, NOT A TIGHTER PIN: this subcase's rho values are 0.50,
		// 0.75 and 1.00, which sit 0.18 inside the lower edge and 0.31 inside the
		// upper one, and the two OUT-OF-WINDOW controls at the bottom of this
		// subcase are what keep the window a measurement rather than a claim.
		const double kSyncLockRhoLo = 0.50;
		const double kSyncLockRhoHi = 1.00;
		const double kMeasuredLockRhoLo = 0.320;   // measured, this shape, all three rates
		const double kMeasuredLockRhoHi = 1.310;   // measured, this shape, all three rates
		CHECK(kSyncLockRhoLo - kMeasuredLockRhoLo >= 0.15);
		CHECK(kMeasuredLockRhoHi - kSyncLockRhoHi >= 0.25);

		// -------------------------------------------------------------------
		// THE WINDOW LENGTH, MEASURED RATHER THAN CHOSEN.
		//
		// The residual in the readings below is dominated by the estimator's
		// first-crossing-to-last-crossing endpoint timing, so it shrinks with the
		// window. Worst |cents| against the master over the whole population, at
		// three window lengths:
		//     0.25 s -> 0.148484      0.50 s -> 0.020443      1.00 s -> 0.015707
		// Half a second is the knee: doubling it again buys 0.005 cents. Taking
		// the quarter-second window instead would have forced a tolerance three
		// times looser for no reason other than a shorter run.
		const double kSyncLockWindowSeconds = 0.5;

		// THE TOLERANCE FOR THE MASTER-LOCK CLAIM, AND IT IS DELIBERATELY NOT
		// kTrackingToleranceCents.
		//
		// This is a DIFFERENT QUANTITY from the 1 V/oct law. It measures the
		// MASTER's period recovered through a reset discontinuity, not the
		// frozen exponential's accuracy, and it must not be read as, presented
		// as, or merged into this file's v/oct tolerance. Hence its own constant.
		//
		// PINNED FROM A TWO-SIDED INTERVAL, in the shape plan 33-08 used for the
		// SC-3 delta bound:
		//   (a) it must be AT OR ABOVE the measured worst, 0.020443 cents, over
		//       all 27 cells of the population below; and
		//   (b) it must be STRICTLY BELOW the ONE CENT that PITCH-01 actually
		//       asks for, so that passing it is a musically meaningful statement
		//       rather than a bound wide enough to admit an audible error.
		// Admissible interval [0.020443, 1.0). Pinned at 0.10 -- 4.89x above the
		// measurement, which is the Apple-clang-only cushion this phase has been
		// spending explicitly (plan 33-07 spent 5x on its snap floor, plan 33-08
		// spent 2.38x on its margin, and both said so), and 10x under PITCH-01.
		const double kSyncLockToleranceCents = 0.10;
		CHECK(kSyncLockToleranceCents > kTrackingToleranceCents);   // deliberately LOOSER, and never a widening of it
		CHECK(kSyncLockToleranceCents < 1.0);                       // and still strictly inside what PITCH-01 asks for

		// Master rates. Three, an octave apart, all far below every rate's
		// estimator ceiling so nothing here is near the resolution floor.
		static const double SYNC_MASTER_HZ[] = {55.0, 110.0, 220.0};
		static const size_t SYNC_MASTER_N = sizeof(SYNC_MASTER_HZ) / sizeof(SYNC_MASTER_HZ[0]);

		// rho = 1.00 is the UNISON row and it is a different claim from the other
		// two -- see its own branch below.
		static const double SYNC_LOCK_RHO[] = {0.50, 0.75, 1.00};
		static const size_t SYNC_LOCK_RHO_N = sizeof(SYNC_LOCK_RHO) / sizeof(SYNC_LOCK_RHO[0]);

		for (double sr : SAMPLE_RATES) {
			const int n = (int)std::lround(sr * kSyncLockWindowSeconds);

			for (size_t mi = 0; mi < SYNC_MASTER_N; ++mi) {
				const double masterHz = SYNC_MASTER_HZ[mi];

				for (size_t ki = 0; ki < SYNC_LOCK_RHO_N; ++ki) {
					const double rho      = SYNC_LOCK_RHO[ki];
					const double slaveHz  = masterHz * rho;
					// The volt that asks for that slave frequency, computed
					// against the DECIMAL C4 reference in double -- the same
					// independent ground truth expectedFreqHz uses, never the
					// header's float constant (TRAP 2).
					const double volts    = std::log2(slaveHz / 261.6256);
					const double reference = expectedFreqHz(volts);

					forge::VcoBlockDriver d(sr);
					forge::VcoInputs base = pitchBase();
					base.pitchCV   = (float)volts;
					base.morph     = 0.5f;
					base.character = 1.f;
					base.syncConnected = true;

					long fireCount = 0;
					std::vector<float> out;
					out.reserve((size_t)n);
					for (int i = 0; i < n; ++i) {
						forge::VcoInputs in = base;
						in.syncVolts  = (float)(5.0 * std::fmod((double)i * (masterHz / sr), 1.0));
						in.sampleTime = (float)(1.0 / sr);
						in.sampleRate = (float)sr;
						out.push_back(d.core.step(in));
						if (d.core.tel.syncFired) ++fireCount;
					}

					int nUp = 0;
					const double measured = estimateFreqRising(out, sr, &nUp);
					const double samplesPerCycle = (measured > 0.0) ? (sr / measured) : -1.0;

					CAPTURE(sr);
					CAPTURE(masterHz);
					CAPTURE(rho);
					CAPTURE(volts);
					CAPTURE(slaveHz);
					CAPTURE(reference);
					CAPTURE(measured);
					CAPTURE(nUp);
					CAPTURE(fireCount);
					CAPTURE(samplesPerCycle);

					// The oscillation precondition comes FIRST, the same
					// discipline the tracking tiers use: a block the estimator
					// could not read makes every number after it meaningless
					// rather than merely wrong.
					REQUIRE(nUp >= 16);
					REQUIRE(measured > 0.0);

					// AND THE INSTRUMENT IS INSIDE ITS OWN COMPETENCE HERE,
					// asserted rather than assumed. Measured minimum over the
					// whole population: 200.45 samples per cycle, eighty times
					// the cutoff.
					CHECK(samplesPerCycle > kEstimatorMinSamplesPerCycle);

					// THE NON-VACUITY TIE, RESTATED PER CELL. Every claim below
					// stands only while the detector actually fired on this
					// cell, and this is the assertion that says so.
					CHECK(fireCount > 0);

					if (rho == 1.00) {
						// -----------------------------------------------------
						// THE UNISON ROW: 1 V/oct SURVIVES HARD SYNC.
						//
						// The master is at the slave's own rate, so the measured
						// output frequency is checked against the LIBM REFERENCE
						// at this file's own kTrackingToleranceCents, UNWIDENED.
						// MEASURED worst over the nine unison cells: 0.006936
						// cents, a 7.2x margin -- better than the 5.17x the
						// primary tier records for itself.
						//
						// AND IT IS SAID PLAINLY THAT THIS ROW DOES NOT
						// DISCRIMINATE FOR SYNC. An unpatched slave at the same
						// volts would read the same frequency, so this row alone
						// cannot evidence that sync did anything. It is a
						// NON-REGRESSION claim -- hard sync does not break the
						// pitch law at unison -- and the firing assertion above
						// is what carries the sync half. The two rows below are
						// the discriminating ones.
						// -----------------------------------------------------
						const double cents = centsError(measured, reference);
						CAPTURE(cents);
						INFO("UNISON: this row is deliberately NOT discriminating for sync - an unpatched slave reads the same frequency. It says the v/oct law is not BROKEN by sync; the master-locked rows say sync is DOING something");
						bool withinTolerance = true;
						if (!(std::fabs(cents) < kTrackingToleranceCents)) withinTolerance = false;
						CHECK(withinTolerance);
					} else {
						// -----------------------------------------------------
						// THE MASTER-LOCKED ROWS: THE OUTPUT PITCH IS THE
						// MASTER'S, AND THIS IS THE CLAIM THAT ONLY EXISTS
						// BECAUSE SYNC FIRED.
						//
						// MEASURED worst over the eighteen cells: 0.020443
						// cents against the master.
						//
						// The separation below is what makes it discriminating,
						// and it is ASSERTED rather than described: an unsynced
						// slave at these volts would read `reference`, which is
						// rho of the master -- 1200 cents away at rho = 0.50 and
						// 498 at rho = 0.75. MEASURED smallest separation over
						// the population: 498.0449 cents, which is 4980 times
						// the pinned tolerance. There is no reading that can
						// satisfy both.
						// -----------------------------------------------------
						const double centsVsMaster = centsError(measured, masterHz);
						const double separationCents = std::fabs(centsError(reference, masterHz));
						CAPTURE(centsVsMaster);
						CAPTURE(separationCents);
						INFO("MASTER-LOCKED: the output period is the MASTER's. Remove the reset and this reading moves to `reference`, which the separation assertion pins hundreds of tolerances away");

						CHECK(separationCents > 100.0 * kSyncLockToleranceCents);
						bool lockedToMaster = true;
						if (!(std::fabs(centsVsMaster) < kSyncLockToleranceCents)) lockedToMaster = false;
						CHECK(lockedToMaster);
					}
				}
			}
		}

		// -------------------------------------------------------------------
		// THE TWO OUT-OF-WINDOW CONTROLS.
		//
		// Without these the rho window above is just a set of values that
		// happened to work. These drive rho OUTSIDE it, at both ends, and assert
		// what the instrument does there -- which is the difference between a
		// restriction stated on a physical criterion and one tuned to exclude
		// awkward numbers.
		//
		// BOTH CONTROLS FIRE. rho = 0.20 produces 28, 55 or 110 sync events per
		// block and the estimator STILL returns its negative sentinel: the
		// detector is working perfectly and the ruler simply cannot read the
		// result. That is the whole of this subcase's discipline in one row.
		// -------------------------------------------------------------------
		for (double sr : SAMPLE_RATES) {
			const int n = (int)std::lround(sr * kSyncLockWindowSeconds);

			for (size_t mi = 0; mi < SYNC_MASTER_N; ++mi) {
				const double masterHz = SYNC_MASTER_HZ[mi];

				for (int ci = 0; ci < 2; ++ci) {
					const double rho = (ci == 0) ? 0.20 : 1.60;
					const double volts = std::log2((masterHz * rho) / 261.6256);

					forge::VcoBlockDriver d(sr);
					forge::VcoInputs base = pitchBase();
					base.pitchCV   = (float)volts;
					base.morph     = 0.5f;
					base.character = 1.f;
					base.syncConnected = true;

					long fireCount = 0;
					std::vector<float> out;
					out.reserve((size_t)n);
					for (int i = 0; i < n; ++i) {
						forge::VcoInputs in = base;
						in.syncVolts  = (float)(5.0 * std::fmod((double)i * (masterHz / sr), 1.0));
						in.sampleTime = (float)(1.0 / sr);
						in.sampleRate = (float)sr;
						out.push_back(d.core.step(in));
						if (d.core.tel.syncFired) ++fireCount;
					}

					int nUp = 0;
					const double measured = estimateFreqRising(out, sr, &nUp);

					CAPTURE(sr);
					CAPTURE(masterHz);
					CAPTURE(rho);
					CAPTURE(volts);
					CAPTURE(measured);
					CAPTURE(nUp);
					CAPTURE(fireCount);

					// The detector fires on BOTH controls. Asserted first, so a
					// reader cannot mistake either row for a case where sync
					// stopped working.
					CHECK(fireCount > 0);

					if (ci == 0) {
						INFO("BELOW the lock window: the slave never reaches its own rising crossing between resets, so the estimator returns its NEGATIVE SENTINEL. Sync fired; the ruler cannot read. NO PITCH CLAIM IS MADE HERE, and that is the point of the row");
						// The estimator's own documented sentinel condition:
						// fewer than two crossings, hence a negative return.
						CHECK(nUp < 2);
						CHECK(measured < 0.0);
					} else {
						INFO("ABOVE the lock window: a second crossing appears inside each master period and the reading walks to TWICE the master rate - which is why the population above stops at rho = 1.00 rather than running on");
						REQUIRE(measured > 0.0);
						// MEASURED departures from an exact octave above the
						// master: 0.003 to 3.99 cents over the eighteen control
						// cells. Bounded loosely on purpose -- this row exists to
						// show the lock BREAKS in a stated direction, not to pin
						// a second tolerance.
						const double centsVsDouble = centsError(measured, 2.0 * masterHz);
						CAPTURE(centsVsDouble);
						CHECK(std::fabs(centsVsDouble) < 10.0);
					}
				}
			}
		}
	}
}
