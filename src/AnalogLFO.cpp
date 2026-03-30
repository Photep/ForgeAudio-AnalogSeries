#include "plugin.hpp"
#include <cmath>
#include <atomic>
#include <array>
#include <random>

struct AnalogLFO : Module {
	enum ParamId {
		MORPH_PARAM,
		CHARACTER_PARAM,
		DRIFT_PARAM,
		RATE_PARAM,
		MORPH_ATTEN_PARAM,
		CHARACTER_ATTEN_PARAM,
		DRIFT_ATTEN_PARAM,
		PHASE_OFFSET_PARAM,
		PHASE_OFFSET_ATTEN_PARAM,
		FM_ATTEN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MORPH_CV_INPUT,
		DRIFT_CV_INPUT,
		CHARACTER_CV_INPUT,
		CLK_INPUT,
		RESET_INPUT,
		PHASE_OFFSET_CV_INPUT,
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

	// Clock tracking state machine
	enum ClockState { FREE = 0, ACQUIRING = 1, LOCKED = 2 };

	// 15 musical ratios as frequency multipliers relative to clock frequency
	static constexpr float RATIO_TABLE[15] = {
		1.f/16.f,   // /16  = 0.0625
		1.f/8.f,    // /8   = 0.125
		1.f/6.f,    // /6   = 0.166667
		1.f/4.f,    // /4   = 0.25
		1.f/3.f,    // /3   = 0.333333
		1.f/2.f,    // /2   = 0.5
		2.f/3.f,    // /1.5 = 0.666667
		1.f,        // x1   = 1.0
		3.f/2.f,    // x1.5 = 1.5
		2.f,        // x2   = 2.0
		3.f,        // x3   = 3.0
		4.f,        // x4   = 4.0
		6.f,        // x6   = 6.0
		8.f,        // x8   = 8.0
		16.f        // x16  = 16.0
	};

	static constexpr const char* RATIO_LABELS[15] = {
		"/16", "/8", "/6", "/4", "/3", "/2", "/1.5",
		"x1",
		"x1.5", "x2", "x3", "x4", "x6", "x8", "x16"
	};

	// Swing presets: index into SWING_FRACTIONS table (PHASE-03)
	static constexpr float SWING_FRACTIONS[6] = {
		0.50f,   // Straight
		0.54f,   // Light
		0.58f,   // Medium
		0.66f,   // Triplet (2/3)
		0.71f,   // Heavy
		0.75f    // Max (3/4)
	};

	static constexpr const char* SWING_MENU_LABELS[6] = {
		"Straight 50%", "Light 54%", "Medium 58%",
		"Triplet 66%", "Heavy 71%", "Max 75%"
	};

	static constexpr const char* SWING_OVERLAY_LABELS[6] = {
		"",             // hidden at Straight
		"LIGHT 54%",
		"MEDIUM 58%",
		"TRIPLET 66%",
		"HEAVY 71%",
		"MAX 75%"
	};

	double phase = 0.0;

	// Drift engine: multi-timescale Ornstein-Uhlenbeck process
	struct OULayer {
		float state = 0.f;
		float theta;    // mean reversion rate
		float sigma;    // noise amplitude
		float weight;   // contribution weight
	};

	rack::random::Xoroshiro128Plus rng;
	std::normal_distribution<float> normalDist{0.f, 1.f};
	float sqrtSampleTime = 0.f;
	static constexpr int NUM_OU_LAYERS = 4;
	OULayer ouLayers[NUM_OU_LAYERS];

	// Display double buffer (lock-free audio-to-GUI transfer)
	static constexpr int DISPLAY_SAMPLES = 256;
	std::array<float, DISPLAY_SAMPLES> displayBuffers[2] = {};
	std::atomic<int> displayReadIdx{0};
	std::atomic<float> displayPhase{0.f};
	std::atomic<float> displayDrift{0.f};   // Combined CV-modulated drift level for display

	// Display update tracking
	float prevDisplayMorph = -1.f;
	float prevDisplayCharacter = -1.f;
	double prevPhaseForDisplay = 0.0;
	float displayUpdateTimer = 0.f;

	// Clock tracking members
	dsp::SchmittTrigger clockTrigger;
	dsp::Timer clockTimer;
	float smoothedPeriod = 0.f;
	float lastSmoothedPeriod = 0.f;
	int clockEdgeCount = 0;
	ClockState clockState = FREE;
	bool prevClkConnected = false;
	std::atomic<int> displayClockState{0};
	std::atomic<int> displayRatioIndex{-1};  // -1 = free-running, 0-14 = ratio index
	std::atomic<float> displaySmoothedPeriod{0.f};

	// Phase 9: division-aware phase reset and anti-click crossfade
	int clockBeatCount = 0;          // counts clock edges within one LFO cycle
	int prevRatioIdx = -1;           // tracks ratio changes to reset beat counter
	float crossfadeFrom = 0.f;      // output value captured at moment of phase reset
	float crossfadeProgress = 1.f;  // 0.0 = just reset, 1.0 = crossfade complete
	float crossfadeDuration = 0.003f; // 3ms default
	float lastOutputVoltage = 0.f;  // previous frame's output for crossfade capture
	dsp::TExponentialFilter<float> freqSlew; // frequency smoother for mode transitions
	dsp::TExponentialFilter<float> driftSlew; // thermal pitch slew (Drift-gated, CHAR-03)

	// DC offset wander: dedicated slow OU layer (CHAR-02)
	OULayer dcOffsetOU;
	float dcOffsetV = 0.f;  // computed DC offset voltage, applied after crossfade

	// Component spread: per-instance parameter offsets (CHAR-04)
	uint64_t spreadSeed[2] = {};          // persisted seed for reproducible spread
	float ouWeightSpread[NUM_OU_LAYERS] = {};       // OU layer weight offsets
	float characterSpread = 0.f;          // character curve response offset
	float sawCurvatureSpread = 0.f;       // saw exponential ramp spread
	float squareDutySpread = 0.f;         // square duty cycle spread
	float triAsymmetrySpread = 0.f;       // triangle asymmetry spread
	float bleedSpread = 0.f;              // waveform bleed magnitude spread (CHAR-05)

	// Swing state (PHASE-03, PHASE-04)
	int swingIndex = 0;  // 0=Straight (default), persisted via dataToJson
	int prevSwingIndex = -1;  // tracks changes for display buffer refresh

	// Display bridge atomics for swing
	std::atomic<int> displaySwingIndex{0};
	std::atomic<float> displaySwingFraction{0.5f};

	// Phase 12: RESET trigger with bidirectional blanking
	dsp::SchmittTrigger resetTrigger;
	dsp::PulseGenerator resetBlanking;  // 1ms bidirectional blanking window

	struct RateParamQuantity : ParamQuantity {
		std::string getDisplayValueString() override {
			AnalogLFO* lfo = static_cast<AnalogLFO*>(module);
			if (!lfo) return ParamQuantity::getDisplayValueString();

			int ratioIdx = lfo->displayRatioIndex.load(std::memory_order_relaxed);
			if (ratioIdx < 0) {
				return ParamQuantity::getDisplayValueString();
			}

			return std::string(RATIO_LABELS[ratioIdx]);
		}

		std::string getUnit() override {
			AnalogLFO* lfo = static_cast<AnalogLFO*>(module);
			if (!lfo) return ParamQuantity::getUnit();

			int ratioIdx = lfo->displayRatioIndex.load(std::memory_order_relaxed);
			if (ratioIdx < 0) {
				return " Hz";
			}
			return " (synced)";
		}
	};

	float progressiveCurve(float character) {
		return character * character;  // x^2: subtle first half, aggressive second half
	}

	void initComponentSpread() {
		rack::random::Xoroshiro128Plus spreadRng;
		spreadRng.seed(spreadSeed[0], spreadSeed[1]);
		std::normal_distribution<float> d{0.f, 1.f};
		// OU layer weight offsets: +/-2% per layer
		for (int i = 0; i < NUM_OU_LAYERS; i++) {
			ouWeightSpread[i] = d(spreadRng) * 0.02f;
		}
		// Character response: +/-1.5%
		characterSpread = d(spreadRng) * 0.015f;
		// Waveform shape coefficients: +/-1-3%
		sawCurvatureSpread = d(spreadRng) * 0.02f;
		squareDutySpread = d(spreadRng) * 0.01f;
		triAsymmetrySpread = d(spreadRng) * 0.015f;
		// Bleed magnitude spread: +/-2% (CHAR-05)
		bleedSpread = d(spreadRng) * 0.02f;
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		sqrtSampleTime = std::sqrt(e.sampleTime);
	}

	float computeSine(float phase, float character) {
		float sine = std::sin(2.f * (float)M_PI * phase);
		if (character < 0.001f) return sine;
		float c = progressiveCurve(character);
		// Triangle-derived analog sine: residual THD via Chebyshev polynomials
		float h2 = 2.f * sine * sine - 1.f;                     // T2: 2nd harmonic
		float h3 = 4.f * sine * sine * sine - 3.f * sine;       // T3: 3rd harmonic
		float thd3 = c * 0.08f;   // 8% 3rd harmonic at full character
		float thd2 = c * 0.03f;   // 3% 2nd harmonic at full character
		return sine + thd3 * h3 + thd2 * h2;
	}

	float computeTriangle(float phase, float character) {
		// Digital: peak (+1) at phase=0 and 1, valley (-1) at phase=0.5
		float tri = 2.f * std::fabs(2.f * phase - 1.f) - 1.f;
		if (character < 0.001f) return tri;
		float c = progressiveCurve(character);
		// Slope asymmetry: valley shifts slightly right (falling slope longer)
		float asymmetry = c * (0.10f + triAsymmetrySpread);
		float valley = 0.5f + asymmetry * 0.5f;
		float analogTri;
		if (phase < valley) {
			// Falling from +1 (phase=0) to -1 (phase=valley)
			analogTri = 1.f - 2.f * phase / valley;
		} else {
			// Rising from -1 (phase=valley) to +1 (phase=1)
			analogTri = -1.f + 2.f * (phase - valley) / (1.f - valley);
		}
		// Rounded peaks via sinusoidal smoothing
		float roundAmount = c * 0.35f;
		if (analogTri > (1.f - roundAmount)) {
			float t = (analogTri - (1.f - roundAmount)) / roundAmount;
			analogTri = (1.f - roundAmount) + roundAmount * std::sin(t * (float)M_PI * 0.5f);
		} else if (analogTri < -(1.f - roundAmount)) {
			float t = (-(1.f - roundAmount) - analogTri) / roundAmount;
			analogTri = -(1.f - roundAmount) - roundAmount * std::sin(t * (float)M_PI * 0.5f);
		}
		return analogTri;
	}

	float computeSaw(float phase, float character) {
		float saw = 1.f - 2.f * phase;  // falling ramp (Minimoog convention)
		if (character < 0.001f) return saw;
		float c = progressiveCurve(character);
		// Exponential ramp curvature (50% blend toward exponential at full character)
		float expRamp = 1.f - 2.f * (1.f - std::exp(-3.f * phase)) / (1.f - std::exp(-3.f));
		float curvedSaw = saw + c * (0.5f + sawCurvatureSpread) * (expRamp - saw);
		// Soft capacitor reset (~8% of cycle at full character)
		float resetWidth = c * 0.08f;
		if (phase < resetWidth && resetWidth > 0.001f) {
			float t = phase / resetWidth;
			float smoothT = 0.5f - 0.5f * std::cos(t * (float)M_PI);
			float resetValue = 1.f;
			curvedSaw = resetValue + smoothT * (curvedSaw - resetValue);
		}
		return curvedSaw;
	}

	float computeSquare(float phase, float character) {
		// Digital: +1 for phase<0.5, -1 for phase>=0.5
		float sqr = (phase < 0.5f) ? 1.f : -1.f;
		if (character < 0.001f) return sqr;
		float c = progressiveCurve(character);
		// Duty cycle asymmetry (4% at full)
		float duty = 0.5f + c * (0.04f + squareDutySpread);
		// Sigmoid edge softening via tanh (~8% edge width at full)
		float edgeWidth = c * 0.08f;
		float sharpness = 1.f / std::fmax(edgeWidth, 0.001f);
		// Soft square: +1 region [0, duty], -1 region [duty, 1]
		// Use distance from center of +1 region with wrap-aware calculation
		float center = duty * 0.5f;
		float halfWidth = duty * 0.5f;
		float d = phase - center;
		if (d > 0.5f) d -= 1.f;
		if (d < -0.5f) d += 1.f;
		float dist = halfWidth - std::fabs(d);
		float analog = std::tanh(sharpness * dist);
		// Crossfade: prevents snap at low character values
		return sqr + c * (analog - sqr);
	}

	float computeMorphedWave(float phase, float morph, float character) {
		float sine = computeSine(phase, character);
		float tri  = computeTriangle(phase, character);
		float saw  = computeSaw(phase, character);
		float sqr  = computeSquare(phase, character);

		// Store shapes in array for indexed neighbor access
		float shapes[4] = { sine, tri, saw, sqr };

		float scaled = morph * 3.f;
		int segment = std::min((int)scaled, 2);
		float frac = scaled - (float)segment;

		// Primary crossfade (unchanged from v1.1)
		float result = shapes[segment] + frac * (shapes[segment + 1] - shapes[segment]);

		// Waveform bleed: adjacent-shape crosstalk (CHAR-05)
		if (character >= 0.001f) {
			float c = progressiveCurve(character);

			// Base bleed magnitude (4%) with component spread offset, clamped non-negative
			float effectiveBleed = std::fmax(0.f, 0.04f + bleedSpread);
			float bleedIntensity = c * effectiveBleed;

			// Slow modulation from existing OU layer 0 (~20s cycle, +/-20% fluctuation)
			bleedIntensity *= (1.f + ouLayers[0].state * 0.2f);
			bleedIntensity = std::fmax(0.f, bleedIntensity);  // ensure non-negative after modulation

			// Neighbor identification (wrapping ring: sine-tri-saw-sqr-sine)
			int leftIdx  = (segment - 1 + 4) % 4;   // shape left of segment start
			int rightIdx = (segment + 2) % 4;        // shape right of segment end

			// Proximity weighting: closer neighbor bleeds more
			float leftWeight  = 1.f - frac;   // frac=0 -> full left bleed
			float rightWeight = frac;          // frac=1 -> full right bleed

			float bleedSignal = leftWeight * shapes[leftIdx] + rightWeight * shapes[rightIdx];
			result += bleedIntensity * bleedSignal;

			// Normalize to maintain +/-1 range (prevents >+/-5V after 5V scaling)
			result /= (1.f + bleedIntensity);
		}

		return result;
	}

	void updateDisplayBuffer(float morph, float character, float swingFrac = 0.5f) {
		int writeIdx = 1 - displayReadIdx.load(std::memory_order_relaxed);
		for (int i = 0; i < DISPLAY_SAMPLES; i++) {
			float t = (float)i / (float)DISPLAY_SAMPLES;  // uniform time
			float p;
			if (swingFrac <= 0.5001f) {
				p = t;  // fast path: no swing
			} else if (t < swingFrac) {
				p = t * 0.5f / swingFrac;                              // even: [0,S) -> [0,0.5)
			} else {
				p = 0.5f + (t - swingFrac) * 0.5f / (1.f - swingFrac); // odd: [S,1) -> [0.5,1)
			}
			displayBuffers[writeIdx][i] = computeMorphedWave(p, morph, character);
		}
		displayReadIdx.store(writeIdx, std::memory_order_release);
	}

	void processClockInput(float sampleTime) {
		bool clkConnected = inputs[CLK_INPUT].isConnected();

		// Instant revert on cable disconnect
		if (!clkConnected) {
			if (prevClkConnected) {
				if (smoothedPeriod > 0.f) {
					lastSmoothedPeriod = smoothedPeriod;
				}
				clockState = FREE;
				clockEdgeCount = 0;
				clockBeatCount = 0;
				clockTimer.reset();
				smoothedPeriod = 0.f;
				displayClockState.store(FREE, std::memory_order_relaxed);
			}
			prevClkConnected = false;
			return;
		}
		prevClkConnected = true;

		// Accumulate time
		clockTimer.process(sampleTime);

		// Timeout check (only when connected but no pulses arriving)
		if (clockState != FREE && smoothedPeriod > 0.f) {
			float timeout = std::fmax(1.0f, std::fmin(3.0f * smoothedPeriod, 5.0f));
			if (clockTimer.getTime() > timeout) {
				lastSmoothedPeriod = smoothedPeriod;
				clockState = FREE;
				clockEdgeCount = 0;
				clockBeatCount = 0;
				smoothedPeriod = 0.f;
				clockTimer.reset();
				displayClockState.store(FREE, std::memory_order_relaxed);
			}
		}

		// Edge detection
		float clkVoltage = inputs[CLK_INPUT].getVoltage();
		if (clockTrigger.process(clkVoltage, 0.1f, 1.0f)) {
			float rawPeriod = clockTimer.getTime();
			clockTimer.reset();
			clockEdgeCount++;

			if (clockEdgeCount == 1) {
				// First edge: always reset phase and enter ACQUIRING
				clockState = ACQUIRING;
				clockBeatCount = 0;
				crossfadeFrom = lastOutputVoltage;
				crossfadeProgress = 0.f;
				phase = 0.0;
				displayClockState.store(ACQUIRING, std::memory_order_relaxed);
				resetBlanking.trigger(0.001f);  // Bidirectional blanking
			}
			else if (rawPeriod > 0.001f) {
				// Second edge onward: we have a period measurement

				// Outlier rejection (LOCKED state only, per CLK-05)
				if (clockState == LOCKED && smoothedPeriod > 0.f) {
					bool isOutlier = (rawPeriod > 3.0f * smoothedPeriod) ||
					                 (rawPeriod < smoothedPeriod / 3.0f);
					if (isOutlier) {
						return;  // Silently discard
					}
				}

				// Fast-track re-acquisition check
				// clockEdgeCount == 2 in ACQUIRING with a remembered period means
				// this is the second edge of a fresh acquisition sequence
				if (clockState == ACQUIRING && clockEdgeCount == 2 && lastSmoothedPeriod > 0.f) {
					float ratio = rawPeriod / lastSmoothedPeriod;
					if (ratio > 0.8f && ratio < 1.2f) {
						// Fast-track to LOCKED: same clock source reconnected
						smoothedPeriod = lastSmoothedPeriod;
						smoothedPeriod += 0.3f * (rawPeriod - smoothedPeriod);
						clockState = LOCKED;
						displayClockState.store(LOCKED, std::memory_order_relaxed);
					}
				}

				// EMA smoothing (per CLK-03)
				if (smoothedPeriod <= 0.f) {
					smoothedPeriod = rawPeriod;  // First measurement: snap (avoids Pitfall 4)
				} else {
					smoothedPeriod += 0.3f * (rawPeriod - smoothedPeriod);
				}

				// State transition: ACQUIRING -> LOCKED after 4 consistent edges
				if (clockState == ACQUIRING && clockEdgeCount >= 4) {
					clockState = LOCKED;
					displayClockState.store(LOCKED, std::memory_order_relaxed);
				} else if (clockState == ACQUIRING) {
					displayClockState.store(ACQUIRING, std::memory_order_relaxed);
				}

				// Division-aware phase reset (RATE-04)
				clockBeatCount++;

				// Determine current ratio for division check
				int currentRatioIdx = -1;
				if ((clockState == ACQUIRING || clockState == LOCKED) && smoothedPeriod > 0.f) {
					float knobNormalized = paramQuantities[RATE_PARAM]->getScaledValue();
					currentRatioIdx = (int)std::round(knobNormalized * 14.f);
					currentRatioIdx = rack::math::clamp(currentRatioIdx, 0, 14);
				}

				// Reset beat counter if ratio changed
				if (currentRatioIdx != prevRatioIdx && prevRatioIdx >= 0) {
					clockBeatCount = 1;  // This edge counts as beat 1 of new ratio
				}
				prevRatioIdx = currentRatioIdx;

				bool shouldReset = true;
				if (currentRatioIdx >= 0 && RATIO_TABLE[currentRatioIdx] < 1.f) {
					int divisor = (int)std::round(1.f / RATIO_TABLE[currentRatioIdx]);
					shouldReset = (clockBeatCount >= divisor);
				}

				if (shouldReset) {
					// Capture pre-reset output for crossfade (RATE-05)
					crossfadeFrom = lastOutputVoltage;
					crossfadeProgress = 0.f;
					phase = 0.0;
					clockBeatCount = 0;
					resetBlanking.trigger(0.001f);  // Bidirectional blanking
				}
			}
		}
	}

	void processResetInput(float sampleTime) {
		// Always advance blanking timer (Pitfall 5: must run every sample)
		bool blanking = resetBlanking.process(sampleTime);

		if (!inputs[RESET_INPUT].isConnected()) return;

		float resetVoltage = inputs[RESET_INPUT].getVoltage();
		if (resetTrigger.process(resetVoltage, 0.1f, 1.0f)) {
			if (!blanking) {
				// Trigger crossfade and reset phase (reuses existing crossfade -- MOD-04)
				crossfadeFrom = lastOutputVoltage;
				crossfadeProgress = 0.f;
				phase = 0.0;
				// Start blanking window to suppress subsequent CLK reset
				resetBlanking.trigger(0.001f);
				// NOTE: Do NOT reset clockTimer, clockBeatCount, smoothedPeriod,
				// clockEdgeCount, or clockState. RESET is independent of clock.
			}
		}
	}

	AnalogLFO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MORPH_PARAM, 0.f, 1.f, 0.f, "Morph");
		configParam(CHARACTER_PARAM, 0.f, 1.f, 0.f, "Character");
		configParam(DRIFT_PARAM, 0.f, 1.f, 0.f, "Drift");
		configParam<RateParamQuantity>(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz");
		configParam(MORPH_ATTEN_PARAM, 0.f, 1.f, 1.f, "Morph CV", "%", 0.f, 100.f);
		configParam(CHARACTER_ATTEN_PARAM, 0.f, 1.f, 1.f, "Character CV", "%", 0.f, 100.f);
		configParam(DRIFT_ATTEN_PARAM, 0.f, 1.f, 1.f, "Drift CV", "%", 0.f, 100.f);
		configInput(MORPH_CV_INPUT, "Morph CV");
		configInput(DRIFT_CV_INPUT, "Drift CV");
		configInput(CHARACTER_CV_INPUT, "Character CV");
		configInput(CLK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configParam(PHASE_OFFSET_PARAM, 0.f, 1.f, 0.f, "Phase Offset", " deg", 0.f, 360.f);
		configParam(PHASE_OFFSET_ATTEN_PARAM, 0.f, 1.f, 1.f, "Phase Offset CV", "%", 0.f, 100.f);
		configInput(PHASE_OFFSET_CV_INPUT, "Phase Offset CV");
		configParam(FM_ATTEN_PARAM, 0.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f);
		configInput(FM_INPUT, "FM");
		configOutput(OUTPUT, "LFO");
		updateDisplayBuffer(0.f, 0.f);

		// Phase 9: frequency slew for smooth mode transitions (DISP-05)
		freqSlew.setLambda(20.f);  // 50ms time constant (lambda = 1/tau = 1/0.05 = 20)
		freqSlew.out = 0.7f;       // matches default Rate param value

		// Phase 14: thermal pitch slew for Drift-dependent frequency lag (CHAR-03)
		driftSlew.setLambda(500.f);  // effectively instant at init
		driftSlew.out = 0.7f;        // match default Rate param value

		// Per-module unique RNG seed
		std::random_device rd;
		rng.seed(rd(), rd());

		// Component spread: generate unique seed from per-module RNG (CHAR-04)
		spreadSeed[0] = rng();
		spreadSeed[1] = rng();
		initComponentSpread();

		// Initialize OU layers (multi-timescale drift)
		// Layer 0: 0.05Hz slow wander
		ouLayers[0].theta = 2.f * (float)M_PI * 0.05f;   // 0.314
		ouLayers[0].sigma = 0.793f;
		ouLayers[0].weight = 0.50f;
		// Layer 1: 0.2Hz medium drift
		ouLayers[1].theta = 2.f * (float)M_PI * 0.2f;    // 1.257
		ouLayers[1].sigma = 1.586f;
		ouLayers[1].weight = 0.25f;
		// Layer 2: 0.8Hz fast drift
		ouLayers[2].theta = 2.f * (float)M_PI * 0.8f;    // 5.027
		ouLayers[2].sigma = 3.170f;
		ouLayers[2].weight = 0.15f;
		// Layer 3: ~2Hz jitter
		ouLayers[3].theta = 2.f * (float)M_PI * 2.0f;    // 12.566
		ouLayers[3].sigma = 5.013f;
		ouLayers[3].weight = 0.10f;

		// DC offset wander OU layer (CHAR-02)
		// 0.03Hz center frequency (~33s wander cycle)
		dcOffsetOU.theta = 2.f * (float)M_PI * 0.03f;   // 0.188
		dcOffsetOU.sigma = 0.614f;                         // stationary std ~1.0
		dcOffsetOU.weight = 1.f;                           // single layer, no weighting
		dcOffsetOU.state = 0.f;
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		// Store spread seed as hex strings to avoid uint64_t -> int64_t sign issues (Pitfall 6)
		char buf[32];
		snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)spreadSeed[0]);
		json_object_set_new(rootJ, "spreadSeed0", json_string(buf));
		snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)spreadSeed[1]);
		json_object_set_new(rootJ, "spreadSeed1", json_string(buf));
		// Swing preset (PHASE-03)
		json_object_set_new(rootJ, "swingIndex", json_integer(swingIndex));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* s0J = json_object_get(rootJ, "spreadSeed0");
		json_t* s1J = json_object_get(rootJ, "spreadSeed1");
		if (s0J && s1J) {
			spreadSeed[0] = std::stoull(json_string_value(s0J), nullptr, 16);
			spreadSeed[1] = std::stoull(json_string_value(s1J), nullptr, 16);
			initComponentSpread();  // regenerate deterministic offsets from restored seed
		}
		// Swing preset (PHASE-03)
		json_t* swingJ = json_object_get(rootJ, "swingIndex");
		if (swingJ)
			swingIndex = rack::math::clamp((int)json_integer_value(swingJ), 0, 5);
	}

	void process(const ProcessArgs& args) override {
		// Clock detection (Phase 7)
		processClockInput(args.sampleTime);
		processResetInput(args.sampleTime);

		// Rate: dual-mode frequency calculation
		float targetFreq;
		int ratioIdx = -1;
		bool isClocked = (clockState == ACQUIRING || clockState == LOCKED) && smoothedPeriod > 0.f;

		if (isClocked) {
			// Clocked mode: derive frequency from clock period and ratio
			float knobNormalized = paramQuantities[RATE_PARAM]->getScaledValue();
			ratioIdx = (int)std::round(knobNormalized * 14.f);
			ratioIdx = rack::math::clamp(ratioIdx, 0, 14);
			float clockFreq = 1.f / smoothedPeriod;
			targetFreq = clockFreq * RATIO_TABLE[ratioIdx];
		} else {
			// Free-running mode: direct Hz from knob (identical to v1.0)
			targetFreq = params[RATE_PARAM].getValue();
		}
		targetFreq = std::fmax(targetFreq, 0.001f);

		// Frequency slew for smooth mode transitions (DISP-05)
		float freq = freqSlew.process(args.sampleTime, targetFreq);
		freq = std::fmax(freq, 0.001f);

		// Drift parameter reads (moved early for pitch slew gate)
		float driftKnob = params[DRIFT_PARAM].getValue();
		float driftAtten = params[DRIFT_ATTEN_PARAM].getValue();
		float driftCV = inputs[DRIFT_CV_INPUT].getVoltage();
		float drift = rack::math::clamp(driftKnob + driftAtten * driftCV / 5.f, 0.f, 1.f);
		displayDrift.store(drift, std::memory_order_relaxed);

		// Pitch slew: thermal frequency lag (CHAR-03)
		// Separate from freqSlew (mode transitions) -- this adds Drift-dependent lag
		if (drift >= 0.001f) {
			float driftAmount = progressiveCurve(drift);
			// Time constant: 2ms at low drift -> 300ms at full drift
			// lambda = 1/tau, high lambda = instant, low lambda = sluggish
			float slewTau = 0.002f + driftAmount * 0.298f;
			driftSlew.setLambda(1.f / slewTau);
		} else {
			driftSlew.setLambda(500.f);  // bypass: effectively instant
		}
		freq = driftSlew.process(args.sampleTime, freq);
		freq = std::fmax(freq, 0.001f);

		// FM processing AFTER slew — slew smooths base frequency for mode transitions,
		// FM modulates on top so it isn't filtered out (MOD-01, MOD-02)
		if (inputs[FM_INPUT].isConnected()) {
			float fmCV = inputs[FM_INPUT].getVoltage();
			float fmAtten = params[FM_ATTEN_PARAM].getValue();
			// Authority: reduced in clocked mode to prevent clock-phase fighting (MOD-02)
			float depthScale = isClocked ? 0.5f : 0.6f;
			float fmPitch = fmCV * fmAtten * depthScale;
			freq *= dsp::exp2_taylor5(fmPitch);
			freq = std::fmax(freq, 0.001f);
		}

		// Update display atomics for tooltip/display use
		displayRatioIndex.store(ratioIdx, std::memory_order_relaxed);
		displaySmoothedPeriod.store(smoothedPeriod, std::memory_order_relaxed);

		// Phase accumulation (double precision to prevent stall at low frequencies)
		double deltaPhase = (double)freq * (double)args.sampleTime;

		// Drift processing (modifies deltaPhase BEFORE phase accumulation)
		if (drift >= 0.001f) {
			// Lazy init sqrtSampleTime (edge case before onSampleRateChange fires)
			if (sqrtSampleTime == 0.f) sqrtSampleTime = std::sqrt(args.sampleTime);

			float driftAmount = progressiveCurve(drift);
			float combinedOU = 0.f;
			for (int i = 0; i < NUM_OU_LAYERS; i++) {
				float noise = normalDist(rng);
				ouLayers[i].state += ouLayers[i].theta * (0.f - ouLayers[i].state) * args.sampleTime
				                   + ouLayers[i].sigma * sqrtSampleTime * noise;
				combinedOU += ouLayers[i].state * (ouLayers[i].weight + ouWeightSpread[i]);
			}
			// Reduced drift authority in clocked mode (DISP-04)
			float maxDrift = isClocked ? 0.02f : 0.075f;
			float driftScale = driftAmount * maxDrift;
			deltaPhase *= (1.0 + (double)(driftScale * combinedOU));

			// Phase jitter: per-sample random phase deviation (CHAR-01)
			// Independent white noise (not correlated OU layers)
			float jitterNoise = normalDist(rng);
			float jitterAuthority = isClocked ? 0.02f : 0.075f;  // same scaling as pitch drift
			float jitterScale = driftAmount * jitterAuthority * 0.003f;  // ~0.3% max deviation
			deltaPhase *= (1.0 + (double)(jitterScale * jitterNoise));

			// DC offset wander: independent slow OU process (CHAR-02)
			// Continuous wander -- does NOT reset on clock phase resets
			float dcNoise = normalDist(rng);
			dcOffsetOU.state += dcOffsetOU.theta * (0.f - dcOffsetOU.state) * args.sampleTime
			                  + dcOffsetOU.sigma * sqrtSampleTime * dcNoise;
			float dcOffsetAuthority = isClocked ? 0.02f : 0.075f;
			dcOffsetV = driftAmount * dcOffsetAuthority * dcOffsetOU.state * 0.1f;
			// Results in ~50-100mV max wander at full drift in free-running mode
		} else {
			dcOffsetV = 0.f;
		}

		// Swing: warp phase timing for clocked mode (PHASE-03)
		// Even beat (first half, phase < 0.5): slower accumulation -> longer duration
		// Odd beat (second half, phase >= 0.5): faster accumulation -> shorter duration
		// PHASE-04: swing inactive in free-running mode (isClocked gate)
		float swingFrac = SWING_FRACTIONS[swingIndex];
		if (isClocked && swingFrac > 0.5001f) {
			double swingMul = (phase < 0.5)
				? (0.5 / (double)swingFrac)
				: (0.5 / (1.0 - (double)swingFrac));
			deltaPhase *= swingMul;
		}

		// Update swing display atomics
		displaySwingIndex.store(swingIndex, std::memory_order_relaxed);
		displaySwingFraction.store(swingFrac, std::memory_order_relaxed);

		phase += deltaPhase;
		if (phase >= 1.0) phase -= 1.0;

		// Morph with CV (additive offset, attenuator, hard clamp)
		float morphKnob = params[MORPH_PARAM].getValue();
		float morphAtten = params[MORPH_ATTEN_PARAM].getValue();
		float morphCV = inputs[MORPH_CV_INPUT].getVoltage();
		float morph = rack::math::clamp(morphKnob + morphAtten * morphCV / 5.f, 0.f, 1.f);

		// Character with CV (additive offset, attenuator, hard clamp)
		float charKnob = params[CHARACTER_PARAM].getValue();
		float charAtten = params[CHARACTER_ATTEN_PARAM].getValue();
		float charCV = inputs[CHARACTER_CV_INPUT].getVoltage();
		float character = rack::math::clamp(charKnob + charAtten * charCV / 5.f + characterSpread, 0.f, 1.f);

		// Update display buffer on phase wrap, morph change, character change, or swing change
		bool phaseWrapped = (phase < prevPhaseForDisplay);
		bool morphChanged = (std::fabs(morph - prevDisplayMorph) > 0.002f);
		bool characterChanged = (std::fabs(character - prevDisplayCharacter) > 0.002f);
		bool swingChanged = (swingIndex != prevSwingIndex);
		displayUpdateTimer += args.sampleTime;
		// Phase wrap always triggers; param changes rate-limited to ~30fps
		// to prevent visual artifacts from fast CV modulation
		bool paramReady = displayUpdateTimer >= (1.f / 30.f);
		if (phaseWrapped || ((morphChanged || characterChanged || swingChanged) && paramReady)) {
			float displaySwing = isClocked ? swingFrac : 0.5f;  // no warp in free-running display
			updateDisplayBuffer(morph, character, displaySwing);
			prevDisplayMorph = morph;
			prevDisplayCharacter = character;
			prevSwingIndex = swingIndex;
			displayUpdateTimer = 0.f;
		}
		prevPhaseForDisplay = phase;

		// Phase offset computation (PHASE-01, PHASE-02)
		float offsetKnob = params[PHASE_OFFSET_PARAM].getValue();
		float offsetCV = 0.f;
		if (inputs[PHASE_OFFSET_CV_INPUT].isConnected()) {
			float offsetAtten = params[PHASE_OFFSET_ATTEN_PARAM].getValue();
			offsetCV = offsetAtten * inputs[PHASE_OFFSET_CV_INPUT].getVoltage() / 5.f;
		}
		float phaseOffset = rack::math::clamp(offsetKnob + offsetCV, 0.f, 1.f);

		// Apply offset at readout (not accumulator -- per PHASE-01)
		float p = (float)phase + phaseOffset;
		if (p >= 1.f) p -= 1.f;
		float sample = computeMorphedWave(p, morph, character);

		// Display phase includes offset (dot position matches audio output)
		float displayP = (float)phase + phaseOffset;
		if (displayP >= 1.f) displayP -= 1.f;
		displayPhase.store(displayP, std::memory_order_relaxed);
		float outputVoltage = 5.f * sample;

		// Anti-click crossfade on phase reset (RATE-05)
		if (crossfadeProgress < 1.f) {
			crossfadeProgress += args.sampleTime / crossfadeDuration;
			if (crossfadeProgress >= 1.f) {
				crossfadeProgress = 1.f;
			} else {
				float mix = 0.5f - 0.5f * std::cos((float)M_PI * crossfadeProgress);
				outputVoltage = crossfadeFrom + mix * (outputVoltage - crossfadeFrom);
			}
		}

		// Store for next frame's crossfade capture
		lastOutputVoltage = outputVoltage;

		// Apply DC offset wander AFTER crossfade (CHAR-02, avoids Pitfall 3)
		outputVoltage += dcOffsetV;

		// Bipolar +/-5V output
		outputs[OUTPUT].setVoltage(outputVoltage);
	}
};

struct WaveformDisplay : rack::widget::TransparentWidget {
	AnalogLFO* module = nullptr;

	// Animation state
	float breathePhase = 0.f;
	float prevFramePhase = 0.f;

	// Fade animation state for text overlays (200ms transitions)
	float syncFadeAlpha = 0.f;
	float ratioFadeAlpha = 0.f;
	float bpmFadeAlpha = 0.f;
	float hzFadeAlpha = 1.f;  // starts visible (free-running mode default)
	float swingFadeAlpha = 0.f;  // swing overlay (bottom-left)

	void step() override {
		// Advance breathe animation (~0.8Hz cycle)
		breathePhase += 2.f * (float)M_PI * 0.8f / 60.f;
		if (breathePhase > 2.f * (float)M_PI) breathePhase -= 2.f * (float)M_PI;

		// Advance fade timers for text overlays (200ms transitions)
		if (module) {
			int clockState = module->displayClockState.load(std::memory_order_relaxed);
			float fadeSpeed = 1.f / (0.2f * 60.f);  // 200ms at ~60fps

			bool showClocked = (clockState != 0);  // ACQUIRING or LOCKED
			float syncTarget = showClocked ? 1.f : 0.f;
			float ratioTarget = showClocked ? 1.f : 0.f;
			float bpmTarget = showClocked ? 1.f : 0.f;
			float hzTarget = showClocked ? 0.f : 1.f;

			syncFadeAlpha += rack::math::clamp(syncTarget - syncFadeAlpha, -fadeSpeed, fadeSpeed);
			ratioFadeAlpha += rack::math::clamp(ratioTarget - ratioFadeAlpha, -fadeSpeed, fadeSpeed);
			bpmFadeAlpha += rack::math::clamp(bpmTarget - bpmFadeAlpha, -fadeSpeed, fadeSpeed);
			hzFadeAlpha += rack::math::clamp(hzTarget - hzFadeAlpha, -fadeSpeed, fadeSpeed);

			// Swing overlay: visible when clocked AND swing > Straight
			int swingIdx = module->displaySwingIndex.load(std::memory_order_relaxed);
			bool showSwing = showClocked && (swingIdx > 0);
			float swingTarget = showSwing ? 1.f : 0.f;
			swingFadeAlpha += rack::math::clamp(swingTarget - swingFadeAlpha, -fadeSpeed, fadeSpeed);
		}

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
		// Swing phase-to-time remapping: dot position reflects swing-warped timing
		float swingFrac = module ? module->displaySwingFraction.load(std::memory_order_relaxed) : 0.5f;
		if (swingFrac > 0.5001f) {
			if (phase < 0.5f)
				phase = phase * 2.f * swingFrac;                           // [0,0.5) -> [0,S)
			else
				phase = swingFrac + (phase - 0.5f) * 2.f * (1.f - swingFrac);  // [0.5,1) -> [S,1)
		}

		float dotRadius = box.size.y * 0.03f;

		// Read drift level for visual instability, scaled down at low rates
		float driftLevel = module ? module->displayDrift.load(std::memory_order_relaxed) : 0.f;
		if (driftLevel > 0.f && module) {
			float rate = module->params[AnalogLFO::RATE_PARAM].getValue();
			float rateScale = std::min(1.f, 0.3f + rate * 0.7f);  // 30% floor, full at 1Hz+
			driftLevel *= rateScale;
		}

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

			// Drift instability: trail jitter (scaled to display size)
			if (driftLevel > 0.01f) {
				float jitterAmount = driftLevel * box.size.y * 0.05f;
				float trailJitter = jitterAmount * std::sin(breathePhase * 3.7f + (float)i * 1.3f);
				ty += trailJitter;
				tx += jitterAmount * 0.3f * std::sin(breathePhase * 4.1f + (float)i * 2.1f);
			}

			float trailAlpha = 0.3f * (1.f - (float)(i + 1) / 5.f) * dimFactor * breatheFactor;
			float trailRadius = dotRadius * (0.5f + 0.5f * (1.f - (float)(i + 1) / 5.f));

			nvgBeginPath(vg);
			nvgCircle(vg, tx, ty, trailRadius);
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, trailAlpha));
			nvgFill(vg);
		}

		// Glow halo with drift instability
		float x = phaseToX(phase);
		float y = valueToY(interpolateBuffer(buffer, phase));

		// Drift jitter on main dot position
		if (driftLevel > 0.01f) {
			float dotJitter = driftLevel * box.size.y * 0.03f;
			y += dotJitter * std::sin(breathePhase * 5.1f);
			x += dotJitter * 0.5f * std::sin(breathePhase * 3.3f);
		}

		float haloJitter = 1.f + driftLevel * 0.75f * std::sin(breathePhase * 2.3f);
		float haloRadius = dotRadius * 3.f * haloJitter;
		float haloAlpha = (0.3f + driftLevel * 0.25f) * dimFactor * breatheFactor;
		NVGpaint halo = nvgRadialGradient(vg, x, y, 0.f, haloRadius,
			nvgRGBAf(1.f, 0.9f, 0.5f, haloAlpha),
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

	void drawGlowText(NVGcontext* vg, int fontHandle, float x, float y,
	                  const char* text, float fontSize, int align, float alpha) {
		nvgFontFaceId(vg, fontHandle);
		nvgFontSize(vg, fontSize);
		nvgTextAlign(vg, align);

		// Pass 1: Glow (blurred, lower alpha)
		nvgFontBlur(vg, 3.0f);
		nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha * 0.4f));
		nvgText(vg, x, y, text, NULL);

		// Pass 2: Sharp text on top
		nvgFontBlur(vg, 0.0f);
		nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha));
		nvgText(vg, x, y, text, NULL);
	}

	void drawPillText(NVGcontext* vg, int fontHandle, float x, float y,
	                  const char* text, float fontSize, int align, float alpha) {
		// Set font state for measurement (must match drawing state)
		nvgFontFaceId(vg, fontHandle);
		nvgFontSize(vg, fontSize);
		nvgTextAlign(vg, align);

		// Measure text bounds
		float bounds[4]; // [xmin, ymin, xmax, ymax]
		nvgTextBounds(vg, x, y, text, NULL, bounds);

		// Pill dimensions with padding
		float pad = 4.f;
		float feather = 3.f;
		float cornerRadius = 3.f;
		float px = bounds[0] - pad;
		float py = bounds[1] - pad;
		float pw = (bounds[2] - bounds[0]) + 2.f * pad;
		float ph = (bounds[3] - bounds[1]) + 2.f * pad;

		// Draw feathered pill background using box gradient
		// Inner color: dark navy at 80% opacity scaled by alpha
		// Outer color: fade to transparent for soft edges
		NVGpaint pillPaint = nvgBoxGradient(vg,
			px, py, pw, ph,
			cornerRadius, feather,
			nvgRGBAf(0.102f, 0.102f, 0.180f, 0.80f * alpha),
			nvgRGBAf(0.102f, 0.102f, 0.180f, 0.0f));
		nvgBeginPath(vg);
		// Path must be larger than gradient bounds by feather distance
		// to prevent clipping of feathered edges
		nvgRoundedRect(vg, px - feather, py - feather,
		               pw + 2.f * feather, ph + 2.f * feather,
		               cornerRadius + feather);
		nvgFillPaint(vg, pillPaint);
		nvgFill(vg);

		// Draw 2-pass glow text on top of pill
		// Pass 1: Glow (blurred, lower alpha)
		nvgFontBlur(vg, 3.0f);
		nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha * 0.4f));
		nvgText(vg, x, y, text, NULL);

		// Pass 2: Sharp text on top
		nvgFontBlur(vg, 0.0f);
		nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha));
		nvgText(vg, x, y, text, NULL);
	}

	void drawBpmStack(NVGcontext* vg, int fontHandle, float alpha) {
		int ratioIdx = module->displayRatioIndex.load(std::memory_order_relaxed);
		float period = module->displaySmoothedPeriod.load(std::memory_order_relaxed);
		if (ratioIdx < 0 || period <= 0.f) return;

		float margin = 4.f;
		float pad = 4.f;
		float feather = 3.f;
		float cornerRadius = 3.f;

		// Calculate BPM values
		float rawBPM = 60.f / period;
		float effectiveBPM = rawBPM * AnalogLFO::RATIO_TABLE[ratioIdx];
		bool isX1 = (ratioIdx == 7); // RATIO_TABLE[7] = 1.0

		// Format effective BPM text
		std::string bpmText;
		if (effectiveBPM < 1.f)
			bpmText = rack::string::f("%.1f BPM", effectiveBPM);
		else
			bpmText = rack::string::f("%d BPM", (int)std::round(effectiveBPM));

		// Position: bottom-right corner
		float bpmFontSize = 10.f;
		float bpmX = box.size.x - margin;
		float bpmY = box.size.y - margin;

		if (isX1) {
			// Single line: just effective BPM with pill
			drawPillText(vg, fontHandle, bpmX, bpmY, bpmText.c_str(),
			             bpmFontSize, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM, alpha);
		} else {
			// Dual line: CLK on top, BPM below, shared pill

			// Format CLK text
			std::string clkText;
			if (rawBPM < 1.f)
				clkText = rack::string::f("%.1f CLK", rawBPM);
			else
				clkText = rack::string::f("%d CLK", (int)std::round(rawBPM));

			float clkFontSize = 8.f;
			float clkAlpha = alpha * 0.6f;

			// Measure BPM line bounds (10px, right-bottom aligned)
			nvgFontFaceId(vg, fontHandle);
			nvgFontSize(vg, bpmFontSize);
			nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
			float bpmBounds[4];
			nvgTextBounds(vg, bpmX, bpmY, bpmText.c_str(), NULL, bpmBounds);

			// Get BPM line height for spacing
			float ascender, descender, lineh;
			nvgTextMetrics(vg, &ascender, &descender, &lineh);

			// CLK line positioned above BPM line with 2px gap
			float clkY = bpmBounds[1] - 2.f; // top of BPM bounds minus gap
			float clkX = bpmX;

			// Measure CLK line bounds (8px, right-bottom aligned)
			nvgFontSize(vg, clkFontSize);
			nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
			float clkBounds[4];
			nvgTextBounds(vg, clkX, clkY, clkText.c_str(), NULL, clkBounds);

			// Union both bounds for shared pill
			float unionLeft = std::fmin(bpmBounds[0], clkBounds[0]);
			float unionTop = std::fmin(bpmBounds[1], clkBounds[1]);
			float unionRight = std::fmax(bpmBounds[2], clkBounds[2]);
			float unionBottom = std::fmax(bpmBounds[3], clkBounds[3]);

			// Shared pill dimensions
			float px = unionLeft - pad;
			float py = unionTop - pad;
			float pw = (unionRight - unionLeft) + 2.f * pad;
			float ph = (unionBottom - unionTop) + 2.f * pad;

			// Draw shared feathered pill background
			NVGpaint pillPaint = nvgBoxGradient(vg,
				px, py, pw, ph,
				cornerRadius, feather,
				nvgRGBAf(0.102f, 0.102f, 0.180f, 0.80f * alpha),
				nvgRGBAf(0.102f, 0.102f, 0.180f, 0.0f));
			nvgBeginPath(vg);
			nvgRoundedRect(vg, px - feather, py - feather,
			               pw + 2.f * feather, ph + 2.f * feather,
			               cornerRadius + feather);
			nvgFillPaint(vg, pillPaint);
			nvgFill(vg);

			// Draw CLK text (smaller, dimmer) on top of pill
			nvgFontFaceId(vg, fontHandle);
			nvgFontSize(vg, clkFontSize);
			nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);

			// CLK glow pass
			nvgFontBlur(vg, 3.0f);
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, clkAlpha * 0.4f));
			nvgText(vg, clkX, clkY, clkText.c_str(), NULL);

			// CLK sharp pass
			nvgFontBlur(vg, 0.0f);
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, clkAlpha));
			nvgText(vg, clkX, clkY, clkText.c_str(), NULL);

			// Draw BPM text (standard size, full alpha) on top of pill
			nvgFontSize(vg, bpmFontSize);
			nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);

			// BPM glow pass
			nvgFontBlur(vg, 3.0f);
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha * 0.4f));
			nvgText(vg, bpmX, bpmY, bpmText.c_str(), NULL);

			// BPM sharp pass
			nvgFontBlur(vg, 0.0f);
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.66f, 0.22f, alpha));
			nvgText(vg, bpmX, bpmY, bpmText.c_str(), NULL);
		}
	}

	void drawTextOverlays(NVGcontext* vg) {
		std::shared_ptr<Font> font = APP->window->loadFont(
			asset::system("res/fonts/ShareTechMono-Regular.ttf"));
		if (!font) return;

		float margin = 4.f;
		float fontSize = 10.f;

		int clockState = module->displayClockState.load(std::memory_order_relaxed);
		int ratioIdx = module->displayRatioIndex.load(std::memory_order_relaxed);

		// Hz readout (free-running mode, top-left)
		if (hzFadeAlpha > 0.001f) {
			float rate = module->params[AnalogLFO::RATE_PARAM].getValue();
			std::string hzText = rack::string::f("%.2f Hz", rate);
			drawPillText(vg, font->handle, margin, margin + fontSize,
			             hzText.c_str(), fontSize,
			             NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM, hzFadeAlpha);
		}

		// Ratio label (clocked mode, top-left)
		if (ratioFadeAlpha > 0.001f && ratioIdx >= 0) {
			drawPillText(vg, font->handle, margin, margin + fontSize,
			             AnalogLFO::RATIO_LABELS[ratioIdx], fontSize,
			             NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM, ratioFadeAlpha);
		}

		// SYNC badge (clocked mode, top-right)
		if (syncFadeAlpha > 0.001f) {
			float effectiveAlpha = syncFadeAlpha;
			if (clockState == AnalogLFO::ACQUIRING) {
				// Blink at ~2Hz: breathePhase runs at 0.8Hz, scale by 2.5
				float blink = 0.5f + 0.5f * std::sin(breathePhase * 2.5f);
				effectiveAlpha *= blink;
			}
			drawPillText(vg, font->handle, box.size.x - margin, margin + fontSize,
			             "SYNC", fontSize,
			             NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM, effectiveAlpha);
		}

		// BPM stack (clocked mode, bottom-right)
		// Dual-line CLK/BPM at non-x1 ratios, single BPM at x1
		if (bpmFadeAlpha > 0.001f) {
			drawBpmStack(vg, font->handle, bpmFadeAlpha);
		}

		// Swing overlay (clocked mode, bottom-left, only when swing > Straight)
		if (swingFadeAlpha > 0.001f) {
			int swingIdx = module->displaySwingIndex.load(std::memory_order_relaxed);
			if (swingIdx > 0 && swingIdx <= 5) {
				drawPillText(vg, font->handle, margin, box.size.y - margin,
				             AnalogLFO::SWING_OVERLAY_LABELS[swingIdx], fontSize,
				             NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM, swingFadeAlpha);
			}
		}
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
				drawTextOverlays(vg);
			} else {
				drawPlaceholder(vg);
			}

			nvgResetScissor(vg);
			nvgRestore(vg);
		}
		TransparentWidget::drawLayer(args, layer);
	}
};

// ============================================================
// Forge Noir Custom Widget Components
// ============================================================

struct ForgeKnobHero : app::SvgKnob {
	widget::SvgWidget* bg;
	ForgeKnobHero() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		shadow->opacity = 0.0;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobHero.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobHero_bg.svg")));
	}
};

struct ForgeKnobSecondary : app::SvgKnob {
	widget::SvgWidget* bg;
	ForgeKnobSecondary() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		shadow->opacity = 0.0;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobSecondary.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobSecondary_bg.svg")));
	}
};

struct ForgeKnobUtility : app::SvgKnob {
	widget::SvgWidget* bg;
	ForgeKnobUtility() {
		minAngle = -0.83 * M_PI;
		maxAngle = 0.83 * M_PI;
		shadow->opacity = 0.0;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobUtility.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobUtility_bg.svg")));
	}
};

struct ForgeTrimpot : app::SvgKnob {
	widget::SvgWidget* bg;
	ForgeTrimpot() {
		minAngle = -0.75 * M_PI;
		maxAngle = 0.75 * M_PI;
		shadow->opacity = 0.0;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeTrimpot.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeTrimpot_bg.svg")));
	}
};

struct ForgeJackInput : app::SvgPort {
	ForgeJackInput() {
		shadow->opacity = 0.0;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeJackInput.svg")));
	}
};

struct ForgeJackOutput : app::SvgPort {
	ForgeJackOutput() {
		shadow->opacity = 0.0;
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeJackOutput.svg")));
	}
};

struct ForgeHexBolt : app::SvgScrew {
	ForgeHexBolt() {
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeHexBolt.svg")));
	}
};

struct AnalogLFOWidget : ModuleWidget {
	AnalogLFOWidget(AnalogLFO* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/AnalogLFO.svg")));

		// Hex bolt screws (4 corners)
		addChild(createWidget<ForgeHexBolt>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ForgeHexBolt>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ForgeHexBolt>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ForgeHexBolt>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Waveform display (repositioned for 14HP Forge Noir layout)
		{
			WaveformDisplay* display = new WaveformDisplay();
			display->module = module;
			display->box.pos = mm2px(Vec(3.60f, 13.19f));
			display->box.size = mm2px(Vec(63.93f, 17.98f));
			addChild(display);
		}

		// Main knobs
		addParam(createParamCentered<ForgeKnobHero>(mm2px(Vec(35.56f, 47.35f)),
		         module, AnalogLFO::MORPH_PARAM));
		addParam(createParamCentered<ForgeKnobSecondary>(mm2px(Vec(21.18f, 67.32f)),
		         module, AnalogLFO::CHARACTER_PARAM));
		addParam(createParamCentered<ForgeKnobSecondary>(mm2px(Vec(49.94f, 67.32f)),
		         module, AnalogLFO::DRIFT_PARAM));
		addParam(createParamCentered<ForgeKnobUtility>(mm2px(Vec(21.18f, 83.51f)),
		         module, AnalogLFO::RATE_PARAM));
		addParam(createParamCentered<ForgeKnobUtility>(mm2px(Vec(49.94f, 83.51f)),
		         module, AnalogLFO::PHASE_OFFSET_PARAM));

		// CV trimpots
		addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(9.19f, 95.89f)),
		         module, AnalogLFO::MORPH_ATTEN_PARAM));
		addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(22.77f, 95.89f)),
		         module, AnalogLFO::CHARACTER_ATTEN_PARAM));
		addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(35.56f, 95.89f)),
		         module, AnalogLFO::DRIFT_ATTEN_PARAM));
		addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(48.75f, 95.89f)),
		         module, AnalogLFO::FM_ATTEN_PARAM));
		addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(61.93f, 95.89f)),
		         module, AnalogLFO::PHASE_OFFSET_ATTEN_PARAM));

		// CV input jacks
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(9.19f, 103.08f)),
		         module, AnalogLFO::MORPH_CV_INPUT));
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(22.77f, 103.08f)),
		         module, AnalogLFO::CHARACTER_CV_INPUT));
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(35.56f, 103.08f)),
		         module, AnalogLFO::DRIFT_CV_INPUT));
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(48.75f, 103.08f)),
		         module, AnalogLFO::FM_INPUT));
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(61.93f, 103.08f)),
		         module, AnalogLFO::PHASE_OFFSET_CV_INPUT));

		// Bottom I/O row
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(14.38f, 117.47f)),
		         module, AnalogLFO::CLK_INPUT));
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(35.56f, 117.47f)),
		         module, AnalogLFO::RESET_INPUT));
		addOutput(createOutputCentered<ForgeJackOutput>(mm2px(Vec(56.74f, 117.47f)),
		          module, AnalogLFO::OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		AnalogLFO* module = getModule<AnalogLFO>();
		if (!module) return;

		menu->addChild(new MenuSeparator);

		menu->addChild(createIndexSubmenuItem("Swing",
			{"Straight 50%", "Light 54%", "Medium 58%",
			 "Triplet 66%", "Heavy 71%", "Max 75%"},
			[=]() { return (size_t)module->swingIndex; },
			[=](size_t idx) { module->swingIndex = (int)idx; }
		));
	}
};

Model* modelAnalogLFO = createModel<AnalogLFO, AnalogLFOWidget>("ForgeAnalogLFO");
