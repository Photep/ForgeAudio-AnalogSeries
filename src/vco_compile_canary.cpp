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
//
// PHASE 33 ADDS NO VCO HEADER EITHER, so the growth point is still TAKEN by
// Phase 32 and this include list is unchanged. Recorded rather than left silent,
// because the field block below DID need addition this time and the two rules
// are easy to conflate: D-08 governs HEADERS, and Phase 33's hard sync lives
// entirely inside dsp/VcoCore.hpp, which is already the first include above. The
// separate growth rule that governs FIELDS bound hard — D-02 adds two
// forge::VcoInputs members and both are fed below.
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
	//
	// ---------------------------------------------------------------------
	// PHASE 33 ADDS A STRICTLY STRONGER REQUIREMENT THAN "NO LITERALS", AND
	// THIS PARAGRAPH IS THE REASON THE TWO SYNC FEEDS BELOW ARE NOT MERELY
	// MECHANICAL.
	//
	// The rule above is about a field's VALUE being unfoldable. `syncConnected`
	// introduces a second failure mode that the rule above does not reach,
	// because the field is not an operand of any arithmetic — it is the OUTER
	// GATE on the whole hard-sync block in forge::VcoCore::step. Fed a constant
	// `false`, -O3 does not merely fold a value: it proves the branch dead and
	// DELETES the detector, the SchmittTrigger transition, the sub-sample solve,
	// the reset and the extra morphedWave call. The MinGW link leg would then
	// have nothing of the sync path left to resolve, and the canary would cover
	// the newest code in the seam with NOTHING.
	//
	// AND THE GATE WOULD STILL REPORT PASS. tests/check_canary.sh [2b/5]
	// enumerates the FLOAT members of struct VcoInputs, so `syncConnected` — a
	// bool — is not in its field list at all and could never be reported
	// missing. `syncVolts` IS enumerated, so a constant-false flag paired with a
	// runtime-derived voltage produces the worst available outcome: a green
	// per-field report over a branch that no longer exists. That is a false
	// green of exactly the class this whole file was written against.
	//
	// THE TWO OBLIGATIONS THAT FOLLOW, both of which the feeds below satisfy:
	//   1. `syncConnected` must be a runtime BIT TEST — never a literal, never
	//      the NSDMI default (which is a literal `false`) — so the flag VARIES
	//      with the runtime parameter and is true for half of its domain. It is
	//      written in exactly the shape `fmConnected` uses, for that reason.
	//   2. `syncVolts` must STRADDLE BOTH hysteresis thresholds (0.1 V low,
	//      1.0 V high — see the syncTrig.process call in dsp/VcoCore.hpp). A
	//      voltage that only ever sat on one side of them would leave the
	//      trigger idling in a single state, so the LOW -> HIGH arm — the only
	//      arm that returns true, and therefore the only route into the solve
	//      and the reset — would never be exercised. The bit slice below spans
	//      -3 V to +4 V, which crosses both, and the per-iteration term inside
	//      the loop makes the trigger transition WITHIN a call rather than only
	//      across calls.
	//
	// DO NOT "SIMPLIFY" EITHER FEED. A later editor who replaces the flag with
	// `true` has not simplified anything — `true` is a literal too, and it
	// deletes the unpatched half of the branch instead of the patched half.
	// ---------------------------------------------------------------------
	//
	// BIT BUDGET, enumerated rather than assumed, because the two naming groups
	// below deliberately OVERLAP each other and "non-overlapping" cannot be read
	// off the shift amounts alone. Group A (pitch/tune/FM) occupies bits 0-12:
	// pitchCV 0-2, coarse 3-4, fine 5-6, fmVolts 7-9, fmAtten 10-11,
	// fmConnected 12. Group B (morph/character/drift) occupies bits 0-11:
	// morph 0-3, character 4-7, drift 8-11. The loop trip count below re-uses
	// bits 0-1. The highest bit spoken for anywhere is therefore 12, so the two
	// new slices start at 13 and 16 and collide with nothing.
	in.pitchCV     = (float)(i & 7) - 4.f;
	in.coarse      = (float)((i >> 3) & 3);
	in.fine        = (float)((i >> 5) & 3);
	in.fmVolts     = (float)((i >> 7) & 7) * 0.25f;
	in.fmAtten     = (float)((i >> 10) & 3) * 0.5f - 1.f;
	in.fmConnected = ((i >> 12) & 1) != 0;
	in.morph       = (float)(i & 15) / 15.f;
	in.character   = (float)((i >> 4) & 15) / 15.f;
	in.drift       = (float)((i >> 8) & 15) / 15.f;
	in.syncVolts   = (float)((i >> 13) & 7) - 3.f;   // bits 13-15 -> -3..+4 V: crosses BOTH 0.1 V and 1.0 V
	in.syncConnected = ((i >> 16) & 1) != 0;         // bit 16, a bit test exactly like fmConnected — never a literal

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
		// The master voltage OSCILLATES by a 4 V step — up on even iterations,
		// down on odd — which is wider than the 0.9 V hysteresis band. It must
		// oscillate rather than accumulate: `in.pitchCV` above is deliberately
		// monotonic, but a monotonic syncVolts would walk past the thresholds
		// once and never come back, leaving the trigger latched and the
		// LOW -> HIGH arm — the only arm that returns true, and so the only route
		// into the sub-sample solve and the reset — permanently unreached. With
		// the -3..+4 V base above, part of the runtime parameter's domain drives
		// a genuine LOW -> HIGH transition WITHIN a single call rather than only
		// across separate calls the compiler cannot see anyway.
		in.syncVolts += (float)(1 - 2 * (n & 1)) * 4.f;
		acc += core.step(in);
	}

	// Reading telemetry odr-uses the nested Telemetry type as well, so the whole
	// seam — not just step() — is carried into both gates.
	return acc + core.tel.displayPhase + core.tel.freqHz;
}

} // namespace forge
