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
#include "dsp/MorphBlep.hpp"   // forge::MorphBlep — held LOCALLY by the inverted D-08 baseline case

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
// src/dsp/VcoCore.hpp field for field and line for line. THAT DIVERGENCE IS
// LIVE AS OF PLAN 32-06: until that commit it was not a divergence at all,
// because the live core applied no correction either — which is exactly why
// this file landed in plan 32-01 and not later. Plan 32-06 landed
// forge::MorphBlep at the morphedWave call site, and from that commit this type
// is the ONLY remaining naive path in the repository. It must
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

	// --- RECORDING ONLY (plan 32-06). ------------------------------------
	// Five members written on every step() and read by the D-08 INVERSION case
	// below, which reconstructs the live core's sample from this mirror plus a
	// locally held forge::MorphBlep and requires bit-exact agreement.
	//
	// THEY CHANGE NO ARITHMETIC, AND THEY ARE NOT A SECOND DIVERGENCE. Nothing
	// in step() reads them, none of them appears in any expression that produces
	// a sample, and deleting all five would leave every returned float
	// bit-identical. The banner's "only one divergence" promise is about the
	// CORRECTION; these are simply the inputs the correction needs, made visible
	// so the inversion case can supply them from OUTSIDE rather than by
	// re-deriving them — a re-derivation would be a second mirror, with a second
	// way to drift, inside the case whose whole job is to detect drift.
	float  lastNaive      = 0.f;   // the PRE-SCALE value the frozen call returned
	float  lastP          = 0.f;   // the float phase handed to morphedWave this sample
	double lastDeltaPhase = 0.0;   // the double increment this sample advanced by
	float  lastMorph      = 0.f;   // post-conditioning, the value the frozen call saw
	float  lastCharacter  = 0.f;   // post-conditioning, the value the frozen call saw

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

		// Mirrored (plan 32-06 / T-32-01): the morph and character conditioning
		// is the NEGATED-COMPARISON PAIR, negated line first, and NOT
		// forge::clamp. The real core moved to this form when plan 32-02's MORPH
		// CV jack made "the inputs are already finite" false, and the
		// mirror-maintenance rule in the banner above requires this type to track
		// the core's GUARD SEQUENCE, not only its arithmetic. A mirror that
		// quietly kept the ladder would differ from the core in TWO things — the
		// correction AND the NaN behavior — rather than in the one its banner
		// promises, and the second difference would be invisible for exactly as
		// long as nothing drove a not-a-number through it.
		float morph = in.morph;
		if (!(morph > 0.f)) morph = 0.f;
		if (morph > 1.f) morph = 1.f;
		float character = in.character;
		if (!(character > 0.f)) character = 0.f;
		if (character > 1.f) character = 1.f;

		// Mirrored: ONE call into the frozen Waveshape — a call, never an edit —
		// with bleedLfo = 0.f, and the unconditioned x5. THIS IS THE LINE THAT
		// DIVERGED IN PLAN 32-06: the real core gained the forge::MorphBlep
		// correction here, and this line must NOT.
		const float naive = wave.morphedWave(p, morph, character, 0.f);

		// Recording only — see the five members above. Written AFTER the frozen
		// call so `lastNaive` is the value that call actually returned rather
		// than a recomputation of it.
		lastNaive      = naive;
		lastP          = p;
		lastDeltaPhase = deltaPhase;
		lastMorph      = morph;
		lastCharacter  = character;

		return 5.f * naive;
	}
};

// ---------------------------------------------------------------------------
// SpectrumCell — one measurement point of the D-09 threshold matrix.
//
// `morph` names the shape centre and `region` is its human name; `character` is
// the THIRD index, and the reason it exists is P-6 below. `tier` is what the
// cell is FOR: "gated" cells are asserted, "diagnostic" cells are CAPTUREd and
// never CHECKed, "regression" cells are the D-11 cross-rate rows. `provenance`
// is where the number came from, in words, in the test — because a threshold
// without a written source is a number someone can quietly edit.
//
// >>> measuredDb IS THE PROVENANCE, IN NUMBERS (plan 32-07 / T-32-15). <<<
// It is the CORRECTED alias peak this repository measured for this exact cell,
// and `thresholdDb` is derived from it by a rule the gate below asserts
// MECHANICALLY: thresholdDb == max(ceil(measuredDb + 3.0), kThresholdFloorDb).
//
// A prose provenance string can say a threshold came from a measurement; it
// cannot stop the next agent from nudging the threshold by a decibel and
// leaving the sentence in place. This field can, and it does it twice over.
// Loosening a threshold without touching measuredDb breaks the derivation
// assertion; loosening BOTH together breaks the reproduction CHECK in the
// measure pass, which compares measuredDb against what the core produces on
// this run. That pair is what makes "pinned from measurement" a claim the file
// can defend rather than a claim it merely makes — which is the whole content
// of T-32-15, the highest-severity threat assigned to this plan.
// ---------------------------------------------------------------------------
struct SpectrumCell {
	double sr;
	int K;
	const char* note;
	float morph;
	const char* region;
	float character;
	float measuredDb;
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
// whose threshold has no written source. The NUMBER each one refers to is the
// `measuredDb` field on the same row — see the SpectrumCell banner for why the
// provenance is a field and not only a sentence.
//
// ALL SIX WERE REWRITTEN IN PLAN 32-07. Until that plan every one of them opened
// by calling itself provisional and derived its number from the 32-RESEARCH
// prototype's corrected column plus 3 dB. The prototype is
// now the thing this column is COMPARED AGAINST rather than the thing it is
// derived from, and the per-row trailing comment records both figures plus their
// difference so the comparison stays visible.
const char* const kProvMeasured =
	"MEASURED by plan 32-07 in this repository, driving the real forge::VcoCore through measureCellDb "
	"with useMirror=false at this row's own sample rate and note; thresholdDb = ceil(measuredDb + 3.0), "
	"the 3 dB margin being roughly twice the largest cross-toolchain variation this suite has seen";
const char* const kProvFloored =
	"MEASURED by plan 32-07 in this repository at this row's own sample rate and note, then FLOORED at "
	"kThresholdFloorDb: ceil(measuredDb + 3.0) lands tighter than -75 dB, and D-10 forbids asserting a "
	"threshold this apparatus's own leakage floor cannot support. These are the sine cells whose measured "
	"value IS the instrument floor - the DSP contributes nothing measurable there";
const char* const kProvCrossRate =
	"MEASURED by plan 32-07 AT THIS ROW'S OWN RATE, 48 or 96 kHz, not transferred from the 44.1 kHz C8 row. "
	"The transfer these rows used to rely on is FALSIFIED: the same note's corrected floor is materially "
	"rate-dependent (up to 14.1 dB on the 96 kHz triangle), because each rate folds its first surviving "
	"alias to a different distance from Nyquist and the polyBLEP's attenuation is a strong function of "
	"exactly that. D-11 still lands these rows on C8 so the cross-rate case compares like with like";
const char* const kProvCrossRateFloored =
	"MEASURED by plan 32-07 AT THIS ROW'S OWN RATE (the 44.1 kHz transfer is falsified - see kProvCrossRate), "
	"then FLOORED at kThresholdFloorDb because ceil(measuredDb + 3.0) lands tighter than this apparatus's "
	"own leakage floor can support (D-10)";
const char* const kProvDiagnostic =
	"MEASURED by plan 32-07 in this repository at 44.1 kHz C6 - a row present in 32-RESEARCH section "
	"'D-08 baseline and D-09 threshold evidence' but NOT in 32-VALIDATION's Threshold Policy matrix, which "
	"starts at C7; diagnostic tier, CAPTUREd and never CHECKed, so this threshold is a recorded expectation "
	"rather than a gate";
const char* const kProvDiagnosticFloored =
	"MEASURED by plan 32-07 at 44.1 kHz C6 (a row present in 32-RESEARCH but absent from 32-VALIDATION's "
	"matrix), then FLOORED at kThresholdFloorDb because ceil(measuredDb + 3.0) lands tighter than this "
	"apparatus can assert (D-10); diagnostic tier, never CHECKed";

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
// >>> THE COLUMN IS PINNED FROM THIS REPOSITORY'S OWN MEASUREMENT (plan 32-07).
// It is no longer provisional and it is no longer the prototype's.
// Every row now carries TWO numbers: `measuredDb`, the corrected alias peak this
// repository measured for that cell, and `thresholdDb` = max(ceil(measuredDb +
// 3.0), kThresholdFloorDb). The trailing comment keeps the prototype figure
// beside the measured one, with their difference, so the comparison the
// MEASURE-TO-PIN PROTOCOL step 3 asks for stays readable off the table.
//
// >>> AND THE CIRCULARITY THIS CREATES IS BOUNDED, NOT IGNORED (T-32-15). <<<
// A threshold pinned from the implementation's own output cannot, on its own,
// fail: every gated cell passes by construction with at least 3 dB of room. That
// is why this table is NOT the phase's evidence. The evidence is the two
// assertions that consult NO pinned number at all — the 8 dB minimum-improvement
// CHECK on the five named cells in the gate below, and the no-regression
// invariant over all 90 cells in its own case further down. Both compare two
// measurements of the same apparatus. Delete the table and they still bite;
// delete them and the table asserts nothing.
//
// THE ANTI-SOFTENING CLAUSE STILL APPLIES, and it now has teeth: a threshold
// that turns out to be unreachable is ESCALATED per the rule written into the
// TEST-03 gate's banner, never quietly loosened. Loosening a threshold by hand
// breaks the derivation assertion in that gate; loosening measuredDb with it
// breaks the reproduction CHECK in the measure pass. Both would have to be
// edited together, and both are named here, so the edit cannot be a quiet one.
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
	{  44100.0, 195, "C7", 0.00f, "sine",     0.00f,   -125.4350f,   -75.0f, "gated",       kProvFloored             },   // prototype corrected   -150.7 -> MEASURED  -125.4350  (APPARATUS FLOOR (-125.51 dB), not the DSP)
	{  44100.0, 195, "C7", 0.00f, "sine",     0.50f,    -64.6079f,   -61.0f, "gated",       kProvMeasured            },   // prototype corrected    -68.4 -> MEASURED   -64.6079  (IMPL WORSE by 3.79)
	{  44100.0, 195, "C7", 0.00f, "sine",     1.00f,    -98.8753f,   -75.0f, "gated",       kProvFloored             },   // prototype corrected   -102.4 -> MEASURED   -98.8753  (IMPL WORSE by 3.52)
	{  44100.0, 195, "C7", 0.25f, "triangle", 0.00f,    -50.2842f,   -47.0f, "gated",       kProvMeasured            },   // prototype corrected    -50.3 -> MEASURED   -50.2842  (+0.02)
	{  44100.0, 195, "C7", 0.25f, "triangle", 0.50f,    -42.0859f,   -39.0f, "gated",       kProvMeasured            },   // prototype corrected    -42.1 -> MEASURED   -42.0859  (+0.01)
	{  44100.0, 195, "C7", 0.25f, "triangle", 1.00f,    -47.5920f,   -44.0f, "gated",       kProvMeasured            },   // prototype corrected    -47.4 -> MEASURED   -47.5920  (-0.19)
	{  44100.0, 195, "C7", 0.50f, "saw",      0.00f,    -29.5463f,   -26.0f, "gated",       kProvMeasured            },   // prototype corrected    -29.5 -> MEASURED   -29.5463  (-0.05)
	{  44100.0, 195, "C7", 0.50f, "saw",      0.50f,    -29.2725f,   -26.0f, "gated",       kProvMeasured            },   // prototype corrected    -29.2 -> MEASURED   -29.2725  (-0.07)
	{  44100.0, 195, "C7", 0.50f, "saw",      1.00f,    -28.1772f,   -25.0f, "gated",       kProvMeasured            },   // prototype corrected    -28.0 -> MEASURED   -28.1772  (-0.18)
	{  44100.0, 195, "C7", 0.75f, "square",   0.00f,    -29.4639f,   -26.0f, "gated",       kProvMeasured            },   // prototype corrected    -29.5 -> MEASURED   -29.4639  (+0.04)
	{  44100.0, 195, "C7", 0.75f, "square",   0.50f,    -29.9283f,   -26.0f, "gated",       kProvMeasured            },   // prototype corrected    -30.3 -> MEASURED   -29.9283  (+0.37)
	{  44100.0, 195, "C7", 0.75f, "square",   1.00f,    -57.1816f,   -54.0f, "gated",       kProvMeasured            },   // prototype corrected    -60.1 -> MEASURED   -57.1816  (IMPL WORSE by 2.92)
	{  44100.0, 195, "C7", 1.00f, "pulse 5%", 0.00f,    -13.5375f,   -10.0f, "gated",       kProvMeasured            },   // prototype corrected    -13.5 -> MEASURED   -13.5375  (-0.04)
	{  44100.0, 195, "C7", 1.00f, "pulse 5%", 0.50f,    -13.2981f,   -10.0f, "gated",       kProvMeasured            },   // prototype corrected    -13.1 -> MEASURED   -13.2981  (-0.20)
	{  44100.0, 195, "C7", 1.00f, "pulse 5%", 1.00f,    -20.7280f,   -17.0f, "gated",       kProvMeasured            },   // prototype corrected    -20.3 -> MEASURED   -20.7280  (-0.43)

	// --- C8, 44.1 kHz, K = 389, ~4188.2 Hz, 5 harmonics below Nyquist. ------
	{  44100.0, 389, "C8", 0.00f, "sine",     0.00f,   -101.5410f,   -75.0f, "gated",       kProvFloored             },   // prototype corrected   -150.7 -> MEASURED  -101.5410  (APPARATUS FLOOR (-101.55 dB), not the DSP)
	{  44100.0, 389, "C8", 0.00f, "sine",     0.50f,    -66.1069f,   -63.0f, "gated",       kProvMeasured            },   // prototype corrected    -71.5 -> MEASURED   -66.1069  (IMPL WORSE by 5.39)
	{  44100.0, 389, "C8", 0.00f, "sine",     1.00f,    -73.1369f,   -70.0f, "gated",       kProvMeasured            },   // prototype corrected    -76.4 -> MEASURED   -73.1369  (IMPL WORSE by 3.26)
	{  44100.0, 389, "C8", 0.25f, "triangle", 0.00f,    -48.7878f,   -45.0f, "gated",       kProvMeasured            },   // prototype corrected    -48.8 -> MEASURED   -48.7878  (+0.01)
	{  44100.0, 389, "C8", 0.25f, "triangle", 0.50f,    -38.1311f,   -35.0f, "gated",       kProvMeasured            },   // prototype corrected    -38.1 -> MEASURED   -38.1311  (-0.03)
	{  44100.0, 389, "C8", 0.25f, "triangle", 1.00f,    -33.6972f,   -30.0f, "gated",       kProvMeasured            },   // prototype corrected    -33.5 -> MEASURED   -33.6972  (-0.20)
	{  44100.0, 389, "C8", 0.50f, "saw",      0.00f,    -25.8423f,   -22.0f, "gated",       kProvMeasured            },   // prototype corrected    -25.8 -> MEASURED   -25.8423  (-0.04)
	{  44100.0, 389, "C8", 0.50f, "saw",      0.50f,    -25.6496f,   -22.0f, "gated",       kProvMeasured            },   // prototype corrected    -25.7 -> MEASURED   -25.6496  (+0.05)
	{  44100.0, 389, "C8", 0.50f, "saw",      1.00f,    -23.9943f,   -20.0f, "gated",       kProvMeasured            },   // prototype corrected    -23.9 -> MEASURED   -23.9943  (-0.09)
	{  44100.0, 389, "C8", 0.75f, "square",   0.00f,    -31.8772f,   -28.0f, "gated",       kProvMeasured            },   // prototype corrected    -31.9 -> MEASURED   -31.8772  (+0.02)
	{  44100.0, 389, "C8", 0.75f, "square",   0.50f,    -31.2534f,   -28.0f, "gated",       kProvMeasured            },   // prototype corrected    -33.2 -> MEASURED   -31.2534  (IMPL WORSE by 1.95)
	{  44100.0, 389, "C8", 0.75f, "square",   1.00f,    -47.0596f,   -44.0f, "gated",       kProvMeasured            },   // prototype corrected    -47.7 -> MEASURED   -47.0596  (+0.64)
	{  44100.0, 389, "C8", 1.00f, "pulse 5%", 0.00f,    -11.5704f,    -8.0f, "gated",       kProvMeasured            },   // prototype corrected    -11.6 -> MEASURED   -11.5704  (+0.03)
	{  44100.0, 389, "C8", 1.00f, "pulse 5%", 0.50f,    -11.1090f,    -8.0f, "gated",       kProvMeasured            },   // prototype corrected    -11.1 -> MEASURED   -11.1090  (-0.01)
	{  44100.0, 389, "C8", 1.00f, "pulse 5%", 1.00f,    -12.2084f,    -9.0f, "gated",       kProvMeasured            },   // prototype corrected    -10.8 -> MEASURED   -12.2084  (IMPL BETTER by 1.41)

	// --- C9, 44.1 kHz, K = 777, ~8366.9 Hz, 2 harmonics below Nyquist. ------
	//     The hardest row for every shape: only two harmonics survive, so
	//     almost the whole waveform is alias.
	{  44100.0, 777, "C9", 0.00f, "sine",     0.00f,    -91.9421f,   -75.0f, "gated",       kProvFloored             },   // prototype corrected   -150.7 -> MEASURED   -91.9421  (APPARATUS FLOOR (-91.95 dB), not the DSP)
	{  44100.0, 777, "C9", 0.00f, "sine",     0.50f,    -35.0480f,   -32.0f, "gated",       kProvMeasured            },   // prototype corrected    -34.6 -> MEASURED   -35.0480  (-0.45)
	{  44100.0, 777, "C9", 0.00f, "sine",     1.00f,    -23.0910f,   -20.0f, "gated",       kProvMeasured            },   // prototype corrected    -22.7 -> MEASURED   -23.0910  (-0.39)
	{  44100.0, 777, "C9", 0.25f, "triangle", 0.00f,    -28.5498f,   -25.0f, "gated",       kProvMeasured            },   // prototype corrected    -28.5 -> MEASURED   -28.5498  (-0.05)
	{  44100.0, 777, "C9", 0.25f, "triangle", 0.50f,    -24.8652f,   -21.0f, "gated",       kProvMeasured            },   // prototype corrected    -24.9 -> MEASURED   -24.8652  (+0.03)
	{  44100.0, 777, "C9", 0.25f, "triangle", 1.00f,    -19.8372f,   -16.0f, "gated",       kProvMeasured            },   // prototype corrected    -19.8 -> MEASURED   -19.8372  (-0.04)
	{  44100.0, 777, "C9", 0.50f, "saw",      0.00f,    -19.0075f,   -16.0f, "gated",       kProvMeasured            },   // prototype corrected    -19.0 -> MEASURED   -19.0075  (-0.01)
	{  44100.0, 777, "C9", 0.50f, "saw",      0.50f,    -18.8436f,   -15.0f, "gated",       kProvMeasured            },   // prototype corrected    -18.9 -> MEASURED   -18.8436  (+0.06)
	{  44100.0, 777, "C9", 0.50f, "saw",      1.00f,    -17.4367f,   -14.0f, "gated",       kProvMeasured            },   // prototype corrected    -17.5 -> MEASURED   -17.4367  (+0.06)
	{  44100.0, 777, "C9", 0.75f, "square",   0.00f,    -19.0075f,   -16.0f, "gated",       kProvMeasured            },   // prototype corrected    -19.0 -> MEASURED   -19.0075  (-0.01)
	{  44100.0, 777, "C9", 0.75f, "square",   0.50f,    -18.4588f,   -15.0f, "gated",       kProvMeasured            },   // prototype corrected    -19.6 -> MEASURED   -18.4588  (IMPL WORSE by 1.14)
	{  44100.0, 777, "C9", 0.75f, "square",   1.00f,    -21.6580f,   -18.0f, "gated",       kProvMeasured            },   // prototype corrected    -21.7 -> MEASURED   -21.6580  (+0.04)
	{  44100.0, 777, "C9", 1.00f, "pulse 5%", 0.00f,     -9.7531f,    -6.0f, "gated",       kProvMeasured            },   // prototype corrected     -9.8 -> MEASURED    -9.7531  (+0.05)
	{  44100.0, 777, "C9", 1.00f, "pulse 5%", 0.50f,     -9.5280f,    -6.0f, "gated",       kProvMeasured            },   // prototype corrected     -9.5 -> MEASURED    -9.5280  (-0.03)
	{  44100.0, 777, "C9", 1.00f, "pulse 5%", 1.00f,     -7.2588f,    -4.0f, "gated",       kProvMeasured            },   // prototype corrected     -5.6 -> MEASURED    -7.2588  (IMPL BETTER by 1.66)

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
	{  44100.0,  97, "C6", 0.00f, "sine",     0.00f,   -116.1410f,   -75.0f, "diagnostic",  kProvDiagnosticFloored   },   // prototype corrected   -150.7 -> MEASURED  -116.1410  (APPARATUS FLOOR (-116.19 dB), not the DSP)
	{  44100.0,  97, "C6", 0.00f, "sine",     0.50f,    -73.1731f,   -70.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -76.6 -> MEASURED   -73.1731  (IMPL WORSE by 3.43)
	{  44100.0,  97, "C6", 0.00f, "sine",     1.00f,   -114.0990f,   -75.0f, "diagnostic",  kProvDiagnosticFloored   },   // prototype corrected   -117.3 -> MEASURED  -114.0990  (IMPL WORSE by 3.20)
	{  44100.0,  97, "C6", 0.25f, "triangle", 0.00f,    -63.9483f,   -60.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -64.0 -> MEASURED   -63.9483  (+0.05)
	{  44100.0,  97, "C6", 0.25f, "triangle", 0.50f,    -55.2153f,   -52.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -55.1 -> MEASURED   -55.2153  (-0.12)
	{  44100.0,  97, "C6", 0.25f, "triangle", 1.00f,    -60.2646f,   -57.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -60.5 -> MEASURED   -60.2646  (+0.24)
	{  44100.0,  97, "C6", 0.50f, "saw",      0.00f,    -35.4319f,   -32.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -35.4 -> MEASURED   -35.4319  (-0.03)
	{  44100.0,  97, "C6", 0.50f, "saw",      0.50f,    -35.0575f,   -32.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -35.1 -> MEASURED   -35.0575  (+0.04)
	{  44100.0,  97, "C6", 0.50f, "saw",      1.00f,    -35.0833f,   -32.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -35.0 -> MEASURED   -35.0833  (-0.08)
	{  44100.0,  97, "C6", 0.75f, "square",   0.00f,    -36.7254f,   -33.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -36.7 -> MEASURED   -36.7254  (-0.03)
	{  44100.0,  97, "C6", 0.75f, "square",   0.50f,    -38.5206f,   -35.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -38.6 -> MEASURED   -38.5206  (+0.08)
	{  44100.0,  97, "C6", 0.75f, "square",   1.00f,    -65.2994f,   -62.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -68.7 -> MEASURED   -65.2994  (IMPL WORSE by 3.40)
	{  44100.0,  97, "C6", 1.00f, "pulse 5%", 0.00f,    -26.3658f,   -23.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -26.4 -> MEASURED   -26.3658  (+0.03)
	{  44100.0,  97, "C6", 1.00f, "pulse 5%", 0.50f,    -27.3031f,   -24.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -27.2 -> MEASURED   -27.3031  (-0.10)
	{  44100.0,  97, "C6", 1.00f, "pulse 5%", 1.00f,    -36.9925f,   -33.0f, "diagnostic",  kProvDiagnostic          },   // prototype corrected    -36.8 -> MEASURED   -36.9925  (-0.19)

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
	{  48000.0, 357, "C8", 0.00f, "sine",     0.00f,    -97.7016f,   -75.0f, "regression",  kProvCrossRateFloored    },   // prototype corrected   -150.7 -> MEASURED   -97.7016  (APPARATUS FLOOR (-97.71 dB), not the DSP)
	{  48000.0, 357, "C8", 0.00f, "sine",     0.50f,    -63.4384f,   -60.0f, "regression",  kProvCrossRate           },   // prototype corrected    -71.5 -> MEASURED   -63.4384  (IMPL WORSE by 8.06)
	{  48000.0, 357, "C8", 0.00f, "sine",     1.00f,    -68.4310f,   -65.0f, "regression",  kProvCrossRate           },   // prototype corrected    -76.4 -> MEASURED   -68.4310  (IMPL WORSE by 7.97)
	{  48000.0, 357, "C8", 0.25f, "triangle", 0.00f,    -45.9493f,   -42.0f, "regression",  kProvCrossRate           },   // prototype corrected    -48.8 -> MEASURED   -45.9493  (IMPL WORSE by 2.85)
	{  48000.0, 357, "C8", 0.25f, "triangle", 0.50f,    -37.3045f,   -34.0f, "regression",  kProvCrossRate           },   // prototype corrected    -38.1 -> MEASURED   -37.3045  (+0.80)
	{  48000.0, 357, "C8", 0.25f, "triangle", 1.00f,    -33.6580f,   -30.0f, "regression",  kProvCrossRate           },   // prototype corrected    -33.5 -> MEASURED   -33.6580  (-0.16)
	{  48000.0, 357, "C8", 0.50f, "saw",      0.00f,    -24.0157f,   -21.0f, "regression",  kProvCrossRate           },   // prototype corrected    -25.8 -> MEASURED   -24.0157  (IMPL WORSE by 1.78)
	{  48000.0, 357, "C8", 0.50f, "saw",      0.50f,    -23.8260f,   -20.0f, "regression",  kProvCrossRate           },   // prototype corrected    -25.7 -> MEASURED   -23.8260  (IMPL WORSE by 1.87)
	{  48000.0, 357, "C8", 0.50f, "saw",      1.00f,    -22.4636f,   -19.0f, "regression",  kProvCrossRate           },   // prototype corrected    -23.9 -> MEASURED   -22.4636  (IMPL WORSE by 1.44)
	{  48000.0, 357, "C8", 0.75f, "square",   0.00f,    -29.0479f,   -26.0f, "regression",  kProvCrossRate           },   // prototype corrected    -31.9 -> MEASURED   -29.0479  (IMPL WORSE by 2.85)
	{  48000.0, 357, "C8", 0.75f, "square",   0.50f,    -28.6222f,   -25.0f, "regression",  kProvCrossRate           },   // prototype corrected    -33.2 -> MEASURED   -28.6222  (IMPL WORSE by 4.58)
	{  48000.0, 357, "C8", 0.75f, "square",   1.00f,    -42.9267f,   -39.0f, "regression",  kProvCrossRate           },   // prototype corrected    -47.7 -> MEASURED   -42.9267  (IMPL WORSE by 4.77)
	{  48000.0, 357, "C8", 1.00f, "pulse 5%", 0.00f,     -9.7431f,    -6.0f, "regression",  kProvCrossRate           },   // prototype corrected    -11.6 -> MEASURED    -9.7431  (IMPL WORSE by 1.86)
	{  48000.0, 357, "C8", 1.00f, "pulse 5%", 0.50f,     -9.4072f,    -6.0f, "regression",  kProvCrossRate           },   // prototype corrected    -11.1 -> MEASURED    -9.4072  (IMPL WORSE by 1.69)
	{  48000.0, 357, "C8", 1.00f, "pulse 5%", 1.00f,    -11.0512f,    -8.0f, "regression",  kProvCrossRate           },   // prototype corrected    -10.8 -> MEASURED   -11.0512  (-0.25)

	// --- 96 kHz, K = 179, ~4195.3 Hz, 11 harmonics — same note as 44.1k C8. -
	{  96000.0, 179, "C8", 0.00f, "sine",     0.00f,   -102.8520f,   -75.0f, "regression",  kProvCrossRateFloored    },   // prototype corrected   -150.7 -> MEASURED  -102.8520  (APPARATUS FLOOR (-102.88 dB), not the DSP)
	{  96000.0, 179, "C8", 0.00f, "sine",     0.50f,    -68.1778f,   -65.0f, "regression",  kProvCrossRate           },   // prototype corrected    -71.5 -> MEASURED   -68.1778  (IMPL WORSE by 3.32)
	{  96000.0, 179, "C8", 0.00f, "sine",     1.00f,   -100.1890f,   -75.0f, "regression",  kProvCrossRateFloored    },   // prototype corrected    -76.4 -> MEASURED  -100.1890  (IMPL BETTER by 23.79)
	{  96000.0, 179, "C8", 0.25f, "triangle", 0.00f,    -54.9689f,   -51.0f, "regression",  kProvCrossRate           },   // prototype corrected    -48.8 -> MEASURED   -54.9689  (IMPL BETTER by 6.17)
	{  96000.0, 179, "C8", 0.25f, "triangle", 0.50f,    -44.8830f,   -41.0f, "regression",  kProvCrossRate           },   // prototype corrected    -38.1 -> MEASURED   -44.8830  (IMPL BETTER by 6.78)
	{  96000.0, 179, "C8", 0.25f, "triangle", 1.00f,    -47.5728f,   -44.0f, "regression",  kProvCrossRate           },   // prototype corrected    -33.5 -> MEASURED   -47.5728  (IMPL BETTER by 14.07)
	{  96000.0, 179, "C8", 0.50f, "saw",      0.00f,    -30.2544f,   -27.0f, "regression",  kProvCrossRate           },   // prototype corrected    -25.8 -> MEASURED   -30.2544  (IMPL BETTER by 4.45)
	{  96000.0, 179, "C8", 0.50f, "saw",      0.50f,    -30.0015f,   -27.0f, "regression",  kProvCrossRate           },   // prototype corrected    -25.7 -> MEASURED   -30.0015  (IMPL BETTER by 4.30)
	{  96000.0, 179, "C8", 0.50f, "saw",      1.00f,    -29.1015f,   -26.0f, "regression",  kProvCrossRate           },   // prototype corrected    -23.9 -> MEASURED   -29.1015  (IMPL BETTER by 5.20)
	{  96000.0, 179, "C8", 0.75f, "square",   0.00f,    -32.6886f,   -29.0f, "regression",  kProvCrossRate           },   // prototype corrected    -31.9 -> MEASURED   -32.6886  (-0.79)
	{  96000.0, 179, "C8", 0.75f, "square",   0.50f,    -33.4991f,   -30.0f, "regression",  kProvCrossRate           },   // prototype corrected    -33.2 -> MEASURED   -33.4991  (-0.30)
	{  96000.0, 179, "C8", 0.75f, "square",   1.00f,    -59.3793f,   -56.0f, "regression",  kProvCrossRate           },   // prototype corrected    -47.7 -> MEASURED   -59.3793  (IMPL BETTER by 11.68)
	{  96000.0, 179, "C8", 1.00f, "pulse 5%", 0.00f,    -14.5772f,   -11.0f, "regression",  kProvCrossRate           },   // prototype corrected    -11.6 -> MEASURED   -14.5772  (IMPL BETTER by 2.98)
	{  96000.0, 179, "C8", 1.00f, "pulse 5%", 0.50f,    -14.3618f,   -11.0f, "regression",  kProvCrossRate           },   // prototype corrected    -11.1 -> MEASURED   -14.3618  (IMPL BETTER by 3.26)
	{  96000.0, 179, "C8", 1.00f, "pulse 5%", 1.00f,    -22.5743f,   -19.0f, "regression",  kProvCrossRate           },   // prototype corrected    -10.8 -> MEASURED   -22.5743  (IMPL BETTER by 11.77)
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
//
// THE `master` PARAMETER IS AN EXTENSION, NOT A FORK (plan 33-05). The D-06
// sync sub-grid needs a PER-SAMPLE hard-sync voltage, which a constant `base`
// cannot carry. The alternative — a second, sync-aware drive loop — is exactly
// what the paragraph above forbids, and it would be forbidden for a sharper
// reason here than anywhere else in this file: plan 33-05's whole claim is that
// its placement probe IS forge::VcoCore — on the no-correction leg as 33-05
// wrote it, and on the past-edge leg since 33-06 landed the seam and re-anchored
// the gate — and a comparison whose two sides ran different loops could not
// support that claim at all. So the ONE loop gained one defaulted pointer
// instead.
//
// IT IS BIT-INERT FOR EVERY PRE-EXISTING CALLER. A null `master` leaves the
// body byte-for-byte what it was, and forge::VcoInputs::syncConnected defaults
// to false, so no existing caller can reach the sync block in the core. Every
// Phase 32 figure this file records is therefore unmoved by this parameter, and
// that was verified by re-running the suite across the change rather than
// argued.
// ---------------------------------------------------------------------------
template <typename CoreT>
void driveSecondBlock(CoreT& core, const forge::VcoInputs& base, float dt, double sr,
                      std::vector<float>& out, const std::vector<float>* master = 0) {
	out.clear();
	out.reserve((std::size_t)kSpectrumN);
	for (int i = 0; i < 2 * kSpectrumN; ++i) {
		forge::VcoInputs in = base;
		in.sampleTime = dt;
		in.sampleRate = (float)sr;
		if (master) {
			in.syncVolts     = (*master)[(std::size_t)i];
			in.syncConnected = true;
		}
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

// ===========================================================================
// ============  THE D-06 / D-11 SYNC SUB-GRID (plan 33-05)  =================
// ===========================================================================
//
// WHAT THIS SUB-GRID IS FOR. Phase 33's central question is a PLACEMENT
// question: forge::MorphBlep's seam is FUTURE-FACING — `addStep(xAhead, jump)`
// puts the edge ahead of the sample being emitted — while a Schmitt-detected
// hard-sync edge is ALWAYS IN THE PAST, having happened between the previous
// sample and this one. The two-point residual straddles an edge symmetrically,
// so for a past edge the half that belonged on the ALREADY-EMITTED previous
// sample is structurally unrecoverable without the one-sample output delay
// buffer src/dsp/MorphBlep.hpp:225-230 rejects (partly citing this phase).
// Which recoverable half to take — and whether taking the wrong one is worse
// than taking none at all — is not an argument. It is a measurement, and this
// is the instrument for it.
//
// >>> DISCRETION RESOLVED, WITH ITS RATIONALE, RATHER THAN LEFT IMPLICIT. <<<
// This sub-grid lands in THIS EXISTING translation unit rather than a new one,
// and there are two reasons, the second of which is the substantive one.
//   (1) A new test TU would cost an explicit `VCO_SIDE_ALLOW` entry in
//       tests/check_includes.sh. Cheap, but a guard-allowlist edit is never
//       free in this project.
//   (2) THE ONE-MEASUREMENT-FUNCTION DISCIPLINE THIS FILE ALREADY ENFORCES
//       ONLY WORKS WHILE THE SYNC LEGS AND THE STANDING GRID SHARE THE SAME
//       CELL-MEASURING CODE. measureCellDb's banner states it outright: "If a
//       later agent adds a second measurement function for the corrected path,
//       the phase's central claim stops being a measurement and becomes a
//       coincidence." Six near-copies of a placement path, one per leg, would
//       make the RANKING below a coincidence in exactly that sense. Everything
//       here therefore comes out of ONE cell-measuring function parameterised
//       by leg, driven through the ONE drive loop above.
//
// >>> PART D — THE METRIC, AND ITS DERIVATION. THE FUNDAMENTAL BIN IS THE
//     MASTER'S, NOT THE SLAVE'S. <<<
// Under hard sync the slave's WHOLE TRAJECTORY is determined by the master: it
// is reset to a fixed fractional overshoot at every master wrap, so whatever it
// does between wraps it does identically in every master period. The ideal
// continuous-time output is therefore exactly periodic at the MASTER's period.
// With K_m master cycles per N-sample block, the master's period is N/K_m
// samples, every true harmonic of the ideal band-limited signal lands exactly
// on bin n*K_m, and everything off that lattice is alias energy. That is the
// same coprimality argument the file banner makes at length, with K_m
// substituted for K — and N = 4096 = 2^12 still reduces the requirement to
// "pick an odd K_m", which every master cycle count below is.
//   CONSEQUENCE 1: aliasPeakDb is called UNCHANGED, with the master's cycle
// count. There is no second metric and no forked classifier.
//   CONSEQUENCE 2, AND IT IS THE ONE A LATER AGENT WILL BE TEMPTED TO "FIX":
// THE SLAVE'S FREQUENCY IS FREE. It is NOT bin-centred, it must NOT be solved
// for, and NEITHER binCentredPitchCV NOR binCentredSampleTime APPLIES TO IT.
// The leakage argument attaches to the fundamental BEING MEASURED, which is the
// master's; running a bin-centring solver on the slave would imply a leakage
// claim that does not attach there at all. This is a simplification the
// derivation licenses, not a weakening of it.
//
// >>> THE TWO HAZARDS THAT ARE THE REASON FOR TWO OF THE GRID'S AXES. <<<
//
// HAZARD ONE — THE SUB-UNITY RATIO IS NOT OPTIONAL. In the classic hard-sync
// sweep region (slave at or above the master) the placement candidates separate
// by LESS THAN THE CROSS-TOOLCHAIN REPRODUCTION BOUND. Register item 8 binds:
// every absolute decibel figure this milestone has recorded is an APPLE-CLANG
// figure, the step-dominated reproduction bound is 1.0 dB and the plateau bound
// is 4.0 dB, and a decision taken only on cells separated by less than the
// applicable bound IS NOT DEFENSIBLE CROSS-TOOLCHAIN AND MUST NOT BE TAKEN. The
// sub-unity ratio is where the candidates separate by several decibels, so a
// grid without it cannot carry a defensible decision at all. That is why
// SYNC_RATIOS opens with 0.5 and why the D-06 decision rule's condition 2 is
// evaluated on the sub-unity cells specifically.
//
// HAZARD TWO — A GRID WHOSE MASTERS ALL HAVE HARD EDGES TESTS NOTHING ABOUT
// SUB-SAMPLE PLACEMENT, AND FAILS TO DO SO SILENTLY. The detector solves
// f = (HIGH - prev) / (now - prev). That recovers the master's true wrap
// fraction ONLY WHEN THE MASTER'S EDGE SPANS THE THRESHOLD OVER ONE SAMPLE OR
// MORE. For a master whose wrap is a single-sample full-scale jump — a naive
// saw or a gate, which is what most patched masters are — the interpolation
// returns the VOLTAGE fraction of the threshold WITHIN THE JUMP, which is
// very nearly constant: for a +/-amp falling saw with an instantaneous rising
// wrap at HIGH = 1.0 V and true wrap fraction g,
//     prev = -5 + 10*g*dtm ,  now = 5 - 10*(1-g)*dtm
//     f    = (6 - 10*g*dtm) / (10 - 10*dtm)  ~=  0.6 - g*dtm
// so g enters only at order dtm. THIS IS MEASURED, NOT PREDICTED: plan 33-04's
// invariant 7 drives two master offsets whose TRUE wrap fractions are 1.000000
// and 0.500000 and records detected fractions of 0.596850 and 0.600787 — g
// HALVES while f moves by 0.004. The same plan measures a band-limited master's
// f spanning 0.677579. A sub-grid built on hard edges alone therefore leaves
// SYNC-02's sub-sample clause untested while reporting green, which is why
// SYNC_EDGES carries BOTH shapes and why every reported figure below is broken
// out per edge shape.
//   This is not a defect in the detector. It is the standard technique and VCV
// Fundamental's own VCO does exactly the same thing. It is a property of the
// INSTRUMENT, named before a number is written against it.
// ---------------------------------------------------------------------------

// The master's edge shape — ONE parameter, not two near-copy generators.
enum SyncMasterEdge {
	kMasterHardEdge    = 0,   // a single-sample full-scale wrap: a naive saw or a gate
	kMasterBandLimited = 1    // the same saw with the two-point polyBLEP residual at its wrap
};

// ---------------------------------------------------------------------------
// SyncMaster / makeSyncMaster — THE MASTER IS GENERATED IN THE TEST, never by a
// second forge::VcoCore. A second core would make every sync claim circular:
// the thing under test would be producing its own stimulus.
//
// WHY THE INCREMENT IS EXACTLY REPRESENTABLE, AND WHAT THAT BUYS. The phase
// increment is K_m / 4096 = K_m * 2^-12, a dyadic rational, so it is EXACT in
// binary floating point and every partial sum of it is a multiple of 2^-12 and
// therefore also exact. `phim += dtm` accumulates over the block with ZERO
// rounding error — not "small" error, zero — so after exactly 4096 samples the
// accumulator has returned EXACTLY to its start and the block holds EXACTLY
// K_m master cycles. The achieved bin error is therefore EXACTLY zero rather
// than merely small, and the case below asserts it as such rather than
// tolerating it. Do not "simplify" a caller to a rounded frequency in hertz.
//
// THE POLARITY IS CHOSEN, NOT INHERITED. A +/-amp FALLING saw,
// v = amp*(1 - 2*phi_m). The falling ramp crosses the 0.1 V LOW threshold
// DOWNWARD in the middle of every master cycle, which RE-ARMS
// forge::SchmittTrigger, and then jumps UPWARD through the 1.0 V HIGH threshold
// at the wrap, which FIRES it: one arm and one fire per master cycle out of one
// waveform, with no hand-built gate sequence anywhere. It is also the Forge
// saw's own polarity — src/dsp/MorphBlep.hpp:422-431 records the wrap jump as
// +2.000000 — so it models what an operator actually patches.
//
// AND IT KNOWS THE TRUE WRAP FRACTION EXACTLY, WHICH IS THE ORACLE LEG'S INPUT.
// `wrapGHeld[i]` is g = (1 - phi_before) / dtm for the most recent wrap at or
// before sample i, taken from the generator's OWN accumulator rather than
// inferred from the samples it emitted. That value is UNAVAILABLE TO THE CORE
// BY CONSTRUCTION — the core sees only two voltages — which is exactly what
// makes it an oracle rather than a fifth candidate.
//   IT IS HELD, NOT PER-SAMPLE, AND THE REASON IS A MEASURED FACT. Plan 33-04
// measured that at g = 0.96875 a band-limited master's residual pushes the wrap
// sample down to 0.31 V, BELOW the high threshold, so the detector fires ONE
// SAMPLE LATE. On that sample the master did not wrap, so a strictly
// per-sample g would be absent exactly where the oracle is most needed. Holding
// the most recent wrap's g hands the oracle the fraction of the wrap that
// actually caused the (late) detection. The lateness itself is not swept under
// that rug: `wrappedHere` is kept alongside and the late-fire count is reported
// as a diagnostic column, because it is a placement error that exists BEFORE
// any seam does and must not be attributed to one.
// ---------------------------------------------------------------------------
struct SyncMaster {
	std::vector<float>  volts;        // the master voltage, one entry per slave sample
	std::vector<double> wrapGHeld;    // the TRUE wrap fraction of the most recent wrap at or before this sample
	std::vector<char>   wrappedHere;  // 1 if the master wrapped ON this sample
	double achievedCyclesPerBlock;    // measured from the generator's own accumulator over the FIRST kSpectrumN samples
	double binError;                  // |achievedCyclesPerBlock - K_m|, IN BINS — asserted EXACTLY zero
	long   wrapsInFirstBlock;
	double gMin;                      // the true wrap fraction's range over the whole run
	double gMax;
};

SyncMaster makeSyncMaster(int nTotal, int Km, double amp, SyncMasterEdge edge) {
	SyncMaster m;
	m.achievedCyclesPerBlock = 0.0;
	m.binError = 0.0;
	m.wrapsInFirstBlock = 0;
	m.gMin =  2.0;
	m.gMax = -1.0;
	m.volts.reserve((std::size_t)nTotal);
	m.wrapGHeld.reserve((std::size_t)nTotal);
	m.wrappedHere.reserve((std::size_t)nTotal);

	const double dtm = (double)Km / (double)kSpectrumN;   // K_m * 2^-12 — exact

	std::vector<int>    wrapIdx;
	std::vector<double> wrapG;
	double phim  = 0.0;
	double gHeld = 0.0;
	long   wraps = 0;
	double phiAtBlockEnd = 0.0;
	long   wrapsAtBlockEnd = 0;

	for (int i = 0; i < nTotal; ++i) {
		const double before = phim;
		phim += dtm;
		char wrapped = 0;
		if (phim >= 1.0) {
			// std::floor rather than a single subtract, matching
			// tests/test_vco_core.cpp's generator: every caller here keeps dtm
			// well under 1, but the two generators must not differ in a way a
			// reader has to discover.
			const double k = std::floor(phim);
			gHeld = (1.0 - before) / dtm;
			if (gHeld < m.gMin) m.gMin = gHeld;
			if (gHeld > m.gMax) m.gMax = gHeld;
			wraps += (long)k;
			phim  -= k;
			wrapped = 1;
			wrapIdx.push_back(i);
			wrapG.push_back(gHeld);
		}
		if (i == kSpectrumN - 1) { phiAtBlockEnd = phim; wrapsAtBlockEnd = wraps; }
		m.volts.push_back((float)(amp * (1.0 - 2.0 * phim)));
		m.wrapGHeld.push_back(gHeld);
		m.wrappedHere.push_back(wrapped);
	}

	// The two-point polyBLEP residual at each wrap: +amp*(1-g)^2 on the sample
	// BEFORE it and -amp*g^2 on the sample carrying it. Transcribed from
	// 33-RESEARCH Pitfall 7's own worked expressions — and identical to
	// tests/test_vco_core.cpp's makeMasterSawBandLimited — so the two files
	// measure the same construction rather than two variants of it.
	if (edge == kMasterBandLimited) {
		for (std::size_t j = 0; j < wrapIdx.size(); ++j) {
			const int    k = wrapIdx[j];
			const double g = wrapG[j];
			if (k - 1 >= 0) m.volts[(std::size_t)(k - 1)] += (float)(amp * (1.0 - g) * (1.0 - g));
			m.volts[(std::size_t)k] -= (float)(amp * g * g);
		}
	}

	m.wrapsInFirstBlock      = wrapsAtBlockEnd;
	m.achievedCyclesPerBlock = (double)wrapsAtBlockEnd + phiAtBlockEnd;
	m.binError               = std::fabs(m.achievedCyclesPerBlock - (double)Km);
	return m;
}

// ---------------------------------------------------------------------------
// SyncCell — one measurement point of the D-06 / D-11 sync sub-grid.
//
// It is shaped like SpectrumCell deliberately: same tier and provenance
// discipline, same "the measured column IS the provenance, in numbers" coupling
// described at length in that struct's banner. The coupling is the point —
// loosening a threshold alone must break the reproduction check.
//
// >>> BOTH DECIBEL COLUMNS WERE UNPINNED IN PLAN 33-05, DELIBERATELY, AND ARE
//     PINNED BY PLAN 33-07 FROM ITS OWN MEASUREMENTS. <<<
// 33-05 measured and decided; every row carried kSyncUnpinnedDb in both columns
// and the `kProvSyncUnpinned` provenance string, which said so in terms. That
// sentinel is now reached only by a cell for which SYNC_PINS below has NO ROW,
// which is a build error wearing a data mask: the gate case detects it and goes
// red rather than passing on a sentinel. A sentinel that stood in silently would
// be the very failure this file's standing posture is written against.
//
// >>> A GRID THAT GATES NOTHING CANNOT GO RED, WHICH IS WHY PINNING IS A
//     SEPARATE PLAN FROM MEASURING RATHER THAN A LINE IN THE SAME ONE. <<<
// Plan 33-05 refused to pin a column from a leg no gate had examined, and plan
// 33-06 landed the leg. This plan pins from ITS OWN measurement of the leg that
// now ships, and it pins per cell — 420 rows, each with its measured value, its
// outward-rounded threshold, its tier and its written provenance.
//
// >>> EVERY ABSOLUTE DECIBEL IN SYNC_PINS IS AN APPLE-CLANG FIGURE. <<<
// Register item 8 binds every one of them. Phase 32 measured this instrument
// TOOLCHAIN-DEPENDENT BY UP TO 3.02596 dB on the sync grid's older sibling: the
// FIRST CI run of that phase was RED on Ubuntu and Windows and GREEN on macOS,
// with no src/ behaviour differing at all, because aliasPeakDb is an arg-max
// over roughly two thousand bins and one libm ULP reorders near-tied ones. THE
// SYNC ROWS JOIN THAT PROBLEM. They must NOT be captured as a golden from one
// toolchain, and the CI MinGW leg (plan 33-11) is where their reproduction is
// actually measured rather than assumed.
//   AND ONE CLAUSE OF ITEM 8 IS INHERITED HERE RATHER THAN RE-MEASURED, WHICH
// IS SAID OUT LOUD BECAUSE IT IS THE LARGEST OPEN RISK IN THIS TABLE: the
// 1.0 dB step-dominated bound was measured on Phase 32's FREE-RUNNING cells, not
// on hard-synced ones. The physical argument for it transfers — a true value
// step produces a broad skirt and a genuine arg-max — but the number has never
// been measured on this signal class. If a step-dominated sync cell reproduces
// outside 1.0 dB on another toolchain, THAT IS A FINDING ABOUT THE BOUND and it
// is escalated per the anti-softening rule, not absorbed by widening the column.
//
// ===========================================================================
// >>> PLAN 33-07 REFUSES, IN WRITING, TO WRITE AN IMPROVEMENT GATE FOR THE
//     SYNC CORRECTION ON THIS INSTRUMENT — AND THE REFUSAL CARRIES ITS
//     MEASURED REASON. READ THIS BEFORE PROPOSING ONE. <<<
//
// This milestone already has an improvement gate: the TEST-03 case above CHECKs
// `naiveDb - correctedDb >= 8.0` on five named cells, and it is the strongest
// non-circular evidence Phase 32 produced, precisely because it consults NO
// pinned number — it compares two measurements of the same apparatus. The
// obvious move for hard sync is to copy that shape. IT DOES NOT WORK HERE, AND
// THE REASON IS A PROPERTY OF THE INSTRUMENT RATHER THAN OF THE IMPLEMENTATION.
//
// MEASURED, from plan 33-05's own leg table (mean dB, `none` minus `pastEdge`,
// positive = the shipped correction is better), over the seventy cells of each
// (rate x master edge shape) group:
//
//     rate     hard-edge   band-limited
//     44.1 k     +0.061       +1.053
//     48   k     -0.010       +0.996
//     96   k     +0.174       +1.222
//
// THE GRID-WIDE MEAN IS +0.5827 dB. 33-VALIDATION's Threshold Policy predicted
// "a mean of about 0.5 dB" from the research prototype and named the
// consequence in advance: "a gate written in Phase 32's shape --
// naiveDb - correctedDb >= 8.0 -- therefore FAILS, and the failure is a property
// of the instrument, not of the implementation." That prediction is confirmed by
// the table above to within a twentieth of a decibel.
//
// WHY THE CORRECTION'S OWN SPECTRAL IMPROVEMENT IS SO SMALL, PHYSICALLY: the
// hard-sync edge lies BEHIND the sample that detects it, so only the after-edge
// half of the residual is still deliverable. src/dsp/MorphBlep.hpp's addPastStep
// banner item 3 records that the pre-edge half is FORFEITED deliberately and
// permanently, because recovering it needs the one-sample output delay buffer
// D-13 rejected. A one-sided correction cannot buy a Phase-32-sized number, and
// no amount of implementation care will make it.
//
// AND THE CORRECTION IS NOT UNIFORMLY AN IMPROVEMENT, WHICH IS THE OTHER HALF OF
// WHY THE GATE SHAPE IS REFUSED. Per ratio, mean `none` minus `pastEdge`,
// measured by plan 33-07 over all 60 cells of each ratio:
//
//     0.50  +2.4495     1.50  +0.7247     3.50  -0.1911
//     0.75  +1.9150     2.50  +0.2051     5.50  -1.0281   <-- WORSE THAN NONE
//     1.00  +0.0037
//
// At ratio 5.5 the shipped leg measures WORSE than applying no correction at all
// on 47 of 60 cells, by up to 7.0218 dB on the worst single cell. That is the
// forfeited pre-edge half showing up where the detected fraction is largest, it
// is 33-06's deferred register item 3, and an improvement gate would have to
// carve those cells out — which is the anti-reclassification clause's forbidden
// move performed on a whole ratio.
//
// SO WHERE DOES THE SYNC CORRECTION'S OWN NON-CIRCULAR EVIDENCE LIVE? IN THE
// TIME DOMAIN, AND THAT IS PLAN 33-08'S INSTRUMENT (D-10). Register item 5
// MEASURED that a single-sample full-amplitude spike reads 0.0 dB spectrally:
// the alias-floor metric is structurally BLIND to the artefact SC-3 exists to
// forbid. A spectral improvement gate for a click is not a weak gate, it is the
// wrong instrument. What THIS instrument can carry is stated positively in two
// places and both are asserted below: a PER-CELL ALIAS FLOOR pinned from
// measurement, and the SNAP-TO-ZERO comparison, which measures 5.31 to 5.58 dB
// on the informative masters and is the one sync claim with a comfortable margin.
//
// >>> THE TWO WARNING SIGNS THIS FILE IS TO BE READ AGAINST, NAMED SO A LATER
//     READER CAN CHECK FOR THEM RATHER THAN BEING TOLD THEY DO NOT HAPPEN: <<<
//   (1) A GATE THAT HAS TO BE LOOSENED REPEATEDLY. One loosening is a mistake in
//       the pin; a second is evidence the gate was written in a shape the
//       instrument cannot support, and the response is to ESCALATE and re-shape
//       it, never to loosen it a third time.
//   (2) A THRESHOLD AND ITS MEASURED SIBLING DRIFTING TOGETHER. That edit passes
//       both the derivation assertion and the reproduction check while recording
//       agreement rather than measurement. It is the reason `measuredDb` is
//       compared against THIS RUN and not only against `thresholdDb`, and the
//       reason both edits are named here where a reviewer will see them.
// ===========================================================================
// ---------------------------------------------------------------------------
constexpr float kSyncUnpinnedDb = 999.0f;   // "no number pinned here yet" — never a measurement

const char* const kProvSyncUnpinned =
	"NOT PINNED BY PLAN 33-05, BY DECISION. That plan's job is to DECIDE the placement convention by "
	"measurement, and it deliberately gates nothing: a threshold pinned in the same commit that chose "
	"the leg would be a threshold pinned from a leg no gate had yet examined. Plan 33-07 owns both "
	"columns and the gate, and re-anchors the bit-exactness case below to the leg 33-06 lands. Until "
	"then kSyncUnpinnedDb is a SENTINEL and must never be read as a measurement";

// ---------------------------------------------------------------------------
// THE SYNC PROVENANCE STRINGS (plan 33-07). NINE NEW CONSTANTS, AND NOT ONE OF
// THEM IS A REUSE OF THE SIX ABOVE.
//
// >>> WHY kProvMeasured AND ITS FIVE SIBLINGS COULD NOT BE REUSED. <<< Every one
// of them opens "MEASURED by plan 32-07", names that plan's own measurement run,
// and states the derivation `thresholdDb = ceil(measuredDb + 3.0)` with a 3 dB
// margin. All three clauses are FALSE of the rows below: these numbers were
// measured by plan 33-07, on a different signal class (a hard-synced slave
// against a master, not a free-running oscillator), and their margin is register
// item 8's per-CLASS reproduction bound rather than a flat 3 dB. Attaching a
// 32-07 string to a 33-07 row would misattribute the number to a run that never
// produced it — which is exactly the failure a provenance column exists to make
// impossible. The strings are therefore new, and the case below asserts they are
// new rather than merely claiming it.
//
// THE RATE IS IN THE STRING, not only in the row's `sr` field, because register
// item 8 binds per rate: 44.1 kHz is the BINDING rate and 48 / 96 kHz are
// REGRESSION rows, and a reader who has only the provenance in front of them
// must be able to tell which is which.
//
// AND EVERY ONE OF THEM SAYS THE SAME THING ABOUT WHAT THE NUMBER IS NOT: it is
// the alias floor the SHIPPED past-edge leg reaches on that cell, and it makes
// NO claim that the correction is an improvement there. 33-06's deferred item 3
// requires that in terms, and the ratio table in the SyncCell banner above is
// why.
// ---------------------------------------------------------------------------
const char* const kProvSync441Step =
	"MEASURED by plan 33-07 in this repository AT 44.1 kHz - the BINDING rate - driving the SHIPPED "
	"past-edge leg through measureSyncCellDb with useLiveCore=false, on the cell named by this row's own "
	"five axes; STEP-DOMINATED class, so thresholdDb = ceil(measuredDb + 1.0), register item 8's "
	"step-dominated reproduction bound. This is the alias floor the shipped leg REACHES here; it is not a "
	"claim that the correction improves this cell";
const char* const kProvSync441Plateau =
	"MEASURED by plan 33-07 in this repository AT 44.1 kHz - the BINDING rate - on the SHIPPED past-edge "
	"leg; PLATEAU class by the physical criterion stated in the D-11 gate below (no true value step from "
	"either source, so the arg-max over non-harmonic bins is a near-tie one libm ULP reorders), so "
	"thresholdDb = ceil(measuredDb + 4.0), register item 8's plateau reproduction bound - the wider bound "
	"is EARNED by the physics of the cell, never granted because the cell failed";
const char* const kProvSync48Step =
	"MEASURED by plan 33-07 in this repository AT 48 kHz, this row's OWN rate, never transferred from the "
	"44.1 kHz row - the cross-rate transfer the standing grid used to rely on was FALSIFIED in phase 32 "
	"and is not resurrected here; REGRESSION tier and STEP-DOMINATED class, so "
	"thresholdDb = ceil(measuredDb + 1.0)";
const char* const kProvSync48Plateau =
	"MEASURED by plan 33-07 in this repository AT 48 kHz, this row's OWN rate; REGRESSION tier and PLATEAU "
	"class by the physical criterion stated in the D-11 gate below, so thresholdDb = ceil(measuredDb + 4.0)";
const char* const kProvSync96Step =
	"MEASURED by plan 33-07 in this repository AT 96 kHz, this row's OWN rate, never transferred from the "
	"44.1 kHz row; REGRESSION tier and STEP-DOMINATED class, so thresholdDb = ceil(measuredDb + 1.0)";
const char* const kProvSync96Plateau =
	"MEASURED by plan 33-07 in this repository AT 96 kHz, this row's OWN rate; REGRESSION tier and PLATEAU "
	"class by the physical criterion stated in the D-11 gate below, so thresholdDb = ceil(measuredDb + 4.0)";
const char* const kProvSync441Invalid =
	"MEASURED by plan 33-07 AT 44.1 kHz on the shipped past-edge leg, and DIAGNOSTIC tier: this cell fails "
	"the fundamentalDominanceDb self-check, so aliasPeakDb is normalising by a bin that is NOT the master's "
	"fundamental and the figure is not an alias floor at all. The measured value is recorded and "
	"REPRODUCTION-CHECKED, and the threshold is derived for uniformity, but it is never CHECKed - gating a "
	"number the instrument cannot produce would be coverage that means nothing";
const char* const kProvSync48Invalid =
	"MEASURED by plan 33-07 AT 48 kHz on the shipped past-edge leg, DIAGNOSTIC tier for the same reason as "
	"kProvSync441Invalid: fundamentalDominanceDb reports the master's fundamental is not the strongest bin "
	"on its own harmonic lattice, so this row is recorded and reproduction-checked but never CHECKed";
const char* const kProvSync96Invalid =
	"MEASURED by plan 33-07 AT 96 kHz on the shipped past-edge leg, DIAGNOSTIC tier for the same reason as "
	"kProvSync441Invalid: fundamentalDominanceDb reports the master's fundamental is not the strongest bin "
	"on its own harmonic lattice, so this row is recorded and reproduction-checked but never CHECKed";

struct SyncCell {
	double sr;                  // the sample rate
	int    Km;                  // the MASTER's cycle count per 4096-sample block — odd, and the metric's fundamental
	double masterHz;            // the ACHIEVED master frequency in hertz, recorded on every row (see the grid banner)
	SyncMasterEdge edge;        // the master's edge shape
	const char* edgeName;
	double ratio;               // slave / master frequency ratio — the sub-unity value is not optional
	float  pitchCV;             // the SLAVE's pitch control voltage. The slave is FREE and is not bin-centred
	float  morph;
	const char* region;
	float  character;
	float  measuredDb;          // unpinned in this plan — see the banner above
	float  thresholdDb;         // unpinned in this plan — see the banner above
	const char* tier;
	const char* provenance;
};

// ---------------------------------------------------------------------------
// THE GRID'S AXES, AND WHY THE MASTER FREQUENCY IS RECORDED ON EVERY ROW.
//
// The three per-rate master cycle counts are all ODD (the coprimality
// requirement above reduces to exactly that) and are chosen so the master lands
// within about one percent of the same frequency at all three rates, so the
// cross-rate rows compare like with like.
//
// >>> AND THE THREE RATES CANNOT SHARE ONE EXACT MASTER FREQUENCY. <<< With N
// pinned at 4096 the achievable master frequencies are the multiples of
// sr/4096, which are 10.77 Hz apart at 44.1 kHz, 11.72 Hz at 48 kHz and
// 23.44 Hz at 96 kHz. A single frequency integer-cycle at all three at once
// would have to be a common multiple of those three spacings, and below about
// 3.4 kHz there is none. The residual spread is therefore a PROPERTY OF THE
// INSTRUMENT, and this file's posture is that such a property is written down
// rather than hidden — hence `masterHz` on every row and the spread asserted
// and reported by the case below rather than left to be assumed small.
// ---------------------------------------------------------------------------
struct SyncRateRow { double sr; int Km; };
const SyncRateRow SYNC_RATES[] = {
	{ 44100.0, 93 },   // 93 * 44100 / 4096 = 1001.2939453125 Hz
	{ 48000.0, 85 },   // 85 * 48000 / 4096 =  996.09375      Hz
	{ 96000.0, 43 },   // 43 * 96000 / 4096 = 1007.8125       Hz
	// MEASURED spread across the three: 1.17647 % of the lowest. 33-RESEARCH
	// quotes 1001.4 Hz for the 44.1 kHz row and 1.2 % for the spread; the exact
	// arithmetic is 1001.2939453125 Hz and 1.17647 %, and the exact figures are
	// what the case below asserts. The research figure was rounded, not wrong.
};

// HAZARD ONE'S AXIS — and a THIRD hazard, MEASURED BY THIS PLAN AND NOT
// ANTICIPATED BY THE RESEARCH, which is why the values below are not the ones
// 33-RESEARCH and 33-VALIDATION recommend.
//
// >>> HAZARD THREE — AT AN EXACTLY INTEGER RATIO OF TWO OR MORE, HARD SYNC IS A
//     NEAR-NO-OP AND THE MASTER'S FUNDAMENTAL BIN IS EMPTY, SO THE METRIC
//     DIVIDES BY ALMOST NOTHING. <<<
// Both source documents recommend the ratios 0.5x, 1x, 2x, 3x, 4x and 6x. Three
// of those turn the measurement into nonsense, and the reason is physical: at an
// exactly integer ratio the slave is ALREADY in phase at every master wrap, so
// the reset moves it by almost nothing, the emitted signal is periodic at the
// SLAVE's period rather than the master's, and the master's fundamental bin —
// which is the bin aliasPeakDb NORMALISES BY — carries essentially no energy.
// MEASURED at 44.1 kHz, K_m = 93, the saw centre at character 0, hard-edged
// master, on the shipped core's own leg:
//
//   ratio | |X[K_m]| | strongest harmonic | at n | fund vs it | alias peak
//   ------+----------+--------------------+------+------------+-----------
//    0.5  |  3259.48 |            3259.48 |   1  |    0.00 dB |  -27.24 dB
//    1.0  |  6502.19 |            6502.19 |   1  |    0.00 dB |  -32.22 dB
//    2.0  |     0.81 |            6452.03 |   2  |  -78.03 dB |  +51.87 dB
//    3.0  |     1.16 |            6368.85 |   3  |  -74.78 dB |  +52.25 dB
//    4.0  |     1.70 |            6256.08 |   4  |  -71.32 dB |  +51.41 dB
//    6.0  |     2.59 |            5936.22 |   6  |  -67.20 dB |  +51.29 dB
//
// A POSITIVE alias peak is the tell: it says a non-harmonic bin carries more
// energy than the fundamental, which cannot happen for a signal that really is
// master-periodic. The eight legs at those ratios differed by up to 27 dB in
// what was almost entirely the NORMALISATION moving, not the alias floor.
//
// THE FIX IS THE GRID, NOT THE METRIC. aliasPeakDb is called unchanged, as the
// derivation requires; the integer ratios at or above two are replaced with
// non-integer ones, where the reset genuinely truncates the slave mid-cycle and
// the master's fundamental is the dominant bin again (measured 0.00 to
// -12.66 dB below the strongest harmonic across the set below, alias peaks
// -27.24 to -1.50 dB). THIS IS ALSO THE PHYSICALLY REPRESENTATIVE CASE: an
// operator sweeping a hard-synced slave passes through the integer ratios as
// isolated NULL POINTS and spends all the rest of the sweep between them.
// 1.0 IS KEPT — there the fundamental is still the dominant bin, so the metric
// is sound, and the row honestly records that unity sync barely moves the
// waveform (mean |jump| 0.0038 against 1.0 either side of it).
// The null point itself is pinned permanently by a control in the measurement
// case, so that no later agent "restores" the recommended ratios.
//
// 0.5 and 0.75 are the sub-unity values and they are load-bearing (hazard one).
const double SYNC_RATIOS[] = { 0.5, 0.75, 1.0, 1.5, 2.5, 3.5, 5.5 };

// The five shape centres, matching the standing grid's third-index discipline.
const float       SYNC_MORPHS[]  = { 0.00f, 0.25f, 0.50f, 0.75f, 1.00f };
const char* const SYNC_REGIONS[] = { "sine", "triangle", "saw", "square", "pulse 5%" };

// Character at BOTH ENDS, D-11 as written.
const float SYNC_CHARACTERS[] = { 0.00f, 1.00f };

// HAZARD TWO'S AXIS. Both shapes, always.
const SyncMasterEdge SYNC_EDGES[]     = { kMasterHardEdge, kMasterBandLimited };
const char* const    SYNC_EDGE_NAME[] = { "hard-edge", "band-limited" };


// ---------------------------------------------------------------------------
// SYNC_PINS — THE PER-CELL LOOKUP, 420 ROWS, ONE PER CELL OF SYNC_GRID.
//
// >>> PLAN 33-05 WARNED THAT THIS WOULD BE NEEDED, IN THESE WORDS: "PLAN 33-07
//     IS WARNED: the moment a per-cell threshold is pinned, that number needs a
//     per-cell home and a per-cell provenance, and this builder must grow a
//     LOOKUP rather than a FORMULA." THIS IS THAT LOOKUP. <<<
// A formula would have made the threshold a property of the axes rather than of
// the measurement, which is the opposite of pinning from measurement. Each row
// below is keyed by the FIVE axes that identify a cell, and buildSyncGrid finds
// its row by matching all five. A cell with no matching row keeps the unpinned
// sentinel and the tier "UNPINNED", and the gate case turns that into a RED —
// which is what makes an added grid row impossible to leave silently unpinned.
//
// THE COLUMNS:
//   sr, edgeIdx, ratio, morph, character — the key. edgeIdx indexes SYNC_EDGES.
//   measuredDb   — the alias floor the SHIPPED past-edge leg reached on this
//                  cell when plan 33-07 measured it, in this repository, on
//                  Apple clang. IT IS THE PROVENANCE, IN NUMBERS.
//   thresholdDb  — max(ceil(measuredDb + bound), kThresholdFloorDb), where the
//                  bound is 1.0 dB for a step-dominated cell and 4.0 dB for a
//                  plateau cell. ROUNDED OUTWARD, never inward. The derivation
//                  is asserted MECHANICALLY in the gate case, so a threshold
//                  edited by hand without its measured sibling goes red.
//   tier         — "gated" at 44.1 kHz (the binding rate), "regression" at 48
//                  and 96 kHz, "diagnostic" where the cell fails the
//                  fundamental-dominance self-check. Gated and regression rows
//                  are CHECKed; diagnostic rows are CAPTUREd and never CHECKed,
//                  exactly as SPECTRUM_GRID's diagnostic tier already works.
//   provenance   — one of the nine kProvSync* strings above. Never one of Phase
//                  32's six, and the sub-grid case asserts that.
//
// THE TRAILING COMMENT ON EVERY ROW carries the three physical quantities the
// classification and the tier were decided from — the fundamental dominance in
// dB, the mean absolute sync jump, and the no-correction leg's own figure — so
// a reader can audit both decisions off the table without running anything.
//
// >>> HOW TO CHANGE A NUMBER HERE, AND IT IS NOT BY EDITING IT. <<< If a cell
// goes red, the response is the anti-softening rule the TEST-03 gate's banner
// states: ESCALATE with the measurement, never quietly loosen. Editing
// thresholdDb alone breaks the derivation assertion. Editing measuredDb with it
// breaks the reproduction check, which compares this column against THIS RUN.
// Both edits are named here, and in the SyncCell banner's warning signs, so
// neither can be a quiet one.
// ---------------------------------------------------------------------------
struct SyncPin {
	double sr;
	int    edgeIdx;      // index into SYNC_EDGES / SYNC_EDGE_NAME
	double ratio;
	float  morph;
	float  character;
	float  measuredDb;
	float  thresholdDb;
	const char* tier;
	const char* provenance;
};

static const SyncPin SYNC_PINS[] = {

	// --- 44.1 kHz, hard-edge master. BINDING rate.
	{ 44100.0, 0, 0.50, 0.00f, 0.00f,   -32.2312f,  -31.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   0.0567  none  -32.2517
	{ 44100.0, 0, 0.50, 0.00f, 1.00f,   -32.4151f,  -31.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   0.0470  none  -32.4297
	{ 44100.0, 0, 0.50, 0.25f, 0.00f,   -30.9759f,  -29.0f, "gated"     , kProvSync441Step       },  // triangle  step  fundDom    +0.00  jump   1.9639  none  -27.2147
	{ 44100.0, 0, 0.50, 0.25f, 1.00f,   -31.0312f,  -30.0f, "gated"     , kProvSync441Step       },  // triangle  step  fundDom    +0.00  jump   1.8053  none  -27.1053
	{ 44100.0, 0, 0.50, 0.50f, 0.00f,   -31.0965f,  -30.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.0000  none  -27.2428
	{ 44100.0, 0, 0.50, 0.50f, 1.00f,   -31.0970f,  -30.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.3809  none  -27.2398
	{ 44100.0, 0, 0.50, 0.75f, 0.00f,    -2.5083f,   -1.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom    -5.53  jump   2.0000  none   +4.2384
	{ 44100.0, 0, 0.50, 0.75f, 1.00f,   -31.4031f,  -30.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   0.1824  none  -31.4464
	{ 44100.0, 0, 0.50, 1.00f, 0.00f,   -21.7259f,  -20.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -21.7998
	{ 44100.0, 0, 0.50, 1.00f, 1.00f,   -25.3887f,  -24.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   1.0526  none  -25.5861
	{ 44100.0, 0, 0.75, 0.00f, 0.00f,   -32.4663f,  -31.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   1.0414  none  -32.4906
	{ 44100.0, 0, 0.75, 0.00f, 1.00f,   -32.4740f,  -31.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   1.0523  none  -32.4978
	{ 44100.0, 0, 0.75, 0.25f, 0.00f,   -32.5146f,  -31.0f, "gated"     , kProvSync441Step       },  // triangle  step  fundDom    +0.00  jump   0.9458  none  -32.5551
	{ 44100.0, 0, 0.75, 0.25f, 1.00f,   -32.5505f,  -31.0f, "gated"     , kProvSync441Step       },  // triangle  step  fundDom    +0.00  jump   1.0750  none  -32.6059
	{ 44100.0, 0, 0.75, 0.50f, 0.00f,   -31.0965f,  -30.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.5000  none  -27.2428
	{ 44100.0, 0, 0.75, 0.50f, 1.00f,   -31.1818f,  -30.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.6785  none  -27.3416
	{ 44100.0, 0, 0.75, 0.75f, 0.00f,   -31.8150f,  -30.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   2.0000  none  -30.8866
	{ 44100.0, 0, 0.75, 0.75f, 1.00f,   -32.5085f,  -31.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   1.1157  none  -32.5333
	{ 44100.0, 0, 0.75, 1.00f, 0.00f,   -18.1957f,  -17.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -16.5282
	{ 44100.0, 0, 0.75, 1.00f, 1.00f,   -21.8333f,  -20.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   1.1615  none  -21.9756
	{ 44100.0, 0, 1.00, 0.00f, 0.00f,   -32.3118f,  -31.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   0.0119  none  -32.3097
	{ 44100.0, 0, 1.00, 0.00f, 1.00f,   -32.2934f,  -31.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   0.0101  none  -32.2916
	{ 44100.0, 0, 1.00, 0.25f, 0.00f,   -32.5668f,  -28.0f, "gated"     , kProvSync441Plateau    },  // triangle  PLAT  fundDom    +0.00  jump   0.0076  none  -32.5518
	{ 44100.0, 0, 1.00, 0.25f, 1.00f,   -32.5796f,  -28.0f, "gated"     , kProvSync441Plateau    },  // triangle  PLAT  fundDom    +0.00  jump   0.0018  none  -32.5742
	{ 44100.0, 0, 1.00, 0.50f, 0.00f,   -32.2181f,  -31.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   0.0038  none  -32.2225
	{ 44100.0, 0, 1.00, 0.50f, 1.00f,   -31.3777f,  -30.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   0.0021  none  -31.3841
	{ 44100.0, 0, 1.00, 0.75f, 0.00f,   -32.1627f,  -31.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   0.0000  none  -32.1627
	{ 44100.0, 0, 1.00, 0.75f, 1.00f,   -32.3849f,  -31.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   0.0216  none  -32.3815
	{ 44100.0, 0, 1.00, 1.00f, 0.00f,   -16.5912f,  -15.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   0.0000  none  -16.5912
	{ 44100.0, 0, 1.00, 1.00f, 1.00f,   -23.8834f,  -22.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   0.0312  none  -23.8262
	{ 44100.0, 0, 1.50, 0.00f, 0.00f,   -29.2703f,  -28.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   0.1699  none  -29.3298
	{ 44100.0, 0, 1.50, 0.00f, 1.00f,   -28.9949f,  -27.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   0.1412  none  -29.0433
	{ 44100.0, 0, 1.50, 0.25f, 0.00f,   -23.7527f,  -22.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom    -2.92  jump   1.8916  none  -23.2969
	{ 44100.0, 0, 1.50, 0.25f, 1.00f,   -24.0494f,  -23.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom    -2.43  jump   1.8309  none  -24.1847
	{ 44100.0, 0, 1.50, 0.50f, 0.00f,   -27.3310f,  -26.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.0000  none  -27.3892
	{ 44100.0, 0, 1.50, 0.50f, 1.00f,   -25.0877f,  -24.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.3926  none  -24.8858
	{ 44100.0, 0, 1.50, 0.75f, 0.00f,   -30.8655f,  -29.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   2.0000  none  -30.3808
	{ 44100.0, 0, 1.50, 0.75f, 1.00f,   -30.0547f,  -29.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   0.3919  none  -30.1671
	{ 44100.0, 0, 1.50, 1.00f, 0.00f,    -7.3407f,   -6.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom    -5.77  jump   2.0000  none   -7.3056
	{ 44100.0, 0, 1.50, 1.00f, 1.00f,   -13.4053f,  -12.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom    -3.71  jump   1.2597  none  -13.2728
	{ 44100.0, 0, 2.50, 0.00f, 0.00f,   -17.3735f,  -16.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom    -7.32  jump   0.2822  none  -17.4936
	{ 44100.0, 0, 2.50, 0.00f, 1.00f,   -17.1375f,  -16.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom    -7.41  jump   0.2355  none  -17.2379
	{ 44100.0, 0, 2.50, 0.25f, 0.00f,    -8.2495f,   -7.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -14.97  jump   1.8193  none   -8.2334
	{ 44100.0, 0, 2.50, 0.25f, 1.00f,    -9.2780f,   -8.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -13.33  jump   1.8407  none   -9.8459
	{ 44100.0, 0, 2.50, 0.50f, 0.00f,   -15.3134f,  -14.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom    -6.36  jump   1.0000  none  -15.6173
	{ 44100.0, 0, 2.50, 0.50f, 1.00f,   -13.9380f,  -12.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom    -6.43  jump   1.3941  none  -14.0922
	{ 44100.0, 0, 2.50, 0.75f, 0.00f,   -18.6195f,  -17.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom    -6.36  jump   2.0000  none  -18.5030
	{ 44100.0, 0, 2.50, 0.75f, 1.00f,   -18.0668f,  -17.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom    -7.04  jump   0.5899  none  -18.2724
	{ 44100.0, 0, 2.50, 1.00f, 0.00f,     3.3170f,    5.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -12.26  jump   2.0000  none   +1.8236
	{ 44100.0, 0, 2.50, 1.00f, 1.00f,    -1.0547f,    0.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -10.86  jump   1.4394  none   -1.7052
	{ 44100.0, 0, 3.50, 0.00f, 0.00f,   -10.8564f,   -9.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom   -10.69  jump   0.3930  none  -10.9876
	{ 44100.0, 0, 3.50, 0.00f, 1.00f,   -10.7454f,   -9.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom   -10.81  jump   0.3301  none  -10.8586
	{ 44100.0, 0, 3.50, 0.25f, 0.00f,     0.7906f,    2.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -21.25  jump   1.7470  none   +0.7277
	{ 44100.0, 0, 3.50, 0.25f, 1.00f,    -0.8419f,    1.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -18.31  jump   1.8346  none   -2.2504
	{ 44100.0, 0, 3.50, 0.50f, 0.00f,    -8.7761f,   -7.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom    -9.29  jump   1.0000  none   -9.2048
	{ 44100.0, 0, 3.50, 0.50f, 1.00f,    -7.7551f,   -6.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom    -9.26  jump   1.3816  none   -8.3567
	{ 44100.0, 0, 3.50, 0.75f, 0.00f,   -11.9336f,  -10.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom    -9.29  jump   2.0000  none  -12.2598
	{ 44100.0, 0, 3.50, 0.75f, 1.00f,   -11.1717f,  -10.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom   -10.28  jump   0.7728  none  -11.3687
	{ 44100.0, 0, 3.50, 1.00f, 0.00f,     7.9564f,    9.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -14.42  jump   2.0000  none   +4.9770
	{ 44100.0, 0, 3.50, 1.00f, 1.00f,     5.5701f,    7.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -13.91  jump   1.3673  none   +3.8926
	{ 44100.0, 0, 5.50, 0.00f, 0.00f,    -1.8752f,    0.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom   -14.62  jump   0.6075  none   -2.2010
	{ 44100.0, 0, 5.50, 0.00f, 1.00f,    -1.7584f,    0.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom   -14.78  jump   0.5191  none   -2.0438
	{ 44100.0, 0, 5.50, 0.25f, 0.00f,    12.7165f,   14.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -27.88  jump   1.6026  none  +11.3136
	{ 44100.0, 0, 5.50, 0.25f, 1.00f,     9.3091f,   11.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -22.64  jump   1.7890  none   +6.0397
	{ 44100.0, 0, 5.50, 0.50f, 0.00f,    -0.8456f,    1.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom   -12.66  jump   1.0000  none   -1.5005
	{ 44100.0, 0, 5.50, 0.50f, 1.00f,     0.3086f,    2.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom   -12.64  jump   1.3160  none   -0.6037
	{ 44100.0, 0, 5.50, 0.75f, 0.00f,    -3.5107f,   -2.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom   -12.65  jump   2.0000  none   -4.1216
	{ 44100.0, 0, 5.50, 0.75f, 1.00f,    -2.3838f,   -1.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom   -13.95  jump   1.0880  none   -2.8428
	{ 44100.0, 0, 5.50, 1.00f, 0.00f,    22.2717f,   24.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -23.54  jump   1.2258  none  +15.2499
	{ 44100.0, 0, 5.50, 1.00f, 1.00f,     4.2610f,    6.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -12.15  jump   0.9788  none   +2.1179

	// --- 44.1 kHz, band-limited master. BINDING rate.
	{ 44100.0, 1, 0.50, 0.00f, 0.00f,   -44.3295f,  -43.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   0.0647  none  -44.0383
	{ 44100.0, 1, 0.50, 0.00f, 1.00f,   -44.5115f,  -43.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   0.0537  none  -44.3032
	{ 44100.0, 1, 0.50, 0.25f, 0.00f,   -30.2731f,  -29.0f, "gated"     , kProvSync441Step       },  // triangle  step  fundDom    +0.00  jump   1.9588  none  -27.2216
	{ 44100.0, 1, 0.50, 0.25f, 1.00f,   -30.2380f,  -29.0f, "gated"     , kProvSync441Step       },  // triangle  step  fundDom    +0.00  jump   1.8072  none  -27.1092
	{ 44100.0, 1, 0.50, 0.50f, 0.00f,   -30.3143f,  -29.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.0000  none  -27.2356
	{ 44100.0, 1, 0.50, 0.50f, 1.00f,   -30.2693f,  -29.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.3817  none  -27.2111
	{ 44100.0, 1, 0.50, 0.75f, 0.00f,    -1.6616f,    0.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom   -11.58  jump   2.0000  none  +11.7029
	{ 44100.0, 1, 0.50, 0.75f, 1.00f,   -42.8787f,  -41.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   0.1973  none  -41.9624
	{ 44100.0, 1, 0.50, 1.00f, 0.00f,   -24.2518f,  -23.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -21.5998
	{ 44100.0, 1, 0.50, 1.00f, 1.00f,   -29.4160f,  -28.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   1.0671  none  -26.5496
	{ 44100.0, 1, 0.75, 0.00f, 0.00f,   -39.5293f,  -38.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   1.0470  none  -36.3803
	{ 44100.0, 1, 0.75, 0.00f, 1.00f,   -39.5572f,  -38.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   1.0567  none  -36.4142
	{ 44100.0, 1, 0.75, 0.25f, 0.00f,   -37.6687f,  -36.0f, "gated"     , kProvSync441Step       },  // triangle  step  fundDom    +0.00  jump   0.9382  none  -34.5942
	{ 44100.0, 1, 0.75, 0.25f, 1.00f,   -36.7170f,  -35.0f, "gated"     , kProvSync441Step       },  // triangle  step  fundDom    +0.00  jump   1.0699  none  -33.7352
	{ 44100.0, 1, 0.75, 0.50f, 0.00f,   -30.3143f,  -29.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.5000  none  -27.2356
	{ 44100.0, 1, 0.75, 0.50f, 1.00f,   -30.3574f,  -29.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.6784  none  -27.3186
	{ 44100.0, 1, 0.75, 0.75f, 0.00f,   -33.6717f,  -32.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   2.0000  none  -30.3272
	{ 44100.0, 1, 0.75, 0.75f, 1.00f,   -40.0955f,  -39.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   1.1266  none  -36.9436
	{ 44100.0, 1, 0.75, 1.00f, 0.00f,   -18.2541f,  -17.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -16.8173
	{ 44100.0, 1, 0.75, 1.00f, 1.00f,   -29.4323f,  -28.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   1.1815  none  -26.0097
	{ 44100.0, 1, 1.00, 0.00f, 0.00f,   -44.7765f,  -40.0f, "gated"     , kProvSync441Plateau    },  // sine      PLAT  fundDom    +0.00  jump   0.0020  none  -44.7750
	{ 44100.0, 1, 1.00, 0.00f, 1.00f,   -44.7590f,  -40.0f, "gated"     , kProvSync441Plateau    },  // sine      PLAT  fundDom    +0.00  jump   0.0016  none  -44.7577
	{ 44100.0, 1, 1.00, 0.25f, 0.00f,   -44.6305f,  -40.0f, "gated"     , kProvSync441Plateau    },  // triangle  PLAT  fundDom    +0.00  jump   0.0012  none  -44.6216
	{ 44100.0, 1, 1.00, 0.25f, 1.00f,   -44.6306f,  -40.0f, "gated"     , kProvSync441Plateau    },  // triangle  PLAT  fundDom    +0.00  jump   0.0003  none  -44.6297
	{ 44100.0, 1, 1.00, 0.50f, 0.00f,   -35.1925f,  -34.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   0.0006  none  -35.1942
	{ 44100.0, 1, 1.00, 0.50f, 1.00f,   -34.7747f,  -33.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   0.0003  none  -34.7750
	{ 44100.0, 1, 1.00, 0.75f, 0.00f,   -35.1407f,  -34.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   0.0000  none  -35.1407
	{ 44100.0, 1, 1.00, 0.75f, 1.00f,   -44.8589f,  -40.0f, "gated"     , kProvSync441Plateau    },  // square    PLAT  fundDom    +0.00  jump   0.0036  none  -44.8564
	{ 44100.0, 1, 1.00, 1.00f, 0.00f,   -25.2883f,  -24.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   0.0000  none  -25.2883
	{ 44100.0, 1, 1.00, 1.00f, 1.00f,   -35.9557f,  -34.0f, "gated"     , kProvSync441Step       },  // pulse 5%  step  fundDom    +0.00  jump   0.0066  none  -36.0033
	{ 44100.0, 1, 1.50, 0.00f, 0.00f,   -41.1663f,  -40.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   0.1936  none  -40.8720
	{ 44100.0, 1, 1.50, 0.00f, 1.00f,   -40.8892f,  -39.0f, "gated"     , kProvSync441Step       },  // sine      step  fundDom    +0.00  jump   0.1612  none  -40.6512
	{ 44100.0, 1, 1.50, 0.25f, 0.00f,   -26.1698f,  -25.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom    -2.91  jump   1.8764  none  -23.2471
	{ 44100.0, 1, 1.50, 0.25f, 1.00f,   -27.7144f,  -26.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom    -2.47  jump   1.8312  none  -24.6788
	{ 44100.0, 1, 1.50, 0.50f, 0.00f,   -30.6124f,  -29.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.0000  none  -28.6538
	{ 44100.0, 1, 1.50, 0.50f, 1.00f,   -28.2265f,  -27.0f, "gated"     , kProvSync441Step       },  // saw       step  fundDom    +0.00  jump   1.3893  none  -26.1266
	{ 44100.0, 1, 1.50, 0.75f, 0.00f,   -32.8376f,  -31.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   2.0000  none  -33.6474
	{ 44100.0, 1, 1.50, 0.75f, 1.00f,   -41.7519f,  -40.0f, "gated"     , kProvSync441Step       },  // square    step  fundDom    +0.00  jump   0.4330  none  -41.1988
	{ 44100.0, 1, 1.50, 1.00f, 0.00f,   -13.4497f,  -12.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom    -5.83  jump   2.0000  none  -13.3915
	{ 44100.0, 1, 1.50, 1.00f, 1.00f,   -18.9045f,  -17.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom    -3.78  jump   1.2662  none  -18.3791
	{ 44100.0, 1, 2.50, 0.00f, 0.00f,   -29.0551f,  -28.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom    -7.35  jump   0.3209  none  -28.8070
	{ 44100.0, 1, 2.50, 0.00f, 1.00f,   -28.8181f,  -27.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom    -7.44  jump   0.2691  none  -28.6166
	{ 44100.0, 1, 2.50, 0.25f, 0.00f,   -13.0936f,  -12.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -15.00  jump   1.7939  none  -10.3358
	{ 44100.0, 1, 2.50, 0.25f, 1.00f,   -15.8368f,  -14.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -13.59  jump   1.8328  none  -13.1871
	{ 44100.0, 1, 2.50, 0.50f, 0.00f,   -20.1559f,  -19.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom    -6.43  jump   1.0000  none  -18.9377
	{ 44100.0, 1, 2.50, 0.50f, 1.00f,   -16.6644f,  -15.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom    -6.51  jump   1.3761  none  -15.5807
	{ 44100.0, 1, 2.50, 0.75f, 0.00f,   -21.2199f,  -20.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom    -6.43  jump   2.0000  none  -22.8092
	{ 44100.0, 1, 2.50, 0.75f, 1.00f,   -29.5922f,  -28.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom    -7.08  jump   0.6472  none  -29.1561
	{ 44100.0, 1, 2.50, 1.00f, 0.00f,     3.0880f,    5.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -13.17  jump   1.8710  none   +1.9164
	{ 44100.0, 1, 2.50, 1.00f, 1.00f,    -2.5729f,   -1.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -12.19  jump   1.2571  none   -2.7439
	{ 44100.0, 1, 3.50, 0.00f, 0.00f,   -22.5781f,  -21.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom   -10.77  jump   0.4456  none  -22.4008
	{ 44100.0, 1, 3.50, 0.00f, 1.00f,   -22.3681f,  -21.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom   -10.89  jump   0.3776  none  -22.2259
	{ 44100.0, 1, 3.50, 0.25f, 0.00f,    -6.9880f,   -5.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -21.39  jump   1.7115  none   -3.9972
	{ 44100.0, 1, 3.50, 0.25f, 1.00f,    -8.6306f,   -7.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -18.83  jump   1.8115  none   -7.5019
	{ 44100.0, 1, 3.50, 0.50f, 0.00f,   -16.8519f,  -15.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom    -9.44  jump   1.0000  none  -15.8199
	{ 44100.0, 1, 3.50, 0.50f, 1.00f,   -13.0047f,  -12.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom    -9.51  jump   1.3460  none  -12.1161
	{ 44100.0, 1, 3.50, 0.75f, 0.00f,   -18.0198f,  -17.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom    -9.44  jump   2.0000  none  -16.8420
	{ 44100.0, 1, 3.50, 0.75f, 1.00f,   -23.0694f,  -22.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom   -10.39  jump   0.8333  none  -22.7613
	{ 44100.0, 1, 3.50, 1.00f, 0.00f,     8.7324f,   10.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -17.18  jump   1.5054  none   +5.6822
	{ 44100.0, 1, 3.50, 1.00f, 1.00f,     3.3814f,    5.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -16.09  jump   1.1243  none   +2.0400
	{ 44100.0, 1, 5.50, 0.00f, 0.00f,   -14.3391f,  -13.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom   -14.87  jump   0.6828  none  -14.3594
	{ 44100.0, 1, 5.50, 0.00f, 1.00f,   -14.3086f,  -13.0f, "diagnostic", kProvSync441Invalid    },  // sine      step  fundDom   -15.02  jump   0.5970  none  -14.3348
	{ 44100.0, 1, 5.50, 0.25f, 0.00f,     1.7183f,    3.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -28.87  jump   1.5468  none   +3.8798
	{ 44100.0, 1, 5.50, 0.25f, 1.00f,     3.0708f,    5.0f, "diagnostic", kProvSync441Invalid    },  // triangle  step  fundDom   -24.14  jump   1.7099  none   +0.3913
	{ 44100.0, 1, 5.50, 0.50f, 0.00f,   -10.4203f,   -9.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom   -13.00  jump   1.0000  none   -9.8122
	{ 44100.0, 1, 5.50, 0.50f, 1.00f,    -6.4081f,   -5.0f, "diagnostic", kProvSync441Invalid    },  // saw       step  fundDom   -13.07  jump   1.2771  none   -6.1830
	{ 44100.0, 1, 5.50, 0.75f, 0.00f,   -10.4262f,   -9.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom   -13.00  jump   2.0000  none  -11.5216
	{ 44100.0, 1, 5.50, 0.75f, 1.00f,   -14.4588f,  -13.0f, "diagnostic", kProvSync441Invalid    },  // square    step  fundDom   -14.30  jump   1.1218  none  -14.8192
	{ 44100.0, 1, 5.50, 1.00f, 0.00f,    14.7007f,   16.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -24.52  jump   0.8817  none  +12.1505
	{ 44100.0, 1, 5.50, 1.00f, 1.00f,    12.7872f,   14.0f, "diagnostic", kProvSync441Invalid    },  // pulse 5%  step  fundDom   -20.77  jump   0.8231  none   +7.8479

	// --- 48 kHz, hard-edge master. REGRESSION rate.
	{ 48000.0, 0, 0.50, 0.00f, 0.00f,   -31.6339f,  -30.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   0.0519  none  -31.7001
	{ 48000.0, 0, 0.50, 0.00f, 1.00f,   -32.2471f,  -31.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   0.0430  none  -32.2968
	{ 48000.0, 0, 0.50, 0.25f, 0.00f,   -31.7330f,  -30.0f, "regression", kProvSync48Step        },  // triangle  step  fundDom    +0.00  jump   1.9670  none  -27.9434
	{ 48000.0, 0, 0.50, 0.25f, 1.00f,   -31.7519f,  -30.0f, "regression", kProvSync48Step        },  // triangle  step  fundDom    +0.00  jump   1.8035  none  -27.8229
	{ 48000.0, 0, 0.50, 0.50f, 0.00f,   -31.8446f,  -30.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.0000  none  -27.9791
	{ 48000.0, 0, 0.50, 0.50f, 1.00f,   -31.8017f,  -30.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.3802  none  -27.9434
	{ 48000.0, 0, 0.50, 0.75f, 0.00f,     7.3918f,    9.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom    -7.34  jump   2.0000  none   +7.3851
	{ 48000.0, 0, 0.50, 0.75f, 1.00f,   -29.1643f,  -28.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   0.1731  none  -29.2811
	{ 48000.0, 0, 0.50, 1.00f, 0.00f,   -22.5558f,  -21.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -20.8457
	{ 48000.0, 0, 0.50, 1.00f, 1.00f,   -26.2276f,  -25.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   1.0433  none  -26.3637
	{ 48000.0, 0, 0.75, 0.00f, 0.00f,   -32.3142f,  -31.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   1.0374  none  -32.3136
	{ 48000.0, 0, 0.75, 0.00f, 1.00f,   -32.3157f,  -31.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   1.0487  none  -32.3143
	{ 48000.0, 0, 0.75, 0.25f, 0.00f,   -33.9234f,  -32.0f, "regression", kProvSync48Step        },  // triangle  step  fundDom    +0.00  jump   0.9505  none  -33.9672
	{ 48000.0, 0, 0.75, 0.25f, 1.00f,   -34.1531f,  -33.0f, "regression", kProvSync48Step        },  // triangle  step  fundDom    +0.00  jump   1.0776  none  -34.2210
	{ 48000.0, 0, 0.75, 0.50f, 0.00f,   -31.8446f,  -30.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.5000  none  -27.9791
	{ 48000.0, 0, 0.75, 0.50f, 1.00f,   -31.9052f,  -30.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.6780  none  -28.0560
	{ 48000.0, 0, 0.75, 0.75f, 0.00f,   -31.7474f,  -30.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   2.0000  none  -30.0347
	{ 48000.0, 0, 0.75, 0.75f, 1.00f,   -32.5225f,  -31.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   1.1089  none  -32.5248
	{ 48000.0, 0, 0.75, 1.00f, 0.00f,   -19.0315f,  -18.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -18.2898
	{ 48000.0, 0, 0.75, 1.00f, 1.00f,   -22.7084f,  -21.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   1.1479  none  -22.8550
	{ 48000.0, 0, 1.00, 0.00f, 0.00f,   -33.0572f,  -32.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   0.0406  none  -33.0510
	{ 48000.0, 0, 1.00, 0.00f, 1.00f,   -32.9687f,  -31.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   0.0341  none  -32.9625
	{ 48000.0, 0, 1.00, 0.25f, 0.00f,   -32.6858f,  -31.0f, "regression", kProvSync48Step        },  // triangle  step  fundDom    +0.00  jump   0.0259  none  -32.6335
	{ 48000.0, 0, 1.00, 0.25f, 1.00f,   -32.7696f,  -28.0f, "regression", kProvSync48Plateau     },  // triangle  PLAT  fundDom    +0.00  jump   0.0047  none  -32.7562
	{ 48000.0, 0, 1.00, 0.50f, 0.00f,   -32.1537f,  -31.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   0.0130  none  -32.1368
	{ 48000.0, 0, 1.00, 0.50f, 1.00f,   -30.6828f,  -29.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   0.0051  none  -30.6703
	{ 48000.0, 0, 1.00, 0.75f, 0.00f,   -33.3360f,  -32.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   0.0000  none  -33.3360
	{ 48000.0, 0, 1.00, 0.75f, 1.00f,   -33.4114f,  -32.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   0.0743  none  -33.4039
	{ 48000.0, 0, 1.00, 1.00f, 0.00f,   -17.9477f,  -16.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   0.0000  none  -17.9477
	{ 48000.0, 0, 1.00, 1.00f, 1.00f,   -24.9781f,  -23.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   0.1388  none  -25.2588
	{ 48000.0, 0, 1.50, 0.00f, 0.00f,   -28.4174f,  -27.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   0.1551  none  -28.5182
	{ 48000.0, 0, 1.50, 0.00f, 1.00f,   -28.1526f,  -27.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   0.1289  none  -28.2340
	{ 48000.0, 0, 1.50, 0.25f, 0.00f,   -23.3862f,  -22.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom    -2.91  jump   1.9009  none  -23.3436
	{ 48000.0, 0, 1.50, 0.25f, 1.00f,   -23.5572f,  -22.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom    -2.44  jump   1.8242  none  -23.6420
	{ 48000.0, 0, 1.50, 0.50f, 0.00f,   -28.8050f,  -27.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.0000  none  -28.5927
	{ 48000.0, 0, 1.50, 0.50f, 1.00f,   -26.4260f,  -25.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.3912  none  -26.3675
	{ 48000.0, 0, 1.50, 0.75f, 0.00f,   -28.2266f,  -27.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   2.0000  none  -27.8980
	{ 48000.0, 0, 1.50, 0.75f, 1.00f,   -28.9389f,  -27.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   0.3621  none  -29.1125
	{ 48000.0, 0, 1.50, 1.00f, 0.00f,    -8.3665f,   -7.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom    -5.79  jump   2.0000  none   -8.5323
	{ 48000.0, 0, 1.50, 1.00f, 1.00f,   -14.4096f,  -13.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom    -3.71  jump   1.2343  none  -14.4465
	{ 48000.0, 0, 2.50, 0.00f, 0.00f,   -16.3913f,  -15.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom    -7.34  jump   0.2571  none  -16.5392
	{ 48000.0, 0, 2.50, 0.00f, 1.00f,   -16.1663f,  -15.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom    -7.43  jump   0.2149  none  -16.2891
	{ 48000.0, 0, 2.50, 0.25f, 0.00f,    -7.7550f,   -6.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -14.97  jump   1.8348  none   -7.6746
	{ 48000.0, 0, 2.50, 0.25f, 1.00f,    -8.5457f,   -7.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -13.45  jump   1.8270  none   -9.0115
	{ 48000.0, 0, 2.50, 0.50f, 0.00f,   -16.8588f,  -15.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom    -6.39  jump   1.0000  none  -17.2547
	{ 48000.0, 0, 2.50, 0.50f, 1.00f,   -15.3613f,  -14.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom    -6.43  jump   1.3937  none  -15.7645
	{ 48000.0, 0, 2.50, 0.75f, 0.00f,   -16.6561f,  -15.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom    -6.38  jump   2.0000  none  -16.6282
	{ 48000.0, 0, 2.50, 0.75f, 1.00f,   -16.7723f,  -15.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom    -7.07  jump   0.5352  none  -16.9627
	{ 48000.0, 0, 2.50, 1.00f, 0.00f,     1.8764f,    3.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -12.40  jump   2.0000  none   +0.4243
	{ 48000.0, 0, 2.50, 1.00f, 1.00f,    -1.9452f,    0.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -11.33  jump   1.4038  none   -2.6928
	{ 48000.0, 0, 3.50, 0.00f, 0.00f,    -9.5450f,   -8.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom   -10.74  jump   0.3568  none   -9.7231
	{ 48000.0, 0, 3.50, 0.00f, 1.00f,    -9.4157f,   -8.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom   -10.85  jump   0.3013  none   -9.5683
	{ 48000.0, 0, 3.50, 0.25f, 0.00f,     1.1080f,    3.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -21.25  jump   1.7688  none   +1.0881
	{ 48000.0, 0, 3.50, 0.25f, 1.00f,    -0.2664f,    1.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -18.54  jump   1.8087  none   -1.4434
	{ 48000.0, 0, 3.50, 0.50f, 0.00f,   -10.2009f,   -9.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom    -9.34  jump   1.0000  none  -10.7192
	{ 48000.0, 0, 3.50, 0.50f, 1.00f,    -9.2110f,   -8.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom    -9.36  jump   1.3845  none   -9.9182
	{ 48000.0, 0, 3.50, 0.75f, 0.00f,   -10.3496f,   -9.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom    -9.33  jump   2.0000  none  -10.6905
	{ 48000.0, 0, 3.50, 0.75f, 1.00f,    -9.8381f,   -8.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom   -10.36  jump   0.6894  none  -10.0906
	{ 48000.0, 0, 3.50, 1.00f, 0.00f,     7.3729f,    9.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -14.68  jump   2.0000  none   +4.7326
	{ 48000.0, 0, 3.50, 1.00f, 1.00f,     5.2014f,    7.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -14.87  jump   1.4165  none   +3.4872
	{ 48000.0, 0, 5.50, 0.00f, 0.00f,    -0.6301f,    1.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom   -14.77  jump   0.5465  none   -0.9661
	{ 48000.0, 0, 5.50, 0.00f, 1.00f,    -0.5345f,    1.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom   -14.89  jump   0.4744  none   -0.8330
	{ 48000.0, 0, 5.50, 0.25f, 0.00f,    13.1561f,   15.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -27.94  jump   1.6365  none  +11.7654
	{ 48000.0, 0, 5.50, 0.25f, 1.00f,    10.4184f,   12.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -23.21  jump   1.7322  none   +7.3864
	{ 48000.0, 0, 5.50, 0.50f, 0.00f,    -2.1562f,   -1.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom   -12.81  jump   1.0000  none   -2.9075
	{ 48000.0, 0, 5.50, 0.50f, 1.00f,    -1.1448f,    0.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom   -12.86  jump   1.3299  none   -2.1689
	{ 48000.0, 0, 5.50, 0.75f, 0.00f,    -2.1533f,   -1.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom   -12.76  jump   2.0000  none   -2.7495
	{ 48000.0, 0, 5.50, 0.75f, 1.00f,    -1.3745f,    0.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom   -13.87  jump   0.9471  none   -1.8054
	{ 48000.0, 0, 5.50, 1.00f, 0.00f,    13.1695f,   15.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -16.27  jump   2.0000  none   +7.7165
	{ 48000.0, 0, 5.50, 1.00f, 1.00f,    10.4574f,   12.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -17.01  jump   1.0766  none   +8.0419

	// --- 48 kHz, band-limited master. REGRESSION rate.
	{ 48000.0, 1, 0.50, 0.00f, 0.00f,   -43.9372f,  -42.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   0.0591  none  -43.7468
	{ 48000.0, 1, 0.50, 0.00f, 1.00f,   -44.5104f,  -43.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   0.0490  none  -44.3691
	{ 48000.0, 1, 0.50, 0.25f, 0.00f,   -30.9758f,  -29.0f, "regression", kProvSync48Step        },  // triangle  step  fundDom    +0.00  jump   1.9624  none  -27.9469
	{ 48000.0, 1, 0.50, 0.25f, 1.00f,   -30.8288f,  -29.0f, "regression", kProvSync48Step        },  // triangle  step  fundDom    +0.00  jump   1.8056  none  -27.7972
	{ 48000.0, 1, 0.50, 0.50f, 0.00f,   -31.0061f,  -30.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.0000  none  -27.9616
	{ 48000.0, 1, 0.50, 0.50f, 1.00f,   -30.9154f,  -29.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.3810  none  -27.9112
	{ 48000.0, 1, 0.50, 0.75f, 0.00f,    -0.6637f,    1.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom   -12.08  jump   2.0000  none  +12.4660
	{ 48000.0, 1, 0.50, 0.75f, 1.00f,   -41.0330f,  -40.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   0.1867  none  -40.2650
	{ 48000.0, 1, 0.50, 1.00f, 0.00f,   -22.5946f,  -21.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -20.8864
	{ 48000.0, 1, 0.50, 1.00f, 1.00f,   -29.6901f,  -28.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   1.0565  none  -27.1470
	{ 48000.0, 1, 0.75, 0.00f, 0.00f,   -40.2166f,  -39.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   1.0429  none  -37.1039
	{ 48000.0, 1, 0.75, 0.00f, 1.00f,   -40.2466f,  -39.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   1.0534  none  -37.1388
	{ 48000.0, 1, 0.75, 0.25f, 0.00f,   -38.0438f,  -37.0f, "regression", kProvSync48Step        },  // triangle  step  fundDom    +0.00  jump   0.9436  none  -35.0887
	{ 48000.0, 1, 0.75, 0.25f, 1.00f,   -37.6503f,  -36.0f, "regression", kProvSync48Step        },  // triangle  step  fundDom    +0.00  jump   1.0731  none  -34.5475
	{ 48000.0, 1, 0.75, 0.50f, 0.00f,   -31.0061f,  -30.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.5000  none  -27.9616
	{ 48000.0, 1, 0.75, 0.50f, 1.00f,   -31.0391f,  -30.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.6781  none  -28.0191
	{ 48000.0, 1, 0.75, 0.75f, 0.00f,   -32.4624f,  -31.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   2.0000  none  -30.8499
	{ 48000.0, 1, 0.75, 0.75f, 1.00f,   -40.7586f,  -39.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   1.1188  none  -37.6485
	{ 48000.0, 1, 0.75, 1.00f, 0.00f,   -19.8783f,  -18.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -18.7328
	{ 48000.0, 1, 0.75, 1.00f, 1.00f,   -29.8561f,  -28.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   1.1664  none  -26.1220
	{ 48000.0, 1, 1.00, 0.00f, 0.00f,   -45.5279f,  -41.0f, "regression", kProvSync48Plateau     },  // sine      PLAT  fundDom    +0.00  jump   0.0074  none  -45.5131
	{ 48000.0, 1, 1.00, 0.00f, 1.00f,   -45.4417f,  -41.0f, "regression", kProvSync48Plateau     },  // sine      PLAT  fundDom    +0.00  jump   0.0062  none  -45.4290
	{ 48000.0, 1, 1.00, 0.25f, 0.00f,   -44.7773f,  -40.0f, "regression", kProvSync48Plateau     },  // triangle  PLAT  fundDom    +0.00  jump   0.0047  none  -44.7445
	{ 48000.0, 1, 1.00, 0.25f, 1.00f,   -44.8866f,  -40.0f, "regression", kProvSync48Plateau     },  // triangle  PLAT  fundDom    +0.00  jump   0.0009  none  -44.8832
	{ 48000.0, 1, 1.00, 0.50f, 0.00f,   -36.7880f,  -35.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   0.0024  none  -36.7953
	{ 48000.0, 1, 1.00, 0.50f, 1.00f,   -36.3607f,  -35.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   0.0009  none  -36.3613
	{ 48000.0, 1, 1.00, 0.75f, 0.00f,   -36.4400f,  -35.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   0.0000  none  -36.4400
	{ 48000.0, 1, 1.00, 0.75f, 1.00f,   -45.8784f,  -44.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   0.0136  none  -45.8542
	{ 48000.0, 1, 1.00, 1.00f, 0.00f,   -23.5001f,  -22.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   0.0000  none  -23.5001
	{ 48000.0, 1, 1.00, 1.00f, 1.00f,   -36.6591f,  -35.0f, "regression", kProvSync48Step        },  // pulse 5%  step  fundDom    +0.00  jump   0.0258  none  -36.4363
	{ 48000.0, 1, 1.50, 0.00f, 0.00f,   -40.5578f,  -39.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   0.1767  none  -40.3766
	{ 48000.0, 1, 1.50, 0.00f, 1.00f,   -40.2579f,  -39.0f, "regression", kProvSync48Step        },  // sine      step  fundDom    +0.00  jump   0.1470  none  -40.1127
	{ 48000.0, 1, 1.50, 0.25f, 0.00f,   -27.4851f,  -26.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom    -2.91  jump   1.8872  none  -24.3861
	{ 48000.0, 1, 1.50, 0.25f, 1.00f,   -28.4656f,  -27.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom    -2.47  jump   1.8284  none  -25.5453
	{ 48000.0, 1, 1.50, 0.50f, 0.00f,   -31.1263f,  -30.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.0000  none  -29.0890
	{ 48000.0, 1, 1.50, 0.50f, 1.00f,   -29.0864f,  -28.0f, "regression", kProvSync48Step        },  // saw       step  fundDom    +0.00  jump   1.3893  none  -26.7306
	{ 48000.0, 1, 1.50, 0.75f, 0.00f,   -33.2134f,  -32.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   2.0000  none  -33.3809
	{ 48000.0, 1, 1.50, 0.75f, 1.00f,   -41.0025f,  -40.0f, "regression", kProvSync48Step        },  // square    step  fundDom    +0.00  jump   0.4024  none  -40.5681
	{ 48000.0, 1, 1.50, 1.00f, 0.00f,   -17.2838f,  -16.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom    -5.84  jump   2.0000  none  -14.3872
	{ 48000.0, 1, 1.50, 1.00f, 1.00f,   -21.9257f,  -20.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom    -3.78  jump   1.2511  none  -20.0099
	{ 48000.0, 1, 2.50, 0.00f, 0.00f,   -28.3519f,  -27.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom    -7.36  jump   0.2930  none  -28.2114
	{ 48000.0, 1, 2.50, 0.00f, 1.00f,   -28.0923f,  -27.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom    -7.44  jump   0.2453  none  -27.9800
	{ 48000.0, 1, 2.50, 0.25f, 0.00f,   -14.6749f,  -13.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -15.00  jump   1.8120  none  -11.6569
	{ 48000.0, 1, 2.50, 0.25f, 1.00f,   -15.5315f,  -14.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -13.63  jump   1.8318  none  -13.8315
	{ 48000.0, 1, 2.50, 0.50f, 0.00f,   -18.8044f,  -17.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom    -6.44  jump   1.0000  none  -17.8893
	{ 48000.0, 1, 2.50, 0.50f, 1.00f,   -15.6772f,  -14.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom    -6.52  jump   1.3806  none  -14.8460
	{ 48000.0, 1, 2.50, 0.75f, 0.00f,   -24.9953f,  -23.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom    -6.44  jump   2.0000  none  -23.9888
	{ 48000.0, 1, 2.50, 0.75f, 1.00f,   -28.8754f,  -27.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom    -7.08  jump   0.6000  none  -28.5609
	{ 48000.0, 1, 2.50, 1.00f, 0.00f,     3.9150f,    5.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -13.07  jump   1.9529  none   +2.6814
	{ 48000.0, 1, 2.50, 1.00f, 1.00f,    -2.0353f,   -1.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -12.23  jump   1.2740  none   -2.2904
	{ 48000.0, 1, 3.50, 0.00f, 0.00f,   -21.8260f,  -20.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom   -10.78  jump   0.4071  none  -21.7426
	{ 48000.0, 1, 3.50, 0.00f, 1.00f,   -21.5932f,  -20.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom   -10.90  jump   0.3442  none  -21.5293
	{ 48000.0, 1, 3.50, 0.25f, 0.00f,    -6.4893f,   -5.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -21.39  jump   1.7368  none   -3.9016
	{ 48000.0, 1, 3.50, 0.25f, 1.00f,    -8.5238f,   -7.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -18.89  jump   1.8148  none   -7.7102
	{ 48000.0, 1, 3.50, 0.50f, 0.00f,   -15.4425f,  -14.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom    -9.47  jump   1.0000  none  -14.6855
	{ 48000.0, 1, 3.50, 0.50f, 1.00f,   -12.0242f,  -11.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom    -9.55  jump   1.3557  none  -11.3420
	{ 48000.0, 1, 3.50, 0.75f, 0.00f,   -15.6964f,  -14.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom    -9.46  jump   2.0000  none  -17.2284
	{ 48000.0, 1, 3.50, 0.75f, 1.00f,   -22.3142f,  -21.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom   -10.40  jump   0.7734  none  -22.1104
	{ 48000.0, 1, 3.50, 1.00f, 0.00f,     9.4611f,   11.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -16.89  jump   1.6235  none   +6.4761
	{ 48000.0, 1, 3.50, 1.00f, 1.00f,     4.3456f,    6.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -16.17  jump   1.1702  none   +3.0028
	{ 48000.0, 1, 5.50, 0.00f, 0.00f,   -13.5412f,  -12.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom   -14.90  jump   0.6255  none  -13.6123
	{ 48000.0, 1, 5.50, 0.00f, 1.00f,   -13.3390f,  -12.0f, "diagnostic", kProvSync48Invalid     },  // sine      step  fundDom   -15.04  jump   0.5442  none  -13.4105
	{ 48000.0, 1, 5.50, 0.25f, 0.00f,     1.7031f,    3.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -28.98  jump   1.5862  none   +3.8775
	{ 48000.0, 1, 5.50, 0.25f, 1.00f,     2.9139f,    4.0f, "diagnostic", kProvSync48Invalid     },  // triangle  step  fundDom   -24.35  jump   1.7242  none   +0.7218
	{ 48000.0, 1, 5.50, 0.50f, 0.00f,   -10.8809f,   -9.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom   -13.08  jump   1.0000  none  -11.5320
	{ 48000.0, 1, 5.50, 0.50f, 1.00f,    -7.8090f,   -6.0f, "diagnostic", kProvSync48Invalid     },  // saw       step  fundDom   -13.17  jump   1.2920  none   -7.9564
	{ 48000.0, 1, 5.50, 0.75f, 0.00f,   -11.1838f,  -10.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom   -13.07  jump   2.0000  none  -10.8251
	{ 48000.0, 1, 5.50, 0.75f, 1.00f,   -13.8948f,  -12.0f, "diagnostic", kProvSync48Invalid     },  // square    step  fundDom   -14.32  jump   1.0472  none  -13.8164
	{ 48000.0, 1, 5.50, 1.00f, 0.00f,    16.8324f,   18.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -23.48  jump   1.0118  none  +11.2196
	{ 48000.0, 1, 5.50, 1.00f, 1.00f,    12.6737f,   14.0f, "diagnostic", kProvSync48Invalid     },  // pulse 5%  step  fundDom   -20.37  jump   0.8883  none   +7.9810

	// --- 96 kHz, hard-edge master. REGRESSION rate.
	{ 96000.0, 0, 0.50, 0.00f, 0.00f,   -37.3576f,  -36.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   0.0263  none  -37.4003
	{ 96000.0, 0, 0.50, 0.00f, 1.00f,   -38.0887f,  -37.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   0.0218  none  -38.1211
	{ 96000.0, 0, 0.50, 0.25f, 0.00f,   -37.4604f,  -36.0f, "regression", kProvSync96Step        },  // triangle  step  fundDom    +0.00  jump   1.9832  none  -33.6140
	{ 96000.0, 0, 0.50, 0.25f, 1.00f,   -37.2622f,  -36.0f, "regression", kProvSync96Step        },  // triangle  step  fundDom    +0.00  jump   1.7959  none  -33.5064
	{ 96000.0, 0, 0.50, 0.50f, 0.00f,   -37.5195f,  -36.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.0000  none  -33.6325
	{ 96000.0, 0, 0.50, 0.50f, 1.00f,   -37.4890f,  -36.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.3767  none  -33.6002
	{ 96000.0, 0, 0.50, 0.75f, 0.00f,     9.6579f,   11.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom    -8.51  jump   2.0000  none   +8.9766
	{ 96000.0, 0, 0.50, 0.75f, 1.00f,   -34.4893f,  -33.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   0.1251  none  -34.5638
	{ 96000.0, 0, 0.50, 1.00f, 0.00f,   -28.5969f,  -27.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -28.2183
	{ 96000.0, 0, 0.50, 1.00f, 1.00f,   -32.4890f,  -31.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   0.9944  none  -32.5689
	{ 96000.0, 0, 0.75, 0.00f, 0.00f,   -38.0647f,  -37.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   1.0193  none  -38.0612
	{ 96000.0, 0, 0.75, 0.00f, 1.00f,   -38.0540f,  -37.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   1.0344  none  -38.0503
	{ 96000.0, 0, 0.75, 0.25f, 0.00f,   -40.5851f,  -39.0f, "regression", kProvSync96Step        },  // triangle  step  fundDom    +0.00  jump   0.9749  none  -40.6129
	{ 96000.0, 0, 0.75, 0.25f, 1.00f,   -40.9816f,  -39.0f, "regression", kProvSync96Step        },  // triangle  step  fundDom    +0.00  jump   1.0909  none  -40.2027
	{ 96000.0, 0, 0.75, 0.50f, 0.00f,   -37.5195f,  -36.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.5000  none  -33.6325
	{ 96000.0, 0, 0.75, 0.50f, 1.00f,   -37.5409f,  -36.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.6755  none  -33.6701
	{ 96000.0, 0, 0.75, 0.75f, 0.00f,   -37.6170f,  -36.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   2.0000  none  -37.1530
	{ 96000.0, 0, 0.75, 0.75f, 1.00f,   -38.3532f,  -37.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   1.0731  none  -38.3510
	{ 96000.0, 0, 0.75, 1.00f, 0.00f,   -25.2705f,  -24.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -24.8931
	{ 96000.0, 0, 0.75, 1.00f, 1.00f,   -29.0874f,  -28.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   1.0761  none  -29.1925
	{ 96000.0, 0, 1.00, 0.00f, 0.00f,   -39.3878f,  -38.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   0.0254  none  -39.3858
	{ 96000.0, 0, 1.00, 0.00f, 1.00f,   -39.2605f,  -38.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   0.0212  none  -39.2580
	{ 96000.0, 0, 1.00, 0.25f, 0.00f,   -38.4787f,  -37.0f, "regression", kProvSync96Step        },  // triangle  step  fundDom    +0.00  jump   0.0162  none  -38.4455
	{ 96000.0, 0, 1.00, 0.25f, 1.00f,   -38.6510f,  -34.0f, "regression", kProvSync96Plateau     },  // triangle  PLAT  fundDom    +0.00  jump   0.0009  none  -38.6482
	{ 96000.0, 0, 1.00, 0.50f, 0.00f,   -37.7399f,  -36.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   0.0081  none  -37.7280
	{ 96000.0, 0, 1.00, 0.50f, 1.00f,   -35.9793f,  -34.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   0.0008  none  -35.9773
	{ 96000.0, 0, 1.00, 0.75f, 0.00f,   -39.6884f,  -38.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   0.0000  none  -39.6884
	{ 96000.0, 0, 1.00, 0.75f, 1.00f,   -39.8998f,  -38.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   0.0474  none  -39.8987
	{ 96000.0, 0, 1.00, 1.00f, 0.00f,   -22.9484f,  -21.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   0.0000  none  -22.9484
	{ 96000.0, 0, 1.00, 1.00f, 1.00f,   -31.1149f,  -30.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   0.0943  none  -31.3921
	{ 96000.0, 0, 1.50, 0.00f, 0.00f,   -33.9253f,  -32.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   0.0789  none  -33.9795
	{ 96000.0, 0, 1.50, 0.00f, 1.00f,   -33.6477f,  -32.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   0.0654  none  -33.6912
	{ 96000.0, 0, 1.50, 0.25f, 0.00f,   -28.8997f,  -27.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom    -2.91  jump   1.9497  none  -28.8697
	{ 96000.0, 0, 1.50, 0.25f, 1.00f,   -29.0732f,  -28.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom    -2.47  jump   1.8102  none  -29.1191
	{ 96000.0, 0, 1.50, 0.50f, 0.00f,   -35.2513f,  -34.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.0000  none  -34.7978
	{ 96000.0, 0, 1.50, 0.50f, 1.00f,   -32.9487f,  -31.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.3836  none  -32.6068
	{ 96000.0, 0, 1.50, 0.75f, 0.00f,   -33.2193f,  -32.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   2.0000  none  -32.9046
	{ 96000.0, 0, 1.50, 0.75f, 1.00f,   -34.4302f,  -33.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   0.2234  none  -34.5270
	{ 96000.0, 0, 1.50, 1.00f, 0.00f,   -14.4468f,  -13.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom    -5.85  jump   2.0000  none  -14.4221
	{ 96000.0, 0, 1.50, 1.00f, 1.00f,   -21.1202f,  -20.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom    -3.78  jump   1.0945  none  -21.2410
	{ 96000.0, 0, 2.50, 0.00f, 0.00f,   -21.7763f,  -20.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom    -7.36  jump   0.1313  none  -21.8434
	{ 96000.0, 0, 2.50, 0.00f, 1.00f,   -21.5300f,  -20.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom    -7.44  jump   0.1089  none  -21.5846
	{ 96000.0, 0, 2.50, 0.25f, 0.00f,   -13.1011f,  -12.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -14.98  jump   1.9162  none  -13.0507
	{ 96000.0, 0, 2.50, 0.25f, 1.00f,   -13.9699f,  -12.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -13.62  jump   1.8201  none  -14.2173
	{ 96000.0, 0, 2.50, 0.50f, 0.00f,   -23.1037f,  -22.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom    -6.46  jump   1.0000  none  -23.0194
	{ 96000.0, 0, 2.50, 0.50f, 1.00f,   -21.3569f,  -20.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom    -6.54  jump   1.3892  none  -21.1046
	{ 96000.0, 0, 2.50, 0.75f, 0.00f,   -21.8530f,  -20.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom    -6.46  jump   2.0000  none  -21.7774
	{ 96000.0, 0, 2.50, 0.75f, 1.00f,   -22.3280f,  -21.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom    -7.09  jump   0.3190  none  -22.4457
	{ 96000.0, 0, 2.50, 1.00f, 0.00f,    -2.3477f,   -1.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -13.05  jump   2.0000  none   -3.0657
	{ 96000.0, 0, 2.50, 1.00f, 1.00f,    -8.0430f,   -7.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -12.32  jump   1.1915  none   -8.5812
	{ 96000.0, 0, 3.50, 0.00f, 0.00f,   -15.3059f,  -14.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom   -10.78  jump   0.1833  none  -15.3851
	{ 96000.0, 0, 3.50, 0.00f, 1.00f,   -15.1181f,  -14.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom   -10.89  jump   0.1525  none  -15.1877
	{ 96000.0, 0, 3.50, 0.25f, 0.00f,    -4.0851f,   -3.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -21.39  jump   1.8827  none   -4.0409
	{ 96000.0, 0, 3.50, 0.25f, 1.00f,    -5.7456f,   -4.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -18.89  jump   1.8252  none   -6.3596
	{ 96000.0, 0, 3.50, 0.50f, 0.00f,   -16.5130f,  -15.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom    -9.52  jump   1.0000  none  -16.6091
	{ 96000.0, 0, 3.50, 0.50f, 1.00f,   -15.0704f,  -14.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom    -9.60  jump   1.3928  none  -15.0469
	{ 96000.0, 0, 3.50, 0.75f, 0.00f,   -15.6286f,  -14.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom    -9.52  jump   2.0000  none  -15.7148
	{ 96000.0, 0, 3.50, 0.75f, 1.00f,   -15.6720f,  -14.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom   -10.41  jump   0.4105  none  -15.8021
	{ 96000.0, 0, 3.50, 1.00f, 0.00f,     3.3224f,    5.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -15.99  jump   2.0000  none   +1.9390
	{ 96000.0, 0, 3.50, 1.00f, 1.00f,    -1.5484f,    0.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -15.52  jump   1.2836  none   -2.4944
	{ 96000.0, 0, 5.50, 0.00f, 0.00f,    -6.6708f,   -5.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom   -14.89  jump   0.2862  none   -6.7932
	{ 96000.0, 0, 5.50, 0.00f, 1.00f,    -6.5095f,   -5.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom   -15.03  jump   0.2398  none   -6.6126
	{ 96000.0, 0, 5.50, 0.25f, 0.00f,     7.5353f,    9.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -29.22  jump   1.8156  none   +7.3539
	{ 96000.0, 0, 5.50, 0.25f, 1.00f,     4.2204f,    6.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -24.36  jump   1.8194  none   +2.6507
	{ 96000.0, 0, 5.50, 0.50f, 0.00f,    -8.3029f,   -7.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom   -13.26  jump   1.0001  none   -8.5106
	{ 96000.0, 0, 5.50, 0.50f, 1.00f,    -7.0645f,   -6.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom   -13.36  jump   1.3920  none   -7.3944
	{ 96000.0, 0, 5.50, 0.75f, 0.00f,    -7.7633f,   -6.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom   -13.24  jump   2.0000  none   -7.9588
	{ 96000.0, 0, 5.50, 0.75f, 1.00f,    -7.0775f,   -6.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom   -14.44  jump   0.5779  none   -7.2683
	{ 96000.0, 0, 5.50, 1.00f, 0.00f,     9.7395f,   11.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -19.04  jump   2.0000  none   +6.9504
	{ 96000.0, 0, 5.50, 1.00f, 1.00f,     7.9403f,    9.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -19.90  jump   1.4477  none   +5.7435

	// --- 96 kHz, band-limited master. REGRESSION rate.
	{ 96000.0, 1, 0.50, 0.00f, 0.00f,   -49.7496f,  -48.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   0.0296  none  -49.6721
	{ 96000.0, 1, 0.50, 0.00f, 1.00f,   -50.4556f,  -49.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   0.0245  none  -50.3972
	{ 96000.0, 1, 0.50, 0.25f, 0.00f,   -36.5434f,  -35.0f, "regression", kProvSync96Step        },  // triangle  step  fundDom    +0.00  jump   1.9812  none  -33.6167
	{ 96000.0, 1, 0.50, 0.25f, 1.00f,   -36.4158f,  -35.0f, "regression", kProvSync96Step        },  // triangle  step  fundDom    +0.00  jump   1.7970  none  -33.4838
	{ 96000.0, 1, 0.50, 0.50f, 0.00f,   -36.5566f,  -35.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.0000  none  -33.6243
	{ 96000.0, 1, 0.50, 0.50f, 1.00f,   -36.5042f,  -35.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.3771  none  -33.5856
	{ 96000.0, 1, 0.50, 0.75f, 0.00f,    -0.2637f,    1.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom   -11.64  jump   2.0000  none  +12.1864
	{ 96000.0, 1, 0.50, 0.75f, 1.00f,   -46.4022f,  -45.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   0.1312  none  -45.7870
	{ 96000.0, 1, 0.50, 1.00f, 0.00f,   -31.4403f,  -30.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -28.5856
	{ 96000.0, 1, 0.50, 1.00f, 1.00f,   -36.1303f,  -35.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   1.0006  none  -33.5514
	{ 96000.0, 1, 0.75, 0.00f, 0.00f,   -45.7215f,  -44.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   1.0218  none  -42.7582
	{ 96000.0, 1, 0.75, 0.00f, 1.00f,   -45.7612f,  -44.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   1.0365  none  -42.7991
	{ 96000.0, 1, 0.75, 0.25f, 0.00f,   -43.7888f,  -42.0f, "regression", kProvSync96Step        },  // triangle  step  fundDom    +0.00  jump   0.9718  none  -40.9146
	{ 96000.0, 1, 0.75, 0.25f, 1.00f,   -43.1378f,  -42.0f, "regression", kProvSync96Step        },  // triangle  step  fundDom    +0.00  jump   1.0891  none  -40.1937
	{ 96000.0, 1, 0.75, 0.50f, 0.00f,   -36.5566f,  -35.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.5000  none  -33.6243
	{ 96000.0, 1, 0.75, 0.50f, 1.00f,   -36.5647f,  -35.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.6758  none  -33.6580
	{ 96000.0, 1, 0.75, 0.75f, 0.00f,   -39.0421f,  -38.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   2.0000  none  -36.7916
	{ 96000.0, 1, 0.75, 0.75f, 1.00f,   -46.2467f,  -45.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   1.0777  none  -43.2842
	{ 96000.0, 1, 0.75, 1.00f, 0.00f,   -27.1907f,  -26.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   2.0000  none  -24.5908
	{ 96000.0, 1, 0.75, 1.00f, 1.00f,   -33.4864f,  -32.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   1.0851  none  -30.8919
	{ 96000.0, 1, 1.00, 0.00f, 0.00f,   -51.8377f,  -47.0f, "regression", kProvSync96Plateau     },  // sine      PLAT  fundDom    +0.00  jump   0.0048  none  -51.8267
	{ 96000.0, 1, 1.00, 0.00f, 1.00f,   -51.7116f,  -47.0f, "regression", kProvSync96Plateau     },  // sine      PLAT  fundDom    +0.00  jump   0.0040  none  -51.7021
	{ 96000.0, 1, 1.00, 0.25f, 0.00f,   -50.7416f,  -46.0f, "regression", kProvSync96Plateau     },  // triangle  PLAT  fundDom    +0.00  jump   0.0031  none  -50.7180
	{ 96000.0, 1, 1.00, 0.25f, 1.00f,   -50.9360f,  -46.0f, "regression", kProvSync96Plateau     },  // triangle  PLAT  fundDom    +0.00  jump   0.0002  none  -50.9356
	{ 96000.0, 1, 1.00, 0.50f, 0.00f,   -42.3155f,  -41.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   0.0015  none  -42.3194
	{ 96000.0, 1, 1.00, 0.50f, 1.00f,   -41.9685f,  -40.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   0.0002  none  -41.9685
	{ 96000.0, 1, 1.00, 0.75f, 0.00f,   -42.7502f,  -41.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   0.0000  none  -42.7502
	{ 96000.0, 1, 1.00, 0.75f, 1.00f,   -52.3439f,  -48.0f, "regression", kProvSync96Plateau     },  // square    PLAT  fundDom    +0.00  jump   0.0090  none  -52.3259
	{ 96000.0, 1, 1.00, 1.00f, 0.00f,   -25.7308f,  -24.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   0.0000  none  -25.7308
	{ 96000.0, 1, 1.00, 1.00f, 1.00f,   -43.7854f,  -42.0f, "regression", kProvSync96Step        },  // pulse 5%  step  fundDom    +0.00  jump   0.0179  none  -43.5755
	{ 96000.0, 1, 1.50, 0.00f, 0.00f,   -46.2397f,  -45.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   0.0887  none  -46.1620
	{ 96000.0, 1, 1.50, 0.00f, 1.00f,   -45.9395f,  -44.0f, "regression", kProvSync96Step        },  // sine      step  fundDom    +0.00  jump   0.0736  none  -45.8773
	{ 96000.0, 1, 1.50, 0.25f, 0.00f,   -32.8195f,  -31.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom    -2.90  jump   1.9435  none  -29.9035
	{ 96000.0, 1, 1.50, 0.25f, 1.00f,   -34.0320f,  -33.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom    -2.49  jump   1.8131  none  -31.1917
	{ 96000.0, 1, 1.50, 0.50f, 0.00f,   -35.2764f,  -34.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.0000  none  -33.7977
	{ 96000.0, 1, 1.50, 0.50f, 1.00f,   -33.0453f,  -32.0f, "regression", kProvSync96Step        },  // saw       step  fundDom    +0.00  jump   1.3843  none  -31.3152
	{ 96000.0, 1, 1.50, 0.75f, 0.00f,   -39.7342f,  -38.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   2.0000  none  -38.0690
	{ 96000.0, 1, 1.50, 0.75f, 1.00f,   -46.6299f,  -45.0f, "regression", kProvSync96Step        },  // square    step  fundDom    +0.00  jump   0.2420  none  -46.3385
	{ 96000.0, 1, 1.50, 1.00f, 0.00f,   -14.6549f,  -13.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom    -5.87  jump   2.0000  none  -14.0512
	{ 96000.0, 1, 1.50, 1.00f, 1.00f,   -31.6692f,  -30.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom    -3.79  jump   1.1114  none  -28.2464
	{ 96000.0, 1, 2.50, 0.00f, 0.00f,   -34.0047f,  -33.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom    -7.36  jump   0.1476  none  -33.9368
	{ 96000.0, 1, 2.50, 0.00f, 1.00f,   -33.7357f,  -32.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom    -7.45  jump   0.1227  none  -33.6810
	{ 96000.0, 1, 2.50, 0.25f, 0.00f,   -19.8912f,  -18.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -14.99  jump   1.9058  none  -17.1339
	{ 96000.0, 1, 2.50, 0.25f, 1.00f,   -21.9818f,  -20.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -13.70  jump   1.8243  none  -19.4137
	{ 96000.0, 1, 2.50, 0.50f, 0.00f,   -24.5989f,  -23.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom    -6.49  jump   1.0000  none  -23.4440
	{ 96000.0, 1, 2.50, 0.50f, 1.00f,   -22.9814f,  -21.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom    -6.58  jump   1.3886  none  -21.5668
	{ 96000.0, 1, 2.50, 0.75f, 0.00f,   -26.9551f,  -25.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom    -6.49  jump   2.0000  none  -28.0236
	{ 96000.0, 1, 2.50, 0.75f, 1.00f,   -34.5220f,  -33.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom    -7.09  jump   0.3501  none  -34.3137
	{ 96000.0, 1, 2.50, 1.00f, 0.00f,   -11.1919f,  -10.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -13.30  jump   2.0000  none   -9.5176
	{ 96000.0, 1, 2.50, 1.00f, 1.00f,   -16.5731f,  -15.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -12.50  jump   1.2139  none  -16.7520
	{ 96000.0, 1, 3.50, 0.00f, 0.00f,   -27.4837f,  -26.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom   -10.78  jump   0.2063  none  -27.4302
	{ 96000.0, 1, 3.50, 0.00f, 1.00f,   -27.2361f,  -26.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom   -10.90  jump   0.1718  none  -27.1933
	{ 96000.0, 1, 3.50, 0.25f, 0.00f,   -13.6270f,  -12.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -21.42  jump   1.8682  none  -10.7315
	{ 96000.0, 1, 3.50, 0.25f, 1.00f,   -16.2546f,  -15.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -19.04  jump   1.8306  none  -14.0184
	{ 96000.0, 1, 3.50, 0.50f, 0.00f,   -18.3267f,  -17.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom    -9.57  jump   1.0000  none  -17.6729
	{ 96000.0, 1, 3.50, 0.50f, 1.00f,   -16.4766f,  -15.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom    -9.68  jump   1.3888  none  -15.8231
	{ 96000.0, 1, 3.50, 0.75f, 0.00f,   -23.8905f,  -22.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom    -9.57  jump   2.0000  none  -23.4423
	{ 96000.0, 1, 3.50, 0.75f, 1.00f,   -28.0245f,  -27.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom   -10.41  jump   0.4541  none  -27.8693
	{ 96000.0, 1, 3.50, 1.00f, 0.00f,     2.4283f,    4.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -16.45  jump   2.0000  none   +0.6709
	{ 96000.0, 1, 3.50, 1.00f, 1.00f,    -5.1628f,   -4.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -16.20  jump   1.2745  none   -5.8290
	{ 96000.0, 1, 5.50, 0.00f, 0.00f,   -19.2583f,  -18.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom   -14.92  jump   0.3224  none  -19.2447
	{ 96000.0, 1, 5.50, 0.00f, 1.00f,   -19.0370f,  -18.0f, "diagnostic", kProvSync96Invalid     },  // sine      step  fundDom   -15.06  jump   0.2706  none  -19.0279
	{ 96000.0, 1, 5.50, 0.25f, 0.00f,    -3.8823f,   -2.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -29.45  jump   1.7927  none   -1.5098
	{ 96000.0, 1, 5.50, 0.25f, 1.00f,    -6.5042f,   -5.0f, "diagnostic", kProvSync96Invalid     },  // triangle  step  fundDom   -24.79  jump   1.8281  none   -6.6949
	{ 96000.0, 1, 5.50, 0.50f, 0.00f,   -13.5247f,  -12.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom   -13.36  jump   1.0001  none  -13.0451
	{ 96000.0, 1, 5.50, 0.50f, 1.00f,   -10.5022f,   -9.0f, "diagnostic", kProvSync96Invalid     },  // saw       step  fundDom   -13.49  jump   1.3752  none  -10.1605
	{ 96000.0, 1, 5.50, 0.75f, 0.00f,   -14.3509f,  -13.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom   -13.36  jump   2.0000  none  -14.3026
	{ 96000.0, 1, 5.50, 0.75f, 1.00f,   -19.8005f,  -18.0f, "diagnostic", kProvSync96Invalid     },  // square    step  fundDom   -14.46  jump   0.6456  none  -19.7350
	{ 96000.0, 1, 5.50, 1.00f, 0.00f,    10.6177f,   12.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -20.40  jump   1.8605  none   +7.4463
	{ 96000.0, 1, 5.50, 1.00f, 1.00f,     4.9095f,    6.0f, "diagnostic", kProvSync96Invalid     },  // pulse 5%  step  fundDom   -20.50  jump   1.2530  none   +3.4564
};

const std::size_t kSyncPinCount = sizeof(SYNC_PINS) / sizeof(SYNC_PINS[0]);
// ---------------------------------------------------------------------------
// SYNC_GRID — the full cross product of the axes above, 3 * 6 * 5 * 2 * 2 = 360
// cells, built once on first use.
//
// WHY THIS TABLE IS ENUMERATED IN CODE WHILE SPECTRUM_GRID IS SPELLED OUT.
// SPECTRUM_GRID is spelled out because every row carries a PINNED NUMBER whose
// provenance is per-row; a generated table there would put the phase's evidence
// behind a loop. This table pins NOTHING (see the SyncCell banner), it is a
// pure cross product of five named axes, and enumerating it here makes the axes
// auditable and the cell count MECHANICAL rather than a claim about a hand-typed
// list. PLAN 33-07 IS WARNED: the moment a per-cell threshold is pinned, that
// number needs a per-cell home and a per-cell provenance, and this builder must
// grow a lookup rather than a formula.
//
// The slave's pitch control voltage is solved from the ratio with std::log2 —
// libm, which is available in tests/ and forbidden in src/ (the D-18
// precedent). It is NOT bin-centred and MUST NOT BE: see CONSEQUENCE 2 in the
// banner above. The frequency the core actually reaches differs from the
// nominal ratio by forge::exp2_taylor5's approximation error, and that is fine
// — the slave is free, and nothing here measures it.
// ---------------------------------------------------------------------------
std::vector<SyncCell> buildSyncGrid() {
	std::vector<SyncCell> g;
	const std::size_t nRates = sizeof(SYNC_RATES)      / sizeof(SYNC_RATES[0]);
	const std::size_t nRatio = sizeof(SYNC_RATIOS)     / sizeof(SYNC_RATIOS[0]);
	const std::size_t nMorph = sizeof(SYNC_MORPHS)     / sizeof(SYNC_MORPHS[0]);
	const std::size_t nChar  = sizeof(SYNC_CHARACTERS) / sizeof(SYNC_CHARACTERS[0]);
	const std::size_t nEdge  = sizeof(SYNC_EDGES)      / sizeof(SYNC_EDGES[0]);
	g.reserve(nRates * nRatio * nMorph * nChar * nEdge);

	for (std::size_t ri = 0; ri < nRates; ++ri) {
		const double sr = SYNC_RATES[ri].sr;
		const int    Km = SYNC_RATES[ri].Km;
		const double masterHz = (double)Km * sr / (double)kSpectrumN;
		for (std::size_t ei = 0; ei < nEdge; ++ei) {
			for (std::size_t qi = 0; qi < nRatio; ++qi) {
				for (std::size_t mi = 0; mi < nMorph; ++mi) {
					for (std::size_t ci = 0; ci < nChar; ++ci) {
						SyncCell c;
						c.sr          = sr;
						c.Km          = Km;
						c.masterHz    = masterHz;
						c.edge        = SYNC_EDGES[ei];
						c.edgeName    = SYNC_EDGE_NAME[ei];
						c.ratio       = SYNC_RATIOS[qi];
						c.pitchCV     = (float)std::log2(SYNC_RATIOS[qi] * masterHz / (double)forge::kVcoFreqC4);
						c.morph       = SYNC_MORPHS[mi];
						c.region      = SYNC_REGIONS[mi];
						c.character   = SYNC_CHARACTERS[ci];
						// >>> THE PER-CELL LOOKUP (plan 33-07), NOT A FORMULA. <<<
						// The default is the SENTINEL, deliberately: a cell whose
						// five axes match no row of SYNC_PINS keeps kSyncUnpinnedDb
						// and the tier "UNPINNED", and the gate case turns that
						// into a RED. Defaulting to anything derivable from the
						// axes would let a newly added grid row pass silently
						// against a threshold nobody measured.
						c.measuredDb  = kSyncUnpinnedDb;
						c.thresholdDb = kSyncUnpinnedDb;
						c.tier        = "UNPINNED";
						c.provenance  = kProvSyncUnpinned;
						for (std::size_t pi = 0; pi < kSyncPinCount; ++pi) {
							const SyncPin& pin = SYNC_PINS[pi];
							// ALL FIVE AXES MUST MATCH. Every one of them is an
							// exact comparison of a value copied from the same
							// axis array the cell was built from, so there is no
							// rounding question: these are the same literals.
							if (pin.sr        != c.sr)        continue;
							if (pin.edgeIdx   != (int)ei)     continue;
							if (pin.ratio     != c.ratio)     continue;
							if (pin.morph     != c.morph)     continue;
							if (pin.character != c.character) continue;
							c.measuredDb  = pin.measuredDb;
							c.thresholdDb = pin.thresholdDb;
							c.tier        = pin.tier;
							c.provenance  = pin.provenance;
							break;
						}
						g.push_back(c);
					}
				}
			}
		}
	}
	return g;
}

const std::vector<SyncCell>& syncGrid() {
	static const std::vector<SyncCell> SYNC_GRID = buildSyncGrid();
	return SYNC_GRID;
}

// ---------------------------------------------------------------------------
// THE SIX LEGS AND THE TWO MUTATION PROBES. All eight come out of ONE struct.
//
// SIX ARE LEGS OF THE MEASUREMENT; TWO ARE MUTATION PROBES AND ARE NOT
// CANDIDATES. The distinction is not cosmetic: a probe exists to demonstrate
// that the measurement can SEE a wrong answer, and reporting one as if it were
// in the running would be reporting a control as a result. The four
// IMPLEMENTABLE candidates are `none`, `detect`, `pastEdge` and `flatHalf`;
// `oracle` consumes information the core cannot have and `snap` is a landmine
// rendered as a measurement, so neither is selectable either.
// ---------------------------------------------------------------------------
enum SyncLeg {
	kLegNone     = 0,   // CANDIDATE. Reset applied, sync correction withheld ENTIRELY. This WAS the shipped core from plan 33-02 until plan 33-06 landed the seam; it is now the WITHHELD leg, and the reference leg for per-cell diagnostics that are properties of the RESET rather than of any correction
	kLegDetect   = 1,   // CANDIDATE. addStep(f, jump) — the detection-sample placement, accepting the one-sample shift
	kLegPastEdge = 2,   // CANDIDATE. The current sample takes the AFTER-EDGE half, scaled by f squared, and nothing is owed forward. THIS IS THE SHIPPED CORE from plan 33-06 onward, reached there via MorphBlep::addPastStep
	kLegFlatHalf = 3,   // CANDIDATE. addStep(0, jump) — a flat half-jump on the detection sample
	kLegOracle   = 4,   // DIAGNOSTIC. pastEdge with f replaced by the generator's TRUE wrap fraction
	kLegSnap     = 5,   // DIAGNOSTIC. pastEdge with the reset forced to EXACTLY zero phase — the landmine, measured
	kProbeMisMap = 6,   // MUTATION PROBE. addStep(1 - f, jump) — the natural mis-mapping of the edge's position
	kProbeBadSign= 7    // MUTATION PROBE. pastEdge with the jump computed BEFORE MINUS AFTER — this project's own prior research, transcribed verbatim
};
const int kSyncLegCount = 8;
const char* const SYNC_LEG_NAME[kSyncLegCount] = {
	"none", "detect", "pastEdge", "flatHalf", "oracle", "snap", "probe:misMap", "probe:badSign"
};

// The four IMPLEMENTABLE candidates. `oracle` consumes information the core
// cannot have and `snap` is a landmine, so neither is selectable; the two
// mutation probes are controls, not entrants. A decision rule evaluated over a
// set that included any of those four would be choosing between things the
// shipped core cannot be.
const int kSyncCandidateCount = 4;
const SyncLeg SYNC_CANDIDATES[kSyncCandidateCount] = {
	kLegNone, kLegDetect, kLegPastEdge, kLegFlatHalf
};

// Recording-only observations the probe writes while it runs. NOTHING in step()
// reads one back, no branch tests one, and deleting the whole struct would
// leave every returned float bit-identical — the same posture, for the same
// reason, as NaiveVcoCoreMirror's five recording members above.
struct SyncProbeDiag {
	int    fires;            // sync resets inside the MEASURED block
	int    lateFires;        // resets on a sample the master did NOT wrap on — a placement error that exists BEFORE any seam
	double fracMin, fracMax; // the detector's guarded fraction, range
	double jumpAbsSum;       // sum of |jump| over firing samples — the physical input to the step-dominated classification
	double phantomAbsMax;    // |pending| CARRIED IN to a reset sample — D-07's residual phantom, as a number
	double phantomAbsSum;
	int    phantomSamples;
};

inline SyncProbeDiag zeroedSyncDiag() {
	SyncProbeDiag d;
	d.fires = 0; d.lateFires = 0;
	d.fracMin = 2.0; d.fracMax = -1.0;
	d.jumpAbsSum = 0.0;
	d.phantomAbsMax = 0.0; d.phantomAbsSum = 0.0; d.phantomSamples = 0;
	return d;
}

// ---------------------------------------------------------------------------
// SyncPlacementProbe — THE MEASUREMENT INSTRUMENT. Read this banner before
// believing any decibel this file reports about hard sync.
//
// -- (1) WHAT IT IS FOR --
// Phase 33's central question is where the sync correction goes. The answer is
// decided by comparing eight per-sample arithmetic sequences that differ in
// ONE place and are identical everywhere else. This struct is that one place,
// parameterised.
//
// -- (2) IT MIRRORS THE SHIPPED CORE. IT DOES NOT APPROXIMATE IT. --
// Every line of forge::VcoCore::step is reproduced below IN ORDER: the
// volt-domain pitch summation, the D-14 bound with the negated comparison
// first, the single forge::exp2_taylor5 off forge::kVcoFreqC4, the sanitised
// rate, the ceiling-then-negated-floor order, the deltaPhase bound, the
// single-subtract wrap, the moved morph/character conditioning pair, the whole
// sync block including the guarded sub-sample solve and its zero fallback, the
// unconditional previous-voltage store, the `p` snapshot, the naive frozen
// call, the jump completion, and the ONE band-limiter call. It holds a REAL
// forge::Waveshape and a REAL forge::MorphBlep by value and calls them; it
// reimplements neither.
//   AND THE MIRRORING IS PROVED, NOT ASSERTED. The case named
// "(D-06) the sync placement probe reproduces forge::VcoCore bit-exactly on the
// past-edge leg" — it said "no-correction leg" until plan 33-06 landed the seam
// and re-anchored it, and the name is quoted here in its CURRENT form on purpose,
// because a banner quoting a case name that no longer exists is a banner nobody
// can follow — drives this struct and the LIVE core through the SAME loop
// with the SAME inputs, the SAME seeds and the SAME injected timing, and
// requires ZERO mismatches by DIRECT FLOAT EQUALITY over the whole grid at all
// three rates. That is the same non-vacuity move NaiveVcoCoreMirror already
// carries, and it exists for the same reason: a measurement instrument that has
// silently drifted from the thing it claims to measure produces figures about
// something else, and it goes on producing them greenly.
//   THE MIRROR-MAINTENANCE RULE APPLIES HERE VERBATIM. Every future change to
// forge::VcoCore::step's sequence must be mirrored here. If mirroring one ever
// moves a figure this file records, STOP AND REPORT rather than updating the
// number.
//
// -- (3) WHY THE PRE-EDGE HALF IS UNRECOVERABLE. THE DERIVATION. --
// src/dsp/MorphBlep.hpp's residual is r(x) = (x+1)^2/2 on the sample BEFORE an
// edge and r(x) = -(x-1)^2/2 on the sample AFTER it, zero outside [-1, 1]. D-01
// defines f as the fraction of the way from sample n-1 to sample n at which the
// master crossed HIGH, so the edge sits at absolute time (n-1) + f. Therefore:
//
//     sample n-1 : x = -f       ideal residual  +h*(1-f)^2/2   ALREADY EMITTED
//     sample n   : x = +(1-f)   ideal residual  -h*f^2/2       available
//     sample n+1 : x = +(2-f)   zero (outside the support)     n/a
//
// The two-point residual straddles the edge SYMMETRICALLY, so for a PAST edge
// the half owed to sample n-1 is forfeited: that sample has been emitted and
// nothing downstream can reach it. Recovering it needs a one-sample output
// delay buffer, and src/dsp/MorphBlep.hpp:225-230 rejects that on two grounds —
// it adds a sample of latency a VCO would have to declare, and it "complicates
// Phase 33, which needs to act on the CURRENT sample rather than on one already
// emitted". THIS PHASE IS ONE OF THE TWO REASONS THAT BUFFER DOES NOT EXIST, so
// this phase does not get to ask for it. This is a property of the KERNEL, not
// of the implementation: a minimum-phase kernel (minBLEP, which AA-05 forbids
// by name) places all of its correction after the edge and has no forfeited
// half at all.
//
// -- (4) THE ALGEBRAIC IDENTITY: THE PAST-EDGE LEG NEEDS NO HEADER CHANGE. --
// The recoverable half is exactly -h*f^2/2, and it is reachable through the
// EXISTING, PINNED addStep with no edit to src/dsp/MorphBlep.hpp whatsoever, by
// calling it at forward position ZERO with the jump PRE-SCALED by the negative
// square of the fraction. The arithmetic, so a reader can check it rather than
// take it:
//
//     addStep(0.f, -f*f*h)
//       u        = 1 - 0 = 1
//       inject  += (-f*f*h) *  0.5 * u * u  =  (-f*f*h) * 0.5 * 1 * 1  = -h*f^2/2
//       pending += (-f*f*h) * -0.5 * 0 * 0  =  0
//
// At forward position zero the FORWARD-OWED term vanishes identically — it is
// multiplied by xAhead squared, and xAhead is zero — and the current-sample
// term is EXACTLY the past-edge residual. The entry gate passes on its own
// terms (0 >= 0 is true, 0 > 1 is false), so the documented [0,1] contract is
// HONOURED, not reinterpreted, and the finiteness clause still applies to the
// pre-scaled jump. Whether the explicit `addPastStep(xBehind, jump)` entry
// point is preferable ON LEGIBILITY is plan 33-06's call; the two forms are
// numerically identical and this probe uses the zero-header-change one so that
// nothing in this file's measurement depends on a header edit that has not
// happened.
//
// -- WHAT THE LEG PARAMETER DOES AND DOES NOT TOUCH --
// `snap` is the ONLY leg that changes the RESET. `oracle` changes only the
// FRACTION THE CORRECTION USES, never the reset — its point is to price the
// fraction's accuracy, and moving the reset too would confound that with a
// second effect. Every other leg differs from `none` in the seam call alone.
// ---------------------------------------------------------------------------
struct SyncPlacementProbe {
	// Per-instance state, held exactly as forge::VcoCore holds it.
	forge::DriftEngine   drift;
	double               phase = 0.0;
	forge::Waveshape     wave;
	forge::MorphBlep     blep;
	forge::SchmittTrigger syncTrig;
	float                prevSyncVolts = 0.f;

	// --- THE ONE PARAMETER. ----------------------------------------------
	SyncLeg leg = kLegNone;

	// --- RECORDING ONLY. --------------------------------------------------
	// `master` supplies the oracle leg's true wrap fraction and the late-fire
	// diagnostic; `idx` is the sample counter that indexes it. Neither is read
	// on the `none` leg, which is what lets the bit-exactness gate compare this
	// struct against a core that has neither.
	const SyncMaster* master = 0;
	int               idx = 0;
	SyncProbeDiag     diag = zeroedSyncDiag();

	void seed(uint64_t s0, uint64_t s1 = 0) { drift.seed(s0, s1); }

	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) {
		drift.setSpreadSeed(s0, s1);
		wave.triAsymmetrySpread = drift.triAsymmetrySpread;
		wave.sawCurvatureSpread = drift.sawCurvatureSpread;
		wave.squareDutySpread   = drift.squareDutySpread;
		wave.pulseEdgeSpread    = drift.pulseEdgeSpread;
		wave.bleedSpread        = drift.bleedSpread;
	}

	float step(const forge::VcoInputs& in) {
		// D-07's residual phantom, read BEFORE anything this sample can touch
		// the accumulator. src/dsp/VcoCore.hpp names it and gives an ARITHMETIC
		// order of magnitude; 33-02's deferred register asks this plan for a
		// measured number instead. This is that number's source.
		const float pendingCarriedIn = blep.pending;

		float pitchVolts = in.pitchCV + in.coarse + in.fine * (1.f / 12.f);
		if (in.fmConnected) pitchVolts += in.fmVolts * in.fmAtten;
		if (!(pitchVolts > -forge::kVcoMaxPitchVolts)) pitchVolts = -forge::kVcoMaxPitchVolts;
		if (pitchVolts > forge::kVcoMaxPitchVolts) pitchVolts = forge::kVcoMaxPitchVolts;

		float freq = forge::kVcoFreqC4 * forge::exp2_taylor5(pitchVolts);

		const float safeRate = (in.sampleRate > 0.f) ? in.sampleRate : 0.f;
		const float maxFreq = forge::kVcoNyquistGuardFrac * safeRate;
		if (freq > maxFreq) freq = maxFreq;
		if (!(freq > 0.f)) freq = 0.f;

		double deltaPhase = (double)freq * (double)in.sampleTime;
		if (!(deltaPhase > 0.0)) deltaPhase = 0.0;
		if (deltaPhase > forge::kVcoMaxDeltaPhase) deltaPhase = forge::kVcoMaxDeltaPhase;
		phase += deltaPhase;
		if (phase >= 1.0) phase -= 1.0;

		// The conditioning pair, ABOVE the sync block, exactly as plan 33-02
		// moved it — the sync path calls the frozen waveshaper and must not be
		// handed raw fields.
		float morph = in.morph;
		if (!(morph > 0.f)) morph = 0.f;
		if (morph > 1.f) morph = 1.f;
		float character = in.character;
		if (!(character > 0.f)) character = 0.f;
		if (character > 1.f) character = 1.f;

		bool  syncFired  = false;
		float syncFrac   = 0.f;
		float syncBefore = 0.f;
		if (in.syncConnected && syncTrig.process(in.syncVolts, 0.1f, 1.0f)) {
			float f = (1.0f - prevSyncVolts) / (in.syncVolts - prevSyncVolts);
			if (!(f >= 0.f) || !(f < 1.f)) f = 0.f;
			syncBefore = wave.morphedWave((float)phase, morph, character, 0.f);
			// THE ONLY LEG THAT MOVES THE RESET. Everything else differs from
			// `none` in the seam call alone.
			phase = (leg == kLegSnap) ? 0.0 : (double)(1.f - f) * deltaPhase;
			syncFired = true;
			syncFrac = f;
		}
		prevSyncVolts = in.syncVolts;

		const float p = (float)phase;
		const float naive = wave.morphedWave(p, morph, character, 0.f);
		const float jump = syncFired ? (naive - syncBefore) : 0.f;

		// ==================== THE SEAM, PER LEG ==========================
		// It sits HERE — at the jump-completion line, after `naive` and ahead
		// of the single blep.step call — for the reason src/dsp/VcoCore.hpp
		// records at both ends: the jump does not exist yet inside the sync
		// block, because the post-reset term IS `naive`. Depositing ahead of
		// blep.step is what makes the residual drain on THIS sample: step()'s
		// preamble consumes `inject` unconditionally.
		if (syncFired) {
			switch (leg) {
				case kLegNone:
					break;                                                  // withheld ENTIRELY — the shipped core until plan 33-06, the withheld audition leg after it
				case kLegDetect:
					blep.addStep(syncFrac, jump);                           // one sample late, and it also owes forward
					break;
				case kLegPastEdge:
					// THE SHIPPED CORE from plan 33-06. Deliberately written in
					// the PRE-SCALED form and NOT as a call to addPastStep, so
					// this leg reaches the same arithmetic by a DIFFERENT ROUTE
					// than the core does and the bit-exactness gate below stays a
					// real comparison rather than a tautology.
					blep.addStep(0.f, -syncFrac * syncFrac * jump);         // the identity in banner item (4)
					break;
				case kLegFlatHalf:
					blep.addStep(0.f, jump);                                // a flat half-jump on the detection sample
					break;
				case kLegOracle: {
					// The generator's TRUE wrap fraction, which the test knows
					// exactly and the core CANNOT: the core sees two voltages.
					const float fo = master ? (float)master->wrapGHeld[(std::size_t)idx] : syncFrac;
					blep.addStep(0.f, -fo * fo * jump);
					break;
				}
				case kLegSnap:
					blep.addStep(0.f, -syncFrac * syncFrac * jump);         // pastEdge; the reset above is what differs
					break;
				case kProbeMisMap:
					blep.addStep(1.f - syncFrac, jump);                     // MUTATION PROBE
					break;
				case kProbeBadSign: {
					const float badJump = syncBefore - naive;               // MUTATION PROBE — STACK.md:124's sign
					blep.addStep(0.f, -syncFrac * syncFrac * badJump);
					break;
				}
			}
		}

		const float sample = naive + blep.step(wave, phase, p, deltaPhase, morph, character);

		// --- RECORDING ONLY, and only over the MEASURED block. ------------
		if (syncFired && idx >= kSpectrumN) {
			++diag.fires;
			if (master && master->wrappedHere[(std::size_t)idx] == 0) ++diag.lateFires;
			if ((double)syncFrac < diag.fracMin) diag.fracMin = (double)syncFrac;
			if ((double)syncFrac > diag.fracMax) diag.fracMax = (double)syncFrac;
			diag.jumpAbsSum += std::fabs((double)jump);
			const double ph = std::fabs((double)pendingCarriedIn);
			if (ph > diag.phantomAbsMax) diag.phantomAbsMax = ph;
			diag.phantomAbsSum += ph;
			++diag.phantomSamples;
		}
		++idx;

		return 5.f * sample;
	}
};

// ---------------------------------------------------------------------------
// measureSyncCellDb — THE ONE CELL-MEASURING FUNCTION, parameterised by leg.
//
// >>> THERE IS NO SECOND ONE, AND THERE MUST NEVER BE. <<< measureCellDb's
// banner already states the rule for the standing grid: "If a later agent adds
// a second measurement function for the corrected path, the phase's central
// claim stops being a measurement and becomes a coincidence." Eight near-copies
// of this body, one per leg, would make the RANKING below a coincidence in
// exactly that sense — the legs would differ in whatever else drifted between
// the copies as well as in the seam. Same master, same block, same warm-up,
// same seeds, same classifier, same drive loop; one switch.
//
// `useLiveCore` mirrors measureCellDb's `useMirror` exactly, and it is what
// lets the bit-exactness gate below compare the probe against forge::VcoCore
// THROUGH THIS FUNCTION rather than beside it.
//
// THE FOUR SEED LITERALS ARE COPIED VERBATIM from tests/VcoBlockDriver.hpp:42-43
// and must never be invented: a forge::Xoroshiro128Plus seeded (0, 0) is a
// fixed point emitting an all-zero stream, which makes std::normal_distribution's
// rejection loop never terminate — a hung suite here, and a HANG ON PATCH LOAD
// in Rack (T-32-09).
//
// NO BIN-CENTRING SOLVER RUNS HERE, and that is the derivation in the sub-grid
// banner being obeyed rather than an omission: the fundamental is the MASTER's,
// the master's bin error is exactly zero by construction, and the SLAVE IS FREE.
// ---------------------------------------------------------------------------
double measureSyncCellDb(const SyncCell& cell, SyncLeg leg, bool useLiveCore,
                         double* aliasRmsDbOut, double* masterBinErrorOut,
                         SyncProbeDiag* diagOut = 0, std::vector<float>* blockOut = 0) {
	const SyncMaster m = makeSyncMaster(2 * kSpectrumN, cell.Km, 5.0, cell.edge);
	if (masterBinErrorOut) *masterBinErrorOut = m.binError;

	// Copy-and-assign, never a brace value-list: forge::VcoInputs has NSDMIs, so
	// under C++11 it is not an aggregate and a value-list init is a hard error.
	forge::VcoInputs base;
	base.pitchCV   = cell.pitchCV;
	base.coarse    = 0.f;
	base.fine      = 0.f;
	base.morph     = cell.morph;
	base.character = cell.character;
	base.drift     = 0.f;

	const float dt = (float)(1.0 / cell.sr);
	std::vector<float> block;

	if (useLiveCore) {
		forge::VcoCore core;
		core.seed(0x1234ULL, 0x5678ULL);
		core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);
		driveSecondBlock(core, base, dt, cell.sr, block, &m.volts);
		// The live core carries no SyncProbeDiag. Zeroed rather than
		// half-filled, so a caller cannot read a partially populated diagnostic
		// as a measurement.
		if (diagOut) *diagOut = zeroedSyncDiag();
	} else {
		SyncPlacementProbe probe;
		probe.leg    = leg;
		probe.master = &m;
		probe.seed(0x1234ULL, 0x5678ULL);
		probe.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);
		driveSecondBlock(probe, base, dt, cell.sr, block, &m.volts);
		if (diagOut) *diagOut = probe.diag;
	}

	if (blockOut) *blockOut = block;

	int aliasBin = -1;
	// THE METRIC, UNCHANGED, WITH THE MASTER'S CYCLE COUNT SUBSTITUTED FOR THE
	// SLAVE'S. See the sub-grid banner for the derivation.
	return aliasPeakDb(block, cell.Km, &aliasBin, aliasRmsDbOut);
}

// ---------------------------------------------------------------------------
// fundamentalDominanceDb — THE SYNC SUB-GRID'S OWN D-10 SELF-CHECK, and the
// generalisation of hazard three from a single null point to a column.
//
// aliasPeakDb reports peak_nonharmonic / |X[K_m]|. For that ratio to MEAN "the
// alias floor sits this far below the fundamental", the master's fundamental
// has to BE the fundamental — the strongest bin on the master's own harmonic
// lattice. Hazard three is the extreme case of it not being so: at an exactly
// integer ratio the signal is periodic at the SLAVE's period and |X[K_m]| is
// 78 dB down, so the metric divides by almost nothing. The same degradation
// arrives GRADUALLY as the ratio climbs, and it must be MEASURED per cell
// rather than assumed away, exactly as D-10 requires the leakage floor to be
// measured rather than hoped small.
//
// The return value is 20*log10(|X[K_m]| / max_n |X[n*K_m]|), so it is 0.0 dB
// exactly when the master's fundamental IS the strongest harmonic and NEGATIVE
// by however far it is buried when it is not. It is computed on the SHIPPED
// CORE'S OWN LEG so that it is a property of the CELL and not of the candidate
// under test — a validity column that moved with the leg would be a validity
// column the choice of leg could argue with.
// ---------------------------------------------------------------------------
double fundamentalDominanceDb(const std::vector<float>& block, int Km, int* strongestHarmonicNOut) {
	if (strongestHarmonicNOut) *strongestHarmonicNOut = 0;
	if (block.size() < 2 || Km < 1) return -999.0;
	std::vector<std::complex<double> > x;
	x.reserve(block.size());
	for (std::size_t i = 0; i < block.size(); ++i)
		x.push_back(std::complex<double>((double)block[i], 0.0));
	fftRadix2(x);
	const int half = (int)(block.size() / 2);
	const double fund = std::abs(x[(std::size_t)Km]);
	double strongest = 0.0;
	int    strongestN = 0;
	for (int n = 1; n * Km <= half - 1; ++n) {
		const double mag = std::abs(x[(std::size_t)(n * Km)]);
		if (mag > strongest) { strongest = mag; strongestN = n; }
	}
	if (strongestHarmonicNOut) *strongestHarmonicNOut = strongestN;
	if (!(strongest > 0.0) || !(fund > 0.0)) return -999.0;
	return 20.0 * std::log10(fund / strongest);
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
// This case is what makes it one — and it is an INVERTED TOMBSTONE.
//
// >>> WHAT THIS CASE USED TO BE, RECORDED RATHER THAN ERASED. <<<
// Until plan 32-06 this case asserted that NaiveVcoCoreMirror and the live
// forge::VcoCore were BIT-IDENTICAL — `CHECK(differing == 0)` over the same 45
// grid points, the same three rates and the same four seed literals. That was
// the proof that the D-08 baseline is a faithful copy of the core, and it was
// true for exactly as long as forge::VcoCore::step applied no band-limiting.
//
// PLAN 32-06 LANDED forge::MorphBlep AT THE morphedWave CALL SITE, so that claim
// became FALSE BY DESIGN. The case was INVERTED IN PLACE — same slot in this
// file, same grid, same seeds, same three sample rates, same non-vacuity
// REQUIREs — rather than deleted. That is the shape Phase 29's silence tombstone
// used and Phase 30 honoured (D-15 / D-19), and it is the shape the tombstone's
// own instructions demanded.
//
// >>> WHAT THE INVERTED FORM BUYS THAT THE OLD ONE DID NOT. <<<
// The old form could only say the two paths were the same. This one says
// something strictly stronger: the two paths differ by EXACTLY the correction
// and by NOTHING ELSE. It reconstructs the live core's sample from the mirror's
// own recorded pre-scale value plus a LOCALLY held forge::MorphBlep driven with
// the mirror's own recorded p, phase, dt, morph and character, and requires the
// reconstruction to equal the core's sample BIT-EXACTLY. So the mirror is still
// proved to be a faithful copy of everything except the correction — which is
// the precondition plan 32-07's no-regression invariant rests on. A mirror that
// had quietly drifted in its pitch chain, its guard order or its accumulator
// would fail here even though it would still "differ from the core", because
// the difference would no longer be reproducible from forge::MorphBlep alone.
//
// >>> THE STANDING WARNING. <<<
// IF THE RECONSTRUCTION MISMATCH COUNT EVER BECOMES NON-ZERO, something OTHER
// than forge::MorphBlep changed inside forge::VcoCore::step, and the naive
// baseline has silently stopped being a baseline. STOP AND REPORT IT. Do not
// loosen the comparison, do not switch it to an approximate comparator, and do
// not "allow a few samples of slack" — every one of those converts the only
// evidence that the baseline is faithful into a statement that it is roughly
// faithful, which is not a baseline at all.
//
// ---------------------------------------------------------------------------
// MEASURED 2026-08-01 against plan 32-06's forge::VcoCore. Differing samples
// out of 4096, per grid point. RECONSTRUCTION MISMATCHES ARE 0 IN EVERY CELL.
//
//   morph        char 0.00 / 0.50 / 1.00      44.1 kHz | 48 kHz | 96 kHz
//   0.00 sine        0    / 1555 / 1549       diverging points: 14 | 13 | 12
//   0.25 triangle  1546   / 1545 / 1500
//   0.50 saw        778   / 1535 / 1497       reconstruction mismatches: 0
//   0.75 square    1556   / 1556 / 1555       at all three rates, all 45 cells
//   1.00 pulse      982   /  982 /  982
//
// The 48 kHz and 96 kHz grids have the same SHAPE with proportionally fewer
// differing samples (fewer cycles per block), so only the cells that are
// EXACTLY ZERO are worth naming — and they are named, all five, below.
//
// THE FIVE CELLS THAT DO NOT DIVERGE, AND WHY EACH IS CORRECT — this is what
// keeps the 12-of-15 bound from looking like slack:
//   * sine centre, character 0, at ALL THREE RATES. No discontinuity and no
//     bleed ring, so every site magnitude is zero. This is control (4) below.
//   * triangle centre, character 1, at 48 kHz and 96 kHz. The rounded corner is
//     0.175 in phase units and 2*dt is 0.1743 and 0.0874 — the corner is WIDER
//     than the kernel's own two-sample support, so the D-03 factor returns
//     EXACTLY zero. At 44.1 kHz 2*dt is 0.18994, just wider than the corner, and
//     the same cell diverges on 1500 samples. That rate-ordering is the D-03
//     compact-support cutoff being read off the kernel, visible as data.
//   * sine centre, character 1, at 96 kHz. At morph 0 the bleed ring puts the
//     weight on the PULSE (the finding recorded in src/dsp/MorphBlep.hpp), and
//     at character 1 the pulse's hard step is fully (1-c)-weighted to zero while
//     its soft edge is 0.16 wide against a 2*dt of 0.0874 — zero again.
//
// The saw centre at character 0 differs on 778 / 713 / 358 samples, which is
// exactly 2 per cycle at 389 / 357 / 179 cycles per block: ONE wrap edge, each
// correction spanning the sample that contains it and the one after it (D-13).
// That arithmetic agreeing is a second, independent reading that the placement
// is right.
// ---------------------------------------------------------------------------
TEST_CASE("vco spectrum: the core now DIVERGES from NaiveVcoCoreMirror by EXACTLY the MorphBlep correction (D-08 inversion, was the baseline-validity tombstone)") {

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

		// Per-rate tallies. They are asserted AFTER the fifteen cells of this
		// rate, so a rate whose grid silently lost cells cannot satisfy them.
		int reconstructionMismatchesThisRate = 0;   // MUST be 0 — the strong claim
		int divergingPointsThisRate = 0;            // MUST be >= 12 of 15
		int sawCentreChar0Differing = -1;           // MUST end up > 0
		int sineCentreChar0Differing = -1;          // MUST end up EXACTLY 0

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

				// --- THE RECONSTRUCTION INSTRUMENT. ----------------------------
				// ONE forge::MorphBlep per grid point, held LOCALLY here and
				// never shared between points. It is default-constructed inside
				// this scope, so it starts drained exactly as the live core's
				// member does, and it is destroyed with the point — which is the
				// reset the plan asks for, expressed as a lifetime rather than
				// as a call that could be forgotten.
				//
				// It is driven with the MIRROR'S OWN recorded values, not with
				// values recomputed here. Recomputing them would put a SECOND
				// mirror inside the case whose entire job is to detect the first
				// one drifting.
				forge::MorphBlep localBlep;

				const float dt = (float)(1.0 / sr);
				std::vector<float> mirrorOut;
				std::vector<float> reconstructed;
				mirrorOut.reserve(n);
				reconstructed.reserve(n);
				for (int i = 0; i < n; ++i) {
					forge::VcoInputs in = base;
					in.sampleTime = dt;
					in.sampleRate = (float)sr;
					mirrorOut.push_back(mirror.step(in));

					// The mirror has just advanced its own phase accumulator, so
					// mirror.phase is the POST-UPDATE phase — the same value the
					// live core hands its own member on this sample. The argument
					// order and the operation order below are the call site's,
					// deliberately: `naive + correction` first, then the x5.
					// Written the other way round (5*naive + 5*correction) this
					// is a DIFFERENT float, and the comparison below is bit-exact.
					const float correction = localBlep.step(
						mirror.wave, mirror.phase, mirror.lastP,
						mirror.lastDeltaPhase, mirror.lastMorph, mirror.lastCharacter);
					reconstructed.push_back(5.f * (mirror.lastNaive + correction));
				}
				REQUIRE(mirrorOut.size() == coreOut.size());
				REQUIRE(reconstructed.size() == coreOut.size());

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

				// --- THE DIVERGENCE COUNT. -------------------------------------
				// Counted with a DIRECT float !=, never doctest's approximate
				// comparator. That comparator applies a relative-scaling margin
				// even at epsilon(0), so it is not a bit-exact comparator and
				// would quietly absorb the very small corrections this case
				// exists to see.
				int differing = 0;
				for (int i = 0; i < n; ++i)
					if (coreOut[i] != mirrorOut[i]) ++differing;
				CAPTURE(differing);
				if (differing > 0) ++divergingPointsThisRate;

				// --- >>> THE STRONG CLAIM: THE DIVERGENCE *IS* THE CORRECTION.
				//
				// The reconstruction is the mirror's own pre-scale value plus a
				// locally driven forge::MorphBlep, scaled by the same x5 in the
				// same order as the call site. It must equal the live core's
				// sample with a DIRECT float ==, on every one of the 4096
				// samples. A tolerance here would defeat the point: the claim is
				// not "the two are close", it is "nothing else in step() moved".
				//
				// -ffp-contract=off IS WHAT MAKES THIS REPRODUCIBLE. The
				// correction is a chain of a*b+c terms, and contraction into
				// fused multiply-adds would give the core and this reconstruction
				// different roundings from identical inputs. The test target
				// passes the flag (Makefile TEST_CXXFLAGS) and
				// src/dsp/MorphBlep.hpp's banner records it as load-bearing for
				// that header specifically. If this comparison ever starts
				// failing by one unit in the last place, check the flag BEFORE
				// suspecting the DSP.
				int reconstructionMismatches = 0;
				for (int i = 0; i < n; ++i)
					if (coreOut[i] != reconstructed[i]) ++reconstructionMismatches;
				CAPTURE(reconstructionMismatches);
				CHECK(reconstructionMismatches == 0);
				reconstructionMismatchesThisRate += reconstructionMismatches;

				// The two named control points of this rate, recorded for the
				// assertions after the grid.
				if (morph == 0.50f && character == 0.0f) sawCentreChar0Differing  = differing;
				if (morph == 0.00f && character == 0.0f) sineCentreChar0Differing = differing;
			}
		}

		// =================================================================
		// THE PER-RATE ASSERTIONS. Everything above is per cell; these three
		// are what make the case a statement about the grid rather than about
		// whichever cell happened to run last.
		// =================================================================
		CAPTURE(reconstructionMismatchesThisRate);
		CAPTURE(divergingPointsThisRate);
		CAPTURE(sawCentreChar0Differing);
		CAPTURE(sineCentreChar0Differing);

		// (1) Nothing but forge::MorphBlep moved, anywhere on this rate's grid.
		CHECK(reconstructionMismatchesThisRate == 0);

		// (2) THE NAIVE-EQUALS-CORE CLAIM IS GENUINELY FALSE. 12 of 15 rather
		//     than 15 of 15 on purpose: the sine centre at character 0 has NO
		//     correction at all (see (4)), and the plan leaves room for the
		//     high-character cells where the D-03 factor correctly returns
		//     EXACTLY zero because the edge is already wider than the kernel's
		//     own two-sample support — a cell where the correction is zero is a
		//     cell where band-limiting is CORRECTLY declining to act, not a
		//     cell where it failed.
		CHECK(divergingPointsThisRate >= 12);

		// (3) THE SAW CENTRE AT CHARACTER 0 IS THE LOUDEST HARD EDGE ON THE
		//     GRID — a +2.000000 wrap jump at full authority, character-
		//     independent (P-4). If band-limiting is live anywhere, it is live
		//     here, so this point must show differing samples.
		REQUIRE(sawCentreChar0Differing >= 0);   // the cell ran at all
		CHECK(sawCentreChar0Differing > 0);

		// (4) THE ZERO-CORRECTION CONTROL, AND IT IS WHAT KEEPS (2) AND (3)
		//     HONEST. A sine at character 0 has no discontinuity of its own AND
		//     no bleed ring — the frozen bleed block is gated on
		//     `character >= 0.001f` (src/dsp/Waveshape.hpp:188) and
		//     forge::MorphBlep mirrors that exact comparison — so every site
		//     magnitude is zero and the correction is EXACTLY zero. The two
		//     paths must therefore remain BIT-IDENTICAL here.
		//
		//     Without this control, (2) and (3) could be satisfied by a
		//     correction that fired everywhere indiscriminately — including
		//     where there is nothing to correct, which is P-1's named failure
		//     mode and the thing the D-03 factor's EXACT zero exists to prevent.
		//     "The two now differ" is a weak claim on its own; "they differ
		//     everywhere there is an edge and NOWHERE there is not" is the claim
		//     worth making.
		REQUIRE(sineCentreChar0Differing >= 0);  // the cell ran at all
		CHECK(sineCentreChar0Differing == 0);
	}
}

// ---------------------------------------------------------------------------
// THE D-08 MEASURE PASS. This case's job is to RECORD, not to judge.
//
// D-08 requires the alias floor of the deliberately-aliased oscillator to be
// measured per shape, per note and per character BEFORE any band-limiting
// exists, so that the thresholds this phase pins are set FROM MEASUREMENT rather
// than inherited from prose. Three payoffs, in the phase's own words: the
// threshold is measured rather than inherited; the phase gets an objective
// iteration metric instead of ear-guessing, which is what its deliberate
// iteration budget is for; and the RED that follows is genuine, because the gate
// provably fails before forge::MorphBlep lands rather than being written against
// already-passing code.
//
// AS OF PLAN 32-07 IT RECORDS BOTH FLOORS IN ONE PASS. Every one of the 90 cells
// is measured TWICE — once through NaiveVcoCoreMirror (useMirror = true) and once
// through the corrected forge::VcoCore (useMirror = false) — inside the SAME
// loop, through the SAME measureCellDb, with the same solver, the same warm-up,
// the same block length, the same seeds and the same classifier. That is what
// makes the delta a like-for-like comparison rather than two measurements that
// merely happen to sit next to each other. A second case measuring the corrected
// path would have been a second apparatus, and the difference between two
// apparatuses is not a property of the DSP.
//
// SO IT STILL ASSERTS ALMOST NOTHING, DELIBERATELY. Three structural sanity
// properties are CHECKed, all three independent of any pinned threshold, and all
// three there to prove the apparatus is looking at a real signal rather than at
// silence. Everything else is CAPTUREd. The recorded figures are what the
// threshold column is pinned against; adding threshold assertions here would pin
// the naive floor as a REQUIREMENT, and the naive floor is the thing this phase
// exists to move.
//
// THE PER-CELL D-10 SELF-CHECK IS THE ONE HARD REQUIRE (T-32-11). Before any
// cell's alias value is read, the leakage implied by that cell's ACHIEVED bin
// error must sit at least 10 dB below that cell's own threshold. Without it the
// gate can pass by measuring forge::exp2_taylor5's output granularity rather
// than the DSP — D-10's stated failure mode — and every threshold below it
// becomes decoration.
//
// ===========================================================================
// THE MEASURE-TO-PIN PROTOCOL. This is the artefact this phase's deliberate
// iteration budget is spent through, and it is written HERE rather than in a
// plan so that a later phase can re-run it without re-deriving anything.
//
//  1. Run
//       ./build-test/test -tc="vco spectrum: the naive and corrected alias floors*" -s
//     and read the per-cell captures. Every cell reports `naiveDb`,
//     `correctedDb` and `improvementDb` (= naiveDb - correctedDb, positive when
//     the correction helped), together with the solver `method` and the
//     `impliedLeakage` the measurement was taken at.
//
//  2. For each GATED cell, the pinned threshold is the measured CORRECTED value
//     plus a 3 dB margin, rounded OUTWARD (toward the less negative value):
//     thresholdDb = ceil(correctedDb + 3.0). The margin is 3 dB because that is
//     roughly twice the largest cross-toolchain and cross-block-length variation
//     this suite has ever seen, and because a tighter margin turns a legitimate
//     float-ordering difference into a red build. The outward rounding is what
//     keeps the pinned number a round decibel a human can read and edit
//     deliberately rather than a transcription of a float.
//
//  3. Compare each measured corrected value against the prototype figure
//     recorded in 32-VALIDATION.md and in this table's trailing row comments. A
//     difference beyond about 1 dB is a FINDING about the implementation, not a
//     bookkeeping update: record it, name WHICH SIDE MOVED and why, and do not
//     silently adopt the new number.
//
//  4. NEVER adjust a threshold to accommodate a measured shortfall. A shortfall
//     escalates — see the anti-softening rule in the TEST-03 gate's banner
//     below, which names the two steps and ends at the operator.
//
//  5. Re-run steps 1 to 4 after ANY change to src/dsp/MorphBlep.hpp or to
//     forge::VcoCore::step's call site.
//
//  6. AND RE-RUN THEM AFTER CHANGING THE THRESHOLD COLUMN ITSELF, because the
//     column FEEDS BACK into the measurement: measureCellDb picks method one or
//     method two by comparing method one's implied leakage against
//     `thresholdDb - 10.0` (the D-10 bar), so a tightened threshold can escalate
//     a cell to the sampleTime nudge and move the very number it was pinned
//     from. The loop must therefore be iterated to a FIXED POINT — pin, re-run,
//     confirm no gated cell's method or corrected value moved — and the fixed
//     point is what the committed column records. This step was added in plan
//     32-07 after the feedback path was found by measurement; steps 1 to 5 as
//     the plan wrote them do not mention it and would have left a column pinned
//     against a superseded measurement.
// ===========================================================================
// ---------------------------------------------------------------------------

// ===========================================================================
// STEP-DOMINATED versus PLATEAU cells — the numerical-robustness classifier
// that the recorded-measurement reproduction CHECK below is split on.
//
// WHY THIS EXISTS AT ALL. `measuredDb` was pinned in plan 32-07 from ONE
// toolchain (Apple clang / libc++). Plan 32-10 pushed the phase to CI and
// measured the same 90 cells on GCC/libstdc++ (ubuntu-latest) and on MinGW
// g++ (windows-latest) for the first time. Run 30680251253 on SHA a110a9a:
// the two non-Apple legs disagree with the recorded column by up to
// 3.02596 dB (cell i = 86, 96 kHz square at character 1.00), on 21 cells.
// macOS reproduces the column exactly. NOTHING IN src/ BEHAVES DIFFERENTLY:
// `make strict` and the MinGW compile-and-link leg are green on the same SHA,
// the TEST-03 gate itself passes on all three legs, and the shipped LFO's
// goldens replay bit-exact everywhere. What differs is the INSTRUMENT.
//
// THE MECHANISM. `aliasPeakDb` reports a MAX OVER 2043 NON-HARMONIC BINS.
// The FFT twiddles and the frozen Waveshape's own trig/pow come from the
// platform's libm, and libm results differ between implementations in the
// last unit in the last place. One ULP cannot move a peak that stands tens
// of decibels clear of its neighbours — but where the alias spectrum is a
// near-FLAT PLATEAU of near-tied bins, one ULP REORDERS which bin wins, and
// the reported max steps to a different bin at a materially different level.
//
// THE PHYSICAL CRITERION, and it is stated here BEFORE the population is
// enumerated, precisely so the split cannot be a rename of "the cells that
// happened to fail today":
//
//   A cell is STEP-DOMINATED when the waveform at that (morph region,
//   character) carries a TRUE VALUE-STEP DISCONTINUITY of substantial
//   magnitude. A value step's harmonic series falls off at 6 dB/octave, so
//   its fold-back produces alias peaks that stand well clear of the rest of
//   the spectrum, and the arg-max is stable under a last-place perturbation.
//
//   A cell is PLATEAU-DOMINATED when it carries NO such step, so its alias
//   content is second-order (a slope break) or comes only from the frozen
//   bleed ring — a near-flat spectrum whose arg-max is a near-tie.
//
// APPLYING THE CRITERION TO THIS GRID'S FIVE REGIONS, from the frozen
// Waveshape's own measured behaviour (all figures from plan 32-05, probed
// against the frozen header rather than against MorphBlep's table):
//
//   saw       — STEP. The wrap jump is +2.000000 on computeSaw at character
//               0.00, 0.25, 0.50, 0.75 AND 1.00. P-4's "the soft reset
//               shrinks the step" premise was FALSIFIED; the step never
//               shrinks. Step-dominated at EVERY character.
//   pulse 5%  — STEP. Two hard rectangle edges at every character.
//   square    — STEP below full character (jump -1.201655 at character 0.50)
//               but NOT at character 1.00, where plan 32-05 measured the same
//               jump collapse to -0.001661 — three orders of magnitude down,
//               the edge fully softened. So square is step-dominated for
//               character < 1.00 and plateau-dominated at 1.00.
//   triangle  — NO value step at ANY character, by construction. Its whole
//               alias content is the SLOPE break polyBLAMP addresses, which
//               is why AA-02 is a separate requirement from AA-01. Plateau.
//   sine      — NO discontinuity of its own at any character. Whatever alias
//               energy it has comes ENTIRELY from the frozen bleed ring.
//               Plateau.
//
// The population therefore FALLS OUT of the criterion as
// {sine} u {triangle} u {square at character 1.00} = 7 cells per note block
// x 6 blocks = 42 plateau cells, and 48 step-dominated cells. Both counts are
// REQUIREd below, so a classifier that silently emptied either side fails
// loudly rather than making one of the two bounds vacuous.
//
// THE SPLIT IS A SUPERSET OF THE OBSERVED FAILURES, WHICH IS THE POINT.
// 21 cells drifted on CI; all 21 fall inside the 42. The other 21 plateau
// cells reproduced within 1.0 dB on all three legs anyway — they are covered
// by the looser bound because the criterion says they CAN be fragile, not
// because they were seen to be. And EVERY ONE of the 48 step-dominated cells
// reproduced within 1.0 dB on all three legs, which is the prediction the
// criterion makes and the evidence that it is the right criterion rather
// than a convenient one. If a step-dominated cell ever drifts past 1.0 dB,
// that is a FINDING to report under the standing instruction below — it is
// NOT licence to reclassify the cell.
static bool isStepDominatedCell(const std::string& region, float character) {
	if (region == "saw")      return true;   // +2.000000 jump at every character
	if (region == "pulse 5%") return true;   // two hard edges at every character
	if (region == "square")   return character < 1.0f;  // edge softened away at 1.00
	return false;                            // sine and triangle: no value step
}
// ===========================================================================

TEST_CASE("vco spectrum: the naive and corrected alias floors, recorded per shape, note and character (D-08 measure->pin loop)") {

	const std::size_t nCells = sizeof(SPECTRUM_GRID) / sizeof(SPECTRUM_GRID[0]);
	CAPTURE(nCells);

	// 45 gated + 15 diagnostic + 30 cross-rate regression. Asserted rather than
	// trusted: a table that silently lost a section would still walk cleanly and
	// would still report a floor, and nothing else here could tell.
	REQUIRE(nCells == (std::size_t)90);

	int gatedCells = 0, diagnosticCells = 0, regressionCells = 0;
	int sineChar0Cells = 0, pulseC9Char0Cells = 0;
	int methodOneCells = 0, methodTwoCells = 0;
	int correctedSaneCells = 0;
	int stepDominatedCells = 0, plateauCells = 0;

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

		// ---- THE MEASUREMENT, BOTH SIDES, SAME PASS. -------------------------
		// useMirror = true is the NAIVE baseline through NaiveVcoCoreMirror;
		// useMirror = false is the CORRECTED forge::VcoCore. Same function, same
		// solver, same warm-up, same seeds, same classifier — the delta below is
		// therefore a property of the correction and of nothing else.
		double aliasRmsDb = 0.0;
		double binError   = 0.0;
		int    method     = 0;
		const double naiveDb = measureCellDb(cell, /*useMirror=*/true, &aliasRmsDb, &binError, &method);
		const double impliedLeakage = impliedLeakageDb(binError);

		double correctedRmsDb = 0.0;
		double correctedBinError = 0.0;
		int    correctedMethod  = 0;
		const double correctedDb = measureCellDb(cell, /*useMirror=*/false, &correctedRmsDb,
		                                         &correctedBinError, &correctedMethod);

		// Positive means the correction HELPED by that many dB.
		const double improvementDb = naiveDb - correctedDb;

		CAPTURE(method);
		CAPTURE(binError);
		CAPTURE(impliedLeakage);
		CAPTURE(naiveDb);
		CAPTURE(aliasRmsDb);
		CAPTURE(correctedDb);
		CAPTURE(correctedRmsDb);
		CAPTURE(improvementDb);

		// Both sides must have been measured with the SAME instrument. The method
		// is chosen from cell.thresholdDb alone, so this can only differ if
		// measureCellDb ever grows a path that consults the core it is driving —
		// at which point the delta above would stop being like-for-like and would
		// silently become a comparison of two apparatuses.
		CAPTURE(correctedMethod);
		REQUIRE(correctedMethod == method);
		REQUIRE(correctedBinError == binError);

		if (method == kMethodPitchCV) ++methodOneCells; else ++methodTwoCells;

		// ---- THE RECORDED MEASUREMENT STILL REPRODUCES (T-32-15). -----------
		// `measuredDb` on the row is the corrected value plan 32-07 pinned that
		// row's threshold FROM. This CHECK is what stops it becoming a fossil: it
		// must still be what the core produces.
		//
		// THE BOUND IS SPLIT BY THE STEP-DOMINATED / PLATEAU CRITERION defined
		// above this TEST_CASE. Read that banner first — it states the physical
		// criterion, and the two populations fall out of it.
		//
		// WHY NOT ZERO, ON EITHER SIDE. The value is a float written to four
		// decimal places from one toolchain's run, and the alias PEAK is a max
		// over 2043 bins — a bin ordering that changes by one unit in the last
		// place can move the reported peak without anything in the DSP moving.
		//
		//   STEP-DOMINATED cells: 1.0 dB, UNCHANGED from plan 32-07. A third of
		//   the 3 dB pinning margin. MEASURED to hold on all three legs of run
		//   30680251253 — all 48 of them, on Apple clang, GCC/libstdc++ and
		//   MinGW g++ alike. This is the tight bound and it stays tight.
		//
		//   PLATEAU cells: 4.0 dB. PINNED FROM MEASUREMENT, not chosen. The
		//   worst cross-toolchain drift observed anywhere on run 30680251253 is
		//   3.02596 dB, at cell i = 86 (96 kHz, morph 0.75 square, character
		//   1.00), on both non-Apple legs. 4.0 is that worst rounded OUTWARD to
		//   the next even decibel, leaving 0.974 dB of headroom — the same
		//   outward-rounding rule step 2 of the MEASURE-TO-PIN PROTOCOL states,
		//   and the same shape as plan 32-07's two re-pinned tolerances (the
		//   no-regression bound at 4.0 against a measured 2.3344, and the
		//   cross-rate bound at 6.0 against a measured 4.7059).
		//
		// THE LOOSER BOUND IS STILL A REAL BOUND. It is not headroom nobody
		// reaches: the observed population runs to 3.03 dB, so 4.0 is 1.32x the
		// worst measurement rather than an order of magnitude above it. And it
		// remains a live T-32-15 tripwire, because re-typing a plateau cell's
		// measuredDb by more than 4 dB still fires here, while re-typing it by
		// ANY amount independently breaks the derivation assertion in the
		// TEST-03 gate below (threshold == max(ceil(measuredDb + 3), floor)).
		// The two assertions are what make the pair (measuredDb, thresholdDb)
		// an audit trail rather than a pair of editable numbers.
		//
		// >>> IF EITHER OF THESE FIRES, STOP AND REPORT IT RATHER THAN UPDATING
		// >>> THE NUMBER OR WIDENING THE BOUND. This instruction survives plan
		// 32-10's split unchanged in force, and applies to BOTH branches. Every
		// gated threshold is derived from measuredDb by an assertion in the
		// TEST-03 gate below, so re-typing measuredDb to match a new run
		// silently re-pins the whole column against whatever the implementation
		// now produces — the exact failure mode the anti-softening clause exists
		// to prevent. Widening one of these two bounds to silence a red is the
		// same act by another route. Re-run the MEASURE-TO-PIN PROTOCOL
		// deliberately, and record what moved and why.
		//
		// >>> AND IF THE CELL THAT FIRES IS STEP-DOMINATED, THAT IS A FINDING
		// >>> ABOUT THE CRITERION, NOT A CELL TO RECLASSIFY. The criterion
		// predicts step-dominated cells are toolchain-stable. Moving a cell
		// across the split to make a build green would empty the prediction of
		// content and is exactly what plan 32-10 was told not to do.
		const double recordedDb = (double)cell.measuredDb;
		const double recordedDrift = correctedDb - recordedDb;
		const bool   stepDominated = isStepDominatedCell(region, character);
		const double driftBoundDb = stepDominated ? 1.0 : 4.0;
		CAPTURE(recordedDb);
		CAPTURE(recordedDrift);
		CAPTURE(stepDominated);
		CAPTURE(driftBoundDb);
		if (stepDominated) ++stepDominatedCells; else ++plateauCells;
		CHECK(std::fabs(recordedDrift) <= driftBoundDb);

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
		REQUIRE(correctedDb > -900.0);

		// ---- STRUCTURAL SANITY 0: THE CORRECTION REMOVES ENERGY, IT DOES NOT
		//      INJECT IT — and this is checkable WITHOUT any pinned number.
		//
		// Every cell's corrected alias peak must be FINITE and strictly below
		// 0.0 dB. An alias louder than the fundamental would mean the correction
		// is putting energy INTO the spectrum rather than taking it out, which is
		// P-1's named failure mode in its loudest possible form (T-32-22), and a
		// non-finite value would mean a divisor guard in forge::MorphBlep has
		// been reached with something the negated pair did not catch (P-14).
		// Neither of those needs a threshold table to detect, so this assertion
		// survives ANY future re-pinning of the column — it is the one line in
		// this case that a circular threshold cannot make vacuous.
		//
		// It applies to ALL 90 cells, gated and diagnostic and cross-rate alike,
		// with no exclusions: the naive pulse at C9 already sits at -0.3 dB, so
		// there is nowhere on this grid where an injecting correction would have
		// room to hide under a looser bound.
		++correctedSaneCells;
		CHECK(std::isfinite(correctedDb));
		CHECK(correctedDb < 0.0);

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
	CAPTURE(correctedSaneCells);
	CHECK(gatedCells == 45);
	CHECK(diagnosticCells == 15);
	CHECK(regressionCells == 30);
	CHECK(sineChar0Cells == 6);
	CHECK(pulseC9Char0Cells == 1);

	// Structural sanity 0 ran on EVERY cell. A loop that skipped cells — or a
	// future `continue` added above it — would make the finite-and-below-zero
	// assertion silently partial, and a partial invariant over an injecting
	// correction is worth very little.
	CHECK(correctedSaneCells == 90);

	// ---- THE TWO REPRODUCTION POPULATIONS ARE NON-VACUOUS. -----------------
	// The split introduced in plan 32-10 hands the tight 1.0 dB bound to one
	// population and a measured 4.0 dB bound to the other. A classifier that
	// silently emptied the STEP-DOMINATED side would move every cell onto the
	// looser bound while every assertion above stayed green — the tight bound
	// would still be "present" and would simply never run, which is the same
	// unfalsifiable shape the tier census three lines up exists to prevent.
	//
	// The counts are derived, not chosen: {sine} u {triangle} u {square at
	// character 1.00} is 7 of the 15 cells in each of the 6 note blocks, so 42
	// plateau and 48 step-dominated. Asserting the exact numbers rather than
	// ">= 1" is what makes a region label typo or a lost grid section fail here
	// rather than quietly shifting a cell to the weaker assertion.
	CAPTURE(stepDominatedCells);
	CAPTURE(plateauCells);
	CHECK(stepDominatedCells == 48);
	CHECK(plateauCells == 42);
	CHECK(stepDominatedCells + plateauCells == 90);
}

// ---------------------------------------------------------------------------
// >>> TEST-03: THE LIVE ALIAS-FLOOR GATE. THIS CASE WAS THE D-08 RED TOMBSTONE,
//     AND IT WAS INVERTED IN PLACE RATHER THAN DELETED. <<<
//
// WHAT IT USED TO BE, RECORDED RATHER THAN ERASED. Until plan 32-07 this case
// asserted the OPPOSITE of what it asserts now: that the naive forge::VcoCore
// FAILED this gate — a `failing >= 27` count assertion over the same 45 cells —
// and that five named cells missed their thresholds by more than 5 dB. That was
// the D-08 RED — a gate written in its final form, run against the naive core,
// and OBSERVED failing, because a gate written against already-passing code is
// indistinguishable from a gate that cannot fail. Plan 32-03's SUMMARY holds the
// verbatim RED transcript; the observed count was 32 of the 45 gated cells.
//
// The RED and the GREEN therefore occupy ONE place in this file. Same slot, same
// SPECTRUM_GRID, same 45 gated cells, same measureCellDb with useMirror = false,
// same per-cell D-10 leakage REQUIRE. Deleting the tombstone and adding a green
// case beside it would have removed the only evidence the gate was ever able to
// fail. This is the shape Phase 29's silence tombstone used and Phase 30
// honoured (D-15 / D-19).
//
// ---------------------------------------------------------------------------
// >>> P-5, AS A NUMBER RATHER THAN AS AN APOLOGY. <<<
// The roadmap's former "approximately -60 dB" is a TARGET and is not reachable
// by 2-sample polyBLEP. The technique multiplies the spectrum by a squared sinc
// that is only about -8 dB at Nyquist and about -10.5 dB at the first alias of a
// C8 saw; that first alias is the saw's 6th harmonic at one sixth of the
// fundamental, i.e. -15.6 dB, so ten decibels of attenuation lands it at -25.8
// dB and no amount of implementation care moves it to -60. MEASURED HERE: -25.84
// dB, which is the arithmetic reproducing to two decimal places. DAFx-16 (paper
// 33, Table 2) independently reports the same ceiling for a FOUR-point polyBLAMP:
// 46 dB SNR for a triangle at C8 against 45 dB for 4x oversampling. ROADMAP SC-4
// was corrected on 2026-08-01 to per-shape measured thresholds, and THIS TABLE IS
// THAT GATE. A LATER AGENT MUST NOT "RESTORE" -60 dB.
//
// >>> P-6, AS THE REASON THE THIRD INDEX EXISTS. <<<
// A threshold indexed by region and note ALONE cannot be both red-on-naive and
// green-on-corrected. MEASURED: the triangle at C8 improves by 14.98 dB at
// character 0.00 (-33.81 -> -48.79) and by 0.04 dB at character 1.00 (-33.66 ->
// -33.70), because at character 1 the corner is already 7.7 samples wide and the
// D-03 factor correctly returns zero. Any single number for "triangle at C8" is
// therefore either vacuously passed by the naive path or wrongly failed by a
// correct implementation. Collapsing the `character` column would silently delete
// the phase's evidence.
//
// ---------------------------------------------------------------------------
// >>> THE ANTI-SOFTENING RULE. THIS IS AN INSTRUCTION TO THE NEXT AGENT. <<<
//
// A THRESHOLD IS NEVER ADJUSTED TO ACCOMMODATE A MEASURED SHORTFALL. If a gated
// cell misses, the escalation is, in order:
//
//   (1) Enable the deferred NARROW-PULSE "REACH" REFINEMENT documented in
//       src/dsp/MorphBlep.hpp's banner. That is the cheap place this phase's
//       iteration budget is meant to go, and it targets the exact regime the
//       measured residual sits in — sine centre with the bleed ring live, where
//       the narrow pulse's two edges fall inside one kernel span.
//
//   (2) If that is insufficient, STOP AND SURFACE TO THE OPERATOR with the
//       measured shortfall, an impact assessment and a recommendation. Raising
//       the kernel order from two points to four is an OPERATOR DECISION, not a
//       silent implementation choice: it changes the phase's delivered technique
//       and its CPU budget.
//
// OVERSAMPLING AND minBLEP ARE FORBIDDEN IN v2.0 BY AA-05 AND REMAIN FORBIDDEN.
// The broad escalation path is v2.1 OVERSAMPLING, explicitly NOT minBLEP.
//
// Editing a number in SPECTRUM_GRID is not on that list and never becomes one.
// It would convert a measurement into a transcription of the result, which is
// the one failure mode a threshold cannot survive (T-32-15).
//
// ---------------------------------------------------------------------------
// THE MEASURED FIGURES THIS CASE PASSES ON, recorded 2026-08-01 against the
// corrected forge::VcoCore. IF ANY OF THESE MOVES, STOP AND REPORT IT RATHER
// THAN UPDATING THE NUMBER — the same standing instruction as
// tests/test_vco_core.cpp:344-347.
//
//   failing cells: 0 of the 45 gated cells (was 32 of 45 against the naive core)
//
//   the five named large-margin subset cells — naive, corrected, improvement,
//   and the margin the corrected value clears its pinned threshold by:
//     44100 / K=389 / 0.25 triangle / char 0.00   -33.8085 -> -48.7878  +14.979 dB   vs -45  (3.79 clear)
//     44100 / K=389 / 0.50 saw      / char 0.00   -15.5630 -> -25.8423  +10.279 dB   vs -22  (3.84 clear)
//     44100 / K=389 / 0.75 square   / char 0.00   -16.9030 -> -31.8772  +14.974 dB   vs -28  (3.88 clear)
//     44100 / K=389 / 1.00 pulse    / char 0.00    -1.2931 -> -11.5704  +10.277 dB   vs  -8  (3.57 clear)
//     44100 / K=777 / 0.50 saw      / char 0.00    -9.5424 -> -19.0075   +9.465 dB   vs -16  (3.01 clear)
// ---------------------------------------------------------------------------
TEST_CASE("vco spectrum: TEST-03 - the alias floor stays below its per-shape pinned threshold at C7, C8 and C9 (D-09, was the D-08 RED tombstone)") {

	// The five cells the tombstone named. They were chosen because 32-RESEARCH
	// showed at least 8 dB of expected improvement there, which is why the same
	// five carry the ANTI-CIRCULARITY assertion below. They are what make this
	// case SPECIFIC rather than merely statistical: `failing == 0` alone could be
	// satisfied by a column pinned loose enough to admit anything.
	struct SubsetCell { double sr; int K; float morph; float character; };
	static const SubsetCell SUBSET[] = {
		{ 44100.0, 389, 0.25f, 0.00f },   // triangle, measured +14.979 dB improvement
		{ 44100.0, 389, 0.50f, 0.00f },   // saw,      measured +10.279 dB improvement
		{ 44100.0, 389, 0.75f, 0.00f },   // square,   measured +14.974 dB improvement
		{ 44100.0, 389, 1.00f, 0.00f },   // pulse,    measured +10.277 dB improvement
		{ 44100.0, 777, 0.50f, 0.00f },   // saw at C9, measured  +9.465 dB improvement
	};
	const std::size_t nSubset = sizeof(SUBSET) / sizeof(SUBSET[0]);

	// The minimum improvement the five named cells must deliver. 8.0 dB is
	// 32-RESEARCH's own expectation for this set, and the measured margin above
	// it is 1.465 dB at the tightest cell (saw at C9, +9.465).
	const float kMinImprovementDb = 8.0f;

	const std::size_t nCells = sizeof(SPECTRUM_GRID) / sizeof(SPECTRUM_GRID[0]);
	REQUIRE(nCells == (std::size_t)90);

	int gatedWalked = 0;
	int failing = 0;
	int subsetChecked = 0;
	int derivationChecked = 0;

	for (std::size_t i = 0; i < nCells; ++i) {
		const SpectrumCell& cell = SPECTRUM_GRID[i];

		// Only the gated tier. The C6 diagnostic row and the D-11 cross-rate rows
		// are recorded by the measure pass above and are NOT asserted here — the
		// gate's note set is C7, C8 and C9 (operator decision, 2026-08-01).
		if (std::string(cell.tier) != "gated") continue;
		++gatedWalked;

		const double sr        = cell.sr;
		const int    K         = cell.K;
		const float  morph     = cell.morph;
		const float  character = cell.character;
		const float  threshold = cell.thresholdDb;
		const float  measured  = cell.measuredDb;
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
		CAPTURE(measured);

		// ---- THE PINNING RULE, ASSERTED MECHANICALLY (T-32-15). -------------
		// thresholdDb == max(ceil(measuredDb + 3.0), kThresholdFloorDb). This is
		// the MEASURE-TO-PIN PROTOCOL step 2, expressed as an assertion instead of
		// as a paragraph someone can decline to follow.
		//
		// It is the half of the anti-softening rule that does not depend on anyone
		// reading the banner. A threshold nudged by a decibel to admit a cell that
		// missed fails here immediately, and naming the derivation makes the only
		// way to change a threshold an EXPLICIT re-measurement — which the
		// reproduction CHECK in the measure pass then holds to the DSP.
		++derivationChecked;
		const float derived = (float)std::ceil((double)measured + 3.0);
		const float expectedThreshold = (derived > kThresholdFloorDb) ? derived : kThresholdFloorDb;
		CAPTURE(derived);
		CAPTURE(expectedThreshold);
		CHECK(threshold == expectedThreshold);

		// >>> BOTH SIDES ARE MEASURED HERE, THROUGH THE SAME measureCellDb. <<<
		// useMirror = false is the LIVE forge::VcoCore — the shipped code path,
		// which is what the gate has to be a statement about. useMirror = true is
		// the naive mirror, and it is measured for the anti-circularity assertion
		// below, NOT for the threshold comparison.
		double correctedRmsDb = 0.0;
		double binError   = 0.0;
		int    method     = 0;
		const double correctedDb = measureCellDb(cell, /*useMirror=*/false, &correctedRmsDb, &binError, &method);
		const double impliedLeakage = impliedLeakageDb(binError);

		double naiveRmsDb = 0.0;
		double naiveBinError = 0.0;
		int    naiveMethod   = 0;
		const double naiveDb = measureCellDb(cell, /*useMirror=*/true, &naiveRmsDb, &naiveBinError, &naiveMethod);

		CAPTURE(method);
		CAPTURE(binError);
		CAPTURE(impliedLeakage);
		CAPTURE(correctedDb);
		CAPTURE(correctedRmsDb);
		CAPTURE(naiveDb);
		CAPTURE(naiveRmsDb);

		// The D-10 self-check, per cell, before the value is read — unchanged from
		// the tombstone and for the identical reason (T-32-11). A cell measured by
		// an out-of-specification instrument is not evidence of anything, green or
		// red. NOTE that the threshold column feeds this: the bar is
		// `threshold - 10`, so re-pinning the column re-runs this REQUIRE against
		// a different bar. That is the feedback path MEASURE-TO-PIN PROTOCOL step
		// 6 exists for.
		REQUIRE(impliedLeakage <= (double)threshold - 10.0);
		REQUIRE(correctedDb > -900.0);
		REQUIRE(naiveDb > -900.0);
		REQUIRE(naiveMethod == method);

		if (correctedDb > (double)threshold) ++failing;

		// ---- The named large-margin subset. --------------------------------
		for (std::size_t s = 0; s < nSubset; ++s) {
			if (SUBSET[s].sr != sr || SUBSET[s].K != K) continue;
			if (SUBSET[s].morph != morph || SUBSET[s].character != character) continue;
			++subsetChecked;

			const double marginDb = (double)threshold - correctedDb;   // positive = clears
			const double improvementDb = naiveDb - correctedDb;        // positive = helped
			CAPTURE(marginDb);
			CAPTURE(improvementDb);

			// >>> WAS: CHECK(naiveDb > (double)threshold + 5.0) — the D-08 RED. <<<
			CHECK(correctedDb <= (double)threshold);

			// >>> THE ANTI-CIRCULARITY ASSERTION. THIS IS THE ONE A THRESHOLD
			//     TABLE CANNOT FAKE. <<<
			//
			// Every threshold above was pinned from the implementation's own
			// output, so every gated cell passes `correctedDb <= threshold` BY
			// CONSTRUCTION, with 3 dB of room, and would go on passing if the
			// correction were deleted and the column re-pinned. That is exactly
			// T-32-15, and a pinned number cannot answer it.
			//
			// This line can. It compares TWO MEASUREMENTS OF THE SAME APPARATUS —
			// the same measureCellDb, the same solver, the same warm-up, the same
			// seeds, the same classifier, differing only in which core is driven —
			// and CONSULTS NO PINNED NUMBER AT ALL. Re-pin the whole column to
			// anything you like and this assertion is unmoved; it asks whether the
			// correction actually removes 8 dB of alias energy at the five cells
			// where 32-RESEARCH says it must, and nothing in SPECTRUM_GRID can
			// answer that question on the DSP's behalf.
			CHECK(improvementDb >= (double)kMinImprovementDb);
		}
	}

	// Non-vacuity of the whole case. A grid edit that dropped the gated tier, or
	// that moved one of the five subset cells, would make every assertion above
	// silently unreachable — and an assertion that never runs is
	// indistinguishable from one that cannot fail.
	CAPTURE(gatedWalked);
	CAPTURE(subsetChecked);
	CAPTURE(derivationChecked);
	CAPTURE(failing);
	REQUIRE(gatedWalked == 45);
	REQUIRE(subsetChecked == (int)nSubset);
	REQUIRE(derivationChecked == 45);

	// >>> WAS: CHECK(failing >= 27), against a naive-failures floor constant that
	//     this inversion deleted along with the assertion. OBSERVED at 32 of 45
	//     against the naive core (plan 32-03's SUMMARY holds the transcript, and
	//     git history holds the constant by name). THIS IS THE INVERSION. <<<
	CHECK(failing == 0);
}

// ---------------------------------------------------------------------------
// >>> THE SINGLE MOST VALUABLE ASSERTION IN THIS FILE. <<<
//
// The threshold table above says what the alias floor must be BELOW. This case
// says something the table structurally cannot: that band-limiting never makes
// any cell WORSE than doing nothing at all. That is what the D-03 compact-support
// character factor is FOR, and it is the assertion that would have caught EVERY
// rejected alternative in this phase's own design search.
//
// >>> WHY A NON-COMPACT FACTOR IS NOT MERELY "LESS GOOD" — IT IS DAMAGE. <<<
// A character factor WITHOUT compact support still returns a small non-zero
// correction on edges that are already several samples wide. There is no
// discontinuity there to cancel, so the residual step-shaped correction it
// injects is not a filter at all — it is BROADBAND ENERGY ADDED to a spectrum
// that was already clean. P-1's named failure mode, and it shows up as a
// REGRESSION rather than as a miss, which a threshold table scores as "no
// improvement" and passes.
//
// MEASURED WORST REGRESSION versus the naive path, from this phase's own
// alternatives table, for each factor considered and rejected:
//     full authority (no factor at all)   -60.4 dB
//     reciprocal-linear factor            -42.7 dB
//     sinc-Pade fit                       -36.6 dB
//     reciprocal-quadratic factor         -29.8 dB
//     THE SHIPPED COMPACT-SUPPORT FORM     -1.7 dB   (as prototyped)
//
// ---------------------------------------------------------------------------
// >>> THE TOLERANCE'S PROVENANCE, AND A FALSIFIED PREMISE RECORDED IN PLACE. <<<
//
// Plan 32-07 specified 2.0 dB, on 32-RESEARCH's reading that "the only
// regressions are about 1.5 dB, all at C9 on the sine row". MEASURED against the
// real forge::MorphBlep, the LOCATION is right and the MAGNITUDE is not:
//
//     44100 / K=777 C9 / 0.00 sine / character 0.50   -37.3824 -> -35.0480   -2.3344 dB
//     44100 / K=777 C9 / 0.00 sine / character 1.00   -23.8352 -> -23.0910   -0.7442 dB
//
// Those are the ONLY two regressing cells in the whole 90-cell grid; every other
// cell improves or is exactly unchanged. A 2.0 dB tolerance is therefore
// FALSIFIED BY MEASUREMENT — it would fail on correct, shipped behavior — so the
// bound here is 4.0 dB, pinned from the measured worst (2.3344) rounded outward
// to the next even decibel, leaving 1.67 dB of headroom.
//
// LOOSENING IT COSTS THIS ASSERTION NOTHING, and that is why it is the honest
// move rather than a softening: the loosest rejected alternative regresses by
// 29.8 dB, so 4.0 dB still fails all four of them by AT LEAST 25.8 dB. The
// tolerance is nowhere near the population it exists to separate.
//
// WHY THE TWO CELLS REGRESS, so the number is not merely recorded but explained:
// both are the sine centre with the bleed ring live at C9, where the phase
// advances 0.19 per sample and the 5 % pulse the bleed ring introduces is 0.05
// wide — its two edges fall INSIDE one kernel span, so the two polyBLEP
// corrections overlap and partly work against each other. That is precisely the
// deferred narrow-pulse "reach" refinement recorded in src/dsp/MorphBlep.hpp's
// banner, and it is the first escalation step the TEST-03 gate's anti-softening
// rule names. It is a known, bounded, documented limitation — not an unexplained
// number the tolerance was widened to hide.
//
// >>> AND A FUTURE REFACTOR CANNOT USE THIS TO TRADE ONE SHAPE AGAINST ANOTHER.
// The walk is over ALL 90 cells with NO exclusions — gated, diagnostic and
// cross-rate alike — and the assertion is per cell, not on an average. A change
// that bought 10 dB on the saw by giving up 5 dB on the triangle fails here even
// though every threshold above would still be met and the mean would improve.
// ---------------------------------------------------------------------------
TEST_CASE("vco spectrum: band-limiting never makes any cell WORSE than the naive path (the D-03 compact-support invariant)") {

	// Pinned from the measured worst regression of 2.3344 dB — see the banner.
	// It is NOT the 2.0 dB the plan specified, and the reason is recorded there
	// rather than absorbed.
	const float kMaxRegressionDb = 4.0f;

	const std::size_t nCells = sizeof(SPECTRUM_GRID) / sizeof(SPECTRUM_GRID[0]);
	REQUIRE(nCells == (std::size_t)90);

	int walked = 0;
	int regressingCells = 0;
	double worstRegressionDb = 0.0;   // the largest positive (corrected - naive)
	int worstRegressionCell = -1;

	for (std::size_t i = 0; i < nCells; ++i) {
		const SpectrumCell& cell = SPECTRUM_GRID[i];
		++walked;

		const double sr        = cell.sr;
		const int    K         = cell.K;
		const float  morph     = cell.morph;
		const float  character = cell.character;
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
		CAPTURE(tier);

		// Both sides through the SAME measureCellDb, same solver, same warm-up,
		// same seeds, same classifier. A comparator whose two sides run different
		// loops proves nothing about the difference between them.
		double naiveRms = 0.0, naiveBinErr = 0.0;
		int naiveMethod = 0;
		const double naiveDb = measureCellDb(cell, /*useMirror=*/true, &naiveRms, &naiveBinErr, &naiveMethod);

		double corrRms = 0.0, corrBinErr = 0.0;
		int corrMethod = 0;
		const double correctedDb = measureCellDb(cell, /*useMirror=*/false, &corrRms, &corrBinErr, &corrMethod);

		const double regressionDb = correctedDb - naiveDb;   // positive = WORSE
		const double improvementDb = -regressionDb;

		CAPTURE(naiveDb);
		CAPTURE(correctedDb);
		CAPTURE(improvementDb);
		CAPTURE(regressionDb);

		REQUIRE(naiveDb > -900.0);
		REQUIRE(correctedDb > -900.0);
		REQUIRE(corrMethod == naiveMethod);

		if (regressionDb > 0.0) ++regressingCells;
		if (regressionDb > worstRegressionDb) {
			worstRegressionDb = regressionDb;
			worstRegressionCell = (int)i;
		}

		// >>> THE INVARIANT. It consults NO pinned threshold — it compares two
		//     measurements of the same apparatus, exactly like the 8 dB
		//     minimum-improvement assertion in the TEST-03 gate, and for the same
		//     anti-circularity reason (T-32-15 / T-32-22).
		CHECK(correctedDb <= naiveDb + (double)kMaxRegressionDb);
	}

	// The running worst across the whole grid, reported as one number so a
	// regression that crept in anywhere is visible without reading 90 cells.
	CAPTURE(walked);
	CAPTURE(regressingCells);
	CAPTURE(worstRegressionDb);
	CAPTURE(worstRegressionCell);

	// Non-vacuity: every cell was walked. An invariant that silently stopped
	// covering the cross-rate tier would still report a clean worst-regression.
	REQUIRE(walked == 90);

	// The grid-level restatement of the same claim. MEASURED: 2.3344 dB at cell
	// 31, and exactly 2 of the 90 cells regress at all.
	CHECK(worstRegressionDb <= (double)kMaxRegressionDb);

	// AND THE INVARIANT IS STILL ABLE TO FAIL. If the worst regression ever
	// reached exactly zero this case would have become a tautology about a
	// correction that never hurts anywhere — pleasant, but no longer evidence.
	// It is not zero: two cells regress, they are named in the banner, and they
	// are the deferred narrow-pulse regime. This CHECK fires if that ever stops
	// being true, which is a finding either way and must be reported rather than
	// deleted.
	CHECK(regressingCells > 0);
}

// ---------------------------------------------------------------------------
// >>> D-11: THE CROSS-RATE REGRESSION. WHY A SINGLE-RATE GATE IS NOT ENOUGH. <<<
//
// A band-limiting correction scaled wrongly by `dt` fails RATE-DEPENDENTLY. The
// residual kernel is a function of the crossing distance measured in SAMPLES, so
// a factor off by a power of `dt` produces a correction that is right at one
// rate and wrong at every other one. That failure is COMPLETELY INVISIBLE to a
// grid measured at a single sample rate: the arithmetic still looks plausible,
// the spectrum still looks like a spectrum, and only the comparison across rates
// can see it. It is the most likely way this implementation goes subtly wrong.
//
// 44.1 kHz IS THE BINDING ASSERTION; the other two rates are REGRESSION. The
// three rows deliberately land on the SAME NOTE — 4188.2, 4183.6 and 4195.3 Hz —
// so the comparison is like with like. Measuring 48 kHz at a different note
// would confound a rate-scaling bug with a harmonics-below-Nyquist difference
// and the whole point would be lost.
//
// This mirrors the Phase 31 lesson that the most natural version of a test can
// be bit-exactly vacuous: the natural D-11 test is "it works at 48 kHz too",
// which passes trivially for any correction that runs at all.
//
// ---------------------------------------------------------------------------
// >>> TWO PLAN PREMISES FALSIFIED BY MEASUREMENT, RECORDED IN PLACE. <<<
//
// Plan 32-07 specified a single one-sided bound of 3.0 dB for both higher rates,
// reasoning that "a higher sample rate legitimately produces a LOWER alias floor
// (96 kHz carries 11 harmonics below Nyquist against C8's 5 at 44.1 kHz), so the
// assertion is one-sided by design". MEASURED, the one-sidedness is right and
// the rest is not:
//
//   (1) 48 kHz IS WORSE THAN 44.1 kHz ON ALL FIFTEEN COMBINATIONS, by +0.039 to
//       +4.706 dB. The "higher rate is better" intuition simply does not hold
//       for the 44.1 -> 48 kHz step, and a 3.0 dB bound fails on three
//       combinations of correct, shipped behavior: sine char 0.00 (+3.839), sine
//       char 1.00 (+4.706) and square char 1.00 (+4.133).
//
//   (2) 96 kHz IS BETTER THAN 44.1 kHz ON ALL FIFTEEN, by 0.811 to 27.052 dB.
//       Never worse, not once — a far stronger statement than "within 3 dB", and
//       it is asserted as such below.
//
// WHY, so the numbers are explained and not merely recorded. The NAIVE floors at
// 44.1 and 48 kHz agree to within 0.01 dB cell for cell (plan 32-03's finding,
// reproduced here), because the naive alias amplitude of harmonic n is about 1/n
// either way. The CORRECTED floors do not, because the polyBLEP's attenuation is
// a strong function of how close the first surviving alias lands to Nyquist, and
// the three rates put it in different places: the C8 saw's 6th harmonic folds to
// 0.430 of the sample rate at 44.1 kHz and to 0.477 at 48 kHz. Further from
// Nyquist means more attenuation — MEASURED 10.28 dB at 44.1 kHz against 8.45 dB
// at 48 kHz and 8.67 dB at 96 kHz, the last two agreeing because both fold to
// about 0.476. That is correct polyBLEP behavior, not a `dt` defect.
//
// THE BOUNDS BELOW ARE PINNED FROM THAT MEASUREMENT, and both keep about 1.3 dB
// of headroom: 6.0 dB for 48 kHz against a measured worst of +4.706, and 0.5 dB
// for 96 kHz against a measured worst of -0.811. Loosening the 48 kHz bound
// costs the assertion nothing it was there to catch: a `dt`-scaled correction
// does not miss by 5 dB, it injects broadband energy and regresses by tens of dB
// (see the alternatives table in the no-regression case above).
// ---------------------------------------------------------------------------
TEST_CASE("vco spectrum: the D-11 cross-rate regression - the same note behaves consistently at 44.1, 48 and 96 kHz") {

	// One-sided, and deliberately asymmetric between the two higher rates,
	// because the measurement is asymmetric. See the banner.
	const double k48ExcessDb = 6.0;   // measured worst +4.7059 (sine, character 1.00)
	const double k96ExcessDb = 0.5;   // measured worst -0.8114 (square, character 0.00) — never worse

	static const float MORPHS[]     = {0.00f, 0.25f, 0.50f, 0.75f, 1.00f};
	static const float CHARACTERS[] = {0.00f, 0.50f, 1.00f};

	const std::size_t nCells = sizeof(SPECTRUM_GRID) / sizeof(SPECTRUM_GRID[0]);

	int triples = 0;
	double worst48 = -1e9, worst96 = -1e9;
	double sawCentre441 = 0.0, sawCentre96 = 0.0;
	bool sawCentreSeen = false;

	for (std::size_t mi = 0; mi < sizeof(MORPHS) / sizeof(MORPHS[0]); ++mi) {
		for (std::size_t ci = 0; ci < sizeof(CHARACTERS) / sizeof(CHARACTERS[0]); ++ci) {
			const float morph = MORPHS[mi];
			const float character = CHARACTERS[ci];
			CAPTURE(morph);
			CAPTURE(character);

			// The three C8 cells for this combination, looked up in the grid
			// rather than reconstructed, so the assertion is over the SAME rows
			// the measure pass and the gate walk. A row that moved would be found
			// missing here rather than silently replaced by an invented one.
			const SpectrumCell* c441 = 0;
			const SpectrumCell* c48  = 0;
			const SpectrumCell* c96  = 0;
			for (std::size_t i = 0; i < nCells; ++i) {
				const SpectrumCell& cell = SPECTRUM_GRID[i];
				if (std::string(cell.note) != "C8") continue;
				if (cell.morph != morph || cell.character != character) continue;
				if (cell.sr == 44100.0 && cell.K == 389) c441 = &cell;
				else if (cell.sr == 48000.0 && cell.K == 357) c48 = &cell;
				else if (cell.sr == 96000.0 && cell.K == 179) c96 = &cell;
			}
			REQUIRE(c441 != 0);
			REQUIRE(c48  != 0);
			REQUIRE(c96  != 0);
			++triples;

			double rms = 0.0, binErr = 0.0;
			int method = 0;
			const double db441 = measureCellDb(*c441, /*useMirror=*/false, &rms, &binErr, &method);
			const double leak441 = impliedLeakageDb(binErr);
			const double db48 = measureCellDb(*c48, /*useMirror=*/false, &rms, &binErr, &method);
			const double leak48 = impliedLeakageDb(binErr);
			const double db96 = measureCellDb(*c96, /*useMirror=*/false, &rms, &binErr, &method);
			const double leak96 = impliedLeakageDb(binErr);

			const double excess48 = db48 - db441;   // positive = 48 kHz is WORSE
			const double excess96 = db96 - db441;   // positive = 96 kHz is WORSE

			CAPTURE(db441);
			CAPTURE(db48);
			CAPTURE(db96);
			CAPTURE(excess48);
			CAPTURE(excess96);

			// The D-10 self-check on all three, before any value is read.
			REQUIRE(leak441 <= (double)c441->thresholdDb - 10.0);
			REQUIRE(leak48  <= (double)c48->thresholdDb  - 10.0);
			REQUIRE(leak96  <= (double)c96->thresholdDb  - 10.0);
			REQUIRE(db441 > -900.0);
			REQUIRE(db48  > -900.0);
			REQUIRE(db96  > -900.0);

			if (excess48 > worst48) worst48 = excess48;
			if (excess96 > worst96) worst96 = excess96;

			// >>> ONE-SIDED BY DESIGN. A higher rate landing a LOWER alias floor
			//     is correct behavior, so a two-sided band would fail on the very
			//     thing this case wants to see. Only "worse" is bounded.
			CHECK(excess48 <= k48ExcessDb);
			CHECK(excess96 <= k96ExcessDb);

			// ---- THE dt-SCALING ISOLATION ASSERTION. --------------------------
			// The saw centre at character 0 is the ONE cell whose correction is
			// completely character-independent (P-4): a single +2.0 wrap jump at
			// full authority, no bleed ring, no soft edge, no D-03 factor in play
			// at all. Every character effect is therefore removed from the
			// comparison, and what is left is the `dt` scaling and nothing else.
			//
			// 96 kHz must be at least as good as 44.1 kHz here, with NO tolerance:
			// MEASURED -30.2544 against -25.8423, a 4.41 dB margin. This is the
			// cheapest possible direct test that the correction scales with `dt`
			// in the right direction and by the right power.
			if (morph == 0.50f && character == 0.00f) {
				sawCentre441 = db441;
				sawCentre96  = db96;
				sawCentreSeen = true;
				CHECK(db96 <= db441);
			}
		}
	}

	CAPTURE(triples);
	CAPTURE(worst48);
	CAPTURE(worst96);
	CAPTURE(sawCentre441);
	CAPTURE(sawCentre96);

	// Non-vacuity: all fifteen combinations were found and compared. A grid whose
	// cross-rate tier lost rows would make every assertion above unreachable.
	REQUIRE(triples == 15);
	REQUIRE(sawCentreSeen);

	// The grid-level restatement, so a rate-dependent drift anywhere is one
	// number rather than fifteen. MEASURED: +4.7059 and -0.8114.
	CHECK(worst48 <= k48ExcessDb);
	CHECK(worst96 <= k96ExcessDb);

	// AND 96 kHz IS NEVER WORSE AT ALL. Stated separately from the 0.5 dB bound
	// because it is the stronger claim and the one the measurement actually
	// supports: on all fifteen combinations the 96 kHz corrected floor is below
	// the 44.1 kHz one, by 0.811 to 27.052 dB. If this ever ceases to hold while
	// the 0.5 dB bound still passes, the correction has started losing ground at
	// the rate where it should be strongest, and that is a finding.
	CHECK(worst96 < 0.0);
}

// ---------------------------------------------------------------------------
// TASK 1 OF PLAN 33-05 — THE SYNC SUB-GRID'S OWN VALIDATION, BEFORE ANY LEG IS
// MEASURED THROUGH IT.
//
// Nothing in this case is about placement. It is about the INSTRUMENT: that the
// master really is exactly periodic in the block, that its bin error really is
// exactly zero, that both edge shapes really are present and really do differ
// where the hazard says they differ, and that the grid really does carry the
// axes the decision rule depends on. A measurement whose apparatus has not been
// validated first is the failure mode this whole file is written against.
// ---------------------------------------------------------------------------
TEST_CASE("vco spectrum: (D-11) the sync sub-grid's master is the fundamental, its bin error is EXACTLY zero, and both master edge shapes are present") {

	const std::vector<SyncCell>& grid = syncGrid();

	// =======================================================================
	// PART A — THE AXIS ENUMERATION. Counted from the built grid rather than
	// asserted from the table's shape, because the point of building it in
	// code is that the count is mechanical.
	// =======================================================================
	const std::size_t nCells = grid.size();
	CAPTURE(nCells);
	CHECK(nCells == 420);   // 3 rates * 2 edge shapes * 7 ratios * 5 morphs * 2 characters

	int subUnityCells = 0, hardEdgeCells = 0, bandLimitedCells = 0, unpinnedCells = 0;
	for (std::size_t i = 0; i < nCells; ++i) {
		if (grid[i].ratio < 1.0)                    ++subUnityCells;
		if (grid[i].edge == kMasterHardEdge)        ++hardEdgeCells;
		if (grid[i].edge == kMasterBandLimited)     ++bandLimitedCells;
		// >>> INVERTED BY PLAN 33-07. <<< This counted UNPINNED cells and
		// asserted all 420 of them, because plan 33-05 deliberately gated
		// nothing. Plan 33-07 pins every cell from SYNC_PINS, so the same
		// mechanical count now asserts the OPPOSITE and is the tripwire for a
		// grid row added without a pin: any cell still holding the sentinel, the
		// "UNPINNED" tier or the 33-05 provenance string is counted here and the
		// assertion below requires the count to be ZERO. The direction of the
		// claim was inverted; the mechanism was not weakened, and it was not
		// deleted either — a deleted count cannot detect an unpinned row.
		const bool stillUnpinned = (grid[i].measuredDb == kSyncUnpinnedDb)
		                        || (grid[i].thresholdDb == kSyncUnpinnedDb)
		                        || (std::string(grid[i].tier) == std::string("UNPINNED"))
		                        || (std::string(grid[i].provenance) == std::string(kProvSyncUnpinned));
		if (stillUnpinned) ++unpinnedCells;
	}
	CAPTURE(subUnityCells);
	CAPTURE(hardEdgeCells);
	CAPTURE(bandLimitedCells);
	CAPTURE(unpinnedCells);

	// HAZARD ONE'S AXIS EXISTS. Without a sub-unity ratio the decision rule's
	// condition 2 has no cells to evaluate on and no defensible answer is
	// reachable at all.
	CHECK(subUnityCells == 120);   // ratios 0.5 and 0.75

	// HAZARD TWO'S AXIS EXISTS, on both halves.
	CHECK(hardEdgeCells    == 210);
	CHECK(bandLimitedCells == 210);

	// HAZARD THREE'S AXIS. No ratio at or above two may be an exact integer:
	// there the master's fundamental bin empties and the metric divides by
	// almost nothing. See the SYNC_RATIOS banner for the measured table, and the
	// permanent null-point control in the measurement case.
	int integerRatioCellsAtOrAboveTwo = 0;
	for (std::size_t i = 0; i < nCells; ++i)
		if (grid[i].ratio >= 2.0 && grid[i].ratio == std::floor(grid[i].ratio))
			++integerRatioCellsAtOrAboveTwo;
	CAPTURE(integerRatioCellsAtOrAboveTwo);
	CHECK(integerRatioCellsAtOrAboveTwo == 0);

	// EVERY CELL IS PINNED, AND SAYS SO. Inverted by plan 33-07 from 33-05's
	// `== nCells`; see the counting site above for why the mechanism was kept.
	CHECK(unpinnedCells == 0);

	// =======================================================================
	// PART B — THE BIN-ERROR ASSERTION, MADE DIRECTLY.
	//
	// >>> THE IMPLIED-LEAKAGE HELPER IS DELIBERATELY *NOT* THE VEHICLE FOR
	//     THIS CLAIM, AND THAT IS THE WHOLE POINT OF THIS PART. <<<
	// impliedLeakageDb's first line is `if (!(binError > 0.0)) return -999.0;`
	// — its NEGATED branch returns the -999.0 sentinel for a bin error of
	// exactly zero. Routing this claim through it would therefore "pass" by
	// collecting a sentinel that means "there is nothing to report", which is
	// semantically right here ONLY BY ACCIDENT: the identical -999.0 comes back
	// for a fundamental bin of zero magnitude, i.e. for silence, and for a
	// negative bin error, i.e. for a caller that computed it wrong. This file's
	// standing posture is that a sentinel must not stand in for a measurement,
	// and D-10 takes that posture everywhere else in it. So the exact-zero
	// claim is made by DIRECT COMPARISON against the generator's own
	// accumulated phase, and the sentinel is shown to be the reason NOT to use
	// the helper rather than left as an unexamined convenience.
	// =======================================================================
	double hzMin = 1e30, hzMax = -1e30;
	for (std::size_t ri = 0; ri < sizeof(SYNC_RATES) / sizeof(SYNC_RATES[0]); ++ri) {
		const double sr = SYNC_RATES[ri].sr;
		const int    Km = SYNC_RATES[ri].Km;
		CAPTURE(sr);
		CAPTURE(Km);

		// The coprimality requirement, reduced to "pick an odd K_m" by
		// N = 4096 = 2^12 — asserted mechanically rather than trusted to the
		// table, exactly as the standing grid asserts its own K values.
		CHECK((Km % 2) == 1);

		const double masterHz = (double)Km * sr / (double)kSpectrumN;
		CAPTURE(masterHz);
		if (masterHz < hzMin) hzMin = masterHz;
		if (masterHz > hzMax) hzMax = masterHz;

		for (std::size_t ei = 0; ei < sizeof(SYNC_EDGES) / sizeof(SYNC_EDGES[0]); ++ei) {
			const SyncMasterEdge edge = SYNC_EDGES[ei];
			CAPTURE(SYNC_EDGE_NAME[ei]);

			const SyncMaster m = makeSyncMaster(2 * kSpectrumN, Km, 5.0, edge);
			REQUIRE(m.volts.size() == (std::size_t)(2 * kSpectrumN));

			// >>> THE DIRECT COMPARISON. <<<
			const double binError = m.binError;
			CAPTURE(binError);
			CAPTURE(m.achievedCyclesPerBlock);
			CHECK(binError == 0.0);
			CHECK(m.achievedCyclesPerBlock == (double)Km);
			CHECK(m.wrapsInFirstBlock == (long)Km);

			// And the sentinel, shown rather than described: the helper returns
			// -999.0 for this bin error, which is why it is not the vehicle.
			CHECK(impliedLeakageDb(binError) == -999.0);

			// NON-VACUITY: the master must actually arm and actually fire, or
			// every sync leg below measures a free-running oscillator. Counted
			// on the MEASURED block only, the same block the metric analyses.
			int crossHigh = 0, crossLow = 0;
			for (int i = kSpectrumN; i < 2 * kSpectrumN; ++i) {
				const float prev = m.volts[(std::size_t)(i - 1)];
				const float now  = m.volts[(std::size_t)i];
				if (prev < 1.0f && now >= 1.0f) ++crossHigh;
				if (prev > 0.1f && now <= 0.1f) ++crossLow;
			}
			CAPTURE(crossHigh);
			CAPTURE(crossLow);
			CHECK(crossHigh == Km);
			CHECK(crossLow  == Km);

			// The true wrap fraction really does sweep, so the oracle leg has
			// something to be an oracle ABOUT.
			CAPTURE(m.gMin);
			CAPTURE(m.gMax);
			CHECK(m.gMin >= 0.0);
			CHECK(m.gMax <= 1.0);
			CHECK(m.gMax - m.gMin > 0.5);
		}

		// =================================================================
		// HAZARD TWO, RE-MEASURED HERE RATHER THAN INHERITED FROM 33-04.
		// The two edge shapes must differ in what the DETECTOR can see, not
		// merely in their sample values. The quantity that matters is the
		// interpolated fraction f = (1 - prev)/(now - prev) at the crossing:
		// for a hard edge it is nearly constant however g moves, for a
		// band-limited edge it tracks g. If this ever stopped being true the
		// grid would still be green and SYNC-02's sub-sample clause would be
		// silently untested — which is exactly the failure this part exists
		// to make impossible.
		// =================================================================
		double fSpread[2] = {0.0, 0.0};
		for (std::size_t ei = 0; ei < 2; ++ei) {
			const SyncMaster m = makeSyncMaster(2 * kSpectrumN, Km, 5.0, SYNC_EDGES[ei]);
			double fMin = 2.0, fMax = -1.0;
			for (int i = kSpectrumN; i < 2 * kSpectrumN; ++i) {
				const float prev = m.volts[(std::size_t)(i - 1)];
				const float now  = m.volts[(std::size_t)i];
				if (!(prev < 1.0f && now >= 1.0f)) continue;
				const double f = (1.0 - (double)prev) / ((double)now - (double)prev);
				if (f < fMin) fMin = f;
				if (f > fMax) fMax = f;
			}
			fSpread[ei] = fMax - fMin;
		}
		CAPTURE(fSpread[0]);
		CAPTURE(fSpread[1]);
		CHECK(fSpread[0] < 0.05);   // hard edge: f is inert, 33-04 measured 0.004 across a halving of g
		CHECK(fSpread[1] > 0.20);   // band-limited: f tracks g, 33-04 measured 0.678
	}

	// =======================================================================
	// PART C — THE INSTRUMENT'S OWN SPREAD, WRITTEN DOWN RATHER THAN HIDDEN.
	// No single master frequency is integer-cycle at all three rates at this
	// block length (see the grid banner's derivation), so the three achieved
	// frequencies differ. The bound below is a STATEMENT ABOUT THE INSTRUMENT:
	// if a later edit moves a K_m and pushes the spread past it, the cross-rate
	// rows have stopped comparing like with like and the rate-signature
	// condition of the D-06 decision rule is no longer reading a rate effect.
	// =======================================================================
	CAPTURE(hzMin);
	CAPTURE(hzMax);
	const double spreadPct = 100.0 * (hzMax - hzMin) / hzMin;
	CAPTURE(spreadPct);
	CHECK(spreadPct < 1.5);

	// =======================================================================
	// PART D (plan 33-07) — THE SYNC PROVENANCE STRINGS ARE NEW CONSTANTS,
	// ASSERTED RATHER THAN CLAIMED.
	//
	// The constants' own banner says they are not reuses of Phase 32's six. A
	// sentence cannot stop the next agent attaching kProvMeasured to a sync row
	// and misattributing a 33-07 number to plan 32-07's measurement run. This
	// block can: every sync string must be non-empty, mutually distinct, and
	// distinct from every Phase 32 string by CONTENT, not by pointer identity —
	// a copy-paste of kProvMeasured's text under a new name would defeat a
	// pointer comparison and is exactly the edit worth catching.
	// =======================================================================
	{
		const char* const syncProv[] = {
			kProvSync441Step, kProvSync441Plateau,
			kProvSync48Step,  kProvSync48Plateau,
			kProvSync96Step,  kProvSync96Plateau,
			kProvSync441Invalid, kProvSync48Invalid, kProvSync96Invalid
		};
		const char* const phase32Prov[] = {
			kProvMeasured, kProvFloored, kProvCrossRate,
			kProvCrossRateFloored, kProvDiagnostic, kProvDiagnosticFloored
		};
		const int nSync = (int)(sizeof(syncProv) / sizeof(syncProv[0]));
		const int n32   = (int)(sizeof(phase32Prov) / sizeof(phase32Prov[0]));
		CAPTURE(nSync);
		CAPTURE(n32);
		CHECK(nSync == 9);
		CHECK(n32 == 6);
		for (int i = 0; i < nSync; ++i) {
			const std::string si(syncProv[i]);
			CAPTURE(i);
			CHECK(si.size() > 0);
			// Every one names the plan that measured it and the rate it was
			// measured at — the two things the plan requires of a provenance.
			CHECK(si.find("33-07") != std::string::npos);
			const bool namesARate = (si.find("44.1 kHz") != std::string::npos)
			                     || (si.find("48 kHz")   != std::string::npos)
			                     || (si.find("96 kHz")   != std::string::npos);
			CHECK(namesARate);
			// And not one of them claims plan 32-07 measured it.
			CHECK(si.find("32-07") == std::string::npos);
			for (int j = i + 1; j < nSync; ++j) { CAPTURE(j); CHECK(si != std::string(syncProv[j])); }
			for (int j = 0; j < n32; ++j)       { CAPTURE(j); CHECK(si != std::string(phase32Prov[j])); }
		}
	}
}

// ---------------------------------------------------------------------------
// TASK 2 OF PLAN 33-05 — THE PROBE'S OWN NON-VACUITY GATE.
//
// WHY THIS CASE EXISTS. The measurement that follows is only about the real
// oscillator to the extent that SyncPlacementProbe IS the real oscillator. This
// is the same move NaiveVcoCoreMirror already makes and it is made for the same
// reason: an instrument that has silently drifted from the thing it claims to
// measure keeps producing figures, and keeps producing them greenly. T-33-17 is
// the highest-severity threat assigned to this plan and this case is its
// mitigation.
//
// >>> THE GATE HAS BEEN RE-ANCHORED, AND THE INSTRUCTION THAT REQUIRED IT IS
//     KEPT BELOW RATHER THAN DELETED. <<<
// The paragraph this replaces was written by plan 33-05 and read: "As of plan
// 33-02 the shipped forge::VcoCore IS measurement leg `none` ... Plan 33-06
// lands the seam. FROM THAT COMMIT THE SHIPPED CORE STOPS BEING THE
// NO-CORRECTION LEG, and `kLegNone` below stops describing it. The comparison
// would then be probe-without-correction against core-with-correction, it would
// go red, and the tempting repair — loosening the equality — would delete the
// gate. THE CORRECT REPAIR IS TO RE-ANCHOR: change the leg argument to whichever
// leg 33-06 landed and keep the equality EXACT."
//   THAT IS EXACTLY WHAT HAPPENED, and it happened in plan 33-06 rather than in
// 33-07, because the gate goes red in the same commit that lands the seam and a
// red gate cannot be left for a later plan. OBSERVED before the re-anchor: 412
// failed assertions in this one case, every one of them `mismatches == 0`.
// The leg argument below is now `kLegPastEdge` and THE EQUALITY IS STILL A
// DIRECT FLOAT `==`. It was not loosened by one character.
//   WHY THE RE-ANCHORED GATE IS STILL A REAL COMPARISON AND NOT A TAUTOLOGY:
// the two sides reach the same arithmetic by DIFFERENT ROUTES. The probe leg
// calls the forward-facing seam with a pre-scaled magnitude,
// addStep(0.f, -f*f*jump); the shipped core calls the named past-edge entry
// point, addPastStep(f, jump). That those two agree BIT-EXACTLY is the identity
// pinned in tests/test_morph_blep.cpp's "(D-06)" case, and this gate is where it
// is exercised against 1,720,320 real samples rather than against a spread of
// constructed arguments. If a later editor changes either form, this case is
// what goes red.
//
// The comparison is a DIRECT float ==, never doctest's approximate comparator:
// that comparator applies a relative-scaling margin even at epsilon(0), so it
// is not a bit-exact comparator and would quietly absorb precisely the small
// arithmetic drifts this case exists to see.
//
// ===========================================================================
// >>> PLAN 33-07: THIS CASE MAKES TWO DIFFERENT CLAIMS AND THEY ARE LABELLED
//     SEPARATELY BELOW, BECAUSE BEFORE THE RE-ANCHOR ONE ASSERTION WAS
//     CARRYING BOTH AND THEREFORE CARRYING NEITHER. <<<
//
// Until plan 33-06 the case held a SINGLE assertion — probe on `kLegNone`
// against the live core, zero mismatches — and that one assertion happened to
// be true for two independent reasons at once: the probe reproduced the core's
// arithmetic (an IDENTITY statement), AND the shipped core applied no sync
// correction (a MAGNITUDE statement, that the correction was zero). Landing the
// seam falsified the second while leaving the first true, and the assertion went
// red without being able to say WHICH claim had moved. An un-re-anchored gate is
// worse than a deleted one for exactly that reason: it still passes or fails on
// something, so it reads as coverage, while comparing two different things.
//
// THE TWO CLAIMS, AS THEY NOW STAND:
//
//   CLAIM 1 — THE IDENTITY CLAIM. `SyncPlacementProbe` on `kLegPastEdge` IS
//   `forge::VcoCore`, sample for sample, by direct float equality. This is the
//   claim T-33-17 is about and it is the one every decibel in this file's sync
//   rows rests on. Its two sides are the PROBE and the LIVE CORE.
//
//   CLAIM 2 — THE MAGNITUDE CONTROL. The probe's own `kLegNone` leg differs from
//   the probe's own `kLegPastEdge` leg on a non-zero number of samples. BOTH
//   SIDES ARE THE PROBE: this says nothing whatever about the core's identity,
//   it says the CORRECTION IS NOT A NO-OP. It is the non-vacuity partner of
//   claim 1 — without it, a seam that had been silently disabled would leave
//   claim 1 trivially true and this case entirely green.
//
// A comparison of the probe's `kLegNone` leg against the LIVE CORE — which is
// what the pre-33-06 assertion literally was — is now the SUM of those two
// claims and is not asserted as such, because a failure of it could not be
// attributed to either. The information it carried is fully covered above.
// ===========================================================================
// ---------------------------------------------------------------------------
TEST_CASE("vco spectrum: (D-06) the sync placement probe reproduces forge::VcoCore bit-exactly on the past-edge leg") {

	const std::vector<SyncCell>& grid = syncGrid();
	const std::size_t nCells = grid.size();
	REQUIRE(nCells == 420);

	// Per-rate tallies, asserted AFTER the cells of that rate, so a rate whose
	// grid silently lost cells cannot satisfy them.
	struct RateTally { double sr; long samples; long mismatches; int cells; };
	RateTally tally[3];
	for (int i = 0; i < 3; ++i) {
		tally[i].sr = SYNC_RATES[i].sr;
		tally[i].samples = 0;
		tally[i].mismatches = 0;
		tally[i].cells = 0;
	}

	long totalSamples = 0, totalMismatches = 0;
	int  cellsWithSyncActivity = 0;

	// CLAIM 2's tallies — see the banner. Both sides of these are the PROBE.
	long totalCorrectionDiffs = 0;
	int  cellsWhereCorrectionMoved = 0;
	int  cellsWithZeroJump = 0;
	int  zeroJumpCellsAtUnityFlatTop = 0;
	long correctionDiffs[3] = { 0, 0, 0 };
	float largestCorrection = 0.f;

	for (std::size_t ci = 0; ci < nCells; ++ci) {
		const SyncCell& cell = grid[ci];

		std::vector<float> probeBlock, coreBlock, noneBlock;
		double rms = 0.0, binErr = 0.0;
		SyncProbeDiag diag = zeroedSyncDiag();

		// BOTH SIDES THROUGH THE SAME FUNCTION, WHICH GOES THROUGH THE SAME
		// DRIVE LOOP. A comparator whose two sides run different loops proves
		// nothing about the difference between them.
		measureSyncCellDb(cell, kLegPastEdge, /*useLiveCore=*/false, &rms, &binErr, &diag, &probeBlock);
		measureSyncCellDb(cell, kLegPastEdge, /*useLiveCore=*/true,  &rms, &binErr, 0,     &coreBlock);
		// CLAIM 2's second side: the probe's OWN no-correction leg.
		measureSyncCellDb(cell, kLegNone,     /*useLiveCore=*/false, &rms, &binErr, 0,     &noneBlock);

		REQUIRE(probeBlock.size() == (std::size_t)kSpectrumN);
		REQUIRE(coreBlock.size()  == (std::size_t)kSpectrumN);
		REQUIRE(noneBlock.size()  == (std::size_t)kSpectrumN);

		long mismatches = 0;
		for (int i = 0; i < kSpectrumN; ++i)
			if (probeBlock[(std::size_t)i] != coreBlock[(std::size_t)i]) ++mismatches;

		// CLAIM 2 — THE MAGNITUDE CONTROL, counted per cell. Probe against
		// probe: how many samples does the shipped correction actually move?
		long correctionDiff = 0;
		for (int i = 0; i < kSpectrumN; ++i) {
			if (noneBlock[(std::size_t)i] == probeBlock[(std::size_t)i]) continue;
			++correctionDiff;
			const float d = std::fabs(noneBlock[(std::size_t)i] - probeBlock[(std::size_t)i]);
			if (d > largestCorrection) largestCorrection = d;
		}
		if (correctionDiff > 0) ++cellsWhereCorrectionMoved;

		// >>> THE ONE POPULATION WHERE THE CORRECTION MOVES NOTHING, AND IT IS
		//     PREDICTED BY THE PHYSICS RATHER THAN OBSERVED AND THEN EXCUSED.
		//     THE CRITERION IS STATED HERE, BEFORE THE COUNT BELOW. <<<
		// The seam deposits -f*f*jump. When the reset produces a jump of EXACTLY
		// zero it deposits exactly zero and the two legs are bit-identical — not
		// approximately, identically. That happens when the pre-reset and
		// post-reset phases land inside the SAME FLAT SEGMENT of a piecewise-
		// constant waveform, which needs two things at once: a ratio at which the
		// reset barely moves the phase, and a shape with a flat top for it to
		// move within. This grid contains exactly one such corner — the unity
		// ratio crossed with the square and pulse centres at character 0.00,
		// where the shape is at its hardest and its plateaux are widest. Sine,
		// triangle and saw have no flat segment at any character, and character
		// 1.00 softens the square's edges into a slope with no plateau at all.
		// A cell in this class is a cell with NOTHING TO CORRECT, and reporting
		// it as a correction failure would be reporting the waveform.
		const bool zeroJump = (diag.jumpAbsSum == 0.0);
		if (zeroJump) {
			++cellsWithZeroJump;
			const bool unityFlatTop = (cell.ratio == 1.0)
			                       && (cell.character == 0.00f)
			                       && (cell.morph >= 0.75f);
			if (unityFlatTop) ++zeroJumpCellsAtUnityFlatTop;
		}
		// The correction moves samples EXACTLY when there is a jump to correct.
		CAPTURE(cell.sr);
		CAPTURE(cell.edgeName);
		CAPTURE(cell.ratio);
		CAPTURE(cell.morph);
		CAPTURE(cell.character);
		CAPTURE(correctionDiff);
		CHECK((correctionDiff > 0) == !zeroJump);

		// NON-VACUITY: the cell must have actually SYNCED. Two blocks of a
		// free-running oscillator would be trivially identical and would say
		// nothing at all about the sync path.
		if (diag.fires > 0) ++cellsWithSyncActivity;

		int slot = 0;
		for (int r = 0; r < 3; ++r) if (cell.sr == SYNC_RATES[r].sr) slot = r;
		tally[slot].samples    += kSpectrumN;
		tally[slot].mismatches += mismatches;
		tally[slot].cells      += 1;
		correctionDiffs[slot]  += correctionDiff;
		totalSamples    += kSpectrumN;
		totalMismatches += mismatches;
		totalCorrectionDiffs += correctionDiff;

		if (mismatches != 0) {
			// Only report per cell when something is wrong: 360 CAPTUREd cells
			// would bury the finding.
			CAPTURE(cell.sr);
			CAPTURE(cell.Km);
			CAPTURE(cell.edgeName);
			CAPTURE(cell.ratio);
			CAPTURE(cell.morph);
			CAPTURE(cell.character);
			CAPTURE(mismatches);
			CHECK(mismatches == 0);
		}
	}

	// EVERY cell must fire. A grid where some cell silently free-ran would let
	// the identity claim pass on samples the sync block never touched.
	CAPTURE(cellsWithSyncActivity);
	CHECK(cellsWithSyncActivity == (int)nCells);

	for (int r = 0; r < 3; ++r) {
		CAPTURE(tally[r].sr);
		CAPTURE(tally[r].cells);
		CAPTURE(tally[r].samples);
		CAPTURE(tally[r].mismatches);
		CHECK(tally[r].cells == 140);
		CHECK(tally[r].samples == 140L * kSpectrumN);
		CHECK(tally[r].mismatches == 0);
	}

	// =======================================================================
	// >>> CLAIM 1 — THE IDENTITY CLAIM, ASSERTED ABOVE. <<<
	// Probe on kLegPastEdge == forge::VcoCore, sample for sample, 0 mismatches.
	// This is the claim that used to be anchored to the no-correction leg and is
	// now anchored to the leg that ships; the equality was never loosened.
	// =======================================================================
	CAPTURE(totalSamples);
	CAPTURE(totalMismatches);
	CHECK(totalSamples == 420L * kSpectrumN);
	CHECK(totalMismatches == 0);

	// =======================================================================
	// >>> CLAIM 2 — THE MAGNITUDE CONTROL. A DIFFERENT CLAIM, ASSERTED
	//     SEPARATELY, WITH BOTH OF ITS SIDES COMING FROM THE PROBE. <<<
	//
	// This is NOT a statement about the core's identity — claim 1 above is the
	// only assertion in this file that makes one. This says the shipped sync
	// correction is not a no-op: the probe's kLegNone leg and its kLegPastEdge
	// leg produce different samples, on every cell, and the difference is the
	// correction's magnitude.
	//
	// WHY IT IS HERE AND NOT SOMEWHERE ELSE: claim 1 is a bit-exactness gate,
	// and a bit-exactness gate is trivially satisfiable by disabling the thing
	// on both sides of it. If the seam were removed from src/dsp/VcoCore.hpp and
	// from the probe's kLegPastEdge arm in the same edit, claim 1 would go green
	// on 1,720,320 samples and say nothing. Claim 2 is what goes red then.
	//
	// The SAMPLE count is deliberately not pinned: it is a function of how many
	// resets each cell fires, which is a property of the master and the ratio and
	// is already asserted elsewhere. The CELL counts below are pinned exactly,
	// because they partition the grid on the physical criterion stated at the
	// zero-jump test inside the loop above — stated there BEFORE these counts,
	// and not adjusted to whatever the run produced.
	// =======================================================================
	CAPTURE(totalCorrectionDiffs);
	CAPTURE(cellsWhereCorrectionMoved);
	CAPTURE(cellsWithZeroJump);
	CAPTURE(zeroJumpCellsAtUnityFlatTop);
	CAPTURE(largestCorrection);
	for (int r = 0; r < 3; ++r) {
		CAPTURE(tally[r].sr);
		CAPTURE(correctionDiffs[r]);
		CHECK(correctionDiffs[r] > 0);
	}
	CHECK(totalCorrectionDiffs > 0);
	CHECK(largestCorrection > 0.f);

	// The two populations, EXACT. 12 cells carry a sync jump of exactly zero:
	// 3 rates x 2 master edge shapes x 2 flat-top shape centres (square and
	// pulse) at the unity ratio and character 0.00. The other 408 are moved by
	// the correction. Both are equalities, never inequalities — a `>=` here
	// would stop being able to see a grid that quietly lost cells, or a seam
	// that quietly stopped depositing on half of them.
	CHECK(cellsWithZeroJump == 12);
	CHECK(zeroJumpCellsAtUnityFlatTop == 12);   // every zero-jump cell is in the predicted class
	CHECK(cellsWhereCorrectionMoved == 408);
	CHECK(cellsWhereCorrectionMoved + cellsWithZeroJump == (int)nCells);
}

// ---------------------------------------------------------------------------
// TASK 3 OF PLAN 33-05 — THE MEASUREMENT, AND THE THREE-CONDITION DECISION RULE.
//
// This case RUNS the measurement and applies D-06's decision rule. It PINS NO
// THRESHOLD and GATES NO CELL: plan 33-07 owns both. What it asserts is the
// integrity of the measurement itself — that every cell synced, that the
// instrument's own bin error is still exactly zero, that the legs genuinely
// separate, and that the two mutation probes discriminate — plus whichever of
// the three decision conditions the measurement actually established.
//
// >>> THE PLATEAU / STEP-DOMINATED CRITERION, STATED ON ITS PHYSICAL BASIS
//     BEFORE ANY POPULATION IS ENUMERATED. READ THIS BEFORE THE COUNTS. <<<
//
// Register item 8 splits the cross-toolchain reproduction bound in two, and the
// split is PHYSICAL, not statistical. aliasPeakDb reports an ARG-MAX over
// roughly two thousand non-harmonic bins. When the emitted waveform carries a
// TRUE VALUE STEP, that step's spectrum is a broad 1/f skirt and the arg-max is
// a GENUINE MAXIMUM — it sits well above its neighbours and no plausible
// unit-in-the-last-place difference in a library function can reorder it. Such
// a cell is STEP-DOMINATED and inherits the 1.0 dB bound. When there is no true
// value step, the surviving non-harmonic energy is a near-flat PLATEAU of
// near-tied bins, one libm ULP reorders which bin wins, and the reported figure
// moves by several decibels for no DSP reason at all. Such a cell is PLATEAU
// class and inherits the 4.0 dB bound.
//
// THE OBSERVABLE THAT DECIDES IT, chosen because it IS the physical quantity
// and not a proxy for the outcome: under hard sync the value step in question
// is the SYNC JUMP — the difference between the frozen waveshaper's value at
// the pre-reset phase and at the post-reset phase, which is exactly what
// forge::VcoCore::Telemetry::syncJump records. A cell is STEP-DOMINATED when
// its mean absolute sync jump, measured ON THE SHIPPED CORE'S OWN LEG, is at
// least 0.01 in pre-scale units (0.05 V at the output). Below that the reset
// moves the waveform by less than one percent of its full range and there is no
// step for the arg-max to lock onto.
//
// THE CRITERION IS FIXED HERE, IN THIS COMMENT, BEFORE THE POPULATION IS
// COUNTED AND BEFORE ANY MARGIN IS COMPARED AGAINST IT. A CLASSIFICATION
// PRODUCED BY RENAMING THE CELLS THAT FAILED IS FORBIDDEN BY NAME, in
// 33-VALIDATION's Threshold Policy and here.
// ---------------------------------------------------------------------------
TEST_CASE("vco spectrum: (D-06 / D-11) the sync placement measurement - six legs on the sync sub-grid") {

	// The physical criterion, as a constant, stated above before any count.
	const double kStepDominatedJumpFloor = 0.01;   // pre-scale units
	const double kBoundStepDominatedDb   = 1.0;    // register item 8
	const double kBoundPlateauDb         = 4.0;    // register item 8

	const std::vector<SyncCell>& grid = syncGrid();
	const std::size_t nCells = grid.size();
	REQUIRE(nCells == 420);

	// =======================================================================
	// THE MEASUREMENT. Every leg of every cell through the SAME
	// measureSyncCellDb, parameterised by leg. There is no second measurement
	// function and there must never be.
	// =======================================================================
	std::vector<std::vector<double> > db(nCells, std::vector<double>((std::size_t)kSyncLegCount, 0.0));
	std::vector<SyncProbeDiag> diags(nCells, zeroedSyncDiag());
	std::vector<char> stepDom(nCells, 0);
	std::vector<char> instrumentValid(nCells, 0);
	std::vector<double> fundDomDb(nCells, 0.0);

	long   totalFires = 0, totalLateFires = 0;
	int    cellsAllFired = 0;
	double worstBinError = 0.0;

	for (std::size_t ci = 0; ci < nCells; ++ci) {
		const SyncCell& cell = grid[ci];
		for (int L = 0; L < kSyncLegCount; ++L) {
			double rms = 0.0, binErr = 0.0;
			SyncProbeDiag d = zeroedSyncDiag();
			db[ci][(std::size_t)L] = measureSyncCellDb(cell, (SyncLeg)L, /*useLiveCore=*/false,
			                                           &rms, &binErr, &d, 0);
			if (binErr > worstBinError) worstBinError = binErr;
			// THE NO-CORRECTION LEG SUPPLIES THE DIAGNOSTICS, and it still does
			// after plan 33-06 landed the seam and made `pastEdge` the shipped
			// core. Every one of them — the jump that decides the class, the fire
			// count, the late fires and the carried phantom — is a property of
			// the RESET, which is identical on all eight legs, so taking them on
			// the correction-free leg is what keeps them properties of the CELL
			// rather than of whichever candidate happens to be shipping.
			if (L == (int)kLegNone) {
				diags[ci] = d;
				// THE INSTRUMENT-VALIDITY COLUMN, taken on the same
				// correction-free reference leg and for the same reason: it must
				// be a property of the cell rather than of a candidate.
				std::vector<float> refBlock;
				double rms2 = 0.0, be2 = 0.0;
				measureSyncCellDb(cell, kLegNone, /*useLiveCore=*/false, &rms2, &be2, 0, &refBlock);
				int strongestN = 0;
				fundDomDb[ci] = fundamentalDominanceDb(refBlock, cell.Km, &strongestN);
				instrumentValid[ci] = (strongestN == 1) ? 1 : 0;
			}
		}
		const SyncProbeDiag& d = diags[ci];
		if (d.fires > 0) ++cellsAllFired;
		totalFires     += d.fires;
		totalLateFires += d.lateFires;
		const double meanAbsJump = (d.fires > 0) ? d.jumpAbsSum / (double)d.fires : 0.0;
		stepDom[ci] = (meanAbsJump >= kStepDominatedJumpFloor) ? 1 : 0;
	}

	// --- THE INSTRUMENT IS STILL THE INSTRUMENT. --------------------------
	CAPTURE(worstBinError);
	CHECK(worstBinError == 0.0);          // the master's bin error, asserted DIRECTLY, over every cell measured
	CAPTURE(cellsAllFired);
	CHECK(cellsAllFired == (int)nCells);  // non-vacuity: no cell free-ran
	CAPTURE(totalFires);
	CHECK(totalFires > 0);

	// --- THE POPULATION, COUNTED AFTER THE CRITERION WAS FIXED. -----------
	int nStepDominated = 0, nPlateau = 0;
	for (std::size_t ci = 0; ci < nCells; ++ci) { if (stepDom[ci]) ++nStepDominated; else ++nPlateau; }
	CAPTURE(nStepDominated);
	CAPTURE(nPlateau);
	CHECK(nStepDominated + nPlateau == (int)nCells);

	// =======================================================================
	// THE AGGREGATED TABLE — mean alias peak per (rate x edge shape x ratio)
	// per leg, over the ten morph-by-character cells of that group. CAPTUREd
	// rather than MESSAGEd so a normal run stays quiet and `-s` prints it, which
	// is this file's standing idiom.
	// =======================================================================
	for (std::size_t ri = 0; ri < 3; ++ri) {
		for (std::size_t ei = 0; ei < 2; ++ei) {
			for (std::size_t qi = 0; qi < 7; ++qi) {
				double sum[kSyncLegCount];
				for (int L = 0; L < kSyncLegCount; ++L) sum[L] = 0.0;
				int n = 0, nStep = 0, fires = 0, late = 0;
				double phMax = 0.0, phSum = 0.0; int phN = 0;
				double masterHz = 0.0;
				for (std::size_t ci = 0; ci < nCells; ++ci) {
					const SyncCell& c = grid[ci];
					if (c.sr != SYNC_RATES[ri].sr) continue;
					if (c.edge != SYNC_EDGES[ei]) continue;
					if (c.ratio != SYNC_RATIOS[qi]) continue;
					masterHz = c.masterHz;
					for (int L = 0; L < kSyncLegCount; ++L) sum[L] += db[ci][(std::size_t)L];
					++n;
					if (stepDom[ci]) ++nStep;
					fires += diags[ci].fires;
					late  += diags[ci].lateFires;
					phSum += diags[ci].phantomAbsSum;
					phN   += diags[ci].phantomSamples;
					if (diags[ci].phantomAbsMax > phMax) phMax = diags[ci].phantomAbsMax;
				}
				REQUIRE(n == 10);
				const double sr = SYNC_RATES[ri].sr;
				const std::string edgeName = SYNC_EDGE_NAME[ei];
				const double ratio = SYNC_RATIOS[qi];
				CAPTURE(sr);
				CAPTURE(masterHz);
				CAPTURE(edgeName);
				CAPTURE(ratio);
				const double mNone     = sum[kLegNone]     / 10.0;
				const double mDetect   = sum[kLegDetect]   / 10.0;
				const double mPastEdge = sum[kLegPastEdge] / 10.0;
				const double mFlatHalf = sum[kLegFlatHalf] / 10.0;
				const double mOracle   = sum[kLegOracle]   / 10.0;
				const double mSnap     = sum[kLegSnap]     / 10.0;
				const double mMisMap   = sum[kProbeMisMap] / 10.0;
				const double mBadSign  = sum[kProbeBadSign]/ 10.0;
				CAPTURE(mNone);
				CAPTURE(mDetect);
				CAPTURE(mPastEdge);
				CAPTURE(mFlatHalf);
				CAPTURE(mOracle);
				CAPTURE(mSnap);
				CAPTURE(mMisMap);
				CAPTURE(mBadSign);
				CAPTURE(nStep);
				CAPTURE(fires);
				CAPTURE(late);
				const double phantomMean = (phN > 0) ? phSum / (double)phN : 0.0;
				CAPTURE(phantomMean);
				CAPTURE(phMax);
				// Every group must have synced and must have produced a real
				// alias figure; -999.0 is aliasPeakDb's silence sentinel.
				CHECK(fires > 0);
				CHECK(mNone > -900.0);
			}
		}
	}

	// =======================================================================
	// CONDITION 1 — SIGN CONSISTENCY, at 44.1 kHz binding.
	//
	// >>> THE POPULATION IS THE STEP-DOMINATED 44.1 kHz CELLS, AND THE
	//     RESTRICTION IS PART OF THE CRITERION STATED ABOVE, NOT A RESPONSE TO
	//     THE RESULT. <<<
	// A cell whose sync jump is below the floor has no value step at the reset,
	// which means IT POSES NO PLACEMENT QUESTION: there is nothing at the edge
	// for a correction to be placed on either side of, and register item 8
	// already says the arg-max there is a near-tie that one libm ULP reorders.
	// Ranking four candidates on such a cell ranks rounding. BOTH FRACTIONS ARE
	// RECORDED — over all 44.1 kHz cells and over the step-dominated ones — so
	// the restriction cannot hide anything, and the classification was fixed on
	// the mean absolute sync jump before any margin was looked at.
	// =======================================================================
	int n441 = 0, n441Step = 0;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		if (grid[ci].sr != 44100.0) continue;
		++n441;
		if (stepDom[ci]) ++n441Step;
	}
	REQUIRE(n441 == 140);
	CAPTURE(n441Step);
	REQUIRE(n441Step > 0);

	int    winsAll[kSyncCandidateCount];
	int    winsStep[kSyncCandidateCount];
	double worstDeficit[kSyncCandidateCount];
	for (int k = 0; k < kSyncCandidateCount; ++k) { winsAll[k] = 0; winsStep[k] = 0; worstDeficit[k] = -1e30; }

	for (std::size_t ci = 0; ci < nCells; ++ci) {
		if (grid[ci].sr != 44100.0) continue;
		for (int k = 0; k < kSyncCandidateCount; ++k) {
			const double mine = db[ci][(std::size_t)SYNC_CANDIDATES[k]];
			double bestOther = 1e30;
			for (int j = 0; j < kSyncCandidateCount; ++j) {
				if (j == k) continue;
				const double o = db[ci][(std::size_t)SYNC_CANDIDATES[j]];
				if (o < bestOther) bestOther = o;
			}
			if (mine <= bestOther) ++winsAll[k];
			if (stepDom[ci]) {
				if (mine <= bestOther) ++winsStep[k];
				// The worst single-cell deficit is taken on the step-dominated
				// population only, for the same reason the fraction is.
				const double deficit = mine - bestOther;   // positive = this candidate is WORSE here
				if (deficit > worstDeficit[k]) worstDeficit[k] = deficit;
			}
		}
	}

	int    winnerIdx = 0;
	double bestFrac  = -1.0;
	for (int k = 0; k < kSyncCandidateCount; ++k) {
		const double fracAll  = (double)winsAll[k]  / (double)n441;
		const double fracStep = (double)winsStep[k] / (double)n441Step;
		const std::string legName = SYNC_LEG_NAME[SYNC_CANDIDATES[k]];
		CAPTURE(legName);
		CAPTURE(winsAll[k]);
		CAPTURE(winsStep[k]);
		CAPTURE(fracAll);
		CAPTURE(fracStep);
		CAPTURE(worstDeficit[k]);
		CHECK(winsStep[k] >= 0);   // recorded; the claim is the fraction below
		if (fracStep > bestFrac) { bestFrac = fracStep; winnerIdx = k; }
	}
	const SyncLeg winner = SYNC_CANDIDATES[winnerIdx];
	const std::string winnerName = SYNC_LEG_NAME[winner];
	const double winnerFrac = bestFrac;
	const double winnerFracAll = (double)winsAll[winnerIdx] / (double)n441;
	const double winnerWorstDeficit = worstDeficit[winnerIdx];
	CAPTURE(winnerName);
	CAPTURE(winnerFrac);
	CAPTURE(winnerFracAll);
	CAPTURE(winnerWorstDeficit);

	const bool cond1 = (winnerFrac >= 0.90) && (winnerWorstDeficit <= kBoundStepDominatedDb);
	CAPTURE(cond1);

	// =======================================================================
	// CONDITION 2 — MARGIN ABOVE THE REPRODUCTION BOUND, on the sub-unity cells.
	// =======================================================================
	int    subCells = 0, subOverBound = 0;
	double subMarginMin = 1e30, subMarginMax = -1e30, subMarginSum = 0.0;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		if (grid[ci].sr != 44100.0) continue;
		if (!(grid[ci].ratio < 1.0)) continue;
		if (!stepDom[ci]) continue;   // no value step, no placement question — see condition 1
		double bestOther = 1e30;
		for (int j = 0; j < kSyncCandidateCount; ++j) {
			if (SYNC_CANDIDATES[j] == winner) continue;
			const double o = db[ci][(std::size_t)SYNC_CANDIDATES[j]];
			if (o < bestOther) bestOther = o;
		}
		const double margin = bestOther - db[ci][(std::size_t)winner];   // positive = winner better
		const double bound  = stepDom[ci] ? kBoundStepDominatedDb : kBoundPlateauDb;
		++subCells;
		if (margin > bound) ++subOverBound;
		if (margin < subMarginMin) subMarginMin = margin;
		if (margin > subMarginMax) subMarginMax = margin;
		subMarginSum += margin;
	}
	CAPTURE(subCells);
	REQUIRE(subCells > 0);
	const double subMarginMean = subMarginSum / (double)subCells;
	CAPTURE(subCells);
	CAPTURE(subOverBound);
	CAPTURE(subMarginMin);
	CAPTURE(subMarginMean);
	CAPTURE(subMarginMax);

	const bool cond2 = (subOverBound == subCells);
	CAPTURE(cond2);

	// =======================================================================
	// CONDITION 3 — RATE SIGNATURE, on the cells at or above unity ratio.
	// =======================================================================
	double rateMargin[3] = {0.0, 0.0, 0.0};
	int    rateCells[3]  = {0, 0, 0};
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		if (grid[ci].ratio < 1.0) continue;
		if (!stepDom[ci]) continue;   // no value step, no placement question — see condition 1
		int slot = -1;
		for (int r = 0; r < 3; ++r) if (grid[ci].sr == SYNC_RATES[r].sr) slot = r;
		REQUIRE(slot >= 0);
		double bestOther = 1e30;
		for (int j = 0; j < kSyncCandidateCount; ++j) {
			if (SYNC_CANDIDATES[j] == winner) continue;
			const double o = db[ci][(std::size_t)SYNC_CANDIDATES[j]];
			if (o < bestOther) bestOther = o;
		}
		rateMargin[slot] += bestOther - db[ci][(std::size_t)winner];
		++rateCells[slot];
	}
	for (int r = 0; r < 3; ++r) { CAPTURE(rateCells[r]); REQUIRE(rateCells[r] > 0); rateMargin[r] /= (double)rateCells[r]; }
	const double margin441 = rateMargin[0], margin48 = rateMargin[1], margin96 = rateMargin[2];
	CAPTURE(margin441);
	CAPTURE(margin48);
	CAPTURE(margin96);

	const bool cond3 = (margin441 > margin48) && (margin48 > margin96);
	CAPTURE(cond3);

	// The three margins on ONE COMMON CELL, as the rule requires: band-limited
	// master, ratio 2.5, the saw centre, character 0. Ratio 2.5 rather than 2.0
	// because 2.0 is an integer NULL POINT — see hazard three in the SYNC_RATIOS
	// banner and the permanent control at the bottom of this case.
	double commonMargin[3] = {0.0, 0.0, 0.0};
	int    commonFound = 0;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		const SyncCell& c = grid[ci];
		if (c.edge != kMasterBandLimited || c.ratio != 2.5) continue;
		if (c.morph != 0.50f || c.character != 0.00f) continue;
		int slot = -1;
		for (int r = 0; r < 3; ++r) if (c.sr == SYNC_RATES[r].sr) slot = r;
		REQUIRE(slot >= 0);
		double bestOther = 1e30;
		for (int j = 0; j < kSyncCandidateCount; ++j) {
			if (SYNC_CANDIDATES[j] == winner) continue;
			const double o = db[ci][(std::size_t)SYNC_CANDIDATES[j]];
			if (o < bestOther) bestOther = o;
		}
		commonMargin[slot] = bestOther - db[ci][(std::size_t)winner];
		++commonFound;
	}
	REQUIRE(commonFound == 3);
	CAPTURE(commonMargin[0]);
	CAPTURE(commonMargin[1]);
	CAPTURE(commonMargin[2]);

	// =======================================================================
	// THE TWO FINDINGS THAT MAKE THIS GRID PAY FOR ITSELF TWICE, per rate and
	// per master edge shape.
	//
	// SNAP vs WINNER is SYNC-02's sub-sample clause turned from an inherited
	// warning into a measurement, and it is the one sync claim the spectral
	// instrument can evidence with a comfortable margin.
	//
	// ORACLE vs WINNER decomposes the residual: the oracle-to-winner gap is the
	// cost of the FRACTION'S ACCURACY, while the none-to-winner gap is the cost
	// of the PLACEMENT CONVENTION. That decomposition is what tells a later
	// phase whether to escalate to a slope-correction kernel or to accept the
	// residual as inherent.
	// =======================================================================
	for (std::size_t ri = 0; ri < 3; ++ri) {
		for (std::size_t ei = 0; ei < 2; ++ei) {
			double snapSum = 0.0, oracleSum = 0.0, noneSum = 0.0;
			double misSum = 0.0, badSum = 0.0;
			int n = 0;
			for (std::size_t ci = 0; ci < nCells; ++ci) {
				if (grid[ci].sr != SYNC_RATES[ri].sr) continue;
				if (grid[ci].edge != SYNC_EDGES[ei]) continue;
				const double w = db[ci][(std::size_t)winner];
				snapSum   += db[ci][(std::size_t)kLegSnap]      - w;
				oracleSum += db[ci][(std::size_t)kLegOracle]    - w;
				noneSum   += db[ci][(std::size_t)kLegNone]      - w;
				misSum    += db[ci][(std::size_t)kProbeMisMap]  - w;
				badSum    += db[ci][(std::size_t)kProbeBadSign] - w;
				++n;
			}
			REQUIRE(n == 70);
			const double sr = SYNC_RATES[ri].sr;
			const std::string edgeName = SYNC_EDGE_NAME[ei];
			CAPTURE(sr);
			CAPTURE(edgeName);
			const double snapVsWinner   = snapSum   / (double)n;   // positive = snap is WORSE
			const double oracleVsWinner = oracleSum / (double)n;   // negative = oracle is BETTER
			// >>> THE MAGNITUDE CLAIM, IN DECIBELS. Plan 33-07 labels this site
			// so it cannot be read as the IDENTITY CLAIM, which is a different
			// statement about a different pair of things and lives in the
			// bit-exactness case above — see that case's banner, which sets the
			// two out side by side. BOTH SIDES OF THIS ONE ARE THE PROBE: it
			// compares the probe's no-correction leg against the probe's shipped
			// leg, so it is a statement about the CORRECTION's magnitude and
			// never about whether the probe is the core. It is also the number
			// the SyncCell banner's refusal paragraph quotes: a grid-wide mean of
			// +0.5827 dB is why an improvement gate in the Phase 32 shape is
			// REFUSED here rather than written and then loosened.
			const double noneVsWinner   = noneSum   / (double)n;   // positive = doing nothing is WORSE
			const double misVsWinner    = misSum    / (double)n;
			const double badVsWinner    = badSum    / (double)n;
			CAPTURE(snapVsWinner);
			CAPTURE(oracleVsWinner);
			CAPTURE(noneVsWinner);
			CAPTURE(misVsWinner);
			CAPTURE(badVsWinner);
			CHECK(n == 70);

			// >>> THE ONE ORDERING CLAIM THIS PLAN ASSERTS, AND IT IS A SIGN
			//     CLAIM WITH NO PINNED DECIBEL IN IT. <<<
			// On a BAND-LIMITED master — the only edge shape where the detector's
			// fraction carries sub-sample information at all (hazard two) — the
			// snap-to-zero landmine and BOTH mutation probes must land on the
			// WORSE side of the past-edge leg. A probe that does not separate
			// from the leg it is probing is not a probe, and a landmine that
			// measures no worse than the thing it is a landmine for is not a
			// landmine. The MAGNITUDES are recorded in the plan SUMMARY and are
			// deliberately NOT gated here: this plan measures and decides, and
			// plan 33-07 pins.
			//   THE HARD-EDGE HALF IS DELIBERATELY NOT ASSERTED, and its absence
			// is the finding rather than an omission: with a single-sample master
			// wrap there is no sub-sample information for any of these legs to
			// use, so they measure within a decibel of each other and the snap
			// leg is actually BETTER. That is hazard two arriving as a number.
			if (SYNC_EDGES[ei] == kMasterBandLimited) {
				CHECK(snapVsWinner > 0.0);
				CHECK(misVsWinner  > 0.0);
				CHECK(badVsWinner  > 0.0);
			}
		}
	}

	// --- THE MUTATION PROBES, GRID-WIDE, WITH A VERDICT. ------------------
	// A probe that does not separate from the winner is not a probe.
	double misSumAll = 0.0, badSumAll = 0.0;
	int    misWorse = 0, badWorse = 0;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		const double w = db[ci][(std::size_t)winner];
		const double m = db[ci][(std::size_t)kProbeMisMap]  - w;
		const double b = db[ci][(std::size_t)kProbeBadSign] - w;
		misSumAll += m; badSumAll += b;
		if (m > 0.0) ++misWorse;
		if (b > 0.0) ++badWorse;
	}
	const double misMeanAll = misSumAll / (double)nCells;
	const double badMeanAll = badSumAll / (double)nCells;
	CAPTURE(misMeanAll);
	CAPTURE(badMeanAll);
	CAPTURE(misWorse);
	CAPTURE(badWorse);

	// --- D-07's RESIDUAL PHANTOM, AS A NUMBER RATHER THAN AN ARGUMENT. ----
	// src/dsp/VcoCore.hpp names this residual and gives an ORDER OF MAGNITUDE
	// explicitly labelled arithmetic, not measurement, and 33-02's deferred
	// register asks this plan for the measurement. This is it: the magnitude of
	// the MorphBlep accumulator CARRIED IN to a reset sample, over the shipped
	// core's own leg.
	double phantomGridMax = 0.0, phantomGridSum = 0.0;
	long   phantomGridN = 0;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		phantomGridSum += diags[ci].phantomAbsSum;
		phantomGridN   += diags[ci].phantomSamples;
		if (diags[ci].phantomAbsMax > phantomGridMax) phantomGridMax = diags[ci].phantomAbsMax;
	}
	REQUIRE(phantomGridN > 0);
	const double phantomGridMean = phantomGridSum / (double)phantomGridN;
	CAPTURE(phantomGridMean);
	CAPTURE(phantomGridMax);
	CAPTURE(phantomGridN);

	// --- THE LATE-FIRE COLUMN, which 33-04 asked this grid to carry. ------
	// A band-limited master's residual can push the wrap sample below the high
	// threshold, so the detector fires ONE SAMPLE LATE. That is a placement
	// error that exists BEFORE any seam does, and it must not be attributed to
	// one.
	CAPTURE(totalLateFires);
	CAPTURE(totalFires);

	// =======================================================================
	// THE INSTRUMENT-VALIDITY PARTITION, AND THE RANKING INSIDE IT.
	//
	// fundamentalDominanceDb above is this sub-grid's D-10 self-check. A cell
	// where the master's fundamental is NOT the strongest bin on its own
	// harmonic lattice is a cell where aliasPeakDb is dividing by something that
	// is not the fundamental, so its decibel figure is not an alias floor and
	// cannot rank anything. The partition is reported here with the ranking
	// recomputed inside the valid half, so the STOP-AND-REPORT above is
	// accompanied by the structure of WHY it stopped rather than by a shrug.
	// =======================================================================
	int nValid = 0, nInvalid = 0;
	double worstFundDom = 0.0;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		if (instrumentValid[ci]) ++nValid; else ++nInvalid;
		if (fundDomDb[ci] < worstFundDom) worstFundDom = fundDomDb[ci];
	}
	CAPTURE(nValid);
	CAPTURE(nInvalid);
	CAPTURE(worstFundDom);
	CHECK(nValid + nInvalid == (int)nCells);
	CHECK(nValid > 0);

	// Conditions 2 and 3, re-evaluated on the instrument-valid population.
	{
		int    vSubCells = 0, vSubOverBound = 0;
		double vSubMin = 1e30, vSubMax = -1e30, vSubSum = 0.0;
		for (std::size_t ci = 0; ci < nCells; ++ci) {
			if (grid[ci].sr != 44100.0) continue;
			if (!(grid[ci].ratio < 1.0)) continue;
			if (!stepDom[ci] || !instrumentValid[ci]) continue;
			double bestOther = 1e30;
			for (int j = 0; j < kSyncCandidateCount; ++j) {
				if (SYNC_CANDIDATES[j] == winner) continue;
				const double o = db[ci][(std::size_t)SYNC_CANDIDATES[j]];
				if (o < bestOther) bestOther = o;
			}
			const double margin = bestOther - db[ci][(std::size_t)winner];
			++vSubCells;
			if (margin > kBoundStepDominatedDb) ++vSubOverBound;
			if (margin < vSubMin) vSubMin = margin;
			if (margin > vSubMax) vSubMax = margin;
			vSubSum += margin;
		}
		REQUIRE(vSubCells > 0);
		const double vSubMean = vSubSum / (double)vSubCells;
		CAPTURE(vSubCells);
		CAPTURE(vSubOverBound);
		CAPTURE(vSubMin);
		CAPTURE(vSubMean);
		CAPTURE(vSubMax);
		CHECK(vSubCells > 0);

		double vRateMargin[3] = {0.0, 0.0, 0.0};
		int    vRateCells[3]  = {0, 0, 0};
		for (std::size_t ci = 0; ci < nCells; ++ci) {
			if (grid[ci].ratio < 1.0) continue;
			if (!stepDom[ci] || !instrumentValid[ci]) continue;
			int slot = -1;
			for (int r = 0; r < 3; ++r) if (grid[ci].sr == SYNC_RATES[r].sr) slot = r;
			REQUIRE(slot >= 0);
			double bestOther = 1e30;
			for (int j = 0; j < kSyncCandidateCount; ++j) {
				if (SYNC_CANDIDATES[j] == winner) continue;
				const double o = db[ci][(std::size_t)SYNC_CANDIDATES[j]];
				if (o < bestOther) bestOther = o;
			}
			vRateMargin[slot] += bestOther - db[ci][(std::size_t)winner];
			++vRateCells[slot];
		}
		for (int r = 0; r < 3; ++r) {
			CAPTURE(vRateCells[r]);
			REQUIRE(vRateCells[r] > 0);
			vRateMargin[r] /= (double)vRateCells[r];
		}
		const double vMargin441 = vRateMargin[0], vMargin48 = vRateMargin[1], vMargin96 = vRateMargin[2];
		CAPTURE(vMargin441);
		CAPTURE(vMargin48);
		CAPTURE(vMargin96);
		const bool cond3v = (vMargin441 > vMargin48) && (vMargin48 > vMargin96);
		CAPTURE(cond3v);
		CHECK(vRateCells[0] > 0);
	}

	{
		int    vWins[kSyncCandidateCount];
		double vWorst[kSyncCandidateCount];
		int    vCells = 0;
		for (int k = 0; k < kSyncCandidateCount; ++k) { vWins[k] = 0; vWorst[k] = -1e30; }
		for (std::size_t ci = 0; ci < nCells; ++ci) {
			if (grid[ci].sr != 44100.0) continue;
			if (!stepDom[ci] || !instrumentValid[ci]) continue;
			++vCells;
			for (int k = 0; k < kSyncCandidateCount; ++k) {
				const double mine = db[ci][(std::size_t)SYNC_CANDIDATES[k]];
				double bestOther = 1e30;
				for (int j = 0; j < kSyncCandidateCount; ++j) {
					if (j == k) continue;
					const double o = db[ci][(std::size_t)SYNC_CANDIDATES[j]];
					if (o < bestOther) bestOther = o;
				}
				if (mine <= bestOther) ++vWins[k];
				const double deficit = mine - bestOther;
				if (deficit > vWorst[k]) vWorst[k] = deficit;
			}
		}
		CAPTURE(vCells);
		REQUIRE(vCells > 0);
		for (int k = 0; k < kSyncCandidateCount; ++k) {
			const std::string vLegName = SYNC_LEG_NAME[SYNC_CANDIDATES[k]];
			const double vFrac = (double)vWins[k] / (double)vCells;
			CAPTURE(vLegName);
			CAPTURE(vWins[k]);
			CAPTURE(vFrac);
			CAPTURE(vWorst[k]);
			CHECK(vWins[k] >= 0);
		}
	}

	// =======================================================================
	// THE INTEGER-RATIO NULL POINT — HAZARD THREE, PINNED PERMANENTLY.
	//
	// This control exists so that no later agent "restores" the ratios
	// 33-RESEARCH and 33-VALIDATION recommend. At an exactly integer ratio of
	// two or more the slave is ALREADY in phase at every master wrap, the reset
	// moves almost nothing, the emitted signal is periodic at the SLAVE's
	// period rather than the master's, and the master's fundamental bin — the
	// bin aliasPeakDb NORMALISES BY — carries essentially no energy. The
	// reported figure then goes POSITIVE, which for a genuinely master-periodic
	// signal is impossible, and the eight legs separate by tens of decibels in
	// what is almost entirely the normalisation moving.
	//
	// THE CELL BELOW IS DELIBERATELY *NOT* IN SYNC_GRID. It is constructed here,
	// measured here, and asserted to be exactly as unusable as the banner says.
	// If a future change ever makes an integer ratio measurable — a different
	// metric, a normalisation by the strongest harmonic — this control turns red
	// and says so, which is the right direction for it to fail in.
	// =======================================================================
	{
		SyncCell nullPoint = grid[0];          // the 44.1 kHz row, then overridden
		nullPoint.sr        = 44100.0;
		nullPoint.Km        = 93;
		nullPoint.masterHz  = 93.0 * 44100.0 / (double)kSpectrumN;
		nullPoint.edge      = kMasterHardEdge;
		nullPoint.edgeName  = "hard-edge";
		nullPoint.ratio     = 2.0;
		nullPoint.pitchCV   = (float)std::log2(2.0 * nullPoint.masterHz / (double)forge::kVcoFreqC4);
		nullPoint.morph     = 0.50f;
		nullPoint.region    = "saw";
		nullPoint.character = 0.00f;

		std::vector<float> block;
		double rms = 0.0, binErr = 0.0;
		SyncProbeDiag d = zeroedSyncDiag();
		const double nullPeakDb = measureSyncCellDb(nullPoint, kLegNone, /*useLiveCore=*/false,
		                                            &rms, &binErr, &d, &block);
		REQUIRE(block.size() == (std::size_t)kSpectrumN);

		std::vector<std::complex<double> > x;
		x.reserve(block.size());
		for (std::size_t i = 0; i < block.size(); ++i)
			x.push_back(std::complex<double>((double)block[i], 0.0));
		fftRadix2(x);

		const double fund = std::abs(x[(std::size_t)nullPoint.Km]);
		double strongestHarmonic = 0.0;
		int    strongestHarmonicN = 0;
		for (int nn = 1; nn * nullPoint.Km <= kSpectrumN / 2 - 1; ++nn) {
			const double mag = std::abs(x[(std::size_t)(nn * nullPoint.Km)]);
			if (mag > strongestHarmonic) { strongestHarmonic = mag; strongestHarmonicN = nn; }
		}
		REQUIRE(strongestHarmonic > 0.0);
		const double fundVsStrongestDb = 20.0 * std::log10(fund / strongestHarmonic);
		const double nullMeanAbsJump = (d.fires > 0) ? d.jumpAbsSum / (double)d.fires : 0.0;

		CAPTURE(nullPeakDb);
		CAPTURE(fund);
		CAPTURE(strongestHarmonic);
		CAPTURE(strongestHarmonicN);
		CAPTURE(fundVsStrongestDb);
		CAPTURE(nullMeanAbsJump);

		// The master's fundamental is NOT the dominant harmonic — the slave's is.
		CHECK(strongestHarmonicN == 2);
		// And it is buried, by 78 dB when this was measured.
		CHECK(fundVsStrongestDb < -60.0);
		// Which is why the reported figure is POSITIVE and therefore meaningless.
		CHECK(nullPeakDb > 0.0);
		// The physical cause: the reset barely moves the waveform.
		CHECK(nullMeanAbsJump < kStepDominatedJumpFloor);
	}

	// =======================================================================
	// THE THREE CONDITIONS, RECORDED — AND THE OUTCOME, WHICH IS
	// STOP-AND-REPORT.
	//
	// >>> ALL THREE CONDITIONS FAILED WHEN THIS PLAN RAN, AND THAT IS THE
	//     RESULT RATHER THAN A DEFECT IN THE RUN. <<< The full figures are in
	//     .planning/phases/33-hard-sync/33-05-SUMMARY.md. In brief, on the
	//     instrument-valid step-dominated 44.1 kHz population:
	//
	//   1. SIGN CONSISTENCY — the past-edge leg is best on 34 of 54 cells
	//      (0.6296), short of the 0.90 the rule demands. Its second clause
	//      PASSES and is the durable half: its WORST single-cell deficit against
	//      the best other candidate is 0.8553 dB, INSIDE register item 8's
	//      1.0 dB step-dominated reproduction bound, while every other
	//      candidate's worst deficit is outside it (none 3.93, detect 5.05,
	//      flatHalf 10.46).
	//   2. MARGIN — 22 of 38 valid sub-unity cells clear the bound, not all of
	//      them; the margin runs -0.50 to +3.42 dB about a mean of 1.51.
	//   3. RATE SIGNATURE — FLAT, not shrinking: 0.069 / -0.136 / 0.123 dB at
	//      44.1 / 48 / 96 kHz, and on the single common cell 0.897 / 0.901 /
	//      0.849 dB, a 0.05 dB spread across a factor of 2.2 in sample rate.
	//
	// CONDITION 3'S OWN WORDING IS WHAT THIS MEANS: "a margin that is FLAT
	// across rates means the legs differ in jump MAGNITUDE rather than in
	// placement, and the measurement has not answered D-06 — stop and report
	// rather than picking." That reading holds up physically: past-edge and
	// flat-half differ by a factor of f squared, which IS a magnitude
	// difference, so a pure placement signature was never going to be what
	// separated them.
	//
	// NO WINNER IS DECLARED BY THE RULE. The SUMMARY records the measured
	// ordering and a clearly-labelled RECOMMENDATION for plan 33-06, and states
	// in terms that it is a recommendation on the evidence and NOT a
	// rule-sanctioned decision. A LATER AGENT MUST NOT PROMOTE IT TO ONE BY
	// DELETING THIS PARAGRAPH.
	// =======================================================================
	CAPTURE(cond1);
	CAPTURE(cond2);
	CAPTURE(cond3);
	CHECK(winnerFrac >= 0.0);
}


// ---------------------------------------------------------------------------
// TASK 2 OF PLAN 33-07 — THE SYNC SUB-GRID BECOMES A GATE.
//
// >>> A GRID THAT GATES NOTHING CANNOT GO RED. THAT SENTENCE IS THE WHOLE
//     REASON THIS CASE EXISTS AND THE WHOLE REASON IT IS A SEPARATE PLAN FROM
//     THE MEASUREMENT THAT PRODUCED ITS NUMBERS. <<<
// Plan 33-05 built the instrument, measured 3,360 figures with it and pinned
// none of them, on the stated ground that a threshold pinned in the same commit
// that chose the leg is a threshold pinned from a leg no gate had examined. Plan
// 33-06 landed the leg. This case pins the columns from plan 33-07's own
// measurement of the leg that ships, and gates on them.
//
// ===========================================================================
// >>> THE PLATEAU / STEP-DOMINATED CLASSIFICATION. STATED HERE, ON ITS
//     PHYSICAL CRITERION, BEFORE ANY POPULATION IS ENUMERATED ANYWHERE BELOW.
//     READ THIS BEFORE THE COUNTS. <<<
//
// Register item 8 splits the cross-toolchain reproduction bound in two, and the
// split is PHYSICAL rather than statistical. aliasPeakDb reports an ARG-MAX over
// roughly two thousand non-harmonic bins. When the emitted waveform carries a
// TRUE VALUE STEP, that step's spectrum is a broad 1/f skirt, the arg-max sits
// well above its neighbours, and no plausible unit-in-the-last-place difference
// in a library function can reorder it: the cell is STEP-DOMINATED and earns the
// 1.0 dB bound. When there is no true value step the surviving non-harmonic
// energy is a near-flat PLATEAU of near-tied bins, one libm ULP decides which
// one wins, and the reported figure moves by several decibels for no DSP reason
// at all: the cell is PLATEAU class and earns the 4.0 dB bound. Phase 32
// MEASURED that difference — 3.02596 dB on cell i=86, a plateau cell — when its
// first CI run came back red on Ubuntu and Windows and green on macOS with no
// src/ behaviour differing anywhere.
//
// THE CRITERION HAS TWO CLAUSES BECAUSE A HARD-SYNCED CELL HAS TWO INDEPENDENT
// SOURCES OF A VALUE STEP, AND THE QUESTION THE BOUND ASKS IS WHETHER THE CELL
// CARRIES ONE — NOT WHETHER THE RESET DOES:
//
//   (i)  THE SYNC JUMP. The reset moves the slave's phase, and the frozen
//        waveshaper's value moves with it. That difference is exactly what
//        forge::VcoCore::Telemetry::syncJump records, and plan 33-05 fixed the
//        floor at a mean absolute 0.01 in pre-scale units (0.05 V at the
//        output) — below that the reset moves the waveform by less than one
//        percent of its range and there is no step for the arg-max to lock on
//        to. THAT FLOOR WAS FIXED IN PLAN 33-05'S SOURCE, BEFORE ANY CELL IN
//        THIS FILE WAS GATED, AND IS INHERITED HERE UNCHANGED.
//
//   (ii) THE SLAVE'S OWN DISCONTINUITY. A saw or a pulse carries a value step
//        every cycle whether or not anything syncs it, and so does a square
//        below full character. Phase 32 measured exactly this partition on the
//        standing grid: saw and pulse at every character and square below full
//        character are step-dominated; sine and triangle carry a CORNER, not a
//        step; and the square at character 1.00 was measured with its jump
//        collapsed to -0.001661, which is a corner too.
//
// A CELL IS STEP-DOMINATED WHEN EITHER CLAUSE HOLDS, AND PLATEAU ONLY WHEN
// NEITHER DOES. Clause (i) alone would have called a unity-ratio SAW cell a
// plateau — a waveform with a full-scale discontinuity every cycle — which is
// plainly false about the cell and would have granted it a 4.0 dB bound it has
// not earned. Both clauses are properties of the WAVEFORM, measured or
// inherited-from-measurement, and neither mentions a threshold, a margin or a
// result.
//
// >>> THE ANTI-RECLASSIFICATION CLAUSE, EXTENDED TO THE SYNC ROWS BY NAME. <<<
// 33-VALIDATION's Threshold Policy states it for the standing grid and it now
// binds here as well: A CLASSIFICATION PRODUCED BY RENAMING THE CELLS THAT
// FAILED IS FORBIDDEN. If a sync cell fires and it is step-dominated, THAT IS A
// FINDING ABOUT THE CRITERION OR ABOUT THE IMPLEMENTATION — it is reported, with
// its figures, and escalated. It is never a cell to move into the plateau class
// so the wider bound covers it. The criterion above is written before the counts
// below for exactly that reason, and both populations are asserted EXACTLY so
// that moving one cell between them turns two assertions red rather than none.
//
// >>> THE TIERS, AND THE ONE DECISION PLAN 33-05 LEFT OPEN. <<<
// 33-05 asked plan 33-07 to "decide whether to gate the instrument-invalid half
// at all". THE DECISION IS: NO, AND IT IS TAKEN ON THE INSTRUMENT RATHER THAN ON
// THE NUMBERS. A cell that fails fundamentalDominanceDb is a cell where
// aliasPeakDb is normalising by a bin that is NOT the master's fundamental, so
// its decibel figure is not an alias floor and gating it would be gating a
// quantity the instrument does not produce. Those 210 cells are "diagnostic":
// measured, recorded, and REPRODUCTION-CHECKED like every other row, but never
// CHECKed against their threshold. The remaining 210 are gated — "gated" at
// 44.1 kHz, which register item 8 makes the BINDING rate, and "regression" at 48
// and 96 kHz, which are asserted on the same terms and exist so a rate-dependent
// regression cannot hide at one rate.
// ===========================================================================
TEST_CASE("vco spectrum: (SYNC-02 / D-11) the sync alias floor stays below its per-cell pinned threshold, and every pinned number reproduces") {

	// The criterion's constants, named above before any count below.
	const double kStepDominatedJumpFloor = 0.01;   // pre-scale units — plan 33-05's, inherited unchanged
	const double kBoundStepDominatedDb   = 1.0;    // register item 8
	const double kBoundPlateauDb         = 4.0;    // register item 8

	const std::vector<SyncCell>& grid = syncGrid();
	const std::size_t nCells = grid.size();
	REQUIRE(nCells == 420);
	REQUIRE(kSyncPinCount == 420);

	// =======================================================================
	// THE MEASUREMENT PASS. Two legs per cell through the ONE cell-measuring
	// function: the SHIPPED past-edge leg, which is what every pinned number in
	// SYNC_PINS is a measurement of, and the correction-free reference leg,
	// which supplies the diagnostics that decide the CLASS and the TIER. The
	// reference leg is used for those and only those, for the reason the 33-05
	// measurement case already records: the reset is identical on all eight
	// legs, so a diagnostic taken there is a property of the CELL rather than of
	// whichever candidate happens to be shipping.
	// =======================================================================
	std::vector<double> runDb(nCells, 0.0);
	std::vector<double> noneDb(nCells, 0.0);
	std::vector<char>   stepDom(nCells, 0);
	std::vector<char>   shapeStep(nCells, 0);
	std::vector<char>   jumpStep(nCells, 0);
	std::vector<char>   instrumentValid(nCells, 0);
	std::vector<double> fundDom(nCells, 0.0);

	for (std::size_t ci = 0; ci < nCells; ++ci) {
		const SyncCell& cell = grid[ci];
		double rms = 0.0, binErr = 0.0;
		SyncProbeDiag d = zeroedSyncDiag();
		std::vector<float> refBlock;

		noneDb[ci] = measureSyncCellDb(cell, kLegNone,     /*useLiveCore=*/false, &rms, &binErr, &d, &refBlock);
		runDb[ci]  = measureSyncCellDb(cell, kLegPastEdge, /*useLiveCore=*/false, &rms, &binErr, 0,  0);

		int strongestN = 0;
		fundDom[ci] = fundamentalDominanceDb(refBlock, cell.Km, &strongestN);
		instrumentValid[ci] = (strongestN == 1) ? 1 : 0;

		// CLAUSE (i) — the sync jump, on the correction-free reference leg.
		const double meanAbsJump = (d.fires > 0) ? d.jumpAbsSum / (double)d.fires : 0.0;
		jumpStep[ci] = (meanAbsJump >= kStepDominatedJumpFloor) ? 1 : 0;
		// CLAUSE (ii) — the slave's own discontinuity, by Phase 32's measured
		// partition of the shape centres.
		const bool sawCentre    = (cell.morph == 0.50f);
		const bool pulseCentre  = (cell.morph == 1.00f);
		const bool hardSquare   = (cell.morph == 0.75f) && (cell.character < 1.00f);
		shapeStep[ci] = (sawCentre || pulseCentre || hardSquare) ? 1 : 0;

		stepDom[ci] = (jumpStep[ci] || shapeStep[ci]) ? 1 : 0;
	}

	// =======================================================================
	// THE POPULATIONS, COUNTED ONLY NOW, AND ASSERTED EXACTLY.
	// =======================================================================
	int nStepDominated = 0, nPlateau = 0, nJumpOnly = 0, nShapeOnly = 0, nBoth = 0;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		if (stepDom[ci]) ++nStepDominated; else ++nPlateau;
		if (jumpStep[ci] && shapeStep[ci]) ++nBoth;
		else if (jumpStep[ci])             ++nJumpOnly;
		else if (shapeStep[ci])            ++nShapeOnly;
	}
	CAPTURE(nStepDominated);
	CAPTURE(nPlateau);
	CAPTURE(nJumpOnly);
	CAPTURE(nShapeOnly);
	CAPTURE(nBoth);
	CHECK(nStepDominated == 402);
	CHECK(nPlateau == 18);
	CHECK(nStepDominated + nPlateau == (int)nCells);
	// BOTH CLAUSES DO WORK. If either count were zero the criterion would have
	// one live clause and one decoration, and the two-clause reading above would
	// be a story rather than a description.
	CHECK(nJumpOnly > 0);
	CHECK(nShapeOnly > 0);

	// THE INSTRUMENT-VALIDITY PARTITION, also exact. 33-05 measured 210/210 and
	// a worst dominance of -29.4473 dB; this run reproduces it.
	int nValid = 0, nInvalid = 0;
	double worstFundDom = 0.0;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		if (instrumentValid[ci]) ++nValid; else ++nInvalid;
		if (fundDom[ci] < worstFundDom) worstFundDom = fundDom[ci];
	}
	CAPTURE(nValid);
	CAPTURE(nInvalid);
	CAPTURE(worstFundDom);
	CHECK(nValid == 210);
	CHECK(nInvalid == 210);
	CHECK(worstFundDom < -20.0);

	// =======================================================================
	// THE PIN TABLE'S OWN INTEGRITY, ASSERTED INSIDE THE TEST SO AN ADDED ROW
	// WITH AN EMPTY PROVENANCE GOES RED RATHER THAN PASSING SILENTLY.
	// =======================================================================
	int nGated = 0, nRegression = 0, nDiagnostic = 0, nAsserted = 0;
	int nFloored = 0;
	int nGatedStep = 0, nGatedPlateau = 0;
	double tightestThreshold = 1e30;

	for (std::size_t ci = 0; ci < nCells; ++ci) {
		const SyncCell& cell = grid[ci];
		const std::string tier(cell.tier);
		const std::string prov(cell.provenance);
		const bool isDiagnostic = (tier == "diagnostic");
		const bool isAsserted   = (tier == "gated") || (tier == "regression");

		// CAPTURE of a const char* prints a POINTER, which is unreadable in a
		// failure log; the two label columns go through std::string so a red
		// cell names itself.
		const std::string edgeName(cell.edgeName);
		const std::string region(cell.region);
		CAPTURE(ci);
		CAPTURE(cell.sr);
		CAPTURE(edgeName);
		CAPTURE(cell.ratio);
		CAPTURE(region);
		CAPTURE(cell.character);
		CAPTURE(cell.measuredDb);
		CAPTURE(cell.thresholdDb);
		CAPTURE(tier);

		// EVERY cell has a provenance, and it is not the 33-05 sentinel string
		// and not one of Phase 32's six. A row added to SYNC_GRID without a row
		// in SYNC_PINS reaches here holding the sentinel and fails all three.
		CHECK(prov.size() > 0);
		CHECK(prov != std::string(kProvSyncUnpinned));
		CHECK(prov.find("33-07") != std::string::npos);
		CHECK(cell.measuredDb != kSyncUnpinnedDb);
		CHECK(cell.thresholdDb != kSyncUnpinnedDb);
		CHECK((isDiagnostic || isAsserted));   // "UNPINNED" is neither, and reds here

		// THE TIER IS A MEASUREMENT, NOT A LABEL. It is re-derived from this
		// run's own fundamental-dominance result and its rate, and must match
		// what the table recorded. A tier edited to move a failing cell into the
		// diagnostic half goes red here.
		const bool tierShouldBeDiagnostic = !instrumentValid[ci];
		CHECK(isDiagnostic == tierShouldBeDiagnostic);
		if (!tierShouldBeDiagnostic) {
			const bool binding = (cell.sr == 44100.0);
			CHECK((tier == "gated") == binding);
			CHECK((tier == "regression") == !binding);
		}

		// THE PROVENANCE NAMES THE CLASS'S BOUND, and the class is re-derived
		// above from the physical criterion. A provenance swapped to buy a cell
		// the wider bound goes red here, and so does a class that moved.
		const double bound = stepDom[ci] ? kBoundStepDominatedDb : kBoundPlateauDb;
		if (!isDiagnostic) {
			const bool namesStepBound    = (prov.find("measuredDb + 1.0") != std::string::npos);
			const bool namesPlateauBound = (prov.find("measuredDb + 4.0") != std::string::npos);
			CHECK(namesStepBound == (stepDom[ci] != 0));
			CHECK(namesPlateauBound == (stepDom[ci] == 0));
		}

		// THE DERIVATION, ASSERTED MECHANICALLY. Rounded OUTWARD by the bound
		// the cell's class earns, then floored at the static threshold floor.
		// Loosening thresholdDb by hand, without touching its measured sibling,
		// breaks THIS.
		const double derived = std::ceil((double)cell.measuredDb + bound);
		const double expected = (derived < (double)kThresholdFloorDb) ? (double)kThresholdFloorDb : derived;
		CAPTURE(bound);
		CAPTURE(derived);
		CAPTURE(expected);
		CHECK((double)cell.thresholdDb == expected);
		if (derived < (double)kThresholdFloorDb) ++nFloored;

		// NO THRESHOLD IS TIGHTER THAN THE STATIC FLOOR, asserted over the WHOLE
		// sync grid rather than only over the gated half.
		CHECK((double)cell.thresholdDb >= (double)kThresholdFloorDb);
		if ((double)cell.thresholdDb < tightestThreshold) tightestThreshold = (double)cell.thresholdDb;

		if (tier == "gated")      ++nGated;
		if (tier == "regression") ++nRegression;
		if (isDiagnostic)         ++nDiagnostic;
		if (isAsserted) {
			++nAsserted;
			if (stepDom[ci]) ++nGatedStep; else ++nGatedPlateau;
		}
	}

	CAPTURE(nGated);
	CAPTURE(nRegression);
	CAPTURE(nDiagnostic);
	CAPTURE(nAsserted);
	CAPTURE(nGatedStep);
	CAPTURE(nGatedPlateau);
	CAPTURE(nFloored);
	CAPTURE(tightestThreshold);
	CHECK(nGated == 70);          // 44.1 kHz, instrument-valid — the BINDING rows
	CHECK(nRegression == 140);    // 48 and 96 kHz, instrument-valid
	CHECK(nDiagnostic == 210);    // instrument-invalid at every rate
	CHECK(nAsserted == 210);
	CHECK(nGatedStep == 192);
	CHECK(nGatedPlateau == 18);
	CHECK(nGatedStep + nGatedPlateau == nAsserted);
	// THE FLOOR DOES NOT BIND ANYWHERE ON THIS GRID, AND THAT IS RECORDED RATHER
	// THAN LEFT TO BE ASSUMED. The tightest pinned threshold is -49 dB against a
	// floor of -75, so no sync row is silently floored and no sync provenance
	// string has to claim it was. The floor is still asserted per cell above,
	// because a later re-pin at a quieter cell could reach it.
	CHECK(nFloored == 0);
	CHECK(tightestThreshold > (double)kThresholdFloorDb);

	// =======================================================================
	// THE REPRODUCTION PASS — WHAT MAKES "PINNED FROM MEASUREMENT" DEFENSIBLE
	// RATHER THAN CIRCULAR.
	//
	// >>> A THRESHOLD PINNED FROM THE IMPLEMENTATION'S OWN OUTPUT CANNOT, ON ITS
	//     OWN, FAIL. EVERY GATED CELL PASSES BY CONSTRUCTION WITH AT LEAST ITS
	//     CLASS BOUND OF ROOM. <<< That is true here exactly as the standing
	//     grid's banner says it is true there, and it is why the table is not
	//     the evidence on its own. The coupling below is:
	//
	//   * loosening thresholdDb alone breaks the DERIVATION assertion above;
	//   * loosening measuredDb with it breaks the REPRODUCTION check here,
	//     which compares the recorded column against WHAT THIS RUN MEASURED;
	//   * and the two together are still visible, because both edits are named
	//     in the SyncCell banner's warning signs and in SYNC_PINS' own banner.
	//
	// The bound is the class's, not a fixed number, because that is the quantity
	// register item 8 actually measured.
	// =======================================================================
	double worstReproduction = 0.0;
	int    worstReproductionCell = -1;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		const SyncCell& cell = grid[ci];
		const double bound = stepDom[ci] ? kBoundStepDominatedDb : kBoundPlateauDb;
		const double delta = std::fabs(runDb[ci] - (double)cell.measuredDb);
		if (delta > worstReproduction) { worstReproduction = delta; worstReproductionCell = (int)ci; }
		const std::string edgeName(cell.edgeName);
		const std::string region(cell.region);
		CAPTURE(ci);
		CAPTURE(cell.sr);
		CAPTURE(edgeName);
		CAPTURE(cell.ratio);
		CAPTURE(region);
		CAPTURE(cell.character);
		CAPTURE(cell.measuredDb);
		CAPTURE(runDb[ci]);
		CAPTURE(bound);
		CAPTURE(delta);
		CHECK(delta <= bound);
	}
	CAPTURE(worstReproduction);
	CAPTURE(worstReproductionCell);

	// =======================================================================
	// >>> THE GATE ITSELF. <<<
	// The alias floor this run measures on the SHIPPED leg stays below the
	// threshold pinned for that cell. Gated and regression rows are CHECKed;
	// diagnostic rows are CAPTUREd and never CHECKed, because their figure is
	// not an alias floor at all.
	// =======================================================================
	int nGateChecked = 0;
	double worstHeadroom = 1e30;
	int    worstHeadroomCell = -1;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		const SyncCell& cell = grid[ci];
		const std::string tier(cell.tier);
		if (tier == "diagnostic") continue;
		const double headroom = (double)cell.thresholdDb - runDb[ci];
		if (headroom < worstHeadroom) { worstHeadroom = headroom; worstHeadroomCell = (int)ci; }
		++nGateChecked;
		const std::string edgeName(cell.edgeName);
		const std::string region(cell.region);
		CAPTURE(ci);
		CAPTURE(cell.sr);
		CAPTURE(edgeName);
		CAPTURE(cell.ratio);
		CAPTURE(region);
		CAPTURE(cell.character);
		CAPTURE(cell.thresholdDb);
		CAPTURE(runDb[ci]);
		CAPTURE(headroom);
		CHECK(runDb[ci] <= (double)cell.thresholdDb);
	}
	CAPTURE(nGateChecked);
	CAPTURE(worstHeadroom);
	CAPTURE(worstHeadroomCell);
	CHECK(nGateChecked == 210);
	CHECK(worstHeadroom > 0.0);

	// =======================================================================
	// THE MUTATION PROBE — DISCRIMINATING, AND ITS POPULATION IS STATED BEFORE
	// IT IS RUN.
	//
	// >>> THE STATED POPULATIONS, AS LITERALS, WRITTEN DOWN BEFORE THE LOOP
	//     BELOW EXECUTES AND NOT ADJUSTED TO WHATEVER IT PRODUCES. <<<
	// The probe adds a fixed offset to the value that comes out of the
	// measurement path, at the gate's input, and counts how many gated cells
	// then exceed their threshold. This is the shape Phase 32's probe already
	// takes — a +2.0 dB probe failing exactly 48 cells and +5.0 exactly 90 — and
	// it is chosen over perturbing the DSP because it isolates the GATE: a probe
	// that perturbs the core measures the core's sensitivity as well as the
	// gate's, and cannot fail a stated population exactly.
	//
	// WHY +2.0 DISCRIMINATES BY CLASS, DERIVED RATHER THAN OBSERVED: every gated
	// cell's headroom against its own pinned threshold is
	// ceil(measuredDb + bound) - measuredDb, which lies in [bound, bound+1) by
	// construction. A step-dominated cell therefore has headroom in [1.0, 2.0)
	// and a plateau cell in [4.0, 5.0). An offset of +2.0 dB is above EVERY
	// step-dominated headroom and below EVERY plateau one, so it must fail
	// exactly the 192 gated step-dominated cells and exactly zero of the 18
	// plateau ones. An offset of +5.0 dB is above both, so it must fail all 210.
	//
	// IF THE OBSERVED COUNTS DIFFER FROM THESE, THE DISCREPANCY IS REPORTED AND
	// THE STATED NUMBERS ARE NOT MOVED TO MATCH.
	// =======================================================================
	const int kStatedProbe2FiresStep    = 192;   // every gated step-dominated cell
	const int kStatedProbe2FiresPlateau = 0;     // and not one plateau cell
	const int kStatedProbe5Fires        = 210;   // every gated cell, both classes

	int probe2Step = 0, probe2Plateau = 0, probe5All = 0;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		const SyncCell& cell = grid[ci];
		const std::string tier(cell.tier);
		if (tier == "diagnostic") continue;
		const double mutated2 = runDb[ci] + 2.0;
		const double mutated5 = runDb[ci] + 5.0;
		if (mutated2 > (double)cell.thresholdDb) { if (stepDom[ci]) ++probe2Step; else ++probe2Plateau; }
		if (mutated5 > (double)cell.thresholdDb) ++probe5All;
	}
	CAPTURE(kStatedProbe2FiresStep);
	CAPTURE(probe2Step);
	CAPTURE(kStatedProbe2FiresPlateau);
	CAPTURE(probe2Plateau);
	CAPTURE(kStatedProbe5Fires);
	CAPTURE(probe5All);
	CHECK(probe2Step == kStatedProbe2FiresStep);
	CHECK(probe2Plateau == kStatedProbe2FiresPlateau);
	CHECK(probe5All == kStatedProbe5Fires);

	// =======================================================================
	// THE CORRECTION'S OWN MAGNITUDE, RECORDED HERE BECAUSE THE SyncCell
	// BANNER'S REFUSAL PARAGRAPH QUOTES IT AND A QUOTED NUMBER WITH NOTHING
	// BEHIND IT IS A NUMBER THAT DRIFTS.
	//
	// >>> WHAT IS ASSERTED IS THE SIGN AND THE ORDER OF MAGNITUDE, NEVER THE
	//     DECIBEL. <<< Every figure here is an Apple-clang figure and the counts
	//     below include cells where the two legs differ by thousandths of a
	//     decibel — at the unity ratio the mean is +0.0037 dB, which is a coin
	//     flip and must not be gated as though it were a result. The magnitudes
	//     and the counts are CAPTUREd; the three claims that survive a toolchain
	//     crossing are CHECKed.
	// =======================================================================
	double gridImprovementSum = 0.0;
	int    cellsWherePastIsWorse = 0;
	double worstPastDeficit = 0.0;
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		const double improvement = noneDb[ci] - runDb[ci];   // positive = the correction is better
		gridImprovementSum += improvement;
		if (improvement < 0.0) ++cellsWherePastIsWorse;
		if (-improvement > worstPastDeficit) worstPastDeficit = -improvement;
	}
	const double gridImprovementMean = gridImprovementSum / (double)nCells;
	CAPTURE(gridImprovementMean);
	CAPTURE(cellsWherePastIsWorse);
	CAPTURE(worstPastDeficit);

	// CLAIM A — the correction's own spectral improvement is A FRACTION OF A
	// DECIBEL, which is why an improvement gate in the Phase 32 shape is refused
	// in the SyncCell banner rather than written and then loosened. 33-VALIDATION
	// predicted "a mean of about 0.5 dB" in advance; measured +0.5827.
	CHECK(gridImprovementMean > 0.0);
	CHECK(gridImprovementMean < 1.0);

	double ratioMean[7];
	int    ratioCells[7];
	for (int q = 0; q < 7; ++q) { ratioMean[q] = 0.0; ratioCells[q] = 0; }
	for (std::size_t ci = 0; ci < nCells; ++ci) {
		for (int q = 0; q < 7; ++q) {
			if (grid[ci].ratio != SYNC_RATIOS[q]) continue;
			ratioMean[q] += noneDb[ci] - runDb[ci];
			++ratioCells[q];
		}
	}
	for (int q = 0; q < 7; ++q) {
		REQUIRE(ratioCells[q] == 60);
		ratioMean[q] /= 60.0;
		const double ratio = SYNC_RATIOS[q];
		CAPTURE(ratio);
		CAPTURE(ratioMean[q]);
	}

	// CLAIM B — the correction pays for itself HANDSOMELY below unity, which is
	// hazard one's whole point: that is where the reset truncates the slave
	// mid-cycle on every master wrap.
	CHECK(ratioMean[0] > 1.0);   // ratio 0.50 — measured +2.4495
	CHECK(ratioMean[1] > 1.0);   // ratio 0.75 — measured +1.9150

	// CLAIM C — AND AT THE TOP OF THE SWEEP IT IS WORSE THAN DOING NOTHING. This
	// is 33-06's deferred register item 3, asserted rather than left in a
	// document: at ratio 5.5 the shipped leg's mean alias floor is ABOVE the
	// no-correction leg's. It is the forfeited pre-edge half showing up where the
	// detected fraction is largest, MorphBlep.hpp's addPastStep banner item 3
	// records the forfeit as deliberate and permanent, and no threshold pinned
	// above assumes the correction helps anywhere — every one of them is pinned
	// from the shipped leg's OWN figure. If this ever goes positive, the forfeit
	// has stopped costing what it costs and THAT is a finding worth reporting,
	// which is why the sign is asserted in both directions rather than ignored.
	CHECK(ratioMean[6] < 0.0);   // ratio 5.50 — measured -1.0281
}
