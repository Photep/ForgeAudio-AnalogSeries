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
