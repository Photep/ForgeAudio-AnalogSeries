// tests/test_vco_spectrum.cpp
//
// Phase 32's spectral apparatus: the TEST-03 alias-floor gate and the D-08
// naive baseline it is measured against. This file lands BEFORE one line of
// band-limiting exists, and that ordering is the point (D-08). A gate written
// against already-passing code proves nothing, so the DFT, the alias-bin
// classifier, the bin-centred frequency solver and the naive mirror all have to
// exist and be validated first. Plan 32-03 runs the baseline sweep through this
// apparatus and records the RED; plan 32-06 lands forge::MorphBlep and turns it
// green.
//
// ---------------------------------------------------------------------------
// THE D-10 GATE-CORRECTNESS ARGUMENT, IN FULL. Read this before changing any
// number in this file.
//
// The analysis block is N = 4096 samples and the oscillator is driven at
// EXACTLY K cycles per block. That single choice is what makes the measurement
// trustworthy, and it does so in one step: with an integer number of cycles per
// block the block is exactly periodic, so the RECTANGULAR window is not an
// approximation of an infinite signal — it is exact. Leakage is not "small", it
// is ZERO. Every true harmonic and every folded alias lands dead on a bin
// CENTRE. "Alias energy" therefore reduces to something with no free
// parameters: the magnitude at the non-harmonic bins. No window coefficients,
// no guard bands either side of a harmonic, no floor that depends on which
// window someone picked. A Blackman-Harris window was REJECTED for exactly this
// reason (D-10): it would have made the achievable floor a property of the
// window rather than a property of the DSP.
//
// THE COPRIMALITY ARGUMENT — the whole justification for the bin
// classification. N = 4096 = 2^12, so gcd(K, N) = 1 for EVERY odd K; a power of
// two has no odd divisors at all. Harmonic n of the waveform sits at bin nK. A
// harmonic above Nyquist folds back and lands on bin (nK mod N), reflected
// about N/2. An alias can therefore only be MISTAKEN for a true harmonic if
// some folded harmonic m lands on the bin of some true harmonic n — that is, if
// N divides (n +/- m)K. With gcd(K, N) = 1 that requires N | (n +/- m), i.e.
// |n +/- m| >= 4096. No waveform this oscillator produces carries a 4096th
// harmonic anywhere near the fundamental's amplitude, so the collision never
// happens. Choosing N a power of two reduces D-10's coprimality requirement to
// "pick an odd number", and that is why every K in the grid below is odd and
// why this file ASSERTS their oddness mechanically rather than trusting the
// table. The assertion is the mechanical form of this whole paragraph.
//
// THE TEST FREQUENCIES SIT AT BIN CENTRES, NOT AT EQUAL-TEMPERED NOTES. K = 389
// at 44.1 kHz is 4188.2 Hz; the real C8 is 4186.0 Hz. That 2.2 Hz difference is
// IRRELEVANT to aliasing behavior — folding depends on the ratio of the
// harmonic frequency to Nyquist, which moves by 0.05 % here — and it is
// load-bearing for the measurement, because an exact equal-tempered frequency
// would put a non-integer number of cycles in the block and reintroduce every
// window artefact the paragraph above exists to eliminate. A LATER AGENT MUST
// NOT "FIX" THESE FREQUENCIES TO THE EXACT NOTES. Doing so silently converts a
// zero-leakage measurement into a windowed one and makes every threshold in
// plans 32-03 and 32-07 a statement about the window.
//
// ---------------------------------------------------------------------------
// WHAT MAKES THIS SUITE VALIDATED RATHER THAN MERELY GREEN. The alias
// classifier is proven by DETECTING a spur planted at a known amplitude in a
// known bin, not by having been observed green. A detector that has only ever
// been seen passing is indistinguishable from one that cannot fail — the
// standing posture of check_frozen.sh [3/3], check_includes.sh [6/7],
// check_canary.sh [4/5] and tests/test_vco_core.cpp invariant 5. Part B below
// is that fixture, it runs on every invocation, and IT MUST NEVER BE DELETED.
//
// This TU does NOT define the doctest impl macro (tests/main.cpp owns it).

#include "doctest.h"

#include "VcoBlockDriver.hpp"

#include <vector>     // std::vector — the sample block and the FFT scratch buffer
#include <complex>    // std::complex<double> — the FFT's working element type
#include <cmath>      // std::log10 — the dB conversion every metric here ends in
#include <cstddef>    // std::size_t — the FFT's index and length type
#include <cstdint>    // uint64_t — NaiveVcoCoreMirror's seeding entry points
#include <string>     // std::string — readable CAPTURE of the grid's label columns

namespace {

// ---------------------------------------------------------------------------
// Pinned parameters (32-VALIDATION.md § "Spectral Construction (D-10)").
// ---------------------------------------------------------------------------

// Block length. 4096 = 2^12: see the coprimality argument in the file banner.
// At 44.1 kHz that is 92.9 ms — long enough for 97 cycles of the lowest test
// note and short enough that the full sweep in plan 32-03 runs in seconds.
constexpr int kSpectrumN = 4096;

// The three production sample rates, matching tests/test_vco_core.cpp:93. Held
// here rather than shared with that TU on purpose: both live in anonymous
// namespaces with internal linkage, so there is no ODR hazard and no coupling
// between two files that must be free to diverge.
constexpr double SAMPLE_RATES[] = {44100.0, 48000.0, 96000.0};

// pi to double precision. <cmath>'s M_PI is not guaranteed by the standard and
// is absent under -std=c++11 -pedantic on some toolchains; the strict gate
// compiles src/ that way and this file is written to the same habits.
constexpr double kPi = 3.14159265358979323846;

// ---------------------------------------------------------------------------
// fftRadix2 — in-place iterative radix-2 decimation-in-time complex FFT.
//
// WHY A HAND-ROLLED FFT AND WHY IT IS SOUND. This is the textbook Cooley-Tukey
// arrangement: a bit-reversal permutation followed by log2(n) butterfly stages
// with twiddles taken from std::cos / std::sin. Everything is double. It is
// validated in Part A of the case below by a synthetic whose exact spectrum is
// known in closed form — a pure cosine at an integer cycle count, whose energy
// must land entirely on two bins — and in Part B by a second synthetic with a
// spur planted at a known amplitude and bin, which the classifier must report
// to within 0.1 dB. That is what makes it evidence rather than a hope.
//
// libm IS AVAILABLE HERE. std::cos, std::sin and std::log10 are used freely,
// which would be forbidden under src/ (the VCO seam uses the frozen
// forge::exp2_taylor5 and nothing from libm, so bit-identity of the FM path
// cannot depend on a platform math library). This is tests/, not src/ — the
// D-18 precedent — and the distinction is the reason the analysis code lives in
// a test TU instead of beside the DSP.
//
// PRECONDITION: a.size() must be a power of two. There is no REQUIRE here
// because this helper is called from non-test code paths too (the solvers
// below do not use it, but plan 32-03's sweep will); a non-power-of-two size
// returns the input UNCHANGED, which the caller's own magnitude assertions
// cannot mistake for a valid spectrum.
// ---------------------------------------------------------------------------
void fftRadix2(std::vector<std::complex<double> >& a) {
	const std::size_t n = a.size();
	if (n < 2) return;
	if ((n & (n - 1)) != 0) return;   // not a power of two — see the precondition above

	// Bit-reversal permutation.
	for (std::size_t i = 1, j = 0; i < n; ++i) {
		std::size_t bit = n >> 1;
		for (; (j & bit) != 0; bit >>= 1) j ^= bit;
		j ^= bit;
		if (i < j) {
			const std::complex<double> t = a[i];
			a[i] = a[j];
			a[j] = t;
		}
	}

	// Butterfly stages.
	for (std::size_t len = 2; len <= n; len <<= 1) {
		const double ang = -2.0 * kPi / (double)len;
		const std::size_t half = len >> 1;
		for (std::size_t i = 0; i < n; i += len) {
			for (std::size_t k = 0; k < half; ++k) {
				const double th = ang * (double)k;
				const std::complex<double> w(std::cos(th), std::sin(th));
				const std::complex<double> u = a[i + k];
				const std::complex<double> v = a[i + k + half] * w;
				a[i + k]        = u + v;
				a[i + k + half] = u - v;
			}
		}
	}
}

// ---------------------------------------------------------------------------
// aliasPeakDb — the alias metric, in the exact form 32-VALIDATION.md pins:
//
//     aliasPeak_dB = 20*log10( max_{i in [1, N/2] \ H} |X_i| / |X_K| )
//     H = { n*K : 1 <= n <= floor((N/2 - 1)/K) }
//
// H is the set of TRUE harmonic bins that fit below Nyquist. Everything else in
// [1, N/2] is alias energy by the coprimality argument in the file banner — not
// by a heuristic about which bins "look like" harmonics.
//
// BIN 0 IS EXCLUDED, DELIBERATELY. The narrow pulse legitimately carries DC:
// a 5 %-duty pulse has a mean nowhere near zero, and that is a property of the
// waveform, not an aliasing artefact. Counting it would make the pulse rows
// fail for a reason that has nothing to do with band-limiting. The DC blocker
// is Phase 34's decision to make (OUT-02), and until it does, DC is not this
// gate's business.
//
// THE PEAK IS THE GATE, THE RMS IS THE DIAGNOSTIC. aliasRmsDbOut reports the
// root-mean-square over the same excluded set. A shape whose peak barely moves
// while its RMS drops by 10 dB is telling plan 32-06 something real about where
// the correction went, but it is not what the threshold asserts.
//
// SENTINEL: if the fundamental bin has zero magnitude there is no ratio to
// report and the caller is measuring silence, so -999.0 is returned. It is far
// below any threshold this suite will assert, which would be the wrong
// direction for a sentinel to fail — hence the non-vacuity REQUIREs every
// caller in this file and in plan 32-03 carries BEFORE reading this value.
// ---------------------------------------------------------------------------
double aliasPeakDb(const std::vector<float>& block, int K,
                   int* aliasBinOut, double* aliasRmsDbOut) {
	const std::size_t n = block.size();
	if (aliasBinOut) *aliasBinOut = -1;
	if (aliasRmsDbOut) *aliasRmsDbOut = -999.0;
	if (n < 2 || K < 1) return -999.0;

	std::vector<std::complex<double> > x;
	x.reserve(n);
	for (std::size_t i = 0; i < n; ++i) x.push_back(std::complex<double>((double)block[i], 0.0));
	fftRadix2(x);

	const int half = (int)(n / 2);
	const double fund = std::abs(x[(std::size_t)K]);
	if (!(fund > 0.0)) return -999.0;

	const int maxHarmonic = (half - 1) / K;   // the count of true harmonics below Nyquist

	double peak = 0.0;
	int peakBin = -1;
	double sumSq = 0.0;
	int count = 0;
	for (int i = 1; i <= half; ++i) {
		// Skip the true-harmonic bins H = { n*K }.
		if ((i % K) == 0 && (i / K) <= maxHarmonic) continue;
		const double m = std::abs(x[(std::size_t)i]);
		if (m > peak) { peak = m; peakBin = i; }
		sumSq += m * m;
		++count;
	}
	if (aliasBinOut) *aliasBinOut = peakBin;
	if (aliasRmsDbOut && count > 0) {
		const double rms = std::sqrt(sumSq / (double)count);
		*aliasRmsDbOut = (rms > 0.0) ? 20.0 * std::log10(rms / fund) : -999.0;
	}
	if (!(peak > 0.0)) return -999.0;
	return 20.0 * std::log10(peak / fund);
}

// ---------------------------------------------------------------------------
// deltaPhaseForPitchCV — forge::VcoCore::step()'s pitch/guard/accumulate chain,
// replicated EXACTLY, for the two solvers below.
//
// It mirrors src/dsp/VcoCore.hpp:339-472 with the shell's neutral inputs
// (coarse = 0, fine = 0, the FM jack unpatched, so the volt-domain summation
// reduces to the pitch volt): the D-14 bound with the NEGATED comparison FIRST
// as the NaN catcher, then EXACTLY ONE forge::exp2_taylor5 off
// forge::kVcoFreqC4, then the sanitised rate, then the Nyquist ceiling followed
// by the negated frequency floor IN THAT ORDER (the floor must be the last
// writer — CR-01), then deltaPhase = (double)freq * (double)dt with the negated
// floor and the forge::kVcoMaxDeltaPhase ceiling.
//
// THIS IS A MIRROR AND MUST BE KEPT IN STEP WITH THE REAL CORE, exactly like
// tests/test_vco_core.cpp's DeliberatelyBrokenSharedStateCore. It is not
// asserted to be a mirror by inspection: Part C below drives the LIVE
// forge::VcoCore through forge::VcoBlockDriver at each solved pitchCV and
// REQUIREs the frequency the core actually computed to reproduce this
// function's prediction BIT-EXACTLY. A solver that quietly drifted from the
// core would place the drive frequency off the bin centre and quietly
// reintroduce leakage — the failure this file exists to prevent — and nothing
// downstream would notice, because a leaky spectrum still looks like a
// spectrum. That REQUIRE is what makes the drift visible.
// ---------------------------------------------------------------------------
double deltaPhaseForPitchCV(float pitchCV, double sr, float dt, float* freqOut) {
	float pitchVolts = pitchCV;
	if (!(pitchVolts > -forge::kVcoMaxPitchVolts)) pitchVolts = -forge::kVcoMaxPitchVolts;
	if (pitchVolts > forge::kVcoMaxPitchVolts) pitchVolts = forge::kVcoMaxPitchVolts;

	float freq = forge::kVcoFreqC4 * forge::exp2_taylor5(pitchVolts);

	const float safeRate = ((float)sr > 0.f) ? (float)sr : 0.f;
	const float maxFreq = forge::kVcoNyquistGuardFrac * safeRate;
	if (freq > maxFreq) freq = maxFreq;
	if (!(freq > 0.f)) freq = 0.f;
	if (freqOut) *freqOut = freq;

	double deltaPhase = (double)freq * (double)dt;
	if (!(deltaPhase > 0.0)) deltaPhase = 0.0;
	if (deltaPhase > forge::kVcoMaxDeltaPhase) deltaPhase = forge::kVcoMaxDeltaPhase;
	return deltaPhase;
}

// ---------------------------------------------------------------------------
// binCentredPitchCV — METHOD ONE, and the one that keeps the shared harness.
//
// Solves for the pitchCV whose resulting deltaPhase sits on the bin centre
// K / kSpectrumN, driving forge::VcoBlockDriver COMPLETELY UNCHANGED. The
// driver's per-sample overwrite of sampleTime and sampleRate
// (tests/VcoBlockDriver.hpp:56-60) is unconditional and documented as
// load-bearing; it must never become conditional, so the only free variable
// left is pitchCV, and this function bisects it.
//
// Bisection over [-2, +8] volts for 60 iterations (deltaPhase is monotone
// non-decreasing in pitchCV over that interval, which is what makes bisection
// valid), then a +/-4096-ULP scan around the resulting float with
// std::nextafterf keeping whichever candidate minimises |deltaPhase - K/N|.
// The achieved error is reported IN BINS through achievedBinErrorOut.
//
// THE EXPECTED ENVELOPE IS 2.3e-4 TO 1.5e-3 BINS. 32-RESEARCH.md § Validation
// Architecture measured exactly that range with exactly this method, and this
// session reproduced it row for row (see the table above the grid in Part C).
// If a future run lands OUTSIDE that envelope, that is a finding about
// forge::exp2_taylor5 or about the guard chain, not a number to update here.
//
// WHY IT CANNOT DO BETTER, MEASURED. The limit is not the bisection and not the
// ULP scan: it is forge::exp2_taylor5's OUTPUT GRANULARITY. At 44.1 kHz,
// K = 389, consecutive pitchCV floats either side of the solution produce
// frequencies of 4188.180175781 and 4188.224121094 Hz and NOTHING IN BETWEEN —
// a 0.0439 Hz step, which is 90 float ULPs at that magnitude and 4.08e-3 bins
// wide. Six ULPs either side of the solution give bit-identical frequencies.
// The best reachable error is therefore half that step, and scanning further is
// wasted work. THIS IS PRECISELY D-10's NAMED FAILURE MODE — a gate that
// measures forge::exp2_taylor5's output granularity instead of the DSP — which
// is why method two exists and why Part C asserts against it.
// ---------------------------------------------------------------------------
float binCentredPitchCV(double sr, int K, double* achievedBinErrorOut) {
	const double target = (double)K / (double)kSpectrumN;
	const float dt = (float)(1.0 / sr);   // the value forge::VcoBlockDriver injects

	double lo = -2.0, hi = 8.0;
	for (int it = 0; it < 60; ++it) {
		const double mid = 0.5 * (lo + hi);
		if (deltaPhaseForPitchCV((float)mid, sr, dt, 0) < target) lo = mid; else hi = mid;
	}

	const float start = (float)(0.5 * (lo + hi));
	float best = start;
	double bestErr = std::fabs(deltaPhaseForPitchCV(start, sr, dt, 0) - target);
	for (int dir = -1; dir <= 1; dir += 2) {
		float c = start;
		for (int i = 0; i < 4096; ++i) {
			c = std::nextafterf(c, dir < 0 ? -1e30f : 1e30f);
			const double e = std::fabs(deltaPhaseForPitchCV(c, sr, dt, 0) - target);
			if (e < bestErr) { bestErr = e; best = c; }
		}
	}
	if (achievedBinErrorOut) *achievedBinErrorOut = bestErr * (double)kSpectrumN;
	return best;
}

// ---------------------------------------------------------------------------
// binCentredSampleTime — METHOD TWO, and the one the D-10 self-check stands on.
//
// WHY THIS EXISTS, AND WHY IT IS NOT SCOPE CREEP. 32-RESEARCH.md § Validation
// Architecture specifies both methods and states the switching rule outright:
// "If any threshold ends up tighter than about -50 dB (only the sine rows do),
// switch that case to the second method." 32-VALIDATION.md:103 repeats it. The
// tightest threshold this suite will ever assert is the D-09 sine row (-62 dB
// at C7, -64 dB at C8), and D-10 requires the gate's own leakage floor to sit
// at least 10 dB BELOW the threshold being asserted — i.e. at or under -74 dB.
// Method one measures -56.2 to -72.9 dB on this grid. IT CANNOT MEET THE
// SELF-CHECK ON FOUR OF THE SIX ROWS, and no amount of tuning fixes that,
// because the limit is forge::exp2_taylor5's output granularity (see method
// one's banner). Without method two the D-10 self-check would have to be
// weakened to whatever method one happens to achieve, which is the self-check
// deleting itself.
//
// WHAT IT DOES. It leaves pitchCV where method one put it — so the frequency,
// and therefore every guard the core applies, is unchanged — and instead nudges
// the injected sampleTime to the nearest float of (K/N)/freq, then scans
// +/-256 ULPs. MEASURED this session: 5.3e-7 to 2.5e-5 bins, i.e. -92 to -126
// dB implied leakage, at a sampleTime deviation of at most 4.4 ppm from
// nominal. That reproduces RESEARCH's "~1e-5 bins, ~-100 dB at <= 5 ppm".
//
// WHAT IT DOES *NOT* DO, AND MUST NEVER DO. It does not touch
// forge::VcoBlockDriver and it does not make that driver's sampleTime overwrite
// conditional. The driver stays exactly as it is; a caller wanting a nudged
// sampleTime drives the core through its own local sample loop instead, which
// is a different DRIVING PATH, not a modified driver. If anyone ever "unifies"
// these by adding a flag to VcoBlockDriver, they will have changed what the
// macOS bit-exact drift-ON golden replay leg of the SHIPPED LFO's sibling
// driver looks like to a future reader, and re-opened the R-2 / P-4 argument
// that keeps the two drivers independent forever.
//
// THE 5 PPM DEVIATION IS ASSERTED, NOT ASSUMED. Part C bounds it. A nudge that
// ran away from nominal would mean this function is compensating for a broken
// chain rather than for float granularity, and the whole construction would be
// measuring itself again.
// ---------------------------------------------------------------------------
float binCentredSampleTime(double sr, float pitchCV, int K, double* achievedBinErrorOut) {
	const double target = (double)K / (double)kSpectrumN;
	float freq = 0.f;
	deltaPhaseForPitchCV(pitchCV, sr, (float)(1.0 / sr), &freq);
	if (!(freq > 0.f)) {
		if (achievedBinErrorOut) *achievedBinErrorOut = 1e9;
		return (float)(1.0 / sr);
	}

	const float start = (float)(target / (double)freq);
	float best = start;
	double bestErr = std::fabs((double)freq * (double)start - target);
	for (int dir = -1; dir <= 1; dir += 2) {
		float c = start;
		for (int i = 0; i < 256; ++i) {
			c = std::nextafterf(c, dir < 0 ? -1e30f : 1e30f);
			const double e = std::fabs((double)freq * (double)c - target);
			if (e < bestErr) { bestErr = e; best = c; }
		}
	}
	if (achievedBinErrorOut) *achievedBinErrorOut = bestErr * (double)kSpectrumN;
	return best;
}

// ---------------------------------------------------------------------------
// impliedLeakageDb — the rectangular-window leakage into the ADJACENT bin at a
// fractional bin offset, which for small offsets is simply 20*log10(offset).
//
// CORROBORATED AGAINST RESEARCH'S TWO MEASURED POINTS: 1.5e-3 bins gives
// -56.5 dB and 2.3e-4 bins gives -73 dB. Both reproduce to the digit here
// (20*log10(1.5e-3) = -56.48; 20*log10(2.3e-4) = -72.77).
//
// WHAT THIS EXISTS TO STOP, in D-10's own words: without this self-check the
// gate can pass by measuring forge::exp2_taylor5's OUTPUT GRANULARITY rather
// than the DSP. A drive frequency sitting a fraction of a bin off centre smears
// the fundamental into its neighbours, and that smear is indistinguishable from
// alias energy to the classifier above — so a threshold tighter than the smear
// is not a statement about band-limiting at all. It is D-10's stated failure
// mode wearing a different costume, and the only defence is for the gate to
// assert its own noise floor rather than to hope it is small.
// ---------------------------------------------------------------------------
double impliedLeakageDb(double binError) {
	if (!(binError > 0.0)) return -999.0;
	return 20.0 * std::log10(binError);
}

// ---------------------------------------------------------------------------
// NaiveVcoCoreMirror — the PERMANENT D-08 naive baseline. Read this banner
// before touching anything below it.
//
// THIS TYPE IS A TEST FIXTURE. IT IS NOT PRODUCTION CODE, IT IS NOT A WORK IN
// PROGRESS, AND IT MUST NEVER BE MOVED UNDER src/ — least of all into
// src/dsp/VcoCore.hpp, which is the file it is a copy of. It exists so that
// Phase 32 has something to measure AGAINST: D-08 requires the naive path to
// stay callable for the whole phase, and this is how it stays callable WITHOUT
// a flag in the core, without a second entry point beside step(), and without
// one line of production code that exists only for a test. That is the whole
// design: the naive path is preserved by COPYING it into the test TU at the
// moment it is still the live behavior, not by leaving a switch in the shipped
// oscillator.
//
// >>> THE DELIBERATE DIVERGENCE, AND THE ONLY ONE. <<<
// This mirror applies NO BAND-LIMITING CORRECTION. Everything else is
// src/dsp/VcoCore.hpp field for field and line for line. Right now that is not
// a divergence at all, because the live core applies no correction either —
// which is exactly why this file lands in plan 32-01 and not later. Plan 32-06
// lands forge::MorphBlep at the VcoCore.hpp:484 call site, and from that commit
// onward this type is the ONLY remaining naive path in the repository. It must
// stay naive FOREVER. Plan 32-07's no-regression invariant compares the
// corrected core against it cell by cell, and a mirror that quietly acquired
// the correction would make that invariant compare the corrected core against
// itself — a comparison that cannot fail, asserting nothing, in the exact place
// the phase's central claim is supposed to be proven.
//
// CONTAINMENT IS BY PLACEMENT, AND IT IS ASSERTED RATHER THAN ASSUMED. This
// type lives in this TU's anonymous namespace. It has internal linkage, it
// appears in no header, and it appears in no shipped build graph, so
// check_includes.sh, check_canary.sh and the strict C++11 gate never see it —
// all three scan src/ only. Plan 32-01's acceptance criteria require
// `grep -rc 'NaiveVcoCoreMirror' src/` to find nothing, on every run.
//
// THE MIRROR-MAINTENANCE RULE, inherited verbatim in spirit from
// tests/test_vco_core.cpp's DeliberatelyBrokenSharedStateCore. Every future
// change to forge::VcoCore::step()'s pitch / guard / accumulate sequence MUST
// be mirrored here — the volt-domain summation, the D-14 bound with the negated
// comparison first, the single exponential, the sanitised rate, the
// ceiling-then-negated-floor order, the deltaPhase bound, the single-subtract
// wrap. If they are not, this type differs from the real core in MORE than the
// one thing its banner promises, and — worse than merely being wrong — it can
// go on passing, because a divergence outside the inputs the grid drives is
// invisible precisely when it is inert.
//
// AND IF MIRRORING ONE EVER MOVES A FIGURE THIS FILE RECORDS, STOP AND REPORT
// IT RATHER THAN UPDATING THE NUMBER. A moved figure means the change was NOT
// inert — it means it altered behavior this fixture was pinning, which is a
// finding about the real core and not a bookkeeping update here.
// ---------------------------------------------------------------------------
struct NaiveVcoCoreMirror {
	// Per-instance, exactly as forge::VcoCore holds them (CORE-03 / D-14).
	// `phase` is double for the same reason the real one is: a float
	// accumulator loses low-order increments at audio rates over long blocks.
	forge::DriftEngine drift;
	forge::Waveshape wave;
	double phase = 0.0;

	void seed(uint64_t s0, uint64_t s1 = 0) { drift.seed(s0, s1); }

	// The D-11 five-coefficient copy, mirroring forge::VcoCore::setSpreadSeed
	// field for field. characterSpread is deliberately NOT copied, for the same
	// reason the real core does not copy it: folding it in would silently change
	// what character = 1.0 means.
	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) {
		drift.setSpreadSeed(s0, s1);
		wave.triAsymmetrySpread = drift.triAsymmetrySpread;
		wave.sawCurvatureSpread = drift.sawCurvatureSpread;
		wave.squareDutySpread   = drift.squareDutySpread;
		wave.pulseEdgeSpread    = drift.pulseEdgeSpread;
		wave.bleedSpread        = drift.bleedSpread;
	}

	// Signature matches forge::VcoCore::step(...) so the same forge::VcoInputs
	// can be handed to both with no adaptation anywhere.
	//
	// Telemetry is the ONE thing deliberately absent, and it is not a behavioral
	// divergence: tel is documented in src/dsp/VcoCore.hpp:251-252 as display
	// state that is NOT part of the audio path, nothing here reads it, and
	// omitting it cannot move a returned sample.
	float step(const forge::VcoInputs& in) {
		// Mirrored: the volt-domain summation of V/OCT, coarse and the divided
		// fine value, with the semitone-to-octave division owned by the core
		// (D-05), then the fmConnected-GATED FM contribution added into those
		// same volts — never multiplied onto a resolved frequency (D-01/FM-03).
		float pitchVolts = in.pitchCV + in.coarse + in.fine * (1.f / 12.f);
		if (in.fmConnected) pitchVolts += in.fmVolts * in.fmAtten;

		// Mirrored: the D-14 bound on the summed volts, with the NEGATED
		// comparison FIRST as the NaN catcher. Never forge::clamp — both of its
		// comparisons are false for a not-a-number, so a NaN passes through it
		// unchanged, and a NaN is precisely the input class this pair stops.
		if (!(pitchVolts > -forge::kVcoMaxPitchVolts)) pitchVolts = -forge::kVcoMaxPitchVolts;
		if (pitchVolts > forge::kVcoMaxPitchVolts) pitchVolts = forge::kVcoMaxPitchVolts;

		// Mirrored: EXACTLY ONE exponential, off the C4 reference, using the
		// frozen polynomial approximation and never libm.
		float freq = forge::kVcoFreqC4 * forge::exp2_taylor5(pitchVolts);

		// Mirrored: the rate is sanitised BEFORE it is scaled (WR-06), then the
		// ceiling, then the negated floor LAST so the floor is always the final
		// writer (CR-01). Do not swap those two lines.
		const float safeRate = (in.sampleRate > 0.f) ? in.sampleRate : 0.f;
		const float maxFreq = forge::kVcoNyquistGuardFrac * safeRate;
		if (freq > maxFreq) freq = maxFreq;
		if (!(freq > 0.f)) freq = 0.f;

		// Mirrored: the direct bound on the increment (which is NOT inferred
		// from the frequency guard — nothing in forge::VcoInputs couples
		// sampleTime to sampleRate), then the single-subtract wrap.
		double deltaPhase = (double)freq * (double)in.sampleTime;
		if (!(deltaPhase > 0.0)) deltaPhase = 0.0;
		if (deltaPhase > forge::kVcoMaxDeltaPhase) deltaPhase = forge::kVcoMaxDeltaPhase;
		phase += deltaPhase;
		if (phase >= 1.0) phase -= 1.0;

		const float p = (float)phase;
		const float morph = forge::clamp(in.morph, 0.f, 1.f);
		const float character = forge::clamp(in.character, 0.f, 1.f);

		// Mirrored: ONE call into the frozen Waveshape — a call, never an edit —
		// with bleedLfo = 0.f, and the unconditioned x5. THIS IS THE LINE THAT
		// DIVERGES IN PLAN 32-06: the real core gains the forge::MorphBlep
		// correction here, and this line must NOT.
		return 5.f * wave.morphedWave(p, morph, character, 0.f);
	}
};

// ---------------------------------------------------------------------------
// SpectrumCell — one measurement point of the D-09 threshold matrix.
//
// `morph` names the shape centre and `region` is its human name; `character` is
// the THIRD index, and the reason it exists is P-6 below. `tier` is what the
// cell is FOR: "gated" cells are asserted, "diagnostic" cells are CAPTUREd and
// never CHECKed, "regression" cells are the D-11 cross-rate pair. `provenance`
// is where the number came from, in words, in the test — because a threshold
// without a written source is a number someone can quietly edit.
// ---------------------------------------------------------------------------
struct SpectrumCell {
	double sr;
	int K;
	const char* note;
	float morph;
	const char* region;
	float character;
	float thresholdDb;
	const char* tier;
	const char* provenance;
};

// ---------------------------------------------------------------------------
// THE THRESHOLD FLOOR, and why the grid is not free to be tighter than it.
//
// D-10 requires the gate's own leakage floor to sit at least 10 dB BELOW
// whatever threshold it asserts. Plan 32-01 measured this apparatus's best
// achievable leakage over the six grid rows: -116.19 / -125.51 / -101.55 /
// -91.95 / -97.71 / -102.87 dB (method two, the sampleTime nudge; see
// binCentredSampleTime's banner). The WORST of those is -91.95 dB at 44.1 kHz
// C9, so the tightest threshold this apparatus can honestly assert anywhere on
// the grid is -81.95 dB. -75.0 is used, leaving that worst row 16.95 dB of
// margin and every other row 22 to 50 dB.
//
// THIS IS NOT A SOFTENING OF THE EVIDENCE, IT IS A STATEMENT ABOUT THE
// INSTRUMENT. Six cells' prototype figures are tighter than -75 dB: every sine
// cell at character 0.00 (-150.7 dB, where a pure sine has no discontinuity to
// alias at all), plus sine C7 and sine C6 at character 1.00. Asserting -147 dB
// on those cells would not be a stronger claim about band-limiting — it would
// be a claim the measurement cannot support, and D-10 exists to stop exactly
// that. The floored cells say so in their own provenance string.
//
// The floor is a STATIC constant, deliberately. It is NOT computed from the
// binError the self-check reads, because a threshold derived from the
// measurement it is checked against is a self-check that can never fail.
// ---------------------------------------------------------------------------
constexpr float kThresholdFloorDb = -75.0f;

// The provenance strings. Every cell carries one; there is no cell in the grid
// whose threshold has no written source.
const char* const kProvDirect =
	"PROVISIONAL, from the 32-RESEARCH prototype's corrected column plus 3 dB, rounded outward; "
	"re-pinned in plan 32-07 from this repository's own measurement of the real forge::MorphBlep";
const char* const kProvFloored =
	"PROVISIONAL, from the 32-RESEARCH prototype's corrected column plus 3 dB, then FLOORED at "
	"kThresholdFloorDb because the prototype figure is tighter than this apparatus can assert (D-10); "
	"re-pinned in plan 32-07 from this repository's own measurement of the real forge::MorphBlep";
const char* const kProvSameNote =
	"PROVISIONAL, from the 32-RESEARCH prototype's corrected column plus 3 dB at the 44.1 kHz row for the "
	"SAME NOTE - the prototype matrix has no 48 or 96 kHz rows, and D-11 lands these cells on C8 precisely "
	"so the 44.1 kHz C8 threshold transfers rather than being invented; "
	"re-pinned in plan 32-07 from this repository's own measurement of the real forge::MorphBlep";
const char* const kProvSameNoteFloored =
	"PROVISIONAL, from the 32-RESEARCH prototype's corrected column plus 3 dB at the 44.1 kHz row for the "
	"SAME NOTE, then FLOORED at kThresholdFloorDb because the prototype figure is tighter than this "
	"apparatus can assert (D-10); "
	"re-pinned in plan 32-07 from this repository's own measurement of the real forge::MorphBlep";
const char* const kProvDiagnostic =
	"PROVISIONAL, from the 32-RESEARCH prototype's corrected column plus 3 dB - the C6 rows exist in "
	"32-RESEARCH section 'D-08 baseline and D-09 threshold evidence' but NOT in 32-VALIDATION's "
	"Threshold Policy matrix, which starts at C7; diagnostic tier, CAPTUREd and never CHECKed; "
	"re-pinned in plan 32-07 from this repository's own measurement of the real forge::MorphBlep";
const char* const kProvDiagnosticFloored =
	"PROVISIONAL, from the 32-RESEARCH prototype's corrected column plus 3 dB (a C6 row present in "
	"32-RESEARCH but absent from 32-VALIDATION's matrix), then FLOORED at kThresholdFloorDb because the "
	"prototype figure is tighter than this apparatus can assert (D-10); diagnostic tier, never CHECKed; "
	"re-pinned in plan 32-07 from this repository's own measurement of the real forge::MorphBlep";

// ---------------------------------------------------------------------------
// SPECTRUM_GRID — the 90 cells of the D-09 threshold matrix. READ THESE TWO
// PARAGRAPHS BEFORE CHANGING ANY NUMBER BELOW.
//
// >>> P-5: THE ROADMAP'S FORMER "APPROXIMATELY -60 dB" IS UNREACHABLE, AND
//     THAT IS A PROPERTY OF THE TECHNIQUE, NOT OF THIS IMPLEMENTATION. <<<
// A 2-sample polyBLEP multiplies the spectrum by a squared sinc, which is only
// about -8 dB at Nyquist and about -10.5 dB at the first alias of a C8 saw. The
// first alias of a saw at C8 is its 6th harmonic at one sixth of the
// fundamental, i.e. -15.6 dB; ten decibels of attenuation lands it at -25.8 dB,
// and no amount of implementation care moves it to -60. DAFx-16 (paper 33,
// Table 2) independently reports the same ceiling for a FOUR-point polyBLAMP:
// 46 dB SNR for a triangle at C8, versus 45 dB for 4x oversampling. Four-point
// would roughly double the dB attenuation and still land near -36 dB for the
// saw. The roadmap's figure is a TARGET, which the roadmap itself qualifies
// with "pinned empirically". The thresholds below are the empirical pinning.
// A LATER AGENT MUST NOT "RESTORE" -60 dB HERE.
//
// >>> P-6: THIS IS WHY THERE IS A THIRD INDEX. <<<
// A threshold indexed by (morph region, note) ALONE cannot be both red against
// the naive core and green against the corrected one. The triangle at C8
// improves by 15.0 dB at character 0.00 (-33.8 -> -48.8) and by EXACTLY 0.0 dB
// at character 1.00 (-33.5 -> -33.5) — because at character 1 the corner is
// already 7.7 samples wide and the D-03 character factor correctly returns
// zero. Any single number for "triangle at C8" is therefore either vacuously
// passed by the naive path or wrongly failed by a correct implementation. That
// is the entire reason `character` is a column in this table, and collapsing it
// would silently delete the phase's evidence.
//
// The threshold column is the prototype's CORRECTED figure plus a 3 dB margin,
// rounded outward, floored at kThresholdFloorDb. It is PROVISIONAL: plan 32-07
// re-pins it from this repository's own measurement of the real
// forge::MorphBlep, and the trailing comment on every row records the
// prototype's naive and corrected pair so the re-pinning has something to move
// against. THE ANTI-SOFTENING CLAUSE APPLIES: a threshold that turns out to be
// unreachable is escalated per plan 32-07's documented procedure, never quietly
// loosened, and the corrected column is never edited to match whatever the
// implementation happened to produce.
// ---------------------------------------------------------------------------
static const SpectrumCell SPECTRUM_GRID[] = {

	// =======================================================================
	// (a) THE GATED GRID — 45 cells. 44.1 kHz at K = 195 (C7), K = 389 (C8)
	//     and K = 777 (C9), each crossed with the five shape centres and with
	//     character 0.00 / 0.50 / 1.00.
	//
	//     WHAT THIS GROUP PROVES: that the alias floor is bounded at the three
	//     notes where a morphing oscillator actually aliases, for every shape
	//     and at every character. These are the cells plan 32-07's gate
	//     asserts on.
	//
	//     THE OPERATOR SETTLED THE NOTE SET ON 2026-08-01: the gate asserts on
	//     C7, C8 AND C9. That decision SUPERSEDES the roadmap's former
	//     undefined phrase "top two octaves" outright rather than picking one
	//     of its two readings, and ROADMAP SC-4 has already been edited to say
	//     so. C6 is retained below as a diagnostic row.
	// =======================================================================

	// --- C7, 44.1 kHz, K = 195, ~2099.5 Hz, 10 harmonics below Nyquist. -----
	{ 44100.0, 195, "C7", 0.00f, "sine",     0.00f,  -75.0f, "gated",        kProvFloored },   // prototype naive  -150.7 -> corrected  -150.7 dB
	{ 44100.0, 195, "C7", 0.00f, "sine",     0.50f,  -65.0f, "gated",        kProvDirect },   // prototype naive   -60.0 -> corrected   -68.4 dB
	{ 44100.0, 195, "C7", 0.00f, "sine",     1.00f,  -75.0f, "gated",        kProvFloored },   // prototype naive  -102.4 -> corrected  -102.4 dB
	{ 44100.0, 195, "C7", 0.25f, "triangle", 0.00f,  -47.0f, "gated",        kProvDirect },   // prototype naive   -41.7 -> corrected   -50.3 dB
	{ 44100.0, 195, "C7", 0.25f, "triangle", 0.50f,  -39.0f, "gated",        kProvDirect },   // prototype naive   -40.7 -> corrected   -42.1 dB
	{ 44100.0, 195, "C7", 0.25f, "triangle", 1.00f,  -44.0f, "gated",        kProvDirect },   // prototype naive   -47.4 -> corrected   -47.4 dB
	{ 44100.0, 195, "C7", 0.50f, "saw",      0.00f,  -26.0f, "gated",        kProvDirect },   // prototype naive   -20.8 -> corrected   -29.5 dB
	{ 44100.0, 195, "C7", 0.50f, "saw",      0.50f,  -26.0f, "gated",        kProvDirect },   // prototype naive   -20.6 -> corrected   -29.2 dB
	{ 44100.0, 195, "C7", 0.50f, "saw",      1.00f,  -25.0f, "gated",        kProvDirect },   // prototype naive   -20.0 -> corrected   -28.0 dB
	{ 44100.0, 195, "C7", 0.75f, "square",   0.00f,  -26.0f, "gated",        kProvDirect },   // prototype naive   -20.8 -> corrected   -29.5 dB
	{ 44100.0, 195, "C7", 0.75f, "square",   0.50f,  -27.0f, "gated",        kProvDirect },   // prototype naive   -22.0 -> corrected   -30.3 dB
	{ 44100.0, 195, "C7", 0.75f, "square",   1.00f,  -57.0f, "gated",        kProvDirect },   // prototype naive   -53.0 -> corrected   -60.1 dB
	{ 44100.0, 195, "C7", 1.00f, "pulse 5%", 0.00f,  -10.0f, "gated",        kProvDirect },   // prototype naive    -4.8 -> corrected   -13.5 dB
	{ 44100.0, 195, "C7", 1.00f, "pulse 5%", 0.50f,  -10.0f, "gated",        kProvDirect },   // prototype naive    -5.3 -> corrected   -13.1 dB
	{ 44100.0, 195, "C7", 1.00f, "pulse 5%", 1.00f,  -17.0f, "gated",        kProvDirect },   // prototype naive   -19.7 -> corrected   -20.3 dB

	// --- C8, 44.1 kHz, K = 389, ~4188.2 Hz, 5 harmonics below Nyquist. ------
	{ 44100.0, 389, "C8", 0.00f, "sine",     0.00f,  -75.0f, "gated",        kProvFloored },   // prototype naive  -150.7 -> corrected  -150.7 dB
	{ 44100.0, 389, "C8", 0.00f, "sine",     0.50f,  -68.0f, "gated",        kProvDirect },   // prototype naive   -55.5 -> corrected   -71.5 dB
	{ 44100.0, 389, "C8", 0.00f, "sine",     1.00f,  -73.0f, "gated",        kProvDirect },   // prototype naive   -70.6 -> corrected   -76.4 dB
	{ 44100.0, 389, "C8", 0.25f, "triangle", 0.00f,  -45.0f, "gated",        kProvDirect },   // prototype naive   -33.8 -> corrected   -48.8 dB
	{ 44100.0, 389, "C8", 0.25f, "triangle", 0.50f,  -35.0f, "gated",        kProvDirect },   // prototype naive   -33.2 -> corrected   -38.1 dB
	{ 44100.0, 389, "C8", 0.25f, "triangle", 1.00f,  -30.0f, "gated",        kProvDirect },   // prototype naive   -33.5 -> corrected   -33.5 dB
	{ 44100.0, 389, "C8", 0.50f, "saw",      0.00f,  -22.0f, "gated",        kProvDirect },   // prototype naive   -15.6 -> corrected   -25.8 dB
	{ 44100.0, 389, "C8", 0.50f, "saw",      0.50f,  -22.0f, "gated",        kProvDirect },   // prototype naive   -15.4 -> corrected   -25.7 dB
	{ 44100.0, 389, "C8", 0.50f, "saw",      1.00f,  -20.0f, "gated",        kProvDirect },   // prototype naive   -14.7 -> corrected   -23.9 dB
	{ 44100.0, 389, "C8", 0.75f, "square",   0.00f,  -28.0f, "gated",        kProvDirect },   // prototype naive   -16.9 -> corrected   -31.9 dB
	{ 44100.0, 389, "C8", 0.75f, "square",   0.50f,  -30.0f, "gated",        kProvDirect },   // prototype naive   -17.5 -> corrected   -33.2 dB
	{ 44100.0, 389, "C8", 0.75f, "square",   1.00f,  -44.0f, "gated",        kProvDirect },   // prototype naive   -40.1 -> corrected   -47.7 dB
	{ 44100.0, 389, "C8", 1.00f, "pulse 5%", 0.00f,   -8.0f, "gated",        kProvDirect },   // prototype naive    -1.3 -> corrected   -11.6 dB
	{ 44100.0, 389, "C8", 1.00f, "pulse 5%", 0.50f,   -8.0f, "gated",        kProvDirect },   // prototype naive    -1.5 -> corrected   -11.1 dB
	{ 44100.0, 389, "C8", 1.00f, "pulse 5%", 1.00f,   -7.0f, "gated",        kProvDirect },   // prototype naive    -7.6 -> corrected   -10.8 dB

	// --- C9, 44.1 kHz, K = 777, ~8366.9 Hz, 2 harmonics below Nyquist. ------
	//     The hardest row for every shape: only two harmonics survive, so
	//     almost the whole waveform is alias.
	{ 44100.0, 777, "C9", 0.00f, "sine",     0.00f,  -75.0f, "gated",        kProvFloored },   // prototype naive  -150.7 -> corrected  -150.7 dB
	{ 44100.0, 777, "C9", 0.00f, "sine",     0.50f,  -31.0f, "gated",        kProvDirect },   // prototype naive   -36.1 -> corrected   -34.6 dB
	{ 44100.0, 777, "C9", 0.00f, "sine",     1.00f,  -19.0f, "gated",        kProvDirect },   // prototype naive   -23.2 -> corrected   -22.7 dB
	{ 44100.0, 777, "C9", 0.25f, "triangle", 0.00f,  -25.0f, "gated",        kProvDirect },   // prototype naive   -19.1 -> corrected   -28.5 dB
	{ 44100.0, 777, "C9", 0.25f, "triangle", 0.50f,  -21.0f, "gated",        kProvDirect },   // prototype naive   -19.0 -> corrected   -24.9 dB
	{ 44100.0, 777, "C9", 0.25f, "triangle", 1.00f,  -16.0f, "gated",        kProvDirect },   // prototype naive   -18.5 -> corrected   -19.8 dB
	{ 44100.0, 777, "C9", 0.50f, "saw",      0.00f,  -16.0f, "gated",        kProvDirect },   // prototype naive    -9.5 -> corrected   -19.0 dB
	{ 44100.0, 777, "C9", 0.50f, "saw",      0.50f,  -15.0f, "gated",        kProvDirect },   // prototype naive    -9.4 -> corrected   -18.9 dB
	{ 44100.0, 777, "C9", 0.50f, "saw",      1.00f,  -14.0f, "gated",        kProvDirect },   // prototype naive    -8.9 -> corrected   -17.5 dB
	{ 44100.0, 777, "C9", 0.75f, "square",   0.00f,  -16.0f, "gated",        kProvDirect },   // prototype naive    -9.5 -> corrected   -19.0 dB
	{ 44100.0, 777, "C9", 0.75f, "square",   0.50f,  -16.0f, "gated",        kProvDirect },   // prototype naive    -9.6 -> corrected   -19.6 dB
	{ 44100.0, 777, "C9", 0.75f, "square",   1.00f,  -18.0f, "gated",        kProvDirect },   // prototype naive   -15.5 -> corrected   -21.7 dB
	{ 44100.0, 777, "C9", 1.00f, "pulse 5%", 0.00f,   -6.0f, "gated",        kProvDirect },   // prototype naive    -0.3 -> corrected    -9.8 dB
	{ 44100.0, 777, "C9", 1.00f, "pulse 5%", 0.50f,   -6.0f, "gated",        kProvDirect },   // prototype naive    -0.4 -> corrected    -9.5 dB
	{ 44100.0, 777, "C9", 1.00f, "pulse 5%", 1.00f,   -2.0f, "gated",        kProvDirect },   // prototype naive    -2.3 -> corrected    -5.6 dB

	// =======================================================================
	// (b) THE DIAGNOSTIC ROW — 15 cells. 44.1 kHz at K = 97 (C6), same fifteen
	//     morph-by-character cells. These are CAPTUREd and NEVER CHECKed.
	//
	//     WHY C6 EARNS ITS RUNTIME. It is the row where the naive path is
	//     ALREADY CLEAN at high character — the square measures -60.1 dB naive
	//     at character 1.00, cleaner than most corrected cells anywhere else
	//     on the grid. That makes C6 the row that would expose an
	//     over-correcting character factor as a REGRESSION rather than as a
	//     miss: P-1's failure mode is a factor that returns a small non-zero
	//     value where the real edge is already several samples wide, and the
	//     residual step-shaped correction it injects is broadband energy ADDED
	//     to an already-clean spectrum. A grid that only looked at C7 and above
	//     would score that as "no improvement" instead of "damage".
	// =======================================================================
	{ 44100.0,  97, "C6", 0.00f, "sine",     0.00f,  -75.0f, "diagnostic",   kProvDiagnosticFloored },   // prototype naive  -150.7 -> corrected  -150.7 dB
	{ 44100.0,  97, "C6", 0.00f, "sine",     0.50f,  -73.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -67.5 -> corrected   -76.6 dB
	{ 44100.0,  97, "C6", 0.00f, "sine",     1.00f,  -75.0f, "diagnostic",   kProvDiagnosticFloored },   // prototype naive  -117.3 -> corrected  -117.3 dB
	{ 44100.0,  97, "C6", 0.25f, "triangle", 0.00f,  -61.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -54.5 -> corrected   -64.0 dB
	{ 44100.0,  97, "C6", 0.25f, "triangle", 0.50f,  -52.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -55.1 -> corrected   -55.1 dB
	{ 44100.0,  97, "C6", 0.25f, "triangle", 1.00f,  -57.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -60.5 -> corrected   -60.5 dB
	{ 44100.0,  97, "C6", 0.50f, "saw",      0.00f,  -32.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -26.8 -> corrected   -35.4 dB
	{ 44100.0,  97, "C6", 0.50f, "saw",      0.50f,  -32.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -26.6 -> corrected   -35.1 dB
	{ 44100.0,  97, "C6", 0.50f, "saw",      1.00f,  -32.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -26.4 -> corrected   -35.0 dB
	{ 44100.0,  97, "C6", 0.75f, "square",   0.00f,  -33.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -27.2 -> corrected   -36.7 dB
	{ 44100.0,  97, "C6", 0.75f, "square",   0.50f,  -35.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -29.4 -> corrected   -38.6 dB
	{ 44100.0,  97, "C6", 0.75f, "square",   1.00f,  -65.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -60.1 -> corrected   -68.7 dB
	{ 44100.0,  97, "C6", 1.00f, "pulse 5%", 0.00f,  -23.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -13.2 -> corrected   -26.4 dB
	{ 44100.0,  97, "C6", 1.00f, "pulse 5%", 0.50f,  -24.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -14.9 -> corrected   -27.2 dB
	{ 44100.0,  97, "C6", 1.00f, "pulse 5%", 1.00f,  -33.0f, "diagnostic",   kProvDiagnostic },   // prototype naive   -36.8 -> corrected   -36.8 dB

	// =======================================================================
	// (c) THE D-11 CROSS-RATE REGRESSION — 30 cells. 48000 Hz at K = 357 and
	//     96000 Hz at K = 179, same fifteen cells each.
	//
	//     WHY D-11 EXISTS, IN FULL. A band-limiting correction scaled wrongly
	//     by `dt` fails RATE-DEPENDENTLY: the residual kernel is a function of
	//     the crossing distance measured in SAMPLES, so a factor that is off by
	//     a power of `dt` produces a correction that is right at one rate and
	//     wrong at every other one. That failure is COMPLETELY INVISIBLE to a
	//     grid measured at a single sample rate, and it is the most likely way
	//     this implementation goes subtly wrong — the arithmetic still looks
	//     plausible, the spectrum still looks like a spectrum, and only the
	//     comparison across rates can see it.
	//
	//     THE TWO EXTRA RATES DELIBERATELY LAND ON THE SAME NOTE as the
	//     44.1 kHz C8 row above (4183.6 Hz and 4195.3 Hz against 4188.2 Hz),
	//     so the cross-rate comparison is LIKE WITH LIKE. Measuring 48 kHz at a
	//     different note would confound a rate-scaling bug with a
	//     harmonics-below-Nyquist difference, and the whole point of the
	//     comparison would be lost. A LATER AGENT MUST NOT "SPREAD THESE OUT"
	//     ACROSS DIFFERENT NOTES.
	// =======================================================================

	// --- 48 kHz, K = 357, ~4183.6 Hz, 5 harmonics — same note as 44.1k C8. --
	{ 48000.0, 357, "C8", 0.00f, "sine",     0.00f,  -75.0f, "regression",   kProvSameNoteFloored },   // prototype naive  -150.7 -> corrected  -150.7 dB
	{ 48000.0, 357, "C8", 0.00f, "sine",     0.50f,  -68.0f, "regression",   kProvSameNote },   // prototype naive   -55.5 -> corrected   -71.5 dB
	{ 48000.0, 357, "C8", 0.00f, "sine",     1.00f,  -73.0f, "regression",   kProvSameNote },   // prototype naive   -70.6 -> corrected   -76.4 dB
	{ 48000.0, 357, "C8", 0.25f, "triangle", 0.00f,  -45.0f, "regression",   kProvSameNote },   // prototype naive   -33.8 -> corrected   -48.8 dB
	{ 48000.0, 357, "C8", 0.25f, "triangle", 0.50f,  -35.0f, "regression",   kProvSameNote },   // prototype naive   -33.2 -> corrected   -38.1 dB
	{ 48000.0, 357, "C8", 0.25f, "triangle", 1.00f,  -30.0f, "regression",   kProvSameNote },   // prototype naive   -33.5 -> corrected   -33.5 dB
	{ 48000.0, 357, "C8", 0.50f, "saw",      0.00f,  -22.0f, "regression",   kProvSameNote },   // prototype naive   -15.6 -> corrected   -25.8 dB
	{ 48000.0, 357, "C8", 0.50f, "saw",      0.50f,  -22.0f, "regression",   kProvSameNote },   // prototype naive   -15.4 -> corrected   -25.7 dB
	{ 48000.0, 357, "C8", 0.50f, "saw",      1.00f,  -20.0f, "regression",   kProvSameNote },   // prototype naive   -14.7 -> corrected   -23.9 dB
	{ 48000.0, 357, "C8", 0.75f, "square",   0.00f,  -28.0f, "regression",   kProvSameNote },   // prototype naive   -16.9 -> corrected   -31.9 dB
	{ 48000.0, 357, "C8", 0.75f, "square",   0.50f,  -30.0f, "regression",   kProvSameNote },   // prototype naive   -17.5 -> corrected   -33.2 dB
	{ 48000.0, 357, "C8", 0.75f, "square",   1.00f,  -44.0f, "regression",   kProvSameNote },   // prototype naive   -40.1 -> corrected   -47.7 dB
	{ 48000.0, 357, "C8", 1.00f, "pulse 5%", 0.00f,   -8.0f, "regression",   kProvSameNote },   // prototype naive    -1.3 -> corrected   -11.6 dB
	{ 48000.0, 357, "C8", 1.00f, "pulse 5%", 0.50f,   -8.0f, "regression",   kProvSameNote },   // prototype naive    -1.5 -> corrected   -11.1 dB
	{ 48000.0, 357, "C8", 1.00f, "pulse 5%", 1.00f,   -7.0f, "regression",   kProvSameNote },   // prototype naive    -7.6 -> corrected   -10.8 dB

	// --- 96 kHz, K = 179, ~4195.3 Hz, 11 harmonics — same note as 44.1k C8. -
	{ 96000.0, 179, "C8", 0.00f, "sine",     0.00f,  -75.0f, "regression",   kProvSameNoteFloored },   // prototype naive  -150.7 -> corrected  -150.7 dB
	{ 96000.0, 179, "C8", 0.00f, "sine",     0.50f,  -68.0f, "regression",   kProvSameNote },   // prototype naive   -55.5 -> corrected   -71.5 dB
	{ 96000.0, 179, "C8", 0.00f, "sine",     1.00f,  -73.0f, "regression",   kProvSameNote },   // prototype naive   -70.6 -> corrected   -76.4 dB
	{ 96000.0, 179, "C8", 0.25f, "triangle", 0.00f,  -45.0f, "regression",   kProvSameNote },   // prototype naive   -33.8 -> corrected   -48.8 dB
	{ 96000.0, 179, "C8", 0.25f, "triangle", 0.50f,  -35.0f, "regression",   kProvSameNote },   // prototype naive   -33.2 -> corrected   -38.1 dB
	{ 96000.0, 179, "C8", 0.25f, "triangle", 1.00f,  -30.0f, "regression",   kProvSameNote },   // prototype naive   -33.5 -> corrected   -33.5 dB
	{ 96000.0, 179, "C8", 0.50f, "saw",      0.00f,  -22.0f, "regression",   kProvSameNote },   // prototype naive   -15.6 -> corrected   -25.8 dB
	{ 96000.0, 179, "C8", 0.50f, "saw",      0.50f,  -22.0f, "regression",   kProvSameNote },   // prototype naive   -15.4 -> corrected   -25.7 dB
	{ 96000.0, 179, "C8", 0.50f, "saw",      1.00f,  -20.0f, "regression",   kProvSameNote },   // prototype naive   -14.7 -> corrected   -23.9 dB
	{ 96000.0, 179, "C8", 0.75f, "square",   0.00f,  -28.0f, "regression",   kProvSameNote },   // prototype naive   -16.9 -> corrected   -31.9 dB
	{ 96000.0, 179, "C8", 0.75f, "square",   0.50f,  -30.0f, "regression",   kProvSameNote },   // prototype naive   -17.5 -> corrected   -33.2 dB
	{ 96000.0, 179, "C8", 0.75f, "square",   1.00f,  -44.0f, "regression",   kProvSameNote },   // prototype naive   -40.1 -> corrected   -47.7 dB
	{ 96000.0, 179, "C8", 1.00f, "pulse 5%", 0.00f,   -8.0f, "regression",   kProvSameNote },   // prototype naive    -1.3 -> corrected   -11.6 dB
	{ 96000.0, 179, "C8", 1.00f, "pulse 5%", 0.50f,   -8.0f, "regression",   kProvSameNote },   // prototype naive    -1.5 -> corrected   -11.1 dB
	{ 96000.0, 179, "C8", 1.00f, "pulse 5%", 1.00f,   -7.0f, "regression",   kProvSameNote },   // prototype naive    -7.6 -> corrected   -10.8 dB
};

// Which bin-centre solver a cell was measured with. Reported through
// measureCellDb's methodOut and CAPTUREd, so a red cell names the instrument it
// was measured with rather than leaving it to be guessed.
enum SpectrumMethod {
	kMethodPitchCV    = 1,   // METHOD ONE: bisect pitchCV, forge::VcoBlockDriver unchanged
	kMethodSampleTime = 2    // METHOD TWO: nudge the injected sampleTime, local sample loop
};

// ---------------------------------------------------------------------------
// driveSecondBlock — the ONE sample loop, shared by the mirror and the live
// core, and the reason it is a template.
//
// A comparator whose two sides run different loops proves nothing about the
// difference between them. That is the argument already written into
// tests/check_includes.sh [6/7] — every negative control there calls the SAME
// function its section calls — and into runInterleaveCheck in
// tests/test_vco_core.cpp:170-180, which is a template for exactly this reason:
// invariant 5's deliberately broken stand-in has to run through byte-identically
// the same drive loop as the real core. It applies here unchanged. DO NOT FORK
// THIS INTO TWO NEAR-COPIES, one for NaiveVcoCoreMirror and one for
// forge::VcoCore.
//
// It drives 2 * kSpectrumN samples and returns only the SECOND block. The first
// is the warm-up discard 32-VALIDATION.md pins: the phase accumulator — and,
// from plan 32-06 onward, forge::MorphBlep's `pending` accumulator — must reach
// steady state before the block is analysed, and with deltaPhase sitting on the
// bin centre the second block is exactly periodic while the first is not.
//
// sampleTime and sampleRate are injected per sample, exactly as
// forge::VcoBlockDriver does (tests/VcoBlockDriver.hpp:56-60), because nothing
// in forge::VcoInputs couples the two and the harness owns timing.
// ---------------------------------------------------------------------------
template <typename CoreT>
void driveSecondBlock(CoreT& core, const forge::VcoInputs& base, float dt, double sr,
                      std::vector<float>& out) {
	out.clear();
	out.reserve((std::size_t)kSpectrumN);
	for (int i = 0; i < 2 * kSpectrumN; ++i) {
		forge::VcoInputs in = base;
		in.sampleTime = dt;
		in.sampleRate = (float)sr;
		const float s = core.step(in);
		if (i >= kSpectrumN) out.push_back(s);
	}
}

// ---------------------------------------------------------------------------
// measureCellDb — the alias peak of ONE grid cell, from either the naive mirror
// or the live core.
//
// >>> BOTH BRANCHES EXIST FROM THE START, AND THAT IS THE POINT. <<<
// This plan drives NaiveVcoCoreMirror through it; plan 32-07 drives the
// corrected forge::VcoCore through it, and the naive-versus-corrected delta is
// therefore a LIKE-FOR-LIKE comparison: same solver, same warm-up, same block
// length, same seeds, same classifier, same arithmetic. A comparator whose two
// sides run different code proves nothing about the difference between them —
// the check_includes.sh [6/7] argument and the runInterleaveCheck template
// argument in tests/test_vco_core.cpp:170-180. If a later agent adds a second
// measurement function for the corrected path, the phase's central claim stops
// being a measurement and becomes a coincidence.
//
// THE FOUR SEED LITERALS ARE COPIED VERBATIM from tests/VcoBlockDriver.hpp:42-43
// and must never be invented. A forge::Xoroshiro128Plus seeded (0, 0) is a
// fixed point emitting an all-zero stream, which makes std::normal_distribution's
// rejection loop never terminate — a hung suite here, and a HANG ON PATCH LOAD
// in Rack (T-32-09).
//
// WHICH SOLVER IS USED, AND WHY THE CHOICE IS NOT A WEAKENING. The cell is
// measured with METHOD ONE — bisect pitchCV, forge::VcoBlockDriver completely
// unchanged — whenever method one's implied leakage already sits at least 10 dB
// below that cell's threshold, which is the D-10 bar. Where it does not (the
// sine cells, and a handful of C6 and cross-rate cells whose thresholds are
// tighter than about -60 dB), the cell ESCALATES to METHOD TWO, the sampleTime
// nudge, exactly as 32-RESEARCH's switching rule prescribes: "If any threshold
// ends up tighter than about -50 dB (only the sine rows do), switch that case to
// the second method."
//
// The escalation cannot hide a failure, because the THRESHOLD COLUMN IS STATIC.
// The caller still asserts the D-10 self-check against whatever leakage the
// chosen method actually achieved, so if method two ALSO cannot meet the bar for
// some cell, that REQUIRE fires — and that is a finding about the apparatus,
// which is precisely what the self-check is for. What the escalation removes is
// only the alternative: silently loosening the threshold to whatever method one
// happens to achieve, which is the self-check deleting itself.
//
// METHOD ONE DRIVES THE LIVE CORE THROUGH forge::VcoBlockDriver; method two
// cannot, and must not try. The driver's per-sample overwrite of sampleTime is
// unconditional and documented as load-bearing (tests/VcoBlockDriver.hpp:50-52),
// so a nudged dt is unreachable through it and MAKING THAT OVERWRITE
// CONDITIONAL IS FORBIDDEN — it would re-open the R-2 / P-4 argument that keeps
// the VCO and LFO drivers independent forever. Method two therefore drives the
// live core through driveSecondBlock, the same loop the mirror uses. That the
// two paths agree is not assumed: the case at the bottom of this file REQUIREs
// forge::VcoBlockDriver's output to be bit-identical to a local loop's over a
// 45-point grid at these very frequencies.
// ---------------------------------------------------------------------------
double measureCellDb(const SpectrumCell& cell, bool useMirror,
                     double* aliasRmsDbOut, double* binErrorOut,
                     int* methodOut = 0) {
	// The bin-centred pitch, method one. Computed for every cell, because
	// method two starts from it: the nudge leaves pitchCV — and therefore the
	// frequency and every guard the core applies to it — exactly where method
	// one put it.
	double binErrorPitch = 0.0;
	const float pitchCV = binCentredPitchCV(cell.sr, cell.K, &binErrorPitch);

	const double d10BarDb = (double)cell.thresholdDb - 10.0;
	int method = kMethodPitchCV;
	float dt = (float)(1.0 / cell.sr);
	double binError = binErrorPitch;
	if (!(impliedLeakageDb(binErrorPitch) <= d10BarDb)) {
		double binErrorDt = 0.0;
		const float nudged = binCentredSampleTime(cell.sr, pitchCV, cell.K, &binErrorDt);
		method  = kMethodSampleTime;
		dt      = nudged;
		binError = binErrorDt;
	}
	if (binErrorOut) *binErrorOut = binError;
	if (methodOut)   *methodOut   = method;

	// The constant input the whole block is driven at. Copy-and-assign, never a
	// brace value-list: forge::VcoInputs has NSDMIs, so under C++11 it is not an
	// aggregate and a value-list init is a hard error.
	forge::VcoInputs base;
	base.pitchCV   = pitchCV;
	base.coarse    = 0.f;
	base.fine      = 0.f;
	base.morph     = cell.morph;
	base.character = cell.character;
	base.drift     = 0.f;

	std::vector<float> block;
	if (useMirror) {
		NaiveVcoCoreMirror mirror;
		mirror.seed(0x1234ULL, 0x5678ULL);
		mirror.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);
		driveSecondBlock(mirror, base, dt, cell.sr, block);
	} else if (method == kMethodPitchCV) {
		forge::VcoBlockDriver d(cell.sr);
		const std::vector<float> full = d.run(2 * kSpectrumN, [=](int) { return base; });
		block.assign(full.begin() + kSpectrumN, full.end());
	} else {
		forge::VcoCore core;
		core.seed(0x1234ULL, 0x5678ULL);
		core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);
		driveSecondBlock(core, base, dt, cell.sr, block);
	}

	int aliasBin = -1;
	return aliasPeakDb(block, cell.K, &aliasBin, aliasRmsDbOut);
}

}  // namespace

// ---------------------------------------------------------------------------
// The apparatus validates itself: a positive control, a DETECTION control, and
// the D-10 leakage self-check over the pinned grid.
// ---------------------------------------------------------------------------
TEST_CASE("vco spectrum: the DFT apparatus is validated by DETECTING a planted spur, and its own leakage floor sits 10 dB under the tightest threshold this suite asserts (D-10)") {

	const int K = 389;   // odd, and the 44.1 kHz C8 row of the grid below

	// =======================================================================
	// PART A — THE POSITIVE CONTROL.
	//
	// A pure cosine at an integer cycle count has its entire energy on bins K
	// and N-K and nothing anywhere else. It is a single harmonic, so it has NO
	// alias energy by construction, and the classifier must say so. This proves
	// the classifier does not MANUFACTURE a floor out of its own arithmetic —
	// which is the failure that would make every threshold in plans 32-03 and
	// 32-07 a measurement of this file rather than of the DSP.
	// =======================================================================
	{
		std::vector<float> pure;
		pure.reserve(kSpectrumN);
		for (int i = 0; i < kSpectrumN; ++i)
			pure.push_back((float)std::cos(2.0 * kPi * (double)K * (double)i / (double)kSpectrumN));

		int aliasBin = -1;
		double aliasRmsDb = 0.0;
		const double peakDb = aliasPeakDb(pure, K, &aliasBin, &aliasRmsDb);
		CAPTURE(peakDb);
		CAPTURE(aliasRmsDb);
		CAPTURE(aliasBin);

		// Non-vacuity first: the block must actually be a signal. A silent block
		// would return the -999 sentinel and sail through the bound below.
		REQUIRE(pure.size() == (std::size_t)kSpectrumN);
		REQUIRE(peakDb > -900.0);

		// Float storage of the samples is the only noise source, and it is
		// incoherent across the block while the fundamental adds coherently, so
		// the ratio sits far below -140 dB. Measured here: about -170 dB.
		CHECK(peakDb < -140.0);
	}

	// =======================================================================
	// PART B — THE DETECTION CONTROL. THE LOAD-BEARING HALF OF THIS CASE.
	//
	// >>> DO NOT DELETE THIS BLOCK. <<<
	//
	// Part A can only ever say "the classifier reported nothing". That is
	// equally consistent with a classifier that WORKS and with one that is
	// incapable of reporting anything at all — a detector observed only green
	// is indistinguishable from a detector that cannot fail. This block is the
	// other half: a spur of known amplitude is planted at a known NON-harmonic
	// bin, and the classifier must find it, name its bin exactly, and report
	// its amplitude to within 0.1 dB.
	//
	// The bin is K + 1 = 390, which is not a multiple of K = 389 and is
	// therefore not in H = { 389, 778, 1167, 1556, 1945 }. The amplitude is
	// 1e-3 against a fundamental of 1.0, so the expected report is exactly
	// 20*log10(1e-3) = -60.0 dB.
	//
	// This is the posture of check_frozen.sh [3/3], check_includes.sh [6/7],
	// check_canary.sh [4/5] and tests/test_vco_core.cpp invariant 5, and it is
	// the posture this file is written in.
	// =======================================================================
	{
		const int spurBin = K + 1;
		const double spurAmp = 1e-3;

		std::vector<float> spurred;
		spurred.reserve(kSpectrumN);
		for (int i = 0; i < kSpectrumN; ++i) {
			const double t = 2.0 * kPi * (double)i / (double)kSpectrumN;
			spurred.push_back((float)(std::cos(t * (double)K) + spurAmp * std::cos(t * (double)spurBin)));
		}

		int aliasBin = -1;
		double aliasRmsDb = 0.0;
		const double peakDb = aliasPeakDb(spurred, K, &aliasBin, &aliasRmsDb);
		CAPTURE(peakDb);
		CAPTURE(aliasBin);
		CAPTURE(aliasRmsDb);

		// The spur is not a harmonic of K — assert it rather than trusting the
		// arithmetic above, because a future edit to K could silently make it one
		// and turn this control into a measurement of the harmonic set.
		REQUIRE((spurBin % K) != 0);
		REQUIRE(peakDb > -900.0);

		// The classifier found the spur, in the right bin, at the right level.
		// The amplitude is compared with an EXPLICIT absolute tolerance, never
		// with doctest's approximate comparator: that comparator applies a
		// relative-scaling margin, which is the wrong shape for a dB figure and
		// is not a tolerance anyone reading this line would be able to state.
		CHECK(aliasBin == spurBin);
		CHECK(std::fabs(peakDb - (-60.0)) < 0.1);
	}

	// =======================================================================
	// PART C — THE D-10 LEAKAGE SELF-CHECK, over the pinned grid.
	//
	// MEASURED THIS SESSION, and reproducing 32-RESEARCH.md § Validation
	// Architecture row for row:
	//
	//   rate     K    pitchCV       method-1 bins   leak dB   method-2 bins   leak dB   dt dev
	//   44100    97   1.997047424    2.739e-04      -71.25     1.551e-06     -116.19   +2.81 ppm
	//   44100   195   3.004463196    7.972e-04      -61.97     5.301e-07     -125.51   +4.09 ppm
	//   44100   389   4.000755310    1.489e-03      -56.54     8.365e-06     -101.55   -3.85 ppm
	//   44100   777   4.998908997    2.864e-04      -70.86     2.528e-05      -91.95   +0.40 ppm
	//   48000   357   3.999168158    1.545e-03      -56.22     1.301e-05      -97.71   +4.37 ppm
	//   96000   179   4.003196716    2.272e-04      -72.87     7.182e-06     -102.87   -1.31 ppm
	//
	// Method one's whole column sits between -56 and -73 dB, which is the
	// envelope RESEARCH predicted — and which is NOT 10 dB below the -62 dB
	// sine row on four of the six rows. That is why the self-check below is
	// asserted against method two, exactly as RESEARCH's switching rule
	// requires. Method one is still measured and still bounded, because plans
	// 32-03 and 32-07 drive the shared harness with it for every row whose
	// threshold is loose enough, and a silent regression in it would move those
	// rows off their bin centres.
	//
	// IF ANY OF THESE FIGURES EVER MOVES, STOP AND REPORT IT RATHER THAN
	// UPDATING THE NUMBER. They are properties of forge::exp2_taylor5 and of
	// the guard chain in src/dsp/VcoCore.hpp:339-472. A moved figure is a
	// finding about the core, not bookkeeping here.
	// =======================================================================
	{
		struct Row { double sr; int K; const char* role; };

		static const Row GRID[] = {
			// (a) 44.1 kHz, the four-note column. C6 is the diagnostic row; C7,
			//     C8 and C9 are what the gate asserts on (operator decision,
			//     2026-08-01, superseding the undefined "top two octaves").
			{ 44100.0,  97, "44.1k C6 ~1044.4 Hz, 21 harmonics below Nyquist (diagnostic row)" },
			{ 44100.0, 195, "44.1k C7 ~2099.5 Hz, 10 harmonics — carries the TIGHTEST threshold (-62 dB sine)" },
			{ 44100.0, 389, "44.1k C8 ~4188.2 Hz, 5 harmonics — the cross-rate reference note" },
			{ 44100.0, 777, "44.1k C9 ~8366.9 Hz, 2 harmonics — the hardest row for every shape" },
			// (b) The D-11 cross-rate pair. Both land on the SAME note as the
			//     44.1 kHz C8 row above, so the cross-rate regression compares
			//     like with like rather than comparing two different notes.
			{ 48000.0, 357, "48k C8 ~4183.6 Hz, 5 harmonics — D-11 cross-rate, same note as 44.1k C8" },
			{ 96000.0, 179, "96k C8 ~4195.3 Hz, 11 harmonics — D-11 cross-rate, same note as 44.1k C8" },
		};
		const std::size_t nRows = sizeof(GRID) / sizeof(GRID[0]);

		// The tightest threshold this suite will ever assert is the D-09 sine
		// row: -62 dB at C7, -64 dB at C8. D-10 requires the gate's own floor to
		// sit at least 10 dB below whatever it asserts, so -74 dB is the real
		// bar. -72.0 is used below because the plan pins the self-check to
		// "10 dB below -62.0"; method two clears BOTH by 20 dB or more, so the
		// distinction costs nothing and the looser of the two is asserted.
		const double kTightestThresholdDb = -62.0;
		const double kSelfCheckDb = kTightestThresholdDb - 10.0;

		for (std::size_t r = 0; r < nRows; ++r) {
			const double sr = GRID[r].sr;
			const int Kr = GRID[r].K;
			const char* role = GRID[r].role;
			CAPTURE(sr);
			CAPTURE(Kr);
			CAPTURE(role);

			// ---- The coprimality assertion. -----------------------------------
			// The mechanical form of the whole bin classification: see the
			// coprimality argument in the file banner. gcd(Kr, 4096) = 1 for every
			// odd Kr, and that is what makes "non-harmonic bin" mean "alias".
			CHECK((Kr % 2) == 1);

			// This rate is one of the three production rates, not an invented one.
			bool rateIsProduction = false;
			for (std::size_t s = 0; s < sizeof(SAMPLE_RATES) / sizeof(SAMPLE_RATES[0]); ++s)
				if (SAMPLE_RATES[s] == sr) rateIsProduction = true;
			CHECK(rateIsProduction);

			// ---- METHOD ONE: bisect pitchCV, harness unchanged. ----------------
			double achievedBinError = 0.0;
			const float pitchCV = binCentredPitchCV(sr, Kr, &achievedBinError);
			const double leakagePitchDb = impliedLeakageDb(achievedBinError);
			CAPTURE(pitchCV);
			CAPTURE(achievedBinError);
			CAPTURE(leakagePitchDb);

			// ---- VALIDITY FIRST: the solver's mirrored chain must agree with the
			//      LIVE core, bit-exactly. -----------------------------------------
			// deltaPhaseForPitchCV replicates src/dsp/VcoCore.hpp's pitch/guard
			// sequence by hand. If it ever drifts from the real thing, every
			// frequency this file solves for lands off its bin centre and the
			// spectra grow leakage that looks exactly like alias energy — and
			// nothing downstream can tell the difference. So the prediction is
			// checked against what forge::VcoCore ACTUALLY computed, read from its
			// own telemetry after one real step through the real harness. The same
			// habit as tests/test_vco_core.cpp:1040-1057.
			{
				float predictedFreq = 0.f;
				const float dt = (float)(1.0 / sr);
				const double predictedDp = deltaPhaseForPitchCV(pitchCV, sr, dt, &predictedFreq);

				forge::VcoInputs in;
				in.pitchCV   = pitchCV;
				in.coarse    = 0.f;
				in.fine      = 0.f;
				in.morph     = 0.f;
				in.character = 0.f;
				in.drift     = 0.f;

				forge::VcoBlockDriver d(sr);
				std::vector<float> out = d.run(1, [=](int) { return in; });
				REQUIRE(out.size() == (std::size_t)1);

				const float liveFreq = d.core.tel.freqHz;
				CAPTURE(predictedFreq);
				CAPTURE(liveFreq);
				REQUIRE(liveFreq == predictedFreq);

				// And the increment the core would accumulate is the one solved for.
				const double liveDp = (double)liveFreq * (double)d.core.tel.lastSampleTime;
				CAPTURE(predictedDp);
				CAPTURE(liveDp);
				REQUIRE(liveDp == predictedDp);
			}

			// Method one's achieved error. 2e-3 bins is the bound; note that it is
			// the SAME assertion as "implied leakage below -53.98 dB", because the
			// two are related by 20*log10 — the bin figure is asserted rather than
			// the dB figure only because the bin figure is what the solver reports.
			CHECK(achievedBinError < 2e-3);
			CHECK(achievedBinError > 0.0);

			// ---- METHOD TWO: nudge the injected sampleTime. --------------------
			double achievedBinErrorDt = 0.0;
			const float nudgedDt = binCentredSampleTime(sr, pitchCV, Kr, &achievedBinErrorDt);
			const double leakageDtDb = impliedLeakageDb(achievedBinErrorDt);
			const double nominalDt = (double)(float)(1.0 / sr);
			const double dtDeviationPpm = ((double)nudgedDt / nominalDt - 1.0) * 1e6;
			CAPTURE(nudgedDt);
			CAPTURE(achievedBinErrorDt);
			CAPTURE(leakageDtDb);
			CAPTURE(dtDeviationPpm);

			// THE D-10 SELF-CHECK ITSELF. The gate's own noise floor sits at least
			// 10 dB below the tightest threshold it will ever assert, so no
			// threshold in plans 32-03 or 32-07 can be satisfied by the
			// measurement apparatus rather than by the DSP.
			CHECK(leakageDtDb <= kSelfCheckDb);

			// The nudge stays inside RESEARCH's measured 5 ppm envelope. A runaway
			// nudge would mean method two is compensating for a broken chain
			// rather than for float granularity.
			CHECK(std::fabs(dtDeviationPpm) <= 5.0);
		}
	}
}

// ---------------------------------------------------------------------------
// The D-08 baseline is only a baseline if it is a FAITHFUL copy of the core.
// This case is what makes it one — and it is a TOMBSTONE.
// ---------------------------------------------------------------------------
TEST_CASE("vco spectrum: NaiveVcoCoreMirror is bit-identical to the live forge::VcoCore (D-08 baseline validity) - THIS CASE INVERTS IN PLAN 32-06") {

	// The bin-centred C8 row at each rate, from the pinned grid. Driving the
	// mirror at a bin centre is not required for a bit-identity check — any
	// pitch would do — but it means this case exercises the SAME frequencies
	// plans 32-03 and 32-07 measure at, so a divergence that only shows up at
	// the grid's own operating point cannot hide from it.
	struct RateRow { double sr; int K; };
	static const RateRow RATES[] = {
		{ 44100.0, 389 },   // C8
		{ 48000.0, 357 },   // C8, D-11 cross-rate
		{ 96000.0, 179 },   // C8, D-11 cross-rate
	};

	static const float MORPHS[]     = {0.00f, 0.25f, 0.50f, 0.75f, 1.00f};
	static const float CHARACTERS[] = {0.0f, 0.5f, 1.0f};

	const int n = kSpectrumN;

	for (std::size_t ri = 0; ri < sizeof(RATES) / sizeof(RATES[0]); ++ri) {
		const double sr = RATES[ri].sr;
		const int K = RATES[ri].K;
		CAPTURE(sr);
		CAPTURE(K);

		double binErr = 0.0;
		const float pitchCV = binCentredPitchCV(sr, K, &binErr);
		CAPTURE(pitchCV);

		for (std::size_t mi = 0; mi < sizeof(MORPHS) / sizeof(MORPHS[0]); ++mi) {
			for (std::size_t ci = 0; ci < sizeof(CHARACTERS) / sizeof(CHARACTERS[0]); ++ci) {
				const float morph = MORPHS[mi];
				const float character = CHARACTERS[ci];
				CAPTURE(morph);
				CAPTURE(character);

				forge::VcoInputs base;
				base.pitchCV   = pitchCV;
				base.coarse    = 0.f;
				base.fine      = 0.f;
				base.morph     = morph;
				base.character = character;
				base.drift     = 0.f;

				// --- The live core, through the shared harness. ------------------
				forge::VcoBlockDriver d(sr);
				std::vector<float> coreOut = d.run(n, [=](int) { return base; });
				REQUIRE(coreOut.size() == (std::size_t)n);

				// --- The mirror, through a local loop that injects sampleTime and
				//     sampleRate IDENTICALLY. ------------------------------------
				// THE FOUR SEED LITERALS ARE COPIED VERBATIM from
				// tests/VcoBlockDriver.hpp:42-43 — the driver's own defaults — and
				// must never be invented. They are documented and proven
				// non-degenerate there for a reason that is not stylistic: a
				// forge::Xoroshiro128Plus seeded (0, 0) is a fixed point emitting
				// an all-zero stream, which makes std::normal_distribution's
				// rejection loop never terminate. In a test that is a hung suite;
				// in Rack it is a HANG ON PATCH LOAD.
				NaiveVcoCoreMirror mirror;
				mirror.seed(0x1234ULL, 0x5678ULL);
				mirror.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);

				const float dt = (float)(1.0 / sr);
				std::vector<float> mirrorOut;
				mirrorOut.reserve(n);
				for (int i = 0; i < n; ++i) {
					forge::VcoInputs in = base;
					in.sampleTime = dt;
					in.sampleRate = (float)sr;
					mirrorOut.push_back(mirror.step(in));
				}
				REQUIRE(mirrorOut.size() == coreOut.size());

				// --- NON-VACUITY FIRST. ----------------------------------------
				// "Identical" is trivially satisfied by two blocks of silence, and
				// it is nearly as trivially satisfied by two DC blocks. Both are
				// asserted away BEFORE the identity claim is made, the same habit
				// as tests/test_vco_core.cpp:1040-1057: assert the fixture tests
				// what it claims before asserting the result.
				int nonZero = 0;
				bool constantBlock = true;
				for (int i = 0; i < n; ++i) {
					if (coreOut[i] != 0.f) ++nonZero;
					if (coreOut[i] != coreOut[0]) constantBlock = false;
				}
				CAPTURE(nonZero);
				REQUIRE(nonZero >= (n * 9) / 10);
				REQUIRE(constantBlock == false);

				// --- THE IDENTITY CLAIM. ---------------------------------------
				// Counted with a DIRECT float !=, never doctest's approximate
				// comparator. That comparator applies a relative-scaling margin
				// even at epsilon(0), so it is not a bit-exact comparator and
				// would quietly accept the very small corrections plan 32-06 is
				// about to introduce — the one class of difference this case
				// exists to see.
				int differing = 0;
				for (int i = 0; i < n; ++i)
					if (coreOut[i] != mirrorOut[i]) ++differing;

				// >>> THIS ASSERTION IS A TOMBSTONE. <<<
				//
				// It is true only for exactly as long as forge::VcoCore::step()
				// applies no band-limiting correction. PLAN 32-06 MUST INVERT IT
				// IN PLACE — same slot, same 45-point grid, same three rates, same
				// four seed literals — into an assertion that the two now DIVERGE,
				// and that they diverge by exactly the correction
				// forge::MorphBlep::step returns.
				//
				// DELETING THIS CASE INSTEAD OF INVERTING IT silently removes the
				// only proof that NaiveVcoCoreMirror is a faithful copy of the
				// core, and therefore the only reason plan 32-07's no-regression
				// invariant means anything. This is the shape Phase 29's silence
				// tombstone used and Phase 30 honoured (D-15/D-19: inverted in
				// place, same slot, and OBSERVED red against a silenced core).
				CAPTURE(differing);
				CHECK(differing == 0);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// THE D-08 BASELINE. This case's job is to RECORD, not to judge.
//
// D-08 requires the alias floor of today's deliberately-aliased oscillator to be
// measured per shape, per note and per character BEFORE any band-limiting
// exists, so that the thresholds this phase pins are set FROM MEASUREMENT rather
// than inherited from prose. Three payoffs, in the phase's own words: the
// threshold is measured rather than inherited; the phase gets an objective
// iteration metric instead of ear-guessing, which is what its deliberate
// iteration budget is for; and the RED that follows is genuine, because the gate
// provably fails before forge::MorphBlep lands rather than being written against
// already-passing code.
//
// SO IT ASSERTS ALMOST NOTHING, DELIBERATELY. Only two structural sanity
// properties are CHECKed, both with wide measured margins so they are robust
// across toolchains, and both there to prove the apparatus is looking at a real
// signal rather than at silence. Everything else is CAPTUREd. The recorded
// figures are what plan 32-07 pins against; adding assertions here would pin the
// naive floor as a REQUIREMENT, and the naive floor is the thing this phase
// exists to move.
//
// THE PER-CELL D-10 SELF-CHECK IS THE ONE HARD REQUIRE (T-32-11). Before any
// cell's alias value is read, the leakage implied by that cell's ACHIEVED bin
// error must sit at least 10 dB below that cell's own threshold. Without it the
// gate can pass by measuring forge::exp2_taylor5's output granularity rather
// than the DSP — D-10's stated failure mode — and every threshold below it
// becomes decoration.
// ---------------------------------------------------------------------------
TEST_CASE("vco spectrum: the NAIVE alias floor, recorded per shape, note and character (D-08 baseline)") {

	const std::size_t nCells = sizeof(SPECTRUM_GRID) / sizeof(SPECTRUM_GRID[0]);
	CAPTURE(nCells);

	// 45 gated + 15 diagnostic + 30 cross-rate regression. Asserted rather than
	// trusted: a table that silently lost a section would still walk cleanly and
	// would still report a floor, and nothing else here could tell.
	REQUIRE(nCells == (std::size_t)90);

	int gatedCells = 0, diagnosticCells = 0, regressionCells = 0;
	int sineChar0Cells = 0, pulseC9Char0Cells = 0;
	int methodOneCells = 0, methodTwoCells = 0;

	for (std::size_t i = 0; i < nCells; ++i) {
		const SpectrumCell& cell = SPECTRUM_GRID[i];

		const double sr        = cell.sr;
		const int    K         = cell.K;
		const float  morph     = cell.morph;
		const float  character = cell.character;
		const float  threshold = cell.thresholdDb;

		// The three label columns are copied into std::string before being
		// CAPTUREd. doctest stringifies a bare `const char*` as a POINTER unless
		// DOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING is defined project-wide, and
		// a -s dump full of `note := 0x100453f6b` would make the recorded
		// baseline unreadable — which for a case whose entire job is to RECORD
		// would defeat the case. That macro is deliberately NOT defined here:
		// it is a global doctest configuration switch and would change how every
		// other TU in the suite renders, including the shipped LFO's cases.
		const std::string note(cell.note);
		const std::string region(cell.region);
		const std::string tier(cell.tier);

		CAPTURE(i);
		CAPTURE(sr);
		CAPTURE(K);
		CAPTURE(note);
		CAPTURE(morph);
		CAPTURE(region);
		CAPTURE(character);
		CAPTURE(threshold);
		CAPTURE(tier);

		// The coprimality assertion, in its mechanical form: gcd(K, 4096) = 1
		// for every odd K, and that is what makes "non-harmonic bin" mean
		// "alias". See the file banner.
		REQUIRE((K % 2) == 1);

		// Every cell carries a written source for its threshold.
		REQUIRE(cell.provenance != 0);
		REQUIRE(cell.thresholdDb >= kThresholdFloorDb);

		if (tier == "gated") ++gatedCells;
		else if (tier == "diagnostic") ++diagnosticCells;
		else if (tier == "regression") ++regressionCells;

		// ---- THE MEASUREMENT. useMirror = true: this is the NAIVE baseline. --
		double aliasRmsDb = 0.0;
		double binError   = 0.0;
		int    method     = 0;
		const double naiveDb = measureCellDb(cell, /*useMirror=*/true, &aliasRmsDb, &binError, &method);
		const double impliedLeakage = impliedLeakageDb(binError);

		CAPTURE(method);
		CAPTURE(binError);
		CAPTURE(impliedLeakage);
		CAPTURE(naiveDb);
		CAPTURE(aliasRmsDb);

		if (method == kMethodPitchCV) ++methodOneCells; else ++methodTwoCells;

		// ---- THE D-10 SELF-CHECK, PER CELL, BEFORE THE VALUE IS READ. -------
		// The gate's own noise floor sits at least 10 dB below the threshold
		// this cell will be judged against, so no figure recorded below can be
		// an artefact of where the drive frequency landed relative to its bin
		// centre. This is a REQUIRE, not a CHECK: a cell whose instrument is
		// out of specification has no business reporting a number at all.
		REQUIRE(impliedLeakage <= (double)threshold - 10.0);

		// Non-vacuity: -999.0 is aliasPeakDb's silence sentinel, and it is far
		// BELOW every threshold here — the wrong direction for a sentinel to
		// fail. Catch it explicitly rather than letting silence look clean.
		REQUIRE(naiveDb > -900.0);

		// ---- STRUCTURAL SANITY 1: a sine at character 0 has nothing to alias.
		//
		// It has no discontinuity of its own and no bleed ring, so it emits NO
		// alias energy at all and everything the classifier reports for it is
		// the INSTRUMENT'S OWN FLOOR — the rectangular-window leakage of a drive
		// frequency that could not be placed exactly on the bin centre.
		//
		// MEASURED, and this is the whole point: at all six sine cells the
		// reported alias peak sits within 0.078 dB of that cell's own implied
		// leakage. 44.1k C7 -125.435 against a floor of -125.513 (+0.078);
		// 44.1k C8 -101.541 against -101.550 (+0.009); 44.1k C9 -91.9421
		// against -91.9452 (+0.003); 44.1k C6 -116.141 against -116.186
		// (+0.045); 48k C8 -97.7016 against -97.7123 (+0.011); 96k C8 -102.852
		// against -102.875 (+0.023). The sine is not merely quiet — it is
		// EXACTLY the floor, at every rate and every note.
		//
		// >>> WHY THIS IS NOT THE "BELOW -140 dB" BOUND THE PLAN NAMED. <<<
		// The 32-RESEARCH prototype measured -150.7 dB for this cell. THAT
		// FIGURE IS UNREACHABLE THROUGH THIS APPARATUS, and not because the
		// oscillator is worse: it is because -150.7 dB is 25 to 59 dB BELOW this
		// gate's own leakage floor on every row of the grid (-91.95 to -125.51
		// dB, plan 32-01's measured method-two column). An instrument cannot
		// report a number quieter than its own noise, so a -140 dB CHECK here
		// would not be a stronger claim about the DSP — it would be a claim the
		// measurement cannot carry, which is exactly what the D-10 self-check
		// three lines above exists to forbid. Asserting AGAINST THE FLOOR is the
		// stronger statement anyway: it says the sine contributes nothing
		// measurable, at whatever floor the instrument happens to have that day,
		// rather than fixing a number that only holds on one apparatus.
		//
		// The 1.0 dB margin is a 12x cushion on the worst measured excess
		// (0.078 dB). The absolute bound is the second half: it pins the floor
		// far below every threshold on the grid, so a future apparatus
		// regression that lifted BOTH the floor and this cell together — keeping
		// the delta small while the whole measurement went soft — still fails
		// here. Worst measured is -91.94 dB, so -85.0 leaves 6.9 dB.
		if (morph == 0.00f && character == 0.00f) {
			++sineChar0Cells;
			CHECK(naiveDb <= impliedLeakage + 1.0);
			CHECK(naiveDb < -85.0);
		}

		// ---- STRUCTURAL SANITY 2: the naive worst case is genuinely aliased.
		// The 5 % pulse at C9 carries two harmonics below Nyquist and folds
		// essentially everything else back. RESEARCH measured -0.3 dB — the
		// alias is as loud as the fundamental. This is the proof the apparatus
		// is looking at a real, badly aliased signal rather than at silence, and
		// -5.0 dB is the bound.
		if (morph == 1.00f && K == 777 && character == 0.00f) {
			++pulseC9Char0Cells;
			CHECK(naiveDb > -5.0);
		}
	}

	// The tier census, and the non-vacuity of both structural properties. A
	// grid that lost its pulse C9 cell, or whose sine column moved, would make
	// the two CHECKs above silently unreachable — an assertion that never runs
	// is indistinguishable from one that cannot fail.
	CAPTURE(gatedCells);
	CAPTURE(diagnosticCells);
	CAPTURE(regressionCells);
	CAPTURE(sineChar0Cells);
	CAPTURE(pulseC9Char0Cells);
	CAPTURE(methodOneCells);
	CAPTURE(methodTwoCells);
	CHECK(gatedCells == 45);
	CHECK(diagnosticCells == 15);
	CHECK(regressionCells == 30);
	CHECK(sineChar0Cells == 6);
	CHECK(pulseC9Char0Cells == 1);
}

// ---------------------------------------------------------------------------
// >>> THIS CASE IS A TOMBSTONE. <<<
//
// WHY THIS CASE EXISTS. D-08 requires the Phase 32 alias-floor gate to be
// OBSERVED FAILING against the naive core before forge::MorphBlep lands. A gate
// written against already-passing code proves nothing — it is indistinguishable
// from a gate that cannot fail — and this phase's whole iteration budget is
// spent against an objective metric rather than against ear-guessing, so the
// metric has to be shown to bite first. The gate below was written in its FINAL
// form, run against the naive forge::VcoCore, and observed RED. This case is
// that failure, pinned.
//
// >>> WHAT PLAN 32-07 MUST DO. <<<
// INVERT THIS CASE IN PLACE — same slot in this file, same SPECTRUM_GRID, same
// 45 gated cells, same measureCellDb call with useMirror = false — into
// CHECK(failing == 0) against the real forge::MorphBlep, and flip the five named
// subset assertions from "misses its threshold by more than 5 dB" to
// CHECK(correctedDb <= cell.thresholdDb). DELETING THIS CASE INSTEAD OF
// INVERTING IT silently removes the phase's only recorded RED, and with it the
// only evidence that the gate was ever able to fail. This is the shape Phase
// 29's silence tombstone used and Phase 30 honoured (D-15 / D-19: inverted in
// place, same slot, and observed red against a silenced core).
//
// >>> THE STANDING WARNING. <<<
// IF THIS CASE EVER GOES GREEN WHILE forge::VcoCore::step IS STILL NAIVE, the
// gate has stopped being able to see aliasing at all, and every threshold in
// SPECTRUM_GRID above is decoration. A green result here does not mean the
// oscillator improved — the oscillator is supposed to be aliasing. It means the
// measurement stopped working, which is precisely the condition that would let
// plan 32-07 land a broken forge::MorphBlep unnoticed.
//
// >>> THE ANTI-SOFTENING CLAUSE. <<<
// A threshold that turns out to be unreachable is ESCALATED per plan 32-07's
// documented procedure. It is NEVER quietly loosened, and the corrected column
// in SPECTRUM_GRID is NEVER edited to match whatever the implementation happened
// to produce. That edit would convert a measurement into a transcription of the
// result, which is the one failure mode a threshold cannot survive (T-32-15).
//
// ---------------------------------------------------------------------------
// THE OBSERVED FIGURES, recorded 2026-08-01 against the naive forge::VcoCore at
// commit 13ea1c7. IF ANY OF THESE EVER MOVES WHEN THIS CASE IS TOUCHED, STOP AND
// REPORT IT RATHER THAN UPDATING THE NUMBER — the same standing instruction as
// tests/test_vco_core.cpp:344-347. A moved figure means the change was not
// inert; it is a finding about forge::VcoCore, not bookkeeping here.
//
//   failing cells: 32 of the 45 gated cells
//
//   the five named large-margin subset cells, measured over-threshold margins:
//     44100 / K=389 / morph 0.25 triangle / character 0.00   -33.8085 vs -45  -> +11.191 dB
//     44100 / K=389 / morph 0.50 saw      / character 0.00   -15.5630 vs -22  ->  +6.437 dB
//     44100 / K=389 / morph 0.75 square   / character 0.00   -16.9030 vs -28  -> +11.097 dB
//     44100 / K=389 / morph 1.00 pulse    / character 0.00    -1.2931 vs  -8  ->  +6.707 dB
//     44100 / K=777 / morph 0.50 saw      / character 0.00    -9.5424 vs -16  ->  +6.458 dB
//
// The 13 gated cells that ALREADY PASS against the naive core are not a defect
// in the grid and must not be "fixed" by tightening their thresholds. Ten of
// them are the sine and high-character cells P-6 predicts: the sine at character
// 0 has no discontinuity to alias, and the triangle at C8 / character 1 improves
// by exactly 0.0 dB because the corner is already 7.7 samples wide and the D-03
// factor correctly returns zero. A grid where every cell were red would mean the
// thresholds had been set to fail rather than to measure.
// ---------------------------------------------------------------------------
TEST_CASE("vco spectrum: TOMBSTONE - the NAIVE core FAILS the Phase 32 alias-floor gate (D-08 RED evidence) - INVERTS IN PLAN 32-07") {

	// THE OBSERVED COUNT WAS 32. This constant is 32 minus 5, and the gap is
	// deliberate: a cell sitting within a decibel or two of its threshold can
	// flip across it on a different toolchain — cell i = 19 (triangle C8,
	// character 0.5) misses by only 1.768 dB and cell i = 34 (triangle C9,
	// character 0.5) by 2.026 dB — and a floor pinned at the observed count
	// would turn that into a red build for a reason that has nothing to do with
	// the DSP.
	//
	// THE OBSERVED NUMBER IS A MEASUREMENT; THIS CONSTANT IS A FLOOR DERIVED
	// FROM IT. Do not later "tighten" the floor to the observed count. The floor
	// is not trying to be precise about how naive the naive core is — it is
	// asserting that the gate can see a large, unambiguous population of
	// failures, which is what makes plan 32-07's inversion to zero meaningful.
	const int kNaiveFailuresFloor = 27;

	// The five cells named in the plan, where 32-RESEARCH shows at least 8 dB of
	// expected improvement from band-limiting and the naive value therefore
	// misses its threshold by a wide margin. These are what make the tombstone
	// SPECIFIC rather than merely statistical: a counter alone could be
	// satisfied by 27 cells each missing by 0.1 dB, which would be a much weaker
	// claim about how far the naive core actually is from the gate.
	//
	// The 5 dB cushion is what makes them robust: the tightest measured margin
	// in the set is +6.437 dB, so every one of them has at least 1.4 dB of room
	// before the assertion is in danger.
	struct SubsetCell { double sr; int K; float morph; float character; };
	static const SubsetCell SUBSET[] = {
		{ 44100.0, 389, 0.25f, 0.00f },   // triangle, measured +11.191 dB over threshold
		{ 44100.0, 389, 0.50f, 0.00f },   // saw,      measured  +6.437 dB over threshold
		{ 44100.0, 389, 0.75f, 0.00f },   // square,   measured +11.097 dB over threshold
		{ 44100.0, 389, 1.00f, 0.00f },   // pulse,    measured  +6.707 dB over threshold
		{ 44100.0, 777, 0.50f, 0.00f },   // saw at C9, measured +6.458 dB over threshold
	};
	const std::size_t nSubset = sizeof(SUBSET) / sizeof(SUBSET[0]);

	const std::size_t nCells = sizeof(SPECTRUM_GRID) / sizeof(SPECTRUM_GRID[0]);
	REQUIRE(nCells == (std::size_t)90);

	int gatedWalked = 0;
	int failing = 0;
	int subsetChecked = 0;

	for (std::size_t i = 0; i < nCells; ++i) {
		const SpectrumCell& cell = SPECTRUM_GRID[i];

		// Only the gated tier. The C6 diagnostic row and the D-11 cross-rate
		// rows are recorded by the baseline case above and are NOT asserted
		// here — the gate's note set is C7, C8 and C9 (operator decision,
		// 2026-08-01).
		if (std::string(cell.tier) != "gated") continue;
		++gatedWalked;

		const double sr        = cell.sr;
		const int    K         = cell.K;
		const float  morph     = cell.morph;
		const float  character = cell.character;
		const float  threshold = cell.thresholdDb;
		const std::string note(cell.note);
		const std::string region(cell.region);

		CAPTURE(i);
		CAPTURE(sr);
		CAPTURE(K);
		CAPTURE(note);
		CAPTURE(morph);
		CAPTURE(region);
		CAPTURE(character);
		CAPTURE(threshold);

		// >>> useMirror = false. THIS IS THE LIVE forge::VcoCore, NOT THE
		//     MIRROR. <<< The whole point of the RED is that the gate fails
		//     against the SHIPPED code path. Measuring the mirror here would
		//     make this case a statement about a test fixture, and plan 32-07's
		//     inversion would then be comparing the corrected core against
		//     nothing at all.
		double aliasRmsDb = 0.0;
		double binError   = 0.0;
		int    method     = 0;
		const double naiveDb = measureCellDb(cell, /*useMirror=*/false, &aliasRmsDb, &binError, &method);
		const double impliedLeakage = impliedLeakageDb(binError);

		CAPTURE(method);
		CAPTURE(binError);
		CAPTURE(impliedLeakage);
		CAPTURE(naiveDb);
		CAPTURE(aliasRmsDb);

		// The D-10 self-check, per cell, before the value is read — identical to
		// the baseline case above and for the identical reason (T-32-11). A red
		// cell measured by an out-of-specification instrument is not evidence of
		// anything.
		REQUIRE(impliedLeakage <= (double)threshold - 10.0);
		REQUIRE(naiveDb > -900.0);

		if (naiveDb > (double)threshold) ++failing;

		// ---- The named large-margin subset. --------------------------------
		for (std::size_t s = 0; s < nSubset; ++s) {
			if (SUBSET[s].sr != sr || SUBSET[s].K != K) continue;
			if (SUBSET[s].morph != morph || SUBSET[s].character != character) continue;
			++subsetChecked;
			const double marginDb = naiveDb - (double)threshold;
			CAPTURE(marginDb);

			// >>> PLAN 32-07 FLIPS THIS LINE to CHECK(naiveDb <= (double)threshold). <<<
			CHECK(naiveDb > (double)threshold + 5.0);
		}
	}

	// Non-vacuity of the whole case. A grid edit that dropped the gated tier, or
	// that moved one of the five subset cells, would make every assertion above
	// silently unreachable — and an assertion that never runs is
	// indistinguishable from one that cannot fail.
	CAPTURE(gatedWalked);
	CAPTURE(subsetChecked);
	CAPTURE(failing);
	REQUIRE(gatedWalked == 45);
	REQUIRE(subsetChecked == (int)nSubset);

	// >>> PLAN 32-07 FLIPS THIS LINE to CHECK(failing == 0). <<<
	CHECK(failing >= kNaiveFailuresFloor);
}
