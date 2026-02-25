#include "plugin.hpp"
#include <cmath>

struct AnalogLFO : Module {
	enum ParamId {
		MORPH_PARAM,
		CHARACTER_PARAM,
		DRIFT_PARAM,
		RATE_PARAM,
		OCTAVE_PARAM,
		MORPH_ATTEN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MORPH_CV_INPUT,
		DRIFT_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	double phase = 0.0;

	float computeMorphedWave(float phase, float morph) {
		float sine = std::sin(2.f * (float)M_PI * phase);
		float tri  = 2.f * std::fabs(2.f * phase - 1.f) - 1.f;
		float saw  = 1.f - 2.f * phase;
		float sqr  = (phase < 0.5f) ? 1.f : -1.f;

		float scaled = morph * 4.f;
		int segment = std::min((int)scaled, 3);
		float frac = scaled - (float)segment;

		switch (segment) {
			case 0: return sine + frac * (tri - sine);
			case 1: return tri  + frac * (saw - tri);
			case 2: return saw  + frac * (sqr - saw);
			case 3: return sqr;
		}
		return sine;
	}

	AnalogLFO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MORPH_PARAM, 0.f, 1.f, 0.f, "Morph");
		configParam(CHARACTER_PARAM, 0.f, 1.f, 0.f, "Character");
		configParam(DRIFT_PARAM, 0.f, 1.f, 0.f, "Drift");
		configParam(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz");
		configParam(OCTAVE_PARAM, -4.f, 4.f, 0.f, "Octave");
		configParam(MORPH_ATTEN_PARAM, 0.f, 1.f, 0.f, "Morph CV", "%", 0.f, 100.f);
		configInput(MORPH_CV_INPUT, "Morph CV");
		configInput(DRIFT_CV_INPUT, "Drift CV");
		configOutput(OUTPUT, "LFO");
	}

	void process(const ProcessArgs& args) override {
		// Rate (linear Hz, direct value)
		float freq = params[RATE_PARAM].getValue();
		freq = std::fmax(freq, 0.001f);  // safety floor

		// Phase accumulation (double precision to prevent stall at low frequencies)
		double deltaPhase = (double)freq * (double)args.sampleTime;
		phase += deltaPhase;
		if (phase >= 1.0) phase -= 1.0;

		// Morph with CV (additive offset, attenuator, hard clamp)
		float morphKnob = params[MORPH_PARAM].getValue();
		float morphAtten = params[MORPH_ATTEN_PARAM].getValue();
		float morphCV = inputs[MORPH_CV_INPUT].getVoltage();
		float morph = rack::math::clamp(morphKnob + morphAtten * morphCV / 10.f, 0.f, 1.f);

		// Waveform generation
		float p = (float)phase;
		float sample = computeMorphedWave(p, morph);

		// Bipolar +/-5V output
		outputs[OUTPUT].setVoltage(5.f * sample);
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
		addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(30.48, 54.0)), module, AnalogLFO::MORPH_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(18.0, 69.0)), module, AnalogLFO::CHARACTER_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(42.96, 69.0)), module, AnalogLFO::DRIFT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(18.0, 86.0)), module, AnalogLFO::RATE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(42.96, 86.0)), module, AnalogLFO::OCTAVE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(9.0, 104.0)), module, AnalogLFO::MORPH_ATTEN_PARAM));

		// Inputs
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.0, 104.0)), module, AnalogLFO::MORPH_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.0, 104.0)), module, AnalogLFO::DRIFT_CV_INPUT));

		// Outputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(51.0, 104.0)), module, AnalogLFO::OUTPUT));
	}
};

Model* modelAnalogLFO = createModel<AnalogLFO, AnalogLFOWidget>("ForgeAnalogLFO");
