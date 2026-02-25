#include "plugin.hpp"

struct AnalogLFO : Module {
	enum ParamId {
		MORPH_PARAM,
		CHARACTER_PARAM,
		DRIFT_PARAM,
		RATE_PARAM,
		OCTAVE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MORPH_CV_INPUT,
		DRIFT_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT,
		INV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	AnalogLFO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MORPH_PARAM, 0.f, 1.f, 0.f, "Morph");
		configParam(CHARACTER_PARAM, 0.f, 1.f, 0.f, "Character");
		configParam(DRIFT_PARAM, 0.f, 1.f, 0.f, "Drift");
		configParam(RATE_PARAM, -8.f, 4.f, -3.f, "Rate", " Hz", 2.f, 1.f);
		configParam(OCTAVE_PARAM, -4.f, 4.f, 0.f, "Octave");
		configInput(MORPH_CV_INPUT, "Morph CV");
		configInput(DRIFT_CV_INPUT, "Drift CV");
		configOutput(OUTPUT, "LFO");
		configOutput(INV_OUTPUT, "Inverted LFO");
	}

	void process(const ProcessArgs& args) override {
		// DSP added in Phase 2
	}
};

struct AnalogLFOWidget : ModuleWidget {
	AnalogLFOWidget(AnalogLFO* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/AnalogLFO.svg")));

		// Screws
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Params
		addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(30.48, 52.0)), module, AnalogLFO::MORPH_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(18.0, 69.0)), module, AnalogLFO::CHARACTER_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(42.96, 69.0)), module, AnalogLFO::DRIFT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(18.0, 86.0)), module, AnalogLFO::RATE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(42.96, 86.0)), module, AnalogLFO::OCTAVE_PARAM));

		// Inputs
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(11.0, 104.0)), module, AnalogLFO::MORPH_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(24.5, 104.0)), module, AnalogLFO::DRIFT_CV_INPUT));

		// Outputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.0, 104.0)), module, AnalogLFO::OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(51.5, 104.0)), module, AnalogLFO::INV_OUTPUT));
	}
};

Model* modelAnalogLFO = createModel<AnalogLFO, AnalogLFOWidget>("ForgeAnalogLFO");
