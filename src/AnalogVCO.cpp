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
// Four controls, no more (D-07): V/OCT in, MORPH, CHARACTER, OUT. They are the
// four the Phase-30 DSP consumes, so every control that moves is a control you
// can hear and an in-Rack check is honest. CHARACTER is here as a consequence of
// D-11, not as a pull-forward of CHAR-01: every component-spread coefficient in
// forge::Waveshape is gated behind character >= 0.001f, so at character = 0 the
// per-instance divergence is invisible in Rack. Later phases add their controls
// alongside the behavior that reads them; nothing has shipped, so param and
// input ID churn is free right now and declaring the full Phase-35 enum early
// would buy stability nobody needs yet.
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
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
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
		configInput(VOCT_INPUT, "V/Oct");
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
