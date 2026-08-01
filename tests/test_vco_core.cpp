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
//      (D-17), carrying all five measured non-vacuity preconditions
//   5. the permanent POSITIVE CONTROL for invariant 4: a deliberately-broken
//      stand-in core that shares one static phase accumulator between instances
//      is required to FAIL the same check, through the same helper (D-17)
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

		const float p = (float)sharedPhase;

		// MIRRORED FROM src/dsp/VcoCore.hpp:597-602 (plan 32-06, T-32-01). The
		// comparison-ladder forge::clamp that used to sit here is transparent to
		// a not-a-number, and the frozen forge::Waveshape::morphedWave casts
		// `morph * 4.f` to int. The real core replaced the ladder with this
		// NEGATED-comparison pair — negation FIRST as the NaN catcher — so this
		// mirror does too.
		float morph = in.morph;
		if (!(morph > 0.f)) morph = 0.f;
		if (morph > 1.f) morph = 1.f;
		float character = in.character;
		if (!(character > 0.f)) character = 0.f;
		if (character > 1.f) character = 1.f;

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
	// THE OUTER BOUND. Applies to EVERY scenario below, with no exceptions, and
	// to every scenario any later plan adds to this suite.
	const float kHostileBoundV = 10.0f;
	// THE TIGHTER, ADDITIONAL BOUND. Asserted ON TOP OF the outer one wherever
	// the measurement in this case's banner entitles a scenario to it — never
	// instead of it.
	const float kMusicalBoundV = 5.55f;

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
	// The seeds are the proven non-degenerate literals used everywhere else in
	// this suite. NEVER a pair of zeros (T-30-02): a degenerate Xoroshiro seed
	// is a fixed point emitting an all-zero stream, which makes
	// std::normal_distribution's rejection loop never terminate — in Rack that
	// is a hang on patch load, not a failing test.
	{
		// -44100 is the reproduced CR-01 case; 0 is the other non-positive rate;
		// 44100 is the legitimate CONTROL rate; NaN is what a mis-wired host or
		// an uninitialised ProcessArgs would deliver.
		static const float HOSTILE_RATES[] = {
			-44100.f, 0.f, 44100.f, std::numeric_limits<float>::quiet_NaN()
		};
		// 1/44100 paired with 44100 is the one legitimate control point. 1/1000
		// and 999 are DECOUPLED from every rate above — the shape Phase 32's
		// oversampled inner loop will produce naturally.
		static const float HOSTILE_TIMES[] = {
			-1.f / 44100.f, 0.f, 1.f / 44100.f, 1.f / 1000.f, 999.f,
			std::numeric_limits<float>::quiet_NaN()
		};
		// Named _T4 on purpose: HOSTILE_PITCH is already taken by scenario three
		// inside this same TEST_CASE.
		static const float HOSTILE_PITCH_T4[] = {0.f, 10.f};

		// The step count both the code reviewer and the verifier reproduced at.
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

					// ACCUMULATED, not asserted per sample: 48 configs at 20000
					// steps would otherwise add nearly four million assertions to
					// a 2.6 million assertion suite. Same idiom as scenario three.
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
					// OUTER tier, then the tighter one. MEASURED across all 48
					// configurations at 5.000000 V exactly — the frozen-phase DC
					// value the guards produce when a hostile rate or time
					// zeroes the increment — so this grid is entitled to the
					// musical tier with 0.55 V to spare. The four named
					// assertions beside these are UNTOUCHED and none is merged:
					// the banner above explains why no one of them is redundant
					// with its neighbours.
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
		const std::function<forge::VcoInputs(int)> inA = [=](int i) {
			forge::VcoInputs in = base;
			in.pitchCV = -1.f + 2.f * ((float)i / denom);   // -1 V .. +1 V
			in.morph   = 0.25f;
			return in;
		};
		const std::function<forge::VcoInputs(int)> inB = [=](int i) {
			forge::VcoInputs in = base;
			in.pitchCV = 0.5f;                              // a different, FIXED pitch
			in.morph   = (float)i / denom;                  // 0 .. 1
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
		std::vector<float> harnessA = d.run(n, inA);
		REQUIRE(harnessA.size() == r.soloA.size());
		bool helperMatchesHarness = true;
		for (size_t i = 0; i < harnessA.size(); ++i) {
			if (harnessA[i] != r.soloA[i]) { helperMatchesHarness = false; break; }
		}
		REQUIRE(helperMatchesHarness);

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
