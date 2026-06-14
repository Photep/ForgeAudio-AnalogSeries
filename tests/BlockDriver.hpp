#pragma once
// tests/BlockDriver.hpp
//
// Headless block-driver harness over forge::LfoCore (TEST-04). Drives the
// extracted DSP core over a block of N samples, injecting
// sampleTime = 1/sampleRate per sample and seeding the core deterministically.
// Links NOTHING outside the Rack-free core (zero rack/ includes).
//
// Source: RESEARCH.md L422-461 (BlockDriver template + clockedScenario helper),
// fleshed out against the actual LfoCore boundary (src/dsp/LfoCore.hpp).
//
// Seeding note (Pitfall 4 / freerun_seeds.txt landmine): LfoCore::seed(s0,s1)
// seeds ONLY the drift RNG; component-spread coefficients stay zero until
// setSpreadSeed(sp0,sp1) is called. Both default to non-zero values here because
// forge::Xoroshiro128Plus seeded (0,0) is a degenerate fixed point that emits an
// all-zero stream, which makes std::normal_distribution loop forever. For golden
// replay, construct with the EXACT (s0,s1,sp0,sp1) documented in freerun_seeds.txt.

#include "dsp/LfoCore.hpp"

#include <vector>
#include <functional>
#include <cmath>
#include <cstdint>

namespace forge {

struct BlockDriver {
	forge::LfoCore core;
	double sampleRate = 44100.0;

	// Default seeds are non-zero (never the degenerate (0,0) Xoroshiro fixed point).
	// sp0/sp1 default to the canonical golden spread seeds so any drift-on run
	// produces the same component-spread coefficients as the frozen baseline.
	explicit BlockDriver(double sr = 44100.0,
	                     uint64_t s0 = 0x1234ULL, uint64_t s1 = 0x5678ULL,
	                     uint64_t sp0 = 0x9E3779B9ULL, uint64_t sp1 = 0x7F4A7C15ULL)
		: sampleRate(sr) {
		core.seed(s0, s1);
		core.setSpreadSeed(sp0, sp1);
	}

	// Drive nSamples through the core. inputAt(i) supplies the per-sample Inputs;
	// sampleTime is always overwritten to 1/sampleRate (the harness owns timing).
	std::vector<float> run(int nSamples, const std::function<forge::Inputs(int)>& inputAt) {
		std::vector<float> out;
		out.reserve(nSamples);
		const float dt = (float)(1.0 / sampleRate);
		for (int i = 0; i < nSamples; ++i) {
			forge::Inputs in = inputAt(i);
			in.sampleTime = dt;
			out.push_back(core.step(in));
		}
		return out;
	}

	// Square-wave clock at `bpm` (rising edge to +10V, 50% duty) into clkVoltage,
	// holding the rest of `base` constant. Returns an inputAt() functor for run().
	static std::function<forge::Inputs(int)> clockedScenario(double sr, double bpm,
	                                                         forge::Inputs base) {
		const double period = 60.0 / bpm;   // seconds per beat
		const double half   = period * 0.5;
		return [=](int i) {
			forge::Inputs in = base;
			const double t  = (double)i / sr;
			const double ph = std::fmod(t, period);
			in.clkVoltage   = (ph < half) ? 10.f : 0.f;
			in.clkConnected = true;
			return in;
		};
	}
};

} // namespace forge
