// tools/render_sync_ab.cpp
//
// HARD-SYNC A/B AUDITION RENDERER (Phase 33, plan 33-10 — D-13 / D-14 / D-15 /
// D-16). One-shot, NOT part of `make test`, and NOT committed.
//
// Build:  make audition   (Rack-free; the SAME TEST_CXXFLAGS as `make test`, so
//                          the rendered audio is bit-comparable with what the
//                          420-cell sync grid and the SC-3 time-domain gate
//                          actually measure)
// Run from the repo root — output paths are relative to CWD, exactly as
// tools/capture_golden.cpp assumes.
//
// ===========================================================================
// WHY THIS FILE EXISTS.
//
// Phase 32's in-Rack audition asked the operator whether an improvement was
// AUDIBLE and supplied no reference to compare against. The reply, recorded
// verbatim, was "Seems to work well enough - but it's hard to remember what the
// old audio sounded like." That is a correct and useful answer to a question
// that was unanswerable BY CONSTRUCTION, and it is deferred register item 26.
// This phase's questions are worse — "buzzy rather than smeared" and "no click
// per sync" have NO automated instrument at all, so an audition without a
// reference would be the whole verdict rather than half of it. This tool puts
// the reference in the room.
//
// ===========================================================================
// >>> THE BINDING CONSTRAINT: THE TWO LEGS MUST COME FROM THE SAME PASS. <<<
//
// This is D-14 and it is not stylistic. If the two legs came from two runs, two
// cores, a mirror, or a `bool bandLimit` flag in the shipped body, then anything
// the operator hears could be a difference between the two APPARATUSES rather
// than between the two DSP behaviours, and the verdict would be about the rig.
// The rig is what Phase 32's audition got wrong; reproducing it here with more
// files would be worse, not better.
//
// THE MECHANISM ACTUALLY USED. `forge::VcoCore::Telemetry::syncCorrection` is a
// RECORDING-ONLY float — nothing in step() ever reads it back, no branch tests
// it, and deleting it would leave every returned sample bit-identical. Plan
// 33-06 landed the sync seam at the PAST-EDGE placement, where the correction
// deposits into `MorphBlep::inject` ONLY and leaves `pending` untouched, so it
// is purely additive per sample. That makes a whole family of legs
// reconstructible from ONE drive of ONE real core:
//
//     leg_k[n] = out_shipped[n] + 5.f * (k - 1.f) * syncCorrection[n]
//
// with k = 1 the shipped leg unchanged and k = 0 the leg with the sync
// correction withheld entirely. The factor of five is forge::VcoCore::step()'s
// own output multiplier: syncCorrection is recorded in the PRE-MULTIPLY domain.
// This is the same expression plan 33-08's anti-circularity margin uses, and it
// is used here for the same reason.
//
// >>> ITS PRECONDITION, STATED RATHER THAN ASSUMED. <<< The reconstruction is
// exact only while the sync correction deposits NOTHING FORWARD. That is a
// property of the placement plan 33-06 landed, not of hard sync in general. Of
// the four candidates plan 33-05 measured, `detect` and `flatHalf` BOTH also
// deposit into `pending`, and under either of them the per-sample subtraction is
// no longer exact and this renderer would have to reconstruct from BOTH halves.
// IF A LATER PHASE MOVES THE SEAM OFF PAST-EDGE, THIS FILE MUST BE REVISITED.
// The placement is decision D-06; plan 33-06 is the plan that landed it (on an
// operator decision of 2026-08-30, after the D-06 three-condition rule REFUSED),
// and any plan that revisits D-06 owns this paragraph along with it.
//
// >>> AND THE SHORTCUT HAS A MEASURED ERROR BAR, NOT AN ADJECTIVE. <<< Plan
// 33-02's header called the subtraction "EXACT"; plan 33-06 MEASURED it on
// landing and it is not bit-exact. Over 49,152 samples with 93 resets: 49,136
// reconstruct bit-exactly, 16 differ by EXACTLY ONE ULP, worst absolute
// departure 4.77e-07 V, and every non-reset sample is exact so nothing
// accumulates. FOUR AND THREE QUARTER TENTHS OF A MICROVOLT IS INAUDIBLE, which
// is why the shortcut is fit for an audition — but DO NOT write a bit-exact
// equality assertion against it anywhere. Plan 33-08 is the other consumer of
// this relationship and it declined to write one for the same reason.
//
// ===========================================================================
// >>> WHAT THIS TOOL IS NOT. <<<
//
// NOT A GOLDEN. NOT A FIXTURE. NOT COMMITTED. The output lands under
// build-test/audition/, which .gitignore already covers, and it is regenerated
// on demand every session. That is D-15, and the reason is not tidiness: EVERY
// decibel and EVERY volt this phase has measured is an Apple-clang figure, and
// Phase 32 measured its spectral instrument toolchain-dependent by up to
// 3.02596 dB with NO src/ behaviour differing at all. A rendered pair captured
// from one toolchain that drifted into being treated as a reference would be
// exactly that hazard with a waveform editor attached. Generating on demand into
// an ignored directory enforces the rule BY CONSTRUCTION rather than by anyone
// remembering it.
//
// ===========================================================================
// >>> WHAT THE OPERATOR SHOULD HONESTLY EXPECT — DO NOT OVERSELL THIS. <<<
//
// Three instruments have now measured the shipped correction, and none of them
// supports a dramatic difference:
//
//   * TIME DOMAIN (plan 33-08). The worst per-sample step across a reset is
//     9.793601 V shipped, against 10.000000 V with the correction withheld. THE
//     SHIPPED SYNC BLEP REMOVES ABOUT TWO PERCENT OF THE WORST-CASE RESET STEP.
//   * SPECTRAL (plan 33-07). The grid-wide mean improvement is +0.5827 dB.
//     Phase 32's own improvement gate shape (>= 8.0 dB) fails here BY
//     CONSTRUCTION and was refused in writing.
//   * AND ON PART OF THE GRID THE CORRECTION IS MEASURABLY WORSE THAN NONE.
//     Per-ratio mean, none minus pastEdge (positive = shipped better):
//         0.50  +2.4495     1.50  +0.7247     3.50  -0.1911
//         0.75  +1.9150     2.50  +0.2051     5.50  -1.0281  <-- WORSE
//         1.00  +0.0037
//     Plan 33-08 found the same region in the time domain: 56 of 420 cells have
//     a NEGATIVE margin, worst -0.246492 V at ratio 5.50.
//
// The render-point table below therefore includes points where the correction
// LOSES and a point where it does essentially NOTHING, not only points where it
// wins. An operator told to expect an obvious "click disappearing" would be told
// something no instrument in this phase supports, and would then report a defect
// that is not there.
//
// ===========================================================================
// >>> THE SEED LITERALS ARE COPIED VERBATIM AND MUST NEVER BE INVENTED. <<<
// The four values below come from tests/VcoBlockDriver.hpp:42-43, which is where
// tests/test_vco_spectrum.cpp copies them from too. A forge::Xoroshiro128Plus
// seeded (0, 0) is a DEGENERATE FIXED POINT emitting an all-zero stream, which
// makes std::normal_distribution's rejection loop never terminate. Here that is
// a hung render; in Rack it is a HANG WHILE OPENING A PATCH (T-32-09 / T-33-34).
// ===========================================================================

#include "dsp/VcoCore.hpp"

#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <cmath>

namespace {

// --- The four documented seed literals, verbatim from tests/VcoBlockDriver.hpp:42-43.
const uint64_t kDriftS0  = 0x1234ULL;
const uint64_t kDriftS1  = 0x5678ULL;
const uint64_t kSpreadS0 = 0x9E3779B9ULL;
const uint64_t kSpreadS1 = 0x7F4A7C15ULL;

// The sync grid's block length. The master's cycles-per-block count is exact in
// binary against this denominator, which is what makes the master increment a
// dyadic rational and the render point identical in construction to the cell the
// gate measured. Do not change it without changing the render-point table.
const int kMasterCycleDenom = 4096;

// Discarded before the audible portion starts, matching the warm-up discipline
// tests/test_vco_spectrum.cpp's driveSecondBlock uses: the phase accumulator AND
// forge::MorphBlep's `pending` accumulator must reach steady state, or the
// operator's first quarter-second is a transient of the harness rather than of
// the DSP.
const int kWarmupSamples = 4096;

// --- THE STATED SCALE, VOLTS TO FULL SCALE ---------------------------------
// 10.0 V maps to digital full scale, so the factor is 0.1 exactly. The number is
// not arbitrary: 10.0 V is kHostileBoundV, the outer output tier every scenario
// in tests/test_vco_core.cpp asserts, so A CLIPPED SAMPLE HERE MEANS THE CORE
// EXCEEDED THE BOUND THE TEST SUITE PINS. That makes the clip count evidence
// rather than a mixing decision. The largest envelope ever measured on this
// suite is plan 33-08's 8.218569 V on the sync sweep itself, so the expected
// clip count is zero and a non-zero one is a finding.
const float kVoltsToFullScale = 0.1f;
const float kFullScaleVolts   = 10.0f;

// >>> NEVER NORMALISE PER LEG. <<< Level-matching the two legs is the ENTIRE
// POINT of an A/B: the shipped correction changes the waveform by about two
// percent of the worst-case reset step, and any per-leg gain staging would be
// larger than the effect under audition. One scale factor, stated above, applied
// identically to every leg of every render point. If a leg is too quiet, that is
// what the DSP does.

enum MasterEdge { kMasterHardEdge = 0, kMasterBandLimited = 1 };

// ---------------------------------------------------------------------------
// PART 1 — THE RENDER-POINT TABLE.
//
// Kept as DATA, separate from the rendering code below, and that separation IS
// the reusability D-16 requires: Phase 34's DRIFT-03 value is audition-gated and
// will be decided on exactly this kind of comparison, so it must be able to
// change a table rather than a program. Register item 26 stayed open through
// Phase 32 precisely because there was no budget to build a harness mid
// checkpoint; there is no excuse for the next phase to hit the same wall.
//
// THE POINTS ARE THE SUB-GRID POINTS PLAN 33-05 MEASURED AND PLAN 33-07 PINNED,
// so what the operator hears is what the gate measured — same rates, same master
// cycle counts, same ratios, same shape centres, same character ends, same
// master edge shapes. And they are chosen HONESTLY: two points where the
// correction wins, two where it is measurably WORSE than applying none, one
// where it does essentially nothing, and the exact cell plan 33-08 measured the
// worst-case reset step on.
//
// ALL POINTS ARE AT 44.1 kHz, DELIBERATELY. Plan 33-08 measured the reset
// envelope RATE-INDEPENDENT TO SIX DECIMAL PLACES (9.793601 V at all three
// rates) because both grids are parametrised by master cycles PER SAMPLE rather
// than by hertz, so a second rate would double the file count without adding
// anything an ear could use. Adding one is a one-line edit to this table.
//
// ===========================================================================
// >>> BUT THE TABLE CARRIES TWO DIFFERENT MASTERS, AND THAT IS A FINDING THIS
//     PLAN MEASURED RATHER THAN A CONVENIENCE. READ THIS BEFORE ADDING A ROW OR
//     QUOTING A NUMBER FROM EITHER GRID. <<<
//
// This phase built TWO instruments over what LOOK like the same 420 cells — the
// same five axes, in the same order, with the same names:
//
//   * the SPECTRAL sub-grid (plans 33-05 / 33-07, tests/test_vco_spectrum.cpp)
//     drives a master of K_m cycles per 4096 samples with K_m ODD and coprime to
//     4096 (93 / 85 / 43), and
//   * the TIME-DOMAIN SC-3 grid (plan 33-08, tests/test_vco_core.cpp) drives a
//     master at a dyadic increment of 1/128 — the equivalent of K_m = 32.
//
// THOSE ARE NOT THE SAME SIGNAL, AND THE CELL LABELS DO NOT SAY SO. The five
// axes a cell is named by (rate, master edge shape, ratio, shape centre,
// character) DO NOT INCLUDE THE MASTER FREQUENCY, so a per-cell figure from one
// grid is not transferable to the same-named cell of the other. MEASURED, over
// 4096 samples, on the master wrap fraction g:
//
//     K_m = 93 (spectral):   93 wraps, g in [0.010752688, 1.000000000]
//     K_m = 32 (SC-3 grid):  32 wraps, g == 1.000000000 EXACTLY, on every wrap
//
// >>> THE SC-3 GRID'S MASTER NEVER WRAPS BETWEEN TWO SAMPLES. <<< 1/128 divides
// the sample grid exactly, so every master edge lands ON a sample boundary and
// the sub-sample fraction the whole seam exists to handle is never exercised
// there. The spectral grid's coprime K_m spreads g across almost the entire unit
// interval instead. That single difference is enough to REVERSE the sign of the
// correction's benefit, which is why render points 06 and 07 below are the SAME
// CELL on the two masters and are meant to be auditioned back to back.
//
// Two consequences, both recorded rather than absorbed, and NEITHER of them is
// a red test — each instrument is correct about its own grid:
//   1. Plan 33-08's negative-margin region (56 of 420 cells, worst -0.246492 V
//      at ratio 5.50) does NOT reproduce on the spectral master. Measured here
//      on all 140 spectral-master cells of the same axes: only 7 have a negative
//      reset-step margin, all of them at the ratio-1.00 null point, worst
//      -0.003739 V. On the SC-3 master the same scan reproduces 33-08 closely:
//      20 of 140 negative (33-08: 56 of 420 — the same proportion).
//   2. Plan 33-08's `kSyncResetDeltaBoundV = 9.90 V` is a property of ITS
//      master. On the spectral master this renderer measures a worst reset step
//      of 9.999983 V on render point 02 — ABOVE that bound. The bound is not
//      wrong and its case is not red: its own grid measures 9.793601 V, which
//      this renderer reproduces EXACTLY when pointed at K_m = 32. DO NOT WIDEN
//      IT. It is escalated per the anti-softening rule as a scope question for
//      plan 33-11, alongside that plan's register item 2.
// ---------------------------------------------------------------------------
struct RenderPoint {
	const char* label;        // leads the output filename, so a pair stays adjacent under an alphabetical sort
	double      sr;           // sample rate
	int         masterKm;     // master cycles per 4096 samples -> the master frequency, exactly
	MasterEdge  edge;         // master edge shape (hazard two's axis)
	const char* edgeName;
	double      ratio;        // master/slave frequency ratio
	float       morph;        // shape centre
	const char* region;       // its human-readable name
	float       character;
	double      seconds;      // audible duration, after the warm-up discard
	const char* why;          // why this point is in the table, in measured terms
};

const RenderPoint RENDER_POINTS[] = {
	// --- Points 01..05 are on the SPECTRAL sub-grid's master (K_m = 93), which
	//     is the grid plans 33-05 and 33-07 measured and pinned per cell.
	{ "01-ratio0.50-saw-bandlimited-master1001Hz",   44100.0, 93, kMasterBandLimited, "band-limited", 0.50, 0.50f, "saw",      0.00f, 2.0,
	  "WHERE THE CORRECTION WINS MOST SPECTRALLY. Ratio 0.50 measures +2.4495 dB mean improvement (33-07, this master), the largest of any ratio. A conventional hard-synced saw." },
	{ "02-ratio0.50-pulse-bandlimited-master1001Hz", 44100.0, 93, kMasterBandLimited, "band-limited", 0.50, 1.00f, "pulse 5%", 0.00f, 2.0,
	  "THE LARGEST RESET STEP THIS RENDERER PRODUCES ANYWHERE. Measured here: 9.999983 V shipped against 10.000000 V withheld - the correction removes EIGHTEEN MICROVOLTS of a ten-volt step. The seam's deposit is proportional to f squared, so a reset detected at f near zero gets essentially no correction and reproduces the full step; on this master f is equidistributed, so such a reset always occurs. Plan 33-08 names this same cell as its worst, but on ITS master (see the two-master note above) and measures 9.793601 V there." },
	{ "03-ratio1.50-saw-hardedge-master1001Hz",      44100.0, 93, kMasterHardEdge,    "hard-edge",    1.50, 0.50f, "saw",      0.00f, 2.0,
	  "THE MIDDLE OF THE SWEEP. Ratio 1.50 measures +0.7247 dB mean spectrally (33-07) - a real but modest win. Measured here, the worst reset step improves by +1.001893 V, the clearest time-domain benefit in this table." },
	{ "04-ratio5.50-saw-bandlimited-master1001Hz",   44100.0, 93, kMasterBandLimited, "band-limited", 5.50, 0.50f, "saw",      0.00f, 2.0,
	  "WHERE THE CORRECTION LOSES SPECTRALLY. Ratio 5.50 measures -1.0281 dB mean (33-07, this master): the shipped leg is WORSE than applying no correction at all on 47 of that ratio's 60 cells. Note honestly that its TIME-DOMAIN margin here is slightly positive (+0.019630 V) - the two instruments disagree in sign on this point, and neither is wrong: they measure different quantities." },
	{ "05-ratio1.00-saw-hardedge-master1001Hz",      44100.0, 93, kMasterHardEdge,    "hard-edge",    1.00, 0.50f, "saw",      0.00f, 2.0,
	  "THE NULL POINT, AND THE OPERATOR'S OWN CONTROL. At unity ratio the slave is already in phase at every master wrap, so the reset barely moves the waveform (mean |jump| 0.0038 against 1.0 either side) and the spectral improvement is +0.0037 dB. THE TWO LEGS SHOULD SOUND THE SAME HERE - measured RMS delta -0.0001 dB. If they do not, the listening conditions are reporting something the DSP is not. It is also the ONLY region on this master where the reset-step margin goes negative, and only by -0.001738 V." },

	// --- Points 06 and 07 are THE SAME CELL on the two different masters, and
	//     are meant to be auditioned back to back. This pair is the whole reason
	//     `masterKm` is a table field rather than a constant.
	{ "06-ratio5.50-square-hardedge-master1001Hz",   44100.0, 93, kMasterHardEdge,    "hard-edge",    5.50, 0.75f, "square",   1.00f, 2.0,
	  "PLAN 33-08's WORST NEGATIVE-MARGIN CELL, ON THE SPECTRAL MASTER. 33-08 measured -0.246492 V on this cell's five axes - but on its own master, where g is always exactly 1.0. On THIS master the same five axes measure a margin of +1.419190 V: the correction HELPS, clearly. Audition this against point 07." },
	{ "07-ratio5.50-square-hardedge-master344Hz",    44100.0, 32, kMasterHardEdge,    "hard-edge",    5.50, 0.75f, "square",   1.00f, 2.0,
	  "THE SAME CELL ON PLAN 33-08's MASTER (K_m = 32, the 1/128 dyadic increment where every master edge lands exactly on a sample boundary). Measured here: margin -0.427492 V - THE CORRECTION MAKES THE WORST RESET STEP LARGER. This is the region where the correction loses, reproduced in the time domain, and the ONLY thing that changed between points 06 and 07 is the master frequency." },
};
const std::size_t kRenderPointCount = sizeof(RENDER_POINTS) / sizeof(RENDER_POINTS[0]);

// ---------------------------------------------------------------------------
// PART 2 — THE CORE-CONFIGURATION PAIR.
//
// The renderer takes a PAIR (or any number) of configurations and renders all of
// them from one drive. Expressed as DATA, never as a branch inside the render
// loop, so the next phase supplies a different pair without touching the
// program — D-16 again.
//
// `correctionScale` is the k of the reconstruction in the banner: 1.0 is the
// shipped core unchanged, 0.0 withholds the sync correction entirely. Values
// between and beyond are legitimate and are what plan 33-08's mutation probes
// use (0.25, 0.5, -1.0), so a later phase can audition a partial or inverted
// correction without a second core either.
//
// ONE HONEST FOOTNOTE ON THE k = 1 LEG. The loop applies the reconstruction
// uniformly rather than branching on k, so the shipped leg is computed as
// `s + 5.f * 0.f * corr`. That is `s + 0.f`, which equals `s` for every finite
// sample and differs only for a NEGATIVE ZERO, where it yields +0.0f. Both
// quantise to PCM sample 0. A branch here would be a branch in the render loop,
// which is precisely what the design forbids, and the alternative cost is a
// sign-of-zero change that no ear and no 16-bit encoder can represent.
// ---------------------------------------------------------------------------
struct CoreConfig {
	const char* fileTag;          // trails the render-point label in the filename
	const char* legName;          // for the run's own output
	float       correctionScale;  // the k above
	const char* what;
};

const CoreConfig CORE_PAIR[] = {
	{ "leg-A-shipped",  "A / SHIPPED  (sync BLEP active - what forge::VcoCore does today)", 1.0f,
	  "the shipped past-edge sync correction, landed by plan 33-06" },
	{ "leg-B-withheld", "B / WITHHELD (sync correction removed - the reference)",           0.0f,
	  "the same reset with the sync correction withheld entirely - leg `none`" },
};
const std::size_t kCoreConfigCount = sizeof(CORE_PAIR) / sizeof(CORE_PAIR[0]);

// ---------------------------------------------------------------------------
// The master generator. TRANSCRIBED FROM tests/test_vco_spectrum.cpp's
// makeSyncMaster, which is itself documented as identical in construction to
// tests/test_vco_core.cpp's makeMasterSawBandLimited, "so the two files measure
// the same construction rather than two variants of it". This is the third
// transcription and the same rule binds it: if any of the three changes, all
// three change, or the audition stops being of the cell the gate measured.
//
// The increment is K_m * 2^-12 exactly, so it is a dyadic rational and the
// master is bit-reproducible. The band-limited variant applies the two-point
// polyBLEP residual at each wrap: +amp*(1-g)^2 on the sample BEFORE it and
// -amp*g^2 on the sample carrying it (33-RESEARCH Pitfall 7's own expressions).
// ---------------------------------------------------------------------------
std::vector<float> makeMaster(int nTotal, int Km, double amp, MasterEdge edge) {
	std::vector<float> volts;
	volts.reserve((std::size_t)nTotal);

	const double dtm = (double)Km / (double)kMasterCycleDenom;
	std::vector<int>    wrapIdx;
	std::vector<double> wrapG;
	double phim = 0.0;

	for (int i = 0; i < nTotal; ++i) {
		const double before = phim;
		phim += dtm;
		if (phim >= 1.0) {
			// std::floor rather than a single subtract, matching both existing
			// generators: every caller here keeps dtm well under 1, but the
			// three generators must not differ in a way a reader has to find.
			const double k = std::floor(phim);
			wrapIdx.push_back(i);
			wrapG.push_back((1.0 - before) / dtm);
			phim -= k;
		}
		volts.push_back((float)(amp * (1.0 - 2.0 * phim)));
	}

	if (edge == kMasterBandLimited) {
		for (std::size_t j = 0; j < wrapIdx.size(); ++j) {
			const int    k = wrapIdx[j];
			const double g = wrapG[j];
			if (k - 1 >= 0) volts[(std::size_t)(k - 1)] += (float)(amp * (1.0 - g) * (1.0 - g));
			volts[(std::size_t)k] -= (float)(amp * g * g);
		}
	}
	return volts;
}

// ---------------------------------------------------------------------------
// PART 3 — THE RENDER LOOP.
//
// ONE forge::VcoCore. ONE pass. Every leg accumulated simultaneously from that
// one pass via the telemetry reconstruction. There is deliberately NO second
// core, NO mirror class, NO second drive and NO flag in the shipped body — see
// the binding-constraint paragraph in the banner. The core is constructed at
// exactly one site in this file, inside this function.
// ---------------------------------------------------------------------------
struct LegResult {
	std::vector<float> volts;   // output volts, pre-scaling
	double peakAbsV;
	long   nonFinite;

	// --- THE TWO METRICS THAT KEEP THE AUDITION HONEST ---------------------
	// rmsV, NOT peakAbsV, IS THE LEVEL-MATCH METRIC, and that is a measurement
	// rather than a preference. The peak of a hard-synced waveform lands ON A
	// RESET SAMPLE, which is precisely the sample the correction modifies, so
	// the two legs' PEAKS are supposed to differ - measured here by up to
	// 1.075448 V at render point 04. Judging "are these level-matched" on the
	// peak would therefore report the DSP as a rig fault. The bodies of the two
	// signals carry the same level because ONE scale factor is applied to both
	// and no leg is ever normalised; rmsV is what shows it.
	double rmsV;
	// worstResetStepV is plan 33-08's own metric, computed here so the operator
	// gate's expected-results block rests on THIS render rather than on inherited
	// figures: max |x[n] - x[n-1]| over the samples where tel.syncFired. 33-08
	// measured 9.793601 V shipped against 10.000000 V withheld grid-wide, and a
	// NEGATIVE margin (shipped worse than withheld) on 56 of 420 cells.
	double worstResetStepV;
};

std::vector<LegResult> renderPoint(const RenderPoint& p, double* masterHzOut, float* pitchCvOut,
                                   long* resetsOut) {
	const double masterHz = (double)p.masterKm * p.sr / (double)kMasterCycleDenom;
	// The slave's pitch is solved from the ratio with std::log2 — libm, available
	// in tools/ and tests/ and forbidden in src/ (the D-18 precedent). The slave
	// is FREE and is deliberately not bin-centred: forge::exp2_taylor5's own
	// approximation error moves the achieved frequency slightly off the nominal
	// ratio, exactly as it does on the measured grid, and nothing here depends on
	// the slave's frequency being exact.
	const float pitchCV = (float)std::log2(p.ratio * masterHz / (double)forge::kVcoFreqC4);
	if (masterHzOut) *masterHzOut = masterHz;
	if (pitchCvOut)  *pitchCvOut  = pitchCV;

	const int nAudible = (int)(p.seconds * p.sr + 0.5);
	const int nTotal   = kWarmupSamples + nAudible;

	const std::vector<float> master = makeMaster(nTotal, p.masterKm, 5.0, p.edge);

	// Copy-and-assign, never a brace value-list: forge::VcoInputs has NSDMIs, so
	// under C++11 it is not an aggregate and a value-list init is a hard error.
	forge::VcoInputs base;
	base.pitchCV   = pitchCV;
	base.coarse    = 0.f;
	base.fine      = 0.f;
	base.morph     = p.morph;
	base.character = p.character;
	base.drift     = 0.f;   // drift OFF, as the whole sync grid drives it

	std::vector<LegResult> legs(kCoreConfigCount);
	std::vector<float>     prevSample(kCoreConfigCount, 0.f);
	std::vector<double>    sumSq(kCoreConfigCount, 0.0);
	for (std::size_t c = 0; c < kCoreConfigCount; ++c) {
		legs[c].volts.reserve((std::size_t)nAudible);
		legs[c].peakAbsV        = 0.0;
		legs[c].nonFinite       = 0;
		legs[c].rmsV            = 0.0;
		legs[c].worstResetStepV = 0.0;
	}
	long resets = 0;

	forge::VcoCore core;
	core.seed(kDriftS0, kDriftS1);
	core.setSpreadSeed(kSpreadS0, kSpreadS1);

	const float dt = (float)(1.0 / p.sr);
	for (int i = 0; i < nTotal; ++i) {
		forge::VcoInputs in = base;
		in.sampleTime    = dt;
		in.sampleRate    = (float)p.sr;
		in.syncVolts     = master[(std::size_t)i];
		in.syncConnected = true;

		const float s    = core.step(in);
		// Read back AFTER step(), off the accumulator, from the same pass that
		// produced `s`. Plan 33-06 populates this field by measuring the seam's
		// actual deposit rather than recomputing it, so it reports nothing on a
		// sample where the seam's own entry gate rejected the event.
		const float corr  = core.tel.syncCorrection;
		// Reset samples are identified FROM TELEMETRY, never inferred from the
		// waveform. Plan 33-08 makes the same choice and gives the reason: in a
		// measurement whose entire subject is how large the step at a reset is,
		// inferring "a large step means a reset" would be circular.
		const bool  fired = core.tel.syncFired;

		const bool audible = (i >= kWarmupSamples);
		if (audible && fired) ++resets;

		for (std::size_t c = 0; c < kCoreConfigCount; ++c) {
			const float k = CORE_PAIR[c].correctionScale;
			const float x = s + 5.f * (k - 1.f) * corr;

			// The step is measured across the warm-up boundary too, so the first
			// audible reset is not silently exempt from the metric.
			if (fired && i > 0) {
				const double d = (double)std::fabs((double)x - (double)prevSample[c]);
				if (audible && d > legs[c].worstResetStepV) legs[c].worstResetStepV = d;
			}
			prevSample[c] = x;

			if (!audible) continue;

			// Negated comparison, the project idiom: BOTH comparisons of a clamp
			// ladder are false for a not-a-number, so a ladder is inert against
			// exactly the input class a finiteness check exists to catch.
			if (!(x - x == 0.f)) ++legs[c].nonFinite;
			const double a = (double)std::fabs(x);
			if (a > legs[c].peakAbsV) legs[c].peakAbsV = a;
			sumSq[c] += (double)x * (double)x;
			legs[c].volts.push_back(x);
		}
	}

	for (std::size_t c = 0; c < kCoreConfigCount; ++c) {
		const std::size_t n = legs[c].volts.size();
		legs[c].rmsV = n ? std::sqrt(sumSq[c] / (double)n) : 0.0;
	}
	if (resetsOut) *resetsOut = resets;
	return legs;
}

// ---------------------------------------------------------------------------
// PART 4 — THE OUTPUT WRITER.
//
// A 16-bit mono PCM RIFF/WAVE file. There is no audio-file writer anywhere in
// this repository and NO DEPENDENCY IS JUSTIFIED FOR ONE: the header is 44 bytes
// of fixed layout, and this phase's threat register records that no npm/PyPI/
// crates package is installed and no third-party source is vendored (T-33-SC).
// Every field is written explicitly with a comment naming it, so a reader can
// check the layout against any RIFF reference without opening this file's
// history. All multi-byte fields are written LITTLE-ENDIAN BY HAND rather than
// by memcpy'ing a struct, so the output does not depend on the host's byte order
// or on the compiler's struct padding.
// ---------------------------------------------------------------------------
void putU32le(std::ofstream& f, uint32_t v) {
	unsigned char b[4];
	b[0] = (unsigned char)(v & 0xFFu);
	b[1] = (unsigned char)((v >> 8) & 0xFFu);
	b[2] = (unsigned char)((v >> 16) & 0xFFu);
	b[3] = (unsigned char)((v >> 24) & 0xFFu);
	f.write(reinterpret_cast<const char*>(b), 4);
}

void putU16le(std::ofstream& f, uint16_t v) {
	unsigned char b[2];
	b[0] = (unsigned char)(v & 0xFFu);
	b[1] = (unsigned char)((v >> 8) & 0xFFu);
	f.write(reinterpret_cast<const char*>(b), 2);
}

// Volts -> 16-bit PCM at the ONE stated scale, counting clipped samples. The
// range test is the negated-comparison pair for the reason given above: a NaN
// must be caught, not passed through into an arbitrary sample value.
int16_t toPcm16(float volts, long& clipped) {
	float fs = volts * kVoltsToFullScale;
	if (!(fs >= -1.f))      { ++clipped; fs = -1.f; }   // catches NaN and anything under -kFullScaleVolts
	else if (!(fs <= 1.f))  { ++clipped; fs =  1.f; }
	long v = std::lround((double)fs * 32767.0);
	if (v >  32767) v =  32767;
	if (v < -32768) v = -32768;
	return (int16_t)v;
}

bool writeWav16(const std::string& path, const std::vector<float>& volts, uint32_t sampleRate,
                long& clippedOut, std::size_t& bytesOut) {
	const uint16_t kChannels      = 1;
	const uint16_t kBitsPerSample = 16;
	const uint16_t kFormatPcm     = 1;
	const uint32_t dataBytes      = (uint32_t)(volts.size() * (kBitsPerSample / 8) * kChannels);

	std::ofstream f(path.c_str(), std::ios::binary);
	if (!f) {
		std::fprintf(stderr, "render_sync_ab: cannot open %s for writing\n", path.c_str());
		return false;
	}

	// --- RIFF chunk descriptor (12 bytes) ---
	f.write("RIFF", 4);                                    // ChunkID
	putU32le(f, 36u + dataBytes);                          // ChunkSize = 36 + Subchunk2Size
	f.write("WAVE", 4);                                    // Format
	// --- "fmt " sub-chunk (24 bytes) ---
	f.write("fmt ", 4);                                    // Subchunk1ID
	putU32le(f, 16u);                                      // Subchunk1Size = 16 for PCM
	putU16le(f, kFormatPcm);                               // AudioFormat = 1 (linear PCM, uncompressed)
	putU16le(f, kChannels);                                // NumChannels = 1 (mono)
	putU32le(f, sampleRate);                               // SampleRate
	putU32le(f, sampleRate * kChannels * (kBitsPerSample / 8u));  // ByteRate
	putU16le(f, (uint16_t)(kChannels * (kBitsPerSample / 8)));    // BlockAlign
	putU16le(f, kBitsPerSample);                           // BitsPerSample
	// --- "data" sub-chunk (8 bytes + payload) ---
	f.write("data", 4);                                    // Subchunk2ID
	putU32le(f, dataBytes);                                // Subchunk2Size

	clippedOut = 0;
	for (std::size_t i = 0; i < volts.size(); ++i) {
		const int16_t s = toPcm16(volts[i], clippedOut);
		putU16le(f, (uint16_t)s);
	}

	bytesOut = 44u + (std::size_t)dataBytes;
	return true;
}

}  // namespace

int main() {
	const char* kOutDir = "build-test/audition";


	std::printf("=====================================================================\n");
	std::printf("HARD-SYNC A/B AUDITION RENDER (Phase 33, plan 33-10 / D-13..D-16)\n");
	std::printf("=====================================================================\n");
	std::printf("Output directory : %s/   (gitignored - NEVER COMMITTED, regenerate per session)\n", kOutDir);
	std::printf("Scale            : %.6f volts-to-full-scale  (%.6f V = digital full scale,\n",
	            (double)kVoltsToFullScale, (double)kFullScaleVolts);
	std::printf("                   which is kHostileBoundV, the outer output tier the test\n");
	std::printf("                   suite asserts - so a clipped sample is a FINDING)\n");
	std::printf("Format           : 16-bit mono PCM WAV, uncompressed\n");
	std::printf("Legs             : NEVER normalised per leg - one scale, applied identically\n");
	std::printf("Warm-up discarded: %d samples per point\n", kWarmupSamples);
	std::printf("Seeds            : drift 0x%llX/0x%llX  spread 0x%llX/0x%llX  (verbatim, tests/VcoBlockDriver.hpp:42-43)\n",
	            (unsigned long long)kDriftS0,  (unsigned long long)kDriftS1,
	            (unsigned long long)kSpreadS0, (unsigned long long)kSpreadS1);
	std::printf("Reconstruction   : leg_k[n] = out_shipped[n] + 5.f*(k-1.f)*tel.syncCorrection[n]\n");
	std::printf("                   ONE core, ONE pass, both legs. Exact to one ulp (33-06:\n");
	std::printf("                   49,136 of 49,152 bit-exact, 16 at one ulp, worst 4.77e-07 V).\n");
	std::printf("EXPECT A SMALL DIFFERENCE: the shipped BLEP removes about 2%% of the worst-case\n");
	std::printf("reset step on plan 33-08's grid, and improves the spectral alias floor by a\n");
	std::printf("grid-wide mean of +0.5827 dB (33-07). AND IT IS NOT A UNIFORM IMPROVEMENT:\n");
	std::printf("point 04 is where it LOSES spectrally (-1.0281 dB mean at ratio 5.50) and point\n");
	std::printf("07 is where it LOSES in the time domain (margin -0.427492 V). Point 05 is the\n");
	std::printf("null-point control where the two legs should be indistinguishable, and points\n");
	std::printf("06/07 are THE SAME CELL on two different masters - audition them back to back.\n");
	std::printf("\n");

	for (std::size_t c = 0; c < kCoreConfigCount; ++c) {
		std::printf("  leg %s  k=%.2f  %s\n",
		            CORE_PAIR[c].fileTag, (double)CORE_PAIR[c].correctionScale, CORE_PAIR[c].legName);
	}
	std::printf("\n");

	long   totalClipped   = 0;
	long   totalNonFinite = 0;
	int    filesWritten   = 0;
	bool   anyFailure     = false;

	for (std::size_t pi = 0; pi < kRenderPointCount; ++pi) {
		const RenderPoint& p = RENDER_POINTS[pi];

		double masterHz = 0.0;
		float  pitchCV  = 0.f;
		long   resets   = 0;
		const std::vector<LegResult> legs = renderPoint(p, &masterHz, &pitchCV, &resets);

		std::printf("---------------------------------------------------------------------\n");
		std::printf("[%zu/%zu] %s\n", pi + 1, kRenderPointCount, p.label);
		std::printf("      rate %.0f Hz | master %.4f Hz (K_m=%d, %s) | ratio %.2f\n",
		            p.sr, masterHz, p.masterKm, p.edgeName, p.ratio);
		std::printf("      slave %.4f Hz nominal (pitchCV %+.6f V) | morph %.2f (%s) | character %.2f\n",
		            p.ratio * masterHz, (double)pitchCV, (double)p.morph, p.region, (double)p.character);
		std::printf("      %.2f s = %zu samples | %ld sync resets fired\n",
		            p.seconds, legs[0].volts.size(), resets);
		std::printf("      WHY: %s\n", p.why);

		for (std::size_t c = 0; c < kCoreConfigCount; ++c) {
			const std::string path = std::string(kOutDir) + "/" + p.label + "__" + CORE_PAIR[c].fileTag + ".wav";
			long        clipped = 0;
			std::size_t bytes   = 0;
			if (!writeWav16(path, legs[c].volts, (uint32_t)p.sr, clipped, bytes)) {
				anyFailure = true;
				continue;
			}
			++filesWritten;
			totalClipped   += clipped;
			totalNonFinite += legs[c].nonFinite;
			std::printf("      wrote %s (%zu samples, %zu bytes)\n", path.c_str(), legs[c].volts.size(), bytes);
			std::printf("            peak %.6f V | RMS %.6f V | worst reset step %.6f V | clipped %ld | non-finite %ld\n",
			            legs[c].peakAbsV, legs[c].rmsV, legs[c].worstResetStepV, clipped, legs[c].nonFinite);
		}

		// The legs must NOT be identical. If they are, the withheld leg is not
		// actually withheld and the whole apparatus is decorative. Reported here
		// rather than asserted, because this is a tool and not a test - but it is
		// reported on every run so it cannot pass unnoticed.
		if (kCoreConfigCount >= 2) {
			std::size_t nDiff  = 0;
			double      maxAbs = 0.0;
			const std::vector<float>& a = legs[0].volts;
			const std::vector<float>& b = legs[1].volts;
			const std::size_t n = a.size() < b.size() ? a.size() : b.size();
			for (std::size_t i = 0; i < n; ++i) {
				if (a[i] != b[i]) {
					++nDiff;
					const double d = (double)std::fabs((double)a[i] - (double)b[i]);
					if (d > maxAbs) maxAbs = d;
				}
			}
			std::printf("      A vs B : %zu of %zu samples differ (%.4f%%), largest |A-B| = %.6f V\n",
			            nDiff, n, n ? 100.0 * (double)nDiff / (double)n : 0.0, maxAbs);
			std::printf("               LEVEL MATCH  : RMS A %.6f V vs B %.6f V, delta %+.6f V (%+.4f dB)\n",
			            legs[0].rmsV, legs[1].rmsV, legs[0].rmsV - legs[1].rmsV,
			            (legs[0].rmsV > 0.0 && legs[1].rmsV > 0.0)
			                ? 20.0 * std::log10(legs[0].rmsV / legs[1].rmsV) : 0.0);
			// The RESET-STEP margin, in plan 33-08's own sign convention:
			// withheld minus shipped, so POSITIVE means the correction helped on
			// this render point and NEGATIVE means it made the worst step LARGER.
			// 33-08 measured a negative margin on 56 of 420 cells; this line is
			// what lets the operator gate state which of these points are which
			// FROM THIS RENDER rather than from an inherited table.
			const double margin = legs[1].worstResetStepV - legs[0].worstResetStepV;
			std::printf("               RESET STEP   : worst A %.6f V vs B %.6f V, margin %+.6f V  <-- %s\n",
			            legs[0].worstResetStepV, legs[1].worstResetStepV, margin,
			            margin > 0.0 ? "the correction HELPS here"
			                         : "the correction is NO BETTER OR WORSE here (33-08's negative-margin region)");
			if (nDiff == 0)
				std::printf("      >>> WARNING: the two legs are BIT-IDENTICAL. The withheld leg is not withheld.\n");
		}
	}

	std::printf("---------------------------------------------------------------------\n");
	std::printf("DONE: %d files written into %s/\n", filesWritten, kOutDir);
	std::printf("      total clipped samples   : %ld  (expected 0 - see the scale note above)\n", totalClipped);
	std::printf("      total non-finite samples: %ld  (expected 0)\n", totalNonFinite);
	std::printf("      These files are NOT committed and are NOT a golden. Regenerate with\n");
	std::printf("      `make audition` in every session that needs them.\n");
	return anyFailure ? 1 : 0;
}
