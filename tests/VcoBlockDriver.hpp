#pragma once
// tests/VcoBlockDriver.hpp
//
// Headless block-driver harness over forge::VcoCore (TEST-01). Drives the
// Phase 29 seam over a block of N samples, injecting sampleTime = 1/sampleRate
// and sampleRate per sample, and seeding the core deterministically.
// Links NOTHING outside the Rack-free core (zero rack/ includes).
//
// Source: an INDEPENDENT COPY of tests/BlockDriver.hpp, retargeted to the VCO
// seam. It is deliberately NOT shared with that file (R-2 / P-4).
//
// Why ~40 duplicated lines are the correct design here, and why no one should
// "DRY" this later: tests/BlockDriver.hpp feeds the macOS bit-exact drift-ON
// golden replay leg of the SHIPPED Analog LFO. Templating the two drivers into
// a common base, or touching BlockDriver's ctor seed defaults or its run()
// loop, changes what that leg feeds forge::LfoCore and moves
// tests/golden/freerun_*.f32. The two drivers are independent files, forever.
// Do not template them. Do not subclass. Do not alias.
//
// Seeding note (Pitfall 4 / freerun_seeds.txt landmine): VcoCore::seed(s0,s1)
// seeds ONLY the drift RNG; component-spread coefficients stay zero until
// setSpreadSeed(sp0,sp1) is called. Both default to non-zero values here because
// forge::Xoroshiro128Plus seeded (0,0) is a degenerate fixed point that emits an
// all-zero stream, which makes std::normal_distribution loop forever. The four
// default values below are copied verbatim from tests/BlockDriver.hpp — already
// documented, already proven non-degenerate. Do not invent new ones.

#include "dsp/VcoCore.hpp"

#include <vector>
#include <functional>
#include <cstdint>

namespace forge {

struct VcoBlockDriver {
	forge::VcoCore core;
	double sampleRate = 44100.0;

	// Default seeds are non-zero (never the degenerate (0,0) Xoroshiro fixed point).
	explicit VcoBlockDriver(double sr = 44100.0,
	                        uint64_t s0 = 0x1234ULL, uint64_t s1 = 0x5678ULL,
	                        uint64_t sp0 = 0x9E3779B9ULL, uint64_t sp1 = 0x7F4A7C15ULL)
		: sampleRate(sr) {
		core.seed(s0, s1);
		core.setSpreadSeed(sp0, sp1);
	}

	// Drive nSamples through the core. inputAt(i) supplies the per-sample
	// VcoInputs; sampleTime and sampleRate are ALWAYS overwritten (the harness
	// owns timing). This overwrite is load-bearing — it must never become
	// conditional on what the caller's functor happened to put there.
	std::vector<float> run(int nSamples, const std::function<forge::VcoInputs(int)>& inputAt) {
		std::vector<float> out;
		out.reserve(nSamples);
		const float dt = (float)(1.0 / sampleRate);
		for (int i = 0; i < nSamples; ++i) {
			forge::VcoInputs in = inputAt(i);
			in.sampleTime = dt;
			in.sampleRate = (float)sampleRate;
			out.push_back(core.step(in));
		}
		return out;
	}

	// Varying-input sweep across the block, holding the rest of `base` constant.
	// Returns an inputAt() functor for run().
	//
	// Anti-vacuity driver (P-7): while the Phase 29 seam is silent by design, a
	// constant input would make "same seeds produce bit-identical blocks" and
	// "output is finite" trivially true forever — and they would stay trivially
	// true even after Phase 30 lands DSP, because nothing would ever modulate.
	// Sweeping pitch/morph/character means those invariants start proving
	// something the instant real DSP replaces VcoCore::step().
	static std::function<forge::VcoInputs(int)> sweepScenario(int nSamples, forge::VcoInputs base) {
		const float denom = (float)(nSamples > 1 ? nSamples - 1 : 1);
		return [=](int i) {
			// Copy + assign, never a brace value-list: VcoInputs has NSDMIs, so
			// under C++11 it is not an aggregate and a value-list init is a hard
			// error (P-8). Kept C++11-shaped even though tests build at C++17.
			forge::VcoInputs in = base;
			const float t = (float)i / denom;   // normalized position in [0,1]
			in.pitchCV   = -2.f + 4.f * t;      // -2 V .. +2 V
			in.morph     = t;                   // 0 .. 1
			in.character = 1.f - t;             // 1 .. 0
			return in;
		};
	}
};

} // namespace forge
