#include "plugin.hpp"
#include <cmath>
#include <atomic>
#include <array>

struct AnalogLFO : Module {
	enum ParamId {
		MORPH_PARAM,
		CHARACTER_PARAM,
		DRIFT_PARAM,
		RATE_PARAM,
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

	// Display double buffer (lock-free audio-to-GUI transfer)
	static constexpr int DISPLAY_SAMPLES = 256;
	std::array<float, DISPLAY_SAMPLES> displayBuffers[2] = {};
	std::atomic<int> displayReadIdx{0};
	std::atomic<float> displayPhase{0.f};

	// Display update tracking
	float prevDisplayMorph = -1.f;
	double prevPhaseForDisplay = 0.0;

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

	void updateDisplayBuffer(float morph) {
		int writeIdx = 1 - displayReadIdx.load(std::memory_order_relaxed);
		for (int i = 0; i < DISPLAY_SAMPLES; i++) {
			float p = (float)i / (float)DISPLAY_SAMPLES;
			displayBuffers[writeIdx][i] = computeMorphedWave(p, morph);
		}
		displayReadIdx.store(writeIdx, std::memory_order_release);
	}

	AnalogLFO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MORPH_PARAM, 0.f, 1.f, 0.f, "Morph");
		configParam(CHARACTER_PARAM, 0.f, 1.f, 0.f, "Character");
		configParam(DRIFT_PARAM, 0.f, 1.f, 0.f, "Drift");
		configParam(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz");
		configParam(MORPH_ATTEN_PARAM, 0.f, 1.f, 0.f, "Morph CV", "%", 0.f, 100.f);
		configInput(MORPH_CV_INPUT, "Morph CV");
		configInput(DRIFT_CV_INPUT, "Drift CV");
		configOutput(OUTPUT, "LFO");
		updateDisplayBuffer(0.f);
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

		// Update display phase
		displayPhase.store((float)phase, std::memory_order_relaxed);

		// Update display buffer on phase wrap or morph change
		// TODO Phase 5/6: add characterChanged and driftChanged triggers here
		bool phaseWrapped = (phase < prevPhaseForDisplay);
		bool morphChanged = (std::fabs(morph - prevDisplayMorph) > 0.002f);
		if (phaseWrapped || morphChanged) {
			updateDisplayBuffer(morph);
			prevDisplayMorph = morph;
		}
		prevPhaseForDisplay = phase;

		// Waveform generation
		float p = (float)phase;
		float sample = computeMorphedWave(p, morph);

		// Bipolar +/-5V output
		outputs[OUTPUT].setVoltage(5.f * sample);
	}
};

struct WaveformDisplay : rack::widget::TransparentWidget {
	AnalogLFO* module = nullptr;

	// Animation state
	float breathePhase = 0.f;
	float prevFramePhase = 0.f;

	void step() override {
		// Advance breathe animation (~0.8Hz cycle)
		breathePhase += 2.f * (float)M_PI * 0.8f / 60.f;
		if (breathePhase > 2.f * (float)M_PI) breathePhase -= 2.f * (float)M_PI;
		TransparentWidget::step();
	}

	// Coordinate helpers
	float phaseToX(float phase) const {
		float margin = 4.f;
		return margin + phase * (box.size.x - 2.f * margin);
	}

	float valueToY(float value) const {
		float margin = 6.f;
		return box.size.y / 2.f - value * (box.size.y / 2.f - margin);
	}

	float interpolateBuffer(const std::array<float, 256>& buffer, float phase) const {
		float idx = phase * 256.f;
		int i0 = (int)idx;
		int i1 = (i0 + 1);
		if (i0 >= 256) i0 = 255;
		if (i1 >= 256) i1 = 0;  // wrap for interpolation at boundary
		if (i0 < 0) i0 = 0;
		float frac = idx - (float)(int)idx;
		return buffer[i0] + frac * (buffer[i1] - buffer[i0]);
	}

	void drawBackground(NVGcontext* vg) {
		nvgBeginPath(vg);
		nvgRoundedRect(vg, 0, 0, box.size.x, box.size.y, 2.0f);
		nvgFillColor(vg, nvgRGBAf(0.051f, 0.051f, 0.102f, 1.f));
		nvgFill(vg);
	}

	void drawInsetFrame(NVGcontext* vg) {
		float w = box.size.x;
		float h = box.size.y;

		// Outer shadow: dark border suggesting depth
		nvgBeginPath(vg);
		nvgRoundedRect(vg, 0, 0, w, h, 2.0f);
		nvgStrokeColor(vg, nvgRGBAf(0.f, 0.f, 0.f, 0.3f));
		nvgStrokeWidth(vg, 1.0f);
		nvgStroke(vg);

		// Inner highlight: faint amber border
		nvgBeginPath(vg);
		nvgRoundedRect(vg, 0.5f, 0.5f, w - 1.f, h - 1.f, 1.5f);
		nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, 0.15f));
		nvgStrokeWidth(vg, 0.5f);
		nvgStroke(vg);
	}

	void drawWaveformTrace(NVGcontext* vg, const std::array<float, 256>& buffer, float dimFactor) {
		// Four-pass glow rendering
		const float widths[]  = {6.0f, 4.0f, 2.5f, 1.5f};
		const float alphas[] = {0.04f, 0.08f, 0.15f, 0.85f};

		for (int pass = 0; pass < 4; pass++) {
			nvgBeginPath(vg);
			for (int i = 0; i < 256; i++) {
				float p = (float)i / 256.f;
				float x = phaseToX(p);
				float y = valueToY(buffer[i]);
				if (i == 0)
					nvgMoveTo(vg, x, y);
				else
					nvgLineTo(vg, x, y);
			}
			nvgLineCap(vg, NVG_ROUND);
			nvgLineJoin(vg, NVG_ROUND);
			nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alphas[pass] * dimFactor));
			nvgStrokeWidth(vg, widths[pass]);
			nvgStroke(vg);
		}
	}

	void drawPhaseDot(NVGcontext* vg, const std::array<float, 256>& buffer, float phase, float dimFactor) {
		float dotRadius = box.size.y * 0.03f;

		// Movement detection and breathe
		float movement = std::fabs(phase - prevFramePhase);
		if (movement > 0.5f) movement = 1.f - movement;  // wrapped
		float breatheFactor = 1.f;
		if (movement < 0.001f) {
			breatheFactor = 0.8f + 0.2f * std::sin(breathePhase);
		}
		prevFramePhase = phase;

		// Comet trail (draw before dot so dot renders on top)
		for (int i = 3; i >= 0; i--) {
			float trailPhase = phase - (float)(i + 1) * 0.015f;
			if (trailPhase < 0.f) trailPhase += 1.f;
			float tx = phaseToX(trailPhase);
			float ty = valueToY(interpolateBuffer(buffer, trailPhase));
			float trailAlpha = 0.3f * (1.f - (float)(i + 1) / 5.f) * dimFactor * breatheFactor;
			float trailRadius = dotRadius * (0.5f + 0.5f * (1.f - (float)(i + 1) / 5.f));

			nvgBeginPath(vg);
			nvgCircle(vg, tx, ty, trailRadius);
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, trailAlpha));
			nvgFill(vg);
		}

		// Glow halo
		float x = phaseToX(phase);
		float y = valueToY(interpolateBuffer(buffer, phase));
		float haloRadius = dotRadius * 3.f;
		NVGpaint halo = nvgRadialGradient(vg, x, y, 0.f, haloRadius,
			nvgRGBAf(1.f, 0.9f, 0.5f, 0.3f * dimFactor * breatheFactor),
			nvgRGBAf(1.f, 0.9f, 0.5f, 0.f));
		nvgBeginPath(vg);
		nvgCircle(vg, x, y, haloRadius);
		nvgFillPaint(vg, halo);
		nvgFill(vg);

		// Bright center dot
		nvgBeginPath(vg);
		nvgCircle(vg, x, y, dotRadius);
		nvgFillColor(vg, nvgRGBAf(1.f, 0.91f, 0.63f, 1.f * dimFactor * breatheFactor));
		nvgFill(vg);
	}

	void drawPlaceholder(NVGcontext* vg) {
		// Static sine wave for module browser thumbnail
		nvgBeginPath(vg);
		for (int i = 0; i < 256; i++) {
			float p = (float)i / 256.f;
			float val = std::sin(2.f * (float)M_PI * p);
			float x = phaseToX(p);
			float y = valueToY(val);
			if (i == 0)
				nvgMoveTo(vg, x, y);
			else
				nvgLineTo(vg, x, y);
		}
		nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, 0.5f));
		nvgStrokeWidth(vg, 1.5f);
		nvgLineCap(vg, NVG_ROUND);
		nvgStroke(vg);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			NVGcontext* vg = args.vg;
			nvgSave(vg);
			nvgScissor(vg, 0, 0, box.size.x, box.size.y);

			drawBackground(vg);
			drawInsetFrame(vg);

			if (module) {
				int readIdx = module->displayReadIdx.load(std::memory_order_acquire);
				const auto& buffer = module->displayBuffers[readIdx];
				float phase = module->displayPhase.load(std::memory_order_relaxed);
				float rate = module->params[AnalogLFO::RATE_PARAM].getValue();
				bool isStill = (rate <= 0.001f);  // effectively zero rate
				float dimFactor = (module->isBypassed() || isStill) ? 0.25f : 1.f;

				drawWaveformTrace(vg, buffer, dimFactor);
				drawPhaseDot(vg, buffer, phase, dimFactor);
			} else {
				drawPlaceholder(vg);
			}

			nvgResetScissor(vg);
			nvgRestore(vg);
		}
		TransparentWidget::drawLayer(args, layer);
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

		// Waveform display -- positioned to match SVG placeholder rect
		// ~2mm side margins, starts right below title line, ~27% of panel height
		// per CONTEXT.md: "generous window 25-35%", "no gap below title", "~2mm each side"
		{
			WaveformDisplay* display = new WaveformDisplay();
			display->module = module;
			display->box.pos = mm2px(Vec(2.f, 15.f));
			display->box.size = mm2px(Vec(57.f, 27.f));
			addChild(display);
		}

		// Params
		addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(30.48, 54.0)), module, AnalogLFO::MORPH_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(18.0, 69.0)), module, AnalogLFO::CHARACTER_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(42.96, 69.0)), module, AnalogLFO::DRIFT_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(18.0, 86.0)), module, AnalogLFO::RATE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(9.0, 104.0)), module, AnalogLFO::MORPH_ATTEN_PARAM));

		// Inputs
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.0, 104.0)), module, AnalogLFO::MORPH_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(35.0, 104.0)), module, AnalogLFO::DRIFT_CV_INPUT));

		// Outputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(51.0, 104.0)), module, AnalogLFO::OUTPUT));
	}
};

Model* modelAnalogLFO = createModel<AnalogLFO, AnalogLFOWidget>("ForgeAnalogLFO");
