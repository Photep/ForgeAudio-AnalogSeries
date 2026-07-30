// src/AnalogVCO.cpp
//
// The minimum-viable Rack shell for the Phase-30 VCO skeleton. It owns Rack
// indices; forge::VcoCore owns arithmetic; the POD between them is the same one
// the Phase-29 harness drives.
//
// THIS FILE DOES NO DSP, AND THAT IS LOAD-BEARING. No pitch maths, no output
// scaling, no conditioning, no clamping, no smoothing. Every sample Rack hears
// comes out of core.step(in). The headless suite in tests/ is only evidence
// about what Rack produces for exactly as long as that stays true — grow a
// calculation here and tests/test_vco_core.cpp silently stops describing the
// module a user plugged a cable into.
//
// Eight controls, no more (D-07, carried forward by Phase 31's D-16): V/OCT in,
// FM in, MORPH, CHARACTER, COARSE, FINE, FM DEPTH, OUT. They are the eight the
// DSP consumes, so every control that moves is a control you can hear and an
// in-Rack check is honest. The converse binds just as hard, and it is why all
// four Phase-31 controls are declared in the same phase as the arithmetic they
// feed: DSP that no control can reach cannot be auditioned, and this phase signs
// off in an operator-driven Rack session rather than on a headless count.
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
// The panel is throwaway on purpose. res/AnalogVCO.svg is six rectangles at the
// FINAL 18 HP geometry and the FINAL filename, so Phase 35 (PANEL-01/PANEL-02)
// is an art swap rather than a rewiring. The CRT display is Phase 35's
// DISP-01..03 and is deliberately absent here, as is any patch-state
// serialization: the VCO persists nothing in Phase 30.
//
// The oscillator this exposes is CRUDE AND ALIASED ON PURPOSE. Phase 32 owns
// band-limiting; nothing here should be judged on how it sounds.
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
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		FM_INPUT,
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

		configInput(VOCT_INPUT, "V/Oct");
		configInput(FM_INPUT, "FM");
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
		in.morph = params[MORPH_PARAM].getValue();
		in.character = params[CHARACTER_PARAM].getValue();
		in.sampleTime = args.sampleTime;
		in.sampleRate = args.sampleRate;
		outputs[OUTPUT].setVoltage(core.step(in));

		// The remaining forge::VcoInputs fields stay at their header defaults:
		// coarse, fine, fmVolts, fmAtten and fmConnected are Phase 31's, drift
		// is Phase 34's, and each is wired by the phase that lands the DSP
		// reading it.
		//
		// Consequence worth stating here, because this file's arrival is the
		// moment it looks wrong: this shell feeds runtime-derived values into
		// only THREE of the eight VcoInputs DSP fields, while
		// src/vco_compile_canary.cpp feeds all EIGHT. That is what keeps
		// check_canary.sh [2b/5] reporting eight fields runtime-live at -O3.
		// This file therefore does NOT make the canary redundant — swapping one
		// for the other would silently cut constant-fold coverage from eight
		// fields to three, in exactly the fields Phases 31, 33 and 34 are about
		// to make load-bearing.
	}
};

struct AnalogVCOWidget : ModuleWidget {
	AnalogVCOWidget(AnalogVCO* module) {
		setModule(module);
		// setPanel derives box.size from the SVG. Never hardcode it — that
		// desynchronises the widget from the art the moment Phase 35 swaps the
		// panel. No screws, no display, no context menu, no serialization hooks.
		setPanel(createPanel(asset::plugin(pluginInstance, "res/AnalogVCO.svg")));

		// These four coordinates are the four marker rects drawn in
		// res/AnalogVCO.svg. The two files are written together; move one and
		// the panel starts lying about where its controls are.
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(30.48f, 40.f)),
		         module, AnalogVCO::MORPH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(60.96f, 40.f)),
		         module, AnalogVCO::CHARACTER_PARAM));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.48f, 100.f)),
		         module, AnalogVCO::VOCT_INPUT));
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
