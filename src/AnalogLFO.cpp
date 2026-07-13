#include "plugin.hpp"
#include "dsp/LfoCore.hpp"   // forge::LfoCore — the extracted DSP core the shell delegates to
#include "dsp/MathConst.hpp"  // forge::kPi (D-06, rack-free pi constant)
#include "dsp/PatchParse.hpp"  // forge::parseSeedHex — non-throwing seed parse (BUG-04)
#include "dsp/DisplayFill.hpp"  // pure forge preview fill + DISPLAY_SAMPLES (CLEAN-05, 24-01)
#include "dsp/Anim.hpp"  // frame-rate-independent dt clamp + badge-decay helpers (CLEAN-04, 24-01)
#include <cmath>
#include <atomic>
#include <array>
#include <cstdint>
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

	// The extracted DSP core (D-02/D-08). process() delegates the whole per-sample
	// chain to core.step(); the inline DSP that used to live here is DELETED — there
	// is exactly one definition (Pitfall 4). RNG seeding (random_device) stays on the
	// shell side and is forwarded into the core via core.seed()/core.setSpreadSeed().
	forge::LfoCore core;

	// Display double buffer (lock-free audio-to-GUI transfer)
	static constexpr int DISPLAY_SAMPLES = forge::DISPLAY_SAMPLES;  // single-source the sample count (Pitfall 6)
	std::array<float, DISPLAY_SAMPLES> displayBuffers[2] = {};
	std::atomic<int> displayReadIdx{0};

	// CLEAN-05 / D-01/D-02: the heavy 256x preview fill no longer runs on the audio
	// thread. process() publishes this tear-free seqlock snapshot at the trigger
	// instant; WaveformDisplay::step() (GUI thread) consumes it and runs the fill.
	// bleedLfo is captured AT TRIGGER (never re-read live at paint — D-02 / Pitfall 3).
	struct DisplaySnapshot {        // 16-byte POD copied by value across the audio->GUI boundary
		float morph     = 0.f;
		float character = 0.f;
		float swingFrac = 0.5f;     // EFFECTIVE (gated) value — never recomputed GUI-side
		float bleedLfo  = 0.f;      // core.drift.ouLayers[0].state captured at the trigger instant
	};
	DisplaySnapshot displaySnapshot;                  // non-atomic payload (guarded by the seqlock)
	std::atomic<uint32_t> displaySnapshotSeq{0};      // even=stable, odd=writing; doubles as the dirty flag
	std::atomic<float> displayPhase{0.f};
	std::atomic<float> displayDrift{0.f};   // Combined CV-modulated drift level for display

	// Display update tracking
	float prevDisplayMorph = -1.f;
	float prevDisplayCharacter = -1.f;
	double prevPhaseForDisplay = 0.0;
	float displayUpdateTimer = 0.f;

	// Clock/drift/slew/crossfade/reset DSP state now lives inside `core`
	// (forge::LfoCore). The shell keeps ONLY the GUI-thread display atomics it
	// publishes from process() each frame, plus the persisted spread seed and
	// the swing index. (Inline DSP members DELETED — Pitfall 4.)
	std::atomic<int> displayClockState{0};
	std::atomic<int> displayClockEdge{0};  // ANIM-01: incremented once per LOCKED clock edge (audio write-only; widget read-only)
	std::atomic<int> displayRatioIndex{-1};  // -1 = free-running, 0-14 = ratio index
	std::atomic<float> displaySmoothedPeriod{0.f};

	// Component spread: per-instance reproducible seed (CHAR-04). The spread
	// COEFFICIENTS now live in core.drift / core.wave; only the persisted seed
	// stays in the shell (dataToJson/dataFromJson).
	uint64_t spreadSeed[2] = {};

	// Swing state (PHASE-03, PHASE-04)
	int swingIndex = 0;  // 0=Straight (default), persisted via dataToJson
	int prevSwingIndex = -1;  // tracks changes for display buffer refresh

	// Display bridge atomics for swing
	std::atomic<int> displaySwingIndex{0};
	std::atomic<float> displaySwingFraction{0.5f};

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

	// Re-seed the core's component spread from the persisted spreadSeed. The spread
	// COEFFICIENTS (saw/square/tri/bleed/pulse + characterSpread + OU weights) now
	// live in core.drift / core.wave (forge::DriftEngine::setSpreadSeed). This wraps
	// the inline initComponentSpread() so dataFromJson can reproduce them on load.
	void initComponentSpread() {
		core.setSpreadSeed(spreadSeed[0], spreadSeed[1]);
	}

	// GUI-thread fill: runs the heavy 256x preview loop behind the existing
	// displayBuffers[2] + displayReadIdx double-buffer. Delegates to the pure
	// header helper (24-01) — same compiled fill, so the rendered preview
	// is byte-identical. bleedLfo comes from the snapshot (captured at trigger), so
	// this fill structurally cannot read live drift at paint time (D-02). The release
	// store at the end is unchanged and still pairs with the drawLayer acquire load.
	void fillFromSnapshot(const DisplaySnapshot& s) {
		int writeIdx = 1 - displayReadIdx.load(std::memory_order_relaxed);
		forge::fillDisplayBuffer(displayBuffers[writeIdx], core.wave,
		                         s.morph, s.character, s.swingFrac, s.bleedLfo);
		displayReadIdx.store(writeIdx, std::memory_order_release);
	}

	AnalogLFO() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(MORPH_PARAM, 0.f, 1.f, 0.f, "Morph");
		configParam(CHARACTER_PARAM, 0.f, 1.f, 0.f, "Character");
		configParam(DRIFT_PARAM, 0.f, 1.f, 0.f, "Drift");
		configParam<RateParamQuantity>(RATE_PARAM, 0.01f, 20.f, 0.7f, "Rate", " Hz");
		configParam(MORPH_ATTEN_PARAM, 0.f, 1.f, 0.f, "Morph CV", "%", 0.f, 100.f);
		configParam(CHARACTER_ATTEN_PARAM, 0.f, 1.f, 0.f, "Character CV", "%", 0.f, 100.f);
		configParam(DRIFT_ATTEN_PARAM, 0.f, 1.f, 0.f, "Drift CV", "%", 0.f, 100.f);
		configInput(MORPH_CV_INPUT, "Morph CV");
		configInput(DRIFT_CV_INPUT, "Drift CV");
		configInput(CHARACTER_CV_INPUT, "Character CV");
		configInput(CLK_INPUT, "Clock");
		configInput(RESET_INPUT, "Reset");
		configParam(PHASE_OFFSET_PARAM, 0.f, 1.f, 0.f, "Phase Offset", " deg", 0.f, 360.f);
		configParam(PHASE_OFFSET_ATTEN_PARAM, 0.f, 1.f, 0.f, "Phase Offset CV", "%", 0.f, 100.f);
		configInput(PHASE_OFFSET_CV_INPUT, "Phase Offset CV");
		configParam(FM_ATTEN_PARAM, 0.f, 1.f, 0.f, "FM Depth", "%", 0.f, 100.f);
		configInput(FM_INPUT, "FM");
		configOutput(OUTPUT, "LFO");
		// Prime the display buffer with a default snapshot so the preview is
		// initialised before the first process()/step() (was updateDisplayBuffer(0,0)).
		fillFromSnapshot(DisplaySnapshot{});

		// The slew filters and OU-layer constants are initialized inside the core
		// (forge::LfoCore / forge::DriftEngine constructors) — no longer here.

		// Per-module unique seed (random_device) forwarded into the core. The spread
		// seed is derived from a module-side RNG draw, exactly as the inline code did
		// (it seeded a Xoroshiro from random_device, then drew spreadSeed[0/1]).
		std::random_device rd;
		forge::Xoroshiro128Plus seedRng;
		seedRng.seed(rd(), rd());
		core.seed(seedRng.state[0], seedRng.state[1]);   // drift RNG seed (post-seed state)
		spreadSeed[0] = seedRng();
		spreadSeed[1] = seedRng();
		initComponentSpread();   // -> core.setSpreadSeed(spreadSeed[0], spreadSeed[1])
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
		// Type-check, not just presence: a hand-edited patch with non-string
		// nodes makes json_string_value() return NULL -> std::stoull(NULL) is UB (CR-02).
		if (json_is_string(s0J) && json_is_string(s1J)) {
			// BUG-04: non-throwing parse — a malformed/over-long/empty seed string can no
			// longer throw and crash Rack on load. Parse into temporaries and only commit
			// the restored seed (+ regenerate spread) when BOTH succeed; otherwise keep the
			// constructor-seeded spread (the CODE-REVIEW #4 safe fallback).
			uint64_t s0 = 0, s1 = 0;
			if (forge::parseSeedHex(json_string_value(s0J), s0) &&
			    forge::parseSeedHex(json_string_value(s1J), s1)) {
				spreadSeed[0] = s0;
				spreadSeed[1] = s1;
				initComponentSpread();  // regenerate deterministic offsets from restored seed
			}
		}
		// Swing preset (PHASE-03)
		json_t* swingJ = json_object_get(rootJ, "swingIndex");
		if (swingJ)
			swingIndex = rack::math::clamp((int)json_integer_value(swingJ), 0, 5);
	}

	void process(const ProcessArgs& args) override {
		// --- Pack the Rack arrays into the POD forge::Inputs and delegate the entire
		//     per-sample DSP chain to the extracted core (D-02/D-08). The shell does
		//     only param/input/output I/O + display publishing + JSON. ---
		forge::Inputs in;
		in.rate         = params[RATE_PARAM].getValue();
		in.ratioScaled  = paramQuantities[RATE_PARAM]->getScaledValue();

		// Morph with CV (additive offset, attenuator, hard clamp)
		{
			float k = params[MORPH_PARAM].getValue();
			float a = params[MORPH_ATTEN_PARAM].getValue();
			float cv = inputs[MORPH_CV_INPUT].getVoltage();
			in.morph = rack::math::clamp(k + a * cv / 5.f, 0.f, 1.f);
		}
		// Character with CV (+ characterSpread folded in, matching the inline path)
		{
			float k = params[CHARACTER_PARAM].getValue();
			float a = params[CHARACTER_ATTEN_PARAM].getValue();
			float cv = inputs[CHARACTER_CV_INPUT].getVoltage();
			in.character = rack::math::clamp(k + a * cv / 5.f + core.drift.characterSpread, 0.f, 1.f);
		}
		// Drift with CV
		{
			float k = params[DRIFT_PARAM].getValue();
			float a = params[DRIFT_ATTEN_PARAM].getValue();
			float cv = inputs[DRIFT_CV_INPUT].getVoltage();
			in.drift = rack::math::clamp(k + a * cv / 5.f, 0.f, 1.f);
		}
		// Phase offset (knob + optional CV)
		{
			float k = params[PHASE_OFFSET_PARAM].getValue();
			float cv = 0.f;
			if (inputs[PHASE_OFFSET_CV_INPUT].isConnected()) {
				float a = params[PHASE_OFFSET_ATTEN_PARAM].getValue();
				cv = a * inputs[PHASE_OFFSET_CV_INPUT].getVoltage() / 5.f;
			}
			in.phaseOffset = rack::math::clamp(k + cv, 0.f, 1.f);
		}
		// FM (volts; 0 if unpatched)
		in.fmConnected = inputs[FM_INPUT].isConnected();
		in.fmCV        = in.fmConnected ? inputs[FM_INPUT].getVoltage() : 0.f;
		in.fmAtten     = params[FM_ATTEN_PARAM].getValue();
		// Clock / Reset
		in.clkConnected   = inputs[CLK_INPUT].isConnected();
		in.clkVoltage     = inputs[CLK_INPUT].getVoltage();
		in.resetConnected = inputs[RESET_INPUT].isConnected();
		in.resetVoltage   = inputs[RESET_INPUT].getVoltage();
		// Swing + sampleTime (injected)
		in.swingIndex  = swingIndex;
		in.sampleTime  = args.sampleTime;

		float outputVoltage = core.step(in);

		// --- Publish display atomics from the core's last-step telemetry ---
		const forge::LfoCore::Telemetry& t = core.tel;
		displayClockState.store(t.clockState, std::memory_order_relaxed);
		displayRatioIndex.store(t.ratioIdx, std::memory_order_relaxed);
		displaySmoothedPeriod.store(t.smoothedPeriod, std::memory_order_relaxed);
		displayDrift.store(t.drift, std::memory_order_relaxed);
		displayPhase.store(t.displayPhase, std::memory_order_relaxed);
		displaySwingIndex.store(swingIndex, std::memory_order_relaxed);
		// BUG-03: store the EFFECTIVE (gated) swing — free-run stores 0.5 (no warp),
		// clocked stores swingFrac. Mirrors the buffer-gen gate at L332 so drawPhaseDot
		// warps the dot with the same value the trace/audio uses (no free-run desync).
		displaySwingFraction.store(t.isClocked ? t.swingFrac : 0.5f, std::memory_order_relaxed);
		// ANIM-01: re-arm SYNC badge flash once per LOCKED clock edge (AnalogLFO.cpp:540-542).
		if (t.lockedEdge) displayClockEdge.fetch_add(1, std::memory_order_relaxed);

		// --- Display preview buffer refresh (GUI-thread bridge; same trigger logic
		//     as before: phase-wrap always, param changes rate-limited to ~30fps). ---
		double phaseNow = core.phase;
		float morph = in.morph;
		float character = in.character;
		bool phaseWrapped = (phaseNow < prevPhaseForDisplay);
		bool morphChanged = (std::fabs(morph - prevDisplayMorph) > 0.002f);
		bool characterChanged = (std::fabs(character - prevDisplayCharacter) > 0.002f);
		bool swingChanged = (swingIndex != prevSwingIndex);
		displayUpdateTimer += args.sampleTime;
		bool paramReady = displayUpdateTimer >= (1.f / 30.f);
		if (phaseWrapped || ((morphChanged || characterChanged || swingChanged) && paramReady)) {
			float displaySwing = t.isClocked ? t.swingFrac : 0.5f;  // no warp in free-running display
			// D-01: publish a tear-free seqlock snapshot instead of running the fill on
			// the audio thread (wait-free: three atomic stores + a release fence, no loop).
			// The heavy 256x fill now runs GUI-side in WaveformDisplay::step().
			uint32_t s = displaySnapshotSeq.load(std::memory_order_relaxed);
			displaySnapshotSeq.store(s + 1, std::memory_order_relaxed);   // begin write (odd)
			std::atomic_thread_fence(std::memory_order_release);
			displaySnapshot.morph     = morph;
			displaySnapshot.character = character;
			displaySnapshot.swingFrac = displaySwing;                     // same effective gate as L344
			displaySnapshot.bleedLfo  = core.drift.ouLayers[0].state;     // capture-at-trigger (D-02 / Pitfall 3)
			displaySnapshotSeq.store(s + 2, std::memory_order_release);   // commit (even)
			prevDisplayMorph = morph;
			prevDisplayCharacter = character;
			prevSwingIndex = swingIndex;
			displayUpdateTimer = 0.f;
		}
		prevPhaseForDisplay = phaseNow;

		// Bipolar +/-5V output
		outputs[OUTPUT].setVoltage(outputVoltage);
	}
};

// Out-of-line definitions for the in-class static constexpr arrays. Required under
// C++11/14 (the Rack toolchain builds with -std=c++11): in-class initializers are
// declarations only, and ODR-use (runtime indexing takes the array's address) needs
// a definition or MinGW's linker fails with "undefined reference". Redundant but
// harmless from C++17 onward.
constexpr float AnalogLFO::RATIO_TABLE[15];
constexpr const char* AnalogLFO::RATIO_LABELS[15];
constexpr float AnalogLFO::SWING_FRACTIONS[6];
constexpr const char* AnalogLFO::SWING_MENU_LABELS[6];
constexpr const char* AnalogLFO::SWING_OVERLAY_LABELS[6];

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

	// CLEAN-03 / D-05: widget-side cache of the last genuinely-clocked ratio index +
	// period. On disconnect the audio thread publishes -1, but the cache holds the last
	// valid content so the ratio + BPM pills fade out (alpha-gated) instead of popping off.
	int cachedRatioIdx = -1;
	float cachedPeriod = 0.f;

	// CRT scanline scroll accumulator
	float scanlineScrollPhase = 0.f;

	// Dedicated 2Hz blink accumulator for SYNC ACQUIRING indicator (Plan 02 uses this)
	// Separate from breathePhase so border glow (0.2Hz) and blink (2Hz) are independent
	float blinkPhase = 0.f;

	// SYNC badge per-edge flash envelope (ANIM-01/ANIM-02, Phase 21).
	// prevClockEdge caches the last-seen edge counter; flashIntensity re-arms to 1.0 on a
	// new LOCKED edge then decays via the continuous-time badge-decay helper (pow(0.92, dt*60)). Widget reads the atomic only.
	int prevClockEdge = 0;
	float flashIntensity = 0.f;

	// CLEAN-05 / D-01: last seqlock value consumed from the module. The heavy 256x
	// preview fill now runs here (GUI thread) only when the snapshot changes.
	uint32_t lastConsumedSeq = 0;

	void step() override {
		// CLEAN-04 / D-03/D-04: advance every animation by clamped wall-clock dt so
		// the feel is frame-rate-independent (identical at 60fps, correct at 144Hz).
		// GUI-thread-only call — the window's last-frame duration must never be read off-thread.
		float dt = forge::clampFrameDt((float)APP->window->getLastFrameDuration());

		// Advance breathe animation (0.2Hz = 5-second cycle, per D-13)
		breathePhase += 2.f * (float)forge::kPi * 0.2f * dt;
		if (breathePhase > 2.f * (float)forge::kPi) breathePhase -= 2.f * (float)forge::kPi;

		// 2Hz blink for SYNC ACQUIRING indicator (independent of border glow rate)
		blinkPhase += 2.f * (float)forge::kPi * 2.f * dt;
		if (blinkPhase > 100.f * (float)forge::kPi) blinkPhase -= 100.f * (float)forge::kPi;  // prevent float overflow

		// Advance scanline scroll (~1px/sec)
		scanlineScrollPhase += dt;
		scanlineScrollPhase = fmodf(scanlineScrollPhase, 4.f);  // wrap at tile height

		// Advance fade timers for text overlays (200ms transitions)
		if (module) {
			// CLEAN-05 / D-01: consume the audio-thread display snapshot tear-free,
			// then run the heavy preview fill on the GUI thread only when it changed.
			// Tear-free per the seqlock contract: retry while the writer is mid-update
			// (odd seq) or the seq moved between the acquire-load and the payload copy.
			uint32_t seq;
			AnalogLFO::DisplaySnapshot snap;
			do {
				seq = module->displaySnapshotSeq.load(std::memory_order_acquire);
				if (seq & 1u) continue;                                  // writer mid-update -> re-read
				snap = module->displaySnapshot;                          // copy the 16-byte payload
				std::atomic_thread_fence(std::memory_order_acquire);
			} while (seq != module->displaySnapshotSeq.load(std::memory_order_relaxed));
			if (seq != lastConsumedSeq) {                                // dirty?
				module->fillFromSnapshot(snap);                          // GUI-thread fill (the moved 256x loop)
				lastConsumedSeq = seq;
			}

			// CLEAN-03 / D-05: refresh the ratio/period cache ONLY while genuinely clocked.
			// On disconnect the atomics go to -1; the cache then retains the last valid
			// content so the ratio + BPM pills fade out in sync with the SYNC badge.
			int ri = module->displayRatioIndex.load(std::memory_order_relaxed);
			float per = module->displaySmoothedPeriod.load(std::memory_order_relaxed);
			if (ri >= 0 && per > 0.f) {
				cachedRatioIdx = ri;
				cachedPeriod = per;
			}

			int clockState = module->displayClockState.load(std::memory_order_relaxed);
			float fadeSpeed = dt / 0.2f;  // 200ms ramp, frame-rate-independent (CLEAN-04)

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

			// SYNC badge flash envelope (ANIM-01/ANIM-02): re-arm to peak on each new LOCKED
			// edge, then exponential decay. Advanced exactly once per step() (frame-paced,
			// matching breathePhase/blinkPhase); the widget only reads the atomic.
			int edge = module->displayClockEdge.load(std::memory_order_relaxed);
			if (edge != prevClockEdge) {
				flashIntensity = 1.f;  // re-peak (clean retrigger even mid-decay)
				prevClockEdge = edge;
			}
			// ANIM-02 locked decay -- the 0.92 factor now lives inside the continuous-time decay
			// helper (pow(0.92, dt*60)), preserved by mathematical equivalence (D-03/D-04); do NOT re-tune it.
			flashIntensity = forge::flashDecay(flashIntensity, dt);
			if (flashIntensity < 0.001f) flashIntensity = 0.f;  // snap to steady state
		}

		TransparentWidget::step();
	}

	// Coordinate helpers
	float phaseToX(float phase) const {
		// Center column: 20% to 80% of display width (per D-02)
		float centerStart = box.size.x * 0.20f;
		float centerWidth = box.size.x * 0.60f;
		return centerStart + phase * centerWidth;
	}

	float valueToY(float value) const {
		float margin = 8.f;  // 14HP=6f; bumped for the taller 18HP box (Plan 04, D-06) so the trace breathes
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
		nvgRoundedRect(vg, 0, 0, box.size.x, box.size.y, 4.0f);
		nvgFillColor(vg, nvgRGBAf(0.012f, 0.012f, 0.012f, 1.f));
		nvgFill(vg);
	}

	void drawBorder(NVGcontext* vg) {
		float w = box.size.x;
		float h = box.size.y;

		// Breathing glow: 0.15 to 0.40 opacity, 5s cycle (per D-13, boosted for visibility)
		float glowAlpha = 0.15f + 0.25f * (0.5f + 0.5f * std::sin(breathePhase));

		// Outer glow (inner cutout approach to stay within scissor bounds — per Pitfall 7)
		nvgBeginPath(vg);
		nvgRoundedRect(vg, -4.f, -4.f, w + 8.f, h + 8.f, 8.f);
		nvgRoundedRect(vg, 0, 0, w, h, 4.f);
		nvgPathWinding(vg, NVG_HOLE);
		nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, glowAlpha));
		nvgFill(vg);

		// Solid 1.5px ember border
		nvgBeginPath(vg);
		nvgRoundedRect(vg, 0, 0, w, h, 4.f);
		nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.3f));
		nvgStrokeWidth(vg, 1.5f);
		nvgStroke(vg);
	}

	void drawCornerBrackets(NVGcontext* vg) {
		float inset = 4.f;   // 14HP=3f; bumped ~+33% for the larger 18HP box (Plan 04, D-06)
		float size = 6.f;    // 14HP=5f; bumped ~+20% in step
		float w = box.size.x;
		float h = box.size.y;

		nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.4f));
		nvgStrokeWidth(vg, 1.0f);
		nvgLineCap(vg, NVG_BUTT);

		// Top-left
		nvgBeginPath(vg);
		nvgMoveTo(vg, inset, inset + size);
		nvgLineTo(vg, inset, inset);
		nvgLineTo(vg, inset + size, inset);
		nvgStroke(vg);

		// Top-right
		nvgBeginPath(vg);
		nvgMoveTo(vg, w - inset - size, inset);
		nvgLineTo(vg, w - inset, inset);
		nvgLineTo(vg, w - inset, inset + size);
		nvgStroke(vg);

		// Bottom-left
		nvgBeginPath(vg);
		nvgMoveTo(vg, inset, h - inset - size);
		nvgLineTo(vg, inset, h - inset);
		nvgLineTo(vg, inset + size, h - inset);
		nvgStroke(vg);

		// Bottom-right
		nvgBeginPath(vg);
		nvgMoveTo(vg, w - inset - size, h - inset);
		nvgLineTo(vg, w - inset, h - inset);
		nvgLineTo(vg, w - inset, h - inset - size);
		nvgStroke(vg);
	}

	void drawScanlines(NVGcontext* vg) {
		// CRT phosphor-row effect: faint bright ember lines on dark background
		float spacing = 2.5f;
		float scrollOffset = fmodf(scanlineScrollPhase * spacing, spacing);

		nvgBeginPath(vg);
		for (float y = -spacing + scrollOffset; y < box.size.y + spacing; y += spacing) {
			nvgRect(vg, 0, y, box.size.x, 0.5f);
		}
		nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.10f));
		nvgFill(vg);
	}

	void drawWaveformTrace(NVGcontext* vg, const std::array<float, 256>& buffer, float dimFactor) {
		// 3-layer glow per design language (D-09): wide diffuse, medium, sharp core
		const float widths[]  = {7.0f, 3.5f, 2.0f};
		const float alphas[] = {0.06f, 0.15f, 0.85f};

		for (int pass = 0; pass < 3; pass++) {
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
			// Ember color #e85d26 per D-11 (replaces amber 0.91, 0.66, 0.22)
			nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, alphas[pass] * dimFactor));
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
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, trailAlpha));
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

		// 3-circle concentric phase dot (replaces radial gradient halo)
		// Outer glow: ember, 3x radius, 0.08 opacity
		nvgBeginPath(vg);
		nvgCircle(vg, x, y, dotRadius * 3.f * haloJitter);
		nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.08f * dimFactor * breatheFactor));
		nvgFill(vg);

		// Mid ring: ember, 2x radius, 0.22 opacity
		nvgBeginPath(vg);
		nvgCircle(vg, x, y, dotRadius * 2.f * haloJitter);
		nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.22f * dimFactor * breatheFactor));
		nvgFill(vg);

		// Core: hot gold #f0a030, 1x radius, 0.95 opacity (per D-11)
		nvgBeginPath(vg);
		nvgCircle(vg, x, y, dotRadius);
		nvgFillColor(vg, nvgRGBAf(0.941f, 0.627f, 0.188f, 0.95f * dimFactor * breatheFactor));
		nvgFill(vg);
	}

	void drawGlowText(NVGcontext* vg, int fontHandle, float x, float y,
	                  const char* text, float fontSize, int align, float alpha) {
		nvgFontFaceId(vg, fontHandle);
		nvgFontSize(vg, fontSize);
		nvgTextAlign(vg, align);

		// Glow pass (reduced blur for small fonts)
		nvgFontBlur(vg, 2.0f);
		nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, alpha * 0.3f));
		nvgText(vg, x, y, text, NULL);

		// Sharp pass
		nvgFontBlur(vg, 0.0f);
		nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, alpha));
		nvgText(vg, x, y, text, NULL);
	}

	void drawPillText(NVGcontext* vg, int fontHandle, float x, float y,
	                  const char* text, float fontSize, int align, float alpha, float flash = 0.f) {
		nvgFontFaceId(vg, fontHandle);
		nvgFontSize(vg, fontSize);
		nvgTextAlign(vg, align);

		float bounds[4];
		nvgTextBounds(vg, x, y, text, NULL, bounds);

		float pad = 3.f;
		float cornerRadius = 2.f;  // per D-08: 2-2.5px
		float px = bounds[0] - pad;
		float py = bounds[1] - pad;
		float pw = (bounds[2] - bounds[0]) + 2.f * pad;
		float ph = (bounds[3] - bounds[1]) + 2.f * pad;

		// Ember-tinted fill (per D-06)
		nvgBeginPath(vg);
		nvgRoundedRect(vg, px, py, pw, ph, cornerRadius);
		nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.10f * alpha));
		nvgFill(vg);

		// Ember stroke border (per D-06/D-08)
		nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.25f * alpha));
		nvgStrokeWidth(vg, 0.5f);
		nvgStroke(vg);

		// Flash modulation (Phase 21, ANIM-01 / D-01): lerp ember -> white-hot and bloom the
		// glow, driven by flash in [0,1]. Brightness comes from color + glow, NOT alpha.
		// Peak constants (white-hot hue, peak blur, peak glow alpha) are gate-tuned per D-03;
		// these are the UI-SPEC starting values. At flash == 0 this is byte-for-byte the prior
		// steady-state render (and every other caller passes the default, so unchanged).
		float r = 0.91f  + flash * (1.00f - 0.91f);   // ember -> white-hot (1.0, 0.93, 0.78)
		float g = 0.365f + flash * (0.93f - 0.365f);
		float b = 0.149f + flash * (0.78f - 0.149f);
		float glowBlur  = 2.0f + flash * (5.0f - 2.0f);   // 2.0 steady -> ~5.0 peak bloom
		float glowAlpha = alpha * (0.3f + flash * (0.9f - 0.3f));  // 0.3 -> ~0.9 stronger bloom

		// Glow text pass
		nvgFontBlur(vg, glowBlur);  // base 2.0 (reduced from 3.0 for small font sizes)
		nvgFillColor(vg, nvgRGBAf(r, g, b, glowAlpha));
		nvgText(vg, x, y, text, NULL);

		// Sharp text pass (same lerped color; alpha stays full per D-01)
		nvgFontBlur(vg, 0.0f);
		nvgFillColor(vg, nvgRGBAf(r, g, b, alpha));
		nvgText(vg, x, y, text, NULL);
	}

	// CLEAN-03 / D-05: ratioIdx + period are the widget-cached last-clocked values
	// (passed in), so the stack draws behind the bpmFadeAlpha gate at the call site
	// instead of self-loading the atomics + popping off on disconnect.
	void drawBpmStack(NVGcontext* vg, int fontHandle, float alpha, int ratioIdx, float period) {
		float pad = 3.f;
		float cornerRadius = 2.f;

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

		// Position: right column bottom
		float bpmFontSize = 4.0f;  // 14HP=3.5f; bumped ~+14% for the larger 18HP box (Plan 04, D-06)
		float bpmX = box.size.x * 0.89f;
		float bpmY = box.size.y - 4.f;

		if (isX1) {
			// Single line: just effective BPM with pill
			drawPillText(vg, fontHandle, bpmX, bpmY, bpmText.c_str(),
			             bpmFontSize, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM, alpha);
		} else {
			// Dual line: CLK on top, BPM below, shared pill

			// Format CLK text
			std::string clkText;
			if (rawBPM < 1.f)
				clkText = rack::string::f("%.1f CLK", rawBPM);
			else
				clkText = rack::string::f("%d CLK", (int)std::round(rawBPM));

			float clkFontSize = 3.3f;  // 14HP=2.9f; bumped in step with bpmFontSize (Plan 04, D-06)
			float clkAlpha = alpha * 0.6f;

			// Measure BPM line bounds (center-bottom aligned)
			nvgFontFaceId(vg, fontHandle);
			nvgFontSize(vg, bpmFontSize);
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
			float bpmBounds[4];
			nvgTextBounds(vg, bpmX, bpmY, bpmText.c_str(), NULL, bpmBounds);

			// Get BPM line height for spacing
			float ascender, descender, lineh;
			nvgTextMetrics(vg, &ascender, &descender, &lineh);

			// CLK line positioned above BPM line with 2px gap
			float clkY = bpmBounds[1] - 2.f; // top of BPM bounds minus gap
			float clkX = bpmX;

			// Measure CLK line bounds (center-bottom aligned)
			nvgFontSize(vg, clkFontSize);
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
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

			// Shared pill -- ember-tinted fill + stroke (replaces navy box gradient)
			nvgBeginPath(vg);
			nvgRoundedRect(vg, px, py, pw, ph, cornerRadius);
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.10f * alpha));
			nvgFill(vg);
			nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.25f * alpha));
			nvgStrokeWidth(vg, 0.5f);
			nvgStroke(vg);

			// Draw CLK text (smaller, dimmer) on top of pill
			nvgFontFaceId(vg, fontHandle);
			nvgFontSize(vg, clkFontSize);
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);

			// CLK glow pass
			nvgFontBlur(vg, 2.0f);
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, clkAlpha * 0.3f));
			nvgText(vg, clkX, clkY, clkText.c_str(), NULL);

			// CLK sharp pass
			nvgFontBlur(vg, 0.0f);
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, clkAlpha));
			nvgText(vg, clkX, clkY, clkText.c_str(), NULL);

			// Draw BPM text (standard size, full alpha) on top of pill
			nvgFontSize(vg, bpmFontSize);
			nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);

			// BPM glow pass
			nvgFontBlur(vg, 2.0f);
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, alpha * 0.3f));
			nvgText(vg, bpmX, bpmY, bpmText.c_str(), NULL);

			// BPM sharp pass
			nvgFontBlur(vg, 0.0f);
			nvgFillColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, alpha));
			nvgText(vg, bpmX, bpmY, bpmText.c_str(), NULL);
		}
	}

	void drawTextOverlays(NVGcontext* vg) {
		// Load JetBrains Mono NL font per D-07
		std::shared_ptr<Font> font = APP->window->loadFont(
			asset::plugin(pluginInstance, "res/fonts/JetBrainsMonoNL-Regular.ttf"));
		if (!font) return;

		// Column positions (proportional, per UI-SPEC spacing scale)
		float leftColX = box.size.x * 0.11f;   // center of left column
		float rightColX = box.size.x * 0.89f;  // center of right column
		float topY = 7.f;                       // top row baseline (14HP=6f; +1px so the larger top pills clear the top edge, Plan 04 D-06)
		float bottomY = box.size.y - 4.f;       // bottom row baseline (kept; the BPM/swing stacks grow upward from here, no bottom clip in the taller box)

		// Font sizes at widget scale (from UI-SPEC Typography section)
		float pillLabelSize = 4.6f;   // ratio, SYNC (14HP=4.1f; bumped ~+12% for 18HP box, Plan 04 D-06)
		// BPM value size lives in drawBpmStack (bpmFontSize), not here (14HP=3.5f; bumped in step)
		float pillSmallSize = 3.6f;   // swing percentage (14HP=3.2f; bumped ~+12%)
		float pillMicroSize = 3.3f;   // Hz, SWING label, CLK/LFO sub-labels (14HP=2.9f; bumped ~+14%)

		int clockState = module->displayClockState.load(std::memory_order_relaxed);

		// === LEFT COLUMN ===

		// Hz readout (free-running mode, left column top)
		if (hzFadeAlpha > 0.001f) {
			float rate = module->params[AnalogLFO::RATE_PARAM].getValue();
			std::string hzText;
			if (rate < 10.f)
				hzText = rack::string::f("%.1f Hz", rate);
			else
				hzText = rack::string::f("%d Hz", (int)std::round(rate));
			drawPillText(vg, font->handle, leftColX, topY + pillMicroSize,
			             hzText.c_str(), pillMicroSize,
			             NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM, hzFadeAlpha);
		}

		// Ratio label (clocked mode, left column top)
		// CLEAN-03 / D-05: gate on the widget-cached index so the pill fades out (alpha)
		// on disconnect rather than popping off when the atomic flips to -1.
		if (ratioFadeAlpha > 0.001f && cachedRatioIdx >= 0) {
			drawPillText(vg, font->handle, leftColX, topY + pillLabelSize,
			             AnalogLFO::RATIO_LABELS[cachedRatioIdx], pillLabelSize,
			             NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM, ratioFadeAlpha);
		}

		// Swing overlay (clocked mode, left column bottom)
		if (swingFadeAlpha > 0.001f) {
			int swingIdx = module->displaySwingIndex.load(std::memory_order_relaxed);
			if (swingIdx > 0 && swingIdx <= 5) {
				drawPillText(vg, font->handle, leftColX, bottomY,
				             AnalogLFO::SWING_OVERLAY_LABELS[swingIdx], pillSmallSize,
				             NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM, swingFadeAlpha);
			}
		}

		// === RIGHT COLUMN ===

		// SYNC badge (clocked mode, right column top)
		// Uses blinkPhase (2Hz) for ACQUIRING blink -- NOT breathePhase (0.2Hz)
		if (syncFadeAlpha > 0.001f) {
			float effectiveAlpha = syncFadeAlpha;
			if (clockState == AnalogLFO::ACQUIRING) {
				// ACQUIRING: 2Hz blink only, NO per-edge flash (D-02 -- untouched)
				float blink = 0.5f + 0.5f * std::sin(blinkPhase);
				effectiveAlpha *= blink;
				drawPillText(vg, font->handle, rightColX, topY + pillLabelSize,
				             "SYNC", pillLabelSize,
				             NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM, effectiveAlpha);
			} else {
				// LOCKED (and any non-ACQUIRING clocked path): per-edge white-hot flash (D-02)
				drawPillText(vg, font->handle, rightColX, topY + pillLabelSize,
				             "SYNC", pillLabelSize,
				             NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM, effectiveAlpha, flashIntensity);
			}
		}

		// BPM stack (clocked mode, right column bottom)
		if (bpmFadeAlpha > 0.001f) {
			drawBpmStack(vg, font->handle, bpmFadeAlpha, cachedRatioIdx, cachedPeriod);
		}
	}

	void drawPlaceholder(NVGcontext* vg) {
		// Static sine wave for module browser thumbnail
		nvgBeginPath(vg);
		for (int i = 0; i < 256; i++) {
			float p = (float)i / 256.f;
			float val = std::sin(2.f * (float)forge::kPi * p);
			float x = phaseToX(p);
			float y = valueToY(val);
			if (i == 0)
				nvgMoveTo(vg, x, y);
			else
				nvgLineTo(vg, x, y);
		}
		nvgStrokeColor(vg, nvgRGBAf(0.91f, 0.365f, 0.149f, 0.5f));
		nvgStrokeWidth(vg, 1.5f);
		nvgLineCap(vg, NVG_ROUND);
		nvgStroke(vg);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			NVGcontext* vg = args.vg;
			nvgSave(vg);

			// Expanded scissor for border glow (extends 4px beyond widget bounds)
			nvgScissor(vg, -4.f, -4.f, box.size.x + 8.f, box.size.y + 8.f);

			drawBackground(vg);
			drawBorder(vg);

			// Tight scissor for display content
			nvgScissor(vg, 0, 0, box.size.x, box.size.y);

			drawCornerBrackets(vg);

			if (module) {
				int readIdx = module->displayReadIdx.load(std::memory_order_acquire);
				const auto& buffer = module->displayBuffers[readIdx];
				float phase = module->displayPhase.load(std::memory_order_relaxed);
				// CLEAN-02 / D-07: the old `rate <= 0.001f` still-check is unreachable
				// (RATE_PARAM min is 0.01f; REQUIREMENTS forbids lowering it), so only the
				// reachable bypass dim remains.
				float dimFactor = module->isBypassed() ? 0.25f : 1.f;

				drawWaveformTrace(vg, buffer, dimFactor);
				drawPhaseDot(vg, buffer, phase, dimFactor);
				drawTextOverlays(vg);
				drawScanlines(vg);
			} else {
				drawPlaceholder(vg);
				drawScanlines(vg);
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
		minAngle = -0.75 * forge::kPi;
		maxAngle = 0.75 * forge::kPi;
		shadow->opacity = 0.0;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobHero.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobHero_bg.svg")));
		// bg art is larger than the foreground (which sizes box). Center it so the
		// knob body sits on its panel socket art instead of offset down-right.
		bg->box.pos = box.size.minus(bg->box.size).div(2);
	}
};

struct ForgeKnobSecondary : app::SvgKnob {
	widget::SvgWidget* bg;
	ForgeKnobSecondary() {
		minAngle = -0.75 * forge::kPi;
		maxAngle = 0.75 * forge::kPi;
		shadow->opacity = 0.0;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobSecondary.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeKnobSecondary_bg.svg")));
		// Center the larger bg art on the foreground-sized box (see ForgeKnobHero).
		bg->box.pos = box.size.minus(bg->box.size).div(2);
	}
};

struct ForgeTrimpot : app::SvgKnob {
	widget::SvgWidget* bg;
	ForgeTrimpot() {
		minAngle = -0.75 * forge::kPi;
		maxAngle = 0.75 * forge::kPi;
		shadow->opacity = 0.0;
		bg = new widget::SvgWidget;
		fb->addChildBelow(bg, tw);
		setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeTrimpot.svg")));
		bg->setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/ForgeTrimpot_bg.svg")));
		// Center the larger bg art on the foreground-sized box (see ForgeKnobHero).
		bg->box.pos = box.size.minus(bg->box.size).div(2);
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

		// Hex bolt screws (4 corners) — centered on the 18HP art bolt-holes,
		// pushed into the corners so the bottom-right bolt clears OUTPUT's ember rings (D-09).
		addChild(createWidgetCentered<ForgeHexBolt>(mm2px(Vec(2.60f, 2.60f))));
		addChild(createWidgetCentered<ForgeHexBolt>(mm2px(Vec(88.84f, 2.60f))));
		addChild(createWidgetCentered<ForgeHexBolt>(mm2px(Vec(2.60f, 125.90f))));
		addChild(createWidgetCentered<ForgeHexBolt>(mm2px(Vec(88.84f, 125.90f))));

		// Waveform display (repositioned for 18HP Forge Noir layout)
		{
			WaveformDisplay* display = new WaveformDisplay();
			display->module = module;
			display->box.pos = mm2px(Vec(5.00f, 19.00f));
			display->box.size = mm2px(Vec(81.44f, 26.00f));
			addChild(display);
		}

		// Main knobs
		addParam(createParamCentered<ForgeKnobHero>(mm2px(Vec(45.72f, 61.00f)),
		         module, AnalogLFO::MORPH_PARAM));
		addParam(createParamCentered<ForgeKnobSecondary>(mm2px(Vec(18.00f, 87.00f)),
		         module, AnalogLFO::CHARACTER_PARAM));
		addParam(createParamCentered<ForgeKnobSecondary>(mm2px(Vec(36.24f, 87.00f)),
		         module, AnalogLFO::DRIFT_PARAM));
		addParam(createParamCentered<ForgeKnobSecondary>(mm2px(Vec(54.48f, 87.00f)),
		         module, AnalogLFO::RATE_PARAM));
		addParam(createParamCentered<ForgeKnobSecondary>(mm2px(Vec(72.72f, 87.00f)),
		         module, AnalogLFO::PHASE_OFFSET_PARAM));

		// CV trimpots
		addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(7.70f, 108.50f)),
		         module, AnalogLFO::MORPH_ATTEN_PARAM));
		addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(18.56f, 108.50f)),
		         module, AnalogLFO::CHARACTER_ATTEN_PARAM));
		addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(29.43f, 108.50f)),
		         module, AnalogLFO::DRIFT_ATTEN_PARAM));
		addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(40.29f, 108.50f)),
		         module, AnalogLFO::FM_ATTEN_PARAM));
		addParam(createParamCentered<ForgeTrimpot>(mm2px(Vec(51.15f, 108.50f)),
		         module, AnalogLFO::PHASE_OFFSET_ATTEN_PARAM));

		// CV input jacks
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(7.70f, 119.50f)),
		         module, AnalogLFO::MORPH_CV_INPUT));
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(18.56f, 119.50f)),
		         module, AnalogLFO::CHARACTER_CV_INPUT));
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(29.43f, 119.50f)),
		         module, AnalogLFO::DRIFT_CV_INPUT));
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(40.29f, 119.50f)),
		         module, AnalogLFO::FM_INPUT));
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(51.15f, 119.50f)),
		         module, AnalogLFO::PHASE_OFFSET_CV_INPUT));

		// Bottom I/O row
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(62.01f, 119.50f)),
		         module, AnalogLFO::CLK_INPUT));
		addInput(createInputCentered<ForgeJackInput>(mm2px(Vec(72.88f, 119.50f)),
		         module, AnalogLFO::RESET_INPUT));
		addOutput(createOutputCentered<ForgeJackOutput>(mm2px(Vec(83.74f, 119.50f)),
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
