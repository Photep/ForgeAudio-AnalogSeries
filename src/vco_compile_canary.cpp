// src/vco_compile_canary.cpp
//
// PERMANENT C++11 / ODR COMPILE CANARY — Phase 29 decisions D-07 and D-08.
//
// PURPOSE. This translation unit exists to force every VCO DSP header through
// two gates that would otherwise cover no VCO code at all:
//   1. the -std=c++11 -pedantic-errors syntax gate (`make strict` locally, and
//      the CI "Strict C++11 pedantic gate" step), which hard-errors on the
//      C++17-isms the Rack toolchain rejects; and
//   2. the CI MinGW compile-plus-link-against-libRack leg, which is the ONLY
//      gate that catches the in-class `static constexpr` failure class. Under
//      C++11 an in-class initializer is a DECLARATION only, so runtime indexing
//      odr-uses it and MinGW's linker fails with "undefined reference". That
//      exact class got version 2.0.0 rejected from the VCV Library; the shipped
//      fix is the out-of-line definition block at the end of src/AnalogLFO.cpp.
//      There is no local substitute for this gate: Apple clang materializes the
//      same construct as a per-translation-unit local symbol and links cleanly
//      at every optimization level.
// Both gates only see headers reached through a .cpp. Until Phase 30 lands
// src/AnalogVCO.cpp, nothing else under src/ includes a VCO header — so without
// this file both gates would report PASS while covering zero VCO code.
//
// GROWTH RULE (D-08). EVERY new VCO header must be added to the include list
// below by the phase that creates it. Phase 32's morph-BLEP header
// (dsp/MorphBlep.hpp) is CARRIED as of this commit — it landed with plan 32-04
// and its include below was activated in the SAME commit that created it,
// because tests/check_canary.sh [5/5] strips comment lines before it looks, so
// a commented placeholder stops satisfying the check the instant the header
// exists. The rule itself stands UNCHANGED for the phase that adds the next
// header. This is not left as a convention: tests/check_canary.sh section [5/5]
// fails if any src/dsp/Vco*.hpp or dsp/MorphBlep.hpp is missing from the live
// includes here.
//
// NOT DEAD CODE — DO NOT DELETE, DO NOT MAKE static, DO NOT REDUCE TO A BARE
// #include. This translation unit deliberately contains no DSP and
// forge::vcoCompileCanaryProbe is never called at runtime. That is the design,
// not leftover scaffolding. A translation unit that only includes a header
// emits no code, odr-uses nothing, and leaves the link leg with nothing to
// resolve — it would be permanently and silently green. tests/check_canary.sh
// section [2/5] runs nm over this file's object and fails the build if the
// defined probe symbol ever stops being emitted.
//
// SHIPPED-ARTIFACT COST (operator-approved: Phase 29 plan 29-03 Task 1, answer
// `option-a`). Living under src/ means this file is picked up by
// `SOURCES += $(wildcard src/*.cpp)` and compiled into the released
// plugin.dylib / plugin.so / plugin.dll distributed through the VCV Library, so
// one unused namespaced external-linkage symbol ships. It has no static
// initializer, is never invoked, references no LFO code path, and its name is
// unique so it cannot collide. That cost was weighed against placing this file
// in tools/ — which would have needed three separate build/CI wiring edits that
// can silently rot — and was accepted deliberately. Living here also means this
// file is gated identically to how the real src/AnalogVCO.cpp will be gated in
// Phase 30, which is D-08's stated intent.
//
// Include hygiene: NO Rack-SDK header (this file must stay link-checkable
// without the SDK) and NO tests/ header (tests/ builds at C++17 and would break
// the C++11 gate this file exists to exercise).

#include "dsp/VcoCore.hpp"
// D-08 growth point — TAKEN by Phase 32 (plan 32-04). The next phase to add a
// VCO header appends its include below this line.
//
// The runtime-derived field block below needs NO addition for Phase 32, and
// that is a FINDING rather than an omission: D-17 keeps all MORPH CV summing in
// the shell, so Phase 32 adds no forge::VcoInputs field for the block to derive
// from `i`. forge::MorphBlep is reached through forge::VcoCore::step (plan
// 32-06), whose inputs are already runtime-derived here, so this header is
// odr-used with non-constant arguments exactly as [2b/5] requires.
#include "dsp/MorphBlep.hpp"

namespace forge {

// Declared, then defined. The separate declaration is deliberate: it gives the
// definition below external linkage that no compiler can prove unreachable, so
// neither the optimizer nor the linker may discard it.
float vcoCompileCanaryProbe(int i);

float vcoCompileCanaryProbe(int i) {
	VcoCore core;
	core.seed(0x1234ULL, 0x5678ULL);
	core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);

	// Never brace-initialize VcoInputs with a value list (P-8): under C++11 a
	// class with non-static data member initializers is not an aggregate.
	VcoInputs in;
	in.sampleTime = 1.f / 44100.f;
	in.sampleRate = 44100.f;

	// LOAD-BEARING — DO NOT REPLACE THESE WITH LITERALS.
	//
	// Every DSP field the seam may index, branch or table-look-up on must be
	// derived from the runtime parameter `i`. A literal here (including the NSDMI
	// defaults, which are literals) lets -O2/-O3 constant-propagate the value into
	// the index after inlining, fold the lookup to a constant, and DELETE the
	// odr-use before the linker is ever consulted — which is precisely the failure
	// this file exists to catch. Measured: with an in-class `static constexpr float
	// kTable[4]` indexed by `in.pitchCV` in dsp/VcoCore.hpp, a canary fed only
	// constants emits NO kTable symbol at -O3 at all, so the MinGW link leg has
	// nothing to fail on and every gate reports PASS.
	//
	// A runtime-derived LOOP TRIP COUNT is NOT sufficient on its own: it preserves
	// how many times step() is called, but nothing inside step() depends on it.
	// tests/check_canary.sh [2b/5] asserts this property mechanically.
	in.pitchCV     = (float)(i & 7) - 4.f;
	in.coarse      = (float)((i >> 3) & 3);
	in.fine        = (float)((i >> 5) & 3);
	in.fmVolts     = (float)((i >> 7) & 7) * 0.25f;
	in.fmAtten     = (float)((i >> 10) & 3) * 0.5f - 1.f;
	in.fmConnected = ((i >> 12) & 1) != 0;
	in.morph       = (float)(i & 15) / 15.f;
	in.character   = (float)((i >> 4) & 15) / 15.f;
	in.drift       = (float)((i >> 8) & 15) / 15.f;

	// The runtime-derived trip count is load-bearing too, exactly like the external
	// linkage above. A compile-time-constant count would let the compiler unroll
	// and collapse the loop; combined with constant inputs the translation unit
	// then emits nothing, the VCO headers are odr-used by nothing, and the MinGW
	// link leg has nothing to resolve.
	const int reps = (i & 3) + 1;
	float acc = 0.f;
	for (int n = 0; n < reps; ++n) {
		// Keeps every iteration distinct, so per-iteration folding cannot collapse
		// the loop body to a single constant evaluation either.
		in.pitchCV += (float)n * 0.125f;
		acc += core.step(in);
	}

	// Reading telemetry odr-uses the nested Telemetry type as well, so the whole
	// seam — not just step() — is carried into both gates.
	return acc + core.tel.displayPhase + core.tel.freqHz;
}

} // namespace forge
