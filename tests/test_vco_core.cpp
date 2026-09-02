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
//   2. |out| stays inside TWO NESTED MEASURED TIERS — a phase-wide 10.0 V outer
//      bound with no exceptions anywhere, and an additional 5.55 V musical
//      bound layered on top of it wherever the measurement entitles a scenario
//      to it. Both tiers are exercised rather than merely satisfied: the fixed
//      worst case is proven to exceed 5.1 V, and the Nyquist-ceiling scenario
//      is proven to exceed 5.65 V, which is above the musical tier itself
//      (D-18b / T-30-01 / T-32-03 / T-32-16). Plan 32-08 replaced the single
//      6.0 V loose bound; the derivation that bound rested on reasoned about
//      `morphedWave` alone, and Phase 32's corrections are additive and bipolar.
//   3. two instances differing ONLY in spread seed diverge measurably at
//      character = 1.0, with bit-identity at character = 0 pinned as the
//      in-test control (D-18a / D-10 / D-11)
//   4. two differently-seeded cores driven INTERLEAVED, sample by sample, each
//      reproduce their solo block BIT-EXACTLY — the behavioral form of CORE-03
//      (D-17), carrying all five measured non-vacuity preconditions. PLAN
//      33-04 ADDED A SIXTH: the two drives now carry DIFFERENT HARD-SYNC
//      MASTERS, which is what puts Phase 33's per-instance trigger and
//      previous-voltage store inside the window this invariant covers
//   5. the permanent POSITIVE CONTROL for invariant 4: a deliberately-broken
//      stand-in core that shares one static phase accumulator between instances
//      is required to FAIL the same check, through the same helper (D-17)
//   6. audio-rate MORPH sweeping through every segment boundary stays finite
//      and inside the OUTER tier across 27 configurations — three rates, three
//      notes, three modulation rates — with the boundary crossings proven to
//      happen BEFORE any value is asserted, and with the modulated excess over
//      the static-input envelope pinned and explained rather than hidden
//      (MORPH-01 / MORPH-02 / D-13 / D-16 / P-13). Appended by plan 32-09;
//      nothing above it was renumbered.
//   7. HARD SYNC — a master rising edge through the real forge::VcoInputs
//      boundary resets the phase to the fractional overshoot, the connected
//      flag gates it, the hysteresis band is OBSERVED rather than assumed, and
//      the reset is never exactly zero including on the exactly-on-threshold
//      case that reaches a fraction of one BY ARITHMETIC (SYNC-01 / D-01 /
//      D-03)
//   8. the detector's STRUCTURAL CEILING, with the limitation named before the
//      gate is written against it: one voltage per sample can carry at most one
//      rising transition, so every observable edge fires exactly once and the
//      missed set is identical at all three rates (SYNC-01 / D-09 / SC-3)
//   9. the new divisor cannot poison the phase accumulator — a hostile sync
//      population each entry of which is WITHDRAWN and the instance re-checked
//      afterwards, the previous-voltage store's invariant asserted directly on
//      every branch, and the sample-rate-change choice asserted rather than
//      inherited (D-12 / D-02)
//      Appended by plan 33-04; nothing above them was renumbered.
//  10. SC-3's OWN INSTRUMENT — the per-sample step across a reset, bounded by a
//      MEASURED envelope over a 420-cell sync sweep, pinned outward from that
//      measurement and NOT from a smallness claim: a legitimate reset at a
//      slave at or below its master's rate genuinely steps the output by
//      nearly its full range, and this case ASSERTS that it does. The
//      instrument is time-domain BY NECESSITY — single-sample full-amplitude
//      spikes were measured at 0.0 dB spectrally, so the alias-floor gate is
//      structurally blind to the artefact this criterion forbids (SC-3 / D-10)
//      Appended by plan 33-08; nothing above it was renumbered.
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
// THE OSCILLATOR IS BAND-LIMITED AS OF PLAN 32-06 — A THIRD SENTENCE CORRECTED
// IN PLACE (plan 32-08). This paragraph used to open "THE PHASE-30 OSCILLATOR
// ALIASES BY DESIGN. step() is a naive, deliberately unband-limited morphed
// oscillator (D-12)", and said Phase 32 "OWNS" band-limiting as future work.
// Both became FALSE the moment plan 32-06's call-site change landed:
// forge::VcoCore::step now returns 5 * (naive + correction) with a
// forge::MorphBlep held by value. Phase 32 has DELIVERED band-limiting. The
// same correction was made to src/dsp/VcoCore.hpp's own banner by that plan,
// for the same reason — a file whose self-description contradicts its body is
// the exact failure the house rule about falsified premises exists to prevent.
//
// WHAT SURVIVES UNCHANGED, and it is the operative half: NO assertion in this
// file may be written about alias content, harmonic structure or spectral
// cleanliness — not now, and not when the numbers here start looking
// respectable. That is not because the oscillator aliases; it is because this
// file is the wrong INSTRUMENT for the question. Spectral claims belong to
// tests/test_vco_spectrum.cpp, which gates the alias floor at C7, C8 and C9
// against per-shape thresholds measured from this repository's own output.
//
// Deliberately NOT here: harness plumbing (tests/test_vco_harness.cpp), the
// < 1-cent V/Oct tracking gate (Phase 31, TEST-02), the alias floor (Phase 32),
// and output conditioning plus the MOVING drift engine (Phase 34, OUT-01..03 /
// DRIFT-*). (Plan 30-03 wrote "also not here yet: the CORE-03 independence
// pair"; plan 30-04 landed it, as invariants 4 and 5 below, reusing 30-03's
// helpers unchanged and adding its own into the same anonymous namespace.)
//
// WHAT MAKES THIS SUITE VALIDATED RATHER THAN MERELY GREEN. Invariant 5 drives
// a deliberately-broken core — one static phase accumulator shared by every
// instance, the exact construct CORE-03 forbids — through the SAME helper
// invariant 4 uses, and requires it to FAIL. Invariant 3 pins the same kind of
// fact from the other direction, asserting bit-identity at the character value
// where the divergence mechanism is switched off. Both run on EVERY invocation,
// and both are observed detecting something rather than never having been
// anything but green. That is the posture of check_frozen.sh [3/3],
// check_includes.sh [6/7] and check_canary.sh [4/5], and it is the posture this
// file is written in. Do not remove them.
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "VcoBlockDriver.hpp"

#include <vector>
#include <functional>   // std::function — the interleave helper's seeder/input parameters
#include <cmath>
#include <cstdint>
#include <limits>       // std::numeric_limits<float>::quiet_NaN() — scenario four's hostile timing grid
#include <string>       // std::string — plan 33-08's cell labels; doctest prints a bare const char* as a POINTER

namespace {

// The three production sample rates every invariant is parametrized over.
// Plan 30-04 appends its CORE-03 helpers — the deliberately-broken shared-state
// stand-in and the interleave runner — into this SAME anonymous namespace, and
// owns none of the three helpers defined here.
constexpr double SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0};

// THE TWO NESTED OUTPUT TIERS, pinned by plan 32-08 and HOISTED HERE by plan
// 32-09. Their full provenance — the analytic derivation of 5.55, the operator
// decision of 2026-08-01 behind 10.0, and every measured figure that entitles a
// scenario to the tighter one — lives in invariant 2's banner below and is NOT
// duplicated here. Read it there.
//
// WHY THEY ARE NAMESPACE-SCOPE RATHER THAN LOCAL TO INVARIANT 2. Plan 32-08
// recorded kHostileBoundV as "the PHASE-WIDE OUTER output bound, no exceptions
// anywhere, BINDING ON EVERY SCENARIO ANY LATER PLAN ADDS". Plan 32-09 is the
// first such later plan, and invariant 6 below is bound by exactly that
// sentence. Left function-local, invariant 6 would have had to RE-DECLARE both
// numbers, and this suite already carries one mirror it has to keep in step by
// hand (DeliberatelyBrokenSharedStateCore) and has watched it drift. A bound
// that binds two cases gets ONE definition. Do not copy these values into a
// case body.
constexpr float kHostileBoundV = 10.0f;
constexpr float kMusicalBoundV = 5.55f;

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

// ===========================================================================
// CORE-03 support (plan 30-04). Everything below this line belongs to
// invariants 4 and 5; the three helpers above are plan 30-03's and are not
// redefined, wrapped or shadowed here.
// ===========================================================================

// What runInterleaveCheck() hands back. The two mismatch counts are the
// property itself; soloEqual is the NON-VACUITY precondition (two identical
// signals would satisfy "interleaved == solo" for free, so the caller has to
// prove the two solo blocks are distinguishable before the result means
// anything); soloA is carried out so the caller can pin the helper against
// forge::VcoBlockDriver::run() over the same seeds and inputs.
struct InterleaveResult {
	int mismatchA = 0;          // interleaved A samples that differ from solo A
	int mismatchB = 0;          // interleaved B samples that differ from solo B
	int soloEqual = 0;          // positions where soloA[i] == soloB[i]
	std::vector<float> soloA;   // instance A's solo block, for the validity check
};

// The shared solo-versus-interleaved drive loop behind BOTH invariant 4 and
// invariant 5.
//
// WHY IT IS A TEMPLATE, and why it must stay one. Invariant 5's deliberately
// broken stand-in has to run through BYTE-IDENTICALLY the same drive loop as
// the real core, because a control that exercises different code than the check
// proves nothing about the check. That argument is already written into
// tests/check_includes.sh [6/7]'s banner — every negative control there calls
// the SAME function its section calls — and it applies here unchanged. Do not
// fork this into two near-copies.
//
// Parameters: `seedInstance(core, which)` seeds a freshly default-constructed
// core as instance 0 or instance 1; `sr` and `n` are the rate and block length;
// `inA` / `inB` supply each instance's per-sample inputs. The two instances are
// deliberately given DIFFERENT input functors — see the case below.
//
// TIMING IS OWNED HERE, exactly as the harness owns it. sampleTime and
// sampleRate are ALWAYS overwritten, for both instances, on every sample. This
// overwrite is load-bearing — it must never become conditional on what the
// caller's functor happened to put there. (Same wording, and the same reason,
// as tests/VcoBlockDriver.hpp:49-52; the case below asserts the two loops agree
// bit-for-bit precisely so this cannot silently drift.)
//
// Comparison is a direct float !=, NEVER doctest::Approx: Approx's epsilon(0)
// still applies a relative-scaling margin and is not a true bit-exact
// comparator. Independence is a bit-exactness claim or it is nothing.
template <typename CoreT>
InterleaveResult runInterleaveCheck(
		const std::function<void(CoreT&, int)>& seedInstance,
		double sr, int n,
		const std::function<forge::VcoInputs(int)>& inA,
		const std::function<forge::VcoInputs(int)>& inB) {
	const float dt = (float)(1.0 / sr);
	const float srf = (float)sr;

	InterleaveResult r;
	std::vector<float> soloB;
	r.soloA.reserve((size_t)n);
	soloB.reserve((size_t)n);

	// --- Solo baselines: each instance alone, in its own fresh core. --------
	{
		CoreT a;
		seedInstance(a, 0);
		for (int i = 0; i < n; ++i) {
			forge::VcoInputs in = inA(i);
			in.sampleTime = dt;
			in.sampleRate = srf;
			r.soloA.push_back(a.step(in));
		}
	}
	{
		CoreT b;
		seedInstance(b, 1);
		for (int i = 0; i < n; ++i) {
			forge::VcoInputs in = inB(i);
			in.sampleTime = dt;
			in.sampleRate = srf;
			soloB.push_back(b.step(in));
		}
	}

	// --- The interleaved run: two MORE fresh cores, alternating one sample of
	//     A with one sample of B. This is the arrangement v2.1 polyphony will
	//     actually use — sixteen channels stepped inside one process() call —
	//     scaled down to the two instances that can already be built today.
	{
		CoreT ia, ib;
		seedInstance(ia, 0);
		seedInstance(ib, 1);
		for (int i = 0; i < n; ++i) {
			forge::VcoInputs a = inA(i);
			a.sampleTime = dt;
			a.sampleRate = srf;
			if (ia.step(a) != r.soloA[i]) ++r.mismatchA;

			forge::VcoInputs b = inB(i);
			b.sampleTime = dt;
			b.sampleRate = srf;
			if (ib.step(b) != soloB[i]) ++r.mismatchB;
		}
	}

	for (int i = 0; i < n; ++i) {
		if (r.soloA[i] == soloB[i]) ++r.soloEqual;
	}
	return r;
}

// ---------------------------------------------------------------------------
// DeliberatelyBrokenSharedStateCore — the PERMANENT positive control for
// invariant 4. Read this banner before touching anything below it.
//
// THIS TYPE IS A TEST CONTROL. IT IS NOT PRODUCTION CODE, IT IS NOT A WORK IN
// PROGRESS, AND IT MUST NEVER BE MOVED UNDER src/ — least of all into
// src/dsp/VcoCore.hpp, which is the file it is a deliberately-broken copy of.
// It implements EXACTLY the construct CORE-03 forbids: a phase accumulator held
// in a function-local static and therefore shared by every instance in the
// process. Landing that in the shipped core would silently destroy v2.1
// polyphony — sixteen voices would fight over one accumulator — and the guard
// that would catch it is the very case this type exists to validate.
//
// WHY IT EXISTS. A check that has only ever been observed green is unvalidated:
// it is indistinguishable from a check that cannot fail. That is this
// repository's standing posture, written into check_frozen.sh [3/3],
// check_includes.sh [6/7] and check_canary.sh [4/5], each of which validates a
// detector by running it over a synthetic fixture that MUST produce a hit. This
// type is invariant 4's fixture. Invariant 5 drives it through the SAME
// runInterleaveCheck() the real core goes through and REQUIRES it to fail the
// independence property, on every single invocation of the suite.
//
// DO NOT delete it, do not disable it, do not convert invariant 5 into a
// skipped or commented-out case, and do not "clean this up" because a
// deliberately broken oscillator looks like dead code. If it goes away, or if
// invariant 5 ever passes by NOT detecting the defect, invariant 4 stops being
// evidence of anything and CORE-03 reverts to an assertion nobody has tested.
//
// It is contained by PLACEMENT: an anonymous namespace inside this test TU. It
// has internal linkage, it is in no header, and it is in no shipped build
// graph, so check_includes.sh, check_canary.sh and the strict C++11 gate never
// see it — all three scan src/ only. Containment is asserted rather than
// assumed: plan 30-04's acceptance criteria require `grep -r
// 'DeliberatelyBrokenSharedStateCore' src/` to find nothing.
//
// THE DEFECT IS ISOLATED TO ONE FIELD, on purpose. Everything else mirrors
// src/dsp/VcoCore.hpp: a PER-INSTANCE forge::Waveshape and a PER-INSTANCE
// forge::MorphBlep, a per-instance seeding entry point performing the same D-11
// five-coefficient copy, the same exp2_taylor5 pitch off kVcoFreqC4, the same
// NaN-safe zero test and Nyquist clamp, the same single-subtract wrap, the same
// NaN-catching morph/character pair, the same one frozen morphedWave call plus
// an additive band-limiting correction, the same x5 unconditioned output. That
// isolation is what makes the control SPECIFIC: it demonstrates the helper
// catches SHARED STATE, not merely that two different classes produce two
// different streams of numbers.
//
// THE GUARD SEQUENCE IS MIRRORED DELIBERATELY AND MUST BE KEPT IN STEP WITH THE
// REAL CORE (plan 30-08). When src/dsp/VcoCore.hpp's frequency clamps were
// reordered (ceiling first, NaN-safe floor last) and a direct bound on the phase
// increment was added, the same two changes were made below. If they had not
// been, this control would differ from the shipped core in TWO things rather
// than the one field its banner promises, and the banner sentence above would
// have become exactly the class of false comment plan 30-08 exists to remove.
// The mirror is behaviorally INERT at this control's own inputs — pitchCV in
// [-1, +1] and 0.5 with sampleTime = 1/sampleRate gives an increment of roughly
// 0.006 to 0.012, so neither guard can fire — and invariant 5's captured
// mismatch figures (512, 512, total 1024) are unchanged by it, proven by plan
// 30-08 Task 3's before/after diff rather than asserted by eye.
//
// BROUGHT IN STEP AGAIN BY PLAN 31-07, IN OBEDIENCE TO THE RULE THE PARAGRAPH
// ABOVE STATES. Phase 31 replaced the real core's single-term pitch expression
// with a four-term VOLT-DOMAIN summation through one exponential, plus an
// fmConnected-gated FM contribution, plus a hostile-input bound on the summed
// volts ahead of the exponential, plus a sanitised sample rate ahead of the
// Nyquist ceiling. All of it is mirrored below. Had it not been, this type would
// again differ from the real core in more than the one field its banner
// promises, and — worse than merely being wrong — it would have gone on PASSING,
// because its own inputs never reach the new arithmetic. A stand-in drifting
// from what it claims to mirror is invisible precisely when the drift is inert.
//
// THE ADDITION IS BEHAVIORALLY INERT AT THIS CONTROL'S OWN INPUTS, and that is
// why the figures below did not move. Invariant 5 drives this type through
// coreBase(), which leaves coarse and fine at zero and leaves all three FM
// fields at their header defaults with the jack UNPATCHED, over pitchCV in
// [-1, +1] and 0.5. So the summation reduces to the pitch volt exactly, the
// gated FM term is NOT EVALUATED AT ALL, the pitch-volt bound cannot fire two
// orders of magnitude inside its own range, and the sanitising ternary returns a
// legitimate positive rate unchanged. Not one sample can move.
//
// RE-OBSERVED AFTER THE PLAN-31-07 UPDATE, AND UNCHANGED: 512 / 512 / total
// 1024 at every one of the three rates — identical to the pre-edit capture, and
// identical to the figures plan 30-08 recorded. The whole-suite case and
// assertion counts are unchanged too. THE FIGURES ARE COMPARED AS NUMBERS,
// NEVER BY DIFFING THE RAW success output: every successful-assertion line
// carries its own source line number, so a raw diff of a before and after
// capture is guaranteed to differ for reasons that mean nothing.
//
// BROUGHT IN STEP A THIRD TIME BY PLAN 32-08, FOR THE SAME REASON. Phase 32
// band-limited the real core (plan 32-06): forge::VcoCore gained a
// forge::MorphBlep member held by value, its single frozen call became a named
// `naive` local plus a SEPARATE ADDITIVE correction term, and its morph and
// character conditioning moved off the comparison-ladder forge::clamp onto the
// NaN-catching negated pair. All three are mirrored below. The `blep` member is
// PER-INSTANCE here as well as there, and the paragraph beside its declaration
// says why a deliberately-broken control must not be given a second shared
// thing.
//
// RE-OBSERVED AFTER THE PLAN-32-08 UPDATE, AND ALSO UNCHANGED: 512 / 512 /
// total 1024 at all three rates, unmoved from every figure above. BUT THE
// REASON IS NOT THE ONE THE TWO PARAGRAPHS ABOVE GIVE, AND WRITING "unchanged,
// therefore inert" HERE WOULD BE FALSE. Measured out-of-tree, on this control's
// own two drives, by running its step body with and against the correction term
// and comparing bit-exactly:
//
//     drive A (morph 0.25, pitchCV swept -1..+1, character 1.0)
//         44.1 kHz:   0 of 512 samples changed, max |delta| 0.000000 V
//         48   kHz:   0 of 512 samples changed, max |delta| 0.000000 V
//         96   kHz:   0 of 512 samples changed, max |delta| 0.000000 V
//     drive B (pitchCV 0.5 fixed, morph swept 0..1, character 1.0)
//         44.1 kHz:   8 of 512 samples changed, max |delta| 2.224924 V
//         48   kHz:   8 of 512 samples changed, max |delta| 2.078773 V
//         96   kHz:   4 of 512 samples changed, max |delta| 2.344383 V
//
// So the addition is INERT ON DRIVE A AND LIVE ON DRIVE B. Drive A is inert for
// a real and checkable reason — the D-03 compact-support factor. At character
// 1.0 the triangle's rounded corner is about 0.175 wide in phase units, while
// pitchCV in [-1, +1] puts the phase increment at roughly 0.003 to 0.012, so
// 2*dt is one to two orders of magnitude NARROWER than the softened edge and
// the factor returns EXACTLY zero. Drive B sweeps morph across the saw, square
// and pulse hard steps at a fixed 369.99 Hz, and there the correction fires:
// 512 samples is 4.30 / 3.95 / 1.97 cycles at the three rates, giving 4 / 4 / 2
// wrap edges at 2 samples per edge (D-13 places the second half of every
// correction on the FOLLOWING sample) — 8 / 8 / 4, which is exactly the
// measured count. That arithmetic agreeing independently is a second reading
// that the placement is right and the counts are not an accident.
//
// WHY THE MISMATCH FIGURES DID NOT MOVE ANYWAY, AND WHAT THAT COSTS. They are
// SATURATED. mismatchA and mismatchB count, out of n = 512, the interleaved
// samples that differ from the solo baseline, and they are already at 512 —
// every single sample. A count pinned at the ceiling of its own metric cannot
// rise when a second divergence route is added, so "unchanged" here is
// INSENSITIVITY rather than evidence of inertness. That is a real limitation of
// this fixture and it is recorded rather than glossed: these three figures can
// only ever detect a change that pushes the count DOWN. They are not a fine
// pin. What invariant 5 actually asserts — totalMismatch > 0, the helper can
// see shared state at all — is unaffected and holds with 1024 of a possible
// 1024.
//
// IF ANY OF THOSE FIGURES EVER MOVES WHEN THIS MIRROR IS UPDATED, STOP AND
// REPORT IT RATHER THAN UPDATING THE NUMBER. A moved figure means the addition
// was NOT inert — it means the change altered behavior this control was pinning,
// which is a finding about the real core and not a bookkeeping update here.
//
// BROUGHT IN STEP A FOURTH TIME BY PLAN 33-04, AND THE PRECEDENT IS WHAT
// DECIDED IT. Phase 33 gave the real core a hard-sync block: a
// forge::SchmittTrigger, a previous-voltage store, a guarded sub-sample solve
// and a fractional-overshoot reset, with the morph/character conditioning MOVED
// above it and the `p` snapshot left below it (src/dsp/VcoCore.hpp:850-892 and
// 922). All of it is mirrored below, INCLUDING the reordering.
//
// IT WOULD HAVE BEEN CHEAPER NOT TO. Invariant 5's drives leave the jack
// UNPATCHED, so the real core's sync branch is not entered on this control's
// own inputs either, and a mirror that simply omitted the block would have been
// behaviourally exact today. That is precisely the argument the plan-31-07
// paragraph above REJECTS: the FM addition was inert on these drives too and
// was mirrored anyway, because "a stand-in drifting from what it claims to
// mirror is invisible precisely when the drift is inert". A later plan that
// gives invariant 5 sync voltages — and plan 33-04 came within one decision of
// being that plan — would otherwise find a control that silently stopped being
// a copy of the thing it is a copy of.
//
// ONE LINE OF THE REAL SYNC BLOCK IS DELIBERATELY NOT MIRRORED, AND IT IS NAMED
// RATHER THAN LEFT TO BE NOTICED: the real core's extra
// `wave.morphedWave((float)phase, ...)` call, which captures the PRE-reset
// value for `tel.syncJump`. forge::Waveshape::morphedWave is `const`
// (src/dsp/Waveshape.hpp:158) and its result feeds telemetry only, so the call
// cannot move a sample; this type has no Telemetry, so mirroring it would add a
// frozen call whose result is discarded. The behavioural mirror is complete.
//
// RE-OBSERVED AFTER THE PLAN-33-04 UPDATE, AND UNCHANGED AGAIN: 512 / 512 /
// total 1024 at every one of the three rates — the fourth consecutive capture
// at those figures. The addition is genuinely inert here, for a checkable
// reason rather than a hopeful one: invariant 5 drives coreBase(), which leaves
// `syncConnected` at its header default of FALSE, so the branch is not merely
// unfired, it is NOT EVALUATED, and the only sync line that runs at all is the
// unconditional store — which nothing downstream reads.
// ---------------------------------------------------------------------------
struct DeliberatelyBrokenSharedStateCore {
	// Per-instance, exactly as the real core holds them. Only `sharedPhase`
	// inside step(...) below is broken.
	forge::DriftEngine drift;
	forge::Waveshape wave;
	// PER-INSTANCE ON PURPOSE, EVEN IN A DELIBERATELY-BROKEN CONTROL (plan
	// 32-08). The real core holds forge::MorphBlep by value beside its
	// Waveshape (src/dsp/VcoCore.hpp:278), and this mirror does the same. It is
	// tempting to reason "this type is the broken one, so share this too" —
	// that reasoning is wrong. The defect this control demonstrates is A SHARED
	// PHASE ACCUMULATOR, singular. Giving it a SECOND shared thing would blur
	// what invariant 5 proves: a failure would no longer isolate to the
	// accumulator, and the control would stop being the specific fixture its
	// banner claims. One defect, named and marked, and nothing else.
	forge::MorphBlep blep;
	// PER-INSTANCE FOR THE SAME REASON, AND THE REASON IS NOW LOAD-BEARING IN
	// BOTH DIRECTIONS (plan 33-04). These two mirror forge::VcoCore's own
	// per-instance sync state (src/dsp/VcoCore.hpp:330-331). Making either of
	// them a shared static here would give this control a SECOND defect and
	// blur what invariant 5 isolates — and it would also destroy the only
	// negative control invariant 4's new sync coverage has, because a shared
	// sync store is exactly the defect invariant 4 now exists to detect.
	forge::SchmittTrigger syncTrig;
	float prevSyncVolts = 0.f;

	void seed(uint64_t s0, uint64_t s1 = 0) { drift.seed(s0, s1); }

	// The D-11 five-coefficient copy, mirroring forge::VcoCore::setSpreadSeed
	// field for field so the two cores can share one seeding callable.
	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) {
		drift.setSpreadSeed(s0, s1);
		wave.triAsymmetrySpread = drift.triAsymmetrySpread;
		wave.sawCurvatureSpread = drift.sawCurvatureSpread;
		wave.squareDutySpread   = drift.squareDutySpread;
		wave.pulseEdgeSpread    = drift.pulseEdgeSpread;
		wave.bleedSpread        = drift.bleedSpread;
	}

	// Signature matches forge::VcoCore::step(...) so runInterleaveCheck accepts
	// this type with no change whatsoever to the helper.
	float step(const forge::VcoInputs& in) {
		// >>> THE DELIBERATE DEFECT, AND THE ONLY ONE. <<<
		// A function-local static: ONE accumulator for every instance of this
		// type in the process. This is the whole point of the control. Do not
		// make it a member — that would "fix" the control and quietly turn
		// invariant 5 green, which is the failure mode the banner above warns
		// about.
		static double sharedPhase = 0.0;

		// PITCH BLOCK MIRRORED FROM src/dsp/VcoCore.hpp (Phase 31 / plan 31-07).
		// The volt-domain summation of V/OCT, coarse and the divided fine value,
		// with the semitone-to-octave division owned by the core (D-05), then the
		// fmConnected-GATED FM contribution added into those same volts — not
		// multiplied onto a resolved frequency (D-01/FM-03).
		float pitchVolts = in.pitchCV + in.coarse + in.fine * (1.f / 12.f);
		if (in.fmConnected) pitchVolts += in.fmVolts * in.fmAtten;

		// Mirrored: the D-14 undefined-behavior bound on the summed volts, with
		// the NEGATED comparison FIRST as the NaN catcher, against the same
		// forge:: constant. Never the comparison-ladder helper — it is
		// transparent to a not-a-number.
		if (!(pitchVolts > -forge::kVcoMaxPitchVolts)) pitchVolts = -forge::kVcoMaxPitchVolts;
		if (pitchVolts > forge::kVcoMaxPitchVolts) pitchVolts = forge::kVcoMaxPitchVolts;

		// Mirrored: EXACTLY ONE exponential, off the C4 reference.
		float freq = forge::kVcoFreqC4 * forge::exp2_taylor5(pitchVolts);

		// Mirrored: the rate is sanitised BEFORE it is scaled (WR-06).
		const float safeRate = (in.sampleRate > 0.f) ? in.sampleRate : 0.f;
		const float maxFreq = forge::kVcoNyquistGuardFrac * safeRate;
		// GUARD SEQUENCE MIRRORED FROM src/dsp/VcoCore.hpp — ceiling first, then
		// the NaN-safe floor as the last writer, then the direct bound on the
		// increment. Kept in step with the real core deliberately (plans 30-08
		// and 31-07); see the banner above.
		if (freq > maxFreq) freq = maxFreq;
		if (!(freq > 0.f)) freq = 0.f;

		double deltaPhase = (double)freq * (double)in.sampleTime;
		if (!(deltaPhase > 0.0)) deltaPhase = 0.0;
		if (deltaPhase > forge::kVcoMaxDeltaPhase) deltaPhase = forge::kVcoMaxDeltaPhase;
		sharedPhase += deltaPhase;
		if (sharedPhase >= 1.0) sharedPhase -= 1.0;

		// MIRRORED FROM src/dsp/VcoCore.hpp:688-693 (plan 32-06, T-32-01). The
		// comparison-ladder forge::clamp that used to sit here is transparent to
		// a not-a-number, and the frozen forge::Waveshape::morphedWave casts
		// `morph * 4.f` to int. The real core replaced the ladder with this
		// NEGATED-comparison pair — negation FIRST as the NaN catcher — so this
		// mirror does too.
		//
		// THE PAIR MOVED UP HERE IN PLAN 33-04, MIRRORING THE MOVE PLAN 33-02
		// MADE IN THE REAL CORE, and the move is part of what is being mirrored
		// rather than a tidy-up. The real core's sync block calls the frozen
		// waveshaper AGAIN, above the `p` snapshot, so the conditioned values
		// have to exist by then; leaving the pair below would hand the sync path
		// raw fields. The pair itself is byte-unchanged — same order, same
		// wording, same comparisons.
		float morph = in.morph;
		if (!(morph > 0.f)) morph = 0.f;
		if (morph > 1.f) morph = 1.f;
		float character = in.character;
		if (!(character > 0.f)) character = 0.f;
		if (character > 1.f) character = 1.f;

		// THE HARD-SYNC BLOCK, MIRRORED FROM src/dsp/VcoCore.hpp:850-892 (plan
		// 33-02, mirrored here by plan 33-04). The connected gate first, the
		// same two inherited threshold literals, the same guarded sub-sample
		// solve with the negation FIRST and the upper bound STRICT, the same
		// fractional-overshoot reset, and the same UNCONDITIONAL store. The
		// trigger and the store are per-instance members of this type; see their
		// declarations above for why that is not a place to add a second defect.
		if (in.syncConnected && syncTrig.process(in.syncVolts, 0.1f, 1.0f)) {
			float f = (1.0f - prevSyncVolts) / (in.syncVolts - prevSyncVolts);
			if (!(f >= 0.f) || !(f < 1.f)) f = 0.f;
			sharedPhase = (double)(1.f - f) * deltaPhase;
		}
		prevSyncVolts = in.syncVolts;

		// STRICTLY BELOW THE SYNC BLOCK, exactly as in the real core (D-07): the
		// snapshot, the naive sample and the single band-limiter call all see
		// the POST-reset phase.
		const float p = (float)sharedPhase;

		// MIRRORED FROM src/dsp/VcoCore.hpp:608-645 (plan 32-06). The single
		// frozen call is now a NAMED local plus a SEPARATE ADDITIVE correction —
		// the frozen waveshaper is still called exactly once and is neither
		// edited nor bypassed. The band-limiter is handed the SAME float `p` the
		// frozen call gets, with the double accumulator travelling alongside it
		// as its own argument (P-3), which is exactly the shape the real core
		// uses.
		const float naive = wave.morphedWave(p, morph, character, 0.f);
		return 5.f * (naive + blep.step(wave, sharedPhase, p, deltaPhase, morph, character));
	}
};

// ===========================================================================
// HARD SYNC support (plan 33-04). Everything below this line belongs to the
// four "vco sync: ..." cases at the bottom of this file. Nothing above it is
// redefined, wrapped or shadowed — the same discipline plan 30-04 followed
// when it appended its CORE-03 helpers into this same anonymous namespace.
// ===========================================================================

// ---------------------------------------------------------------------------
// MasterBlock / makeMasterSaw — THE MASTER IS GENERATED IN THE TEST, never by a
// second forge::VcoCore. A second core would make every sync claim circular:
// the thing under test would be producing its own stimulus.
//
// WHAT IT PRODUCES, AND WHY THE POLARITY IS CHOSEN RATHER THAN INHERITED. A
// +/-amp FALLING saw, sampled once per slave sample. A falling ramp crosses the
// 0.1 V LOW threshold DOWNWARD in the middle of every master cycle, which
// RE-ARMS forge::SchmittTrigger, and then jumps UPWARD through the 1.0 V HIGH
// threshold at the wrap, which FIRES it. One arm and one fire per master cycle,
// out of one waveform, with no hand-built gate sequence anywhere — and it is
// the Forge saw's own polarity, so it is what an operator actually patches.
//
// WHY THE INCREMENT MUST BE DYADIC. `dtm` is master cycles per SAMPLE and every
// caller passes Km / 2^k. A dyadic increment is exactly representable, so
// `phim += dtm` carries NO accumulated rounding across the block, the block
// holds an EXACT integer number of master cycles, and the reset counts the
// cases below assert are exact expectations rather than tolerances. Same
// argument as 33-RESEARCH's spectral sub-grid master and as Phase 32's
// bin-centred frequencies; do not "simplify" a caller to a rounded Hz figure.
//
// AND IT KNOWS THE TRUE WRAP FRACTION, WHICH IS THE PART LATER PLANS NEED.
// `wrapG[j]` is g = (1 - phim[k-1]) / dtm — the fraction of the sample interval
// at which the master ACTUALLY crossed, taken from the generator's own state
// rather than inferred from the samples it emitted. THIS PLAN ASSERTS NOTHING
// AGAINST IT, and the first case below records the MEASURED reason why not: for
// a hard-edged master g and the detector's interpolated fraction are different
// quantities and do not track each other. Plan 33-05's placement grid needs an
// oracle for the true crossing instant, and this field is it.
// ---------------------------------------------------------------------------
struct MasterBlock {
	std::vector<float>  volts;         // the master voltage, one entry per slave sample
	std::vector<int>    wrapAt;        // sample indices on which the master wrapped
	std::vector<double> wrapG;         // the TRUE wrap fraction of the first wrap in that sample
	std::vector<long>   wrapsBySample; // cumulative wraps completed THROUGH each sample
	long                totalWraps = 0;
};

MasterBlock makeMasterSaw(int n, double dtm, double amp, double phi0) {
	MasterBlock m;
	m.volts.reserve((size_t)n);
	m.wrapsBySample.reserve((size_t)n);
	double phim = phi0;
	for (int i = 0; i < n; ++i) {
		const double before = phim;
		phim += dtm;
		if (phim >= 1.0) {
			// std::floor rather than a single subtract: `dtm` deliberately
			// EXCEEDS 1.0 in the structural-ceiling case, where a single
			// subtract would silently leave the accumulator above 1.
			const double k = std::floor(phim);
			m.wrapAt.push_back(i);
			m.wrapG.push_back((1.0 - before) / dtm);
			m.totalWraps += (long)k;
			phim -= k;
		}
		m.wrapsBySample.push_back(m.totalWraps);
		m.volts.push_back((float)(amp * (1.0 - 2.0 * phim)));
	}
	return m;
}

// makeMasterSawBandLimited — the same saw with the TWO-POINT polyBLEP residual
// applied at each wrap: +amp*(1-g)^2 on the sample BEFORE it and -amp*g^2 on
// the sample carrying it. Transcribed from 33-RESEARCH Pitfall 7's own worked
// expressions rather than re-derived, so the case that consumes it measures the
// research's construction and not a variant of it.
MasterBlock makeMasterSawBandLimited(int n, double dtm, double amp, double phi0) {
	MasterBlock m = makeMasterSaw(n, dtm, amp, phi0);
	for (size_t j = 0; j < m.wrapAt.size(); ++j) {
		const int    k = m.wrapAt[j];
		const double g = m.wrapG[j];
		if (k - 1 >= 0) m.volts[(size_t)(k - 1)] += (float)(amp * (1.0 - g) * (1.0 - g));
		m.volts[(size_t)k] -= (float)(amp * g * g);
	}
	return m;
}

// ---------------------------------------------------------------------------
// HOSTILE_SYNC — the sync-voltage population for the D-12 case, following
// scenario four's discipline exactly: a NAMED array, one entry per PHYSICAL
// case, and a trailing comment on every entry saying which one. An array whose
// entries a reader cannot map back to something that can actually happen on a
// patch cable is a list of numbers, not a threat model.
//
// EVERY ENTRY IS HELD CONSTANT ACROSS ITS BLOCK, which is not incidental — it
// is how the "equal consecutive samples" case (D-12's zero divisor) is
// delivered. Each entry is driven TWICE: once from sample 0, where the trigger
// initialises against the hostile value itself, and once after a 0 V arming
// prefix, where the trigger is LOW when the value arrives. Only the second
// state can reach the divisor at all, and the two together are what stop the
// grid from covering one branch and reporting both.
// ---------------------------------------------------------------------------
static const float HOSTILE_SYNC[] = {
	5.f,                                        // a +5 V gate ALREADY HIGH when the cable is patched — Pitfall 5's stale-store zero-divisor case, verbatim
	2.f,                                        // THE LEGITIMATE CONTROL EDGE: an ordinary +2 V gate. It MUST fire, or this grid passes by never detecting anything
	1.f,                                        // EXACTLY the 1.0 V high threshold — the raw quotient is exactly 1 and lands on the guard's STRICT upper bound
	0.1f,                                       // EXACTLY the 0.1 V low threshold — the arming edge of the hysteresis band, held rather than crossed
	std::numeric_limits<float>::quiet_NaN(),    // a mis-wired host, an uninitialised port read, or an upstream 0/0 — the ONLY entry that can poison `phase` PERMANENTLY
	std::numeric_limits<float>::infinity(),     // an upstream overflow on a cable: it DOES fire, with a divisor of +infinity, giving a fraction of exactly zero
	-std::numeric_limits<float>::infinity(),    // its sign partner: it cannot fire, but it IS stored, and inf/inf on the next real crossing is a not-a-number
	1e30f,                                      // finite and enormous: passes a naive std::isfinite check while dwarfing every threshold
	std::numeric_limits<float>::denorm_min(),   // an upstream underflow: below the low threshold, so it ARMS rather than fires
};

// ---------------------------------------------------------------------------
// SyncTrace / driveTraced — per-sample observation of the sync path THROUGH
// forge::VcoBlockDriver, with tests/VcoBlockDriver.hpp completely unchanged.
// That file's banner forbids templating, subclassing or aliasing it because it
// feeds the SHIPPED Analog LFO's bit-exact golden replay leg; this pair obtains
// everything these cases need without touching a byte of it.
//
// THE OFF-BY-ONE IS DELIBERATE AND IS THE WHOLE TRICK. The driver owns its loop
// and hands back only the samples, so there is no hook after each step(). But
// the input functor is invoked IMMEDIATELY BEFORE step(i), so the telemetry it
// can see is step(i-1)'s. driveTraced therefore records at the TOP of call i
// for every i > 0, and records the final sample once more after run() returns.
// The result is exactly n entries, entry i describing step i — asserted by the
// callers with a REQUIRE on the size rather than trusted.
//
// WHY THE PHASE ACCUMULATOR IS CARRIED HERE TOO. The reset expression is
// `phase = (double)(1.f - f) * deltaPhase`, and the only way to check it is to
// read `core.phase` on the sample the reset happened, in double. Reading the
// returned SAMPLE instead would re-derive the claim through the whole frozen
// waveshaper and the band-limiter, which is a different assertion.
// ---------------------------------------------------------------------------
// TWO FIELDS APPENDED BY PLAN 33-08, and nothing above them changed. `jump` and
// `correction` are the two Phase 33 telemetry members the SC-3 cases need and
// the four 33-04 cases do not read: the jump the seam was handed, and the value
// it actually deposited into the band-limiter's accumulator. They are recorded
// HERE rather than in a second recorder for the reason this file gives for
// every mirror it declines to create — a hand-kept copy is a thing to keep in
// step, and this file has watched one drift.
struct SyncTrace {
	std::vector<char>   fired;
	std::vector<float>  frac;
	std::vector<double> phase;
	std::vector<float>  freqHz;
	std::vector<float>  prevStore;   // forge::VcoCore::prevSyncVolts AFTER the step
	std::vector<float>  jump;        // forge::VcoCore::Telemetry::syncJump (plan 33-08)
	std::vector<float>  correction;  // forge::VcoCore::Telemetry::syncCorrection, PRE-multiply (plan 33-08)

	void record(const forge::VcoCore& c) {
		fired.push_back(c.tel.syncFired ? (char)1 : (char)0);
		frac.push_back(c.tel.syncFrac);
		phase.push_back(c.phase);
		freqHz.push_back(c.tel.freqHz);
		prevStore.push_back(c.prevSyncVolts);
		jump.push_back(c.tel.syncJump);
		correction.push_back(c.tel.syncCorrection);
	}
};

std::vector<float> driveTraced(forge::VcoBlockDriver& d, int n,
                               const std::function<forge::VcoInputs(int)>& inputAt,
                               SyncTrace& tr) {
	std::vector<float> out = d.run(n, [&](int i) {
		if (i > 0) tr.record(d.core);
		return inputAt(i);
	});
	tr.record(d.core);
	return out;
}

// expectedDeltaPhase — the phase increment the core is SUPPOSED to have used,
// recomputed here from the recorded frequency and the driver's own float
// sampleTime, through the same two guards and in the same order the header
// applies them (VcoCore.hpp:620-622). Stated independently rather than echoed:
// the point of the reset assertion is that the accumulator matches an increment
// the TEST derived, not one the header handed back.
double expectedDeltaPhase(float freqHz, float dt) {
	double dp = (double)freqHz * (double)dt;
	if (!(dp > 0.0)) dp = 0.0;
	if (dp > forge::kVcoMaxDeltaPhase) dp = forge::kVcoMaxDeltaPhase;
	return dp;
}

// ===========================================================================
// THE SC-3 SWEEP (plan 33-08). Everything below this line belongs to
// invariants 10 and 11 — the time-domain per-sample delta bound and its
// anti-circularity margin. Nothing above it is redefined or shadowed.
// ===========================================================================

// ---------------------------------------------------------------------------
// THE GRID'S AXES, DELIBERATELY THE SAME FIVE AS THE SPECTRAL SUB-GRID'S.
//
// tests/test_vco_spectrum.cpp's SYNC_GRID sweeps master/slave ratio x the five
// shape centres x character at both ends x both master edge shapes x three
// sample rates, and plans 33-05 and 33-07 measured every one of those axes for
// a reason that is written out at length there. The axes are REPRODUCED here
// rather than transferred, because the two files are separate translation units
// and this one may not include that one's helpers — but the VALUES are the same
// values, so a finding in one instrument can be looked up cell-for-cell in the
// other. That cross-reference is the whole point: 33-07's spectral gate and
// this file's time-domain gate are two instruments pointed at ONE grid.
//
// THE RATIO SET IS 33-05's, INCLUDING ITS THREE CORRECTIONS TO THE RESEARCH.
// 0.5 and 0.75 are the sub-unity region (a slave BELOW its master, where the
// reset truncates a cycle that has barely started); 1.0 is unity; 1.5, 2.5, 3.5
// and 5.5 are the classic sweep region. The integer ratios at and above two are
// deliberately ABSENT — 33-05 measured them to be near-no-ops where hard sync
// does almost nothing, and replaced them with non-integer neighbours. Do not
// "restore" them.
//
// WHY dtm IS 1/128 AND NOT A FREQUENCY. Same argument as makeMasterSaw's own
// banner: a dyadic master increment is exactly representable, so the block
// holds an EXACT integer number of master cycles at every sample rate and the
// reset counts below are exact expectations. The slave's pitch is solved from
// the ratio with std::log2 (libm, allowed in tests/ and forbidden in src/).
// ---------------------------------------------------------------------------
const double SYNC_D10_RATIOS[]     = {0.5, 0.75, 1.0, 1.5, 2.5, 3.5, 5.5};
const float  SYNC_D10_MORPHS[]     = {0.00f, 0.25f, 0.50f, 0.75f, 1.00f};
const char* const SYNC_D10_REGIONS[] = {"sine", "triangle", "saw", "square", "pulse 5%"};
const float  SYNC_D10_CHARS[]      = {0.00f, 1.00f};
const char* const SYNC_D10_EDGES[] = {"hard-edge", "band-limited"};

// The block length and master increment every SC-3 drive uses. 4096 samples at
// dtm = 1/128 is exactly 32 master cycles, so every cell fires 32 resets (31
// with a predecessor on a band-limited master, whose first wrap is moved by the
// residual applied to the sample BEFORE it).
const int    kSyncD10N   = 4096;
const double kSyncD10Dtm = 1.0 / 128.0;

// ---------------------------------------------------------------------------
// SyncResetObs / SyncDeltaCell / worstResetDeltaAt — ONE PASS, EVERY LEG.
//
// >>> THIS IS THE STRUCTURE THAT MAKES THE NAIVE-VERSUS-CORRECTED COMPARISON
//     LIKE FOR LIKE. <<< Both legs come from the SAME drive of the SAME core:
// there is no second forge::VcoCore, no NaiveVcoCoreMirror, no `bool bandLimit`
// flag in the shipped body and no second pass. What is recorded per reset is
// the shipped output at the reset and at its predecessor, plus the sync
// correction `tel.syncCorrection` deposited on each of those two samples.
//
// EVERY OTHER LEG IS THEN ARITHMETIC ON THOSE FOUR NUMBERS, and the licence to
// do that is a MEASURED property rather than an assumption. Under the past-edge
// placement plan 33-06 landed, the seam deposits into forge::MorphBlep::inject
// ONLY and leaves `pending` untouched, so the correction is confined to the
// sample that carries it and nothing is owed forward. src/dsp/VcoCore.hpp
// states the reconstruction as leg_none[n] = leg_full[n] - 5.f *
// syncCorrection[n] and carries plan 33-06's measurement of it: over a
// 49,152-sample patched block, 93 resets fired and EXACTLY 93 samples differed,
// and the reconstruction reproduced the withheld leg bit-exactly on 49,136 of
// 49,152 samples with the remaining 16 off by EXACTLY ONE ULP, worst absolute
// 4.77e-07 V.
//
// >>> SO NO EQUALITY IS WRITTEN AGAINST IT. <<< That error bar is why the cases
// below compare ENVELOPES against pinned volt-scale numbers and never assert a
// bit-exact identity on a reconstructed sample. A bit-exact assertion here
// would be red on sixteen samples in fifty thousand, on correct behaviour, and
// plan 33-06 wrote its measurement down precisely so that this plan would not
// write one.
//
// `worstResetDeltaAt(c, k)` returns the worst |x[n] - x[n-1]| over the cell's
// reset samples on the leg whose seam deposited k TIMES the correction it
// actually deposited. k = 1 is the shipped leg. k = 0 is the withheld leg.
// k = 0.25 and k = -1 are the two mutation probes of invariant 11.
// ---------------------------------------------------------------------------
struct SyncResetObs {
	float outPrev  = 0.f;   // the SHIPPED output on the sample before the reset, in volts
	float outNow   = 0.f;   // the SHIPPED output on the reset sample, in volts
	float corrPrev = 0.f;   // tel.syncCorrection on that predecessor, PRE-multiply
	float corrNow  = 0.f;   // tel.syncCorrection on the reset sample, PRE-multiply
	float jump     = 0.f;   // tel.syncJump on the reset sample, PRE-multiply
};

struct SyncDeltaCell {
	double      sr        = 0.0;
	const char* edgeName  = "";
	double      ratio     = 0.0;
	float       morph     = 0.f;
	const char* region    = "";
	float       character = 0.f;
	int         resets    = 0;     // resets fired anywhere in the block
	bool        allFinite = true;
	float       maxAbsOut = 0.f;   // worst |out| ANYWHERE in the block, not only on resets
	std::vector<SyncResetObs> obs; // one entry per reset that has a predecessor
};

double worstResetDeltaAt(const SyncDeltaCell& c, double k) {
	double worst = 0.0;
	for (size_t j = 0; j < c.obs.size(); ++j) {
		const SyncResetObs& o = c.obs[j];
		// The factor of five is forge::VcoCore::step's own output multiplier;
		// syncCorrection is recorded in the pre-multiply domain to match `naive`.
		const double now  = (double)o.outNow  + 5.0 * (k - 1.0) * (double)o.corrNow;
		const double prev = (double)o.outPrev + 5.0 * (k - 1.0) * (double)o.corrPrev;
		const double d    = std::fabs(now - prev);
		if (d > worst) worst = d;
	}
	return worst;
}

// ---------------------------------------------------------------------------
// sweepSyncDeltaGrid — the 420-cell drive both SC-3 cases run on.
//
// It is a FUNCTION rather than a lazily-built static, deliberately: invariants
// 10 and 11 each take their own pass, so neither can be made to pass by state
// the other left behind, and each case's `-tc=` selector really does exercise
// the whole sweep. MEASURED cost: 0.12 s per pass for 1,720,320 core steps, on
// a suite that already runs in six and a half seconds.
// ---------------------------------------------------------------------------
std::vector<SyncDeltaCell> sweepSyncDeltaGrid() {
	std::vector<SyncDeltaCell> grid;
	grid.reserve(420);

	const int    n   = kSyncD10N;
	const double dtm = kSyncD10Dtm;

	for (int ri = 0; ri < 3; ++ri) {
		const double sr       = SAMPLE_RATES[ri];
		const double masterHz = dtm * sr;
		for (int ei = 0; ei < 2; ++ei) {
			// The master is generated by this test, never by a second
			// forge::VcoCore — makeMasterSaw's banner says why.
			const MasterBlock m = (ei == 0) ? makeMasterSaw(n, dtm, 5.0, 0.0)
			                                : makeMasterSawBandLimited(n, dtm, 5.0, 0.0);
			for (int qi = 0; qi < 7; ++qi) {
				const double ratio   = SYNC_D10_RATIOS[qi];
				const float  pitchCV = (float)std::log2(ratio * masterHz / (double)forge::kVcoFreqC4);
				for (int mi = 0; mi < 5; ++mi) {
					for (int ci = 0; ci < 2; ++ci) {
						forge::VcoInputs base = coreBase();
						base.pitchCV   = pitchCV;
						base.morph     = SYNC_D10_MORPHS[mi];
						base.character = SYNC_D10_CHARS[ci];

						forge::VcoBlockDriver d(sr);
						SyncTrace tr;
						std::vector<float> out = driveTraced(d, n, [&](int i) {
							forge::VcoInputs in = base;
							in.syncVolts     = m.volts[(size_t)i];
							in.syncConnected = true;
							return in;
						}, tr);

						SyncDeltaCell c;
						c.sr        = sr;
						c.edgeName  = SYNC_D10_EDGES[ei];
						c.ratio     = ratio;
						c.morph     = SYNC_D10_MORPHS[mi];
						c.region    = SYNC_D10_REGIONS[mi];
						c.character = SYNC_D10_CHARS[ci];
						c.obs.reserve(32);

						for (int i = 0; i < n; ++i) {
							if (!std::isfinite(out[(size_t)i])) c.allFinite = false;
							const float a = std::fabs(out[(size_t)i]);
							if (a > c.maxAbsOut) c.maxAbsOut = a;
							// >>> RESET SAMPLES ARE IDENTIFIED FROM THE TELEMETRY
							//     FLAG, NEVER INFERRED FROM THE WAVEFORM. <<< A
							//     waveform-side heuristic ("a big step means a
							//     reset") would be circular in a case whose whole
							//     subject is how big the step at a reset is.
							if (!tr.fired[(size_t)i]) continue;
							++c.resets;
							if (i == 0) continue;   // no predecessor to difference against
							SyncResetObs o;
							o.outPrev  = out[(size_t)(i - 1)];
							o.outNow   = out[(size_t)i];
							o.corrPrev = tr.correction[(size_t)(i - 1)];
							o.corrNow  = tr.correction[(size_t)i];
							o.jump     = tr.jump[(size_t)i];
							c.obs.push_back(o);
						}
						grid.push_back(c);
					}
				}
			}
		}
	}
	return grid;
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
	// at 44.1 kHz).
	//
	// A FALSIFIED PREMISE, CORRECTED HERE BY MEASUREMENT (plan 32-08). The
	// sentence above used to END by calling that "sampling loss in a
	// deliberately unband-limited oscillator, not a pitch defect — PHASE 32 OWNS
	// IT." Phase 32 has now arrived, the core IS band-limited as of plan 32-06,
	// and the forward reference was resolved by RE-RUNNING the two recorded
	// points rather than by assuming either outcome. THE PREMISE IS FALSE. The
	// CONCLUSION is untouched and is the part worth keeping: the grid still
	// stops at +2.
	//
	// RE-MEASURED against the band-limited core, morph 1.00 / character 0.0 /
	// 44.1 kHz, over this same 250 ms window, through the same
	// forge::VcoBlockDriver and the same estimateFreqRising:
	//
	//     pitchCV   5 % region    pre-Phase-32      NOW (band-limited)
	//      +2.00    2.11 samples       —            +0.0010 %
	//      +3.00    1.05 samples       —            +0.0031 %
	//      +3.50    0.74 samples    -24.53 %        -34.7383 %
	//      +4.00    0.53 samples    -46.89 %        NO CROSSINGS AT ALL
	//
	// Band-limiting did not fix it. It made the MEASUREMENT WORSE, which is the
	// clearest possible evidence for the corrected premise: at +4.0 the output
	// now never reaches zero at all — MEASURED max -0.426132 V, with 0 of 11025
	// samples at or above zero — so estimateFreqRising counts nUp = 0 and
	// returns its negative sentinel. The REQUIRE(nUp >= 8) above would fire
	// before the tolerance is ever consulted. At +3.5 the counter resolves only
	// 482 of the 740 true cycles, which is the whole of the -34.74 % figure.
	//
	// THE CORRECTED PREMISE. estimateFreqRising counts RISING ZERO CROSSINGS.
	// At morph 1.00 the 5 %-duty pulse's positive region falls under about two
	// samples above pitchCV +2, and the sampler steps over it. Band-limiting
	// reduces the ALIAS ENERGY that narrow region radiates; it does not and
	// CANNOT restore a zero-crossing counter's ability to resolve a
	// sub-two-sample feature, because that is a property of the ESTIMATOR'S
	// SAMPLING, not of the waveform's spectrum. Softening the pulse's edges in
	// fact lowers its peak further inside that window, which is exactly why the
	// figures moved the wrong way. So this was never Phase 32's to fix, and the
	// sentence claiming it was is corrected here.
	//
	// WHERE HIGH-NOTE BEHAVIOUR IS ACTUALLY ASSERTED NOW: tests/test_vco_spectrum.cpp
	// gates the alias floor at C7, C8 and C9 — 2099, 4188 and 8367 Hz, ABOVE the
	// pitch range this grid stops at — and it does so SPECTRALLY, against
	// per-shape thresholds measured from this repository's own output, rather
	// than by counting crossings. That suite is the right instrument for the
	// question; this one is not, and that is a statement about instruments and
	// not about the DSP.
	//
	// THE STANDING INSTRUCTION THIS FILE'S OTHER HISTORICAL FIGURES CARRY
	// APPLIES TO ALL SIX NUMBERS ABOVE: they are OBSERVATIONS OF A REPRODUCTION
	// RUN, not a current expectation to recompute when a constant moves. If a
	// later phase changes the pulse duty, the guard fraction or the estimator,
	// re-run the reproduction and record what it says — do not adjust these to
	// match a prediction.
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
// 2. Output magnitude stays inside TWO MEASURED TIERS (D-18b / T-30-01 /
//    T-32-03 / T-32-16). Plan 32-08 replaced the single 6.0 V loose bound with
//    kHostileBoundV and kMusicalBoundV. Read the nesting paragraph before
//    reading the assertions, because the two are NOT a partition.
//
//    WHAT THIS BOUND IS NOW FOR — it acquired a SECOND job this phase, and it
//    is the reason the case is worth more than it looks. It is the ONLY
//    assertion in the whole suite that can see two of Phase 32's three most
//    likely defects: placing the square's hard step at its SOFT edge's position,
//    and deriving the sub-sample SIDE decision from the double accumulator
//    rather than the float. Both measure 0.0 dB against the spectral gate in
//    tests/test_vco_spectrum.cpp — spectrally invisible, exactly zero
//    difference — while swinging the output envelope to about +/-9.78 V. The
//    alias-floor gate cannot see either one. This can.
//
//    ------------------------------------------------------------------------
//    THE TWO TIERS ARE NESTED, NOT PARTITIONED. THIS IS NOT A CARVE-OUT.
//    ------------------------------------------------------------------------
//    kHostileBoundV is the PHASE-WIDE OUTER BOUND. It applies to EVERY scenario
//    in this suite with NO exceptions anywhere — including the audio-rate MORPH
//    sweep plan 32-09 adds. There is exactly ONE outer bound and nothing is
//    excused from it.
//
//    THAT FORWARD REFERENCE HAS LANDED, and it is recorded here rather than
//    left dangling: invariant 6 at the bottom of this file is the audio-rate
//    MORPH sweep, it asserts kHostileBoundV at all 27 of its configurations, and
//    it is MEASURED at a grid-wide worst of 6.289864 V. It is also the first
//    scenario in the suite that declines the tighter tier, which is why it
//    carries its own assertion proving the excess is real — see its banner.
//
//    kMusicalBoundV is an ADDITIONAL, STRICTLY TIGHTER assertion layered ON TOP
//    of the outer one, asserted wherever the measurement entitles a scenario to
//    it. It is not permission to skip the outer bound — every scenario that
//    asserts the tighter number asserts the looser one as well, on the line
//    above it. Asserting the tighter number NOWHERE would throw away the
//    tightest thing this suite knows about the oscillator; asserting it
//    EVERYWHERE would be red on correct shipped behavior at the Nyquist
//    ceiling. Each scenario names its tier in its own INFO label together with
//    the measured figure that entitles it, so a reader never has to infer which
//    assertions apply to which drive.
//
//    ------------------------------------------------------------------------
//    kMusicalBoundV = 5.55 V — PROVENANCE
//    ------------------------------------------------------------------------
//    THE ANALYTIC PART, WHICH SURVIVES PHASE 32 UNCHANGED. For character
//    >= 0.001 the sine path is f(s) = 0.32s^3 + 0.06s^2 + 0.76s - 0.03, whose
//    derivative is strictly positive, so f is monotone on [-1,1] with range
//    [-1.05, +1.11]; triangle, saw, square and pulse are each bounded by 1; the
//    morph crossfade is a linear interpolation and cannot exceed the larger of
//    its two shapes; and the bleed step is a convex combination, which cannot
//    raise a maximum. Hence |morphedWave| <= 1.11, and 5 * 1.11 = 5.55 V.
//
//    WHY THAT NO LONGER COVERS THE OUTPUT ON ITS OWN — the part that CHANGED,
//    and the reason this whole derivation had to be redone. The reasoning above
//    bounds `morphedWave`. Since plan 32-06 the returned sample is
//    5 * (naive + correction), and the correction is ADDITIVE and BIPOLAR. A
//    bound on `morphedWave` alone therefore says NOTHING about `naive +
//    correction`: the two could add. The bound MUST be MEASURED, and every
//    number below is measured rather than argued.
//
//    THE MEASUREMENT, AND IT IS BETTER THAN EXPECTED. At every phase increment
//    in the musical range the corrected envelope stays INSIDE the pre-Phase-32
//    figure of 1.1047 (5.523 V) — the corrections consistently REDUCE the
//    excursion at edges rather than adding to it. Measured by plan 32-08 over
//    this case's own four scenarios, at all three sample rates, with the bound
//    temporarily raised to 100 V so nothing could fire:
//
//        scenario one   (sweep)                   5.438490 / 5.438490 / 5.438290
//        scenario two   (fixed worst, pitchCV 0)  5.518030 / 5.518030 / 5.518030
//        scenario three (hostile V/OCT +10, +14)  5.421220 / 5.421220 / 5.421220
//        scenario four  (hostile timing, 48 cfg)  5.000000 / 5.000000 / 5.000000
//
//    All four are inside 5.55 V, so all four assert the tighter tier. The
//    tightest margin is scenario two's, at 5.55 - 5.518030 = 0.032 V, and that
//    is stated plainly rather than hidden: this is a TIGHT bound. It is kept at
//    the ANALYTIC ceiling rather than nudged up to a comfortable round number
//    because 5.55 is a DERIVED figure — the naive path's true supremum — and a
//    measured envelope sitting just under a derived ceiling is exactly the
//    relationship worth pinning. 5.55 V is a REAL bound, not a round number.
//
//    ------------------------------------------------------------------------
//    kHostileBoundV = 10.0 V — PROVENANCE
//    ------------------------------------------------------------------------
//    THE OPERATOR DECISION OF 2026-08-01, by name: ONE outer number with NO
//    per-scenario exceptions anywhere in the suite, on D-09's own reasoning
//    that an exception invites a second exception. 10.0 V is comfortably inside
//    Rack's +/-12 V cable norm, so a bound this loose still cannot let anything
//    reach a downstream module that the norm does not already permit (T-32-03).
//
//    THE MEASURED TABLE FROM 32-RESEARCH P-10, reproduced as rows. max|out| per
//    phase increment, over morph x character grids of 400 x 40 points:
//
//        increment 0.0005  (~22 Hz)                            5.523 V
//        increment 0.0200  (~882 Hz)                           5.506 V
//        increment 0.0949  (~4185 Hz)                          5.523 V
//        increment 0.1897  (~8366 Hz)                          5.523 V
//        increment 0.2500  (~11025 Hz)                         5.523 V
//        increment 0.4000  (~17640 Hz)                         5.829 V
//        increment 0.4950  (21830 Hz, the guarded Nyquist ceiling)  9.198 V
//        increment 0.5000  (22050 Hz, kVcoMaxDeltaPhase)       6.150 V
//
//    A FALSIFIED PREMISE FROM THAT TABLE, CORRECTED HERE BY MEASUREMENT. P-10's
//    prose attributes the 9.198 V row to "the morph at the 5-percent pulse with
//    character NEAR ZERO". Measured against the SHIPPED forge::MorphBlep that
//    is WRONG IN BOTH COORDINATES, and the error is not small:
//      - at pitchCV +10 (which pins the increment at 0.495 at every rate) and
//        character 0.00, the envelope is EXACTLY 5.000000 V — the naive pulse
//        amplitude, with the correction contributing nothing that reaches the
//        peak. Low character is not the worst case; it is close to the BEST.
//      - the worst point on that increment is at character 1.00, not near zero,
//        and it measures 7.150197 V at 44.1 kHz and 7.150281 V at 48 and
//        96 kHz — 2.05 V BELOW the 9.198 V the table records.
//    The shipped implementation is therefore materially BETTER at the ceiling
//    than the prototype the table was measured on. The conclusion — that the
//    musical tier is exceeded at the guarded ceiling and an outer tier is
//    needed — is UNCHANGED and is what the table is kept for. The premise about
//    WHERE on the morph/character plane it is exceeded is falsified, and
//    scenario five below is built at the measured location rather than the
//    stated one.
//
//    THE WORST FIGURE MEASURED ANYWHERE by plan 32-08, over a 101 x 21
//    morph-by-character sweep at twelve pitch points at all three rates:
//    7.201301 V, at pitchCV +6.38 / 44.1 kHz (increment 0.494100) at morph 1.00
//    character 1.00. 10.0 V clears that by 2.80 V.
//
//    WHY THE HEADROOM EXISTS AT ALL, in one sentence: the tighter figure is
//    exceeded only where the guarded frequency is pinned at the ceiling and two
//    edges of the narrow pulse fall inside a single sample — the deliberate
//    D-07 overlapping-edge case at literally Nyquist, where the naive path is
//    already meaningless — and where an input is MODULATED across the block,
//    because the pending accumulator deliberately delivers the second half of a
//    correction computed with the PREVIOUS sample's weight vector (D-13).
//
//    THIS IS EXPLICITLY NOT A +/-5 V OUTPUT-RANGE ASSERTION. D-13 returns the
//    waveform UNCONDITIONED by decision — no DC blocker, no saturation, no
//    clamp — so a >5 V overshoot at high character is the expected behavior,
//    not a defect. Phase 34's OUT-01..03 owns output conditioning; writing a
//    +/-5 V assertion here would contradict D-13 and pin an output stage that
//    has not been designed yet.
// ---------------------------------------------------------------------------
TEST_CASE("vco core: output magnitude stays inside two measured tiers (D-18b)") {
	// kHostileBoundV — THE OUTER BOUND. Applies to EVERY scenario below, with no
	// exceptions, and to every scenario any later plan adds to this suite.
	// kMusicalBoundV — THE TIGHTER, ADDITIONAL BOUND. Asserted ON TOP OF the
	// outer one wherever the measurement in this case's banner entitles a
	// scenario to it — never instead of it.
	//
	// BOTH WERE HOISTED to the anonymous namespace at the top of this file by
	// plan 32-09, so that invariant 6 — the first scenario a later plan added
	// under the "binding on every scenario any later plan adds" clause — reads
	// the SAME definition rather than a hand-kept copy of these two numbers. The
	// values, the nesting rule and this provenance are unchanged; only the
	// declaration site moved.

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
			// OUTER tier first, then the tighter one layered on top. MEASURED
			// 5.438490 / 5.438490 / 5.438290 at 44.1 / 48 / 96 kHz — inside
			// 5.55 V, which is what entitles this scenario to the musical tier.
			CHECK(maxAbs <= kHostileBoundV);
			CHECK(maxAbs <= kMusicalBoundV);
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
			// MEASURED 5.518030 at all three rates — inside 5.55 V by 0.032 V,
			// the tightest margin in the case, so this scenario asserts the
			// musical tier and is what pins it.
			CHECK(maxAbs <= kHostileBoundV);
			CHECK(maxAbs <= kMusicalBoundV);
			// RE-DERIVED BY PLAN 32-08 AND STILL LOAD-BEARING. Re-measured
			// against the BAND-LIMITED core at 5.518030 V, so the assertion
			// still exceeds 5.1 V by 0.418 V and is still exercised rather than
			// merely satisfied. Do not soften or delete it: it is what stops the
			// musical tier from being a bound nothing approaches.
			CHECK(maxAbs > 5.1f);
		}

		// --- Scenario three: hostile V/OCT. ---------------------------------
		// What this guards, measured rather than imagined: with the Nyquist
		// clamp removed, pitchCV = +10 drove the phase accumulator to 1,014,986
		// and the output to -8,655,011 V — and EVERY SINGLE SAMPLE of that
		// catastrophe stayed std::isfinite. Finiteness therefore cannot see a
		// runaway accumulator; this magnitude bound is what sees a runaway caused
		// by PITCH. That is precisely why the two assertions sit side by side
		// here instead of one being deleted as redundant with the other, or with
		// the harness suite's finiteness case. Do not merge them.
		//
		// SCOPE, corrected by plan 30-08. The sentence above used to claim this
		// was "the only invariant in the suite that can" see a runaway
		// accumulator. That held for a runaway caused by PITCH and for nothing
		// else: this scenario varies pitchCV alone, and every drive in this file
		// routes through forge::VcoBlockDriver or runInterleaveCheck, both of
		// which unconditionally overwrite sampleTime and sampleRate — so a
		// runaway caused by TIMING could not reach step() from here at all.
		// Scenario four below is the other half of the pair: it calls step()
		// DIRECTLY with hostile timing, with no driver in the way. Read the two
		// together; neither one covers the other's runaway.
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
				// MEASURED 5.421220 at both pitches at all three rates. This
				// scenario reaches the guarded Nyquist ceiling but holds morph
				// at 0.0 — the SINE centre — so it never visits the narrow
				// pulse where the ceiling actually costs anything, and it is
				// comfortably entitled to the musical tier. Scenario five is
				// the drive that goes to the same ceiling at the morph that
				// does cost something.
				CHECK(maxAbs <= kHostileBoundV);
				CHECK(maxAbs <= kMusicalBoundV);
				CHECK(allFinite);
			}
		}
	}

	// --- Scenario four: hostile TIMING, driven straight into the core. ------
	// Plan 30-08. This scenario supplies its OWN sampleTime and sampleRate, so
	// it deliberately sits OUTSIDE the `for (double sr : SAMPLE_RATES)` loop
	// above and inherits none of its rates.
	//
	// THE COVERAGE GAP THIS CLOSES, and why it existed. Every other drive in
	// this file goes through forge::VcoBlockDriver or through
	// runInterleaveCheck, and BOTH overwrite in.sampleTime and in.sampleRate
	// unconditionally on every sample (tests/VcoBlockDriver.hpp:49-52 says so in
	// its own banner, and says the overwrite must never become conditional).
	// That is correct for them — it is what makes timing injected rather than
	// ambient — and it is exactly why hostile timing was the ONE input class
	// forge::VcoCore::step() had never been exposed to. So this scenario calls
	// core.step(in) DIRECTLY on a caller-built forge::VcoInputs. The bypass IS
	// the coverage. Do not "tidy" this onto the driver: routing it through
	// either helper silently deletes the entire scenario while leaving it green.
	//
	// WHAT WAS MEASURED HERE BEFORE THE FIX (CR-01, reproduced independently by
	// the code reviewer and the verifier at sampleRate = -44100, morph = 0.5,
	// 20000 steps): freqHz = -21609.00, phase = -9800.00, maxAbs = 1.476e38 V,
	// isfinite = 0. The old guard applied its zero-floor BEFORE its ceiling, so
	// a non-positive sampleRate made maxFreq negative and the ceiling wrote a
	// negative frequency straight over the value the floor had just sanitised.
	// This scenario was OBSERVED RED against that header before the fix landed.
	//
	// THE FOUR ASSERTIONS, and why none of them is redundant with its
	// neighbours:
	//
	//   freqNonNegative — the CR-01-SPECIFIC pin, and the only one of the four
	//     that a clamp-order defect can still trip once the increment bound in
	//     src/dsp/VcoCore.hpp exists. Without it the increment bound absorbs the
	//     negative step and the OUTPUT looks perfectly healthy while tel.freqHz
	//     hands Phase 35's display a negative frequency. Do not delete it as
	//     redundant; the revert-one-only probe P1 in plan 30-08 shows this
	//     assertion going red ALONE.
	//   phaseInRange — the WR-01 pin. The single-subtract wrap in step() is
	//     correct only for an increment inside [0, 1), and NOTHING in
	//     forge::VcoInputs couples sampleTime to sampleRate: the ceiling is
	//     computed from the rate while the increment is computed from the time.
	//     Probe P2 shows this one going red while freqNonNegative stays green.
	//   allFinite and the 6.0 V magnitude bound — the user-visible consequence,
	//     and the reason src/dsp/VcoCore.hpp calls its guard LOAD-BEARING in the
	//     first place. Note that a NaN sample fails allFinite but NOT the
	//     magnitude bound (every comparison against NaN is false), which is
	//     precisely why both are asserted.
	//   freqNyquistBounded — the WR-06 pin, and the assertion that makes the
	//     ceiling's OWN job observable rather than merely its side effects. The
	//     four assertions above check that nothing DOWNSTREAM of the ceiling
	//     breaks; none of them checks that the ceiling actually fired. It did
	//     not, for one input class: with a NaN sampleRate, maxFreq is NaN and
	//     `freq > maxFreq` is false for every freq, so freq passed through
	//     completely unclamped and tel.freqHz carried it. MEASURED at
	//     sampleRate = NaN, pitchCV = +10: tel.freqHz = 267904.625 Hz against a
	//     ceiling that should have bounded it. freqNonNegative could not see it
	//     (the unclamped value is positive, so it passes trivially) and the
	//     other three could not either, because the independent
	//     kVcoMaxDeltaPhase bound absorbs the oversized frequency and keeps the
	//     OUTPUT healthy. Phase 35 is the named future consumer of tel.freqHz
	//     for a display; an arbitrary non-Nyquist-relative number reaching it is
	//     the user-visible failure this pins.
	//
	// THE GRID WAS EXTENDED BY PLAN 32-09, FROM 48 CONFIGURATIONS TO 176.
	// 4 rates x 6 times x 2 pitches became 8 x 11 x 2. The five named assertions
	// below, the accumulate-don't-assert idiom and the DIRECT core.step(in) call
	// are all UNCHANGED; only the two input arrays grew. The assertion budget is
	// what the accumulation buys: 176 configurations x 20000 steps is 3.5 million
	// per-sample observations, and the case contributes SIX assertions per
	// configuration — 1056 in total, up from 288 — to a suite already carrying
	// more than 2.6 million. Asserting per sample instead would add roughly
	// twenty-one million.
	//
	// EVERY GUARD THIS GRID REACHES IS THE NEGATED-COMPARISON FORM, and it is
	// stated here rather than left to be re-derived at each site. In
	// src/dsp/VcoCore.hpp: the pitch-volt bound (`!(pitchVolts > -kVcoMaxPitchVolts)`),
	// the frequency floor (`!(freq > 0.f)`), the deltaPhase floor
	// (`!(deltaPhase > 0.0)`) and the morph/character pair. In
	// src/dsp/MorphBlep.hpp: `!(dt > 0.f) || !(dt <= 1.f)` at the top of step(),
	// the same pair inside morphBlepCharFactor, `!(u > 0.f)` on its numerator and
	// `!(s <= 1.f)` on the crossing gate. NOT ONE of them is a comparison-ladder
	// helper. forge::clamp is rejected BY NAME at src/dsp/VcoCore.hpp and again
	// at src/dsp/MorphBlep.hpp because BOTH of its comparisons are false for a
	// not-a-number, which makes it inert against precisely the input class half
	// this grid is made of.
	//
	// The seeds are the proven non-degenerate literals used everywhere else in
	// this suite. NEVER a pair of zeros (T-30-02): a degenerate Xoroshiro seed
	// is a fixed point emitting an all-zero stream, which makes
	// std::normal_distribution's rejection loop never terminate — in Rack that
	// is a hang on patch load, not a failing test.
	{
		// Each entry carries its own reason. The first four are plan 30-08's and
		// are unchanged; the last four are plan 32-09's extension.
		static const float HOSTILE_RATES[] = {
			-44100.f,                                          // the reproduced CR-01 case
			0.f,                                               // the other non-positive rate
			44100.f,                                           // the legitimate CONTROL rate
			std::numeric_limits<float>::quiet_NaN(),           // a mis-wired host or an uninitialised ProcessArgs
			std::numeric_limits<float>::infinity(),            // the same, in its other non-finite direction
			-std::numeric_limits<float>::infinity(),           // ditto, signed the way a negated uninitialised field lands
			std::numeric_limits<float>::denorm_min(),          // what an arithmetic underflow upstream produces
			1e30f                                              // neither infinite nor small: passes a naive std::isfinite check while making the Nyquist ceiling meaningless
		};
		// THE FALSIFIED PREMISE THAT POINTED THIS WORK HERE, CORRECTED IN PLACE
		// (plan 32-09, D-15). The sentence that used to sit here said 1/1000 and
		// 999 are "DECOUPLED from every rate above — the shape Phase 32's
		// OVERSAMPLED INNER LOOP will produce naturally". THAT PREMISE IS FALSE.
		// AA-05 forbids oversampling in v2.0 BY NAME — its own wording is "no
		// minBLEP, no oversampling in v2.0" — so no such loop exists in this
		// phase and none will be added to it. src/dsp/VcoCore.hpp corrected the
		// identical sentence in its own deltaPhase-bound paragraph for the same
		// reason; this is the copy of it that lives in the test.
		//
		// THE CONCLUSION IS KEPT AND THE ENTRIES ALL STAY. The corrected premise
		// is stronger than the one it replaces:
		//   (a) sampleTime and sampleRate are INDEPENDENT POD fields that any
		//       caller may set independently, and this scenario is the caller
		//       that does — no oversampled loop is needed to produce a decoupled
		//       pair, only a caller, and the harness IS one; and
		//   (b) Phase 32 put a DIVISION BY dt behind sampleTime for the first
		//       time — in forge::morphBlepCharFactor and again at the sub-sample
		//       crossing position inside forge::MorphBlep::step — so these values
		//       now reach arithmetic that did not exist when this grid was
		//       written. THAT is why D-15 kept the item in Phase 32.
		//
		// THE MEASURED SCOPE LIMIT, alongside it (P-14). The SHIPPED formulation
		// divides by `dt` ONLY. There is no division by an edge width anywhere in
		// src/dsp/MorphBlep.hpp, and the optional narrow-pulse "reach" refinement
		// that would add one is deliberately deferred by that header. So hostile
		// timing reaches exactly two divisors in the whole band-limiter, both
		// behind the same negated guard, and no others.
		//
		// WHICH CLASSES ACTUALLY REACH THAT GUARD — MEASURED, and it is NOT what
		// this plan assumed. Of the 176 configurations below, the MorphBlep dt
		// guard fires on 140 and is passed by 36, and the three interesting
		// classes behave as follows:
		//   - a NEGATIVE subnormal, -infinity, zero, negative or NaN sampleTime
		//     reaches the guard: forge::VcoCore's own `!(deltaPhase > 0.0)` floor
		//     has already driven dt to 0.0, and MorphBlep::step returns the
		//     drained accumulator without dividing. 140 of 176.
		//   - a POSITIVE INFINITE sampleTime DOES NOT reach it when the rate is
		//     legitimate. deltaPhase = freq * inf is +infinity, which the core's
		//     OWN kVcoMaxDeltaPhase ceiling clamps to 0.5 — an entirely ordinary
		//     value from the band-limiter's point of view, correcting on 4096 of
		//     4096 samples in a direct probe. It reaches the guard only when the
		//     rate has already forced freq to 0, where 0 * inf is a NaN.
		//   - a POSITIVE SUBNORMAL sampleTime DOES NOT reach it either. At
		//     sampleRate 44100 it yields dt = 3.66616e-43 (pitchCV 0) and
		//     3.05896e-41 (pitchCV +10), both of which clear `dt > 0.f` and
		//     `dt <= 1.f`. What stops it is the NEXT guard down: `d / dt`
		//     overflows the float to +infinity, `!(s <= 1.f)` fires, and MEASURED
		//     0 of 4096 samples receive any correction at all.
		//   - the guard's UPPER bound `!(dt <= 1.f)` fires 0 times in 176
		//     configurations, and PROVABLY cannot fire from this call site:
		//     forge::VcoCore clamps at kVcoMaxDeltaPhase = 0.5, a full factor of
		//     two below it. src/dsp/MorphBlep.hpp says so itself, and plan 32-05
		//     reached that bound through the header's OWN unit tests rather than
		//     through the core. This grid tests what its call site can actually
		//     reach and says so, rather than claiming a guard it cannot touch.
		static const float HOSTILE_TIMES[] = {
			-1.f / 44100.f,                                    // the negative of the one legitimate value
			0.f,                                               // the increment-zeroing case
			1.f / 44100.f,                                     // paired with 44100: the one legitimate CONTROL point
			1.f / 1000.f,                                      // DECOUPLED from every rate above (see the corrected premise)
			999.f,                                             // decoupled and absurdly large, but finite
			std::numeric_limits<float>::quiet_NaN(),           // a mis-wired host or an uninitialised ProcessArgs
			std::numeric_limits<float>::infinity(),            // the same, non-finite
			-std::numeric_limits<float>::infinity(),           // ditto, signed
			std::numeric_limits<float>::denorm_min(),          // THE NEW ONE relative to sampleRate: an upstream underflow, the class that reaches the new divisor
			-std::numeric_limits<float>::denorm_min(),         // its sign partner, which the deltaPhase floor catches instead
			1e30f                                              // finite, enormous: passes a naive std::isfinite check
		};
		// Named _T4 on purpose: HOSTILE_PITCH is already taken by scenario three
		// inside this same TEST_CASE. DELIBERATELY NOT WIDENED by plan 32-09:
		// pitch is not what this scenario is about, and scenario three already
		// owns hostile V/OCT.
		static const float HOSTILE_PITCH_T4[] = {0.f, 10.f};

		// The step count both the code reviewer and the verifier reproduced at.
		// Unchanged by the extension.
		const int nHostile = 20000;

		for (float rate : HOSTILE_RATES) {
			for (float dt : HOSTILE_TIMES) {
				for (float pitchCV : HOSTILE_PITCH_T4) {
					forge::VcoCore core;
					core.seed(0xC0FFEEULL, 0xBADF00DULL);
					core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);

					forge::VcoInputs in = coreBase();
					in.pitchCV    = pitchCV;
					in.morph      = 0.5f;
					in.character  = 1.f;
					in.sampleTime = dt;
					in.sampleRate = rate;

					// ACCUMULATED, not asserted per sample: 176 configs at
					// 20000 steps would otherwise add roughly twenty-one million
					// assertions to a suite already past 2.6 million. Same idiom
					// as scenario three, and it is what makes the extension from
					// 48 configurations to 176 cost 768 added assertions rather
					// than seventeen million more.
					bool  allFinite       = true;
					bool  phaseInRange    = true;
					bool  freqNonNegative = true;
					float maxAbs          = 0.f;
					int   firstBadStep    = -1;

					// WR-06. The ceiling the core is SUPPOSED to have applied,
					// recomputed here from the same constant and the same
					// sanitising rule the header uses, so the test states the
					// contract independently rather than echoing whatever the
					// header happened to compute. A non-positive OR NaN rate has
					// no meaningful Nyquist limit, so the only defensible bound
					// is zero — `rate > 0.f` is false for negatives, for zero and
					// for NaN alike, which is exactly the classification wanted.
					const float expectedMaxFreq =
						forge::kVcoNyquistGuardFrac * ((rate > 0.f) ? rate : 0.f);
					bool  freqNyquistBounded = true;
					float maxFreqSeen        = 0.f;

					for (int i = 0; i < nHostile; ++i) {
						const float s = core.step(in);
						const float a = std::fabs(s);
						if (a > maxAbs) maxAbs = a;
						if (core.tel.freqHz > maxFreqSeen) maxFreqSeen = core.tel.freqHz;

						bool bad = false;
						if (!std::isfinite(s))                            { allFinite = false;       bad = true; }
						if (a > kHostileBoundV)                           {                          bad = true; }
						if (!(core.phase >= 0.0 && core.phase < 1.0))     { phaseInRange = false;    bad = true; }
						if (!(core.tel.freqHz >= 0.f))                    { freqNonNegative = false; bad = true; }
						// Negated so a NaN freqHz also counts as a failure, the
						// same way the header's own floor is written negated.
						if (!(core.tel.freqHz <= expectedMaxFreq))        { freqNyquistBounded = false; bad = true; }
						if (bad && firstBadStep < 0) firstBadStep = i;
					}

					CAPTURE(rate);
					CAPTURE(dt);
					CAPTURE(pitchCV);
					CAPTURE(maxAbs);
					CAPTURE(maxFreqSeen);
					CAPTURE(expectedMaxFreq);
					CAPTURE(firstBadStep);
					INFO("scenario: hostile timing driven straight into the core - no driver, nothing overwrites sampleTime/sampleRate");

					CHECK(allFinite);
					// OUTER tier, then the tighter one. RE-MEASURED BY PLAN
					// 32-09 OVER ALL 176 CONFIGURATIONS: the grid maximum is
					// still 5.000000 V exactly, so this grid keeps the musical
					// tier with 0.55 V to spare, and the extension did not
					// raise the envelope by so much as a bit.
					//
					// A STALE FIGURE CORRECTED IN PLACE WHILE RE-MEASURING. The
					// sentence here used to read "MEASURED across all 48
					// configurations at 5.000000 V exactly — the frozen-phase
					// DC value the guards produce when a hostile rate or time
					// zeroes the increment". The GRID MAXIMUM was right; the
					// per-configuration claim was not. MEASURED, 42 of the
					// original 48 sit exactly at 5.000000 V, and the other SIX
					// are strictly BELOW it because the oscillator genuinely
					// RUNS there — all six at sampleRate 44100, at increments
					// 0.005933 / 0.495 / 0.261626 / 0.5 / 0.5 / 0.5 measuring
					// 4.997915 / 2.390221 / 3.389438 / 1.770179 / 1.770179 /
					// 1.770179 V. Over the extended 176 the split is 146 at
					// exactly 5.000000 V and 30 below, the lowest of them at
					// 1.770179 V, and only FOUR distinct values appear beneath
					// the maximum. So the frozen-phase explanation describes
					// the maximum and the majority of the grid; it does not
					// describe every cell, and the sentence used to say it did.
					//
					// The four named assertions beside these are UNTOUCHED and
					// none is merged: the banner above explains why no one of
					// them is redundant with its neighbours.
					CHECK(maxAbs <= kHostileBoundV);
					CHECK(maxAbs <= kMusicalBoundV);
					CHECK(phaseInRange);
					CHECK(freqNonNegative);
					CHECK(freqNyquistBounded);
				}
			}
		}
	}

	// --- Scenario five: the P-10 Nyquist-ceiling overlapping-edge worst case. -
	// THE LOAD-BEARING SCENARIO FOR THE HOSTILE TIER. Without it kHostileBoundV
	// is decoration: a tier no scenario ever approaches proves nothing, which is
	// precisely the criticism scenario two's banner levels at a sweep-only bound
	// test. This drive is what makes the outer tier EXERCISED.
	//
	// WHERE IT DRIVES, AND WHY THERE. pitchCV +10 puts the requested frequency
	// (267,904 Hz) far above every guarded ceiling, so the clamp pins it at
	// kVcoNyquistGuardFrac * sampleRate and the phase increment lands at
	// EXACTLY 0.495 at all three sample rates — the 9.198 V row of the P-10
	// table, and the one increment where the 5 %-duty pulse's two edges fall
	// inside a single sample (the deliberate D-07 overlapping-edge case). That
	// the increment is rate-independent here is why the three rates measure
	// almost identically; it is a property of the clamp, not a coincidence.
	//
	// THE CHARACTER AXIS IS THE PLAN'S GRID CORRECTED BY MEASUREMENT. P-10's
	// prose puts the worst point at "character near zero", and plan 32-08's grid
	// followed it: morph {0.90, 0.95, 1.00} x character {0.00, 0.05, 0.10,
	// 0.20}. MEASURED, that grid maxes at 5.000000 V — it does not exceed even
	// the MUSICAL tier, and a scenario five built on it would have asserted
	// nothing. The character axis is therefore extended to 0.50 and 1.00, where
	// the measurement actually puts the peak. That is not inventing a scenario
	// to make a bound fire; it is the same pitch and the same intent at the
	// coordinates the oscillator actually peaks at, and the falsified premise is
	// recorded in this case's banner rather than quietly fixed.
	{
		static const float MORPHS_T5[] = {0.90f, 0.95f, 1.00f};
		static const float CHARS_T5[]  = {0.00f, 0.05f, 0.10f, 0.20f, 0.50f, 1.00f};

		for (double sr : SAMPLE_RATES) {
			// ACCUMULATED over the whole grid, not asserted per cell: the claim
			// is about the grid's peak, and one assertion per cell would say the
			// same thing eighteen times per rate.
			float gridMax = 0.f;
			bool  allFinite = true;
			float atMorph = -1.f, atChar = -1.f;

			for (float morph : MORPHS_T5) {
				for (float character : CHARS_T5) {
					forge::VcoInputs base = coreBase();
					base.pitchCV   = 10.f;
					base.morph     = morph;
					base.character = character;

					const int n = 4096;
					forge::VcoBlockDriver d(sr);
					std::vector<float> out = d.run(n, [=](int) { return base; });
					REQUIRE(out.size() == (size_t)n);

					for (size_t i = 0; i < out.size(); ++i) {
						if (!std::isfinite(out[i])) allFinite = false;
						const float a = std::fabs(out[i]);
						if (a > gridMax) { gridMax = a; atMorph = morph; atChar = character; }
					}
				}
			}

			CAPTURE(sr);
			CAPTURE(gridMax);
			CAPTURE(atMorph);
			CAPTURE(atChar);
			INFO("scenario: P-10 Nyquist-ceiling overlapping-edge worst case - HOSTILE TIER, measured 7.150197 at 44.1 kHz and 7.150281 at 48/96 kHz, at morph 1.00 character 1.00");

			CHECK(allFinite);

			// THE OUTER TIER. Measured 7.150197 / 7.150281 / 7.150281 against
			// 10.0 V, so it clears by about 2.85 V.
			CHECK(gridMax <= kHostileBoundV);

			// THE EXERCISE FLOOR, and it is the whole point of this scenario.
			// Pinned from step one's own measurement: the minimum of the three
			// per-rate grid maxima is 7.150197 V, less a 1.5 V cushion, giving
			// 5.650197 — pinned at 5.65 V.
			//
			// NOTE WHAT THIS FLOOR SITS ABOVE: 5.65 V is GREATER than
			// kMusicalBoundV at 5.55 V. So this assertion does not merely prove
			// the hostile tier is approached — it proves the observed maximum
			// genuinely EXCEEDS the musical bound, which is what makes the two
			// tiers a real distinction rather than a pair of numbers with the
			// same content. If a future change brought this grid back under
			// 5.55 V, this line goes RED and asks whether the outer tier still
			// needs to exist at all. That is the intended behavior.
			const float kExerciseFloorV = 5.65f;
			CHECK(gridMax > kExerciseFloorV);
			CHECK(kExerciseFloorV > kMusicalBoundV);
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

// ---------------------------------------------------------------------------
// 4. Two-instance independence under sample-by-sample interleaving
//    (D-17 / CORE-03).
//
//    WHAT IS BEING PROVEN, and why it is a behavioral test rather than a grep.
//    CORE-03 is a claim about what is ABSENT: no static, no global, no engine
//    accidentally shared between voices. Absence is exactly what a source-text
//    guard proves badly — a grep for `static` catches the obvious declaration
//    form and nothing else, and misses a function-local static, a shared
//    reference member, a singleton behind an accessor, and a shared pointer.
//    So the claim is tested as the PROPERTY polyphony actually needs: run two
//    differently-seeded cores interleaved, one sample each, alternating, and
//    require each to reproduce bit-exactly what it produced running alone.
//    Shared mutable state of ANY shape breaks that.
//
//    This case is trivially green on a correct implementation — and, written
//    carelessly, equally green on a broken one. Five distinct ways it goes
//    vacuous were MEASURED, and all five are implemented here:
//
//      (i)   two DIFFERENT spread seeds — with identical seeds a hypothetical
//            shared static forge::Waveshape produces identical coefficients and
//            is invisible;
//      (ii)  character = 1.0 — see the measured trap below;
//      (iii) genuinely DIFFERENT per-sample inputs per instance — a shared
//            phase accumulator is caught either way, but different inputs also
//            catch a shared freq, a shared telemetry block or a shared
//            Waveshape;
//      (iv)  an explicit assertion that the two solo blocks are
//            DISTINGUISHABLE — without it "interleaved == solo" is satisfiable
//            by two identical signals;
//      (v)   a permanent positive control, which is invariant 5 below. It is
//            what proves this helper can actually detect a defect. If invariant
//            5 ever goes green, THIS case is meaningless regardless of its own
//            verdict.
//
//    THE MEASURED CHARACTER TRAP (D-10), which must not be optimised away.
//    Every component-spread coefficient in the frozen forge::Waveshape is
//    consumed only behind a `character >= 0.001f` gate. Detectability of a
//    clobbered shared Waveshape — instance A running with instance B's
//    coefficients — was measured across character:
//
//        character 0.00 ->  0.000000 V,    0 / 1024 differing  (UNDETECTABLE)
//        character 0.05 ->  0.000520 V, 1024 / 1024
//        character 0.30 ->  0.024821 V, 1024 / 1024
//        character 1.00 ->  0.233187 V, 1024 / 1024
//
//    An independence test written at character = 0 therefore proves NOTHING
//    about shared shaper state. character = 1.0 below is that measurement, not
//    a stylistic choice. Invariant 3's control pins the same fact from the
//    other direction.
//
//    MEASURED RESULT for this construction: interleaved-versus-solo mismatches
//    A = 0 / 1024 and B = 0 / 1024, with soloA[i] == soloB[i] on 0 of 1024
//    samples.
//
//    ------------------------------------------------------------------------
//    (vi) THE DRIVES CARRY SYNC. ADDED BY PLAN 33-04, AND THE REASON IS THAT
//    WITHOUT IT THIS CASE WOULD HAVE COVERED PHASE 33'S NEW STATE WITH NOTHING
//    WHILE STILL REPORTING GREEN.
//    ------------------------------------------------------------------------
//    Phase 33 gave forge::VcoCore two new per-instance members: a
//    forge::SchmittTrigger and a previous-voltage store
//    (src/dsp/VcoCore.hpp:330-331). ABSENCE OF SHARING IS EXACTLY THE PROPERTY
//    A SOURCE-TEXT GREP PROVES BADLY — `grep static src/dsp/VcoCore.hpp` finds
//    nothing today and would also find nothing if the state were shared through
//    a namespace-scope object, a singleton accessor, or a static inside a
//    helper called from step(). The proof this file relies on is BEHAVIOURAL,
//    and a behavioural proof covers only what its inputs exercise.
//
//    With `syncConnected` false on both drives — which is what this case did
//    before plan 33-04 — the sync branch is NOT ENTERED, so a hypothetical
//    shared trigger or shared store would be invisible here. The header said so
//    itself, in a paragraph that deliberately claimed less than the one above
//    it and named this plan as what would make the claim true. This is that
//    change; the header's paragraph was corrected in the same phase.
//
//    THE TWO DRIVES ARE GIVEN GENUINELY DIFFERENT SYNC, not the same master
//    twice. A runs 16 master cycles across the block with the jack patched
//    throughout; B runs 24 and has its jack PULLED over samples 384..639. So
//    the two instances take different sync branches on the same sample index,
//    which is the arrangement a shared trigger or a shared store would corrupt:
//    each instance's branch would start depending on the other's voltage. A
//    version of this check that gave both drives the SAME master would still
//    catch a shared store, but only by luck of the sample alignment.
//
//    MEASURED with the sync extension, and the two property assertions are
//    UNMOVED at every rate: A = 0 / 1024, B = 0 / 1024, soloEqual 0 / 1024,
//    with 16 resets on drive A and 18 on drive B (24 cycles less the 6 that
//    fall inside the unpatched window) and 0 resets on B while unpatched.
//
//    AND THE NEW COVERAGE IS PROVED ABLE TO FAIL, BY MEASUREMENT RATHER THAN
//    BY INSPECTION. Invariant 5's permanent control shares a PHASE ACCUMULATOR,
//    which says nothing about whether this check can see a shared SYNC member.
//    Two out-of-tree probes were built against a scratch copy of
//    src/dsp/VcoCore.hpp — one per new member, each turning exactly one of them
//    into a process-wide static and changing nothing else — and both produce a
//    real red on BOTH instances at all three rates:
//
//        probe                        mismatchA     mismatchB
//        prevSyncVolts shared          961 / 1024    982 / 1024
//        syncTrig shared               630 / 1024    976 / 1024
//
//    Neither figure is saturated, so unlike invariant 5's 512/512 these two CAN
//    move in both directions and are a real pin rather than a ceiling. The
//    header was restored and re-verified byte-identical afterwards; no
//    repository artifact was created.
//
//    THE POSITIVE CONTROL WAS RE-RUN AND ITS FIGURES ARE UNMOVED TOO: 512 /
//    512 / total 1024 at all three rates, the fourth consecutive capture at
//    those numbers. Read the saturation paragraph on
//    DeliberatelyBrokenSharedStateCore before reading anything into that: those
//    three figures are pinned at the ceiling of their own metric and can only
//    ever detect a change that pushes the count DOWN. "Unchanged" there is
//    insensitivity, not evidence of inertness — the evidence of inertness is
//    the separate argument written at the mirror itself.
// ---------------------------------------------------------------------------
TEST_CASE("vco core: two-instance independence under sample-by-sample interleaving (D-17)") {
	const int n = 1024;

	// All four seeds spelled out per instance, the idiom
	// tests/test_vco_harness.cpp:162-163 uses. The DRIFT pair is IDENTICAL for
	// both instances and only the SPREAD pair differs — requirement (i) — so the
	// two cores differ in exactly the one mechanism this phase has (D-11), and a
	// hypothetical shared static shaper cannot hide behind matching coefficients.
	// These are the researcher's measured pairs; substituting others would leave
	// the figures recorded above describing a variant of this code.
	const std::function<void(forge::VcoCore&, int)> seedInstance =
		[](forge::VcoCore& c, int which) {
			c.seed(0xC0FFEEULL, 0xBADF00DULL);
			if (which == 0) {
				c.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);
			} else {
				c.setSpreadSeed(0xDEADBEEFULL, 0xCAFEF00DULL);
			}
		};

	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);

		forge::VcoInputs base = coreBase();
		// REQUIREMENT (ii). Not decoration — see the measured table above.
		base.character = 1.f;

		const float denom = (float)(n - 1);

		// REQUIREMENT (iii): two genuinely different drives. A sweeps pitch at a
		// fixed morph; B holds a DIFFERENT fixed pitch and sweeps morph. Neither
		// instance ever sees the other's input, so any state they share shows up
		// as a mismatch against their own solo run.
		//
		// REQUIREMENT (vi), ADDED BY PLAN 33-04: THE TWO DRIVES CARRY SYNC, AND
		// THEY CARRY DIFFERENT SYNC. See the banner paragraph above for why a
		// version of this check that left the jack unpatched would cover the two
		// new members with NOTHING while still reporting green.
		//   The masters are computed IN CLOSED FORM from the sample index, never
		// from a running accumulator. That is load-bearing: runInterleaveCheck
		// calls each functor FOUR times over the block — solo A, solo B and the
		// two interleaved instances — and a stateful generator would hand the
		// four runs four different masters and the case would fail for a reason
		// that has nothing to do with the core. Both increments are dyadic and
		// (i+1)*dtm is exact in double for every i in this block, so all four
		// calls return bit-identical voltages.
		const double kDtmA = 16.0 / 1024.0;   // 16 master cycles across the block
		const double kDtmB = 24.0 / 1024.0;   // 24 — a DIFFERENT master, deliberately
		const int    kUnpatchedFrom = 384;    // B's jack is pulled for this window...
		const int    kUnpatchedTo   = 640;    // ...and pushed back in here
		const std::function<forge::VcoInputs(int)> inA = [=](int i) {
			forge::VcoInputs in = base;
			in.pitchCV = -1.f + 2.f * ((float)i / denom);   // -1 V .. +1 V
			in.morph   = 0.25f;
			const double ph = (double)(i + 1) * kDtmA;
			in.syncVolts     = (float)(5.0 * (1.0 - 2.0 * (ph - std::floor(ph))));
			in.syncConnected = true;                        // patched for the whole block
			return in;
		};
		const std::function<forge::VcoInputs(int)> inB = [=](int i) {
			forge::VcoInputs in = base;
			in.pitchCV = 0.5f;                              // a different, FIXED pitch
			in.morph   = (float)i / denom;                  // 0 .. 1
			const double ph = (double)(i + 1) * kDtmB;
			in.syncVolts     = (float)(5.0 * (1.0 - 2.0 * (ph - std::floor(ph))));
			// UNPATCHED over a window in the middle, so the two instances take
			// DIFFERENT sync branches on the same sample index. That difference
			// is the point: a shared trigger or a shared store would make each
			// instance's branch depend on the other's voltage, and every one of
			// those samples would diverge from its own solo run.
			in.syncConnected = !(i >= kUnpatchedFrom && i < kUnpatchedTo);
			return in;
		};

		const InterleaveResult r =
			runInterleaveCheck<forge::VcoCore>(seedInstance, sr, n, inA, inB);
		REQUIRE(r.soloA.size() == (size_t)n);

		// --- 1. VALIDITY CHECK, FIRST. --------------------------------------
		// Assert the fixture tests what it claims BEFORE asserting the result —
		// the same habit as check_includes.sh [6/7]'s nc2_direct guard, which
		// fails the whole section if its two-hop fixture turns out to be
		// detectable one-hop. Here: the helper's solo block for instance A must
		// be bit-identical to what forge::VcoBlockDriver produces from the same
		// four seeds over the same inputs. A helper that quietly stopped
		// overwriting sampleTime and sampleRate — or that seeded in the wrong
		// order, or reused a core between runs — would sail through every
		// assertion below this line. It cannot get past this one.
		forge::VcoBlockDriver d(sr, 0xC0FFEEULL, 0xBADF00DULL, 0x9E3779B9ULL, 0x7F4A7C15ULL);
		SyncTrace trA;
		std::vector<float> harnessA = driveTraced(d, n, inA, trA);
		REQUIRE(harnessA.size() == r.soloA.size());
		bool helperMatchesHarness = true;
		for (size_t i = 0; i < harnessA.size(); ++i) {
			if (harnessA[i] != r.soloA[i]) { helperMatchesHarness = false; break; }
		}
		REQUIRE(helperMatchesHarness);

		// --- 1b. THE SYNC DRIVES ARE NON-VACUOUS (requirement vi). -----------
		// REQUIREd, not CHECKed, and ahead of everything below it — the same
		// validity-first habit as the block above. Without this the extension is
		// unfalsifiable in the worst way: a functor that stopped setting
		// syncVolts, or a `syncConnected` that quietly went false, would leave
		// invariant 4 passing exactly as it did before Phase 33 and still
		// claiming to cover the two new members. This is the only line that
		// would notice.
		//   Instance B's trace comes from a SECOND driver run rather than from
		// runInterleaveCheck, which returns samples and not telemetry. That is
		// 1024 extra steps and it buys the one assertion the extension rests on.
		forge::VcoBlockDriver dB(sr, 0xC0FFEEULL, 0xBADF00DULL, 0xDEADBEEFULL, 0xCAFEF00DULL);
		SyncTrace trB;
		std::vector<float> harnessB = driveTraced(dB, n, inB, trB);
		REQUIRE(harnessB.size() == (size_t)n);

		int firedA = 0, firedB = 0, firedBWhileUnpatched = 0;
		for (int i = 0; i < n; ++i) {
			if (trA.fired[(size_t)i]) ++firedA;
			if (trB.fired[(size_t)i]) {
				++firedB;
				if (i >= kUnpatchedFrom && i < kUnpatchedTo) ++firedBWhileUnpatched;
			}
		}
		CAPTURE(firedA);
		CAPTURE(firedB);
		CAPTURE(firedBWhileUnpatched);
		INFO("requirement vi: both drives must actually RESET, and B must reset on a different schedule from A");
		// MEASURED: A = 16 resets (one per master cycle), B = 18 — 24 cycles
		// less the 6 that fall inside the unpatched window.
		REQUIRE(firedA > 0);
		REQUIRE(firedB > 0);
		REQUIRE(firedA != firedB);
		// And the gate holds inside the interleave drive too, not only in
		// invariant 7's own dedicated case.
		REQUIRE(firedBWhileUnpatched == 0);

		// --- 2. DISTINGUISHABILITY (requirement iv). ------------------------
		// Measured with this construction: equal at 0 of 1024 samples. The
		// threshold is a tenth rather than zero so a chance coincidence at a
		// shared zero crossing cannot make the suite flaky across toolchains.
		CAPTURE(r.soloEqual);
		CHECK(r.soloEqual < n / 10);

		// --- 3. THE PROPERTY ITSELF. ----------------------------------------
		// Measured on the real core: A = 0 / 1024, B = 0 / 1024. Captured so the
		// figures appear in `-s` output on a PASS, which is the audit trail plan
		// 30-07's phase gate compares the first CI run against.
		CAPTURE(r.mismatchA);
		CAPTURE(r.mismatchB);
		CHECK(r.mismatchA == 0);
		CHECK(r.mismatchB == 0);
	}
}

// ---------------------------------------------------------------------------
// 5. THE POSITIVE CONTROL for invariant 4 (D-17 / CORE-03).
//
//    A deliberately-broken stand-in core that shares one static phase
//    accumulator across every instance is driven through the SAME
//    runInterleaveCheck() helper, with the same shape of input functors, and is
//    REQUIRED to fail the independence property. See the banner on
//    DeliberatelyBrokenSharedStateCore above for why the type exists, why it
//    lives in this TU's anonymous namespace, and why it must never be deleted
//    or moved under src/.
//
//    IF THIS CASE EVER GOES GREEN, INVARIANT 4 ABOVE IS MEANINGLESS AND MUST BE
//    TREATED AS FAILING regardless of its own verdict. A green result here does
//    not mean the shared accumulator has been fixed — the accumulator is
//    supposed to be broken. It means runInterleaveCheck() has stopped being
//    able to see shared state at all, which is precisely the condition that
//    would let a real regression into forge::VcoCore pass unnoticed.
//
//    WHY THE ASSERTION IS "AT LEAST ONE MISMATCH" RATHER THAN AN EXACT COUNT.
//    The control's own SOLO baselines are polluted BY DESIGN: all four runs
//    inside the helper — solo A, solo B, and the two interleaved instances —
//    share the one static, so the accumulator is never at the same value twice
//    and the broken core is not even reproducible against itself. That is not
//    a flaw in the fixture, it IS the symptom of the defect, and it is why an
//    exact count would be pinning an accident of run order rather than the
//    property. The researcher measured 511 of 512 and 512 of 512 mismatches
//    with different inputs (511 of 512 with identical inputs); the contract
//    asserted here is only the inequality that matters: the helper detects
//    shared state.
// ---------------------------------------------------------------------------
TEST_CASE("vco core: independence positive control - a shared static accumulator FAILS the same check (D-17)") {
	// The 512-sample block the researcher measured.
	const int n = 512;

	// The same seeding shape as invariant 4: identical drift pair, different
	// spread pairs, so the control differs from the real case in exactly one
	// thing — the type being driven.
	const std::function<void(DeliberatelyBrokenSharedStateCore&, int)> seedInstance =
		[](DeliberatelyBrokenSharedStateCore& c, int which) {
			c.seed(0xC0FFEEULL, 0xBADF00DULL);
			if (which == 0) {
				c.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);
			} else {
				c.setSpreadSeed(0xDEADBEEFULL, 0xCAFEF00DULL);
			}
		};

	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);

		forge::VcoInputs base = coreBase();
		base.character = 1.f;

		const float denom = (float)(n - 1);
		const std::function<forge::VcoInputs(int)> inA = [=](int i) {
			forge::VcoInputs in = base;
			in.pitchCV = -1.f + 2.f * ((float)i / denom);
			in.morph   = 0.25f;
			return in;
		};
		const std::function<forge::VcoInputs(int)> inB = [=](int i) {
			forge::VcoInputs in = base;
			in.pitchCV = 0.5f;
			in.morph   = (float)i / denom;
			return in;
		};

		// THE SAME HELPER, instantiated on the broken type. A control that ran
		// its own copy of the drive loop would prove nothing about the loop the
		// real case uses.
		const InterleaveResult r =
			runInterleaveCheck<DeliberatelyBrokenSharedStateCore>(seedInstance, sr, n, inA, inB);
		REQUIRE(r.soloA.size() == (size_t)n);

		const int totalMismatch = r.mismatchA + r.mismatchB;
		CAPTURE(r.mismatchA);
		CAPTURE(r.mismatchB);
		CAPTURE(totalMismatch);

		// THE CONTRACT: the helper detects shared state. Measured 511/512 and
		// 512/512 on this construction.
		CHECK(totalMismatch > 0);
	}
}

// ---------------------------------------------------------------------------
// 6. Audio-rate MORPH sweeping through every segment boundary stays finite and
//    bounded (MORPH-01 / MORPH-02 / D-16 / P-13 / T-32-24 / T-32-25).
//
//    WHY THIS CASE EXISTS AND WHY IT IS THE HARD ONE. D-16 pulled MORPH's CV
//    jack into Phase 32 precisely because audio-rate morph sweeping through the
//    crossfade's segment boundaries is the hardest case this phase's
//    band-limiting has to survive: at every boundary the weight vector the
//    correction is computed from changes SHAPE, not merely magnitude, and at
//    0.75 it changes FORM — the frozen path stops crossfading two rectangles and
//    switches to the direct-duty special case (src/dsp/Waveshape.hpp:179-182,
//    mirrored at src/dsp/MorphBlep.hpp's `if (segment == 3)`). Nothing in this
//    suite asserted that drive before this case.
//
//    WHAT DRIVES IT. The cross product of the three production sample rates,
//    three notes DERIVED from forge::kVcoFreqC4 (pitchCV +3, +4 and +5, which
//    place the fundamental at 2093.00, 4186.01 and 8372.02 Hz — C7, C8 and C9,
//    the same three notes tests/test_vco_spectrum.cpp gates the alias floor at),
//    and three morph modulation rates of 50, 500 and 2000 Hz. 27 configurations,
//    20000 samples each. Character is held at 1.0, the value at which every
//    component-spread coefficient and the whole bleed ring are live. Morph is a
//    FULL-RANGE sinusoid over the unit interval, which is what a patched
//    audio-rate LFO into the MORPH CV jack at a full attenuverter produces.
//
//    IT DRIVES THE CORE POD DIRECTLY, through forge::VcoBlockDriver, with no
//    Rack shell anywhere in the way. src/AnalogVCO.cpp mixes
//    MORPH_PARAM + MORPH_CV_INPUT * 0.1 * MORPH_ATTEN_PARAM and conditions the
//    result; plan 32-06 added the MATCHING guard inside forge::VcoCore for
//    exactly this reason — the headless harness builds forge::VcoInputs itself
//    and the core must not rely on its caller.
//
//    ------------------------------------------------------------------------
//    NON-VACUITY FIRST, BECAUSE THIS CASE IS TRIVIALLY GREEN IF THE MODULATION
//    DOES NOTHING (T-32-25).
//    ------------------------------------------------------------------------
//    The same validity-first habit invariant 4 uses: the preconditions are
//    REQUIREd before any value assertion, so a modulation that silently stopped
//    modulating fails loudly here instead of leaving 27 bounds passing on a
//    steady tone.
//
//    A PLAN PREMISE FALSIFIED AND CORRECTED IN PLACE. Plan 32-09 specified
//    asserting that morph "crosses all four segment boundaries at 0.25, 0.50,
//    0.75 and 1.00". THERE ARE ONLY THREE crossable boundaries. The frozen path
//    computes `segment = (int)(morph * 4)` and then clamps `segment > 3` to 3
//    (Waveshape.hpp:165-166, mirrored at MorphBlep.hpp), so morph = 1.00 is the
//    top ENDPOINT of segment 3, not a division between two segments — a
//    full-range sinusoid reaches it and turns around without ever crossing it.
//    An assertion counting a crossing at 1.00 would have been RED on correct
//    behavior at every configuration. The conclusion is kept and made stronger
//    rather than weaker: this case counts crossings of the THREE real interior
//    boundaries AND requires all FOUR segments to be occupied AND requires the
//    sweep to reach both endpoints, which together prove strictly more than the
//    four-crossing claim did.
//
//    MEASURED crossing counts, which are two to three orders of magnitude above
//    the floor asserted: 61 at the slowest configuration (96 kHz, 50 Hz morph)
//    and 5441 at the fastest (44.1 kHz, 2000 Hz morph).
//
//    ------------------------------------------------------------------------
//    THE ONE NUMBER A READER WILL TRIP OVER, PINNED RATHER THAN LEFT TO BE
//    REDISCOVERED AND MISTAKEN FOR A REGRESSION (D-13 / T-32-24).
//    ------------------------------------------------------------------------
//    A MODULATED morph legitimately EXCEEDS the static-input musical envelope.
//    This case asserts kHostileBoundV — the phase-wide outer tier, which has no
//    exceptions and binds this scenario like every other — and DOES NOT assert
//    kMusicalBoundV, and the reason is measured rather than asserted:
//
//        grid-wide worst, swept morph   6.289864 V  (44.1 kHz, C9, 2000 Hz)
//        the same note and rate, morph  5.518032 V  (static-morph worst over
//        held STATIC, scanned over                   the whole morph axis,
//        201 points of the morph axis                201 points)
//        the excess                     0.771832 V
//
//    So the excess is produced by the MODULATION, not by the note: the identical
//    oscillator at the identical note and rate stays inside the musical tier at
//    every static morph value, and only moves outside it when morph is swept.
//
//    THE MECHANISM, MEASURED. The peak sample lands at morph 0.7485 (44.1 kHz)
//    and 0.7500 (48 kHz) — ON the 0.75 boundary, the square-to-pulse switch. At
//    that sample the naive path contributes 5.517806 V and the additive
//    correction contributes the rest. This is D-13's design, stated plainly: the
//    `pending` accumulator deliberately delivers the SECOND HALF of a correction
//    computed with the PREVIOUS sample's weight vector, site position and phase
//    increment. The rejected alternative — recomputing that second half at the
//    next sample from the then-current values — is REJECTED BY D-13 precisely
//    because under audio-rate MORPH and FM all three of those have moved by
//    then, so one consistent set of values for both halves is strictly more
//    robust even though it costs a slightly larger transient at a swept
//    boundary. That transient is what the 0.77 V above is.
//
//    THE RESEARCHER'S ENVELOPE, AND THIS RUN'S OWN FIGURE BESIDE IT. 32-RESEARCH
//    P-13 measured this same grid at max|out| <= 1.3171 in waveshaper units,
//    i.e. 6.5855 V, with the worst point at 8372 Hz under a 2 kHz morph sweep.
//    This run measures 1.257973 units — 6.289864 V — at the SAME coordinates.
//    Inside P-13's envelope, and the same worst point.
//
//    WHY THE EXCESS ASSERTION IS GRID-WIDE AND NOT PER-RATE, which is not a
//    convenience. MEASURED per rate, the grid maxima are 6.289864 V at 44.1 kHz,
//    6.006541 V at 48 kHz and 5.517825 V at 96 kHz. Only TWO of the 27
//    configurations exceed the musical tier at all, both of them C9 at 2000 Hz;
//    the other 25 sit between 5.508759 and 5.518031 V. At 96 kHz the excess
//    vanishes entirely — the phase increment at C9 is half as large, the swept
//    peak coincides with the naive envelope, and a per-rate form of this
//    assertion would be RED there on correct behavior. The honest claim is
//    therefore about the GRID's worst configuration, and that is the one made.
//
//    THE LINK TO THE OPERATOR CHECK, and the two are deliberately NOT
//    substitutes. This case is the headless counterpart of plan 32-10's in-Rack
//    UAT: the same control surface, the same three sample rates, the same three
//    modulation rates. THIS one bounds the numbers. THAT one judges whether it
//    SOUNDS like zipper noise at a crossfade seam, which no assertion in this
//    file can see. Neither one covers the other.
// ---------------------------------------------------------------------------
TEST_CASE("vco core: audio-rate MORPH sweeping through every segment boundary stays finite and bounded (MORPH-01 / MORPH-02)") {
	// Derived from forge::kVcoFreqC4 rather than hardcoded volts, so this case
	// moves with the constant instead of silently describing a note it no longer
	// plays: +3, +4 and +5 octaves above C4 are C7, C8 and C9.
	static const float PITCHES_I6[] = {3.f, 4.f, 5.f};
	static const float MORPH_RATES_HZ[] = {50.f, 500.f, 2000.f};

	// The same step count scenario four uses.
	const int n = 20000;

	// The grid-wide accumulator for the excess assertion below. Deliberately
	// OUTSIDE all three loops: the claim is about the worst configuration
	// anywhere on the grid, for the reason given in the banner.
	float gridWorstV  = 0.f;
	double atRate     = 0.0;
	float  atPitchCV  = -1.f;
	float  atModHz    = -1.f;

	for (double sr : SAMPLE_RATES) {
		for (float pitchCV : PITCHES_I6) {
			for (float modHz : MORPH_RATES_HZ) {
				CAPTURE(sr);
				CAPTURE(pitchCV);
				CAPTURE(modHz);
				INFO("scenario: audio-rate MORPH through the segment boundaries - HOSTILE TIER ONLY, measured 6.289864 V grid-wide worst at 44.1 kHz / C9 / 2000 Hz against a static-morph worst of 5.518032 V at the same note and rate");

				const double fundHz =
					(double)forge::kVcoFreqC4 * std::pow(2.0, (double)pitchCV);
				CAPTURE(fundHz);

				// Full-range sinusoid over the unit interval — a patched
				// audio-rate LFO at a full attenuverter. The angular step is
				// computed in DOUBLE so a 2 kHz modulator at 96 kHz does not
				// accumulate a phase error across 20000 samples.
				const double w = 2.0 * forge::kPi * (double)modHz / sr;

				forge::VcoInputs base = coreBase();
				base.pitchCV   = pitchCV;
				base.character = 1.f;   // every spread coefficient and the whole bleed ring live

				forge::VcoBlockDriver d(sr);
				std::vector<float> out = d.run(n, [=](int i) {
					forge::VcoInputs in = base;
					in.morph = (float)(0.5 + 0.5 * std::sin(w * (double)i));
					return in;
				});
				REQUIRE(out.size() == (size_t)n);

				// --- NON-VACUITY, ASSERTED FIRST (T-32-25) ------------------
				// The morph functor's OWN output, recomputed here from the same
				// expression the driver was handed, so the precondition is a
				// statement about the modulation rather than about the output it
				// produced.
				int  boundaryCrossings = 0;
				bool segmentSeen[4]    = {false, false, false, false};
				float minMorph = 2.f, maxMorph = -1.f;
				int  prevSegment = -1;
				for (int i = 0; i < n; ++i) {
					const float m = (float)(0.5 + 0.5 * std::sin(w * (double)i));
					if (m < minMorph) minMorph = m;
					if (m > maxMorph) maxMorph = m;
					// The FROZEN classification, mirrored exactly: (int)(morph*4)
					// with the minimum-of-3 clamp (Waveshape.hpp:165-166). This is
					// also why 1.00 is not a crossable boundary — see the banner.
					int s = (int)(m * 4.f);
					if (s > 3) s = 3;
					segmentSeen[s] = true;
					if (prevSegment >= 0 && s != prevSegment) ++boundaryCrossings;
					prevSegment = s;
				}
				CAPTURE(boundaryCrossings);
				CAPTURE(minMorph);
				CAPTURE(maxMorph);

				// MEASURED 61 at the slowest configuration and 5441 at the
				// fastest. The floor of 4 is two orders of magnitude below the
				// slowest and is a NON-VACUITY floor, not a characterisation:
				// it says the sweep genuinely traverses the crossfade, and it
				// would fire instantly if a future edit froze the modulator.
				REQUIRE(boundaryCrossings >= 4);
				// All four segments occupied, and both endpoints reached. This
				// is what replaces the falsified "crosses 1.00" claim, and it is
				// strictly stronger: it pins that the sweep visits the sine
				// centre AND the narrow pulse, not merely that it moves.
				REQUIRE(segmentSeen[0]);
				REQUIRE(segmentSeen[1]);
				REQUIRE(segmentSeen[2]);
				REQUIRE(segmentSeen[3]);
				REQUIRE(minMorph < 0.01f);
				REQUIRE(maxMorph > 0.99f);

				// --- NON-VACUITY OF THE OUTPUT BLOCK ITSELF -----------------
				int  nonZero = 0;
				bool constantBlock = true;
				for (int i = 0; i < n; ++i) {
					if (out[i] != 0.f) ++nonZero;
					if (out[i] != out[0]) constantBlock = false;
				}
				CAPTURE(nonZero);
				REQUIRE_FALSE(constantBlock);
				// MEASURED 100.00 % non-zero at all 27 configurations.
				REQUIRE(nonZero >= (n * 9) / 10);

				// --- THE VALUE ASSERTIONS, ACCUMULATED ----------------------
				// 27 configurations at 20000 steps would otherwise add more than
				// half a million assertions to a suite already past 2.6 million.
				// Same idiom as scenarios three, four and five.
				bool  allFinite = true;
				float maxAbs    = 0.f;
				for (int i = 0; i < n; ++i) {
					if (!std::isfinite(out[i])) allFinite = false;
					const float a = std::fabs(out[i]);
					if (a > maxAbs) maxAbs = a;
				}
				// Captured so the measured figure appears in `-s` output on a
				// PASS: 27 per-configuration maxima are the audit trail this
				// case exists to leave behind.
				CAPTURE(maxAbs);

				CHECK(allFinite);
				// THE OUTER TIER, WITH NO EXCEPTION AVAILABLE. Plan 32-08 pinned
				// kHostileBoundV as binding on every scenario any later plan
				// adds, and this is that scenario. MEASURED worst 6.289864 V
				// against 10.0 V, clearing by 3.71 V.
				//
				// kMusicalBoundV is NOT asserted here, and that is not a
				// carve-out from the outer tier — the outer tier is asserted on
				// the line above, exactly as scenario five does. The measurement
				// in the banner is what withholds the tighter one.
				CHECK(maxAbs <= kHostileBoundV);

				if (maxAbs > gridWorstV) {
					gridWorstV = maxAbs;
					atRate     = sr;
					atPitchCV  = pitchCV;
					atModHz    = modHz;
				}
			}
		}
	}

	// --- THE ASSERTION THAT KEEPS THIS CASE HONEST --------------------------
	// Without it, withholding kMusicalBoundV above would be an unexamined
	// exemption: a scenario that declines the tighter tier and never approaches
	// it would be claiming headroom it does not use. This pins that the excess
	// is REAL and pins its measured size, so a future change that brought the
	// grid back under 5.55 V goes RED and asks whether this case still needs the
	// exemption at all. That is the intended behavior, and it is the same
	// argument scenario five's exercise floor rests on.
	//
	// NO CUSHIONED FLOOR IS PINNED HERE, unlike scenario five, and the reason is
	// measured: scenario five clears the musical tier by 1.60 V and can afford a
	// 1.5 V cushion, while this grid clears it by 0.74 V. A 1.5 V cushion would
	// put the floor at 4.79 V — BELOW kMusicalBoundV — where it would assert
	// nothing, and any smaller cushion would be a number with no derivation
	// behind it. The bound itself is the honest comparator.
	CAPTURE(gridWorstV);
	CAPTURE(atRate);
	CAPTURE(atPitchCV);
	CAPTURE(atModHz);
	INFO("grid-wide worst across all 27 configurations; static-morph worst at the same note and rate is 5.518032 V, so the modulated excess is 0.771832 V (D-13's pending accumulator)");
	CHECK(gridWorstV > kMusicalBoundV);
	CHECK(gridWorstV <= kHostileBoundV);
}

// ---------------------------------------------------------------------------
// 7. HARD SYNC — detection, the gate, the hysteresis band and the
//    fractional-overshoot reset (SYNC-01 / D-01 / D-03). Appended by plan
//    33-04; nothing above it was renumbered.
//
//    WHY THIS CASE CAN EXIST AT ALL, which is a design decision being cashed in
//    rather than a convenience. D-02 put the SchmittTrigger, the previous-volts
//    store and the sub-sample solve INSIDE forge::VcoCore and made
//    forge::VcoInputs carry RAW VOLTS. A shell-side trigger would have handed
//    the core an already-decided boolean, and no headless test could ever have
//    watched a mis-detected edge — the same vacuity shape this project's
//    register records for a shell-side morph mix. Every assertion below reads
//    the real POD boundary, so all of it is reachable.
//
//    WHAT WAS ASSERTED BEFORE THIS CASE: NOTHING. Plans 33-01, 33-02 and 33-03
//    added zero assertions to this suite for the sync path between them; every
//    behaviour they recorded came from one-shot probes built outside the
//    repository and discarded. This is the first permanent evidence.
//
//    ------------------------------------------------------------------------
//    THE THRESHOLD LITERALS ARE INHERITED, AND THE TWO SHIPPED SITES ARE NAMED
//    ------------------------------------------------------------------------
//    low = 0.1 V, high = 1.0 V. Byte-for-byte what src/dsp/LfoCore.hpp:137 and
//    src/dsp/ClockTracker.hpp:111 already pass (D-03). They are written into
//    this banner rather than into a test-local constant on purpose: a local
//    copy would be a fourth site to keep in step, and this file already carries
//    one hand-kept mirror it has watched drift.
//
//    ------------------------------------------------------------------------
//    A MEASURED FACT THAT LOOKS LIKE A BUG AND IS NOT, RECORDED BEFORE THE
//    ASSERTIONS SO NOBODY "FIXES" IT: g AND f ARE DIFFERENT QUANTITIES.
//    ------------------------------------------------------------------------
//    `MasterBlock::wrapG` is g, the TRUE fraction of the sample interval at
//    which the master crossed. `tel.syncFrac` is f, the linear interpolation
//    the detector computes between the previous and current SAMPLE voltages.
//    For a HARD-EDGED master they do not track each other at all, and the
//    algebra says why: with a falling saw of increment dtm, prev and now sit on
//    opposite sides of a jump of fixed height, so f collapses to
//    (phim[k-1] - 0.4) / (1 - dtm), which barely moves.
//
//    MEASURED at dtm = 1/128, both offsets, every rate:
//
//        phi0 = 0        -> g = 1.000000 at every wrap, f = 0.596850
//        phi0 = 0.5/128  -> g = 0.500000 at every wrap, f = 0.600787
//
//    g HALVES and f moves by four thousandths. That is not a detector defect:
//    linear interpolation across a discontinuity has no information about where
//    inside the sample the discontinuity was. It is 33-RESEARCH Pitfall 10
//    ("a grid whose masters all have hard edges tests nothing about sub-sample
//    placement") arriving as a number, and it is the reason plan 33-05's
//    placement grid MUST NOT be built on hard-edged masters alone. The
//    band-limited subcase in the D-12 case below is where f starts tracking g.
//
//    So this case asserts what the detector actually contracts to do — the
//    reset lands at (1 - f) * deltaPhase for the f it recorded — and does NOT
//    assert f == g, which would be red on correct behaviour.
// ---------------------------------------------------------------------------
TEST_CASE("vco sync: (SYNC-01 / D-01 / D-03) a master rising edge resets the phase to the fractional overshoot") {
	const int    n   = 4096;
	const int    Km  = 32;                              // master cycles in the block
	const double dtm = (double)Km / (double)n;          // 1/128 — exactly representable

	// Two master phase offsets, giving TRUE wrap fractions of 1.0 and 0.5. Both
	// are driven so the g-versus-f measurement in the banner is reproduced by
	// this case on every run rather than quoted from a session that has gone.
	static const double PHI0[] = {0.0, 0.5 / 128.0};

	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);
		const float  dtf      = (float)(1.0 / sr);      // exactly what the driver injects
		const double masterHz = dtm * sr;
		CAPTURE(masterHz);

		forge::VcoInputs base = coreBase();
		// A LIVE INCREMENT, and it is a requirement rather than a default. Plan
		// 33-02 recorded that `(1 - f) * deltaPhase` is still ZERO when
		// deltaPhase is zero — which the guards produce at a non-positive or
		// non-finite rate and at extreme negative pitch — so the strictly-positive
		// claim below belongs on a running oscillator and nowhere else. C4.
		base.pitchCV   = 0.f;
		base.morph     = 0.35f;
		base.character = 0.6f;

		// --- Subcase A: the reset fires, once per master cycle, and lands
		//     exactly where D-01 says it does. --------------------------------
		for (double phi0 : PHI0) {
			CAPTURE(phi0);
			const MasterBlock m = makeMasterSaw(n, dtm, 5.0, phi0);
			REQUIRE(m.totalWraps == (long)Km);

			forge::VcoBlockDriver d(sr);
			SyncTrace tr;
			std::vector<float> out = driveTraced(d, n, [&](int i) {
				forge::VcoInputs in = base;
				in.syncVolts     = m.volts[(size_t)i];
				in.syncConnected = true;
				return in;
			}, tr);
			REQUIRE(out.size() == (size_t)n);
			REQUIRE(tr.fired.size() == (size_t)n);

			// ACCUMULATE-THEN-ASSERT. This suite is already past two and a half
			// million assertions; a per-sample form at 4096 x 2 offsets x 3 rates
			// would add a quarter of a million more and say nothing extra.
			int    fired          = 0;
			int    phaseMismatch  = 0;
			int    nonPositive    = 0;
			int    firstBadStep   = -1;
			double minPostReset   = 1.0;
			float  minFrac        = 2.f, maxFrac = -1.f;
			bool   allFinite      = true;
			float  maxAbs         = 0.f;

			for (int i = 0; i < n; ++i) {
				if (!std::isfinite(out[(size_t)i])) allFinite = false;
				const float a = std::fabs(out[(size_t)i]);
				if (a > maxAbs) maxAbs = a;
				if (!tr.fired[(size_t)i]) continue;
				++fired;
				const float  f        = tr.frac[(size_t)i];
				const double dp       = expectedDeltaPhase(tr.freqHz[(size_t)i], dtf);
				// The header's own expression, in the header's own order and
				// precision: a float subtraction widened to double, then a double
				// multiply. Compared with ==, never doctest::Approx — Approx's
				// epsilon(0) still applies a relative margin and this is a
				// bit-exactness claim or it is nothing.
				const double expected = (double)(1.f - f) * dp;
				if (tr.phase[(size_t)i] != expected) {
					++phaseMismatch;
					if (firstBadStep < 0) firstBadStep = i;
				}
				if (!(tr.phase[(size_t)i] > 0.0)) ++nonPositive;
				if (tr.phase[(size_t)i] < minPostReset) minPostReset = tr.phase[(size_t)i];
				if (f < minFrac) minFrac = f;
				if (f > maxFrac) maxFrac = f;
			}

			CAPTURE(fired);
			CAPTURE(minFrac);
			CAPTURE(maxFrac);
			CAPTURE(minPostReset);
			CAPTURE(firstBadStep);
			CAPTURE(maxAbs);
			INFO("subcase A: +/-5 V falling-saw master through the real forge::VcoInputs boundary, syncConnected true");

			// Exactly one reset per master cycle, at every rate.
			CHECK(fired == Km);
			// The reset lands at the fractional overshoot, to the bit.
			CHECK(phaseMismatch == 0);
			// SYNC-02's never-zero clause, on a live increment.
			CHECK(nonPositive == 0);
			CHECK(allFinite);
			// The phase-wide OUTER tier binds this scenario like every other
			// (plan 32-08). The tighter musical tier is deliberately NOT asserted
			// on any sync drive in this plan, and the withholding is stated with
			// its measurement rather than left to be inferred: this drive
			// measures 4.920715 / 4.920976 / 4.921710 V at the three rates and
			// would clear 5.55 V with 0.63 V to spare. It is withheld because the
			// sync reset is currently UN-BAND-LIMITED BY DESIGN — the seam is
			// withheld until plan 33-06 — so a tier asserted now would be pinning
			// a transient plan 33-06 is about to change. T-33-07 makes plan 33-08
			// the owner of the sync envelope's tighter tier. THE HEADROOM IS
			// RECORDED SO THAT PLAN IS NOT STARTING FROM NOTHING.
			CHECK(maxAbs <= kHostileBoundV);
		}

		// --- Subcase B: the gate, and the hysteresis band OBSERVED. ----------
		// Three drives that only agree with each other if the band is real:
		// unpatched fires nothing however live the master; a master peaking
		// BELOW the high threshold fires nothing; the same master crossing both
		// thresholds fires once per cycle.
		{
			const MasterBlock full = makeMasterSaw(n, dtm, 5.0, 0.0);
			const MasterBlock low  = makeMasterSaw(n, dtm, 0.9, 0.0);   // peak 0.9 V < 1.0 V

			struct GateDrive { const MasterBlock* m; bool connected; int expect; const char* what; };
			const GateDrive DRIVES[] = {
				{&full, false, 0,  "unpatched: a live +/-5 V master, syncConnected FALSE"},
				{&low,  true,  0,  "patched but never reaching the 1.0 V high threshold (peak 0.9 V)"},
				{&full, true,  Km, "patched and crossing BOTH thresholds"},
			};

			for (const GateDrive& gd : DRIVES) {
				forge::VcoBlockDriver d(sr);
				SyncTrace tr;
				std::vector<float> out = driveTraced(d, n, [&](int i) {
					forge::VcoInputs in = base;
					in.syncVolts     = gd.m->volts[(size_t)i];
					in.syncConnected = gd.connected;
					return in;
				}, tr);
				REQUIRE(out.size() == (size_t)n);

				int fired = 0;
				for (int i = 0; i < n; ++i) if (tr.fired[(size_t)i]) ++fired;
				CAPTURE(fired);
				CAPTURE(gd.expect);
				INFO(gd.what);
				CHECK(fired == gd.expect);
			}
		}

		// --- Subcase B2: the band itself, with a master built to sit INSIDE it.
		// The three drives above are consistent with a plain one-threshold
		// comparator. This one is not: after the first fire the master dips to
		// 0.5 V — BELOW the high threshold but ABOVE the low one — and rises
		// again. A comparator fires twice. A hysteresis band fires ONCE, and
		// only fires a second time after the master goes below 0.1 V.
		{
			static const float BAND_V[]   = {0.05f, 3.f, 0.5f, 3.f, 0.05f, 3.f};
			const int          seg        = 16;
			const int          nBand      = seg * 6;
			forge::VcoBlockDriver d(sr);
			SyncTrace tr;
			std::vector<float> out = driveTraced(d, nBand, [&](int i) {
				forge::VcoInputs in = base;
				in.syncVolts     = BAND_V[i / seg];
				in.syncConnected = true;
				return in;
			}, tr);
			REQUIRE(out.size() == (size_t)nBand);

			int fired = 0, firedInMiddleRise = 0;
			for (int i = 0; i < nBand; ++i) {
				if (!tr.fired[(size_t)i]) continue;
				++fired;
				if (i / seg == 3) ++firedInMiddleRise;   // the 0.5 V -> 3 V rise
			}
			CAPTURE(fired);
			CAPTURE(firedInMiddleRise);
			INFO("subcase B2: 0.05 -> 3 -> 0.5 -> 3 -> 0.05 -> 3 V. The 0.5 V dip does NOT re-arm; only the 0.05 V dip does");
			CHECK(fired == 2);
			CHECK(firedInMiddleRise == 0);
		}

		// --- Subcase C: the never-zero claim, and the landmine reached BY
		//     ARITHMETIC rather than by choice. ------------------------------
		// A master sample landing EXACTLY on the high threshold makes the raw
		// quotient (1 - prev)/(now - prev) exactly 1.0 when prev is 0 — and the
		// reset would then be (1 - 1) * deltaPhase, i.e. exactly zero, with the
		// sub-sample timing the whole block exists to preserve destroyed and
		// nobody having chosen to snap. This is NOT a measure-zero event: a gate
		// output idling at that level, or a quantised CV, produces it routinely.
		//
		// WHAT STOPS IT IS THE GUARD'S STRICT UPPER BOUND, `!(f < 1.f)`, TOGETHER
		// WITH ITS FALLBACK OF ZERO. The fallback value is itself load-bearing:
		// ONE would reintroduce this exact landmine through the guard that was
		// supposed to stop it, which is the failure mode of a guard that
		// "sanitises" to the nearest bound. Any value except one would do; zero
		// means "treat the edge as coincident with the previous sample" and
		// yields the LARGEST in-range overshoot, deltaPhase itself.
		{
			const int   seg   = 8;
			const int   nAdv  = seg * 3;
			// 0 V arms the trigger LOW, then EXACTLY 1.0f — bit-for-bit the high
			// threshold the core passes to forge::SchmittTrigger.
			auto voltsAt = [&](int i) { return (i < seg) ? 0.f : 1.f; };

			forge::VcoBlockDriver d(sr);
			SyncTrace tr;
			std::vector<float> out = driveTraced(d, nAdv, [&](int i) {
				forge::VcoInputs in = base;
				in.syncVolts     = voltsAt(i);
				in.syncConnected = true;
				return in;
			}, tr);
			REQUIRE(out.size() == (size_t)nAdv);

			// The raw quotient the core WOULD have used, recomputed here from the
			// two voltages the test itself supplied. Asserting this is what makes
			// the subcase non-vacuous: without it, a green result is equally
			// consistent with the arithmetic never having reached 1 at all.
			const float rawQuotient = (1.0f - voltsAt(seg - 1)) / (voltsAt(seg) - voltsAt(seg - 1));

			int   fired = 0, firstFire = -1, nonPositive = 0;
			for (int i = 0; i < nAdv; ++i) {
				if (!tr.fired[(size_t)i]) continue;
				++fired;
				if (firstFire < 0) firstFire = i;
				if (!(tr.phase[(size_t)i] > 0.0)) ++nonPositive;
			}
			REQUIRE(fired == 1);
			REQUIRE(firstFire == seg);

			const double dp       = expectedDeltaPhase(tr.freqHz[(size_t)seg], dtf);
			const float  recorded = tr.frac[(size_t)seg];
			CAPTURE(rawQuotient);
			CAPTURE(recorded);
			CAPTURE(dp);
			CAPTURE(tr.phase[(size_t)seg]);
			INFO("subcase C: a master sample landing EXACTLY on the 1.0 V high threshold - the snap-to-zero landmine reached by arithmetic");

			// The arithmetic really does reach exactly one.
			CHECK(rawQuotient == 1.0f);
			// The guard really does catch it, and lands on ZERO rather than on
			// the nearest bound.
			CHECK(recorded == 0.f);
			// So the reset is the largest in-range overshoot, not the snap.
			CHECK(tr.phase[(size_t)seg] == dp);
			CHECK(nonPositive == 0);
			CHECK(tr.phase[(size_t)seg] > 0.0);
		}
	}
}

// ---------------------------------------------------------------------------
// 8. THE DETECTOR'S STRUCTURAL CEILING, NAMED BEFORE IT IS GATED
//    (SYNC-01 / D-09 / SC-3). Appended by plan 33-04.
//
//    ------------------------------------------------------------------------
//    THE LIMITATION, STATED FIRST, BEFORE ANY ASSERTION IS WRITTEN AGAINST IT
//    ------------------------------------------------------------------------
//    A Schmitt trigger reading ONE VOLTAGE PER SAMPLE can observe AT MOST ONE
//    RISING EDGE PER SAMPLE, by construction. forge::SchmittTrigger::process
//    takes a single float and returns a single bool
//    (src/dsp/RackCompat.hpp:50-56), so a master running faster than the sample
//    rate has edges this VCO cannot see — not because the detection is wrong,
//    but because the information is not in its input.
//
//    THAT IS WHAT SC-3's "handles >= 1 sync event within a single sample"
//    CLAUSE IS DISCHARGED AS. The criterion is satisfiable as a statement about
//    HANDLING, and the handling asserted here is: every edge the detector CAN
//    observe fires EXACTLY ONCE, the edges it misses are missed IDENTICALLY at
//    all three sample rates, and the output stays finite and inside the outer
//    tier throughout. No document edit is required and none was made (D-09).
//
//    THE ALTERNATIVE IS REJECTED BY NAME, not left unconsidered: inferring the
//    master's RATE from the timing of successive edges and firing several
//    events inside one sample. It is rejected for two reasons. The VCO sees a
//    VOLTAGE, not a phase — there is nothing in a single sample to interpolate
//    a second edge from. And period estimation is the CLOCK machinery the
//    shipped LFO uses at clock rates (src/dsp/ClockTracker.hpp), which is the
//    wrong instrument at audio rate: it needs several cycles to converge, and a
//    master four times the sample rate gives it aliased garbage to converge on.
//
//    AND THE ORDER OF THOSE TWO PARAGRAPHS IS THE POINT. This project's own
//    register (item 6) records the precedent that an instrument's limitation
//    must be NAMED rather than worked around. Naming it BEFORE the gate is
//    written is that move applied ahead of time instead of as a rescue after a
//    measurement came back inconvenient.
//
//    ------------------------------------------------------------------------
//    WHY THE CROSS-RATE AGREEMENT IS THE RIGHT INSTRUMENT, AND WHAT IT WOULD
//    CATCH
//    ------------------------------------------------------------------------
//    The masters below are parametrised by dtm — master cycles PER SAMPLE — and
//    NOT by Hz. So the voltage sequence handed to the detector is bit-identical
//    at 44.1, 48 and 96 kHz, and the fired/missed pattern must be too. That is
//    not a tautology dressed as a test: it is exactly the property that FAILS
//    if any part of the detection ever starts reading sampleRate or sampleTime
//    — which is precisely what the rejected alternative above would require.
//    The assertion is a permanent tripwire on that design boundary.
//
//    MEASURED, and the three descriptions agree at every dtm:
//
//        dtm      master vs rate   wraps   observed   what the ceiling costs
//        0.0625   1/16x            512     512        nothing
//        0.125    1/8x            1024    1024        nothing
//        0.25     1/4x            2048    2048        nothing
//        0.75     3/4x            6144    2048        2 of every 3 edges
//        1.0      1x              8192       0        EVERY edge: at exactly the
//                                                     sample rate the master is a
//                                                     CONSTANT +5 V and the trigger
//                                                     never re-arms
//        1.5      3/2x           12288    4096        2 of every 3
//        2.5      5/2x           20480    4096        4 of every 5
//        4.0      4x             32768       0        every edge, same constant-
//                                                     voltage reason as 1.0
//
//    The dtm = 1.0 and dtm = 4.0 rows are the honest face of the ceiling and
//    are kept in the grid for that reason: an integer-ratio master above the
//    sample rate is INVISIBLE, not merely under-sampled. Nothing here pretends
//    otherwise.
// ---------------------------------------------------------------------------
TEST_CASE("vco sync: (SYNC-01 / D-09) the detector's structural ceiling, named before it is gated") {
	// Swept from well below the sample rate to four times above it. Every value
	// is dyadic so the master accumulates without rounding at any of them, and
	// `makeMasterSaw` uses std::floor rather than a single subtract precisely so
	// the above-1.0 entries wrap correctly.
	static const double SYNC_DTM[] = {1.0 / 16, 1.0 / 8, 1.0 / 4, 3.0 / 4, 1.0, 3.0 / 2, 5.0 / 2, 4.0};
	// Below this the falling saw is guaranteed to place a sample in BOTH the
	// arming region (phim >= 0.49, where the ramp is under 0.1 V) and the firing
	// region (phim < 0.4, where it is over 1.0 V), so every wrap is observable
	// and the count is an equality rather than an inequality.
	const double kEveryWrapObservable = 0.4;
	const int    n = 8192;

	for (double dtm : SYNC_DTM) {
		CAPTURE(dtm);

		// The three per-rate descriptions, compared as VALUES at the end. Not a
		// boolean: a boolean would say the rates agreed without saying on what.
		long              descWraps[3]  = {0, 0, 0};
		int               descFired[3]  = {0, 0, 0};
		unsigned long long descHash[3]  = {0, 0, 0};

		for (int r = 0; r < 3; ++r) {
			const double sr  = SAMPLE_RATES[r];
			CAPTURE(sr);
			const MasterBlock m = makeMasterSaw(n, dtm, 5.0, 0.0);
			REQUIRE(m.totalWraps > 0);

			forge::VcoInputs base = coreBase();
			base.pitchCV   = 0.f;
			base.morph     = 0.5f;
			base.character = 1.f;   // every spread coefficient and the whole bleed ring live

			forge::VcoBlockDriver d(sr);
			SyncTrace tr;
			std::vector<float> out = driveTraced(d, n, [&](int i) {
				forge::VcoInputs in = base;
				in.syncVolts     = m.volts[(size_t)i];
				in.syncConnected = true;
				return in;
			}, tr);
			REQUIRE(out.size() == (size_t)n);

			// cycleFired[j] — did the master's OWN cycle j produce an observed
			// reset? Expressing the missed set against the MASTER'S cycle index,
			// rather than against the sample index, is what makes the three rates
			// comparable at all.
			std::vector<char> cycleFired((size_t)m.totalWraps, (char)0);
			int   firedSamples = 0;
			bool  allFinite    = true;
			bool  phaseInRange = true;
			float maxAbs       = 0.f;
			int   firstBadStep = -1;

			for (int i = 0; i < n; ++i) {
				bool bad = false;
				if (!std::isfinite(out[(size_t)i]))                { allFinite = false;    bad = true; }
				const float a = std::fabs(out[(size_t)i]);
				if (a > maxAbs) maxAbs = a;
				if (a > kHostileBoundV)                            {                       bad = true; }
				if (!(tr.phase[(size_t)i] >= 0.0 && tr.phase[(size_t)i] < 1.0)) { phaseInRange = false; bad = true; }
				if (bad && firstBadStep < 0) firstBadStep = i;
				if (tr.fired[(size_t)i]) {
					++firedSamples;
					const long ci = m.wrapsBySample[(size_t)i] - 1;
					if (ci >= 0 && ci < (long)cycleFired.size()) cycleFired[(size_t)ci] = (char)1;
				}
			}

			int firedCycles = 0;
			for (size_t j = 0; j < cycleFired.size(); ++j) if (cycleFired[j]) ++firedCycles;

			// FNV-1a over the whole fired/missed pattern. A count alone would let
			// two rates agree on HOW MANY edges were missed while disagreeing on
			// WHICH; this pins the positions.
			unsigned long long h = 1469598103934665603ULL;
			for (size_t j = 0; j < cycleFired.size(); ++j) {
				h ^= (unsigned long long)(unsigned char)cycleFired[j];
				h *= 1099511628211ULL;
			}

			descWraps[r] = m.totalWraps;
			descFired[r] = firedCycles;
			descHash[r]  = h;

			CAPTURE(m.totalWraps);
			CAPTURE(firedSamples);
			CAPTURE(firedCycles);
			CAPTURE(maxAbs);
			CAPTURE(firstBadStep);
			INFO("the detector's structural ceiling: one voltage per sample can carry at most one rising transition");

			// (1) THE CEILING. Every observable edge fires EXACTLY ONCE. The
			//     boolean tel.syncFired already caps a sample at one event; this
			//     is the stronger statement — no master CYCLE fires twice either,
			//     so the counts cannot be inflated by a re-fire inside one cycle.
			CHECK(firedSamples == firedCycles);
			CHECK((long)firedCycles <= m.totalWraps);
			// ... and when the master is slow enough that every wrap is separated
			//     by at least one sample, "at most" becomes "exactly".
			if (dtm <= kEveryWrapObservable) CHECK((long)firedCycles == m.totalWraps);

			// (3) Bounded and finite THROUGHOUT, including at four times the
			//     sample rate. The outer tier binds here as everywhere; the
			//     tighter musical tier is withheld for the reason recorded in
			//     invariant 7 (the sync reset is un-band-limited until 33-06).
			//     MEASURED worst on this whole grid: 4.999978 V.
			CHECK(allFinite);
			CHECK(maxAbs <= kHostileBoundV);
			CHECK(phaseInRange);
		}

		// (2) THE MISSED-EDGE RULE IS THE SAME AT ALL THREE RATES, compared as
		//     three concrete descriptions rather than as a boolean.
		CAPTURE(descWraps[0]); CAPTURE(descWraps[1]); CAPTURE(descWraps[2]);
		CAPTURE(descFired[0]); CAPTURE(descFired[1]); CAPTURE(descFired[2]);
		CAPTURE(descHash[0]);  CAPTURE(descHash[1]);  CAPTURE(descHash[2]);
		INFO("the limitation is a property of the INSTRUMENT, not a rate-dependent defect - if any part of detection ever started reading sampleRate, these three would part company");
		CHECK(descWraps[0] == descWraps[1]);
		CHECK(descWraps[1] == descWraps[2]);
		CHECK(descFired[0] == descFired[1]);
		CHECK(descFired[1] == descFired[2]);
		CHECK(descHash[0] == descHash[1]);
		CHECK(descHash[1] == descHash[2]);
	}
}

// ---------------------------------------------------------------------------
// 9. THE NEW DIVISOR CANNOT POISON THE PHASE ACCUMULATOR (D-12 / D-02 /
//    T-33-04 / T-33-05 / T-33-11). Appended by plan 33-04.
//
//    THE DIVISOR IS `in.syncVolts - prevSyncVolts`, the first new divisor
//    src/dsp/VcoCore.hpp has acquired since Phase 32, and it has three
//    landmines. Two of them are pinned by invariant 7 above. THIS case owns the
//    third, and the third is the one an ordinary hostile-input test cannot see.
//
//    ------------------------------------------------------------------------
//    WHY THE WITHDRAWAL PHASE IS THE HALF THAT MATTERS
//    ------------------------------------------------------------------------
//    Scenario four's hostile-timing grid records a FIRST-BAD-STEP INDEX, and
//    that index cannot distinguish BAD-DURING from BAD-FOREVER. For `phase` the
//    difference is the whole defect. A not-a-number syncVolts cannot fire the
//    trigger — all three of forge::SchmittTrigger::process's comparisons are
//    false for it — so the detector looks robust. But the value IS stored, and
//    on the NEXT finite crossing the quotient is (1 - NaN)/(now - NaN) = NaN.
//    `phase` carries NO guard of its own: the header guards deltaPhase, the
//    INCREMENT, not the ACCUMULATOR. From that sample on `phase += deltaPhase`
//    stays NaN, `phase >= 1.0` is false so the wrap never fires again, and the
//    instance is dead — AFTER the hostile input has gone.
//
//    THIS IS A REAL DIFFERENCE FROM THE MorphBlep CASE PLAN 33-01 MEASURED, and
//    it is recorded because the two look identical from the plan. There the
//    "permanent poisoning" narrative was FALSIFIED: MorphBlep::step's preamble
//    drains and zeroes both accumulators unconditionally, so exactly one of the
//    following twenty samples went non-finite. Here it is TRUE, and plan 33-02
//    measured it at 200 OF 200 post-withdrawal samples with the guard removed.
//    The structural reason is one sentence: `phase` has no drain.
//
//    So a hostile-input test that only asserts finiteness DURING the hostile
//    drive books coverage it does not have. Every entry below is followed by a
//    block of wholly legitimate master and wholly legitimate timing, and that
//    block carries its own assertions.
//
//    ------------------------------------------------------------------------
//    A 33-RESEARCH PREMISE FALSIFIED HERE BY MEASUREMENT, AND CORRECTED IN
//    PLACE RATHER THAN QUIETLY DROPPED (Pitfall 7)
//    ------------------------------------------------------------------------
//    Pitfall 7 states that a band-limited master pushes the fraction OUT of
//    [0,1] and works it to 1.2 at g = 1. The subcase at the bottom of this case
//    drives exactly that construction — the research's own two-point residual,
//    transcribed rather than re-derived — and MEASURES the raw quotient at
//    every firing sample at 0.150582 .. 0.828161. It never approaches 1, let
//    alone exceeds it.
//
//    THE REASON IS STRUCTURAL AND IS WORTH MORE THAN THE CORRECTION. f > 1
//    requires (1 - prev) > (now - prev), i.e. now < 1.0. But the trigger only
//    RETURNS TRUE when now >= 1.0. And it only returns true from LOW, which
//    means the previous sample failed the same comparison, so prev < 1.0 <= now
//    and f lands in (0, 1] BY CONSTRUCTION. Pitfall 7's worked value of 1.2 is
//    computed at g = 1, where the residual drives the wrap sample to 0 V — and
//    at 0 V the trigger does not fire at all, so that fraction is never taken.
//
//    WHAT THE GUARD'S LOWER BOUND IS THEREFORE FOR, since it is NOT for this:
//    a not-a-number, and only a not-a-number. It arrives by two reachable
//    routes, both in the grid below — a NaN stored from a sample that could not
//    fire, and a -infinity stored from one that could not either, which gives
//    inf/inf on the next real crossing. The guard is load-bearing; the reason
//    written for it in the research is the wrong one. The CONCLUSION (guard
//    both ends) is unchanged and is what the pitfall is kept for.
//
//    ------------------------------------------------------------------------
//    A SECOND FALSIFIED PREMISE, THIS ONE FROM THE PLAN'S OWN PROBE, AND IT IS
//    THE SHARPER OF THE TWO: THE NEGATED PAIR IS REDUNDANT AGAINST A
//    NOT-A-NUMBER
//    ------------------------------------------------------------------------
//    Plan 33-04's acceptance criterion prescribes proving this case can fail by
//    "removing the negated lower comparison from the fraction guard". MEASURED:
//    that mutant is GREEN. Every assertion in this case passes, and so does the
//    whole suite.
//
//    The reason is that BOTH halves of `if (!(f >= 0.f) || !(f < 1.f))` are
//    negated comparisons, and a not-a-number fails BOTH `f >= 0.f` AND
//    `f < 1.f`. So EITHER half alone catches it. The pair is redundant against
//    the one input class the lower half was written for, and each half is
//    individually load-bearing only for its own FINITE out-of-range direction:
//
//        f < 0   -> the lower half. UNREACHABLE at a firing sample, by the
//                   structural argument above.
//        f >= 1  -> the upper half. REACHABLE, and invariant 7's subcase C
//                   drives it with a master sample landing exactly on 1.0 V.
//
//    So on this call site, TODAY, the lower comparison's only live duty is a
//    redundant one. It is still correct to keep it — it costs one comparison on
//    sync samples only, it is the file's standing idiom, and a future caller or
//    a future master-conditioning stage could make f < 0 reachable — but a
//    reader must not believe the two halves are independently load-bearing,
//    because a mutation probe aimed at the lower one will come back green and
//    be mistaken for a passing test rather than an insensitive one. This is the
//    same shape plan 33-02 recorded when its mutant B stayed green on case 5.
//
//    THE MUTANT THAT DOES DISCRIMINATE is the WHOLE GUARD LINE REMOVED, and its
//    red lands EXACTLY where this case's design predicts:
//
//        assertion                          hostile block   withdrawal block
//        CHECK(hFinite) / CHECK(hRange)     0 reds          -
//        CHECK(wFinite)                     -               9 reds
//        CHECK(wRange)                      -               9 reds
//        CHECK(wFirstFrac == 0.f)           -               3 reds
//
//    ZERO reds during the hostile drive and 21 after it was withdrawn. That is
//    the whole argument for the withdrawal phase, measured rather than argued:
//    a case that stopped at the hostile block would have reported SUCCESS on a
//    core whose instances were permanently dead.
//
//    AND ONE MEASURED FACT PLAN 33-05 SHOULD INHERIT RATHER THAN REDISCOVER: at
//    g = 0.96875 the residual pushes the wrap sample to 0.31 V, BELOW the high
//    threshold, so the detector fires ONE SAMPLE LATE with a fraction of
//    0.150582 instead of the ~0.99 the geometry would suggest. A late wrap
//    fraction under a band-limited master is a one-sample placement error
//    before any seam exists. That belongs in 33-05's grid.
// ---------------------------------------------------------------------------
TEST_CASE("vco sync: (D-12) the new divisor cannot poison the phase accumulator") {
	const int nArm  = 64;     // 0 V, long enough to arm the trigger LOW
	const int nHost = 256;    // the hostile value, HELD (equal consecutive samples)
	const int nWith = 512;    // the withdrawal: wholly legitimate master and timing

	for (double sr : SAMPLE_RATES) {
		CAPTURE(sr);

		forge::VcoInputs base = coreBase();
		base.pitchCV   = 0.f;
		base.morph     = 0.5f;
		base.character = 1.f;

		// --- Subcase 1: the hostile population, each entry with a WITHDRAWAL. -
		{
			// The withdrawal master starts at phim = 0, so its FIRST sample is
			// +4.92 V. That is deliberate: when the trigger is LOW coming out of
			// the hostile block, the very first legitimate sample fires with the
			// HOSTILE value still in the store — which is the only way the
			// poisoned quotient is ever computed.
			const MasterBlock w = makeMasterSaw(nWith, 1.0 / 128.0, 5.0, 0.0);

			int totalFiredAnywhere = 0;

			for (float hv : HOSTILE_SYNC) {
				for (int armed = 0; armed < 2; ++armed) {
					CAPTURE(hv);
					CAPTURE(armed);

					forge::VcoBlockDriver d(sr);

					// Phase 1 — the arming prefix, present only for armed == 1.
					if (armed) {
						SyncTrace t0;
						std::vector<float> o0 = driveTraced(d, nArm, [&](int) {
							forge::VcoInputs in = base;
							in.syncVolts     = 0.f;
							in.syncConnected = true;
							return in;
						}, t0);
						REQUIRE(o0.size() == (size_t)nArm);
					}

					// Phase 2 — the hostile value, HELD.
					SyncTrace th;
					std::vector<float> oh = driveTraced(d, nHost, [&](int) {
						forge::VcoInputs in = base;
						in.syncVolts     = hv;
						in.syncConnected = true;
						return in;
					}, th);
					REQUIRE(oh.size() == (size_t)nHost);

					// Phase 3 — THE WITHDRAWAL, on the SAME instance.
					SyncTrace tw;
					std::vector<float> ow = driveTraced(d, nWith, [&](int i) {
						forge::VcoInputs in = base;
						in.syncVolts     = w.volts[(size_t)i];
						in.syncConnected = true;
						return in;
					}, tw);
					REQUIRE(ow.size() == (size_t)nWith);

					bool  hFinite = true, hRange = true;
					float hMaxAbs = 0.f;
					int   hFired  = 0, hFirstBad = -1;
					for (int i = 0; i < nHost; ++i) {
						bool bad = false;
						if (!std::isfinite(oh[(size_t)i]))                       { hFinite = false; bad = true; }
						const float a = std::fabs(oh[(size_t)i]);
						if (a > hMaxAbs) hMaxAbs = a;
						if (a > kHostileBoundV)                                  {                  bad = true; }
						if (!(th.phase[(size_t)i] >= 0.0 && th.phase[(size_t)i] < 1.0)) { hRange = false; bad = true; }
						if (bad && hFirstBad < 0) hFirstBad = i;
						if (th.fired[(size_t)i]) ++hFired;
					}

					bool  wFinite = true, wRange = true;
					float wMaxAbs = 0.f;
					int   wFired  = 0, wFirstBad = -1;
					float wFirstFrac = -1.f;
					for (int i = 0; i < nWith; ++i) {
						bool bad = false;
						if (!std::isfinite(ow[(size_t)i]))                       { wFinite = false; bad = true; }
						const float a = std::fabs(ow[(size_t)i]);
						if (a > wMaxAbs) wMaxAbs = a;
						if (a > kHostileBoundV)                                  {                  bad = true; }
						if (!(tw.phase[(size_t)i] >= 0.0 && tw.phase[(size_t)i] < 1.0)) { wRange = false; bad = true; }
						if (bad && wFirstBad < 0) wFirstBad = i;
						if (tw.fired[(size_t)i]) {
							++wFired;
							if (wFirstFrac < 0.f) wFirstFrac = tw.frac[(size_t)i];
						}
					}
					totalFiredAnywhere += hFired + wFired;

					CAPTURE(hFired);
					CAPTURE(hMaxAbs);
					CAPTURE(hFirstBad);
					CAPTURE(wFired);
					CAPTURE(wMaxAbs);
					CAPTURE(wFirstBad);
					CAPTURE(wFirstFrac);

					// --- DURING the hostile drive. ------------------------
					INFO("hostile sync held constant across the block - the equal-consecutive-samples case D-12 names");
					CHECK(hFinite);
					CHECK(hMaxAbs <= kHostileBoundV);
					CHECK(hRange);

					// --- AFTER it is WITHDRAWN. This is the half that
					//     distinguishes this case from the existing grid.
					CHECK(wFinite);
					CHECK(wMaxAbs <= kHostileBoundV);
					CHECK(wRange);

					// The poisoning path, pinned specifically rather than
					// inferred from the finiteness above. With the trigger LOW
					// and a not-a-number in the store, the FIRST withdrawal
					// sample fires and the quotient is a not-a-number — so the
					// guard must be what is observed, and its observable
					// signature is the fallback value.
					if (armed && std::isnan(hv)) {
						CHECK(wFired > 0);
						CHECK(wFirstFrac == 0.f);
					}
					// NON-VACUITY: the legitimate control edge MUST fire. A
					// hostile grid in which nothing ever fires asserts nothing
					// about sync, however green it is.
					if (armed && hv == 2.f) {
						CHECK(hFired == 1);
					}
				}
			}

			CAPTURE(totalFiredAnywhere);
			CHECK(totalFiredAnywhere > 0);
		}

		// --- Subcase 2: the store's invariant, ASSERTED DIRECTLY. -------------
		// D-02 / Pattern 3. The invariant is one sentence — `prevSyncVolts` is
		// the voltage the trigger saw on the IMMEDIATELY PRECEDING sample — and
		// it is what makes the zero divisor unreachable. Inferring it from a
		// finite result after the fact would be the weaker claim: a store that
		// was correct on nine branches out of ten would still produce finite
		// output almost always.
		//
		// FINITE VOLTAGES ONLY IN THIS SUBCASE, and the reason is stated so it
		// does not read as a gap: a not-a-number compares unequal to itself, so
		// an equality check against a NaN supplied voltage would report a
		// mismatch on a CORRECT store. The NaN store is covered in subcase 1,
		// through the guard's fallback, which is an observable consequence
		// rather than an equality.
		{
			const int nStore = 400;
			const MasterBlock m = makeMasterSaw(nStore, 1.0 / 32.0, 5.0, 0.0);

			forge::VcoBlockDriver d(sr);
			// The first sample after construction: the NSDMI, before any step.
			CHECK(d.core.prevSyncVolts == 0.f);

			SyncTrace tr;
			std::vector<float> out = driveTraced(d, nStore, [&](int i) {
				forge::VcoInputs in = base;
				in.syncVolts = m.volts[(size_t)i];
				// Toggling every 7 samples, so the invariant is asserted on
				// UNPATCHED samples as well as patched ones. The store is
				// unconditional; a store gated on this flag is the exact defect
				// Pitfall 5 says produces the zero divisor.
				in.syncConnected = ((i / 7) % 2) == 0;
				return in;
			}, tr);
			REQUIRE(out.size() == (size_t)nStore);
			REQUIRE(tr.prevStore.size() == (size_t)nStore);

			int storeMismatch = 0, firstMismatch = -1;
			for (int i = 0; i < nStore; ++i) {
				if (tr.prevStore[(size_t)i] != m.volts[(size_t)i]) {
					++storeMismatch;
					if (firstMismatch < 0) firstMismatch = i;
				}
			}
			CAPTURE(storeMismatch);
			CAPTURE(firstMismatch);
			INFO("the store runs on EVERY sample and EVERY branch, patched or not");
			CHECK(storeMismatch == 0);
		}

		// --- Subcase 3: NEITHER the store NOR the trigger is reset on a
		//     sample-rate change — the stated choice ASSERTED, not inherited. --
		// Plan 33-02 resolved the standing discretion item in the source: a rate
		// change does not alter which sample was the previous one, so resetting
		// either would MANUFACTURE the stale-store case the guard rests against.
		// That sentence is a claim about behaviour and this is where it is
		// measured.
		//
		// HOW EACH HALF IS DISCRIMINATED, because "it still works" would not be
		// evidence of either:
		//   THE TRIGGER. The sample before the change is 0.05 V, below the low
		//   threshold, so the trigger is LOW going in. The sample AT the change
		//   is 3 V. A surviving LOW trigger takes the LOW -> HIGH arm and returns
		//   TRUE. A trigger reset to UNINITIALIZED takes the UNINITIALIZED arm,
		//   which sets HIGH and returns FALSE (RackCompat.hpp:51,53). So the fire
		//   itself is the discriminator.
		//   THE STORE. A surviving store gives (1 - 0.05)/(3 - 0.05) = 0.322034.
		//   A store reset to its NSDMI would give (1 - 0)/(3 - 0) = 0.333333. The
		//   two differ in the second decimal place and the comparison below is
		//   bit-exact, so the discrimination is real rather than nominal.
		{
			const int   nPre  = 32;
			const int   nPost = 16;
			const float vPre  = 0.05f;   // below the 0.1 V low threshold: arms LOW
			const float vPost = 3.f;

			forge::VcoBlockDriver d(44100.0);
			SyncTrace tPre;
			std::vector<float> oPre = driveTraced(d, nPre, [&](int) {
				forge::VcoInputs in = base;
				in.syncVolts     = vPre;
				in.syncConnected = true;
				return in;
			}, tPre);
			REQUIRE(oPre.size() == (size_t)nPre);

			// THE RATE CHANGE, on the SAME instance. forge::VcoBlockDriver
			// recomputes sampleTime from this on its next run(), so both timing
			// fields move together exactly as a host would move them.
			d.sampleRate = 96000.0;

			SyncTrace tPost;
			std::vector<float> oPost = driveTraced(d, nPost, [&](int) {
				forge::VcoInputs in = base;
				in.syncVolts     = vPost;
				in.syncConnected = true;
				return in;
			}, tPost);
			REQUIRE(oPost.size() == (size_t)nPost);

			const float expectFrac = (1.0f - vPre) / (vPost - vPre);
			CAPTURE(expectFrac);
			CAPTURE(tPost.frac[0]);
			CAPTURE(tPre.prevStore[(size_t)(nPre - 1)]);
			INFO("a sample-rate change resets NEITHER prevSyncVolts NOR syncTrig - plan 33-02 Decisions #3, asserted here");

			// The store carried the pre-change voltage across the transition...
			CHECK(tPre.prevStore[(size_t)(nPre - 1)] == vPre);
			// ...the trigger was still LOW, so the first post-change sample fired...
			CHECK(tPost.fired[0] != 0);
			// ...and the fraction was computed from the SURVIVING store.
			CHECK(tPost.frac[0] == expectFrac);
			// The invariant still holds on the far side of the transition.
			int postMismatch = 0;
			for (int i = 0; i < nPost; ++i) if (tPost.prevStore[(size_t)i] != vPost) ++postMismatch;
			CAPTURE(postMismatch);
			CHECK(postMismatch == 0);
		}

		// --- Subcase 4: the fraction under a BAND-LIMITED master. -------------
		// Read the falsified-premise paragraph in this case's banner first: the
		// out-of-range direction this subcase was written to find turns out to
		// be unreachable, and the measurement is what says so.
		{
			// Dyadic wrap fractions, so `phi0 = 1 - dtm*(1+g)` is exact and every
			// wrap in the block lands at the SAME g.
			static const double SYNC_G[] = {1.0 / 32, 0.25, 0.5, 0.75, 31.0 / 32};
			const double dtm = 1.0 / 128.0;
			const int    nb  = 2048;

			float spreadMin = 2.f, spreadMax = -1.f;

			for (double g : SYNC_G) {
				CAPTURE(g);
				const double      phi0 = 1.0 - dtm * (1.0 + g);
				const MasterBlock m    = makeMasterSawBandLimited(nb, dtm, 5.0, phi0);
				REQUIRE(m.totalWraps == 16);

				forge::VcoBlockDriver d(sr);
				SyncTrace tr;
				std::vector<float> out = driveTraced(d, nb, [&](int i) {
					forge::VcoInputs in = base;
					in.syncVolts     = m.volts[(size_t)i];
					in.syncConnected = true;
					return in;
				}, tr);
				REQUIRE(out.size() == (size_t)nb);

				int   fired = 0, fracOutOfRange = 0, rawOutOfRange = 0, guardFired = 0;
				bool  allFinite = true, phaseInRange = true;
				float maxAbs = 0.f, fMin = 2.f, fMax = -1.f, rawMin = 9.f, rawMax = -9.f;

				for (int i = 0; i < nb; ++i) {
					if (!std::isfinite(out[(size_t)i])) allFinite = false;
					const float a = std::fabs(out[(size_t)i]);
					if (a > maxAbs) maxAbs = a;
					if (!(tr.phase[(size_t)i] >= 0.0 && tr.phase[(size_t)i] < 1.0)) phaseInRange = false;
					if (!tr.fired[(size_t)i] || i == 0) continue;
					++fired;
					const float f = tr.frac[(size_t)i];
					if (!(f >= 0.f && f < 1.f)) ++fracOutOfRange;
					if (f < fMin) fMin = f;
					if (f > fMax) fMax = f;
					// The RAW quotient, from the two voltages this test itself
					// supplied, in float and in the header's own order.
					const float raw = (1.0f - m.volts[(size_t)(i - 1)]) / (m.volts[(size_t)i] - m.volts[(size_t)(i - 1)]);
					if (!(raw > 0.f && raw <= 1.f)) ++rawOutOfRange;
					if (raw < rawMin) rawMin = raw;
					if (raw > rawMax) rawMax = raw;
					// The guard did NOT have to intervene anywhere on this drive,
					// and that is asserted rather than assumed: the recorded
					// fraction IS the raw quotient, bit for bit.
					if (f != raw) ++guardFired;
				}

				if (fMin < spreadMin) spreadMin = fMin;
				if (fMax > spreadMax) spreadMax = fMax;

				CAPTURE(fired);
				CAPTURE(fMin);
				CAPTURE(fMax);
				CAPTURE(rawMin);
				CAPTURE(rawMax);
				CAPTURE(maxAbs);
				INFO("band-limited master: the two-point polyBLEP residual from 33-RESEARCH Pitfall 7, applied at every wrap");

				// Every wrap still produces exactly one reset — at g = 0.96875 it
				// arrives one sample LATE, which is a placement finding rather
				// than a detection failure. See the banner.
				CHECK(fired == (int)m.totalWraps);
				// The recorded fraction never leaves the guarded range...
				CHECK(fracOutOfRange == 0);
				// ...and MEASURED, neither does the raw quotient, for the
				// structural reason in the banner.
				CHECK(rawOutOfRange == 0);
				CHECK(guardFired == 0);
				CHECK(allFinite);
				CHECK(maxAbs <= kHostileBoundV);
				CHECK(phaseInRange);
			}

			// NON-VACUITY, AND THE CONTRAST THAT MAKES THIS SUBCASE WORTH ITS
			// COST. Invariant 7 measured the HARD-EDGED master's fraction moving
			// by 0.004 while the true wrap fraction halved. Here the fraction
			// spans 0.150582 .. 0.828161 — MEASURED spread 0.677579, two orders
			// of magnitude larger. A band-limited master is what makes the
			// sub-sample solve carry information at all, and that is exactly
			// 33-RESEARCH Pitfall 10's warning to plan 33-05's placement grid,
			// stated as a number this suite reproduces on every run.
			CAPTURE(spreadMin);
			CAPTURE(spreadMax);
			CHECK(spreadMax - spreadMin > 0.3f);
		}
	}
}

// ---------------------------------------------------------------------------
// 10. SC-3 — THE PER-SAMPLE STEP ACROSS A RESET, BOUNDED BY A MEASURED
//     ENVELOPE (SC-3 / D-10). Appended by plan 33-08; nothing above it was
//     renumbered.
//
//     ------------------------------------------------------------------------
//     WHAT THIS CRITERION IS NOT, STATED BEFORE ANYTHING IS ASSERTED
//     ------------------------------------------------------------------------
//     >>> THIS IS NOT A CLAIM THAT THE STEP ACROSS A RESET IS SMALL. IT CANNOT
//         BE, AND A PLAN THAT READ SC-3 THAT WAY WOULD BE ASSERTING SOMETHING
//         FALSE ABOUT CORRECT BEHAVIOUR. <<<
//     A legitimate hard-sync reset at a slave AT OR BELOW its master's rate
//     truncates a cycle that has barely started and steps the output by nearly
//     its full peak-to-peak range IN ONE SAMPLE. That is not an artefact; it is
//     hard sync working. MEASURED on this grid: the worst step on a reset
//     sample is 9.793601 V out of a naive +/-5 V waveform, and the case below
//     ASSERTS that the worst step exceeds 9.0 V so that the sentence you are
//     reading cannot quietly stop being true.
//
//     What SC-3 forbids is a full-scale ARTEFACT — a discontinuity the
//     band-limiting failed to absorb — which is a DIFFERENT QUANTITY from the
//     step itself. This case bounds the step with a measured ENVELOPE; the
//     evidence that the band-limiting is doing something lives in invariant 11,
//     which compares two measurements and consults no pinned number at all.
//
//     ------------------------------------------------------------------------
//     WHY THE INSTRUMENT IS TIME-DOMAIN, MEASURED RATHER THAN PREFERRED
//     ------------------------------------------------------------------------
//     The obvious home for a sync gate is tests/test_vco_spectrum.cpp, which
//     since plan 33-07 gates 210 sync cells against per-cell alias-floor
//     thresholds. IT IS STRUCTURALLY BLIND TO THE ARTEFACT SC-3 NAMES. Phase
//     32's register item 5 MEASURED a single-sample full-amplitude spike at
//     0.0 dB on that metric — exactly zero difference, completely invisible.
//     A sync gate built only on the alias floor would therefore be GREEN on
//     precisely the click SC-3 exists to forbid, and plan 33-07 wrote that
//     blindness into its own source as the reason it REFUSED an
//     improvement-shaped spectral gate. This case is the other instrument.
//
//     ------------------------------------------------------------------------
//     AND THE ANALYTIC BOUND IS REJECTED IN WRITING, NOT LEFT UNCONSIDERED
//     ------------------------------------------------------------------------
//     There is a bound available for free. Every sample is inside
//     kHostileBoundV, so |x[n] - x[n-1]| <= 2 * kHostileBoundV = 20.0 V,
//     permanently, with no measurement and no maintenance. IT IS CLOSE TO
//     VACUOUS, AND THE REASON IS THE CRITERION ITSELF: a full-scale artefact —
//     a +/-5 V waveform jumping to the opposite rail in one sample — is a step
//     of about 10 V, which a 20 V bound ADMITS. A bound that admits the thing
//     it exists to forbid is coverage, not evidence. The case below asserts
//     mechanically that the pinned bound is strictly inside the analytic one,
//     so this paragraph cannot become false without going red.
//
//     ------------------------------------------------------------------------
//     THE BOUND — PROVENANCE, IN THE SHAPE INVARIANT 2 USES FOR ITS TIERS
//     ------------------------------------------------------------------------
//     MEASURED by plan 33-08 in this repository, on the SHIPPED past-edge leg
//     (forge::VcoCore calling forge::MorphBlep::addPastStep since plan 33-06),
//     over all 420 cells of the sweep below with the bound temporarily raised
//     so nothing could fire:
//
//         grid worst |x[n] - x[n-1]| on a reset sample   9.793601 V
//         the cell it came from   44.1 kHz, band-limited master, ratio 0.50,
//                                 the 5 % pulse centre, character 0.00
//         per rate (44.1 / 48 / 96 kHz)   9.793601 / 9.793601 / 9.793601 V
//
//     >>> THE ENVELOPE IS RATE-INDEPENDENT TO SIX DECIMAL PLACES, AND THAT IS
//         RECORDED RATHER THAN ASSUMED. <<< It is the same cell and the same
//         value at all three rates, because the grid is parametrised by master
//         cycles PER SAMPLE rather than by hertz — the same construction
//         invariant 8 uses, and the same reason it works.
//
//     THE SAME SWEEP, WITH THE SYNC CORRECTION WITHHELD (the diagnostic pair,
//     reconstructed per sample from tel.syncCorrection in the SAME pass):
//
//         grid worst on the withheld leg                10.000000 V
//         per rate                       10.000000 / 10.000000 / 10.000000 V
//
//     PINNED AT 9.90 V, and the derivation is TWO-SIDED, which is what stops it
//     being either a restatement of the implementation or the analytic bound in
//     disguise:
//       (a) it must be AT OR ABOVE the measured worst, 9.793601 V; and
//       (b) it must be STRICTLY BELOW what a seam-free core measures on this
//           same grid, 10.000000 V, or the bound could not tell a core with the
//           sync BLEP from a core without it.
//     The admissible interval is therefore [9.793601, 10.000000), whose
//     midpoint is 9.896800; ROUNDED OUTWARD — upward, since this is an upper
//     bound — to the nearest hundredth of a volt, giving 9.90 V. Headroom above
//     the measurement: 0.106 V. Clearance below the withheld leg: 0.100 V.
//
//     >>> THE HONEST READING OF THAT INTERVAL, WHICH IS NARROW ON PURPOSE AND
//         MUST NOT BE WIDENED INTO COMFORT. <<< There are only 0.206 V between
//         what the shipped leg does at this grid's worst cell and what a core
//         with NO sync correction at all does there. The seam buys about two
//         percent of the step at the worst cell. That is a small number and it
//         is stated plainly rather than dressed up: the sync BLEP's benefit is
//         real, it is asserted per cell in invariant 11 over a stated
//         population, and it is NOT large. Widening this bound to a round 10.0
//         or 11.0 V would delete constraint (b) and turn the case into the
//         vacuous analytic bound the paragraph above rejects.
//
//     EVERY NUMBER ABOVE IS AN APPLE-CLANG FIGURE, like every other decibel and
//     volt this phase has recorded. `make strict` passes locally at C++11
//     -pedantic-errors; the CI MinGW leg is plan 33-11's.
// ---------------------------------------------------------------------------
TEST_CASE("vco sync: (SC-3 / D-10) the per-sample step across a reset is bounded by a measured envelope") {
	// THE BOUND. Local to this case, exactly as scenario five's exercise floor
	// is, and deliberately NOT hoisted to namespace scope: invariant 11 must be
	// able to say it consults no pinned number, and the cheapest way to make
	// that structural rather than a promise is for this constant not to be in
	// scope there at all.
	const float kSyncResetDeltaBoundV = 9.90f;

	// THE EXERCISE FLOOR. Without it the bound is decoration — see scenario
	// five's banner for the same argument applied to the hostile tier. This is
	// also the assertable form of the sentence this case opens with: a
	// legitimate reset really does step the output by nearly its full range.
	// Pinned from the measured worst of 9.793601 V, less a 0.79 V cushion.
	const float kSyncResetDeltaFloorV = 9.0f;

	// THE ANALYTIC BOUND THE BANNER REJECTS, written as an expression of the
	// suite's own outer tier rather than as a literal, so it moves with it.
	const float kSyncAnalyticDeltaBoundV = 2.f * kHostileBoundV;   // 20.0 V

	// --- THE RECORDER'S FLAG IS forge::VcoCore::Telemetry::syncFired, AND THAT
	//     IS ASSERTED HERE RATHER THAN TRUSTED FROM THE HELPER --------------
	// Every reset sample in this case and in invariant 11 is identified from
	// the TELEMETRY FLAG and never inferred from the waveform, because
	// inferring "a large step means a reset" would be circular in a case whose
	// entire subject is how large the step at a reset is. SyncTrace records
	// that flag with a deliberate off-by-one (see its banner), so the linkage
	// between the recorder and the core's own `tel` is checked on a live core
	// before anything is measured through it — the same validity-first REQUIRE
	// habit invariant 4 uses on its interleave helper.
	{
		const int nv = 512;   // 4 master wraps at dtm = 1/128
		const MasterBlock mv = makeMasterSaw(nv, kSyncD10Dtm, 5.0, 0.0);
		forge::VcoInputs bv = coreBase();
		bv.morph     = 0.5f;
		bv.character = 1.f;

		forge::VcoBlockDriver dv(SAMPLE_RATES[0]);
		SyncTrace tv;
		std::vector<float> ov = driveTraced(dv, nv, [&](int i) {
			forge::VcoInputs in = bv;
			in.syncVolts     = mv.volts[(size_t)i];
			in.syncConnected = true;
			return in;
		}, tv);
		REQUIRE(ov.size() == (size_t)nv);
		REQUIRE(tv.fired.size() == (size_t)nv);

		// The final entry is recorded from the live core AFTER run() returns,
		// so all three members can be compared against the core's own telemetry
		// directly. Exact equality, never a tolerance: this is an identity
		// claim about a recorder, not a measurement.
		CHECK(tv.fired[(size_t)(nv - 1)]      == (dv.core.tel.syncFired ? (char)1 : (char)0));
		CHECK(tv.correction[(size_t)(nv - 1)] == dv.core.tel.syncCorrection);
		CHECK(tv.jump[(size_t)(nv - 1)]       == dv.core.tel.syncJump);

		// And the flag is not stuck at either value. MEASURED 4 resets in 512
		// samples, one per master wrap.
		int firedHere = 0;
		for (int i = 0; i < nv; ++i) if (tv.fired[(size_t)i]) ++firedHere;
		CAPTURE(firedHere);
		REQUIRE(firedHere == 4);
	}

	const std::vector<SyncDeltaCell> grid = sweepSyncDeltaGrid();
	REQUIRE(grid.size() == 420u);

	double gridWorstCorrected = 0.0;
	double gridWorstWithheld  = 0.0;
	double perRateCorrected[3] = {0.0, 0.0, 0.0};
	double perRateWithheld[3]  = {0.0, 0.0, 0.0};
	int    totalResets    = 0;
	int    cellsWithNoReset = 0;
	int    cellsNotFinite   = 0;
	std::string worstCellLabel;

	for (size_t ci = 0; ci < grid.size(); ++ci) {
		const SyncDeltaCell& c = grid[ci];
		if (!c.allFinite)     ++cellsNotFinite;
		if (c.obs.empty())    ++cellsWithNoReset;
		totalResets += c.resets;

		// k = 1 is the shipped leg; k = 0 is the leg with the seam's deposit
		// withheld, reconstructed from tel.syncCorrection in this same pass.
		const double wCorrected = worstResetDeltaAt(c, 1.0);
		const double wWithheld  = worstResetDeltaAt(c, 0.0);

		const int r = (c.sr == 44100.0) ? 0 : (c.sr == 48000.0 ? 1 : 2);
		if (wCorrected > perRateCorrected[r]) perRateCorrected[r] = wCorrected;
		if (wWithheld  > perRateWithheld[r])  perRateWithheld[r]  = wWithheld;
		if (wWithheld  > gridWorstWithheld)   gridWorstWithheld   = wWithheld;
		if (wCorrected > gridWorstCorrected) {
			gridWorstCorrected = wCorrected;
			// std::string, never a bare const char*: doctest stringifies a
			// pointer as a hex address, which is the failure plan 33-07 spent a
			// debugging session on with a 420-cell gate exactly like this one.
			worstCellLabel = std::string("sr ") + std::to_string((long)c.sr) + " / " + c.edgeName
			               + " / ratio " + std::to_string(c.ratio) + " / " + c.region
			               + " / character " + std::to_string(c.character);
		}
	}

	// --- NON-VACUITY, ASSERTED BEFORE ANY VALUE CLAIM ---------------------
	// A sweep in which nothing ever synced would satisfy every bound below by
	// having no reset samples to bound. MEASURED 13,230 resets: 32 per cell on
	// the 210 hard-edge cells and 31 on the 210 band-limited ones, where the
	// polyBLEP residual applied to the sample BEFORE the first wrap moves that
	// wrap's detection out of the block.
	CAPTURE(totalResets);
	CAPTURE(cellsWithNoReset);
	REQUIRE(totalResets == 13230);
	REQUIRE(cellsWithNoReset == 0);
	CHECK(cellsNotFinite == 0);

	CAPTURE(gridWorstCorrected);
	CAPTURE(gridWorstWithheld);
	INFO("worst corrected reset delta at: " << worstCellLabel);

	// --- THE BOUND --------------------------------------------------------
	// MEASURED 9.793601 V against 9.90 V, clearing by 0.106 V.
	CHECK(gridWorstCorrected <= kSyncResetDeltaBoundV);

	// --- THE BOUND IS EXERCISED, NOT MERELY SATISFIED ---------------------
	// This is the "it is an ENVELOPE, not a smallness claim" sentence made
	// assertable. If a future change brought the worst reset step under 9.0 V
	// this goes RED and asks whether hard sync is still resetting the phase.
	CHECK(gridWorstCorrected > kSyncResetDeltaFloorV);

	// --- THE BOUND IS NOT THE ANALYTIC ONE --------------------------------
	// Mechanically asserted so the rejection paragraph cannot rot.
	CHECK(kSyncResetDeltaBoundV < kSyncAnalyticDeltaBoundV);

	// --- THE BOUND CAN FAIL, AND THAT IS MEASURED HERE RATHER THAN CLAIMED -
	// A bound pinned from an implementation's own output cannot fail by
	// construction. This is the other half: the SAME grid with the seam's
	// deposit withheld measures 10.000000 V, which is ABOVE the pinned bound —
	// so a core that stopped calling forge::MorphBlep::addPastStep would turn
	// this case red on the very next run, without anyone editing a test.
	//
	// THIS IS A STATEMENT ABOUT THE BOUND'S FALSIFIABILITY, NOT ABOUT THE
	// CORRECTION'S QUALITY. It is grid-wide and one-sided. The per-cell claim
	// that the correction actually helps — over a stated population, with the
	// population where it does NOT help asserted alongside it — is invariant 11
	// and is deliberately not made here.
	CHECK(gridWorstWithheld > kSyncResetDeltaBoundV);

	// --- PER RATE, RECORDED AND ASSERTED ----------------------------------
	// Recorded separately so a later phase can see whether the envelope is
	// rate-dependent. MEASURED: it is not — 9.793601 V at all three rates, from
	// the same cell, because the grid is parametrised by master cycles per
	// sample rather than by hertz.
	for (int r = 0; r < 3; ++r) {
		CAPTURE(SAMPLE_RATES[r]);
		CAPTURE(perRateCorrected[r]);
		CAPTURE(perRateWithheld[r]);
		INFO("per-rate SC-3 envelope; MEASURED 9.793601 corrected and 10.000000 withheld at every rate");
		CHECK(perRateCorrected[r] <= kSyncResetDeltaBoundV);
		CHECK(perRateCorrected[r] > kSyncResetDeltaFloorV);
		CHECK(perRateWithheld[r]  > kSyncResetDeltaBoundV);
	}
}
