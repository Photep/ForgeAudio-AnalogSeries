#pragma once
// src/dsp/VcoCore.hpp
//
// Phase 29 VCO boundary seam (D-01): a POD forge::VcoInputs goes in, an output
// voltage comes out, and last-step telemetry is left behind for the shell. That
// is the WHOLE contract. step() deliberately returns silence (0 V) — there is
// NO VCO DSP in Phase 29. Phase 30 (CORE-01) lands the naive morphed oscillator
// and Phase 31 (CORE-03 / PITCH-*) lands the pitch chain; this header exists now
// so Phases 30-36 inherit a boundary that never has to churn.
//
// Naming landmine (R-9): the POD is forge::VcoInputs, NOT forge::Inputs.
// forge::Inputs already belongs to the LFO (src/dsp/LfoCore.hpp:40). A second
// struct of that name in another header is a cross-TU ODR violation that
// compiles silently in any TU that includes only one of the two headers, and
// only detonates at link time on the strictest toolchain (MinGW/GCC in CI).
//
// Sync fields (syncVoltage / syncConnected) are deliberately ABSENT. Phase 33
// owns hard sync and adds them then; adding POD fields later is a non-breaking
// additive change because the VCO has no golden fixtures until Phase 36.
//
// Two-standard rule (TEST-06): this header must compile clean under BOTH
// -std=c++11 -pedantic-errors (the Rack plugin toolchain — this is the standard
// the shipped build uses, enforced by `make strict` and the CI MinGW leg) AND
// -std=c++17 (the Rack-free test target). The C++11 restrictions below are
// therefore binding on this file and on every VCO header that follows it:
//   - No `inline constexpr` variables (C++17 inline variables). Plain constexpr
//     at namespace scope has internal linkage per TU and is the C++11 form —
//     see src/dsp/MathConst.hpp for the same rationale.
//   - No `if constexpr`, no structured bindings, no [[maybe_unused]], no
//     nested-namespace definition syntax, no auto return-type deduction, no
//     generic lambdas.
//   - No std::clamp (C++17 <algorithm>) — use forge::clamp / forge::clampi from
//     dsp/RackCompat.hpp if clamping is ever needed here.
//   - No pi macro from <cmath> — use forge::kPi from dsp/MathConst.hpp.
//   - Any constant table must be a namespace-scope `static constexpr`, never an
//     in-class `static constexpr` that is indexed at runtime: under C++11 the
//     in-class form is a DECLARATION only, and runtime indexing odr-uses it,
//     producing an undefined reference at MinGW link time. That exact class of
//     bug got v2.0.0 rejected from the VCV Library. Phase 29 needs no table; the
//     rule is recorded here because Phases 30+ will.
//   - Do NOT brace-initialize VcoInputs with a value list. Under C++11 a class
//     with non-static data member initializers is not an aggregate, so
//     `VcoInputs in{1.f, 2.f}` is a hard error. `VcoInputs in;` and
//     `VcoInputs in{};` are both fine.
//
// Include hygiene (Pitfall 1 / TEST-01 / TEST-06): ZERO Rack-SDK includes.

#include <cstdint>

#include "dsp/DriftEngine.hpp"   // forge::DriftEngine

namespace forge {

// POD core boundary (D-03; RESEARCH "Recommended VcoInputs field set"). Each
// field maps to the params[]/inputs[]/ProcessArgs source the shell will read;
// the core never sees Rack indices. Field set covers the near-term Phase 30/31
// needs; sync is deferred to Phase 33.
struct VcoInputs {
	float pitchCV = 0.f;        // V/OCT input volts (Phase 30/31)
	float coarse = 0.f;         // coarse tune, octaves (Phase 31)
	float fine = 0.f;           // fine tune, semitones (Phase 31)
	float fmVolts = 0.f;        // exponential FM input volts, summed into the pitch volt domain before the single exp2 (Phase 31)
	float fmAtten = 0.f;        // bipolar FM attenuverter (Phase 31)
	bool  fmConnected = false;  // FM jack patched
	float morph = 0.f;          // post-CV, post-clamp [0,1] (Phase 34; name matches forge::Inputs::morph on purpose so the shared Waveshape/DriftEngine wiring is copy-paste later)
	float character = 0.f;      // post-CV, post-clamp [0,1] (Phase 34)
	float drift = 0.f;          // post-CV, post-clamp [0,1] (Phase 34)
	float sampleTime = 1.f / 44100.f;  // INJECTED by the harness/shell, never read from a global
	float sampleRate = 44100.f;        // INJECTED; the Nyquist clamp (PITCH-04, Phase 31) needs it
};

struct VcoCore {
	// Analog-character engine (CHAR-*). Held and seeded from day one even though
	// Phase 29 never steps it: it makes the driver's seeding discipline real
	// immediately, so Phases 30/34 do not have to change VcoBlockDriver when the
	// DSP lands. Holding + seeding an RNG is not DSP.
	DriftEngine drift;

	// --- Last-step telemetry (the shell reads these to feed display atomics;
	//     NOT part of the audio path). Populated by step() each sample. ---
	struct Telemetry {
		float freqHz = 0.f;        // final oscillator frequency (Phase 31)
		float displayPhase = 0.f;  // wrapped phase for the animated display (Phase 35)
		bool  syncFired = false;   // a hard-sync reset fired this sample (Phase 33)

		// seam observability (TEST-01) — populated by step() even while the seam
		// is silent, so the harness's timing-injection assertions are NOT vacuous.
		float lastSampleTime = 0.f;
		float lastSampleRate = 0.f;
		uint32_t stepCount = 0;
	};
	Telemetry tel;

	void seed(uint64_t s0, uint64_t s1 = 0) { drift.seed(s0, s1); }
	void setSpreadSeed(uint64_t s0, uint64_t s1 = 0) { drift.setSpreadSeed(s0, s1); }

	// step() — the Phase 29 seam. Records the injected timing so the harness can
	// prove it, counts the sample, and returns SILENCE.
	float step(const VcoInputs& in) {
		tel.lastSampleTime = in.sampleTime;
		tel.lastSampleRate = in.sampleRate;
		++tel.stepCount;

		// Returning 0 V is D-01, not an oversight: Phase 29 is the boundary
		// contract only. Phase 30 (CORE-01) replaces this body with the naive
		// morphed oscillator, at which point tests/test_vco_harness.cpp's
		// "silent by construction" tombstone case must be deleted.
		return 0.f;
	}
};

} // namespace forge
