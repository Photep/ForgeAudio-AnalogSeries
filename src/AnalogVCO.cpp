// src/AnalogVCO.cpp
//
// The minimum-viable Rack shell for the VCO — Phase 30 built it around the
// skeleton core, Phase 31 added the tune and FM controls. It owns Rack indices;
// forge::VcoCore owns arithmetic; the POD between them is the same one the
// Phase-29 harness drives.
//
// THIS FILE DOES NO DSP, AND THAT IS LOAD-BEARING. No pitch maths, no output
// scaling, no conditioning, no clamping, no smoothing. Every sample Rack hears
// comes out of core.step(in). The headless suite in tests/ is only evidence
// about what Rack produces for exactly as long as that stays true — grow a
// calculation here and tests/test_vco_core.cpp silently stops describing the
// module a user plugged a cable into.
//
// Ten controls, no more (D-07, carried forward by Phase 31's D-16 and again by
// Phase 32's D-16): V/OCT in, FM in, MORPH CV in, MORPH, CHARACTER, COARSE,
// FINE, FM DEPTH, MORPH DEPTH, OUT. They are the ten the DSP consumes, so every
// control that moves is a control you can hear and an in-Rack check is honest.
// The converse binds just as hard, and it is why all four Phase-31 controls are
// declared in the same phase as the arithmetic they feed: DSP that no control
// can reach cannot be auditioned, and this phase signs off in an
// operator-driven Rack session rather than on a headless count.
//
// PHASE 32'S TWO ARE THAT SAME RULE APPLIED TO ITSELF, which is the whole
// argument for declaring them here rather than in Phase 34. The audio-rate MORPH
// sweep that MORPH CV and MORPH DEPTH enable — the crossfade crossing segment
// boundaries at modulator rates — is the HARDEST case this phase's alias floor
// has to survive, and it is exactly what the operator drives in plan 32-10's
// in-Rack UAT session. A band-limiter with no cable to sweep it could only ever
// be signed off headless.
// CHARACTER is here as a consequence of D-11, not as a pull-forward of CHAR-01:
// every component-spread coefficient in forge::Waveshape is gated behind
// character >= 0.001f, so at character = 0 the per-instance divergence is
// invisible in Rack. Later phases add their controls alongside the behavior that
// reads them; nothing has shipped, so param and input ID churn is free right now
// and declaring the full Phase-35 enum early would buy stability nobody needs
// yet.
//
// The two tune controls read in DIFFERENT units on purpose (D-04): COARSE in
// octaves, FINE in cents, so they are visibly different tools rather than one
// control at two zoom levels. Reading both in cents was rejected — ten octaves
// then display as awkward four-digit numbers. Reading COARSE in Hz was rejected
// for a harder reason: a displayed frequency becomes a LIE the instant a cable is
// patched into V/OCT, because a param declaration cannot see an input. The
// conversion that produces the cents readout lives inside the SDK's quantity
// object and never in this file — see the FINE declaration below.
//
// The FM depth control's PHYSICAL form is deliberately NOT settled here. Whether
// it ends up a full knob or a scalloped trimpot is Phase 35's call, because that
// phase owns the real layout and the whole control budget. This phase declares
// the param, gives it a stock widget and a marker rect, and stops there.
//
// Stock SDK widgets by decision (D-08). The Forge Noir knob and jack structs are
// LOCAL to src/AnalogLFO.cpp, the shipped module's translation unit; reusing
// them would mean extracting components out of live, released source. Not doing
// that is why src/AnalogLFO.cpp does not appear in this milestone's diff at all,
// which is the cleanest position this phase has against the guardrail. A
// dedicated pre-Phase-35 phase owns the knob redesign and its backport.
//
// The panel is throwaway on purpose. res/AnalogVCO.svg is twelve rectangles at
// the FINAL 18 HP geometry and the FINAL filename, so Phase 35 (PANEL-01/PANEL-02)
// is an art swap rather than a rewiring. The CRT display is Phase 35's
// DISP-01..03 and is deliberately absent here, as is any patch-state
// serialization: the VCO persisted nothing in Phase 30 and still persists
// nothing in Phase 31, so no control declared above survives a patch save yet.
//
// Band-limiting is Phase 32's, and it lands in the SAME phase as the two MORPH
// CV controls above — but it lands ENTIRELY BEHIND THIS FILE. The correction
// lives in src/dsp/MorphBlep.hpp and is called from forge::VcoCore::step, and
// nothing in this shell changes as a consequence of it. That is not a
// coincidence, it is the property being protected: a whole anti-aliasing
// subsystem arrived without one sample of it passing through this translation
// unit, which is precisely what keeps the headless suite evidence about what
// Rack produces. The oscillator this shell exposed BEFORE that landed was crude
// and aliased on purpose, and the phases that said so were describing
// forge::VcoCore, never this file.
//
// Toolchain contract: this file joins SOURCES += $(wildcard src/*.cpp), the
// `make strict` glob and the CI toolchain-gate MinGW compile-and-link loop
// AUTOMATICALLY — no Makefile edit and no CI wiring exists to forget, which also
// means a C++11 violation here is a real VCV Library submission blocker rather
// than test-only noise. Every rule in src/dsp/VcoCore.hpp's banner binds here
// identically: no C++17 constructs, no standard-library clamp helper, no
// compile-time-conditional branch form, no in-class constant table indexed at
// runtime (that exact class got v2.0.0 REJECTED from the library), and never a
// brace value-list init of forge::VcoInputs — its NSDMIs make it a non-aggregate
// under C++11, so that is a hard error, not a style question.

#include "plugin.hpp"
#include "dsp/VcoCore.hpp"   // forge::VcoCore — the extracted DSP core the shell delegates to

struct AnalogVCO : Module {
	enum ParamId {
		MORPH_PARAM,
		CHARACTER_PARAM,
		COARSE_PARAM,
		FINE_PARAM,
		FM_ATTEN_PARAM,
		MORPH_ATTEN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		FM_INPUT,
		MORPH_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	forge::VcoCore core;

	AnalogVCO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MORPH_PARAM, 0.f, 1.f, 0.f, "Morph");
		configParam(CHARACTER_PARAM, 0.f, 1.f, 0.f, "Character");

		// A full COARSE sweep is TEN octaves and it is CONTINUOUS. PITCH-02 says
		// "continuously" and D-02 honours that literally: nothing here turns on
		// the SDK's integer-stepping flag, nothing quantises the value, and no
		// right-click menu offers whole-octave steps. That idea exists and is a
		// deferred item, not an omission. The default of zero is concert pitch,
		// which Rack's click-to-default already returns the knob to, so no extra
		// mechanism is needed for "get me back to A440".
		configParam(COARSE_PARAM, -5.f, 5.f, 0.f, "Coarse Tune", " oct");

		// FINE's RAW range is SEMITONES and its readout is CENTS. The raw range
		// is what forge::VcoInputs documents the field as, and D-05 keeps this
		// shell a pure forwarder, so the trailing 100 is a DISPLAY multiplier:
		// the semitone-to-cent conversion happens inside the SDK's quantity
		// object, never here. A full sweep is one semitone, which is what
		// doubles the raw resolution for the unison-beating job the control
		// exists for.
		//
		// DIVERGENCE FROM D-04's ILLUSTRATION, recorded on purpose. Rack's
		// default readout carries five significant digits, so these tooltips
		// read "+2.0000 oct" and "-14.000 cents" rather than the two-decimal
		// examples the decision draws. D-04 fixes the UNITS, not the digit
		// count, and the shipped module tightens the digit count nowhere either.
		// If exactness ever matters it is one line per param on the pointer the
		// declaration hands back — cosmetic, and nothing structural depends on
		// it.
		configParam(FINE_PARAM, -1.f, 1.f, 0.f, "Fine Tune", " cents", 0.f, 100.f);

		// BIPOLAR by FM-02 / D-07, so a negative setting gives inverted FM. The
		// shipped module's styling is borrowed — linear taper, default off,
		// percentage readout, and the very same control name — but its RANGE is
		// NOT: that control is a unipolar attenuator over 0..1, and copying its
		// range here would silently drop half of what FM-02 asks for.
		configParam(FM_ATTEN_PARAM, -1.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f);

		// BIPOLAR by D-17, which inherits Phase 31's D-07 attenuverter styling
		// WHOLESALE rather than re-deciding it: linear taper, default off, and a
		// percentage readout, exactly like the FM depth control declared
		// immediately above. RANGE included this time — both controls are
		// genuine attenuVERTERS over -1..+1, not the shipped LFO's unipolar
		// attenuator over 0..1.
		//
		// WHY BIPOLAR RATHER THAN UNIPOLAR, since MORPH's own range is 0..1 and
		// a one-directional control would look like the obvious match for it. A
		// negative setting INVERTS the CV, so a rising modulator sweeps the
		// crossfade BACKWARDS — pulse toward sine — while the same patch cable
		// at a positive setting sweeps it forwards, sine toward pulse. Halving
		// that to one direction would make the modulator's own polarity the only
		// way to reach the other, which is a patching chore the panel can just
		// remove.
		//
		// THE PHASE BOUNDARY, STATED HERE BECAUSE THIS IS THE EXACT LINE A LATER
		// READER WILL GET WRONG. MORPH's CV jack and attenuverter are declared
		// HERE, in Phase 32, by D-16 — REQUIREMENTS.md maps MORPH-02 to this
		// phase and roadmap SC-2 spells out "knob + CV + attenuverter". Phase
		// 31's CONTEXT lumped MORPH's and CHARACTER's CV together into Phase 34;
		// that lumping is CORRECTED here, not followed. CHARACTER's CV and
		// attenuverter remain Phase 34's (CHAR-01) and are deliberately NOT
		// added alongside these two, however symmetric the panel would look with
		// them: CHARACTER has no behavior in this phase that a cable would let
		// an operator audition, and D-07 above forbids declaring a control ahead
		// of the reason to move it.
		configParam(MORPH_ATTEN_PARAM, -1.f, 1.f, 0.f, "Morph CV Depth", "%", 0.f, 100.f);

		configInput(VOCT_INPUT, "V/Oct");
		configInput(FM_INPUT, "FM");
		configInput(MORPH_CV_INPUT, "Morph CV");
		configOutput(OUTPUT, "Audio");

		// T-30-02. BOTH calls are required and neither is optional. seed()
		// seeds only the drift RNG; without the spread seed every
		// component-spread coefficient stays at zero and the D-11 spread
		// mechanism does nothing at all.
		//
		// READ THIS BEFORE TRUSTING THE DIVERGENCE TESTS. The four literals
		// below are HARDCODED, so every AnalogVCO in a patch is constructed
		// from the same pair of seeds: two instances are bit-identical clones
		// of one another, measured at 0 of 2048 differing samples and
		// reproduced independently by this phase's code review and by its
		// verification. What the D-11 spread actually buys here is divergence
		// from an UNSPREAD default core — not divergence from the next VCO the
		// user adds. tests/test_vco_core.cpp's divergence invariants drive two
		// DIFFERENTLY-seeded cores, which this shell never constructs, so they
		// are evidence about forge::VcoCore and must NOT be read as describing
		// the shipped module. Shell-forwarded per-instance entropy plus patch
		// persistence is Phase 34/35's, tracked as item 2 in this phase's
		// deferred-items.md. The pattern does not need designing — the shipped
		// LFO module already draws its seed from std::random_device and
		// persists the drawn spread seed in the patch.
		//
		// The four literals are copied VERBATIM from tests/VcoBlockDriver.hpp,
		// which already documents them as proven non-degenerate. Do not invent
		// values, and never seed with a pair of zeros: forge::Xoroshiro128Plus
		// seeded (0,0) is a fixed point emitting an all-zero stream, which makes
		// the rejection loop inside std::normal_distribution never terminate.
		// In Rack that is a HANG ON PATCH LOAD, not a failing test — the user's
		// Rack stops responding while opening a patch. That prohibition covers
		// the clone behavior too: do NOT "fix" the cloning here by hand-picking
		// a different pair of literals — route it through the deferred item.
		// Phases 34/35 replace these literals with shell-forwarded entropy plus
		// patch persistence, and must re-validate any deserialized value the
		// same way.
		core.seed(0x1234ULL, 0x5678ULL);
		core.setSpreadSeed(0x9E3779B9ULL, 0x7F4A7C15ULL);
	}

	void process(const ProcessArgs& args) override {
		// Default-construct then assign, never a brace value list — see the
		// banner's C++11 note.
		forge::VcoInputs in;
		in.pitchCV = inputs[VOCT_INPUT].getVoltage();

		// THE ONE AUTHORISED DIVERGENCE FROM THE BANNER'S "THIS FILE DOES NO
		// DSP" RULE, and D-17 authorises it BY NAME rather than by silence. What
		// follows is knob + CV x attenuverter, conditioned into the range
		// forge::VcoInputs::morph has documented since Phase 29 — post-CV,
		// post-clamp [0,1]. That is PARAM PLUMBING: it makes the POD's own
		// declaration true at the seam it is declared for, and it computes
		// nothing the DSP would otherwise compute. FM was a genuinely different
		// case and the contrast is the point — FM had to enter the volt-domain
		// summation INSIDE the core, ahead of the single exponential, so Phase
		// 31's D-17 put it there and left this shell forwarding raw values and
		// calculating nothing at all. The 0.1f is Rack's plus-or-minus 10 V
		// bipolar CV convention mapped onto the POD's full plus-or-minus 1.0
		// range, the same factor the shipped LFO's shell uses.
		//
		// NO POD FIELD IS ADDED, AND THAT IS DELIBERATE TWICE OVER. morph was
		// designed post-CV/post-clamp in Phase 29 and has been stable ever
		// since; Phase 31's D-05 forbids churning field semantics mid-milestone,
		// so re-documenting it as raw and moving this mix into the core would
		// pay a documentation cost to buy nothing. The second reason is
		// structural: morphCV/morphAtten fields would land under
		// tests/check_canary.sh [2b/5]'s "every field stays runtime-live"
		// obligation while the canary's unique-field margin is EXACTLY ONE FIELD
		// (Phase 31 deferred item 9). Two new fields would overdraw a margin of
		// one.
		//
		// THE CONDITIONING IS WRITTEN NEGATED, AND forge::clamp IS REJECTED HERE
		// BY NAME. Rack does not sanitise cable voltages, so this jack is the
		// FIRST route by which a non-finite voltage can reach
		// forge::Waveshape::morphedWave — a FROZEN function that computes
		// morph * 4.f and casts the result to int. A float-to-int cast of a NaN
		// is undefined behavior, and the frozen header cannot be edited to
		// defend itself: that is a guardrail event, not a VCO fix. forge::clamp
		// is a comparison LADDER — x < lo ? lo : (x > hi ? hi : x) — and BOTH of
		// its comparisons are FALSE for a NaN, so a NaN passes straight through
		// it unchanged and the guard is inert against precisely the input class
		// it exists to stop. The negated form below puts the NaN on the fallback
		// branch: a NaN fails `morph > 0.f`, the negation is TRUE, and it becomes
		// 0.f, which then fails the plain upper comparison and survives. Same
		// guard SHAPE and same reasoning as the kVcoMaxPitchVolts bound
		// forge::VcoCore puts ahead of the frozen exponential.
		//
		// THE CORE GUARDS THIS FIELD TOO, AND NEITHER GUARD IS REDUNDANT. Plan
		// 32-06 hardens the same field inside forge::VcoCore::step, because the
		// core must not rely on its caller — the headless harness builds
		// forge::VcoInputs directly with no shell in the way, so a core that
		// trusted this line would be undefended in every test that exists. This
		// guard exists for the mirror-image reason: so the POD actually arriving
		// at the seam honours the range its own declaration promises.
		float morph = params[MORPH_PARAM].getValue();
		if (inputs[MORPH_CV_INPUT].isConnected())
			morph += inputs[MORPH_CV_INPUT].getVoltage()
			         * 0.1f * params[MORPH_ATTEN_PARAM].getValue();
		if (!(morph > 0.f)) morph = 0.f;
		if (morph > 1.f) morph = 1.f;
		in.morph = morph;

		in.character = params[CHARACTER_PARAM].getValue();
		in.sampleTime = args.sampleTime;
		in.sampleRate = args.sampleRate;

		// Phase 31's five fields, forwarded RAW and UNCONDITIONALLY. The tune
		// values go across in the units forge::VcoInputs documents them in —
		// octaves and semitones — so nothing here divides by twelve, scales,
		// sums, bounds or smooths. The semitone-to-octave conversion, the
		// volt-domain summation and the hostile-input bound on the exponent
		// argument all live in forge::VcoCore::step, which is where D-05 and
		// D-17 put them.
		//
		// BOTH FM fields cross unconditionally, and that is a DECISION
		// (D-09/D-17) rather than an oversight. The shipped module's shell
		// zeroes its own FM voltage behind a conditional before handing it to
		// its core; that placement is the ANTI-pattern here, for three reasons
		// that each stand alone. Rack already returns zero volts from an
		// unpatched input, so the conditional buys nothing. The core owns the
		// gate, and it does something strictly stronger than substituting zero:
		// it does not evaluate the term at all, whatever the two fields hold.
		// And a conditional in this function would be the first computation in a
		// file whose banner promises none — which is the property that lets the
		// headless suite stand as evidence about what a user hears.
		in.coarse = params[COARSE_PARAM].getValue();
		in.fine = params[FINE_PARAM].getValue();
		in.fmAtten = params[FM_ATTEN_PARAM].getValue();
		in.fmVolts = inputs[FM_INPUT].getVoltage();
		in.fmConnected = inputs[FM_INPUT].isConnected();
		outputs[OUTPUT].setVoltage(core.step(in));

		// Exactly ONE forge::VcoInputs field is still left at its header
		// default: drift, which belongs to Phase 34. The rule that produced the
		// longer list this sentence used to carry has not changed — each field
		// is wired by the phase that lands the DSP reading it — and Phase 31 is
		// that phase for the five directly above, so they moved out of this
		// sentence and into the block above.
		//
		// Consequence worth stating here, with numbers this phase moved: this
		// shell now feeds runtime-derived values into SEVEN of the eight
		// VcoInputs DSP fields, while src/vco_compile_canary.cpp feeds all
		// EIGHT. The single field the canary feeds and this shell does not is
		// drift. That is what keeps check_canary.sh [2b/5] reporting eight
		// fields runtime-live at -O3.
		//
		// The field-count margin is therefore down to ONE, so read what follows
		// as the actual load-bearing argument rather than as arithmetic — it
		// never was the arithmetic, and saying so now matters because Phase 34
		// closes the gap entirely. The canary is the translation unit that guard
		// COMPILES against a deliberately perturbed copy of the VCO header, and
		// it is the only VCO translation unit that is link-checkable WITHOUT the
		// Rack SDK. Neither property is one this file can acquire: this shell
		// cannot be compiled at all without the SDK, and it cannot be perturbed
		// by a guard whose whole discipline is to leave the real source alone.
		// So it could not substitute for the canary even if it fed every field
		// twice over, and retiring the canary once the counts converge would
		// delete the only local gate that sees the constant-fold class at all —
		// the class that got v2.0.0 rejected from the library.
		//
		// Growth rule, unchanged and still binding: the next phase that adds a
		// VcoInputs field must give the canary a runtime value for it, or
		// [2b/5] quietly stops covering that field while still reporting PASS.
		//
		// PHASE 32 RE-READ THAT RULE AND FOUND NOTHING TO DO. Recorded as a
		// finding rather than skipped in silence, so a later reader does not
		// have to re-derive it from the diff: D-17 adds NO forge::VcoInputs
		// field. The mix above feeds a field that already existed and was
		// already runtime-live. So every number in this block is unmoved — this
		// shell still feeds SEVEN of the eight DSP fields, the canary still
		// feeds all EIGHT, drift is still the single field the canary feeds and
		// this shell does not, the margin is still ONE, and
		// tests/check_canary.sh [2b/5] still reports eight fields runtime-live
		// at -O3. Phase 34 is still the phase that closes the gap.
	}
};

struct AnalogVCOWidget : ModuleWidget {
	AnalogVCOWidget(AnalogVCO* module) {
		setModule(module);
		// setPanel derives box.size from the SVG. Never hardcode it — that
		// desynchronises the widget from the art the moment Phase 35 swaps the
		// panel. No screws, no display, no context menu, no serialization hooks.
		setPanel(createPanel(asset::plugin(pluginInstance, "res/AnalogVCO.svg")));

		// These eight coordinates are the eight marker rects drawn in
		// res/AnalogVCO.svg, each rect sitting at its control centre minus five
		// in both axes. The two files are written together; move one and the
		// panel starts lying about where its controls are — which is exactly why
		// both edits land in a single commit rather than in two.
		//
		// Phase 31 added a middle row of three knobs and one jack between the two
		// already on the bottom row. The four Phase-30 coordinates below are
		// unmoved.
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.48f, 40.f)),
		         module, AnalogVCO::MORPH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.96f, 40.f)),
		         module, AnalogVCO::CHARACTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32f, 60.f)),
		         module, AnalogVCO::COARSE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(45.72f, 60.f)),
		         module, AnalogVCO::FINE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(71.12f, 60.f)),
		         module, AnalogVCO::FM_ATTEN_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48f, 100.f)),
		         module, AnalogVCO::VOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(45.72f, 100.f)),
		         module, AnalogVCO::FM_INPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(60.96f, 100.f)),
		          module, AnalogVCO::OUTPUT));
	}
};

// D-01, one-way door: the slug is written into every user patch that ever
// contains this module and can NEVER change. The SDK documents Model::slug as
// "Never change this after releasing your module"; the operator confirmed this
// exact spelling at plan 30-01's blocking checkpoint.
//
// Nothing registers this symbol with the plugin yet — src/plugin.hpp and
// src/plugin.cpp are plan 30-06's. After this file lands the symbol exists and
// the plugin links, and the module still does not appear in Rack's browser.
// That is the intended intermediate state; do not "fix" it here.
Model* modelAnalogVCO = createModel<AnalogVCO, AnalogVCOWidget>("ForgeAnalogVCO");
